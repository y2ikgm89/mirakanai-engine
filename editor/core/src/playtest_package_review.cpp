// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/playtest_package_review.hpp"

#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/runtime_diagnostics.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string ascii_path_key(std::string_view path) {
    std::string key;
    key.reserve(path.size());
    for (const auto ch : path) {
        key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch == '\\' ? '/' : ch))));
    }
    return key;
}

[[nodiscard]] const RuntimeSceneValidationTargetRow*
select_runtime_scene_validation_target(const std::vector<RuntimeSceneValidationTargetRow>& targets,
                                       std::string_view selected_id) {
    if (selected_id.empty() && targets.size() == 1U) {
        return &targets.front();
    }
    const auto match = std::ranges::find_if(
        targets, [selected_id](const RuntimeSceneValidationTargetRow& target) { return target.id == selected_id; });
    return match == targets.end() ? nullptr : &(*match);
}

void push_step(EditorPlaytestPackageReviewModel& model, std::string id, std::string surface,
               EditorPlaytestReviewStepStatus status, bool mutates, std::string diagnostic) {
    model.steps.push_back(EditorPlaytestPackageReviewStepRow{
        .id = std::move(id),
        .surface = std::move(surface),
        .status = status,
        .mutates = mutates,
        .diagnostic = std::move(diagnostic),
    });
}

[[nodiscard]] bool contains_string(const std::vector<std::string>& values, std::string_view value) {
    return std::ranges::any_of(values, [value](const std::string& candidate) { return candidate == value; });
}

[[nodiscard]] const EditorAiValidationRecipeDryRunPlanRow*
find_dry_run_plan(const std::vector<EditorAiValidationRecipeDryRunPlanRow>& rows, std::string_view recipe_id) {
    const auto match = std::ranges::find_if(
        rows, [recipe_id](const EditorAiValidationRecipeDryRunPlanRow& row) { return row.id == recipe_id; });
    return match == rows.end() ? nullptr : &(*match);
}

[[nodiscard]] const EditorAiPlaytestValidationEvidenceRow*
find_evidence_row(const std::vector<EditorAiPlaytestValidationEvidenceRow>& rows, std::string_view recipe_id) {
    const auto match = std::ranges::find_if(
        rows, [recipe_id](const EditorAiPlaytestValidationEvidenceRow& row) { return row.recipe_id == recipe_id; });
    return match == rows.end() ? nullptr : &(*match);
}

void append_diagnostic(std::string& target, std::string_view text) {
    if (!target.empty()) {
        target += "; ";
    }
    target += text;
}

void append_unique(std::vector<std::string>& target, const std::string& value) {
    if (!contains_string(target, value)) {
        target.push_back(value);
    }
}

void append_unique(std::vector<std::string>& target, const std::vector<std::string>& values) {
    for (const auto& value : values) {
        append_unique(target, value);
    }
}

void reject_unsupported_claim(EditorAiValidationRecipePreflightModel& model, std::string claim,
                              std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorAiPlaytestReadinessReportModel& model, std::string claim, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorAiPlaytestOperatorHandoffModel& model, std::string claim, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorAiPlaytestEvidenceSummaryModel& model, std::string claim, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorAiPlaytestEvidenceImportModel& model, std::string claim, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorAiPlaytestRemediationQueueModel& model, std::string claim, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorAiPlaytestRemediationHandoffModel& model, std::string claim,
                              std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorAiPlaytestOperatorWorkflowReportModel& model, std::string claim,
                              std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorRuntimeScenePackageValidationExecutionModel& model, std::string claim,
                              std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(std::move(diagnostic));
}

void append_validation_blocker(EditorRuntimeScenePackageValidationExecutionModel& model, std::string blocker,
                               std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.blocked_by.push_back(std::move(blocker));
    model.diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] EditorAiPackageAuthoringDiagnosticStatus aggregate_status(bool blocked, bool host_gated) noexcept {
    if (blocked) {
        return EditorAiPackageAuthoringDiagnosticStatus::blocked;
    }
    if (host_gated) {
        return EditorAiPackageAuthoringDiagnosticStatus::host_gated;
    }
    return EditorAiPackageAuthoringDiagnosticStatus::ready;
}

[[nodiscard]] EditorAiPlaytestEvidenceStatus aggregate_evidence_status(bool blocked, bool failed, bool missing,
                                                                       bool host_gated) noexcept {
    if (blocked) {
        return EditorAiPlaytestEvidenceStatus::blocked;
    }
    if (failed) {
        return EditorAiPlaytestEvidenceStatus::failed;
    }
    if (missing) {
        return EditorAiPlaytestEvidenceStatus::missing;
    }
    if (host_gated) {
        return EditorAiPlaytestEvidenceStatus::host_gated;
    }
    return EditorAiPlaytestEvidenceStatus::passed;
}

void push_readiness_row(EditorAiPlaytestReadinessReportModel& model, std::string id, std::string surface,
                        EditorAiPackageAuthoringDiagnosticStatus status, std::string diagnostic) {
    model.rows.push_back(EditorAiPlaytestReadinessReportRow{
        .id = std::move(id),
        .surface = std::move(surface),
        .status = status,
        .diagnostic = std::move(diagnostic),
    });
}

void push_workflow_stage(EditorAiPlaytestOperatorWorkflowReportModel& model,
                         EditorAiPlaytestOperatorWorkflowStageRow row) {
    if (row.status == EditorAiPlaytestOperatorWorkflowStageStatus::blocked) {
        model.has_blocking_diagnostics = true;
    }
    if (row.status == EditorAiPlaytestOperatorWorkflowStageStatus::host_gated || !row.host_gates.empty()) {
        model.has_host_gates = true;
    }
    model.stage_rows.push_back(std::move(row));
}

[[nodiscard]] EditorAiPlaytestOperatorWorkflowStageStatus
workflow_status_from_package_status(EditorAiPackageAuthoringDiagnosticStatus status, bool blocked,
                                    bool host_gated) noexcept {
    if (blocked || status == EditorAiPackageAuthoringDiagnosticStatus::blocked) {
        return EditorAiPlaytestOperatorWorkflowStageStatus::blocked;
    }
    if (host_gated || status == EditorAiPackageAuthoringDiagnosticStatus::host_gated) {
        return EditorAiPlaytestOperatorWorkflowStageStatus::host_gated;
    }
    return EditorAiPlaytestOperatorWorkflowStageStatus::ready;
}

[[nodiscard]] EditorAiPlaytestOperatorWorkflowStageStatus
workflow_status_from_evidence_summary(const EditorAiPlaytestEvidenceSummaryModel& model) noexcept {
    if (model.has_blocking_diagnostics || model.status == EditorAiPlaytestEvidenceStatus::blocked) {
        return EditorAiPlaytestOperatorWorkflowStageStatus::blocked;
    }
    if (model.has_failed_evidence || model.status == EditorAiPlaytestEvidenceStatus::failed) {
        return EditorAiPlaytestOperatorWorkflowStageStatus::remediation_required;
    }
    if (model.has_missing_evidence || model.status == EditorAiPlaytestEvidenceStatus::missing) {
        return EditorAiPlaytestOperatorWorkflowStageStatus::awaiting_external_evidence;
    }
    if (model.has_host_gates || model.status == EditorAiPlaytestEvidenceStatus::host_gated) {
        return EditorAiPlaytestOperatorWorkflowStageStatus::host_gated;
    }
    return EditorAiPlaytestOperatorWorkflowStageStatus::ready;
}

void append_model_diagnostics(EditorAiPlaytestOperatorWorkflowReportModel& target, std::string_view source,
                              const std::vector<std::string>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        target.diagnostics.push_back(std::string(source) + ": " + diagnostic);
    }
}

void append_model_unsupported_claims(EditorAiPlaytestOperatorWorkflowReportModel& target,
                                     const std::vector<std::string>& unsupported_claims) {
    for (const auto& claim : unsupported_claims) {
        append_unique(target.unsupported_claims, claim);
    }
}

void reject_readonly_input(EditorAiPlaytestOperatorWorkflowReportModel& target, std::string_view source, bool mutates,
                           bool executes) {
    if (mutates) {
        target.has_blocking_diagnostics = true;
        target.diagnostics.push_back(std::string(source) + " input must be read-only and must not mutate");
    }
    if (executes) {
        target.has_blocking_diagnostics = true;
        target.diagnostics.push_back(std::string(source) + " input must not execute");
    }
}

[[nodiscard]] std::string make_command_display_from_argv(const std::vector<std::string>& argv) {
    std::string display;
    for (const auto& part : argv) {
        if (!display.empty()) {
            display += ' ';
        }
        display += part;
    }
    return display;
}

[[nodiscard]] std::string make_evidence_summary(const EditorAiPlaytestValidationEvidenceRow& evidence) {
    if (!evidence.summary.empty()) {
        return evidence.summary;
    }
    if (!evidence.stdout_summary.empty()) {
        return evidence.stdout_summary;
    }
    return evidence.stderr_summary;
}

[[nodiscard]] std::string trim_text(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return std::string(value);
}

[[nodiscard]] std::string sanitize_import_text(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        if (character == '\n' || character == '\r') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text;
}

[[nodiscard]] std::string sanitize_element_id(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::isalnum(byte) != 0 || character == '_' || character == '-' || character == '.') {
            text.push_back(character);
        } else {
            text.push_back('_');
        }
    }
    return text.empty() ? "row" : text;
}

[[nodiscard]] std::vector<std::string> split_import_list(std::string_view value) {
    std::vector<std::string> values;
    while (!value.empty()) {
        const auto comma = value.find(',');
        const auto part = comma == std::string_view::npos ? value : value.substr(0, comma);
        auto item = trim_text(part);
        if (!item.empty()) {
            values.push_back(std::move(item));
        }
        if (comma == std::string_view::npos) {
            break;
        }
        value.remove_prefix(comma + 1U);
    }
    return values;
}

[[nodiscard]] std::optional<int> parse_import_exit_code(std::string_view value) {
    value = std::string_view{value.data(), value.size()};
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    if (value.empty()) {
        return std::nullopt;
    }

    int parsed = 0;
    const auto* first = value.data();
    const auto* last = value.data() + value.size();
    const auto result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc{} || result.ptr != last) {
        return std::nullopt;
    }
    return parsed;
}

