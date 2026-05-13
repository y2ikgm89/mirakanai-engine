// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/macos_file_watcher.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
#define MK_PLATFORM_HAS_MACOS_FSEVENTS 1
#else
#define MK_PLATFORM_HAS_MACOS_FSEVENTS 0
#endif

#if MK_PLATFORM_HAS_MACOS_FSEVENTS
#include <CoreServices/CoreServices.h>

#include <condition_variable>
#include <mutex>
#include <thread>
#endif

namespace mirakana {
namespace {

[[nodiscard]] bool has_control_character(std::string_view value) noexcept {
    return value.find('\n') != std::string_view::npos || value.find('\r') != std::string_view::npos ||
           value.find('\0') != std::string_view::npos;
}

[[nodiscard]] bool is_absolute_like(std::string_view value) noexcept {
    return value.starts_with('/') || value.starts_with('\\') ||
           (value.size() >= 2 && value[1] == ':' &&
            ((value[0] >= 'A' && value[0] <= 'Z') || (value[0] >= 'a' && value[0] <= 'z')));
}

[[nodiscard]] bool has_parent_segment(std::string_view value) noexcept {
    std::size_t begin = 0;
    while (begin <= value.size()) {
        const auto separator = value.find_first_of("/\\", begin);
        const auto end = separator == std::string_view::npos ? value.size() : separator;
        if (value.substr(begin, end - begin) == "..") {
            return true;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        begin = separator + 1;
    }
    return false;
}

[[nodiscard]] std::string normalize_prefix(std::string_view prefix) {
    if (has_control_character(prefix)) {
        throw std::invalid_argument("macos file watcher path prefix must not contain control characters");
    }
    if (is_absolute_like(prefix)) {
        throw std::invalid_argument("macos file watcher path prefix must be relative");
    }
    if (has_parent_segment(prefix)) {
        throw std::invalid_argument("macos file watcher path prefix must not contain '..'");
    }

    std::string result(prefix);
    std::ranges::replace(result, '\\', '/');
    while (!result.empty() && result.back() == '/') {
        result.pop_back();
    }
    return result;
}

[[nodiscard]] std::string join_event_path(std::string_view prefix, std::string_view relative_path) {
    if (prefix.empty()) {
        return std::string(relative_path);
    }
    if (relative_path.empty()) {
        return std::string(prefix);
    }
    std::string result(prefix);
    result.push_back('/');
    result.append(relative_path);
    return result;
}

[[nodiscard]] FileWatchEvent make_event(FileWatchEventKind kind, std::string path) {
    FileWatchEvent event;
    event.kind = kind;
    event.path = std::move(path);
    return event;
}

struct MacOSFileSnapshot {
    std::uint64_t revision{0};
    std::uint64_t size_bytes{0};
};

[[nodiscard]] std::uint64_t make_revision(std::filesystem::file_time_type time) noexcept {
    return static_cast<std::uint64_t>(time.time_since_epoch().count());
}

[[nodiscard]] std::filesystem::path normalize_watch_path(const std::filesystem::path& path) {
    std::error_code error;
    auto normalized = std::filesystem::weakly_canonical(std::filesystem::absolute(path), error);
    if (!error) {
        return normalized;
    }
    return std::filesystem::absolute(path);
}

[[nodiscard]] bool event_less(const FileWatchEvent& left, const FileWatchEvent& right) noexcept {
    if (left.path != right.path) {
        return left.path < right.path;
    }
    return static_cast<int>(left.kind) < static_cast<int>(right.kind);
}

#if MK_PLATFORM_HAS_MACOS_FSEVENTS
[[nodiscard]] bool is_dropped_event(FSEventStreamEventFlags flags) noexcept {
    return (flags & (kFSEventStreamEventFlagMustScanSubDirs | kFSEventStreamEventFlagKernelDropped |
                     kFSEventStreamEventFlagUserDropped)) != 0U;
}

[[nodiscard]] FileWatchEventKind map_fsevent_flags(FSEventStreamEventFlags flags) noexcept {
    if ((flags & kFSEventStreamEventFlagItemRemoved) != 0U) {
        return FileWatchEventKind::removed;
    }
    if ((flags & kFSEventStreamEventFlagItemCreated) != 0U) {
        return FileWatchEventKind::added;
    }
    if ((flags & (kFSEventStreamEventFlagItemModified | kFSEventStreamEventFlagItemRenamed |
                  kFSEventStreamEventFlagItemInodeMetaMod | kFSEventStreamEventFlagItemFinderInfoMod |
                  kFSEventStreamEventFlagItemChangeOwner | kFSEventStreamEventFlagItemXattrMod)) != 0U) {
        return FileWatchEventKind::modified;
    }
    return FileWatchEventKind::unknown;
}
#endif

} // namespace

#if MK_PLATFORM_HAS_MACOS_FSEVENTS
struct MacOSFileWatcher::Impl {
    std::filesystem::path directory_path;
    std::string path_prefix;
    bool recursive{true};
    double latency_seconds{0.05};
    bool active{false};
    bool overflowed_since_poll{false};
    bool ready{false};
    bool stop_requested{false};
    std::string diagnostic;
    std::vector<FileWatchEvent> queued_events;
    std::unordered_map<std::string, MacOSFileSnapshot> snapshots_by_path;
    std::thread worker;
    mutable std::mutex mutex;
    std::condition_variable ready_cv;
    CFRunLoopRef run_loop{nullptr};
    FSEventStreamRef event_stream{nullptr};

