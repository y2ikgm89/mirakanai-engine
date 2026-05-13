// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/file_watcher.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace mirakana {

struct MacOSFileWatcherDesc {
    std::filesystem::path directory;
    std::string path_prefix;
    bool recursive{true};
    double latency_seconds{0.05};
};

struct MacOSFileWatcherPollResult {
    bool active{false};
    bool overflowed{false};
    std::string diagnostic;
    std::vector<FileWatchEvent> events;
};

class MacOSFileWatcher final {
  public:
    explicit MacOSFileWatcher(MacOSFileWatcherDesc desc);
    ~MacOSFileWatcher();

    MacOSFileWatcher(const MacOSFileWatcher&) = delete;
    MacOSFileWatcher& operator=(const MacOSFileWatcher&) = delete;
    MacOSFileWatcher(MacOSFileWatcher&&) noexcept;
    MacOSFileWatcher& operator=(MacOSFileWatcher&&) noexcept;

    [[nodiscard]] MacOSFileWatcherPollResult poll();
    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] const std::filesystem::path& directory() const noexcept;
    [[nodiscard]] static FileWatchBackendKind backend_kind() noexcept;
    [[nodiscard]] static FileWatchNativeBackendKind native_backend_kind() noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana
