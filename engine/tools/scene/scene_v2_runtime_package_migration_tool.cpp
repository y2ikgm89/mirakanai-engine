// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/scene_v2_runtime_package_migration_tool.hpp"

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/source_asset_registry.hpp"
#include "mirakana/math/vec.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/scene/components.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene/schema_v2.hpp"
#include "mirakana/tools/scene_tool.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

struct ProjectedScene {
    Scene scene{"Scene"};
    std::vector<AssetId> mesh_dependencies;
    std::vector<AssetId> material_dependencies;
    std::vector<AssetId> sprite_dependencies;
    std::vector<AssetIdentityPlacementRequestV2> placement_requests;
    std::vector<SourceAssetDependencyRowV1> dependency_rows;
    std::vector<SceneV2RuntimePackageMigrationDiagnostic> diagnostics;
};

struct PreparedSceneV2RuntimePackageMigration {
    Scene scene{"Scene"};
    AssetId scene_asset;
    std::vector<AssetId> mesh_dependencies;
    std::vector<AssetId> material_dependencies;
    std::vector<AssetId> sprite_dependencies;
    std::vector<AssetIdentityPlacementRowV2> placement_rows;
    std::vector<SourceAssetDependencyRowV1> dependency_rows;
};

struct ResolvedAssetReference {
    AssetKeyV2 key;
    AssetId id;
};

[[nodiscard]] std::vector<std::string> default_validation_recipes() {
    return {"agent-contract", "public-api-boundary", "default"};
}

[[nodiscard]] std::vector<std::string> default_unsupported_gap_ids() {
    return {
        "scene-component-prefab-schema-v2",         "runtime-resource-v2",   "renderer-rhi-resource-foundation",
        "production-ui-importer-platform-adapters", "editor-productization", "3d-playable-vertical-slice",
    };
}

[[nodiscard]] bool has_control_character(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char character) {
        const auto byte = static_cast<unsigned char>(character);
        return byte < 0x20U || byte == 0x7FU;
    });
}

[[nodiscard]] bool has_ascii_space(std::string_view value) noexcept {
    return std::ranges::any_of(value,
                               [](char character) { return std::isspace(static_cast<unsigned char>(character)) != 0; });
}

[[nodiscard]] bool is_valid_asset_key_segment(std::string_view segment) noexcept {
    if (segment.empty() || segment == "." || segment == "..") {
        return false;
    }
    return std::ranges::all_of(segment, [](char character) {
        return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '.' ||
               character == '_' || character == '-';
    });
}

