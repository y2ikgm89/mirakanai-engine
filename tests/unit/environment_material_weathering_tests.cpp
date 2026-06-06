// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/environment/material_weathering.hpp"

#include <limits>

namespace {

[[nodiscard]] mirakana::EnvironmentProfileDesc make_profile(mirakana::EnvironmentWeatherKind weather,
                                                            mirakana::EnvironmentPrecipitationKind precipitation,
                                                            float intensity) {
    mirakana::EnvironmentProfileDesc profile{};
    profile.id = "environment_material_weathering";
    profile.weather = weather;
    profile.precipitation = mirakana::EnvironmentPrecipitationDesc{
        .kind = precipitation,
        .intensity = intensity,
        .particle_radius_mm = 1.0F,
        .fall_speed_mps = 2.0F,
        .wind_speed_mps = 0.5F,
    };
    return profile;
}

} // namespace

MK_TEST("environment material weathering emits dry wet snow icy and mixed value rows without mutations") {
    const auto dry = mirakana::plan_environment_material_weathering(mirakana::EnvironmentMaterialWeatheringDesc{
        .environment =
            make_profile(mirakana::EnvironmentWeatherKind::clear, mirakana::EnvironmentPrecipitationKind::none, 0.0F),
    });
    MK_REQUIRE(dry.succeeded());
    MK_REQUIRE(dry.rows.size() == 1);
    MK_REQUIRE(dry.rows[0].state == mirakana::EnvironmentMaterialWeatheringState::dry);
    MK_REQUIRE(dry.wet_rows.empty());
    MK_REQUIRE(dry.snow_rows.empty());
    MK_REQUIRE(!dry.mutates_source_materials);

    const auto wet = mirakana::plan_environment_material_weathering(mirakana::EnvironmentMaterialWeatheringDesc{
        .environment =
            make_profile(mirakana::EnvironmentWeatherKind::rain, mirakana::EnvironmentPrecipitationKind::rain, 0.65F),
    });
    MK_REQUIRE(wet.succeeded());
    MK_REQUIRE(wet.rows[0].state == mirakana::EnvironmentMaterialWeatheringState::wet);
    MK_REQUIRE(wet.wet_rows.size() == 1);
    MK_REQUIRE(wet.wet_rows[0].intensity == 0.65F);
    MK_REQUIRE(wet.snow_rows.empty());
    MK_REQUIRE(!wet.wet_rows[0].mutates_source_material);

    const auto snow = mirakana::plan_environment_material_weathering(mirakana::EnvironmentMaterialWeatheringDesc{
        .environment =
            make_profile(mirakana::EnvironmentWeatherKind::snow, mirakana::EnvironmentPrecipitationKind::snow, 0.55F),
    });
    MK_REQUIRE(snow.succeeded());
    MK_REQUIRE(snow.rows[0].state == mirakana::EnvironmentMaterialWeatheringState::snow_covered);
    MK_REQUIRE(snow.wet_rows.empty());
    MK_REQUIRE(snow.snow_rows.size() == 1);
    MK_REQUIRE(snow.snow_rows[0].coverage == 0.55F);
    MK_REQUIRE(!snow.snow_rows[0].mutates_source_material);

    const auto icy = mirakana::plan_environment_material_weathering(mirakana::EnvironmentMaterialWeatheringDesc{
        .environment =
            make_profile(mirakana::EnvironmentWeatherKind::clear, mirakana::EnvironmentPrecipitationKind::none, 0.0F),
        .ice_intensity = 0.4F,
    });
    MK_REQUIRE(icy.succeeded());
    MK_REQUIRE(icy.rows[0].state == mirakana::EnvironmentMaterialWeatheringState::icy);
    MK_REQUIRE(icy.ice_rows.size() == 1);

    const auto mixed = mirakana::plan_environment_material_weathering(mirakana::EnvironmentMaterialWeatheringDesc{
        .environment =
            make_profile(mirakana::EnvironmentWeatherKind::storm, mirakana::EnvironmentPrecipitationKind::rain, 0.75F),
        .snow_accumulation = 0.5F,
        .ice_intensity = 0.25F,
    });
    MK_REQUIRE(mixed.succeeded());
    MK_REQUIRE(mixed.rows[0].state == mirakana::EnvironmentMaterialWeatheringState::mixed);
    MK_REQUIRE(mixed.wet_rows.size() == 1);
    MK_REQUIRE(mixed.snow_rows.size() == 1);
    MK_REQUIRE(mixed.ice_rows.size() == 1);
    MK_REQUIRE(!mixed.mutates_source_materials);
    MK_REQUIRE(!mixed.invokes_backend);
    MK_REQUIRE(!mixed.exposes_native_handles);
}

MK_TEST("environment material weathering fails closed for invalid values and unsupported side effects") {
    auto desc = mirakana::EnvironmentMaterialWeatheringDesc{
        .environment =
            make_profile(mirakana::EnvironmentWeatherKind::rain, mirakana::EnvironmentPrecipitationKind::rain, 0.8F),
        .wetness_intensity = std::numeric_limits<float>::quiet_NaN(),
        .snow_accumulation = 1.5F,
        .ice_intensity = -0.1F,
        .request_backend_execution = true,
        .request_native_handle_access = true,
        .request_source_material_mutation = true,
    };

    const auto plan = mirakana::plan_environment_material_weathering(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::EnvironmentMaterialWeatheringPlanStatus::blocked);
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.mutates_source_materials);
    MK_REQUIRE(mirakana::has_environment_material_weathering_diagnostic(
        plan, mirakana::EnvironmentMaterialWeatheringDiagnosticCode::invalid_wetness_intensity));
    MK_REQUIRE(mirakana::has_environment_material_weathering_diagnostic(
        plan, mirakana::EnvironmentMaterialWeatheringDiagnosticCode::invalid_snow_accumulation));
    MK_REQUIRE(mirakana::has_environment_material_weathering_diagnostic(
        plan, mirakana::EnvironmentMaterialWeatheringDiagnosticCode::invalid_ice_intensity));
    MK_REQUIRE(mirakana::has_environment_material_weathering_diagnostic(
        plan, mirakana::EnvironmentMaterialWeatheringDiagnosticCode::unsupported_backend_execution));
    MK_REQUIRE(mirakana::has_environment_material_weathering_diagnostic(
        plan, mirakana::EnvironmentMaterialWeatheringDiagnosticCode::unsupported_native_handle_claim));
    MK_REQUIRE(mirakana::has_environment_material_weathering_diagnostic(
        plan, mirakana::EnvironmentMaterialWeatheringDiagnosticCode::unsupported_source_material_mutation));
}

int main() {
    return mirakana::test::run_all();
}