[[nodiscard]] std::optional<bool> parse_import_bool(std::string_view value) {
    auto text = trim_text(value);
    std::ranges::transform(text, text.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (text == "true" || text == "1" || text == "yes") {
        return true;
    }
    if (text == "false" || text == "0" || text == "no") {
        return false;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<EditorAiPlaytestEvidenceStatus> parse_import_evidence_status(std::string_view value) {
    const auto text = trim_text(value);
    if (text == editor_ai_playtest_evidence_status_label(EditorAiPlaytestEvidenceStatus::passed)) {
        return EditorAiPlaytestEvidenceStatus::passed;
    }
    if (text == editor_ai_playtest_evidence_status_label(EditorAiPlaytestEvidenceStatus::failed)) {
        return EditorAiPlaytestEvidenceStatus::failed;
    }
    if (text == editor_ai_playtest_evidence_status_label(EditorAiPlaytestEvidenceStatus::blocked)) {
        return EditorAiPlaytestEvidenceStatus::blocked;
    }
    if (text == editor_ai_playtest_evidence_status_label(EditorAiPlaytestEvidenceStatus::host_gated)) {
        return EditorAiPlaytestEvidenceStatus::host_gated;
    }
    if (text == editor_ai_playtest_evidence_status_label(EditorAiPlaytestEvidenceStatus::missing)) {
        return EditorAiPlaytestEvidenceStatus::missing;
    }
    return std::nullopt;
}

struct PendingEvidenceImportRow {
    EditorAiPlaytestValidationEvidenceRow evidence;
    EditorAiPlaytestEvidenceImportReviewRow review;
    std::unordered_set<std::string> seen_fields;
    bool status_seen{false};
    bool blocked{false};
};

void add_import_blocker(PendingEvidenceImportRow& row, const std::string& code, const std::string& diagnostic) {
    append_unique(row.review.blocked_by, code);
    append_diagnostic(row.review.diagnostic, diagnostic);
    row.blocked = true;
}

void set_import_review_labels(EditorAiPlaytestEvidenceImportReviewRow& row) {
    row.status_label = std::string(editor_ai_playtest_evidence_import_status_label(row.status));
    row.evidence_status_label = std::string(editor_ai_playtest_evidence_status_label(row.evidence_status));
}

[[nodiscard]] mirakana::ui::TextContent make_ui_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

[[nodiscard]] mirakana::ui::ElementDesc make_ui_root(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_ui_child(std::string id, mirakana::ui::ElementId parent,
                                                      mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.parent = std::move(parent);
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

void add_ui_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("editor ai evidence import ui element could not be added");
    }
}

void append_ui_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                     std::string label) {
    mirakana::ui::ElementDesc desc = make_ui_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_ui_text(std::move(label));
    add_ui_or_throw(document, std::move(desc));
}

[[nodiscard]] EditorAiPlaytestRemediationCategory
remediation_category_for(EditorAiPlaytestEvidenceStatus status) noexcept {
    switch (status) {
    case EditorAiPlaytestEvidenceStatus::failed:
        return EditorAiPlaytestRemediationCategory::investigate_failure;
    case EditorAiPlaytestEvidenceStatus::blocked:
        return EditorAiPlaytestRemediationCategory::resolve_blocker;
    case EditorAiPlaytestEvidenceStatus::host_gated:
        return EditorAiPlaytestRemediationCategory::satisfy_host_gate;
    case EditorAiPlaytestEvidenceStatus::passed:
    case EditorAiPlaytestEvidenceStatus::missing:
        return EditorAiPlaytestRemediationCategory::collect_missing_evidence;
    }
    return EditorAiPlaytestRemediationCategory::collect_missing_evidence;
}

[[nodiscard]] std::string join_values(const std::vector<std::string>& values) {
    std::string joined;
    for (const auto& value : values) {
        if (!joined.empty()) {
            joined += ", ";
        }
        joined += value;
    }
    return joined;
}

[[nodiscard]] EditorAiPlaytestRemediationHandoffActionKind
handoff_action_kind_for(EditorAiPlaytestRemediationCategory category) noexcept {
    switch (category) {
    case EditorAiPlaytestRemediationCategory::investigate_failure:
        return EditorAiPlaytestRemediationHandoffActionKind::investigate_external_failure;
    case EditorAiPlaytestRemediationCategory::resolve_blocker:
        return EditorAiPlaytestRemediationHandoffActionKind::resolve_external_blocker;
    case EditorAiPlaytestRemediationCategory::collect_missing_evidence:
        return EditorAiPlaytestRemediationHandoffActionKind::collect_external_evidence;
    case EditorAiPlaytestRemediationCategory::satisfy_host_gate:
        return EditorAiPlaytestRemediationHandoffActionKind::satisfy_host_gate;
    }
    return EditorAiPlaytestRemediationHandoffActionKind::collect_external_evidence;
}

[[nodiscard]] std::string_view external_owner_for(EditorAiPlaytestRemediationHandoffActionKind action_kind) noexcept {
    switch (action_kind) {
    case EditorAiPlaytestRemediationHandoffActionKind::investigate_external_failure:
    case EditorAiPlaytestRemediationHandoffActionKind::collect_external_evidence:
        return "external-validation-operator";
    case EditorAiPlaytestRemediationHandoffActionKind::resolve_external_blocker:
        return "external-workflow-owner";
    case EditorAiPlaytestRemediationHandoffActionKind::satisfy_host_gate:
        return "host-environment-operator";
    }
    return "external-validation-operator";
}

[[nodiscard]] std::string make_remediation_next_action(const EditorAiPlaytestEvidenceSummaryRow& row,
                                                       EditorAiPlaytestRemediationCategory category) {
    switch (category) {
    case EditorAiPlaytestRemediationCategory::investigate_failure:
        return "Inspect external validation output for recipe '" + row.recipe_id +
               "', fix owning code or data through reviewed surfaces, then collect new external evidence";
    case EditorAiPlaytestRemediationCategory::resolve_blocker:
        return "Resolve blockers for recipe '" + row.recipe_id +
               "' through reviewed preflight, manifest, or package surfaces before collecting new evidence";
    case EditorAiPlaytestRemediationCategory::collect_missing_evidence:
        return "Collect externally supplied validation evidence for recipe '" + row.recipe_id +
               "' through the reviewed operator workflow";
    case EditorAiPlaytestRemediationCategory::satisfy_host_gate:
        if (!row.host_gates.empty()) {
            return "Run or defer recipe '" + row.recipe_id +
                   "' on a host satisfying host gates: " + join_values(row.host_gates);
        }
        return "Run or defer recipe '" + row.recipe_id + "' on a host satisfying its documented host gate requirements";
    }
    return "Collect externally supplied validation evidence for recipe '" + row.recipe_id +
           "' through the reviewed operator workflow";
}

[[nodiscard]] std::string make_remediation_handoff_text(const EditorAiPlaytestRemediationQueueRow& row,
                                                        EditorAiPlaytestRemediationHandoffActionKind action_kind) {
    std::string text = "Handoff recipe '" + row.recipe_id + "' to ";
    text += external_owner_for(action_kind);
    text += " outside editor core to ";

    switch (action_kind) {
    case EditorAiPlaytestRemediationHandoffActionKind::investigate_external_failure:
        text += "inspect external validation output and select a reviewed fix or authoring surface";
        break;
    case EditorAiPlaytestRemediationHandoffActionKind::resolve_external_blocker:
        text += "resolve blockers before collecting new external validation evidence";
        break;
    case EditorAiPlaytestRemediationHandoffActionKind::collect_external_evidence:
        text += "collect externally supplied validation evidence through the reviewed operator workflow";
        break;
    case EditorAiPlaytestRemediationHandoffActionKind::satisfy_host_gate:
        if (!row.host_gates.empty()) {
            text += "satisfy host gates: " + join_values(row.host_gates);
        } else {
            text += "satisfy the documented host gate requirements";
        }
        break;
    }

    if (!row.next_action.empty()) {
        text += "; remediation queue next action: ";
        text += row.next_action;
    }
    return text;
}

} // namespace

std::string_view editor_playtest_review_step_status_label(EditorPlaytestReviewStepStatus status) noexcept {
    switch (status) {
    case EditorPlaytestReviewStepStatus::ready:
        return "ready";
    case EditorPlaytestReviewStepStatus::blocked:
        return "blocked";
    case EditorPlaytestReviewStepStatus::host_gated:
        return "host_gated";
    }
    return "unknown";
}

std::string_view
editor_ai_package_authoring_diagnostic_status_label(EditorAiPackageAuthoringDiagnosticStatus status) noexcept {
    switch (status) {
    case EditorAiPackageAuthoringDiagnosticStatus::ready:
        return "ready";
    case EditorAiPackageAuthoringDiagnosticStatus::blocked:
        return "blocked";
    case EditorAiPackageAuthoringDiagnosticStatus::host_gated:
        return "host_gated";
    }
    return "unknown";
}

std::string_view editor_ai_playtest_evidence_status_label(EditorAiPlaytestEvidenceStatus status) noexcept {
    switch (status) {
    case EditorAiPlaytestEvidenceStatus::passed:
        return "passed";
    case EditorAiPlaytestEvidenceStatus::failed:
        return "failed";
    case EditorAiPlaytestEvidenceStatus::blocked:
        return "blocked";
    case EditorAiPlaytestEvidenceStatus::host_gated:
        return "host_gated";
    case EditorAiPlaytestEvidenceStatus::missing:
        return "missing";
    }
    return "unknown";
}

std::string_view editor_ai_playtest_evidence_import_status_label(EditorAiPlaytestEvidenceImportStatus status) noexcept {
    switch (status) {
    case EditorAiPlaytestEvidenceImportStatus::importable:
        return "importable";
    case EditorAiPlaytestEvidenceImportStatus::missing:
        return "missing";
    case EditorAiPlaytestEvidenceImportStatus::blocked:
        return "blocked";
    }
    return "unknown";
}

std::string_view editor_runtime_scene_package_validation_execution_status_label(
    EditorRuntimeScenePackageValidationExecutionStatus status) noexcept {
    switch (status) {
    case EditorRuntimeScenePackageValidationExecutionStatus::ready:
        return "ready";
    case EditorRuntimeScenePackageValidationExecutionStatus::blocked:
        return "blocked";
    case EditorRuntimeScenePackageValidationExecutionStatus::passed:
        return "passed";
    case EditorRuntimeScenePackageValidationExecutionStatus::failed:
        return "failed";
    }
    return "unknown";
}

std::string_view editor_ai_playtest_remediation_category_label(EditorAiPlaytestRemediationCategory category) noexcept {
    switch (category) {
    case EditorAiPlaytestRemediationCategory::investigate_failure:
        return "investigate_failure";
    case EditorAiPlaytestRemediationCategory::resolve_blocker:
        return "resolve_blocker";
    case EditorAiPlaytestRemediationCategory::collect_missing_evidence:
        return "collect_missing_evidence";
    case EditorAiPlaytestRemediationCategory::satisfy_host_gate:
        return "satisfy_host_gate";
    }
    return "unknown";
}

std::string_view editor_ai_playtest_remediation_handoff_action_kind_label(
    EditorAiPlaytestRemediationHandoffActionKind action_kind) noexcept {
    switch (action_kind) {
    case EditorAiPlaytestRemediationHandoffActionKind::investigate_external_failure:
        return "investigate_external_failure";
    case EditorAiPlaytestRemediationHandoffActionKind::resolve_external_blocker:
        return "resolve_external_blocker";
    case EditorAiPlaytestRemediationHandoffActionKind::collect_external_evidence:
        return "collect_external_evidence";
    case EditorAiPlaytestRemediationHandoffActionKind::satisfy_host_gate:
        return "satisfy_host_gate";
    }
    return "unknown";
}

std::string_view
editor_ai_playtest_operator_workflow_stage_status_label(EditorAiPlaytestOperatorWorkflowStageStatus status) noexcept {
    switch (status) {
    case EditorAiPlaytestOperatorWorkflowStageStatus::ready:
        return "ready";
    case EditorAiPlaytestOperatorWorkflowStageStatus::blocked:
        return "blocked";
    case EditorAiPlaytestOperatorWorkflowStageStatus::host_gated:
        return "host_gated";
    case EditorAiPlaytestOperatorWorkflowStageStatus::awaiting_external_evidence:
        return "awaiting_external_evidence";
    case EditorAiPlaytestOperatorWorkflowStageStatus::remediation_required:
        return "remediation_required";
    case EditorAiPlaytestOperatorWorkflowStageStatus::handoff_required:
        return "handoff_required";
    case EditorAiPlaytestOperatorWorkflowStageStatus::closed:
        return "closed";
    }
    return "unknown";
}

EditorPlaytestPackageReviewModel
make_editor_playtest_package_review_model(const EditorPlaytestPackageReviewDesc& desc) {
    EditorPlaytestPackageReviewModel model;

    std::unordered_set<std::string> registered_runtime_files;
    std::size_t rejected_runtime_candidates = 0U;
    for (const auto& row : desc.package_registration_draft_rows) {
        if (row.status == ScenePackageRegistrationDraftStatus::already_registered) {
            registered_runtime_files.insert(ascii_path_key(row.runtime_package_path));
        }
        if (row.status == ScenePackageRegistrationDraftStatus::add_runtime_file) {
            model.pending_runtime_package_files.push_back(row.runtime_package_path);
        }
        if (row.runtime_file && (row.status == ScenePackageRegistrationDraftStatus::rejected_unsafe_path ||
                                 row.status == ScenePackageRegistrationDraftStatus::rejected_duplicate)) {
            ++rejected_runtime_candidates;
        }
    }

    std::string review_diagnostic =
        "reviewed " + std::to_string(desc.package_registration_draft_rows.size()) + " package registration rows";
    if (rejected_runtime_candidates > 0U) {
        review_diagnostic += "; rejected runtime candidates require review";
        model.diagnostics.emplace_back("package registration draft contains rejected runtime candidates");
    }
    push_step(model, "review-editor-package-candidates",
              "ScenePackageCandidateRow and ScenePackageRegistrationDraftRow", EditorPlaytestReviewStepStatus::ready,
              false, std::move(review_diagnostic));

    const auto pending_count = model.pending_runtime_package_files.size();
    push_step(model, "apply-reviewed-runtime-package-files", "apply_scene_package_registration_to_manifest",
              EditorPlaytestReviewStepStatus::ready, true,
              pending_count == 0U ? "runtimePackageFiles already reviewed"
                                  : "Apply Package Registration for " + std::to_string(pending_count) +
                                        " reviewed runtimePackageFiles entries");

    const auto* selected_target = select_runtime_scene_validation_target(
        desc.runtime_scene_validation_targets, desc.selected_runtime_scene_validation_target_id);
    if (selected_target != nullptr) {
        model.selected_runtime_scene_validation_target = *selected_target;
    }

    std::string target_diagnostic;
    if (selected_target == nullptr) {
        target_diagnostic = "runtimeSceneValidationTargets selection is missing";
    } else if (!model.pending_runtime_package_files.empty()) {
        target_diagnostic = "Apply Package Registration before selecting runtimeSceneValidationTargets for validation";
    } else if (selected_target->scene_asset_key.empty()) {
        target_diagnostic = "runtimeSceneValidationTargets sceneAssetKey is empty";
    } else if (!registered_runtime_files.contains(ascii_path_key(selected_target->package_index_path))) {
        target_diagnostic = "runtimeSceneValidationTargets packageIndexPath is not registered in runtimePackageFiles";
    } else {
        target_diagnostic = "runtimeSceneValidationTargets row selected";
        model.ready_for_runtime_scene_validation = true;
    }

    push_step(model, "select-runtime-scene-validation-target", "game.agent.json.runtimeSceneValidationTargets",
              model.ready_for_runtime_scene_validation ? EditorPlaytestReviewStepStatus::ready
                                                       : EditorPlaytestReviewStepStatus::blocked,
              false, target_diagnostic);

    push_step(model, "validate-runtime-scene-package", "validate-runtime-scene-package",
              model.ready_for_runtime_scene_validation ? EditorPlaytestReviewStepStatus::ready
                                                       : EditorPlaytestReviewStepStatus::blocked,
              false,
              model.ready_for_runtime_scene_validation
                  ? "run non-mutating runtime scene package validation before desktop smoke"
                  : "runtime scene package validation is blocked until a manifest target is ready");

    model.host_gated_desktop_smoke_available =
        model.ready_for_runtime_scene_validation && !desc.host_gated_smoke_recipes.empty();
    push_step(model, "run-host-gated-desktop-smoke", "run-validation-recipe or package-desktop-runtime",
              model.host_gated_desktop_smoke_available ? EditorPlaytestReviewStepStatus::host_gated
                                                       : EditorPlaytestReviewStepStatus::blocked,
              false,
              model.host_gated_desktop_smoke_available
                  ? "desktop smoke remains host-gated and separate from package validation"
                  : "desktop smoke is blocked until runtime scene validation is ready and a smoke recipe is selected");

    return model;
}

EditorRuntimeScenePackageValidationExecutionModel make_editor_runtime_scene_package_validation_execution_model(
    const EditorRuntimeScenePackageValidationExecutionDesc& desc) {
    EditorRuntimeScenePackageValidationExecutionModel model;

    const auto reject_requested_claims = [&model, &desc]() {
        if (desc.request_package_cooking) {
            reject_unsupported_claim(model, "package cooking",
                                     "runtime scene package validation execution does not cook packages");
        }
        if (desc.request_runtime_source_parsing) {
            reject_unsupported_claim(model, "runtime source parsing",
                                     "runtime scene package validation execution does not parse source assets");
        }
        if (desc.request_external_importer_execution) {
            reject_unsupported_claim(model, "external importer execution",
                                     "runtime scene package validation execution does not run importers");
        }
        if (desc.request_renderer_rhi_residency) {
            reject_unsupported_claim(model, "renderer/RHI residency",
                                     "runtime scene package validation execution does not create renderer residency");
        }
        if (desc.request_package_streaming) {
            reject_unsupported_claim(model, "package streaming",
                                     "runtime scene package validation execution does not stream packages");
        }
        if (desc.request_material_graph) {
            reject_unsupported_claim(model, "material graph",
                                     "runtime scene package validation execution does not make material graphs ready");
        }
        if (desc.request_shader_graph) {
            reject_unsupported_claim(model, "shader graph",
                                     "runtime scene package validation execution does not make shader graphs ready");
        }
        if (desc.request_live_shader_generation) {
            reject_unsupported_claim(model, "live shader generation",
                                     "runtime scene package validation execution does not generate shaders");
        }
        if (desc.request_broad_editor_productization) {
            reject_unsupported_claim(model, "broad editor productization",
                                     "runtime scene package validation execution is not a broad editor ready claim");
        }
        if (desc.request_metal_readiness) {
            reject_unsupported_claim(model, "Metal readiness",
                                     "runtime scene package validation execution does not make Metal ready");
        }
        if (desc.request_public_native_rhi_handles) {
            reject_unsupported_claim(
                model, "public native/RHI handles",
                "runtime scene package validation execution does not expose native or RHI handles");
        }
        if (desc.request_general_production_renderer_quality) {
            reject_unsupported_claim(
                model, "general production renderer quality",
                "runtime scene package validation execution does not claim general renderer quality");
        }
        if (desc.request_arbitrary_shell_execution) {
            reject_unsupported_claim(model, "arbitrary shell",
                                     "runtime scene package validation execution rejects arbitrary shell execution");
        }
        if (desc.request_free_form_edit) {
            reject_unsupported_claim(model, "free-form edits",
                                     "runtime scene package validation execution rejects free-form edits");
        }
    };

    reject_requested_claims();

    if (!desc.playtest_review.ready_for_runtime_scene_validation) {
        append_validation_blocker(model, "runtime-scene-validation-not-ready",
                                  "playtest package review is not ready for runtime scene validation");
    }
    if (!desc.playtest_review.pending_runtime_package_files.empty()) {
        append_validation_blocker(model, "pending-runtime-package-files",
                                  "apply reviewed runtimePackageFiles before runtime scene validation");
    }
    if (!desc.playtest_review.selected_runtime_scene_validation_target.has_value()) {
        append_validation_blocker(model, "missing-runtime-scene-validation-target",
                                  "runtimeSceneValidationTargets selection is missing");
    } else {
        model.selected_target = desc.playtest_review.selected_runtime_scene_validation_target;
        const auto& target = *model.selected_target;
        model.id = sanitize_element_id(target.id);
        model.request.kind = RuntimeScenePackageValidationCommandKind::validate_runtime_scene_package;
        model.request.package_index_path = target.package_index_path;
        model.request.content_root = target.content_root;
        model.request.scene_asset_key = AssetKeyV2{target.scene_asset_key};
        model.request.validate_asset_references = target.validate_asset_references;
        model.request.require_unique_node_names = target.require_unique_node_names;

        const auto request_plan = mirakana::plan_runtime_scene_package_validation(model.request);
        for (const auto& diagnostic : request_plan.diagnostics) {
            append_validation_blocker(model, diagnostic.code,
                                      diagnostic.message.empty() ? "runtime scene package validation request is invalid"
                                                                 : diagnostic.message);
        }
    }

    if (model.has_blocking_diagnostics) {
        model.status = EditorRuntimeScenePackageValidationExecutionStatus::blocked;
    } else {
        model.status = EditorRuntimeScenePackageValidationExecutionStatus::ready;
        model.can_execute = true;
        model.diagnostics.emplace_back("runtime scene package validation can execute non-mutating package validation");
    }
    model.status_label = std::string(editor_runtime_scene_package_validation_execution_status_label(model.status));
    return model;
}

EditorRuntimeScenePackageValidationExecutionResult
execute_editor_runtime_scene_package_validation(IFileSystem& filesystem,
                                                const EditorRuntimeScenePackageValidationExecutionModel& model) {
    EditorRuntimeScenePackageValidationExecutionResult result;
    result.model = model;

    if (!model.can_execute) {
        result.status = EditorRuntimeScenePackageValidationExecutionStatus::blocked;
        result.status_label =
            std::string(editor_runtime_scene_package_validation_execution_status_label(result.status));
        result.diagnostics = model.diagnostics;
        if (result.diagnostics.empty()) {
            result.diagnostics.emplace_back("runtime scene package validation execution is not ready");
        }
        return result;
    }

    try {
        result.validation = mirakana::execute_runtime_scene_package_validation(filesystem, model.request);
        result.evidence_available = true;
        result.status = result.validation.succeeded() ? EditorRuntimeScenePackageValidationExecutionStatus::passed
                                                      : EditorRuntimeScenePackageValidationExecutionStatus::failed;
        result.model.package_record_count = result.validation.summary.package_record_count;
        result.model.scene_node_count = result.validation.summary.scene_node_count;
        result.model.reference_count = result.validation.summary.references.size();
        if (result.validation.succeeded()) {
            result.diagnostics.emplace_back("runtime scene package validation passed");
        } else {
            for (const auto& diagnostic : result.validation.diagnostics) {
                result.diagnostics.push_back(diagnostic.code + ": " + diagnostic.message);
            }
        }
    } catch (const std::exception& error) {
        result.status = EditorRuntimeScenePackageValidationExecutionStatus::blocked;
        result.diagnostics.emplace_back(error.what());
    }

    result.status_label = std::string(editor_runtime_scene_package_validation_execution_status_label(result.status));
    result.model.status = result.status;
    result.model.status_label = result.status_label;
    result.model.can_execute = result.status == EditorRuntimeScenePackageValidationExecutionStatus::ready;
    return result;
}

mirakana::ui::UiDocument make_editor_runtime_scene_package_validation_execution_ui_model(
    const EditorRuntimeScenePackageValidationExecutionResult& result) {
    mirakana::ui::UiDocument document;
    add_ui_or_throw(
        document, make_ui_root("playtest_package_review.runtime_scene_validation", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"playtest_package_review.runtime_scene_validation"};

    const auto target_id = result.model.selected_target.has_value()
                               ? sanitize_element_id(result.model.selected_target->id)
                               : std::string{"selected-target"};
    const auto row_id = "playtest_package_review.runtime_scene_validation." + target_id;
    auto row = make_ui_child(row_id, root, mirakana::ui::SemanticRole::list_item);
    row.text = make_ui_text(target_id);
    add_ui_or_throw(document, std::move(row));
    const mirakana::ui::ElementId row_root{row_id};

    append_ui_label(document, row_root, row_id + ".status",
                    result.status_label.empty() ? result.model.status_label : result.status_label);
    append_ui_label(document, row_root, row_id + ".package",
                    result.model.request.package_index_path.empty() ? "-" : result.model.request.package_index_path);
    append_ui_label(document, row_root, row_id + ".scene_asset_key",
                    result.model.request.scene_asset_key.value.empty() ? "-"
                                                                       : result.model.request.scene_asset_key.value);
    append_ui_label(document, row_root, row_id + ".content_root",
                    result.model.request.content_root.empty() ? "-" : result.model.request.content_root);
    append_ui_label(document, row_root, row_id + ".package_records",
                    std::to_string(result.validation.summary.package_record_count != 0U
                                       ? result.validation.summary.package_record_count
                                       : result.model.package_record_count));
    append_ui_label(document, row_root, row_id + ".scene_nodes",
                    std::to_string(result.validation.summary.scene_node_count != 0U
                                       ? result.validation.summary.scene_node_count
                                       : result.model.scene_node_count));
    append_ui_label(document, row_root, row_id + ".references",
                    std::to_string(!result.validation.summary.references.empty()
                                       ? result.validation.summary.references.size()
                                       : result.model.reference_count));
    append_ui_label(document, row_root, row_id + ".blocked_by", join_values(result.model.blocked_by));
    append_ui_label(document, row_root, row_id + ".unsupported", join_values(result.model.unsupported_claims));
    append_ui_label(document, row_root, row_id + ".diagnostics",
                    join_values(result.diagnostics.empty() ? result.model.diagnostics : result.diagnostics));

    return document;
}

EditorAiValidationRecipePreflightModel
make_editor_ai_validation_recipe_preflight_model(const EditorAiValidationRecipePreflightDesc& desc) {
    EditorAiValidationRecipePreflightModel model;

    const auto& selected_recipe_ids = desc.selected_validation_recipe_ids.empty() ? desc.manifest_validation_recipe_ids
                                                                                  : desc.selected_validation_recipe_ids;
    if (selected_recipe_ids.empty()) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("validation recipe preflight has no selected manifest validation recipes");
    }

    for (const auto& recipe_id : selected_recipe_ids) {
        EditorAiValidationRecipePreflightRow row;
        row.id = recipe_id;
        row.manifest_declared = contains_string(desc.manifest_validation_recipe_ids, recipe_id);

        if (!row.manifest_declared) {
            row.blocked_by.emplace_back("not-declared-in-game-agent-validationRecipes");
            append_diagnostic(row.diagnostic, "validation recipe is not declared in game.agent.json.validationRecipes");
        }

        const auto* dry_run_plan = find_dry_run_plan(desc.dry_run_plan_rows, recipe_id);
        row.reviewed_dry_run_available = dry_run_plan != nullptr;
        if (dry_run_plan == nullptr) {
            row.blocked_by.emplace_back("missing-reviewed-dry-run-plan");
            append_diagnostic(row.diagnostic, "reviewed run-validation-recipe dry-run plan is missing");
        } else {
            row.command_display = dry_run_plan->command_display;
            row.argv = dry_run_plan->argv;
            row.host_gates = dry_run_plan->host_gates;
            row.blocked_by.insert(row.blocked_by.end(), dry_run_plan->blocked_by.begin(),
                                  dry_run_plan->blocked_by.end());
            if (!dry_run_plan->diagnostic.empty()) {
                append_diagnostic(row.diagnostic, dry_run_plan->diagnostic);
            }
            if (dry_run_plan->executes) {
                row.blocked_by.emplace_back("execution-result-supplied");
                append_diagnostic(row.diagnostic, "validation recipe preflight is preflight-only and must not execute");
            }
        }

        if (!row.blocked_by.empty()) {
            row.status = EditorAiPackageAuthoringDiagnosticStatus::blocked;
            model.has_blocking_diagnostics = true;
            model.diagnostics.push_back("validation recipe '" + row.id + "' is blocked: " + row.diagnostic);
        } else if (!row.host_gates.empty()) {
            row.status = EditorAiPackageAuthoringDiagnosticStatus::host_gated;
            model.has_host_gated_recipes = true;
        } else {
            row.status = EditorAiPackageAuthoringDiagnosticStatus::ready;
        }

        model.rows.push_back(std::move(row));
    }

    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell",
                                 "validation recipe preflight rejects arbitrary shell execution");
    }
    if (desc.request_raw_manifest_command_evaluation) {
        reject_unsupported_claim(model, "raw manifest command",
                                 "validation recipe preflight rejects raw manifest command evaluation");
    }
    if (desc.request_package_script_execution) {
        reject_unsupported_claim(model, "package script execution",
                                 "validation recipe preflight rejects package script execution from editor core");
    }
    if (desc.request_play_in_editor_productization) {
        reject_unsupported_claim(model, "play-in-editor productization",
                                 "validation recipe preflight rejects play-in-editor productization");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "validation recipe preflight rejects renderer/RHI handle exposure");
    }
    if (desc.request_metal_readiness_claim) {
        reject_unsupported_claim(
            model, "Metal readiness",
            "validation recipe preflight rejects Metal readiness claims without Apple-host evidence");
    }
    if (desc.request_renderer_quality_claim) {
        reject_unsupported_claim(model, "renderer quality",
                                 "validation recipe preflight rejects renderer quality claims");
    }

    return model;
}