[[nodiscard]] bool is_valid_asset_key(std::string_view key) noexcept {
    if (key.empty() || key.front() == '/' || key.find('\\') != std::string_view::npos ||
        key.find(':') != std::string_view::npos || has_ascii_space(key) || has_control_character(key)) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= key.size()) {
        const auto end = key.find('/', begin);
        const auto segment = key.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (!is_valid_asset_key_segment(segment)) {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

[[nodiscard]] bool is_safe_repository_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos ||
        path.find('\\') != std::string_view::npos || path.find(';') != std::string_view::npos ||
        has_control_character(path)) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        const auto token = path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (token.empty() || token == "." || token == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

[[nodiscard]] bool is_safe_package_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos ||
        path.find('\\') != std::string_view::npos || path.find('\0') != std::string_view::npos ||
        path.find('\n') != std::string_view::npos || path.find('\r') != std::string_view::npos ||
        path.find(';') != std::string_view::npos) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        const auto token = path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (token.empty() || token == "." || token == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

[[nodiscard]] bool ends_with(std::string_view value, std::string_view suffix) noexcept {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

void add_diagnostic(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics, std::string code,
                    std::string message, std::string path = {}, AssetKeyV2 asset_key = {}, AuthoringId node = {},
                    AuthoringId component = {}, SceneComponentTypeId component_type = {}, std::string property = {},
                    std::string unsupported_gap_id = {}, std::string validation_recipe = {}) {
    diagnostics.push_back(SceneV2RuntimePackageMigrationDiagnostic{
        .severity = "error",
        .code = std::move(code),
        .message = std::move(message),
        .path = std::move(path),
        .asset_key = std::move(asset_key),
        .node = std::move(node),
        .component = std::move(component),
        .component_type = std::move(component_type),
        .property = std::move(property),
        .unsupported_gap_id = std::move(unsupported_gap_id),
        .validation_recipe = std::move(validation_recipe),
    });
}

void sort_diagnostics(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics, [](const SceneV2RuntimePackageMigrationDiagnostic& lhs,
                                      const SceneV2RuntimePackageMigrationDiagnostic& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.code != rhs.code) {
            return lhs.code < rhs.code;
        }
        if (lhs.node.value != rhs.node.value) {
            return lhs.node.value < rhs.node.value;
        }
        if (lhs.component.value != rhs.component.value) {
            return lhs.component.value < rhs.component.value;
        }
        if (lhs.property != rhs.property) {
            return lhs.property < rhs.property;
        }
        if (lhs.asset_key.value != rhs.asset_key.value) {
            return lhs.asset_key.value < rhs.asset_key.value;
        }
        return lhs.message < rhs.message;
    });
}

void validate_claim(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics, std::string_view value,
                    std::string code, std::string message, std::string unsupported_gap_id) {
    if (value != "unsupported") {
        add_diagnostic(diagnostics, std::move(code), std::move(message), {}, {}, {}, {}, {}, {},
                       std::move(unsupported_gap_id));
    }
}

void validate_unsupported_claims(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                 const SceneV2RuntimePackageMigrationRequest& request) {
    validate_claim(diagnostics, request.package_cooking, "unsupported_package_cooking",
                   "package cooking is not supported by Scene v2 runtime package migration", "runtime-resource-v2");
    validate_claim(diagnostics, request.dependent_asset_cooking, "unsupported_dependent_asset_cooking",
                   "dependent asset cooking is not supported by Scene v2 runtime package migration",
                   "runtime-resource-v2");
    validate_claim(diagnostics, request.external_importer_execution, "unsupported_external_importer_execution",
                   "external importer execution is not supported by Scene v2 runtime package migration",
                   "production-ui-importer-platform-adapters");
    validate_claim(diagnostics, request.renderer_rhi_residency, "unsupported_renderer_rhi_residency",
                   "renderer/RHI residency is not supported by Scene v2 runtime package migration",
                   "renderer-rhi-resource-foundation");
    validate_claim(diagnostics, request.package_streaming, "unsupported_package_streaming",
                   "package streaming is not supported by Scene v2 runtime package migration", "runtime-resource-v2");
    validate_claim(diagnostics, request.material_graph, "unsupported_material_graph",
                   "material graph is not supported by Scene v2 runtime package migration",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.shader_graph, "unsupported_shader_graph",
                   "shader graph is not supported by Scene v2 runtime package migration", "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.live_shader_generation, "unsupported_live_shader_generation",
                   "live shader generation is not supported by Scene v2 runtime package migration",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.editor_productization, "unsupported_editor_productization",
                   "editor productization is not supported by Scene v2 runtime package migration",
                   "editor-productization");
    validate_claim(diagnostics, request.metal_readiness, "unsupported_metal_readiness",
                   "Metal readiness is host-gated and is not supported by Scene v2 runtime package migration",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.public_native_rhi_handles, "unsupported_public_native_rhi_handles",
                   "public native/RHI handles are not supported by Scene v2 runtime package migration",
                   "renderer-rhi-resource-foundation");
    validate_claim(diagnostics, request.general_production_renderer_quality,
                   "unsupported_general_production_renderer_quality",
                   "general production renderer quality is not supported by Scene v2 runtime package migration",
                   "renderer-rhi-resource-foundation");
    validate_claim(diagnostics, request.arbitrary_shell, "unsupported_arbitrary_shell",
                   "arbitrary shell execution is not supported by Scene v2 runtime package migration",
                   "editor-productization");
    validate_claim(diagnostics, request.free_form_edit, "unsupported_free_form_edit",
                   "free-form edits are not supported by Scene v2 runtime package migration", "editor-productization");
}

void validate_request_shape(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                            const SceneV2RuntimePackageMigrationRequest& request) {
    if (!is_safe_repository_path(request.scene_v2_path) || !ends_with(request.scene_v2_path, ".scene")) {
        add_diagnostic(diagnostics, "unsafe_scene_v2_path",
                       "scene v2 path must be a safe repository-relative .scene path", request.scene_v2_path);
    }
    if (!is_safe_repository_path(request.source_registry_path) ||
        !ends_with(request.source_registry_path, ".geassets")) {
        add_diagnostic(diagnostics, "unsafe_source_registry_path",
                       "source registry path must be a safe repository-relative .geassets path",
                       request.source_registry_path);
    }
    if (!is_safe_repository_path(request.package_index_path) || !is_safe_package_path(request.package_index_path) ||
        !ends_with(request.package_index_path, ".geindex")) {
        add_diagnostic(diagnostics, "unsafe_package_index_path",
                       "package index path must be a safe repository-relative package-relative .geindex path",
                       request.package_index_path);
    }
    if (!is_safe_repository_path(request.output_scene_path) || !is_safe_package_path(request.output_scene_path) ||
        !ends_with(request.output_scene_path, ".scene")) {
        add_diagnostic(diagnostics, "unsafe_output_scene_path",
                       "output scene path must be a safe repository-relative package-relative .scene path",
                       request.output_scene_path);
    }
    if (!request.output_scene_path.empty() && (request.output_scene_path == request.scene_v2_path ||
                                               request.output_scene_path == request.source_registry_path ||
                                               request.output_scene_path == request.package_index_path)) {
        add_diagnostic(diagnostics, "aliased_output_path",
                       "output scene path must not alias an input path for Scene v2 runtime package migration",
                       request.output_scene_path, request.scene_asset_key);
    }

    validate_unsupported_claims(diagnostics, request);

    if (request.kind == SceneV2RuntimePackageMigrationCommandKind::free_form_edit) {
        add_diagnostic(diagnostics, "unsupported_free_form_edit",
                       "free-form edits are not supported by Scene v2 runtime package migration", request.scene_v2_path,
                       request.scene_asset_key, {}, {}, {}, {}, "editor-productization");
        return;
    }
    if (request.kind != SceneV2RuntimePackageMigrationCommandKind::migrate_scene_v2_runtime_package) {
        add_diagnostic(diagnostics, "unsupported_operation",
                       "only migrate_scene_v2_runtime_package is supported by Scene v2 runtime package migration",
                       request.scene_v2_path, request.scene_asset_key);
    }
    if (request.scene_asset_key.value.empty()) {
        add_diagnostic(diagnostics, "invalid_scene_asset_key",
                       "scene asset key must be non-empty for Scene v2 runtime package migration",
                       request.scene_v2_path, request.scene_asset_key);
    } else if (!is_valid_asset_key(request.scene_asset_key.value)) {
        add_diagnostic(diagnostics, "invalid_scene_asset_key",
                       "scene asset key must be a valid AssetKeyV2 for Scene v2 runtime package migration",
                       request.scene_v2_path, request.scene_asset_key);
    }
    if (request.source_revision == 0) {
        add_diagnostic(diagnostics, "invalid_source_revision",
                       "scene source revision must be non-zero for Scene v2 runtime package migration",
                       request.output_scene_path, request.scene_asset_key);
    }
}

[[nodiscard]] std::string schema_diagnostic_code(SceneSchemaV2DiagnosticCode code) {
    switch (code) {
    case SceneSchemaV2DiagnosticCode::invalid_scene_name:
        return "invalid_scene_name";
    case SceneSchemaV2DiagnosticCode::invalid_authoring_id:
        return "invalid_authoring_id";
    case SceneSchemaV2DiagnosticCode::duplicate_node_id:
        return "duplicate_node_id";
    case SceneSchemaV2DiagnosticCode::duplicate_component_id:
        return "duplicate_component_id";
    case SceneSchemaV2DiagnosticCode::missing_parent_node:
        return "missing_parent_node";
    case SceneSchemaV2DiagnosticCode::missing_component_node:
        return "missing_component_node";
    case SceneSchemaV2DiagnosticCode::invalid_component_type:
        return "invalid_component_type";
    case SceneSchemaV2DiagnosticCode::duplicate_component_property:
        return "duplicate_component_property";
    case SceneSchemaV2DiagnosticCode::invalid_component_property:
        return "invalid_component_property";
    case SceneSchemaV2DiagnosticCode::invalid_text_value:
        return "invalid_text_value";
    case SceneSchemaV2DiagnosticCode::invalid_transform:
        return "invalid_transform";
    case SceneSchemaV2DiagnosticCode::missing_override_target:
        return "missing_override_target";
    case SceneSchemaV2DiagnosticCode::duplicate_override_path:
        return "duplicate_override_path";
    }
    return "invalid_scene_v2_document";
}

[[nodiscard]] std::string schema_diagnostic_message(SceneSchemaV2DiagnosticCode code) {
    switch (code) {
    case SceneSchemaV2DiagnosticCode::duplicate_component_id:
        return "duplicate component id";
    case SceneSchemaV2DiagnosticCode::duplicate_node_id:
        return "duplicate node id";
    case SceneSchemaV2DiagnosticCode::invalid_transform:
        return "scene v2 transform is invalid";
    case SceneSchemaV2DiagnosticCode::missing_component_node:
        return "scene v2 component node is missing";
    case SceneSchemaV2DiagnosticCode::missing_parent_node:
        return "scene v2 parent node is missing";
    default:
        break;
    }
    return "scene v2 document is invalid";
}

void append_scene_schema_diagnostics(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                     const std::vector<SceneSchemaV2Diagnostic>& schema_diagnostics,
                                     const std::string& scene_path) {
    for (const auto& diagnostic : schema_diagnostics) {
        add_diagnostic(diagnostics, schema_diagnostic_code(diagnostic.code), schema_diagnostic_message(diagnostic.code),
                       scene_path, {}, diagnostic.node, diagnostic.component, diagnostic.component_type,
                       diagnostic.property);
    }
}

void append_scene_package_failures(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                   const std::vector<ScenePackageUpdateFailure>& failures) {
    for (const auto& failure : failures) {
        add_diagnostic(diagnostics, "scene_package_update_failed", "scene package update failed: " + failure.diagnostic,
                       failure.path);
    }
}

void append_duplicate_identity_diagnostics(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                           std::string_view text, const std::string& scene_path) {
    std::unordered_set<std::string> node_ids;
    std::unordered_set<std::string> component_ids;

    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                     : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;

        const auto separator = line.find('=');
        if (separator == std::string_view::npos) {
            continue;
        }
        const auto key = line.substr(0, separator);
        const auto value = line.substr(separator + 1U);
        if (value.empty()) {
            continue;
        }

        if (key.starts_with("node.") && ends_with(key, ".id")) {
            if (!node_ids.insert(std::string{value}).second) {
                add_diagnostic(diagnostics, "duplicate_node_id", "duplicate node id", scene_path, {},
                               AuthoringId{std::string{value}});
            }
        } else if (key.starts_with("component.") && ends_with(key, ".id")) {
            if (!component_ids.insert(std::string{value}).second) {
                add_diagnostic(diagnostics, "duplicate_component_id", "duplicate component id", scene_path, {}, {},
                               AuthoringId{std::string{value}});
            }
        }
    }
}

