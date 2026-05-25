// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene/schema.hpp"

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

void add_diagnostic(std::vector<SceneSchemaDiagnostic>& diagnostics, SceneSchemaDiagnosticCode code,
                    AuthoringId node = {}, AuthoringId component = {}, SceneComponentTypeId component_type = {},
                    std::string property = {}) {
    diagnostics.push_back(SceneSchemaDiagnostic{
        .code = code,
        .node = std::move(node),
        .component = std::move(component),
        .component_type = std::move(component_type),
        .property = std::move(property),
    });
}

void throw_if_invalid(const SceneDocument& scene, std::string_view message) {
    if (!validate_scene_document(scene).empty()) {
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

struct PendingPrefabSource {
    std::string prefab_path;
    AuthoringId source_id;
    bool has_prefab_path{false};
    bool has_source_id{false};
};

void assign_node_field(SceneNodeDocument& node, std::string_view field, std::string_view value,
                       PendingPrefabSource& pending_prefab_source) {
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
    } else if (field == "prefab_source.prefab_path") {
        pending_prefab_source.prefab_path = std::string(value);
        pending_prefab_source.has_prefab_path = true;
    } else if (field == "prefab_source.source_node_id") {
        pending_prefab_source.source_id.value = std::string(value);
        pending_prefab_source.has_source_id = true;
    } else {
        throw std::invalid_argument("unknown scene v2 node field");
    }
}

void assign_component_field(SceneComponentDocument& component, std::string_view field, std::string_view value,
                            std::unordered_map<std::size_t, PendingProperty>& pending_properties,
                            PendingPrefabSource& pending_prefab_source) {
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
    } else if (field == "prefab_source.prefab_path") {
        pending_prefab_source.prefab_path = std::string(value);
        pending_prefab_source.has_prefab_path = true;
    } else if (field == "prefab_source.source_component_id") {
        pending_prefab_source.source_id.value = std::string(value);
        pending_prefab_source.has_source_id = true;
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

[[nodiscard]] SceneNodeDocument* find_node(SceneDocument& scene, std::string_view id) noexcept {
    for (auto& node : scene.nodes) {
        if (node.id.value == id) {
            return &node;
        }
    }
    return nullptr;
}

[[nodiscard]] const SceneNodeDocument* find_node_by_id(const SceneDocument& scene, std::string_view node_id) noexcept {
    const auto it = std::ranges::find_if(scene.nodes, [node_id](const auto& node) { return node.id.value == node_id; });
    return it == scene.nodes.end() ? nullptr : &*it;
}

[[nodiscard]] SceneComponentDocument* find_component(SceneDocument& scene, std::string_view id,
                                                     std::string_view node_id) noexcept {
    for (auto& component : scene.components) {
        if (component.id.value == id && component.node.value == node_id) {
            return &component;
        }
    }
    return nullptr;
}

[[nodiscard]] SceneComponentProperty* find_property(SceneComponentDocument& component, std::string_view name) noexcept {
    for (auto& property : component.properties) {
        if (property.name == name) {
            return &property;
        }
    }
    return nullptr;
}

[[nodiscard]] const SceneNodePrefabSource* find_node_prefab_source(const SceneDocument& scene,
                                                                   std::string_view node_id) noexcept {
    const auto it = std::ranges::find_if(scene.node_prefab_sources,
                                         [node_id](const auto& source) { return source.node.value == node_id; });
    return it == scene.node_prefab_sources.end() ? nullptr : &*it;
}

[[nodiscard]] const SceneComponentPrefabSource* find_component_prefab_source(const SceneDocument& scene,
                                                                             std::string_view component_id) noexcept {
    const auto it = std::ranges::find_if(scene.component_prefab_sources, [component_id](const auto& source) {
        return source.component.value == component_id;
    });
    return it == scene.component_prefab_sources.end() ? nullptr : &*it;
}

[[nodiscard]] const SceneComponentDocument* find_component_by_id(const SceneDocument& scene,
                                                                 std::string_view component_id) noexcept {
    const auto it = std::ranges::find_if(
        scene.components, [component_id](const auto& component) { return component.id.value == component_id; });
    return it == scene.components.end() ? nullptr : &*it;
}

[[nodiscard]] std::unordered_set<std::string> collect_scene_subtree_node_ids(const SceneDocument& scene,
                                                                             std::string_view root_node_id) {
    std::unordered_set<std::string> subtree;
    subtree.insert(std::string(root_node_id));

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& node : scene.nodes) {
            if (!node.parent.value.empty() && subtree.contains(node.parent.value) &&
                subtree.insert(node.id.value).second) {
                changed = true;
            }
        }
    }

    return subtree;
}