EditorAiPlaytestReadinessReportModel
make_editor_ai_playtest_readiness_report_model(const EditorAiPlaytestReadinessReportDesc& desc) {
    EditorAiPlaytestReadinessReportModel model;

    const auto package_status = aggregate_status(desc.package_diagnostics.has_blocking_diagnostics,
                                                 desc.package_diagnostics.has_host_gated_recipes);
    push_readiness_row(model, "package-authoring-diagnostics", "EditorAiPackageAuthoringDiagnosticsModel",
                       package_status,
                       desc.package_diagnostics.has_blocking_diagnostics
                           ? "package diagnostics blocked"
                           : (desc.package_diagnostics.has_host_gated_recipes ? "package diagnostics are host-gated"
                                                                              : "package diagnostics are ready"));
    if (desc.package_diagnostics.has_blocking_diagnostics) {
        model.has_blocking_diagnostics = true;
    }
    if (desc.package_diagnostics.has_host_gated_recipes) {
        model.has_host_gates = true;
    }
    if (desc.package_diagnostics.mutates) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("package diagnostics input must be read-only and must not mutate");
    }
    if (desc.package_diagnostics.executes) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("package diagnostics input must be read-only and must not execute");
    }
    for (const auto& diagnostic : desc.package_diagnostics.diagnostics) {
        model.diagnostics.push_back("package diagnostics: " + diagnostic);
    }

    const auto preflight_status = aggregate_status(desc.validation_preflight.has_blocking_diagnostics,
                                                   desc.validation_preflight.has_host_gated_recipes);
    push_readiness_row(model, "validation-recipe-preflight", "EditorAiValidationRecipePreflightModel", preflight_status,
                       desc.validation_preflight.has_blocking_diagnostics
                           ? "validation preflight blocked"
                           : (desc.validation_preflight.has_host_gated_recipes ? "validation preflight is host-gated"
                                                                               : "validation preflight is ready"));
    if (desc.validation_preflight.has_blocking_diagnostics) {
        model.has_blocking_diagnostics = true;
    }
    if (desc.validation_preflight.has_host_gated_recipes) {
        model.has_host_gates = true;
    }
    if (desc.validation_preflight.executes) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("validation preflight input must be read-only and must not execute");
    }
    for (const auto& diagnostic : desc.validation_preflight.diagnostics) {
        model.diagnostics.push_back("validation preflight: " + diagnostic);
    }

    if (desc.request_mutation) {
        reject_unsupported_claim(
            model, "mutation", "playtest readiness report is read-only and must not mutate manifests or package data");
    }
    if (desc.request_execution) {
        reject_unsupported_claim(model, "execution",
                                 "playtest readiness report is read-only and must not execute validation commands");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell",
                                 "playtest readiness report rejects arbitrary shell execution");
    }
    if (desc.request_free_form_manifest_edit) {
        reject_unsupported_claim(model, "free-form manifest edits",
                                 "playtest readiness report rejects free-form manifest edits");
    }
    if (desc.request_play_in_editor_productization) {
        reject_unsupported_claim(model, "play-in-editor productization",
                                 "playtest readiness report rejects play-in-editor productization");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "playtest readiness report rejects renderer/RHI handle exposure");
    }
    if (desc.request_metal_readiness_claim) {
        reject_unsupported_claim(
            model, "Metal readiness",
            "playtest readiness report rejects Metal readiness claims without Apple-host evidence");
    }
    if (desc.request_renderer_quality_claim) {
        reject_unsupported_claim(model, "renderer quality",
                                 "playtest readiness report rejects renderer quality claims");
    }

    model.status = aggregate_status(model.has_blocking_diagnostics, model.has_host_gates);
    model.ready_for_operator_validation = !model.has_blocking_diagnostics;
    return model;
}

