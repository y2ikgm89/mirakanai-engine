// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/metal/metal_environment_weather_solver.hpp"

#include "test_framework.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace {

[[nodiscard]] std::array<mirakana::rhi::metal::MetalEnvironmentWeatherSolverCellRow, 4> make_weather_solver_cells() {
    using mirakana::rhi::metal::MetalEnvironmentWeatherSolverCellRow;
    return {
        MetalEnvironmentWeatherSolverCellRow{
            .temperature_celsius = 12.0F,
            .vapor_water_kg_per_m2 = 0.011F,
            .cloud_water_kg_per_m2 = 0.002F,
            .surface_water_kg_per_m2 = 1.0F,
            .surface_evaporation_kg_per_m2_s = 0.0004F,
            .cloud_precipitation_rate_per_s = 0.10F,
        },
        MetalEnvironmentWeatherSolverCellRow{
            .temperature_celsius = 4.0F,
            .vapor_water_kg_per_m2 = 0.008F,
            .cloud_water_kg_per_m2 = 0.006F,
            .surface_water_kg_per_m2 = 0.5F,
            .surface_evaporation_kg_per_m2_s = 0.0001F,
            .cloud_precipitation_rate_per_s = 0.12F,
        },
        MetalEnvironmentWeatherSolverCellRow{
            .temperature_celsius = -3.0F,
            .vapor_water_kg_per_m2 = 0.004F,
            .cloud_water_kg_per_m2 = 0.003F,
            .surface_water_kg_per_m2 = 0.25F,
            .surface_evaporation_kg_per_m2_s = 0.0F,
            .cloud_precipitation_rate_per_s = 0.06F,
        },
        MetalEnvironmentWeatherSolverCellRow{
            .temperature_celsius = 20.0F,
            .vapor_water_kg_per_m2 = 0.017F,
            .cloud_water_kg_per_m2 = 0.001F,
            .surface_water_kg_per_m2 = 2.0F,
            .surface_evaporation_kg_per_m2_s = 0.0008F,
            .cloud_precipitation_rate_per_s = 0.02F,
        },
    };
}

} // namespace

[[nodiscard]] std::string environment_variable_value(const char* name) {
#if defined(_MSC_VER)
    char* value = nullptr;
    std::size_t value_size = 0;
    if (_dupenv_s(&value, &value_size, name) != 0 || value == nullptr) {
        return {};
    }

    std::string result{value};
    std::free(value);
    return result;
#else
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
#endif
}

MK_TEST("metal environment weather solver remains host gated on non Apple hosts") {
    const auto cells = make_weather_solver_cells();
    const auto result = mirakana::rhi::metal::dispatch_environment_weather_solver(
        mirakana::rhi::metal::MetalEnvironmentWeatherSolverDesc{
            .host = mirakana::rhi::RhiHostPlatform::windows,
            .metallib_path = "missing.metallib",
            .cell_rows = cells,
            .effective_timestep_s = 0.5F,
            .air_pressure_hpa = 1013.25F,
            .mixing_height_m = 1200.0F,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.executed_gpu_solver);
    MK_REQUIRE(result.host_evidence_required);
    MK_REQUIRE(result.failure_stage == 1U);
    MK_REQUIRE(result.cell_count == cells.size());
    MK_REQUIRE(result.output_rows.empty());
    MK_REQUIRE(result.output_checksum == 0U);
    MK_REQUIRE(!result.exposes_native_handles);
    MK_REQUIRE(!result.backend_parity_ready);
    MK_REQUIRE(!result.d3d12_inferred);
    MK_REQUIRE(!result.vulkan_inferred);
    MK_REQUIRE(!result.command_queue_ready);
    MK_REQUIRE(!result.command_buffer_ready);
    MK_REQUIRE(!result.metallib_valid);
    MK_REQUIRE(!result.compute_pipeline_ready);
    MK_REQUIRE(result.diagnostic == "Metal environment weather solver requires an Apple host");
}

MK_TEST("metal environment weather solver validates selected package contract before backend readiness") {
    const auto empty_cells = mirakana::rhi::metal::dispatch_environment_weather_solver(
        mirakana::rhi::metal::MetalEnvironmentWeatherSolverDesc{
            .host = mirakana::rhi::RhiHostPlatform::macos,
            .metallib_path = "weather.metallib",
        });
    MK_REQUIRE(!empty_cells.succeeded);
    MK_REQUIRE(empty_cells.failure_stage == 2U);
    MK_REQUIRE(empty_cells.diagnostic == "Metal environment weather solver cells are required");

    const auto cells = make_weather_solver_cells();
    const auto missing_library = mirakana::rhi::metal::dispatch_environment_weather_solver(
        mirakana::rhi::metal::MetalEnvironmentWeatherSolverDesc{
            .host = mirakana::rhi::RhiHostPlatform::macos,
            .cell_rows = cells,
            .effective_timestep_s = 0.5F,
            .air_pressure_hpa = 1013.25F,
            .mixing_height_m = 1200.0F,
        });
    MK_REQUIRE(!missing_library.succeeded);
    MK_REQUIRE(missing_library.failure_stage == 3U);
    MK_REQUIRE(missing_library.diagnostic == "Metal environment weather solver metallib path is required");
    MK_REQUIRE(missing_library.cell_count == cells.size());
}

MK_TEST("metal environment weather solver executes selected water transfer on Apple host") {
#if defined(__APPLE__)
    const auto metallib_path = environment_variable_value("MK_METAL_TEST_ENVIRONMENT_WEATHER_SOLVER_METALLIB");
    MK_REQUIRE(!metallib_path.empty());

    const auto cells = make_weather_solver_cells();
    const auto result = mirakana::rhi::metal::dispatch_environment_weather_solver(
        mirakana::rhi::metal::MetalEnvironmentWeatherSolverDesc{
            .host = mirakana::rhi::current_rhi_host_platform(),
            .metallib_path = metallib_path,
            .cell_rows = cells,
            .effective_timestep_s = 0.5F,
            .air_pressure_hpa = 1013.25F,
            .mixing_height_m = 1200.0F,
        });

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.executed_gpu_solver);
    MK_REQUIRE(!result.host_evidence_required);
    MK_REQUIRE(result.failure_stage == 0U);
    MK_REQUIRE(result.cell_count == cells.size());
    MK_REQUIRE(result.compute_dispatches == 1U);
    MK_REQUIRE(result.buffer_bindings == 3U);
    MK_REQUIRE(result.synchronization_points == 1U);
    MK_REQUIRE(result.command_queue_ready);
    MK_REQUIRE(result.command_buffer_ready);
    MK_REQUIRE(result.metallib_valid);
    MK_REQUIRE(result.compute_pipeline_ready);
    MK_REQUIRE(result.output_readback_nonzero);
    MK_REQUIRE(result.output_checksum != 0U);
    MK_REQUIRE(!result.exposes_native_handles);
    MK_REQUIRE(!result.backend_parity_ready);
    MK_REQUIRE(!result.d3d12_inferred);
    MK_REQUIRE(!result.vulkan_inferred);
    MK_REQUIRE(result.output_rows.size() == cells.size());
#endif
}

int main() {
    return mirakana::test::run_all();
}
