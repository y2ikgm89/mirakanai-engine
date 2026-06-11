// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/environment_authoring.hpp"

#include "mirakana/environment/environment_io.hpp"

#include <algorithm>
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

[[nodiscard]] bool row_id_starts_with(const EnvironmentAuthoringInspectorRow& row, std::string_view prefix) noexcept {
    return std::string_view{row.id}.starts_with(prefix);
}

[[nodiscard]] bool belongs_to_environment_settings_section(std::string_view section_id,
                                                           const EnvironmentAuthoringInspectorRow& row) noexcept {
    if (section_id == "environment_settings.global") {
        return row_id_starts_with(row, "environment.sky.") || row_id_starts_with(row, "environment.sun.") ||
               row_id_starts_with(row, "environment.moon.") || row_id_starts_with(row, "environment.atmosphere.") ||
               row_id_starts_with(row, "environment.fog.") || row_id_starts_with(row, "environment.cloud_layer.") ||
               row_id_starts_with(row, "environment.precipitation.");
    }
    if (section_id == "environment_settings.volumes") {
        return row.id == "environment.profile_v2.volume_count" || row_id_starts_with(row, "environment.volume.");
    }
    if (section_id == "environment_settings.weather") {
        return row.id == "environment.weather.preset" || row.id == "environment.profile_v2.weather_keyframes" ||
               row_id_starts_with(row, "environment.weather_keyframe.");
    }
    if (section_id == "environment_settings.quality") {
        return row.id == "environment.quality.tier";
    }
    if (section_id == "environment_settings.preview") {
        return row_id_starts_with(row, "environment.capture.");
    }
    if (section_id == "environment_settings.readiness") {
        return row_id_starts_with(row, "environment.readiness.");
    }
    return false;
}

void summarize_environment_settings_rows(EnvironmentSettingsWorkflowSectionRow& section,
                                         std::span<const EnvironmentAuthoringInspectorRow> rows) noexcept {
    for (const auto& row : rows) {
        if (!belongs_to_environment_settings_section(section.id, row)) {
            continue;
        }
        ++section.row_count;
        if (row.editable) {
            ++section.editable_row_count;
        } else {
            ++section.read_only_row_count;
        }
    }
}

[[nodiscard]] std::string_view
environment_settings_preview_handoff_status_label(EnvironmentSettingsPreviewHandoffStatus status) noexcept {
    switch (status) {
    case EnvironmentSettingsPreviewHandoffStatus::available:
        return "available";
    case EnvironmentSettingsPreviewHandoffStatus::requested:
        return "requested";
    case EnvironmentSettingsPreviewHandoffStatus::blocked_by_validation:
        return "blocked_by_validation";
    case EnvironmentSettingsPreviewHandoffStatus::host_gated:
        return "host_gated";
    case EnvironmentSettingsPreviewHandoffStatus::ready_for_operator_handoff:
        return "ready_for_operator_handoff";
    }
    return "blocked_by_validation";
}

[[nodiscard]] EnvironmentSettingsPreviewHandoffRow
make_environment_settings_preview_handoff_row(const EnvironmentSettingsPreviewRecipeDesc& recipe,
                                              bool cubemap_preview_requested) {
    EnvironmentSettingsPreviewHandoffRow row{
        .recipe_id = recipe.recipe_id,
        .host_gates = recipe.host_gates,
        .requests_cubemap_capture = cubemap_preview_requested,
    };

    if (!cubemap_preview_requested) {
        row.status = EnvironmentSettingsPreviewHandoffStatus::available;
    } else if (!recipe.validation_available) {
        row.status = EnvironmentSettingsPreviewHandoffStatus::blocked_by_validation;
        row.blocked_by.push_back("validation_recipe_missing");
    } else if (!recipe.selected) {
        row.status = EnvironmentSettingsPreviewHandoffStatus::requested;
        row.blocked_by.push_back("operator_recipe_selection_required");
    } else if (!recipe.host_gates.empty() && !recipe.host_available) {
        row.status = EnvironmentSettingsPreviewHandoffStatus::host_gated;
        row.blocked_by = recipe.host_gates;
    } else {
        row.status = EnvironmentSettingsPreviewHandoffStatus::ready_for_operator_handoff;
    }

    row.status_label = std::string{environment_settings_preview_handoff_status_label(row.status)};
    return row;
}

