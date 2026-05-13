// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/file_watcher.hpp"

#include "mirakana/platform/filesystem.hpp"

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <utility>

#if defined(__APPLE__)
#include <TargetConditionals.h>
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

[[nodiscard]] std::string normalize_watch_root(std::string_view root) {
    if (root.empty()) {
        throw std::invalid_argument("file watch root must not be empty");
    }
    if (has_control_character(root)) {
        throw std::invalid_argument("file watch root must not contain control characters");
    }
    if (is_absolute_like(root)) {
        throw std::invalid_argument("file watch root must be relative");
    }
    if (has_parent_segment(root)) {
        throw std::invalid_argument("file watch root must not contain '..'");
    }

    std::string normalized(root);
    std::ranges::replace(normalized, '\\', '/');
    while (normalized.size() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    return normalized;
}

[[nodiscard]] bool is_under_root(std::string_view path, std::string_view root) noexcept {
    if (root == ".") {
        return !path.empty();
    }
    return path == root || (path.size() > root.size() && path.starts_with(root) && path[root.size()] == '/');
}

[[nodiscard]] std::uint64_t hash_content(std::string_view text) noexcept {
    std::uint64_t result = 1469598103934665603ULL;
    for (const char value : text) {
        result ^= static_cast<unsigned char>(value);
        result *= 1099511628211ULL;
    }
    return result == 0 ? 1 : result;
}

[[nodiscard]] FileWatchSnapshot make_snapshot(const IFileSystem& filesystem, const std::string& path) {
    const auto text = filesystem.read_text(path);
    return FileWatchSnapshot{
        .path = path,
        .revision = hash_content(text),
        .size_bytes = static_cast<std::uint64_t>(text.size()),
    };
}

[[nodiscard]] FileWatchEvent make_added_event(const FileWatchSnapshot& snapshot) {
    return FileWatchEvent{
        .kind = FileWatchEventKind::added,
        .path = snapshot.path,
        .previous_revision = 0,
        .current_revision = snapshot.revision,
        .previous_size_bytes = 0,
        .current_size_bytes = snapshot.size_bytes,
    };
}

[[nodiscard]] FileWatchEvent make_modified_event(const FileWatchSnapshot& previous, const FileWatchSnapshot& current) {
    return FileWatchEvent{
        .kind = FileWatchEventKind::modified,
        .path = current.path,
        .previous_revision = previous.revision,
        .current_revision = current.revision,
        .previous_size_bytes = previous.size_bytes,
        .current_size_bytes = current.size_bytes,
    };
}

[[nodiscard]] FileWatchEvent make_removed_event(const FileWatchSnapshot& previous) {
    return FileWatchEvent{
        .kind = FileWatchEventKind::removed,
        .path = previous.path,
        .previous_revision = previous.revision,
        .current_revision = 0,
        .previous_size_bytes = previous.size_bytes,
        .current_size_bytes = 0,
    };
}

[[nodiscard]] bool event_less(const FileWatchEvent& left, const FileWatchEvent& right) noexcept {
    if (left.path != right.path) {
        return left.path < right.path;
    }
    return static_cast<int>(left.kind) < static_cast<int>(right.kind);
}

[[nodiscard]] bool snapshot_less(const FileWatchSnapshot& left, const FileWatchSnapshot& right) noexcept {
    return left.path < right.path;
}

[[nodiscard]] bool is_available_native_backend(FileWatchNativeBackendKind backend) noexcept {
    return backend != FileWatchNativeBackendKind::unavailable;
}

[[nodiscard]] FileWatchNativeBackendKind resolve_native_backend(FileWatchBackendAvailability availability) noexcept {
    if (availability.native_backend != FileWatchNativeBackendKind::unavailable) {
        return availability.native_backend;
    }
    return host_file_watch_native_backend();
}

} // namespace

FileWatchNativeBackendKind host_file_watch_native_backend() noexcept {
#if defined(_WIN32)
    return FileWatchNativeBackendKind::windows_read_directory_changes;
#elif defined(__linux__)
    return FileWatchNativeBackendKind::linux_inotify;
#elif defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
    return FileWatchNativeBackendKind::macos_fsevents;
#else
    return FileWatchNativeBackendKind::unavailable;
#endif
}

const char* file_watch_native_backend_name(FileWatchNativeBackendKind backend) noexcept {
    switch (backend) {
    case FileWatchNativeBackendKind::windows_read_directory_changes:
        return "windows_read_directory_changes";
    case FileWatchNativeBackendKind::linux_inotify:
        return "linux_inotify";
    case FileWatchNativeBackendKind::macos_fsevents:
        return "macos_fsevents";
    case FileWatchNativeBackendKind::unavailable:
        return "unavailable";
    }
    return "unavailable";
}

