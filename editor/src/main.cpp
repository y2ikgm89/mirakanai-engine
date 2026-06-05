// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "first_party_editor_document.hpp"
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
        const auto& text_atlas = app.text_atlas_handoff_evidence();
        const auto& text_input = app.text_input_state();
        const auto& accessibility = app.accessibility_state();
        const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
        const auto counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);
        std::cout
            << (result.succeeded ? "editor_shell_status=ready" : "editor_shell_status=failed") << '\n'
            << "editor_shell_ui=first_party\n"
            << "editor_shell_backend=" << backend_name(result.adapter_kind) << '\n'
            << "editor_shell_imgui=0\n"
            << "editor_shell_sdl3=0\n"
            << "editor_shell_file_dialog_service=" << services.file_dialog_service_id << '\n'
            << "editor_shell_clipboard_service=" << services.clipboard_service_id << '\n'
            << "editor_shell_reviewed_process_runner=" << services.reviewed_process_runner_id << '\n'
            << "editor_shell_viewport_status=" << viewport_display.status_id << '\n'
            << "editor_shell_viewport_visible_texture_composites=" << viewport_display.visible_texture_composites
            << '\n'
            << "editor_shell_viewport_native_handles_exposed="
            << (viewport_display.native_texture_handles_exposed ? 1 : 0) << '\n'
            << "editor_shell_material_preview_status=" << material_preview_display.status_id << '\n'
            << "editor_shell_material_preview_visible_texture_composites="
            << material_preview_display.visible_texture_composites << '\n'
            << "editor_shell_material_preview_native_handles_exposed="
            << (material_preview_display.native_texture_handles_exposed ? 1 : 0) << '\n'
            << "editor_shell_text_atlas_handoff_status=" << text_atlas.status << '\n'
            << "editor_shell_text_font_adapter_invoked="
            << (text_atlas.text_shaping_adapter_invoked && text_atlas.font_rasterizer_adapter_invoked ? 1 : 0) << '\n'
            << "editor_shell_text_font_glyphs_ready=" << (text_atlas.glyphs_ready ? 1 : 0) << '\n'
            << "editor_shell_text_font_fallback_used=" << (text_atlas.fallback_used ? 1 : 0) << '\n'
            << "editor_shell_text_atlas_handoff_host_gated_rows=" << text_atlas.host_gated_rows << '\n'
            << "editor_shell_text_atlas_handoff_unsupported_rows=" << text_atlas.unsupported_rows << '\n'
            << "editor_shell_text_font_native_handles_exposed=" << (text_atlas.native_handles_exposed ? 1 : 0) << '\n'
            << "editor_shell_text_input_service=" << services.platform_text_input_service_id << '\n'
            << "editor_shell_ime_service=" << services.ime_service_id << '\n'
            << "editor_shell_ime_status=" << mirakana::editor::native_editor_text_input_status(text_input) << '\n'
            << "editor_shell_ime_text_input_session_rows=" << (text_input.session_active ? 1 : 0) << '\n'
            << "editor_shell_ime_composition_rows=" << (text_input.composition_active ? 1 : 0) << '\n'
            << "editor_shell_ime_committed_text_rows=" << (text_input.commit_applied ? 1 : 0) << '\n'
            << "editor_shell_ime_caret_rect_rows=" << (text_input.caret_rect_ready ? 1 : 0) << '\n'
            << "editor_shell_ime_surrounding_text_rows=" << (text_input.surrounding_text_ready ? 1 : 0) << '\n'
            << "editor_shell_ime_candidate_ui_host_owned=" << (text_input.candidate_ui_host_owned ? 1 : 0) << '\n'
            << "editor_shell_ime_native_handles_exposed=" << (text_input.native_handles_exposed ? 1 : 0) << '\n'
            << "editor_shell_accessibility_service=" << services.accessibility_service_id << '\n'
            << "editor_shell_accessibility_status=" << accessibility.status_id << '\n'
            << "editor_shell_accessibility_nodes=" << accessibility.nodes.size() << '\n'
            << "editor_shell_accessibility_role_rows=" << accessibility.role_rows << '\n'
            << "editor_shell_accessibility_name_rows=" << accessibility.name_rows << '\n'
            << "editor_shell_accessibility_state_rows=" << accessibility.state_rows << '\n'
            << "editor_shell_accessibility_focus_rows=" << accessibility.focus_rows << '\n'
            << "editor_shell_accessibility_action_rows=" << accessibility.action_rows << '\n'
            << "editor_shell_accessibility_relationship_rows=" << accessibility.relationship_rows << '\n'
            << "editor_shell_accessibility_tree_navigation_rows=" << accessibility.tree_navigation_rows << '\n'
            << "editor_shell_accessibility_diagnostics=" << accessibility.diagnostics.size() << '\n'
            << "editor_shell_accessibility_missing_name_diagnostics=" << accessibility.missing_name_diagnostics << '\n'
            << "editor_shell_accessibility_missing_role_diagnostics=" << accessibility.missing_role_diagnostics << '\n'
            << "editor_shell_accessibility_invalid_bounds_diagnostics=" << accessibility.invalid_bounds_diagnostics
            << '\n'
            << "editor_shell_accessibility_hidden_nodes=" << accessibility.hidden_nodes << '\n'
            << "editor_shell_accessibility_unsupported_pattern_diagnostics="
            << accessibility.unsupported_pattern_diagnostics << '\n'
            << "editor_shell_accessibility_native_handles_exposed=" << (accessibility.native_handles_exposed ? 1 : 0)
            << '\n'
            << "editor_shell_frames=" << result.frames_rendered << '\n'
            << "editor_shell_panels=" << app.panels_rendered_last_frame() << '\n'
            << "editor_shell_docking_status=" << app.docking_status_last_frame() << '\n'
            << "editor_shell_dock_tab_headers=" << app.dock_tab_headers_last_frame() << '\n'
            << "editor_shell_dock_split_gutters=" << app.dock_split_gutters_last_frame() << '\n'
            << "editor_shell_dock_active_panels=" << app.dock_active_panels_last_frame() << '\n'
            << "editor_shell_dock_focusable_controls=" << app.dock_focusable_controls_last_frame() << '\n'
            << "editor_shell_resizes=" << result.resize_count << '\n'
            << "editor_shell_adapter=" << adapter_name(result.adapter_kind) << '\n'
            << "editor_shell_renderer_boxes_submitted=" << result.renderer_boxes_submitted << '\n'
            << "editor_shell_renderer_text_runs_available=" << result.renderer_text_runs_available << '\n'
            << "editor_ui_performance_budget_status=" << counters.ui_performance_budget_status << '\n'
            << "editor_ui_performance_layout_us_p95=" << counters.ui_performance_layout_us_p95 << '\n'
            << "editor_ui_performance_document_build_us_p95=" << counters.ui_performance_document_build_us_p95 << '\n'
            << "editor_ui_performance_renderer_submission_us_p95=" << counters.ui_performance_renderer_submission_us_p95
            << '\n'
            << "editor_ui_performance_text_runs=" << counters.ui_performance_text_runs << '\n'
            << "editor_ui_performance_renderer_boxes=" << counters.ui_performance_renderer_boxes << '\n'
            << "editor_ui_performance_visible_texture_composites=" << counters.ui_performance_visible_texture_composites
            << '\n'
            << "editor_ui_performance_memory_high_water_bytes=" << counters.ui_performance_memory_high_water_bytes
            << '\n'
            << "editor_ui_performance_budget_violations=" << counters.ui_performance_budget_violations << '\n'
            << "editor_ui_performance_diagnostics=" << counters.ui_performance_diagnostics << '\n'
            << "editor_ui_performance_broad_optimization_claimed="
            << (counters.ui_performance_broad_optimization_claimed ? 1 : 0) << '\n'
            << "ui_retained_diff_status=" << counters.ui_retained_diff_status << '\n'
            << "ui_retained_dirty_rows=" << counters.ui_retained_dirty_rows << '\n'
            << "ui_retained_layout_cache_hits=" << counters.ui_retained_layout_cache_hits << '\n'
            << "ui_retained_layout_cache_misses=" << counters.ui_retained_layout_cache_misses << '\n'
            << "ui_retained_text_cache_hits=" << counters.ui_retained_text_cache_hits << '\n'
            << "ui_retained_text_cache_misses=" << counters.ui_retained_text_cache_misses << '\n'
            << "ui_retained_submission_reused_rows=" << counters.ui_retained_submission_reused_rows << '\n'
            << "ui_retained_submission_rebuilt_rows=" << counters.ui_retained_submission_rebuilt_rows << '\n'
            << "ui_retained_cache_native_handle_access=" << (counters.ui_retained_cache_native_handle_access ? 1 : 0)
            << '\n';
    }
    if (!result.succeeded && !result.diagnostic.empty()) {
        std::cerr << result.diagnostic << '\n';
    }
    return result.exit_code;
}
