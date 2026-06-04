// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene_renderer/environment_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] Vec3 normalized_or(Vec3 value, Vec3 fallback) noexcept {
    const auto value_length = length(value);
    if (!std::isfinite(value_length) || value_length <= std::numeric_limits<float>::epsilon()) {
        return fallback;
    }
    const auto inverse = 1.0F / value_length;
    return Vec3{.x = value.x * inverse, .y = value.y * inverse, .z = value.z * inverse};
}

[[nodiscard]] Vec3 scene_light_direction(const Mat4& world_from_node) noexcept {
    const auto local_positive_z =
        normalized_or(Vec3{.x = world_from_node.at(0, 2), .y = world_from_node.at(1, 2), .z = world_from_node.at(2, 2)},
                      Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F});
    return local_positive_z * -1.0F;
}

void add_diagnostic(EnvironmentRenderPacket& packet, EnvironmentRenderPacketDiagnosticCode code, SceneNodeId node,
                    std::size_t row_index, std::string message) {
    packet.diagnostics.push_back(EnvironmentRenderPacketDiagnostic{
        .code = code,
        .node = node,
        .row_index = row_index,
        .message = std::move(message),
    });
}

[[nodiscard]] const SceneRenderLight* find_light(const SceneRenderPacket& packet, SceneNodeId node) noexcept {
    const auto found =
        std::ranges::find_if(packet.lights, [node](const auto& light) noexcept { return light.node == node; });
    return found == packet.lights.end() ? nullptr : &(*found);
}

[[nodiscard]] bool has_duplicate_binding(std::span<const EnvironmentSceneLightBinding> bindings, SceneNodeId node,
                                         std::size_t current_index) noexcept {
    for (std::size_t index = 0; index < current_index; ++index) {
        if (bindings[index].node == node && bindings[index].binding != EnvironmentSceneLightBindingKind::none) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] EnvironmentSceneLightBindingKind binding_for_node(std::span<const EnvironmentSceneLightBinding> bindings,
                                                                SceneNodeId node) noexcept {
    const auto found =
        std::ranges::find_if(bindings, [node](const auto& binding) noexcept { return binding.node == node; });
    return found == bindings.end() ? EnvironmentSceneLightBindingKind::none : found->binding;
}

[[nodiscard]] EnvironmentCelestialLightRenderRow make_celestial_light(EnvironmentCelestialLightKind kind,
                                                                      const EnvironmentSunMoonDesc& desc) noexcept {
    return EnvironmentCelestialLightRenderRow{
        .kind = kind,
        .direction = normalized_or(desc.direction, Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F}),
        .color = desc.color,
        .illuminance_lux = desc.illuminance_lux,
        .angular_radius_radians = desc.angular_radius_radians,
        .visible_disk = desc.visible_disk,
        .affects_atmosphere = desc.affects_atmosphere,
        .affects_clouds = desc.affects_clouds,
        .casts_environment_shadows = desc.casts_environment_shadows,
    };
}

[[nodiscard]] bool weather_has_cloud_layer(EnvironmentWeatherKind weather) noexcept {
    switch (weather) {
    case EnvironmentWeatherKind::cloudy:
    case EnvironmentWeatherKind::rain:
    case EnvironmentWeatherKind::storm:
    case EnvironmentWeatherKind::snow:
        return true;
    case EnvironmentWeatherKind::clear:
    case EnvironmentWeatherKind::foggy:
    case EnvironmentWeatherKind::dust:
    case EnvironmentWeatherKind::ash:
        return false;
    }
    return false;
}

[[nodiscard]] EnvironmentSceneLightRenderRow make_scene_light_row(const SceneRenderLight& light,
                                                                  EnvironmentSceneLightBindingKind binding) noexcept {
    return EnvironmentSceneLightRenderRow{
        .node = light.node,
        .type = light.light.type,
        .direction = scene_light_direction(light.world_from_node),
        .color = light.light.color,
        .intensity = light.light.intensity,
        .casts_shadows = light.light.casts_shadows,
        .binding = binding,
    };
}

void validate_binding(EnvironmentRenderPacket& packet, const SceneRenderPacket& scene_packet,
                      const EnvironmentRenderPacketDesc& desc, const EnvironmentSceneLightBinding& binding,
                      std::size_t binding_index) {
    if (binding.binding == EnvironmentSceneLightBindingKind::none) {
        return;
    }
    if (has_duplicate_binding(desc.light_bindings, binding.node, binding_index)) {
        add_diagnostic(packet, EnvironmentRenderPacketDiagnosticCode::duplicate_light_binding, binding.node,
                       binding_index, "environment scene light binding is duplicated");
        return;
    }
    const auto* light = find_light(scene_packet, binding.node);
    if (light == nullptr) {
        add_diagnostic(packet, EnvironmentRenderPacketDiagnosticCode::missing_bound_light, binding.node, binding_index,
                       "environment scene light binding references a missing light");
        return;
    }
    if (light->light.type != LightType::directional) {
        add_diagnostic(packet, EnvironmentRenderPacketDiagnosticCode::unsupported_bound_light_type, binding.node,
                       binding_index, "environment sun/moon binding requires a directional scene light");
        return;
    }

    const auto scene_direction = scene_light_direction(light->world_from_node);
    const auto environment_direction =
        binding.binding == EnvironmentSceneLightBindingKind::environment_sun
            ? normalized_or(desc.environment->sun.direction, Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F})
            : normalized_or(desc.environment->moon.direction, Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F});
    if (dot(scene_direction, environment_direction) < desc.direction_conflict_cosine_threshold) {
        add_diagnostic(packet,
                       binding.binding == EnvironmentSceneLightBindingKind::environment_sun
                           ? EnvironmentRenderPacketDiagnosticCode::conflicting_environment_sun_direction
                           : EnvironmentRenderPacketDiagnosticCode::conflicting_environment_moon_direction,
                       binding.node, binding_index,
                       "bound scene directional light conflicts with environment celestial direction");
    }
}

} // namespace

