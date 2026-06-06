// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/ai_operation_surface.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <string>

namespace mirakana::editor {

class NativeEditorApp;

struct FirstPartyEditorDocument {
    ui::UiDocument document;
    ui::LayoutResult layout;
    ui::RendererSubmission renderer_submission;
    double document_build_us{0.0};
    double layout_us{0.0};
    double renderer_submission_us{0.0};
    std::uint64_t retained_memory_high_water_bytes{0};
    std::string docking_status{"not_rendered"};
    ui::ElementId focused_element;
    std::uint32_t panel_root_count{0};
    std::uint32_t tab_header_count{0};
    std::uint32_t split_gutter_count{0};
    std::uint32_t active_panel_count{0};
    std::uint32_t focusable_dock_control_count{0};
    bool native_handles_exposed{false};
};

struct FirstPartyEditorShellSmokeCounters {
    std::string ui{"first_party"};
    std::string backend{"d3d12"};
    std::uint32_t panel_count{0};
    bool imgui_enabled{false};
    bool sdl3_enabled{false};
    std::string viewport_status{"host_unavailable"};
    std::uint64_t viewport_visible_texture_composites{0};
    bool viewport_native_handles_exposed{false};
    std::string viewport_vulkan_status{"host_gated"};
    std::uint64_t viewport_vulkan_visible_texture_composites{0};
    std::string material_preview_status{"host_unavailable"};
    std::uint64_t material_preview_visible_texture_composites{0};
    bool material_preview_native_handles_exposed{false};
    std::string material_preview_vulkan_status{"host_gated"};
    std::uint64_t material_preview_vulkan_visible_texture_composites{0};
    bool vulkan_validation_layer_ready{false};
    bool vulkan_native_handles_exposed{false};
    std::string text_atlas_handoff_status{"not_evaluated"};
    bool text_font_adapter_invoked{false};
    bool text_font_glyphs_ready{false};
    bool text_font_fallback_used{false};
    bool text_atlas_handoff_ready{false};
    bool text_font_native_handles_exposed{false};
    std::uint32_t text_atlas_handoff_host_gated_rows{0};
    std::uint32_t text_atlas_handoff_unsupported_rows{0};
    std::string text_shaping_status{"not_evaluated"};
    std::string text_font_fallback_status{"not_evaluated"};
    std::string text_glyph_atlas_status{"not_evaluated"};
    std::string text_bidi_status{"not_evaluated"};
    std::string text_line_break_status{"not_evaluated"};
    std::string text_dependency_license_records{"not_evaluated"};
    std::string text_harfbuzz_dependency_status{"dependency_gated"};
    std::string text_freetype_dependency_status{"dependency_gated"};
    std::string text_icu_dependency_status{"dependency_gated"};
    std::uint32_t text_shaping_segment_rows{0};
    std::uint32_t text_glyph_cluster_rows{0};
    std::uint32_t text_glyph_advance_offset_rows{0};
    std::uint32_t text_bidi_boundary_rows{0};
    std::uint32_t text_word_boundary_rows{0};
    std::uint32_t text_line_break_boundary_rows{0};
    std::uint32_t text_font_face_rows{0};
    std::uint32_t text_glyph_metric_rows{0};
    std::uint32_t text_glyph_bitmap_format_rows{0};
    std::uint32_t text_glyph_atlas_allocation_rows{0};
    std::uint32_t text_font_license_provenance_rows{0};
    std::uint32_t text_dependency_gated_rows{0};
    bool text_native_handles_exposed{false};
    std::string ime_status{"value_text_input_not_started"};
    std::uint32_t ime_text_input_session_rows{0};
    std::uint32_t ime_composition_rows{0};
    std::uint32_t ime_committed_text_rows{0};
    std::uint32_t ime_caret_rect_rows{0};
    std::uint32_t ime_surrounding_text_rows{0};
    bool ime_candidate_ui_host_owned{true};
    std::string ime_parity_status{"not_ready"};
    std::string ime_windows_tsf_status{"host_gated"};
    std::string ime_macos_status{"host_gated"};
    std::string ime_linux_ibus_status{"host_gated"};
    std::string ime_linux_fcitx_status{"host_gated"};
    std::string ime_android_status{"host_gated"};
    std::string ime_ios_status{"host_gated"};
    std::uint32_t ime_grapheme_boundary_rows{0};
    std::uint32_t ime_grapheme_cursor_rows{0};
    std::uint32_t ime_grapheme_selection_rows{0};
    std::uint32_t ime_composition_range_rows{0};
    std::uint32_t ime_candidate_selection_rows{0};
    std::uint32_t ime_reconversion_request_rows{0};
    bool ime_native_handles_exposed{false};
    std::string accessibility_status{"uia_provider_not_published"};
    std::uint32_t accessibility_nodes{0};
    std::uint32_t accessibility_role_rows{0};
    std::uint32_t accessibility_name_rows{0};
    std::uint32_t accessibility_state_rows{0};
    std::uint32_t accessibility_focus_rows{0};
    std::uint32_t accessibility_action_rows{0};
    std::uint32_t accessibility_relationship_rows{0};
    std::uint32_t accessibility_tree_navigation_rows{0};
    std::uint32_t accessibility_diagnostics{0};
    std::uint32_t accessibility_missing_name_diagnostics{0};
    std::uint32_t accessibility_missing_role_diagnostics{0};
    std::uint32_t accessibility_invalid_bounds_diagnostics{0};
    std::uint32_t accessibility_hidden_nodes{0};
    std::uint32_t accessibility_unsupported_pattern_diagnostics{0};
    std::string accessibility_parity_status{"not_ready"};
    bool accessibility_windows_uia_patterns_ready{false};
    bool accessibility_windows_uia_events_ready{false};
    std::string accessibility_macos_status{"host_gated"};
    std::string accessibility_linux_at_spi_status{"host_gated"};
    std::string accessibility_android_status{"host_gated"};
    std::string accessibility_ios_status{"host_gated"};
    std::uint32_t accessibility_live_region_rows{0};
    std::uint32_t accessibility_uia_pattern_rows{0};
    std::uint32_t accessibility_uia_event_rows{0};
    bool accessibility_native_handles_exposed{false};
    std::string docking_status{"not_rendered"};
    std::uint32_t dock_tab_header_count{0};
    std::uint32_t dock_split_gutter_count{0};
    std::uint32_t dock_active_panel_count{0};
    std::uint32_t dock_focusable_control_count{0};
    std::string multi_window_docking_status{"not_ready"};
    std::uint32_t dock_window_count{0};
    std::uint32_t dock_tear_off_command_count{0};
    std::uint32_t dock_window_merge_command_count{0};
    std::string workspace_v3_status{"not_ready"};
    bool multi_window_native_handles_exposed{false};
    std::string rich_text_edit_status{"not_ready"};
    std::uint32_t rich_text_editable_documents{0};
    std::uint32_t rich_text_command_rows{0};
    bool rich_text_clipboard_plain_ready{false};
    bool rich_text_clipboard_rich_ready{false};
    bool rich_text_native_handles_exposed{false};
    std::string ui_performance_budget_status{"missing_samples"};
    double ui_performance_layout_us_p95{0.0};
    double ui_performance_document_build_us_p95{0.0};
    double ui_performance_renderer_submission_us_p95{0.0};
    std::uint64_t ui_performance_text_runs{0};
    std::uint64_t ui_performance_renderer_boxes{0};
    std::uint64_t ui_performance_visible_texture_composites{0};
    std::uint64_t ui_performance_memory_high_water_bytes{0};
    std::uint32_t ui_performance_budget_violations{0};
    std::uint32_t ui_performance_diagnostics{0};
    bool ui_performance_broad_optimization_claimed{false};
    std::string ui_retained_diff_status{"invalid_request"};
    std::uint64_t ui_retained_dirty_rows{0};
    std::uint64_t ui_retained_layout_cache_hits{0};
    std::uint64_t ui_retained_layout_cache_misses{0};
    std::uint64_t ui_retained_text_cache_hits{0};
    std::uint64_t ui_retained_text_cache_misses{0};
    std::uint64_t ui_retained_submission_reused_rows{0};
    std::uint64_t ui_retained_submission_rebuilt_rows{0};
    bool ui_retained_cache_native_handle_access{false};
};

[[nodiscard]] FirstPartyEditorDocument make_first_party_editor_document(const NativeEditorApp& app);

[[nodiscard]] FirstPartyEditorShellSmokeCounters
make_first_party_editor_shell_smoke_counters(const NativeEditorApp& app, const FirstPartyEditorDocument& document);
[[nodiscard]] EditorAiOperationUxStatusDesc
make_first_party_editor_ai_operation_ux_status_desc(const NativeEditorApp& app,
                                                    const FirstPartyEditorDocument& document);
[[nodiscard]] EditorAiOperationSnapshot
make_first_party_editor_ai_operation_snapshot(const NativeEditorApp& app, const FirstPartyEditorDocument& document);

} // namespace mirakana::editor