[[nodiscard]] std::optional<float> parse_float_value(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                                     std::string_view text, const SceneComponentDocumentV2& component,
                                                     std::string_view property, const std::string& scene_path) {
    try {
        const auto value_text = std::string(text);
        std::size_t consumed = 0;
        const auto value = std::stof(value_text, &consumed);
        if (consumed != value_text.size() || !std::isfinite(value)) {
            throw std::invalid_argument("invalid float");
        }
        return value;
    } catch (const std::exception&) {
        add_diagnostic(diagnostics, "invalid_numeric_value", "component property numeric value is invalid", scene_path,
                       {}, component.node, component.id, component.type, std::string{property});
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<bool> parse_bool_value(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                                   std::string_view text, const SceneComponentDocumentV2& component,
                                                   std::string_view property, const std::string& scene_path) {
    if (text == "true") {
        return true;
    }
    if (text == "false") {
        return false;
    }
    add_diagnostic(diagnostics, "invalid_bool_value", "component property boolean value is invalid", scene_path, {},
                   component.node, component.id, component.type, std::string{property});
    return std::nullopt;
}

[[nodiscard]] std::optional<Vec2> parse_vec2_value(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                                   std::string_view text, const SceneComponentDocumentV2& component,
                                                   std::string_view property, const std::string& scene_path) {
    const auto separator = text.find(',');
    if (separator == std::string_view::npos || text.find(',', separator + 1U) != std::string_view::npos) {
        add_diagnostic(diagnostics, "invalid_vector_value", "component property vector value is invalid", scene_path,
                       {}, component.node, component.id, component.type, std::string{property});
        return std::nullopt;
    }
    const auto x = parse_float_value(diagnostics, text.substr(0, separator), component, property, scene_path);
    const auto y = parse_float_value(diagnostics, text.substr(separator + 1U), component, property, scene_path);
    if (!x.has_value() || !y.has_value()) {
        return std::nullopt;
    }
    return Vec2{.x = *x, .y = *y};
}

[[nodiscard]] std::optional<Vec3> parse_vec3_value(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                                   std::string_view text, const SceneComponentDocumentV2& component,
                                                   std::string_view property, const std::string& scene_path) {
    const auto first = text.find(',');
    const auto second = first == std::string_view::npos ? std::string_view::npos : text.find(',', first + 1U);
    if (first == std::string_view::npos || second == std::string_view::npos ||
        text.find(',', second + 1U) != std::string_view::npos) {
        add_diagnostic(diagnostics, "invalid_vector_value", "component property vector value is invalid", scene_path,
                       {}, component.node, component.id, component.type, std::string{property});
        return std::nullopt;
    }
    const auto x = parse_float_value(diagnostics, text.substr(0, first), component, property, scene_path);
    const auto y =
        parse_float_value(diagnostics, text.substr(first + 1U, second - first - 1U), component, property, scene_path);
    const auto z = parse_float_value(diagnostics, text.substr(second + 1U), component, property, scene_path);
    if (!x.has_value() || !y.has_value() || !z.has_value()) {
        return std::nullopt;
    }
    return Vec3{.x = *x, .y = *y, .z = *z};
}

[[nodiscard]] std::optional<std::array<float, 4>>
parse_rgba_value(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics, std::string_view text,
                 const SceneComponentDocumentV2& component, std::string_view property, const std::string& scene_path) {
    const auto first = text.find(',');
    const auto second = first == std::string_view::npos ? std::string_view::npos : text.find(',', first + 1U);
    const auto third = second == std::string_view::npos ? std::string_view::npos : text.find(',', second + 1U);
    if (first == std::string_view::npos || second == std::string_view::npos || third == std::string_view::npos ||
        text.find(',', third + 1U) != std::string_view::npos) {
        add_diagnostic(diagnostics, "invalid_vector_value", "component property vector value is invalid", scene_path,
                       {}, component.node, component.id, component.type, std::string{property});
        return std::nullopt;
    }
    const auto r = parse_float_value(diagnostics, text.substr(0, first), component, property, scene_path);
    const auto g =
        parse_float_value(diagnostics, text.substr(first + 1U, second - first - 1U), component, property, scene_path);
    const auto b =
        parse_float_value(diagnostics, text.substr(second + 1U, third - second - 1U), component, property, scene_path);
    const auto a = parse_float_value(diagnostics, text.substr(third + 1U), component, property, scene_path);
    if (!r.has_value() || !g.has_value() || !b.has_value() || !a.has_value()) {
        return std::nullopt;
    }
    return std::array<float, 4>{*r, *g, *b, *a};
}

[[nodiscard]] std::string_view property_value(const SceneComponentDocumentV2& component,
                                              std::string_view property) noexcept {
    const auto it = std::ranges::find_if(
        component.properties, [property](const SceneComponentPropertyV2& row) { return row.name == property; });
    return it == component.properties.end() ? std::string_view{} : std::string_view{it->value};
}

[[nodiscard]] bool has_property(const SceneComponentDocumentV2& component, std::string_view property) noexcept {
    return std::ranges::any_of(component.properties,
                               [property](const SceneComponentPropertyV2& row) { return row.name == property; });
}

[[nodiscard]] bool is_allowed_property(std::string_view type, std::string_view property) noexcept {
    if (type == "camera") {
        return property == "projection" || property == "primary" || property == "vertical_fov_radians" ||
               property == "orthographic_height" || property == "near_plane" || property == "far_plane";
    }
    if (type == "light") {
        return property == "type" || property == "color" || property == "intensity" || property == "range" ||
               property == "casts_shadows";
    }
    if (type == "mesh_renderer") {
        return property == "mesh" || property == "material" || property == "visible";
    }
    if (type == "sprite_renderer") {
        return property == "sprite" || property == "material" || property == "size" || property == "tint" ||
               property == "visible";
    }
    return false;
}

void validate_supported_properties(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                   const SceneComponentDocumentV2& component, const std::string& scene_path) {
    for (const auto& property : component.properties) {
        if (!is_allowed_property(component.type.value, property.name)) {
            add_diagnostic(diagnostics, "unsupported_component_property",
                           "unsupported component property for Scene v2 runtime package migration", scene_path, {},
                           component.node, component.id, component.type, property.name);
        }
    }
}

[[nodiscard]] const SourceAssetRegistryRowV1* find_source_row(const SourceAssetRegistryDocumentV1& registry,
                                                              std::string_view key) noexcept {
    const auto it = std::ranges::find_if(registry.assets, [key](const auto& row) { return row.key.value == key; });
    return it == registry.assets.end() ? nullptr : &*it;
}

[[nodiscard]] std::string_view asset_kind_label(AssetKind kind) noexcept {
    switch (kind) {
    case AssetKind::mesh:
        return "mesh";
    case AssetKind::morph_mesh_cpu:
        return "morph_mesh_cpu";
    case AssetKind::animation_float_clip:
        return "animation_float_clip";
    case AssetKind::skinned_mesh:
        return "skinned_mesh";
    case AssetKind::material:
        return "material";
    case AssetKind::texture:
        return "texture";
    case AssetKind::scene:
        return "scene";
    case AssetKind::audio:
        return "audio";
    case AssetKind::script:
        return "script";
    case AssetKind::shader:
        return "shader";
    case AssetKind::ui_atlas:
        return "ui_atlas";
    case AssetKind::tilemap:
        return "tilemap";
    case AssetKind::physics_collision_scene:
        return "physics_collision_scene";
    case AssetKind::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] std::optional<ResolvedAssetReference>
resolve_asset_reference(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                        const SourceAssetRegistryDocumentV1& registry, const SceneComponentDocumentV2& component,
                        std::string_view property, AssetKind required_kind, std::string_view required_label,
                        const std::string& scene_path) {
    const auto key = property_value(component, property);
    if (key.empty()) {
        add_diagnostic(diagnostics, "missing_required_component_property",
                       "required component property is missing for Scene v2 runtime package migration", scene_path, {},
                       component.node, component.id, component.type, std::string{property});
        return std::nullopt;
    }

    const auto* row = find_source_row(registry, key);
    if (row == nullptr) {
        add_diagnostic(diagnostics, "missing_source_asset_key", "source asset key is missing", scene_path,
                       AssetKeyV2{std::string{key}}, component.node, component.id, component.type,
                       std::string{property});
        return std::nullopt;
    }
    if (row->kind != required_kind) {
        add_diagnostic(diagnostics, "wrong_source_asset_kind",
                       std::string{component.type.value} + "." + std::string{property} + " must reference a " +
                           std::string{required_label} + " source asset",
                       scene_path, row->key, component.node, component.id, component.type, std::string{property});
        return std::nullopt;
    }
    return ResolvedAssetReference{.key = row->key, .id = asset_id_from_key_v2(row->key)};
}

void append_dependency(std::vector<AssetId>& ids, std::vector<SourceAssetDependencyRowV1>& rows, AssetId id,
                       AssetDependencyKind kind, AssetKeyV2 key) {
    ids.push_back(id);
    rows.push_back(SourceAssetDependencyRowV1{.kind = kind, .key = std::move(key)});
}

void sort_unique_asset_ids(std::vector<AssetId>& ids) {
    std::ranges::sort(ids, [](AssetId lhs, AssetId rhs) { return lhs.value < rhs.value; });
    const auto duplicates = std::ranges::unique(ids, [](AssetId lhs, AssetId rhs) { return lhs == rhs; });
    ids.erase(duplicates.begin(), duplicates.end());
}

[[nodiscard]] bool dependency_row_less(const SourceAssetDependencyRowV1& lhs,
                                       const SourceAssetDependencyRowV1& rhs) noexcept {
    const auto lhs_kind = source_asset_dependency_kind_name_v1(lhs.kind);
    const auto rhs_kind = source_asset_dependency_kind_name_v1(rhs.kind);
    if (lhs_kind != rhs_kind) {
        return lhs_kind < rhs_kind;
    }
    return lhs.key.value < rhs.key.value;
}

void sort_unique_dependency_rows(std::vector<SourceAssetDependencyRowV1>& rows) {
    std::ranges::sort(rows, dependency_row_less);
    const auto duplicates =
        std::ranges::unique(rows, [](const SourceAssetDependencyRowV1& lhs, const SourceAssetDependencyRowV1& rhs) {
            return lhs.kind == rhs.kind && lhs.key.value == rhs.key.value;
        });
    rows.erase(duplicates.begin(), duplicates.end());
}

[[nodiscard]] bool placement_name_matches_base(std::string_view placement, std::string_view base) noexcept {
    return placement == base ||
           (placement.starts_with(base) && placement.size() > base.size() && placement[base.size()] == '.');
}

[[nodiscard]] bool placement_exists(const std::vector<AssetIdentityPlacementRequestV2>& requests,
                                    std::string_view placement) noexcept {
    return std::ranges::any_of(requests, [placement](const auto& request) { return request.placement == placement; });
}

void append_placement_request(std::vector<AssetIdentityPlacementRequestV2>& requests, std::string_view placement,
                              AssetKeyV2 key, AssetKind expected_kind) {
    for (const auto& request : requests) {
        if (placement_name_matches_base(request.placement, placement) && request.key.value == key.value &&
            request.expected_kind == expected_kind) {
            return;
        }
    }

    auto resolved_placement = std::string{placement};
    for (std::uint32_t ordinal = 1; placement_exists(requests, resolved_placement); ++ordinal) {
        resolved_placement = std::string{placement} + "." + std::to_string(ordinal);
    }

    requests.push_back(AssetIdentityPlacementRequestV2{
        .placement = std::move(resolved_placement),
        .key = std::move(key),
        .expected_kind = expected_kind,
    });
}

void sort_placement_requests(std::vector<AssetIdentityPlacementRequestV2>& requests) {
    std::ranges::sort(requests, [](const auto& lhs, const auto& rhs) {
        if (lhs.placement != rhs.placement) {
            return lhs.placement < rhs.placement;
        }
        return lhs.key.value < rhs.key.value;
    });
}

[[nodiscard]] AssetIdentityDocumentV2
make_scene_runtime_package_identity_document(const SourceAssetRegistryDocumentV1& registry,
                                             const SceneV2RuntimePackageMigrationRequest& request) {
    auto identity = project_source_asset_registry_identity_v2(registry);
    const auto existing_scene_row = std::ranges::find_if(
        identity.assets, [&request](const auto& row) { return row.key.value == request.scene_asset_key.value; });
    if (existing_scene_row == identity.assets.end()) {
        identity.assets.push_back(AssetIdentityRowV2{
            .key = request.scene_asset_key,
            .kind = AssetKind::scene,
            .source_path = request.scene_v2_path,
        });
    }
    return identity;
}

void append_placement_plan_diagnostics(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                       const AssetIdentityPlacementPlanV2& plan,
                                       const SceneV2RuntimePackageMigrationRequest& request) {
    for (const auto& diagnostic : plan.identity_diagnostics) {
        add_diagnostic(diagnostics, "invalid_asset_identity_projection",
                       "source asset registry identity projection is invalid", request.source_registry_path,
                       diagnostic.key);
    }
    for (const auto& diagnostic : plan.diagnostics) {
        add_diagnostic(diagnostics, "asset_identity_placement_failed",
                       "asset identity placement evidence could not be planned", request.source_registry_path,
                       diagnostic.key);
    }
}

[[nodiscard]] std::vector<AssetIdentityPlacementRowV2>
plan_scene_runtime_package_placement_rows(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                          const SourceAssetRegistryDocumentV1& registry,
                                          const SceneV2RuntimePackageMigrationRequest& request,
                                          std::vector<AssetIdentityPlacementRequestV2> placement_requests) {
    append_placement_request(placement_requests, "scene.runtime_package", request.scene_asset_key, AssetKind::scene);
    sort_placement_requests(placement_requests);

    const auto identity = make_scene_runtime_package_identity_document(registry, request);
    auto plan = plan_asset_identity_placements_v2(identity, placement_requests);
    if (!plan.can_place) {
        append_placement_plan_diagnostics(diagnostics, plan, request);
        return {};
    }
    return std::move(plan.rows);
}

void assign_camera_component(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                             SceneNodeComponents& components, const SceneComponentDocumentV2& component,
                             const std::string& scene_path) {
    CameraComponent camera;
    const auto projection = property_value(component, "projection");
    if (!projection.empty()) {
        if (projection == "perspective") {
            camera.projection = CameraProjectionMode::perspective;
        } else if (projection == "orthographic") {
            camera.projection = CameraProjectionMode::orthographic;
        } else {
            add_diagnostic(diagnostics, "invalid_component_value", "camera projection is unsupported", scene_path, {},
                           component.node, component.id, component.type, "projection");
        }
    }
    if (has_property(component, "primary")) {
        if (const auto value =
                parse_bool_value(diagnostics, property_value(component, "primary"), component, "primary", scene_path);
            value.has_value()) {
            camera.primary = *value;
        }
    }
    if (has_property(component, "vertical_fov_radians")) {
        if (const auto value = parse_float_value(diagnostics, property_value(component, "vertical_fov_radians"),
                                                 component, "vertical_fov_radians", scene_path);
            value.has_value()) {
            camera.vertical_fov_radians = *value;
        }
    }
    if (has_property(component, "orthographic_height")) {
        if (const auto value = parse_float_value(diagnostics, property_value(component, "orthographic_height"),
                                                 component, "orthographic_height", scene_path);
            value.has_value()) {
            camera.orthographic_height = *value;
        }
    }
    if (has_property(component, "near_plane")) {
        if (const auto value = parse_float_value(diagnostics, property_value(component, "near_plane"), component,
                                                 "near_plane", scene_path);
            value.has_value()) {
            camera.near_plane = *value;
        }
    }
    if (has_property(component, "far_plane")) {
        if (const auto value = parse_float_value(diagnostics, property_value(component, "far_plane"), component,
                                                 "far_plane", scene_path);
            value.has_value()) {
            camera.far_plane = *value;
        }
    }
    if (!is_valid_camera_component(camera)) {
        add_diagnostic(diagnostics, "invalid_scene_v1_projection",
                       "camera component values are invalid for Scene v1 runtime payload", scene_path, {},
                       component.node, component.id, component.type);
        return;
    }
    components.camera = camera;
}

void assign_light_component(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                            SceneNodeComponents& components, const SceneComponentDocumentV2& component,
                            const std::string& scene_path) {
    LightComponent light;
    const auto type = property_value(component, "type");
    if (!type.empty()) {
        if (type == "directional") {
            light.type = LightType::directional;
        } else if (type == "point") {
            light.type = LightType::point;
        } else if (type == "spot") {
            add_diagnostic(diagnostics, "unsupported_component_value",
                           "spot lights are not supported by the first Scene v2 runtime package migration slice",
                           scene_path, {}, component.node, component.id, component.type, "type");
        } else {
            add_diagnostic(diagnostics, "invalid_component_value", "light type is unsupported", scene_path, {},
                           component.node, component.id, component.type, "type");
        }
    }
    if (has_property(component, "color")) {
        if (const auto value =
                parse_vec3_value(diagnostics, property_value(component, "color"), component, "color", scene_path);
            value.has_value()) {
            light.color = *value;
        }
    }
    if (has_property(component, "intensity")) {
        if (const auto value = parse_float_value(diagnostics, property_value(component, "intensity"), component,
                                                 "intensity", scene_path);
            value.has_value()) {
            light.intensity = *value;
        }
    }
    if (has_property(component, "range")) {
        if (const auto value =
                parse_float_value(diagnostics, property_value(component, "range"), component, "range", scene_path);
            value.has_value()) {
            light.range = *value;
        }
    }
    if (has_property(component, "casts_shadows")) {
        if (const auto value = parse_bool_value(diagnostics, property_value(component, "casts_shadows"), component,
                                                "casts_shadows", scene_path);
            value.has_value()) {
            light.casts_shadows = *value;
        }
    }
    if (!is_valid_light_component(light)) {
        add_diagnostic(diagnostics, "invalid_component_value",
                       "light component values are invalid for Scene v1 runtime payload", scene_path, {},
                       component.node, component.id, component.type);
        return;
    }
    components.light = light;
}

void assign_mesh_renderer_component(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                    SceneNodeComponents& components, const SourceAssetRegistryDocumentV1& registry,
                                    ProjectedScene& projected, const SceneComponentDocumentV2& component,
                                    const std::string& scene_path) {
    MeshRendererComponent renderer;
    const auto mesh =
        resolve_asset_reference(diagnostics, registry, component, "mesh", AssetKind::mesh, "mesh", scene_path);
    const auto material = resolve_asset_reference(diagnostics, registry, component, "material", AssetKind::material,
                                                  "material", scene_path);
    if (mesh.has_value()) {
        renderer.mesh = mesh->id;
        append_dependency(projected.mesh_dependencies, projected.dependency_rows, mesh->id,
                          AssetDependencyKind::scene_mesh, mesh->key);
        append_placement_request(projected.placement_requests, "scene.component.mesh_renderer.mesh", mesh->key,
                                 AssetKind::mesh);
    }
    if (material.has_value()) {
        renderer.material = material->id;
        append_dependency(projected.material_dependencies, projected.dependency_rows, material->id,
                          AssetDependencyKind::scene_material, material->key);
        append_placement_request(projected.placement_requests, "scene.component.mesh_renderer.material", material->key,
                                 AssetKind::material);
    }
    if (has_property(component, "visible")) {
        if (const auto value =
                parse_bool_value(diagnostics, property_value(component, "visible"), component, "visible", scene_path);
            value.has_value()) {
            renderer.visible = *value;
        }
    }
    if (!is_valid_mesh_renderer_component(renderer)) {
        add_diagnostic(diagnostics, "invalid_component_value",
                       "mesh renderer component values are invalid for Scene v1 runtime payload", scene_path, {},
                       component.node, component.id, component.type);
        return;
    }
    components.mesh_renderer = renderer;
}

void assign_sprite_renderer_component(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                      SceneNodeComponents& components, const SourceAssetRegistryDocumentV1& registry,
                                      ProjectedScene& projected, const SceneComponentDocumentV2& component,
                                      const std::string& scene_path) {
    SpriteRendererComponent renderer;
    const auto sprite =
        resolve_asset_reference(diagnostics, registry, component, "sprite", AssetKind::texture, "texture", scene_path);
    const auto material = resolve_asset_reference(diagnostics, registry, component, "material", AssetKind::material,
                                                  "material", scene_path);
    if (sprite.has_value()) {
        renderer.sprite = sprite->id;
        append_dependency(projected.sprite_dependencies, projected.dependency_rows, sprite->id,
                          AssetDependencyKind::scene_sprite, sprite->key);
        append_placement_request(projected.placement_requests, "scene.component.sprite_renderer.sprite", sprite->key,
                                 AssetKind::texture);
    }
    if (material.has_value()) {
        renderer.material = material->id;
        append_dependency(projected.material_dependencies, projected.dependency_rows, material->id,
                          AssetDependencyKind::scene_material, material->key);
        append_placement_request(projected.placement_requests, "scene.component.sprite_renderer.material",
                                 material->key, AssetKind::material);
    }
    if (has_property(component, "size")) {
        if (const auto value =
                parse_vec2_value(diagnostics, property_value(component, "size"), component, "size", scene_path);
            value.has_value()) {
            renderer.size = *value;
        }
    }
    if (has_property(component, "tint")) {
        if (const auto value =
                parse_rgba_value(diagnostics, property_value(component, "tint"), component, "tint", scene_path);
            value.has_value()) {
            renderer.tint = *value;
        }
    }
    if (has_property(component, "visible")) {
        if (const auto value =
                parse_bool_value(diagnostics, property_value(component, "visible"), component, "visible", scene_path);
            value.has_value()) {
            renderer.visible = *value;
        }
    }
    if (!is_valid_sprite_renderer_component(renderer)) {
        add_diagnostic(diagnostics, "invalid_component_value",
                       "sprite renderer component values are invalid for Scene v1 runtime payload", scene_path, {},
                       component.node, component.id, component.type);
        return;
    }
    components.sprite_renderer = renderer;
}

[[nodiscard]] bool is_supported_runtime_component_type(std::string_view type) noexcept {
    return type == "camera" || type == "light" || type == "mesh_renderer" || type == "sprite_renderer";
}

[[nodiscard]] ProjectedScene project_scene_v2_to_runtime_scene(const SceneDocumentV2& scene,
                                                               const SourceAssetRegistryDocumentV1& registry,
                                                               const std::string& scene_path) {
    ProjectedScene projected{.scene = Scene{scene.name}};

    std::unordered_map<std::string, SceneNodeId> runtime_nodes;
    runtime_nodes.reserve(scene.nodes.size());
    for (const auto& node_doc : scene.nodes) {
        const auto id = projected.scene.create_node(node_doc.name);
        if (auto* node = projected.scene.find_node(id); node != nullptr) {
            node->transform = node_doc.transform;
        }
        runtime_nodes.emplace(node_doc.id.value, id);
    }

    std::unordered_map<std::string, SceneNodeComponents> components_by_node;
    std::unordered_set<std::string> node_component_types;
    for (const auto& component : scene.components) {
        if (!is_supported_runtime_component_type(component.type.value)) {
            add_diagnostic(projected.diagnostics, "unsupported_component_type",
                           "unsupported component type for Scene v2 runtime package migration", scene_path, {},
                           component.node, component.id, component.type);
            continue;
        }

        validate_supported_properties(projected.diagnostics, component, scene_path);
        const auto type_key = component.node.value + ":" + component.type.value;
        if (!node_component_types.insert(type_key).second) {
            add_diagnostic(projected.diagnostics, "duplicate_runtime_component_type",
                           "duplicate runtime component type on one scene node", scene_path, {}, component.node,
                           component.id, component.type);
            continue;
        }

        auto& components = components_by_node[component.node.value];
        if (component.type.value == "camera") {
            assign_camera_component(projected.diagnostics, components, component, scene_path);
        } else if (component.type.value == "light") {
            assign_light_component(projected.diagnostics, components, component, scene_path);
        } else if (component.type.value == "mesh_renderer") {
            assign_mesh_renderer_component(projected.diagnostics, components, registry, projected, component,
                                           scene_path);
        } else if (component.type.value == "sprite_renderer") {
            assign_sprite_renderer_component(projected.diagnostics, components, registry, projected, component,
                                             scene_path);
        }
    }

    if (!projected.diagnostics.empty()) {
        sort_diagnostics(projected.diagnostics);
        return projected;
    }

    for (const auto& node_doc : scene.nodes) {
        const auto component_it = components_by_node.find(node_doc.id.value);
        if (component_it != components_by_node.end()) {
            try {
                projected.scene.set_components(runtime_nodes.at(node_doc.id.value), component_it->second);
            } catch (const std::exception& error) {
                add_diagnostic(projected.diagnostics, "invalid_scene_v1_projection",
                               std::string{"failed to assign runtime scene components: "} + error.what(), scene_path,
                               {}, node_doc.id);
            }
        }
    }

    for (const auto& node_doc : scene.nodes) {
        if (node_doc.parent.value.empty()) {
            continue;
        }
        try {
            projected.scene.set_parent(runtime_nodes.at(node_doc.id.value), runtime_nodes.at(node_doc.parent.value));
        } catch (const std::exception& error) {
            add_diagnostic(projected.diagnostics, "invalid_scene_v1_projection",
                           std::string{"failed to assign runtime scene parent: "} + error.what(), scene_path, {},
                           node_doc.id);
        }
    }

    sort_unique_asset_ids(projected.mesh_dependencies);
    sort_unique_asset_ids(projected.material_dependencies);
    sort_unique_asset_ids(projected.sprite_dependencies);
    sort_unique_dependency_rows(projected.dependency_rows);
    sort_diagnostics(projected.diagnostics);
    return projected;
}

void append_changed_file(std::vector<SceneV2RuntimePackageMigrationChangedFile>& files,
                         const ScenePackageChangedFile& source, const SceneV2RuntimePackageMigrationRequest& request) {
    files.push_back(SceneV2RuntimePackageMigrationChangedFile{
        .path = source.path,
        .document_kind =
            source.path == request.output_scene_path ? "GameEngine.Scene.v1" : "GameEngine.CookedPackageIndex.v1",
        .content = source.content,
        .content_hash = source.content_hash,
    });
}

void append_model_mutation(std::vector<SceneV2RuntimePackageMigrationModelMutation>& mutations,
                           const SceneV2RuntimePackageMigrationRequest& request,
                           std::vector<AssetIdentityPlacementRowV2> placement_rows,
                           std::vector<SourceAssetDependencyRowV1> dependency_rows) {
    mutations.push_back(SceneV2RuntimePackageMigrationModelMutation{
        .kind = "migrate_scene_v2_runtime_package",
        .target_path = request.output_scene_path,
        .scene_v2_path = request.scene_v2_path,
        .package_index_path = request.package_index_path,
        .scene_asset_key = request.scene_asset_key,
        .scene_asset = asset_id_from_key_v2(request.scene_asset_key),
        .placement_rows = std::move(placement_rows),
        .dependency_rows = std::move(dependency_rows),
    });
}

[[nodiscard]] SourceAssetRegistryDocumentV1
parse_source_registry_content(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                              const SceneV2RuntimePackageMigrationRequest& request) {
    try {
        return deserialize_source_asset_registry_document(request.source_registry_content);
    } catch (const std::exception& error) {
        add_diagnostic(diagnostics, "invalid_source_registry_document",
                       std::string{"failed to parse source asset registry: "} + error.what(),
                       request.source_registry_path, request.scene_asset_key);
    }
    return {};
}

[[nodiscard]] SceneDocumentV2 parse_scene_v2_content(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                                     const SceneV2RuntimePackageMigrationRequest& request) {
    try {
        auto scene = deserialize_scene_document_v2(request.scene_v2_content);
        append_scene_schema_diagnostics(diagnostics, validate_scene_document_v2(scene), request.scene_v2_path);
        return scene;
    } catch (const std::exception& error) {
        add_diagnostic(diagnostics, "invalid_scene_v2_document",
                       std::string{"failed to parse Scene v2 document: "} + error.what(), request.scene_v2_path,
                       request.scene_asset_key);
    }
    return {};
}

[[nodiscard]] std::optional<PreparedSceneV2RuntimePackageMigration>
prepare_scene_v2_runtime_package_migration(std::vector<SceneV2RuntimePackageMigrationDiagnostic>& diagnostics,
                                           const SceneV2RuntimePackageMigrationRequest& request) {
    append_duplicate_identity_diagnostics(diagnostics, request.scene_v2_content, request.scene_v2_path);
    if (!diagnostics.empty()) {
        return std::nullopt;
    }

    const auto scene = parse_scene_v2_content(diagnostics, request);
    if (!diagnostics.empty()) {
        return std::nullopt;
    }

    const auto registry = parse_source_registry_content(diagnostics, request);
    if (!diagnostics.empty()) {
        return std::nullopt;
    }

    auto projected = project_scene_v2_to_runtime_scene(scene, registry, request.scene_v2_path);
    if (!projected.diagnostics.empty()) {
        diagnostics = std::move(projected.diagnostics);
        return std::nullopt;
    }

    auto placement_rows = plan_scene_runtime_package_placement_rows(diagnostics, registry, request,
                                                                    std::move(projected.placement_requests));
    if (!diagnostics.empty()) {
        return std::nullopt;
    }

    PreparedSceneV2RuntimePackageMigration prepared;
    prepared.scene = std::move(projected.scene);
    prepared.scene_asset = asset_id_from_key_v2(request.scene_asset_key);
    prepared.mesh_dependencies = std::move(projected.mesh_dependencies);
    prepared.material_dependencies = std::move(projected.material_dependencies);
    prepared.sprite_dependencies = std::move(projected.sprite_dependencies);
    prepared.placement_rows = std::move(placement_rows);
    prepared.dependency_rows = std::move(projected.dependency_rows);
    return prepared;
}

} // namespace

SceneV2RuntimePackageMigrationResult
plan_scene_v2_runtime_package_migration(const SceneV2RuntimePackageMigrationRequest& request) {
    SceneV2RuntimePackageMigrationResult result;
    result.validation_recipes = default_validation_recipes();
    result.unsupported_gap_ids = default_unsupported_gap_ids();

    validate_request_shape(result.diagnostics, request);
    if (!result.succeeded()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    auto prepared = prepare_scene_v2_runtime_package_migration(result.diagnostics, request);
    if (!prepared.has_value()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    ScenePackageUpdateDesc package_request;
    package_request.package_index_path = request.package_index_path;
    package_request.package_index_content = request.package_index_content;
    package_request.output_path = request.output_scene_path;
    package_request.source_revision = request.source_revision;
    package_request.scene_asset = prepared->scene_asset;
    package_request.scene = std::move(prepared->scene);
    package_request.mesh_dependencies = prepared->mesh_dependencies;
    package_request.material_dependencies = prepared->material_dependencies;
    package_request.sprite_dependencies = prepared->sprite_dependencies;

    const auto package_result = plan_scene_package_update(package_request);
    if (!package_result.succeeded()) {
        append_scene_package_failures(result.diagnostics, package_result.failures);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    result.scene_v1_content = package_result.scene_content;
    result.package_index_content = package_result.package_index_content;
    for (const auto& file : package_result.changed_files) {
        append_changed_file(result.changed_files, file, request);
    }
    append_model_mutation(result.model_mutations, request, std::move(prepared->placement_rows),
                          std::move(prepared->dependency_rows));
    return result;
}

SceneV2RuntimePackageMigrationResult
apply_scene_v2_runtime_package_migration(IFileSystem& filesystem,
                                         const SceneV2RuntimePackageMigrationRequest& request) {
    SceneV2RuntimePackageMigrationResult input_result;
    input_result.validation_recipes = default_validation_recipes();
    input_result.unsupported_gap_ids = default_unsupported_gap_ids();
    validate_request_shape(input_result.diagnostics, request);
    if (!input_result.succeeded()) {
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    auto apply_request = request;
    try {
        apply_request.scene_v2_content = filesystem.read_text(request.scene_v2_path);
        apply_request.source_registry_content = filesystem.read_text(request.source_registry_path);
    } catch (const std::exception& error) {
        add_diagnostic(input_result.diagnostics, "filesystem_read_failed",
                       std::string{"failed to read Scene v2 migration inputs: "} + error.what(), request.scene_v2_path,
                       request.scene_asset_key);
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    auto prepared = prepare_scene_v2_runtime_package_migration(input_result.diagnostics, apply_request);
    if (!prepared.has_value()) {
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    ScenePackageApplyDesc package_apply;
    package_apply.package_index_path = request.package_index_path;
    package_apply.output_path = request.output_scene_path;
    package_apply.source_revision = request.source_revision;
    package_apply.scene_asset = prepared->scene_asset;
    package_apply.scene = std::move(prepared->scene);
    package_apply.mesh_dependencies = prepared->mesh_dependencies;
    package_apply.material_dependencies = prepared->material_dependencies;
    package_apply.sprite_dependencies = prepared->sprite_dependencies;

    const auto package_result = apply_scene_package_update(filesystem, package_apply);
    input_result.scene_v1_content = package_result.scene_content;
    input_result.package_index_content = package_result.package_index_content;
    for (const auto& file : package_result.changed_files) {
        append_changed_file(input_result.changed_files, file, request);
    }
    append_model_mutation(input_result.model_mutations, request, std::move(prepared->placement_rows),
                          std::move(prepared->dependency_rows));
    if (!package_result.succeeded()) {
        append_scene_package_failures(input_result.diagnostics, package_result.failures);
        sort_diagnostics(input_result.diagnostics);
    }
    return input_result;
}

} // namespace mirakana
