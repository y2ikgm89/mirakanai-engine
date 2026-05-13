// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/linux_file_watcher.hpp"

#include <algorithm>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

#if defined(__linux__)
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <unordered_map>
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
        throw std::invalid_argument("linux file watcher path prefix must not contain control characters");
    }
    if (is_absolute_like(prefix)) {
        throw std::invalid_argument("linux file watcher path prefix must be relative");
    }
    if (has_parent_segment(prefix)) {
        throw std::invalid_argument("linux file watcher path prefix must not contain '..'");
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

[[nodiscard]] bool event_less(const FileWatchEvent& left, const FileWatchEvent& right) noexcept {
    if (left.path != right.path) {
        return left.path < right.path;
    }
    return static_cast<int>(left.kind) < static_cast<int>(right.kind);
}

#if defined(__linux__)
[[nodiscard]] std::string errno_message(std::string_view prefix, int error_code = errno) {
    std::string message(prefix);
    message.append(" failed with errno ");
    message.append(std::to_string(error_code));
    message.append(": ");
    message.append(std::strerror(error_code));
    return message;
}

[[nodiscard]] std::uint32_t watch_mask() noexcept {
    return IN_CREATE | IN_MOVED_TO | IN_DELETE | IN_MOVED_FROM | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE |
           IN_DELETE_SELF | IN_MOVE_SELF | IN_ONLYDIR;
}

[[nodiscard]] FileWatchEventKind map_inotify_mask(std::uint32_t mask) noexcept {
    if ((mask & (IN_DELETE | IN_MOVED_FROM | IN_DELETE_SELF)) != 0U) {
        return FileWatchEventKind::removed;
    }
    if ((mask & (IN_CREATE | IN_MOVED_TO)) != 0U) {
        return FileWatchEventKind::added;
    }
    if ((mask & (IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE | IN_MOVE_SELF)) != 0U) {
        return FileWatchEventKind::modified;
    }
    return FileWatchEventKind::unknown;
}

class UniqueFd {
  public:
    UniqueFd() = default;
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd_(other.release()) {}

    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    ~UniqueFd() {
        reset();
    }

    [[nodiscard]] int get() const noexcept {
        return fd_;
    }

    [[nodiscard]] bool valid() const noexcept {
        return fd_ >= 0;
    }

    [[nodiscard]] int release() noexcept {
        const auto result = fd_;
        fd_ = -1;
        return result;
    }

    void reset(int fd = -1) noexcept {
        if (valid()) {
            ::close(fd_);
        }
        fd_ = fd;
    }

  private:
    int fd_{-1};
};
#endif

} // namespace

#if defined(__linux__)
struct LinuxFileWatcher::Impl {
    std::filesystem::path directory_path;
    std::string path_prefix;
    bool recursive{true};
    bool active{false};
    bool overflowed_since_poll{false};
    std::string diagnostic;
    std::vector<char> buffer;
    UniqueFd fd;
    std::unordered_map<int, std::filesystem::path> directory_by_watch;
    std::unordered_map<int, std::string> output_prefix_by_watch;

    explicit Impl(LinuxFileWatcherDesc desc)
        : directory_path(std::filesystem::absolute(desc.directory)), path_prefix(normalize_prefix(desc.path_prefix)),
          recursive(desc.recursive) {
        if (directory_path.empty()) {
            throw std::invalid_argument("linux file watcher directory must not be empty");
        }
        if (!std::filesystem::is_directory(directory_path)) {
            throw std::invalid_argument("linux file watcher directory must exist");
        }

        constexpr std::uint32_t default_buffer_size_bytes = 65536;
        const auto buffer_size = desc.buffer_size_bytes == 0 ? default_buffer_size_bytes : desc.buffer_size_bytes;
        buffer.resize(buffer_size);

        fd.reset(::inotify_init1(IN_NONBLOCK | IN_CLOEXEC));
        if (!fd.valid()) {
            diagnostic = errno_message("inotify_init1");
            return;
        }

        if (!add_directory_tree(directory_path, path_prefix, true)) {
            return;
        }
        active = true;
    }

    [[nodiscard]] bool add_watch(const std::filesystem::path& directory, const std::string& output_prefix,
                                 bool required) {
        const auto native_path = directory.string();
        const auto watch = ::inotify_add_watch(fd.get(), native_path.c_str(), watch_mask());
        if (watch < 0) {
            if (required) {
                diagnostic = errno_message("inotify_add_watch");
                active = false;
            }
            return false;
        }
        directory_by_watch[watch] = directory;
        output_prefix_by_watch[watch] = output_prefix;
        return true;
    }

    [[nodiscard]] bool add_directory_tree(const std::filesystem::path& directory, const std::string& output_prefix,
                                          bool required) {
        if (!add_watch(directory, output_prefix, required)) {
            return false;
        }
        if (!recursive) {
            return true;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (!entry.is_directory()) {
                continue;
            }
            const auto relative = std::filesystem::relative(entry.path(), directory_path).generic_string();
            (void)add_watch(entry.path(), join_event_path(path_prefix, relative), false);
        }
        return true;
    }