[[nodiscard]] std::unordered_set<std::string> collect_prefab_node_ids(const PrefabDocument& prefab) {
    std::unordered_set<std::string> node_ids;
    for (const auto& node : prefab.scene.nodes) {
        node_ids.insert(node.id.value);
    }
    return node_ids;
}

[[nodiscard]] std::unordered_map<std::string, const SceneComponentDocument*>
collect_prefab_components_by_id(const PrefabDocument& prefab) {
    std::unordered_map<std::string, const SceneComponentDocument*> components_by_id;
    for (const auto& component : prefab.scene.components) {
        components_by_id.emplace(component.id.value, &component);
    }
    return components_by_id;
}

[[nodiscard]] AuthoringId make_scene_prefab_refresh_added_id(const AuthoringId& instance_root_node,
                                                             const AuthoringId& source_id) {
    return AuthoringId{.value = instance_root_node.value + "/refresh/" + source_id.value};
}

void add_scene_prefab_instance_refresh_row(ScenePrefabInstanceRefreshPlan& plan, ScenePrefabInstanceRefreshRow row) {
    switch (row.kind) {
    case ScenePrefabInstanceRefreshRowKind::preserve_node:
        ++plan.preserve_node_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::add_source_node:
        ++plan.add_node_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::remove_stale_node:
        ++plan.remove_node_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::preserve_component:
        ++plan.preserve_component_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::add_source_component:
        ++plan.add_component_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::remove_stale_component:
        ++plan.remove_component_count;
        break;
    }

    plan.rows.push_back(std::move(row));
}

void add_prefab_refresh_source_identity_diagnostics(ScenePrefabInstanceRefreshPlan& plan, const SceneDocument& scene,
                                                    const std::unordered_set<std::string>& subtree_node_ids) {
    std::unordered_set<std::string> source_node_ids;
    for (const auto& source : scene.node_prefab_sources) {
        if (source.prefab_path != plan.prefab_path || !subtree_node_ids.contains(source.node.value)) {
            continue;
        }
        if (!source_node_ids.insert(source.source_node_id.value).second) {
            add_diagnostic(plan.diagnostics, SceneSchemaDiagnosticCode::duplicate_prefab_source_identity, source.node,
                           {}, {}, "node.prefab_source.source_node_id");
        }
    }

    std::unordered_set<std::string> source_component_ids;
    for (const auto& source : scene.component_prefab_sources) {
        if (source.prefab_path != plan.prefab_path) {
            continue;
        }
        const auto* component = find_component_by_id(scene, source.component.value);
        if (component == nullptr || !subtree_node_ids.contains(component->node.value)) {
            continue;
        }
        if (!source_component_ids.insert(source.source_component_id.value).second) {
            add_diagnostic(plan.diagnostics, SceneSchemaDiagnosticCode::duplicate_prefab_source_identity, {},
                           source.component, {}, "component.prefab_source.source_component_id");
        }
    }
}

void add_prefab_refresh_nested_prefab_diagnostics(ScenePrefabInstanceRefreshPlan& plan, const SceneDocument& scene,
                                                  const std::unordered_set<std::string>& subtree_node_ids) {
    for (const auto& source : scene.node_prefab_sources) {
        if (!subtree_node_ids.contains(source.node.value) || source.prefab_path == plan.prefab_path) {
            continue;
        }
        add_diagnostic(plan.diagnostics, SceneSchemaDiagnosticCode::unsupported_nested_prefab_instance, source.node, {},
                       {}, "node.prefab_source.prefab_path");
    }
}

