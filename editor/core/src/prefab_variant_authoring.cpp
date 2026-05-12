// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/prefab_variant_authoring.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] bool valid_text_field(std::string_view value) noexcept {
    return !value.empty() && value.find_first_of("\r\n=") == std::string_view::npos;
}

[[nodiscard]] bool has_node(const PrefabDefinition& prefab, std::uint32_t node_index) noexcept {
    return node_index != 0 && node_index <= prefab.nodes.size();
}

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool valid_transform(Transform3D transform) noexcept {
    return finite_vec3(transform.position) && finite_vec3(transform.rotation_radians) && finite_vec3(transform.scale) &&
           transform.scale.x > 0.0F && transform.scale.y > 0.0F && transform.scale.z > 0.0F;
}

[[nodiscard]] std::string node_name_for(const PrefabDefinition& prefab, std::uint32_t node_index) {
    return has_node(prefab, node_index) ? prefab.nodes[node_index - 1U].name : std::string{};
}

[[nodiscard]] bool same_override_key(const PrefabNodeOverride& lhs, const PrefabNodeOverride& rhs) noexcept {
    return lhs.node_index == rhs.node_index && lhs.kind == rhs.kind;
}

[[nodiscard]] bool upsert_override(PrefabVariantDefinition& variant, PrefabNodeOverride override) {
    auto next = variant;
    const auto existing = std::ranges::find_if(next.overrides, [&override](const PrefabNodeOverride& candidate) {
        return same_override_key(candidate, override);
    });

    if (existing == next.overrides.end()) {
        next.overrides.push_back(std::move(override));
    } else {
        *existing = std::move(override);
    }

    if (!is_valid_prefab_variant_definition(next)) {
        return false;
    }

    variant = std::move(next);
    return true;
}

