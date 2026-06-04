// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/volumetric_cloud_policy.hpp"

#include <array>
#include <limits>
#include <span>

namespace {

[[nodiscard]] mirakana::VolumetricCloudAtmosphericLightDesc make_light(mirakana::Vec3 direction,
                                                                       std::uint32_t source_index) {
    return mirakana::VolumetricCloudAtmosphericLightDesc{
        .direction = direction,
        .color = mirakana::Vec3{.x = 1.0F, .y = 0.92F, .z = 0.82F},
        .illuminance_lux = source_index == 0U ? 100000.0F : 0.25F,
        .casts_cloud_shadows = true,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::VolumetricCloudPolicyDesc
make_valid_cloud_desc(std::span<const mirakana::VolumetricCloudAtmosphericLightDesc> lights) {
    return mirakana::VolumetricCloudPolicyDesc{
        .layer =
            mirakana::VolumetricCloudLayerDesc{
                .weather_map_asset_ref = "environment/clouds/storm_weather_map",
                .shape_noise_asset_ref = "environment/clouds/storm_shape_noise_3d",
                .erosion_noise_asset_ref = "environment/clouds/storm_erosion_noise_3d",
                .coverage = 0.68F,
                .density = 0.42F,
                .altitude_min_m = 1600.0F,
                .altitude_max_m = 7200.0F,
                .wind_velocity_mps = mirakana::Vec2{.x = 18.0F, .y = -3.0F},
            },
        .quality_tier = mirakana::VolumetricCloudQualityTier::high,
        .raymarch =
            mirakana::VolumetricCloudRaymarchBudgetDesc{
                .primary_steps = 96,
                .light_steps = 16,
                .shadow_mode = mirakana::VolumetricCloudShadowMode::beer_shadow_map_intent,
                .temporal_reprojection_enabled = true,
                .temporal_history_weight = 0.88F,
            },
        .atmospheric_lights = lights,
        .storm =
            mirakana::VolumetricCloudStormDesc{
                .enabled = true,
                .lightning_flash_intensity = 42000.0F,
                .lightning_direction = mirakana::Vec3{.x = -0.2F, .y = -1.0F, .z = 0.1F},
                .thunder_delay_seconds = 2.4F,
                .cloud_darkening = 0.35F,
                .precipitation_boost = 0.45F,
                .wind_gust_mps = 14.0F,
                .exposure_response = 0.6F,
            },
        .shader_contract_evidence_ready = true,
    };
}

} // namespace

MK_TEST("renderer volumetric cloud policy plans maps volume lighting raymarch and shader rows") {
    const std::array lights = {
        make_light(mirakana::Vec3{.x = 0.25F, .y = -1.0F, .z = 0.1F}, 0U),
        make_light(mirakana::Vec3{.x = -0.2F, .y = -0.9F, .z = 0.15F}, 1U),
    };

    const auto plan = mirakana::plan_volumetric_cloud_policy(make_valid_cloud_desc(lights));

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::VolumetricCloudPolicyStatus::planned);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.uses_volumetric_clouds);
    MK_REQUIRE(plan.shader_contract_evidence_ready);
    MK_REQUIRE(!plan.execution_evidence_ready);
    MK_REQUIRE(!plan.uploads_volume_textures);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.plays_audio);
    MK_REQUIRE(!plan.executes_precipitation);

    MK_REQUIRE(plan.map_rows.size() == 1);
    MK_REQUIRE(plan.map_rows[0].weather_map_asset_ref == "environment/clouds/storm_weather_map");
    MK_REQUIRE(plan.map_rows[0].shape_noise_asset_ref == "environment/clouds/storm_shape_noise_3d");
    MK_REQUIRE(plan.map_rows[0].erosion_noise_asset_ref == "environment/clouds/storm_erosion_noise_3d");
    MK_REQUIRE(plan.map_rows[0].weather_map_binding_slot == mirakana::volumetric_cloud_weather_map_binding());
    MK_REQUIRE(plan.map_rows[0].shape_noise_binding_slot == mirakana::volumetric_cloud_shape_noise_binding());
    MK_REQUIRE(plan.map_rows[0].erosion_noise_binding_slot == mirakana::volumetric_cloud_erosion_noise_binding());

    MK_REQUIRE(plan.layer_rows.size() == 1);
    MK_REQUIRE(plan.layer_rows[0].coverage == 0.68F);
    MK_REQUIRE(plan.layer_rows[0].density == 0.42F);
    MK_REQUIRE(plan.layer_rows[0].altitude_min_m == 1600.0F);
    MK_REQUIRE(plan.layer_rows[0].altitude_max_m == 7200.0F);
    MK_REQUIRE(plan.layer_rows[0].wind_velocity_mps.x == 18.0F);

    MK_REQUIRE(plan.lighting_rows.size() == 2);
    MK_REQUIRE(plan.lighting_rows[0].source_index == 0U);
    MK_REQUIRE(plan.lighting_rows[1].source_index == 1U);
    MK_REQUIRE(plan.lighting_rows[0].casts_cloud_shadows);

    MK_REQUIRE(plan.raymarch_rows.size() == 1);
    MK_REQUIRE(plan.raymarch_rows[0].primary_steps == 96U);
    MK_REQUIRE(plan.raymarch_rows[0].light_steps == 16U);
    MK_REQUIRE(plan.raymarch_rows[0].shadow_mode == mirakana::VolumetricCloudShadowMode::beer_shadow_map_intent);

    MK_REQUIRE(plan.temporal_rows.size() == 1);
    MK_REQUIRE(plan.temporal_rows[0].enabled);
    MK_REQUIRE(plan.temporal_rows[0].history_weight == 0.88F);

    MK_REQUIRE(plan.shader_contract_rows.size() == 1);
    MK_REQUIRE(plan.shader_contract_rows[0].constants_binding_slot == mirakana::volumetric_cloud_constants_binding());
    MK_REQUIRE(plan.shader_contract_rows[0].sampler_binding_slot == mirakana::volumetric_cloud_sampler_binding());
    MK_REQUIRE(plan.shader_contract_rows[0].shader_contract_evidence_ready);
}

