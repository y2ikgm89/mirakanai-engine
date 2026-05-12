// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene/scene.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_scene_link_text(std::string_view value) noexcept {
    return !value.empty() && value.find_first_of("\r\n=") == std::string_view::npos;
}

[[nodiscard]] bool valid_optional_scene_link_path(std::string_view value) noexcept {
    if (value.find_first_of("\r\n=") != std::string_view::npos) {
        return false;
    }
    if (value.empty()) {
        return true;
    }
    if (value.front() == '/' || value.front() == '\\') {
        return false;
    }
    if (value.size() >= 2U && value[1] == ':' &&
        ((value[0] >= 'A' && value[0] <= 'Z') || (value[0] >= 'a' && value[0] <= 'z'))) {
        return false;
    }

    std::size_t start = 0U;
    while (start <= value.size()) {
        const auto slash = value.find_first_of("/\\", start);
        const auto end = slash == std::string_view::npos ? value.size() : slash;
        const auto segment = value.substr(start, end - start);
        if (segment.empty() || segment == "..") {
            return false;
        }
        if (slash == std::string_view::npos) {
            break;
        }
        start = slash + 1U;
    }
    return true;
}

[[nodiscard]] bool contains_descendant(const Scene& scene, SceneNodeId root, SceneNodeId candidate) {
    const auto* root_node = scene.find_node(root);
    if (root_node == nullptr) {
        return false;
    }

    std::vector<bool> visited(scene.nodes().size() + 1U, false);
    std::vector<SceneNodeId> stack = root_node->children;
    while (!stack.empty()) {
        const auto current = stack.back();
        stack.pop_back();
        if (current == candidate) {
            return true;
        }
        if (current.value == 0 || current.value >= visited.size() || visited[current.value]) {
            continue;
        }
        visited[current.value] = true;

        const auto* node = scene.find_node(current);
        if (node != nullptr) {
            stack.insert(stack.end(), node->children.begin(), node->children.end());
        }
    }

    return false;
}

} // namespace

Scene::Scene(std::string name) : name_(std::move(name)) {
    if (name_.empty()) {
        throw std::invalid_argument("scene name must not be empty");
    }
}

std::string_view Scene::name() const noexcept {
    return name_;
}

SceneNodeId Scene::create_node(std::string name) {
    if (name.empty()) {
        throw std::invalid_argument("scene node name must not be empty");
    }

    const auto id = SceneNodeId{static_cast<std::uint32_t>(nodes_.size() + 1)};
    SceneNode node;
    node.id = id;
    node.name = std::move(name);
    nodes_.push_back(std::move(node));
    return id;
}

SceneNode* Scene::find_node(SceneNodeId id) noexcept {
    if (id == null_scene_node || id.value > nodes_.size()) {
        return nullptr;
    }
    return &nodes_[id.value - 1];
}

const SceneNode* Scene::find_node(SceneNodeId id) const noexcept {
    if (id == null_scene_node || id.value > nodes_.size()) {
        return nullptr;
    }
    return &nodes_[id.value - 1];
}

const std::vector<SceneNode>& Scene::nodes() const noexcept {
    return nodes_;
}

void Scene::set_components(SceneNodeId node, SceneNodeComponents components) {
    auto* scene_node = find_node(node);
    if (scene_node == nullptr || !is_valid_scene_node_components(components)) {
        throw std::invalid_argument("invalid scene node components");
    }
    scene_node->components = components;
}

void Scene::set_parent(SceneNodeId child, SceneNodeId parent) {
    auto* child_node = find_node(child);
    auto* parent_node = find_node(parent);
    if (child_node == nullptr || parent_node == nullptr || child == parent ||
        contains_descendant(*this, child, parent)) {
        throw std::invalid_argument("invalid scene parenting");
    }

    if (child_node->parent != null_scene_node) {
        auto* previous_parent = find_node(child_node->parent);
        if (previous_parent != nullptr) {
            auto& siblings = previous_parent->children;
            const auto removed = std::ranges::remove(siblings, child);
            siblings.erase(removed.begin(), removed.end());
        }
    }

    child_node->parent = parent;
    parent_node->children.push_back(child);
}

std::size_t Scene::index_for(SceneNodeId id) const {
    if (id == null_scene_node || id.value > nodes_.size()) {
        throw std::out_of_range("scene node does not exist");
    }
    return id.value - 1;
}

bool is_valid_scene_prefab_source_link(const ScenePrefabSourceLink& link) noexcept {
    return valid_scene_link_text(link.prefab_name) && valid_optional_scene_link_path(link.prefab_path) &&
           link.source_node_index > 0U && valid_scene_link_text(link.source_node_name);
}

} // namespace mirakana