[[nodiscard]] bool repairable_for_prefab_variant_authoring(const PrefabVariantDefinition& variant) {
    for (const auto& diagnostic : validate_prefab_variant_definition(variant)) {
        switch (diagnostic.kind) {
        case PrefabVariantDiagnosticKind::invalid_node_index:
        case PrefabVariantDiagnosticKind::duplicate_override:
            break;
        case PrefabVariantDiagnosticKind::invalid_base_prefab:
        case PrefabVariantDiagnosticKind::invalid_variant_name:
        case PrefabVariantDiagnosticKind::invalid_override_kind:
        case PrefabVariantDiagnosticKind::invalid_node_name:
        case PrefabVariantDiagnosticKind::invalid_source_node_name:
        case PrefabVariantDiagnosticKind::invalid_transform:
        case PrefabVariantDiagnosticKind::invalid_components:
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string serialize_prefab_variant_if_valid(const PrefabVariantDefinition& variant) {
    try {
        return serialize_prefab_variant_definition(variant);
    } catch (const std::invalid_argument&) {
        return {};
    }
}

void add_asset_reference_diagnostic(std::vector<PrefabVariantAuthoringDiagnostic>& diagnostics,
                                    std::uint32_t node_index, PrefabOverrideKind override_kind, AssetId asset,
                                    std::string field, AssetKind expected_kind, const AssetRegistry& registry) {
    const auto* record = registry.find(asset);
    if (record == nullptr) {
        diagnostics.push_back(PrefabVariantAuthoringDiagnostic{
            .kind = PrefabVariantAuthoringDiagnosticKind::missing_asset,
            .node_index = node_index,
            .override_kind = override_kind,
            .asset = asset,
            .field = std::move(field),
            .diagnostic = "missing asset reference",
        });
        return;
    }

    if (record->kind != expected_kind) {
        diagnostics.push_back(PrefabVariantAuthoringDiagnostic{
            .kind = PrefabVariantAuthoringDiagnosticKind::wrong_asset_kind,
            .node_index = node_index,
            .override_kind = override_kind,
            .asset = asset,
            .field = std::move(field),
            .diagnostic = "asset reference has wrong kind",
        });
    }
}

void add_component_reference_diagnostics(std::vector<PrefabVariantAuthoringDiagnostic>& diagnostics,
                                         const PrefabNodeOverride& override, const AssetRegistry& registry) {
    if (!override.components.mesh_renderer.has_value() && !override.components.sprite_renderer.has_value()) {
        return;
    }

    if (override.components.mesh_renderer.has_value()) {
        add_asset_reference_diagnostic(diagnostics, override.node_index, override.kind,
                                       override.components.mesh_renderer->mesh, "mesh_renderer.mesh", AssetKind::mesh,
                                       registry);
        add_asset_reference_diagnostic(diagnostics, override.node_index, override.kind,
                                       override.components.mesh_renderer->material, "mesh_renderer.material",
                                       AssetKind::material, registry);
    }

    if (override.components.sprite_renderer.has_value()) {
        add_asset_reference_diagnostic(diagnostics, override.node_index, override.kind,
                                       override.components.sprite_renderer->sprite, "sprite_renderer.sprite",
                                       AssetKind::texture, registry);
        add_asset_reference_diagnostic(diagnostics, override.node_index, override.kind,
                                       override.components.sprite_renderer->material, "sprite_renderer.material",
                                       AssetKind::material, registry);
    }
}

void sort_diagnostics(std::vector<PrefabVariantAuthoringDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.node_index != rhs.node_index) {
            return lhs.node_index < rhs.node_index;
        }
        if (lhs.field != rhs.field) {
            return lhs.field < rhs.field;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        return static_cast<unsigned int>(lhs.kind) < static_cast<unsigned int>(rhs.kind);
    });
}

[[nodiscard]] std::size_t diagnostic_count_for(const std::vector<PrefabVariantAuthoringDiagnostic>& diagnostics,
                                               std::uint32_t node_index, PrefabOverrideKind kind) {
    return static_cast<std::size_t>(std::ranges::count_if(diagnostics, [node_index, kind](const auto& diagnostic) {
        return diagnostic.node_index == node_index && diagnostic.override_kind == kind;
    }));
}

[[nodiscard]] std::vector<PrefabVariantOverrideRow>
make_override_rows(const PrefabVariantDefinition& variant,
                   const std::vector<PrefabVariantAuthoringDiagnostic>& diagnostics) {
    std::vector<PrefabVariantOverrideRow> rows;
    rows.reserve(variant.overrides.size());
    for (const auto& override : variant.overrides) {
        rows.push_back(PrefabVariantOverrideRow{
            .node_index = override.node_index,
            .kind = override.kind,
            .node_name = node_name_for(variant.base_prefab, override.node_index),
            .kind_label = std::string(prefab_override_kind_label(override.kind)),
            .has_name = override.kind == PrefabOverrideKind::name,
            .has_transform = override.kind == PrefabOverrideKind::transform,
            .has_components = override.kind == PrefabOverrideKind::components,
            .has_camera = override.kind == PrefabOverrideKind::components && override.components.camera.has_value(),
            .has_light = override.kind == PrefabOverrideKind::components && override.components.light.has_value(),
            .has_mesh_renderer =
                override.kind == PrefabOverrideKind::components && override.components.mesh_renderer.has_value(),
            .has_sprite_renderer =
                override.kind == PrefabOverrideKind::components && override.components.sprite_renderer.has_value(),
            .diagnostic_count = diagnostic_count_for(diagnostics, override.node_index, override.kind),
        });
    }
    return rows;
}

[[nodiscard]] PrefabVariantAuthoringModel make_model(const PrefabVariantAuthoringDocument& document,
                                                     const AssetRegistry* registry) {
    auto diagnostics = registry == nullptr ? validate_prefab_variant_authoring_document(document)
                                           : validate_prefab_variant_authoring_document(document, *registry);
    sort_diagnostics(diagnostics);

    PrefabVariantAuthoringModel model;
    model.path = std::string(document.path());
    model.name = document.variant().name;
    model.override_rows = make_override_rows(document.variant(), diagnostics);
    model.diagnostics = std::move(diagnostics);
    model.dirty = document.dirty();
    return model;
}

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

[[nodiscard]] std::string format_float(float value) {
    char buffer[32]{};
    (void)std::snprintf(buffer, sizeof(buffer), "%.3f", static_cast<double>(value));
    return std::string(buffer);
}

[[nodiscard]] std::string format_vec3(Vec3 value) {
    return format_float(value.x) + "," + format_float(value.y) + "," + format_float(value.z);
}

[[nodiscard]] std::string format_transform(Transform3D transform) {
    return "pos=" + format_vec3(transform.position) + " rot=" + format_vec3(transform.rotation_radians) +
           " scale=" + format_vec3(transform.scale);
}

[[nodiscard]] std::string format_asset_id(AssetId asset) {
    return std::to_string(asset.value);
}

[[nodiscard]] std::string format_bool(bool value) {
    return value ? "true" : "false";
}

[[nodiscard]] std::string format_camera_projection(CameraProjectionMode projection) {
    switch (projection) {
    case CameraProjectionMode::perspective:
        return "perspective";
    case CameraProjectionMode::orthographic:
        return "orthographic";
    case CameraProjectionMode::unknown:
        return "unknown";
    }
    return "unknown";
}

[[nodiscard]] std::string format_light_type(LightType type) {
    switch (type) {
    case LightType::directional:
        return "directional";
    case LightType::point:
        return "point";
    case LightType::spot:
        return "spot";
    case LightType::unknown:
        return "unknown";
    }
    return "unknown";
}

[[nodiscard]] std::string format_color4(const std::array<float, 4>& color) {
    return format_float(color[0]) + "," + format_float(color[1]) + "," + format_float(color[2]) + "," +
           format_float(color[3]);
}

[[nodiscard]] std::string format_camera(const CameraComponent& camera) {
    return format_camera_projection(camera.projection) + "/fov=" + format_float(camera.vertical_fov_radians) +
           "/ortho=" + format_float(camera.orthographic_height) + "/near=" + format_float(camera.near_plane) +
           "/far=" + format_float(camera.far_plane) + "/primary=" + format_bool(camera.primary);
}

[[nodiscard]] std::string format_light(const LightComponent& light) {
    return format_light_type(light.type) + "/color=" + format_vec3(light.color) +
           "/intensity=" + format_float(light.intensity) + "/range=" + format_float(light.range) +
           "/inner=" + format_float(light.inner_cone_radians) + "/outer=" + format_float(light.outer_cone_radians) +
           "/shadows=" + format_bool(light.casts_shadows);
}

[[nodiscard]] std::string format_components(const SceneNodeComponents& components) {
    std::string text = components.camera.has_value() ? "camera=" + format_camera(*components.camera) : "camera=no";
    text += components.light.has_value() ? " light=" + format_light(*components.light) : " light=no";
    if (components.mesh_renderer.has_value()) {
        text += " mesh=" + format_asset_id(components.mesh_renderer->mesh) + "/" +
                format_asset_id(components.mesh_renderer->material) + "/" +
                (components.mesh_renderer->visible ? std::string("visible") : std::string("hidden"));
    } else {
        text += " mesh=no";
    }
    if (components.sprite_renderer.has_value()) {
        text += " sprite=" + format_asset_id(components.sprite_renderer->sprite) + "/" +
                format_asset_id(components.sprite_renderer->material) +
                "/size=" + format_float(components.sprite_renderer->size.x) + "x" +
                format_float(components.sprite_renderer->size.y) +
                "/tint=" + format_color4(components.sprite_renderer->tint) + "/" +
                (components.sprite_renderer->visible ? std::string("visible") : std::string("hidden"));
    } else {
        text += " sprite=no";
    }
    return text;
}

[[nodiscard]] unsigned int component_family_mask(const SceneNodeComponents& components) noexcept {
    unsigned int mask = 0;
    if (components.camera.has_value()) {
        mask |= 1U << 0U;
    }
    if (components.light.has_value()) {
        mask |= 1U << 1U;
    }
    if (components.mesh_renderer.has_value()) {
        mask |= 1U << 2U;
    }
    if (components.sprite_renderer.has_value()) {
        mask |= 1U << 3U;
    }
    return mask;
}

[[nodiscard]] std::string override_key(const PrefabNodeOverride& override) {
    return std::to_string(override.node_index) + ":" + std::string(prefab_override_kind_label(override.kind));
}

[[nodiscard]] std::string override_row_base_id(const PrefabNodeOverride& override) {
    return "node." + std::to_string(override.node_index) + "." + std::string(prefab_override_kind_label(override.kind));
}

[[nodiscard]] std::string override_row_id(const PrefabNodeOverride& override, std::size_t occurrence) {
    auto id = override_row_base_id(override);
    if (occurrence > 1U) {
        id += ".duplicate." + std::to_string(occurrence);
    }
    return id;
}

[[nodiscard]] std::string remove_resolution_id_for(std::string_view row_id) {
    return "remove." + std::string(row_id);
}

[[nodiscard]] std::string retarget_resolution_id_for(std::string_view row_id, std::uint32_t target_node_index) {
    return "retarget." + std::string(row_id) + ".to." + std::to_string(target_node_index);
}

[[nodiscard]] std::string accept_current_resolution_id_for(std::string_view row_id) {
    return "accept_current." + std::string(row_id);
}

[[nodiscard]] bool has_safe_remove_resolution(PrefabVariantConflictKind conflict) noexcept {
    return conflict == PrefabVariantConflictKind::missing_node ||
           conflict == PrefabVariantConflictKind::redundant_override ||
           conflict == PrefabVariantConflictKind::duplicate_override;
}

[[nodiscard]] std::string resolution_label_for(PrefabVariantConflictKind conflict) {
    switch (conflict) {
    case PrefabVariantConflictKind::missing_node:
        return "Remove missing-node override";
    case PrefabVariantConflictKind::redundant_override:
        return "Remove redundant override";
    case PrefabVariantConflictKind::duplicate_override:
        return "Remove duplicate override";
    case PrefabVariantConflictKind::clean:
    case PrefabVariantConflictKind::source_node_mismatch:
    case PrefabVariantConflictKind::invalid_override:
    case PrefabVariantConflictKind::component_family_replacement:
        return {};
    }
    return {};
}

[[nodiscard]] std::string resolution_diagnostic_for(PrefabVariantConflictKind conflict);

struct PrefabVariantResolutionMetadata {
    bool available{false};
    PrefabVariantConflictResolutionKind kind{PrefabVariantConflictResolutionKind::none};
    std::string id;
    std::string label;
    std::string diagnostic;
    std::uint32_t target_node_index{0};
    std::string target_node_name;
};

[[nodiscard]] std::uint32_t find_unique_source_node_index(const PrefabDefinition& prefab,
                                                          std::string_view source_node_name) noexcept {
    if (source_node_name.empty()) {
        return 0;
    }

    std::uint32_t match = 0;
    for (std::size_t index = 0; index < prefab.nodes.size(); ++index) {
        if (prefab.nodes[index].name != source_node_name) {
            continue;
        }
        if (match != 0) {
            return 0;
        }
        match = static_cast<std::uint32_t>(index + 1U);
    }
    return match;
}

[[nodiscard]] bool variant_has_override_key(const PrefabVariantDefinition& variant, std::uint32_t node_index,
                                            PrefabOverrideKind kind) {
    return std::ranges::any_of(variant.overrides, [node_index, kind](const PrefabNodeOverride& override) {
        return override.node_index == node_index && override.kind == kind;
    });
}

struct SourceNodeMatch {
    std::uint32_t index{0};
    std::size_t count{0};
};

[[nodiscard]] SourceNodeMatch find_source_node_match(const PrefabDefinition& prefab,
                                                     std::string_view source_node_name) {
    SourceNodeMatch match;
    if (source_node_name.empty()) {
        return match;
    }

    for (std::size_t index = 0; index < prefab.nodes.size(); ++index) {
        if (prefab.nodes[index].name != source_node_name) {
            continue;
        }
        ++match.count;
        if (match.count == 1U) {
            match.index = static_cast<std::uint32_t>(index + 1U);
        }
    }
    return match;
}

[[nodiscard]] std::string base_refresh_row_id(const PrefabNodeOverride& override, std::size_t override_index) {
    return "override." + std::to_string(override_index + 1U) + ".node." + std::to_string(override.node_index) + "." +
           std::string(prefab_override_kind_label(override.kind));
}

[[nodiscard]] std::string target_override_key(std::uint32_t node_index, PrefabOverrideKind kind) {
    return std::to_string(node_index) + ":" + std::string(prefab_override_kind_label(kind));
}

[[nodiscard]] PrefabVariantBaseRefreshStatus
base_refresh_status_for_row_kind(PrefabVariantBaseRefreshRowKind kind) noexcept {
    switch (kind) {
    case PrefabVariantBaseRefreshRowKind::preserve_index:
        return PrefabVariantBaseRefreshStatus::ready;
    case PrefabVariantBaseRefreshRowKind::retarget_by_source_name:
        return PrefabVariantBaseRefreshStatus::warning;
    case PrefabVariantBaseRefreshRowKind::missing_source_node_hint:
    case PrefabVariantBaseRefreshRowKind::missing_source_node:
    case PrefabVariantBaseRefreshRowKind::ambiguous_source_node:
    case PrefabVariantBaseRefreshRowKind::duplicate_target_override:
        return PrefabVariantBaseRefreshStatus::blocked;
    }
    return PrefabVariantBaseRefreshStatus::blocked;
}

[[nodiscard]] std::string base_refresh_diagnostic_for(const PrefabVariantBaseRefreshRow& row) {
    switch (row.kind) {
    case PrefabVariantBaseRefreshRowKind::preserve_index:
        return "source node remains at node " + std::to_string(row.refreshed_node_index);
    case PrefabVariantBaseRefreshRowKind::retarget_by_source_name:
        return "retarget override from node " + std::to_string(row.old_node_index) + " to refreshed node " +
               std::to_string(row.refreshed_node_index);
    case PrefabVariantBaseRefreshRowKind::missing_source_node_hint:
        return "override has no source node hint for reviewed base refresh";
    case PrefabVariantBaseRefreshRowKind::missing_source_node:
        return "source node hint is missing from the refreshed base prefab";
    case PrefabVariantBaseRefreshRowKind::ambiguous_source_node:
        return "source node hint matches multiple refreshed base prefab nodes";
    case PrefabVariantBaseRefreshRowKind::duplicate_target_override:
        return "refresh would create a duplicate target node and override kind";
    }
    return "prefab variant base refresh row is blocked";
}

void finalize_base_refresh_row(PrefabVariantBaseRefreshRow& row) {
    row.status = base_refresh_status_for_row_kind(row.kind);
    row.status_label = std::string(prefab_variant_base_refresh_status_label(row.status));
    row.kind_label = std::string(prefab_variant_base_refresh_row_kind_label(row.kind));
    row.diagnostic = base_refresh_diagnostic_for(row);
    row.blocking = row.status == PrefabVariantBaseRefreshStatus::blocked;
}

[[nodiscard]] PrefabVariantResolutionMetadata make_remove_resolution(std::string_view row_id,
                                                                     PrefabVariantConflictKind conflict) {
    PrefabVariantResolutionMetadata metadata;
    if (!has_safe_remove_resolution(conflict)) {
        return metadata;
    }

    metadata.available = true;
    metadata.kind = PrefabVariantConflictResolutionKind::remove_override;
    metadata.id = remove_resolution_id_for(row_id);
    metadata.label = resolution_label_for(conflict);
    metadata.diagnostic = resolution_diagnostic_for(conflict);
    return metadata;
}

[[nodiscard]] PrefabVariantResolutionMetadata make_resolution_metadata(const PrefabVariantDefinition& variant,
                                                                       const PrefabNodeOverride& override,
                                                                       PrefabVariantConflictKind conflict,
                                                                       std::string_view row_id) {
    if (conflict == PrefabVariantConflictKind::missing_node ||
        conflict == PrefabVariantConflictKind::source_node_mismatch) {
        const auto target_index = find_unique_source_node_index(variant.base_prefab, override.source_node_name);
        if (target_index != 0 && !variant_has_override_key(variant, target_index, override.kind)) {
            PrefabVariantResolutionMetadata metadata;
            metadata.available = true;
            metadata.kind = PrefabVariantConflictResolutionKind::retarget_override;
            metadata.target_node_index = target_index;
            metadata.target_node_name = sanitize_text(variant.base_prefab.nodes[target_index - 1U].name);
            metadata.id = retarget_resolution_id_for(row_id, target_index);
            metadata.label = "Retarget override to node " + std::to_string(target_index);
            metadata.diagnostic = "retarget the stale override to unique source node " + metadata.target_node_name;
            return metadata;
        }
    }

    if (conflict == PrefabVariantConflictKind::source_node_mismatch &&
        has_node(variant.base_prefab, override.node_index)) {
        PrefabVariantResolutionMetadata metadata;
        metadata.available = true;
        metadata.kind = PrefabVariantConflictResolutionKind::accept_current_node;
        metadata.target_node_index = override.node_index;
        metadata.target_node_name = sanitize_text(node_name_for(variant.base_prefab, override.node_index));
        metadata.id = accept_current_resolution_id_for(row_id);
        metadata.label = "Accept current node " + std::to_string(override.node_index);
        metadata.diagnostic = "update the source node hint to current node " + metadata.target_node_name;
        return metadata;
    }

    return make_remove_resolution(row_id, conflict);
}

[[nodiscard]] std::string resolution_diagnostic_for(PrefabVariantConflictKind conflict) {
    switch (conflict) {
    case PrefabVariantConflictKind::missing_node:
        return "remove the stale override because the target node is absent from the base prefab";
    case PrefabVariantConflictKind::redundant_override:
        return "remove the override because it matches the current base value";
    case PrefabVariantConflictKind::duplicate_override:
        return "remove the later duplicate override and preserve the first reviewed override";
    case PrefabVariantConflictKind::clean:
    case PrefabVariantConflictKind::source_node_mismatch:
    case PrefabVariantConflictKind::invalid_override:
    case PrefabVariantConflictKind::component_family_replacement:
        return {};
    }
    return {};
}

[[nodiscard]] const PrefabNodeTemplate* find_prefab_node(const PrefabDefinition& prefab,
                                                         std::uint32_t node_index) noexcept {
    return has_node(prefab, node_index) ? &prefab.nodes[node_index - 1U] : nullptr;
}

[[nodiscard]] std::string base_value_for(const PrefabNodeTemplate* node, PrefabOverrideKind kind) {
    if (node == nullptr) {
        return "-";
    }
    switch (kind) {
    case PrefabOverrideKind::name:
        return sanitize_text(node->name);
    case PrefabOverrideKind::transform:
        return format_transform(node->transform);
    case PrefabOverrideKind::components:
        return format_components(node->components);
    }
    return "-";
}

[[nodiscard]] std::string override_value_for(const PrefabNodeOverride& override) {
    switch (override.kind) {
    case PrefabOverrideKind::name:
        return sanitize_text(override.name);
    case PrefabOverrideKind::transform:
        return format_transform(override.transform);
    case PrefabOverrideKind::components:
        return format_components(override.components);
    }
    return "-";
}

[[nodiscard]] bool source_node_name_mismatch(const PrefabNodeTemplate& node,
                                             const PrefabNodeOverride& override) noexcept {
    return !override.source_node_name.empty() && override.source_node_name != node.name;
}

[[nodiscard]] PrefabVariantConflictKind classify_override_conflict(const PrefabNodeTemplate* node,
                                                                   const PrefabNodeOverride& override, bool duplicate) {
    if (duplicate) {
        return PrefabVariantConflictKind::duplicate_override;
    }
    if (node == nullptr) {
        return PrefabVariantConflictKind::missing_node;
    }
    if (source_node_name_mismatch(*node, override)) {
        return PrefabVariantConflictKind::source_node_mismatch;
    }

    switch (override.kind) {
    case PrefabOverrideKind::name:
        return override.name == node->name ? PrefabVariantConflictKind::redundant_override
                                           : PrefabVariantConflictKind::clean;
    case PrefabOverrideKind::transform:
        return override.transform.position == node->transform.position &&
                       override.transform.rotation_radians == node->transform.rotation_radians &&
                       override.transform.scale == node->transform.scale
                   ? PrefabVariantConflictKind::redundant_override
                   : PrefabVariantConflictKind::clean;
    case PrefabOverrideKind::components: {
        const auto base_mask = component_family_mask(node->components);
        const auto override_mask = component_family_mask(override.components);
        if (base_mask == override_mask) {
            return format_components(node->components) == format_components(override.components)
                       ? PrefabVariantConflictKind::redundant_override
                       : PrefabVariantConflictKind::clean;
        }
        if (base_mask != 0U && override_mask != 0U) {
            return PrefabVariantConflictKind::component_family_replacement;
        }
        return PrefabVariantConflictKind::clean;
    }
    }
    return PrefabVariantConflictKind::invalid_override;
}

[[nodiscard]] PrefabVariantConflictStatus status_for_conflict(PrefabVariantConflictKind conflict) noexcept {
    switch (conflict) {
    case PrefabVariantConflictKind::clean:
        return PrefabVariantConflictStatus::ready;
    case PrefabVariantConflictKind::redundant_override:
    case PrefabVariantConflictKind::component_family_replacement:
        return PrefabVariantConflictStatus::warning;
    case PrefabVariantConflictKind::missing_node:
    case PrefabVariantConflictKind::source_node_mismatch:
    case PrefabVariantConflictKind::duplicate_override:
    case PrefabVariantConflictKind::invalid_override:
        return PrefabVariantConflictStatus::blocked;
    }
    return PrefabVariantConflictStatus::blocked;
}

[[nodiscard]] std::string diagnostic_for_conflict(PrefabVariantConflictKind conflict) {
    switch (conflict) {
    case PrefabVariantConflictKind::clean:
        return "override can be composed";
    case PrefabVariantConflictKind::missing_node:
        return "override targets a node that is not present in the base prefab";
    case PrefabVariantConflictKind::source_node_mismatch:
        return "override source node hint does not match the current target node";
    case PrefabVariantConflictKind::duplicate_override:
        return "duplicate override targets the same node and override kind";
    case PrefabVariantConflictKind::invalid_override:
        return "override is invalid for the current base prefab";
    case PrefabVariantConflictKind::redundant_override:
        return "override matches the current base value";
    case PrefabVariantConflictKind::component_family_replacement:
        return "component override replaces the current base component family";
    }
    return "override is invalid for the current base prefab";
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("prefab variant conflict ui element could not be added");
    }
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

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

void append_button(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                   std::string label, bool enabled) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::button);
    desc.text = make_text(std::move(label));
    desc.enabled = enabled;
    add_or_throw(document, std::move(desc));
}

