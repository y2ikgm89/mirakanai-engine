// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/environment/environment_io.hpp"
#include "mirakana/environment/environment_preset_pack.hpp"
#include "mirakana/environment/environment_profile.hpp"
#include "mirakana/environment/environment_quality_budget.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

[[nodiscard]] std::size_t diagnostic_count(const mirakana::EnvironmentProfileValidationResult& result,
                                           mirakana::EnvironmentProfileDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("environment profile validation accepts clean first party defaults") {
    mirakana::EnvironmentProfileDesc desc{};
    desc.id = "default_outdoor";

    const auto result = mirakana::validate_environment_profile(desc);

    MK_REQUIRE(result.status == mirakana::EnvironmentProfileValidationStatus::valid);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(mirakana::is_valid_environment_profile(desc));
}

MK_TEST("environment quality budgets expand presets and pass selected high usage") {
    const auto limits = mirakana::environment_quality_budget_preset_limits(mirakana::EnvironmentQualityPreset::high);

    MK_REQUIRE(limits.physical_sky_sample_budget == 108U);
    MK_REQUIRE(limits.height_fog_sample_step_budget == 48U);
    MK_REQUIRE(limits.volumetric_fog_raymarch_step_budget == 64U);
    MK_REQUIRE(limits.volumetric_cloud_primary_step_budget == 64U);
    MK_REQUIRE(limits.volumetric_cloud_light_step_budget == 8U);
    MK_REQUIRE(limits.precipitation_particle_budget == 1U);
    MK_REQUIRE(limits.transient_gpu_byte_budget == 32ULL * 1024ULL * 1024ULL);
    MK_REQUIRE(limits.framegraph_pass_budget == 8U);
    MK_REQUIRE(limits.framegraph_barrier_step_budget == 24U);
    MK_REQUIRE(limits.texture_upload_budget == 6U);
    MK_REQUIRE(limits.renderer_draw_budget == 6U);
    MK_REQUIRE(limits.compute_dispatch_budget == 2U);

    const auto plan = mirakana::evaluate_environment_quality_budget(mirakana::EnvironmentQualityBudgetRequest{
        .preset = mirakana::EnvironmentQualityPreset::high,
        .usage =
            mirakana::EnvironmentQualityBudgetUsageDesc{
                .physical_sky_sample_budget = 108U,
                .height_fog_sample_step_budget = 48U,
                .volumetric_fog_raymarch_step_budget = 48U,
                .volumetric_cloud_primary_step_budget = 48U,
                .volumetric_cloud_light_step_budget = 8U,
                .precipitation_particle_rows = 1U,
                .transient_gpu_byte_estimate = 2ULL * 1024ULL * 1024ULL,
                .framegraph_passes = 4U,
                .framegraph_barrier_steps = 15U,
                .texture_uploads = 4U,
                .renderer_draws = 3U,
                .compute_dispatches = 1U,
            },
    });

    MK_REQUIRE(plan.status == mirakana::EnvironmentQualityBudgetStatus::ready);
    MK_REQUIRE(plan.ready);
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.limits.physical_sky_sample_budget == 108U);
}

