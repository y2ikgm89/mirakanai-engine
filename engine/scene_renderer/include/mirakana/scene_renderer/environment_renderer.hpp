// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/environment/environment_profile.hpp"
#include "mirakana/math/vec.hpp"
#include "mirakana/scene/render_packet.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class EnvironmentCelestialLightKind : std::uint8_t {
    sun = 0,
    moon,
};

enum class EnvironmentSceneLightBindingKind : std::uint8_t {
    none = 0,
    environment_sun,
    environment_moon,
};

enum class EnvironmentRenderPacketDiagnosticCode : std::uint8_t {
    none = 0,
    missing_scene_packet,
    missing_environment_profile,
    invalid_environment_profile,
    missing_primary_camera,
    duplicate_light_binding,
    missing_bound_light,
    unsupported_bound_light_type,
    conflicting_environment_sun_direction,
    conflicting_environment_moon_direction,
};

struct EnvironmentSceneLightBinding {
    SceneNodeId node;
    EnvironmentSceneLightBindingKind binding{EnvironmentSceneLightBindingKind::none};
};

struct EnvironmentRenderPacketDesc {
    const SceneRenderPacket* scene_packet{nullptr};
    const EnvironmentProfileDesc* environment{nullptr};
    std::span<const EnvironmentSceneLightBinding> light_bindings;
    float direction_conflict_cosine_threshold{0.98F};
};

struct EnvironmentAtmosphereRenderRow {
    float planet_radius_km{0.0F};
    float atmosphere_height_km{0.0F};
    Vec3 rayleigh_scattering;
    Vec3 mie_scattering;
    Vec3 ground_albedo;
};

struct EnvironmentCelestialLightRenderRow {
    EnvironmentCelestialLightKind kind{EnvironmentCelestialLightKind::sun};
    Vec3 direction{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    Vec3 color{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float illuminance_lux{0.0F};
    float angular_radius_radians{0.0F};
    bool visible_disk{false};
    bool affects_atmosphere{false};
    bool affects_clouds{false};
    bool casts_environment_shadows{false};
};

struct EnvironmentSkyRenderRow {
    EnvironmentSkyModel model{EnvironmentSkyModel::none};
    Vec3 sun_direction{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    Vec3 moon_direction{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    std::uint32_t directional_scene_light_count{0};
};

struct EnvironmentFogRenderRow {
    bool enabled{false};
    float density{0.0F};
    float height_falloff{1.0F};
    Vec3 albedo{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float anisotropy{0.0F};
    bool uses_scene_depth{false};
};

struct EnvironmentCloudRenderRow {
    bool enabled{false};
    bool casts_shadows{false};
};

struct EnvironmentPrecipitationRenderRow {
    EnvironmentPrecipitationKind kind{EnvironmentPrecipitationKind::none};
    float intensity{0.0F};
    float particle_radius_mm{0.0F};
    float fall_speed_mps{0.0F};
    float wind_speed_mps{0.0F};
};

struct EnvironmentSceneLightRenderRow {
    SceneNodeId node;
    LightType type{LightType::unknown};
    Vec3 direction{.x = 0.0F, .y = 0.0F, .z = -1.0F};
    Vec3 color{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float intensity{0.0F};
    bool casts_shadows{false};
    EnvironmentSceneLightBindingKind binding{EnvironmentSceneLightBindingKind::none};
};

struct EnvironmentRenderPacketDiagnostic {
    EnvironmentRenderPacketDiagnosticCode code{EnvironmentRenderPacketDiagnosticCode::none};
    SceneNodeId node{null_scene_node};
    std::size_t row_index{0};
    std::string message;
};

struct EnvironmentRenderPacket {
    std::string profile_id;
    AssetId profile_asset;
    SceneNodeId primary_camera_node{null_scene_node};
    EnvironmentAtmosphereRenderRow atmosphere;
    EnvironmentSkyRenderRow sky;
    EnvironmentFogRenderRow fog;
    EnvironmentCloudRenderRow clouds;
    EnvironmentPrecipitationRenderRow precipitation;
    std::vector<EnvironmentCelestialLightRenderRow> celestial_lights;
    std::vector<EnvironmentSceneLightRenderRow> scene_lights;
    std::vector<EnvironmentRenderPacketDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] EnvironmentRenderPacket build_environment_render_packet(const EnvironmentRenderPacketDesc& desc);

[[nodiscard]] EnvironmentRenderPacket build_environment_render_packet(const SceneRenderPacket& scene_packet,
                                                                      const EnvironmentProfileDesc& environment);

[[nodiscard]] bool has_environment_render_packet_diagnostic(const EnvironmentRenderPacket& packet,
                                                            EnvironmentRenderPacketDiagnosticCode code) noexcept;

} // namespace mirakana