[[nodiscard]] std::vector<EnvironmentSettingsPreviewHandoffRow>
make_environment_settings_preview_handoff_rows(std::span<const EnvironmentSettingsPreviewRecipeDesc> recipes,
                                               bool cubemap_preview_requested) {
    std::vector<EnvironmentSettingsPreviewHandoffRow> rows;
    rows.reserve(recipes.size());
    for (const auto& recipe : recipes) {
        rows.push_back(make_environment_settings_preview_handoff_row(recipe, cubemap_preview_requested));
    }
    return rows;
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
    case EnvironmentAuthoringCommandKind::edit_volume:
        return "Edit Environment Volume";
    case EnvironmentAuthoringCommandKind::remove_volume:
        return "Remove Environment Volume";
    case EnvironmentAuthoringCommandKind::reorder_volume:
        return "Reorder Environment Volume";
    case EnvironmentAuthoringCommandKind::add_weather_keyframe:
        return "Add Weather Keyframe";
    case EnvironmentAuthoringCommandKind::edit_weather_keyframe:
        return "Edit Weather Keyframe";
    case EnvironmentAuthoringCommandKind::remove_weather_keyframe:
        return "Remove Weather Keyframe";
    case EnvironmentAuthoringCommandKind::reorder_weather_keyframe:
        return "Reorder Weather Keyframe";
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
    case EnvironmentAuthoringCommandKind::edit_volume:
        return "environment.command.volume.edit";
    case EnvironmentAuthoringCommandKind::remove_volume:
        return "environment.command.volume.remove";
    case EnvironmentAuthoringCommandKind::reorder_volume:
        return "environment.command.volume.reorder";
    case EnvironmentAuthoringCommandKind::add_weather_keyframe:
        return "environment.command.weather_keyframe.add";
    case EnvironmentAuthoringCommandKind::edit_weather_keyframe:
        return "environment.command.weather_keyframe.edit";
    case EnvironmentAuthoringCommandKind::remove_weather_keyframe:
        return "environment.command.weather_keyframe.remove";
    case EnvironmentAuthoringCommandKind::reorder_weather_keyframe:
        return "environment.command.weather_keyframe.reorder";
    case EnvironmentAuthoringCommandKind::select_quality_preset:
        return "environment.command.quality_preset.select";
    case EnvironmentAuthoringCommandKind::request_cubemap_capture:
        return "environment.command.capture.cubemap.request";
    }
    return "environment.command.invalid";
}

