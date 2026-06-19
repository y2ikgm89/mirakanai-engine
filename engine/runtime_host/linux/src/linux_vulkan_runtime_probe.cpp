// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/linux/linux_vulkan_runtime_probe.hpp"

#include <span>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] std::vector<rhi::vulkan::VulkanEnvironmentWeatherSolverCellRow> make_probe_rows() {
    return {
        rhi::vulkan::VulkanEnvironmentWeatherSolverCellRow{
            .temperature_celsius = 15.0F,
            .vapor_water_kg_per_m2 = 0.001F,
            .cloud_water_kg_per_m2 = 0.004F,
            .surface_water_kg_per_m2 = 0.010F,
            .surface_evaporation_kg_per_m2_s = 0.002F,
            .temperature_delta_celsius_per_s = 0.0F,
            .cloud_precipitation_rate_per_s = 0.5F,
        },
        rhi::vulkan::VulkanEnvironmentWeatherSolverCellRow{
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

LinuxVulkanRuntimeProbeResult run_linux_vulkan_runtime_probe(const LinuxVulkanRuntimeProbeDesc& desc) noexcept {
    LinuxVulkanRuntimeProbeResult result{};
    const auto platform = platform::linux::query_linux_platform_runtime_evidence();
    result.first_party_runtime_host_ready =
        platform.host_matches_linux && platform.first_party_platform_module_ready && !platform.exposes_native_handles;
    result.exposes_native_handles = platform.exposes_native_handles;
    result.surface_family = platform.surface_family;

    if (!result.first_party_runtime_host_ready || desc.weather_solver_spirv.empty()) {
        result.failure_stage = 1U;
        return result;
    }

    const auto rows = make_probe_rows();
    const auto gpu_result =
        rhi::vulkan::dispatch_environment_weather_solver(rhi::vulkan::VulkanEnvironmentWeatherSolverDesc{
            .compute_shader_spirv = desc.weather_solver_spirv,
            .compute_shader_entry_point = "cs_environment_weather",
            .cell_rows = std::span<const rhi::vulkan::VulkanEnvironmentWeatherSolverCellRow>{rows},
            .effective_timestep_s = desc.effective_timestep_s,
            .air_pressure_hpa = desc.air_pressure_hpa,
            .mixing_height_m = desc.mixing_height_m,
        });

    result.cell_count = gpu_result.cell_count;
    result.compute_dispatches = gpu_result.compute_dispatches;
    result.descriptor_set_bindings = gpu_result.descriptor_set_bindings;
    result.resource_barriers_recorded = gpu_result.resource_barriers_recorded;
    result.failure_stage = gpu_result.failure_stage;
    result.output_checksum = gpu_result.output_checksum;
    result.exposes_native_handles = result.exposes_native_handles || gpu_result.exposes_native_handles;
    result.readback_ready = gpu_result.succeeded && gpu_result.executed_gpu_solver &&
                            gpu_result.output_readback_nonzero && gpu_result.output_checksum != 0U &&
                            gpu_result.output_rows.size() == rows.size();
    result.probe_ready = result.first_party_runtime_host_ready && result.readback_ready &&
                         gpu_result.compute_dispatches == 1U && gpu_result.descriptor_set_bindings == 3U &&
                         gpu_result.resource_barriers_recorded >= 2U && !result.exposes_native_handles;
    return result;
}

} // namespace mirakana