FileWatchBackendAvailability default_file_watch_backend_availability() noexcept {
    const auto native_backend = host_file_watch_native_backend();
    return FileWatchBackendAvailability{
        .polling = true,
        .native = is_available_native_backend(native_backend),
        .native_backend = native_backend,
    };
}

FileWatchBackendChoice choose_file_watch_backend(FileWatchBackendKind requested) {
    return choose_file_watch_backend(requested, default_file_watch_backend_availability());
}

FileWatchBackendChoice choose_file_watch_backend(FileWatchBackendKind requested,
                                                 FileWatchBackendAvailability availability) {
    FileWatchBackendChoice choice;
    choice.requested = requested;
    const auto native_backend =
        availability.native ? resolve_native_backend(availability) : FileWatchNativeBackendKind::unavailable;
    const auto native_available = availability.native && is_available_native_backend(native_backend);

    if (requested == FileWatchBackendKind::native) {
        if (native_available) {
            choice.selected = FileWatchBackendKind::native;
            choice.native_backend = native_backend;
            choice.available = true;
            return choice;
        }
        if (availability.polling) {
            choice.selected = FileWatchBackendKind::polling;
            choice.available = true;
            choice.fallback = true;
            choice.diagnostic = "native file watcher unavailable; using polling";
            return choice;
        }
        choice.selected = FileWatchBackendKind::native;
        choice.available = false;
        choice.diagnostic = "native file watcher unavailable";
        return choice;
    }

    if (requested == FileWatchBackendKind::polling) {
        choice.selected = FileWatchBackendKind::polling;
        choice.available = availability.polling;
        if (!choice.available) {
            choice.diagnostic = "polling file watcher unavailable";
        }
        return choice;
    }

    if (native_available) {
        choice.selected = FileWatchBackendKind::native;
        choice.native_backend = native_backend;
        choice.available = true;
        return choice;
    }
    if (availability.polling) {
        choice.selected = FileWatchBackendKind::polling;
        choice.available = true;
        return choice;
    }

    choice.selected = FileWatchBackendKind::polling;
    choice.available = false;
    choice.diagnostic = "no file watcher backend available";
    return choice;
}

bool is_valid_file_watch_snapshot(const FileWatchSnapshot& snapshot) noexcept {
    return !snapshot.path.empty() && !has_control_character(snapshot.path) && !is_absolute_like(snapshot.path) &&
           !has_parent_segment(snapshot.path) && snapshot.revision != 0;
}

PollingFileWatcher::PollingFileWatcher(const PollingFileWatcherDesc& desc)
    : filesystem_(desc.filesystem), root_(normalize_watch_root(desc.root)) {
    if (filesystem_ == nullptr) {
        throw std::invalid_argument("polling file watcher filesystem must not be null");
    }
}

FileWatchPollResult PollingFileWatcher::poll() {
    FileWatchPollResult result;
    std::unordered_map<std::string, FileWatchSnapshot> current_by_path;

    auto paths = filesystem_->list_files(root_);
    std::ranges::sort(paths);
    for (auto& path : paths) {
        std::ranges::replace(path, '\\', '/');
        if (!is_under_root(path, root_) || !filesystem_->exists(path)) {
            continue;
        }

        auto snapshot = make_snapshot(*filesystem_, path);
        if (!is_valid_file_watch_snapshot(snapshot)) {
            continue;
        }

        const auto previous = snapshots_by_path_.find(snapshot.path);
        if (previous == snapshots_by_path_.end()) {
            result.events.push_back(make_added_event(snapshot));
        } else if (previous->second.revision != snapshot.revision ||
                   previous->second.size_bytes != snapshot.size_bytes) {
            result.events.push_back(make_modified_event(previous->second, snapshot));
        }

        result.snapshots.push_back(snapshot);
        current_by_path.emplace(snapshot.path, std::move(snapshot));
    }

    for (const auto& [path, previous] : snapshots_by_path_) {
        if (current_by_path.find(path) == current_by_path.end()) {
            result.events.push_back(make_removed_event(previous));
        }
    }

    std::ranges::sort(result.events, event_less);
    std::ranges::sort(result.snapshots, snapshot_less);
    snapshots_by_path_ = std::move(current_by_path);
    return result;
}

std::size_t PollingFileWatcher::watched_count() const noexcept {
    return snapshots_by_path_.size();
}

const std::string& PollingFileWatcher::root() const noexcept {
    return root_;
}

FileWatchBackendKind PollingFileWatcher::backend_kind() noexcept {
    return FileWatchBackendKind::polling;
}

} // namespace mirakana
