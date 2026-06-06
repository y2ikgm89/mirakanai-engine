// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/mavg_gpu_cluster_format.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

namespace mirakana {
namespace {

constexpr std::uint32_t supported_position_normal_uv_stride_bytes = 32;
constexpr std::uint32_t supported_uint32_index_stride_bytes = 4;

void add_diagnostic(MavgGpuClusterFormatPlan& plan, MavgGpuClusterFormatDiagnosticCode code, AssetId graph_asset,
                    std::uint32_t cluster_index, std::string message) {
    plan.diagnostics.push_back(MavgGpuClusterFormatDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .cluster_index = cluster_index,
        .message = std::move(message),
    });
}

[[nodiscard]] const MavgClusterPayloadPage* find_payload_page(const MavgClusterPayloadDocument& payload,
                                                              std::uint32_t page_index) noexcept {
    for (const auto& page : payload.pages) {
        if (page.page_index == page_index) {
            return &page;
        }
    }
    return nullptr;
}

[[nodiscard]] bool cluster_draw_range_fits_payload(const MavgClusterGraphCluster& cluster,
                                                   std::uint32_t payload_index_count) noexcept {
    if (cluster.index_count == 0U) {
        return false;
    }
    const auto first_index = static_cast<std::uint64_t>(cluster.first_index);
    const auto index_count = static_cast<std::uint64_t>(cluster.index_count);
    const auto payload_count = static_cast<std::uint64_t>(payload_index_count);
    return first_index <= payload_count && index_count <= payload_count - first_index;
}

[[nodiscard]] Vec3 center_of(const MavgBounds3f& bounds) noexcept {
    return Vec3{
        .x = (bounds.min.x + bounds.max.x) * 0.5F,
        .y = (bounds.min.y + bounds.max.y) * 0.5F,
        .z = (bounds.min.z + bounds.max.z) * 0.5F,
    };
}

[[nodiscard]] float radius_of(const MavgBounds3f& bounds) noexcept {
    const auto dx = bounds.max.x - bounds.min.x;
    const auto dy = bounds.max.y - bounds.min.y;
    const auto dz = bounds.max.z - bounds.min.z;
    return std::sqrt((dx * dx) + (dy * dy) + (dz * dz)) * 0.5F;
}

void fail_closed(MavgGpuClusterFormatPlan& plan) {
    plan.rows.clear();
    plan.cluster_row_count = 0;
    plan.meshlet_ready_cluster_count = 0;
}

} // namespace

bool MavgGpuClusterFormatPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

MavgGpuClusterFormatPlan plan_mavg_gpu_cluster_format_rows(const MavgGpuClusterFormatDesc& desc) {
    MavgGpuClusterFormatPlan plan;
    plan.d3d12_mesh_shader_limit_triangles = desc.max_meshlet_triangles;
    plan.d3d12_mesh_shader_limit_vertices = desc.max_meshlet_vertices;
    plan.vulkan_mesh_shader_limit_triangles = desc.max_meshlet_triangles;
    plan.vulkan_mesh_shader_limit_vertices = desc.max_meshlet_vertices;

    const auto* const graph = desc.graph;
    const auto* const payload = desc.payload;
    if (graph == nullptr) {
        add_diagnostic(plan, MavgGpuClusterFormatDiagnosticCode::invalid_graph, AssetId{}, 0,
                       "MAVG GPU cluster format planning requires a graph document");
    }
    if (payload == nullptr) {
        add_diagnostic(plan, MavgGpuClusterFormatDiagnosticCode::invalid_payload, AssetId{}, 0,
                       "MAVG GPU cluster format planning requires a payload document");
    }
    if (graph == nullptr || payload == nullptr) {
        fail_closed(plan);
        return plan;
    }

    const auto graph_validation = validate_mavg_cluster_graph(*graph);
    if (!graph_validation.valid()) {
        add_diagnostic(plan, MavgGpuClusterFormatDiagnosticCode::invalid_graph, graph->asset, 0,
                       "MAVG GPU cluster format planning requires a valid cluster graph");
    }

    const auto payload_validation = validate_mavg_cluster_payload(*payload, *graph);
    if (!payload_validation.valid()) {
        add_diagnostic(plan, MavgGpuClusterFormatDiagnosticCode::invalid_payload, graph->asset, 0,
                       "MAVG GPU cluster format planning requires a valid cluster payload");
    }
    if (payload->vertex_stride_bytes != supported_position_normal_uv_stride_bytes) {
        add_diagnostic(plan, MavgGpuClusterFormatDiagnosticCode::unsupported_vertex_stride, graph->asset, 0,
                       "MAVG GPU cluster format rows currently support only 32-byte position/normal/uv vertices");
    }
    if (payload->index_format != "uint32") {
        add_diagnostic(plan, MavgGpuClusterFormatDiagnosticCode::unsupported_index_format, graph->asset, 0,
                       "MAVG GPU cluster format rows currently support only uint32 indices");
    }

    for (const auto& cluster : graph->clusters) {
        if (!cluster_draw_range_fits_payload(cluster, payload->index_count)) {
            add_diagnostic(plan, MavgGpuClusterFormatDiagnosticCode::cluster_draw_range_out_of_payload, graph->asset,
                           cluster.cluster_index, "MAVG GPU cluster draw range is outside the payload index buffer");
        }
    }

    if (!plan.diagnostics.empty()) {
        fail_closed(plan);
        return plan;
    }

    plan.vertex_stride_bytes = payload->vertex_stride_bytes;
    plan.index_stride_bytes = supported_uint32_index_stride_bytes;
    plan.index_format_uint32 = true;

    auto canonical_graph = canonicalize_mavg_cluster_graph(*graph);
    plan.rows.reserve(canonical_graph.clusters.size());
    for (const auto& cluster : canonical_graph.clusters) {
        const auto* const payload_page = find_payload_page(*payload, cluster.page_index);
        const auto meshlet_ready =
            cluster.triangle_count <= desc.max_meshlet_triangles && cluster.vertex_count <= desc.max_meshlet_vertices;
        if (meshlet_ready) {
            ++plan.meshlet_ready_cluster_count;
        }

        plan.rows.push_back(MavgGpuClusterFormatRow{
            .graph_asset = canonical_graph.asset,
            .cluster_index = cluster.cluster_index,
            .page_index = cluster.page_index,
            .local_cluster_index = cluster.local_cluster_index,
            .lod_level = cluster.lod_level,
            .material_partition = cluster.material_partition,
            .parent_cluster_index = cluster.parent_cluster_index,
            .has_parent = cluster.has_parent,
            .resident_fallback_cluster_index = cluster.resident_fallback_cluster_index,
            .first_index = cluster.first_index,
            .index_count = cluster.index_count,
            .vertex_base = cluster.vertex_base,
            .triangle_count = cluster.triangle_count,
            .vertex_count = cluster.vertex_count,
            .payload_page_byte_offset = payload_page == nullptr ? 0U : payload_page->byte_offset,
            .payload_page_byte_size = payload_page == nullptr ? 0U : payload_page->byte_size,
            .bounds_center = center_of(cluster.bounds),
            .bounds_radius = radius_of(cluster.bounds),
            .meshlet_ready = meshlet_ready,
        });
    }

    plan.cluster_row_count = static_cast<std::uint32_t>(plan.rows.size());
    return plan;
}

bool has_mavg_gpu_cluster_format_diagnostic(const MavgGpuClusterFormatPlan& plan,
                                            MavgGpuClusterFormatDiagnosticCode code) noexcept {
    return std::ranges::any_of(
        plan.diagnostics, [code](const MavgGpuClusterFormatDiagnostic& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
