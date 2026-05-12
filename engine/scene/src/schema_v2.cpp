// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene/schema_v2.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

constexpr std::size_t max_document_row_count = 65536U;

[[nodiscard]] bool is_valid_authoring_id(std::string_view value) noexcept {
    if (value.empty()) {
        return false;
    }

    return std::ranges::all_of(value, [](const auto character) {
        const auto byte = static_cast<unsigned char>(character);
        return std::iscntrl(byte) == 0 && std::isspace(byte) == 0;
    });
}

[[nodiscard]] bool is_line_text_value(std::string_view value) noexcept {
    return std::ranges::all_of(
        value, [](const auto character) { return std::iscntrl(static_cast<unsigned char>(character)) == 0; });
}

[[nodiscard]] bool is_valid_component_type(std::string_view value) noexcept {
    static constexpr std::array<std::string_view, 12> supported_types = {
        "transform3d", "camera",       "light",      "mesh_renderer", "sprite_renderer", "tilemap",
        "audio_cue",   "rigid_body2d", "collider2d", "nav_agent",     "animation",       "ui_attachment",
    };

    return std::ranges::any_of(supported_types, [value](const auto supported) { return value == supported; });
}

void add_diagnostic(std::vector<SceneSchemaV2Diagnostic>& diagnostics, SceneSchemaV2DiagnosticCode code,
                    AuthoringId node = {}, AuthoringId component = {}, SceneComponentTypeId component_type = {},
                    std::string property = {}) {
    diagnostics.push_back(SceneSchemaV2Diagnostic{
        .code = code,
        .node = std::move(node),
        .component = std::move(component),
        .component_type = std::move(component_type),
        .property = std::move(property),
    });
}

void throw_if_invalid(const SceneDocumentV2& scene, std::string_view message) {
    if (!validate_scene_document_v2(scene).empty()) {
        throw std::invalid_argument(std::string(message));
    }
}

[[nodiscard]] std::string format_vec3(Vec3 value) {
    std::ostringstream output;
    output << value.x << ' ' << value.y << ' ' << value.z;
    return output.str();
}

[[nodiscard]] float parse_float(std::string_view value, std::string_view label) {
    const auto text = std::string(value);
    std::size_t consumed = 0;
    const auto parsed = std::stof(text, &consumed);
    if (consumed != text.size() || !std::isfinite(parsed)) {
        throw std::invalid_argument(std::string(label) + " is invalid");
    }
    return parsed;
}

[[nodiscard]] bool is_finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool is_valid_transform(Transform3D transform) noexcept {
    return is_finite_vec3(transform.position) && is_finite_vec3(transform.rotation_radians) &&
           is_finite_vec3(transform.scale) && transform.scale.x != 0.0F && transform.scale.y != 0.0F &&
           transform.scale.z != 0.0F;
}

[[nodiscard]] Vec3 parse_vec3(std::string_view value, std::string_view label) {
    const auto first = value.find(' ');
    const auto second = first == std::string_view::npos ? std::string_view::npos : value.find(' ', first + 1U);
    if (first == std::string_view::npos || second == std::string_view::npos ||
        value.find(' ', second + 1U) != std::string_view::npos) {
        throw std::invalid_argument(std::string(label) + " must contain three space separated values");
    }

    return Vec3{
        .x = parse_float(value.substr(0, first), label),
        .y = parse_float(value.substr(first + 1U, second - first - 1U), label),
        .z = parse_float(value.substr(second + 1U), label),
    };
}

[[nodiscard]] std::size_t parse_index(std::string_view value, std::string_view label) {
    if (value.empty() || value.front() == '-') {
        throw std::invalid_argument(std::string(label) + " is invalid");
    }

    const auto text = std::string(value);
    std::size_t consumed = 0;
    const auto parsed = std::stoull(text, &consumed, 10);
    if (consumed != text.size() || parsed > std::numeric_limits<std::size_t>::max() ||
        parsed >= static_cast<unsigned long long>(max_document_row_count)) {
        throw std::invalid_argument(std::string(label) + " is invalid");
    }
    return static_cast<std::size_t>(parsed);
}

