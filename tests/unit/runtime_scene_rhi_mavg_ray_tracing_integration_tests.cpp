// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_scene_rhi/mavg_ray_tracing_integration.hpp"

#include <span>
#include <vector>

namespace {

using mirakana::runtime_scene_rhi::MavgRayTracingGeometryPolicy;
using mirakana::runtime_scene_rhi::MavgRayTracingIntegrationDiagnosticCode;
using mirakana::runtime_scene_rhi::MavgRayTracingMaterialAlphaPolicy;

[[nodiscard]] mirakana::MavgClusterGraphDocument make_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/rt-hero");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/rt-hero-source");
    const auto material_body = mirakana::AssetId::from_name("materials/rt-body");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/rt_hero.gltf",
        .cluster_payload_uri = "runtime/rt_hero.mavgpayload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 4096,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 256, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 256, .byte_size = 256, .first_cluster = 1, .cluster_count = 1},
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material_body, .first_cluster = 0, .cluster_count = 2},
            },
        .clusters =
            {
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 0,
                    .page_index = 0,
                    .local_cluster_index = 0,
                    .lod_level = 0,
                    .triangle_count = 2,
                    .vertex_count = 4,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = -1.0F, .y = -1.0F, .z = -1.0F},
                                                     .max = mirakana::MavgVec3f{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 8.0F,
                    .first_index = 0,
                    .index_count = 6,
                    .vertex_base = 0,
                    .children = {1},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 1,
                    .vertex_count = 3,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = -1.0F, .y = -1.0F, .z = 0.0F},
                                                     .max = mirakana::MavgVec3f{.x = 0.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 6,
                    .index_count = 3,
                    .vertex_base = 12,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] mirakana::runtime_scene_rhi::MavgRayTracingBlasInputRow
make_resident_row(const mirakana::MavgClusterGraphCluster& cluster) {
    return mirakana::runtime_scene_rhi::MavgRayTracingBlasInputRow{
        .cluster_index = cluster.cluster_index,
        .geometry_policy = MavgRayTracingGeometryPolicy::resident_cluster_blas,
        .replay_hash = "rt-replay-resident-0001",
        .geometry_byte_count = 384,
        .index_format = mirakana::runtime_scene_rhi::MavgRayTracingIndexFormat::uint32,
        .page_index = cluster.page_index,
        .transform_row_id = 10,
        .material_alpha_policy = MavgRayTracingMaterialAlphaPolicy::opaque,
        .stable_cluster_id = true,
        .resident_page_valid = true,
        .transform_row_valid = true,
        .uses_resident_cluster_geometry = true,
    };
}

[[nodiscard]] mirakana::runtime_scene_rhi::MavgRayTracingBlasInputRow
make_fallback_row(const mirakana::MavgClusterGraphCluster& cluster) {
    auto row = make_resident_row(cluster);
    row.geometry_policy = MavgRayTracingGeometryPolicy::fallback_mesh_blas;
    row.replay_hash = "rt-replay-fallback-0001";
    row.material_alpha_policy = MavgRayTracingMaterialAlphaPolicy::alpha_tested;
    row.uses_resident_cluster_geometry = false;
    row.uses_fallback_mesh_geometry = true;
    row.fallback_mesh_matches_cluster = true;
    return row;
}

[[nodiscard]] std::vector<mirakana::runtime_scene_rhi::MavgRayTracingBlasInputRow>
make_policy_rows(const mirakana::MavgClusterGraphDocument& graph) {
    return {
        make_resident_row(graph.clusters[0]),
        make_fallback_row(graph.clusters[1]),
    };
}

[[nodiscard]] auto plan(const mirakana::MavgClusterGraphDocument& graph,
                        std::span<const mirakana::runtime_scene_rhi::MavgRayTracingBlasInputRow> rows) {
    return mirakana::runtime_scene_rhi::plan_mavg_ray_tracing_blas_inputs(
        mirakana::runtime_scene_rhi::MavgRayTracingIntegrationDesc{
            .graph = &graph,
            .blas_input_rows = rows,
            .require_backend_execution = false,
        });
}

[[nodiscard]] bool has_code(const mirakana::runtime_scene_rhi::MavgRayTracingIntegrationResult& result,
                            MavgRayTracingIntegrationDiagnosticCode code) noexcept {
    return mirakana::runtime_scene_rhi::has_mavg_ray_tracing_integration_diagnostic(result, code);
}

[[nodiscard]] bool has_cluster_code(const mirakana::runtime_scene_rhi::MavgRayTracingIntegrationResult& result,
                                    std::uint32_t cluster_index,
                                    MavgRayTracingIntegrationDiagnosticCode code) noexcept {
    return mirakana::runtime_scene_rhi::has_mavg_ray_tracing_integration_cluster_diagnostic(result, cluster_index,
                                                                                            code);
}

} // namespace

