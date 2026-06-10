// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/d3d12/d3d12_backend.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace mirakana::rhi::d3d12 {

inline constexpr std::uint32_t d3d12_mavg_gpu_culling_dispatch_cluster_row_stride_bytes = 32U;

struct D3d12MavgGpuCullingDispatchClusterRow {
    std::uint32_t index_count_per_instance{0};
    std::uint32_t instance_count{0};
    std::uint32_t start_index_location{0};
    std::int32_t base_vertex_location{0};
    std::uint32_t start_instance_location{0};
    std::uint32_t visible{0};
    std::uint32_t padding0{0};
    std::uint32_t padding1{0};
};

struct D3d12MavgGpuCullingDispatchDesc {
    DeviceBootstrapDesc device{};
    std::span<const D3d12MavgGpuCullingDispatchClusterRow> cluster_rows;
    std::uint32_t max_command_count{0};
    std::uint32_t record_stride_bytes{20};
    /// When non-zero, writes into an existing RHI-owned argument buffer instead of allocating internally.
    NativeResourceHandle external_argument_buffer{};
    /// When non-zero, writes into an existing RHI-owned count buffer instead of allocating internally.
    NativeResourceHandle external_count_buffer{};
    /// Skip COPY_SOURCE readback transitions so `draw_indexed_indirect` can consume GPU-written buffers.
    bool leave_indirect_argument_state_for_consumption{false};
};

struct D3d12MavgGpuCullingDispatchResult {
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

[[nodiscard]] D3d12MavgGpuCullingDispatchResult
dispatch_mavg_gpu_culling_indirect(const D3d12MavgGpuCullingDispatchDesc& desc) noexcept;

/// Writes MAVG packed indirect buffers on an existing D3D12 RHI device. Requires `external_argument_buffer` and
/// `external_count_buffer` populated through `native_buffer_resource`.
[[nodiscard]] D3d12MavgGpuCullingDispatchResult
dispatch_mavg_gpu_culling_indirect(IRhiDevice& device, const D3d12MavgGpuCullingDispatchDesc& desc) noexcept;

} // namespace mirakana::rhi::d3d12
