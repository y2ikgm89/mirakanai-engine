// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_gpu_cluster_format.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_gpu_cluster_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/gpu-cluster-format");
    const auto source_mesh = mirakana::AssetId::from_name("mesh/source-gpu-cluster-format");
    const auto material = mirakana::AssetId::from_name("material/gpu-cluster-format");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/gpu-cluster-format.glb",
        .cluster_payload_uri = "runtime/mavg/gpu-cluster-format.mavgpayload",
        .target_cluster_triangles = 128,
        .page_size_bytes = 256,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 128, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 128, .byte_size = 128, .first_cluster = 1, .cluster_count = 1},
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material, .first_cluster = 0, .cluster_count = 2},
            },
        .clusters =
            {
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 0,
                    .page_index = 0,
                    .local_cluster_index = 0,
                    .lod_level = 0,
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds =
                        mirakana::MavgBounds3f{
                            .min = {.x = -2.0F, .y = -4.0F, .z = -6.0F},
                            .max = {.x = 2.0F, .y = 4.0F, .z = 6.0F},
                        },
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 8.0F,
                    .first_index = 0,
                    .index_count = 192,
                    .vertex_base = 0,
                    .children = {1},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 32,
                    .vertex_count = 48,
                    .bounds =
                        mirakana::MavgBounds3f{
                            .min = {.x = 0.0F, .y = 0.0F, .z = 0.0F},
                            .max = {.x = 2.0F, .y = 4.0F, .z = 4.0F},
                        },
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 192,
                    .index_count = 96,
                    .vertex_base = 96,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] mirakana::MavgClusterPayloadDocument make_gpu_cluster_payload() {
    const auto graph = make_gpu_cluster_graph();
    return mirakana::MavgClusterPayloadDocument{
        .asset = graph.asset,
        .vertex_count = 144,
        .vertex_stride_bytes = 32,
        .vertex_data_hex = std::string(144U * 32U * 2U, '1'),
        .index_count = 288,
        .index_format = "uint32",
        .index_data_hex = std::string(288U * 4U * 2U, '2'),
        .page_data_hex = std::string(256U * 2U, '3'),
        .pages =
            {
                mirakana::MavgClusterPayloadPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 128, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterPayloadPage{
                    .page_index = 1, .byte_offset = 128, .byte_size = 128, .first_cluster = 1, .cluster_count = 1},
            },
        .clusters =
            {
                mirakana::MavgClusterPayloadCluster{
                    .cluster_index = 0, .page_index = 0, .first_index = 0, .index_count = 192, .vertex_base = 0},
                mirakana::MavgClusterPayloadCluster{
                    .cluster_index = 1, .page_index = 1, .first_index = 192, .index_count = 96, .vertex_base = 96},
            },
    };
}