    void add_dynamic_directory_watch(int watch, std::string_view name) {
        if (!recursive || name.empty()) {
            return;
        }

        const auto directory = directory_by_watch.find(watch);
        const auto output_prefix = output_prefix_by_watch.find(watch);
        if (directory == directory_by_watch.end() || output_prefix == output_prefix_by_watch.end()) {
            return;
        }

        const auto child_directory = directory->second / std::filesystem::path{std::string(name)};
        if (!std::filesystem::is_directory(child_directory)) {
            return;
        }
        (void)add_directory_tree(child_directory, join_event_path(output_prefix->second, name), false);
    }

    [[nodiscard]] std::vector<FileWatchEvent> parse_events(const char* data, std::size_t byte_count) {
        std::vector<FileWatchEvent> events;
        std::span<const char> data_view{data, byte_count};
        std::size_t offset = 0;
        while (offset + sizeof(inotify_event) <= data_view.size()) {
            const auto* record = reinterpret_cast<const inotify_event*>(data_view.subspan(offset).data());
            std::size_t name_length = 0;
            while (name_length < record->len && record->name[name_length] != '\0') {
                ++name_length;
            }
            const std::string_view name(record->len > 0 ? record->name : "", name_length);

            if ((record->mask & IN_Q_OVERFLOW) != 0U) {
                overflowed_since_poll = true;
            } else if ((record->mask & IN_IGNORED) == 0U) {
                if ((record->mask & IN_ISDIR) != 0U && (record->mask & (IN_CREATE | IN_MOVED_TO)) != 0U) {
                    add_dynamic_directory_watch(record->wd, name);
                }

                const auto output_prefix = output_prefix_by_watch.find(record->wd);
                const auto kind = map_inotify_mask(record->mask);
                if (output_prefix != output_prefix_by_watch.end() && !name.empty() &&
                    kind != FileWatchEventKind::unknown) {
                    events.push_back(make_event(kind, join_event_path(output_prefix->second, name)));
                }
            }

            offset += sizeof(inotify_event) + record->len;
        }
        return events;
    }

    [[nodiscard]] LinuxFileWatcherPollResult poll() {
        LinuxFileWatcherPollResult result;
        result.active = active;
        result.diagnostic = diagnostic;
        if (!active || !fd.valid()) {
            return result;
        }

        pollfd descriptor{};
        descriptor.fd = fd.get();
        descriptor.events = POLLIN;

        while (true) {
            const auto ready = ::poll(&descriptor, 1, 0);
            if (ready < 0) {
                if (errno == EINTR) {
                    continue;
                }
                diagnostic = errno_message("poll");
                active = false;
                result.active = false;
                result.diagnostic = diagnostic;
                return result;
            }
            if (ready == 0 || (descriptor.revents & POLLIN) == 0) {
                break;
            }

            const auto read_bytes = ::read(fd.get(), buffer.data(), buffer.size());
            if (read_bytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                    continue;
                }
                diagnostic = errno_message("read");
                active = false;
                result.active = false;
                result.diagnostic = diagnostic;
                return result;
            }
            if (read_bytes == 0) {
                break;
            }

            auto events = parse_events(buffer.data(), static_cast<std::size_t>(read_bytes));
            result.events.insert(result.events.end(), events.begin(), events.end());
        }

        std::ranges::sort(result.events, event_less);
        result.overflowed = overflowed_since_poll;
        overflowed_since_poll = false;
        result.active = active;
        result.diagnostic = diagnostic;
        return result;
    }
};
#else
struct LinuxFileWatcher::Impl {
    std::filesystem::path directory_path;
    std::string diagnostic{"linux inotify file watcher is only available on Linux"};

    explicit Impl(const LinuxFileWatcherDesc& desc) : directory_path(std::filesystem::absolute(desc.directory)) {
        if (directory_path.empty()) {
            throw std::invalid_argument("linux file watcher directory must not be empty");
        }
        (void)normalize_prefix(desc.path_prefix);
    }

    [[nodiscard]] LinuxFileWatcherPollResult poll() const {
        return LinuxFileWatcherPollResult{
            .active = false,
            .overflowed = false,
            .diagnostic = diagnostic,
            .events = {},
        };
    }
};
#endif

LinuxFileWatcher::LinuxFileWatcher(LinuxFileWatcherDesc desc) : impl_(std::make_unique<Impl>(std::move(desc))) {}

LinuxFileWatcher::~LinuxFileWatcher() = default;

LinuxFileWatcher::LinuxFileWatcher(LinuxFileWatcher&&) noexcept = default;

LinuxFileWatcher& LinuxFileWatcher::operator=(LinuxFileWatcher&&) noexcept = default;

LinuxFileWatcherPollResult LinuxFileWatcher::poll() {
    return impl_->poll();
}

bool LinuxFileWatcher::active() const noexcept {
#if defined(__linux__)
    return impl_ != nullptr && impl_->active;
#else
    return false;
#endif
}

const std::filesystem::path& LinuxFileWatcher::directory() const noexcept {
    return impl_->directory_path;
}

FileWatchBackendKind LinuxFileWatcher::backend_kind() noexcept {
    return FileWatchBackendKind::native;
}

FileWatchNativeBackendKind LinuxFileWatcher::native_backend_kind() noexcept {
    return FileWatchNativeBackendKind::linux_inotify;
}

} // namespace mirakana