EditorAiPlaytestOperatorHandoffModel
make_editor_ai_playtest_operator_handoff_model(const EditorAiPlaytestOperatorHandoffDesc& desc) {
    EditorAiPlaytestOperatorHandoffModel model;

    constexpr std::string_view readiness_dependency =
        "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation";
    const bool readiness_blocked =
        desc.readiness_report.has_blocking_diagnostics || !desc.readiness_report.ready_for_operator_validation;

    if (desc.validation_preflight.rows.empty()) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("operator handoff has no validation recipe preflight rows");
    }

    if (readiness_blocked) {
        model.has_blocking_diagnostics = true;
        if (desc.readiness_report.diagnostics.empty()) {
            model.diagnostics.emplace_back("readiness report blocked before operator handoff");
        }
    }
    if (desc.readiness_report.has_host_gates || desc.validation_preflight.has_host_gated_recipes) {
        model.has_host_gates = true;
    }
    for (const auto& diagnostic : desc.readiness_report.diagnostics) {
        model.diagnostics.push_back("readiness report: " + diagnostic);
    }

    if (desc.validation_preflight.has_blocking_diagnostics) {
        model.has_blocking_diagnostics = true;
    }
    if (desc.validation_preflight.executes) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("validation preflight input must be read-only and must not execute");
    }
    for (const auto& diagnostic : desc.validation_preflight.diagnostics) {
        model.diagnostics.push_back("validation preflight: " + diagnostic);
    }

    for (const auto& row : desc.validation_preflight.rows) {
        EditorAiPlaytestOperatorHandoffCommandRow command_row;
        command_row.recipe_id = row.id;
        command_row.status = row.status;
        command_row.command_display =
            row.command_display.empty() ? make_command_display_from_argv(row.argv) : row.command_display;
        command_row.argv = row.argv;
        command_row.host_gates = row.host_gates;
        command_row.blocked_by = row.blocked_by;
        command_row.readiness_dependency = std::string(readiness_dependency);
        command_row.diagnostic = row.diagnostic;

        if (readiness_blocked) {
            command_row.blocked_by.emplace_back("readiness-report-blocked");
            append_diagnostic(command_row.diagnostic, "readiness report must be ready before operator handoff");
            command_row.status = EditorAiPackageAuthoringDiagnosticStatus::blocked;
            model.has_blocking_diagnostics = true;
        }
        if (row.reviewed_dry_run_available && command_row.command_display.empty() && command_row.argv.empty()) {
            command_row.blocked_by.emplace_back("missing-reviewed-command-display-or-argv-plan");
            append_diagnostic(command_row.diagnostic, "reviewed dry-run command display or argv plan data is missing");
            command_row.status = EditorAiPackageAuthoringDiagnosticStatus::blocked;
            model.has_blocking_diagnostics = true;
        }
        if (!command_row.blocked_by.empty() ||
            command_row.status == EditorAiPackageAuthoringDiagnosticStatus::blocked) {
            model.has_blocking_diagnostics = true;
        }
        if (!command_row.host_gates.empty() ||
            command_row.status == EditorAiPackageAuthoringDiagnosticStatus::host_gated) {
            model.has_host_gates = true;
        }

        model.command_rows.push_back(std::move(command_row));
    }

    if (desc.request_mutation) {
        reject_unsupported_claim(model, "mutation",
                                 "operator handoff rejects mutation and must not mutate manifests or package data");
    }
    if (desc.request_validation_execution) {
        reject_unsupported_claim(model, "validation execution",
                                 "operator handoff rejects validation execution from editor core");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell", "operator handoff rejects arbitrary shell execution");
    }
    if (desc.request_raw_manifest_command_evaluation) {
        reject_unsupported_claim(model, "raw manifest command",
                                 "operator handoff rejects raw manifest command evaluation");
    }
    if (desc.request_package_script_execution) {
        reject_unsupported_claim(model, "package script execution",
                                 "operator handoff rejects package script execution from editor core");
    }
    if (desc.request_free_form_manifest_edit) {
        reject_unsupported_claim(model, "free-form manifest edits",
                                 "operator handoff rejects free-form manifest edits");
    }
    if (desc.request_play_in_editor_productization) {
        reject_unsupported_claim(model, "play-in-editor productization",
                                 "operator handoff rejects play-in-editor productization");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "operator handoff rejects renderer/RHI handle exposure");
    }
    if (desc.request_metal_readiness_claim) {
        reject_unsupported_claim(model, "Metal readiness",
                                 "operator handoff rejects Metal readiness claims without Apple-host evidence");
    }
    if (desc.request_renderer_quality_claim) {
        reject_unsupported_claim(model, "renderer quality", "operator handoff rejects renderer quality claims");
    }

    model.status = aggregate_status(model.has_blocking_diagnostics, model.has_host_gates);
    model.ready_for_operator_handoff = !model.has_blocking_diagnostics;
    return model;
}

