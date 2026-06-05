// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/editor_dock_layout.hpp"
#include "mirakana/editor/editor_rich_text.hpp"
#include "mirakana/editor/workspace.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::editor {

struct EditorAiOperationElementRow {
    std::string id;
    std::string role;
    std::string label;
    bool visible{true};
    bool enabled{true};
};

struct EditorAiOperationDiagnostic {
    std::string code;
    std::string message;
};

struct EditorAiOperationStatusRow {
    std::string id;
    std::string role;
    std::string label;
    std::string target_element_id;
    std::string status;
    std::uint64_t count{0};
    bool ready{false};
    bool visible{true};
    bool enabled{true};
    bool host_gated{false};
    bool native_handles_public{false};
};

struct EditorAiOperationUxStatusDesc {
    std::string selected_dock_panel_id;
    std::uint64_t rich_text_document_count{0};
    std::string focused_text_target_id;
    std::string text_input_status;
    std::string ime_service_id;
    std::uint32_t ime_text_input_session_rows{0};
    std::uint32_t ime_composition_rows{0};
    std::uint32_t ime_committed_text_rows{0};
    std::uint32_t ime_caret_rect_rows{0};
    std::uint32_t ime_surrounding_text_rows{0};
    bool ime_candidate_ui_host_owned{true};
    bool ime_native_handles_exposed{false};
    std::string text_atlas_handoff_status;
    bool text_font_adapter_invoked{false};
    bool text_font_glyphs_ready{false};
    bool text_font_fallback_used{false};
    bool text_atlas_handoff_ready{false};
    bool text_font_native_handles_exposed{false};
    std::uint32_t text_atlas_handoff_host_gated_rows{0};
    std::uint32_t text_atlas_handoff_unsupported_rows{0};
    std::string text_shaping_status;
    std::string text_font_fallback_status;
    std::string text_glyph_atlas_status;
    std::string text_bidi_status;
    std::string text_line_break_status;
    std::string text_dependency_license_records;
    std::string text_harfbuzz_dependency_status;
    std::string text_freetype_dependency_status;
    std::string text_icu_dependency_status;
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
    std::string accessibility_status;
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
    std::string viewport_status;
    std::uint64_t viewport_visible_texture_composites{0};
    bool viewport_native_handles_exposed{false};
    std::string material_preview_status;
    std::uint64_t material_preview_visible_texture_composites{0};
    bool material_preview_native_handles_exposed{false};
};

struct EditorAiOperationSnapshot {
    std::uint64_t revision{0};
    std::vector<EditorAiOperationElementRow> elements;
    std::vector<EditorRichTextAiRow> rich_text_rows;
    std::vector<EditorAiOperationStatusRow> status_rows;
    std::vector<EditorAiOperationDiagnostic> diagnostics;
};

struct EditorAiCommandRow {
    std::string id;
    std::string label;
    std::string target_element_id;
    bool enabled{true};
    bool mutates_state{false};
    bool requires_confirmation{false};
};

struct EditorAiCommandCatalog {
    std::uint64_t revision{0};
    std::vector<EditorAiCommandRow> commands;
};

struct EditorAiCommandParameter {
    std::string key;
    std::string value;
};

struct EditorAiCommandRequest {
    std::string command_id;
    std::string target_element_id;
    std::vector<EditorAiCommandParameter> parameters;
    bool user_confirmed{false};
    std::uint64_t expected_revision{0};
};

struct EditorAiCommandDryRunResult {
    bool accepted{false};
    bool would_mutate{false};
    bool requires_confirmation{false};
    std::vector<EditorAiOperationDiagnostic> diagnostics;
    std::string output_text;
    std::string output_mime_type;
};

struct EditorAiCommandApplyResult {
    bool applied{false};
    std::uint64_t before_revision{0};
    std::uint64_t after_revision{0};
    std::vector<EditorAiOperationDiagnostic> diagnostics;
    bool accepted{false};
    bool completed{false};
    std::string output_text;
    std::string output_mime_type;
};

[[nodiscard]] std::vector<EditorAiOperationStatusRow>
make_editor_ai_operation_ux_status_rows(const EditorAiOperationUxStatusDesc& desc);
[[nodiscard]] EditorAiOperationSnapshot make_editor_ai_operation_snapshot(const Workspace& workspace);
[[nodiscard]] EditorAiOperationSnapshot make_editor_ai_operation_snapshot(const Workspace& workspace,
                                                                          const EditorDockLayout& dock_layout);
