// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/mavg_gpu_visible_cluster_packets.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(MavgGpuVisibleClusterPacketPlan& plan, MavgGpuVisibleClusterPacketDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t cluster_index, std::string message) {
    plan.diagnostics.push_back(MavgGpuVisibleClusterPacketDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .cluster_index = cluster_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool same_cluster(const MavgGpuClusterFormatRow& row,
                                const MavgGpuCullingIndirectCommand& command) noexcept {
    return row.graph_asset.value == command.graph_asset.value && row.cluster_index == command.cluster_index;
}

[[nodiscard]] std::uint32_t matching_format_row_count(const MavgGpuClusterFormatPlan& format,
                                                      const MavgGpuCullingIndirectCommand& command) noexcept {
    std::uint32_t count = 0;
    for (const auto& row : format.rows) {
        if (same_cluster(row, command)) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] const MavgGpuClusterFormatRow* find_format_row(const MavgGpuClusterFormatPlan& format,
                                                             const MavgGpuCullingIndirectCommand& command) noexcept {
    for (const auto& row : format.rows) {
        if (same_cluster(row, command)) {
            return &row;
        }
    }
    return nullptr;
}

[[nodiscard]] bool duplicate_format_row_exists(const MavgGpuClusterFormatPlan& format) noexcept {
    for (std::size_t outer = 0; outer < format.rows.size(); ++outer) {
        for (std::size_t inner = outer + 1U; inner < format.rows.size(); ++inner) {
            if (format.rows[outer].graph_asset.value == format.rows[inner].graph_asset.value &&
                format.rows[outer].cluster_index == format.rows[inner].cluster_index) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] bool stale_visible_command(const MavgGpuClusterFormatRow& row,
                                         const MavgGpuCullingIndirectCommand& command) noexcept {
    return row.page_index != command.page_index || row.lod_level != command.lod_level ||
           row.material_partition != command.material_partition || row.first_index != command.start_index_location ||
           row.index_count != command.index_count_per_instance || row.vertex_base != command.base_vertex_location;
}

void fail_closed(MavgGpuVisibleClusterPacketPlan& plan) {
    plan.packets.clear();
    plan.source_cluster_row_count = 0;
    plan.source_visible_command_count = 0;
    plan.packet_count = 0;
    plan.meshlet_ready_packet_count = 0;
    plan.fallback_packet_count = 0;
    plan.payload_byte_span_offset = 0;
    plan.payload_byte_span_size = 0;
}

} // namespace

bool MavgGpuVisibleClusterPacketPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

MavgGpuVisibleClusterPacketPlan plan_mavg_gpu_visible_cluster_packets(const MavgGpuVisibleClusterPacketDesc& desc) {
    MavgGpuVisibleClusterPacketPlan plan;

    const auto* const cluster_format_plan = desc.cluster_format_plan;
    const auto* const culling_plan = desc.culling_plan;
    if (cluster_format_plan == nullptr) {
        add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::missing_cluster_format_plan, AssetId{}, 0,
                       "MAVG visible cluster packet planning requires a cluster format plan");
    }
    if (culling_plan == nullptr) {
        add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::missing_culling_plan, AssetId{}, 0,
                       "MAVG visible cluster packet planning requires a culling plan");
    }
    if (cluster_format_plan == nullptr || culling_plan == nullptr) {
        fail_closed(plan);
        return plan;
    }

    const auto& format = *cluster_format_plan;
    const auto& culling = *culling_plan;
    const auto visible_command_count = static_cast<std::uint64_t>(culling.commands.size());
    plan.source_cluster_row_count = static_cast<std::uint32_t>(format.rows.size());
    plan.source_visible_command_count = static_cast<std::uint32_t>(culling.commands.size());

    if (!format.succeeded()) {
        add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::invalid_cluster_format_plan, AssetId{}, 0,
                       "MAVG visible cluster packet planning requires a successful cluster format plan");
    }
    if (!culling.succeeded()) {
        add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::invalid_culling_plan, AssetId{}, 0,
                       "MAVG visible cluster packet planning requires a successful culling plan");
    }
    if (duplicate_format_row_exists(format)) {
        add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::duplicate_cluster_format_row, AssetId{}, 0,
                       "MAVG visible cluster packet planning requires unique cluster format rows");
    }
    if (static_cast<std::uint64_t>(culling.count_buffer_value) != visible_command_count) {
        add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::visible_command_count_mismatch, AssetId{},
                       culling.count_buffer_value,
                       "MAVG visible cluster packet planning requires count_buffer_value to match visible commands");
    }
    if (desc.max_packet_count != 0U && visible_command_count > desc.max_packet_count) {
        add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::max_packet_count_exceeded, AssetId{},
                       static_cast<std::uint32_t>(culling.commands.size()),
                       "MAVG visible cluster packet count exceeds max_packet_count");
    }

    for (const auto& command : culling.commands) {
        const auto match_count = matching_format_row_count(format, command);
        if (match_count == 0U) {
            add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::missing_cluster_format_row,
                           command.graph_asset, command.cluster_index,
                           "visible MAVG command is missing a cluster format row");
            continue;
        }
        if (match_count > 1U) {
            add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::duplicate_cluster_format_row,
                           command.graph_asset, command.cluster_index,
                           "visible MAVG command has duplicate cluster format rows");
            continue;
        }

        const auto* row = find_format_row(format, command);
        if (row == nullptr) {
            continue;
        }
        if (stale_visible_command(*row, command)) {
            add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::stale_visible_command, command.graph_asset,
                           command.cluster_index, "visible MAVG command no longer matches the cluster format row");
        }
        if (desc.require_meshlet_ready_packets && !row->meshlet_ready) {
            add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::meshlet_not_ready, command.graph_asset,
                           command.cluster_index, "visible MAVG command maps to a cluster row outside meshlet limits");
        }
        if (row->payload_page_byte_offset > (std::numeric_limits<std::uint64_t>::max() - row->payload_page_byte_size)) {
            add_diagnostic(plan, MavgGpuVisibleClusterPacketDiagnosticCode::payload_byte_span_overflow,
                           command.graph_asset, command.cluster_index,
                           "visible MAVG packet payload page byte range overflows");
        }
    }

    if (!plan.diagnostics.empty()) {
        fail_closed(plan);
        return plan;
    }

    plan.packets.reserve(culling.commands.size());
    bool has_payload_span = false;
    std::uint64_t payload_span_begin = 0;
    std::uint64_t payload_span_end = 0;
    for (std::uint32_t command_index = 0; command_index < culling.commands.size(); ++command_index) {
        const auto& command = culling.commands[command_index];
        const auto* row = find_format_row(format, command);
        if (row == nullptr) {
            continue;
        }

        if (row->meshlet_ready) {
            ++plan.meshlet_ready_packet_count;
        }
        if (command.fallback_substitution) {
            ++plan.fallback_packet_count;
        }
        const auto payload_begin = row->payload_page_byte_offset;
        const auto payload_end = row->payload_page_byte_offset + row->payload_page_byte_size;
        if (!has_payload_span) {
            payload_span_begin = payload_begin;
            payload_span_end = payload_end;
            has_payload_span = true;
        } else {
            payload_span_begin = std::min(payload_span_begin, payload_begin);
            payload_span_end = std::max(payload_span_end, payload_end);
        }

        plan.packets.push_back(MavgGpuVisibleClusterPacketRow{
            .graph_asset = row->graph_asset,
            .packet_index = static_cast<std::uint32_t>(plan.packets.size()),
            .indirect_command_index = command_index,
            .cluster_index = row->cluster_index,
            .page_index = row->page_index,
            .local_cluster_index = row->local_cluster_index,
            .lod_level = row->lod_level,
            .material_partition = row->material_partition,
            .parent_cluster_index = row->parent_cluster_index,
            .has_parent = row->has_parent,
            .resident_fallback_cluster_index = row->resident_fallback_cluster_index,
            .fallback_substitution = command.fallback_substitution,
            .first_index = row->first_index,
            .index_count = row->index_count,
            .vertex_base = row->vertex_base,
            .triangle_count = row->triangle_count,
            .vertex_count = row->vertex_count,
            .payload_page_byte_offset = row->payload_page_byte_offset,
            .payload_page_byte_size = row->payload_page_byte_size,
            .bounds_center = row->bounds_center,
            .bounds_radius = row->bounds_radius,
            .meshlet_ready = row->meshlet_ready,
            .index_count_per_instance = command.index_count_per_instance,
            .instance_count = command.instance_count,
            .start_index_location = command.start_index_location,
            .base_vertex_location = command.base_vertex_location,
            .start_instance_location = command.start_instance_location,
        });
    }

    plan.packet_count = static_cast<std::uint32_t>(plan.packets.size());
    if (has_payload_span) {
        plan.payload_byte_span_offset = payload_span_begin;
        plan.payload_byte_span_size = payload_span_end - payload_span_begin;
    }

    return plan;
}

bool has_mavg_gpu_visible_cluster_packet_diagnostic(const MavgGpuVisibleClusterPacketPlan& plan,
                                                    MavgGpuVisibleClusterPacketDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const MavgGpuVisibleClusterPacketDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

} // namespace mirakana
