// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/vulkan/vulkan_backend.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace mirakana::rhi::vulkan {

inline constexpr std::uint32_t vulkan_environment_weather_solver_cell_row_stride_bytes = 32U;
inline constexpr std::uint32_t vulkan_environment_weather_solver_output_row_stride_bytes = 32U;

struct VulkanEnvironmentWeatherSolverCellRow {
    float temperature_celsius{15.0F};
    float vapor_water_kg_per_m2{0.0F};
    float cloud_water_kg_per_m2{0.0F};
    float surface_water_kg_per_m2{0.0F};
    float surface_evaporation_kg_per_m2_s{0.0F};
    float temperature_delta_celsius_per_s{0.0F};
    float cloud_precipitation_rate_per_s{0.0F};
    float padding0{0.0F};
};

struct VulkanEnvironmentWeatherSolverOutputRow {
    float temperature_celsius{0.0F};
    float vapor_water_kg_per_m2{0.0F};
    float cloud_water_kg_per_m2{0.0F};
    float surface_water_kg_per_m2{0.0F};
    float saturation_vapor_kg_per_m2{0.0F};
    float evaporated_kg_per_m2{0.0F};
    float condensed_kg_per_m2{0.0F};
    float precipitated_kg_per_m2{0.0F};
};

struct VulkanEnvironmentWeatherSolverDesc {
    std::span<const std::uint32_t> compute_shader_spirv;
    std::string_view compute_shader_entry_point{"cs_environment_weather"};
    std::span<const VulkanEnvironmentWeatherSolverCellRow> cell_rows;
    float effective_timestep_s{0.0F};
    float air_pressure_hpa{1013.25F};
    float mixing_height_m{1000.0F};
};

struct VulkanEnvironmentWeatherSolverResult {
    bool succeeded{false};
    bool executed_gpu_solver{false};
    bool output_readback_nonzero{false};
    bool exposes_native_handles{false};
    std::uint32_t cell_count{0U};
    std::uint32_t compute_dispatches{0U};
    std::uint32_t descriptor_set_bindings{0U};
    std::uint32_t resource_barriers_recorded{0U};
    std::uint32_t failure_stage{0U};
    std::uint64_t output_buffer_size_bytes{0U};
    std::uint64_t output_checksum{0U};
    std::vector<VulkanEnvironmentWeatherSolverOutputRow> output_rows;
};

[[nodiscard]] VulkanEnvironmentWeatherSolverResult
dispatch_environment_weather_solver(const VulkanEnvironmentWeatherSolverDesc& desc) noexcept;

} // namespace mirakana::rhi::vulkan
