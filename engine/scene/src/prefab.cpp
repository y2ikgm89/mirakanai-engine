// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene/prefab.hpp"

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_name(const std::string& name) noexcept {
    return !name.empty() && name.find_first_of("\r\n=") == std::string::npos;
}

struct SceneNodeIdHash {
    [[nodiscard]] std::size_t operator()(SceneNodeId id) const noexcept {
        return std::hash<std::uint32_t>{}(id.value);
    }
};

bool append_prefab_subtree(const Scene& scene, SceneNodeId node_id, std::uint32_t parent_index,
                           PrefabDefinition& prefab, std::unordered_set<SceneNodeId, SceneNodeIdHash>& visited) {
    if (!visited.insert(node_id).second) {
        return false;
    }

    const auto* node = scene.find_node(node_id);
    if (node == nullptr) {
        return false;
    }

    const auto ordinal = static_cast<std::uint32_t>(prefab.nodes.size() + 1U);
    prefab.nodes.push_back(PrefabNodeTemplate{
        .name = node->name,
        .transform = node->transform,
        .parent_index = parent_index,
        .components = node->components,
    });

    for (const auto child : node->children) {
        if (!append_prefab_subtree(scene, child, ordinal, prefab, visited)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] Scene scene_from_prefab(const PrefabDefinition& prefab) {
    Scene scene(prefab.name);

    for (const auto& node_template : prefab.nodes) {
        const auto node = scene.create_node(node_template.name);
        auto* scene_node = scene.find_node(node);
        scene_node->transform = node_template.transform;
        scene.set_components(node, node_template.components);
    }

    for (std::size_t index = 0; index < prefab.nodes.size(); ++index) {
        const auto parent_index = prefab.nodes[index].parent_index;
        if (parent_index == 0) {
            continue;
        }
        scene.set_parent(SceneNodeId{static_cast<std::uint32_t>(index + 1U)}, SceneNodeId{parent_index});
    }

    return scene;
}

[[nodiscard]] std::string scene_text_to_prefab_text(const std::string& scene_text, std::string_view prefab_name) {
    const auto format_end = scene_text.find('\n');
    const auto name_end = format_end == std::string::npos ? std::string::npos : scene_text.find('\n', format_end + 1U);
    if (format_end == std::string::npos || name_end == std::string::npos) {
        throw std::invalid_argument("serialized scene text is incomplete");
    }

    std::string output;
    output.reserve(scene_text.size() + 8U);
    output += "format=GameEngine.Prefab.v1\n";
    output += "prefab.name=";
    output += prefab_name;
    output += '\n';
    output += scene_text.substr(name_end + 1U);
    return output;
}

[[nodiscard]] std::string prefab_text_to_scene_text(std::string_view text) {
    std::ostringstream output;
    std::istringstream input{std::string(text)};
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::invalid_argument("prefab line must contain '='");
        }

        const auto key = std::string_view(line).substr(0, separator);
        const auto value = std::string_view(line).substr(separator + 1U);
        if (key == "format") {
            if (value != "GameEngine.Prefab.v1") {
                throw std::invalid_argument("unsupported prefab format");
            }
            output << "format=GameEngine.Scene.v1\n";
        } else if (key == "prefab.name") {
            output << "scene.name=" << value << '\n';
        } else if (key == "node.count" || key.starts_with("node.")) {
            output << line << '\n';
        } else {
            throw std::invalid_argument("unknown prefab key");
        }
    }
    return output.str();
}

[[nodiscard]] PrefabDefinition prefab_from_scene(const Scene& scene) {
    PrefabDefinition prefab;
    prefab.name = std::string(scene.name());
    prefab.nodes.reserve(scene.nodes().size());

    for (const auto& node : scene.nodes()) {
        prefab.nodes.push_back(PrefabNodeTemplate{
            .name = node.name,
            .transform = node.transform,
            .parent_index = node.parent.value,
            .components = node.components,
        });
    }
    return prefab;
}

} // namespace

bool is_valid_prefab_definition(const PrefabDefinition& prefab) noexcept {
    if (!valid_name(prefab.name) || prefab.nodes.empty()) {
        return false;
    }

    for (std::size_t index = 0; index < prefab.nodes.size(); ++index) {
        const auto& node = prefab.nodes[index];
        if (!valid_name(node.name) || !is_valid_scene_node_components(node.components)) {
            return false;
        }
        const auto ordinal = static_cast<std::uint32_t>(index + 1U);
        if (node.parent_index == ordinal || node.parent_index > index) {
            return false;
        }
    }

    return true;
}

std::optional<PrefabDefinition> build_prefab_from_scene_subtree(const Scene& scene, SceneNodeId root,
                                                                std::string name) {
    if (!valid_name(name) || scene.find_node(root) == nullptr) {
        return std::nullopt;
    }

    PrefabDefinition prefab;
    prefab.name = std::move(name);
    std::unordered_set<SceneNodeId, SceneNodeIdHash> visited;
    if (!append_prefab_subtree(scene, root, 0, prefab, visited)) {
        return std::nullopt;
    }
    if (!is_valid_prefab_definition(prefab)) {
        return std::nullopt;
    }
    return prefab;
}

PrefabInstance instantiate_prefab(Scene& scene, const PrefabInstantiateDesc& desc) {
    if (!is_valid_prefab_definition(desc.prefab)) {
        throw std::invalid_argument("prefab definition is invalid");
    }

    PrefabInstance instance;
    instance.nodes.reserve(desc.prefab.nodes.size());

    for (std::size_t index = 0; index < desc.prefab.nodes.size(); ++index) {
        const auto& node_template = desc.prefab.nodes[index];
        const auto node = scene.create_node(node_template.name);
        auto* scene_node = scene.find_node(node);
        scene_node->transform = node_template.transform;
        scene_node->prefab_source = ScenePrefabSourceLink{
            .prefab_name = desc.prefab.name,
            .prefab_path = desc.source_path,
            .source_node_index = static_cast<std::uint32_t>(index + 1U),
            .source_node_name = node_template.name,
        };
        scene.set_components(node, node_template.components);
        instance.nodes.push_back(node);
    }

    for (std::size_t index = 0; index < desc.prefab.nodes.size(); ++index) {
        const auto parent_index = desc.prefab.nodes[index].parent_index;
        if (parent_index == 0) {
            continue;
        }
        scene.set_parent(instance.nodes[index], instance.nodes[parent_index - 1U]);
    }

    return instance;
}

PrefabInstance instantiate_prefab(Scene& scene, const PrefabDefinition& prefab) {
    return instantiate_prefab(scene, PrefabInstantiateDesc{.prefab = prefab, .source_path = {}});
}

std::string serialize_prefab_definition(const PrefabDefinition& prefab) {
    if (!is_valid_prefab_definition(prefab)) {
        throw std::invalid_argument("prefab definition is invalid");
    }

    return scene_text_to_prefab_text(serialize_scene(scene_from_prefab(prefab)), prefab.name);
}

PrefabDefinition deserialize_prefab_definition(std::string_view text) {
    auto prefab = prefab_from_scene(deserialize_scene(prefab_text_to_scene_text(text)));
    if (!is_valid_prefab_definition(prefab)) {
        throw std::invalid_argument("prefab definition is invalid");
    }
    return prefab;
}

} // namespace mirakana
