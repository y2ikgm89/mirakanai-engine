// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"
#include "mirakana/renderer/mavg_gpu_cluster_format.hpp"
#include "mirakana/renderer/mavg_gpu_culling.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgGpuVisibleClusterPacketDiagnosticCode : std::uint8_t {
    missing_cluster_format_plan,
    invalid_cluster_format_plan,
    missing_culling_plan,
    invalid_culling_plan,
    duplicate_cluster_format_row,
    missing_cluster_format_row,
    stale_visible_command,
    visible_command_count_mismatch,
    meshlet_not_ready,
    max_packet_count_exceeded,
    payload_byte_span_overflow,
};

struct MavgGpuVisibleClusterPacketDiagnostic {
    MavgGpuVisibleClusterPacketDiagnosticCode code{
        MavgGpuVisibleClusterPacketDiagnosticCode::missing_cluster_format_plan};
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::string message;
};

struct MavgGpuVisibleClusterPacketDesc {
    const MavgGpuClusterFormatPlan* cluster_format_plan{nullptr};
    const MavgGpuCullingIndirectPlan* culling_plan{nullptr};
    std::uint32_t max_packet_count{0};
    bool require_meshlet_ready_packets{false};
};

struct MavgGpuVisibleClusterPacketRow {
    AssetId graph_asset;
    std::uint32_t packet_index{0};
    std::uint32_t indirect_command_index{0};
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t local_cluster_index{0};
    std::uint32_t lod_level{0};
    std::uint32_t material_partition{0};
    std::uint32_t parent_cluster_index{0};
    bool has_parent{false};
    std::uint32_t resident_fallback_cluster_index{0};
    bool fallback_substitution{false};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
    std::uint32_t triangle_count{0};
    std::uint32_t vertex_count{0};
    std::uint64_t payload_page_byte_offset{0};
    std::uint64_t payload_page_byte_size{0};
    Vec3 bounds_center;
    float bounds_radius{0.0F};
    bool meshlet_ready{false};
    std::uint32_t index_count_per_instance{0};
    std::uint32_t instance_count{0};
    std::uint32_t start_index_location{0};
    std::int32_t base_vertex_location{0};
    std::uint32_t start_instance_location{0};
};

struct MavgGpuVisibleClusterPacketPlan {
    std::vector<MavgGpuVisibleClusterPacketRow> packets;
    std::vector<MavgGpuVisibleClusterPacketDiagnostic> diagnostics;
    std::uint32_t source_cluster_row_count{0};
    std::uint32_t source_visible_command_count{0};
    std::uint32_t packet_count{0};
    std::uint32_t meshlet_ready_packet_count{0};
    std::uint32_t fallback_packet_count{0};
    std::uint64_t payload_byte_span_offset{0};
    std::uint64_t payload_byte_span_size{0};
    bool allocated_rhi_resources{false};
    bool submitted_gpu_work{false};
    bool executed_gpu_traversal{false};
    bool wrote_gpu_visibility_buffer{false};
    bool executed_mesh_shader{false};
    bool executed_indirect_draw{false};
    bool used_gpu_decompression{false};
    bool touched_native_handles{false};
    bool claimed_nanite_compatibility{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MavgGpuVisibleClusterPacketPlan
plan_mavg_gpu_visible_cluster_packets(const MavgGpuVisibleClusterPacketDesc& desc);
[[nodiscard]] bool
has_mavg_gpu_visible_cluster_packet_diagnostic(const MavgGpuVisibleClusterPacketPlan& plan,
                                               MavgGpuVisibleClusterPacketDiagnosticCode code) noexcept;

} // namespace mirakana
