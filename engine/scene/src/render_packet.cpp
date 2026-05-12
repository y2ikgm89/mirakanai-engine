// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene/render_packet.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace mirakana {
namespace {

enum class VisitState { unvisited, visiting, visited };

[[nodiscard]] std::size_t checked_node_index(SceneNodeId id, std::size_t node_count) {
    if (id == null_scene_node || id.value > node_count) {
        throw std::invalid_argument("scene render packet references an invalid parent node");
    }
    return id.value - 1;
}

[[nodiscard]] Mat4 resolve_world_matrix(const std::vector<SceneNode>& nodes, std::vector<VisitState>& visit_states,
                                        std::vector<Mat4>& world_matrices, std::size_t index) {
    if (visit_states[index] == VisitState::visited) {
        return world_matrices[index];
    }
    if (visit_states[index] == VisitState::visiting) {
        throw std::invalid_argument("scene render packet cannot be built from a cyclic hierarchy");
    }

    visit_states[index] = VisitState::visiting;

    const auto& node = nodes[index];
    auto world = node.transform.matrix();
    if (node.parent != null_scene_node) {
        const auto parent_index = checked_node_index(node.parent, nodes.size());
        world = resolve_world_matrix(nodes, visit_states, world_matrices, parent_index) * world;
    }

    world_matrices[index] = world;
    visit_states[index] = VisitState::visited;
    return world;
}

} // namespace

const SceneRenderCamera* SceneRenderPacket::primary_camera() const noexcept {
    const auto camera =
        std::ranges::find_if(cameras, [](const SceneRenderCamera& item) { return item.camera.primary; });
    if (camera == cameras.end()) {
        return nullptr;
    }
    return &*camera;
}

SceneRenderPacket build_scene_render_packet(const Scene& scene) {
    const auto& nodes = scene.nodes();
    std::vector<VisitState> visit_states(nodes.size(), VisitState::unvisited);
    std::vector<Mat4> world_matrices(nodes.size(), Mat4::identity());

    SceneRenderPacket packet;
    packet.cameras.reserve(nodes.size());
    packet.lights.reserve(nodes.size());
    packet.meshes.reserve(nodes.size());
    packet.sprites.reserve(nodes.size());

    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const auto& node = nodes[index];
        if (!is_valid_scene_node_components(node.components)) {
            throw std::invalid_argument("scene render packet cannot be built from invalid components");
        }

        const auto world_from_node = resolve_world_matrix(nodes, visit_states, world_matrices, index);
        if (node.components.camera.has_value()) {
            packet.cameras.push_back(SceneRenderCamera{
                .node = node.id, .world_from_node = world_from_node, .camera = *node.components.camera});
        }
        if (node.components.light.has_value()) {
            packet.lights.push_back(
                SceneRenderLight{.node = node.id, .world_from_node = world_from_node, .light = *node.components.light});
        }
        if (node.components.mesh_renderer.has_value() && node.components.mesh_renderer->visible) {
            packet.meshes.push_back(SceneRenderMesh{
                .node = node.id, .world_from_node = world_from_node, .renderer = *node.components.mesh_renderer});
        }
        if (node.components.sprite_renderer.has_value() && node.components.sprite_renderer->visible) {
            packet.sprites.push_back(SceneRenderSprite{
                .node = node.id, .world_from_node = world_from_node, .renderer = *node.components.sprite_renderer});
        }
    }

    return packet;
}

} // namespace mirakana
