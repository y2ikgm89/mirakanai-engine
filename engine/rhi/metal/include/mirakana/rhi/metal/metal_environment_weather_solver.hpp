// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/metal/metal_backend.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::rhi::metal {

inline constexpr std::uint32_t metal_environment_weather_solver_cell_row_stride_bytes = 32U;
inline constexpr std::uint32_t metal_environment_weather_solver_output_row_stride_bytes = 32U;

struct MetalEnvironmentWeatherSolverCellRow {
    float temperature_celsius{15.0F};
    float vapor_water_kg_per_m2{0.0F};
    float cloud_water_kg_per_m2{0.0F};
    float surface_water_kg_per_m2{0.0F};
    float surface_evaporation_kg_per_m2_s{0.0F};
    float temperature_delta_celsius_per_s{0.0F};
    float cloud_precipitation_rate_per_s{0.0F};
    float padding0{0.0F};
};

struct MetalEnvironmentWeatherSolverOutputRow {
    float temperature_celsius{0.0F};
    float vapor_water_kg_per_m2{0.0F};
    float cloud_water_kg_per_m2{0.0F};
    float surface_water_kg_per_m2{0.0F};
    float saturation_vapor_kg_per_m2{0.0F};
    float evaporated_kg_per_m2{0.0F};
    float condensed_kg_per_m2{0.0F};
    float precipitated_kg_per_m2{0.0F};
};

struct MetalEnvironmentWeatherSolverDesc {
    RhiHostPlatform host{RhiHostPlatform::unknown};
    std::string_view metallib_path;
    std::string_view compute_function_name{"mk_environment_weather_solver"};
    std::span<const MetalEnvironmentWeatherSolverCellRow> cell_rows;
    float effective_timestep_s{0.0F};
    float air_pressure_hpa{1013.25F};
    float mixing_height_m{1000.0F};
};

struct MetalEnvironmentWeatherSolverResult {
    bool succeeded{false};
    bool executed_gpu_solver{false};
    bool output_readback_nonzero{false};
    bool exposes_native_handles{false};
    bool host_evidence_required{false};
    bool backend_parity_ready{false};
    bool d3d12_inferred{false};
    bool vulkan_inferred{false};
    bool command_queue_ready{false};
    bool command_buffer_ready{false};
    bool metallib_valid{false};
    bool compute_pipeline_ready{false};
    std::uint32_t cell_count{0U};
    std::uint32_t compute_dispatches{0U};
    std::uint32_t buffer_bindings{0U};
    std::uint32_t synchronization_points{0U};
    std::uint32_t failure_stage{0U};
    std::uint64_t output_buffer_size_bytes{0U};
    std::uint64_t output_checksum{0U};
    std::vector<MetalEnvironmentWeatherSolverOutputRow> output_rows;
    std::string diagnostic;
};

[[nodiscard]] MetalEnvironmentWeatherSolverResult
dispatch_environment_weather_solver(const MetalEnvironmentWeatherSolverDesc& desc) noexcept;

} // namespace mirakana::rhi::metal