[[nodiscard]] std::string_view prefab_variant_file_dialog_mode_id(EditorPrefabVariantFileDialogMode mode) noexcept {
    switch (mode) {
    case EditorPrefabVariantFileDialogMode::open:
        return "open";
    case EditorPrefabVariantFileDialogMode::save:
        return "save";
    }
    return "open";
}

[[nodiscard]] std::string prefab_variant_file_dialog_action(EditorPrefabVariantFileDialogMode mode) {
    return mode == EditorPrefabVariantFileDialogMode::open ? "open" : "save";
}

[[nodiscard]] std::string prefab_variant_file_dialog_status(EditorPrefabVariantFileDialogMode mode,
                                                            std::string_view state) {
    return "Prefab variant " + prefab_variant_file_dialog_action(mode) + " dialog " + std::string(state);
}

[[nodiscard]] std::string selected_filter_value(int selected_filter) {
    return selected_filter < 0 ? "-" : std::to_string(selected_filter);
}

void append_prefab_variant_file_dialog_rows(EditorPrefabVariantFileDialogModel& model, std::size_t selected_count,
                                            int selected_filter) {
    model.rows = {
        EditorPrefabVariantFileDialogRow{.id = "status", .label = "Status", .value = model.status_label},
        EditorPrefabVariantFileDialogRow{
            .id = "selected_path", .label = "Selected path", .value = sanitize_text(model.selected_path)},
        EditorPrefabVariantFileDialogRow{
            .id = "selected_count", .label = "Selected count", .value = std::to_string(selected_count)},
        EditorPrefabVariantFileDialogRow{
            .id = "selected_filter", .label = "Selected filter", .value = selected_filter_value(selected_filter)},
    };
}

