// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/environment/time_of_day.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <span>
#include <utility>

namespace {

[[nodiscard]] mirakana::EnvironmentProfileDesc
make_time_profile(std::string id, mirakana::EnvironmentWeatherKind weather, float precipitation_intensity) {
    mirakana::EnvironmentProfileDesc profile{};
    profile.id = std::move(id);
    profile.weather = weather;
    profile.precipitation = mirakana::EnvironmentPrecipitationDesc{
        .kind = weather == mirakana::EnvironmentWeatherKind::storm ? mirakana::EnvironmentPrecipitationKind::rain
                                                                   : mirakana::EnvironmentPrecipitationKind::none,
        .intensity = precipitation_intensity,
        .particle_radius_mm = 0.8F,
        .fall_speed_mps = 7.0F,
        .wind_speed_mps = 5.0F,
    };
    return profile;
}

[[nodiscard]] mirakana::EnvironmentTimeOfDayPlanDesc
make_valid_time_desc(std::span<const mirakana::EnvironmentWeatherBlendLayerDesc> blend_layers) {
    return mirakana::EnvironmentTimeOfDayPlanDesc{
        .normalized_day_time = 0.25F,
        .moon_phase = 0.6F,
        .profile_blends = blend_layers,
        .weather_transition =
            mirakana::EnvironmentWeatherTransitionDesc{
                .from_weather = mirakana::EnvironmentWeatherKind::clear,
                .to_weather = mirakana::EnvironmentWeatherKind::storm,
                .duration_seconds = 120.0F,
                .elapsed_seconds = 60.0F,
                .blend_mode = mirakana::EnvironmentWeatherBlendMode::smoothstep,
            },
        .exposure =
            mirakana::EnvironmentExposureIntentDesc{
                .fixed_exposure_ev100 = 12.0F,
                .min_exposure_ev100 = -4.0F,
                .max_exposure_ev100 = 16.0F,
                .auto_exposure_enabled = true,
                .adaptation_seconds = 1.5F,
            },
    };
}

[[nodiscard]] float length_of(mirakana::Vec3 value) {
    return std::sqrt(mirakana::dot(value, value));
}

} // namespace

MK_TEST("environment time of day planning emits celestial blend exposure transition and hash rows") {
    const std::array blend_layers = {
        mirakana::EnvironmentWeatherBlendLayerDesc{
            .profile = make_time_profile("clear_morning", mirakana::EnvironmentWeatherKind::clear, 0.0F),
            .weight = 0.25F,
        },
        mirakana::EnvironmentWeatherBlendLayerDesc{
            .profile = make_time_profile("storm_evening", mirakana::EnvironmentWeatherKind::storm, 0.8F),
            .weight = 0.75F,
        },
    };

    const auto plan = mirakana::plan_environment_time_of_day(make_valid_time_desc(blend_layers));

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::EnvironmentTimeOfDayPlanStatus::planned);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.mutates_profiles);
    MK_REQUIRE(plan.replay_hash != 0U);

    MK_REQUIRE(plan.celestial_rows.size() == 2);
    MK_REQUIRE(plan.celestial_rows[0].body == mirakana::EnvironmentCelestialBodyKind::sun);
    MK_REQUIRE(plan.celestial_rows[0].atmosphere_light_index == 0U);
    MK_REQUIRE(plan.celestial_rows[0].direction.y < -0.9F);
    MK_REQUIRE(length_of(plan.celestial_rows[0].direction) > 0.99F);
    MK_REQUIRE(length_of(plan.celestial_rows[0].direction) < 1.01F);
    MK_REQUIRE(plan.celestial_rows[1].body == mirakana::EnvironmentCelestialBodyKind::moon);
    MK_REQUIRE(plan.celestial_rows[1].atmosphere_light_index == 1U);
    MK_REQUIRE(plan.celestial_rows[1].moon_phase == 0.6F);

    MK_REQUIRE(plan.profile_blend_rows.size() == 2);
    MK_REQUIRE(plan.profile_blend_rows[0].profile_id == "clear_morning");
    MK_REQUIRE(plan.profile_blend_rows[0].normalized_weight == 0.25F);
    MK_REQUIRE(plan.profile_blend_rows[1].profile_id == "storm_evening");
    MK_REQUIRE(plan.profile_blend_rows[1].normalized_weight == 0.75F);

    MK_REQUIRE(plan.weather_transition_rows.size() == 1);
    MK_REQUIRE(plan.weather_transition_rows[0].from_weather == mirakana::EnvironmentWeatherKind::clear);
    MK_REQUIRE(plan.weather_transition_rows[0].to_weather == mirakana::EnvironmentWeatherKind::storm);
    MK_REQUIRE(plan.weather_transition_rows[0].duration_seconds == 120.0F);
    MK_REQUIRE(plan.weather_transition_rows[0].elapsed_seconds == 60.0F);
    MK_REQUIRE(plan.weather_transition_rows[0].blend_weight == 0.5F);

    MK_REQUIRE(plan.exposure_rows.size() == 1);
    MK_REQUIRE(plan.exposure_rows[0].fixed_exposure_ev100 == 12.0F);
    MK_REQUIRE(plan.exposure_rows[0].min_exposure_ev100 == -4.0F);
    MK_REQUIRE(plan.exposure_rows[0].max_exposure_ev100 == 16.0F);
    MK_REQUIRE(plan.exposure_rows[0].auto_exposure_enabled);
    MK_REQUIRE(plan.exposure_rows[0].adaptation_seconds == 1.5F);
}

