// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/assets/mavg_cluster_payload.hpp"
#include "mirakana/math/vec.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgGpuClusterFormatDiagnosticCode : std::uint8_t {
    invalid_graph,
    invalid_payload,
    unsupported_vertex_stride,
    unsupported_index_format,
    cluster_draw_range_out_of_payload,
};

struct MavgGpuClusterFormatDiagnostic {
    MavgGpuClusterFormatDiagnosticCode code{MavgGpuClusterFormatDiagnosticCode::invalid_graph};
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::string message;
};

struct MavgGpuClusterFormatDesc {
    const MavgClusterGraphDocument* graph{nullptr};
    const MavgClusterPayloadDocument* payload{nullptr};
    std::uint32_t max_meshlet_triangles{64};
    std::uint32_t max_meshlet_vertices{96};
};

struct MavgGpuClusterFormatRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t local_cluster_index{0};
    std::uint32_t lod_level{0};
    std::uint32_t material_partition{0};
    std::uint32_t parent_cluster_index{0};
    bool has_parent{false};
    std::uint32_t resident_fallback_cluster_index{0};
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
};

struct MavgGpuClusterFormatPlan {
    std::vector<MavgGpuClusterFormatRow> rows;
    std::vector<MavgGpuClusterFormatDiagnostic> diagnostics;
    std::uint32_t cluster_row_count{0};
    std::uint32_t meshlet_ready_cluster_count{0};
    std::uint32_t vertex_stride_bytes{0};
    std::uint32_t index_stride_bytes{0};
    bool index_format_uint32{false};
    std::uint32_t d3d12_mesh_shader_limit_triangles{64};
    std::uint32_t d3d12_mesh_shader_limit_vertices{96};
    std::uint32_t vulkan_mesh_shader_limit_triangles{64};
    std::uint32_t vulkan_mesh_shader_limit_vertices{96};
    bool allocated_rhi_resources{false};
    bool submitted_gpu_work{false};
    bool executed_mesh_shader{false};
    bool executed_gpu_traversal{false};
    bool used_gpu_decompression{false};
    bool touched_native_handles{false};
    bool claimed_nanite_compatibility{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MavgGpuClusterFormatPlan plan_mavg_gpu_cluster_format_rows(const MavgGpuClusterFormatDesc& desc);
[[nodiscard]] bool has_mavg_gpu_cluster_format_diagnostic(const MavgGpuClusterFormatPlan& plan,
                                                          MavgGpuClusterFormatDiagnosticCode code) noexcept;

} // namespace mirakana
