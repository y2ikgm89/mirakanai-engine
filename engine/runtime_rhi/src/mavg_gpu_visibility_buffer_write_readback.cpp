// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_gpu_visibility_buffer_write_readback.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <span>
#include <string>
#include <utility>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgGpuVisibilityBufferWriteReadbackResult& result,
                    RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode code, AssetId graph_asset,
                    std::uint32_t slot_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .slot_index = slot_index,
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

[[nodiscard]] bool add_would_overflow(std::uint64_t offset, std::uint64_t size, std::uint64_t limit) noexcept {
    return offset > limit || size > limit - offset;
}

[[nodiscard]] bool field_fits(std::uint32_t field_offset, std::uint32_t field_size,
                              std::uint32_t record_size) noexcept {
    return field_offset <= record_size && field_size <= record_size - field_offset;
}

void write_u32_le(std::vector<std::uint8_t>& bytes, std::uint64_t offset, std::uint32_t value) {
    bytes[static_cast<std::size_t>(offset + 0U)] = static_cast<std::uint8_t>((value >> 0U) & 0xffU);
    bytes[static_cast<std::size_t>(offset + 1U)] = static_cast<std::uint8_t>((value >> 8U) & 0xffU);
    bytes[static_cast<std::size_t>(offset + 2U)] = static_cast<std::uint8_t>((value >> 16U) & 0xffU);
    bytes[static_cast<std::size_t>(offset + 3U)] = static_cast<std::uint8_t>((value >> 24U) & 0xffU);
}

void write_u64_le(std::vector<std::uint8_t>& bytes, std::uint64_t offset, std::uint64_t value) {
    for (std::uint32_t byte_index = 0; byte_index < 8U; ++byte_index) {
        bytes[static_cast<std::size_t>(offset + byte_index)] =
            static_cast<std::uint8_t>((value >> (byte_index * 8U)) & 0xffU);
    }
}

[[nodiscard]] std::uint32_t slot_flags(const MavgGpuVisibilityBufferSlotRow& slot) noexcept {
    std::uint32_t flags = 0;
    if (slot.fallback_substitution) {
        flags |= 1U << 0U;
    }
    if (slot.meshlet_ready) {
        flags |= 1U << 1U;
    }
    return flags;
}

[[nodiscard]] bool validate_layout_row_fields(const MavgGpuVisibilityBufferLayoutRow& layout_row,
                                              std::uint32_t record_size) noexcept {
    return field_fits(layout_row.slot_index_field_offset_bytes, 4U, record_size) &&
           field_fits(layout_row.packet_index_field_offset_bytes, 4U, record_size) &&
           field_fits(layout_row.cluster_key_field_offset_bytes, 8U, record_size) &&
           field_fits(layout_row.payload_page_byte_offset_field_offset_bytes, 8U, record_size) &&
           field_fits(layout_row.payload_page_byte_size_field_offset_bytes, 8U, record_size) &&
           field_fits(layout_row.flags_field_offset_bytes, 4U, record_size);
}