void apply_command(EnvironmentProfileDocumentV2& document, const EnvironmentAuthoringCommandRequest& request) {
    switch (request.kind) {
    case EnvironmentAuthoringCommandKind::add_volume:
        document.volumes.push_back(request.volume);
        return;
    case EnvironmentAuthoringCommandKind::edit_volume: {
        const auto index = find_volume_index_by_id(document, request.volume_id);
        if (index < document.volumes.size()) {
            document.volumes[index] = request.volume;
        }
        return;
    }
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
    case EnvironmentAuthoringCommandKind::add_weather_keyframe:
        document.weather_timeline.push_back(request.weather_keyframe);
        return;
    case EnvironmentAuthoringCommandKind::edit_weather_keyframe:
        if (request.weather_keyframe_index < document.weather_timeline.size()) {
            document.weather_timeline[request.weather_keyframe_index] = request.weather_keyframe;
        }
        return;
    case EnvironmentAuthoringCommandKind::remove_weather_keyframe:
        if (request.weather_keyframe_index < document.weather_timeline.size()) {
            document.weather_timeline.erase(document.weather_timeline.begin() +
                                            static_cast<std::ptrdiff_t>(request.weather_keyframe_index));
        }
        return;
    case EnvironmentAuthoringCommandKind::reorder_weather_keyframe: {
        const auto source = static_cast<std::size_t>(request.source_index);
        const auto target = static_cast<std::size_t>(request.target_index);
        if (source >= document.weather_timeline.size() || target >= document.weather_timeline.size() ||
            source == target) {
            return;
        }
        auto row = document.weather_timeline[source];
        document.weather_timeline.erase(document.weather_timeline.begin() + static_cast<std::ptrdiff_t>(source));
        document.weather_timeline.insert(document.weather_timeline.begin() + static_cast<std::ptrdiff_t>(target), row);
        return;
    }
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
        request.request_validation_recipe_execution || request.request_shell_execution ||
        request.request_native_handle_access) {
        plan.status = EnvironmentAuthoringCommandStatus::rejected_unsafe_execution;
        add_command_diagnostic(plan, "unsafe_execution",
                               "environment authoring commands cannot execute backend work, package scripts, "
                               "validation recipes, shell commands, or native handle access");
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
    case EnvironmentAuthoringCommandKind::edit_volume: {
        plan.mutates_document = true;
        const auto index = find_volume_index_by_id(profile, request.volume_id);
        if (index >= profile.volumes.size()) {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_not_found;
            add_command_diagnostic(plan, "volume_not_found", "environment volume id was not found");
        } else if (request.volume.id.empty()) {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
            add_command_diagnostic(plan, "empty_volume_id", "environment volume id must not be empty");
        } else {
            auto copy = profile;
            copy.volumes[index] = request.volume;
            const auto validation = validate_environment_profile_v2(copy);
            if (validation.succeeded()) {
                accept_command(plan);
            } else {
                plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
                add_command_diagnostic(plan, "invalid_volume", "environment volume validation failed");
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
    case EnvironmentAuthoringCommandKind::add_weather_keyframe: {
        plan.mutates_document = true;
        auto copy = profile;
        copy.weather_timeline.push_back(request.weather_keyframe);
        const auto validation = validate_environment_profile_v2(copy);
        if (validation.succeeded()) {
            accept_command(plan);
        } else {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
            add_command_diagnostic(plan, "invalid_weather_keyframe", "weather keyframe validation failed");
        }
        return plan;
    }
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
    case EnvironmentAuthoringCommandKind::remove_weather_keyframe:
        plan.mutates_document = true;
        if (request.weather_keyframe_index >= profile.weather_timeline.size()) {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_not_found;
            add_command_diagnostic(plan, "weather_keyframe_not_found", "weather keyframe index was not found");
        } else {
            auto copy = profile;
            copy.weather_timeline.erase(copy.weather_timeline.begin() +
                                        static_cast<std::ptrdiff_t>(request.weather_keyframe_index));
            const auto validation = validate_environment_profile_v2(copy);
            if (validation.succeeded()) {
                accept_command(plan);
            } else {
                plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
                add_command_diagnostic(plan, "invalid_weather_timeline", "weather timeline validation failed");
            }
        }
        return plan;
    case EnvironmentAuthoringCommandKind::reorder_weather_keyframe:
        plan.mutates_document = true;
        if (request.source_index >= profile.weather_timeline.size() ||
            request.target_index >= profile.weather_timeline.size()) {
            plan.status = EnvironmentAuthoringCommandStatus::rejected_not_found;
            add_command_diagnostic(plan, "weather_keyframe_index_not_found",
                                   "weather keyframe reorder index was not found");
        } else if (request.source_index == request.target_index) {
            plan.mutates_document = false;
            plan.status = EnvironmentAuthoringCommandStatus::rejected_invalid_request;
            add_command_diagnostic(plan, "weather_keyframe_reorder_noop",
                                   "weather keyframe reorder requires distinct indexes");
        } else {
            accept_command(plan);
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

EnvironmentSettingsWorkflowModel make_environment_settings_workflow_model(const EnvironmentSettingsWorkflowDesc& desc) {
    const auto inspector = make_environment_authoring_inspector_model(desc.inspector);
    const auto package_candidates = make_environment_package_candidate_rows(
        desc.inspector.document, desc.cooked_profile_path, desc.package_index_path);
    const auto package_draft_rows = make_environment_package_registration_draft_rows(
        package_candidates, desc.project_root_path, desc.existing_runtime_files);

    EnvironmentSettingsWorkflowModel model;
    model.status = inspector.status;
    model.surface_id = "environment_settings";
    model.profile_id = inspector.profile_id;
    model.path = inspector.path;
    model.dirty = inspector.dirty;
    model.revision = inspector.revision;
    model.saved_revision = inspector.saved_revision;
    model.invokes_backend = inspector.invokes_backend;
    model.exposes_native_handles = inspector.exposes_native_handles;
    model.executes_package_scripts = inspector.executes_package_scripts;
    model.rows = inspector.rows;
    model.diagnostics = inspector.diagnostics;
    model.package_draft_rows = package_draft_rows;
    model.validation_recipe_ids.assign(desc.validation_recipe_ids.begin(), desc.validation_recipe_ids.end());
    model.preview_handoff_rows =
        make_environment_settings_preview_handoff_rows(desc.preview_recipes, desc.cubemap_preview_requested);

    model.sections = {
        EnvironmentSettingsWorkflowSectionRow{.id = "environment_settings.global", .label = "Global"},
        EnvironmentSettingsWorkflowSectionRow{.id = "environment_settings.volumes", .label = "Volumes"},
        EnvironmentSettingsWorkflowSectionRow{.id = "environment_settings.weather", .label = "Weather"},
        EnvironmentSettingsWorkflowSectionRow{.id = "environment_settings.quality", .label = "Quality"},
        EnvironmentSettingsWorkflowSectionRow{.id = "environment_settings.preview", .label = "Preview"},
        EnvironmentSettingsWorkflowSectionRow{.id = "environment_settings.package", .label = "Package"},
        EnvironmentSettingsWorkflowSectionRow{.id = "environment_settings.readiness", .label = "Readiness"},
    };

    for (auto& section : model.sections) {
        if (section.id == "environment_settings.package") {
            section.row_count = static_cast<std::uint32_t>(model.package_draft_rows.size());
            section.read_only_row_count = section.row_count;
        } else {
            summarize_environment_settings_rows(section, model.rows);
            if (section.id == "environment_settings.preview") {
                const auto preview_rows = static_cast<std::uint32_t>(model.preview_handoff_rows.size());
                section.row_count += preview_rows;
                section.read_only_row_count += preview_rows;
            }
        }
    }

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
