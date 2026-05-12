// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/scene_edit.hpp"

#include <cmath>

namespace mirakana::editor {
namespace {

[[nodiscard]] bool is_finite(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool is_valid_scale(Vec3 value) noexcept {
    return is_finite(value) && value.x > 0.0F && value.y > 0.0F && value.z > 0.0F;
}

[[nodiscard]] bool is_valid_transform_values(Vec3 position, Vec3 rotation_radians, Vec3 scale) noexcept {
    return is_finite(position) && is_finite(rotation_radians) && is_valid_scale(scale);
}

} // namespace

std::optional<SceneNodeTransformDraft> make_scene_node_transform_draft(const Scene& scene, SceneNodeId node) {
    const auto* scene_node = scene.find_node(node);
    if (scene_node == nullptr) {
        return std::nullopt;
    }

    return SceneNodeTransformDraft{
        .node = node,
        .position = scene_node->transform.position,
        .rotation_radians = scene_node->transform.rotation_radians,
        .scale = scene_node->transform.scale,
    };
}

bool apply_scene_node_transform_draft(Scene& scene, const SceneNodeTransformDraft& draft) {
    auto* scene_node = scene.find_node(draft.node);
    if (scene_node == nullptr || !is_valid_transform_values(draft.position, draft.rotation_radians, draft.scale)) {
        return false;
    }

    scene_node->transform.position = draft.position;
    scene_node->transform.rotation_radians = draft.rotation_radians;
    scene_node->transform.scale = draft.scale;
    return true;
}

std::optional<SceneNodeComponentDraft> make_scene_node_component_draft(const Scene& scene, SceneNodeId node) {
    const auto* scene_node = scene.find_node(node);
    if (scene_node == nullptr) {
        return std::nullopt;
    }

    return SceneNodeComponentDraft{
        .node = node,
        .components = scene_node->components,
    };
}

bool apply_scene_node_component_draft(Scene& scene, const SceneNodeComponentDraft& draft) {
    if (scene.find_node(draft.node) == nullptr || !is_valid_scene_node_components(draft.components)) {
        return false;
    }

    scene.set_components(draft.node, draft.components);
    return true;
}

bool apply_viewport_transform_edit(Scene& scene, const ViewportTransformEdit& edit) {
    auto draft = make_scene_node_transform_draft(scene, edit.node);
    if (!draft.has_value() || !is_finite(edit.delta)) {
        return false;
    }

    switch (edit.tool) {
    case ViewportTool::select:
        return false;
    case ViewportTool::translate:
        draft->position = draft->position + edit.delta;
        break;
    case ViewportTool::rotate:
        draft->rotation_radians = draft->rotation_radians + edit.delta;
        break;
    case ViewportTool::scale:
        draft->scale = draft->scale + edit.delta;
        break;
    }

    return apply_scene_node_transform_draft(scene, *draft);
}

} // namespace mirakana::editor
