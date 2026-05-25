// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/ai_command_panel.hpp"
#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorAiGeneratedGameStudioV1Status : std::uint8_t {
    ready,
    blocked,
    host_gated,
    awaiting_external_evidence,
    remediation_required,
    handoff_required,
    closed
};

struct EditorAiGeneratedGameStudioV1DesignSpecRow {
    std::string id;
    std::string gameplay_family;
    std::string template_id;
    std::string package_target_id;
    std::vector<std::string> validation_recipe_ids;
    std::vector<ScenePackageRegistrationDraftRow> scene_package_registration_draft_rows;
    std::vector<SceneAuthoringNodeRow> scene_nodes;
    std::vector<SceneAuthoringDiagnostic> diagnostics;
    bool three_dimensional{false};
    bool host_gated_package_evidence_required{false};
};

struct EditorAiGeneratedGameStudioV1LoopRow {
    std::string id;
    std::string design_spec_id;
    std::string gameplay_family;
    std::string template_id;
    std::string package_target_id;
    EditorAiGeneratedGameStudioV1Status status{EditorAiGeneratedGameStudioV1Status::blocked};
    std::string status_label;
    bool three_dimensional{false};
    bool host_gated_package_evidence_required{false};
    std::size_t validation_recipe_count{0};
    std::size_t package_registration_draft_count{0};
    std::size_t scene_node_count{0};
    std::size_t workflow_stage_count{0};
    std::size_t command_row_count{0};
    std::size_t evidence_row_count{0};
    std::size_t remediation_row_count{0};
    std::size_t handoff_row_count{0};
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    std::vector<std::string> diagnostics;
};

struct EditorAiGeneratedGameStudioV1Desc {
    std::vector<EditorAiGeneratedGameStudioV1DesignSpecRow> design_specs;
    EditorAiPlaytestOperatorWorkflowReportModel operator_workflow_report;
    EditorAiCommandPanelModel ai_command_panel;
    bool request_manifest_mutation{false};
    bool request_validation_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_engine_internal_mutation{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_broad_editor_productization{false};
};

struct EditorAiGeneratedGameStudioV1Model {
    EditorAiGeneratedGameStudioV1Status status{EditorAiGeneratedGameStudioV1Status::blocked};
    std::string status_label;
    std::vector<EditorAiGeneratedGameStudioV1DesignSpecRow> design_specs;
    EditorAiPlaytestOperatorWorkflowReportModel operator_workflow_report;
    EditorAiCommandPanelModel ai_command_panel;
    std::vector<EditorAiGeneratedGameStudioV1LoopRow> loop_rows;
    bool ready_for_operator_handoff{false};
    bool all_required_evidence_passed{false};
    bool external_action_required{false};
    bool remediation_required{false};
    bool handoff_required{false};
    bool has_blocking_diagnostics{false};
    bool has_host_gates{false};
    bool mutates{false};
    bool executes{false};
    std::size_t ready_loop_count{0};
    std::size_t blocked_loop_count{0};
    std::size_t host_gated_loop_count{0};
    std::size_t external_loop_count{0};
    std::size_t closed_loop_count{0};
    std::size_t workflow_stage_count{0};
    std::size_t command_row_count{0};
    std::size_t evidence_row_count{0};
    std::size_t remediation_row_count{0};
    std::size_t handoff_row_count{0};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view
editor_ai_generated_game_studio_v1_status_label(EditorAiGeneratedGameStudioV1Status status) noexcept;

[[nodiscard]] EditorAiGeneratedGameStudioV1Model
make_editor_ai_generated_game_studio_v1_model(const EditorAiGeneratedGameStudioV1Desc& desc);

[[nodiscard]] mirakana::ui::UiDocument
make_editor_ai_generated_game_studio_v1_ui_model(const EditorAiGeneratedGameStudioV1Model& model);

} // namespace mirakana::editor
