// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/linux/linux_platform_probe.hpp"
#include "mirakana/rhi/vulkan/vulkan_environment_weather_solver.hpp"

#include <cstdint>
#include <span>

namespace mirakana {

struct LinuxVulkanRuntimeProbeDesc {
    std::span<const std::uint32_t> weather_solver_spirv;
    float effective_timestep_s{0.5F};
    float air_pressure_hpa{1013.25F};
    float mixing_height_m{1000.0F};
};

struct LinuxVulkanRuntimeProbeResult {
    bool first_party_runtime_host_ready{false};
    bool probe_ready{false};
    bool readback_ready{false};
    bool exposes_native_handles{false};
    mirakana::platform::linux::LinuxVulkanSurfaceFamily surface_family{
        mirakana::platform::linux::LinuxVulkanSurfaceFamily::offscreen_compute};
    std::uint32_t cell_count{0U};
    std::uint32_t compute_dispatches{0U};
    std::uint32_t descriptor_set_bindings{0U};
    std::uint32_t resource_barriers_recorded{0U};
    std::uint32_t failure_stage{0U};
    std::uint64_t output_checksum{0U};
};

[[nodiscard]] LinuxVulkanRuntimeProbeResult
run_linux_vulkan_runtime_probe(const LinuxVulkanRuntimeProbeDesc& desc) noexcept;

} // namespace mirakana