[[nodiscard]] bool has_diagnostic(const mirakana::MavgGpuClusterFormatPlan& plan,
                                  mirakana::MavgGpuClusterFormatDiagnosticCode code) {
    return std::ranges::any_of(plan.diagnostics, [code](const mirakana::MavgGpuClusterFormatDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

} // namespace

MK_TEST("mavg gpu cluster format rows derive deterministic GPU layout from graph payload") {
    const auto graph = make_gpu_cluster_graph();
    const auto payload = make_gpu_cluster_payload();

    const auto plan = mirakana::plan_mavg_gpu_cluster_format_rows(mirakana::MavgGpuClusterFormatDesc{
        .graph = &graph,
        .payload = &payload,
        .max_meshlet_triangles = 64,
        .max_meshlet_vertices = 96,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.cluster_row_count == 2U);
    MK_REQUIRE(plan.meshlet_ready_cluster_count == 2U);
    MK_REQUIRE(plan.vertex_stride_bytes == 32U);
    MK_REQUIRE(plan.index_stride_bytes == 4U);
    MK_REQUIRE(plan.index_format_uint32);
    MK_REQUIRE(plan.d3d12_mesh_shader_limit_triangles == 64U);
    MK_REQUIRE(plan.d3d12_mesh_shader_limit_vertices == 96U);
    MK_REQUIRE(plan.vulkan_mesh_shader_limit_triangles == 64U);
    MK_REQUIRE(plan.vulkan_mesh_shader_limit_vertices == 96U);
    MK_REQUIRE(!plan.allocated_rhi_resources);
    MK_REQUIRE(!plan.submitted_gpu_work);
    MK_REQUIRE(!plan.executed_mesh_shader);
    MK_REQUIRE(!plan.executed_gpu_traversal);
    MK_REQUIRE(!plan.used_gpu_decompression);
    MK_REQUIRE(!plan.touched_native_handles);
    MK_REQUIRE(!plan.claimed_nanite_compatibility);

    MK_REQUIRE(plan.rows.size() == 2U);
    MK_REQUIRE(plan.rows[0].graph_asset == graph.asset);
    MK_REQUIRE(plan.rows[0].cluster_index == 0U);
    MK_REQUIRE(plan.rows[0].page_index == 0U);
    MK_REQUIRE(plan.rows[0].local_cluster_index == 0U);
    MK_REQUIRE(plan.rows[0].lod_level == 0U);
    MK_REQUIRE(plan.rows[0].material_partition == 0U);
    MK_REQUIRE(!plan.rows[0].has_parent);
    MK_REQUIRE(plan.rows[0].parent_cluster_index == 0U);
    MK_REQUIRE(plan.rows[0].resident_fallback_cluster_index == 0U);
    MK_REQUIRE(plan.rows[0].first_index == 0U);
    MK_REQUIRE(plan.rows[0].index_count == 192U);
    MK_REQUIRE(plan.rows[0].vertex_base == 0);
    MK_REQUIRE(plan.rows[0].triangle_count == 64U);
    MK_REQUIRE(plan.rows[0].vertex_count == 96U);
    MK_REQUIRE(plan.rows[0].payload_page_byte_offset == 0U);
    MK_REQUIRE(plan.rows[0].payload_page_byte_size == 128U);
    MK_REQUIRE(plan.rows[0].bounds_center.x == 0.0F);
    MK_REQUIRE(plan.rows[0].bounds_center.y == 0.0F);
    MK_REQUIRE(plan.rows[0].bounds_center.z == 0.0F);
    MK_REQUIRE(plan.rows[0].bounds_radius > 7.0F);
    MK_REQUIRE(plan.rows[0].meshlet_ready);
    MK_REQUIRE(plan.rows[1].cluster_index == 1U);
    MK_REQUIRE(plan.rows[1].payload_page_byte_offset == 128U);
    MK_REQUIRE(plan.rows[1].bounds_center.x == 1.0F);
    MK_REQUIRE(plan.rows[1].bounds_center.y == 2.0F);
    MK_REQUIRE(plan.rows[1].bounds_center.z == 2.0F);
}

MK_TEST("mavg gpu cluster format rows fail closed on invalid graph and payload") {
    auto invalid_graph = make_gpu_cluster_graph();
    invalid_graph.clusters[0].index_count = 0;
    const auto payload = make_gpu_cluster_payload();
    const auto graph_failure = mirakana::plan_mavg_gpu_cluster_format_rows(mirakana::MavgGpuClusterFormatDesc{
        .graph = &invalid_graph,
        .payload = &payload,
        .max_meshlet_triangles = 64,
        .max_meshlet_vertices = 96,
    });
    MK_REQUIRE(!graph_failure.succeeded());
    MK_REQUIRE(graph_failure.rows.empty());
    MK_REQUIRE(has_diagnostic(graph_failure, mirakana::MavgGpuClusterFormatDiagnosticCode::invalid_graph));

    const auto graph = make_gpu_cluster_graph();
    auto invalid_payload = make_gpu_cluster_payload();
    invalid_payload.clusters[1].first_index = 99;
    const auto payload_failure = mirakana::plan_mavg_gpu_cluster_format_rows(mirakana::MavgGpuClusterFormatDesc{
        .graph = &graph,
        .payload = &invalid_payload,
        .max_meshlet_triangles = 64,
        .max_meshlet_vertices = 96,
    });
    MK_REQUIRE(!payload_failure.succeeded());
    MK_REQUIRE(payload_failure.rows.empty());
    MK_REQUIRE(has_diagnostic(payload_failure, mirakana::MavgGpuClusterFormatDiagnosticCode::invalid_payload));
}

MK_TEST("mavg gpu cluster format rows reject unsupported payload layouts and draw overflow") {
    const auto graph = make_gpu_cluster_graph();

    auto unsupported_stride = make_gpu_cluster_payload();
    unsupported_stride.vertex_stride_bytes = 48;
    const auto stride_failure = mirakana::plan_mavg_gpu_cluster_format_rows(mirakana::MavgGpuClusterFormatDesc{
        .graph = &graph,
        .payload = &unsupported_stride,
        .max_meshlet_triangles = 64,
        .max_meshlet_vertices = 96,
    });
    MK_REQUIRE(!stride_failure.succeeded());
    MK_REQUIRE(has_diagnostic(stride_failure, mirakana::MavgGpuClusterFormatDiagnosticCode::unsupported_vertex_stride));

    auto unsupported_index = make_gpu_cluster_payload();
    unsupported_index.index_format = "uint16";
    const auto index_failure = mirakana::plan_mavg_gpu_cluster_format_rows(mirakana::MavgGpuClusterFormatDesc{
        .graph = &graph,
        .payload = &unsupported_index,
        .max_meshlet_triangles = 64,
        .max_meshlet_vertices = 96,
    });
    MK_REQUIRE(!index_failure.succeeded());
    MK_REQUIRE(has_diagnostic(index_failure, mirakana::MavgGpuClusterFormatDiagnosticCode::unsupported_index_format));

    auto overflow_payload = make_gpu_cluster_payload();
    overflow_payload.index_count = 200;
    const auto overflow_failure = mirakana::plan_mavg_gpu_cluster_format_rows(mirakana::MavgGpuClusterFormatDesc{
        .graph = &graph,
        .payload = &overflow_payload,
        .max_meshlet_triangles = 64,
        .max_meshlet_vertices = 96,
    });
    MK_REQUIRE(!overflow_failure.succeeded());
    MK_REQUIRE(has_diagnostic(overflow_failure,
                              mirakana::MavgGpuClusterFormatDiagnosticCode::cluster_draw_range_out_of_payload));
}

MK_TEST("mavg gpu cluster format rows report clusters outside meshlet limits without executing GPU work") {
    auto graph = make_gpu_cluster_graph();
    graph.clusters[0].triangle_count = 96;
    graph.clusters[0].index_count = 288;
    auto payload = make_gpu_cluster_payload();
    payload.clusters[0].index_count = 288;
    payload.index_count = 384;
    payload.index_data_hex = std::string(384U * 4U * 2U, '2');

    const auto plan = mirakana::plan_mavg_gpu_cluster_format_rows(mirakana::MavgGpuClusterFormatDesc{
        .graph = &graph,
        .payload = &payload,
        .max_meshlet_triangles = 64,
        .max_meshlet_vertices = 96,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.cluster_row_count == 2U);
    MK_REQUIRE(plan.meshlet_ready_cluster_count == 1U);
    MK_REQUIRE(!plan.rows[0].meshlet_ready);
    MK_REQUIRE(plan.rows[1].meshlet_ready);
    MK_REQUIRE(!plan.executed_mesh_shader);
    MK_REQUIRE(!plan.executed_gpu_traversal);
}

int main() {
    return mirakana::test::run_all();
}
