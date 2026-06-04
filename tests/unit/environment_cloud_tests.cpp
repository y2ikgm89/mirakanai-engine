// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/environment/cloud_layer.hpp"

#include <limits>

namespace {

[[nodiscard]] mirakana::EnvironmentCloudLayerDesc make_valid_cloud_layer() {
    return mirakana::EnvironmentCloudLayerDesc{
        .mode = mirakana::EnvironmentCloudLayerMode::equirectangular_2d,
        .coverage = 0.62F,
        .opacity = 0.78F,
        .altitude_m = 2500.0F,
        .wind_velocity_mps = mirakana::Vec2{.x = 8.0F, .y = -2.0F},
        .cloud_map_asset_ref = "environment/clouds/soft_cumulus_latlong",
        .flow_map_asset_ref = "environment/clouds/soft_cumulus_flow",
        .sky_tint_response = mirakana::Vec3{.x = 0.82F, .y = 0.88F, .z = 1.0F},
        .time_of_day_response = 0.7F,
        .ibl_contribution_mode = mirakana::EnvironmentCloudIblContributionMode::sky_tint_only,
        .ibl_contribution = 0.25F,
    };
}

} // namespace

MK_TEST("environment cloud layer validation accepts cheap sky cloud layer values") {
    const auto result = mirakana::validate_environment_cloud_layer(make_valid_cloud_layer());

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::EnvironmentCloudLayerValidationStatus::valid);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(mirakana::is_valid_environment_cloud_layer(make_valid_cloud_layer()));
}

MK_TEST("environment cloud layer validation rejects invalid ranges and unsafe claims") {
    auto desc = make_valid_cloud_layer();
    desc.mode = mirakana::EnvironmentCloudLayerMode::volumetric;
    desc.coverage = 1.2F;
    desc.opacity = -0.1F;
    desc.altitude_m = 0.0F;
    desc.wind_velocity_mps.x = std::numeric_limits<float>::quiet_NaN();
    desc.cloud_map_asset_ref = "native/d3d12/cloud";
    desc.flow_map_asset_ref = "";
    desc.sky_tint_response.y = 1.5F;
    desc.time_of_day_response = -0.5F;
    desc.ibl_contribution_mode = mirakana::EnvironmentCloudIblContributionMode::unknown;
    desc.ibl_contribution = 2.0F;
    desc.request_volumetric_clouds = true;
    desc.request_native_handle_access = true;

    const auto result = mirakana::validate_environment_cloud_layer(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::EnvironmentCloudLayerValidationStatus::invalid);
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::unsupported_mode));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::invalid_coverage));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::invalid_opacity));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::invalid_altitude));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::invalid_wind_velocity));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::invalid_cloud_map_reference));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::invalid_flow_map_reference));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::invalid_sky_tint_response));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::invalid_time_of_day_response));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::invalid_ibl_contribution_mode));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::invalid_ibl_contribution));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::unsupported_volumetric_clouds));
    MK_REQUIRE(mirakana::has_environment_cloud_layer_diagnostic(
        result, mirakana::EnvironmentCloudLayerDiagnosticCode::unsupported_native_handle_claim));
}

int main() {
    return mirakana::test::run_all();
}