    explicit Impl(MacOSFileWatcherDesc desc)
        : directory_path(normalize_watch_path(desc.directory)), path_prefix(normalize_prefix(desc.path_prefix)),
          recursive(desc.recursive), latency_seconds(desc.latency_seconds > 0.0 ? desc.latency_seconds : 0.05) {
        if (directory_path.empty()) {
            throw std::invalid_argument("macos file watcher directory must not be empty");
        }
        if (!std::filesystem::is_directory(directory_path)) {
            throw std::invalid_argument("macos file watcher directory must exist");
        }

        snapshots_by_path = scan_snapshots();
        worker = std::thread(&Impl::run, this);
        std::unique_lock lock(mutex);
        ready_cv.wait(lock, [&] { return ready; });
    }

    ~Impl() {
        stop();
    }

    void stop() noexcept {
        CFRunLoopRef loop = nullptr;
        {
            std::lock_guard lock(mutex);
            stop_requested = true;
            loop = run_loop;
        }
        if (loop != nullptr) {
            CFRunLoopStop(loop);
        }
        if (worker.joinable()) {
            worker.join();
        }
    }

    void mark_ready(bool is_active) {
        {
            std::lock_guard lock(mutex);
            active = is_active;
            ready = true;
        }
        ready_cv.notify_all();
    }

    void set_event_stream(FSEventStreamRef stream) noexcept {
        std::lock_guard lock(mutex);
        event_stream = stream;
    }

    [[nodiscard]] FSEventStreamRef current_event_stream() const noexcept {
        std::lock_guard lock(mutex);
        return event_stream;
    }

    void set_failure(std::string message) {
        {
            std::lock_guard lock(mutex);
            diagnostic = std::move(message);
            active = false;
            ready = true;
        }
        ready_cv.notify_all();
    }

    [[nodiscard]] bool should_stop() const noexcept {
        std::lock_guard lock(mutex);
        return stop_requested;
    }

    [[nodiscard]] std::string make_output_path(std::string_view absolute_event_path) const {
        std::filesystem::path event_path{std::string(absolute_event_path)};
        if (event_path.empty()) {
            return {};
        }

        event_path = normalize_watch_path(event_path);
        auto relative = event_path.lexically_relative(directory_path).generic_string();
        if (relative == ".") {
            relative.clear();
        }
        if (relative.starts_with("../") || relative == ".." || relative.empty()) {
            return relative.empty() ? std::string(path_prefix) : std::string{};
        }
        if (!recursive && relative.find('/') != std::string::npos) {
            return {};
        }
        return join_event_path(path_prefix, relative);
    }

    void add_snapshot(std::unordered_map<std::string, MacOSFileSnapshot>& snapshots,
                      const std::filesystem::path& path) const {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error) || error) {
            return;
        }

        const auto output_path = make_output_path(path.generic_string());
        if (output_path.empty()) {
            return;
        }

        const auto write_time = std::filesystem::last_write_time(path, error);
        if (error) {
            return;
        }
        const auto size_bytes = std::filesystem::file_size(path, error);
        if (error) {
            return;
        }