void add_prefab_refresh_local_ownership_diagnostics(ScenePrefabInstanceRefreshPlan& plan, const SceneDocument& scene,
                                                    const std::unordered_set<std::string>& subtree_node_ids) {
    for (const auto& node : scene.nodes) {
        if (!subtree_node_ids.contains(node.id.value) || node.id.value == plan.instance_root_node.value) {
            continue;
        }
        const auto* source = find_node_prefab_source(scene, node.id.value);
        if (source == nullptr) {
            add_diagnostic(plan.diagnostics, SceneSchemaDiagnosticCode::unsupported_local_prefab_child, node.id, {}, {},
                           "node.prefab_source");
        }
    }

    for (const auto& component : scene.components) {
        if (!subtree_node_ids.contains(component.node.value)) {
            continue;
        }
        const auto* source = find_component_prefab_source(scene, component.id.value);
        if (source == nullptr || source->prefab_path != plan.prefab_path) {
            add_diagnostic(plan.diagnostics, SceneSchemaDiagnosticCode::unsupported_local_prefab_component,
                           component.node, component.id, component.type, "component.prefab_source");
        }
    }
}

void add_override_diagnostic(std::vector<SceneSchemaDiagnostic>& diagnostics, SceneSchemaDiagnosticCode code,
                             std::string_view path) {
    add_diagnostic(diagnostics, code, {}, {}, {}, std::string(path));
}

[[nodiscard]] bool apply_node_field_override(SceneDocument& scene, std::string_view path, std::string_view suffix,
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

[[nodiscard]] bool apply_component_property_override(SceneDocument& scene, std::string_view path,
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

std::vector<SceneSchemaDiagnostic> validate_scene_document(const SceneDocument& scene) {
    std::vector<SceneSchemaDiagnostic> diagnostics;

    if (scene.name.empty()) {
        add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_scene_name);
    } else if (!is_line_text_value(scene.name)) {
        add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_text_value, {}, {}, {}, "scene.name");
    }

    std::unordered_set<std::string> node_ids;
    for (const auto& node : scene.nodes) {
        if (!is_valid_authoring_id(node.id.value)) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_authoring_id, node.id);
            continue;
        }
        if (!is_line_text_value(node.name)) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_text_value, node.id, {}, {}, "node.name");
        }
        if (!is_valid_transform(node.transform)) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_transform, node.id);
        }
        if (!node_ids.insert(node.id.value).second) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::duplicate_node_id, node.id);
        }
    }

    std::unordered_set<std::string> component_ids;
    for (const auto& component : scene.components) {
        if (!is_valid_authoring_id(component.id.value)) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_authoring_id, component.node, component.id,
                           component.type);
            continue;
        }
        if (!component_ids.insert(component.id.value).second) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::duplicate_component_id, component.node, component.id,
                           component.type);
        }
    }

    std::unordered_set<std::string> node_prefab_source_targets;
    for (const auto& source : scene.node_prefab_sources) {
        if (!node_prefab_source_targets.insert(source.node.value).second) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::duplicate_override_path, source.node, {}, {},
                           "node.prefab_source");
        }
        if (node_ids.find(source.node.value) == node_ids.end()) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::missing_override_target, source.node, {}, {},
                           "node.prefab_source");
        }
        if (source.prefab_path.empty() || !is_line_text_value(source.prefab_path)) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_text_value, source.node, {}, {},
                           "node.prefab_source.prefab_path");
        }
        if (!is_valid_authoring_id(source.source_node_id.value)) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_authoring_id, source.node, {}, {},
                           "node.prefab_source.source_node_id");
        }
    }

    std::unordered_set<std::string> component_prefab_source_targets;
    for (const auto& source : scene.component_prefab_sources) {
        if (!component_prefab_source_targets.insert(source.component.value).second) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::duplicate_override_path, {}, source.component, {},
                           "component.prefab_source");
        }
        if (component_ids.find(source.component.value) == component_ids.end()) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::missing_override_target, {}, source.component, {},
                           "component.prefab_source");
        }
        if (source.prefab_path.empty() || !is_line_text_value(source.prefab_path)) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_text_value, {}, source.component, {},
                           "component.prefab_source.prefab_path");
        }
        if (!is_valid_authoring_id(source.source_component_id.value)) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_authoring_id, {}, source.component, {},
                           "component.prefab_source.source_component_id");
        }
    }

    for (const auto& node : scene.nodes) {
        if (!node.parent.value.empty() && node_ids.find(node.parent.value) == node_ids.end()) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::missing_parent_node, node.id);
        }
    }

    for (const auto& component : scene.components) {
        if (node_ids.find(component.node.value) == node_ids.end()) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::missing_component_node, component.node, component.id,
                           component.type);
        }
        if (!is_valid_component_type(component.type.value)) {
            add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_component_type, component.node, component.id,
                           component.type);
        }

        std::unordered_set<std::string> property_names;
        for (const auto& property : component.properties) {
            if (property.name.empty() || !is_line_text_value(property.name) || !is_line_text_value(property.value)) {
                add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_component_property, component.node,
                               component.id, component.type, property.name);
                continue;
            }
            if (!property_names.insert(property.name).second) {
                add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::duplicate_component_property, component.node,
                               component.id, component.type, property.name);
            }
        }
    }

    return diagnostics;
}

