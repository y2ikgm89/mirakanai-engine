// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_scene_rhi/mavg_deformation_integration.hpp"

#include <algorithm>
#include <span>
#include <vector>

namespace {

using mirakana::runtime_scene_rhi::MavgDeformationIntegrationDiagnosticCode;
using mirakana::runtime_scene_rhi::MavgDeformationIntegrationKind;

[[nodiscard]] mirakana::MavgClusterGraphDocument make_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/deformed-hero");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/deformed-hero-source");
    const auto material_body = mirakana::AssetId::from_name("materials/deformed-body");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/deformed_hero.gltf",
        .cluster_payload_uri = "runtime/deformed_hero.mavgpayload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 4096,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 256, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 256, .byte_size = 256, .first_cluster = 1, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 2, .byte_offset = 512, .byte_size = 256, .first_cluster = 2, .cluster_count = 1},
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material_body, .first_cluster = 0, .cluster_count = 3},
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
                    .children = {1, 2},
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
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 2,
                    .page_index = 2,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 1,
                    .vertex_count = 3,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                                                     .max = mirakana::MavgVec3f{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 9,
                    .index_count = 3,
                    .vertex_base = 24,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] mirakana::MavgBounds3f expanded(const mirakana::MavgBounds3f& bounds, float amount) noexcept {
    return mirakana::MavgBounds3f{
        .min = mirakana::MavgVec3f{.x = bounds.min.x - amount, .y = bounds.min.y - amount, .z = bounds.min.z - amount},
        .max = mirakana::MavgVec3f{.x = bounds.max.x + amount, .y = bounds.max.y + amount, .z = bounds.max.z + amount},
    };
}

[[nodiscard]] mirakana::runtime_scene_rhi::MavgDeformationClusterBoundsRow
make_row(const mirakana::MavgClusterGraphCluster& cluster, MavgDeformationIntegrationKind kind) {
    return mirakana::runtime_scene_rhi::MavgDeformationClusterBoundsRow{
        .cluster_index = cluster.cluster_index,
        .deformation_kind = kind,
        .base_bounds = cluster.bounds,
        .conservative_deformed_bounds =
            kind == MavgDeformationIntegrationKind::rigid_transform ? cluster.bounds : expanded(cluster.bounds, 0.25F),
        .page_index = cluster.page_index,
        .material_partition = cluster.material_partition,
        .first_index = cluster.first_index,
        .index_count = cluster.index_count,
        .vertex_base = cluster.vertex_base,
        .max_joint_influences = kind == MavgDeformationIntegrationKind::linear_blend_skinning ? 4U : 0U,
        .stable_cluster_id = true,
        .resident_page_valid = true,
        .material_root_preserved = true,
        .fallback_draw_range_preserved = true,
    };
}

[[nodiscard]] std::vector<mirakana::runtime_scene_rhi::MavgDeformationClusterBoundsRow>
make_policy_rows(const mirakana::MavgClusterGraphDocument& graph) {
    return {
        make_row(graph.clusters[0], MavgDeformationIntegrationKind::rigid_transform),
        make_row(graph.clusters[1], MavgDeformationIntegrationKind::linear_blend_skinning),
        make_row(graph.clusters[2], MavgDeformationIntegrationKind::morph_target),
    };
}

[[nodiscard]] auto plan(const mirakana::MavgClusterGraphDocument& graph,
                        std::span<const mirakana::runtime_scene_rhi::MavgDeformationClusterBoundsRow> rows) {
    return mirakana::runtime_scene_rhi::plan_mavg_deformation_integrated_clusters(
        mirakana::runtime_scene_rhi::MavgDeformationIntegrationDesc{
            .graph = &graph,
            .cluster_bounds_rows = rows,
            .require_backend_execution = false,
        });
}

[[nodiscard]] bool has_code(const mirakana::runtime_scene_rhi::MavgDeformationIntegrationResult& result,
                            MavgDeformationIntegrationDiagnosticCode code) noexcept {
    return mirakana::runtime_scene_rhi::has_mavg_deformation_integration_diagnostic(result, code);
}

[[nodiscard]] bool has_cluster_code(const mirakana::runtime_scene_rhi::MavgDeformationIntegrationResult& result,
                                    std::uint32_t cluster_index,
                                    MavgDeformationIntegrationDiagnosticCode code) noexcept {
    return mirakana::runtime_scene_rhi::has_mavg_deformation_integration_cluster_diagnostic(result, cluster_index,
                                                                                            code);
}

