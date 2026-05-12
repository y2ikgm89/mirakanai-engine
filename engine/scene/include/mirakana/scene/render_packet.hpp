// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/mat4.hpp"
#include "mirakana/scene/components.hpp"
#include "mirakana/scene/scene.hpp"

#include <vector>

namespace mirakana {

struct SceneRenderCamera {
    SceneNodeId node;
    Mat4 world_from_node;
    CameraComponent camera;
};

struct SceneRenderLight {
    SceneNodeId node;
    Mat4 world_from_node;
    LightComponent light;
};

struct SceneRenderMesh {
    SceneNodeId node;
    Mat4 world_from_node;
    MeshRendererComponent renderer;
};

struct SceneRenderSprite {
    SceneNodeId node;
    Mat4 world_from_node;
    SpriteRendererComponent renderer;
};

struct SceneRenderPacket {
    std::vector<SceneRenderCamera> cameras;
    std::vector<SceneRenderLight> lights;
    std::vector<SceneRenderMesh> meshes;
    std::vector<SceneRenderSprite> sprites;

    [[nodiscard]] const SceneRenderCamera* primary_camera() const noexcept;
};

[[nodiscard]] SceneRenderPacket build_scene_render_packet(const Scene& scene);

} // namespace mirakana
