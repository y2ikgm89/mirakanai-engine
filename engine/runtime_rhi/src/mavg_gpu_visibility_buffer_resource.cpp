// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_gpu_visibility_buffer_resource.hpp"

#include <exception>
#include <string>
#include <utility>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgGpuVisibilityBufferResourceResult& result,
                    RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode code, AssetId graph_asset,
                    std::string message) {
    result.diagnostics.push_back(RuntimeMavgGpuVisibilityBufferResourceDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .message = std::move(message),
    });
}

[[nodiscard]] AssetId layout_graph_asset(const MavgGpuVisibilityBufferLayoutPlan& layout) noexcept {
    if (!layout.slots.empty()) {
        return layout.slots.front().graph_asset;
    }
    return {};
}

[[nodiscard]] bool layout_shape_valid(const MavgGpuVisibilityBufferLayoutPlan& layout) noexcept {
    return layout.succeeded() && layout.slot_count == layout.slots.size() &&
           layout.byte_range_count == layout.byte_ranges.size() && layout.layout_row_count == layout.layout_rows.size();
}

} // namespace

RuntimeMavgGpuVisibilityBufferResourceResult
create_runtime_mavg_gpu_visibility_buffer_resource(const RuntimeMavgGpuVisibilityBufferResourceDesc& desc) {
    RuntimeMavgGpuVisibilityBufferResourceResult result;
    auto* const device = desc.device;
    if (device != nullptr) {
        result.device_buffers_created_before = device->stats().buffers_created;
        result.device_buffers_created_after = result.device_buffers_created_before;
    }

    if (device == nullptr) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_rhi_device, {},
                       "MAVG GPU visibility-buffer RHI allocation requires an RHI device");
    }

    const auto* const layout = desc.layout_plan;
    if (layout == nullptr) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_layout_plan, {},
                       "MAVG GPU visibility-buffer RHI allocation requires a visibility-buffer layout plan");
        return result;
    }
    if (device == nullptr) {
        return result;
    }

    const auto graph_asset = layout_graph_asset(*layout);
    result.layout_slot_count = layout->slot_count;
    result.layout_byte_range_count = layout->byte_range_count;

    if (!layout_shape_valid(*layout)) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::invalid_layout_plan, graph_asset,
                       "MAVG GPU visibility-buffer RHI allocation requires a successful layout plan with matching row "
                       "counts");
    }

    if (layout->slot_count == 0U) {
        if (desc.allow_zero_slot_buffer) {
            return result;
        }
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::zero_slot_buffer_disallowed,
                       graph_asset, "MAVG GPU visibility-buffer RHI allocation rejected a zero-slot layout");
    } else if (layout->slot_buffer_size_bytes == 0U) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::invalid_slot_buffer_size,
                       graph_asset,
                       "MAVG GPU visibility-buffer RHI allocation requires a positive slot buffer byte size");
    }

    if (desc.max_buffer_size_bytes != 0U && layout->slot_buffer_size_bytes > desc.max_buffer_size_bytes) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::max_buffer_size_exceeded,
                       graph_asset,
                       "MAVG GPU visibility-buffer RHI allocation exceeded the caller-provided max buffer size");
    }

    if (!rhi::has_flag(desc.visibility_buffer_usage, rhi::BufferUsage::storage)) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_storage_usage, graph_asset,
                       "MAVG GPU visibility-buffer RHI allocation requires storage buffer usage");
    }

    if (desc.require_copy_source_for_readback &&
        !rhi::has_flag(desc.visibility_buffer_usage, rhi::BufferUsage::copy_source)) {
        add_diagnostic(result,
                       RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_copy_source_usage_for_readback,
                       graph_asset,
                       "MAVG GPU visibility-buffer RHI allocation requires copy_source usage for later readback proof");
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    const rhi::BufferDesc buffer_desc{
        .size_bytes = layout->slot_buffer_size_bytes,
        .usage = desc.visibility_buffer_usage,
    };

    rhi::BufferHandle visibility_buffer;
    try {
        visibility_buffer = device->create_buffer(buffer_desc);
        result.device_buffers_created_after = device->stats().buffers_created;
    } catch (const std::exception& error) {
        result.device_buffers_created_after = device->stats().buffers_created;
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::rhi_allocation_failed, graph_asset,
                       std::string{"MAVG GPU visibility-buffer RHI allocation failed: "} + error.what());
        return result;
    }

    if (visibility_buffer.value == 0U) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::invalid_allocated_buffer_handle,
                       graph_asset, "MAVG GPU visibility-buffer RHI allocation returned an empty buffer handle");
        return result;
    }

    result.resource_rows.push_back(RuntimeMavgGpuVisibilityBufferResourceRow{
        .graph_asset = graph_asset,
        .visibility_buffer = visibility_buffer,
        .buffer_desc = buffer_desc,
        .slot_count = layout->slot_count,
        .byte_range_count = layout->byte_range_count,
        .slot_buffer_size_bytes = layout->slot_buffer_size_bytes,
        .storage_usage_ready = rhi::has_flag(buffer_desc.usage, rhi::BufferUsage::storage),
        .copy_source_usage_ready = rhi::has_flag(buffer_desc.usage, rhi::BufferUsage::copy_source),
        .ready_for_readback_proof = rhi::has_flag(buffer_desc.usage, rhi::BufferUsage::copy_source),
    });
    result.allocated_buffer_count = 1;
    result.allocated_buffer_size_bytes = buffer_desc.size_bytes;
    result.allocated_rhi_resources = true;
    result.ready_for_readback_proof = result.resource_rows.front().ready_for_readback_proof;
    return result;
}

bool has_runtime_mavg_gpu_visibility_buffer_resource_diagnostic(
    const RuntimeMavgGpuVisibilityBufferResourceResult& result,
    RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode code) noexcept {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace mirakana::runtime_rhi