[[nodiscard]] mirakana::FileDialogRequest make_prefab_variant_dialog_request(mirakana::FileDialogKind kind,
                                                                             std::string title,
                                                                             std::string accept_label,
                                                                             std::string_view default_location) {
    mirakana::FileDialogRequest request;
    request.kind = kind;
    request.title = std::move(title);
    request.filters = {mirakana::FileDialogFilter{.name = "Prefab Variant", .pattern = "prefabvariant"}};
    request.default_location = std::string(default_location);
    request.allow_many = false;
    request.accept_label = std::move(accept_label);
    request.cancel_label = "Cancel";
    return request;
}

[[nodiscard]] EditorPrefabVariantFileDialogModel
make_prefab_variant_file_dialog_model(const mirakana::FileDialogResult& result,
                                      EditorPrefabVariantFileDialogMode mode) {
    EditorPrefabVariantFileDialogModel model;
    model.mode = mode;
    model.status_label = prefab_variant_file_dialog_status(mode, "idle");

    if (auto diagnostic = mirakana::validate_file_dialog_result(result); !diagnostic.empty()) {
        model.status_label = prefab_variant_file_dialog_status(mode, "blocked");
        model.diagnostics.push_back(std::move(diagnostic));
        append_prefab_variant_file_dialog_rows(model, result.paths.size(), result.selected_filter);
        return model;
    }

    switch (result.status) {
    case mirakana::FileDialogStatus::canceled:
        model.status_label = prefab_variant_file_dialog_status(mode, "canceled");
        break;
    case mirakana::FileDialogStatus::failed:
        model.status_label = prefab_variant_file_dialog_status(mode, "failed");
        model.diagnostics.push_back(result.error);
        break;
    case mirakana::FileDialogStatus::accepted:
        if (result.paths.size() != 1U) {
            model.status_label = prefab_variant_file_dialog_status(mode, "blocked");
            model.diagnostics.push_back("prefab variant " + prefab_variant_file_dialog_action(mode) +
                                        " dialog requires exactly one selected path");
            break;
        }

        model.selected_path = result.paths.front();
        if (!model.selected_path.ends_with(".prefabvariant")) {
            model.status_label = prefab_variant_file_dialog_status(mode, "blocked");
            model.diagnostics.push_back("prefab variant " + prefab_variant_file_dialog_action(mode) +
                                        " dialog selection must end with .prefabvariant");
            break;
        }

        model.accepted = true;
        model.status_label = prefab_variant_file_dialog_status(mode, "accepted");
        break;
    }

    append_prefab_variant_file_dialog_rows(model, result.paths.size(), result.selected_filter);
    return model;
}

[[nodiscard]] UndoableAction make_snapshot_action(std::string label, PrefabVariantAuthoringDocument& document,
                                                  PrefabVariantDefinition before, PrefabVariantDefinition after) {
    return UndoableAction{
        .label = std::move(label),
        .redo = [&document, after = std::move(after)]() mutable { (void)document.replace_variant(after); },
        .undo = [&document, before = std::move(before)]() mutable { (void)document.replace_variant(before); },
    };
}

} // namespace

PrefabVariantAuthoringDocument PrefabVariantAuthoringDocument::from_base_prefab(PrefabDefinition base_prefab,
                                                                                std::string variant_name,
                                                                                std::string path) {
    PrefabVariantDefinition variant;
    variant.name = std::move(variant_name);
    variant.base_prefab = std::move(base_prefab);
    return from_variant(std::move(variant), std::move(path));
}

PrefabVariantAuthoringDocument PrefabVariantAuthoringDocument::from_variant(PrefabVariantDefinition variant,
                                                                            std::string path) {
    if (!repairable_for_prefab_variant_authoring(variant)) {
        throw std::invalid_argument(
            "prefab variant authoring document requires a valid or review-cleanup-repairable variant definition");
    }

    PrefabVariantAuthoringDocument document;
    document.variant_ = std::move(variant);
    document.path_ = std::move(path);
    document.saved_variant_text_ = serialize_prefab_variant_if_valid(document.variant_);
    return document;
}

const PrefabVariantDefinition& PrefabVariantAuthoringDocument::variant() const noexcept {
    return variant_;
}

const PrefabDefinition& PrefabVariantAuthoringDocument::base_prefab() const noexcept {
    return variant_.base_prefab;
}

std::string_view PrefabVariantAuthoringDocument::path() const noexcept {
    return path_;
}

bool PrefabVariantAuthoringDocument::dirty() const {
    try {
        return serialize_prefab_variant_definition(variant_) != saved_variant_text_;
    } catch (const std::exception&) {
        return true;
    }
}

PrefabDefinition PrefabVariantAuthoringDocument::composed_prefab() const {
    const auto result = compose_prefab_variant(variant_);
    if (!result.success) {
        throw std::invalid_argument("prefab variant authoring document has validation diagnostics");
    }
    return result.prefab;
}

PrefabVariantAuthoringModel PrefabVariantAuthoringDocument::model() const {
    return make_model(*this, nullptr);
}

PrefabVariantAuthoringModel PrefabVariantAuthoringDocument::model(const AssetRegistry& registry) const {
    return make_model(*this, &registry);
}

bool PrefabVariantAuthoringDocument::replace_variant(PrefabVariantDefinition variant) {
    if (!repairable_for_prefab_variant_authoring(variant)) {
        return false;
    }

    variant_ = std::move(variant);
    return true;
}

