// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/math/vec.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace mirakana {

enum class CameraProjectionMode : std::uint8_t { unknown, perspective, orthographic };

struct CameraComponent {
    CameraProjectionMode projection{CameraProjectionMode::perspective};
    float vertical_fov_radians{1.04719758F};
    float orthographic_height{10.0F};
    float near_plane{0.1F};
    float far_plane{1000.0F};
    bool primary{false};
};

enum class LightType : std::uint8_t { unknown, directional, point, spot };

struct LightComponent {
    LightType type{LightType::directional};
    Vec3 color{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float intensity{1.0F};
    float range{10.0F};
    float inner_cone_radians{0.0F};
    float outer_cone_radians{0.0F};
    bool casts_shadows{false};
};

struct MeshRendererComponent {
    AssetId mesh;
    AssetId material;
    bool visible{true};
};

struct SpriteRendererComponent {
    AssetId sprite;
    AssetId material;
    Vec2 size{.x = 1.0F, .y = 1.0F};
    std::array<float, 4> tint{1.0F, 1.0F, 1.0F, 1.0F};
    bool visible{true};
};

struct SceneNodeComponents {
    std::optional<CameraComponent> camera;
    std::optional<LightComponent> light;
    std::optional<MeshRendererComponent> mesh_renderer;
    std::optional<SpriteRendererComponent> sprite_renderer;
};

[[nodiscard]] bool is_valid_camera_component(const CameraComponent& camera) noexcept;
[[nodiscard]] bool is_valid_light_component(const LightComponent& light) noexcept;
[[nodiscard]] bool is_valid_mesh_renderer_component(const MeshRendererComponent& renderer) noexcept;
[[nodiscard]] bool is_valid_sprite_renderer_component(const SpriteRendererComponent& renderer) noexcept;
[[nodiscard]] bool is_valid_scene_node_components(const SceneNodeComponents& components) noexcept;

} // namespace mirakana