template <typename T> T& ensure_index(std::vector<T>& values, std::size_t index) {
    if (index >= max_document_row_count) {
        throw std::invalid_argument("scene v2 document index is invalid");
    }
    if (values.size() <= index) {
        values.resize(index + 1U);
    }
    return values[index];
}

struct PendingProperty {
    std::string name;
    std::string value;
    bool has_name{false};
    bool has_value{false};
};

void assign_node_field(SceneNodeDocumentV2& node, std::string_view field, std::string_view value) {
    if (field == "id") {
        node.id.value = std::string(value);
    } else if (field == "name") {
        node.name = std::string(value);
    } else if (field == "parent") {
        node.parent.value = std::string(value);
    } else if (field == "position") {
        node.transform.position = parse_vec3(value, "scene v2 node position");
    } else if (field == "rotation") {
        node.transform.rotation_radians = parse_vec3(value, "scene v2 node rotation");
    } else if (field == "scale") {
        node.transform.scale = parse_vec3(value, "scene v2 node scale");
    } else {
        throw std::invalid_argument("unknown scene v2 node field");
    }
}

void assign_component_field(SceneComponentDocumentV2& component, std::string_view field, std::string_view value,
                            std::unordered_map<std::size_t, PendingProperty>& pending_properties) {
    if (field == "id") {
        component.id.value = std::string(value);
    } else if (field == "node") {
        component.node.value = std::string(value);
    } else if (field == "type") {
        component.type.value = std::string(value);
    } else if (field.starts_with("property.")) {
        const auto after_prefix = field.substr(9U);
        const auto separator = after_prefix.find('.');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("scene v2 component property key must include a field");
        }
        const auto property_index = parse_index(after_prefix.substr(0, separator), "scene v2 component property index");
        auto& property = pending_properties[property_index];
        const auto property_field = after_prefix.substr(separator + 1U);
        if (property_field == "name") {
            property.name = std::string(value);
            property.has_name = true;
        } else if (property_field == "value") {
            property.value = std::string(value);
            property.has_value = true;
        } else {
            throw std::invalid_argument("unknown scene v2 component property field");
        }
    } else {
        throw std::invalid_argument("unknown scene v2 component field");
    }
}

