// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/generated_game_studio.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
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

[[nodiscard]] bool intersects(const std::vector<std::string>& lhs, const std::vector<std::string>& rhs) {
    return std::ranges::any_of(lhs, [&rhs](const std::string& value) { return contains_string(rhs, value); });
}

[[nodiscard]] bool workflow_stage_is_design_global(const EditorAiPlaytestOperatorWorkflowStageRow& stage) noexcept {
    return stage.source_row_ids.empty() || stage.source_model == "EditorAiPackageAuthoringDiagnosticsModel" ||
           stage.source_model == "EditorAiPlaytestReadinessReportModel";
}

[[nodiscard]] bool workflow_stage_matches_design(const EditorAiPlaytestOperatorWorkflowStageRow& stage,
                                                 const EditorAiGeneratedGameStudioV1DesignSpecRow& design_spec) {
    return workflow_stage_is_design_global(stage) ||
           intersects(stage.source_row_ids, design_spec.validation_recipe_ids);
}

[[nodiscard]] std::size_t matching_source_row_count(const EditorAiPlaytestOperatorWorkflowStageRow& stage,
                                                    const EditorAiGeneratedGameStudioV1DesignSpecRow& design_spec) {
    if (workflow_stage_is_design_global(stage)) {
        return stage.source_row_count;
    }
    return static_cast<std::size_t>(
        std::ranges::count_if(stage.source_row_ids, [&design_spec](const std::string& row_id) {
            return contains_string(design_spec.validation_recipe_ids, row_id);
        }));
}

[[nodiscard]] bool command_matches_design(const EditorAiCommandPanelCommandRow& row,
                                          const EditorAiGeneratedGameStudioV1DesignSpecRow& design_spec) {
    return contains_string(design_spec.validation_recipe_ids, row.recipe_id);
}

[[nodiscard]] bool evidence_matches_design(const EditorAiCommandPanelEvidenceRow& row,
                                           const EditorAiGeneratedGameStudioV1DesignSpecRow& design_spec) {
    return contains_string(design_spec.validation_recipe_ids, row.recipe_id);
}

[[nodiscard]] bool design_required_evidence_passed(const EditorAiGeneratedGameStudioV1DesignSpecRow& design_spec,
                                                   const std::vector<EditorAiCommandPanelEvidenceRow>& evidence_rows) {
    return !design_spec.validation_recipe_ids.empty() &&
           std::ranges::all_of(design_spec.validation_recipe_ids, [&evidence_rows](const std::string& recipe_id) {
               return std::ranges::any_of(evidence_rows, [&recipe_id](const EditorAiCommandPanelEvidenceRow& row) {
                   return row.recipe_id == recipe_id && row.status_label == "passed";
               });
           });
}

[[nodiscard]] bool loop_status_is_external(EditorAiGeneratedGameStudioV1Status status) noexcept {
    return status == EditorAiGeneratedGameStudioV1Status::awaiting_external_evidence ||
           status == EditorAiGeneratedGameStudioV1Status::remediation_required ||
           status == EditorAiGeneratedGameStudioV1Status::handoff_required;
}

[[nodiscard]] EditorAiGeneratedGameStudioV1Status
aggregate_status(const EditorAiGeneratedGameStudioV1Model& model) noexcept {
    if (model.has_blocking_diagnostics || model.blocked_loop_count > 0U) {
        return EditorAiGeneratedGameStudioV1Status::blocked;
    }
    if (model.remediation_required) {
        return EditorAiGeneratedGameStudioV1Status::remediation_required;
    }
    if (model.handoff_required) {
        return EditorAiGeneratedGameStudioV1Status::handoff_required;
    }
    if (model.all_required_evidence_passed && model.closed_loop_count == model.loop_rows.size() &&
        !model.loop_rows.empty()) {
        return EditorAiGeneratedGameStudioV1Status::closed;
    }
    if (model.external_action_required || model.external_loop_count > 0U) {
        return EditorAiGeneratedGameStudioV1Status::awaiting_external_evidence;
    }
    if (model.has_host_gates || model.host_gated_loop_count > 0U) {
        return EditorAiGeneratedGameStudioV1Status::host_gated;
    }
    return EditorAiGeneratedGameStudioV1Status::ready;
}

