// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/mavg_gpu_visible_cluster_packets.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgGpuVisibilityBufferLayoutDiagnosticCode : std::uint8_t {
    missing_visible_cluster_packet_plan,
    invalid_visible_cluster_packet_plan,
    source_packet_count_mismatch,
    duplicate_packet_index,
    non_dense_packet_index,
    meshlet_not_ready,
    invalid_slot_stride,
    invalid_slot_alignment,
    max_slot_count_exceeded,
    slot_byte_range_overflow,
    layout_byte_size_overflow,
};

enum class MavgGpuVisibilityBufferSyncProducer : std::uint8_t {
    visibility_buffer_write,
};

enum class MavgGpuVisibilityBufferSyncConsumer : std::uint8_t {
    visibility_resolve_read,
};

struct MavgGpuVisibilityBufferLayoutDiagnostic {
    MavgGpuVisibilityBufferLayoutDiagnosticCode code{
        MavgGpuVisibilityBufferLayoutDiagnosticCode::missing_visible_cluster_packet_plan};
    AssetId graph_asset;
    std::uint32_t packet_index{0};
    std::string message;
};

struct MavgGpuVisibilityBufferLayoutDesc {
    static constexpr std::uint32_t minimum_slot_record_stride_bytes{64};
    static constexpr std::uint32_t default_slot_record_stride_bytes{64};
    static constexpr std::uint32_t default_slot_record_alignment_bytes{16};

    const MavgGpuVisibleClusterPacketPlan* packet_plan{nullptr};
    std::uint32_t max_slot_count{0};
    std::uint32_t slot_record_stride_bytes{default_slot_record_stride_bytes};
    std::uint32_t slot_record_alignment_bytes{default_slot_record_alignment_bytes};
    bool require_meshlet_ready_slots{false};
    bool emit_sync_intent_rows{true};
};

struct MavgGpuVisibilityBufferSlotRow {
    AssetId graph_asset;
    std::uint32_t slot_index{0};
    std::uint32_t packet_index{0};
    std::uint32_t indirect_command_index{0};
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t lod_level{0};
    std::uint32_t material_partition{0};
    bool fallback_substitution{false};
    bool meshlet_ready{false};
    std::uint64_t payload_page_byte_offset{0};
    std::uint64_t payload_page_byte_size{0};
    std::uint64_t slot_byte_offset{0};
    std::uint32_t slot_byte_size{0};
    std::uint64_t cluster_key{0};
};

struct MavgGpuVisibilityBufferByteRangeRow {
    std::uint32_t slot_index{0};
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
};

struct MavgGpuVisibilityBufferLayoutRow {
    std::uint32_t layout_version{1};
    std::uint32_t slot_record_stride_bytes{MavgGpuVisibilityBufferLayoutDesc::default_slot_record_stride_bytes};
    std::uint32_t slot_record_alignment_bytes{MavgGpuVisibilityBufferLayoutDesc::default_slot_record_alignment_bytes};
    std::uint32_t slot_index_field_offset_bytes{0};
    std::uint32_t packet_index_field_offset_bytes{4};
    std::uint32_t cluster_key_field_offset_bytes{8};
    std::uint32_t payload_page_byte_offset_field_offset_bytes{16};
    std::uint32_t payload_page_byte_size_field_offset_bytes{24};
    std::uint32_t flags_field_offset_bytes{32};
    std::uint32_t record_size_bytes{MavgGpuVisibilityBufferLayoutDesc::default_slot_record_stride_bytes};
};

struct MavgGpuVisibilityBufferSyncIntentRow {
    MavgGpuVisibilityBufferSyncProducer producer{MavgGpuVisibilityBufferSyncProducer::visibility_buffer_write};
    MavgGpuVisibilityBufferSyncConsumer consumer{MavgGpuVisibilityBufferSyncConsumer::visibility_resolve_read};
    bool backend_neutral{true};
    bool requires_write_completion_before_read{true};
};

struct MavgGpuVisibilityBufferLayoutPlan {
    std::vector<MavgGpuVisibilityBufferSlotRow> slots;
    std::vector<MavgGpuVisibilityBufferByteRangeRow> byte_ranges;
    std::vector<MavgGpuVisibilityBufferLayoutRow> layout_rows;
    std::vector<MavgGpuVisibilityBufferSyncIntentRow> sync_intents;
    std::vector<MavgGpuVisibilityBufferLayoutDiagnostic> diagnostics;
    std::uint32_t source_packet_count{0};
    std::uint32_t slot_count{0};
    std::uint32_t byte_range_count{0};
    std::uint32_t layout_row_count{0};
    std::uint32_t sync_intent_row_count{0};
    std::uint32_t meshlet_ready_slot_count{0};
    std::uint32_t fallback_slot_count{0};
    std::uint32_t slot_record_stride_bytes{0};
    std::uint32_t slot_record_alignment_bytes{0};
    std::uint64_t slot_buffer_size_bytes{0};
    std::uint64_t payload_byte_span_offset{0};
    std::uint64_t payload_byte_span_size{0};
    bool allocated_rhi_resources{false};
    bool wrote_gpu_visibility_buffer{false};
    bool submitted_gpu_work{false};
    bool executed_gpu_traversal{false};
    bool executed_mesh_shader{false};
    bool executed_indirect_draw{false};
    bool used_gpu_decompression{false};
    bool touched_native_handles{false};
    bool claimed_vulkan_parity{false};
    bool claimed_metal_parity{false};
    bool claimed_nanite_compatibility{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MavgGpuVisibilityBufferLayoutPlan
plan_mavg_gpu_visibility_buffer_layout(const MavgGpuVisibilityBufferLayoutDesc& desc);
[[nodiscard]] bool
has_mavg_gpu_visibility_buffer_layout_diagnostic(const MavgGpuVisibilityBufferLayoutPlan& plan,
                                                 MavgGpuVisibilityBufferLayoutDiagnosticCode code) noexcept;

} // namespace mirakana
