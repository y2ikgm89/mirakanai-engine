// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/mavg_gpu_visibility_buffer_layout.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(MavgGpuVisibilityBufferLayoutPlan& plan, MavgGpuVisibilityBufferLayoutDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t packet_index, std::string message) {
    plan.diagnostics.push_back(MavgGpuVisibilityBufferLayoutDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .packet_index = packet_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool is_power_of_two(std::uint32_t value) noexcept {
    return value != 0U && (value & (value - 1U)) == 0U;
}

[[nodiscard]] bool align_up(std::uint64_t value, std::uint32_t alignment, std::uint64_t& aligned) noexcept {
    const auto mask = static_cast<std::uint64_t>(alignment - 1U);
    if (value > (std::numeric_limits<std::uint64_t>::max() - mask)) {
        return false;
    }
    aligned = (value + mask) & ~mask;
    return true;
}

void fail_closed(MavgGpuVisibilityBufferLayoutPlan& plan) {
    plan.slots.clear();
    plan.byte_ranges.clear();
    plan.layout_rows.clear();
    plan.sync_intents.clear();
    plan.source_packet_count = 0;
    plan.slot_count = 0;
    plan.byte_range_count = 0;
    plan.layout_row_count = 0;
    plan.sync_intent_row_count = 0;
    plan.meshlet_ready_slot_count = 0;
    plan.fallback_slot_count = 0;
    plan.slot_record_stride_bytes = 0;
    plan.slot_record_alignment_bytes = 0;
    plan.slot_buffer_size_bytes = 0;
    plan.payload_byte_span_offset = 0;
    plan.payload_byte_span_size = 0;
}

[[nodiscard]] bool packet_index_is_duplicate(const MavgGpuVisibleClusterPacketPlan& packet_plan,
                                             std::uint32_t packet_index) noexcept {
    std::uint32_t count = 0;
    for (const auto& packet : packet_plan.packets) {
        if (packet.packet_index == packet_index) {
            ++count;
        }
    }
    return count > 1U;
}

[[nodiscard]] std::uint64_t mix_cluster_key(std::uint64_t seed, std::uint64_t value) noexcept {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
    return seed;
}

[[nodiscard]] std::uint64_t cluster_key_of(const MavgGpuVisibleClusterPacketRow& packet) noexcept {
    std::uint64_t key = 0xcbf29ce484222325ULL;
    key = mix_cluster_key(key, packet.graph_asset.value);
    key = mix_cluster_key(key, packet.cluster_index);
    key = mix_cluster_key(key, packet.page_index);
    key = mix_cluster_key(key, packet.lod_level);
    key = mix_cluster_key(key, packet.material_partition);
    key = mix_cluster_key(key, packet.packet_index);
    return key;
}

[[nodiscard]] bool next_slot_range(std::uint64_t cursor, std::uint32_t stride, std::uint32_t alignment,
                                   std::uint64_t& byte_offset, std::uint64_t& next_cursor) noexcept {
    if (!align_up(cursor, alignment, byte_offset)) {
        return false;
    }
    if (byte_offset > (std::numeric_limits<std::uint64_t>::max() - stride)) {
        return false;
    }
    next_cursor = byte_offset + stride;
    return true;
}

} // namespace

bool MavgGpuVisibilityBufferLayoutPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

