// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_deformation.hpp"

#include <algorithm>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgBounds3f bounds(float min_value, float max_value) {
    return mirakana::MavgBounds3f{
        .min = {.x = min_value, .y = min_value, .z = min_value},
        .max = {.x = max_value, .y = max_value, .z = max_value},
    };
}

[[nodiscard]] mirakana::MavgClusterGraphDocument make_graph() {
    const auto graph = mirakana::AssetId::from_name("mavg/deformation_graph");
    const auto mesh = mirakana::AssetId::from_name("source/deformation.glb");
    const auto material = mirakana::AssetId::from_name("materials/deformation");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph,
        .source_mesh = mesh,
        .source_mesh_uri = "source/deformation.glb",
        .cluster_payload_uri = "runtime/deformation.mavgpayload",
        .target_cluster_triangles = 128,
        .page_size_bytes = 4096,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0,
                    .byte_offset = 0,
                    .byte_size = 4096,
                    .first_cluster = 0,
                    .cluster_count = 5,
                },
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material,
                    .first_cluster = 0,
                    .cluster_count = 5,
                },
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
                    .bounds = bounds(-1.0F, 1.0F),
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 0,
                    .index_count = 192,
                    .vertex_base = 0,
                    .children = {},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 0,
                    .local_cluster_index = 1,
                    .lod_level = 0,
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds = bounds(-2.0F, 2.0F),
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 1,
                    .geometric_error = 1.0F,
                    .first_index = 192,
                    .index_count = 192,
                    .vertex_base = 96,
                    .children = {},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 2,
                    .page_index = 0,
                    .local_cluster_index = 2,
                    .lod_level = 0,
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds = bounds(-3.0F, 3.0F),
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 2,
                    .geometric_error = 1.0F,
                    .first_index = 384,
                    .index_count = 192,
                    .vertex_base = 192,
                    .children = {},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 3,
                    .page_index = 0,
                    .local_cluster_index = 3,
                    .lod_level = 0,
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds = bounds(-4.0F, 4.0F),
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 3,
                    .geometric_error = 1.0F,
                    .first_index = 576,
                    .index_count = 192,
                    .vertex_base = 288,
                    .children = {},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 4,
                    .page_index = 0,
                    .local_cluster_index = 4,
                    .lod_level = 0,
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds = bounds(-5.0F, 5.0F),
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 4,
                    .geometric_error = 1.0F,
                    .first_index = 768,
                    .index_count = 192,
                    .vertex_base = 384,
                    .children = {},
                },
            },
    };
}

} // namespace

MK_TEST("mavg deformation static and rigid tiers are ready without runtime refit") {
    const auto graph = make_graph();
    const std::vector<mirakana::MavgDeformationClusterRow> rows{
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 1,
            .tier = mirakana::MavgDeformationTier::rigid_instance,
        },
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 0,
            .tier = mirakana::MavgDeformationTier::static_cluster,
        },
    };

    const auto result = mirakana::plan_mavg_deformation_tier_diagnostics(
        mirakana::MavgDeformationTierDiagnosticsDesc{.graph = &graph, .tier_rows = rows});

    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.rows.size() == 2U);
    MK_REQUIRE(result.rows[0].cluster_index == 0U);
    MK_REQUIRE(result.rows[0].supported_by_mavg_cluster_graph);
    MK_REQUIRE(!result.rows[0].requires_conventional_fallback);
    MK_REQUIRE(!result.rows[0].requires_runtime_refit);
    MK_REQUIRE(result.rows[0].bounds_expansion_radius == 0.0F);
    MK_REQUIRE(result.rows[1].cluster_index == 1U);
    MK_REQUIRE(result.rows[1].supported_by_mavg_cluster_graph);
    MK_REQUIRE(result.supported_row_count == 2U);
    MK_REQUIRE(result.fallback_row_count == 0U);
    MK_REQUIRE(!result.touched_renderer_rhi_handles);
    MK_REQUIRE(!result.executed_runtime_upload);
    MK_REQUIRE(!result.executed_mesh_shader);
}

