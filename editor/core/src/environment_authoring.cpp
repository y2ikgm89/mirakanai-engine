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
