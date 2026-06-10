// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/vulkan/vulkan_backend.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace mirakana::rhi::vulkan {

inline constexpr std::uint32_t vulkan_mavg_gpu_culling_dispatch_cluster_row_stride_bytes = 32U;

struct VulkanMavgGpuCullingDispatchClusterRow {
    std::uint32_t index_count_per_instance{0};
    std::uint32_t instance_count{0};
    std::uint32_t start_index_location{0};
    std::int32_t base_vertex_location{0};
    std::uint32_t start_instance_location{0};
    std::uint32_t visible{0};
    std::uint32_t padding0{0};
    std::uint32_t padding1{0};
};

struct VulkanMavgGpuCullingDispatchDesc {
    std::span<const std::uint32_t> compute_shader_spirv;
    std::string_view compute_shader_entry_point{"main"};
    std::span<const VulkanMavgGpuCullingDispatchClusterRow> cluster_rows;
    std::uint32_t max_command_count{0};
    std::uint32_t record_stride_bytes{20};
};

struct VulkanMavgGpuCullingDispatchResult {
    bool succeeded{false};
    bool executed_gpu_culling{false};
    std::uint32_t visible_cluster_count{0};
    std::uint32_t culled_cluster_count{0};
    std::uint32_t count_buffer_value{0};
    std::uint64_t argument_buffer_size_bytes{0};
    std::uint32_t compute_dispatches{0};
    std::uint32_t resource_barriers_recorded{0};
    std::uint32_t failure_stage{0};
    std::vector<std::uint8_t> argument_readback_bytes;
    std::vector<std::uint8_t> count_readback_bytes;
};

[[nodiscard]] VulkanMavgGpuCullingDispatchResult
dispatch_mavg_gpu_culling_indirect(const VulkanMavgGpuCullingDispatchDesc& desc) noexcept;

} // namespace mirakana::rhi::vulkan
