// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/rhi/d3d12/d3d12_environment_weather_solver.hpp"

#include <cmath>
#include <span>
#include <vector>

namespace {

[[nodiscard]] bool d3d12_warp_available() noexcept {
    const auto probe = mirakana::rhi::d3d12::probe_runtime();
    return probe.windows_sdk_available && probe.warp_device_supported;
}

[[nodiscard]] bool nearly_equal(const float lhs, const float rhs) noexcept {
    return std::fabs(lhs - rhs) <= 0.0001F;
}

[[nodiscard]] std::vector<mirakana::rhi::d3d12::D3d12EnvironmentWeatherSolverCellRow> make_weather_rows() {
    return {
        mirakana::rhi::d3d12::D3d12EnvironmentWeatherSolverCellRow{
            .temperature_celsius = 15.0F,
            .vapor_water_kg_per_m2 = 0.001F,
            .cloud_water_kg_per_m2 = 0.004F,
            .surface_water_kg_per_m2 = 0.010F,
            .surface_evaporation_kg_per_m2_s = 0.002F,
            .temperature_delta_celsius_per_s = 0.0F,
            .cloud_precipitation_rate_per_s = 0.5F,
        },
        mirakana::rhi::d3d12::D3d12EnvironmentWeatherSolverCellRow{
            .temperature_celsius = 10.0F,
            .vapor_water_kg_per_m2 = 0.001F,
            .cloud_water_kg_per_m2 = 0.002F,
            .surface_water_kg_per_m2 = 0.006F,
            .surface_evaporation_kg_per_m2_s = 0.001F,
            .temperature_delta_celsius_per_s = 0.0F,
            .cloud_precipitation_rate_per_s = 0.25F,
        },
    };
}

} // namespace

MK_TEST("d3d12 environment weather solver reports unavailable warp without native handles") {
    if (d3d12_warp_available()) {
        return;
    }

    const auto rows = make_weather_rows();
    const auto result = mirakana::rhi::d3d12::dispatch_environment_weather_solver(
        mirakana::rhi::d3d12::D3d12EnvironmentWeatherSolverDesc{
            .device = mirakana::rhi::d3d12::DeviceBootstrapDesc{.prefer_warp = true, .enable_debug_layer = false},
            .cell_rows = std::span<const mirakana::rhi::d3d12::D3d12EnvironmentWeatherSolverCellRow>{rows},
            .effective_timestep_s = 0.5F,
            .air_pressure_hpa = 1013.25F,
            .mixing_height_m = 1000.0F,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.executed_gpu_solver);
    MK_REQUIRE(!result.exposes_native_handles);
}

MK_TEST("d3d12 environment weather solver executes selected water transfer on warp") {
    if (!d3d12_warp_available()) {
        return;
    }

    const auto rows = make_weather_rows();
    const auto result = mirakana::rhi::d3d12::dispatch_environment_weather_solver(
        mirakana::rhi::d3d12::D3d12EnvironmentWeatherSolverDesc{
            .device = mirakana::rhi::d3d12::DeviceBootstrapDesc{.prefer_warp = true, .enable_debug_layer = false},
            .cell_rows = std::span<const mirakana::rhi::d3d12::D3d12EnvironmentWeatherSolverCellRow>{rows},
            .effective_timestep_s = 0.5F,
            .air_pressure_hpa = 1013.25F,
            .mixing_height_m = 1000.0F,
        });

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.executed_gpu_solver);
    MK_REQUIRE(result.cell_count == 2U);
    MK_REQUIRE(result.compute_dispatches == 1U);
    MK_REQUIRE(result.resource_barriers_recorded > 0U);
    MK_REQUIRE(result.output_readback_nonzero);
    MK_REQUIRE(result.output_checksum != 0U);
    MK_REQUIRE(!result.exposes_native_handles);
    MK_REQUIRE(result.output_rows.size() == 2U);
    MK_REQUIRE(nearly_equal(result.output_rows[0].vapor_water_kg_per_m2, 0.002F));
    MK_REQUIRE(nearly_equal(result.output_rows[0].cloud_water_kg_per_m2, 0.003F));
    MK_REQUIRE(nearly_equal(result.output_rows[0].surface_water_kg_per_m2, 0.010F));
    MK_REQUIRE(nearly_equal(result.output_rows[0].evaporated_kg_per_m2, 0.001F));
    MK_REQUIRE(nearly_equal(result.output_rows[0].precipitated_kg_per_m2, 0.001F));
}

int main() {
    return mirakana::test::run_all();
}