[[nodiscard]] bool
has_evidence_code(const mirakana::runtime_scene_rhi::MavgDeformationBackendExecutionEvidenceResult& result,
                  MavgDeformationIntegrationDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace

MK_TEST("runtime scene rhi mavg deformation integration accepts selected value policy rows") {
    const auto graph = make_graph();
    MK_REQUIRE(mirakana::validate_mavg_cluster_graph(graph).valid());
    const auto rows = make_policy_rows(graph);

    const auto result = plan(graph, rows);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.mavg_deformation_policy_ready);
    MK_REQUIRE(!result.mavg_deformation_integration_ready);
    MK_REQUIRE(!result.mavg_broad_deformation_readiness_ready);
    MK_REQUIRE(!result.native_handles_exposed);
    MK_REQUIRE(result.reviewed_cluster_count == 3U);
    MK_REQUIRE(result.policy_ready_cluster_count == 3U);
    MK_REQUIRE(result.integrated_clusters.size() == 3U);
    MK_REQUIRE(result.integrated_clusters[1].deformation_kind == MavgDeformationIntegrationKind::linear_blend_skinning);
    MK_REQUIRE(result.integrated_clusters[2].deformation_kind == MavgDeformationIntegrationKind::morph_target);
    MK_REQUIRE(result.integrated_clusters[2].conservative_bounds.min.x < graph.clusters[2].bounds.min.x);
    MK_REQUIRE(result.integrated_clusters[2].first_index == graph.clusters[2].first_index);
    MK_REQUIRE(result.integrated_clusters[2].index_count == graph.clusters[2].index_count);
}

MK_TEST("runtime scene rhi mavg deformation integration rejects topology changing rows") {
    const auto graph = make_graph();
    auto rows = make_policy_rows(graph);
    rows[1].topology_changing_deformation = true;
    rows[1].runtime_generated_triangle_topology = true;

    const auto result = plan(graph, rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.mavg_deformation_policy_ready);
    MK_REQUIRE(has_cluster_code(result, rows[1].cluster_index,
                                MavgDeformationIntegrationDiagnosticCode::topology_changing_deformation));
    MK_REQUIRE(has_cluster_code(result, rows[1].cluster_index,
                                MavgDeformationIntegrationDiagnosticCode::runtime_generated_triangle_topology));
}