MK_TEST("renderer volumetric cloud policy emits storm rows without executing audio or precipitation") {
    const std::array lights = {
        make_light(mirakana::Vec3{.x = 0.25F, .y = -1.0F, .z = 0.1F}, 0U),
    };

    const auto plan = mirakana::plan_volumetric_cloud_policy(make_valid_cloud_desc(lights));

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(!plan.plays_audio);
    MK_REQUIRE(!plan.executes_precipitation);
    MK_REQUIRE(plan.storm_rows.size() == 1);
    MK_REQUIRE(plan.storm_rows[0].lightning_flash_intensity == 42000.0F);
    MK_REQUIRE(plan.storm_rows[0].thunder_delay_seconds == 2.4F);
    MK_REQUIRE(plan.storm_rows[0].cloud_darkening == 0.35F);
    MK_REQUIRE(plan.storm_rows[0].precipitation_boost == 0.45F);
    MK_REQUIRE(plan.storm_rows[0].wind_gust_mps == 14.0F);
    MK_REQUIRE(plan.storm_rows[0].exposure_response == 0.6F);
}

MK_TEST("renderer volumetric cloud policy requires execution evidence before ready promotion") {
    const std::array lights = {
        make_light(mirakana::Vec3{.x = 0.25F, .y = -1.0F, .z = 0.1F}, 0U),
    };
    auto desc = make_valid_cloud_desc(lights);
    desc.request_ready_promotion = true;

    const auto blocked_plan = mirakana::plan_volumetric_cloud_policy(desc);

    MK_REQUIRE(!blocked_plan.succeeded());
    MK_REQUIRE(blocked_plan.status == mirakana::VolumetricCloudPolicyStatus::blocked);
    MK_REQUIRE(!blocked_plan.ready());
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        blocked_plan, mirakana::VolumetricCloudDiagnosticCode::missing_execution_evidence));

    desc.execution_evidence_ready = true;
    const auto ready_plan = mirakana::plan_volumetric_cloud_policy(desc);

    MK_REQUIRE(ready_plan.succeeded());
    MK_REQUIRE(ready_plan.status == mirakana::VolumetricCloudPolicyStatus::ready);
    MK_REQUIRE(ready_plan.ready());
    MK_REQUIRE(ready_plan.execution_evidence_ready);
}