EditorAiPlaytestEvidenceSummaryModel
make_editor_ai_playtest_evidence_summary_model(const EditorAiPlaytestEvidenceSummaryDesc& desc) {
    EditorAiPlaytestEvidenceSummaryModel model;

    if (desc.operator_handoff.command_rows.empty()) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("playtest evidence summary has no operator handoff command rows");
    }
    if (desc.operator_handoff.has_blocking_diagnostics || !desc.operator_handoff.ready_for_operator_handoff) {
        model.has_blocking_diagnostics = true;
        if (desc.operator_handoff.diagnostics.empty()) {
            model.diagnostics.emplace_back("operator handoff is blocked before evidence summary");
        }
    }
    if (desc.operator_handoff.has_host_gates) {
        model.has_host_gates = true;
    }
    if (desc.operator_handoff.mutates) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("operator handoff input must be read-only and must not mutate");
    }
    if (desc.operator_handoff.executes) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("operator handoff input must be read-only and must not execute");
    }
    for (const auto& diagnostic : desc.operator_handoff.diagnostics) {
        model.diagnostics.push_back("operator handoff: " + diagnostic);
    }

    std::unordered_set<std::string> handoff_recipe_ids;
    for (const auto& handoff_row : desc.operator_handoff.command_rows) {
        handoff_recipe_ids.insert(handoff_row.recipe_id);

        EditorAiPlaytestEvidenceSummaryRow row;
        row.recipe_id = handoff_row.recipe_id;
        row.handoff_status = handoff_row.status;
        row.handoff_row_available = true;
        row.host_gates = handoff_row.host_gates;
        row.blocked_by = handoff_row.blocked_by;
        row.readiness_dependency = handoff_row.readiness_dependency;
        row.diagnostic = handoff_row.diagnostic;

        const auto* evidence = find_evidence_row(desc.evidence_rows, handoff_row.recipe_id);
        if (evidence == nullptr) {
            row.status = EditorAiPlaytestEvidenceStatus::missing;
            append_unique(row.blocked_by, "missing-external-validation-evidence");
            append_diagnostic(row.diagnostic, "externally supplied validation evidence is missing");
            model.has_missing_evidence = true;
        } else {
            row.status = evidence->status;
            row.exit_code = evidence->exit_code;
            row.summary = make_evidence_summary(*evidence);
            row.stdout_summary = evidence->stdout_summary;
            row.stderr_summary = evidence->stderr_summary;
            append_unique(row.host_gates, evidence->host_gates);
            append_unique(row.blocked_by, evidence->blocked_by);
            if (!row.summary.empty()) {
                append_diagnostic(row.diagnostic, row.summary);
            }

            if (!evidence->externally_supplied) {
                row.status = EditorAiPlaytestEvidenceStatus::blocked;
                append_unique(row.blocked_by, "evidence-not-externally-supplied");
                append_diagnostic(row.diagnostic, "playtest validation evidence must be externally supplied");
                model.has_blocking_diagnostics = true;
                model.diagnostics.push_back("evidence row '" + row.recipe_id + "' must be externally supplied");
            }
            if (evidence->claims_editor_core_execution) {
                row.status = EditorAiPlaytestEvidenceStatus::blocked;
                append_unique(row.blocked_by, "editor-core-validation-execution-claim");
                append_diagnostic(row.diagnostic, "editor core must not execute validation evidence collection");
                model.has_blocking_diagnostics = true;
                model.diagnostics.push_back("evidence row '" + row.recipe_id +
                                            "' claims editor-core validation execution");
            }
            if (evidence->status == EditorAiPlaytestEvidenceStatus::passed && evidence->exit_code.has_value() &&
                evidence->exit_code.value() != 0) {
                row.status = EditorAiPlaytestEvidenceStatus::failed;
                append_unique(row.blocked_by, "nonzero-exit-code");
                append_diagnostic(row.diagnostic, "passed evidence reported a non-zero exit code");
            }
        }

        if (handoff_row.status == EditorAiPackageAuthoringDiagnosticStatus::blocked) {
            row.status = EditorAiPlaytestEvidenceStatus::blocked;
            append_unique(row.blocked_by, "operator-handoff-blocked");
            append_diagnostic(row.diagnostic, "operator handoff row is blocked");
            model.has_blocking_diagnostics = true;
        }
        if (row.status == EditorAiPlaytestEvidenceStatus::failed) {
            model.has_failed_evidence = true;
        }
        if (row.status == EditorAiPlaytestEvidenceStatus::missing) {
            model.has_missing_evidence = true;
        }
        if (row.status == EditorAiPlaytestEvidenceStatus::blocked) {
            model.has_blocking_diagnostics = true;
        }
        if (row.status == EditorAiPlaytestEvidenceStatus::host_gated || !row.host_gates.empty()) {
            model.has_host_gates = true;
        }

        model.rows.push_back(std::move(row));
    }

    for (const auto& evidence : desc.evidence_rows) {
        if (handoff_recipe_ids.contains(evidence.recipe_id)) {
            continue;
        }

        EditorAiPlaytestEvidenceSummaryRow row;
        row.recipe_id = evidence.recipe_id;
        row.status = EditorAiPlaytestEvidenceStatus::blocked;
        row.handoff_row_available = false;
        row.exit_code = evidence.exit_code;
        row.summary = make_evidence_summary(evidence);
        row.stdout_summary = evidence.stdout_summary;
        row.stderr_summary = evidence.stderr_summary;
        row.host_gates = evidence.host_gates;
        row.blocked_by = evidence.blocked_by;
        append_unique(row.blocked_by, "missing-operator-handoff-row");
        row.diagnostic = row.summary;
        append_diagnostic(row.diagnostic, "external validation evidence has no operator handoff row");
        model.has_blocking_diagnostics = true;
        model.diagnostics.push_back("evidence row '" + row.recipe_id + "' has no operator handoff row");
        if (!row.host_gates.empty()) {
            model.has_host_gates = true;
        }
        model.rows.push_back(std::move(row));
    }

    if (desc.request_mutation) {
        reject_unsupported_claim(
            model, "mutation",
            "playtest evidence summary rejects mutation and must not mutate manifests or package data");
    }
    if (desc.request_validation_execution) {
        reject_unsupported_claim(model, "validation execution",
                                 "playtest evidence summary rejects validation execution from editor core");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell",
                                 "playtest evidence summary rejects arbitrary shell execution");
    }
    if (desc.request_raw_manifest_command_evaluation) {
        reject_unsupported_claim(model, "raw manifest command",
                                 "playtest evidence summary rejects raw manifest command evaluation");
    }
    if (desc.request_package_script_execution) {
        reject_unsupported_claim(model, "package script execution",
                                 "playtest evidence summary rejects package script execution from editor core");
    }
    if (desc.request_free_form_manifest_edit) {
        reject_unsupported_claim(model, "free-form manifest edits",
                                 "playtest evidence summary rejects free-form manifest edits");
    }
    if (desc.request_play_in_editor_productization) {
        reject_unsupported_claim(model, "play-in-editor productization",
                                 "playtest evidence summary rejects play-in-editor productization");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "playtest evidence summary rejects renderer/RHI handle exposure");
    }
    if (desc.request_metal_readiness_claim) {
        reject_unsupported_claim(
            model, "Metal readiness",
            "playtest evidence summary rejects Metal readiness claims without Apple-host evidence");
    }
    if (desc.request_renderer_quality_claim) {
        reject_unsupported_claim(model, "renderer quality",
                                 "playtest evidence summary rejects renderer quality claims");
    }

    model.status = aggregate_evidence_status(model.has_blocking_diagnostics, model.has_failed_evidence,
                                             model.has_missing_evidence, model.has_host_gates);
    model.all_required_evidence_passed = !model.rows.empty() && model.status == EditorAiPlaytestEvidenceStatus::passed;
    return model;
}

EditorAiPlaytestEvidenceImportModel
make_editor_ai_playtest_evidence_import_model(const EditorAiPlaytestEvidenceImportDesc& desc) {
    EditorAiPlaytestEvidenceImportModel model;
    std::vector<std::string> expected_recipe_ids;
    for (const auto& recipe_id : desc.expected_recipe_ids) {
        if (!recipe_id.empty() && !contains_string(expected_recipe_ids, recipe_id)) {
            expected_recipe_ids.push_back(recipe_id);
        }
    }

    std::unordered_map<std::string, PendingEvidenceImportRow> pending_rows;
    std::vector<std::string> import_order;
    bool format_seen = false;

    const auto add_global_blocker = [&model](std::string diagnostic) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.push_back(std::move(diagnostic));
    };

    const auto get_pending_row = [&pending_rows,
                                  &import_order](const std::string& recipe_id) -> PendingEvidenceImportRow& {
        auto [it, inserted] = pending_rows.try_emplace(recipe_id);
        if (inserted) {
            it->second.evidence.recipe_id = recipe_id;
            it->second.review.recipe_id = recipe_id;
            import_order.push_back(recipe_id);
        }
        return it->second;
    };

    std::size_t start = 0U;
    while (start <= desc.text.size()) {
        const auto end = desc.text.find('\n', start);
        const auto raw_line = end == std::string::npos ? std::string_view{desc.text}.substr(start)
                                                       : std::string_view{desc.text}.substr(start, end - start);
        const auto line = trim_text(raw_line);
        if (!line.empty() && line.front() != '#') {
            const auto equals = line.find('=');
            if (equals == std::string::npos) {
                add_global_blocker("evidence import line is missing '=': " + line);
            } else {
                const auto key = trim_text(std::string_view{line}.substr(0, equals));
                const auto value = trim_text(std::string_view{line}.substr(equals + 1U));
                if (key == "format") {
                    format_seen = value == "GameEngine.EditorAiPlaytestEvidence.v1";
                    if (!format_seen) {
                        add_global_blocker("unsupported evidence import format: " + value);
                    }
                } else if (key.starts_with("evidence.")) {
                    const std::string_view key_view{key};
                    const auto field_dot = key_view.rfind('.');
                    if (field_dot == std::string_view::npos || field_dot <= std::string_view{"evidence."}.size()) {
                        add_global_blocker("malformed evidence import key: " + key);
                    } else {
                        const auto recipe_id = std::string(key_view.substr(
                            std::string_view{"evidence."}.size(), field_dot - std::string_view{"evidence."}.size()));
                        const auto field = std::string(key_view.substr(field_dot + 1U));
                        auto& row = get_pending_row(recipe_id);
                        if (!row.seen_fields.insert(field).second) {
                            add_import_blocker(row, "duplicate-evidence-field",
                                               "duplicate field '" + field + "' in imported evidence row");
                        }

                        if (field == "status") {
                            row.status_seen = true;
                            if (const auto parsed = parse_import_evidence_status(value); parsed.has_value()) {
                                row.evidence.status = *parsed;
                            } else {
                                add_import_blocker(row, "invalid-evidence-status",
                                                   "invalid evidence status '" + value + "'");
                            }
                        } else if (field == "exit_code") {
                            if (value.empty()) {
                                row.evidence.exit_code.reset();
                            } else if (const auto parsed = parse_import_exit_code(value); parsed.has_value()) {
                                row.evidence.exit_code = *parsed;
                            } else {
                                add_import_blocker(row, "invalid-exit-code", "invalid exit_code '" + value + "'");
                            }
                        } else if (field == "summary") {
                            row.evidence.summary = sanitize_import_text(value);
                        } else if (field == "stdout") {
                            row.evidence.stdout_summary = sanitize_import_text(value);
                        } else if (field == "stderr") {
                            row.evidence.stderr_summary = sanitize_import_text(value);
                        } else if (field == "host_gates") {
                            row.evidence.host_gates = split_import_list(value);
                        } else if (field == "blocked_by") {
                            row.evidence.blocked_by = split_import_list(value);
                        } else if (field == "externally_supplied") {
                            if (const auto parsed = parse_import_bool(value); parsed.has_value()) {
                                row.evidence.externally_supplied = *parsed;
                            } else {
                                add_import_blocker(row, "invalid-externally-supplied",
                                                   "invalid externally_supplied value '" + value + "'");
                            }
                        } else if (field == "claims_editor_core_execution") {
                            if (const auto parsed = parse_import_bool(value); parsed.has_value()) {
                                row.evidence.claims_editor_core_execution = *parsed;
                            } else {
                                add_import_blocker(row, "invalid-editor-core-execution-claim",
                                                   "invalid claims_editor_core_execution value '" + value + "'");
                            }
                        } else {
                            add_import_blocker(row, "unknown-evidence-import-field",
                                               "unknown evidence import field '" + field + "'");
                        }
                    }
                } else {
                    add_global_blocker("unknown evidence import key: " + key);
                }
            }
        }

        if (end == std::string::npos) {
            break;
        }
        start = end + 1U;
    }

    if (!format_seen) {
        add_global_blocker("missing GameEngine.EditorAiPlaytestEvidence.v1 format line");
    }

    const auto finalize_pending_row = [&model, &expected_recipe_ids](PendingEvidenceImportRow& pending, bool expected) {
        auto& review = pending.review;
        review.expected = expected;
        review.evidence_status = pending.evidence.status;
        review.exit_code = pending.evidence.exit_code;
        review.summary = make_evidence_summary(pending.evidence);
        review.stdout_summary = pending.evidence.stdout_summary;
        review.stderr_summary = pending.evidence.stderr_summary;
        review.host_gates = pending.evidence.host_gates;
        const auto import_blockers = review.blocked_by;
        review.blocked_by = pending.evidence.blocked_by;
        append_unique(review.blocked_by, import_blockers);
        review.imported = true;

        if (!pending.status_seen) {
            add_import_blocker(pending, "missing-evidence-status", "imported evidence row is missing a status field");
        }
        if (!expected_recipe_ids.empty() && !contains_string(expected_recipe_ids, pending.evidence.recipe_id)) {
            add_import_blocker(pending, "unexpected-recipe-id", "unexpected recipe id in imported evidence row");
            model.has_unexpected_evidence = true;
        }
        if (!pending.evidence.externally_supplied) {
            add_import_blocker(pending, "evidence-not-externally-supplied",
                               "imported validation evidence must be externally supplied");
        }
        if (pending.evidence.claims_editor_core_execution) {
            add_import_blocker(pending, "editor-core-validation-execution-claim",
                               "imported evidence must not claim editor-core validation execution");
        }

        if (pending.blocked) {
            review.status = EditorAiPlaytestEvidenceImportStatus::blocked;
            ++model.blocked_count;
            model.has_blocking_diagnostics = true;
            if (!review.diagnostic.empty()) {
                model.diagnostics.push_back(review.recipe_id + ": " + review.diagnostic);
            }
        } else {
            review.status = EditorAiPlaytestEvidenceImportStatus::importable;
            model.evidence_rows.push_back(pending.evidence);
            ++model.imported_count;
        }

        set_import_review_labels(review);
        model.review_rows.push_back(review);
    };

    std::unordered_set<std::string> finalized_recipe_ids;
    for (const auto& recipe_id : expected_recipe_ids) {
        const auto pending = pending_rows.find(recipe_id);
        if (pending != pending_rows.end()) {
            finalize_pending_row(pending->second, true);
            finalized_recipe_ids.insert(recipe_id);
        } else {
            EditorAiPlaytestEvidenceImportReviewRow row;
            row.recipe_id = recipe_id;
            row.expected = true;
            row.imported = false;
            row.status = EditorAiPlaytestEvidenceImportStatus::missing;
            row.evidence_status = EditorAiPlaytestEvidenceStatus::missing;
            row.diagnostic = "missing externally supplied evidence for expected recipe";
            set_import_review_labels(row);
            model.review_rows.push_back(std::move(row));
            model.has_missing_expected_evidence = true;
            ++model.missing_expected_count;
        }
    }

    for (const auto& recipe_id : import_order) {
        if (finalized_recipe_ids.contains(recipe_id)) {
            continue;
        }
        finalize_pending_row(pending_rows.at(recipe_id), expected_recipe_ids.empty());
    }

    if (desc.request_mutation) {
        reject_unsupported_claim(model, "mutation",
                                 "evidence import rejects mutation and must not mutate manifests or package data");
    }
    if (desc.request_validation_execution) {
        reject_unsupported_claim(model, "validation execution",
                                 "evidence import rejects validation execution from editor core");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell", "evidence import rejects arbitrary shell execution");
    }
    if (desc.request_raw_manifest_command_evaluation) {
        reject_unsupported_claim(model, "raw manifest command",
                                 "evidence import rejects raw manifest command evaluation");
    }
    if (desc.request_package_script_execution) {
        reject_unsupported_claim(model, "package script execution",
                                 "evidence import rejects package script execution from editor core");
    }
    if (desc.request_free_form_manifest_edit) {
        reject_unsupported_claim(model, "free-form manifest edits", "evidence import rejects free-form manifest edits");
    }
    if (desc.request_play_in_editor_productization) {
        reject_unsupported_claim(model, "play-in-editor productization",
                                 "evidence import rejects play-in-editor productization");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "evidence import rejects renderer/RHI handle exposure");
    }
    if (desc.request_metal_readiness_claim) {
        reject_unsupported_claim(model, "Metal readiness",
                                 "evidence import rejects Metal readiness claims without Apple-host evidence");
    }
    if (desc.request_renderer_quality_claim) {
        reject_unsupported_claim(model, "renderer quality", "evidence import rejects renderer quality claims");
    }

    if (model.has_blocking_diagnostics) {
        model.evidence_rows.clear();
        model.imported_count = 0U;
        model.status = EditorAiPlaytestEvidenceImportStatus::blocked;
    } else if (model.imported_count > 0U) {
        model.status = EditorAiPlaytestEvidenceImportStatus::importable;
    } else {
        model.status = EditorAiPlaytestEvidenceImportStatus::missing;
    }

    model.importable = model.status == EditorAiPlaytestEvidenceImportStatus::importable && !model.evidence_rows.empty();
    model.status_label = std::string(editor_ai_playtest_evidence_import_status_label(model.status));
    return model;
}