MK_TEST("environment time of day planning keeps replay hashes stable and input-sensitive") {
    const std::array blend_layers = {
        mirakana::EnvironmentWeatherBlendLayerDesc{
            .profile = make_time_profile("clear_morning", mirakana::EnvironmentWeatherKind::clear, 0.0F),
            .weight = 1.0F,
        },
    };
    auto desc = make_valid_time_desc(blend_layers);

    const auto first = mirakana::plan_environment_time_of_day(desc);
    const auto second = mirakana::plan_environment_time_of_day(desc);
    desc.normalized_day_time = 0.5F;
    const auto changed = mirakana::plan_environment_time_of_day(desc);

    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(second.succeeded());
    MK_REQUIRE(changed.succeeded());
    MK_REQUIRE(first.replay_hash == second.replay_hash);
    MK_REQUIRE(first.replay_hash != changed.replay_hash);
}

MK_TEST("environment time of day planning fails closed for invalid rows and unsupported side effects") {
    std::array blend_layers = {
        mirakana::EnvironmentWeatherBlendLayerDesc{
            .profile = make_time_profile("native/d3d12/profile", mirakana::EnvironmentWeatherKind::clear, 0.0F),
            .weight = -0.25F,
        },
        mirakana::EnvironmentWeatherBlendLayerDesc{
            .profile = make_time_profile("storm_evening", mirakana::EnvironmentWeatherKind::storm, 0.8F),
            .weight = std::numeric_limits<float>::quiet_NaN(),
        },
    };
    auto desc = make_valid_time_desc(blend_layers);
    desc.normalized_day_time = 1.2F;
    desc.moon_phase = -0.1F;
    desc.weather_transition.duration_seconds = 0.0F;
    desc.weather_transition.elapsed_seconds = -1.0F;
    desc.weather_transition.blend_mode = mirakana::EnvironmentWeatherBlendMode::unknown;
    desc.exposure.fixed_exposure_ev100 = 64.0F;
    desc.exposure.min_exposure_ev100 = 16.0F;
    desc.exposure.max_exposure_ev100 = 8.0F;
    desc.exposure.adaptation_seconds = 0.0F;
    desc.request_backend_execution = true;
    desc.request_native_handle_access = true;
    desc.request_profile_mutation = true;

    const auto plan = mirakana::plan_environment_time_of_day(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::EnvironmentTimeOfDayPlanStatus::blocked);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.mutates_profiles);
    MK_REQUIRE(plan.celestial_rows.empty());
    MK_REQUIRE(plan.profile_blend_rows.empty());
    MK_REQUIRE(plan.weather_transition_rows.empty());
    MK_REQUIRE(plan.exposure_rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::invalid_day_time));
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::invalid_moon_phase));
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::invalid_environment_profile));
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::invalid_profile_blend_weight));
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::invalid_transition_duration));
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::invalid_transition_elapsed));
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::unsupported_blend_mode));
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::invalid_exposure));
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::unsupported_backend_execution));
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::unsupported_native_handle_claim));
    MK_REQUIRE(mirakana::has_environment_time_of_day_diagnostic(
        plan, mirakana::EnvironmentTimeOfDayDiagnosticCode::unsupported_profile_mutation));
}

int main() {
    return mirakana::test::run_all();
}
