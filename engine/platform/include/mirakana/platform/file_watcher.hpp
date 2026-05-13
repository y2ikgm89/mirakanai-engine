// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace mirakana {

class IFileSystem;

enum class FileWatchBackendKind : std::uint8_t {
    automatic,
    polling,
    native,
};

enum class FileWatchNativeBackendKind : std::uint8_t {
    unavailable,
    windows_read_directory_changes,
    linux_inotify,
    macos_fsevents,
};

struct FileWatchBackendAvailability {
    bool polling{true};
    bool native{false};
    FileWatchNativeBackendKind native_backend{FileWatchNativeBackendKind::unavailable};
};

struct FileWatchBackendChoice {
    FileWatchBackendKind requested{FileWatchBackendKind::automatic};
    FileWatchBackendKind selected{FileWatchBackendKind::polling};
    FileWatchNativeBackendKind native_backend{FileWatchNativeBackendKind::unavailable};
    bool available{false};
    bool fallback{false};
    std::string diagnostic;
};

enum class FileWatchEventKind : std::uint8_t {
    unknown,
    added,
    modified,
    removed,
};

struct FileWatchSnapshot {
    std::string path;
    std::uint64_t revision{0};
    std::uint64_t size_bytes{0};
};

struct FileWatchEvent {
    FileWatchEventKind kind{FileWatchEventKind::unknown};
    std::string path;
    std::uint64_t previous_revision{0};
    std::uint64_t current_revision{0};
    std::uint64_t previous_size_bytes{0};
    std::uint64_t current_size_bytes{0};
};

struct FileWatchPollResult {
    std::vector<FileWatchEvent> events;
    std::vector<FileWatchSnapshot> snapshots;
};

struct PollingFileWatcherDesc {
    const IFileSystem* filesystem{nullptr};
    std::string root;
};

class PollingFileWatcher final {
  public:
    explicit PollingFileWatcher(const PollingFileWatcherDesc& desc);

    [[nodiscard]] FileWatchPollResult poll();
    [[nodiscard]] std::size_t watched_count() const noexcept;
    [[nodiscard]] const std::string& root() const noexcept;
    [[nodiscard]] static FileWatchBackendKind backend_kind() noexcept;

  private:
    const IFileSystem* filesystem_{nullptr};
    std::string root_;
    std::unordered_map<std::string, FileWatchSnapshot> snapshots_by_path_;
};

[[nodiscard]] FileWatchBackendAvailability default_file_watch_backend_availability() noexcept;
[[nodiscard]] FileWatchNativeBackendKind host_file_watch_native_backend() noexcept;
[[nodiscard]] const char* file_watch_native_backend_name(FileWatchNativeBackendKind backend) noexcept;
[[nodiscard]] FileWatchBackendChoice choose_file_watch_backend(FileWatchBackendKind requested);
[[nodiscard]] FileWatchBackendChoice choose_file_watch_backend(FileWatchBackendKind requested,
                                                               FileWatchBackendAvailability availability);
[[nodiscard]] bool is_valid_file_watch_snapshot(const FileWatchSnapshot& snapshot) noexcept;

} // namespace mirakana
