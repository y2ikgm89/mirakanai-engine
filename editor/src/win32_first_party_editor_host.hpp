// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "native_editor_launch.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace mirakana::editor {

class NativeEditorApp;

enum class Win32FirstPartyEditorAdapterKind : std::uint8_t {
    hardware,
    warp,
    null_renderer,
    none,
};

struct Win32FirstPartyEditorHostDesc {
    NativeEditorLaunchOptions launch;
    bool enable_debug_layer{false};
    bool vsync{false};
};

struct Win32FirstPartyEditorRunResult {
    bool succeeded{false};
    int exit_code{1};
    std::uint32_t frames_rendered{0};
    std::uint32_t resize_count{0};
    Win32FirstPartyEditorAdapterKind adapter_kind{Win32FirstPartyEditorAdapterKind::none};
    std::uint32_t renderer_boxes_submitted{0};
    std::uint32_t renderer_text_runs_available{0};
    std::string diagnostic;
};

class Win32FirstPartyEditorHost final {
  public:
    explicit Win32FirstPartyEditorHost(Win32FirstPartyEditorHostDesc desc);
    ~Win32FirstPartyEditorHost();

    Win32FirstPartyEditorHost(const Win32FirstPartyEditorHost&) = delete;
    Win32FirstPartyEditorHost& operator=(const Win32FirstPartyEditorHost&) = delete;
    Win32FirstPartyEditorHost(Win32FirstPartyEditorHost&&) = delete;
    Win32FirstPartyEditorHost& operator=(Win32FirstPartyEditorHost&&) = delete;

    [[nodiscard]] Win32FirstPartyEditorRunResult run(NativeEditorApp& app);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana::editor