[[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix) noexcept {
    return value.starts_with(prefix);
}

[[nodiscard]] bool ends_with(std::string_view value, std::string_view suffix) noexcept {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

[[nodiscard]] SceneNodeDocumentV2* find_node(SceneDocumentV2& scene, std::string_view id) noexcept {
    for (auto& node : scene.nodes) {
        if (node.id.value == id) {
            return &node;
        }
    }
    return nullptr;
}

[[nodiscard]] SceneComponentDocumentV2* find_component(SceneDocumentV2& scene, std::string_view id,
                                                       std::string_view node_id) noexcept {
    for (auto& component : scene.components) {
        if (component.id.value == id && component.node.value == node_id) {
            return &component;
        }
    }
    return nullptr;
}

[[nodiscard]] SceneComponentPropertyV2* find_property(SceneComponentDocumentV2& component,
                                                      std::string_view name) noexcept {
    for (auto& property : component.properties) {
        if (property.name == name) {
            return &property;
        }
    }
    return nullptr;
}

void add_override_diagnostic(std::vector<SceneSchemaV2Diagnostic>& diagnostics, SceneSchemaV2DiagnosticCode code,
                             std::string_view path) {
    add_diagnostic(diagnostics, code, {}, {}, {}, std::string(path));
}

[[nodiscard]] bool apply_node_field_override(SceneDocumentV2& scene, std::string_view path, std::string_view suffix,
                                             std::string_view value) {
    constexpr std::string_view prefix = "nodes/";
    const auto node_id = path.substr(prefix.size(), path.size() - prefix.size() - suffix.size());
    auto* node = find_node(scene, node_id);
    if (node == nullptr) {
        return false;
    }

    if (suffix == "/name") {
        node->name = std::string(value);
    } else if (suffix == "/parent") {
        node->parent.value = std::string(value);
    } else {
        return false;
    }
    return true;
}

[[nodiscard]] bool apply_component_property_override(SceneDocumentV2& scene, std::string_view path,
                                                     std::string_view value) {
    constexpr std::string_view prefix = "nodes/";
    constexpr std::string_view component_marker = "/components/";
    constexpr std::string_view property_marker = "/properties/";

    if (!starts_with(path, prefix)) {
        return false;
    }

    const auto component_marker_position = path.find(component_marker, prefix.size());
    if (component_marker_position == std::string_view::npos) {
        return false;
    }
    const auto property_marker_position =
        path.find(property_marker, component_marker_position + component_marker.size());
    if (property_marker_position == std::string_view::npos) {
        return false;
    }

    const auto node_id = path.substr(prefix.size(), component_marker_position - prefix.size());
    const auto component_id =
        path.substr(component_marker_position + component_marker.size(),
                    property_marker_position - component_marker_position - component_marker.size());
    const auto property_name = path.substr(property_marker_position + property_marker.size());

    auto* component = find_component(scene, component_id, node_id);
    if (component == nullptr) {
        return false;
    }

    auto* property = find_property(*component, property_name);
    if (property == nullptr) {
        return false;
    }

    property->value = std::string(value);
    return true;
}

} // namespace

std::vector<SceneSchemaV2Diagnostic> validate_scene_document_v2(const SceneDocumentV2& scene) {
    std::vector<SceneSchemaV2Diagnostic> diagnostics;

    if (scene.name.empty()) {
        add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::invalid_scene_name);
    } else if (!is_line_text_value(scene.name)) {
        add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::invalid_text_value, {}, {}, {}, "scene.name");
    }

    std::unordered_set<std::string> node_ids;
    for (const auto& node : scene.nodes) {
        if (!is_valid_authoring_id(node.id.value)) {
            add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::invalid_authoring_id, node.id);
            continue;
        }
        if (!is_line_text_value(node.name)) {
            add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::invalid_text_value, node.id, {}, {}, "node.name");
        }
        if (!is_valid_transform(node.transform)) {
            add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::invalid_transform, node.id);
        }
        if (!node_ids.insert(node.id.value).second) {
            add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::duplicate_node_id, node.id);
        }
    }

    std::unordered_set<std::string> component_ids;
    for (const auto& component : scene.components) {
        if (!is_valid_authoring_id(component.id.value)) {
            add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::invalid_authoring_id, component.node, component.id,
                           component.type);
            continue;
        }
        if (!component_ids.insert(component.id.value).second) {
            add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::duplicate_component_id, component.node,
                           component.id, component.type);
        }
    }

    for (const auto& node : scene.nodes) {
        if (!node.parent.value.empty() && node_ids.find(node.parent.value) == node_ids.end()) {
            add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::missing_parent_node, node.id);
        }
    }

    for (const auto& component : scene.components) {
        if (node_ids.find(component.node.value) == node_ids.end()) {
            add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::missing_component_node, component.node,
                           component.id, component.type);
        }
        if (!is_valid_component_type(component.type.value)) {
            add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::invalid_component_type, component.node,
                           component.id, component.type);
        }

        std::unordered_set<std::string> property_names;
        for (const auto& property : component.properties) {
            if (property.name.empty() || !is_line_text_value(property.name) || !is_line_text_value(property.value)) {
                add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::invalid_component_property, component.node,
                               component.id, component.type, property.name);
                continue;
            }
            if (!property_names.insert(property.name).second) {
                add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::duplicate_component_property, component.node,
                               component.id, component.type, property.name);
            }
        }
    }

    return diagnostics;
}