MK_TEST("mavg deformation skinned and morph tiers require reviewed bounds rows") {
    const auto graph = make_graph();
    const std::vector<mirakana::MavgDeformationClusterRow> rows{
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 2,
            .tier = mirakana::MavgDeformationTier::skinned_cluster,
        },
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 3,
            .tier = mirakana::MavgDeformationTier::morph_cluster,
        },
    };
    const std::vector<mirakana::MavgSkinnedClusterBoundsRow> skinned_bounds{
        mirakana::MavgSkinnedClusterBoundsRow{
            .graph_asset = graph.asset,
            .cluster_index = 2,
            .influencing_bone_count = 4,
            .conservative_bounds = bounds(-3.5F, 3.5F),
            .max_bone_displacement_radius = 0.5F,
        },
    };
    const std::vector<mirakana::MavgMorphClusterDeltaBoundsRow> morph_bounds{
        mirakana::MavgMorphClusterDeltaBoundsRow{
            .graph_asset = graph.asset,
            .cluster_index = 3,
            .delta_bounds = bounds(-0.25F, 0.25F),
            .max_delta_radius = 0.25F,
        },
    };

    const auto result = mirakana::plan_mavg_deformation_tier_diagnostics(mirakana::MavgDeformationTierDiagnosticsDesc{
        .graph = &graph,
        .tier_rows = rows,
        .skinned_bounds_rows = skinned_bounds,
        .morph_delta_bounds_rows = morph_bounds,
    });

    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.rows.size() == 2U);
    MK_REQUIRE(result.rows[0].tier == mirakana::MavgDeformationTier::skinned_cluster);
    MK_REQUIRE(result.rows[0].supported_by_mavg_cluster_graph);
    MK_REQUIRE(!result.rows[0].requires_conventional_fallback);
    MK_REQUIRE(result.rows[0].bounds_expansion_radius == 0.5F);
    MK_REQUIRE(result.rows[1].tier == mirakana::MavgDeformationTier::morph_cluster);
    MK_REQUIRE(result.rows[1].supported_by_mavg_cluster_graph);
    MK_REQUIRE(result.rows[1].bounds_expansion_radius == 0.25F);
    MK_REQUIRE(result.rows[1].conservative_bounds.min.x == -4.25F);
    MK_REQUIRE(result.rows[1].conservative_bounds.max.x == 4.25F);
    MK_REQUIRE(result.skinned_row_count == 1U);
    MK_REQUIRE(result.morph_row_count == 1U);
}

MK_TEST("mavg deformation missing bounds and dynamic tiers fail closed to conventional fallback") {
    const auto graph = make_graph();
    const std::vector<mirakana::MavgDeformationClusterRow> rows{
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 2,
            .tier = mirakana::MavgDeformationTier::skinned_cluster,
        },
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 3,
            .tier = mirakana::MavgDeformationTier::morph_cluster,
        },
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 4,
            .tier = mirakana::MavgDeformationTier::dynamic_displacement,
        },
    };

    const auto result = mirakana::plan_mavg_deformation_tier_diagnostics(
        mirakana::MavgDeformationTierDiagnosticsDesc{.graph = &graph, .tier_rows = rows});

    MK_REQUIRE(result.rows.size() == 3U);
    MK_REQUIRE(result.supported_row_count == 0U);
    MK_REQUIRE(result.fallback_row_count == 3U);
    MK_REQUIRE(mirakana::has_mavg_deformation_diagnostic(
        result, mirakana::MavgDeformationDiagnosticCode::missing_skinned_bone_bounds));
    MK_REQUIRE(mirakana::has_mavg_deformation_diagnostic(
        result, mirakana::MavgDeformationDiagnosticCode::missing_morph_delta_bounds));
    MK_REQUIRE(mirakana::has_mavg_deformation_diagnostic(
        result, mirakana::MavgDeformationDiagnosticCode::unsupported_dynamic_tier));
    for (const auto& row : result.rows) {
        MK_REQUIRE(!row.supported_by_mavg_cluster_graph);
        MK_REQUIRE(row.requires_conventional_fallback);
    }
}