bool PrefabVariantAuthoringDocument::set_name_override(std::uint32_t node_index, std::string name) {
    if (!has_node(variant_.base_prefab, node_index) || !valid_text_field(name)) {
        return false;
    }

    PrefabNodeOverride override;
    override.node_index = node_index;
    override.kind = PrefabOverrideKind::name;
    override.name = std::move(name);
    override.source_node_name = node_name_for(variant_.base_prefab, node_index);
    return upsert_override(variant_, std::move(override));
}

bool PrefabVariantAuthoringDocument::set_transform_override(std::uint32_t node_index, Transform3D transform) {
    if (!has_node(variant_.base_prefab, node_index) || !valid_transform(transform)) {
        return false;
    }

    PrefabNodeOverride override;
    override.node_index = node_index;
    override.kind = PrefabOverrideKind::transform;
    override.transform = transform;
    override.source_node_name = node_name_for(variant_.base_prefab, node_index);
    return upsert_override(variant_, std::move(override));
}

bool PrefabVariantAuthoringDocument::set_component_override(std::uint32_t node_index, SceneNodeComponents components) {
    if (!has_node(variant_.base_prefab, node_index) || !is_valid_scene_node_components(components)) {
        return false;
    }

    PrefabNodeOverride override;
    override.node_index = node_index;
    override.kind = PrefabOverrideKind::components;
    override.components = components;
    override.source_node_name = node_name_for(variant_.base_prefab, node_index);
    return upsert_override(variant_, std::move(override));
}

void PrefabVariantAuthoringDocument::mark_saved() {
    saved_variant_text_ = serialize_prefab_variant_if_valid(variant_);
}

std::vector<PrefabVariantAuthoringDiagnostic>
validate_prefab_variant_authoring_document(const PrefabVariantAuthoringDocument& document) {
    std::vector<PrefabVariantAuthoringDiagnostic> diagnostics;
    for (const auto& diagnostic : validate_prefab_variant_definition(document.variant())) {
        diagnostics.push_back(PrefabVariantAuthoringDiagnostic{
            .kind = PrefabVariantAuthoringDiagnosticKind::invalid_variant,
            .node_index = diagnostic.node_index,
            .override_kind = diagnostic.override_kind,
            .asset = AssetId{},
            .field = std::string(prefab_variant_diagnostic_kind_label(diagnostic.kind)),
            .diagnostic = diagnostic.message,
        });
    }
    sort_diagnostics(diagnostics);
    return diagnostics;
}

std::vector<PrefabVariantAuthoringDiagnostic>
validate_prefab_variant_authoring_document(const PrefabVariantAuthoringDocument& document,
                                           const AssetRegistry& registry) {
    auto diagnostics = validate_prefab_variant_authoring_document(document);
    for (const auto& override : document.variant().overrides) {
        if (override.kind == PrefabOverrideKind::components) {
            add_component_reference_diagnostics(diagnostics, override, registry);
        }
    }
    sort_diagnostics(diagnostics);
    return diagnostics;
}

