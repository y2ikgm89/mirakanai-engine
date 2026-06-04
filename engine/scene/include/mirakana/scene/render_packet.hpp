// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/mat4.hpp"
#include "mirakana/scene/components.hpp"
#include "mirakana/scene/scene.hpp"

#include <optional>
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
    std::optional<SceneEnvironmentReference> environment;
    std::vector<SceneRenderCamera> cameras;
    std::vector<SceneRenderLight> lights;
    std::vector<SceneRenderMesh> meshes;
    std::vector<SceneRenderSprite> sprites;

    [[nodiscard]] const SceneRenderCamera* primary_camera() const noexcept;
};

struct SceneRenderSpriteSortStats {
    std::uint64_t sprite_count{0};
    std::uint64_t sorting_layers_applied{0};
    std::uint64_t sorted_draws{0};
    std::uint64_t reordered_count{0};
};

struct SceneSpriteExpansionStats {
    std::uint64_t source_sprite_count{0};
    std::uint64_t expanded_sprite_count{0};
    std::uint64_t nine_slice_count{0};
    std::uint64_t tiled_count{0};

    [[nodiscard]] std::uint64_t expansion_count() const noexcept {
        return expanded_sprite_count > source_sprite_count ? expanded_sprite_count - source_sprite_count : 0U;
    }
};

[[nodiscard]] SceneRenderSpriteSortStats sort_scene_render_sprites(std::vector<SceneRenderSprite>& sprites);
[[nodiscard]] SceneRenderPacket build_scene_render_packet(const Scene& scene);

} // namespace mirakana
