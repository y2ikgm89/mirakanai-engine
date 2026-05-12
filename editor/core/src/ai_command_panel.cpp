// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/ai_command_panel.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string sanitize_text(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        if (character == '\n' || character == '\r' || character == '=') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text.empty() ? "-" : text;
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

void append_unique(std::vector<std::string>& values, std::string value) {
    if (value.empty()) {
        return;
    }
    if (std::ranges::find(values, value) == values.end()) {
        values.push_back(std::move(value));
    }
}

void append_many_unique(std::vector<std::string>& destination, const std::vector<std::string>& source) {
    for (const auto& value : source) {
        append_unique(destination, sanitize_text(value));
    }
}

[[nodiscard]] bool contains_string(const std::vector<std::string>& values, std::string_view expected) {
    return std::ranges::any_of(values, [expected](const std::string& value) { return value == expected; });
}

[[nodiscard]] bool stage_is_external(EditorAiPlaytestOperatorWorkflowStageStatus status) noexcept {
    return status == EditorAiPlaytestOperatorWorkflowStageStatus::awaiting_external_evidence ||
           status == EditorAiPlaytestOperatorWorkflowStageStatus::remediation_required ||
           status == EditorAiPlaytestOperatorWorkflowStageStatus::handoff_required;
}

[[nodiscard]] EditorAiCommandPanelStatus aggregate_panel_status(const EditorAiCommandPanelModel& model) noexcept {
    if (model.has_blocking_diagnostics || model.blocked_stage_count > 0U) {
        return EditorAiCommandPanelStatus::blocked;
    }
    if (model.external_action_required || model.external_stage_count > 0U) {
        return EditorAiCommandPanelStatus::external_action_required;
    }
    if (model.has_host_gates || model.host_gated_stage_count > 0U) {
        return EditorAiCommandPanelStatus::host_gated;
    }
    return EditorAiCommandPanelStatus::ready;
}

void reject_mutating_or_executing_inputs(EditorAiCommandPanelModel& model, const EditorAiCommandPanelDesc& desc) {
    const bool mutates = desc.workflow_report.mutates || desc.operator_handoff.mutates || desc.evidence_summary.mutates;
    const bool executes =
        desc.workflow_report.executes || desc.operator_handoff.executes || desc.evidence_summary.executes;
    if (mutates) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back(
            "AI command panel input must be read-only and must not mutate manifests or package data");
    }
    if (executes) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back(
            "AI command panel input must not execute validation recipes, package scripts, or shell commands");
    }
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("editor ai command panel ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

[[nodiscard]] mirakana::ui::ElementDesc make_root(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_child(std::string id, mirakana::ui::ElementId parent,
                                                   mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.parent = std::move(parent);
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

[[nodiscard]] std::string join_values(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "-";
    }
    std::string text;
    for (const auto& value : values) {
        if (!text.empty()) {
            text += ", ";
        }
        text += value;
    }
    return text;
}

[[nodiscard]] std::string make_process_command_display(const mirakana::ProcessCommand& command) {
    if (command.executable.empty()) {
        return "-";
    }
    std::string text = command.executable;
    for (const auto& argument : command.arguments) {
        text += " ";
        text += argument;
    }
    return text;
}

void reject_execution_claim(EditorAiReviewedValidationExecutionModel& model, std::string claim,
                            std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.unsupported_claims, std::move(claim));
    append_unique(model.blocked_by, "unsupported-execution-claim");
    model.diagnostics.push_back(std::move(diagnostic));
}

void append_execution_blocker(EditorAiReviewedValidationExecutionModel& model, std::string blocker,
                              std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.blocked_by, std::move(blocker));
    model.diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] bool safe_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool valid_runner_prefix(const std::vector<std::string>& argv) {
    return argv.size() >= 10U && argv[0] == "pwsh" && argv[1] == "-NoProfile" && argv[2] == "-ExecutionPolicy" &&
           argv[3] == "Bypass" && argv[4] == "-File" && argv[5] == "tools/run-validation-recipe.ps1";
}

