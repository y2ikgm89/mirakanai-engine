// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/scene_renderer/environment_renderer.hpp"

#include <array>

namespace {

[[nodiscard]] mirakana::SceneRenderPacket make_environment_scene_packet(mirakana::SceneNodeId light_node) {
    mirakana::SceneRenderPacket packet;
    packet.environment = mirakana::SceneEnvironmentReference{
        .profile = mirakana::AssetId::from_name("environment/default_outdoor"),
        .required = true,
    };
    packet.cameras.push_back(mirakana::SceneRenderCamera{
        .node = mirakana::SceneNodeId{7},
        .world_from_node = mirakana::Mat4::identity(),
        .camera = mirakana::CameraComponent{.primary = true},
    });
    packet.lights.push_back(mirakana::SceneRenderLight{
        .node = light_node,
        .world_from_node = mirakana::Mat4::identity(),
        .light =
            mirakana::LightComponent{
                .type = mirakana::LightType::directional,
                .color = mirakana::Vec3{.x = 1.0F, .y = 0.92F, .z = 0.82F},
                .intensity = 16.0F,
                .casts_shadows = true,
            },
    });
    return packet;
}

[[nodiscard]] mirakana::EnvironmentProfileDesc make_environment_profile() {
    mirakana::EnvironmentProfileDesc profile;
    profile.id = "default_outdoor";
    profile.sky_model = mirakana::EnvironmentSkyModel::physical_atmosphere;
    profile.sun.direction = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = -1.0F};
    profile.sun.illuminance_lux = 90000.0F;
    profile.weather = mirakana::EnvironmentWeatherKind::rain;
    profile.fog.enabled = true;
    profile.fog.density = 0.25F;
    profile.precipitation.kind = mirakana::EnvironmentPrecipitationKind::rain;
    profile.precipitation.intensity = 0.4F;
    return profile;
}

} // namespace

MK_TEST("scene environment renderer builds deterministic packet rows") {
    constexpr mirakana::SceneNodeId sun_node{42};
    const auto scene_packet = make_environment_scene_packet(sun_node);
    const auto environment = make_environment_profile();
    const std::array bindings = {
        mirakana::EnvironmentSceneLightBinding{
            .node = sun_node,
            .binding = mirakana::EnvironmentSceneLightBindingKind::environment_sun,
        },
    };

    const auto packet = mirakana::build_environment_render_packet(mirakana::EnvironmentRenderPacketDesc{
        .scene_packet = &scene_packet,
        .environment = &environment,
        .light_bindings = bindings,
    });

    MK_REQUIRE(packet.succeeded());
    MK_REQUIRE(packet.profile_id == "default_outdoor");
    MK_REQUIRE(packet.profile_asset == mirakana::AssetId::from_name("environment/default_outdoor"));
    MK_REQUIRE(packet.primary_camera_node == mirakana::SceneNodeId{7});
    MK_REQUIRE(packet.celestial_lights.size() == 2);
    MK_REQUIRE(packet.celestial_lights[0].kind == mirakana::EnvironmentCelestialLightKind::sun);
    MK_REQUIRE(packet.celestial_lights[0].direction == (mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = -1.0F}));
    MK_REQUIRE(packet.celestial_lights[0].illuminance_lux == 90000.0F);
    MK_REQUIRE(packet.scene_lights.size() == 1);
    MK_REQUIRE(packet.scene_lights[0].node == sun_node);
    MK_REQUIRE(packet.scene_lights[0].binding == mirakana::EnvironmentSceneLightBindingKind::environment_sun);
    MK_REQUIRE(packet.sky.model == mirakana::EnvironmentSkyModel::physical_atmosphere);
    MK_REQUIRE(packet.sky.directional_scene_light_count == 1);
    MK_REQUIRE(packet.fog.enabled);
    MK_REQUIRE(packet.fog.uses_scene_depth);
    MK_REQUIRE(packet.precipitation.kind == mirakana::EnvironmentPrecipitationKind::rain);
    MK_REQUIRE(packet.precipitation.intensity == 0.4F);
    MK_REQUIRE(packet.clouds.enabled);
    MK_REQUIRE(packet.clouds.casts_shadows);
}

MK_TEST("scene environment renderer rejects conflicting bound sun directions") {
    constexpr mirakana::SceneNodeId sun_node{42};
    const auto scene_packet = make_environment_scene_packet(sun_node);
    mirakana::EnvironmentProfileDesc environment = make_environment_profile();
    environment.sun.direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    const std::array bindings = {
        mirakana::EnvironmentSceneLightBinding{
            .node = sun_node,
            .binding = mirakana::EnvironmentSceneLightBindingKind::environment_sun,
        },
    };

    const auto packet = mirakana::build_environment_render_packet(mirakana::EnvironmentRenderPacketDesc{
        .scene_packet = &scene_packet,
        .environment = &environment,
        .light_bindings = bindings,
    });

    MK_REQUIRE(!packet.succeeded());
    MK_REQUIRE(mirakana::has_environment_render_packet_diagnostic(
        packet, mirakana::EnvironmentRenderPacketDiagnosticCode::conflicting_environment_sun_direction));
}

int main() {
    return mirakana::test::run_all();
}
