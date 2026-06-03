// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/environment_authoring.hpp"

#include "mirakana/environment/environment_io.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
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

} // namespace

bool EnvironmentAuthoringValidationModel::succeeded() const noexcept {
    return diagnostics.empty();
}

EnvironmentAuthoringDocument EnvironmentAuthoringDocument::from_profile(EnvironmentProfileDesc profile,
                                                                        std::string path) {
    EnvironmentAuthoringDocument document;
    document.profile_ = std::move(profile);
    document.path_ = std::move(path);
    document.dirty_.reset_clean();
    return document;
}

const EnvironmentProfileDesc& EnvironmentAuthoringDocument::profile() const noexcept {
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
    profile_ = std::move(profile);
    dirty_.mark_dirty();
}

void EnvironmentAuthoringDocument::mark_saved() noexcept {
    dirty_.mark_saved();
}

void EnvironmentAuthoringDocument::set_path(std::string path) {
    path_ = std::move(path);
}

std::string_view environment_authoring_quality_tier_label(EnvironmentAuthoringQualityTier tier) noexcept {
    switch (tier) {
    case EnvironmentAuthoringQualityTier::low:
        return "low";
    case EnvironmentAuthoringQualityTier::medium:
        return "medium";
    case EnvironmentAuthoringQualityTier::high:
        return "high";
    case EnvironmentAuthoringQualityTier::cinematic:
        return "cinematic";
    }
    return "high";
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
    return EnvironmentAuthoringDocument::from_profile(deserialize_environment_profile(store.read_text(path)),
                                                      std::string{path});
}

void save_environment_authoring_document(ITextStore& store, std::string_view path,
                                         EnvironmentAuthoringDocument& document) {
    store.write_text(path, serialize_environment_profile(document.profile_));
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

EnvironmentAuthoringValidationModel
make_environment_authoring_validation_model(const EnvironmentAuthoringDocument& document) {
    EnvironmentAuthoringValidationModel model{.status = EnvironmentAuthoringStatus::ready};
    add_diagnostics(model, validate_environment_profile(document.profile()));
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
            std::string{environment_authoring_quality_tier_label(desc.quality_tier)});

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
