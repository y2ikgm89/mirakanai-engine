// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/mavg_gpu_visibility_buffer_layout.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode : std::uint8_t {
    missing_rhi_device = 0,
    missing_layout_plan,
    invalid_layout_plan,
    zero_slot_buffer_disallowed,
    invalid_slot_buffer_size,
    max_buffer_size_exceeded,
    missing_storage_usage,
    missing_copy_source_usage_for_readback,
    rhi_allocation_failed,
    invalid_allocated_buffer_handle,
};

struct RuntimeMavgGpuVisibilityBufferResourceDiagnostic {
    RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode code{
        RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_layout_plan};
    AssetId graph_asset;
    std::string message;
};

struct RuntimeMavgGpuVisibilityBufferResourceDesc {
    rhi::IRhiDevice* device{nullptr};
    const MavgGpuVisibilityBufferLayoutPlan* layout_plan{nullptr};
    rhi::BufferUsage visibility_buffer_usage{rhi::BufferUsage::storage | rhi::BufferUsage::copy_source};
    std::uint64_t max_buffer_size_bytes{0};
    bool allow_zero_slot_buffer{false};
    bool require_copy_source_for_readback{true};
};

struct RuntimeMavgGpuVisibilityBufferResourceRow {
    AssetId graph_asset;
    rhi::BufferHandle visibility_buffer;
    rhi::BufferDesc buffer_desc;
    const rhi::IRhiDevice* owner_device{nullptr};
    std::uint32_t slot_count{0};
    std::uint32_t byte_range_count{0};
    std::uint64_t slot_buffer_size_bytes{0};
    bool storage_usage_ready{false};
    bool copy_source_usage_ready{false};
    bool ready_for_readback_proof{false};
};

struct RuntimeMavgGpuVisibilityBufferResourceResult {
    std::vector<RuntimeMavgGpuVisibilityBufferResourceRow> resource_rows;
    std::vector<RuntimeMavgGpuVisibilityBufferResourceDiagnostic> diagnostics;
    std::uint32_t layout_slot_count{0};
    std::uint32_t layout_byte_range_count{0};
    std::uint32_t allocated_buffer_count{0};
    std::uint64_t allocated_buffer_size_bytes{0};
    std::uint64_t device_buffers_created_before{0};
    std::uint64_t device_buffers_created_after{0};
    const rhi::IRhiDevice* owner_device{nullptr};
    bool allocated_rhi_resources{false};
    bool ready_for_readback_proof{false};
    bool wrote_gpu_visibility_buffer{false};
    bool created_descriptor_bindings{false};
    bool recorded_resource_barriers{false};
    bool submitted_gpu_work{false};
    bool executed_gpu_traversal{false};
    bool executed_mesh_shader{false};
    bool executed_indirect_draw{false};
    bool used_gpu_decompression{false};
    bool enforced_allocator_budget{false};
    bool exposed_native_handles{false};
    bool claimed_vulkan_parity{false};
    bool claimed_metal_parity{false};
    bool claimed_nanite_compatibility{false};
    bool claimed_benchmark_superiority{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimeMavgGpuVisibilityBufferResourceResult
create_runtime_mavg_gpu_visibility_buffer_resource(const RuntimeMavgGpuVisibilityBufferResourceDesc& desc);

[[nodiscard]] bool has_runtime_mavg_gpu_visibility_buffer_resource_diagnostic(
    const RuntimeMavgGpuVisibilityBufferResourceResult& result,
    RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