[[nodiscard]] std::vector<std::uint8_t> encode_records(const MavgGpuVisibilityBufferLayoutPlan& layout,
                                                       RuntimeMavgGpuVisibilityBufferWriteReadbackResult& result) {
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(layout.slot_buffer_size_bytes), std::uint8_t{0});
    if (layout.layout_rows.empty()) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_layout_plan,
                       layout_graph_asset(layout), 0, "MAVG visibility-buffer write/readback requires a layout row");
        return {};
    }

    const auto& layout_row = layout.layout_rows.front();
    if (layout_row.record_size_bytes != layout.slot_record_stride_bytes ||
        layout_row.slot_record_stride_bytes != layout.slot_record_stride_bytes) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::slot_record_stride_mismatch,
                       layout_graph_asset(layout), 0,
                       "MAVG visibility-buffer write/readback requires the layout row stride to match the plan");
        return {};
    }
    if (!validate_layout_row_fields(layout_row, layout.slot_record_stride_bytes)) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::field_offset_out_of_range,
                       layout_graph_asset(layout), 0,
                       "MAVG visibility-buffer write/readback layout field offsets exceed the slot record stride");
        return {};
    }

    for (std::size_t index = 0; index < layout.slots.size(); ++index) {
        const auto& slot = layout.slots[index];
        if (index >= layout.byte_ranges.size()) {
            add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_layout_plan,
                           slot.graph_asset, slot.slot_index,
                           "MAVG visibility-buffer write/readback requires a byte range for every slot");
            return {};
        }

        const auto& byte_range = layout.byte_ranges[index];
        if (byte_range.slot_index != slot.slot_index || byte_range.byte_offset != slot.slot_byte_offset ||
            byte_range.byte_size != slot.slot_byte_size) {
            add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_layout_plan,
                           slot.graph_asset, slot.slot_index,
                           "MAVG visibility-buffer write/readback requires slot rows and byte ranges to match");
            return {};
        }
        if (add_would_overflow(byte_range.byte_offset, byte_range.byte_size, layout.slot_buffer_size_bytes)) {
            add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::slot_byte_range_overflow,
                           slot.graph_asset, slot.slot_index,
                           "MAVG visibility-buffer write/readback byte range exceeds the visibility buffer");
            return {};
        }
        if (byte_range.byte_size < layout.slot_record_stride_bytes) {
            add_diagnostic(result,
                           RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::slot_record_stride_mismatch,
                           slot.graph_asset, slot.slot_index,
                           "MAVG visibility-buffer write/readback byte range is smaller than the slot stride");
            return {};
        }

        const auto base = byte_range.byte_offset;
        write_u32_le(bytes, base + layout_row.slot_index_field_offset_bytes, slot.slot_index);
        write_u32_le(bytes, base + layout_row.packet_index_field_offset_bytes, slot.packet_index);
        write_u64_le(bytes, base + layout_row.cluster_key_field_offset_bytes, slot.cluster_key);
        write_u64_le(bytes, base + layout_row.payload_page_byte_offset_field_offset_bytes,
                     slot.payload_page_byte_offset);
        write_u64_le(bytes, base + layout_row.payload_page_byte_size_field_offset_bytes, slot.payload_page_byte_size);
        write_u32_le(bytes, base + layout_row.flags_field_offset_bytes, slot_flags(slot));
    }
    return bytes;
}

[[nodiscard]] bool resource_result_valid(const RuntimeMavgGpuVisibilityBufferResourceResult& resource) noexcept {
    return resource.succeeded() && resource.allocated_rhi_resources && resource.allocated_buffer_count == 1U &&
           resource.resource_rows.size() == 1U;
}

} // namespace