mirakana::ui::UiDocument
make_editor_ai_playtest_evidence_import_ui_model(const EditorAiPlaytestEvidenceImportModel& model) {
    mirakana::ui::UiDocument document;
    add_ui_or_throw(document, make_ui_root("ai_evidence_import", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"ai_evidence_import"};

    append_ui_label(document, root, "ai_evidence_import.status", model.status_label);
    append_ui_label(document, root, "ai_evidence_import.summary.imported", std::to_string(model.imported_count));
    append_ui_label(document, root, "ai_evidence_import.summary.missing_expected",
                    std::to_string(model.missing_expected_count));
    append_ui_label(document, root, "ai_evidence_import.summary.blocked", std::to_string(model.blocked_count));

    add_ui_or_throw(document, make_ui_child("ai_evidence_import.rows", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId rows_root{"ai_evidence_import.rows"};
    for (const auto& row : model.review_rows) {
        const auto prefix = "ai_evidence_import.rows." + sanitize_element_id(row.recipe_id);
        mirakana::ui::ElementDesc item = make_ui_child(prefix, rows_root, mirakana::ui::SemanticRole::list_item);
        item.enabled = row.status != EditorAiPlaytestEvidenceImportStatus::blocked;
        item.text = make_ui_text(row.recipe_id);
        add_ui_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_ui_label(document, item_root, prefix + ".status", row.status_label);
        append_ui_label(document, item_root, prefix + ".evidence_status", row.evidence_status_label);
        append_ui_label(document, item_root, prefix + ".summary", row.summary.empty() ? "-" : row.summary);
        append_ui_label(document, item_root, prefix + ".blocked_by", join_values(row.blocked_by));
        append_ui_label(document, item_root, prefix + ".diagnostic", row.diagnostic.empty() ? "-" : row.diagnostic);
    }

    add_ui_or_throw(document, make_ui_child("ai_evidence_import.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{"ai_evidence_import.diagnostics"};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_ui_label(document, diagnostics_root, "ai_evidence_import.diagnostics." + std::to_string(index),
                        model.diagnostics[index]);
    }

    return document;
}

EditorAiPlaytestRemediationQueueModel
make_editor_ai_playtest_remediation_queue_model(const EditorAiPlaytestRemediationQueueDesc& desc) {
    EditorAiPlaytestRemediationQueueModel model;

    if (desc.evidence_summary.rows.empty()) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("playtest remediation queue has no evidence summary rows");
    }
    if (desc.evidence_summary.mutates) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("evidence summary input must be read-only and must not mutate");
    }
    if (desc.evidence_summary.executes) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("evidence summary input must not execute");
    }
    for (const auto& unsupported_claim : desc.evidence_summary.unsupported_claims) {
        append_unique(model.unsupported_claims, unsupported_claim);
    }
    for (const auto& diagnostic : desc.evidence_summary.diagnostics) {
        model.diagnostics.push_back("evidence summary: " + diagnostic);
    }

    for (const auto& evidence_row : desc.evidence_summary.rows) {
        if (evidence_row.status == EditorAiPlaytestEvidenceStatus::passed) {
            continue;
        }

        EditorAiPlaytestRemediationQueueRow row;
        row.recipe_id = evidence_row.recipe_id;
        row.evidence_status = evidence_row.status;
        row.category = remediation_category_for(evidence_row.status);
        row.exit_code = evidence_row.exit_code;
        row.evidence_summary = evidence_row.summary;
        row.host_gates = evidence_row.host_gates;
        row.blocked_by = evidence_row.blocked_by;
        row.readiness_dependency = evidence_row.readiness_dependency;
        row.next_action = make_remediation_next_action(evidence_row, row.category);
        row.diagnostic = evidence_row.diagnostic;

        if (row.evidence_status == EditorAiPlaytestEvidenceStatus::failed) {
            model.has_failed_evidence = true;
        }
        if (row.evidence_status == EditorAiPlaytestEvidenceStatus::blocked) {
            model.has_blocked_evidence = true;
        }
        if (row.evidence_status == EditorAiPlaytestEvidenceStatus::missing) {
            model.has_missing_evidence = true;
        }
        if (row.evidence_status == EditorAiPlaytestEvidenceStatus::host_gated || !row.host_gates.empty()) {
            model.has_host_gates = true;
        }

        model.rows.push_back(std::move(row));
    }

    if (desc.request_mutation) {
        reject_unsupported_claim(
            model, "mutation",
            "playtest remediation queue rejects mutation and must not mutate manifests or package data");
    }
    if (desc.request_validation_execution) {
        reject_unsupported_claim(model, "validation execution",
                                 "playtest remediation queue rejects validation execution from editor core");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell",
                                 "playtest remediation queue rejects arbitrary shell execution");
    }
    if (desc.request_raw_manifest_command_evaluation) {
        reject_unsupported_claim(model, "raw/free-form command evaluation",
                                 "playtest remediation queue rejects raw/free-form command evaluation");
    }
    if (desc.request_package_script_execution) {
        reject_unsupported_claim(model, "package script execution",
                                 "playtest remediation queue rejects package script execution from editor core");
    }
    if (desc.request_free_form_manifest_edit) {
        reject_unsupported_claim(model, "free-form manifest edits",
                                 "playtest remediation queue rejects free-form manifest edits");
    }
    if (desc.request_evidence_mutation) {
        reject_unsupported_claim(model, "evidence mutation", "playtest remediation queue rejects evidence mutation");
    }
    if (desc.request_fix_execution) {
        reject_unsupported_claim(model, "fix execution",
                                 "playtest remediation queue rejects fix execution from editor core");
    }
    if (desc.request_play_in_editor_productization) {
        reject_unsupported_claim(model, "play-in-editor productization",
                                 "playtest remediation queue rejects play-in-editor productization");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "playtest remediation queue rejects renderer/RHI handle exposure");
    }
    if (desc.request_metal_readiness_claim) {
        reject_unsupported_claim(
            model, "Metal readiness",
            "playtest remediation queue rejects Metal readiness claims without Apple-host evidence");
    }
    if (desc.request_renderer_quality_claim) {
        reject_unsupported_claim(model, "renderer quality",
                                 "playtest remediation queue rejects renderer quality claims");
    }

    model.remediation_required = !model.rows.empty();
    return model;
}

EditorAiPlaytestRemediationHandoffModel
make_editor_ai_playtest_remediation_handoff_model(const EditorAiPlaytestRemediationHandoffDesc& desc) {
    EditorAiPlaytestRemediationHandoffModel model;

    if (desc.remediation_queue.rows.empty()) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("playtest remediation handoff has no remediation queue rows");
    }
    if (desc.remediation_queue.has_blocking_diagnostics) {
        model.has_blocking_diagnostics = true;
        if (desc.remediation_queue.diagnostics.empty()) {
            model.diagnostics.emplace_back("remediation queue is blocked before handoff");
        }
    }
    if (desc.remediation_queue.mutates) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("remediation queue input must be read-only and must not mutate");
    }
    if (desc.remediation_queue.executes) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("remediation queue input must not execute");
    }
    for (const auto& unsupported_claim : desc.remediation_queue.unsupported_claims) {
        append_unique(model.unsupported_claims, unsupported_claim);
    }
    for (const auto& diagnostic : desc.remediation_queue.diagnostics) {
        model.diagnostics.push_back("remediation queue: " + diagnostic);
    }

    for (const auto& queue_row : desc.remediation_queue.rows) {
        EditorAiPlaytestRemediationHandoffRow row;
        row.recipe_id = queue_row.recipe_id;
        row.evidence_status = queue_row.evidence_status;
        row.remediation_category = queue_row.category;
        row.action_kind = handoff_action_kind_for(queue_row.category);
        row.external_owner = std::string(external_owner_for(row.action_kind));
        row.handoff_text = make_remediation_handoff_text(queue_row, row.action_kind);
        row.exit_code = queue_row.exit_code;
        row.evidence_summary = queue_row.evidence_summary;
        row.host_gates = queue_row.host_gates;
        row.blocked_by = queue_row.blocked_by;
        row.readiness_dependency = queue_row.readiness_dependency;
        row.unsupported_claims = desc.remediation_queue.unsupported_claims;
        row.diagnostic = queue_row.diagnostic;

        if (row.evidence_status == EditorAiPlaytestEvidenceStatus::failed) {
            model.has_failed_evidence = true;
        }
        if (row.evidence_status == EditorAiPlaytestEvidenceStatus::blocked) {
            model.has_blocked_evidence = true;
        }
        if (row.evidence_status == EditorAiPlaytestEvidenceStatus::missing) {
            model.has_missing_evidence = true;
        }
        if (row.evidence_status == EditorAiPlaytestEvidenceStatus::host_gated || !row.host_gates.empty()) {
            model.has_host_gates = true;
        }

        model.rows.push_back(std::move(row));
    }

    if (desc.request_mutation) {
        reject_unsupported_claim(
            model, "mutation",
            "playtest remediation handoff rejects mutation and must not mutate manifests or package data");
    }
    if (desc.request_validation_execution) {
        reject_unsupported_claim(model, "validation execution",
                                 "playtest remediation handoff rejects validation execution from editor core");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell",
                                 "playtest remediation handoff rejects arbitrary shell execution");
    }
    if (desc.request_raw_manifest_command_evaluation) {
        reject_unsupported_claim(model, "raw/free-form command evaluation",
                                 "playtest remediation handoff rejects raw/free-form command evaluation");
    }
    if (desc.request_package_script_execution) {
        reject_unsupported_claim(model, "package script execution",
                                 "playtest remediation handoff rejects package script execution from editor core");
    }
    if (desc.request_free_form_manifest_edit) {
        reject_unsupported_claim(model, "free-form manifest edits",
                                 "playtest remediation handoff rejects free-form manifest edits");
    }
    if (desc.request_evidence_mutation) {
        reject_unsupported_claim(model, "evidence mutation", "playtest remediation handoff rejects evidence mutation");
    }
    if (desc.request_remediation_mutation) {
        reject_unsupported_claim(model, "remediation mutation",
                                 "playtest remediation handoff rejects remediation mutation");
    }
    if (desc.request_fix_execution) {
        reject_unsupported_claim(model, "fix execution",
                                 "playtest remediation handoff rejects fix execution from editor core");
    }
    if (desc.request_play_in_editor_productization) {
        reject_unsupported_claim(model, "play-in-editor productization",
                                 "playtest remediation handoff rejects play-in-editor productization");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "playtest remediation handoff rejects renderer/RHI handle exposure");
    }
    if (desc.request_metal_readiness_claim) {
        reject_unsupported_claim(
            model, "Metal readiness",
            "playtest remediation handoff rejects Metal readiness claims without Apple-host evidence");
    }
    if (desc.request_renderer_quality_claim) {
        reject_unsupported_claim(model, "renderer quality",
                                 "playtest remediation handoff rejects renderer quality claims");
    }

    model.handoff_required = !model.rows.empty();
    return model;
}