[[nodiscard]] std::size_t source_row_count_for_stage(const EditorAiPlaytestOperatorWorkflowReportModel& workflow,
                                                     std::string_view stage_id) noexcept {
    std::size_t count = 0;
    for (const auto& row : workflow.stage_rows) {
        if (row.id == stage_id) {
            count += row.source_row_count;
        }
    }
    return count;
}

void reject_unsupported_claim(EditorAiGeneratedGameStudioV1Model& model, std::string claim, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.unsupported_claims, std::move(claim));
    model.diagnostics.push_back(std::move(diagnostic));
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("generated game studio ui element could not be added");
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

void append_loop_row(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                     const EditorAiGeneratedGameStudioV1LoopRow& row) {
    const std::string row_id = "generated_game_studio.loops." + sanitize_element_id(row.id);
    const mirakana::ui::ElementId item_id{row_id};
    mirakana::ui::ElementDesc item = make_child(row_id, root, mirakana::ui::SemanticRole::list_item);
    item.text = make_text(row.design_spec_id);
    add_or_throw(document, std::move(item));
    append_label(document, item_id, row_id + ".status", row.status_label);
    append_label(document, item_id, row_id + ".gameplay_family", row.gameplay_family);
    append_label(document, item_id, row_id + ".template", row.template_id);
    append_label(document, item_id, row_id + ".package_target", row.package_target_id);
    append_label(document, item_id, row_id + ".recipes", std::to_string(row.validation_recipe_count));
    append_label(document, item_id, row_id + ".evidence", std::to_string(row.evidence_row_count));
    append_label(document, item_id, row_id + ".remediation", std::to_string(row.remediation_row_count));
    append_label(document, item_id, row_id + ".handoff", std::to_string(row.handoff_row_count));
}

