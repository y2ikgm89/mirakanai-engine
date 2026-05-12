// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/windows_file_watcher.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

class UniqueHandle {
  public:
    UniqueHandle() = default;
    explicit UniqueHandle(HANDLE handle) noexcept : handle_(handle) {}

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept : handle_(other.release()) {}

    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    ~UniqueHandle() {
        reset();
    }

    [[nodiscard]] HANDLE get() const noexcept {
        return handle_;
    }

    [[nodiscard]] bool valid() const noexcept {
        return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
    }

    [[nodiscard]] HANDLE release() noexcept {
        auto* const result = handle_;
        handle_ = nullptr;
        return result;
    }

    void reset(HANDLE handle = nullptr) noexcept {
        if (valid()) {
            CloseHandle(handle_);
        }
        handle_ = handle;
    }

  private:
    HANDLE handle_{nullptr};
};

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
        throw std::invalid_argument("windows file watcher path prefix must not contain control characters");
    }
    if (is_absolute_like(prefix)) {
        throw std::invalid_argument("windows file watcher path prefix must be relative");
    }
    if (has_parent_segment(prefix)) {
        throw std::invalid_argument("windows file watcher path prefix must not contain '..'");
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

[[nodiscard]] std::string last_error_message(std::string_view prefix, DWORD error = GetLastError()) {
    LPSTR buffer = nullptr;
    const auto chars = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

    std::string message(prefix);
    message.append(" failed with error ");
    message.append(std::to_string(error));
    if (chars != 0 && buffer != nullptr) {
        message.append(": ");
        message.append(buffer, chars);
        LocalFree(buffer);
    }
    return message;
}

[[nodiscard]] std::string wide_to_utf8(std::wstring_view text) {
    if (text.empty()) {
        return {};
    }

    const auto required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                                              nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        std::string fallback;
        fallback.reserve(text.size());
        for (const auto character : text) {
            fallback.push_back(static_cast<char>(character));
        }
        return fallback;
    }

    std::string result(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(),
                        required, nullptr, nullptr);
    return result;
}

[[nodiscard]] FileWatchEventKind map_file_action(DWORD action) noexcept {
    switch (action) {
    case FILE_ACTION_ADDED:
    case FILE_ACTION_RENAMED_NEW_NAME:
        return FileWatchEventKind::added;
    case FILE_ACTION_REMOVED:
    case FILE_ACTION_RENAMED_OLD_NAME:
        return FileWatchEventKind::removed;
    case FILE_ACTION_MODIFIED:
        return FileWatchEventKind::modified;
    default:
        return FileWatchEventKind::unknown;
    }
}

[[nodiscard]] FileWatchEvent make_event(FileWatchEventKind kind, std::string path) {
    FileWatchEvent event;
    event.kind = kind;
    event.path = std::move(path);
    return event;
}

} // namespace

struct WindowsFileWatcher::Impl {
    std::filesystem::path directory_path;
    std::string path_prefix;
    bool recursive{true};
    bool active{false};
    std::vector<std::uint32_t> buffer_words;
    UniqueHandle directory;
    std::thread worker;
    mutable std::mutex mutex;
    std::condition_variable ready_cv;
    std::vector<FileWatchEvent> queued_events;
    std::string diagnostic;
    bool stop_requested{false};
    bool read_loop_ready{false};
    bool overflowed_since_poll{false};

