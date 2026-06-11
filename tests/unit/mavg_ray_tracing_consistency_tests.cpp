// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/assets/mavg_ray_tracing_consistency.hpp"

#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/rt_consistency_graph");
    const auto source_mesh = mirakana::AssetId::from_name("source/rt_consistency.glb");
    const auto material = mirakana::AssetId::from_name("materials/stone");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/meshes/rt_consistency.glb",
        .cluster_payload_uri = "runtime/mavg/rt_consistency.mavgpayload",
        .target_cluster_triangles = 128,
        .page_size_bytes = 65536,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0,
                    .byte_offset = 0,
                    .byte_size = 512,
                    .first_cluster = 0,
                    .cluster_count = 1,
                },
                mirakana::MavgClusterGraphPage{
                    .page_index = 1,
                    .byte_offset = 512,
                    .byte_size = 512,
                    .first_cluster = 1,
                    .cluster_count = 1,
                },
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material,
                    .first_cluster = 0,
                    .cluster_count = 2,
                },
            },
        .clusters =
            {
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 0,
                    .page_index = 0,
                    .local_cluster_index = 0,
                    .lod_level = 0,
                    .triangle_count = 128,
                    .vertex_count = 192,
                    .bounds = {.min = {.x = -2.0F, .y = -2.0F, .z = -2.0F}, .max = {.x = 2.0F, .y = 2.0F, .z = 2.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 8.0F,
                    .first_index = 0,
                    .index_count = 384,
                    .vertex_base = 0,
                    .children = {1},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds = {.min = {.x = -1.0F, .y = -1.0F, .z = -1.0F}, .max = {.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 384,
                    .index_count = 192,
                    .vertex_base = 192,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] mirakana::MavgRasterClusterPayloadRow make_raster_row(std::uint32_t cluster_index) {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/rt_consistency_graph");
    return mirakana::MavgRasterClusterPayloadRow{
        .graph_asset = graph_asset,
        .cluster_index = cluster_index,
        .material_partition = 0,
        .page_index = cluster_index,
        .first_index = cluster_index == 0U ? 0U : 384U,
        .index_count = cluster_index == 0U ? 384U : 192U,
        .vertex_base = cluster_index == 0U ? 0 : 192,
        .resident_fallback_cluster_index = 0,
    };
}

[[nodiscard]] mirakana::MavgRayTracingClusterPayloadRow make_ray_tracing_row(std::uint32_t cluster_index) {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/rt_consistency_graph");
    return mirakana::MavgRayTracingClusterPayloadRow{
        .graph_asset = graph_asset,
        .cluster_index = cluster_index,
        .material_partition = 0,
        .page_index = cluster_index,
        .first_index = cluster_index == 0U ? 0U : 384U,
        .index_count = cluster_index == 0U ? 384U : 192U,
        .vertex_base = cluster_index == 0U ? 0 : 192,
        .resident_fallback_cluster_index = 0,
        .payload_policy = mirakana::MavgRayTracingPayloadPolicy::static_blas_build,
        .deformation_tier = mirakana::MavgRayTracingDeformationTier::static_cluster,
    };
}

[[nodiscard]] bool has_diagnostic(const std::vector<mirakana::MavgRayTracingConsistencyDiagnostic>& diagnostics,
                                  mirakana::MavgRayTracingConsistencyDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("mavg ray tracing consistency accepts matching raster and rt payload rows") {
    const auto graph = make_graph();
    const auto desc = mirakana::MavgRayTracingConsistencyDesc{
        .cluster_graph = &graph,
        .raster_payloads = {make_raster_row(0), make_raster_row(1)},
        .ray_tracing_payloads = {make_ray_tracing_row(0), make_ray_tracing_row(1)},
    };

    const auto result = mirakana::plan_mavg_ray_tracing_consistency(desc);

    MK_REQUIRE(result.valid());
    MK_REQUIRE(result.rows.size() == 2U);
    MK_REQUIRE(result.rows[0].cluster_index == 0U);
    MK_REQUIRE(result.rows[0].consistent);
    MK_REQUIRE(result.rows[0].requires_backend_blas_build);
    MK_REQUIRE(!result.rows[0].requires_backend_blas_refit);
    MK_REQUIRE(!result.executed_ray_tracing);
    MK_REQUIRE(!result.exposed_native_handles);
}

MK_TEST("mavg ray tracing consistency rejects missing and duplicate evidence") {
    const auto graph = make_graph();
    auto duplicate = make_ray_tracing_row(1);
    duplicate.cluster_index = 0;
    const auto desc = mirakana::MavgRayTracingConsistencyDesc{
        .cluster_graph = &graph,
        .raster_payloads = {make_raster_row(0)},
        .ray_tracing_payloads = {make_ray_tracing_row(0), duplicate},
    };

    const auto result = mirakana::plan_mavg_ray_tracing_consistency(desc);

    MK_REQUIRE(!result.valid());
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              mirakana::MavgRayTracingConsistencyDiagnosticCode::duplicate_ray_tracing_payload_row));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, mirakana::MavgRayTracingConsistencyDiagnosticCode::missing_raster_payload));
}

MK_TEST("mavg ray tracing consistency rejects mismatched raster and rt payload provenance") {
    const auto graph = make_graph();
    auto ray_tracing_row = make_ray_tracing_row(1);
    ray_tracing_row.first_index = 0;
    ray_tracing_row.resident_fallback_cluster_index = 1;
    const auto desc = mirakana::MavgRayTracingConsistencyDesc{
        .cluster_graph = &graph,
        .raster_payloads = {make_raster_row(1)},
        .ray_tracing_payloads = {ray_tracing_row},
    };

    const auto result = mirakana::plan_mavg_ray_tracing_consistency(desc);

    MK_REQUIRE(!result.valid());
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              mirakana::MavgRayTracingConsistencyDiagnosticCode::payload_draw_range_mismatch));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, mirakana::MavgRayTracingConsistencyDiagnosticCode::fallback_mismatch));
}