std::string serialize_scene_document(const SceneDocument& scene) {
    throw_if_invalid(scene, "scene v2 document is invalid");

    std::ostringstream output;
    output << "format=GameEngine.Scene\n";
    output << "scene.name=" << scene.name << '\n';

    for (std::size_t index = 0; index < scene.nodes.size(); ++index) {
        const auto& node = scene.nodes[index];
        output << "node." << index << ".id=" << node.id.value << '\n';
        output << "node." << index << ".name=" << node.name << '\n';
        output << "node." << index << ".parent=" << node.parent.value << '\n';
        output << "node." << index << ".position=" << format_vec3(node.transform.position) << '\n';
        output << "node." << index << ".rotation=" << format_vec3(node.transform.rotation_radians) << '\n';
        output << "node." << index << ".scale=" << format_vec3(node.transform.scale) << '\n';
        if (const auto* source = find_node_prefab_source(scene, node.id.value); source != nullptr) {
            output << "node." << index << ".prefab_source.prefab_path=" << source->prefab_path << '\n';
            output << "node." << index << ".prefab_source.source_node_id=" << source->source_node_id.value << '\n';
        }
    }

    for (std::size_t index = 0; index < scene.components.size(); ++index) {
        const auto& component = scene.components[index];
        output << "component." << index << ".id=" << component.id.value << '\n';
        output << "component." << index << ".node=" << component.node.value << '\n';
        output << "component." << index << ".type=" << component.type.value << '\n';
        if (const auto* source = find_component_prefab_source(scene, component.id.value); source != nullptr) {
            output << "component." << index << ".prefab_source.prefab_path=" << source->prefab_path << '\n';
            output << "component." << index
                   << ".prefab_source.source_component_id=" << source->source_component_id.value << '\n';
        }

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

SceneDocument deserialize_scene_document(std::string_view text) {
    bool has_format = false;
    SceneDocument scene;
    std::vector<std::unordered_map<std::size_t, PendingProperty>> pending_component_properties;
    std::vector<PendingPrefabSource> pending_node_prefab_sources;
    std::vector<PendingPrefabSource> pending_component_prefab_sources;

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
            if (value != "GameEngine.Scene") {
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
            auto& pending_prefab_source = ensure_index(pending_node_prefab_sources, index);
            assign_node_field(node, after_prefix.substr(field_separator + 1U), value, pending_prefab_source);
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
            auto& pending_prefab_source = ensure_index(pending_component_prefab_sources, index);
            assign_component_field(component, after_prefix.substr(field_separator + 1U), value, pending_properties,
                                   pending_prefab_source);
            continue;
        }

        throw std::invalid_argument("unknown scene v2 key");
    }

    if (!has_format) {
        throw std::invalid_argument("scene v2 format is missing");
    }

    for (std::size_t node_index = 0; node_index < pending_node_prefab_sources.size(); ++node_index) {
        const auto& pending_source = pending_node_prefab_sources[node_index];
        if (pending_source.has_prefab_path != pending_source.has_source_id) {
            throw std::invalid_argument("scene v2 node prefab source is incomplete");
        }
        if (!pending_source.has_prefab_path) {
            continue;
        }
        auto& node = ensure_index(scene.nodes, node_index);
        scene.node_prefab_sources.push_back(SceneNodePrefabSource{
            .node = node.id,
            .prefab_path = pending_source.prefab_path,
            .source_node_id = pending_source.source_id,
        });
    }

    for (std::size_t component_index = 0; component_index < pending_component_prefab_sources.size();
         ++component_index) {
        const auto& pending_source = pending_component_prefab_sources[component_index];
        if (pending_source.has_prefab_path != pending_source.has_source_id) {
            throw std::invalid_argument("scene v2 component prefab source is incomplete");
        }
        if (!pending_source.has_prefab_path) {
            continue;
        }
        auto& component = ensure_index(scene.components, component_index);
        scene.component_prefab_sources.push_back(SceneComponentPrefabSource{
            .component = component.id,
            .prefab_path = pending_source.prefab_path,
            .source_component_id = pending_source.source_id,
        });
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
            component.properties.push_back(SceneComponentProperty{.name = property.name, .value = property.value});
        }
    }

    throw_if_invalid(scene, "scene v2 document is invalid");
    return scene;
}