MK_TEST("environment quality budgets reject budget violations and broad optimization claims") {
    const auto plan = mirakana::evaluate_environment_quality_budget(mirakana::EnvironmentQualityBudgetRequest{
        .preset = mirakana::EnvironmentQualityPreset::low,
        .usage =
            mirakana::EnvironmentQualityBudgetUsageDesc{
                .physical_sky_sample_budget = 64U,
                .height_fog_sample_step_budget = 20U,
                .volumetric_fog_raymarch_step_budget = 24U,
                .volumetric_cloud_primary_step_budget = 32U,
                .volumetric_cloud_light_step_budget = 4U,
                .precipitation_particle_rows = 1U,
                .transient_gpu_byte_estimate = 9ULL * 1024ULL * 1024ULL,
                .framegraph_passes = 4U,
                .framegraph_barrier_steps = 13U,
                .texture_uploads = 4U,
                .renderer_draws = 4U,
                .compute_dispatches = 1U,
                .broad_optimization_claimed = true,
            },
    });

    MK_REQUIRE(plan.status == mirakana::EnvironmentQualityBudgetStatus::budget_exceeded);
    MK_REQUIRE(!plan.ready);
    MK_REQUIRE(mirakana::has_environment_quality_budget_diagnostic(
        plan, mirakana::EnvironmentQualityBudgetDiagnosticCode::physical_sky_sample_budget_exceeded));
    MK_REQUIRE(mirakana::has_environment_quality_budget_diagnostic(
        plan, mirakana::EnvironmentQualityBudgetDiagnosticCode::height_fog_sample_step_budget_exceeded));
    MK_REQUIRE(mirakana::has_environment_quality_budget_diagnostic(
        plan, mirakana::EnvironmentQualityBudgetDiagnosticCode::transient_gpu_byte_budget_exceeded));
    MK_REQUIRE(mirakana::has_environment_quality_budget_diagnostic(
        plan, mirakana::EnvironmentQualityBudgetDiagnosticCode::framegraph_barrier_step_budget_exceeded));
    MK_REQUIRE(mirakana::has_environment_quality_budget_diagnostic(
        plan, mirakana::EnvironmentQualityBudgetDiagnosticCode::broad_optimization_claimed));
}

MK_TEST("environment quality budgets require explicit custom limits") {
    const auto missing_custom = mirakana::evaluate_environment_quality_budget(mirakana::EnvironmentQualityBudgetRequest{
        .preset = mirakana::EnvironmentQualityPreset::custom,
        .usage = mirakana::EnvironmentQualityBudgetUsageDesc{},
    });

    MK_REQUIRE(missing_custom.status == mirakana::EnvironmentQualityBudgetStatus::invalid_request);
    MK_REQUIRE(mirakana::has_environment_quality_budget_diagnostic(
        missing_custom, mirakana::EnvironmentQualityBudgetDiagnosticCode::missing_custom_limits));

    const auto custom = mirakana::evaluate_environment_quality_budget(mirakana::EnvironmentQualityBudgetRequest{
        .preset = mirakana::EnvironmentQualityPreset::custom,
        .usage =
            mirakana::EnvironmentQualityBudgetUsageDesc{
                .physical_sky_sample_budget = 2U,
                .height_fog_sample_step_budget = 2U,
                .volumetric_fog_raymarch_step_budget = 2U,
                .volumetric_cloud_primary_step_budget = 2U,
                .volumetric_cloud_light_step_budget = 1U,
                .precipitation_particle_rows = 1U,
                .transient_gpu_byte_estimate = 1024U,
                .framegraph_passes = 1U,
                .framegraph_barrier_steps = 1U,
                .texture_uploads = 1U,
                .renderer_draws = 1U,
                .compute_dispatches = 1U,
            },
        .custom_limits =
            mirakana::EnvironmentQualityBudgetLimitsDesc{
                .physical_sky_sample_budget = 2U,
                .height_fog_sample_step_budget = 2U,
                .volumetric_fog_raymarch_step_budget = 2U,
                .volumetric_cloud_primary_step_budget = 2U,
                .volumetric_cloud_light_step_budget = 1U,
                .precipitation_particle_budget = 1U,
                .transient_gpu_byte_budget = 1024U,
                .framegraph_pass_budget = 1U,
                .framegraph_barrier_step_budget = 1U,
                .texture_upload_budget = 1U,
                .renderer_draw_budget = 1U,
                .compute_dispatch_budget = 1U,
            },
        .custom_limits_provided = true,
    });

    MK_REQUIRE(custom.status == mirakana::EnvironmentQualityBudgetStatus::ready);
    MK_REQUIRE(custom.ready);
    MK_REQUIRE(custom.diagnostics.empty());
}

MK_TEST("environment profile validation rejects native backend and editor tokens") {
    mirakana::EnvironmentProfileDesc desc{};
    desc.id = "native_vk_editor_profile";

    const auto result = mirakana::validate_environment_profile(desc);

    MK_REQUIRE(result.status == mirakana::EnvironmentProfileValidationStatus::invalid);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(diagnostic_count(result, mirakana::EnvironmentProfileDiagnosticCode::forbidden_profile_id_token) == 1U);
    MK_REQUIRE(mirakana::has_environment_profile_diagnostic(
        result, mirakana::EnvironmentProfileDiagnosticCode::forbidden_profile_id_token));
    MK_REQUIRE(!mirakana::is_valid_environment_profile(desc));
}

MK_TEST("environment profile collection validation rejects duplicate ids deterministically") {
    mirakana::EnvironmentProfileDesc first{};
    first.id = "overcast_evening";
    mirakana::EnvironmentProfileDesc second{};
    second.id = "overcast_evening";

    const mirakana::EnvironmentProfileDesc profiles[] = {first, second};
    const auto result = mirakana::validate_environment_profiles(profiles);

    MK_REQUIRE(result.status == mirakana::EnvironmentProfileValidationStatus::invalid);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostics.size() == 1U);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::EnvironmentProfileDiagnosticCode::duplicate_profile_id);
    MK_REQUIRE(result.diagnostics[0].profile_index == 1U);
    MK_REQUIRE(result.diagnostics[0].field == "id");
    MK_REQUIRE(result.diagnostics[0].id == "overcast_evening");
}

MK_TEST("environment profile validation rejects invalid weather atmosphere and quality values") {
    mirakana::EnvironmentProfileDesc desc{};
    desc.sun.direction = mirakana::Vec3{};
    desc.atmosphere.atmosphere_height_km = -1.0F;
    desc.fog.density = -0.25F;
    desc.precipitation.kind = mirakana::EnvironmentPrecipitationKind::rain;
    desc.precipitation.intensity = 1.5F;

    const auto result = mirakana::validate_environment_profile(desc);

    MK_REQUIRE(result.status == mirakana::EnvironmentProfileValidationStatus::invalid);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(diagnostic_count(result, mirakana::EnvironmentProfileDiagnosticCode::invalid_direction) == 1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::EnvironmentProfileDiagnosticCode::invalid_scalar) == 3U);
}

MK_TEST("environment profile validation rejects invalid enum casts") {
    mirakana::EnvironmentProfileDesc desc{};
    desc.id = "invalid_enum_case";
    desc.sky_model = static_cast<mirakana::EnvironmentSkyModel>(255U);
    desc.weather = static_cast<mirakana::EnvironmentWeatherKind>(255U);

    const auto result = mirakana::validate_environment_profile(desc);

    MK_REQUIRE(result.status == mirakana::EnvironmentProfileValidationStatus::invalid);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(diagnostic_count(result, mirakana::EnvironmentProfileDiagnosticCode::invalid_enum) == 2U);
}

MK_TEST("environment profile text io round trips with stable key ordering") {
    mirakana::EnvironmentProfileDesc desc{};
    desc.id = "default_outdoor";
    desc.sky_model = mirakana::EnvironmentSkyModel::physical_atmosphere;
    desc.weather = mirakana::EnvironmentWeatherKind::clear;
    desc.sun.direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    desc.sun.illuminance_lux = 100000.0F;
    desc.fog.enabled = false;
    desc.precipitation.kind = mirakana::EnvironmentPrecipitationKind::none;

    const auto serialized = mirakana::serialize_environment_profile(desc);

    MK_REQUIRE(serialized.find("format=GameEngine.EnvironmentProfile.v1\n") == 0U);
    MK_REQUIRE(serialized.find("id=default_outdoor\nsky.model=physical_atmosphere\nweather.kind=clear\n") !=
               std::string::npos);
    MK_REQUIRE(serialized.find("sun.direction=0,-1,0\n") != std::string::npos);
    MK_REQUIRE(serialized.find("sun.illuminance_lux=100000\n") != std::string::npos);
    MK_REQUIRE(serialized.find("fog.enabled=false\n") != std::string::npos);
    MK_REQUIRE(serialized.find("precipitation.kind=none\n") != std::string::npos);

    const auto round_trip = mirakana::deserialize_environment_profile(serialized);
    MK_REQUIRE(round_trip.id == "default_outdoor");
    MK_REQUIRE(round_trip.sky_model == mirakana::EnvironmentSkyModel::physical_atmosphere);
    MK_REQUIRE(round_trip.weather == mirakana::EnvironmentWeatherKind::clear);
    MK_REQUIRE(round_trip.precipitation.kind == mirakana::EnvironmentPrecipitationKind::none);
    MK_REQUIRE(mirakana::serialize_environment_profile(round_trip) == serialized);
}