[[nodiscard]] bool rewrite_reviewed_runner_argv(const EditorAiPlaytestOperatorHandoffCommandRow& row,
                                                std::vector<std::string>& arguments, std::string& diagnostic) {
    if (!valid_runner_prefix(row.argv)) {
        diagnostic = "reviewed run-validation-recipe argv must start with pwsh -NoProfile -ExecutionPolicy Bypass "
                     "-File tools/run-validation-recipe.ps1";
        return false;
    }

    bool mode_seen = false;
    bool recipe_seen = false;
    arguments.assign(row.argv.begin() + 1, row.argv.end());
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        if (!safe_token(arguments[index])) {
            diagnostic = "reviewed run-validation-recipe argv contains an unsafe token";
            return false;
        }
        if (arguments[index] == "-Mode") {
            if (index + 1U >= arguments.size() || arguments[index + 1U] != "DryRun") {
                diagnostic =
                    "reviewed run-validation-recipe argv must use -Mode DryRun before editor execution rewrite";
                return false;
            }
            arguments[index + 1U] = "Execute";
            mode_seen = true;
            ++index;
            continue;
        }
        if (arguments[index] == "-Recipe") {
            if (index + 1U >= arguments.size() || arguments[index + 1U] != row.recipe_id) {
                diagnostic = "reviewed run-validation-recipe argv recipe must match the handoff row recipe id";
                return false;
            }
            recipe_seen = true;
            ++index;
        }
    }

    if (!mode_seen || !recipe_seen) {
        diagnostic = "reviewed run-validation-recipe argv must include -Mode DryRun and -Recipe";
        return false;
    }
    return true;
}