        snapshots.emplace(output_path, MacOSFileSnapshot{
                                           .revision = make_revision(write_time),
                                           .size_bytes = static_cast<std::uint64_t>(size_bytes),
                                       });
    }

    [[nodiscard]] std::unordered_map<std::string, MacOSFileSnapshot> scan_snapshots() const {
        std::unordered_map<std::string, MacOSFileSnapshot> snapshots;
        constexpr auto options = std::filesystem::directory_options::skip_permission_denied;
        std::error_code error;

        if (recursive) {
            for (std::filesystem::recursive_directory_iterator entry{directory_path, options, error}, end;
                 !error && entry != end; entry.increment(error)) {
                add_snapshot(snapshots, entry->path());
            }
            return snapshots;
        }

        for (std::filesystem::directory_iterator entry{directory_path, options, error}, end; !error && entry != end;
             entry.increment(error)) {
            add_snapshot(snapshots, entry->path());
        }
        return snapshots;
    }

    [[nodiscard]] std::vector<FileWatchEvent> reconcile_snapshot_events() {
        std::vector<FileWatchEvent> events;
        auto current = scan_snapshots();

        for (const auto& [path, snapshot] : current) {
            const auto previous = snapshots_by_path.find(path);
            if (previous == snapshots_by_path.end()) {
                events.push_back(make_event(FileWatchEventKind::added, path));
            } else if (previous->second.revision != snapshot.revision ||
                       previous->second.size_bytes != snapshot.size_bytes) {
                events.push_back(make_event(FileWatchEventKind::modified, path));
            }
        }

        for (const auto& [path, snapshot] : snapshots_by_path) {
            (void)snapshot;
            if (current.find(path) == current.end()) {
                events.push_back(make_event(FileWatchEventKind::removed, path));
            }
        }

        snapshots_by_path = std::move(current);
        return events;
    }

    void push_events(std::vector<FileWatchEvent> events, bool overflowed) {
        std::lock_guard lock(mutex);
        queued_events.insert(queued_events.end(), events.begin(), events.end());
        overflowed_since_poll = overflowed_since_poll || overflowed;
    }

    static void callback(ConstFSEventStreamRef, void* context, std::size_t count, void* event_paths,
                         const FSEventStreamEventFlags flags[], const FSEventStreamEventId[]) {
        auto* self = static_cast<Impl*>(context);
        if (self == nullptr) {
            return;
        }

        bool overflowed = false;
        std::vector<FileWatchEvent> events;
        auto** paths = static_cast<char**>(event_paths);
        for (std::size_t index = 0; index < count; ++index) {
            overflowed = overflowed || is_dropped_event(flags[index]);

            const auto kind = map_fsevent_flags(flags[index]);
            if (kind == FileWatchEventKind::unknown || paths == nullptr || paths[index] == nullptr) {
                continue;
            }

            auto output_path = self->make_output_path(paths[index]);
            if (!output_path.empty()) {
                events.push_back(make_event(kind, std::move(output_path)));
            }
        }

        std::ranges::sort(events, event_less);
        self->push_events(std::move(events), overflowed);
    }

