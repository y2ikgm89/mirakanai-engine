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
    std::string material_preview_status{"host_unavailable"};
    std::uint64_t material_preview_visible_texture_composites{0};
    bool material_preview_native_handles_exposed{false};
    std::string text_atlas_handoff_status{"not_evaluated"};
    bool text_font_adapter_invoked{false};
    bool text_font_glyphs_ready{false};
    bool text_font_fallback_used{false};
    bool text_atlas_handoff_ready{false};
    bool text_font_native_handles_exposed{false};
    std::uint32_t text_atlas_handoff_host_gated_rows{0};
    std::uint32_t text_atlas_handoff_unsupported_rows{0};
    std::string ime_status{"value_text_input_not_started"};
    std::uint32_t ime_text_input_session_rows{0};
    std::uint32_t ime_composition_rows{0};
    std::uint32_t ime_committed_text_rows{0};
    std::uint32_t ime_caret_rect_rows{0};
    std::uint32_t ime_surrounding_text_rows{0};
    bool ime_candidate_ui_host_owned{true};
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
    bool accessibility_native_handles_exposed{false};
    std::uint32_t environment_artist_workflow_command_plan_rows{0};
    std::uint32_t environment_artist_workflow_execution_review_rows{0};
    std::uint32_t environment_artist_workflow_external_execution_rows{0};
    std::uint32_t environment_artist_workflow_operator_review_rows{0};
    bool environment_artist_workflow_executes_backend{false};
    bool environment_artist_workflow_executes_package_scripts{false};
    bool environment_artist_workflow_executes_validation_recipes{false};
    bool environment_artist_workflow_native_handles_exposed{false};
    bool environment_artist_workflow_ready_claimed{false};
    std::string docking_status{"not_rendered"};
    std::uint32_t dock_tab_header_count{0};
    std::uint32_t dock_split_gutter_count{0};
    std::uint32_t dock_active_panel_count{0};
    std::uint32_t dock_focusable_control_count{0};
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