EditorAiPlaytestOperatorWorkflowReportModel
make_editor_ai_playtest_operator_workflow_report_model(const EditorAiPlaytestOperatorWorkflowReportDesc& desc) {
    EditorAiPlaytestOperatorWorkflowReportModel model;

    reject_readonly_input(model, "EditorAiPackageAuthoringDiagnosticsModel", desc.package_diagnostics.mutates,
                          desc.package_diagnostics.executes);
    reject_readonly_input(model, "EditorAiValidationRecipePreflightModel", false, desc.validation_preflight.executes);
    reject_readonly_input(model, "EditorAiPlaytestReadinessReportModel", desc.readiness_report.mutates,
                          desc.readiness_report.executes);
    reject_readonly_input(model, "EditorAiPlaytestOperatorHandoffModel", desc.operator_handoff.mutates,
                          desc.operator_handoff.executes);
    reject_readonly_input(model, "EditorAiPlaytestEvidenceSummaryModel", desc.evidence_summary.mutates,
                          desc.evidence_summary.executes);
    reject_readonly_input(model, "EditorAiPlaytestRemediationQueueModel", desc.remediation_queue.mutates,
                          desc.remediation_queue.executes);
    reject_readonly_input(model, "EditorAiPlaytestRemediationHandoffModel", desc.remediation_handoff.mutates,
                          desc.remediation_handoff.executes);

    append_model_diagnostics(model, "EditorAiPackageAuthoringDiagnosticsModel", desc.package_diagnostics.diagnostics);
    append_model_diagnostics(model, "EditorAiValidationRecipePreflightModel", desc.validation_preflight.diagnostics);
    append_model_diagnostics(model, "EditorAiPlaytestReadinessReportModel", desc.readiness_report.diagnostics);
    append_model_diagnostics(model, "EditorAiPlaytestOperatorHandoffModel", desc.operator_handoff.diagnostics);
    append_model_diagnostics(model, "EditorAiPlaytestEvidenceSummaryModel", desc.evidence_summary.diagnostics);
    append_model_diagnostics(model, "EditorAiPlaytestRemediationQueueModel", desc.remediation_queue.diagnostics);
    if (desc.remediation_queue.remediation_required || !desc.remediation_handoff.rows.empty()) {
        append_model_diagnostics(model, "EditorAiPlaytestRemediationHandoffModel",
                                 desc.remediation_handoff.diagnostics);
    }

    append_model_unsupported_claims(model, desc.validation_preflight.unsupported_claims);
    append_model_unsupported_claims(model, desc.readiness_report.unsupported_claims);
    append_model_unsupported_claims(model, desc.operator_handoff.unsupported_claims);
    append_model_unsupported_claims(model, desc.evidence_summary.unsupported_claims);
    append_model_unsupported_claims(model, desc.remediation_queue.unsupported_claims);
    append_model_unsupported_claims(model, desc.remediation_handoff.unsupported_claims);

    if (desc.request_mutation) {
        reject_unsupported_claim(model, "mutation",
                                 "operator workflow report rejects mutation and must remain read-only");
    }
    if (desc.request_manifest_mutation) {
        reject_unsupported_claim(model, "manifest mutation", "operator workflow report rejects manifest mutation");
    }
    if (desc.request_validation_execution) {
        reject_unsupported_claim(model, "validation execution",
                                 "operator workflow report rejects validation execution from editor core");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell",
                                 "operator workflow report rejects arbitrary shell execution");
    }
    if (desc.request_raw_manifest_command_evaluation) {
        reject_unsupported_claim(model, "raw manifest command strings",
                                 "operator workflow report rejects raw manifest command evaluation");
    }
    if (desc.request_free_form_command_evaluation) {
        reject_unsupported_claim(model, "raw/free-form command evaluation",
                                 "operator workflow report rejects raw/free-form command evaluation");
    }
    if (desc.request_package_script_execution) {
        reject_unsupported_claim(model, "package script execution",
                                 "operator workflow report rejects package script execution from editor core");
    }
    if (desc.request_free_form_manifest_edit) {
        reject_unsupported_claim(model, "free-form manifest edits",
                                 "operator workflow report rejects free-form manifest edits");
    }
    if (desc.request_evidence_mutation) {
        reject_unsupported_claim(model, "evidence mutation", "operator workflow report rejects evidence mutation");
    }
    if (desc.request_remediation_mutation) {
        reject_unsupported_claim(model, "remediation mutation",
                                 "operator workflow report rejects remediation mutation");
    }
    if (desc.request_fix_execution) {
        reject_unsupported_claim(model, "fix execution",
                                 "operator workflow report rejects fix execution from editor core");
    }
    if (desc.request_play_in_editor_productization) {
        reject_unsupported_claim(model, "play-in-editor productization",
                                 "operator workflow report rejects play-in-editor productization");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "operator workflow report rejects renderer/RHI handle exposure");
    }
    if (desc.request_metal_readiness_claim) {
        reject_unsupported_claim(model, "Metal readiness",
                                 "operator workflow report rejects Metal readiness claims without Apple-host evidence");
    }
    if (desc.request_renderer_quality_claim) {
        reject_unsupported_claim(model, "renderer quality", "operator workflow report rejects renderer quality claims");
    }

    std::vector<std::string> package_row_ids;
    package_row_ids.reserve(desc.package_diagnostics.descriptor_rows.size());
    for (const auto& row : desc.package_diagnostics.descriptor_rows) {
        package_row_ids.push_back(row.id);
    }
    for (const auto& row : desc.package_diagnostics.payload_rows) {
        package_row_ids.push_back(row.id);
    }
    std::vector<std::string> package_host_gates;
    for (const auto& row : desc.package_diagnostics.validation_recipe_rows) {
        package_row_ids.push_back(row.id);
        if (row.host_gated || row.status == EditorAiPackageAuthoringDiagnosticStatus::host_gated) {
            append_unique(package_host_gates, row.id);
        }
    }
    push_workflow_stage(
        model,
        EditorAiPlaytestOperatorWorkflowStageRow{
            .id = "package-diagnostics",
            .source_model = "EditorAiPackageAuthoringDiagnosticsModel",
            .status = workflow_status_from_package_status(
                aggregate_status(desc.package_diagnostics.has_blocking_diagnostics,
                                 desc.package_diagnostics.has_host_gated_recipes),
                desc.package_diagnostics.has_blocking_diagnostics, desc.package_diagnostics.has_host_gated_recipes),
            .source_row_count = package_row_ids.size(),
            .source_row_ids = package_row_ids,
            .host_gates = package_host_gates,
            .blocked_by = {},
            .diagnostic = desc.package_diagnostics.has_blocking_diagnostics
                              ? "package diagnostics blocked"
                              : (desc.package_diagnostics.has_host_gated_recipes ? "package diagnostics are host-gated"
                                                                                 : "package diagnostics are ready"),
        });

    std::vector<std::string> preflight_row_ids;
    std::vector<std::string> preflight_host_gates;
    std::vector<std::string> preflight_blockers;
    for (const auto& row : desc.validation_preflight.rows) {
        preflight_row_ids.push_back(row.id);
        append_unique(preflight_host_gates, row.host_gates);
        append_unique(preflight_blockers, row.blocked_by);
    }
    push_workflow_stage(
        model,
        EditorAiPlaytestOperatorWorkflowStageRow{
            .id = "validation-preflight",
            .source_model = "EditorAiValidationRecipePreflightModel",
            .status = workflow_status_from_package_status(
                aggregate_status(desc.validation_preflight.has_blocking_diagnostics,
                                 desc.validation_preflight.has_host_gated_recipes),
                desc.validation_preflight.has_blocking_diagnostics, desc.validation_preflight.has_host_gated_recipes),
            .source_row_count = preflight_row_ids.size(),
            .source_row_ids = preflight_row_ids,
            .host_gates = preflight_host_gates,
            .blocked_by = preflight_blockers,
            .diagnostic = desc.validation_preflight.has_blocking_diagnostics
                              ? "validation preflight blocked"
                              : (desc.validation_preflight.has_host_gated_recipes ? "validation preflight is host-gated"
                                                                                  : "validation preflight is ready"),
        });

    std::vector<std::string> readiness_row_ids;
    readiness_row_ids.reserve(desc.readiness_report.rows.size());
    for (const auto& row : desc.readiness_report.rows) {
        readiness_row_ids.push_back(row.id);
    }
    push_workflow_stage(
        model,
        EditorAiPlaytestOperatorWorkflowStageRow{
            .id = "readiness",
            .source_model = "EditorAiPlaytestReadinessReportModel",
            .status = workflow_status_from_package_status(desc.readiness_report.status,
                                                          desc.readiness_report.has_blocking_diagnostics ||
                                                              !desc.readiness_report.ready_for_operator_validation,
                                                          desc.readiness_report.has_host_gates),
            .source_row_count = readiness_row_ids.size(),
            .source_row_ids = readiness_row_ids,
            .host_gates = desc.readiness_report.has_host_gates ? std::vector<std::string>{"readiness-host-gates"}
                                                               : std::vector<std::string>{},
            .blocked_by = desc.readiness_report.ready_for_operator_validation
                              ? std::vector<std::string>{}
                              : std::vector<std::string>{"readiness-report-blocked"},
            .diagnostic = desc.readiness_report.ready_for_operator_validation
                              ? "readiness report allows operator validation"
                              : "readiness report must pass before operator validation",
        });

    std::vector<std::string> handoff_row_ids;
    std::vector<std::string> handoff_host_gates;
    std::vector<std::string> handoff_blockers;
    for (const auto& row : desc.operator_handoff.command_rows) {
        handoff_row_ids.push_back(row.recipe_id);
        append_unique(handoff_host_gates, row.host_gates);
        append_unique(handoff_blockers, row.blocked_by);
    }
    push_workflow_stage(
        model, EditorAiPlaytestOperatorWorkflowStageRow{
                   .id = "operator-handoff",
                   .source_model = "EditorAiPlaytestOperatorHandoffModel",
                   .status = workflow_status_from_package_status(desc.operator_handoff.status,
                                                                 desc.operator_handoff.has_blocking_diagnostics ||
                                                                     !desc.operator_handoff.ready_for_operator_handoff,
                                                                 desc.operator_handoff.has_host_gates),
                   .source_row_count = handoff_row_ids.size(),
                   .source_row_ids = handoff_row_ids,
                   .host_gates = handoff_host_gates,
                   .blocked_by = handoff_blockers,
                   .diagnostic = desc.operator_handoff.ready_for_operator_handoff
                                     ? "operator handoff rows are ready for external execution"
                                     : "operator handoff is blocked",
               });

    std::vector<std::string> evidence_row_ids;
    std::vector<std::string> evidence_host_gates;
    std::vector<std::string> evidence_blockers;
    for (const auto& row : desc.evidence_summary.rows) {
        evidence_row_ids.push_back(row.recipe_id);
        append_unique(evidence_host_gates, row.host_gates);
        append_unique(evidence_blockers, row.blocked_by);
    }
    push_workflow_stage(model, EditorAiPlaytestOperatorWorkflowStageRow{
                                   .id = "evidence-summary",
                                   .source_model = "EditorAiPlaytestEvidenceSummaryModel",
                                   .status = workflow_status_from_evidence_summary(desc.evidence_summary),
                                   .source_row_count = evidence_row_ids.size(),
                                   .source_row_ids = evidence_row_ids,
                                   .host_gates = evidence_host_gates,
                                   .blocked_by = evidence_blockers,
                                   .diagnostic = desc.evidence_summary.all_required_evidence_passed
                                                     ? "externally supplied evidence passed"
                                                     : "externally supplied evidence still requires operator review",
                               });

    std::vector<std::string> queue_row_ids;
    std::vector<std::string> queue_host_gates;
    std::vector<std::string> queue_blockers;
    for (const auto& row : desc.remediation_queue.rows) {
        queue_row_ids.push_back(row.recipe_id);
        append_unique(queue_host_gates, row.host_gates);
        append_unique(queue_blockers, row.blocked_by);
    }
    auto queue_status = EditorAiPlaytestOperatorWorkflowStageStatus::ready;
    if (desc.remediation_queue.has_blocking_diagnostics) {
        queue_status = EditorAiPlaytestOperatorWorkflowStageStatus::blocked;
    } else if (desc.remediation_queue.remediation_required) {
        queue_status = EditorAiPlaytestOperatorWorkflowStageStatus::remediation_required;
    } else if (desc.remediation_queue.has_host_gates) {
        queue_status = EditorAiPlaytestOperatorWorkflowStageStatus::host_gated;
    }
    push_workflow_stage(model, EditorAiPlaytestOperatorWorkflowStageRow{
                                   .id = "remediation-queue",
                                   .source_model = "EditorAiPlaytestRemediationQueueModel",
                                   .status = queue_status,
                                   .source_row_count = queue_row_ids.size(),
                                   .source_row_ids = queue_row_ids,
                                   .host_gates = queue_host_gates,
                                   .blocked_by = queue_blockers,
                                   .diagnostic = desc.remediation_queue.remediation_required
                                                     ? "non-passing evidence has remediation rows"
                                                     : "no remediation rows remain",
                               });

    std::vector<std::string> remediation_handoff_row_ids;
    std::vector<std::string> remediation_handoff_host_gates;
    std::vector<std::string> remediation_handoff_blockers;
    for (const auto& row : desc.remediation_handoff.rows) {
        remediation_handoff_row_ids.push_back(row.recipe_id);
        append_unique(remediation_handoff_host_gates, row.host_gates);
        append_unique(remediation_handoff_blockers, row.blocked_by);
    }
    auto remediation_handoff_status = EditorAiPlaytestOperatorWorkflowStageStatus::ready;
    const bool remediation_handoff_blocked =
        desc.remediation_handoff.has_blocking_diagnostics &&
        (desc.remediation_queue.remediation_required || !desc.remediation_handoff.rows.empty());
    if (remediation_handoff_blocked) {
        remediation_handoff_status = EditorAiPlaytestOperatorWorkflowStageStatus::blocked;
    } else if (desc.remediation_handoff.handoff_required) {
        remediation_handoff_status = EditorAiPlaytestOperatorWorkflowStageStatus::handoff_required;
    } else if (desc.remediation_handoff.has_host_gates) {
        remediation_handoff_status = EditorAiPlaytestOperatorWorkflowStageStatus::host_gated;
    }
    push_workflow_stage(model, EditorAiPlaytestOperatorWorkflowStageRow{
                                   .id = "remediation-handoff",
                                   .source_model = "EditorAiPlaytestRemediationHandoffModel",
                                   .status = remediation_handoff_status,
                                   .source_row_count = remediation_handoff_row_ids.size(),
                                   .source_row_ids = remediation_handoff_row_ids,
                                   .host_gates = remediation_handoff_host_gates,
                                   .blocked_by = remediation_handoff_blockers,
                                   .diagnostic = desc.remediation_handoff.handoff_required
                                                     ? "remediation handoff rows require external operator action"
                                                     : "no remediation handoff rows remain",
                               });

    model.remediation_required = desc.remediation_queue.remediation_required;
    model.handoff_required = desc.remediation_handoff.handoff_required;
    model.closeout_complete = desc.evidence_summary.all_required_evidence_passed && !model.remediation_required &&
                              !model.handoff_required && desc.remediation_queue.rows.empty() &&
                              desc.remediation_handoff.rows.empty() && !model.has_blocking_diagnostics;
    model.external_action_required = !model.closeout_complete;

    std::vector<std::string> closeout_row_ids = evidence_row_ids;
    append_unique(closeout_row_ids, queue_row_ids);
    append_unique(closeout_row_ids, remediation_handoff_row_ids);
    std::vector<std::string> closeout_host_gates = evidence_host_gates;
    append_unique(closeout_host_gates, queue_host_gates);
    append_unique(closeout_host_gates, remediation_handoff_host_gates);
    std::vector<std::string> closeout_blockers = evidence_blockers;
    append_unique(closeout_blockers, queue_blockers);
    append_unique(closeout_blockers, remediation_handoff_blockers);

    auto closeout_status = EditorAiPlaytestOperatorWorkflowStageStatus::ready;
    if (model.closeout_complete) {
        closeout_status = EditorAiPlaytestOperatorWorkflowStageStatus::closed;
    } else if (model.has_blocking_diagnostics) {
        closeout_status = EditorAiPlaytestOperatorWorkflowStageStatus::blocked;
    } else if (model.handoff_required) {
        closeout_status = EditorAiPlaytestOperatorWorkflowStageStatus::handoff_required;
    } else if (model.remediation_required) {
        closeout_status = EditorAiPlaytestOperatorWorkflowStageStatus::remediation_required;
    } else if (desc.evidence_summary.has_missing_evidence ||
               desc.evidence_summary.status == EditorAiPlaytestEvidenceStatus::missing) {
        closeout_status = EditorAiPlaytestOperatorWorkflowStageStatus::awaiting_external_evidence;
    } else if (!closeout_host_gates.empty()) {
        closeout_status = EditorAiPlaytestOperatorWorkflowStageStatus::host_gated;
    }
    push_workflow_stage(
        model, EditorAiPlaytestOperatorWorkflowStageRow{
                   .id = "closeout",
                   .source_model = "EditorAiPlaytestEvidenceSummaryModel+EditorAiPlaytestRemediationQueueModel+"
                                   "EditorAiPlaytestRemediationHandoffModel",
                   .status = closeout_status,
                   .source_row_count = closeout_row_ids.size(),
                   .source_row_ids = closeout_row_ids,
                   .host_gates = closeout_host_gates,
                   .blocked_by = closeout_blockers,
                   .diagnostic = model.closeout_complete
                                     ? "closeout complete through existing evidence summary rerun"
                                     : "closeout requires external action and another evidence summary rerun",
               });

    return model;
}