MK_TEST("runtime scene rhi mavg deformation integration rejects unbounded skinning") {
    const auto graph = make_graph();
    auto rows = make_policy_rows(graph);
    rows[1].max_joint_influences = 8;
    rows[1].unbounded_vertex_displacement = true;

    const auto result = plan(graph, rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(has_cluster_code(result, rows[1].cluster_index,
                                MavgDeformationIntegrationDiagnosticCode::excessive_joint_influence_count));
    MK_REQUIRE(has_cluster_code(result, rows[1].cluster_index,
                                MavgDeformationIntegrationDiagnosticCode::unbounded_vertex_displacement));
}

MK_TEST("runtime scene rhi mavg deformation integration rejects nonconservative morph bounds") {
    const auto graph = make_graph();
    auto rows = make_policy_rows(graph);
    rows[2].conservative_deformed_bounds.max.x = graph.clusters[2].bounds.max.x - 0.5F;

    const auto result = plan(graph, rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(has_cluster_code(result, rows[2].cluster_index,
                                MavgDeformationIntegrationDiagnosticCode::bounds_not_conservative));
}

MK_TEST("runtime scene rhi mavg deformation integration preserves material page and draw range") {
    const auto graph = make_graph();
    auto rows = make_policy_rows(graph);
    rows[0].material_partition = 1;
    rows[1].resident_page_valid = false;
    rows[2].first_index += 3;

    const auto result = plan(graph, rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(has_cluster_code(result, rows[0].cluster_index,
                                MavgDeformationIntegrationDiagnosticCode::material_root_not_preserved));
    MK_REQUIRE(has_cluster_code(result, rows[1].cluster_index,
                                MavgDeformationIntegrationDiagnosticCode::resident_page_invalid));
    MK_REQUIRE(has_cluster_code(result, rows[2].cluster_index,
                                MavgDeformationIntegrationDiagnosticCode::fallback_draw_range_not_preserved));
}

MK_TEST("runtime scene rhi mavg deformation integration requires backend execution before ready") {
    const auto graph = make_graph();
    const auto rows = make_policy_rows(graph);

    const auto result = mirakana::runtime_scene_rhi::plan_mavg_deformation_integrated_clusters(
        mirakana::runtime_scene_rhi::MavgDeformationIntegrationDesc{
            .graph = &graph,
            .cluster_bounds_rows = rows,
            .require_backend_execution = true,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.mavg_deformation_policy_ready);
    MK_REQUIRE(!result.mavg_deformation_integration_ready);
    MK_REQUIRE(has_code(result, MavgDeformationIntegrationDiagnosticCode::backend_execution_required));
}

MK_TEST("runtime scene rhi mavg deformation backend evidence accepts selected compute morph rows") {
    const auto d3d12_evidence = mirakana::runtime_scene_rhi::evaluate_mavg_deformation_backend_execution_evidence(
        mirakana::runtime_scene_rhi::MavgDeformationBackendExecutionEvidenceDesc{
            .backend = mirakana::runtime_scene_rhi::MavgDeformationBackendKind::d3d12,
            .row_id = "mavg.deformation.backend.d3d12.compute_morph_skinned_upload",
            .reviewed = true,
            .upload_execution_ready = true,
            .compute_morph_skinned_mesh_bindings = 1,
            .morph_mesh_uploads = 1,
            .uploaded_morph_bytes = 292,
            .uploaded_compute_morph_base_position_bytes = 36,
            .compute_morph_output_position_bytes = 36,
            .submitted_upload_fence_count = 4,
            .renderer_consumption_reviewed = true,
        });
    const auto vulkan_evidence = mirakana::runtime_scene_rhi::evaluate_mavg_deformation_backend_execution_evidence(
        mirakana::runtime_scene_rhi::MavgDeformationBackendExecutionEvidenceDesc{
            .backend = mirakana::runtime_scene_rhi::MavgDeformationBackendKind::vulkan,
            .row_id = "mavg.deformation.backend.vulkan.compute_morph_skinned_upload",
            .reviewed = true,
            .upload_execution_ready = true,
            .compute_morph_skinned_mesh_bindings = 1,
            .morph_mesh_uploads = 1,
            .uploaded_morph_bytes = 292,
            .uploaded_compute_morph_base_position_bytes = 36,
            .compute_morph_output_position_bytes = 36,
            .submitted_upload_fence_count = 4,
            .renderer_consumption_reviewed = true,
        });

    MK_REQUIRE(d3d12_evidence.ready);
    MK_REQUIRE(d3d12_evidence.row.ready);
    MK_REQUIRE(d3d12_evidence.row.execution_evidence);
    MK_REQUIRE(d3d12_evidence.diagnostics.empty());
    MK_REQUIRE(vulkan_evidence.ready);
    MK_REQUIRE(vulkan_evidence.row.ready);
    MK_REQUIRE(vulkan_evidence.row.execution_evidence);
    MK_REQUIRE(vulkan_evidence.diagnostics.empty());
}

MK_TEST("runtime scene rhi mavg deformation backend evidence rejects missing execution counters") {
    const auto evidence = mirakana::runtime_scene_rhi::evaluate_mavg_deformation_backend_execution_evidence(
        mirakana::runtime_scene_rhi::MavgDeformationBackendExecutionEvidenceDesc{
            .backend = mirakana::runtime_scene_rhi::MavgDeformationBackendKind::d3d12,
            .row_id = "mavg.deformation.backend.d3d12.missing_output",
            .reviewed = true,
            .upload_execution_ready = true,
            .compute_morph_skinned_mesh_bindings = 1,
            .morph_mesh_uploads = 1,
            .uploaded_morph_bytes = 292,
            .uploaded_compute_morph_base_position_bytes = 36,
            .compute_morph_output_position_bytes = 0,
            .submitted_upload_fence_count = 0,
            .renderer_consumption_reviewed = true,
        });

    MK_REQUIRE(!evidence.ready);
    MK_REQUIRE(!evidence.row.ready);
    MK_REQUIRE(has_evidence_code(evidence, MavgDeformationIntegrationDiagnosticCode::backend_execution_not_ready));
}

MK_TEST("runtime scene rhi mavg deformation backend evidence keeps metal host gated") {
    const auto evidence = mirakana::runtime_scene_rhi::evaluate_mavg_deformation_backend_execution_evidence(
        mirakana::runtime_scene_rhi::MavgDeformationBackendExecutionEvidenceDesc{
            .backend = mirakana::runtime_scene_rhi::MavgDeformationBackendKind::metal_apple_host,
            .row_id = "mavg.deformation.backend.metal.compute_morph_skinned_upload",
            .reviewed = true,
            .upload_execution_ready = true,
            .compute_morph_skinned_mesh_bindings = 1,
            .morph_mesh_uploads = 1,
            .uploaded_morph_bytes = 292,
            .uploaded_compute_morph_base_position_bytes = 36,
            .compute_morph_output_position_bytes = 36,
            .submitted_upload_fence_count = 4,
            .renderer_consumption_reviewed = true,
        });

    MK_REQUIRE(!evidence.ready);
    MK_REQUIRE(!evidence.row.ready);
    MK_REQUIRE(has_evidence_code(evidence, MavgDeformationIntegrationDiagnosticCode::backend_execution_not_ready));
}

MK_TEST("runtime scene rhi mavg deformation backend evidence rejects native handles and broad readiness") {
    const auto evidence = mirakana::runtime_scene_rhi::evaluate_mavg_deformation_backend_execution_evidence(
        mirakana::runtime_scene_rhi::MavgDeformationBackendExecutionEvidenceDesc{
            .backend = mirakana::runtime_scene_rhi::MavgDeformationBackendKind::vulkan,
            .row_id = "mavg.deformation.backend.vulkan.native_handle_claim",
            .reviewed = true,
            .upload_execution_ready = true,
            .compute_morph_skinned_mesh_bindings = 1,
            .morph_mesh_uploads = 1,
            .uploaded_morph_bytes = 292,
            .uploaded_compute_morph_base_position_bytes = 36,
            .compute_morph_output_position_bytes = 36,
            .submitted_upload_fence_count = 4,
            .renderer_consumption_reviewed = true,
            .touched_native_handles = true,
            .request_broad_deformation_readiness = true,
        });

    MK_REQUIRE(!evidence.ready);
    MK_REQUIRE(!evidence.row.ready);
    MK_REQUIRE(evidence.native_handles_exposed);
    MK_REQUIRE(!evidence.broad_deformation_readiness_ready);
    MK_REQUIRE(has_evidence_code(evidence, MavgDeformationIntegrationDiagnosticCode::native_handle_access));
    MK_REQUIRE(has_evidence_code(evidence,
                                 MavgDeformationIntegrationDiagnosticCode::broad_deformation_readiness_not_promoted));
}

MK_TEST("runtime scene rhi mavg deformation integration promotes ready only with reviewed backend rows") {
    const auto graph = make_graph();
    const auto rows = make_policy_rows(graph);
    const std::vector<mirakana::runtime_scene_rhi::MavgDeformationBackendExecutionRow> backend_rows{
        mirakana::runtime_scene_rhi::MavgDeformationBackendExecutionRow{
            .backend = mirakana::runtime_scene_rhi::MavgDeformationBackendKind::d3d12,
            .row_id = "mavg.deformation.backend.d3d12.dynamic_vertex_upload",
            .reviewed = true,
            .execution_evidence = true,
            .ready = true,
        },
        mirakana::runtime_scene_rhi::MavgDeformationBackendExecutionRow{
            .backend = mirakana::runtime_scene_rhi::MavgDeformationBackendKind::vulkan,
            .row_id = "mavg.deformation.backend.vulkan.dynamic_vertex_upload",
            .reviewed = true,
            .execution_evidence = true,
            .ready = true,
        },
    };

    const auto result = mirakana::runtime_scene_rhi::plan_mavg_deformation_integrated_clusters(
        mirakana::runtime_scene_rhi::MavgDeformationIntegrationDesc{
            .graph = &graph,
            .cluster_bounds_rows = rows,
            .backend_execution_rows = backend_rows,
            .require_backend_execution = true,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.mavg_deformation_policy_ready);
    MK_REQUIRE(result.mavg_deformation_integration_ready);
    MK_REQUIRE(result.backend_execution_ready_count == 2U);
}

MK_TEST("runtime scene rhi mavg deformation integration rejects native handles and broad readiness") {
    const auto graph = make_graph();
    auto rows = make_policy_rows(graph);
    rows[0].touched_native_handles = true;

    const auto result = mirakana::runtime_scene_rhi::plan_mavg_deformation_integrated_clusters(
        mirakana::runtime_scene_rhi::MavgDeformationIntegrationDesc{
            .graph = &graph,
            .cluster_bounds_rows = rows,
            .request_broad_deformation_readiness = true,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.native_handles_exposed);
    MK_REQUIRE(!result.mavg_broad_deformation_readiness_ready);
    MK_REQUIRE(has_code(result, MavgDeformationIntegrationDiagnosticCode::native_handle_access));
    MK_REQUIRE(has_code(result, MavgDeformationIntegrationDiagnosticCode::broad_deformation_readiness_not_promoted));
}

int main() {
    return mirakana::test::run_all();
}
