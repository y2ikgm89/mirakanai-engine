// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/environment_authoring.hpp"

#include "mirakana/environment/environment_io.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] UndoableAction empty_action() {
    return UndoableAction{};
}

[[nodiscard]] std::string bool_text(bool value) {
    return value ? "true" : "false";
}

[[nodiscard]] std::string format_float(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << value;
    auto text = out.str();
    while (text.size() > 1U && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text;
}

[[nodiscard]] std::string format_vec2(Vec2 value) {
    return format_float(value.x) + "," + format_float(value.y);
}

[[nodiscard]] std::string format_vec3(Vec3 value) {
    return format_float(value.x) + "," + format_float(value.y) + "," + format_float(value.z);
}

[[nodiscard]] std::string format_uint(std::size_t value) {
    return std::to_string(value);
}

void add_row(EnvironmentAuthoringInspectorModel& model, std::string id, std::string section, std::string label,
             std::string value, bool editable = true) {
    model.rows.push_back(EnvironmentAuthoringInspectorRow{
        .id = std::move(id),
        .section = std::move(section),
        .label = std::move(label),
        .value = std::move(value),
        .editable = editable,
    });
}

void add_readiness_row(EnvironmentAuthoringInspectorModel& model, std::string id, std::string section,
                       std::string label, std::string value) {
    add_row(model, std::move(id), std::move(section), std::move(label), std::move(value), false);
}

[[nodiscard]] const EnvironmentAuthoringInspectorRow*
find_inspector_row(const EnvironmentAuthoringInspectorModel& model, std::string_view id) noexcept {
    const auto it =
        std::ranges::find_if(model.rows, [id](const EnvironmentAuthoringInspectorRow& row) { return row.id == id; });
    return it == model.rows.end() ? nullptr : &(*it);
}

[[nodiscard]] bool has_nonzero_row_value(const EnvironmentAuthoringInspectorModel& model, std::string_view id) {
    const auto* row = find_inspector_row(model, id);
    return row != nullptr && !row->value.empty() && row->value != "0";
}

void add_default_readiness_rows(EnvironmentAuthoringInspectorModel& model) {
    add_readiness_row(model, "environment.readiness.physical_sky.package_status", "Readiness", "Physical Sky Package",
                      "ready");
    add_readiness_row(model, "environment.readiness.height_fog.status", "Readiness", "Height Fog", "ready");
    add_readiness_row(model, "environment.readiness.volumetric_fog.package_status", "Readiness",
                      "Volumetric Fog Package", "ready");
    add_readiness_row(model, "environment.readiness.cloud_layer.renderer_status", "Readiness", "Cloud Layer Renderer",
                      "ready");
    add_readiness_row(model, "environment.readiness.volumetric_clouds.renderer_status", "Readiness",
                      "Volumetric Clouds Renderer", "ready");
    add_readiness_row(model, "environment.readiness.precipitation.rain_renderer_status", "Readiness", "Rain Renderer",
                      "ready");
    add_readiness_row(model, "environment.readiness.precipitation.snow_renderer_status", "Readiness", "Snow Renderer",
                      "ready");
    add_readiness_row(model, "environment.readiness.ibl.package_status", "Readiness", "IBL Package", "ready");
    add_readiness_row(model, "environment.readiness.backend.d3d12_status", "Backend Evidence", "D3D12", "ready");
    add_readiness_row(model, "environment.readiness.backend.vulkan_status", "Backend Evidence", "Vulkan", "host_gated");
    add_readiness_row(model, "environment.readiness.backend.metal_status", "Backend Evidence", "Metal", "host_gated");
    add_readiness_row(model, "environment.readiness.unsupported.environment_ready", "Unsupported Claims",
                      "Broad Environment Ready", "unclaimed");
    add_readiness_row(model, "environment.readiness.unsupported.backend_parity", "Unsupported Claims", "Backend Parity",
                      "unclaimed");
    add_readiness_row(model, "environment.readiness.unsupported.public_backend_handles", "Unsupported Claims",
                      "Public Backend Handles", "unsupported");
}

void add_diagnostics(EnvironmentAuthoringValidationModel& model, const EnvironmentProfileValidationResult& validation) {
    for (const auto& diagnostic : validation.diagnostics) {
        model.diagnostics.push_back(EnvironmentAuthoringDiagnosticRow{
            .profile_code = diagnostic.code,
            .field = diagnostic.field,
            .profile_id = diagnostic.id,
            .message = diagnostic.message,
        });
    }
}

[[nodiscard]] mirakana::ui::ElementId element_id(std::string value) {
    return mirakana::ui::ElementId{.value = std::move(value)};
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "editor-ui";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

[[nodiscard]] mirakana::ui::ElementDesc make_element(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = element_id(std::move(id));
    desc.role = role;
    desc.accessibility_label = desc.id.value;
    desc.style.layout = mirakana::ui::LayoutMode::column;
    desc.style.anchor = mirakana::ui::AnchorMode::fill;
    desc.style.gap = 4.0F;
    desc.style.padding = mirakana::ui::EdgeInsets{.top = 4.0F, .right = 4.0F, .bottom = 4.0F, .left = 4.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_child(std::string id, const mirakana::ui::ElementId& parent,
                                                   mirakana::ui::SemanticRole role) {
    auto desc = make_element(std::move(id), role);
    desc.parent = parent;
    return desc;
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("environment authoring ui element could not be added");
    }
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    auto desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    desc.accessibility_label = desc.text.label;
    add_or_throw(document, std::move(desc));
}

[[nodiscard]] std::string normalize_path(std::string_view path) {
    std::string value{path};
    std::ranges::replace(value, '\\', '/');
    while (value.contains("//")) {
        value.erase(value.find("//"), 1U);
    }
    return value;
}

[[nodiscard]] std::string lower_ascii(std::string_view value) {
    std::string out{value};
    for (char& ch : out) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return out;
}

[[nodiscard]] bool starts_with_case_insensitive(std::string_view value, std::string_view prefix) {
    return lower_ascii(value).starts_with(lower_ascii(prefix));
}

[[nodiscard]] std::string strip_project_root(std::string_view path, std::string_view project_root_path) {
    auto normalized = normalize_path(path);
    auto root = normalize_path(project_root_path);
    if (!root.empty() && !root.ends_with('/')) {
        root.push_back('/');
    }
    if (!root.empty() && starts_with_case_insensitive(normalized, root)) {
        normalized.erase(0, root.size());
    }
    return normalized;
}

[[nodiscard]] bool safe_runtime_package_path(std::string_view path) {
    if (path.empty() || path.find_first_of("\r\n=") != std::string_view::npos || path.contains("..")) {
        return false;
    }
    if (path.starts_with('/') || path.starts_with('\\')) {
        return false;
    }
    if (path.size() > 2U && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':') {
        return false;
    }
    return normalize_path(path).starts_with("runtime/");
}

[[nodiscard]] std::string package_key(std::string_view path) {
    return lower_ascii(normalize_path(path));
}

void add_command_diagnostic(EnvironmentAuthoringCommandPlan& plan, std::string code, std::string message) {
    plan.diagnostics.push_back(EnvironmentAuthoringCommandDiagnosticRow{
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_artist_workflow_diagnostic(EnvironmentArtistWorkflowCommandPlan& plan, std::string code, std::string message) {
    plan.diagnostics.push_back(EnvironmentArtistWorkflowCommandDiagnosticRow{
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_artist_workflow_report_row(EnvironmentArtistWorkflowCommandPlan& plan, std::string id, std::string label,
                                    std::string value) {
    plan.report_rows.push_back(EnvironmentArtistWorkflowCommandReportRow{
        .id = std::move(id),
        .label = std::move(label),
        .value = std::move(value),
    });
}

void add_preset_library_readiness_diagnostic(EnvironmentPresetLibraryReadinessModel& model, std::string field,
                                             std::string message) {
    model.diagnostics.push_back(EnvironmentAuthoringDiagnosticRow{
        .field = std::move(field),
        .profile_id = model.profile_id,
        .message = std::move(message),
    });
}

void add_preset_library_readiness_requirement(EnvironmentPresetLibraryReadinessModel& model, std::string id,
                                              std::string label, bool ready, std::string diagnostic_message) {
    add_readiness_row(model, std::move(id), "AAA Preset Library Readiness", std::move(label),
                      ready ? "ready" : "blocked");
    if (!ready) {
        add_preset_library_readiness_diagnostic(model, model.rows.back().id, std::move(diagnostic_message));
    }
}

void accept_command(EnvironmentAuthoringCommandPlan& plan) noexcept {
    plan.status = EnvironmentAuthoringCommandStatus::accepted;
}

[[nodiscard]] std::size_t find_volume_index_by_id(const EnvironmentProfileDocumentV2& document,
                                                  std::string_view volume_id) {
    const auto it = std::ranges::find_if(
        document.volumes, [volume_id](const EnvironmentVolumeDesc& volume) { return volume.id == volume_id; });
    return it == document.volumes.end() ? document.volumes.size()
                                        : static_cast<std::size_t>(std::distance(document.volumes.begin(), it));
}

[[nodiscard]] bool volume_id_exists(const EnvironmentProfileDocumentV2& document, std::string_view volume_id) {
    return find_volume_index_by_id(document, volume_id) != document.volumes.size();
}

[[nodiscard]] std::string command_label(const EnvironmentAuthoringCommandRequest& request) {
    if (!request.label.empty()) {
        return request.label;
    }
    switch (request.kind) {
    case EnvironmentAuthoringCommandKind::add_volume:
        return "Add Environment Volume";
    case EnvironmentAuthoringCommandKind::remove_volume:
        return "Remove Environment Volume";
    case EnvironmentAuthoringCommandKind::reorder_volume:
        return "Reorder Environment Volume";
    case EnvironmentAuthoringCommandKind::edit_weather_keyframe:
        return "Edit Weather Keyframe";
    case EnvironmentAuthoringCommandKind::select_quality_preset:
        return "Select Environment Quality";
    case EnvironmentAuthoringCommandKind::request_cubemap_capture:
        return "Request Environment Cubemap Capture";
    }
    return "Edit Environment";
}

[[nodiscard]] std::string command_id(EnvironmentAuthoringCommandKind kind) {
    switch (kind) {
    case EnvironmentAuthoringCommandKind::add_volume:
        return "environment.command.volume.add";
    case EnvironmentAuthoringCommandKind::remove_volume:
        return "environment.command.volume.remove";
    case EnvironmentAuthoringCommandKind::reorder_volume:
        return "environment.command.volume.reorder";
    case EnvironmentAuthoringCommandKind::edit_weather_keyframe:
        return "environment.command.weather_keyframe.edit";
    case EnvironmentAuthoringCommandKind::select_quality_preset:
        return "environment.command.quality_preset.select";
    case EnvironmentAuthoringCommandKind::request_cubemap_capture:
        return "environment.command.capture.cubemap.request";
    }
    return "environment.command.invalid";
}

[[nodiscard]] std::string_view artist_workflow_command_label(EnvironmentArtistWorkflowCommandKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowCommandKind::preset_import:
        return "Import Environment Preset";
    case EnvironmentArtistWorkflowCommandKind::source_asset_review:
        return "Review Environment Source Asset";
    case EnvironmentArtistWorkflowCommandKind::cook_preview:
        return "Preview Environment Cook";
    case EnvironmentArtistWorkflowCommandKind::profile_graph_edit:
        return "Edit Environment Profile Graph";
    case EnvironmentArtistWorkflowCommandKind::weather_timeline_edit:
        return "Edit Weather Timeline";
    case EnvironmentArtistWorkflowCommandKind::local_volume_edit:
        return "Edit Local Environment Volume";
    case EnvironmentArtistWorkflowCommandKind::simulation_parameter_edit:
        return "Edit Weather Simulation Parameters";
    case EnvironmentArtistWorkflowCommandKind::quality_budget_edit:
        return "Edit Environment Quality Budget";
    case EnvironmentArtistWorkflowCommandKind::package_preview:
        return "Preview Environment Package";
    case EnvironmentArtistWorkflowCommandKind::validation_remediation:
        return "Remediate Environment Validation";
    case EnvironmentArtistWorkflowCommandKind::publish_package:
        return "Publish Environment Package";
    }
    return "Environment Workflow Command";
}

[[nodiscard]] bool artist_workflow_command_mutates(EnvironmentArtistWorkflowCommandKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowCommandKind::preset_import:
    case EnvironmentArtistWorkflowCommandKind::profile_graph_edit:
    case EnvironmentArtistWorkflowCommandKind::weather_timeline_edit:
    case EnvironmentArtistWorkflowCommandKind::local_volume_edit:
    case EnvironmentArtistWorkflowCommandKind::simulation_parameter_edit:
    case EnvironmentArtistWorkflowCommandKind::quality_budget_edit:
    case EnvironmentArtistWorkflowCommandKind::validation_remediation:
        return true;
    case EnvironmentArtistWorkflowCommandKind::source_asset_review:
    case EnvironmentArtistWorkflowCommandKind::cook_preview:
    case EnvironmentArtistWorkflowCommandKind::package_preview:
    case EnvironmentArtistWorkflowCommandKind::publish_package:
        return false;
    }
    return false;
}

[[nodiscard]] bool artist_workflow_command_requires_confirmation(EnvironmentArtistWorkflowCommandKind kind) noexcept {
    return kind == EnvironmentArtistWorkflowCommandKind::publish_package;
}

[[nodiscard]] std::string_view artist_workflow_asset_label(EnvironmentArtistWorkflowAssetKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowAssetKind::preset_library:
        return "Preset Library";
    case EnvironmentArtistWorkflowAssetKind::openexr_source:
        return "OpenEXR Source";
    case EnvironmentArtistWorkflowAssetKind::ktx2_basis_source:
        return "KTX2/Basis Source";
    case EnvironmentArtistWorkflowAssetKind::cooked_texture:
        return "Cooked Texture";
    case EnvironmentArtistWorkflowAssetKind::environment_profile:
        return "Environment Profile";
    case EnvironmentArtistWorkflowAssetKind::simulation_preset:
        return "Simulation Preset";
    case EnvironmentArtistWorkflowAssetKind::validation_report:
        return "Validation Report";
    case EnvironmentArtistWorkflowAssetKind::package_artifact:
        return "Package Artifact";
    }
    return "Environment Asset";
}

[[nodiscard]] std::string_view asset_row_status_label(EnvironmentArtistWorkflowAssetRowStatus status) noexcept {
    switch (status) {
    case EnvironmentArtistWorkflowAssetRowStatus::ready:
        return "ready";
    case EnvironmentArtistWorkflowAssetRowStatus::missing:
        return "missing";
    case EnvironmentArtistWorkflowAssetRowStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

[[nodiscard]] std::string_view preview_row_status_label(EnvironmentArtistWorkflowPreviewRowStatus status) noexcept {
    switch (status) {
    case EnvironmentArtistWorkflowPreviewRowStatus::ready:
        return "ready";
    case EnvironmentArtistWorkflowPreviewRowStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

[[nodiscard]] std::string_view artist_workflow_preview_label(EnvironmentArtistWorkflowPreviewRowKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowPreviewRowKind::selected_backend:
        return "Selected Backend";
    case EnvironmentArtistWorkflowPreviewRowKind::quality_tier:
        return "Quality Tier";
    case EnvironmentArtistWorkflowPreviewRowKind::missing_host_gate:
        return "Missing Host Gate";
    case EnvironmentArtistWorkflowPreviewRowKind::package_budget:
        return "Package Budget";
    case EnvironmentArtistWorkflowPreviewRowKind::memory_budget:
        return "Memory Budget";
    case EnvironmentArtistWorkflowPreviewRowKind::diagnostics:
        return "Diagnostics";
    case EnvironmentArtistWorkflowPreviewRowKind::unsupported_claim_reason:
        return "Unsupported Claim Reason";
    }
    return "Environment Preview Row";
}

[[nodiscard]] std::string_view
walkthrough_step_status_label(EnvironmentArtistWorkflowWalkthroughStepStatus status) noexcept {
    switch (status) {
    case EnvironmentArtistWorkflowWalkthroughStepStatus::ready:
        return "ready";
    case EnvironmentArtistWorkflowWalkthroughStepStatus::missing:
        return "missing";
    case EnvironmentArtistWorkflowWalkthroughStepStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

[[nodiscard]] std::string_view
artist_workflow_walkthrough_label(EnvironmentArtistWorkflowWalkthroughStepKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowWalkthroughStepKind::import_source_assets:
        return "Import Source Assets";
    case EnvironmentArtistWorkflowWalkthroughStepKind::cook_assets:
        return "Cook Assets";
    case EnvironmentArtistWorkflowWalkthroughStepKind::assemble_preset:
        return "Assemble Preset";
    case EnvironmentArtistWorkflowWalkthroughStepKind::edit_weather_timeline:
        return "Edit Weather Timeline";
    case EnvironmentArtistWorkflowWalkthroughStepKind::run_simulation_preview:
        return "Run Simulation Preview";
    case EnvironmentArtistWorkflowWalkthroughStepKind::package_sample:
        return "Package Sample";
    case EnvironmentArtistWorkflowWalkthroughStepKind::run_installed_validation:
        return "Run Installed Validation";
    case EnvironmentArtistWorkflowWalkthroughStepKind::inspect_report:
        return "Inspect Report";
    }
    return "Environment Walkthrough Step";
}

[[nodiscard]] std::string_view artist_workflow_execution_stage_label(std::string_view row_id) noexcept {
    if (row_id == "environment.workflow.execution.command_catalog") {
        return "Command Catalog";
    }
    if (row_id == "environment.workflow.execution.asset_browser") {
        return "Asset Browser";
    }
    if (row_id == "environment.workflow.execution.preview") {
        return "Preview";
    }
    if (row_id == "environment.workflow.execution.walkthrough") {
        return "Walkthrough";
    }
    if (row_id == "environment.workflow.execution.external_execution") {
        return "External Execution";
    }
    if (row_id == "environment.workflow.execution.evidence_review") {
        return "Evidence Review";
    }
    if (row_id == "environment.workflow.execution.operator_review") {
        return "Operator Review";
    }
    if (row_id == "environment.workflow.execution.ready_promotion_guard") {
        return "Ready Promotion Guard";
    }
    return "Environment Artist Workflow Execution";
}

[[nodiscard]] std::string_view
artist_workflow_ready_requirement_label(EnvironmentArtistWorkflowReadyRequirementKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowReadyRequirementKind::visible_editor_shell:
        return "Visible Editor Shell";
    case EnvironmentArtistWorkflowReadyRequirementKind::asset_pipeline:
        return "Asset Pipeline";
    case EnvironmentArtistWorkflowReadyRequirementKind::selected_preset_library:
        return "Selected Preset Library";
    case EnvironmentArtistWorkflowReadyRequirementKind::validation_remediation:
        return "Validation Remediation";
    case EnvironmentArtistWorkflowReadyRequirementKind::revision_safety:
        return "Revision Safety";
    case EnvironmentArtistWorkflowReadyRequirementKind::production_walkthrough_package:
        return "Production Walkthrough Package";
    case EnvironmentArtistWorkflowReadyRequirementKind::editor_core_execution_boundary:
        return "Editor Core Execution Boundary";
    case EnvironmentArtistWorkflowReadyRequirementKind::operator_review:
        return "Operator Review";
    }
    return "Environment Artist Workflow Ready Requirement";
}

[[nodiscard]] EnvironmentArtistWorkflowCommandKind
walkthrough_step_command_kind(EnvironmentArtistWorkflowWalkthroughStepKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowWalkthroughStepKind::import_source_assets:
        return EnvironmentArtistWorkflowCommandKind::source_asset_review;
    case EnvironmentArtistWorkflowWalkthroughStepKind::cook_assets:
        return EnvironmentArtistWorkflowCommandKind::cook_preview;
    case EnvironmentArtistWorkflowWalkthroughStepKind::assemble_preset:
        return EnvironmentArtistWorkflowCommandKind::preset_import;
    case EnvironmentArtistWorkflowWalkthroughStepKind::edit_weather_timeline:
        return EnvironmentArtistWorkflowCommandKind::weather_timeline_edit;
    case EnvironmentArtistWorkflowWalkthroughStepKind::run_simulation_preview:
        return EnvironmentArtistWorkflowCommandKind::simulation_parameter_edit;
    case EnvironmentArtistWorkflowWalkthroughStepKind::package_sample:
        return EnvironmentArtistWorkflowCommandKind::package_preview;
    case EnvironmentArtistWorkflowWalkthroughStepKind::run_installed_validation:
        return EnvironmentArtistWorkflowCommandKind::validation_remediation;
    case EnvironmentArtistWorkflowWalkthroughStepKind::inspect_report:
        return EnvironmentArtistWorkflowCommandKind::publish_package;
    }
    return EnvironmentArtistWorkflowCommandKind::source_asset_review;
}

void append_unique_string(std::vector<std::string>& values, std::string_view value) {
    if (value.empty()) {
        return;
    }
    const auto it = std::ranges::find_if(values, [value](const std::string& current) { return current == value; });
    if (it == values.end()) {
        values.emplace_back(value);
    }
}

void append_unique_strings(std::vector<std::string>& values, std::span<const std::string> source) {
    for (const auto& value : source) {
        append_unique_string(values, value);
    }
}

[[nodiscard]] bool contains_string(std::span<const std::string> values, std::string_view value) noexcept {
    return std::ranges::find_if(values, [value](const std::string& current) { return current == value; }) !=
           values.end();
}

[[nodiscard]] std::string join_strings(std::span<const std::string> values) {
    if (values.empty()) {
        return "none";
    }
    std::string joined;
    for (const auto& value : values) {
        if (!joined.empty()) {
            joined += ",";
        }
        joined += value;
    }
    return joined;
}

[[nodiscard]] const EnvironmentArtistWorkflowExecutionEvidenceRow*
find_execution_evidence(std::span<const EnvironmentArtistWorkflowExecutionEvidenceRow> evidence_rows,
                        std::string_view recipe_id) noexcept {
    const auto it =
        std::ranges::find_if(evidence_rows, [recipe_id](const EnvironmentArtistWorkflowExecutionEvidenceRow& row) {
            return row.recipe_id == recipe_id;
        });
    return it == evidence_rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const EnvironmentArtistWorkflowReadyRequirementInputRow*
find_ready_requirement_input(std::span<const EnvironmentArtistWorkflowReadyRequirementInputRow> rows,
                             EnvironmentArtistWorkflowReadyRequirementKind kind) noexcept {
    const auto it = std::ranges::find_if(
        rows, [kind](const EnvironmentArtistWorkflowReadyRequirementInputRow& row) { return row.kind == kind; });
    return it == rows.end() ? nullptr : &(*it);
}

void append_execution_unsupported(EnvironmentArtistWorkflowExecutionReviewModel& model, std::string_view claim,
                                  std::string diagnostic) {
    append_unique_string(model.unsupported_claims, claim);
    if (!diagnostic.empty()) {
        model.diagnostics.push_back(std::move(diagnostic));
    }
}

void push_execution_stage(EnvironmentArtistWorkflowExecutionReviewModel& model,
                          EnvironmentArtistWorkflowExecutionReviewStageRow row) {
    if (row.status == EnvironmentArtistWorkflowExecutionStageStatus::blocked) {
        model.has_blocking_diagnostics = true;
    }
    if (row.status == EnvironmentArtistWorkflowExecutionStageStatus::host_gated || !row.host_gates.empty()) {
        model.has_host_gates = true;
    }
    model.stage_rows.push_back(std::move(row));
}

void append_ready_unsupported(EnvironmentArtistWorkflowReadyReviewModel& model, std::string_view claim,
                              std::string diagnostic) {
    append_unique_string(model.unsupported_claims, claim);
    if (!diagnostic.empty()) {
        model.diagnostics.push_back(std::move(diagnostic));
    }
}

[[nodiscard]] EnvironmentArtistWorkflowExecutionStageStatus
source_model_stage_status(EnvironmentAuthoringStatus status, bool has_rows, bool has_host_gates) noexcept {
    if (has_host_gates) {
        return EnvironmentArtistWorkflowExecutionStageStatus::host_gated;
    }
    if (status == EnvironmentAuthoringStatus::ready && has_rows) {
        return EnvironmentArtistWorkflowExecutionStageStatus::ready;
    }
    return EnvironmentArtistWorkflowExecutionStageStatus::blocked;
}

[[nodiscard]] const EnvironmentArtistWorkflowAssetBrowserInputRow*
find_asset_browser_input(std::span<const EnvironmentArtistWorkflowAssetBrowserInputRow> inputs,
                         EnvironmentArtistWorkflowAssetKind kind) noexcept {
    const auto it = std::ranges::find_if(
        inputs, [kind](const EnvironmentArtistWorkflowAssetBrowserInputRow& row) { return row.kind == kind; });
    return it == inputs.end() ? nullptr : &(*it);
}

[[nodiscard]] const EnvironmentArtistWorkflowWalkthroughStepInputRow*
find_walkthrough_input(std::span<const EnvironmentArtistWorkflowWalkthroughStepInputRow> inputs,
                       EnvironmentArtistWorkflowWalkthroughStepKind kind) noexcept {
    const auto it = std::ranges::find_if(
        inputs, [kind](const EnvironmentArtistWorkflowWalkthroughStepInputRow& row) { return row.kind == kind; });
    return it == inputs.end() ? nullptr : &(*it);
}

void add_asset_browser_diagnostic(EnvironmentArtistWorkflowAssetBrowserModel& model, std::string code,
                                  std::string message) {
    model.diagnostics.push_back(EnvironmentArtistWorkflowAssetBrowserDiagnosticRow{
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_preview_diagnostic(EnvironmentArtistWorkflowPreviewModel& model, std::string code, std::string message) {
    model.diagnostics.push_back(EnvironmentArtistWorkflowPreviewDiagnosticRow{
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_walkthrough_diagnostic(EnvironmentArtistWorkflowWalkthroughModel& model, std::string code,
                                std::string message) {
    model.diagnostics.push_back(EnvironmentArtistWorkflowWalkthroughDiagnosticRow{
        .code = std::move(code),
        .message = std::move(message),
    });
}

[[nodiscard]] EnvironmentArtistWorkflowCommandRow
make_artist_workflow_command_row(EnvironmentArtistWorkflowCommandKind kind) {
    const bool mutates = artist_workflow_command_mutates(kind);
    return EnvironmentArtistWorkflowCommandRow{
        .kind = kind,
        .command_id = std::string{environment_artist_workflow_command_id(kind)},
        .label = std::string{artist_workflow_command_label(kind)},
        .mutates_document = mutates,
        .supports_dry_run = true,
        .supports_revision_checked_apply = true,
        .supports_undo_metadata = mutates,
        .requires_confirmation = artist_workflow_command_requires_confirmation(kind),
        .invokes_backend = false,
        .exposes_native_handles = false,
        .executes_package_scripts = false,
    };
}

[[nodiscard]] EnvironmentArtistWorkflowWalkthroughStepRow
make_walkthrough_step_row(const EnvironmentArtistWorkflowWalkthroughStepInputRow* input,
                          EnvironmentArtistWorkflowWalkthroughStepKind kind) {
    EnvironmentArtistWorkflowWalkthroughStepRow row{
        .kind = kind,
        .row_id = std::string{environment_artist_workflow_walkthrough_step_id(kind)},
        .label = std::string{artist_workflow_walkthrough_label(kind)},
        .command_id = std::string{environment_artist_workflow_command_id(walkthrough_step_command_kind(kind))},
        .invokes_backend = false,
        .exposes_native_handles = false,
        .executes_package_scripts = false,
    };
    if (input == nullptr) {
        return row;
    }

    row.evidence_id = input->evidence_id;
    row.completed = input->completed;
    row.reviewed = input->reviewed;
    row.package_visible = input->package_visible;
    row.requires_host_gate = input->requires_host_gate;
    row.host_gate = input->host_gate;
    row.validation_recipe_id = input->validation_recipe_id;
    if (row.requires_host_gate) {
        row.status = EnvironmentArtistWorkflowWalkthroughStepStatus::blocked;
    } else if (row.completed && row.reviewed && !row.evidence_id.empty() && !row.validation_recipe_id.empty()) {
        row.status = EnvironmentArtistWorkflowWalkthroughStepStatus::ready;
    } else {
        row.status = EnvironmentArtistWorkflowWalkthroughStepStatus::blocked;
    }
    return row;
}

void add_artist_workflow_report_rows(EnvironmentArtistWorkflowCommandPlan& plan) {
    add_artist_workflow_report_row(plan, "environment.workflow.command_id", "Command", plan.command_id);
    add_artist_workflow_report_row(plan, "environment.workflow.label", "Label", plan.label);
    add_artist_workflow_report_row(plan, "environment.workflow.mode", "Mode",
                                   plan.dry_run ? "dry_run" : (plan.apply ? "apply" : "rejected"));
    add_artist_workflow_report_row(plan, "environment.workflow.status", "Status",
                                   plan.status == EnvironmentArtistWorkflowCommandStatus::accepted ? "accepted"
                                                                                                   : "rejected");
    add_artist_workflow_report_row(plan, "environment.workflow.before_revision", "Before Revision",
                                   std::to_string(plan.before_revision));
    add_artist_workflow_report_row(plan, "environment.workflow.after_revision", "After Revision",
                                   std::to_string(plan.after_revision));
    add_artist_workflow_report_row(plan, "environment.workflow.revision_checked", "Revision Checked",
                                   bool_text(plan.revision_checked));
    add_artist_workflow_report_row(plan, "environment.workflow.mutates_document", "Mutates Document",
                                   bool_text(plan.mutates_document));
    add_artist_workflow_report_row(plan, "environment.workflow.undo_supported", "Undo Supported",
                                   bool_text(plan.undo_supported));
    add_artist_workflow_report_row(plan, "environment.workflow.rollback_metadata", "Rollback Metadata",
                                   bool_text(plan.rollback_metadata_available));
    add_artist_workflow_report_row(plan, "environment.workflow.requires_confirmation", "Requires Confirmation",
                                   bool_text(plan.requires_confirmation));
}

[[nodiscard]] EnvironmentArtistWorkflowAssetBrowserRow
make_asset_browser_row(const EnvironmentArtistWorkflowAssetBrowserInputRow* input,
                       EnvironmentArtistWorkflowAssetKind kind) {
    EnvironmentArtistWorkflowAssetBrowserRow row{
        .kind = kind,
        .row_id = std::string{environment_artist_workflow_asset_kind_id(kind)},
        .label = std::string{artist_workflow_asset_label(kind)},
    };
    if (input == nullptr) {
        return row;
    }

    row.path = input->path;
    row.available = input->available && !input->path.empty();
    row.package_visible = input->package_visible;
    row.provenance_recorded = input->provenance_recorded;
    row.budget_recorded = input->budget_recorded;
    row.requires_host_gate = input->requires_host_gate;
    row.host_gate = input->host_gate;
    row.validation_recipe_id = input->validation_recipe_id;
    row.status = row.available ? EnvironmentArtistWorkflowAssetRowStatus::ready
                               : EnvironmentArtistWorkflowAssetRowStatus::missing;
    return row;
}

[[nodiscard]] EnvironmentArtistWorkflowPreviewRow make_preview_row(EnvironmentArtistWorkflowPreviewRowKind kind,
                                                                   std::string value,
                                                                   EnvironmentArtistWorkflowPreviewRowStatus status) {
    return EnvironmentArtistWorkflowPreviewRow{
        .kind = kind,
        .status = status,
        .row_id = std::string{environment_artist_workflow_preview_row_id(kind)},
        .label = std::string{artist_workflow_preview_label(kind)},
        .value = std::move(value),
        .invokes_backend = false,
        .exposes_native_handles = false,
        .executes_package_scripts = false,
    };
}

void apply_command(EnvironmentProfileDocumentV2& document, const EnvironmentAuthoringCommandRequest& request) {
    switch (request.kind) {
    case EnvironmentAuthoringCommandKind::add_volume:
        document.volumes.push_back(request.volume);
        return;
    case EnvironmentAuthoringCommandKind::remove_volume: {
        const auto index = find_volume_index_by_id(document, request.volume_id);
        if (index < document.volumes.size()) {
            document.volumes.erase(document.volumes.begin() + static_cast<std::ptrdiff_t>(index));
        }
        return;
    }
    case EnvironmentAuthoringCommandKind::reorder_volume: {
        const auto source = static_cast<std::size_t>(request.source_index);
        const auto target = static_cast<std::size_t>(request.target_index);
        if (source >= document.volumes.size() || target >= document.volumes.size() || source == target) {
            return;
        }
        auto row = document.volumes[source];
        document.volumes.erase(document.volumes.begin() + static_cast<std::ptrdiff_t>(source));
        document.volumes.insert(document.volumes.begin() + static_cast<std::ptrdiff_t>(target), std::move(row));
        return;
    }
    case EnvironmentAuthoringCommandKind::edit_weather_keyframe:
        if (request.weather_keyframe_index < document.weather_timeline.size()) {
            document.weather_timeline[request.weather_keyframe_index] = request.weather_keyframe;
        }
        return;
    case EnvironmentAuthoringCommandKind::select_quality_preset:
        document.quality_preset = request.quality_preset;
        return;
    case EnvironmentAuthoringCommandKind::request_cubemap_capture:
        return;
    }
}

} // namespace

bool EnvironmentAuthoringValidationModel::succeeded() const noexcept {
    return diagnostics.empty();
}

bool EnvironmentArtistWorkflowCommandPlan::succeeded() const noexcept {
    return status == EnvironmentArtistWorkflowCommandStatus::accepted && diagnostics.empty();
}

EnvironmentAuthoringDocument EnvironmentAuthoringDocument::from_profile(EnvironmentProfileDesc profile,
                                                                        std::string path) {
    EnvironmentProfileDocumentV2 document;
    document.global_profile = std::move(profile);
    return from_profile_document_v2(std::move(document), std::move(path));
}

EnvironmentAuthoringDocument
EnvironmentAuthoringDocument::from_profile_document_v2(EnvironmentProfileDocumentV2 profile, std::string path) {
    EnvironmentAuthoringDocument document;
    document.profile_ = std::move(profile);
    document.path_ = std::move(path);
    document.dirty_.reset_clean();
    return document;
}

const EnvironmentProfileDesc& EnvironmentAuthoringDocument::profile() const noexcept {
    return profile_.global_profile;
}

const EnvironmentProfileDocumentV2& EnvironmentAuthoringDocument::profile_document_v2() const noexcept {
    return profile_;
}

std::string_view EnvironmentAuthoringDocument::path() const noexcept {
    return path_;
}

bool EnvironmentAuthoringDocument::dirty() const noexcept {
    return dirty_.dirty();
}

std::uint64_t EnvironmentAuthoringDocument::revision() const noexcept {
    return dirty_.revision();
}

std::uint64_t EnvironmentAuthoringDocument::saved_revision() const noexcept {
    return dirty_.saved_revision();
}

EnvironmentAuthoringDocument::Snapshot EnvironmentAuthoringDocument::snapshot() const {
    return Snapshot{.profile = profile_, .dirty = dirty_};
}

void EnvironmentAuthoringDocument::restore(Snapshot snapshot) {
    profile_ = std::move(snapshot.profile);
    dirty_ = snapshot.dirty;
}

void EnvironmentAuthoringDocument::replace_profile(EnvironmentProfileDesc profile) {
    profile_.global_profile = std::move(profile);
    dirty_.mark_dirty();
}

void EnvironmentAuthoringDocument::replace_profile_document_v2(EnvironmentProfileDocumentV2 profile) {
    profile_ = std::move(profile);
    dirty_.mark_dirty();
}

void EnvironmentAuthoringDocument::mark_saved() noexcept {
    dirty_.mark_saved();
}

void EnvironmentAuthoringDocument::set_path(std::string path) {
    path_ = std::move(path);
}

std::string_view environment_package_candidate_kind_label(EnvironmentPackageCandidateKind kind) noexcept {
    switch (kind) {
    case EnvironmentPackageCandidateKind::profile_source:
        return "profile_source";
    case EnvironmentPackageCandidateKind::profile_cooked:
        return "profile_cooked";
    case EnvironmentPackageCandidateKind::package_index:
        return "package_index";
    case EnvironmentPackageCandidateKind::preset_pack:
        return "preset_pack";
    }
    return "profile_source";
}

std::string_view
environment_package_registration_draft_status_label(EnvironmentPackageRegistrationDraftStatus status) noexcept {
    switch (status) {
    case EnvironmentPackageRegistrationDraftStatus::add_runtime_file:
        return "add_runtime_file";
    case EnvironmentPackageRegistrationDraftStatus::already_registered:
        return "already_registered";
    case EnvironmentPackageRegistrationDraftStatus::rejected_source_file:
        return "rejected_source_file";
    case EnvironmentPackageRegistrationDraftStatus::rejected_unsafe_path:
        return "rejected_unsafe_path";
    case EnvironmentPackageRegistrationDraftStatus::rejected_duplicate:
        return "rejected_duplicate";
    }
    return "rejected_source_file";
}

std::string_view environment_artist_workflow_command_id(EnvironmentArtistWorkflowCommandKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowCommandKind::preset_import:
        return "environment.command.preset.import";
    case EnvironmentArtistWorkflowCommandKind::source_asset_review:
        return "environment.command.source_asset.review";
    case EnvironmentArtistWorkflowCommandKind::cook_preview:
        return "environment.command.cook.preview";
    case EnvironmentArtistWorkflowCommandKind::profile_graph_edit:
        return "environment.command.profile_graph.edit";
    case EnvironmentArtistWorkflowCommandKind::weather_timeline_edit:
        return "environment.command.weather_timeline.edit";
    case EnvironmentArtistWorkflowCommandKind::local_volume_edit:
        return "environment.command.local_volume.edit";
    case EnvironmentArtistWorkflowCommandKind::simulation_parameter_edit:
        return "environment.command.simulation_parameter.edit";
    case EnvironmentArtistWorkflowCommandKind::quality_budget_edit:
        return "environment.command.quality_budget.edit";
    case EnvironmentArtistWorkflowCommandKind::package_preview:
        return "environment.command.package.preview";
    case EnvironmentArtistWorkflowCommandKind::validation_remediation:
        return "environment.command.validation.remediation";
    case EnvironmentArtistWorkflowCommandKind::publish_package:
        return "environment.command.publish.package";
    }
    return "environment.command.invalid";
}

std::string_view environment_artist_workflow_asset_kind_id(EnvironmentArtistWorkflowAssetKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowAssetKind::preset_library:
        return "environment.workflow.asset.preset_library";
    case EnvironmentArtistWorkflowAssetKind::openexr_source:
        return "environment.workflow.asset.openexr_source";
    case EnvironmentArtistWorkflowAssetKind::ktx2_basis_source:
        return "environment.workflow.asset.ktx2_basis_source";
    case EnvironmentArtistWorkflowAssetKind::cooked_texture:
        return "environment.workflow.asset.cooked_texture";
    case EnvironmentArtistWorkflowAssetKind::environment_profile:
        return "environment.workflow.asset.environment_profile";
    case EnvironmentArtistWorkflowAssetKind::simulation_preset:
        return "environment.workflow.asset.simulation_preset";
    case EnvironmentArtistWorkflowAssetKind::validation_report:
        return "environment.workflow.asset.validation_report";
    case EnvironmentArtistWorkflowAssetKind::package_artifact:
        return "environment.workflow.asset.package_artifact";
    }
    return "environment.workflow.asset.invalid";
}

std::string_view environment_artist_workflow_preview_row_id(EnvironmentArtistWorkflowPreviewRowKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowPreviewRowKind::selected_backend:
        return "environment.workflow.preview.selected_backend";
    case EnvironmentArtistWorkflowPreviewRowKind::quality_tier:
        return "environment.workflow.preview.quality_tier";
    case EnvironmentArtistWorkflowPreviewRowKind::missing_host_gate:
        return "environment.workflow.preview.missing_host_gate";
    case EnvironmentArtistWorkflowPreviewRowKind::package_budget:
        return "environment.workflow.preview.package_budget";
    case EnvironmentArtistWorkflowPreviewRowKind::memory_budget:
        return "environment.workflow.preview.memory_budget";
    case EnvironmentArtistWorkflowPreviewRowKind::diagnostics:
        return "environment.workflow.preview.diagnostics";
    case EnvironmentArtistWorkflowPreviewRowKind::unsupported_claim_reason:
        return "environment.workflow.preview.unsupported_claim_reason";
    }
    return "environment.workflow.preview.invalid";
}

std::string_view
environment_artist_workflow_walkthrough_step_id(EnvironmentArtistWorkflowWalkthroughStepKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowWalkthroughStepKind::import_source_assets:
        return "environment.workflow.walkthrough.import_source_assets";
    case EnvironmentArtistWorkflowWalkthroughStepKind::cook_assets:
        return "environment.workflow.walkthrough.cook_assets";
    case EnvironmentArtistWorkflowWalkthroughStepKind::assemble_preset:
        return "environment.workflow.walkthrough.assemble_preset";
    case EnvironmentArtistWorkflowWalkthroughStepKind::edit_weather_timeline:
        return "environment.workflow.walkthrough.edit_weather_timeline";
    case EnvironmentArtistWorkflowWalkthroughStepKind::run_simulation_preview:
        return "environment.workflow.walkthrough.run_simulation_preview";
    case EnvironmentArtistWorkflowWalkthroughStepKind::package_sample:
        return "environment.workflow.walkthrough.package_sample";
    case EnvironmentArtistWorkflowWalkthroughStepKind::run_installed_validation:
        return "environment.workflow.walkthrough.run_installed_validation";
    case EnvironmentArtistWorkflowWalkthroughStepKind::inspect_report:
        return "environment.workflow.walkthrough.inspect_report";
    }
    return "environment.workflow.walkthrough.invalid";
}

std::string_view environment_artist_workflow_execution_stage_status_label(
    EnvironmentArtistWorkflowExecutionStageStatus status) noexcept {
    switch (status) {
    case EnvironmentArtistWorkflowExecutionStageStatus::ready:
        return "ready";
    case EnvironmentArtistWorkflowExecutionStageStatus::awaiting_external_evidence:
        return "awaiting_external_evidence";
    case EnvironmentArtistWorkflowExecutionStageStatus::awaiting_operator_review:
        return "awaiting_operator_review";
    case EnvironmentArtistWorkflowExecutionStageStatus::host_gated:
        return "host_gated";
    case EnvironmentArtistWorkflowExecutionStageStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

std::string_view
environment_artist_workflow_ready_requirement_id(EnvironmentArtistWorkflowReadyRequirementKind kind) noexcept {
    switch (kind) {
    case EnvironmentArtistWorkflowReadyRequirementKind::visible_editor_shell:
        return "environment.workflow.ready.visible_editor_shell";
    case EnvironmentArtistWorkflowReadyRequirementKind::asset_pipeline:
        return "environment.workflow.ready.asset_pipeline";
    case EnvironmentArtistWorkflowReadyRequirementKind::selected_preset_library:
        return "environment.workflow.ready.selected_preset_library";
    case EnvironmentArtistWorkflowReadyRequirementKind::validation_remediation:
        return "environment.workflow.ready.validation_remediation";
    case EnvironmentArtistWorkflowReadyRequirementKind::revision_safety:
        return "environment.workflow.ready.revision_safety";
    case EnvironmentArtistWorkflowReadyRequirementKind::production_walkthrough_package:
        return "environment.workflow.ready.production_walkthrough_package";
    case EnvironmentArtistWorkflowReadyRequirementKind::editor_core_execution_boundary:
        return "environment.workflow.ready.editor_core_execution_boundary";
    case EnvironmentArtistWorkflowReadyRequirementKind::operator_review:
        return "environment.workflow.ready.operator_review";
    }
    return "environment.workflow.ready.invalid";
}

std::string_view environment_artist_workflow_ready_requirement_status_label(
    EnvironmentArtistWorkflowReadyRequirementStatus status) noexcept {
    switch (status) {
    case EnvironmentArtistWorkflowReadyRequirementStatus::ready:
        return "ready";
    case EnvironmentArtistWorkflowReadyRequirementStatus::missing:
        return "missing";
    case EnvironmentArtistWorkflowReadyRequirementStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

EnvironmentAuthoringDocument load_environment_authoring_document(ITextStore& store, std::string_view path) {
    return EnvironmentAuthoringDocument::from_profile_document_v2(
        deserialize_environment_profile_v2(store.read_text(path)), std::string{path});
}

void save_environment_authoring_document(ITextStore& store, std::string_view path,
                                         EnvironmentAuthoringDocument& document) {
    store.write_text(path, serialize_environment_profile_v2(document.profile_));
    document.set_path(std::string{path});
    document.mark_saved();
}

UndoableAction make_environment_authoring_profile_edit_action(EnvironmentAuthoringDocument& document,
                                                              EnvironmentProfileDesc profile, std::string label) {
    if (label.empty()) {
        return empty_action();
    }

    const auto before = document.snapshot();
    EnvironmentAuthoringDocument after_document = document;
    after_document.replace_profile(std::move(profile));
    const auto after = after_document.snapshot();

    return UndoableAction{
        .label = std::move(label),
        .redo = [&document, after]() { document.restore(after); },
        .undo = [&document, before]() { document.restore(before); },
    };
}

EnvironmentAuthoringCommandPlan plan_environment_authoring_command(const EnvironmentAuthoringDocument& document,
                                                                   const EnvironmentAuthoringCommandRequest& request) {
    EnvironmentAuthoringCommandPlan plan{
        .command_id = command_id(request.kind),
        .label = command_label(request),
    };

    if (request.request_backend_execution || request.request_package_script_execution ||
        request.request_native_handle_access) {
        plan.status = EnvironmentAuthoringCommandStatus::rejected_unsafe_execution;
        add_command_diagnostic(
            plan, "unsafe_execution",
            "environment authoring commands cannot execute backend work, package scripts, or native handle access");
        return plan;
    }

    const auto& profile = document.profile_document_v2();
    switch (request.kind) {
    case EnvironmentAuthoringCommandKind::add_volume: {
        plan.mutates_document = true;
        if (request.volume.id.empty()) {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
            add_command_diagnostic(plan, "empty_volume_id", "environment volume id must not be empty");
        } else if (volume_id_exists(profile, request.volume.id)) {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
            add_command_diagnostic(plan, "duplicate_volume_id", "environment volume id already exists");
        } else {
            auto copy = profile;
            copy.volumes.push_back(request.volume);
            const auto validation = validate_environment_profile_v2(copy);
            if (validation.succeeded()) {
                accept_command(plan);
            } else {
                plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
                add_command_diagnostic(plan, "invalid_profile_v2", "environment profile v2 validation failed");
            }
        }
        return plan;
    }
    case EnvironmentAuthoringCommandKind::remove_volume:
        plan.mutates_document = true;
        if (!volume_id_exists(profile, request.volume_id)) {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_not_found;
            add_command_diagnostic(plan, "volume_not_found", "environment volume id was not found");
        } else {
            accept_command(plan);
        }
        return plan;
    case EnvironmentAuthoringCommandKind::reorder_volume:
        plan.mutates_document = true;
        if (request.source_index >= profile.volumes.size() || request.target_index >= profile.volumes.size()) {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_not_found;
            add_command_diagnostic(plan, "volume_index_not_found", "environment volume reorder index was not found");
        } else if (request.source_index == request.target_index) {
            plan.mutates_document = false;
            plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
            add_command_diagnostic(plan, "volume_reorder_noop", "environment volume reorder requires distinct indexes");
        } else {
            accept_command(plan);
        }
        return plan;
    case EnvironmentAuthoringCommandKind::edit_weather_keyframe:
        plan.mutates_document = true;
        if (request.weather_keyframe_index >= profile.weather_timeline.size()) {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_not_found;
            add_command_diagnostic(plan, "weather_keyframe_not_found", "weather keyframe index was not found");
        } else {
            auto copy = profile;
            copy.weather_timeline[request.weather_keyframe_index] = request.weather_keyframe;
            const auto validation = validate_environment_profile_v2(copy);
            if (validation.succeeded()) {
                accept_command(plan);
            } else {
                plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
                add_command_diagnostic(plan, "invalid_weather_keyframe", "weather keyframe validation failed");
            }
        }
        return plan;
    case EnvironmentAuthoringCommandKind::select_quality_preset: {
        plan.mutates_document = true;
        auto copy = profile;
        copy.quality_preset = request.quality_preset;
        const auto validation = validate_environment_profile_v2(copy);
        if (validation.succeeded()) {
            accept_command(plan);
        } else {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
            add_command_diagnostic(plan, "invalid_quality_preset", "environment quality preset validation failed");
        }
        return plan;
    }
    case EnvironmentAuthoringCommandKind::request_cubemap_capture:
        plan.requests_cubemap_capture = true;
        accept_command(plan);
        return plan;
    }

    return plan;
}

UndoableAction make_environment_authoring_command_action(EnvironmentAuthoringDocument& document,
                                                         const EnvironmentAuthoringCommandRequest& request) {
    const auto plan = plan_environment_authoring_command(document, request);
    if (plan.status != EnvironmentAuthoringCommandStatus::accepted || !plan.mutates_document) {
        return empty_action();
    }

    const auto before = document.snapshot();
    auto next_profile = before.profile;
    apply_command(next_profile, request);
    EnvironmentAuthoringDocument after_document = document;
    after_document.replace_profile_document_v2(std::move(next_profile));
    const auto after = after_document.snapshot();

    return UndoableAction{
        .label = plan.label,
        .redo = [&document, after]() { document.restore(after); },
        .undo = [&document, before]() { document.restore(before); },
    };
}

EnvironmentArtistWorkflowCommandCatalog
make_environment_artist_workflow_command_catalog(const EnvironmentAuthoringDocument& document) {
    constexpr std::array kinds{
        EnvironmentArtistWorkflowCommandKind::preset_import,
        EnvironmentArtistWorkflowCommandKind::source_asset_review,
        EnvironmentArtistWorkflowCommandKind::cook_preview,
        EnvironmentArtistWorkflowCommandKind::profile_graph_edit,
        EnvironmentArtistWorkflowCommandKind::weather_timeline_edit,
        EnvironmentArtistWorkflowCommandKind::local_volume_edit,
        EnvironmentArtistWorkflowCommandKind::simulation_parameter_edit,
        EnvironmentArtistWorkflowCommandKind::quality_budget_edit,
        EnvironmentArtistWorkflowCommandKind::package_preview,
        EnvironmentArtistWorkflowCommandKind::validation_remediation,
        EnvironmentArtistWorkflowCommandKind::publish_package,
    };

    EnvironmentArtistWorkflowCommandCatalog catalog{.revision = document.revision()};
    catalog.commands.reserve(kinds.size());
    for (const auto kind : kinds) {
        catalog.commands.push_back(make_artist_workflow_command_row(kind));
    }
    return catalog;
}

EnvironmentArtistWorkflowCommandPlan
plan_environment_artist_workflow_command(const EnvironmentAuthoringDocument& document,
                                         const EnvironmentArtistWorkflowCommandRequest& request) {
    const auto row = make_artist_workflow_command_row(request.kind);
    EnvironmentArtistWorkflowCommandPlan plan{
        .command_id = row.command_id,
        .label = row.label,
        .mutates_document = row.mutates_document,
        .undo_supported = row.supports_undo_metadata,
        .rollback_metadata_available = row.supports_undo_metadata,
        .requires_confirmation = row.requires_confirmation,
        .before_revision = document.revision(),
        .after_revision = document.revision(),
    };

    if (request.request_backend_execution || request.request_package_script_execution ||
        request.request_native_handle_access) {
        plan.status = EnvironmentArtistWorkflowCommandStatus::rejected_unsafe_execution;
        add_artist_workflow_diagnostic(
            plan, "unsafe_execution",
            "environment artist workflow commands are editor-core plans only and cannot execute backend work, package "
            "scripts, or native handle access");
        add_artist_workflow_report_rows(plan);
        return plan;
    }

    if (request.mode == EnvironmentArtistWorkflowCommandMode::apply) {
        if (request.expected_revision != document.revision()) {
            plan.status = EnvironmentArtistWorkflowCommandStatus::rejected_stale_revision;
            add_artist_workflow_diagnostic(plan, "stale_revision",
                                           "environment artist workflow expected_revision does not match the document "
                                           "revision");
            add_artist_workflow_report_rows(plan);
            return plan;
        }
        if (row.requires_confirmation && !request.user_confirmed) {
            plan.status = EnvironmentArtistWorkflowCommandStatus::rejected_invalid_request;
            add_artist_workflow_diagnostic(plan, "confirmation_required",
                                           "environment artist workflow publish/package apply requires confirmation");
            add_artist_workflow_report_rows(plan);
            return plan;
        }
        plan.apply = true;
        plan.revision_checked = true;
        plan.after_revision = plan.mutates_document ? plan.before_revision + 1U : plan.before_revision;
    } else {
        plan.dry_run = true;
    }

    plan.status = EnvironmentArtistWorkflowCommandStatus::accepted;
    add_artist_workflow_report_rows(plan);
    return plan;
}

EnvironmentArtistWorkflowAssetBrowserModel
make_environment_artist_workflow_asset_browser_model(const EnvironmentArtistWorkflowAssetBrowserDesc& desc) {
    constexpr std::array kinds{
        EnvironmentArtistWorkflowAssetKind::preset_library,      EnvironmentArtistWorkflowAssetKind::openexr_source,
        EnvironmentArtistWorkflowAssetKind::ktx2_basis_source,   EnvironmentArtistWorkflowAssetKind::cooked_texture,
        EnvironmentArtistWorkflowAssetKind::environment_profile, EnvironmentArtistWorkflowAssetKind::simulation_preset,
        EnvironmentArtistWorkflowAssetKind::validation_report,   EnvironmentArtistWorkflowAssetKind::package_artifact,
    };

    EnvironmentArtistWorkflowAssetBrowserModel model;
    model.rows.reserve(kinds.size());

    if (desc.request_backend_execution || desc.request_package_script_execution || desc.request_native_handle_access) {
        add_asset_browser_diagnostic(
            model, "unsafe_execution",
            "environment artist workflow asset browser is an editor-core row model and cannot execute backend work, "
            "package scripts, or native handle access");
    }

    for (const auto kind : kinds) {
        auto row = make_asset_browser_row(find_asset_browser_input(desc.assets, kind), kind);
        if (row.status == EnvironmentArtistWorkflowAssetRowStatus::ready) {
            ++model.ready_rows;
        } else {
            add_asset_browser_diagnostic(model, "missing_asset_row",
                                         std::string{environment_artist_workflow_asset_kind_id(kind)} +
                                             " is missing a reviewed asset path");
        }
        model.rows.push_back(std::move(row));
    }

    if (model.diagnostics.empty()) {
        model.status = EnvironmentAuthoringStatus::ready;
    }
    return model;
}

mirakana::ui::UiDocument
make_environment_artist_workflow_asset_browser_ui_model(const EnvironmentArtistWorkflowAssetBrowserModel& model) {
    mirakana::ui::UiDocument document;
    auto root = make_element("environment_artist_workflow_asset_browser", mirakana::ui::SemanticRole::panel);
    root.accessibility_label = "Environment Artist Workflow Asset Browser";
    add_or_throw(document, std::move(root));

    const mirakana::ui::ElementId root_id{"environment_artist_workflow_asset_browser"};
    append_label(document, root_id, "environment_artist_workflow_asset_browser.status",
                 model.status == EnvironmentAuthoringStatus::ready ? "ready" : "blocked");
    append_label(document, root_id, "environment_artist_workflow_asset_browser.ready_rows",
                 std::to_string(model.ready_rows));
    append_label(document, root_id, "environment_artist_workflow_asset_browser.complete_artist_workflow_ready_claimed",
                 bool_text(model.complete_artist_workflow_ready_claimed));

    auto rows_root =
        make_child("environment_artist_workflow_asset_browser.rows", root_id, mirakana::ui::SemanticRole::list);
    rows_root.accessibility_label = "Environment Artist Workflow Asset Rows";
    add_or_throw(document, std::move(rows_root));
    const mirakana::ui::ElementId rows_id{"environment_artist_workflow_asset_browser.rows"};

    for (const auto& row : model.rows) {
        auto item = make_child("environment_artist_workflow_asset_browser.rows." + row.row_id, rows_id,
                               mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = false;
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{"environment_artist_workflow_asset_browser.rows." + row.row_id};
        append_label(document, item_id, "environment_artist_workflow_asset_browser.rows." + row.row_id + ".status",
                     std::string{asset_row_status_label(row.status)});
        append_label(document, item_id, "environment_artist_workflow_asset_browser.rows." + row.row_id + ".value",
                     row.path);
        append_label(document, item_id, "environment_artist_workflow_asset_browser.rows." + row.row_id + ".package",
                     bool_text(row.package_visible));
        append_label(document, item_id, "environment_artist_workflow_asset_browser.rows." + row.row_id + ".provenance",
                     bool_text(row.provenance_recorded));
        append_label(document, item_id, "environment_artist_workflow_asset_browser.rows." + row.row_id + ".budget",
                     bool_text(row.budget_recorded));
        append_label(document, item_id, "environment_artist_workflow_asset_browser.rows." + row.row_id + ".host_gate",
                     row.requires_host_gate ? row.host_gate : "none");
        append_label(document, item_id, "environment_artist_workflow_asset_browser.rows." + row.row_id + ".validation",
                     row.validation_recipe_id);
    }

    return document;
}

EnvironmentArtistWorkflowPreviewModel
make_environment_artist_workflow_preview_model(const EnvironmentArtistWorkflowPreviewDesc& desc) {
    constexpr std::array kinds{
        EnvironmentArtistWorkflowPreviewRowKind::selected_backend,
        EnvironmentArtistWorkflowPreviewRowKind::quality_tier,
        EnvironmentArtistWorkflowPreviewRowKind::missing_host_gate,
        EnvironmentArtistWorkflowPreviewRowKind::package_budget,
        EnvironmentArtistWorkflowPreviewRowKind::memory_budget,
        EnvironmentArtistWorkflowPreviewRowKind::diagnostics,
        EnvironmentArtistWorkflowPreviewRowKind::unsupported_claim_reason,
    };

    EnvironmentArtistWorkflowPreviewModel model;
    model.rows.reserve(kinds.size());

    if (desc.request_backend_execution || desc.request_package_script_execution || desc.request_native_handle_access) {
        add_preview_diagnostic(
            model, "unsafe_execution",
            "environment artist workflow preview is an editor-core row model and cannot execute backend work, package "
            "scripts, or native handle access");
    }

    const auto push_row = [&model](EnvironmentArtistWorkflowPreviewRow row, std::string code, std::string message) {
        if (row.status == EnvironmentArtistWorkflowPreviewRowStatus::ready) {
            ++model.ready_rows;
        } else {
            add_preview_diagnostic(model, std::move(code), std::move(message));
        }
        model.rows.push_back(std::move(row));
    };

    for (const auto kind : kinds) {
        switch (kind) {
        case EnvironmentArtistWorkflowPreviewRowKind::selected_backend:
            push_row(make_preview_row(kind, desc.selected_backend,
                                      desc.selected_backend.empty() ? EnvironmentArtistWorkflowPreviewRowStatus::blocked
                                                                    : EnvironmentArtistWorkflowPreviewRowStatus::ready),
                     "missing_selected_backend", "environment artist workflow preview selected backend is missing");
            break;
        case EnvironmentArtistWorkflowPreviewRowKind::quality_tier:
            push_row(make_preview_row(kind, desc.quality_tier,
                                      desc.quality_tier.empty() ? EnvironmentArtistWorkflowPreviewRowStatus::blocked
                                                                : EnvironmentArtistWorkflowPreviewRowStatus::ready),
                     "missing_quality_tier", "environment artist workflow preview quality tier is missing");
            break;
        case EnvironmentArtistWorkflowPreviewRowKind::missing_host_gate:
            push_row(make_preview_row(kind, desc.missing_host_gate.empty() ? "none" : desc.missing_host_gate,
                                      desc.missing_host_gate.empty()
                                          ? EnvironmentArtistWorkflowPreviewRowStatus::ready
                                          : EnvironmentArtistWorkflowPreviewRowStatus::blocked),
                     "missing_host_gate", "environment artist workflow preview is blocked by a missing host gate");
            break;
        case EnvironmentArtistWorkflowPreviewRowKind::package_budget:
            push_row(make_preview_row(kind, std::to_string(desc.package_budget_bytes),
                                      desc.package_budget_bytes == 0U
                                          ? EnvironmentArtistWorkflowPreviewRowStatus::blocked
                                          : EnvironmentArtistWorkflowPreviewRowStatus::ready),
                     "missing_package_budget", "environment artist workflow preview package budget is missing");
            break;
        case EnvironmentArtistWorkflowPreviewRowKind::memory_budget:
            push_row(make_preview_row(kind, std::to_string(desc.memory_budget_bytes),
                                      desc.memory_budget_bytes == 0U
                                          ? EnvironmentArtistWorkflowPreviewRowStatus::blocked
                                          : EnvironmentArtistWorkflowPreviewRowStatus::ready),
                     "missing_memory_budget", "environment artist workflow preview memory budget is missing");
            break;
        case EnvironmentArtistWorkflowPreviewRowKind::diagnostics:
            push_row(make_preview_row(kind, std::to_string(desc.diagnostics),
                                      desc.diagnostics == 0U ? EnvironmentArtistWorkflowPreviewRowStatus::ready
                                                             : EnvironmentArtistWorkflowPreviewRowStatus::blocked),
                     "preview_diagnostics", "environment artist workflow preview has diagnostics");
            break;
        case EnvironmentArtistWorkflowPreviewRowKind::unsupported_claim_reason:
            push_row(make_preview_row(kind, desc.unsupported_claim_reason,
                                      desc.unsupported_claim_reason.empty()
                                          ? EnvironmentArtistWorkflowPreviewRowStatus::blocked
                                          : EnvironmentArtistWorkflowPreviewRowStatus::ready),
                     "missing_unsupported_claim_reason",
                     "environment artist workflow preview must explain why complete readiness is still unsupported");
            break;
        }
    }

    if (model.diagnostics.empty()) {
        model.status = EnvironmentAuthoringStatus::ready;
    }
    return model;
}

mirakana::ui::UiDocument
make_environment_artist_workflow_preview_ui_model(const EnvironmentArtistWorkflowPreviewModel& model) {
    mirakana::ui::UiDocument document;
    auto root = make_element("environment_artist_workflow_preview", mirakana::ui::SemanticRole::panel);
    root.accessibility_label = "Environment Artist Workflow Preview";
    add_or_throw(document, std::move(root));

    const mirakana::ui::ElementId root_id{"environment_artist_workflow_preview"};
    append_label(document, root_id, "environment_artist_workflow_preview.status",
                 model.status == EnvironmentAuthoringStatus::ready ? "ready" : "blocked");
    append_label(document, root_id, "environment_artist_workflow_preview.ready_rows", std::to_string(model.ready_rows));
    append_label(document, root_id, "environment_artist_workflow_preview.complete_artist_workflow_ready_claimed",
                 bool_text(model.complete_artist_workflow_ready_claimed));

    auto rows_root = make_child("environment_artist_workflow_preview.rows", root_id, mirakana::ui::SemanticRole::list);
    rows_root.accessibility_label = "Environment Artist Workflow Preview Rows";
    add_or_throw(document, std::move(rows_root));
    const mirakana::ui::ElementId rows_id{"environment_artist_workflow_preview.rows"};

    for (const auto& row : model.rows) {
        auto item = make_child("environment_artist_workflow_preview.rows." + row.row_id, rows_id,
                               mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = false;
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{"environment_artist_workflow_preview.rows." + row.row_id};
        append_label(document, item_id, "environment_artist_workflow_preview.rows." + row.row_id + ".status",
                     std::string{preview_row_status_label(row.status)});
        append_label(document, item_id, "environment_artist_workflow_preview.rows." + row.row_id + ".value", row.value);
    }

    return document;
}

EnvironmentArtistWorkflowWalkthroughModel
make_environment_artist_workflow_walkthrough_model(const EnvironmentArtistWorkflowWalkthroughDesc& desc) {
    constexpr std::array kinds{
        EnvironmentArtistWorkflowWalkthroughStepKind::import_source_assets,
        EnvironmentArtistWorkflowWalkthroughStepKind::cook_assets,
        EnvironmentArtistWorkflowWalkthroughStepKind::assemble_preset,
        EnvironmentArtistWorkflowWalkthroughStepKind::edit_weather_timeline,
        EnvironmentArtistWorkflowWalkthroughStepKind::run_simulation_preview,
        EnvironmentArtistWorkflowWalkthroughStepKind::package_sample,
        EnvironmentArtistWorkflowWalkthroughStepKind::run_installed_validation,
        EnvironmentArtistWorkflowWalkthroughStepKind::inspect_report,
    };

    EnvironmentArtistWorkflowWalkthroughModel model;
    model.rows.reserve(kinds.size());

    if (desc.request_backend_execution || desc.request_package_script_execution || desc.request_native_handle_access) {
        add_walkthrough_diagnostic(
            model, "unsafe_execution",
            "environment artist workflow walkthrough is an editor-core row model and cannot execute backend work, "
            "package scripts, validation recipes, or native handle access");
    }

    for (const auto kind : kinds) {
        const auto* input = find_walkthrough_input(desc.steps, kind);
        auto row = make_walkthrough_step_row(input, kind);
        if (row.status == EnvironmentArtistWorkflowWalkthroughStepStatus::ready) {
            ++model.ready_rows;
        } else if (input == nullptr) {
            add_walkthrough_diagnostic(model, "missing_walkthrough_step",
                                       std::string{environment_artist_workflow_walkthrough_step_id(kind)} +
                                           " is missing reviewed walkthrough evidence");
        } else if (row.requires_host_gate) {
            add_walkthrough_diagnostic(model, "walkthrough_host_gate",
                                       std::string{environment_artist_workflow_walkthrough_step_id(kind)} +
                                           " requires host gate " +
                                           (row.host_gate.empty() ? "unspecified" : row.host_gate));
        } else {
            add_walkthrough_diagnostic(model, "incomplete_walkthrough_step",
                                       std::string{environment_artist_workflow_walkthrough_step_id(kind)} +
                                           " must be completed, reviewed, evidence-backed, and validation-linked");
        }
        model.rows.push_back(std::move(row));
    }

    if (model.diagnostics.empty()) {
        model.status = EnvironmentAuthoringStatus::ready;
    }
    return model;
}

mirakana::ui::UiDocument
make_environment_artist_workflow_walkthrough_ui_model(const EnvironmentArtistWorkflowWalkthroughModel& model) {
    mirakana::ui::UiDocument document;
    auto root = make_element("environment_artist_workflow_walkthrough", mirakana::ui::SemanticRole::panel);
    root.accessibility_label = "Environment Artist Workflow Walkthrough";
    add_or_throw(document, std::move(root));

    const mirakana::ui::ElementId root_id{"environment_artist_workflow_walkthrough"};
    append_label(document, root_id, "environment_artist_workflow_walkthrough.status",
                 model.status == EnvironmentAuthoringStatus::ready ? "ready" : "blocked");
    append_label(document, root_id, "environment_artist_workflow_walkthrough.ready_rows",
                 std::to_string(model.ready_rows));
    append_label(document, root_id, "environment_artist_workflow_walkthrough.complete_artist_workflow_ready_claimed",
                 bool_text(model.complete_artist_workflow_ready_claimed));

    auto rows_root =
        make_child("environment_artist_workflow_walkthrough.rows", root_id, mirakana::ui::SemanticRole::list);
    rows_root.accessibility_label = "Environment Artist Workflow Walkthrough Rows";
    add_or_throw(document, std::move(rows_root));
    const mirakana::ui::ElementId rows_id{"environment_artist_workflow_walkthrough.rows"};

    for (const auto& row : model.rows) {
        auto item = make_child("environment_artist_workflow_walkthrough.rows." + row.row_id, rows_id,
                               mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = false;
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{"environment_artist_workflow_walkthrough.rows." + row.row_id};
        append_label(document, item_id, "environment_artist_workflow_walkthrough.rows." + row.row_id + ".status",
                     std::string{walkthrough_step_status_label(row.status)});
        append_label(document, item_id, "environment_artist_workflow_walkthrough.rows." + row.row_id + ".value",
                     row.evidence_id);
        append_label(document, item_id, "environment_artist_workflow_walkthrough.rows." + row.row_id + ".command",
                     row.command_id);
        append_label(document, item_id, "environment_artist_workflow_walkthrough.rows." + row.row_id + ".reviewed",
                     bool_text(row.reviewed));
        append_label(document, item_id, "environment_artist_workflow_walkthrough.rows." + row.row_id + ".package",
                     bool_text(row.package_visible));
        append_label(document, item_id, "environment_artist_workflow_walkthrough.rows." + row.row_id + ".host_gate",
                     row.requires_host_gate ? row.host_gate : "none");
        append_label(document, item_id, "environment_artist_workflow_walkthrough.rows." + row.row_id + ".validation",
                     row.validation_recipe_id);
    }

    return document;
}

EnvironmentArtistWorkflowExecutionReviewModel
make_environment_artist_workflow_execution_review_model(const EnvironmentArtistWorkflowExecutionReviewDesc& desc) {
    EnvironmentArtistWorkflowExecutionReviewModel model;
    model.external_action_required = false;
    model.evidence_review_required = false;

    append_execution_unsupported(model, "environment_artist_workflow_ready",
                                 "environment_artist_workflow_ready remains unsupported after visible workflow wiring");

    const auto reject_request = [&model](bool requested, std::string_view claim, std::string_view diagnostic) {
        if (requested) {
            append_execution_unsupported(model, claim, std::string{diagnostic});
            model.has_blocking_diagnostics = true;
        }
    };
    reject_request(desc.request_backend_execution, "backend execution",
                   "environment artist workflow execution review rejects backend execution from editor core");
    reject_request(desc.request_package_script_execution, "package script execution",
                   "environment artist workflow execution review rejects package script execution from editor core");
    reject_request(desc.request_validation_recipe_execution, "validation recipe execution",
                   "environment artist workflow execution review rejects validation recipe execution from editor core");
    reject_request(desc.request_native_handle_access, "native handle access",
                   "environment artist workflow execution review rejects native handle access");
    reject_request(desc.request_complete_artist_workflow_ready_promotion, "environment_artist_workflow_ready",
                   "environment artist workflow ready promotion requires a later complete closeout");

    std::vector<std::string> command_ids;
    command_ids.reserve(desc.command_catalog.commands.size());
    for (const auto& row : desc.command_catalog.commands) {
        append_unique_string(command_ids, row.command_id);
    }
    push_execution_stage(model, EnvironmentArtistWorkflowExecutionReviewStageRow{
                                    .row_id = "environment.workflow.execution.command_catalog",
                                    .label = std::string{artist_workflow_execution_stage_label(
                                        "environment.workflow.execution.command_catalog")},
                                    .source_model = "EnvironmentArtistWorkflowCommandCatalog",
                                    .status = command_ids.size() == 11U
                                                  ? EnvironmentArtistWorkflowExecutionStageStatus::ready
                                                  : EnvironmentArtistWorkflowExecutionStageStatus::blocked,
                                    .source_row_count = command_ids.size(),
                                    .source_row_ids = command_ids,
                                    .diagnostic = command_ids.size() == 11U ? "reviewed command catalog is visible"
                                                                            : "reviewed command catalog is incomplete",
                                });

    std::vector<std::string> asset_ids;
    std::vector<std::string> asset_host_gates;
    asset_ids.reserve(desc.asset_browser.rows.size());
    for (const auto& row : desc.asset_browser.rows) {
        append_unique_string(asset_ids, row.row_id);
        if (row.requires_host_gate) {
            append_unique_string(asset_host_gates, row.host_gate.empty() ? std::string_view{"unspecified-host-gate"}
                                                                         : std::string_view{row.host_gate});
        }
    }
    const auto asset_status =
        source_model_stage_status(desc.asset_browser.status, !asset_ids.empty(), !asset_host_gates.empty());
    push_execution_stage(
        model,
        EnvironmentArtistWorkflowExecutionReviewStageRow{
            .row_id = "environment.workflow.execution.asset_browser",
            .label = std::string{artist_workflow_execution_stage_label("environment.workflow.execution.asset_browser")},
            .source_model = "EnvironmentArtistWorkflowAssetBrowserModel",
            .status = asset_status,
            .source_row_count = asset_ids.size(),
            .source_row_ids = asset_ids,
            .host_gates = asset_host_gates,
            .diagnostic = asset_status == EnvironmentArtistWorkflowExecutionStageStatus::ready
                              ? "asset browser rows are visible"
                              : "asset browser rows are blocked or host-gated",
        });

    std::vector<std::string> preview_ids;
    std::vector<std::string> preview_host_gates;
    preview_ids.reserve(desc.preview.rows.size());
    for (const auto& row : desc.preview.rows) {
        append_unique_string(preview_ids, row.row_id);
        if (row.kind == EnvironmentArtistWorkflowPreviewRowKind::missing_host_gate &&
            row.status == EnvironmentArtistWorkflowPreviewRowStatus::blocked) {
            append_unique_string(preview_host_gates, row.value.empty() ? std::string_view{"unspecified-host-gate"}
                                                                       : std::string_view{row.value});
        }
    }
    const auto preview_status =
        source_model_stage_status(desc.preview.status, !preview_ids.empty(), !preview_host_gates.empty());
    push_execution_stage(
        model,
        EnvironmentArtistWorkflowExecutionReviewStageRow{
            .row_id = "environment.workflow.execution.preview",
            .label = std::string{artist_workflow_execution_stage_label("environment.workflow.execution.preview")},
            .source_model = "EnvironmentArtistWorkflowPreviewModel",
            .status = preview_status,
            .source_row_count = preview_ids.size(),
            .source_row_ids = preview_ids,
            .host_gates = preview_host_gates,
            .diagnostic = preview_status == EnvironmentArtistWorkflowExecutionStageStatus::ready
                              ? "preview rows are visible"
                              : "preview rows are blocked or host-gated",
        });

    std::vector<std::string> walkthrough_ids;
    std::vector<std::string> walkthrough_host_gates;
    std::vector<std::string> required_recipe_ids;
    walkthrough_ids.reserve(desc.walkthrough.rows.size());
    for (const auto& row : desc.walkthrough.rows) {
        append_unique_string(walkthrough_ids, row.row_id);
        append_unique_string(required_recipe_ids, row.validation_recipe_id);
        if (row.requires_host_gate) {
            append_unique_string(walkthrough_host_gates, row.host_gate.empty()
                                                             ? std::string_view{"unspecified-host-gate"}
                                                             : std::string_view{row.host_gate});
        }
    }
    const auto walkthrough_status =
        source_model_stage_status(desc.walkthrough.status, !walkthrough_ids.empty(), !walkthrough_host_gates.empty());
    push_execution_stage(
        model,
        EnvironmentArtistWorkflowExecutionReviewStageRow{
            .row_id = "environment.workflow.execution.walkthrough",
            .label = std::string{artist_workflow_execution_stage_label("environment.workflow.execution.walkthrough")},
            .source_model = "EnvironmentArtistWorkflowWalkthroughModel",
            .status = walkthrough_status,
            .source_row_count = walkthrough_ids.size(),
            .source_row_ids = walkthrough_ids,
            .external_recipe_ids = required_recipe_ids,
            .host_gates = walkthrough_host_gates,
            .diagnostic = walkthrough_status == EnvironmentArtistWorkflowExecutionStageStatus::ready
                              ? "walkthrough rows are visible"
                              : "walkthrough rows are blocked or host-gated",
        });

    std::vector<std::string> external_blockers;
    std::vector<std::string> external_host_gates;
    bool missing_evidence = required_recipe_ids.empty();
    bool invalid_evidence = false;
    bool failed_evidence = false;
    bool all_evidence_passed = !required_recipe_ids.empty();
    for (const auto& recipe_id : required_recipe_ids) {
        const auto* evidence = find_execution_evidence(desc.evidence_rows, recipe_id);
        if (evidence == nullptr) {
            missing_evidence = true;
            all_evidence_passed = false;
            append_unique_string(external_blockers, "missing-evidence:" + recipe_id);
            continue;
        }
        append_unique_strings(external_host_gates, evidence->host_gates);
        append_unique_strings(external_blockers, evidence->blocked_by);
        if (!evidence->externally_supplied || evidence->claims_editor_core_execution) {
            invalid_evidence = true;
            all_evidence_passed = false;
            append_unique_string(external_blockers, "invalid-editor-core-evidence:" + recipe_id);
        } else if (!evidence->passed) {
            failed_evidence = true;
            all_evidence_passed = false;
            append_unique_string(external_blockers, "nonpassing-evidence:" + recipe_id);
        }
    }
    for (const auto& evidence : desc.evidence_rows) {
        if (!required_recipe_ids.empty() && contains_string(required_recipe_ids, evidence.recipe_id)) {
            continue;
        }
        invalid_evidence = true;
        append_unique_string(external_blockers, evidence.recipe_id.empty()
                                                    ? std::string_view{"unexpected-empty-evidence-recipe"}
                                                    : std::string_view{evidence.recipe_id});
    }

    auto external_status = EnvironmentArtistWorkflowExecutionStageStatus::ready;
    if (invalid_evidence || failed_evidence) {
        external_status = EnvironmentArtistWorkflowExecutionStageStatus::blocked;
    } else if (!external_host_gates.empty()) {
        external_status = EnvironmentArtistWorkflowExecutionStageStatus::host_gated;
    } else if (missing_evidence) {
        external_status = EnvironmentArtistWorkflowExecutionStageStatus::awaiting_external_evidence;
    }
    push_execution_stage(model,
                         EnvironmentArtistWorkflowExecutionReviewStageRow{
                             .row_id = "environment.workflow.execution.external_execution",
                             .label = std::string{artist_workflow_execution_stage_label(
                                 "environment.workflow.execution.external_execution")},
                             .source_model = "EnvironmentArtistWorkflowWalkthroughModel",
                             .status = external_status,
                             .source_row_count = required_recipe_ids.size(),
                             .source_row_ids = required_recipe_ids,
                             .external_recipe_ids = required_recipe_ids,
                             .host_gates = external_host_gates,
                             .blocked_by = external_blockers,
                             .diagnostic = external_status == EnvironmentArtistWorkflowExecutionStageStatus::ready
                                               ? "external operator execution evidence is available"
                                               : "external operator execution requires review",
                             .external_execution_required = true,
                             .evidence_passed = all_evidence_passed,
                         });

    std::vector<std::string> evidence_ids;
    std::vector<std::string> evidence_host_gates;
    std::vector<std::string> evidence_blockers;
    evidence_ids.reserve(desc.evidence_rows.size());
    for (const auto& evidence : desc.evidence_rows) {
        append_unique_string(evidence_ids, evidence.recipe_id);
        append_unique_strings(evidence_host_gates, evidence.host_gates);
        append_unique_strings(evidence_blockers, evidence.blocked_by);
        if (!evidence.externally_supplied) {
            invalid_evidence = true;
            append_unique_string(evidence_blockers, "evidence-not-externally-supplied:" + evidence.recipe_id);
        }
        if (evidence.claims_editor_core_execution) {
            invalid_evidence = true;
            append_unique_string(evidence_blockers, "editor-core-execution-claim:" + evidence.recipe_id);
        }
    }
    auto evidence_status = EnvironmentArtistWorkflowExecutionStageStatus::ready;
    if (invalid_evidence || failed_evidence || required_recipe_ids.empty()) {
        evidence_status = EnvironmentArtistWorkflowExecutionStageStatus::blocked;
    } else if (!evidence_host_gates.empty()) {
        evidence_status = EnvironmentArtistWorkflowExecutionStageStatus::host_gated;
    } else if (missing_evidence) {
        evidence_status = EnvironmentArtistWorkflowExecutionStageStatus::awaiting_external_evidence;
    }
    push_execution_stage(model,
                         EnvironmentArtistWorkflowExecutionReviewStageRow{
                             .row_id = "environment.workflow.execution.evidence_review",
                             .label = std::string{artist_workflow_execution_stage_label(
                                 "environment.workflow.execution.evidence_review")},
                             .source_model = "EnvironmentArtistWorkflowExecutionEvidenceRow",
                             .status = evidence_status,
                             .source_row_count = evidence_ids.size(),
                             .source_row_ids = evidence_ids,
                             .external_recipe_ids = required_recipe_ids,
                             .host_gates = evidence_host_gates,
                             .blocked_by = evidence_blockers,
                             .diagnostic = evidence_status == EnvironmentArtistWorkflowExecutionStageStatus::ready
                                               ? "externally supplied evidence passed review"
                                               : "externally supplied evidence is missing or blocked",
                             .evidence_passed = evidence_status == EnvironmentArtistWorkflowExecutionStageStatus::ready,
                         });

    const auto operator_status = desc.operator_reviewed && !desc.operator_review_id.empty()
                                     ? EnvironmentArtistWorkflowExecutionStageStatus::ready
                                     : EnvironmentArtistWorkflowExecutionStageStatus::awaiting_operator_review;
    push_execution_stage(
        model,
        EnvironmentArtistWorkflowExecutionReviewStageRow{
            .row_id = "environment.workflow.execution.operator_review",
            .label =
                std::string{artist_workflow_execution_stage_label("environment.workflow.execution.operator_review")},
            .source_model = "EnvironmentArtistWorkflowExecutionReviewDesc",
            .status = operator_status,
            .source_row_count = desc.operator_review_id.empty() ? 0U : 1U,
            .source_row_ids = desc.operator_review_id.empty() ? std::vector<std::string>{}
                                                              : std::vector<std::string>{desc.operator_review_id},
            .diagnostic = operator_status == EnvironmentArtistWorkflowExecutionStageStatus::ready
                              ? "operator reviewed the visible workflow"
                              : "operator review is required",
            .operator_reviewed = operator_status == EnvironmentArtistWorkflowExecutionStageStatus::ready,
        });

    const auto guard_status = desc.request_complete_artist_workflow_ready_promotion
                                  ? EnvironmentArtistWorkflowExecutionStageStatus::blocked
                                  : EnvironmentArtistWorkflowExecutionStageStatus::ready;
    push_execution_stage(model, EnvironmentArtistWorkflowExecutionReviewStageRow{
                                    .row_id = "environment.workflow.execution.ready_promotion_guard",
                                    .label = std::string{artist_workflow_execution_stage_label(
                                        "environment.workflow.execution.ready_promotion_guard")},
                                    .source_model = "EnvironmentCommercialExcellencePhase11",
                                    .status = guard_status,
                                    .source_row_count = 1U,
                                    .source_row_ids = {"environment_artist_workflow_ready"},
                                    .blocked_by = guard_status == EnvironmentArtistWorkflowExecutionStageStatus::ready
                                                      ? std::vector<std::string>{}
                                                      : std::vector<std::string>{"complete-ready-promotion-requested"},
                                    .diagnostic = guard_status == EnvironmentArtistWorkflowExecutionStageStatus::ready
                                                      ? "complete artist workflow readiness remains unsupported"
                                                      : "complete artist workflow readiness promotion is blocked",
                                    .ready_promotion_guard = true,
                                });

    for (const auto& stage : model.stage_rows) {
        if (stage.status != EnvironmentArtistWorkflowExecutionStageStatus::ready) {
            model.external_action_required = true;
        }
        if (stage.row_id == "environment.workflow.execution.evidence_review" &&
            stage.status != EnvironmentArtistWorkflowExecutionStageStatus::ready) {
            model.evidence_review_required = true;
        }
    }

    model.visible_first_party_workflow_wired =
        !model.has_blocking_diagnostics &&
        std::ranges::all_of(model.stage_rows, [](const EnvironmentArtistWorkflowExecutionReviewStageRow& row) {
            return row.status == EnvironmentArtistWorkflowExecutionStageStatus::ready;
        });
    if (model.visible_first_party_workflow_wired) {
        model.status = EnvironmentAuthoringStatus::ready;
    }
    return model;
}

mirakana::ui::UiDocument
make_environment_artist_workflow_execution_review_ui_model(const EnvironmentArtistWorkflowExecutionReviewModel& model) {
    mirakana::ui::UiDocument document;
    auto root = make_element("environment_artist_workflow_execution_review", mirakana::ui::SemanticRole::panel);
    root.accessibility_label = "Environment Artist Workflow Execution Review";
    add_or_throw(document, std::move(root));

    const mirakana::ui::ElementId root_id{"environment_artist_workflow_execution_review"};
    append_label(document, root_id, "environment_artist_workflow_execution_review.status",
                 model.status == EnvironmentAuthoringStatus::ready ? "ready" : "blocked");
    append_label(document, root_id, "environment_artist_workflow_execution_review.visible_first_party_workflow_wired",
                 bool_text(model.visible_first_party_workflow_wired));
    append_label(document, root_id, "environment_artist_workflow_execution_review.external_action_required",
                 bool_text(model.external_action_required));
    append_label(document, root_id, "environment_artist_workflow_execution_review.evidence_review_required",
                 bool_text(model.evidence_review_required));
    append_label(document, root_id,
                 "environment_artist_workflow_execution_review.complete_artist_workflow_ready_claimed",
                 bool_text(model.complete_artist_workflow_ready_claimed));

    auto rows_root =
        make_child("environment_artist_workflow_execution_review.rows", root_id, mirakana::ui::SemanticRole::list);
    rows_root.accessibility_label = "Environment Artist Workflow Execution Review Rows";
    add_or_throw(document, std::move(rows_root));
    const mirakana::ui::ElementId rows_id{"environment_artist_workflow_execution_review.rows"};

    for (const auto& row : model.stage_rows) {
        auto item = make_child("environment_artist_workflow_execution_review.rows." + row.row_id, rows_id,
                               mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = false;
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{"environment_artist_workflow_execution_review.rows." + row.row_id};
        append_label(document, item_id, "environment_artist_workflow_execution_review.rows." + row.row_id + ".status",
                     std::string{environment_artist_workflow_execution_stage_status_label(row.status)});
        append_label(document, item_id, "environment_artist_workflow_execution_review.rows." + row.row_id + ".value",
                     row.diagnostic);
        append_label(document, item_id,
                     "environment_artist_workflow_execution_review.rows." + row.row_id + ".source_model",
                     row.source_model);
        append_label(document, item_id,
                     "environment_artist_workflow_execution_review.rows." + row.row_id + ".source_row_count",
                     std::to_string(row.source_row_count));
        append_label(document, item_id,
                     "environment_artist_workflow_execution_review.rows." + row.row_id + ".external_recipes",
                     join_strings(row.external_recipe_ids));
        append_label(document, item_id,
                     "environment_artist_workflow_execution_review.rows." + row.row_id + ".host_gates",
                     join_strings(row.host_gates));
        append_label(document, item_id,
                     "environment_artist_workflow_execution_review.rows." + row.row_id + ".blocked_by",
                     join_strings(row.blocked_by));
    }

    return document;
}

EnvironmentArtistWorkflowReadyReviewModel
make_environment_artist_workflow_ready_review_model(const EnvironmentArtistWorkflowReadyReviewDesc& desc) {
    constexpr std::array kinds{
        EnvironmentArtistWorkflowReadyRequirementKind::visible_editor_shell,
        EnvironmentArtistWorkflowReadyRequirementKind::asset_pipeline,
        EnvironmentArtistWorkflowReadyRequirementKind::selected_preset_library,
        EnvironmentArtistWorkflowReadyRequirementKind::validation_remediation,
        EnvironmentArtistWorkflowReadyRequirementKind::revision_safety,
        EnvironmentArtistWorkflowReadyRequirementKind::production_walkthrough_package,
        EnvironmentArtistWorkflowReadyRequirementKind::editor_core_execution_boundary,
        EnvironmentArtistWorkflowReadyRequirementKind::operator_review,
    };

    const auto expected_evidence_id = [](EnvironmentArtistWorkflowReadyRequirementKind kind) -> std::string_view {
        switch (kind) {
        case EnvironmentArtistWorkflowReadyRequirementKind::visible_editor_shell:
            return "environment-artist-workflow-visible-shell";
        case EnvironmentArtistWorkflowReadyRequirementKind::asset_pipeline:
            return "environment_asset_pipeline_openexr_ktx_basis_ready";
        case EnvironmentArtistWorkflowReadyRequirementKind::selected_preset_library:
            return "environment_aaa_preset_library_ready";
        case EnvironmentArtistWorkflowReadyRequirementKind::validation_remediation:
            return "environment.command.validation.remediation";
        case EnvironmentArtistWorkflowReadyRequirementKind::revision_safety:
            return "environment.workflow.revision_checked";
        case EnvironmentArtistWorkflowReadyRequirementKind::production_walkthrough_package:
            return "environment_artist_workflow_walkthrough_package";
        case EnvironmentArtistWorkflowReadyRequirementKind::editor_core_execution_boundary:
            return "environment-artist-workflow-editor-core-no-execution";
        case EnvironmentArtistWorkflowReadyRequirementKind::operator_review:
            return "environment-artist-workflow-operator-review";
        }
        return "";
    };

    const auto execution_stage_ready = [&desc](std::string_view row_id) {
        const auto it = std::ranges::find_if(
            desc.execution_review.stage_rows,
            [row_id](const EnvironmentArtistWorkflowExecutionReviewStageRow& row) { return row.row_id == row_id; });
        return it != desc.execution_review.stage_rows.end() &&
               it->status == EnvironmentArtistWorkflowExecutionStageStatus::ready;
    };

    EnvironmentArtistWorkflowReadyReviewModel model;
    model.rows.reserve(kinds.size());

    for (const auto& claim : desc.execution_review.unsupported_claims) {
        if (claim != "environment_artist_workflow_ready") {
            append_ready_unsupported(model, claim, "execution review carries unsupported claim " + claim);
        }
    }

    const auto reject_request = [&model](bool requested, std::string_view claim, std::string_view diagnostic) {
        if (requested) {
            append_ready_unsupported(model, claim, std::string{diagnostic});
        }
    };
    reject_request(desc.request_backend_execution, "backend execution",
                   "environment artist workflow ready review rejects backend execution from editor core");
    reject_request(desc.request_package_script_execution, "package script execution",
                   "environment artist workflow ready review rejects package script execution from editor core");
    reject_request(desc.request_validation_recipe_execution, "validation recipe execution",
                   "environment artist workflow ready review rejects validation recipe execution from editor core");
    reject_request(desc.request_native_handle_access, "native handle access",
                   "environment artist workflow ready review rejects native handle access");

    const bool unsafe_request =
        desc.request_backend_execution || desc.request_package_script_execution ||
        desc.request_validation_recipe_execution || desc.request_native_handle_access ||
        desc.execution_review.invokes_backend || desc.execution_review.executes_package_scripts ||
        desc.execution_review.executes_validation_recipes || desc.execution_review.exposes_native_handles ||
        desc.execution_review.has_blocking_diagnostics;

    for (const auto kind : kinds) {
        EnvironmentArtistWorkflowReadyRequirementRow row{
            .row_id = std::string{environment_artist_workflow_ready_requirement_id(kind)},
            .label = std::string{artist_workflow_ready_requirement_label(kind)},
            .kind = kind,
            .diagnostic = "missing package-visible reviewed evidence",
        };

        const auto* input = find_ready_requirement_input(desc.requirements, kind);
        if (input != nullptr) {
            row.evidence_id = input->evidence_id;
            row.reviewed = input->reviewed;
            row.package_visible = input->package_visible;
            row.retained_ui_row_ids = input->retained_ui_row_ids;
        }

        const bool evidence_id_matches = input != nullptr && input->evidence_id == expected_evidence_id(kind);
        bool base_ready = input != nullptr && input->ready && input->reviewed && input->package_visible &&
                          evidence_id_matches && !input->retained_ui_row_ids.empty();
        bool derived_ready = true;

        switch (kind) {
        case EnvironmentArtistWorkflowReadyRequirementKind::visible_editor_shell:
            derived_ready = desc.execution_review.status == EnvironmentAuthoringStatus::ready &&
                            desc.execution_review.visible_first_party_workflow_wired;
            break;
        case EnvironmentArtistWorkflowReadyRequirementKind::asset_pipeline:
        case EnvironmentArtistWorkflowReadyRequirementKind::selected_preset_library:
        case EnvironmentArtistWorkflowReadyRequirementKind::validation_remediation:
        case EnvironmentArtistWorkflowReadyRequirementKind::revision_safety:
            break;
        case EnvironmentArtistWorkflowReadyRequirementKind::production_walkthrough_package:
            derived_ready = input != nullptr && input->retained_ui_row_ids.size() >= 8U;
            break;
        case EnvironmentArtistWorkflowReadyRequirementKind::editor_core_execution_boundary:
            derived_ready = !unsafe_request;
            break;
        case EnvironmentArtistWorkflowReadyRequirementKind::operator_review:
            derived_ready = execution_stage_ready("environment.workflow.execution.operator_review");
            break;
        }

        if (base_ready && derived_ready) {
            row.status = EnvironmentArtistWorkflowReadyRequirementStatus::ready;
            row.diagnostic = "package-visible reviewed evidence is ready";
            ++model.ready_rows;
        } else if (input == nullptr || !base_ready) {
            row.status = EnvironmentArtistWorkflowReadyRequirementStatus::missing;
            if (input == nullptr) {
                append_unique_string(row.blocked_by, "missing-requirement-row");
            }
            if (input != nullptr && !input->ready) {
                append_unique_string(row.blocked_by, "not-ready");
            }
            if (input != nullptr && !input->reviewed) {
                append_unique_string(row.blocked_by, "not-reviewed");
            }
            if (input != nullptr && !input->package_visible) {
                append_unique_string(row.blocked_by, "not-package-visible");
            }
            if (input != nullptr && !evidence_id_matches) {
                append_unique_string(row.blocked_by, "unexpected-evidence-id");
            }
            if (input != nullptr && input->retained_ui_row_ids.empty()) {
                append_unique_string(row.blocked_by, "missing-retained-ui-row");
            }
        } else {
            row.status = EnvironmentArtistWorkflowReadyRequirementStatus::blocked;
            row.diagnostic = "derived editor workflow boundary is blocked";
            if (kind == EnvironmentArtistWorkflowReadyRequirementKind::production_walkthrough_package) {
                append_unique_string(row.blocked_by, "incomplete-walkthrough-ui-evidence");
            }
            if (kind == EnvironmentArtistWorkflowReadyRequirementKind::editor_core_execution_boundary) {
                append_unique_string(row.blocked_by, "editor-core-execution-requested");
            }
            if (kind == EnvironmentArtistWorkflowReadyRequirementKind::visible_editor_shell) {
                append_unique_string(row.blocked_by, "visible-workflow-not-ready");
            }
            if (kind == EnvironmentArtistWorkflowReadyRequirementKind::operator_review) {
                append_unique_string(row.blocked_by, "operator-review-not-ready");
            }
        }

        if (row.status != EnvironmentArtistWorkflowReadyRequirementStatus::ready) {
            model.diagnostics.push_back(row.row_id + ": " + row.diagnostic);
        }
        model.rows.push_back(std::move(row));
    }

    const bool all_requirements_ready = model.rows.size() == kinds.size() && model.ready_rows == model.rows.size();
    if (!all_requirements_ready || !desc.request_environment_artist_workflow_ready ||
        !model.unsupported_claims.empty()) {
        append_ready_unsupported(model, "environment_artist_workflow_ready",
                                 all_requirements_ready
                                     ? "environment_artist_workflow_ready request is missing or blocked"
                                     : "environment_artist_workflow_ready requires all package-visible reviewed rows");
    }

    if (desc.request_environment_artist_workflow_ready && all_requirements_ready && model.unsupported_claims.empty()) {
        model.environment_artist_workflow_ready = true;
        model.status = EnvironmentAuthoringStatus::ready;
    }

    return model;
}

mirakana::ui::UiDocument
make_environment_artist_workflow_ready_review_ui_model(const EnvironmentArtistWorkflowReadyReviewModel& model) {
    mirakana::ui::UiDocument document;
    auto root = make_element("environment_artist_workflow_ready_review", mirakana::ui::SemanticRole::panel);
    root.accessibility_label = "Environment Artist Workflow Ready Review";
    add_or_throw(document, std::move(root));

    const mirakana::ui::ElementId root_id{"environment_artist_workflow_ready_review"};
    append_label(document, root_id, "environment_artist_workflow_ready_review.status",
                 model.status == EnvironmentAuthoringStatus::ready ? "ready" : "blocked");
    append_label(document, root_id, "environment_artist_workflow_ready_review.environment_artist_workflow_ready",
                 bool_text(model.environment_artist_workflow_ready));
    append_label(document, root_id, "environment_artist_workflow_ready_review.package_counter",
                 model.package_counter_id);
    append_label(document, root_id, "environment_artist_workflow_ready_review.ready_rows",
                 std::to_string(model.ready_rows));

    auto rows_root =
        make_child("environment_artist_workflow_ready_review.rows", root_id, mirakana::ui::SemanticRole::list);
    rows_root.accessibility_label = "Environment Artist Workflow Ready Review Rows";
    add_or_throw(document, std::move(rows_root));
    const mirakana::ui::ElementId rows_id{"environment_artist_workflow_ready_review.rows"};

    for (const auto& row : model.rows) {
        auto item = make_child("environment_artist_workflow_ready_review.rows." + row.row_id, rows_id,
                               mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = false;
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{"environment_artist_workflow_ready_review.rows." + row.row_id};
        append_label(document, item_id, "environment_artist_workflow_ready_review.rows." + row.row_id + ".status",
                     std::string{environment_artist_workflow_ready_requirement_status_label(row.status)});
        append_label(document, item_id, "environment_artist_workflow_ready_review.rows." + row.row_id + ".value",
                     row.evidence_id);
        append_label(document, item_id, "environment_artist_workflow_ready_review.rows." + row.row_id + ".reviewed",
                     bool_text(row.reviewed));
        append_label(document, item_id, "environment_artist_workflow_ready_review.rows." + row.row_id + ".package",
                     bool_text(row.package_visible));
        append_label(document, item_id,
                     "environment_artist_workflow_ready_review.rows." + row.row_id + ".retained_ui_rows",
                     join_strings(row.retained_ui_row_ids));
        append_label(document, item_id, "environment_artist_workflow_ready_review.rows." + row.row_id + ".blocked_by",
                     join_strings(row.blocked_by));
    }

    return document;
}

EnvironmentAuthoringValidationModel
make_environment_authoring_validation_model(const EnvironmentAuthoringDocument& document) {
    EnvironmentAuthoringValidationModel model{.status = EnvironmentAuthoringStatus::ready};
    add_diagnostics(model, validate_environment_profile_v2(document.profile_document_v2()));
    if (!model.succeeded()) {
        model.status = EnvironmentAuthoringStatus::blocked;
    }
    return model;
}

EnvironmentAuthoringInspectorModel
make_environment_authoring_inspector_model(const EnvironmentAuthoringInspectorDesc& desc) {
    EnvironmentAuthoringInspectorModel model;
    model.profile_id = desc.document.profile().id;
    model.path = std::string{desc.document.path()};
    model.dirty = desc.document.dirty();
    model.revision = desc.document.revision();
    model.saved_revision = desc.document.saved_revision();

    const auto validation = make_environment_authoring_validation_model(desc.document);
    model.status = validation.status;
    model.diagnostics = validation.diagnostics;

    const auto& profile = desc.document.profile();
    add_row(model, "environment.sky.model", "Sky", "Sky Model",
            std::string{environment_sky_model_name(profile.sky_model)});
    add_row(model, "environment.sun.direction", "Sun", "Sun Direction", format_vec3(profile.sun.direction));
    add_row(model, "environment.sun.illuminance_lux", "Sun", "Sun Illuminance",
            format_float(profile.sun.illuminance_lux));
    add_row(model, "environment.moon.direction", "Moon", "Moon Direction", format_vec3(profile.moon.direction));
    add_row(model, "environment.moon.illuminance_lux", "Moon", "Moon Illuminance",
            format_float(profile.moon.illuminance_lux));
    add_row(model, "environment.atmosphere.planet_radius_km", "Atmosphere", "Planet Radius",
            format_float(profile.atmosphere.planet_radius_km));
    add_row(model, "environment.atmosphere.height_km", "Atmosphere", "Atmosphere Height",
            format_float(profile.atmosphere.atmosphere_height_km));
    add_row(model, "environment.fog.enabled", "Fog", "Fog Enabled", bool_text(profile.fog.enabled));
    add_row(model, "environment.fog.density", "Fog", "Fog Density", format_float(profile.fog.density));
    add_row(model, "environment.cloud_layer.mode", "Cloud Layer", "Cloud Layer Mode",
            desc.cloud_layer.mode == EnvironmentCloudLayerMode::equirectangular_2d ? "equirectangular_2d"
                                                                                   : "unsupported");
    add_row(model, "environment.cloud_layer.coverage", "Cloud Layer", "Cloud Coverage",
            format_float(desc.cloud_layer.coverage));
    add_row(model, "environment.cloud_layer.wind_velocity_mps", "Cloud Layer", "Cloud Wind",
            format_vec2(desc.cloud_layer.wind_velocity_mps));
    add_row(model, "environment.volumetric_clouds.status", "Volumetric Clouds", "Volumetric Clouds",
            desc.volumetric_clouds_policy_available ? "policy_available" : "policy_unavailable", false);
    add_row(model, "environment.precipitation.kind", "Precipitation", "Precipitation",
            std::string{environment_precipitation_kind_name(profile.precipitation.kind)});
    add_row(model, "environment.precipitation.intensity", "Precipitation", "Precipitation Intensity",
            format_float(profile.precipitation.intensity));
    add_row(model, "environment.weather.preset", "Weather", "Weather Preset",
            std::string{environment_weather_kind_name(profile.weather)});
    add_row(model, "environment.quality.tier", "Quality", "Quality Tier",
            std::string{environment_quality_preset_name(desc.document.profile_document_v2().quality_preset)});

    const auto& profile_v2 = desc.document.profile_document_v2();
    add_row(model, "environment.profile_v2.volume_count", "Profile V2", "Volume Count",
            format_uint(profile_v2.volumes.size()), false);
    add_row(model, "environment.profile_v2.weather_keyframes", "Profile V2", "Weather Keyframes",
            format_uint(profile_v2.weather_timeline.size()), false);
    for (std::size_t index = 0U; index < profile_v2.volumes.size(); ++index) {
        const auto& volume = profile_v2.volumes[index];
        const auto prefix = std::string{"environment.volume."} + std::to_string(index) + ".";
        add_row(model, prefix + "id", "Volumes", "Volume " + std::to_string(index) + " Id", volume.id);
        add_row(model, prefix + "shape", "Volumes", "Volume " + std::to_string(index) + " Shape",
                std::string{environment_volume_shape_name(volume.shape)});
        add_row(model, prefix + "priority", "Volumes", "Volume " + std::to_string(index) + " Priority",
                std::to_string(volume.priority));
        add_row(model, prefix + "blend_weight", "Volumes", "Volume " + std::to_string(index) + " Blend Weight",
                format_float(volume.blend_weight));
        add_row(model, prefix + "fade_distance_m", "Volumes", "Volume " + std::to_string(index) + " Fade",
                format_float(volume.fade_distance_m));
    }
    for (std::size_t index = 0U; index < profile_v2.weather_timeline.size(); ++index) {
        const auto& keyframe = profile_v2.weather_timeline[index];
        const auto prefix = std::string{"environment.weather_keyframe."} + std::to_string(index) + ".";
        add_row(model, prefix + "time_of_day_hours", "Weather Timeline",
                "Weather Keyframe " + std::to_string(index) + " Time", format_float(keyframe.time_of_day_hours));
        add_row(model, prefix + "weather", "Weather Timeline", "Weather Keyframe " + std::to_string(index) + " Weather",
                std::string{environment_weather_kind_name(keyframe.weather)});
        add_row(model, prefix + "precipitation", "Weather Timeline",
                "Weather Keyframe " + std::to_string(index) + " Precipitation",
                std::string{environment_precipitation_kind_name(keyframe.precipitation)});
        add_row(model, prefix + "quality_preset", "Weather Timeline",
                "Weather Keyframe " + std::to_string(index) + " Quality",
                std::string{environment_quality_preset_name(keyframe.quality_preset)});
    }
    add_row(model, "environment.capture.cubemap.request_status", "Capture", "Cubemap Capture Request", "available",
            false);
    add_default_readiness_rows(model);

    return model;
}

std::vector<EditorPropertyRow>
make_environment_authoring_editor_property_rows(const EnvironmentAuthoringInspectorModel& model) {
    std::vector<EditorPropertyRow> rows;
    rows.reserve(model.rows.size());
    for (const auto& row : model.rows) {
        rows.push_back(EditorPropertyRow{
            .id = row.id,
            .label = row.section + " / " + row.label,
            .value = row.value,
            .editable = row.editable,
        });
    }
    return rows;
}

mirakana::ui::UiDocument make_environment_authoring_ui_model(const EnvironmentAuthoringInspectorModel& model) {
    mirakana::ui::UiDocument document;
    auto root = make_element("environment_authoring", mirakana::ui::SemanticRole::panel);
    root.accessibility_label = "Environment Authoring";
    add_or_throw(document, std::move(root));

    const mirakana::ui::ElementId root_id{"environment_authoring"};
    append_label(document, root_id, "environment_authoring.profile_id", model.profile_id);
    append_label(document, root_id, "environment_authoring.path", model.path);
    append_label(document, root_id, "environment_authoring.status",
                 model.status == EnvironmentAuthoringStatus::ready ? "ready" : "blocked");

    auto rows_root = make_child("environment_authoring.inspector.rows", root_id, mirakana::ui::SemanticRole::list);
    rows_root.accessibility_label = "Environment Inspector Rows";
    add_or_throw(document, std::move(rows_root));
    const mirakana::ui::ElementId rows_id{"environment_authoring.inspector.rows"};

    for (const auto& row : model.rows) {
        auto item = make_child("environment_authoring.inspector.rows." + row.id, rows_id,
                               mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = row.editable;
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{"environment_authoring.inspector.rows." + row.id};
        append_label(document, item_id, "environment_authoring.inspector.rows." + row.id + ".section", row.section);
        append_label(document, item_id, "environment_authoring.inspector.rows." + row.id + ".value", row.value);
    }

    return document;
}

EnvironmentPresetLibraryModel make_environment_preset_library_model(const EnvironmentPresetLibraryDesc& desc) {
    EnvironmentPresetLibraryModel model;
    model.pack_id = desc.pack.id;
    model.profile_id = desc.pack.id;
    model.path = desc.path;

    const auto validation = validate_environment_preset_pack_v1(desc.pack);
    model.status = validation.succeeded() ? EnvironmentAuthoringStatus::ready : EnvironmentAuthoringStatus::blocked;

    add_readiness_row(model, "environment.preset_library.pack.id", "Preset Library", "Pack Id", desc.pack.id);
    add_readiness_row(model, "environment.preset_library.pack.provenance_id", "Preset Library", "Provenance",
                      desc.pack.provenance_id);
    add_readiness_row(model, "environment.preset_library.pack.license_id", "Preset Library", "License",
                      desc.pack.license_id);
    add_readiness_row(model, "environment.preset_library.pack.art_direction", "Preset Library", "Art Direction",
                      desc.pack.art_direction);
    add_readiness_row(model, "environment.preset_library.pack.quality_tier", "Preset Library", "Quality Tier",
                      std::string{environment_quality_preset_name(desc.pack.quality_tier)});
    add_readiness_row(model, "environment.preset_library.pack.preset_count", "Preset Library", "Preset Count",
                      format_uint(desc.pack.presets.size()));
    add_readiness_row(model, "environment.preset_library.pack.package_size_budget_bytes", "Budget",
                      "Package Budget Bytes", std::to_string(desc.pack.package_size_budget_bytes));
    add_readiness_row(model, "environment.preset_library.pack.installed_size_budget_bytes", "Budget",
                      "Installed Budget Bytes", std::to_string(desc.pack.installed_size_budget_bytes));
    add_readiness_row(model, "environment.preset_library.pack.decoded_memory_budget_bytes", "Budget",
                      "Decoded Memory Budget Bytes", std::to_string(desc.pack.decoded_memory_budget_bytes));
    add_readiness_row(model, "environment.preset_library.pack.gpu_memory_budget_bytes", "Budget",
                      "GPU Memory Budget Bytes", std::to_string(desc.pack.gpu_memory_budget_bytes));
    add_readiness_row(model, "environment.preset_library.package.index_registered", "Package Evidence",
                      "Package Index Registered", bool_text(desc.package_index_registered));
    add_readiness_row(model, "environment.preset_library.package.runtime_path", "Package Evidence", "Runtime Path",
                      desc.runtime_package_path);
    add_readiness_row(model, "environment.preset_library.sample.consumption_evidence", "Package Evidence",
                      "Sample Consumption", desc.sample_consumption_evidence ? "ready" : "missing");

    for (std::size_t index = 0U; index < desc.pack.presets.size(); ++index) {
        const auto& preset = desc.pack.presets[index];
        const auto prefix = std::string{"environment.preset_library.preset."} + std::to_string(index) + ".";
        add_readiness_row(model, prefix + "id", "Presets", "Preset " + std::to_string(index) + " Id", preset.id);
        add_readiness_row(model, prefix + "profile_asset_path", "Presets",
                          "Preset " + std::to_string(index) + " Profile", preset.profile_asset_path);
        add_readiness_row(model, prefix + "quality_tier", "Presets", "Preset " + std::to_string(index) + " Quality",
                          std::string{environment_quality_preset_name(preset.quality_tier)});
        add_readiness_row(model, prefix + "validation_recipe_id", "Presets",
                          "Preset " + std::to_string(index) + " Validation", preset.validation_recipe_id);
    }

    add_readiness_row(model, "environment.readiness.unsupported.environment_aaa_preset_library_ready",
                      "Unsupported Claims", "AAA Preset Library Ready", "unclaimed");

    return model;
}

mirakana::ui::UiDocument make_environment_preset_library_ui_model(const EnvironmentPresetLibraryModel& model) {
    mirakana::ui::UiDocument document;
    auto root = make_element("environment_preset_library", mirakana::ui::SemanticRole::panel);
    root.accessibility_label = "Environment Preset Library";
    add_or_throw(document, std::move(root));

    const mirakana::ui::ElementId root_id{"environment_preset_library"};
    append_label(document, root_id, "environment_preset_library.pack_id", model.pack_id);
    append_label(document, root_id, "environment_preset_library.path", model.path);
    append_label(document, root_id, "environment_preset_library.status",
                 model.status == EnvironmentAuthoringStatus::ready ? "ready" : "blocked");

    auto rows_root = make_child("environment_preset_library.rows", root_id, mirakana::ui::SemanticRole::list);
    rows_root.accessibility_label = "Environment Preset Library Rows";
    add_or_throw(document, std::move(rows_root));
    const mirakana::ui::ElementId rows_id{"environment_preset_library.rows"};

    for (const auto& row : model.rows) {
        auto item =
            make_child("environment_preset_library.rows." + row.id, rows_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = row.editable;
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{"environment_preset_library.rows." + row.id};
        append_label(document, item_id, "environment_preset_library.rows." + row.id + ".section", row.section);
        append_label(document, item_id, "environment_preset_library.rows." + row.id + ".value", row.value);
    }

    return document;
}

EnvironmentPresetLibraryReadinessModel
make_environment_preset_library_readiness_model(const EnvironmentPresetLibraryReadinessDesc& desc) {
    EnvironmentPresetLibraryReadinessModel model;
    model.profile_id = desc.library.profile_id;
    model.path = desc.library.path;
    model.dirty = desc.library.dirty;
    model.revision = desc.library.revision;
    model.saved_revision = desc.library.saved_revision;
    model.invokes_backend = desc.request_backend_execution;
    model.exposes_native_handles = desc.request_native_handle_access;
    model.executes_package_scripts = desc.request_package_script_execution;

    const auto* provenance_row = find_inspector_row(desc.library, "environment.preset_library.pack.provenance_id");
    const auto* license_row = find_inspector_row(desc.library, "environment.preset_library.pack.license_id");
    const auto* sample_row = find_inspector_row(desc.library, "environment.preset_library.sample.consumption_evidence");

    const bool pack_validation_ready = desc.library.status == EnvironmentAuthoringStatus::ready;
    const bool budget_rows_ready =
        has_nonzero_row_value(desc.library, "environment.preset_library.pack.package_size_budget_bytes") &&
        has_nonzero_row_value(desc.library, "environment.preset_library.pack.installed_size_budget_bytes") &&
        has_nonzero_row_value(desc.library, "environment.preset_library.pack.decoded_memory_budget_bytes") &&
        has_nonzero_row_value(desc.library, "environment.preset_library.pack.gpu_memory_budget_bytes");
    const bool license_and_provenance_ready = desc.license_and_provenance_ready && provenance_row != nullptr &&
                                              !provenance_row->value.empty() && license_row != nullptr &&
                                              !license_row->value.empty();
    const bool editor_browsing_ready = desc.editor_browsing_rows_ready && !desc.library.rows.empty();
    const bool sample_scene_consumption_ready =
        desc.sample_scene_consumption_ready && sample_row != nullptr && sample_row->value == "ready";
    const bool unsafe_request_absent =
        !desc.request_backend_execution && !desc.request_package_script_execution && !desc.request_native_handle_access;

    add_preset_library_readiness_requirement(model, "environment.preset_library.readiness.pack_validation",
                                             "Preset Pack Validation", pack_validation_ready,
                                             "preset pack validation must be ready");
    add_preset_library_readiness_requirement(model, "environment.preset_library.readiness.budget_rows",
                                             "Package And Memory Budgets", budget_rows_ready,
                                             "package, installed, decoded-memory, and GPU-memory budgets must be set");
    add_preset_library_readiness_requirement(model, "environment.preset_library.readiness.license_and_provenance",
                                             "License And Provenance", license_and_provenance_ready,
                                             "license and provenance evidence must be reviewed");
    add_preset_library_readiness_requirement(model, "environment.preset_library.readiness.package_smoke",
                                             "Package Smoke", desc.package_smoke_ready,
                                             "package smoke evidence must pass");
    add_preset_library_readiness_requirement(model, "environment.preset_library.readiness.installed_package_smoke",
                                             "Installed Package Smoke", desc.installed_package_smoke_ready,
                                             "installed package smoke evidence must pass");
    add_preset_library_readiness_requirement(model, "environment.preset_library.readiness.editor_browsing",
                                             "Editor Browsing Rows", editor_browsing_ready,
                                             "editor browsing rows must be present");
    add_preset_library_readiness_requirement(model, "environment.preset_library.readiness.sample_scene_consumption",
                                             "Sample Scene Consumption", sample_scene_consumption_ready,
                                             "sample scene consumption evidence must be ready");
    add_preset_library_readiness_requirement(model, "environment.preset_library.readiness.external_asset_notices",
                                             "External Asset Notices", desc.external_asset_notices_ready,
                                             "external asset notices must be recorded or explicitly not required");
    add_preset_library_readiness_requirement(model, "environment.preset_library.readiness.no_unsafe_execution",
                                             "No Unsafe Execution", unsafe_request_absent,
                                             "preset-library readiness review must not execute backend work, package "
                                             "scripts, or expose native handles");

    model.environment_aaa_preset_library_ready =
        model.diagnostics.empty() && pack_validation_ready && budget_rows_ready && license_and_provenance_ready &&
        desc.package_smoke_ready && desc.installed_package_smoke_ready && editor_browsing_ready &&
        sample_scene_consumption_ready && desc.external_asset_notices_ready && unsafe_request_absent;
    model.status = model.environment_aaa_preset_library_ready ? EnvironmentAuthoringStatus::ready
                                                              : EnvironmentAuthoringStatus::blocked;

    add_readiness_row(model, "environment.preset_library.readiness.environment_aaa_preset_library_ready",
                      "AAA Preset Library Readiness", "AAA Preset Library Ready",
                      model.environment_aaa_preset_library_ready ? "ready" : "blocked");

    return model;
}

mirakana::ui::UiDocument
make_environment_preset_library_readiness_ui_model(const EnvironmentPresetLibraryReadinessModel& model) {
    mirakana::ui::UiDocument document;
    auto root = make_element("environment_preset_library_readiness", mirakana::ui::SemanticRole::panel);
    root.accessibility_label = "Environment Preset Library Readiness";
    add_or_throw(document, std::move(root));

    const mirakana::ui::ElementId root_id{"environment_preset_library_readiness"};
    append_label(document, root_id, "environment_preset_library_readiness.path", model.path);
    append_label(document, root_id, "environment_preset_library_readiness.status",
                 model.status == EnvironmentAuthoringStatus::ready ? "ready" : "blocked");
    append_label(document, root_id, "environment_preset_library_readiness.environment_aaa_preset_library_ready",
                 model.environment_aaa_preset_library_ready ? "ready" : "blocked");

    auto rows_root = make_child("environment_preset_library_readiness.rows", root_id, mirakana::ui::SemanticRole::list);
    rows_root.accessibility_label = "Environment Preset Library Readiness Rows";
    add_or_throw(document, std::move(rows_root));
    const mirakana::ui::ElementId rows_id{"environment_preset_library_readiness.rows"};

    for (const auto& row : model.rows) {
        auto item = make_child("environment_preset_library_readiness.rows." + row.id, rows_id,
                               mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = false;
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{"environment_preset_library_readiness.rows." + row.id};
        append_label(document, item_id, "environment_preset_library_readiness.rows." + row.id + ".section",
                     row.section);
        append_label(document, item_id, "environment_preset_library_readiness.rows." + row.id + ".value", row.value);
    }

    return document;
}

std::vector<EnvironmentPackageCandidateRow>
make_environment_package_candidate_rows(const EnvironmentAuthoringDocument& document,
                                        std::string_view cooked_profile_path, std::string_view package_index_path) {
    return {
        EnvironmentPackageCandidateRow{
            .kind = EnvironmentPackageCandidateKind::profile_source,
            .path = std::string{document.path()},
            .runtime_file = false,
        },
        EnvironmentPackageCandidateRow{
            .kind = EnvironmentPackageCandidateKind::profile_cooked,
            .path = std::string{cooked_profile_path},
            .runtime_file = true,
        },
        EnvironmentPackageCandidateRow{
            .kind = EnvironmentPackageCandidateKind::package_index,
            .path = std::string{package_index_path},
            .runtime_file = true,
        },
    };
}

std::vector<EnvironmentPackageCandidateRow>
make_environment_preset_library_package_candidate_rows(std::string_view runtime_preset_pack_path) {
    return {
        EnvironmentPackageCandidateRow{
            .kind = EnvironmentPackageCandidateKind::preset_pack,
            .path = std::string{runtime_preset_pack_path},
            .runtime_file = true,
        },
    };
}

std::vector<EnvironmentPackageRegistrationDraftRow>
make_environment_package_registration_draft_rows(std::span<const EnvironmentPackageCandidateRow> candidates,
                                                 std::string_view project_root_path,
                                                 std::span<const std::string> existing_runtime_files) {
    std::unordered_set<std::string> existing;
    for (const auto& path : existing_runtime_files) {
        existing.insert(package_key(path));
    }

    std::unordered_set<std::string> seen;
    std::vector<EnvironmentPackageRegistrationDraftRow> rows;
    rows.reserve(candidates.size());

    for (const auto& candidate : candidates) {
        EnvironmentPackageRegistrationDraftRow row{
            .kind = candidate.kind,
            .candidate_path = candidate.path,
            .runtime_package_path = strip_project_root(candidate.path, project_root_path),
            .runtime_file = candidate.runtime_file,
        };

        const auto key = package_key(row.runtime_package_path);
        if (!row.runtime_file) {
            row.status = EnvironmentPackageRegistrationDraftStatus::rejected_source_file;
            row.diagnostic = "source environment profile is not a runtime package file";
        } else if (!safe_runtime_package_path(row.runtime_package_path)) {
            row.status = EnvironmentPackageRegistrationDraftStatus::rejected_unsafe_path;
            row.diagnostic = "runtime package path must stay under runtime/";
        } else if (existing.contains(key)) {
            row.status = EnvironmentPackageRegistrationDraftStatus::already_registered;
            row.diagnostic = "runtime package file is already registered";
        } else if (seen.contains(key)) {
            row.status = EnvironmentPackageRegistrationDraftStatus::rejected_duplicate;
            row.diagnostic = "duplicate runtime package file candidate";
        } else {
            row.status = EnvironmentPackageRegistrationDraftStatus::add_runtime_file;
            row.diagnostic = "reviewed runtime package file can be added";
            seen.insert(key);
        }

        rows.push_back(std::move(row));
    }

    return rows;
}

} // namespace mirakana::editor
