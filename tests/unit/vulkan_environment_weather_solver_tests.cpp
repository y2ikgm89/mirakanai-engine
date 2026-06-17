// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/rhi/vulkan/vulkan_environment_weather_solver.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace {

struct SpirvArtifactFromEnvironment {
    bool configured{false};
    std::vector<std::uint32_t> words;
    std::string diagnostic;
};

#if defined(_MSC_VER)
[[nodiscard]] std::string environment_variable_value(const char* name) {
    char* value = nullptr;
    std::size_t value_size = 0;
    if (_dupenv_s(&value, &value_size, name) != 0 || value == nullptr) {
        return {};
    }

    std::string result{value};
    std::free(value);
    return result;
}
#else
[[nodiscard]] std::string environment_variable_value(const char* name) {
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
}
#endif

[[nodiscard]] SpirvArtifactFromEnvironment load_spirv_artifact_from_environment(const char* environment_variable) {
    const auto path = environment_variable_value(environment_variable);
    if (path.empty()) {
        return SpirvArtifactFromEnvironment{.configured = false, .words = {}, .diagnostic = "not configured"};
    }

    std::ifstream input{path, std::ios::binary | std::ios::ate};
    if (!input) {
        return SpirvArtifactFromEnvironment{
            .configured = true, .words = {}, .diagnostic = std::string{"unable to open "} + environment_variable};
    }

    const auto size = input.tellg();
    if (size <= 0 || size % static_cast<std::streamoff>(sizeof(std::uint32_t)) != 0) {
        return SpirvArtifactFromEnvironment{.configured = true,
                                            .words = {},
                                            .diagnostic =
                                                std::string{"invalid SPIR-V byte size in "} + environment_variable};
    }

    input.seekg(0, std::ios::beg);
    std::vector<std::uint32_t> words(
        static_cast<std::size_t>(size / static_cast<std::streamoff>(sizeof(std::uint32_t))));
    input.read(reinterpret_cast<char*>(words.data()), size);
    if (!input) {
        return SpirvArtifactFromEnvironment{
            .configured = true, .words = {}, .diagnostic = std::string{"failed to read "} + environment_variable};
    }

    return SpirvArtifactFromEnvironment{.configured = true, .words = std::move(words), .diagnostic = "loaded"};
}

[[nodiscard]] bool nearly_equal(const float lhs, const float rhs) noexcept {
    return std::fabs(lhs - rhs) <= 0.0001F;
}

[[nodiscard]] std::vector<mirakana::rhi::vulkan::VulkanEnvironmentWeatherSolverCellRow> make_weather_rows() {
    return {
        mirakana::rhi::vulkan::VulkanEnvironmentWeatherSolverCellRow{
            .temperature_celsius = 15.0F,
            .vapor_water_kg_per_m2 = 0.001F,
            .cloud_water_kg_per_m2 = 0.004F,
            .surface_water_kg_per_m2 = 0.010F,
            .surface_evaporation_kg_per_m2_s = 0.002F,
            .temperature_delta_celsius_per_s = 0.0F,
            .cloud_precipitation_rate_per_s = 0.5F,
        },
        mirakana::rhi::vulkan::VulkanEnvironmentWeatherSolverCellRow{
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

MK_TEST("vulkan environment weather solver fails closed when spir-v artifact is not configured") {
#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))
    const auto compute_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_ENVIRONMENT_WEATHER_SOLVER_SPV");
    if (compute_artifact.configured) {
        return;
    }

    const auto rows = make_weather_rows();
    const auto result = mirakana::rhi::vulkan::dispatch_environment_weather_solver(
        mirakana::rhi::vulkan::VulkanEnvironmentWeatherSolverDesc{
            .compute_shader_spirv = {},
            .compute_shader_entry_point = "cs_environment_weather",
            .cell_rows = std::span<const mirakana::rhi::vulkan::VulkanEnvironmentWeatherSolverCellRow>{rows},
            .effective_timestep_s = 0.5F,
            .air_pressure_hpa = 1013.25F,
            .mixing_height_m = 1000.0F,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.executed_gpu_solver);
    MK_REQUIRE(!result.output_readback_nonzero);
    MK_REQUIRE(!result.exposes_native_handles);
    MK_REQUIRE(result.cell_count == 2U);
    MK_REQUIRE(result.output_buffer_size_bytes ==
               2U * mirakana::rhi::vulkan::vulkan_environment_weather_solver_output_row_stride_bytes);
#endif
}

MK_TEST("vulkan environment weather solver executes selected water transfer with configured spir-v") {
#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))
    const auto compute_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_ENVIRONMENT_WEATHER_SOLVER_SPV");
    if (!compute_artifact.configured) {
        return;
    }
    MK_REQUIRE(compute_artifact.diagnostic == "loaded");

    const auto rows = make_weather_rows();
    const auto result = mirakana::rhi::vulkan::dispatch_environment_weather_solver(
        mirakana::rhi::vulkan::VulkanEnvironmentWeatherSolverDesc{
            .compute_shader_spirv = std::span<const std::uint32_t>{compute_artifact.words},
            .compute_shader_entry_point = "cs_environment_weather",
            .cell_rows = std::span<const mirakana::rhi::vulkan::VulkanEnvironmentWeatherSolverCellRow>{rows},
            .effective_timestep_s = 0.5F,
            .air_pressure_hpa = 1013.25F,
            .mixing_height_m = 1000.0F,
        });

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.executed_gpu_solver);
    MK_REQUIRE(result.cell_count == 2U);
    MK_REQUIRE(result.compute_dispatches == 1U);
    MK_REQUIRE(result.descriptor_set_bindings == 3U);
    MK_REQUIRE(result.resource_barriers_recorded >= 2U);
    MK_REQUIRE(result.output_readback_nonzero);
    MK_REQUIRE(result.output_checksum != 0U);
    MK_REQUIRE(!result.exposes_native_handles);
    MK_REQUIRE(result.output_rows.size() == 2U);
    MK_REQUIRE(nearly_equal(result.output_rows[0].vapor_water_kg_per_m2, 0.002F));
    MK_REQUIRE(nearly_equal(result.output_rows[0].cloud_water_kg_per_m2, 0.003F));
    MK_REQUIRE(nearly_equal(result.output_rows[0].surface_water_kg_per_m2, 0.010F));
    MK_REQUIRE(nearly_equal(result.output_rows[0].evaporated_kg_per_m2, 0.001F));
    MK_REQUIRE(nearly_equal(result.output_rows[0].precipitated_kg_per_m2, 0.001F));
#endif
}

int main() {
    return mirakana::test::run_all();
}