    explicit Impl(const WindowsFileWatcherDesc& desc)
        : directory_path(std::filesystem::absolute(desc.directory)), path_prefix(normalize_prefix(desc.path_prefix)),
          recursive(desc.recursive) {
        if (directory_path.empty()) {
            throw std::invalid_argument("windows file watcher directory must not be empty");
        }
        if (!std::filesystem::is_directory(directory_path)) {
            throw std::invalid_argument("windows file watcher directory must exist");
        }

        constexpr std::uint32_t default_buffer_size_bytes = 65536;
        const auto buffer_size = desc.buffer_size_bytes == 0 ? default_buffer_size_bytes : desc.buffer_size_bytes;
        buffer_words.resize((buffer_size + static_cast<std::uint32_t>(sizeof(std::uint32_t)) - 1U) /
                            static_cast<std::uint32_t>(sizeof(std::uint32_t)));

        auto wide_directory = directory_path.wstring();
        HANDLE raw_directory = CreateFileW(wide_directory.c_str(), FILE_LIST_DIRECTORY,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                           OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (raw_directory == INVALID_HANDLE_VALUE) {
            diagnostic = last_error_message("CreateFileW");
            return;
        }
        directory.reset(raw_directory);

        active = true;
        worker = std::thread(&Impl::run, this);
        std::unique_lock lock(mutex);
        ready_cv.wait(lock, [&] { return read_loop_ready || !active || !diagnostic.empty(); });
    }

    ~Impl() {
        stop();
    }

    [[nodiscard]] std::byte* buffer_data() noexcept {
        return reinterpret_cast<std::byte*>(buffer_words.data());
    }

    [[nodiscard]] DWORD buffer_size() const noexcept {
        return static_cast<DWORD>(buffer_words.size() * sizeof(std::uint32_t));
    }

    void stop() noexcept {
        {
            std::lock_guard lock(mutex);
            stop_requested = true;
        }
        ready_cv.notify_all();
        if (directory.valid()) {
            CancelIoEx(directory.get(), nullptr);
        }
        if (worker.joinable()) {
            worker.join();
        }
    }

    [[nodiscard]] bool should_stop() const noexcept {
        std::lock_guard lock(mutex);
        return stop_requested;
    }

    void set_failure(std::string message) {
        std::lock_guard lock(mutex);
        if (!stop_requested) {
            diagnostic = std::move(message);
            active = false;
        }
    }

    void push_events(std::vector<FileWatchEvent> events) {
        std::lock_guard lock(mutex);
        queued_events.insert(queued_events.end(), events.begin(), events.end());
    }

    void mark_overflowed() {
        std::lock_guard lock(mutex);
        overflowed_since_poll = true;
    }

    [[nodiscard]] std::vector<FileWatchEvent> parse_events(const std::byte* buffer, DWORD bytes_returned) const {
        std::vector<FileWatchEvent> events;
        std::span<const std::byte> buffer_view{buffer, bytes_returned};
        DWORD offset = 0;
        while (static_cast<std::size_t>(offset) < buffer_view.size()) {
            const auto* record = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(
                buffer_view.subspan(static_cast<std::size_t>(offset)).data());

            const std::wstring_view wide_name(record->FileName, record->FileNameLength / sizeof(wchar_t));
            auto relative_path = wide_to_utf8(wide_name);
            std::ranges::replace(relative_path, '\\', '/');
            const auto kind = map_file_action(record->Action);
            if (!relative_path.empty() && kind != FileWatchEventKind::unknown) {
                events.push_back(make_event(kind, join_event_path(path_prefix, relative_path)));
            }

            if (record->NextEntryOffset == 0) {
                break;
            }
            offset += record->NextEntryOffset;
        }
        return events;
    }

    void run() {
        constexpr DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                 FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE;

        {
            std::lock_guard lock(mutex);
            read_loop_ready = true;
        }
        ready_cv.notify_all();

        while (!should_stop()) {
            DWORD bytes_returned = 0;
            const auto ok = ReadDirectoryChangesW(directory.get(), buffer_data(), buffer_size(),
                                                  recursive ? TRUE : FALSE, filter, &bytes_returned, nullptr, nullptr);
            if (ok == 0) {
                if (should_stop()) {
                    return;
                }
                const auto error = GetLastError();
                if (error == ERROR_OPERATION_ABORTED) {
                    return;
                }
                set_failure(last_error_message("ReadDirectoryChangesW", error));
                return;
            }

            if (bytes_returned == 0) {
                mark_overflowed();
                continue;
            }
            push_events(parse_events(buffer_data(), bytes_returned));
        }
    }

    [[nodiscard]] WindowsFileWatcherPollResult poll() {
        std::lock_guard lock(mutex);
        WindowsFileWatcherPollResult result;
        result.active = active;
        result.diagnostic = diagnostic;
        result.overflowed = overflowed_since_poll;
        result.events = std::move(queued_events);
        queued_events.clear();
        overflowed_since_poll = false;
        return result;
    }

    [[nodiscard]] bool is_active() const noexcept {
        std::lock_guard lock(mutex);
        return active;
    }
};

WindowsFileWatcher::WindowsFileWatcher(WindowsFileWatcherDesc desc) : impl_(std::make_unique<Impl>(std::move(desc))) {}

WindowsFileWatcher::~WindowsFileWatcher() = default;

WindowsFileWatcher::WindowsFileWatcher(WindowsFileWatcher&&) noexcept = default;

WindowsFileWatcher& WindowsFileWatcher::operator=(WindowsFileWatcher&&) noexcept = default;

WindowsFileWatcherPollResult WindowsFileWatcher::poll() {
    return impl_->poll();
}

bool WindowsFileWatcher::active() const noexcept {
    return impl_ != nullptr && impl_->is_active();
}

const std::filesystem::path& WindowsFileWatcher::directory() const noexcept {
    return impl_->directory_path;
}

FileWatchBackendKind WindowsFileWatcher::backend_kind() noexcept {
    return FileWatchBackendKind::native;
}

FileWatchNativeBackendKind WindowsFileWatcher::native_backend_kind() noexcept {
    return FileWatchNativeBackendKind::windows_read_directory_changes;
}

} // namespace mirakana
