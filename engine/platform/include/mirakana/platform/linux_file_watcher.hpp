// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/file_watcher.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace mirakana {

struct LinuxFileWatcherDesc {
    std::filesystem::path directory;
    std::string path_prefix;
    bool recursive{true};
    std::uint32_t buffer_size_bytes{65536};
};

struct LinuxFileWatcherPollResult {
    bool active{false};
    bool overflowed{false};
    std::string diagnostic;
    std::vector<FileWatchEvent> events;
};

class LinuxFileWatcher final {
  public:
    explicit LinuxFileWatcher(LinuxFileWatcherDesc desc);
    ~LinuxFileWatcher();

    LinuxFileWatcher(const LinuxFileWatcher&) = delete;
    LinuxFileWatcher& operator=(const LinuxFileWatcher&) = delete;
    LinuxFileWatcher(LinuxFileWatcher&&) noexcept;
    LinuxFileWatcher& operator=(LinuxFileWatcher&&) noexcept;

    [[nodiscard]] LinuxFileWatcherPollResult poll();
    [[nodiscard]] static bool active() noexcept;
    [[nodiscard]] const std::filesystem::path& directory() const noexcept;
    [[nodiscard]] static FileWatchBackendKind backend_kind() noexcept;
    [[nodiscard]] static FileWatchNativeBackendKind native_backend_kind() noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana
