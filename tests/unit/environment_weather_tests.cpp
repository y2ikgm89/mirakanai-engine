// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/environment/weather.hpp"

#include <array>
#include <limits>

namespace {

[[nodiscard]] mirakana::EnvironmentProfileDesc
make_weather_profile(mirakana::EnvironmentWeatherKind weather, mirakana::EnvironmentPrecipitationKind precipitation) {
    mirakana::EnvironmentProfileDesc profile{};
    profile.id = "weather_test";
    profile.weather = weather;
    profile.precipitation = mirakana::EnvironmentPrecipitationDesc{
        .kind = precipitation,
        .intensity = precipitation == mirakana::EnvironmentPrecipitationKind::none ? 0.0F : 0.65F,
        .particle_radius_mm = 0.7F,
        .fall_speed_mps = precipitation == mirakana::EnvironmentPrecipitationKind::snow ? 1.2F : 7.5F,
        .wind_speed_mps = 4.0F,
    };
    return profile;
}

[[nodiscard]] mirakana::EnvironmentPrecipitationPlanDesc
make_precipitation_desc(mirakana::EnvironmentWeatherKind weather,
                        mirakana::EnvironmentPrecipitationKind precipitation) {
    return mirakana::EnvironmentPrecipitationPlanDesc{
        .environment = make_weather_profile(weather, precipitation),
        .scene_geometry_occlusion_required = true,
        .occlusion_policy_available = true,
    };
}

} // namespace

MK_TEST("environment weather planning emits deterministic rows for supported weather kinds") {
    struct ExpectedWeather {
        mirakana::EnvironmentWeatherKind weather;
        mirakana::EnvironmentPrecipitationKind precipitation;
        bool expects_particles;
        bool expects_wetness;
    };

    constexpr std::array expected_rows{
        ExpectedWeather{
            .weather = mirakana::EnvironmentWeatherKind::clear,
            .precipitation = mirakana::EnvironmentPrecipitationKind::none,
            .expects_particles = false,
            .expects_wetness = false,
        },
        ExpectedWeather{
            .weather = mirakana::EnvironmentWeatherKind::cloudy,
            .precipitation = mirakana::EnvironmentPrecipitationKind::none,
            .expects_particles = false,
            .expects_wetness = false,
        },
        ExpectedWeather{
            .weather = mirakana::EnvironmentWeatherKind::rain,
            .precipitation = mirakana::EnvironmentPrecipitationKind::rain,
            .expects_particles = true,
            .expects_wetness = true,
        },
        ExpectedWeather{
            .weather = mirakana::EnvironmentWeatherKind::storm,
            .precipitation = mirakana::EnvironmentPrecipitationKind::rain,
            .expects_particles = true,
            .expects_wetness = true,
        },
        ExpectedWeather{
            .weather = mirakana::EnvironmentWeatherKind::snow,
            .precipitation = mirakana::EnvironmentPrecipitationKind::snow,
            .expects_particles = true,
            .expects_wetness = false,
        },
        ExpectedWeather{
            .weather = mirakana::EnvironmentWeatherKind::foggy,
            .precipitation = mirakana::EnvironmentPrecipitationKind::none,
            .expects_particles = false,
            .expects_wetness = false,
        },
        ExpectedWeather{
            .weather = mirakana::EnvironmentWeatherKind::dust,
            .precipitation = mirakana::EnvironmentPrecipitationKind::dust,
            .expects_particles = true,
            .expects_wetness = false,
        },
        ExpectedWeather{
            .weather = mirakana::EnvironmentWeatherKind::ash,
            .precipitation = mirakana::EnvironmentPrecipitationKind::ash,
            .expects_particles = true,
            .expects_wetness = false,
        },
    };

    for (const auto expected : expected_rows) {
        const auto plan =
            mirakana::plan_environment_precipitation(make_precipitation_desc(expected.weather, expected.precipitation));

        MK_REQUIRE(plan.succeeded());
        MK_REQUIRE(plan.status == mirakana::EnvironmentPrecipitationPlanStatus::planned);
        MK_REQUIRE(!plan.invokes_backend);
        MK_REQUIRE(!plan.exposes_native_handles);
        MK_REQUIRE(!plan.mutates_materials);
        MK_REQUIRE(!plan.plays_audio);
        MK_REQUIRE(plan.weather_rows.size() == 1);
        MK_REQUIRE(plan.weather_rows[0].weather == expected.weather);
        MK_REQUIRE(plan.weather_rows[0].precipitation_kind == expected.precipitation);
        MK_REQUIRE(plan.weather_rows[0].precipitation_enabled == expected.expects_particles);
        MK_REQUIRE(plan.weather_rows[0].wet_surface_enabled == expected.expects_wetness);

        if (expected.expects_particles) {
            MK_REQUIRE(plan.particle_rows.size() == 1);
            MK_REQUIRE(plan.particle_rows[0].kind == expected.precipitation);
            MK_REQUIRE(plan.particle_rows[0].intensity == 0.65F);
            MK_REQUIRE(plan.particle_rows[0].spawn_rate_per_second > 0.0F);
            MK_REQUIRE(plan.occlusion_rows.size() == 1);
            MK_REQUIRE(plan.occlusion_rows[0].required);
            MK_REQUIRE(plan.occlusion_rows[0].available);
        } else {
            MK_REQUIRE(plan.particle_rows.empty());
        }

        if (expected.expects_wetness) {
            MK_REQUIRE(plan.wetness_rows.size() == 1);
            MK_REQUIRE(plan.wetness_rows[0].enabled);
            MK_REQUIRE(plan.wetness_rows[0].splash_intent);
            MK_REQUIRE(plan.wetness_rows[0].ripple_intent);
            MK_REQUIRE(!plan.wetness_rows[0].mutates_materials);
        }
    }
}