MK_TEST("mavg ray tracing consistency rejects payload rows that agree with each other but not the cluster graph") {
    const auto graph = make_graph();
    auto raster_row = make_raster_row(1);
    auto ray_tracing_row = make_ray_tracing_row(1);
    raster_row.page_index = 0;
    ray_tracing_row.page_index = 0;
    raster_row.first_index = 0;
    ray_tracing_row.first_index = 0;
    raster_row.resident_fallback_cluster_index = 1;
    ray_tracing_row.resident_fallback_cluster_index = 1;
    const auto desc = mirakana::MavgRayTracingConsistencyDesc{
        .cluster_graph = &graph,
        .raster_payloads = {raster_row},
        .ray_tracing_payloads = {ray_tracing_row},
    };

    const auto result = mirakana::plan_mavg_ray_tracing_consistency(desc);

    MK_REQUIRE(!result.valid());
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, mirakana::MavgRayTracingConsistencyDiagnosticCode::payload_page_mismatch));
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              mirakana::MavgRayTracingConsistencyDiagnosticCode::payload_draw_range_mismatch));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, mirakana::MavgRayTracingConsistencyDiagnosticCode::fallback_mismatch));
}

MK_TEST("mavg ray tracing consistency rejects invalid cluster graphs before producing rows") {
    auto graph = make_graph();
    graph.clusters[1].cluster_index = graph.clusters[0].cluster_index;
    const auto desc = mirakana::MavgRayTracingConsistencyDesc{
        .cluster_graph = &graph,
        .raster_payloads = {make_raster_row(0)},
        .ray_tracing_payloads = {make_ray_tracing_row(0)},
    };

    const auto result = mirakana::plan_mavg_ray_tracing_consistency(desc);

    MK_REQUIRE(!result.valid());
    MK_REQUIRE(result.rows.empty());
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, mirakana::MavgRayTracingConsistencyDiagnosticCode::invalid_cluster_graph));
}

MK_TEST("mavg ray tracing consistency rejects unsupported deformation tiers") {
    const auto graph = make_graph();
    auto refit_row = make_ray_tracing_row(1);
    refit_row.payload_policy = mirakana::MavgRayTracingPayloadPolicy::deformable_blas_refit;
    refit_row.deformation_tier = mirakana::MavgRayTracingDeformationTier::dynamic_displacement;
    const auto desc = mirakana::MavgRayTracingConsistencyDesc{
        .cluster_graph = &graph,
        .raster_payloads = {make_raster_row(1)},
        .ray_tracing_payloads = {refit_row},
    };

    const auto result = mirakana::plan_mavg_ray_tracing_consistency(desc);

    MK_REQUIRE(!result.valid());
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              mirakana::MavgRayTracingConsistencyDiagnosticCode::unsupported_dynamic_displacement));
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              mirakana::MavgRayTracingConsistencyDiagnosticCode::unsupported_deformation_tier));
}

int main() {
    return mirakana::test::run_all();
}
