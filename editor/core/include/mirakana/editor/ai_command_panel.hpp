// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/playtest_package_review.hpp"
#include "mirakana/platform/process.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mirakana::editor {

enum class EditorAiCommandPanelStatus : std::uint8_t { ready, blocked, host_gated, external_action_required };

enum class EditorAiReviewedValidationExecutionStatus : std::uint8_t { ready, blocked, host_gated };

struct EditorAiCommandPanelStageRow {
    std::string id;
    std::string source_model;
    std::string status_label;
    std::size_t source_row_count{0};
    std::vector<std::string> source_row_ids;
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    std::string diagnostic;
};

struct EditorAiCommandPanelCommandRow {
    std::string recipe_id;
    std::string status_label;
    std::string command_display;
    std::vector<std::string> argv;
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    std::string readiness_dependency;
    std::string diagnostic;
};

struct EditorAiCommandPanelEvidenceRow {
    std::string recipe_id;
    std::string status_label;
    std::string handoff_status_label;
    bool handoff_row_available{false};
    std::optional<int> exit_code;
    std::string summary;
    std::string stdout_summary;
    std::string stderr_summary;
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    std::string readiness_dependency;
    std::string diagnostic;
};

struct EditorAiCommandPanelDesc {
    EditorAiPlaytestOperatorWorkflowReportModel workflow_report;
    EditorAiPlaytestOperatorHandoffModel operator_handoff;
    EditorAiPlaytestEvidenceSummaryModel evidence_summary;
};

struct EditorAiCommandPanelModel {
    EditorAiCommandPanelStatus status{EditorAiCommandPanelStatus::blocked};
    std::string status_label;
    bool ready_for_operator_handoff{false};
    bool all_required_evidence_passed{false};
    bool external_action_required{false};
    bool remediation_required{false};
    bool handoff_required{false};
    bool has_blocking_diagnostics{false};
    bool has_host_gates{false};
    bool mutates{false};
    bool executes{false};
    std::size_t ready_stage_count{0};
    std::size_t blocked_stage_count{0};
    std::size_t host_gated_stage_count{0};
    std::size_t external_stage_count{0};
    std::vector<EditorAiCommandPanelStageRow> stage_rows;
    std::vector<EditorAiCommandPanelCommandRow> command_rows;
    std::vector<EditorAiCommandPanelEvidenceRow> evidence_rows;
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorAiReviewedValidationExecutionDesc {
    EditorAiPlaytestOperatorHandoffCommandRow command_row;
    std::string working_directory;
    bool acknowledge_host_gates{false};
    std::vector<std::string> acknowledged_host_gates;
    bool request_validation_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_raw_manifest_command_evaluation{false};
    bool request_package_script_execution{false};
    bool request_free_form_manifest_edit{false};
};

struct EditorAiReviewedValidationExecutionModel {
    std::string recipe_id;
    EditorAiReviewedValidationExecutionStatus status{EditorAiReviewedValidationExecutionStatus::blocked};
    std::string status_label;
    bool can_execute{false};
    bool has_blocking_diagnostics{false};
    bool has_host_gates{false};
    bool host_gate_acknowledgement_required{false};
    bool host_gates_acknowledged{false};
    bool mutates{false};
    bool executes{false};
    mirakana::ProcessCommand command;
    std::vector<std::string> host_gates;
    std::vector<std::string> acknowledged_host_gates;
    std::vector<std::string> blocked_by;
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorAiReviewedValidationExecutionBatchDesc {
    std::vector<EditorAiPlaytestOperatorHandoffCommandRow> command_rows;
    std::string working_directory;
    std::vector<std::string> acknowledged_host_gate_recipe_ids;
};

struct EditorAiReviewedValidationExecutionBatchModel {
    std::vector<EditorAiReviewedValidationExecutionModel> plans;
    std::vector<std::size_t> executable_plan_indexes;
    std::vector<mirakana::ProcessCommand> commands;
    std::size_t ready_count{0};
    std::size_t host_gated_count{0};
    std::size_t blocked_count{0};
    bool can_execute_any{false};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view editor_ai_command_panel_status_label(EditorAiCommandPanelStatus status) noexcept;
[[nodiscard]] std::string_view
editor_ai_reviewed_validation_execution_status_label(EditorAiReviewedValidationExecutionStatus status) noexcept;
[[nodiscard]] EditorAiCommandPanelModel make_editor_ai_command_panel_model(const EditorAiCommandPanelDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument make_ai_command_panel_ui_model(const EditorAiCommandPanelModel& model);
[[nodiscard]] EditorAiReviewedValidationExecutionModel
make_editor_ai_reviewed_validation_execution_plan(const EditorAiReviewedValidationExecutionDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_ai_reviewed_validation_execution_ui_model(const EditorAiReviewedValidationExecutionModel& model);
[[nodiscard]] EditorAiReviewedValidationExecutionBatchModel
make_editor_ai_reviewed_validation_execution_batch(const EditorAiReviewedValidationExecutionBatchDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_ai_reviewed_validation_execution_batch_ui_model(const EditorAiReviewedValidationExecutionBatchModel& model);

} // namespace mirakana::editor
