// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/viewport.hpp"
#include "mirakana/scene/scene.hpp"

#include <optional>

namespace mirakana::editor {

struct SceneNodeTransformDraft {
    SceneNodeId node;
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 rotation_radians{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 scale{.x = 1.0F, .y = 1.0F, .z = 1.0F};
};

struct SceneNodeComponentDraft {
    SceneNodeId node;
    SceneNodeComponents components;
};

struct ViewportTransformEdit {
    SceneNodeId node;
    ViewportTool tool{ViewportTool::select};
    Vec3 delta{.x = 0.0F, .y = 0.0F, .z = 0.0F};
};

[[nodiscard]] std::optional<SceneNodeTransformDraft> make_scene_node_transform_draft(const Scene& scene,
                                                                                     SceneNodeId node);
[[nodiscard]] bool apply_scene_node_transform_draft(Scene& scene, const SceneNodeTransformDraft& draft);
[[nodiscard]] std::optional<SceneNodeComponentDraft> make_scene_node_component_draft(const Scene& scene,
                                                                                     SceneNodeId node);
[[nodiscard]] bool apply_scene_node_component_draft(Scene& scene, const SceneNodeComponentDraft& draft);
[[nodiscard]] bool apply_viewport_transform_edit(Scene& scene, const ViewportTransformEdit& edit);

} // namespace mirakana::editor
