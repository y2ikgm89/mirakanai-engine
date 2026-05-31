// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_app.hpp"
#include "native_editor_launch.hpp"
#include "win32_first_party_editor_host.hpp"

#include <iostream>
#include <string_view>

namespace {

[[nodiscard]] std::string_view adapter_name(mirakana::editor::Win32FirstPartyEditorAdapterKind adapter) noexcept {
    switch (adapter) {
    case mirakana::editor::Win32FirstPartyEditorAdapterKind::hardware:
        return "hardware";
    case mirakana::editor::Win32FirstPartyEditorAdapterKind::warp:
        return "warp";
    case mirakana::editor::Win32FirstPartyEditorAdapterKind::null_renderer:
        return "null_renderer";
    case mirakana::editor::Win32FirstPartyEditorAdapterKind::none:
        break;
    }
    return "none";
}

[[nodiscard]] std::string_view backend_name(mirakana::editor::Win32FirstPartyEditorAdapterKind adapter) noexcept {
    switch (adapter) {
    case mirakana::editor::Win32FirstPartyEditorAdapterKind::hardware:
    case mirakana::editor::Win32FirstPartyEditorAdapterKind::warp:
        return "d3d12";
    case mirakana::editor::Win32FirstPartyEditorAdapterKind::null_renderer:
        return "null_renderer";
    case mirakana::editor::Win32FirstPartyEditorAdapterKind::none:
        break;
    }
    return "none";
}

} // namespace

int main(int argc, char** argv) {
    const auto launch = mirakana::editor::parse_native_editor_launch(argc, argv);
    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    if (!validation.valid) {
        std::cerr << validation.diagnostic << '\n'
                  << mirakana::editor::native_editor_launch_usage(argc > 0 && argv != nullptr && argv[0] != nullptr
                                                                      ? std::string_view{argv[0]}
                                                                      : std::string_view{"MK_editor"})
                  << '\n';
        return mirakana::editor::native_editor_launch_usage_error_exit_code();
    }

    mirakana::editor::NativeEditorApp app{launch.options};
    mirakana::editor::Win32FirstPartyEditorHost host{
        mirakana::editor::Win32FirstPartyEditorHostDesc{.launch = launch.options}};
    const auto result = host.run(app);
    if (launch.options.smoke_frames > 0) {
        const auto& services = app.services();
        const auto& viewport_display = app.viewport_display();
        const auto& material_preview_display = app.material_preview_display();
        std::cout << (result.succeeded ? "editor_shell_status=ready" : "editor_shell_status=failed") << '\n'
                  << "editor_shell_ui=first_party\n"
                  << "editor_shell_backend=" << backend_name(result.adapter_kind) << '\n'
                  << "editor_shell_imgui=0\n"
                  << "editor_shell_sdl3=0\n"
                  << "editor_shell_file_dialog_service=" << services.file_dialog_service_id << '\n'
                  << "editor_shell_clipboard_service=" << services.clipboard_service_id << '\n'
                  << "editor_shell_reviewed_process_runner=" << services.reviewed_process_runner_id << '\n'
                  << "editor_shell_viewport_status=" << viewport_display.status_id << '\n'
                  << "editor_shell_viewport_native_handles_exposed="
                  << (viewport_display.native_texture_handles_exposed ? 1 : 0) << '\n'
                  << "editor_shell_material_preview_status=" << material_preview_display.status_id << '\n'
                  << "editor_shell_material_preview_native_handles_exposed="
                  << (material_preview_display.native_texture_handles_exposed ? 1 : 0) << '\n'
                  << "editor_shell_frames=" << result.frames_rendered << '\n'
                  << "editor_shell_panels=" << app.panels_rendered_last_frame() << '\n'
                  << "editor_shell_resizes=" << result.resize_count << '\n'
                  << "editor_shell_adapter=" << adapter_name(result.adapter_kind) << '\n'
                  << "editor_shell_renderer_boxes_submitted=" << result.renderer_boxes_submitted << '\n'
                  << "editor_shell_renderer_text_runs_available=" << result.renderer_text_runs_available << '\n';
    }
    if (!result.succeeded && !result.diagnostic.empty()) {
        std::cerr << result.diagnostic << '\n';
    }
    return result.exit_code;
}
