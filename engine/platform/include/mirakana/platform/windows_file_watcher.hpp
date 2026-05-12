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

struct WindowsFileWatcherDesc {
    std::filesystem::path directory;
    std::string path_prefix;
    bool recursive{true};
    std::uint32_t buffer_size_bytes{65536};
};

struct WindowsFileWatcherPollResult {
    bool active{false};
    bool overflowed{false};
    std::string diagnostic;
    std::vector<FileWatchEvent> events;
};

class WindowsFileWatcher final {
  public:
    explicit WindowsFileWatcher(WindowsFileWatcherDesc desc);
    ~WindowsFileWatcher();

    WindowsFileWatcher(const WindowsFileWatcher&) = delete;
    WindowsFileWatcher& operator=(const WindowsFileWatcher&) = delete;
    WindowsFileWatcher(WindowsFileWatcher&&) noexcept;
    WindowsFileWatcher& operator=(WindowsFileWatcher&&) noexcept;

    [[nodiscard]] WindowsFileWatcherPollResult poll();
    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] const std::filesystem::path& directory() const noexcept;
    [[nodiscard]] static FileWatchBackendKind backend_kind() noexcept;
    [[nodiscard]] static FileWatchNativeBackendKind native_backend_kind() noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana
