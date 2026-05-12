// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene/components.hpp"

#include <cmath>
#include <numbers>

namespace mirakana {
namespace {

constexpr float pi = std::numbers::pi_v<float>;

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_range(float value, float min_value, float max_value) noexcept {
    return finite(value) && value >= min_value && value <= max_value;
}

[[nodiscard]] bool valid_projection(CameraProjectionMode projection) noexcept {
    switch (projection) {
    case CameraProjectionMode::perspective:
    case CameraProjectionMode::orthographic:
        return true;
    case CameraProjectionMode::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_light_type(LightType type) noexcept {
    switch (type) {
    case LightType::directional:
    case LightType::point:
    case LightType::spot:
        return true;
    case LightType::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_non_negative_color(Vec3 color) noexcept {
    return finite_range(color.x, 0.0F, 64.0F) && finite_range(color.y, 0.0F, 64.0F) &&
           finite_range(color.z, 0.0F, 64.0F);
}

[[nodiscard]] bool valid_tint(const std::array<float, 4>& tint) noexcept {
    return finite_range(tint[0], 0.0F, 64.0F) && finite_range(tint[1], 0.0F, 64.0F) &&
           finite_range(tint[2], 0.0F, 64.0F) && finite_range(tint[3], 0.0F, 64.0F);
}

} // namespace

bool is_valid_camera_component(const CameraComponent& camera) noexcept {
    if (!valid_projection(camera.projection) || !finite_range(camera.vertical_fov_radians, 0.001F, pi - 0.001F) ||
        !finite_range(camera.orthographic_height, 0.001F, 1000000.0F) ||
        !finite_range(camera.near_plane, 0.0001F, 1000000.0F) ||
        !finite_range(camera.far_plane, 0.0001F, 100000000.0F)) {
        return false;
    }
    return camera.far_plane > camera.near_plane;
}

bool is_valid_light_component(const LightComponent& light) noexcept {
    if (!valid_light_type(light.type) || !valid_non_negative_color(light.color) ||
        !finite_range(light.intensity, 0.0F, 100000000.0F) || !finite_range(light.range, 0.0F, 100000000.0F) ||
        !finite_range(light.inner_cone_radians, 0.0F, pi) || !finite_range(light.outer_cone_radians, 0.0F, pi)) {
        return false;
    }

    if (light.type == LightType::spot) {
        return light.range > 0.0F && light.outer_cone_radians > light.inner_cone_radians;
    }
    if (light.type == LightType::point) {
        return light.range > 0.0F;
    }
    return true;
}

bool is_valid_mesh_renderer_component(const MeshRendererComponent& renderer) noexcept {
    return renderer.mesh.value != 0 && renderer.material.value != 0;
}

bool is_valid_sprite_renderer_component(const SpriteRendererComponent& renderer) noexcept {
    return renderer.sprite.value != 0 && renderer.material.value != 0 &&
           finite_range(renderer.size.x, 0.0001F, 1000000.0F) && finite_range(renderer.size.y, 0.0001F, 1000000.0F) &&
           valid_tint(renderer.tint);
}

bool is_valid_scene_node_components(const SceneNodeComponents& components) noexcept {
    if (components.camera.has_value() && !is_valid_camera_component(*components.camera)) {
        return false;
    }
    if (components.light.has_value() && !is_valid_light_component(*components.light)) {
        return false;
    }
    if (components.mesh_renderer.has_value() && !is_valid_mesh_renderer_component(*components.mesh_renderer)) {
        return false;
    }
    if (components.sprite_renderer.has_value() && !is_valid_sprite_renderer_component(*components.sprite_renderer)) {
        return false;
    }
    return true;
}

} // namespace mirakana