MK_TEST("environment profile text io rejects malformed documents") {
    for (const auto* document : {
             "format=GameEngine.LegacyEnvironment.v1\nid=default_outdoor\n",
             "format=GameEngine.EnvironmentProfile.v1\nid=default_outdoor\nid=duplicate\n",
             "format=GameEngine.EnvironmentProfile.v1\nid=default_outdoor\nsky.model=backend_handle\n",
             "format=GameEngine.EnvironmentProfile.v1\nid=default_outdoor\nsun.direction=nan,-1,0\n",
             "format=GameEngine.EnvironmentProfile.v1\nid=default_outdoor\nprecipitation.intensity=-0.1\n",
             "format=GameEngine.EnvironmentProfile.v1\nid=native_vk_editor\n",
         }) {
        bool threw = false;
        try {
            (void)mirakana::deserialize_environment_profile(document);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        MK_REQUIRE(threw);
    }
}

MK_TEST("environment profile v2 text io round trips local volumes weather timeline and quality preset") {
    mirakana::EnvironmentProfileDocumentV2 document{};
    document.global_profile.id = "default_outdoor";
    document.global_profile.weather = mirakana::EnvironmentWeatherKind::rain;
    document.quality_preset = mirakana::EnvironmentQualityPreset::high;
    document.volumes.push_back(mirakana::EnvironmentVolumeDesc{
        .id = "mist_valley",
        .shape = mirakana::EnvironmentVolumeShape::box,
        .center_m = mirakana::Vec3{.x = 10.0F, .y = 2.0F, .z = -4.0F},
        .extents_m = mirakana::Vec3{.x = 30.0F, .y = 8.0F, .z = 20.0F},
        .radius_m = 0.0F,
        .priority = 20,
        .blend_weight = 0.75F,
        .fade_distance_m = 12.0F,
        .blend_mode = mirakana::EnvironmentVolumeBlendMode::weighted_override,
        .profile =
            mirakana::EnvironmentProfileDesc{
                .id = "mist_valley_profile",
                .weather = mirakana::EnvironmentWeatherKind::foggy,
                .fog =
                    mirakana::EnvironmentFogDesc{
                        .enabled = true,
                        .density = 0.35F,
                        .height_falloff = 0.45F,
                        .albedo = mirakana::Vec3{.x = 0.82F, .y = 0.88F, .z = 0.94F},
                        .anisotropy = 0.2F,
                    },
            },
    });
    document.weather_timeline.push_back(mirakana::EnvironmentWeatherKeyframeDesc{
        .time_of_day_hours = 6.0F,
        .weather = mirakana::EnvironmentWeatherKind::clear,
        .precipitation = mirakana::EnvironmentPrecipitationKind::none,
        .storm_intensity = 0.0F,
        .cloud_coverage = 0.2F,
        .fog_density = 0.05F,
        .quality_preset = mirakana::EnvironmentQualityPreset::medium,
    });
    document.weather_timeline.push_back(mirakana::EnvironmentWeatherKeyframeDesc{
        .time_of_day_hours = 18.0F,
        .weather = mirakana::EnvironmentWeatherKind::storm,
        .precipitation = mirakana::EnvironmentPrecipitationKind::rain,
        .storm_intensity = 0.8F,
        .cloud_coverage = 0.9F,
        .fog_density = 0.25F,
        .quality_preset = mirakana::EnvironmentQualityPreset::high,
    });

    const auto serialized = mirakana::serialize_environment_profile_v2(document);

    MK_REQUIRE(serialized.starts_with("format=GameEngine.EnvironmentProfile.v2\n"));
    MK_REQUIRE(serialized.contains("quality.preset=high\n"));
    MK_REQUIRE(serialized.contains("volume.0.id=mist_valley\n"));
    MK_REQUIRE(serialized.contains("volume.0.shape=box\n"));
    MK_REQUIRE(serialized.contains("volume.0.priority=20\n"));
    MK_REQUIRE(serialized.contains("volume.0.fade_distance_m=12\n"));
    MK_REQUIRE(serialized.contains("weather_keyframe.1.weather=storm\n"));
    MK_REQUIRE(serialized.contains("weather_keyframe.1.precipitation=rain\n"));

    const auto round_trip = mirakana::deserialize_environment_profile_v2(serialized);
    MK_REQUIRE(round_trip.global_profile.id == "default_outdoor");
    MK_REQUIRE(round_trip.quality_preset == mirakana::EnvironmentQualityPreset::high);
    MK_REQUIRE(round_trip.volumes.size() == 1U);
    MK_REQUIRE(round_trip.volumes[0].id == "mist_valley");
    MK_REQUIRE(round_trip.volumes[0].shape == mirakana::EnvironmentVolumeShape::box);
    MK_REQUIRE(round_trip.volumes[0].priority == 20);
    MK_REQUIRE(round_trip.weather_timeline.size() == 2U);
    MK_REQUIRE(round_trip.weather_timeline[1].weather == mirakana::EnvironmentWeatherKind::storm);
    MK_REQUIRE(mirakana::serialize_environment_profile_v2(round_trip) == serialized);
}

MK_TEST("environment profile v2 text io rejects legacy v1 documents") {
    bool threw = false;
    try {
        (void)mirakana::deserialize_environment_profile_v2("format=GameEngine.EnvironmentProfile.v1\n"
                                                           "id=default_outdoor\n");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    MK_REQUIRE(threw);
}

MK_TEST("environment profile v2 volumes validate supported shapes deterministic order and blend hash") {
    mirakana::EnvironmentProfileDocumentV2 document{};
    document.global_profile.id = "default_outdoor";
    document.volumes = {
        mirakana::EnvironmentVolumeDesc{
            .id = "beta",
            .shape = mirakana::EnvironmentVolumeShape::sphere,
            .center_m = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .radius_m = 10.0F,
            .priority = 3,
            .blend_weight = 1.0F,
            .fade_distance_m = 2.0F,
        },
        mirakana::EnvironmentVolumeDesc{
            .id = "alpha",
            .shape = mirakana::EnvironmentVolumeShape::global,
            .priority = 3,
            .blend_weight = 0.5F,
        },
        mirakana::EnvironmentVolumeDesc{
            .id = "storm_peak",
            .shape = mirakana::EnvironmentVolumeShape::box,
            .extents_m = mirakana::Vec3{.x = 4.0F, .y = 6.0F, .z = 8.0F},
            .priority = 40,
            .blend_weight = 0.9F,
            .fade_distance_m = 1.0F,
        },
    };

    const auto validation = mirakana::validate_environment_profile_v2(document);
    const auto sorted = mirakana::sorted_environment_volume_rows(document);
    const auto first_hash = mirakana::environment_volume_blend_hash(sorted);
    const auto second_hash = mirakana::environment_volume_blend_hash(sorted);

    MK_REQUIRE(validation.succeeded());
    MK_REQUIRE(sorted.size() == 3U);
    MK_REQUIRE(sorted[0].id == "storm_peak");
    MK_REQUIRE(sorted[1].id == "alpha");
    MK_REQUIRE(sorted[2].id == "beta");
    MK_REQUIRE(first_hash != 0U);
    MK_REQUIRE(first_hash == second_hash);
}

MK_TEST("environment profile v2 validation rejects invalid local volume shape and fade distance") {
    mirakana::EnvironmentProfileDocumentV2 document{};
    document.global_profile.id = "default_outdoor";
    document.volumes.push_back(mirakana::EnvironmentVolumeDesc{
        .id = "bad_volume",
        .shape = static_cast<mirakana::EnvironmentVolumeShape>(255U),
        .fade_distance_m = -1.0F,
    });

    const auto validation = mirakana::validate_environment_profile_v2(document);

    MK_REQUIRE(!validation.succeeded());
    MK_REQUIRE(mirakana::has_environment_profile_diagnostic(
        validation, mirakana::EnvironmentProfileDiagnosticCode::invalid_volume_shape));
    MK_REQUIRE(mirakana::has_environment_profile_diagnostic(
        validation, mirakana::EnvironmentProfileDiagnosticCode::invalid_volume_fade));
}

MK_TEST("environment preset pack serializes required commercial governance rows") {
    mirakana::EnvironmentPresetPackDocumentV1 document{};
    document.id = "sample_environment_commercial_presets";
    document.provenance_id = "provenance.environment.sample_commercial_presets";
    document.license_id = "LicenseRef-Proprietary";
    document.art_direction = "first_party_desktop_runtime_environment_reference";
    document.quality_tier = mirakana::EnvironmentQualityPreset::high;
    document.package_size_budget_bytes = 12000U;
    document.installed_size_budget_bytes = 12000U;
    document.decoded_memory_budget_bytes = 56000U;
    document.gpu_memory_budget_bytes = 44000U;
    document.required_backend_feature_rows = {"environment_platform_windows_d3d12_ready"};
    document.presets = {
        mirakana::EnvironmentPresetPackPresetV1{
            .id = "clear_noon",
            .profile_asset_path = "runtime/assets/desktop_runtime/default_outdoor.geenv",
            .art_direction = "clean_high_sun_reference",
            .sky_atmosphere_values = "physical_atmosphere:sun_lux_100000",
            .fog_cloud_weather_timeline = "clear_none_low_fog",
            .ibl_asset_ref = "runtime/assets/desktop_runtime/environment_radiance_exr.texture.geasset",
            .material_weathering_ref = "material_weathering.dry",
            .audio_trigger_intent = "ambient.daylight.clear",
            .quality_budget_id = "environment.quality.high.selected",
            .quality_tier = mirakana::EnvironmentQualityPreset::high,
            .package_size_bytes = 1024U,
            .installed_size_bytes = 1024U,
            .estimated_decoded_memory_bytes = 4096U,
            .estimated_gpu_memory_bytes = 2048U,
            .validation_recipe_id = "desktop-game-runtime",
        },
        mirakana::EnvironmentPresetPackPresetV1{
            .id = "overcast_storm",
            .profile_asset_path = "runtime/assets/desktop_runtime/default_outdoor.geenv",
            .art_direction = "dense_overcast_storm_front",
            .sky_atmosphere_values = "physical_atmosphere:muted_sun_scatter",
            .fog_cloud_weather_timeline = "storm_rain_high_clouds",
            .ibl_asset_ref = "runtime/assets/desktop_runtime/environment_radiance_exr.texture.geasset",
            .material_weathering_ref = "material_weathering.wet",
            .audio_trigger_intent = "ambient.storm.rain",
            .quality_budget_id = "environment.quality.high.selected",
            .quality_tier = mirakana::EnvironmentQualityPreset::high,
            .package_size_bytes = 1024U,
            .installed_size_bytes = 1024U,
            .estimated_decoded_memory_bytes = 8192U,
            .estimated_gpu_memory_bytes = 4096U,
            .validation_recipe_id = "desktop-game-runtime",
        },
        mirakana::EnvironmentPresetPackPresetV1{
            .id = "night_moonlit",
            .profile_asset_path = "runtime/assets/desktop_runtime/default_outdoor.geenv",
            .art_direction = "cool_moonlit_low_key_reference",
            .sky_atmosphere_values = "physical_atmosphere:moon_lux_0_25",
            .fog_cloud_weather_timeline = "clear_night_light_fog",
            .ibl_asset_ref = "runtime/assets/desktop_runtime/environment_skybox_basis.texture.geasset",
            .material_weathering_ref = "material_weathering.dry",
            .audio_trigger_intent = "ambient.night.insects",
            .quality_budget_id = "environment.quality.medium.selected",
            .quality_tier = mirakana::EnvironmentQualityPreset::medium,
            .package_size_bytes = 1024U,
            .installed_size_bytes = 1024U,
            .estimated_decoded_memory_bytes = 4096U,
            .estimated_gpu_memory_bytes = 2048U,
            .validation_recipe_id = "desktop-game-runtime",
        },
        mirakana::EnvironmentPresetPackPresetV1{
            .id = "snowfield",
            .profile_asset_path = "runtime/assets/desktop_runtime/default_outdoor.geenv",
            .art_direction = "high_albedo_snowfield_reference",
            .sky_atmosphere_values = "physical_atmosphere:cold_scatter",
            .fog_cloud_weather_timeline = "snow_medium_clouds",
            .ibl_asset_ref = "runtime/assets/desktop_runtime/environment_skybox_basis.texture.geasset",
            .material_weathering_ref = "material_weathering.snow",
            .audio_trigger_intent = "ambient.snow.wind",
            .quality_budget_id = "environment.quality.high.selected",
            .quality_tier = mirakana::EnvironmentQualityPreset::high,
            .package_size_bytes = 1024U,
            .installed_size_bytes = 1024U,
            .estimated_decoded_memory_bytes = 8192U,
            .estimated_gpu_memory_bytes = 4096U,
            .validation_recipe_id = "desktop-game-runtime",
        },
        mirakana::EnvironmentPresetPackPresetV1{
            .id = "foggy_valley",
            .profile_asset_path = "runtime/assets/desktop_runtime/default_outdoor.geenv",
            .art_direction = "layered_local_fog_valley",
            .sky_atmosphere_values = "physical_atmosphere:aerial_perspective",
            .fog_cloud_weather_timeline = "foggy_low_clouds",
            .ibl_asset_ref = "runtime/assets/desktop_runtime/environment_radiance_exr.texture.geasset",
            .material_weathering_ref = "material_weathering.damp",
            .audio_trigger_intent = "ambient.valley.mist",
            .quality_budget_id = "environment.quality.high.selected",
            .quality_tier = mirakana::EnvironmentQualityPreset::high,
            .package_size_bytes = 1024U,
            .installed_size_bytes = 1024U,
            .estimated_decoded_memory_bytes = 8192U,
            .estimated_gpu_memory_bytes = 4096U,
            .validation_recipe_id = "desktop-game-runtime",
        },
        mirakana::EnvironmentPresetPackPresetV1{
            .id = "cinematic_sunset",
            .profile_asset_path = "runtime/assets/desktop_runtime/default_outdoor.geenv",
            .art_direction = "warm_low_sun_cinematic_reference",
            .sky_atmosphere_values = "physical_atmosphere:low_sun_warm_scatter",
            .fog_cloud_weather_timeline = "sunset_cloud_layer_light_fog",
            .ibl_asset_ref = "runtime/assets/desktop_runtime/environment_radiance_exr.texture.geasset",
            .material_weathering_ref = "material_weathering.dry",
            .audio_trigger_intent = "ambient.sunset.wind",
            .quality_budget_id = "environment.quality.ultra.selected",
            .quality_tier = mirakana::EnvironmentQualityPreset::ultra,
            .package_size_bytes = 1024U,
            .installed_size_bytes = 1024U,
            .estimated_decoded_memory_bytes = 8192U,
            .estimated_gpu_memory_bytes = 4096U,
            .validation_recipe_id = "desktop-game-runtime",
        },
        mirakana::EnvironmentPresetPackPresetV1{
            .id = "indoor_to_outdoor_transition",
            .profile_asset_path = "runtime/assets/desktop_runtime/default_outdoor.geenv",
            .art_direction = "exposure_transition_reference",
            .sky_atmosphere_values = "physical_atmosphere:dynamic_ambient_reference",
            .fog_cloud_weather_timeline = "clear_transition_volume",
            .ibl_asset_ref = "runtime/assets/desktop_runtime/environment_skybox_basis.texture.geasset",
            .material_weathering_ref = "material_weathering.mixed",
            .audio_trigger_intent = "ambient.transition.doorway",
            .quality_budget_id = "environment.quality.medium.selected",
            .quality_tier = mirakana::EnvironmentQualityPreset::medium,
            .package_size_bytes = 1024U,
            .installed_size_bytes = 1024U,
            .estimated_decoded_memory_bytes = 4096U,
            .estimated_gpu_memory_bytes = 2048U,
            .validation_recipe_id = "desktop-game-runtime",
        },
    };

    const auto validation = mirakana::validate_environment_preset_pack_v1(document);
    MK_REQUIRE(validation.succeeded());

    const auto serialized = mirakana::serialize_environment_preset_pack_v1(document);
    MK_REQUIRE(serialized.starts_with("format=GameEngine.EnvironmentPresetPack.v1\n"));
    MK_REQUIRE(serialized.contains("pack.provenance_id=provenance.environment.sample_commercial_presets\n"));
    MK_REQUIRE(serialized.contains("pack.required_backend_feature_row.0=environment_platform_windows_d3d12_ready\n"));
    MK_REQUIRE(serialized.contains("preset.6.id=indoor_to_outdoor_transition\n"));
    MK_REQUIRE(serialized.contains("preset.6.audio_trigger_intent=ambient.transition.doorway\n"));

    const auto round_trip = mirakana::deserialize_environment_preset_pack_v1(serialized);
    MK_REQUIRE(round_trip.presets.size() == 7U);
    MK_REQUIRE(round_trip.presets[1].id == "overcast_storm");
    MK_REQUIRE(round_trip.presets[5].quality_tier == mirakana::EnvironmentQualityPreset::ultra);
    MK_REQUIRE(mirakana::serialize_environment_preset_pack_v1(round_trip) == serialized);
}

MK_TEST("environment preset pack rejects missing provenance budgets and required presets") {
    mirakana::EnvironmentPresetPackDocumentV1 document{};
    document.id = "sample_environment_commercial_presets";
    document.license_id = "LicenseRef-Proprietary";
    document.art_direction = "first_party_desktop_runtime_environment_reference";
    document.quality_tier = mirakana::EnvironmentQualityPreset::high;
    document.required_backend_feature_rows = {"environment_platform_windows_d3d12_ready"};
    document.presets.push_back(mirakana::EnvironmentPresetPackPresetV1{.id = "clear_noon"});

    const auto validation = mirakana::validate_environment_preset_pack_v1(document);

    MK_REQUIRE(!validation.succeeded());
    MK_REQUIRE(mirakana::has_environment_preset_pack_diagnostic(
        validation, mirakana::EnvironmentPresetPackDiagnosticCode::missing_provenance_id));
    MK_REQUIRE(mirakana::has_environment_preset_pack_diagnostic(
        validation, mirakana::EnvironmentPresetPackDiagnosticCode::invalid_budget));
    MK_REQUIRE(mirakana::has_environment_preset_pack_diagnostic(
        validation, mirakana::EnvironmentPresetPackDiagnosticCode::missing_required_preset));
    MK_REQUIRE(mirakana::has_environment_preset_pack_diagnostic(
        validation, mirakana::EnvironmentPresetPackDiagnosticCode::missing_profile_reference));
}

int main() {
    return mirakana::test::run_all();
}
