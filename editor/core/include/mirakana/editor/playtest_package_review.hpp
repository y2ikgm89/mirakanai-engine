// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/tools/runtime_scene_package_validation_tool.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {
class RuntimeAssetPackage;
}

namespace mirakana::editor {

enum class EditorPlaytestReviewStepStatus { ready, blocked, host_gated };

enum class EditorAiPackageAuthoringDiagnosticStatus { ready, blocked, host_gated };

enum class EditorAiPlaytestEvidenceStatus { passed, failed, blocked, host_gated, missing };

enum class EditorAiPlaytestRemediationCategory {
    investigate_failure,
    resolve_blocker,
    collect_missing_evidence,
    satisfy_host_gate
};

enum class EditorAiPlaytestRemediationHandoffActionKind {
    investigate_external_failure,
    resolve_external_blocker,
    collect_external_evidence,
    satisfy_host_gate
};

enum class EditorAiPlaytestOperatorWorkflowStageStatus {
    ready,
    blocked,
    host_gated,
    awaiting_external_evidence,
    remediation_required,
    handoff_required,
    closed
};

enum class EditorAiPlaytestEvidenceImportStatus { importable, missing, blocked };

enum class EditorRuntimeScenePackageValidationExecutionStatus { ready, blocked, passed, failed };

struct RuntimeSceneValidationTargetRow {
    std::string id;
    std::string package_index_path;
    std::string scene_asset_key;
    std::string content_root;
    bool validate_asset_references{true};
    bool require_unique_node_names{true};
};

struct EditorPlaytestPackageReviewStepRow {
    std::string id;
    std::string surface;
    EditorPlaytestReviewStepStatus status{EditorPlaytestReviewStepStatus::blocked};
    bool mutates{false};
    std::string diagnostic;
};

struct EditorPlaytestPackageReviewDesc {
    std::vector<ScenePackageRegistrationDraftRow> package_registration_draft_rows;
    std::vector<RuntimeSceneValidationTargetRow> runtime_scene_validation_targets;
    std::string selected_runtime_scene_validation_target_id;
    std::vector<std::string> host_gated_smoke_recipes;
};

struct EditorPlaytestPackageReviewModel {
    std::vector<EditorPlaytestPackageReviewStepRow> steps;
    std::vector<std::string> pending_runtime_package_files;
    std::optional<RuntimeSceneValidationTargetRow> selected_runtime_scene_validation_target;
    bool ready_for_runtime_scene_validation{false};
    bool host_gated_desktop_smoke_available{false};
    std::vector<std::string> diagnostics;
};

struct EditorAiPackageDescriptorDiagnosticRow {
    std::string id;
    std::string surface;
    EditorAiPackageAuthoringDiagnosticStatus status{EditorAiPackageAuthoringDiagnosticStatus::blocked};
    bool mutates{false};
    bool executes{false};
    std::string diagnostic;
};

struct EditorAiPackagePayloadDiagnosticRow {
    std::string id;
    EditorAiPackageAuthoringDiagnosticStatus status{EditorAiPackageAuthoringDiagnosticStatus::blocked};
    std::string diagnostic;
};

struct EditorTilemapPackageDiagnosticRow {
    AssetId asset;
    std::string path;
    EditorAiPackageAuthoringDiagnosticStatus status{EditorAiPackageAuthoringDiagnosticStatus::blocked};
    std::size_t layer_count{0};
    std::size_t visible_layer_count{0};
    std::size_t tile_count{0};
    std::size_t non_empty_cell_count{0};
    std::size_t sampled_cell_count{0};
    std::size_t diagnostic_count{0};
    std::string diagnostic;
};

struct EditorTilemapPackageDiagnosticsModel {
    std::vector<EditorTilemapPackageDiagnosticRow> rows;
    bool has_tilemap_payloads{false};
    bool has_blocking_diagnostics{false};
    std::size_t sampled_cell_count{0};
    std::vector<std::string> diagnostics;
};

struct EditorAiPackageValidationRecipeDiagnosticRow {
    std::string id;
    EditorAiPackageAuthoringDiagnosticStatus status{EditorAiPackageAuthoringDiagnosticStatus::blocked};
    bool host_gated{false};
    bool executes{false};
    std::string diagnostic;
};

struct EditorAiPackageAuthoringDiagnosticsDesc {
    EditorPlaytestPackageReviewDesc playtest_review;
    std::vector<EditorAiPackageDescriptorDiagnosticRow> descriptor_rows;
    std::vector<EditorAiPackagePayloadDiagnosticRow> payload_rows;
    std::vector<EditorAiPackageValidationRecipeDiagnosticRow> validation_recipe_rows;
};

struct EditorAiPackageAuthoringDiagnosticsModel {
    EditorPlaytestPackageReviewModel playtest_review;
    std::vector<EditorAiPackageDescriptorDiagnosticRow> descriptor_rows;
    std::vector<EditorAiPackagePayloadDiagnosticRow> payload_rows;
    std::vector<EditorAiPackageValidationRecipeDiagnosticRow> validation_recipe_rows;
    bool has_blocking_diagnostics{false};
    bool has_host_gated_recipes{false};
    bool mutates{false};
    bool executes{false};
    std::vector<std::string> diagnostics;
};

struct EditorAiValidationRecipeDryRunPlanRow {
    std::string id;
    std::string command_display;
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    bool executes{false};
    std::string diagnostic;
    std::vector<std::string> argv;
};

struct EditorAiValidationRecipePreflightDesc {
    std::vector<std::string> manifest_validation_recipe_ids;
    std::vector<std::string> selected_validation_recipe_ids;
    std::vector<EditorAiValidationRecipeDryRunPlanRow> dry_run_plan_rows;
    bool request_arbitrary_shell_execution{false};
    bool request_raw_manifest_command_evaluation{false};
    bool request_package_script_execution{false};
    bool request_play_in_editor_productization{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_metal_readiness_claim{false};
    bool request_renderer_quality_claim{false};
};

struct EditorAiValidationRecipePreflightRow {
    std::string id;
    EditorAiPackageAuthoringDiagnosticStatus status{EditorAiPackageAuthoringDiagnosticStatus::blocked};
    bool manifest_declared{false};
    bool reviewed_dry_run_available{false};
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    std::string diagnostic;
    std::string command_display;
    std::vector<std::string> argv;
};

struct EditorAiValidationRecipePreflightModel {
    std::vector<EditorAiValidationRecipePreflightRow> rows;
    bool has_blocking_diagnostics{false};
    bool has_host_gated_recipes{false};
    bool executes{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorAiPlaytestReadinessReportRow {
    std::string id;
    std::string surface;
    EditorAiPackageAuthoringDiagnosticStatus status{EditorAiPackageAuthoringDiagnosticStatus::blocked};
    std::string diagnostic;
};

struct EditorAiPlaytestReadinessReportDesc {
    EditorAiPackageAuthoringDiagnosticsModel package_diagnostics;
    EditorAiValidationRecipePreflightModel validation_preflight;
    bool request_mutation{false};
    bool request_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_free_form_manifest_edit{false};
    bool request_play_in_editor_productization{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_metal_readiness_claim{false};
    bool request_renderer_quality_claim{false};
};

struct EditorAiPlaytestReadinessReportModel {
    std::vector<EditorAiPlaytestReadinessReportRow> rows;
    EditorAiPackageAuthoringDiagnosticStatus status{EditorAiPackageAuthoringDiagnosticStatus::blocked};
    bool ready_for_operator_validation{false};
    bool has_blocking_diagnostics{false};
    bool has_host_gates{false};
    bool mutates{false};
    bool executes{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorAiPlaytestOperatorHandoffCommandRow {
    std::string recipe_id;
    EditorAiPackageAuthoringDiagnosticStatus status{EditorAiPackageAuthoringDiagnosticStatus::blocked};
    std::string command_display;
    std::vector<std::string> argv;
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    std::string readiness_dependency;
    std::string diagnostic;
};

struct EditorAiPlaytestOperatorHandoffDesc {
    EditorAiPlaytestReadinessReportModel readiness_report;
    EditorAiValidationRecipePreflightModel validation_preflight;
    bool request_mutation{false};
    bool request_validation_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_raw_manifest_command_evaluation{false};
    bool request_package_script_execution{false};
    bool request_free_form_manifest_edit{false};
    bool request_play_in_editor_productization{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_metal_readiness_claim{false};
    bool request_renderer_quality_claim{false};
};

struct EditorAiPlaytestOperatorHandoffModel {
    std::vector<EditorAiPlaytestOperatorHandoffCommandRow> command_rows;
    EditorAiPackageAuthoringDiagnosticStatus status{EditorAiPackageAuthoringDiagnosticStatus::blocked};
    bool ready_for_operator_handoff{false};
    bool has_blocking_diagnostics{false};
    bool has_host_gates{false};
    bool mutates{false};
    bool executes{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorAiPlaytestValidationEvidenceRow {
    std::string recipe_id;
    EditorAiPlaytestEvidenceStatus status{EditorAiPlaytestEvidenceStatus::missing};
    std::optional<int> exit_code;
    std::string summary;
    std::string stdout_summary;
    std::string stderr_summary;
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    bool externally_supplied{true};
    bool claims_editor_core_execution{false};
};

struct EditorAiPlaytestEvidenceSummaryRow {
    std::string recipe_id;
    EditorAiPlaytestEvidenceStatus status{EditorAiPlaytestEvidenceStatus::missing};
    EditorAiPackageAuthoringDiagnosticStatus handoff_status{EditorAiPackageAuthoringDiagnosticStatus::blocked};
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

struct EditorAiPlaytestEvidenceSummaryDesc {
    EditorAiPlaytestOperatorHandoffModel operator_handoff;
    std::vector<EditorAiPlaytestValidationEvidenceRow> evidence_rows;
    bool request_mutation{false};
    bool request_validation_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_raw_manifest_command_evaluation{false};
    bool request_package_script_execution{false};
    bool request_free_form_manifest_edit{false};
    bool request_play_in_editor_productization{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_metal_readiness_claim{false};
    bool request_renderer_quality_claim{false};
};

struct EditorAiPlaytestEvidenceSummaryModel {
    std::vector<EditorAiPlaytestEvidenceSummaryRow> rows;
    EditorAiPlaytestEvidenceStatus status{EditorAiPlaytestEvidenceStatus::missing};
    bool all_required_evidence_passed{false};
    bool has_blocking_diagnostics{false};
    bool has_failed_evidence{false};
    bool has_missing_evidence{false};
    bool has_host_gates{false};
    bool mutates{false};
    bool executes{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorAiPlaytestEvidenceImportReviewRow {
    std::string recipe_id;
    EditorAiPlaytestEvidenceImportStatus status{EditorAiPlaytestEvidenceImportStatus::missing};
    EditorAiPlaytestEvidenceStatus evidence_status{EditorAiPlaytestEvidenceStatus::missing};
    std::string status_label;
    std::string evidence_status_label;
    bool expected{false};
    bool imported{false};
    std::optional<int> exit_code;
    std::string summary;
    std::string stdout_summary;
    std::string stderr_summary;
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    std::string diagnostic;
};

struct EditorAiPlaytestEvidenceImportDesc {
    std::string text;
    std::vector<std::string> expected_recipe_ids;
    bool request_mutation{false};
    bool request_validation_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_raw_manifest_command_evaluation{false};
    bool request_package_script_execution{false};
    bool request_free_form_manifest_edit{false};
    bool request_play_in_editor_productization{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_metal_readiness_claim{false};
    bool request_renderer_quality_claim{false};
};

struct EditorAiPlaytestEvidenceImportModel {
    std::vector<EditorAiPlaytestEvidenceImportReviewRow> review_rows;
    std::vector<EditorAiPlaytestValidationEvidenceRow> evidence_rows;
    EditorAiPlaytestEvidenceImportStatus status{EditorAiPlaytestEvidenceImportStatus::missing};
    std::string status_label;
    bool importable{false};
    bool has_blocking_diagnostics{false};
    bool has_missing_expected_evidence{false};
    bool has_unexpected_evidence{false};
    bool mutates{false};
    bool executes{false};
    std::size_t imported_count{0};
    std::size_t missing_expected_count{0};
    std::size_t blocked_count{0};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorAiPlaytestRemediationQueueRow {
    std::string recipe_id;
    EditorAiPlaytestEvidenceStatus evidence_status{EditorAiPlaytestEvidenceStatus::missing};
    EditorAiPlaytestRemediationCategory category{EditorAiPlaytestRemediationCategory::collect_missing_evidence};
    std::optional<int> exit_code;
    std::string evidence_summary;
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    std::string readiness_dependency;
    std::string next_action;
    std::string diagnostic;
};

struct EditorAiPlaytestRemediationQueueDesc {
    EditorAiPlaytestEvidenceSummaryModel evidence_summary;
    bool request_mutation{false};
    bool request_validation_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_raw_manifest_command_evaluation{false};
    bool request_package_script_execution{false};
    bool request_free_form_manifest_edit{false};
    bool request_evidence_mutation{false};
    bool request_fix_execution{false};
    bool request_play_in_editor_productization{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_metal_readiness_claim{false};
    bool request_renderer_quality_claim{false};
};

struct EditorAiPlaytestRemediationQueueModel {
    std::vector<EditorAiPlaytestRemediationQueueRow> rows;
    bool remediation_required{false};
    bool has_failed_evidence{false};
    bool has_blocked_evidence{false};
    bool has_missing_evidence{false};
    bool has_host_gates{false};
    bool has_blocking_diagnostics{false};
    bool mutates{false};
    bool executes{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorAiPlaytestRemediationHandoffRow {
    std::string recipe_id;
    EditorAiPlaytestEvidenceStatus evidence_status{EditorAiPlaytestEvidenceStatus::missing};
    EditorAiPlaytestRemediationCategory remediation_category{
        EditorAiPlaytestRemediationCategory::collect_missing_evidence};
    EditorAiPlaytestRemediationHandoffActionKind action_kind{
        EditorAiPlaytestRemediationHandoffActionKind::collect_external_evidence};
    std::string external_owner;
    std::string handoff_text;
    std::optional<int> exit_code;
    std::string evidence_summary;
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    std::string readiness_dependency;
    std::vector<std::string> unsupported_claims;
    std::string diagnostic;
};

struct EditorAiPlaytestRemediationHandoffDesc {
    EditorAiPlaytestRemediationQueueModel remediation_queue;
    bool request_mutation{false};
    bool request_validation_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_raw_manifest_command_evaluation{false};
    bool request_package_script_execution{false};
    bool request_free_form_manifest_edit{false};
    bool request_evidence_mutation{false};
    bool request_remediation_mutation{false};
    bool request_fix_execution{false};
    bool request_play_in_editor_productization{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_metal_readiness_claim{false};
    bool request_renderer_quality_claim{false};
};

struct EditorAiPlaytestRemediationHandoffModel {
    std::vector<EditorAiPlaytestRemediationHandoffRow> rows;
    bool handoff_required{false};
    bool has_failed_evidence{false};
    bool has_blocked_evidence{false};
    bool has_missing_evidence{false};
    bool has_host_gates{false};
    bool has_blocking_diagnostics{false};
    bool mutates{false};
    bool executes{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorAiPlaytestOperatorWorkflowStageRow {
    std::string id;
    std::string source_model;
    EditorAiPlaytestOperatorWorkflowStageStatus status{EditorAiPlaytestOperatorWorkflowStageStatus::blocked};
    std::size_t source_row_count{0};
    std::vector<std::string> source_row_ids;
    std::vector<std::string> host_gates;
    std::vector<std::string> blocked_by;
    std::string diagnostic;
};

struct EditorAiPlaytestOperatorWorkflowReportDesc {
    EditorAiPackageAuthoringDiagnosticsModel package_diagnostics;
    EditorAiValidationRecipePreflightModel validation_preflight;
    EditorAiPlaytestReadinessReportModel readiness_report;
    EditorAiPlaytestOperatorHandoffModel operator_handoff;
    EditorAiPlaytestEvidenceSummaryModel evidence_summary;
    EditorAiPlaytestRemediationQueueModel remediation_queue;
    EditorAiPlaytestRemediationHandoffModel remediation_handoff;
    bool request_mutation{false};
    bool request_manifest_mutation{false};
    bool request_validation_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_raw_manifest_command_evaluation{false};
    bool request_free_form_command_evaluation{false};
    bool request_package_script_execution{false};
    bool request_free_form_manifest_edit{false};
    bool request_evidence_mutation{false};
    bool request_remediation_mutation{false};
    bool request_fix_execution{false};
    bool request_play_in_editor_productization{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_metal_readiness_claim{false};
    bool request_renderer_quality_claim{false};
};

struct EditorAiPlaytestOperatorWorkflowReportModel {
    std::vector<EditorAiPlaytestOperatorWorkflowStageRow> stage_rows;
    bool closeout_complete{false};
    bool remediation_required{false};
    bool handoff_required{false};
    bool external_action_required{false};
    bool has_blocking_diagnostics{false};
    bool has_host_gates{false};
    bool mutates{false};
    bool executes{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorRuntimeScenePackageValidationExecutionDesc {
    EditorPlaytestPackageReviewModel playtest_review;
    bool request_package_cooking{false};
    bool request_runtime_source_parsing{false};
    bool request_external_importer_execution{false};
    bool request_renderer_rhi_residency{false};
    bool request_package_streaming{false};
    bool request_material_graph{false};
    bool request_shader_graph{false};
    bool request_live_shader_generation{false};
    bool request_broad_editor_productization{false};
    bool request_metal_readiness{false};
    bool request_public_native_rhi_handles{false};
    bool request_general_production_renderer_quality{false};
    bool request_arbitrary_shell_execution{false};
    bool request_free_form_edit{false};
};

struct EditorRuntimeScenePackageValidationExecutionModel {
    std::string id{"runtime-scene-package-validation"};
    EditorRuntimeScenePackageValidationExecutionStatus status{
        EditorRuntimeScenePackageValidationExecutionStatus::blocked};
    std::string status_label;
    bool can_execute{false};
    bool has_blocking_diagnostics{false};
    bool mutates{false};
    bool executes_external_process{false};
    std::optional<RuntimeSceneValidationTargetRow> selected_target;
    RuntimeScenePackageValidationRequest request;
    std::uint64_t package_record_count{0};
    std::uint32_t scene_node_count{0};
    std::size_t reference_count{0};
    std::vector<std::string> blocked_by;
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorRuntimeScenePackageValidationExecutionResult {
    EditorRuntimeScenePackageValidationExecutionStatus status{
        EditorRuntimeScenePackageValidationExecutionStatus::blocked};
    std::string status_label;
    EditorRuntimeScenePackageValidationExecutionModel model;
    RuntimeScenePackageValidationResult validation;
    bool evidence_available{false};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view editor_playtest_review_step_status_label(EditorPlaytestReviewStepStatus status) noexcept;

[[nodiscard]] std::string_view editor_runtime_scene_package_validation_execution_status_label(
    EditorRuntimeScenePackageValidationExecutionStatus status) noexcept;

[[nodiscard]] std::string_view
editor_ai_package_authoring_diagnostic_status_label(EditorAiPackageAuthoringDiagnosticStatus status) noexcept;

[[nodiscard]] std::string_view editor_ai_playtest_evidence_status_label(EditorAiPlaytestEvidenceStatus status) noexcept;

[[nodiscard]] std::string_view
editor_ai_playtest_evidence_import_status_label(EditorAiPlaytestEvidenceImportStatus status) noexcept;

[[nodiscard]] std::string_view
editor_ai_playtest_remediation_category_label(EditorAiPlaytestRemediationCategory category) noexcept;

[[nodiscard]] std::string_view editor_ai_playtest_remediation_handoff_action_kind_label(
    EditorAiPlaytestRemediationHandoffActionKind action_kind) noexcept;

[[nodiscard]] std::string_view
editor_ai_playtest_operator_workflow_stage_status_label(EditorAiPlaytestOperatorWorkflowStageStatus status) noexcept;

[[nodiscard]] EditorPlaytestPackageReviewModel
make_editor_playtest_package_review_model(const EditorPlaytestPackageReviewDesc& desc);

[[nodiscard]] EditorRuntimeScenePackageValidationExecutionModel
make_editor_runtime_scene_package_validation_execution_model(
    const EditorRuntimeScenePackageValidationExecutionDesc& desc);

[[nodiscard]] EditorRuntimeScenePackageValidationExecutionResult
execute_editor_runtime_scene_package_validation(IFileSystem& filesystem,
                                                const EditorRuntimeScenePackageValidationExecutionModel& model);

[[nodiscard]] mirakana::ui::UiDocument make_editor_runtime_scene_package_validation_execution_ui_model(
    const EditorRuntimeScenePackageValidationExecutionResult& result);

[[nodiscard]] EditorAiPackageAuthoringDiagnosticsModel
make_editor_ai_package_authoring_diagnostics_model(const EditorAiPackageAuthoringDiagnosticsDesc& desc);

[[nodiscard]] EditorAiValidationRecipePreflightModel
make_editor_ai_validation_recipe_preflight_model(const EditorAiValidationRecipePreflightDesc& desc);

[[nodiscard]] EditorAiPlaytestReadinessReportModel
make_editor_ai_playtest_readiness_report_model(const EditorAiPlaytestReadinessReportDesc& desc);

[[nodiscard]] EditorAiPlaytestOperatorHandoffModel
make_editor_ai_playtest_operator_handoff_model(const EditorAiPlaytestOperatorHandoffDesc& desc);

[[nodiscard]] EditorAiPlaytestEvidenceSummaryModel
make_editor_ai_playtest_evidence_summary_model(const EditorAiPlaytestEvidenceSummaryDesc& desc);

[[nodiscard]] EditorAiPlaytestEvidenceImportModel
make_editor_ai_playtest_evidence_import_model(const EditorAiPlaytestEvidenceImportDesc& desc);

[[nodiscard]] mirakana::ui::UiDocument
make_editor_ai_playtest_evidence_import_ui_model(const EditorAiPlaytestEvidenceImportModel& model);

[[nodiscard]] EditorAiPlaytestRemediationQueueModel
make_editor_ai_playtest_remediation_queue_model(const EditorAiPlaytestRemediationQueueDesc& desc);

[[nodiscard]] EditorAiPlaytestRemediationHandoffModel
make_editor_ai_playtest_remediation_handoff_model(const EditorAiPlaytestRemediationHandoffDesc& desc);

[[nodiscard]] EditorAiPlaytestOperatorWorkflowReportModel
make_editor_ai_playtest_operator_workflow_report_model(const EditorAiPlaytestOperatorWorkflowReportDesc& desc);

[[nodiscard]] EditorTilemapPackageDiagnosticsModel
make_editor_tilemap_package_diagnostics_model(const runtime::RuntimeAssetPackage& package);

} // namespace mirakana::editor