MK_TEST("mavg deformation rows are stable and duplicate tier rows are diagnosed") {
    const auto graph = make_graph();
    const std::vector<mirakana::MavgDeformationClusterRow> rows{
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 3,
            .tier = mirakana::MavgDeformationTier::morph_cluster,
        },
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 0,
            .tier = mirakana::MavgDeformationTier::static_cluster,
        },
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 0,
            .tier = mirakana::MavgDeformationTier::static_cluster,
        },
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 99,
            .tier = mirakana::MavgDeformationTier::rigid_instance,
        },
    };

    const auto result = mirakana::plan_mavg_deformation_tier_diagnostics(
        mirakana::MavgDeformationTierDiagnosticsDesc{.graph = &graph, .tier_rows = rows});

    MK_REQUIRE(result.rows.size() == 3U);
    MK_REQUIRE(result.rows[0].cluster_index == 0U);
    MK_REQUIRE(result.rows[1].cluster_index == 3U);
    MK_REQUIRE(result.rows[2].cluster_index == 99U);
    MK_REQUIRE(
        mirakana::has_mavg_deformation_diagnostic(result, mirakana::MavgDeformationDiagnosticCode::duplicate_tier_row));
    MK_REQUIRE(
        mirakana::has_mavg_deformation_diagnostic(result, mirakana::MavgDeformationDiagnosticCode::unknown_cluster));
}

MK_TEST("mavg deformation invalid reviewed bounds rows fail closed") {
    const auto graph = make_graph();
    const std::vector<mirakana::MavgDeformationClusterRow> rows{
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 2,
            .tier = mirakana::MavgDeformationTier::skinned_cluster,
        },
        mirakana::MavgDeformationClusterRow{
            .graph_asset = graph.asset,
            .cluster_index = 3,
            .tier = mirakana::MavgDeformationTier::morph_cluster,
        },
    };
    const std::vector<mirakana::MavgSkinnedClusterBoundsRow> skinned_bounds{
        mirakana::MavgSkinnedClusterBoundsRow{
            .graph_asset = graph.asset,
            .cluster_index = 2,
            .influencing_bone_count = 0,
            .conservative_bounds = bounds(-3.5F, 3.5F),
            .max_bone_displacement_radius = 0.5F,
        },
    };
    const std::vector<mirakana::MavgMorphClusterDeltaBoundsRow> morph_bounds{
        mirakana::MavgMorphClusterDeltaBoundsRow{
            .graph_asset = graph.asset,
            .cluster_index = 3,
            .delta_bounds = bounds(-0.25F, 0.25F),
            .max_delta_radius = -0.25F,
        },
    };

    const auto result = mirakana::plan_mavg_deformation_tier_diagnostics(mirakana::MavgDeformationTierDiagnosticsDesc{
        .graph = &graph,
        .tier_rows = rows,
        .skinned_bounds_rows = skinned_bounds,
        .morph_delta_bounds_rows = morph_bounds,
    });

    MK_REQUIRE(result.supported_row_count == 0U);
    MK_REQUIRE(result.fallback_row_count == 2U);
    MK_REQUIRE(mirakana::has_mavg_deformation_diagnostic(
        result, mirakana::MavgDeformationDiagnosticCode::invalid_skinned_bone_bounds));
    MK_REQUIRE(mirakana::has_mavg_deformation_diagnostic(
        result, mirakana::MavgDeformationDiagnosticCode::invalid_morph_delta_bounds));
}

int main() {
    return mirakana::test::run_all();
}