RuntimeMavgGpuVisibilityBufferWriteReadbackResult
execute_runtime_mavg_gpu_visibility_buffer_write_readback(const RuntimeMavgGpuVisibilityBufferWriteReadbackDesc& desc) {
    RuntimeMavgGpuVisibilityBufferWriteReadbackResult result;

    if (desc.device == nullptr) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_rhi_device, {}, 0,
                       "MAVG visibility-buffer write/readback requires an RHI device");
    }

    const auto* const layout = desc.layout_plan;
    if (layout == nullptr) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_layout_plan, {}, 0,
                       "MAVG visibility-buffer write/readback requires a visibility-buffer layout plan");
        return result;
    }

    const auto graph_asset = layout_graph_asset(*layout);
    if (!layout_shape_valid(*layout) || layout->slot_count == 0U || layout->slot_buffer_size_bytes == 0U) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_layout_plan,
                       graph_asset, 0,
                       "MAVG visibility-buffer write/readback requires a successful non-empty layout plan with "
                       "matching row counts");
    }

    const auto* const resource = desc.resource_result;
    if (resource == nullptr) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_resource_result,
                       graph_asset, 0,
                       "MAVG visibility-buffer write/readback requires an allocated visibility-buffer resource result");
        return result;
    }
    if (!resource_result_valid(*resource)) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_resource_result,
                       graph_asset, 0,
                       "MAVG visibility-buffer write/readback requires a successful single-buffer resource result");
    }
    if (resource->owner_device != desc.device) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::resource_device_mismatch,
                       graph_asset, 0,
                       "MAVG visibility-buffer write/readback resource result owner device must match the RHI device");
    }
    if (resource->resource_rows.empty()) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_resource_row,
                       graph_asset, 0, "MAVG visibility-buffer write/readback requires one resource row");
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    const auto& resource_row = resource->resource_rows.front();
    if (resource_row.owner_device != desc.device) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::resource_device_mismatch,
                       graph_asset, 0,
                       "MAVG visibility-buffer write/readback resource row owner device must match the RHI device");
    }
    if (resource_row.visibility_buffer.value == 0U) {
        add_diagnostic(result,
                       RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_visibility_buffer_handle,
                       graph_asset, 0, "MAVG visibility-buffer write/readback requires a valid visibility buffer");
    }
    if (resource_row.buffer_desc.size_bytes < layout->slot_buffer_size_bytes ||
        resource_row.slot_buffer_size_bytes != layout->slot_buffer_size_bytes) {
        add_diagnostic(
            result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::visibility_buffer_size_mismatch,
            graph_asset, 0, "MAVG visibility-buffer write/readback requires the resource row size to cover the layout");
    }
    if (!rhi::has_flag(resource_row.buffer_desc.usage, rhi::BufferUsage::storage)) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_storage_usage,
                       graph_asset, 0, "MAVG visibility-buffer write/readback requires storage usage");
    }
    if (!rhi::has_flag(resource_row.buffer_desc.usage, rhi::BufferUsage::copy_source)) {
        add_diagnostic(
            result,
            RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_copy_source_usage_for_readback_copy,
            graph_asset, 0,
            "MAVG visibility-buffer write/readback requires copy_source usage for visibility-to-readback "
            "copy");
    }
    if (!rhi::has_flag(resource_row.buffer_desc.usage, rhi::BufferUsage::copy_destination)) {
        add_diagnostic(
            result,
            RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_copy_destination_usage_for_staged_write,
            graph_asset, 0,
            "MAVG visibility-buffer write/readback requires copy_destination usage for upload-to-visibility "
            "copy");
    }
    if (!result.diagnostics.empty()) {
        return result;
    }
    if (desc.max_write_size_bytes != 0U && layout->slot_buffer_size_bytes > desc.max_write_size_bytes) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::max_write_size_exceeded,
                       graph_asset, 0,
                       "MAVG visibility-buffer write/readback encoded bytes exceed the caller-provided limit");
        return result;
    }

    auto encoded = encode_records(*layout, result);
    if (!result.diagnostics.empty()) {
        return result;
    }

    const auto encoded_byte_count = static_cast<std::uint64_t>(encoded.size());
    rhi::BufferHandle upload_buffer;
    rhi::BufferHandle readback_buffer;
    try {
        upload_buffer = desc.device->create_buffer(rhi::BufferDesc{
            .size_bytes = encoded_byte_count,
            .usage = rhi::BufferUsage::copy_source,
        });
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_upload_allocation_failed,
                       graph_asset, 0,
                       std::string{"MAVG visibility-buffer write/readback upload allocation failed: "} + error.what());
        return result;
    }
    try {
        readback_buffer = desc.device->create_buffer(rhi::BufferDesc{
            .size_bytes = encoded_byte_count,
            .usage = rhi::BufferUsage::copy_destination,
        });
    } catch (const std::exception& error) {
        add_diagnostic(
            result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_readback_allocation_failed,
            graph_asset, 0,
            std::string{"MAVG visibility-buffer write/readback readback allocation failed: "} + error.what());
        return result;
    }

    try {
        desc.device->write_buffer(upload_buffer, 0, std::span<const std::uint8_t>{encoded.data(), encoded.size()});
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_upload_write_failed,
                       graph_asset, 0,
                       std::string{"MAVG visibility-buffer write/readback upload write failed: "} + error.what());
        return result;
    }

    std::unique_ptr<rhi::IRhiCommandList> commands;
    try {
        commands = desc.device->begin_command_list(desc.copy_queue);
        if (commands == nullptr) {
            add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_copy_recording_failed,
                           graph_asset, 0, "MAVG visibility-buffer write/readback RHI returned an empty command list");
            return result;
        }
        commands->copy_buffer(upload_buffer, resource_row.visibility_buffer,
                              rhi::BufferCopyRegion{
                                  .source_offset = 0,
                                  .destination_offset = 0,
                                  .size_bytes = encoded_byte_count,
                              });
        ++result.copy_command_count;
        commands->copy_buffer(resource_row.visibility_buffer, readback_buffer,
                              rhi::BufferCopyRegion{
                                  .source_offset = 0,
                                  .destination_offset = 0,
                                  .size_bytes = encoded_byte_count,
                              });
        ++result.copy_command_count;
        commands->close();
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_copy_recording_failed,
                       graph_asset, 0,
                       std::string{"MAVG visibility-buffer write/readback copy recording failed: "} + error.what());
        return result;
    }

    try {
        result.submitted_copy_fence = desc.device->submit(*commands);
        result.submitted_copy_work = true;
        desc.device->wait(result.submitted_copy_fence);
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_submit_failed,
                       graph_asset, 0,
                       std::string{"MAVG visibility-buffer write/readback copy submit/wait failed: "} + error.what());
        return result;
    }

    try {
        result.readback_bytes = desc.device->read_buffer(readback_buffer, 0, encoded_byte_count);
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_readback_failed,
                       graph_asset, 0,
                       std::string{"MAVG visibility-buffer write/readback readback failed: "} + error.what());
        return result;
    }

    result.encoded_bytes = std::move(encoded);
    result.encoded_record_count = layout->slot_count;
    result.encoded_record_stride_bytes = layout->slot_record_stride_bytes;
    result.encoded_byte_count = encoded_byte_count;
    result.uploaded_byte_count = encoded_byte_count;
    result.readback_byte_count = static_cast<std::uint64_t>(result.readback_bytes.size());
    result.readback_bytes_match_encoded_records = result.encoded_bytes == result.readback_bytes;
    if (!result.readback_bytes_match_encoded_records) {
        add_diagnostic(result, RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::readback_mismatch,
                       graph_asset, 0,
                       "MAVG visibility-buffer write/readback readback bytes did not match encoded records");
        return result;
    }

    result.wrote_gpu_visibility_buffer = true;
    result.rows.push_back(RuntimeMavgGpuVisibilityBufferWriteReadbackRow{
        .graph_asset = graph_asset,
        .upload_buffer = upload_buffer,
        .visibility_buffer = resource_row.visibility_buffer,
        .readback_buffer = readback_buffer,
        .submitted_copy_fence = result.submitted_copy_fence,
        .encoded_record_count = result.encoded_record_count,
        .encoded_record_stride_bytes = result.encoded_record_stride_bytes,
        .encoded_byte_count = result.encoded_byte_count,
        .uploaded_byte_count = result.uploaded_byte_count,
        .readback_byte_count = result.readback_byte_count,
        .copy_command_count = result.copy_command_count,
        .wrote_gpu_visibility_buffer = result.wrote_gpu_visibility_buffer,
        .submitted_copy_work = result.submitted_copy_work,
        .readback_bytes_match_encoded_records = result.readback_bytes_match_encoded_records,
    });

    return result;
}

bool has_runtime_mavg_gpu_visibility_buffer_write_readback_diagnostic(
    const RuntimeMavgGpuVisibilityBufferWriteReadbackResult& result,
    RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode code) noexcept {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace mirakana::runtime_rhi
