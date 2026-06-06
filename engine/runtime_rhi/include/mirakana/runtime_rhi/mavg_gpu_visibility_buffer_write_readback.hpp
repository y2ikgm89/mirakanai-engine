// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime_rhi/mavg_gpu_visibility_buffer_resource.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode : std::uint8_t {
    missing_rhi_device = 0,
    missing_layout_plan,
    invalid_layout_plan,
    missing_resource_result,
    invalid_resource_result,
    resource_device_mismatch,
    missing_resource_row,
    invalid_visibility_buffer_handle,
    visibility_buffer_size_mismatch,
    missing_storage_usage,
    missing_copy_source_usage_for_readback_copy,
    missing_copy_destination_usage_for_staged_write,
    slot_record_stride_mismatch,
    slot_byte_range_overflow,
    field_offset_out_of_range,
    max_write_size_exceeded,
    rhi_upload_allocation_failed,
    rhi_readback_allocation_failed,
    rhi_upload_write_failed,
    rhi_copy_recording_failed,
    rhi_submit_failed,
    rhi_readback_failed,
    readback_mismatch,
};

struct RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnostic {
    RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode code{
        RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_layout_plan};
    AssetId graph_asset;
    std::uint32_t slot_index{0};
    std::string message;
};

struct RuntimeMavgGpuVisibilityBufferWriteReadbackDesc {
    rhi::IRhiDevice* device{nullptr};
    const MavgGpuVisibilityBufferLayoutPlan* layout_plan{nullptr};
    const RuntimeMavgGpuVisibilityBufferResourceResult* resource_result{nullptr};
    rhi::QueueKind copy_queue{rhi::QueueKind::graphics};
    std::uint64_t max_write_size_bytes{0};
};

struct RuntimeMavgGpuVisibilityBufferWriteReadbackRow {
    AssetId graph_asset;
    rhi::BufferHandle upload_buffer;
    rhi::BufferHandle visibility_buffer;
    rhi::BufferHandle readback_buffer;
    rhi::FenceValue submitted_copy_fence;
    std::uint32_t encoded_record_count{0};
    std::uint32_t encoded_record_stride_bytes{0};
    std::uint64_t encoded_byte_count{0};
    std::uint64_t uploaded_byte_count{0};
    std::uint64_t readback_byte_count{0};
    std::uint32_t copy_command_count{0};
    bool wrote_gpu_visibility_buffer{false};
    bool submitted_copy_work{false};
    bool readback_bytes_match_encoded_records{false};
};

struct RuntimeMavgGpuVisibilityBufferWriteReadbackResult {
    std::vector<RuntimeMavgGpuVisibilityBufferWriteReadbackRow> rows;
    std::vector<RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnostic> diagnostics;
    std::vector<std::uint8_t> encoded_bytes;
    std::vector<std::uint8_t> readback_bytes;
    rhi::FenceValue submitted_copy_fence;
    std::uint32_t encoded_record_count{0};
    std::uint32_t encoded_record_stride_bytes{0};
    std::uint64_t encoded_byte_count{0};
    std::uint64_t uploaded_byte_count{0};
    std::uint64_t readback_byte_count{0};
    std::uint32_t copy_command_count{0};
    bool wrote_gpu_visibility_buffer{false};
    bool submitted_copy_work{false};
    bool readback_bytes_match_encoded_records{false};
    bool created_descriptor_bindings{false};
    bool created_compute_pipeline{false};
    bool dispatched_compute_shader{false};
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
    bool claimed_async_overlap_or_performance{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimeMavgGpuVisibilityBufferWriteReadbackResult
execute_runtime_mavg_gpu_visibility_buffer_write_readback(const RuntimeMavgGpuVisibilityBufferWriteReadbackDesc& desc);

[[nodiscard]] bool has_runtime_mavg_gpu_visibility_buffer_write_readback_diagnostic(
    const RuntimeMavgGpuVisibilityBufferWriteReadbackResult& result,
    RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