MK_TEST("renderer volumetric cloud policy fails closed for invalid rows and unsupported claims") {
    const std::array lights = {
        mirakana::VolumetricCloudAtmosphericLightDesc{
            .direction = mirakana::Vec3{},
            .color = mirakana::Vec3{.x = 1.2F, .y = -0.1F, .z = 0.5F},
            .illuminance_lux = -1.0F,
            .casts_cloud_shadows = true,
            .source_index = 0U,
        },
        make_light(mirakana::Vec3{.x = -0.2F, .y = -0.9F, .z = 0.15F}, 1U),
        make_light(mirakana::Vec3{.x = 0.1F, .y = -0.8F, .z = -0.25F}, 2U),
    };
    auto desc = make_valid_cloud_desc(lights);
    desc.layer.weather_map_asset_ref = "native/d3d12/weather";
    desc.layer.shape_noise_asset_ref = "";
    desc.layer.erosion_noise_asset_ref = "../unsafe";
    desc.layer.coverage = 1.2F;
    desc.layer.density = -0.1F;
    desc.layer.altitude_min_m = 8000.0F;
    desc.layer.altitude_max_m = 1000.0F;
    desc.layer.wind_velocity_mps.x = std::numeric_limits<float>::quiet_NaN();
    desc.quality_tier = mirakana::VolumetricCloudQualityTier::unknown;
    desc.raymarch.primary_steps = 0U;
    desc.raymarch.light_steps = 999U;
    desc.raymarch.shadow_mode = mirakana::VolumetricCloudShadowMode::unknown;
    desc.raymarch.temporal_history_weight = 1.2F;
    desc.storm.lightning_flash_intensity = -1.0F;
    desc.storm.lightning_direction = mirakana::Vec3{};
    desc.storm.thunder_delay_seconds = -0.5F;
    desc.storm.cloud_darkening = 1.2F;
    desc.storm.precipitation_boost = -0.1F;
    desc.storm.wind_gust_mps = std::numeric_limits<float>::infinity();
    desc.storm.exposure_response = -0.1F;
    desc.shader_contract_evidence_ready = false;
    desc.request_volume_texture_upload = true;
    desc.request_backend_execution = true;
    desc.request_native_handle_access = true;
    desc.request_audio_playback = true;
    desc.request_precipitation_execution = true;

    const auto plan = mirakana::plan_volumetric_cloud_policy(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::VolumetricCloudPolicyStatus::blocked);
    MK_REQUIRE(!plan.uploads_volume_textures);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.plays_audio);
    MK_REQUIRE(!plan.executes_precipitation);
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::invalid_weather_map_reference));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::invalid_shape_noise_reference));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::invalid_erosion_noise_reference));
    MK_REQUIRE(
        mirakana::has_volumetric_cloud_diagnostic(plan, mirakana::VolumetricCloudDiagnosticCode::invalid_coverage));
    MK_REQUIRE(
        mirakana::has_volumetric_cloud_diagnostic(plan, mirakana::VolumetricCloudDiagnosticCode::invalid_density));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::invalid_altitude_range));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::invalid_wind_velocity));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::unsupported_quality_tier));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::invalid_lighting_source_count));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(plan, mirakana::VolumetricCloudDiagnosticCode::invalid_light));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::invalid_raymarch_budget));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::invalid_temporal_reprojection));
    MK_REQUIRE(
        mirakana::has_volumetric_cloud_diagnostic(plan, mirakana::VolumetricCloudDiagnosticCode::invalid_cloud_shadow));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::invalid_storm_lightning));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::missing_shader_contract_evidence));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::unsupported_volume_texture_upload));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::unsupported_backend_execution));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::unsupported_native_handle_claim));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::unsupported_audio_playback));
    MK_REQUIRE(mirakana::has_volumetric_cloud_diagnostic(
        plan, mirakana::VolumetricCloudDiagnosticCode::unsupported_precipitation_execution));
}

int main() {
    return mirakana::test::run_all();
}