std::vector<SceneSchemaDiagnostic> validate_prefab_document(const PrefabDocument& prefab) {
    auto diagnostics = validate_scene_document(prefab.scene);
    if (prefab.name.empty()) {
        add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_scene_name);
    } else if (!is_line_text_value(prefab.name)) {
        add_diagnostic(diagnostics, SceneSchemaDiagnosticCode::invalid_text_value, {}, {}, {}, "prefab.name");
    }
    return diagnostics;
}

std::string serialize_prefab_document(const PrefabDocument& prefab) {
    if (!validate_prefab_document(prefab).empty()) {
        throw std::invalid_argument("prefab v2 document is invalid");
    }

    constexpr std::string_view scene_format = "format=GameEngine.Scene\n";
    auto scene_text = serialize_scene_document(prefab.scene);
    if (!starts_with(scene_text, scene_format)) {
        throw std::invalid_argument("scene v2 serializer returned an unsupported format");
    }

    std::ostringstream output;
    output << "format=GameEngine.Prefab\n";
    output << "prefab.name=" << prefab.name << '\n';
    output << std::string_view(scene_text).substr(scene_format.size());
    return output.str();
}

PrefabDocument deserialize_prefab_document(std::string_view text) {
    bool has_format = false;
    PrefabDocument prefab;
    std::ostringstream scene_text;
    scene_text << "format=GameEngine.Scene\n";

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
            if (value != "GameEngine.Prefab") {
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

    prefab.scene = deserialize_scene_document(scene_text.str());
    if (!validate_prefab_document(prefab).empty()) {
        throw std::invalid_argument("prefab v2 document is invalid");
    }
    return prefab;
}

PrefabVariantComposeResult compose_prefab_variant(const PrefabVariantDocument& variant) {
    PrefabVariantComposeResult result;
    result.prefab = variant.base_prefab;
    if (!variant.name.empty()) {
        result.prefab.name = variant.name;
    }

    result.diagnostics = validate_scene_document(result.prefab.scene);
    std::unordered_set<std::string> override_paths;
    for (const auto& override : variant.overrides) {
        if (!override_paths.insert(override.path.value).second) {
            add_override_diagnostic(result.diagnostics, SceneSchemaDiagnosticCode::duplicate_override_path,
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
            add_override_diagnostic(result.diagnostics, SceneSchemaDiagnosticCode::missing_override_target, path);
        }
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    result.diagnostics = validate_scene_document(result.prefab.scene);
    result.success = result.diagnostics.empty();
    return result;
}

ScenePrefabInstanceRefreshPlan plan_scene_prefab_instance_refresh(const SceneDocument& scene,
                                                                  const AuthoringId& instance_root_node,
                                                                  const PrefabDocument& refreshed_prefab) {
    ScenePrefabInstanceRefreshPlan plan;
    plan.instance_root_node = instance_root_node;

    if (!is_valid_authoring_id(instance_root_node.value)) {
        add_diagnostic(plan.diagnostics, SceneSchemaDiagnosticCode::invalid_authoring_id, instance_root_node, {}, {},
                       "instance_root_node");
    }

    const auto scene_diagnostics = validate_scene_document(scene);
    plan.diagnostics.insert(plan.diagnostics.end(), scene_diagnostics.begin(), scene_diagnostics.end());
    const auto prefab_diagnostics = validate_prefab_document(refreshed_prefab);
    plan.diagnostics.insert(plan.diagnostics.end(), prefab_diagnostics.begin(), prefab_diagnostics.end());
    if (!plan.diagnostics.empty()) {
        return plan;
    }

    const auto* root_source = find_node_prefab_source(scene, instance_root_node.value);
    if (root_source == nullptr) {
        add_diagnostic(plan.diagnostics, SceneSchemaDiagnosticCode::missing_override_target, instance_root_node, {}, {},
                       "node.prefab_source");
        return plan;
    }

    plan.prefab_path = root_source->prefab_path;
    const auto subtree_node_ids = collect_scene_subtree_node_ids(scene, instance_root_node.value);
    add_prefab_refresh_nested_prefab_diagnostics(plan, scene, subtree_node_ids);
    if (!plan.diagnostics.empty()) {
        return plan;
    }

    add_prefab_refresh_local_ownership_diagnostics(plan, scene, subtree_node_ids);
    if (!plan.diagnostics.empty()) {
        return plan;
    }

    add_prefab_refresh_source_identity_diagnostics(plan, scene, subtree_node_ids);
    if (!plan.diagnostics.empty()) {
        return plan;
    }

    const auto refreshed_node_ids = collect_prefab_node_ids(refreshed_prefab);
    const auto refreshed_components_by_id = collect_prefab_components_by_id(refreshed_prefab);

    std::unordered_set<std::string> matched_source_node_ids;
    for (const auto& source : scene.node_prefab_sources) {
        if (source.prefab_path != plan.prefab_path || !subtree_node_ids.contains(source.node.value)) {
            continue;
        }

        const auto is_preserved = refreshed_node_ids.contains(source.source_node_id.value);
        add_scene_prefab_instance_refresh_row(
            plan, ScenePrefabInstanceRefreshRow{
                      .kind = is_preserved ? ScenePrefabInstanceRefreshRowKind::preserve_node
                                           : ScenePrefabInstanceRefreshRowKind::remove_stale_node,
                      .current_node = source.node,
                      .source_node_id = source.source_node_id,
                      .prefab_path = plan.prefab_path,
                  });
        if (is_preserved) {
            matched_source_node_ids.insert(source.source_node_id.value);
        }
    }

    for (const auto& source_node : refreshed_prefab.scene.nodes) {
        if (matched_source_node_ids.contains(source_node.id.value)) {
            continue;
        }

        add_scene_prefab_instance_refresh_row(plan, ScenePrefabInstanceRefreshRow{
                                                        .kind = ScenePrefabInstanceRefreshRowKind::add_source_node,
                                                        .source_node_id = source_node.id,
                                                        .prefab_path = plan.prefab_path,
                                                    });
    }

    std::unordered_set<std::string> matched_source_component_ids;
    for (const auto& source : scene.component_prefab_sources) {
        if (source.prefab_path != plan.prefab_path) {
            continue;
        }
        const auto* component = find_component_by_id(scene, source.component.value);
        if (component == nullptr || !subtree_node_ids.contains(component->node.value)) {
            continue;
        }

        const auto refreshed_component = refreshed_components_by_id.find(source.source_component_id.value);
        const auto is_preserved = refreshed_component != refreshed_components_by_id.end();
        add_scene_prefab_instance_refresh_row(
            plan, ScenePrefabInstanceRefreshRow{
                      .kind = is_preserved ? ScenePrefabInstanceRefreshRowKind::preserve_component
                                           : ScenePrefabInstanceRefreshRowKind::remove_stale_component,
                      .current_node = component->node,
                      .current_component = source.component,
                      .source_node_id = is_preserved ? refreshed_component->second->node : AuthoringId{},
                      .source_component_id = source.source_component_id,
                      .prefab_path = plan.prefab_path,
                  });
        if (is_preserved) {
            matched_source_component_ids.insert(source.source_component_id.value);
        }
    }

    for (const auto& source_component : refreshed_prefab.scene.components) {
        if (matched_source_component_ids.contains(source_component.id.value)) {
            continue;
        }

        add_scene_prefab_instance_refresh_row(plan, ScenePrefabInstanceRefreshRow{
                                                        .kind = ScenePrefabInstanceRefreshRowKind::add_source_component,
                                                        .source_node_id = source_component.node,
                                                        .source_component_id = source_component.id,
                                                        .prefab_path = plan.prefab_path,
                                                    });
    }

    plan.valid = true;
    return plan;
}

ScenePrefabInstanceRefreshResult apply_scene_prefab_instance_refresh(const SceneDocument& scene,
                                                                     const AuthoringId& instance_root_node,
                                                                     const PrefabDocument& refreshed_prefab) {
    ScenePrefabInstanceRefreshResult result;
    result.scene = scene;

    const auto plan = plan_scene_prefab_instance_refresh(scene, instance_root_node, refreshed_prefab);
    if (!plan.valid) {
        result.diagnostics = plan.diagnostics;
        return result;
    }

    const auto* root_source = find_node_prefab_source(scene, instance_root_node.value);
    const auto* current_root = find_node_by_id(scene, instance_root_node.value);
    if (root_source == nullptr || current_root == nullptr) {
        add_diagnostic(result.diagnostics, SceneSchemaDiagnosticCode::missing_override_target, instance_root_node, {},
                       {}, "node.prefab_source");
        return result;
    }

    const auto subtree_node_ids = collect_scene_subtree_node_ids(scene, instance_root_node.value);
    std::unordered_set<std::string> original_node_ids;
    std::unordered_set<std::string> original_component_ids;
    std::unordered_map<std::string, const SceneNodeDocument*> current_nodes_by_source_id;
    std::unordered_map<std::string, const SceneComponentDocument*> current_components_by_source_id;

    for (const auto& node : scene.nodes) {
        original_node_ids.insert(node.id.value);
    }
    for (const auto& component : scene.components) {
        original_component_ids.insert(component.id.value);
    }
    for (const auto& source : scene.node_prefab_sources) {
        if (source.prefab_path != plan.prefab_path || !subtree_node_ids.contains(source.node.value)) {
            continue;
        }
        if (const auto* node = find_node_by_id(scene, source.node.value); node != nullptr) {
            current_nodes_by_source_id.emplace(source.source_node_id.value, node);
        }
    }
    for (const auto& source : scene.component_prefab_sources) {
        if (source.prefab_path != plan.prefab_path) {
            continue;
        }
        const auto* component = find_component_by_id(scene, source.component.value);
        if (component == nullptr || !subtree_node_ids.contains(component->node.value)) {
            continue;
        }
        current_components_by_source_id.emplace(source.source_component_id.value, component);
    }

    SceneDocument applied_scene;
    applied_scene.name = scene.name;
    std::unordered_set<std::string> result_node_ids;
    std::unordered_set<std::string> result_component_ids;
    std::vector<ScenePrefabInstanceRefreshNodeMapping> source_to_result_node_ids;
    std::vector<ScenePrefabInstanceRefreshComponentMapping> source_to_result_component_ids;

    for (const auto& node : scene.nodes) {
        if (subtree_node_ids.contains(node.id.value)) {
            continue;
        }
        result_node_ids.insert(node.id.value);
        applied_scene.nodes.push_back(node);
    }
    for (const auto& component : scene.components) {
        if (subtree_node_ids.contains(component.node.value)) {
            continue;
        }
        result_component_ids.insert(component.id.value);
        applied_scene.components.push_back(component);
    }
    for (const auto& source : scene.node_prefab_sources) {
        if (!subtree_node_ids.contains(source.node.value)) {
            applied_scene.node_prefab_sources.push_back(source);
        }
    }
    for (const auto& source : scene.component_prefab_sources) {
        const auto* component = find_component_by_id(scene, source.component.value);
        if (component != nullptr && !subtree_node_ids.contains(component->node.value)) {
            applied_scene.component_prefab_sources.push_back(source);
        }
    }

    std::unordered_map<std::string, AuthoringId> result_node_by_source_id;
    std::unordered_map<std::string, std::size_t> result_node_index_by_source_id;
    for (const auto& source_node : refreshed_prefab.scene.nodes) {
        const auto current_it = current_nodes_by_source_id.find(source_node.id.value);
        SceneNodeDocument result_node;
        if (current_it != current_nodes_by_source_id.end()) {
            result_node = *current_it->second;
        } else {
            result_node = source_node;
            result_node.id = make_scene_prefab_refresh_added_id(instance_root_node, source_node.id);
            if (original_node_ids.contains(result_node.id.value) || result_node_ids.contains(result_node.id.value)) {
                add_diagnostic(result.diagnostics, SceneSchemaDiagnosticCode::duplicate_node_id, result_node.id);
                return result;
            }
        }

        result_node.parent = {};
        const auto result_node_id = result_node.id;
        result_node_ids.insert(result_node_id.value);
        result_node_by_source_id.emplace(source_node.id.value, result_node_id);
        result_node_index_by_source_id.emplace(source_node.id.value, applied_scene.nodes.size());
        source_to_result_node_ids.push_back(ScenePrefabInstanceRefreshNodeMapping{
            .source_node_id = source_node.id,
            .result_node = result_node_id,
        });
        applied_scene.node_prefab_sources.push_back(SceneNodePrefabSource{
            .node = result_node_id,
            .prefab_path = plan.prefab_path,
            .source_node_id = source_node.id,
        });
        applied_scene.nodes.push_back(std::move(result_node));
    }

    for (const auto& source_node : refreshed_prefab.scene.nodes) {
        auto node_index_it = result_node_index_by_source_id.find(source_node.id.value);
        if (node_index_it == result_node_index_by_source_id.end()) {
            add_diagnostic(result.diagnostics, SceneSchemaDiagnosticCode::missing_override_target, source_node.id, {},
                           {}, "node.prefab_source.source_node_id");
            return result;
        }

        auto& result_node = applied_scene.nodes[node_index_it->second];
        if (source_node.parent.value.empty()) {
            if (source_node.id.value == root_source->source_node_id.value) {
                result_node.parent = current_root->parent;
            }
            continue;
        }

        const auto parent_it = result_node_by_source_id.find(source_node.parent.value);
        if (parent_it == result_node_by_source_id.end()) {
            add_diagnostic(result.diagnostics, SceneSchemaDiagnosticCode::missing_parent_node, result_node.id);
            return result;
        }
        result_node.parent = parent_it->second;
    }

    for (const auto& source_component : refreshed_prefab.scene.components) {
        const auto node_it = result_node_by_source_id.find(source_component.node.value);
        if (node_it == result_node_by_source_id.end()) {
            add_diagnostic(result.diagnostics, SceneSchemaDiagnosticCode::missing_component_node, source_component.node,
                           source_component.id, source_component.type);
            return result;
        }

        const auto current_it = current_components_by_source_id.find(source_component.id.value);
        SceneComponentDocument result_component;
        if (current_it != current_components_by_source_id.end()) {
            result_component = *current_it->second;
            result_component.node = node_it->second;
        } else {
            result_component = source_component;
            result_component.id = make_scene_prefab_refresh_added_id(instance_root_node, source_component.id);
            result_component.node = node_it->second;
            if (original_component_ids.contains(result_component.id.value) ||
                result_component_ids.contains(result_component.id.value)) {
                add_diagnostic(result.diagnostics, SceneSchemaDiagnosticCode::duplicate_component_id,
                               result_component.node, result_component.id, result_component.type);
                return result;
            }
        }

        const auto result_component_id = result_component.id;
        result_component_ids.insert(result_component_id.value);
        source_to_result_component_ids.push_back(ScenePrefabInstanceRefreshComponentMapping{
            .source_component_id = source_component.id,
            .result_component = result_component_id,
        });
        applied_scene.component_prefab_sources.push_back(SceneComponentPrefabSource{
            .component = result_component_id,
            .prefab_path = plan.prefab_path,
            .source_component_id = source_component.id,
        });
        applied_scene.components.push_back(std::move(result_component));
    }

    result.diagnostics = validate_scene_document(applied_scene);
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.scene = std::move(applied_scene);
    result.source_to_result_node_ids = std::move(source_to_result_node_ids);
    result.source_to_result_component_ids = std::move(source_to_result_component_ids);
    result.preserved_node_count = plan.preserve_node_count;
    result.added_node_count = plan.add_node_count;
    result.removed_node_count = plan.remove_node_count;
    result.preserved_component_count = plan.preserve_component_count;
    result.added_component_count = plan.add_component_count;
    result.removed_component_count = plan.remove_component_count;
    result.applied = true;
    result.mutates = true;
    return result;
}

} // namespace mirakana