MavgGpuVisibilityBufferLayoutPlan
plan_mavg_gpu_visibility_buffer_layout(const MavgGpuVisibilityBufferLayoutDesc& desc) {
    MavgGpuVisibilityBufferLayoutPlan plan;

    if (desc.packet_plan == nullptr) {
        add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::missing_visible_cluster_packet_plan,
                       AssetId{}, 0, "MAVG visibility buffer layout planning requires a visible cluster packet plan");
        fail_closed(plan);
        return plan;
    }

    const auto& packet_plan = *desc.packet_plan;
    const auto source_packet_count = static_cast<std::uint64_t>(packet_plan.packets.size());
    plan.source_packet_count = static_cast<std::uint32_t>(packet_plan.packets.size());
    plan.slot_record_stride_bytes = desc.slot_record_stride_bytes;
    plan.slot_record_alignment_bytes = desc.slot_record_alignment_bytes;
    plan.payload_byte_span_offset = packet_plan.payload_byte_span_offset;
    plan.payload_byte_span_size = packet_plan.payload_byte_span_size;

    if (!packet_plan.succeeded()) {
        add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::invalid_visible_cluster_packet_plan,
                       AssetId{}, 0,
                       "MAVG visibility buffer layout planning requires a successful visible cluster packet plan");
    }
    if (static_cast<std::uint64_t>(packet_plan.packet_count) != source_packet_count) {
        add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::source_packet_count_mismatch, AssetId{},
                       packet_plan.packet_count, "MAVG visibility buffer layout packet_count must match packet rows");
    }
    if (desc.slot_record_stride_bytes < MavgGpuVisibilityBufferLayoutDesc::minimum_slot_record_stride_bytes) {
        add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::invalid_slot_stride, AssetId{},
                       desc.slot_record_stride_bytes,
                       "MAVG visibility buffer layout slot stride is smaller than the minimum record size");
    }
    if (!is_power_of_two(desc.slot_record_alignment_bytes)) {
        add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::invalid_slot_alignment, AssetId{},
                       desc.slot_record_alignment_bytes,
                       "MAVG visibility buffer layout slot alignment must be a non-zero power of two");
    }
    if (desc.max_slot_count != 0U && source_packet_count > desc.max_slot_count) {
        add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::max_slot_count_exceeded, AssetId{},
                       static_cast<std::uint32_t>(source_packet_count),
                       "MAVG visibility buffer layout slot count exceeds max_slot_count");
    }

    std::uint64_t cursor = 0;
    for (std::size_t slot_index = 0; slot_index < packet_plan.packets.size(); ++slot_index) {
        const auto& packet = packet_plan.packets[slot_index];
        const auto slot_index_u32 = static_cast<std::uint32_t>(slot_index);
        if (packet_index_is_duplicate(packet_plan, packet.packet_index)) {
            add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::duplicate_packet_index,
                           packet.graph_asset, packet.packet_index,
                           "MAVG visibility buffer layout requires unique packet indexes");
        }
        if (packet.packet_index != slot_index_u32) {
            add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::non_dense_packet_index,
                           packet.graph_asset, packet.packet_index,
                           "MAVG visibility buffer layout requires packet indexes to match packet order");
        }
        if (desc.require_meshlet_ready_slots && !packet.meshlet_ready) {
            add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::meshlet_not_ready, packet.graph_asset,
                           packet.packet_index, "MAVG visibility buffer layout requires meshlet-ready packets");
        }

        std::uint64_t byte_offset = 0;
        std::uint64_t next_cursor = 0;
        if (is_power_of_two(desc.slot_record_alignment_bytes) &&
            !next_slot_range(cursor, desc.slot_record_stride_bytes, desc.slot_record_alignment_bytes, byte_offset,
                             next_cursor)) {
            add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::slot_byte_range_overflow,
                           packet.graph_asset, packet.packet_index,
                           "MAVG visibility buffer layout slot byte range overflows");
        }
        cursor = next_cursor;
    }

    if (!plan.diagnostics.empty()) {
        fail_closed(plan);
        return plan;
    }

    plan.slots.reserve(packet_plan.packets.size());
    plan.byte_ranges.reserve(packet_plan.packets.size());
    plan.layout_rows.push_back(MavgGpuVisibilityBufferLayoutRow{
        .slot_record_stride_bytes = desc.slot_record_stride_bytes,
        .slot_record_alignment_bytes = desc.slot_record_alignment_bytes,
        .record_size_bytes = desc.slot_record_stride_bytes,
    });
    if (desc.emit_sync_intent_rows) {
        plan.sync_intents.push_back(MavgGpuVisibilityBufferSyncIntentRow{});
    }

    cursor = 0;
    for (std::size_t slot_index = 0; slot_index < packet_plan.packets.size(); ++slot_index) {
        const auto& packet = packet_plan.packets[slot_index];
        const auto slot_index_u32 = static_cast<std::uint32_t>(slot_index);
        std::uint64_t byte_offset = 0;
        std::uint64_t next_cursor = 0;
        if (!next_slot_range(cursor, desc.slot_record_stride_bytes, desc.slot_record_alignment_bytes, byte_offset,
                             next_cursor)) {
            add_diagnostic(plan, MavgGpuVisibilityBufferLayoutDiagnosticCode::layout_byte_size_overflow,
                           packet.graph_asset, packet.packet_index,
                           "MAVG visibility buffer layout byte size overflows");
            fail_closed(plan);
            return plan;
        }

        if (packet.meshlet_ready) {
            ++plan.meshlet_ready_slot_count;
        }
        if (packet.fallback_substitution) {
            ++plan.fallback_slot_count;
        }

        plan.byte_ranges.push_back(MavgGpuVisibilityBufferByteRangeRow{
            .slot_index = slot_index_u32,
            .byte_offset = byte_offset,
            .byte_size = desc.slot_record_stride_bytes,
        });
        plan.slots.push_back(MavgGpuVisibilityBufferSlotRow{
            .graph_asset = packet.graph_asset,
            .slot_index = slot_index_u32,
            .packet_index = packet.packet_index,
            .indirect_command_index = packet.indirect_command_index,
            .cluster_index = packet.cluster_index,
            .page_index = packet.page_index,
            .lod_level = packet.lod_level,
            .material_partition = packet.material_partition,
            .fallback_substitution = packet.fallback_substitution,
            .meshlet_ready = packet.meshlet_ready,
            .payload_page_byte_offset = packet.payload_page_byte_offset,
            .payload_page_byte_size = packet.payload_page_byte_size,
            .slot_byte_offset = byte_offset,
            .slot_byte_size = desc.slot_record_stride_bytes,
            .cluster_key = cluster_key_of(packet),
        });
        cursor = next_cursor;
    }

    plan.slot_count = static_cast<std::uint32_t>(plan.slots.size());
    plan.byte_range_count = static_cast<std::uint32_t>(plan.byte_ranges.size());
    plan.layout_row_count = static_cast<std::uint32_t>(plan.layout_rows.size());
    plan.sync_intent_row_count = static_cast<std::uint32_t>(plan.sync_intents.size());
    plan.slot_buffer_size_bytes = cursor;

    return plan;
}

bool has_mavg_gpu_visibility_buffer_layout_diagnostic(const MavgGpuVisibilityBufferLayoutPlan& plan,
                                                      MavgGpuVisibilityBufferLayoutDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const MavgGpuVisibilityBufferLayoutDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

} // namespace mirakana
