// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_mesh_shader_lod.hpp"

#include <algorithm>
#include <span>
#include <vector>

namespace {

[[nodiscard]] mirakana::AssetId graph_asset() {
    return mirakana::AssetId::from_name("mavg/mesh-shader-lod-graph");
}

[[nodiscard]] mirakana::AssetId material_root_a() {
    return mirakana::AssetId::from_name("materials/mesh-shader/root-a");
}

[[nodiscard]] mirakana::AssetId material_root_b() {
    return mirakana::AssetId::from_name("materials/mesh-shader/root-b");
}

[[nodiscard]] mirakana::MavgLodSelectionResult make_selection_result() {
    return mirakana::MavgLodSelectionResult{
        .selected_clusters =
            {
                mirakana::MavgLodSelectedCluster{
                    .graph_asset = graph_asset(),
                    .cluster_index = 7,
                    .page_index = 2,
                    .lod_level = 1,
                    .material_partition = 3,
                    .first_index = 96,
                    .index_count = 144,
                    .vertex_base = 12,
                    .fallback_substitution = false,
                },
                mirakana::MavgLodSelectedCluster{
                    .graph_asset = graph_asset(),
                    .cluster_index = 8,
                    .page_index = 3,
                    .lod_level = 1,
                    .material_partition = 3,
                    .first_index = 240,
                    .index_count = 96,
                    .vertex_base = 44,
                    .fallback_substitution = true,
                },
            },
    };
}

[[nodiscard]] std::vector<mirakana::MavgMeshShaderLodMaterialRootRow> make_material_roots() {
    return {
        mirakana::MavgMeshShaderLodMaterialRootRow{
            .graph_asset = graph_asset(),
            .material_partition = 3,
            .material_root = material_root_a(),
        },
    };
}

[[nodiscard]] std::vector<mirakana::MavgMeshShaderLodMeshletRow> make_meshlets() {
    return {
        mirakana::MavgMeshShaderLodMeshletRow{
            .graph_asset = graph_asset(),
            .cluster_index = 7,
            .meshlet_index = 70,
            .local_meshlet_index = 0,
            .output_vertex_count = 3,
            .output_primitive_count = 1,
            .group_thread_count = 32,
        },
        mirakana::MavgMeshShaderLodMeshletRow{
            .graph_asset = graph_asset(),
            .cluster_index = 8,
            .meshlet_index = 80,
            .local_meshlet_index = 0,
            .output_vertex_count = 3,
            .output_primitive_count = 1,
            .group_thread_count = 32,
        },
        mirakana::MavgMeshShaderLodMeshletRow{
            .graph_asset = graph_asset(),
            .cluster_index = 8,
            .meshlet_index = 81,
            .local_meshlet_index = 1,
            .output_vertex_count = 3,
            .output_primitive_count = 1,
            .group_thread_count = 32,
        },
    };
}

[[nodiscard]] bool has_diagnostic_code(const mirakana::MavgMeshShaderLodPlan& plan,
                                       mirakana::MavgMeshShaderLodDiagnosticCode code) {
    return std::ranges::any_of(plan.diagnostics, [code](const mirakana::MavgMeshShaderLodDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

} // namespace

MK_TEST("mavg mesh shader lod planning converts selected clusters into deterministic meshlet tasks") {
    const auto selection = make_selection_result();
    const auto material_roots = make_material_roots();
    const auto meshlets = make_meshlets();

    const auto plan = mirakana::plan_mavg_mesh_shader_lod_tasks(mirakana::MavgMeshShaderLodDesc{
        .selection = &selection,
        .material_roots = std::span<const mirakana::MavgMeshShaderLodMaterialRootRow>{material_roots},
        .meshlets = std::span<const mirakana::MavgMeshShaderLodMeshletRow>{meshlets},
        .max_task_rows = 8,
        .expected_group_thread_count = 32,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.selected_cluster_count == 2U);
    MK_REQUIRE(plan.meshlet_task_count == 3U);
    MK_REQUIRE(plan.fallback_draw_count == 2U);
    MK_REQUIRE(plan.uses_mesh_shader_bind_points);
    MK_REQUIRE(plan.uses_amplification_shader_bind_point);
    MK_REQUIRE(!plan.requires_input_assembler);
    MK_REQUIRE(!plan.requires_index_buffer);
    MK_REQUIRE(plan.task_rows[0].cluster_index == 7U);
    MK_REQUIRE(plan.task_rows[0].meshlet_index == 70U);
    MK_REQUIRE(plan.task_rows[0].material_root.value == material_root_a().value);
    MK_REQUIRE(plan.task_rows[1].cluster_index == 8U);
    MK_REQUIRE(plan.task_rows[1].fallback_substitution);
    MK_REQUIRE(plan.fallback_draws[0].first_index == 96U);
    MK_REQUIRE(plan.fallback_draws[0].index_count == 144U);
    MK_REQUIRE(plan.fallback_draws[1].vertex_base == 44);
    MK_REQUIRE(!plan.executed_mesh_shader);
    MK_REQUIRE(!plan.executed_d3d12);
    MK_REQUIRE(!plan.executed_vulkan);
    MK_REQUIRE(!plan.claimed_metal_readiness);
    MK_REQUIRE(!plan.claimed_nanite_equivalence);
}

MK_TEST("mavg mesh shader lod planning fails closed on wrong meshlet group size") {
    const auto selection = make_selection_result();
    const auto material_roots = make_material_roots();
    auto meshlets = make_meshlets();
    meshlets[1].group_thread_count = 129;

    const auto plan = mirakana::plan_mavg_mesh_shader_lod_tasks(mirakana::MavgMeshShaderLodDesc{
        .selection = &selection,
        .material_roots = std::span<const mirakana::MavgMeshShaderLodMaterialRootRow>{material_roots},
        .meshlets = std::span<const mirakana::MavgMeshShaderLodMeshletRow>{meshlets},
        .max_task_rows = 8,
        .expected_group_thread_count = 32,
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.task_rows.empty());
    MK_REQUIRE(plan.fallback_draws.empty());
    MK_REQUIRE(has_diagnostic_code(plan, mirakana::MavgMeshShaderLodDiagnosticCode::invalid_meshlet_group_size));
}

MK_TEST("mavg mesh shader lod planning rejects mismatched material roots") {
    const auto selection = make_selection_result();
    auto material_roots = make_material_roots();
    material_roots.push_back(mirakana::MavgMeshShaderLodMaterialRootRow{
        .graph_asset = graph_asset(),
        .material_partition = 3,
        .material_root = material_root_b(),
    });
    const auto meshlets = make_meshlets();

    const auto plan = mirakana::plan_mavg_mesh_shader_lod_tasks(mirakana::MavgMeshShaderLodDesc{
        .selection = &selection,
        .material_roots = std::span<const mirakana::MavgMeshShaderLodMaterialRootRow>{material_roots},
        .meshlets = std::span<const mirakana::MavgMeshShaderLodMeshletRow>{meshlets},
        .max_task_rows = 8,
        .expected_group_thread_count = 32,
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(has_diagnostic_code(plan, mirakana::MavgMeshShaderLodDiagnosticCode::mismatched_material_roots));
}

MK_TEST("mavg mesh shader lod planning preserves conventional fallback without promoting mesh execution") {
    const auto selection = make_selection_result();
    const auto material_roots = make_material_roots();
    const auto meshlets = make_meshlets();

    const auto plan = mirakana::plan_mavg_mesh_shader_lod_tasks(mirakana::MavgMeshShaderLodDesc{
        .selection = &selection,
        .material_roots = std::span<const mirakana::MavgMeshShaderLodMaterialRootRow>{material_roots},
        .meshlets = std::span<const mirakana::MavgMeshShaderLodMeshletRow>{meshlets},
        .max_task_rows = 8,
        .expected_group_thread_count = 32,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.fallback_to_conventional_indexed_draws);
    MK_REQUIRE(plan.fallback_draws.size() == selection.selected_clusters.size());
    MK_REQUIRE(plan.fallback_draws[0].uses_index_buffer);
    MK_REQUIRE(!plan.fallback_promotes_mesh_shader_lod_ready);
    MK_REQUIRE(!plan.mavg_mesh_shader_lod_ready);
}

int main() {
    return mirakana::test::run_all();
}