EditorAiPackageAuthoringDiagnosticsModel
make_editor_ai_package_authoring_diagnostics_model(const EditorAiPackageAuthoringDiagnosticsDesc& desc) {
    EditorAiPackageAuthoringDiagnosticsModel model;
    model.playtest_review = make_editor_playtest_package_review_model(desc.playtest_review);
    model.descriptor_rows = desc.descriptor_rows;
    model.payload_rows = desc.payload_rows;
    model.validation_recipe_rows = desc.validation_recipe_rows;
    model.has_host_gated_recipes = model.playtest_review.host_gated_desktop_smoke_available;

    if (!model.playtest_review.ready_for_runtime_scene_validation) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("playtest package review is blocked before runtime scene validation");
    }
    for (const auto& diagnostic : model.playtest_review.diagnostics) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.push_back(diagnostic);
    }

    for (const auto& row : model.descriptor_rows) {
        if (row.status == EditorAiPackageAuthoringDiagnosticStatus::blocked) {
            model.has_blocking_diagnostics = true;
            model.diagnostics.push_back("descriptor row '" + row.id + "' is blocked: " + row.diagnostic);
        }
        if (row.status == EditorAiPackageAuthoringDiagnosticStatus::host_gated) {
            model.has_host_gated_recipes = true;
        }
        if (row.mutates) {
            model.has_blocking_diagnostics = true;
            model.diagnostics.push_back("descriptor row '" + row.id + "' must be diagnostics-only and must not mutate");
        }
        if (row.executes) {
            model.has_blocking_diagnostics = true;
            model.diagnostics.push_back("descriptor row '" + row.id +
                                        "' must be diagnostics-only and must not execute");
        }
    }

    for (const auto& row : model.validation_recipe_rows) {
        if (row.status == EditorAiPackageAuthoringDiagnosticStatus::blocked) {
            model.has_blocking_diagnostics = true;
            model.diagnostics.push_back("validation recipe row '" + row.id + "' is blocked: " + row.diagnostic);
        }
        if (row.status == EditorAiPackageAuthoringDiagnosticStatus::host_gated || row.host_gated) {
            model.has_host_gated_recipes = true;
        }
        if (row.executes) {
            model.has_blocking_diagnostics = true;
            model.diagnostics.push_back("validation recipe row '" + row.id +
                                        "' must be preflight-only and must not execute");
        }
    }

    for (const auto& row : model.payload_rows) {
        if (row.status == EditorAiPackageAuthoringDiagnosticStatus::blocked) {
            model.has_blocking_diagnostics = true;
            model.diagnostics.push_back("payload row '" + row.id + "' is blocked: " + row.diagnostic);
        }
        if (row.status == EditorAiPackageAuthoringDiagnosticStatus::host_gated) {
            model.has_host_gated_recipes = true;
        }
    }

    return model;
}

EditorTilemapPackageDiagnosticsModel
make_editor_tilemap_package_diagnostics_model(const runtime::RuntimeAssetPackage& package) {
    EditorTilemapPackageDiagnosticsModel model;
    const auto package_report = runtime::inspect_runtime_asset_package(package);

    for (const auto& record : package.records()) {
        if (record.kind != AssetKind::tilemap) {
            continue;
        }

        model.has_tilemap_payloads = true;
        EditorTilemapPackageDiagnosticRow row;
        row.asset = record.asset;
        row.path = record.path;

        std::string package_diagnostic;
        for (const auto& diagnostic : package_report.diagnostics) {
            if (diagnostic.asset == record.asset && diagnostic.severity == runtime::RuntimeDiagnosticSeverity::error) {
                ++row.diagnostic_count;
                append_diagnostic(package_diagnostic, diagnostic.message);
            }
        }

        const auto payload = runtime::runtime_tilemap_payload(record);
        if (!payload.succeeded()) {
            append_diagnostic(row.diagnostic, payload.diagnostic);
        } else {
            const auto sampled = runtime::sample_runtime_tilemap_visible_cells(payload.payload);
            row.layer_count = sampled.layer_count;
            row.visible_layer_count = sampled.visible_layer_count;
            row.tile_count = sampled.tile_count;
            row.non_empty_cell_count = sampled.non_empty_cell_count;
            row.sampled_cell_count = sampled.sampled_cell_count;
            row.diagnostic_count += sampled.diagnostic_count;
            model.sampled_cell_count += sampled.sampled_cell_count;
            if (!sampled.succeeded) {
                append_diagnostic(row.diagnostic, sampled.diagnostic);
            }
        }
        if (!package_diagnostic.empty()) {
            append_diagnostic(row.diagnostic, package_diagnostic);
        }
        if (row.diagnostic.empty()) {
            row.diagnostic = "runtime tilemap package row is ready";
        }

        row.status = row.diagnostic_count == 0 ? EditorAiPackageAuthoringDiagnosticStatus::ready
                                               : EditorAiPackageAuthoringDiagnosticStatus::blocked;
        if (row.status == EditorAiPackageAuthoringDiagnosticStatus::blocked) {
            model.has_blocking_diagnostics = true;
            model.diagnostics.push_back("tilemap package row '" + row.path + "' is blocked: " + row.diagnostic);
        }
        model.rows.push_back(std::move(row));
    }

    if (!model.has_tilemap_payloads) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("runtime package contains no tilemap payload rows");
    }

    return model;
}

} // namespace mirakana::editor