std::string_view prefab_variant_conflict_status_label(PrefabVariantConflictStatus status) noexcept {
    switch (status) {
    case PrefabVariantConflictStatus::ready:
        return "ready";
    case PrefabVariantConflictStatus::warning:
        return "warning";
    case PrefabVariantConflictStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

std::string_view prefab_variant_conflict_kind_label(PrefabVariantConflictKind kind) noexcept {
    switch (kind) {
    case PrefabVariantConflictKind::clean:
        return "clean";
    case PrefabVariantConflictKind::missing_node:
        return "missing_node";
    case PrefabVariantConflictKind::source_node_mismatch:
        return "source_node_mismatch";
    case PrefabVariantConflictKind::duplicate_override:
        return "duplicate_override";
    case PrefabVariantConflictKind::invalid_override:
        return "invalid_override";
    case PrefabVariantConflictKind::redundant_override:
        return "redundant_override";
    case PrefabVariantConflictKind::component_family_replacement:
        return "component_family_replacement";
    }
    return "invalid_override";
}

std::string_view prefab_variant_conflict_resolution_kind_label(PrefabVariantConflictResolutionKind kind) noexcept {
    switch (kind) {
    case PrefabVariantConflictResolutionKind::none:
        return "none";
    case PrefabVariantConflictResolutionKind::remove_override:
        return "remove_override";
    case PrefabVariantConflictResolutionKind::retarget_override:
        return "retarget_override";
    case PrefabVariantConflictResolutionKind::accept_current_node:
        return "accept_current_node";
    }
    return "none";
}

std::string_view prefab_variant_base_refresh_status_label(PrefabVariantBaseRefreshStatus status) noexcept {
    switch (status) {
    case PrefabVariantBaseRefreshStatus::ready:
        return "ready";
    case PrefabVariantBaseRefreshStatus::warning:
        return "warning";
    case PrefabVariantBaseRefreshStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

std::string_view prefab_variant_base_refresh_row_kind_label(PrefabVariantBaseRefreshRowKind kind) noexcept {
    switch (kind) {
    case PrefabVariantBaseRefreshRowKind::preserve_index:
        return "preserve_index";
    case PrefabVariantBaseRefreshRowKind::retarget_by_source_name:
        return "retarget_by_source_name";
    case PrefabVariantBaseRefreshRowKind::missing_source_node_hint:
        return "missing_source_node_hint";
    case PrefabVariantBaseRefreshRowKind::missing_source_node:
        return "missing_source_node";
    case PrefabVariantBaseRefreshRowKind::ambiguous_source_node:
        return "ambiguous_source_node";
    case PrefabVariantBaseRefreshRowKind::duplicate_target_override:
        return "duplicate_target_override";
    }
    return "duplicate_target_override";
}

PrefabVariantConflictBatchResolutionPlan
make_prefab_variant_conflict_batch_resolution_plan(const PrefabVariantConflictReviewModel& model) {
    PrefabVariantConflictBatchResolutionPlan plan;
    for (const auto& row : model.rows) {
        if (!row.resolution_available) {
            continue;
        }

        plan.resolution_ids.push_back(row.resolution_id);
        ++plan.resolution_count;
        if (row.blocking) {
            ++plan.blocking_resolution_count;
        } else {
            ++plan.warning_resolution_count;
        }
    }

    plan.available = plan.resolution_count > 0U;
    plan.mutates = plan.available;
    plan.executes = false;
    if (plan.available) {
        plan.diagnostic = "apply " + std::to_string(plan.resolution_count) +
                          " reviewed prefab variant resolution rows as one undoable action";
    } else {
        plan.diagnostic = "no reviewed prefab variant resolutions are available";
    }
    return plan;
}

PrefabVariantConflictReviewModel make_prefab_variant_conflict_review_model(const PrefabVariantDefinition& variant) {
    PrefabVariantConflictReviewModel model;
    model.variant_name = sanitize_text(variant.name);
    model.base_prefab_name = sanitize_text(variant.base_prefab.name);
    model.can_compose = compose_prefab_variant(variant).success;

    for (const auto& diagnostic : validate_prefab_variant_definition(variant)) {
        model.diagnostics.push_back(std::string(prefab_variant_diagnostic_kind_label(diagnostic.kind)) + ": " +
                                    diagnostic.message);
    }

    std::unordered_map<std::string, std::size_t> override_occurrences;
    model.rows.reserve(variant.overrides.size());
    for (std::size_t override_index = 0; override_index < variant.overrides.size(); ++override_index) {
        const auto& override = variant.overrides[override_index];
        const auto key = override_key(override);
        const auto occurrence = ++override_occurrences[key];
        const auto duplicate = occurrence > 1U;
        const auto* node = find_prefab_node(variant.base_prefab, override.node_index);
        const auto conflict = classify_override_conflict(node, override, duplicate);
        const auto status = status_for_conflict(conflict);
        const auto blocking = status == PrefabVariantConflictStatus::blocked;
        if (blocking) {
            ++model.blocking_count;
        } else if (status == PrefabVariantConflictStatus::warning) {
            ++model.warning_count;
        }

        const auto row_id = override_row_id(override, occurrence);
        const auto resolution = make_resolution_metadata(variant, override, conflict, row_id);
        model.rows.push_back(PrefabVariantConflictRow{
            .id = row_id,
            .override_index = override_index,
            .node_index = override.node_index,
            .override_kind = override.kind,
            .status = status,
            .conflict = conflict,
            .node_name = node == nullptr ? sanitize_text(override.source_node_name) : sanitize_text(node->name),
            .override_kind_label = std::string(prefab_override_kind_label(override.kind)),
            .status_label = std::string(prefab_variant_conflict_status_label(status)),
            .conflict_label = std::string(prefab_variant_conflict_kind_label(conflict)),
            .base_value = base_value_for(node, override.kind),
            .override_value = override_value_for(override),
            .diagnostic = diagnostic_for_conflict(conflict),
            .resolution_available = resolution.available,
            .resolution_id = resolution.id,
            .resolution_label = resolution.label,
            .resolution_diagnostic = resolution.diagnostic,
            .resolution_kind = resolution.kind,
            .resolution_target_node_index = resolution.target_node_index,
            .resolution_target_node_name = resolution.target_node_name,
            .blocking = blocking,
        });
    }

    model.has_blocking_conflicts = model.blocking_count > 0U || (!model.diagnostics.empty() && !model.can_compose);
    if (model.has_blocking_conflicts) {
        model.can_compose = false;
    }
    if (model.has_blocking_conflicts) {
        model.status = PrefabVariantConflictStatus::blocked;
    } else if (model.warning_count > 0U) {
        model.status = PrefabVariantConflictStatus::warning;
    } else {
        model.status = PrefabVariantConflictStatus::ready;
    }
    model.status_label = std::string(prefab_variant_conflict_status_label(model.status));
    model.batch_resolution = make_prefab_variant_conflict_batch_resolution_plan(model);
    return model;
}

PrefabVariantConflictReviewModel
make_prefab_variant_conflict_review_model(const PrefabVariantAuthoringDocument& document) {
    return make_prefab_variant_conflict_review_model(document.variant());
}

mirakana::ui::UiDocument make_prefab_variant_conflict_review_ui_model(const PrefabVariantConflictReviewModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("prefab_variant_conflicts", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"prefab_variant_conflicts"};

    append_label(document, root, "prefab_variant_conflicts.status", model.status_label);
    append_label(document, root, "prefab_variant_conflicts.variant", model.variant_name);
    append_label(document, root, "prefab_variant_conflicts.base", model.base_prefab_name);
    append_label(document, root, "prefab_variant_conflicts.summary.rows", std::to_string(model.rows.size()));
    append_label(document, root, "prefab_variant_conflicts.summary.blocking", std::to_string(model.blocking_count));
    append_label(document, root, "prefab_variant_conflicts.summary.warnings", std::to_string(model.warning_count));

    add_or_throw(document,
                 make_child("prefab_variant_conflicts.batch_resolution", root, mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId batch_root{"prefab_variant_conflicts.batch_resolution"};
    append_button(document, batch_root, "prefab_variant_conflicts.batch_resolution.apply_all",
                  model.batch_resolution.label, model.batch_resolution.available);
    append_label(document, batch_root, "prefab_variant_conflicts.batch_resolution.id", model.batch_resolution.id);
    append_label(document, batch_root, "prefab_variant_conflicts.batch_resolution.available",
                 model.batch_resolution.available ? "true" : "false");
    append_label(document, batch_root, "prefab_variant_conflicts.batch_resolution.count",
                 std::to_string(model.batch_resolution.resolution_count));
    append_label(document, batch_root, "prefab_variant_conflicts.batch_resolution.blocking",
                 std::to_string(model.batch_resolution.blocking_resolution_count));
    append_label(document, batch_root, "prefab_variant_conflicts.batch_resolution.warnings",
                 std::to_string(model.batch_resolution.warning_resolution_count));
    append_label(document, batch_root, "prefab_variant_conflicts.batch_resolution.mutates",
                 model.batch_resolution.mutates ? "true" : "false");
    append_label(document, batch_root, "prefab_variant_conflicts.batch_resolution.executes",
                 model.batch_resolution.executes ? "true" : "false");
    append_label(document, batch_root, "prefab_variant_conflicts.batch_resolution.diagnostic",
                 model.batch_resolution.diagnostic);

    add_or_throw(document, make_child("prefab_variant_conflicts.rows", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId rows_root{"prefab_variant_conflicts.rows"};
    for (const auto& row : model.rows) {
        const auto prefix = "prefab_variant_conflicts.rows." + sanitize_element_id(row.id);
        mirakana::ui::ElementDesc item = make_child(prefix, rows_root, mirakana::ui::SemanticRole::list_item);
        item.enabled = !row.blocking;
        item.text = make_text(row.override_kind_label + " node " + std::to_string(row.node_index));
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".status", row.status_label);
        append_label(document, item_root, prefix + ".conflict", row.conflict_label);
        append_label(document, item_root, prefix + ".base", row.base_value);
        append_label(document, item_root, prefix + ".override", row.override_value);
        append_label(document, item_root, prefix + ".diagnostic", row.diagnostic);
        append_label(document, item_root, prefix + ".resolution",
                     row.resolution_available ? row.resolution_label : "-");
        append_label(document, item_root, prefix + ".resolution_diagnostic",
                     row.resolution_diagnostic.empty() ? "-" : row.resolution_diagnostic);
        append_label(document, item_root, prefix + ".resolution_kind",
                     std::string(prefab_variant_conflict_resolution_kind_label(row.resolution_kind)));
        append_label(document, item_root, prefix + ".resolution_target_node",
                     row.resolution_target_node_index == 0U
                         ? "-"
                         : std::to_string(row.resolution_target_node_index) + " " + row.resolution_target_node_name);
    }

    add_or_throw(document, make_child("prefab_variant_conflicts.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{"prefab_variant_conflicts.diagnostics"};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, "prefab_variant_conflicts.diagnostics." + std::to_string(index),
                     model.diagnostics[index]);
    }

    return document;
}

PrefabVariantBaseRefreshPlan plan_prefab_variant_base_refresh(const PrefabVariantDefinition& variant,
                                                              const PrefabDefinition& refreshed_base) {
    PrefabVariantBaseRefreshPlan plan;
    plan.variant_name = sanitize_text(variant.name);
    plan.current_base_prefab_name = sanitize_text(variant.base_prefab.name);
    plan.refreshed_base_prefab_name = sanitize_text(refreshed_base.name);
    plan.executes = false;

    const auto finalize_plan = [&plan]() {
        plan.row_count = plan.rows.size();
        if (plan.blocking_count > 0U || !plan.diagnostics.empty()) {
            plan.status = PrefabVariantBaseRefreshStatus::blocked;
        } else if (plan.warning_count > 0U) {
            plan.status = PrefabVariantBaseRefreshStatus::warning;
        } else {
            plan.status = PrefabVariantBaseRefreshStatus::ready;
        }
        plan.status_label = std::string(prefab_variant_base_refresh_status_label(plan.status));
        plan.can_apply = plan.status != PrefabVariantBaseRefreshStatus::blocked;
        plan.mutates = plan.can_apply;
    };

    if (!is_valid_prefab_definition(refreshed_base)) {
        plan.diagnostics.emplace_back("refreshed base prefab is invalid");
        finalize_plan();
        return plan;
    }

    for (const auto& diagnostic : validate_prefab_variant_definition(variant)) {
        plan.diagnostics.push_back("current variant " +
                                   std::string(prefab_variant_diagnostic_kind_label(diagnostic.kind)) + ": " +
                                   diagnostic.message);
    }
    if (!plan.diagnostics.empty()) {
        finalize_plan();
        return plan;
    }

    const auto conflict_model = make_prefab_variant_conflict_review_model(variant);
    if (conflict_model.has_blocking_conflicts) {
        plan.diagnostics.emplace_back("current variant has unresolved blocking prefab variant conflicts");
        finalize_plan();
        return plan;
    }

    std::unordered_map<std::string, std::size_t> claimed_targets;
    plan.rows.reserve(variant.overrides.size());
    for (std::size_t override_index = 0; override_index < variant.overrides.size(); ++override_index) {
        const auto& override = variant.overrides[override_index];
        PrefabVariantBaseRefreshRow row;
        row.id = base_refresh_row_id(override, override_index);
        row.override_index = override_index;
        row.old_node_index = override.node_index;
        row.old_node_name = sanitize_text(node_name_for(variant.base_prefab, override.node_index));
        row.source_node_name = sanitize_text(override.source_node_name);
        row.override_kind = override.kind;
        row.override_kind_label = std::string(prefab_override_kind_label(override.kind));

        if (override.source_node_name.empty()) {
            row.kind = PrefabVariantBaseRefreshRowKind::missing_source_node_hint;
            finalize_base_refresh_row(row);
            ++plan.blocking_count;
            plan.rows.push_back(std::move(row));
            continue;
        }

        const auto match = find_source_node_match(refreshed_base, override.source_node_name);
        if (match.count == 0U) {
            row.kind = PrefabVariantBaseRefreshRowKind::missing_source_node;
            finalize_base_refresh_row(row);
            ++plan.blocking_count;
            plan.rows.push_back(std::move(row));
            continue;
        }
        if (match.count > 1U) {
            row.kind = PrefabVariantBaseRefreshRowKind::ambiguous_source_node;
            finalize_base_refresh_row(row);
            ++plan.blocking_count;
            plan.rows.push_back(std::move(row));
            continue;
        }

        row.refreshed_node_index = match.index;
        row.refreshed_node_name = sanitize_text(node_name_for(refreshed_base, match.index));
        const auto target_key = target_override_key(match.index, override.kind);
        if (claimed_targets.contains(target_key)) {
            row.kind = PrefabVariantBaseRefreshRowKind::duplicate_target_override;
            finalize_base_refresh_row(row);
            ++plan.blocking_count;
            plan.rows.push_back(std::move(row));
            continue;
        }
        claimed_targets.emplace(target_key, override_index);

        if (match.index == override.node_index) {
            row.kind = PrefabVariantBaseRefreshRowKind::preserve_index;
        } else {
            row.kind = PrefabVariantBaseRefreshRowKind::retarget_by_source_name;
            ++plan.retarget_count;
        }
        finalize_base_refresh_row(row);
        if (row.status == PrefabVariantBaseRefreshStatus::warning) {
            ++plan.warning_count;
        }
        plan.rows.push_back(std::move(row));
    }

    finalize_plan();
    return plan;
}

PrefabVariantBaseRefreshPlan plan_prefab_variant_base_refresh(const PrefabVariantAuthoringDocument& document,
                                                              const PrefabDefinition& refreshed_base) {
    return plan_prefab_variant_base_refresh(document.variant(), refreshed_base);
}

mirakana::ui::UiDocument make_prefab_variant_base_refresh_ui_model(const PrefabVariantBaseRefreshPlan& plan) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("prefab_variant_base_refresh", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"prefab_variant_base_refresh"};

    append_button(document, root, "prefab_variant_base_refresh.apply", "Apply Base Refresh", plan.can_apply);
    append_label(document, root, "prefab_variant_base_refresh.status", plan.status_label);
    append_label(document, root, "prefab_variant_base_refresh.variant", plan.variant_name);
    append_label(document, root, "prefab_variant_base_refresh.current_base", plan.current_base_prefab_name);
    append_label(document, root, "prefab_variant_base_refresh.refreshed_base", plan.refreshed_base_prefab_name);
    append_label(document, root, "prefab_variant_base_refresh.summary.rows", std::to_string(plan.row_count));
    append_label(document, root, "prefab_variant_base_refresh.summary.retargets", std::to_string(plan.retarget_count));
    append_label(document, root, "prefab_variant_base_refresh.summary.blocking", std::to_string(plan.blocking_count));
    append_label(document, root, "prefab_variant_base_refresh.summary.warnings", std::to_string(plan.warning_count));
    append_label(document, root, "prefab_variant_base_refresh.mutates", plan.mutates ? "true" : "false");
    append_label(document, root, "prefab_variant_base_refresh.executes", plan.executes ? "true" : "false");

    add_or_throw(document, make_child("prefab_variant_base_refresh.rows", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId rows_root{"prefab_variant_base_refresh.rows"};
    for (const auto& row : plan.rows) {
        const auto prefix = "prefab_variant_base_refresh.rows." + sanitize_element_id(row.id);
        mirakana::ui::ElementDesc item = make_child(prefix, rows_root, mirakana::ui::SemanticRole::list_item);
        item.enabled = !row.blocking;
        item.text = make_text(row.override_kind_label + " node " + std::to_string(row.old_node_index));
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".status", row.status_label);
        append_label(document, item_root, prefix + ".kind", row.kind_label);
        append_label(document, item_root, prefix + ".old_node",
                     std::to_string(row.old_node_index) + " " + row.old_node_name);
        append_label(document, item_root, prefix + ".source_node", row.source_node_name);
        append_label(document, item_root, prefix + ".target_node",
                     row.refreshed_node_index == 0U
                         ? "-"
                         : std::to_string(row.refreshed_node_index) + " " + row.refreshed_node_name);
        append_label(document, item_root, prefix + ".diagnostic", row.diagnostic);
        append_label(document, item_root, prefix + ".blocking", row.blocking ? "true" : "false");
    }

    add_or_throw(document,
                 make_child("prefab_variant_base_refresh.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{"prefab_variant_base_refresh.diagnostics"};
    for (std::size_t index = 0; index < plan.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, "prefab_variant_base_refresh.diagnostics." + std::to_string(index),
                     plan.diagnostics[index]);
    }

    return document;
}

mirakana::FileDialogRequest make_prefab_variant_open_dialog_request(std::string_view default_location) {
    return make_prefab_variant_dialog_request(mirakana::FileDialogKind::open_file, "Open Prefab Variant", "Open",
                                              default_location);
}

mirakana::FileDialogRequest make_prefab_variant_save_dialog_request(std::string_view default_location) {
    return make_prefab_variant_dialog_request(mirakana::FileDialogKind::save_file, "Save Prefab Variant", "Save",
                                              default_location);
}

EditorPrefabVariantFileDialogModel make_prefab_variant_open_dialog_model(const mirakana::FileDialogResult& result) {
    return make_prefab_variant_file_dialog_model(result, EditorPrefabVariantFileDialogMode::open);
}

EditorPrefabVariantFileDialogModel make_prefab_variant_save_dialog_model(const mirakana::FileDialogResult& result) {
    return make_prefab_variant_file_dialog_model(result, EditorPrefabVariantFileDialogMode::save);
}

mirakana::ui::UiDocument make_prefab_variant_file_dialog_ui_model(const EditorPrefabVariantFileDialogModel& model) {
    mirakana::ui::UiDocument document;
    const auto mode_id = std::string(prefab_variant_file_dialog_mode_id(model.mode));
    const std::string root_prefix = "prefab_variant_file_dialog." + mode_id;
    add_or_throw(document, make_root(root_prefix, mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{root_prefix};

    for (const auto& row : model.rows) {
        const std::string row_prefix = root_prefix + "." + sanitize_element_id(row.id);
        add_or_throw(document, make_child(row_prefix, root, mirakana::ui::SemanticRole::list_item));
        const mirakana::ui::ElementId row_root{row_prefix};
        append_label(document, row_root, row_prefix + ".label", row.label);
        append_label(document, row_root, row_prefix + ".value", row.value);
    }

    const std::string diagnostics_prefix = root_prefix + ".diagnostics";
    add_or_throw(document, make_child(diagnostics_prefix, root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{diagnostics_prefix};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, diagnostics_prefix + "." + std::to_string(index),
                     model.diagnostics[index]);
    }

    return document;
}

PrefabVariantConflictResolutionResult resolve_prefab_variant_conflict(const PrefabVariantDefinition& variant,
                                                                      std::string_view resolution_id) {
    PrefabVariantConflictResolutionResult result;
    result.variant = variant;

    if (resolution_id.empty()) {
        result.diagnostic = "prefab variant conflict resolution id is empty";
        return result;
    }

    const auto model = make_prefab_variant_conflict_review_model(variant);
    const auto row = std::ranges::find_if(model.rows, [resolution_id](const PrefabVariantConflictRow& candidate) {
        return candidate.resolution_available && candidate.resolution_id == resolution_id;
    });
    if (row == model.rows.end()) {
        result.diagnostic = "prefab variant conflict resolution is unavailable";
        return result;
    }
    if (row->override_index >= variant.overrides.size()) {
        result.diagnostic = "prefab variant conflict resolution row is out of range";
        return result;
    }

    switch (row->resolution_kind) {
    case PrefabVariantConflictResolutionKind::remove_override:
        result.variant.overrides.erase(result.variant.overrides.begin() +
                                       static_cast<std::ptrdiff_t>(row->override_index));
        break;
    case PrefabVariantConflictResolutionKind::retarget_override: {
        if (!has_node(result.variant.base_prefab, row->resolution_target_node_index)) {
            result.diagnostic = "prefab variant conflict retarget node is out of range";
            return result;
        }
        auto& override = result.variant.overrides[row->override_index];
        override.node_index = row->resolution_target_node_index;
        override.source_node_name = node_name_for(result.variant.base_prefab, row->resolution_target_node_index);
        break;
    }
    case PrefabVariantConflictResolutionKind::accept_current_node: {
        if (!has_node(result.variant.base_prefab, row->node_index)) {
            result.diagnostic = "prefab variant conflict accept-current node is out of range";
            return result;
        }
        auto& override = result.variant.overrides[row->override_index];
        override.source_node_name = node_name_for(result.variant.base_prefab, row->node_index);
        break;
    }
    case PrefabVariantConflictResolutionKind::none:
        result.diagnostic = "prefab variant conflict resolution is unavailable";
        return result;
    }
    result.applied = true;

    const auto diagnostics = validate_prefab_variant_definition(result.variant);
    result.valid_after_apply = diagnostics.empty();
    result.diagnostic = result.valid_after_apply
                            ? "prefab variant conflict resolution applied"
                            : "prefab variant conflict resolution applied but validation diagnostics remain";
    return result;
}

PrefabVariantConflictBatchResolutionResult resolve_prefab_variant_conflicts(const PrefabVariantDefinition& variant) {
    PrefabVariantConflictBatchResolutionResult result;
    result.variant = variant;

    const auto max_iterations = (result.variant.overrides.size() * 2U) + 1U;
    while (true) {
        const auto model = make_prefab_variant_conflict_review_model(result.variant);
        const auto row = std::ranges::find_if(
            model.rows, [](const PrefabVariantConflictRow& candidate) { return candidate.resolution_available; });
        if (row == model.rows.end()) {
            result.remaining_available_count = model.batch_resolution.resolution_count;
            result.remaining_blocking_count = model.blocking_count;
            break;
        }

        if (result.applied_count >= max_iterations) {
            result.diagnostic = "prefab variant batch resolution stopped before exhausting reviewed rows";
            return result;
        }

        const auto resolution_id = row->resolution_id;
        auto step = resolve_prefab_variant_conflict(result.variant, resolution_id);
        if (!step.applied) {
            result.diagnostic = "prefab variant batch resolution stopped: " + step.diagnostic;
            return result;
        }

        result.applied = true;
        ++result.applied_count;
        result.applied_resolution_ids.push_back(resolution_id);
        result.variant = std::move(step.variant);
    }

    const auto diagnostics = validate_prefab_variant_definition(result.variant);
    result.valid_after_apply = diagnostics.empty();
    if (!result.applied) {
        result.diagnostic = "no reviewed prefab variant resolutions were available";
    } else if (result.valid_after_apply) {
        result.diagnostic =
            "prefab variant batch resolution applied " + std::to_string(result.applied_count) + " reviewed rows";
    } else {
        result.diagnostic = "prefab variant batch resolution applied " + std::to_string(result.applied_count) +
                            " reviewed rows but validation diagnostics remain";
    }
    return result;
}

UndoableAction make_prefab_variant_conflict_resolution_action(PrefabVariantAuthoringDocument& document,
                                                              std::string_view resolution_id) {
    auto before = document.variant();
    auto result = resolve_prefab_variant_conflict(before, resolution_id);
    if (!result.applied || !result.valid_after_apply) {
        return {};
    }

    return make_snapshot_action("Resolve Prefab Variant Conflict", document, std::move(before),
                                std::move(result.variant));
}

UndoableAction make_prefab_variant_conflict_batch_resolution_action(PrefabVariantAuthoringDocument& document) {
    auto before = document.variant();
    auto result = resolve_prefab_variant_conflicts(before);
    if (!result.applied || !result.valid_after_apply) {
        return {};
    }

    return make_snapshot_action("Resolve Prefab Variant Conflicts", document, std::move(before),
                                std::move(result.variant));
}

PrefabVariantBaseRefreshResult apply_prefab_variant_base_refresh(const PrefabVariantDefinition& variant,
                                                                 const PrefabDefinition& refreshed_base) {
    PrefabVariantBaseRefreshResult result;
    result.variant = variant;

    const auto plan = plan_prefab_variant_base_refresh(variant, refreshed_base);
    if (!plan.can_apply) {
        result.diagnostic = "prefab variant base refresh is blocked";
        return result;
    }

    auto next = variant;
    next.base_prefab = refreshed_base;
    for (const auto& row : plan.rows) {
        if (row.override_index >= next.overrides.size() || !has_node(next.base_prefab, row.refreshed_node_index)) {
            result.diagnostic = "prefab variant base refresh row is out of range";
            return result;
        }

        auto& override = next.overrides[row.override_index];
        override.node_index = row.refreshed_node_index;
        override.source_node_name = node_name_for(next.base_prefab, row.refreshed_node_index);
        if (row.kind == PrefabVariantBaseRefreshRowKind::retarget_by_source_name) {
            ++result.retargeted_count;
        }
    }

    result.variant = std::move(next);
    result.applied = true;
    const auto diagnostics = validate_prefab_variant_definition(result.variant);
    result.valid_after_apply = diagnostics.empty();
    result.diagnostic = result.valid_after_apply
                            ? "prefab variant base refresh applied"
                            : "prefab variant base refresh applied but validation diagnostics remain";
    return result;
}

UndoableAction make_prefab_variant_base_refresh_action(PrefabVariantAuthoringDocument& document,
                                                       const PrefabDefinition& refreshed_base) {
    auto before = document.variant();
    auto result = apply_prefab_variant_base_refresh(before, refreshed_base);
    if (!result.applied || !result.valid_after_apply) {
        return {};
    }

    return make_snapshot_action("Refresh Prefab Variant Base", document, std::move(before), std::move(result.variant));
}

UndoableAction make_prefab_variant_name_override_action(PrefabVariantAuthoringDocument& document,
                                                        std::uint32_t node_index, std::string name) {
    if (!has_node(document.base_prefab(), node_index) || !valid_text_field(name)) {
        return {};
    }

    auto before = document.variant();
    auto after = before;
    PrefabNodeOverride override;
    override.node_index = node_index;
    override.kind = PrefabOverrideKind::name;
    override.name = std::move(name);
    override.source_node_name = node_name_for(document.base_prefab(), node_index);
    if (!upsert_override(after, std::move(override))) {
        return {};
    }
    return make_snapshot_action("Edit Prefab Variant Name Override", document, std::move(before), std::move(after));
}

UndoableAction make_prefab_variant_transform_override_action(PrefabVariantAuthoringDocument& document,
                                                             std::uint32_t node_index, Transform3D transform) {
    if (!has_node(document.base_prefab(), node_index)) {
        return {};
    }

    auto before = document.variant();
    auto after = before;
    PrefabNodeOverride override;
    override.node_index = node_index;
    override.kind = PrefabOverrideKind::transform;
    override.transform = transform;
    override.source_node_name = node_name_for(document.base_prefab(), node_index);
    if (!upsert_override(after, std::move(override))) {
        return {};
    }
    return make_snapshot_action("Edit Prefab Variant Transform Override", document, std::move(before),
                                std::move(after));
}

UndoableAction make_prefab_variant_component_override_action(PrefabVariantAuthoringDocument& document,
                                                             std::uint32_t node_index, SceneNodeComponents components) {
    if (!has_node(document.base_prefab(), node_index) || !is_valid_scene_node_components(components)) {
        return {};
    }

    auto before = document.variant();
    auto after = before;
    PrefabNodeOverride override;
    override.node_index = node_index;
    override.kind = PrefabOverrideKind::components;
    override.components = components;
    override.source_node_name = node_name_for(document.base_prefab(), node_index);
    if (!upsert_override(after, std::move(override))) {
        return {};
    }
    return make_snapshot_action("Edit Prefab Variant Component Override", document, std::move(before),
                                std::move(after));
}

void save_prefab_variant_authoring_document(ITextStore& store, std::string_view path,
                                            PrefabVariantAuthoringDocument& document) {
    const auto diagnostics = validate_prefab_variant_authoring_document(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("prefab variant authoring document has validation diagnostics");
    }
    store.write_text(path, serialize_prefab_variant_definition(document.variant()));
    document = PrefabVariantAuthoringDocument::from_variant(document.variant(), std::string(path));
}

void save_prefab_variant_authoring_document(ITextStore& store, std::string_view path,
                                            PrefabVariantAuthoringDocument& document, const AssetRegistry& registry) {
    const auto diagnostics = validate_prefab_variant_authoring_document(document, registry);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("prefab variant authoring document has validation diagnostics");
    }
    store.write_text(path, serialize_prefab_variant_definition(document.variant()));
    document = PrefabVariantAuthoringDocument::from_variant(document.variant(), std::string(path));
}

PrefabVariantAuthoringDocument load_prefab_variant_authoring_document(ITextStore& store, std::string_view path) {
    return PrefabVariantAuthoringDocument::from_variant(
        deserialize_prefab_variant_definition_for_review(store.read_text(path)), std::string(path));
}

} // namespace mirakana::editor