std::string serialize_scene_document_v2(const SceneDocumentV2& scene) {
    throw_if_invalid(scene, "scene v2 document is invalid");

    std::ostringstream output;
    output << "format=GameEngine.Scene.v2\n";
    output << "scene.name=" << scene.name << '\n';

    for (std::size_t index = 0; index < scene.nodes.size(); ++index) {
        const auto& node = scene.nodes[index];
        output << "node." << index << ".id=" << node.id.value << '\n';
        output << "node." << index << ".name=" << node.name << '\n';
        output << "node." << index << ".parent=" << node.parent.value << '\n';
        output << "node." << index << ".position=" << format_vec3(node.transform.position) << '\n';
        output << "node." << index << ".rotation=" << format_vec3(node.transform.rotation_radians) << '\n';
        output << "node." << index << ".scale=" << format_vec3(node.transform.scale) << '\n';
    }

    for (std::size_t index = 0; index < scene.components.size(); ++index) {
        const auto& component = scene.components[index];
        output << "component." << index << ".id=" << component.id.value << '\n';
        output << "component." << index << ".node=" << component.node.value << '\n';
        output << "component." << index << ".type=" << component.type.value << '\n';

        auto properties = component.properties;
        std::ranges::sort(properties, [](const auto& lhs, const auto& rhs) { return lhs.name < rhs.name; });
        for (std::size_t property_index = 0; property_index < properties.size(); ++property_index) {
            output << "component." << index << ".property." << property_index
                   << ".name=" << properties[property_index].name << '\n';
            output << "component." << index << ".property." << property_index
                   << ".value=" << properties[property_index].value << '\n';
        }
    }

    return output.str();
}

SceneDocumentV2 deserialize_scene_document_v2(std::string_view text) {
    bool has_format = false;
    SceneDocumentV2 scene;
    std::vector<std::unordered_map<std::size_t, PendingProperty>> pending_component_properties;

    std::istringstream input{std::string(text)};
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::invalid_argument("scene v2 line must contain '='");
        }

        const auto key = std::string_view(line).substr(0, separator);
        const auto value = std::string_view(line).substr(separator + 1U);
        if (key == "format") {
            if (value != "GameEngine.Scene.v2") {
                throw std::invalid_argument("unsupported scene v2 format");
            }
            has_format = true;
            continue;
        }
        if (key == "scene.name") {
            scene.name = std::string(value);
            continue;
        }
        if (key.starts_with("node.")) {
            const auto after_prefix = key.substr(5U);
            const auto field_separator = after_prefix.find('.');
            if (field_separator == std::string_view::npos) {
                throw std::invalid_argument("scene v2 node key must include a field");
            }
            const auto index = parse_index(after_prefix.substr(0, field_separator), "scene v2 node index");
            auto& node = ensure_index(scene.nodes, index);
            assign_node_field(node, after_prefix.substr(field_separator + 1U), value);
            continue;
        }
        if (key.starts_with("component.")) {
            const auto after_prefix = key.substr(10U);
            const auto field_separator = after_prefix.find('.');
            if (field_separator == std::string_view::npos) {
                throw std::invalid_argument("scene v2 component key must include a field");
            }
            const auto index = parse_index(after_prefix.substr(0, field_separator), "scene v2 component index");
            auto& component = ensure_index(scene.components, index);
            auto& pending_properties = ensure_index(pending_component_properties, index);
            assign_component_field(component, after_prefix.substr(field_separator + 1U), value, pending_properties);
            continue;
        }

        throw std::invalid_argument("unknown scene v2 key");
    }

    if (!has_format) {
        throw std::invalid_argument("scene v2 format is missing");
    }

    for (std::size_t component_index = 0; component_index < pending_component_properties.size(); ++component_index) {
        auto& component = ensure_index(scene.components, component_index);
        auto& pending_properties = pending_component_properties[component_index];
        std::vector<std::size_t> property_indexes;
        property_indexes.reserve(pending_properties.size());
        for (const auto& [index, property] : pending_properties) {
            if (!property.has_name || !property.has_value) {
                throw std::invalid_argument("scene v2 component property is incomplete");
            }
            property_indexes.push_back(index);
        }
        std::ranges::sort(property_indexes);
        for (const auto index : property_indexes) {
            const auto& property = pending_properties[index];
            component.properties.push_back(SceneComponentPropertyV2{.name = property.name, .value = property.value});
        }
    }

    throw_if_invalid(scene, "scene v2 document is invalid");
    return scene;
}