MK_TEST("environment weather planning emits audio handoff rows without playing audio") {
    const auto plan = mirakana::plan_environment_precipitation(
        make_precipitation_desc(mirakana::EnvironmentWeatherKind::storm, mirakana::EnvironmentPrecipitationKind::rain));

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(!plan.plays_audio);
    MK_REQUIRE(plan.audio_handoff_rows.size() >= 3);
    MK_REQUIRE(mirakana::has_environment_precipitation_audio_cue(
        plan, mirakana::EnvironmentPrecipitationAudioCueKind::rain_loop));
    MK_REQUIRE(mirakana::has_environment_precipitation_audio_cue(
        plan, mirakana::EnvironmentPrecipitationAudioCueKind::indoor_muffling));
    MK_REQUIRE(mirakana::has_environment_precipitation_audio_cue(
        plan, mirakana::EnvironmentPrecipitationAudioCueKind::thunder_delay));
}

MK_TEST("environment weather planning emits snow package rows without wetness or side effects") {
    const auto plan = mirakana::plan_environment_precipitation(
        make_precipitation_desc(mirakana::EnvironmentWeatherKind::snow, mirakana::EnvironmentPrecipitationKind::snow));

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.weather_rows.size() == 1);
    MK_REQUIRE(plan.weather_rows[0].weather == mirakana::EnvironmentWeatherKind::snow);
    MK_REQUIRE(plan.weather_rows[0].precipitation_kind == mirakana::EnvironmentPrecipitationKind::snow);
    MK_REQUIRE(plan.weather_rows[0].precipitation_enabled);
    MK_REQUIRE(!plan.weather_rows[0].wet_surface_enabled);
    MK_REQUIRE(plan.particle_rows.size() == 1);
    MK_REQUIRE(plan.particle_rows[0].kind == mirakana::EnvironmentPrecipitationKind::snow);
    MK_REQUIRE(plan.wetness_rows.empty());
    MK_REQUIRE(!plan.audio_handoff_rows.empty());
    MK_REQUIRE(mirakana::has_environment_precipitation_audio_cue(
        plan, mirakana::EnvironmentPrecipitationAudioCueKind::snow_loop));
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.mutates_materials);
    MK_REQUIRE(!plan.plays_audio);
}

MK_TEST("environment weather planning fails closed for invalid values and unsupported side effects") {
    auto desc =
        make_precipitation_desc(mirakana::EnvironmentWeatherKind::rain, mirakana::EnvironmentPrecipitationKind::rain);
    desc.environment.weather = static_cast<mirakana::EnvironmentWeatherKind>(255);
    desc.environment.precipitation.kind = static_cast<mirakana::EnvironmentPrecipitationKind>(255);
    desc.environment.precipitation.intensity = 1.2F;
    desc.environment.precipitation.particle_radius_mm = 0.0F;
    desc.environment.precipitation.fall_speed_mps = -1.0F;
    desc.environment.precipitation.wind_speed_mps = std::numeric_limits<float>::quiet_NaN();
    desc.scene_geometry_occlusion_required = true;
    desc.occlusion_policy_available = false;
    desc.request_backend_execution = true;
    desc.request_native_handle_access = true;
    desc.request_material_mutation = true;
    desc.request_audio_playback = true;

    const auto plan = mirakana::plan_environment_precipitation(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::EnvironmentPrecipitationPlanStatus::blocked);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.mutates_materials);
    MK_REQUIRE(!plan.plays_audio);
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::unsupported_weather_kind));
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::unsupported_precipitation_kind));
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::invalid_intensity));
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::invalid_particle_radius));
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::invalid_fall_speed));
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::invalid_wind_speed));
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::missing_occlusion_policy));
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::unsupported_backend_execution));
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::unsupported_native_handle_claim));
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::unsupported_material_mutation));
    MK_REQUIRE(mirakana::has_environment_precipitation_diagnostic(
        plan, mirakana::EnvironmentPrecipitationDiagnosticCode::unsupported_audio_playback));
}

int main() {
    return mirakana::test::run_all();
}