[[nodiscard]] EditorAiOperationSnapshot
make_editor_ai_operation_snapshot(const Workspace& workspace, const EditorDockMultiWindowLayout& dock_layout);
[[nodiscard]] EditorAiOperationSnapshot
make_editor_ai_operation_snapshot(const Workspace& workspace, const EditorDockLayout& dock_layout,
                                  std::span<const EditorRichTextDocument> rich_text_documents,
                                  EditorRichTextViewport viewport = {});
[[nodiscard]] EditorAiOperationSnapshot
make_editor_ai_operation_snapshot(const Workspace& workspace, const EditorDockLayout& dock_layout,
                                  std::span<const EditorRichTextDocument> rich_text_documents,
                                  std::span<const EditorAiOperationStatusRow> status_rows,
                                  EditorRichTextViewport viewport = {});

[[nodiscard]] EditorAiCommandCatalog make_editor_ai_command_catalog(const Workspace& workspace);
[[nodiscard]] EditorAiCommandCatalog make_editor_ai_command_catalog(const Workspace& workspace,
                                                                    const EditorDockLayout& dock_layout);
[[nodiscard]] EditorAiCommandCatalog make_editor_ai_command_catalog(const Workspace& workspace,
                                                                    const EditorDockMultiWindowLayout& dock_layout);
[[nodiscard]] EditorAiCommandCatalog
make_editor_ai_command_catalog(const Workspace& workspace, const EditorDockLayout& dock_layout,
                               std::span<const EditorRichTextDocument> rich_text_documents);

[[nodiscard]] EditorAiCommandDryRunResult dry_run_editor_ai_command(const Workspace& workspace,
                                                                    const EditorAiCommandCatalog& catalog,
                                                                    const EditorAiCommandRequest& request);
[[nodiscard]] EditorAiCommandDryRunResult dry_run_editor_ai_command(const Workspace& workspace,
                                                                    const EditorDockLayout& dock_layout,
                                                                    const EditorAiCommandCatalog& catalog,
                                                                    const EditorAiCommandRequest& request);
[[nodiscard]] EditorAiCommandDryRunResult dry_run_editor_ai_command(const Workspace& workspace,
                                                                    const EditorDockMultiWindowLayout& dock_layout,
                                                                    const EditorAiCommandCatalog& catalog,
                                                                    const EditorAiCommandRequest& request);
[[nodiscard]] EditorAiCommandDryRunResult
dry_run_editor_ai_command(const Workspace& workspace, const EditorDockLayout& dock_layout,
                          std::span<const EditorRichTextDocument> rich_text_documents,
                          const EditorAiCommandCatalog& catalog, const EditorAiCommandRequest& request);

[[nodiscard]] EditorAiCommandApplyResult apply_editor_ai_command(Workspace& workspace,
                                                                 const EditorAiCommandCatalog& catalog,
                                                                 const EditorAiCommandRequest& request);
[[nodiscard]] EditorAiCommandApplyResult apply_editor_ai_command(Workspace& workspace, EditorDockLayout& dock_layout,
                                                                 const EditorAiCommandCatalog& catalog,
                                                                 const EditorAiCommandRequest& request);
[[nodiscard]] EditorAiCommandApplyResult apply_editor_ai_command(Workspace& workspace,
                                                                 EditorDockMultiWindowLayout& dock_layout,
                                                                 const EditorAiCommandCatalog& catalog,
                                                                 const EditorAiCommandRequest& request);
[[nodiscard]] EditorAiCommandApplyResult
apply_editor_ai_command(const Workspace& workspace, const EditorDockLayout& dock_layout,
                        std::span<const EditorRichTextDocument> rich_text_documents,
                        const EditorAiCommandCatalog& catalog, const EditorAiCommandRequest& request);
[[nodiscard]] EditorAiCommandApplyResult apply_editor_ai_command(const Workspace& workspace,
                                                                 const EditorDockLayout& dock_layout,
                                                                 std::span<EditorRichTextDocument> rich_text_documents,
                                                                 const EditorAiCommandCatalog& catalog,
                                                                 const EditorAiCommandRequest& request);

} // namespace mirakana::editor