std::vector<SceneSchemaV2Diagnostic> validate_prefab_document_v2(const PrefabDocumentV2& prefab) {
    auto diagnostics = validate_scene_document_v2(prefab.scene);
    if (prefab.name.empty()) {
        add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::invalid_scene_name);
    } else if (!is_line_text_value(prefab.name)) {
        add_diagnostic(diagnostics, SceneSchemaV2DiagnosticCode::invalid_text_value, {}, {}, {}, "prefab.name");
    }
    return diagnostics;
}

std::string serialize_prefab_document_v2(const PrefabDocumentV2& prefab) {
    if (!validate_prefab_document_v2(prefab).empty()) {
        throw std::invalid_argument("prefab v2 document is invalid");
    }

    constexpr std::string_view scene_format = "format=GameEngine.Scene.v2\n";
    auto scene_text = serialize_scene_document_v2(prefab.scene);
    if (!starts_with(scene_text, scene_format)) {
        throw std::invalid_argument("scene v2 serializer returned an unsupported format");
    }

    std::ostringstream output;
    output << "format=GameEngine.Prefab.v2\n";
    output << "prefab.name=" << prefab.name << '\n';
    output << std::string_view(scene_text).substr(scene_format.size());
    return output.str();
}

PrefabDocumentV2 deserialize_prefab_document_v2(std::string_view text) {
    bool has_format = false;
    PrefabDocumentV2 prefab;
    std::ostringstream scene_text;
    scene_text << "format=GameEngine.Scene.v2\n";

    std::istringstream input{std::string(text)};
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::invalid_argument("prefab v2 line must contain '='");
        }

        const auto key = std::string_view(line).substr(0, separator);
        const auto value = std::string_view(line).substr(separator + 1U);
        if (key == "format") {
            if (value != "GameEngine.Prefab.v2") {
                throw std::invalid_argument("unsupported prefab v2 format");
            }
            has_format = true;
            continue;
        }
        if (key == "prefab.name") {
            prefab.name = std::string(value);
            continue;
        }
        if (key == "scene.name" || key.starts_with("node.") || key.starts_with("component.")) {
            scene_text << line << '\n';
            continue;
        }

        throw std::invalid_argument("unknown prefab v2 key");
    }

    if (!has_format) {
        throw std::invalid_argument("prefab v2 format is missing");
    }

    prefab.scene = deserialize_scene_document_v2(scene_text.str());
    if (!validate_prefab_document_v2(prefab).empty()) {
        throw std::invalid_argument("prefab v2 document is invalid");
    }
    return prefab;
}

PrefabVariantComposeResultV2 compose_prefab_variant_v2(const PrefabVariantDocumentV2& variant) {
    PrefabVariantComposeResultV2 result;
    result.prefab = variant.base_prefab;
    if (!variant.name.empty()) {
        result.prefab.name = variant.name;
    }

    result.diagnostics = validate_scene_document_v2(result.prefab.scene);
    std::unordered_set<std::string> override_paths;
    for (const auto& override : variant.overrides) {
        if (!override_paths.insert(override.path.value).second) {
            add_override_diagnostic(result.diagnostics, SceneSchemaV2DiagnosticCode::duplicate_override_path,
                                    override.path.value);
        }
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    for (const auto& override : variant.overrides) {
        const auto path = std::string_view(override.path.value);
        const auto value = std::string_view(override.value);
        bool applied = false;
        if (starts_with(path, "nodes/") && ends_with(path, "/name")) {
            applied = apply_node_field_override(result.prefab.scene, path, "/name", value);
        } else if (starts_with(path, "nodes/") && ends_with(path, "/parent")) {
            applied = apply_node_field_override(result.prefab.scene, path, "/parent", value);
        } else {
            applied = apply_component_property_override(result.prefab.scene, path, value);
        }

        if (!applied) {
            add_override_diagnostic(result.diagnostics, SceneSchemaV2DiagnosticCode::missing_override_target, path);
        }
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    result.diagnostics = validate_scene_document_v2(result.prefab.scene);
    result.success = result.diagnostics.empty();
    return result;
}

} // namespace mirakana