MK_TEST("runtime scene rhi mavg ray tracing integration accepts selected policy rows") {
    const auto graph = make_graph();
    MK_REQUIRE(mirakana::validate_mavg_cluster_graph(graph).valid());
    const auto rows = make_policy_rows(graph);

    const auto result = plan(graph, rows);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.mavg_ray_tracing_policy_ready);
    MK_REQUIRE(!result.mavg_ray_tracing_integration_ready);
    MK_REQUIRE(!result.mavg_metal_ray_tracing_ready);
    MK_REQUIRE(!result.mavg_broad_ray_tracing_readiness_ready);
    MK_REQUIRE(!result.native_handles_exposed);
    MK_REQUIRE(result.reviewed_blas_input_count == 2U);
    MK_REQUIRE(result.policy_ready_blas_input_count == 2U);
    MK_REQUIRE(result.blas_inputs.size() == 2U);
    MK_REQUIRE(result.blas_inputs[0].geometry_policy == MavgRayTracingGeometryPolicy::resident_cluster_blas);
    MK_REQUIRE(result.blas_inputs[1].geometry_policy == MavgRayTracingGeometryPolicy::fallback_mesh_blas);
}

MK_TEST("runtime scene rhi mavg ray tracing integration rejects implicit mode switches") {
    const auto graph = make_graph();
    auto rows = make_policy_rows(graph);
    rows[0].uses_resident_cluster_geometry = false;
    rows[0].uses_fallback_mesh_geometry = true;
    rows[1].fallback_mesh_matches_cluster = false;

    const auto result = plan(graph, rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(has_cluster_code(result, rows[0].cluster_index,
                                MavgRayTracingIntegrationDiagnosticCode::implicit_geometry_mode_switch));
    MK_REQUIRE(has_cluster_code(result, rows[1].cluster_index,
                                MavgRayTracingIntegrationDiagnosticCode::fallback_mode_mismatch));
}

MK_TEST("runtime scene rhi mavg ray tracing integration rejects incomplete blas input evidence") {
    const auto graph = make_graph();
    auto rows = make_policy_rows(graph);
    rows[0].replay_hash = "";
    rows[0].geometry_byte_count = 0;
    rows[0].transform_row_valid = false;
    rows[0].material_alpha_policy = MavgRayTracingMaterialAlphaPolicy::alpha_blended_rejected;
    rows[1].page_index = 99;
    rows[1].geometry_policy = MavgRayTracingGeometryPolicy::resident_cluster_blas;
    rows[1].uses_fallback_mesh_geometry = false;
    rows[1].uses_resident_cluster_geometry = true;

    const auto result = plan(graph, rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(
        has_cluster_code(result, rows[0].cluster_index, MavgRayTracingIntegrationDiagnosticCode::missing_replay_hash));
    MK_REQUIRE(has_cluster_code(result, rows[0].cluster_index,
                                MavgRayTracingIntegrationDiagnosticCode::invalid_geometry_byte_count));
    MK_REQUIRE(has_cluster_code(result, rows[0].cluster_index,
                                MavgRayTracingIntegrationDiagnosticCode::invalid_transform_row));
    MK_REQUIRE(has_cluster_code(result, rows[0].cluster_index,
                                MavgRayTracingIntegrationDiagnosticCode::invalid_material_alpha_policy));
    MK_REQUIRE(has_cluster_code(result, rows[1].cluster_index,
                                MavgRayTracingIntegrationDiagnosticCode::resident_page_invalid));
}

MK_TEST("runtime scene rhi mavg ray tracing integration requires backend execution before ready") {
    const auto graph = make_graph();
    const auto rows = make_policy_rows(graph);

    const auto result = mirakana::runtime_scene_rhi::plan_mavg_ray_tracing_blas_inputs(
        mirakana::runtime_scene_rhi::MavgRayTracingIntegrationDesc{
            .graph = &graph,
            .blas_input_rows = rows,
            .require_backend_execution = true,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.mavg_ray_tracing_policy_ready);
    MK_REQUIRE(!result.mavg_ray_tracing_integration_ready);
    MK_REQUIRE(has_code(result, MavgRayTracingIntegrationDiagnosticCode::backend_execution_required));
}

MK_TEST("runtime scene rhi mavg ray tracing integration promotes ready only with backend as build rows") {
    const auto graph = make_graph();
    const auto rows = make_policy_rows(graph);
    const std::vector<mirakana::runtime_scene_rhi::MavgRayTracingBackendExecutionRow> backend_rows{
        mirakana::runtime_scene_rhi::MavgRayTracingBackendExecutionRow{
            .backend = mirakana::runtime_scene_rhi::MavgRayTracingBackendKind::d3d12,
            .row_id = "mavg.ray_tracing.backend.d3d12.blas_build",
            .reviewed = true,
            .feature_query_ready = true,
            .acceleration_structure_build_evidence = true,
            .ready = true,
        },
        mirakana::runtime_scene_rhi::MavgRayTracingBackendExecutionRow{
            .backend = mirakana::runtime_scene_rhi::MavgRayTracingBackendKind::vulkan,
            .row_id = "mavg.ray_tracing.backend.vulkan.blas_build",
            .reviewed = true,
            .feature_query_ready = true,
            .acceleration_structure_build_evidence = true,
            .ready = true,
        },
    };

    const auto result = mirakana::runtime_scene_rhi::plan_mavg_ray_tracing_blas_inputs(
        mirakana::runtime_scene_rhi::MavgRayTracingIntegrationDesc{
            .graph = &graph,
            .blas_input_rows = rows,
            .backend_execution_rows = backend_rows,
            .require_backend_execution = true,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.mavg_ray_tracing_policy_ready);
    MK_REQUIRE(result.mavg_ray_tracing_integration_ready);
    MK_REQUIRE(result.backend_execution_ready_count == 2U);
}

MK_TEST("runtime scene rhi mavg ray tracing integration rejects incomplete backend rows") {
    const auto graph = make_graph();
    const auto rows = make_policy_rows(graph);
    const std::vector<mirakana::runtime_scene_rhi::MavgRayTracingBackendExecutionRow> backend_rows{
        mirakana::runtime_scene_rhi::MavgRayTracingBackendExecutionRow{
            .backend = mirakana::runtime_scene_rhi::MavgRayTracingBackendKind::d3d12,
            .row_id = "mavg.ray_tracing.backend.d3d12.missing",
            .reviewed = true,
            .ready = true,
        },
        mirakana::runtime_scene_rhi::MavgRayTracingBackendExecutionRow{
            .backend = mirakana::runtime_scene_rhi::MavgRayTracingBackendKind::vulkan,
            .row_id = "mavg.ray_tracing.backend.vulkan.missing",
            .reviewed = true,
            .feature_query_ready = true,
            .ready = true,
        },
    };

    const auto result = mirakana::runtime_scene_rhi::plan_mavg_ray_tracing_blas_inputs(
        mirakana::runtime_scene_rhi::MavgRayTracingIntegrationDesc{
            .graph = &graph,
            .blas_input_rows = rows,
            .backend_execution_rows = backend_rows,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(has_code(result, MavgRayTracingIntegrationDiagnosticCode::d3d12_dxr_feature_not_ready));
    MK_REQUIRE(has_code(result, MavgRayTracingIntegrationDiagnosticCode::d3d12_acceleration_structure_build_missing));
    MK_REQUIRE(has_code(result, MavgRayTracingIntegrationDiagnosticCode::vulkan_acceleration_structure_build_missing));
}

MK_TEST("runtime scene rhi mavg ray tracing integration rejects native handles metal and broad readiness") {
    const auto graph = make_graph();
    auto rows = make_policy_rows(graph);
    rows[0].touched_native_handles = true;
    const std::vector<mirakana::runtime_scene_rhi::MavgRayTracingBackendExecutionRow> backend_rows{
        mirakana::runtime_scene_rhi::MavgRayTracingBackendExecutionRow{
            .backend = mirakana::runtime_scene_rhi::MavgRayTracingBackendKind::metal_apple_host,
            .row_id = "mavg.ray_tracing.backend.metal.host",
            .reviewed = true,
            .feature_query_ready = true,
            .acceleration_structure_build_evidence = true,
            .ready = true,
        },
    };

    const auto result = mirakana::runtime_scene_rhi::plan_mavg_ray_tracing_blas_inputs(
        mirakana::runtime_scene_rhi::MavgRayTracingIntegrationDesc{
            .graph = &graph,
            .blas_input_rows = rows,
            .backend_execution_rows = backend_rows,
            .request_metal_readiness = true,
            .request_broad_ray_tracing_readiness = true,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.native_handles_exposed);
    MK_REQUIRE(!result.mavg_metal_ray_tracing_ready);
    MK_REQUIRE(!result.mavg_broad_ray_tracing_readiness_ready);
    MK_REQUIRE(has_code(result, MavgRayTracingIntegrationDiagnosticCode::native_handle_access));
    MK_REQUIRE(has_code(result, MavgRayTracingIntegrationDiagnosticCode::metal_readiness_not_promoted));
    MK_REQUIRE(has_code(result, MavgRayTracingIntegrationDiagnosticCode::broad_ray_tracing_readiness_not_promoted));
}

int main() {
    return mirakana::test::run_all();
}
