// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/editor_cross_platform_shell.hpp"

#include <array>
#include <string_view>

namespace mirakana::editor {
namespace {

struct StaticAdapterRow {
    std::string_view id;
    std::string_view platform;
    std::string_view capability;
    bool unsupported;
};

constexpr std::array core_rows{
    StaticAdapterRow{"editor.shell.core.window_lifecycle", "cross_platform", "window_lifecycle", false},
    StaticAdapterRow{"editor.shell.core.dpi_scale", "cross_platform", "dpi_scale", false},
    StaticAdapterRow{"editor.shell.core.monitor_rows", "cross_platform", "monitor_rows", false},
    StaticAdapterRow{"editor.shell.core.pointer_keyboard_events", "cross_platform", "pointer_keyboard_events", false},
    StaticAdapterRow{"editor.shell.core.clipboard", "cross_platform", "clipboard", false},
    StaticAdapterRow{"editor.shell.core.file_dialog", "cross_platform", "file_dialog", false},
    StaticAdapterRow{"editor.shell.core.text_input", "cross_platform", "text_input", false},
    StaticAdapterRow{"editor.shell.core.accessibility", "cross_platform", "accessibility", false},
    StaticAdapterRow{"editor.shell.core.renderer_presentation", "cross_platform", "renderer_presentation", false},
};

constexpr std::array macos_rows{
    StaticAdapterRow{"editor.shell.macos.appkit_windowing", "macos", "appkit_windowing", false},
    StaticAdapterRow{"editor.shell.macos.core_text", "macos", "core_text", false},
    StaticAdapterRow{"editor.shell.macos.nstextinputclient", "macos", "nstextinputclient", false},
    StaticAdapterRow{"editor.shell.macos.nsaccessibility_protocol", "macos", "nsaccessibility_protocol", false},
    StaticAdapterRow{"editor.shell.macos.metal_presentation", "macos", "metal_presentation", false},
};

constexpr std::array linux_rows{
    StaticAdapterRow{"editor.shell.linux.x11_wayland_selection", "linux", "x11_wayland_selection", false},
    StaticAdapterRow{"editor.shell.linux.at_spi2", "linux", "at_spi2", false},
    StaticAdapterRow{"editor.shell.linux.ibus", "linux", "ibus", false},
    StaticAdapterRow{"editor.shell.linux.fcitx", "linux", "fcitx", false},
    StaticAdapterRow{"editor.shell.linux.vulkan_presentation", "linux", "vulkan_presentation", false},
};

constexpr std::array mobile_rows{
    StaticAdapterRow{"editor.shell.android.visible_editor_shell", "android", "visible_editor_shell", true},
    StaticAdapterRow{"editor.shell.ios.visible_editor_shell", "ios", "visible_editor_shell", true},
};

void append_rows(EditorCrossPlatformShellAdapterPlan& plan, std::span<const StaticAdapterRow> rows,
                 bool native_handles_exposed) {
    for (const auto& row : rows) {
        const bool blocked = native_handles_exposed && !row.unsupported;
        plan.rows.push_back(EditorCrossPlatformShellAdapterRow{
            .id = std::string{row.id},
            .platform = std::string{row.platform},
            .capability = std::string{row.capability},
            .status = row.unsupported ? "unsupported"
                      : blocked       ? "blocked_native_handles"
                                      : "host_gated",
            .first_party = true,
            .host_gated = !row.unsupported,
            .unsupported = row.unsupported,
            .native_handles_public = native_handles_exposed,
        });
        if (blocked || (native_handles_exposed && row.unsupported)) {
            ++plan.native_handle_blocked_rows;
        }
    }
}

} // namespace

EditorCrossPlatformShellAdapterPlan
make_editor_cross_platform_shell_adapter_plan(EditorCrossPlatformShellAdapterDesc desc) {
    EditorCrossPlatformShellAdapterPlan plan;
    plan.native_handles_exposed = desc.native_handles_exposed;
    plan.core_contract_rows = static_cast<std::uint32_t>(core_rows.size());
    plan.macos_adapter_rows = static_cast<std::uint32_t>(macos_rows.size());
    plan.linux_adapter_rows = static_cast<std::uint32_t>(linux_rows.size());
    if (desc.native_handles_exposed) {
        plan.cross_platform_status = "blocked_native_handles";
        plan.macos_status = "blocked_native_handles";
        plan.linux_status = "blocked_native_handles";
    }
    plan.rows.reserve(core_rows.size() + macos_rows.size() + linux_rows.size() + mobile_rows.size());
    append_rows(plan, core_rows, desc.native_handles_exposed);
    append_rows(plan, macos_rows, desc.native_handles_exposed);
    append_rows(plan, linux_rows, desc.native_handles_exposed);
    append_rows(plan, mobile_rows, desc.native_handles_exposed);
    return plan;
}

const EditorCrossPlatformShellAdapterRow*
find_editor_cross_platform_shell_adapter_row(std::span<const EditorCrossPlatformShellAdapterRow> rows,
                                             std::string_view id) noexcept {
    for (const auto& row : rows) {
        if (row.id == id) {
            return &row;
        }
    }
    return nullptr;
}

} // namespace mirakana::editor