void append_stage_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                       const std::vector<EditorAiCommandPanelStageRow>& rows) {
    add_or_throw(document, make_child("ai_commands.stages", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId stage_root{"ai_commands.stages"};
    for (const auto& row : rows) {
        const std::string row_id = "ai_commands.stages." + sanitize_element_id(row.id);
        mirakana::ui::ElementDesc item = make_child(row_id, stage_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.id);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{row_id};
        append_label(document, item_id, row_id + ".status", row.status_label);
        append_label(document, item_id, row_id + ".source", row.source_model);
        append_label(document, item_id, row_id + ".diagnostic", row.diagnostic);
    }
}

void append_command_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                         const std::vector<EditorAiCommandPanelCommandRow>& rows) {
    add_or_throw(document, make_child("ai_commands.commands", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId command_root{"ai_commands.commands"};
    for (const auto& row : rows) {
        const std::string row_id = "ai_commands.commands." + sanitize_element_id(row.recipe_id);
        mirakana::ui::ElementDesc item = make_child(row_id, command_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.recipe_id);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{row_id};
        append_label(document, item_id, row_id + ".status", row.status_label);
        append_label(document, item_id, row_id + ".command", row.command_display.empty() ? "-" : row.command_display);
        append_label(document, item_id, row_id + ".host_gates", join_values(row.host_gates));
        append_label(document, item_id, row_id + ".blocked_by", join_values(row.blocked_by));
    }
}

void append_evidence_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                          const std::vector<EditorAiCommandPanelEvidenceRow>& rows) {
    add_or_throw(document, make_child("ai_commands.evidence", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId evidence_root{"ai_commands.evidence"};
    for (const auto& row : rows) {
        const std::string row_id = "ai_commands.evidence." + sanitize_element_id(row.recipe_id);
        mirakana::ui::ElementDesc item = make_child(row_id, evidence_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.recipe_id);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{row_id};
        append_label(document, item_id, row_id + ".status", row.status_label);
        append_label(document, item_id, row_id + ".summary", row.summary.empty() ? "-" : row.summary);
        append_label(document, item_id, row_id + ".host_gates", join_values(row.host_gates));
        append_label(document, item_id, row_id + ".blocked_by", join_values(row.blocked_by));
    }
}

} // namespace

std::string_view editor_ai_command_panel_status_label(EditorAiCommandPanelStatus status) noexcept {
    switch (status) {
    case EditorAiCommandPanelStatus::ready:
        return "ready";
    case EditorAiCommandPanelStatus::blocked:
        return "blocked";
    case EditorAiCommandPanelStatus::host_gated:
        return "host_gated";
    case EditorAiCommandPanelStatus::external_action_required:
        return "external_action_required";
    }
    return "unknown";
}

std::string_view
editor_ai_reviewed_validation_execution_status_label(EditorAiReviewedValidationExecutionStatus status) noexcept {
    switch (status) {
    case EditorAiReviewedValidationExecutionStatus::ready:
        return "ready";
    case EditorAiReviewedValidationExecutionStatus::blocked:
        return "blocked";
    case EditorAiReviewedValidationExecutionStatus::host_gated:
        return "host_gated";
    }
    return "unknown";
}

EditorAiCommandPanelModel make_editor_ai_command_panel_model(const EditorAiCommandPanelDesc& desc) {
    EditorAiCommandPanelModel model;
    model.ready_for_operator_handoff = desc.operator_handoff.ready_for_operator_handoff;
    model.all_required_evidence_passed = desc.evidence_summary.all_required_evidence_passed;
    model.external_action_required =
        desc.workflow_report.external_action_required || desc.workflow_report.handoff_required ||
        desc.workflow_report.remediation_required || desc.evidence_summary.has_missing_evidence ||
        desc.evidence_summary.has_failed_evidence;
    model.remediation_required = desc.workflow_report.remediation_required;
    model.handoff_required = desc.workflow_report.handoff_required;
    model.has_blocking_diagnostics = desc.workflow_report.has_blocking_diagnostics ||
                                     desc.operator_handoff.has_blocking_diagnostics ||
                                     desc.evidence_summary.has_blocking_diagnostics;
    model.has_host_gates = desc.workflow_report.has_host_gates || desc.operator_handoff.has_host_gates ||
                           desc.evidence_summary.has_host_gates;

    reject_mutating_or_executing_inputs(model, desc);
    append_many_unique(model.unsupported_claims, desc.workflow_report.unsupported_claims);
    append_many_unique(model.unsupported_claims, desc.operator_handoff.unsupported_claims);
    append_many_unique(model.unsupported_claims, desc.evidence_summary.unsupported_claims);
    append_many_unique(model.diagnostics, desc.workflow_report.diagnostics);
    append_many_unique(model.diagnostics, desc.operator_handoff.diagnostics);
    append_many_unique(model.diagnostics, desc.evidence_summary.diagnostics);

    for (const auto& row : desc.workflow_report.stage_rows) {
        if (row.status == EditorAiPlaytestOperatorWorkflowStageStatus::ready ||
            row.status == EditorAiPlaytestOperatorWorkflowStageStatus::closed) {
            ++model.ready_stage_count;
        } else if (row.status == EditorAiPlaytestOperatorWorkflowStageStatus::blocked) {
            ++model.blocked_stage_count;
            model.has_blocking_diagnostics = true;
        } else if (row.status == EditorAiPlaytestOperatorWorkflowStageStatus::host_gated) {
            ++model.host_gated_stage_count;
            model.has_host_gates = true;
        } else if (stage_is_external(row.status)) {
            ++model.external_stage_count;
            model.external_action_required = true;
        }

        if (!row.host_gates.empty()) {
            model.has_host_gates = true;
        }
        if (!row.blocked_by.empty()) {
            model.has_blocking_diagnostics = true;
        }

        model.stage_rows.push_back(EditorAiCommandPanelStageRow{
            .id = sanitize_text(row.id),
            .source_model = sanitize_text(row.source_model),
            .status_label = std::string(editor_ai_playtest_operator_workflow_stage_status_label(row.status)),
            .source_row_count = row.source_row_count,
            .source_row_ids = row.source_row_ids,
            .host_gates = row.host_gates,
            .blocked_by = row.blocked_by,
            .diagnostic = sanitize_text(row.diagnostic),
        });
    }

    for (const auto& row : desc.operator_handoff.command_rows) {
        if (!row.host_gates.empty()) {
            model.has_host_gates = true;
        }
        if (!row.blocked_by.empty() || row.status == EditorAiPackageAuthoringDiagnosticStatus::blocked) {
            model.has_blocking_diagnostics = true;
        }
        model.command_rows.push_back(EditorAiCommandPanelCommandRow{
            .recipe_id = sanitize_text(row.recipe_id),
            .status_label = std::string(editor_ai_package_authoring_diagnostic_status_label(row.status)),
            .command_display = sanitize_text(row.command_display),
            .argv = row.argv,
            .host_gates = row.host_gates,
            .blocked_by = row.blocked_by,
            .readiness_dependency = sanitize_text(row.readiness_dependency),
            .diagnostic = sanitize_text(row.diagnostic),
        });
    }

    for (const auto& row : desc.evidence_summary.rows) {
        if (!row.host_gates.empty()) {
            model.has_host_gates = true;
        }
        if (!row.blocked_by.empty() || row.status == EditorAiPlaytestEvidenceStatus::blocked ||
            row.status == EditorAiPlaytestEvidenceStatus::failed) {
            model.external_action_required = true;
        }
        model.evidence_rows.push_back(EditorAiCommandPanelEvidenceRow{
            .recipe_id = sanitize_text(row.recipe_id),
            .status_label = std::string(editor_ai_playtest_evidence_status_label(row.status)),
            .handoff_status_label =
                std::string(editor_ai_package_authoring_diagnostic_status_label(row.handoff_status)),
            .handoff_row_available = row.handoff_row_available,
            .exit_code = row.exit_code,
            .summary = sanitize_text(row.summary),
            .stdout_summary = sanitize_text(row.stdout_summary),
            .stderr_summary = sanitize_text(row.stderr_summary),
            .host_gates = row.host_gates,
            .blocked_by = row.blocked_by,
            .readiness_dependency = sanitize_text(row.readiness_dependency),
            .diagnostic = sanitize_text(row.diagnostic),
        });
    }

    model.status = aggregate_panel_status(model);
    model.status_label = std::string(editor_ai_command_panel_status_label(model.status));
    return model;
}

EditorAiReviewedValidationExecutionModel
make_editor_ai_reviewed_validation_execution_plan(const EditorAiReviewedValidationExecutionDesc& desc) {
    EditorAiReviewedValidationExecutionModel model;
    model.recipe_id = sanitize_text(desc.command_row.recipe_id);
    model.host_gates = desc.command_row.host_gates;
    model.blocked_by = desc.command_row.blocked_by;

    if (desc.request_validation_execution) {
        reject_execution_claim(model, "validation execution",
                               "editor core must not execute validation recipes; it only reviews process commands");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_execution_claim(model, "arbitrary shell",
                               "reviewed validation execution rejects arbitrary shell requests");
    }
    if (desc.request_raw_manifest_command_evaluation) {
        reject_execution_claim(model, "raw manifest command evaluation",
                               "reviewed validation execution rejects raw manifest command evaluation");
    }
    if (desc.request_package_script_execution) {
        reject_execution_claim(model, "package script execution",
                               "reviewed validation execution rejects broad package script execution requests");
    }
    if (desc.request_free_form_manifest_edit) {
        reject_execution_claim(model, "free-form manifest edits",
                               "reviewed validation execution rejects free-form manifest edits");
    }

    if (desc.command_row.recipe_id.empty()) {
        append_execution_blocker(model, "missing-recipe-id", "reviewed validation execution requires a recipe id");
    }
    const bool row_is_host_gated = desc.command_row.status == EditorAiPackageAuthoringDiagnosticStatus::host_gated ||
                                   !desc.command_row.host_gates.empty();
    if (desc.command_row.status != EditorAiPackageAuthoringDiagnosticStatus::ready && !row_is_host_gated) {
        append_execution_blocker(model, "handoff-row-not-ready",
                                 "reviewed validation execution requires a ready handoff row");
    }

    if (row_is_host_gated) {
        model.has_host_gates = true;
        model.host_gate_acknowledgement_required = true;
        if (!desc.acknowledge_host_gates) {
            model.diagnostics.emplace_back("host-gated validation recipe execution requires host-gate acknowledgement");
        } else {
            for (const auto& host_gate : desc.command_row.host_gates) {
                if (!safe_token(host_gate)) {
                    append_execution_blocker(model, "unsafe-host-gate",
                                             "reviewed validation execution rejects unsafe host-gate tokens");
                } else if (!contains_string(desc.acknowledged_host_gates, host_gate)) {
                    append_execution_blocker(model, "missing-host-gate-acknowledgement",
                                             "missing host-gate acknowledgement: " + host_gate);
                }
            }
            for (const auto& acknowledged_host_gate : desc.acknowledged_host_gates) {
                if (!safe_token(acknowledged_host_gate)) {
                    append_execution_blocker(
                        model, "unsafe-host-gate-acknowledgement",
                        "reviewed validation execution rejects unsafe host-gate acknowledgement tokens");
                } else if (!contains_string(desc.command_row.host_gates, acknowledged_host_gate)) {
                    append_execution_blocker(model, "unknown-host-gate-acknowledgement",
                                             "unknown host-gate acknowledgement: " + acknowledged_host_gate);
                }
            }
            if (!model.has_blocking_diagnostics) {
                model.host_gates_acknowledged = true;
                model.acknowledged_host_gates = desc.command_row.host_gates;
            }
        }
    }
    if (!desc.command_row.blocked_by.empty()) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("reviewed validation execution inherits handoff blockers");
    }

    if (!model.host_gate_acknowledgement_required || model.host_gates_acknowledged) {
        std::vector<std::string> arguments;
        std::string diagnostic;
        if (!rewrite_reviewed_runner_argv(desc.command_row, arguments, diagnostic)) {
            append_execution_blocker(model, "invalid-reviewed-runner-argv", std::move(diagnostic));
        } else {
            for (const auto& host_gate : model.acknowledged_host_gates) {
                arguments.emplace_back("-HostGateAcknowledgements");
                arguments.push_back(host_gate);
            }
            mirakana::ProcessCommand command{
                .executable = desc.command_row.argv[0],
                .arguments = std::move(arguments),
                .working_directory = desc.working_directory,
            };
            if (!mirakana::is_safe_reviewed_validation_recipe_invocation(command)) {
                append_execution_blocker(model, "unsafe-process-command",
                                         "reviewed run-validation-recipe argv produced an unsafe process command");
            } else if (!model.has_blocking_diagnostics) {
                model.command = std::move(command);
            }
        }
    }

    if (model.has_blocking_diagnostics) {
        model.status = EditorAiReviewedValidationExecutionStatus::blocked;
    } else if (model.host_gate_acknowledgement_required && !model.host_gates_acknowledged) {
        model.status = EditorAiReviewedValidationExecutionStatus::host_gated;
    } else {
        model.status = EditorAiReviewedValidationExecutionStatus::ready;
        model.can_execute = true;
    }
    model.status_label = std::string(editor_ai_reviewed_validation_execution_status_label(model.status));
    return model;
}

EditorAiReviewedValidationExecutionBatchModel
make_editor_ai_reviewed_validation_execution_batch(const EditorAiReviewedValidationExecutionBatchDesc& desc) {
    EditorAiReviewedValidationExecutionBatchModel model;
    model.plans.reserve(desc.command_rows.size());
    model.commands.reserve(desc.command_rows.size());
    model.executable_plan_indexes.reserve(desc.command_rows.size());

    if (desc.command_rows.empty()) {
        model.diagnostics.emplace_back("batch has no reviewed validation rows");
        return model;
    }

    for (const auto& row : desc.command_rows) {
        EditorAiReviewedValidationExecutionDesc plan_desc;
        plan_desc.command_row = row;
        plan_desc.working_directory = desc.working_directory;
        plan_desc.acknowledge_host_gates = contains_string(desc.acknowledged_host_gate_recipe_ids, row.recipe_id);
        if (plan_desc.acknowledge_host_gates) {
            plan_desc.acknowledged_host_gates = row.host_gates;
        }

        auto plan = make_editor_ai_reviewed_validation_execution_plan(plan_desc);
        switch (plan.status) {
        case EditorAiReviewedValidationExecutionStatus::ready:
            ++model.ready_count;
            break;
        case EditorAiReviewedValidationExecutionStatus::blocked:
            ++model.blocked_count;
            break;
        case EditorAiReviewedValidationExecutionStatus::host_gated:
            ++model.host_gated_count;
            break;
        }

        if (plan.can_execute) {
            model.executable_plan_indexes.push_back(model.plans.size());
            model.commands.push_back(plan.command);
        }
        model.plans.push_back(std::move(plan));
    }

    model.can_execute_any = !model.commands.empty();
    if (!model.can_execute_any) {
        model.diagnostics.emplace_back("batch has no executable reviewed validation rows");
    }
    return model;
}

mirakana::ui::UiDocument make_ai_command_panel_ui_model(const EditorAiCommandPanelModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("ai_commands", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"ai_commands"};

    append_label(document, root, "ai_commands.status", model.status_label);
    append_label(document, root, "ai_commands.operator_handoff",
                 model.ready_for_operator_handoff ? "operator handoff ready" : "operator handoff not ready");
    append_stage_rows(document, root, model.stage_rows);
    append_command_rows(document, root, model.command_rows);
    append_evidence_rows(document, root, model.evidence_rows);

    add_or_throw(document, make_child("ai_commands.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{"ai_commands.diagnostics"};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, "ai_commands.diagnostics." + std::to_string(index),
                     model.diagnostics[index]);
    }

    return document;
}

mirakana::ui::UiDocument
make_ai_reviewed_validation_execution_batch_ui_model(const EditorAiReviewedValidationExecutionBatchModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("ai_commands.execution.batch", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"ai_commands.execution.batch"};

    append_label(document, root, "ai_commands.execution.batch.ready_count", std::to_string(model.ready_count));
    append_label(document, root, "ai_commands.execution.batch.host_gated_count",
                 std::to_string(model.host_gated_count));
    append_label(document, root, "ai_commands.execution.batch.blocked_count", std::to_string(model.blocked_count));
    append_label(document, root, "ai_commands.execution.batch.executable_count", std::to_string(model.commands.size()));
    append_label(document, root, "ai_commands.execution.batch.can_execute_any",
                 model.can_execute_any ? "true" : "false");

    add_or_throw(document,
                 make_child("ai_commands.execution.batch.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{"ai_commands.execution.batch.diagnostics"};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, "ai_commands.execution.batch.diagnostics." + std::to_string(index),
                     model.diagnostics[index]);
    }

    return document;
}

mirakana::ui::UiDocument
make_ai_reviewed_validation_execution_ui_model(const EditorAiReviewedValidationExecutionModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("ai_commands.execution", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"ai_commands.execution"};

    append_label(document, root, "ai_commands.execution.status", model.status_label);
    append_label(document, root, "ai_commands.execution.can_execute", model.can_execute ? "true" : "false");

    if (!model.recipe_id.empty()) {
        const std::string row_id = "ai_commands.execution." + sanitize_element_id(model.recipe_id);
        mirakana::ui::ElementDesc item = make_child(row_id, root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(model.recipe_id);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{row_id};
        append_label(document, item_id, row_id + ".status", model.status_label);
        append_label(document, item_id, row_id + ".command", make_process_command_display(model.command));
        append_label(document, item_id, row_id + ".host_gates", join_values(model.host_gates));
        append_label(document, item_id, row_id + ".host_gate_acknowledgement_required",
                     model.host_gate_acknowledgement_required ? "true" : "false");
        append_label(document, item_id, row_id + ".host_gates_acknowledged",
                     model.host_gates_acknowledged ? "true" : "false");
        append_label(document, item_id, row_id + ".acknowledged_host_gates",
                     join_values(model.acknowledged_host_gates));
        append_label(document, item_id, row_id + ".blocked_by", join_values(model.blocked_by));
    }

    add_or_throw(document, make_child("ai_commands.execution.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{"ai_commands.execution.diagnostics"};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, "ai_commands.execution.diagnostics." + std::to_string(index),
                     model.diagnostics[index]);
    }
    return document;
}

} // namespace mirakana::editor
