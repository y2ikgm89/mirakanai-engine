// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "native_editor_launch.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace mirakana::editor {

class NativeEditorApp;

enum class Win32ImguiD3d12AdapterKind : std::uint8_t {
    none = 0,
    hardware,
    warp,
};

struct Win32ImguiD3d12HostDesc {
    NativeEditorLaunchOptions launch;
    bool enable_debug_layer{false};
    bool vsync{false};
};

struct Win32ImguiD3d12HostRunResult {
    bool succeeded{false};
    int exit_code{1};
    std::uint32_t frames_rendered{0};
    std::uint32_t resize_count{0};
    Win32ImguiD3d12AdapterKind adapter_kind{Win32ImguiD3d12AdapterKind::none};
    std::string diagnostic;
};

class Win32ImguiD3d12Host final {
  public:
    explicit Win32ImguiD3d12Host(Win32ImguiD3d12HostDesc desc);
    ~Win32ImguiD3d12Host();

    Win32ImguiD3d12Host(const Win32ImguiD3d12Host&) = delete;
    Win32ImguiD3d12Host& operator=(const Win32ImguiD3d12Host&) = delete;
    Win32ImguiD3d12Host(Win32ImguiD3d12Host&&) = delete;
    Win32ImguiD3d12Host& operator=(Win32ImguiD3d12Host&&) = delete;

    [[nodiscard]] Win32ImguiD3d12HostRunResult run(NativeEditorApp& app);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana::editor