bool EnvironmentRenderPacket::succeeded() const noexcept {
    return diagnostics.empty();
}

EnvironmentRenderPacket build_environment_render_packet(const EnvironmentRenderPacketDesc& desc) {
    EnvironmentRenderPacket packet;
    if (desc.scene_packet == nullptr) {
        add_diagnostic(packet, EnvironmentRenderPacketDiagnosticCode::missing_scene_packet, null_scene_node, 0,
                       "environment render packet requires a scene packet");
        return packet;
    }
    if (desc.environment == nullptr) {
        add_diagnostic(packet, EnvironmentRenderPacketDiagnosticCode::missing_environment_profile, null_scene_node, 0,
                       "environment render packet requires an environment profile");
        return packet;
    }

    const auto validation = validate_environment_profile(*desc.environment);
    if (!validation.succeeded()) {
        add_diagnostic(packet, EnvironmentRenderPacketDiagnosticCode::invalid_environment_profile, null_scene_node, 0,
                       "environment profile is invalid");
    }

    packet.profile_id = desc.environment->id;
    if (desc.scene_packet->environment.has_value()) {
        packet.profile_asset = desc.scene_packet->environment->profile;
    }
    if (const auto* primary_camera = desc.scene_packet->primary_camera(); primary_camera != nullptr) {
        packet.primary_camera_node = primary_camera->node;
    } else {
        add_diagnostic(packet, EnvironmentRenderPacketDiagnosticCode::missing_primary_camera, null_scene_node, 0,
                       "environment render packet requires a primary scene camera");
    }

    packet.atmosphere = EnvironmentAtmosphereRenderRow{
        .planet_radius_km = desc.environment->atmosphere.planet_radius_km,
        .atmosphere_height_km = desc.environment->atmosphere.atmosphere_height_km,
        .rayleigh_scattering = desc.environment->atmosphere.rayleigh_scattering,
        .mie_scattering = desc.environment->atmosphere.mie_scattering,
        .ground_albedo = desc.environment->atmosphere.ground_albedo,
    };
    packet.celestial_lights.push_back(make_celestial_light(EnvironmentCelestialLightKind::sun, desc.environment->sun));
    packet.celestial_lights.push_back(
        make_celestial_light(EnvironmentCelestialLightKind::moon, desc.environment->moon));
    packet.sky = EnvironmentSkyRenderRow{
        .model = desc.environment->sky_model,
        .sun_direction = packet.celestial_lights[0].direction,
        .moon_direction = packet.celestial_lights[1].direction,
    };
    packet.fog = EnvironmentFogRenderRow{
        .enabled = desc.environment->fog.enabled,
        .density = desc.environment->fog.density,
        .height_falloff = desc.environment->fog.height_falloff,
        .albedo = desc.environment->fog.albedo,
        .anisotropy = desc.environment->fog.anisotropy,
        .uses_scene_depth = desc.environment->fog.enabled,
    };
    packet.precipitation = EnvironmentPrecipitationRenderRow{
        .kind = desc.environment->precipitation.kind,
        .intensity = desc.environment->precipitation.intensity,
        .particle_radius_mm = desc.environment->precipitation.particle_radius_mm,
        .fall_speed_mps = desc.environment->precipitation.fall_speed_mps,
        .wind_speed_mps = desc.environment->precipitation.wind_speed_mps,
    };
    packet.clouds = EnvironmentCloudRenderRow{
        .enabled = weather_has_cloud_layer(desc.environment->weather),
        .casts_shadows =
            weather_has_cloud_layer(desc.environment->weather) &&
            (desc.environment->sun.casts_environment_shadows || desc.environment->moon.casts_environment_shadows),
    };

    packet.scene_lights.reserve(desc.scene_packet->lights.size());
    for (const auto& light : desc.scene_packet->lights) {
        const auto binding = binding_for_node(desc.light_bindings, light.node);
        if (light.light.type == LightType::directional) {
            ++packet.sky.directional_scene_light_count;
        }
        packet.scene_lights.push_back(make_scene_light_row(light, binding));
    }

    for (std::size_t index = 0; index < desc.light_bindings.size(); ++index) {
        validate_binding(packet, *desc.scene_packet, desc, desc.light_bindings[index], index);
    }

    return packet;
}

EnvironmentRenderPacket build_environment_render_packet(const SceneRenderPacket& scene_packet,
                                                        const EnvironmentProfileDesc& environment) {
    return build_environment_render_packet(
        EnvironmentRenderPacketDesc{.scene_packet = &scene_packet, .environment = &environment});
}

bool has_environment_render_packet_diagnostic(const EnvironmentRenderPacket& packet,
                                              EnvironmentRenderPacketDiagnosticCode code) noexcept {
    return std::ranges::any_of(packet.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
