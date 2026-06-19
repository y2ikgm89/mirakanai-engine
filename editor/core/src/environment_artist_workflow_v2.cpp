// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/environment_artist_workflow_v2.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>

namespace mirakana::editor {
namespace {

constexpr std::string_view kProductionRecipeId{"environment-artist-workflow-production-closeout"};

using Kind = EnvironmentArtistWorkflowProductionRequirementKind;
using RequirementRow = EnvironmentArtistWorkflowProductionRequirementRow;

[[nodiscard]] constexpr std::array<Kind, 14U> required_kinds() noexcept {
    return {
        Kind::import_openexr,
        Kind::import_ktx2_basis,
        Kind::import_gltf_material,
        Kind::review_usd_materialx_ocio,
        Kind::cook_package,
        Kind::live_preview_d3d12,
        Kind::live_preview_vulkan,
        Kind::live_preview_metal_host,
        Kind::weather_timeline_edit,
        Kind::preset_batch_apply,
        Kind::validation_report,
        Kind::profiler_artifact_review,
        Kind::undo_redo_revision_safety,
        Kind::operator_review,
    };
}

[[nodiscard]] std::string_view requirement_label(Kind kind) noexcept {
    switch (kind) {
    case Kind::import_openexr:
        return "Import OpenEXR";
    case Kind::import_ktx2_basis:
        return "Import KTX2/Basis";
    case Kind::import_gltf_material:
        return "Import glTF Material";
    case Kind::review_usd_materialx_ocio:
        return "Review USD/MaterialX/OCIO";
    case Kind::cook_package:
        return "Cook Package";
    case Kind::live_preview_d3d12:
        return "Live Preview D3D12";
    case Kind::live_preview_vulkan:
        return "Live Preview Vulkan";
    case Kind::live_preview_metal_host:
        return "Live Preview Metal Host";
    case Kind::weather_timeline_edit:
        return "Weather Timeline Edit";
    case Kind::preset_batch_apply:
        return "Preset Batch Apply";
    case Kind::validation_report:
        return "Validation Report";
    case Kind::profiler_artifact_review:
        return "Profiler Artifact Review";
    case Kind::undo_redo_revision_safety:
        return "Undo/Redo Revision Safety";
    case Kind::operator_review:
        return "Operator Review";
    }
    return "";
}

[[nodiscard]] std::string_view expected_evidence_id(Kind kind) noexcept {
    switch (kind) {
    case Kind::import_openexr:
        return "workflow_import_openexr_ready";
    case Kind::import_ktx2_basis:
        return "workflow_import_ktx2_basis_ready";
    case Kind::import_gltf_material:
        return "workflow_import_gltf_material_ready";
    case Kind::review_usd_materialx_ocio:
        return "workflow_review_usd_materialx_ocio_ready";
    case Kind::cook_package:
        return "workflow_cook_package_ready";
    case Kind::live_preview_d3d12:
        return "workflow_live_preview_d3d12_ready";
    case Kind::live_preview_vulkan:
        return "workflow_live_preview_vulkan_ready";
    case Kind::live_preview_metal_host:
        return "workflow_live_preview_metal_host_ready";
    case Kind::weather_timeline_edit:
        return "workflow_weather_timeline_edit_ready";
    case Kind::preset_batch_apply:
        return "workflow_preset_batch_apply_ready";
    case Kind::validation_report:
        return "workflow_validation_report_ready";
    case Kind::profiler_artifact_review:
        return "workflow_profiler_artifact_review_ready";
    case Kind::undo_redo_revision_safety:
        return "workflow_undo_redo_revision_safety_ready";
    case Kind::operator_review:
        return "workflow_operator_review_ready";
    }
    return "";
}

[[nodiscard]] const EnvironmentArtistWorkflowProductionRequirementInputRow*
find_requirement_input(std::span<const EnvironmentArtistWorkflowProductionRequirementInputRow> rows,
                       Kind kind) noexcept {
    const auto it = std::ranges::find_if(
        rows, [kind](const EnvironmentArtistWorkflowProductionRequirementInputRow& row) { return row.kind == kind; });
    return it == rows.end() ? nullptr : std::addressof(*it);
}

void append_unique(std::vector<std::string>& values, std::string value) {
    if (!value.empty() && std::ranges::find(values, value) == values.end()) {
        values.push_back(std::move(value));
    }
}

void append_diagnostic(EnvironmentArtistWorkflowProductionCloseoutModel& model, std::string value) {
    if (!value.empty()) {
        model.diagnostics.push_back(std::move(value));
    }
}

[[nodiscard]] bool selected_package_ready(const EnvironmentArtistWorkflowReadyReviewModel& model) noexcept {
    return model.status == EnvironmentAuthoringStatus::ready && model.environment_artist_workflow_ready &&
           model.ready_rows == 8U && !model.invokes_backend && !model.executes_package_scripts &&
           !model.executes_validation_recipes && !model.exposes_native_handles && model.unsupported_claims.empty() &&
           model.diagnostics.empty();
}

void set_ready_flag(EnvironmentArtistWorkflowProductionCloseoutModel& model, Kind kind, bool ready) noexcept {
    switch (kind) {
    case Kind::import_openexr:
        model.workflow_import_openexr_ready = ready;
        break;
    case Kind::import_ktx2_basis:
        model.workflow_import_ktx2_basis_ready = ready;
        break;
    case Kind::import_gltf_material:
        model.workflow_import_gltf_material_ready = ready;
        break;
    case Kind::review_usd_materialx_ocio:
        model.workflow_review_usd_materialx_ocio_ready = ready;
        break;
    case Kind::cook_package:
        model.workflow_cook_package_ready = ready;
        break;
    case Kind::live_preview_d3d12:
        model.workflow_live_preview_d3d12_ready = ready;
        break;
    case Kind::live_preview_vulkan:
        model.workflow_live_preview_vulkan_ready = ready;
        break;
    case Kind::live_preview_metal_host:
        model.workflow_live_preview_metal_host_ready = ready;
        break;
    case Kind::weather_timeline_edit:
        model.workflow_weather_timeline_edit_ready = ready;
        break;
    case Kind::preset_batch_apply:
        model.workflow_preset_batch_apply_ready = ready;
        break;
    case Kind::validation_report:
        model.workflow_validation_report_ready = ready;
        break;
    case Kind::profiler_artifact_review:
        model.workflow_profiler_artifact_review_ready = ready;
        break;
    case Kind::undo_redo_revision_safety:
        model.workflow_undo_redo_revision_safety_ready = ready;
        break;
    case Kind::operator_review:
        model.workflow_operator_review_ready = ready;
        break;
    }
}

[[nodiscard]] mirakana::ui::ElementDesc make_element(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.visible = true;
    desc.enabled = true;
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_child(std::string id, mirakana::ui::ElementId parent,
                                                   mirakana::ui::SemanticRole role) {
    auto desc = make_element(std::move(id), role);
    desc.parent = std::move(parent);
    return desc;
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    return text;
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::runtime_error{"duplicate environment artist workflow production UI element"};
    }
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    auto desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

} // namespace

std::string_view environment_artist_workflow_production_requirement_id(Kind kind) noexcept {
    switch (kind) {
    case Kind::import_openexr:
        return "environment.workflow.production.import_openexr";
    case Kind::import_ktx2_basis:
        return "environment.workflow.production.import_ktx2_basis";
    case Kind::import_gltf_material:
        return "environment.workflow.production.import_gltf_material";
    case Kind::review_usd_materialx_ocio:
        return "environment.workflow.production.review_usd_materialx_ocio";
    case Kind::cook_package:
        return "environment.workflow.production.cook_package";
    case Kind::live_preview_d3d12:
        return "environment.workflow.production.live_preview_d3d12";
    case Kind::live_preview_vulkan:
        return "environment.workflow.production.live_preview_vulkan";
    case Kind::live_preview_metal_host:
        return "environment.workflow.production.live_preview_metal_host";
    case Kind::weather_timeline_edit:
        return "environment.workflow.production.weather_timeline_edit";
    case Kind::preset_batch_apply:
        return "environment.workflow.production.preset_batch_apply";
    case Kind::validation_report:
        return "environment.workflow.production.validation_report";
    case Kind::profiler_artifact_review:
        return "environment.workflow.production.profiler_artifact_review";
    case Kind::undo_redo_revision_safety:
        return "environment.workflow.production.undo_redo_revision_safety";
    case Kind::operator_review:
        return "environment.workflow.production.operator_review";
    }
    return "";
}

std::string_view environment_artist_workflow_production_requirement_status_label(
    EnvironmentArtistWorkflowProductionRequirementStatus status) noexcept {
    switch (status) {
    case EnvironmentArtistWorkflowProductionRequirementStatus::ready:
        return "ready";
    case EnvironmentArtistWorkflowProductionRequirementStatus::missing:
        return "missing";
    case EnvironmentArtistWorkflowProductionRequirementStatus::blocked:
        return "blocked";
    }
    return "unknown";
}

std::string_view environment_artist_workflow_production_closeout_status_label(
    EnvironmentArtistWorkflowProductionCloseoutStatus status) noexcept {
    switch (status) {
    case EnvironmentArtistWorkflowProductionCloseoutStatus::blocked:
        return "blocked";
    case EnvironmentArtistWorkflowProductionCloseoutStatus::ready:
        return "ready";
    }
    return "unknown";
}

EnvironmentArtistWorkflowProductionCloseoutModel
evaluate_environment_artist_workflow_production_closeout(const EnvironmentArtistWorkflowProductionCloseoutDesc& desc) {
    EnvironmentArtistWorkflowProductionCloseoutModel model;
    constexpr auto kinds = required_kinds();
    model.required_rows = kinds.size();
    model.rows.reserve(kinds.size());
    model.selected_package_workflow_ready = selected_package_ready(desc.selected_package_workflow);

    const bool unsafe_request = desc.request_backend_execution || desc.request_package_script_execution ||
                                desc.request_validation_recipe_execution || desc.request_native_handle_access;
    if (desc.request_backend_execution) {
        append_unique(model.unsupported_claims, "backend execution");
    }
    if (desc.request_package_script_execution) {
        append_unique(model.unsupported_claims, "package script execution");
    }
    if (desc.request_validation_recipe_execution) {
        append_unique(model.unsupported_claims, "validation recipe execution");
    }
    if (desc.request_native_handle_access) {
        append_unique(model.unsupported_claims, "native handle access");
    }
    if (!model.selected_package_workflow_ready) {
        append_unique(model.unsupported_claims, "selected package artist workflow not ready");
    }

    for (const auto kind : kinds) {
        RequirementRow row{
            .row_id = std::string{environment_artist_workflow_production_requirement_id(kind)},
            .label = std::string{requirement_label(kind)},
            .kind = kind,
            .diagnostic = "missing reviewed package-visible host-validated production workflow evidence",
        };

        const auto* input = find_requirement_input(desc.requirements, kind);
        if (input == nullptr) {
            append_unique(row.blocked_by, "missing-requirement-row");
        } else {
            row.evidence_id = input->evidence_id;
            row.reviewed = input->reviewed;
            row.package_visible = input->package_visible;
            row.host_validated = input->host_validated;
            row.validation_recipe_id = input->validation_recipe_id;
            row.retained_ui_row_ids = input->retained_ui_row_ids;
        }

        const bool evidence_id_matches = input != nullptr && input->evidence_id == expected_evidence_id(kind);
        const bool recipe_matches = input != nullptr && input->validation_recipe_id == kProductionRecipeId;
        const bool input_ready = input != nullptr && input->ready && input->reviewed && input->package_visible &&
                                 input->host_validated && evidence_id_matches && recipe_matches &&
                                 !input->retained_ui_row_ids.empty();
        if (input == nullptr || !input_ready) {
            if (input != nullptr && !input->ready) {
                append_unique(row.blocked_by, "not-ready");
            }
            if (input != nullptr && !input->reviewed) {
                append_unique(row.blocked_by, "not-reviewed");
            }
            if (input != nullptr && !input->package_visible) {
                append_unique(row.blocked_by, "not-package-visible");
            }
            if (input != nullptr && !input->host_validated) {
                append_unique(row.blocked_by, "not-host-validated");
            }
            if (input != nullptr && !evidence_id_matches) {
                append_unique(row.blocked_by, "unexpected-evidence-id");
            }
            if (input != nullptr && !recipe_matches) {
                append_unique(row.blocked_by, "unexpected-validation-recipe");
            }
            if (input != nullptr && input->retained_ui_row_ids.empty()) {
                append_unique(row.blocked_by, "missing-retained-ui-row");
            }
        }

        if (input_ready && !unsafe_request && model.selected_package_workflow_ready) {
            row.status = EnvironmentArtistWorkflowProductionRequirementStatus::ready;
            row.diagnostic = "reviewed package-visible host-validated production workflow evidence is ready";
            ++model.ready_rows;
            set_ready_flag(model, kind, true);
        } else if (input_ready) {
            row.status = EnvironmentArtistWorkflowProductionRequirementStatus::blocked;
            row.diagnostic = "production workflow evidence is blocked by selected workflow or unsafe request";
            if (unsafe_request) {
                append_unique(row.blocked_by, "editor-core-execution-requested");
            }
            if (!model.selected_package_workflow_ready) {
                append_unique(row.blocked_by, "selected-package-workflow-not-ready");
            }
        }

        if (row.status != EnvironmentArtistWorkflowProductionRequirementStatus::ready) {
            append_diagnostic(model, row.row_id + ": " + row.diagnostic);
        }
        model.rows.push_back(std::move(row));
    }

    if (!desc.request_environment_artist_workflow_production_ready || !model.unsupported_claims.empty() ||
        model.ready_rows != model.required_rows) {
        append_unique(model.unsupported_claims, "environment_artist_workflow_production_ready");
    }

    if (desc.request_environment_artist_workflow_production_ready && model.unsupported_claims.empty() &&
        model.ready_rows == model.required_rows) {
        model.environment_artist_workflow_production_ready = true;
        model.status = EnvironmentArtistWorkflowProductionCloseoutStatus::ready;
    }

    return model;
}

mirakana::ui::UiDocument make_environment_artist_workflow_production_closeout_ui_model(
    const EnvironmentArtistWorkflowProductionCloseoutModel& model) {
    mirakana::ui::UiDocument document;
    auto root = make_element("environment_artist_workflow_production_closeout", mirakana::ui::SemanticRole::panel);
    root.accessibility_label = "Environment Artist Workflow Production Closeout";
    add_or_throw(document, std::move(root));

    const mirakana::ui::ElementId root_id{"environment_artist_workflow_production_closeout"};
    append_label(document, root_id, "environment_artist_workflow_production_closeout.status",
                 std::string{environment_artist_workflow_production_closeout_status_label(model.status)});
    append_label(document, root_id, "environment_artist_workflow_production_closeout.ready",
                 model.environment_artist_workflow_production_ready ? "true" : "false");
    append_label(document, root_id, "environment_artist_workflow_production_closeout.ready_rows",
                 std::to_string(model.ready_rows));

    add_or_throw(document, make_child("environment_artist_workflow_production_closeout.rows", root_id,
                                      mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId rows_id{"environment_artist_workflow_production_closeout.rows"};
    for (const auto& row : model.rows) {
        const auto item_id_string = "environment_artist_workflow_production_closeout.rows." + row.row_id;
        auto item = make_child(item_id_string, rows_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = false;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{item_id_string};
        append_label(document, item_id, item_id_string + ".status",
                     std::string{environment_artist_workflow_production_requirement_status_label(row.status)});
        append_label(document, item_id, item_id_string + ".evidence", row.evidence_id);
    }

    return document;
}

} // namespace mirakana::editor