void append_string_list(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root, std::string id,
                        const std::vector<std::string>& values) {
    add_or_throw(document, make_child(id, root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId list_id{id};
    for (std::size_t index = 0; index < values.size(); ++index) {
        const std::string item_id = id + "." + std::to_string(index + 1U);
        mirakana::ui::ElementDesc item = make_child(item_id, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(values[index]);
        add_or_throw(document, std::move(item));
    }
}

} // namespace

std::string_view editor_ai_generated_game_studio_v1_status_label(EditorAiGeneratedGameStudioV1Status status) noexcept {
    switch (status) {
    case EditorAiGeneratedGameStudioV1Status::ready:
        return "ready";
    case EditorAiGeneratedGameStudioV1Status::blocked:
        return "blocked";
    case EditorAiGeneratedGameStudioV1Status::host_gated:
        return "host_gated";
    case EditorAiGeneratedGameStudioV1Status::awaiting_external_evidence:
        return "awaiting_external_evidence";
    case EditorAiGeneratedGameStudioV1Status::remediation_required:
        return "remediation_required";
    case EditorAiGeneratedGameStudioV1Status::handoff_required:
        return "handoff_required";
    case EditorAiGeneratedGameStudioV1Status::closed:
        return "closed";
    }
    return "unknown";
}

EditorAiGeneratedGameStudioV1Model
make_editor_ai_generated_game_studio_v1_model(const EditorAiGeneratedGameStudioV1Desc& desc) {
    EditorAiGeneratedGameStudioV1Model model;
    model.design_specs = desc.design_specs;
    model.operator_workflow_report = desc.operator_workflow_report;
    model.ai_command_panel = desc.ai_command_panel;
    model.ready_for_operator_handoff = desc.ai_command_panel.ready_for_operator_handoff;
    model.all_required_evidence_passed = desc.ai_command_panel.all_required_evidence_passed;
    model.external_action_required =
        desc.operator_workflow_report.external_action_required || desc.ai_command_panel.external_action_required;
    model.remediation_required =
        desc.operator_workflow_report.remediation_required || desc.ai_command_panel.remediation_required;
    model.handoff_required = desc.operator_workflow_report.handoff_required || desc.ai_command_panel.handoff_required;
    model.has_blocking_diagnostics =
        desc.operator_workflow_report.has_blocking_diagnostics || desc.ai_command_panel.has_blocking_diagnostics;
    model.has_host_gates = desc.operator_workflow_report.has_host_gates || desc.ai_command_panel.has_host_gates;
    model.workflow_stage_count = desc.operator_workflow_report.stage_rows.size();
    model.command_row_count = desc.ai_command_panel.command_rows.size();
    model.evidence_row_count = desc.ai_command_panel.evidence_rows.size();
    model.remediation_row_count = source_row_count_for_stage(desc.operator_workflow_report, "remediation-queue");
    model.handoff_row_count = source_row_count_for_stage(desc.operator_workflow_report, "remediation-handoff");

    append_many_unique(model.unsupported_claims, desc.operator_workflow_report.unsupported_claims);
    append_many_unique(model.unsupported_claims, desc.ai_command_panel.unsupported_claims);
    append_many_unique(model.diagnostics, desc.operator_workflow_report.diagnostics);
    append_many_unique(model.diagnostics, desc.ai_command_panel.diagnostics);

    if (desc.operator_workflow_report.mutates || desc.ai_command_panel.mutates) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("generated game studio v1 input must not mutate manifests, evidence, or fixes");
    }
    if (desc.operator_workflow_report.executes || desc.ai_command_panel.executes) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back(
            "generated game studio v1 input must not execute validation recipes, package scripts, or shell commands");
    }

    if (desc.design_specs.empty()) {
        model.has_blocking_diagnostics = true;
        model.diagnostics.emplace_back("generated game studio v1 requires at least one design spec row");
    }
    if (desc.request_manifest_mutation) {
        reject_unsupported_claim(model, "manifest mutation",
                                 "generated game studio v1 reviews manifest state but does not mutate manifests");
    }
    if (desc.request_validation_execution) {
        reject_unsupported_claim(model, "validation execution",
                                 "generated game studio v1 does not execute validation recipes from editor core");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell",
                                 "generated game studio v1 rejects arbitrary shell execution claims");
    }
    if (desc.request_engine_internal_mutation) {
        reject_unsupported_claim(model, "engine internal mutation",
                                 "generated game studio v1 routes missing engine capabilities through handoff rows");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "generated game studio v1 must not expose renderer or RHI handles");
    }
    if (desc.request_broad_editor_productization) {
        reject_unsupported_claim(
            model, "broad editor productization",
            "generated game studio v1 is a reviewed workflow surface, not broad editor productization");
    }

    for (const auto& design_spec : model.design_specs) {
        bool row_has_hard_blocker = false;
        EditorAiGeneratedGameStudioV1LoopRow row{
            .id = sanitize_element_id(design_spec.id),
            .design_spec_id = sanitize_text(design_spec.id),
            .gameplay_family = sanitize_text(design_spec.gameplay_family),
            .template_id = sanitize_text(design_spec.template_id),
            .package_target_id = sanitize_text(design_spec.package_target_id),
            .status = EditorAiGeneratedGameStudioV1Status::ready,
            .status_label = {},
            .three_dimensional = design_spec.three_dimensional,
            .host_gated_package_evidence_required = design_spec.host_gated_package_evidence_required,
            .validation_recipe_count = design_spec.validation_recipe_ids.size(),
            .package_registration_draft_count = design_spec.scene_package_registration_draft_rows.size(),
            .scene_node_count = design_spec.scene_nodes.size(),
            .workflow_stage_count = 0,
            .command_row_count = 0,
            .evidence_row_count = 0,
            .remediation_row_count = 0,
            .handoff_row_count = 0,
            .host_gates = {},
            .blocked_by = {},
            .diagnostics = {},
        };

        for (const auto& diagnostic : design_spec.diagnostics) {
            row.diagnostics.push_back(sanitize_text(diagnostic.diagnostic));
        }
        for (const auto& workflow_stage : desc.operator_workflow_report.stage_rows) {
            if (!workflow_stage_matches_design(workflow_stage, design_spec)) {
                continue;
            }
            ++row.workflow_stage_count;
            if (workflow_stage.id == "remediation-queue") {
                row.remediation_row_count += matching_source_row_count(workflow_stage, design_spec);
            }
            if (workflow_stage.id == "remediation-handoff") {
                row.handoff_row_count += matching_source_row_count(workflow_stage, design_spec);
            }
            append_many_unique(row.host_gates, workflow_stage.host_gates);
            append_many_unique(row.blocked_by, workflow_stage.blocked_by);
            if (workflow_stage.status == EditorAiPlaytestOperatorWorkflowStageStatus::blocked) {
                append_unique(row.blocked_by, "workflow-stage-blocked");
                row_has_hard_blocker = true;
            }
            if (workflow_stage.status == EditorAiPlaytestOperatorWorkflowStageStatus::host_gated) {
                append_unique(row.host_gates, "workflow-stage-host-gated");
            }
        }
        for (const auto& command : desc.ai_command_panel.command_rows) {
            if (!command_matches_design(command, design_spec)) {
                continue;
            }
            ++row.command_row_count;
            append_many_unique(row.host_gates, command.host_gates);
            append_many_unique(row.blocked_by, command.blocked_by);
            if (command.status_label == "blocked") {
                append_unique(row.blocked_by, "command-row-blocked");
                row_has_hard_blocker = true;
            }
            if (command.status_label == "host_gated") {
                append_unique(row.host_gates, "command-row-host-gated");
            }
        }
        for (const auto& evidence : desc.ai_command_panel.evidence_rows) {
            if (!evidence_matches_design(evidence, design_spec)) {
                continue;
            }
            ++row.evidence_row_count;
            append_many_unique(row.host_gates, evidence.host_gates);
            append_many_unique(row.blocked_by, evidence.blocked_by);
            if (evidence.status_label == "host_gated") {
                append_unique(row.host_gates, "evidence-host-gated");
            }
        }
        if (design_spec.id.empty()) {
            append_unique(row.blocked_by, "missing-design-spec-id");
            row.diagnostics.emplace_back("generated game studio design spec id is required");
            row_has_hard_blocker = true;
        }
        if (design_spec.gameplay_family.empty()) {
            append_unique(row.blocked_by, "missing-gameplay-family");
            row.diagnostics.emplace_back("generated game studio design spec gameplay family is required");
            row_has_hard_blocker = true;
        }
        if (design_spec.template_id.empty()) {
            append_unique(row.blocked_by, "missing-template-id");
            row.diagnostics.emplace_back("generated game studio design spec template id is required");
            row_has_hard_blocker = true;
        }
        if (design_spec.package_target_id.empty()) {
            append_unique(row.blocked_by, "missing-package-target-id");
            row.diagnostics.emplace_back("generated game studio design spec package target id is required");
            row_has_hard_blocker = true;
        }
        if (design_spec.validation_recipe_ids.empty()) {
            append_unique(row.blocked_by, "missing-validation-recipes");
            row.diagnostics.emplace_back("generated game studio design spec requires validation recipe ids");
            row_has_hard_blocker = true;
        }
        if (design_spec.host_gated_package_evidence_required) {
            append_unique(row.host_gates, "package-evidence-host-gated");
        }

        const bool row_remediation_required =
            row.remediation_row_count > 0U ||
            std::ranges::any_of(desc.ai_command_panel.evidence_rows, [&design_spec](const auto& evidence) {
                return evidence_matches_design(evidence, design_spec) &&
                       (evidence.status_label == "failed" || evidence.status_label == "blocked");
            });
        const bool row_handoff_required = row.handoff_row_count > 0U;
        const bool row_external_action_required =
            std::ranges::any_of(desc.operator_workflow_report.stage_rows,
                                [&design_spec](const auto& stage) {
                                    return workflow_stage_matches_design(stage, design_spec) &&
                                           stage.status ==
                                               EditorAiPlaytestOperatorWorkflowStageStatus::awaiting_external_evidence;
                                }) ||
            std::ranges::any_of(desc.ai_command_panel.evidence_rows, [&design_spec](const auto& evidence) {
                return evidence_matches_design(evidence, design_spec) && evidence.status_label == "missing";
            });

        if (row_has_hard_blocker || model.has_blocking_diagnostics) {
            row.status = EditorAiGeneratedGameStudioV1Status::blocked;
        } else if (row_remediation_required) {
            row.status = EditorAiGeneratedGameStudioV1Status::remediation_required;
        } else if (row_handoff_required) {
            row.status = EditorAiGeneratedGameStudioV1Status::handoff_required;
        } else if (desc.operator_workflow_report.closeout_complete &&
                   design_required_evidence_passed(design_spec, desc.ai_command_panel.evidence_rows)) {
            row.status = EditorAiGeneratedGameStudioV1Status::closed;
        } else if (row_external_action_required) {
            row.status = EditorAiGeneratedGameStudioV1Status::awaiting_external_evidence;
        } else if (!row.host_gates.empty()) {
            row.status = EditorAiGeneratedGameStudioV1Status::host_gated;
        }
        row.status_label = std::string(editor_ai_generated_game_studio_v1_status_label(row.status));

        switch (row.status) {
        case EditorAiGeneratedGameStudioV1Status::ready:
            ++model.ready_loop_count;
            break;
        case EditorAiGeneratedGameStudioV1Status::blocked:
            ++model.blocked_loop_count;
            model.has_blocking_diagnostics = true;
            break;
        case EditorAiGeneratedGameStudioV1Status::host_gated:
            ++model.host_gated_loop_count;
            model.has_host_gates = true;
            break;
        case EditorAiGeneratedGameStudioV1Status::awaiting_external_evidence:
        case EditorAiGeneratedGameStudioV1Status::remediation_required:
        case EditorAiGeneratedGameStudioV1Status::handoff_required:
            ++model.external_loop_count;
            break;
        case EditorAiGeneratedGameStudioV1Status::closed:
            ++model.closed_loop_count;
            break;
        }
        if (loop_status_is_external(row.status)) {
            model.external_action_required = true;
        }
        model.loop_rows.push_back(std::move(row));
    }

    model.status = aggregate_status(model);
    model.status_label = std::string(editor_ai_generated_game_studio_v1_status_label(model.status));
    return model;
}