    void run() {
        const auto native_path = directory_path.string();
        CFStringRef cf_path = CFStringCreateWithCString(nullptr, native_path.c_str(), kCFStringEncodingUTF8);
        if (cf_path == nullptr) {
            set_failure("CFStringCreateWithCString failed");
            return;
        }

        const void* values[] = {cf_path};
        CFArrayRef paths = CFArrayCreate(nullptr, values, 1, &kCFTypeArrayCallBacks);
        if (paths == nullptr) {
            CFRelease(cf_path);
            set_failure("CFArrayCreate failed");
            return;
        }

        FSEventStreamContext context{};
        context.info = this;
        constexpr FSEventStreamCreateFlags flags =
            kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagWatchRoot;
        FSEventStreamRef stream = FSEventStreamCreate(nullptr, &Impl::callback, &context, paths,
                                                      kFSEventStreamEventIdSinceNow, latency_seconds, flags);
        if (stream == nullptr) {
            CFRelease(paths);
            CFRelease(cf_path);
            set_failure("FSEventStreamCreate failed");
            return;
        }

        CFRunLoopRef current_loop = CFRunLoopGetCurrent();
        CFRetain(current_loop);
        {
            std::lock_guard lock(mutex);
            run_loop = current_loop;
        }

        FSEventStreamScheduleWithRunLoop(stream, current_loop, kCFRunLoopDefaultMode);
        set_event_stream(stream);
        if (!FSEventStreamStart(stream)) {
            set_event_stream(nullptr);
            FSEventStreamInvalidate(stream);
            FSEventStreamRelease(stream);
            {
                std::lock_guard lock(mutex);
                run_loop = nullptr;
            }
            CFRelease(current_loop);
            CFRelease(paths);
            CFRelease(cf_path);
            set_failure("FSEventStreamStart failed");
            return;
        }

        mark_ready(true);
        CFRunLoopRun();

        FSEventStreamStop(stream);
        FSEventStreamInvalidate(stream);
        FSEventStreamRelease(stream);
        CFRelease(current_loop);
        CFRelease(paths);
        CFRelease(cf_path);

        {
            std::lock_guard lock(mutex);
            run_loop = nullptr;
            event_stream = nullptr;
            active = false;
        }
    }

    [[nodiscard]] MacOSFileWatcherPollResult poll() {
        if (const auto stream = current_event_stream(); stream != nullptr) {
            FSEventStreamFlushSync(stream);
        }

        auto snapshot_events = reconcile_snapshot_events();
        std::lock_guard lock(mutex);
        MacOSFileWatcherPollResult result;
        result.active = active;
        result.overflowed = overflowed_since_poll;
        result.diagnostic = diagnostic;
        result.events = std::move(queued_events);
        queued_events.clear();
        overflowed_since_poll = false;
        result.events.insert(result.events.end(), snapshot_events.begin(), snapshot_events.end());
        std::ranges::sort(result.events, event_less);
        const auto duplicate_begin = std::unique(result.events.begin(), result.events.end(),
                                                 [](const FileWatchEvent& left, const FileWatchEvent& right) noexcept {
                                                     return left.path == right.path && left.kind == right.kind;
                                                 });
        result.events.erase(duplicate_begin, result.events.end());
        return result;
    }

    [[nodiscard]] bool is_active() const noexcept {
        std::lock_guard lock(mutex);
        return active;
    }
};
#else
struct MacOSFileWatcher::Impl {
    std::filesystem::path directory_path;
    std::string diagnostic{"macos FSEvents file watcher is only available on macOS"};

    explicit Impl(const MacOSFileWatcherDesc& desc) : directory_path(normalize_watch_path(desc.directory)) {
        if (directory_path.empty()) {
            throw std::invalid_argument("macos file watcher directory must not be empty");
        }
        (void)normalize_prefix(desc.path_prefix);
    }

    [[nodiscard]] MacOSFileWatcherPollResult poll() const {
        return MacOSFileWatcherPollResult{
            .active = false,
            .overflowed = false,
            .diagnostic = diagnostic,
            .events = {},
        };
    }
};
#endif

MacOSFileWatcher::MacOSFileWatcher(MacOSFileWatcherDesc desc) : impl_(std::make_unique<Impl>(std::move(desc))) {}

MacOSFileWatcher::~MacOSFileWatcher() = default;

MacOSFileWatcher::MacOSFileWatcher(MacOSFileWatcher&&) noexcept = default;

MacOSFileWatcher& MacOSFileWatcher::operator=(MacOSFileWatcher&&) noexcept = default;

MacOSFileWatcherPollResult MacOSFileWatcher::poll() {
    return impl_->poll();
}

bool MacOSFileWatcher::active() const noexcept {
#if MK_PLATFORM_HAS_MACOS_FSEVENTS
    return impl_ != nullptr && impl_->is_active();
#else
    return false;
#endif
}

const std::filesystem::path& MacOSFileWatcher::directory() const noexcept {
    return impl_->directory_path;
}

FileWatchBackendKind MacOSFileWatcher::backend_kind() noexcept {
    return FileWatchBackendKind::native;
}

FileWatchNativeBackendKind MacOSFileWatcher::native_backend_kind() noexcept {
    return FileWatchNativeBackendKind::macos_fsevents;
}

} // namespace mirakana