mirakana::ui::UiDocument
make_editor_ai_generated_game_studio_v1_ui_model(const EditorAiGeneratedGameStudioV1Model& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("generated_game_studio", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"generated_game_studio"};
    append_label(document, root, "generated_game_studio.status", model.status_label);
    append_label(document, root, "generated_game_studio.operator_handoff",
                 model.ready_for_operator_handoff ? "operator handoff ready" : "operator handoff not ready");
    append_label(document, root, "generated_game_studio.loop_count", std::to_string(model.loop_rows.size()));
    append_label(document, root, "generated_game_studio.evidence_count", std::to_string(model.evidence_row_count));
    append_label(document, root, "generated_game_studio.remediation_count",
                 std::to_string(model.remediation_row_count));
    append_label(document, root, "generated_game_studio.handoff_count", std::to_string(model.handoff_row_count));

    add_or_throw(document, make_child("generated_game_studio.loops", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId loops_root{"generated_game_studio.loops"};
    for (const auto& row : model.loop_rows) {
        append_loop_row(document, loops_root, row);
    }

    append_string_list(document, root, "generated_game_studio.unsupported_claims", model.unsupported_claims);
    append_string_list(document, root, "generated_game_studio.diagnostics", model.diagnostics);
    return document;
}

} // namespace mirakana::editor
