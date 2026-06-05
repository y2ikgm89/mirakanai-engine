// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/mavg_cluster_graph.hpp"

#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_valid_document() {
    const auto asset = mirakana::AssetId::from_name("mavg/cathedral_cluster_graph");
    const auto source_mesh = mirakana::AssetId::from_name("source/cathedral_high.glb");
    const auto material_stone = mirakana::AssetId::from_name("materials/stone");
    const auto material_glass = mirakana::AssetId::from_name("materials/glass");

    return mirakana::MavgClusterGraphDocument{
        .asset = asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/meshes/cathedral_high.glb",
        .cluster_payload_uri = "runtime/mavg/cathedral_cluster_pages.mavgpayload",
        .target_cluster_triangles = 128,
        .page_size_bytes = 65536,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 2,
                    .byte_offset = 768,
                    .byte_size = 256,
                    .first_cluster = 2,
                    .cluster_count = 1,
                },
                mirakana::MavgClusterGraphPage{
                    .page_index = 1,
                    .byte_offset = 512,
                    .byte_size = 256,
                    .first_cluster = 1,
                    .cluster_count = 1,
                },
                mirakana::MavgClusterGraphPage{
                    .page_index = 0,
                    .byte_offset = 0,
                    .byte_size = 512,
                    .first_cluster = 0,
                    .cluster_count = 1,
                },
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material_glass,
                    .first_cluster = 1,
                    .cluster_count = 2,
                },
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material_stone,
                    .first_cluster = 0,
                    .cluster_count = 1,
                },
            },
        .clusters =
            {
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds =
                        mirakana::MavgBounds3f{
                            .min = {.x = -1.0F, .y = -2.0F, .z = -3.0F},
                            .max = {.x = 1.0F, .y = 2.0F, .z = 3.0F},
                        },
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.5F,
                    .first_index = 384,
                    .index_count = 192,
                    .vertex_base = 192,
                    .children = {},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 2,
                    .page_index = 2,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds =
                        mirakana::MavgBounds3f{
                            .min = {.x = 0.0F, .y = -2.0F, .z = -3.0F},
                            .max = {.x = 2.0F, .y = 2.0F, .z = 3.0F},
                        },
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.25F,
                    .first_index = 576,
                    .index_count = 192,
                    .vertex_base = 288,
                    .children = {},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 0,
                    .page_index = 0,
                    .local_cluster_index = 0,
                    .lod_level = 0,
                    .triangle_count = 128,
                    .vertex_count = 192,
                    .bounds =
                        mirakana::MavgBounds3f{
                            .min = {.x = -2.0F, .y = -4.0F, .z = -6.0F},
                            .max = {.x = 2.0F, .y = 4.0F, .z = 6.0F},
                        },
                    .material_partition = 1,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 8.0F,
                    .first_index = 0,
                    .index_count = 384,
                    .vertex_base = 0,
                    .children = {2, 1},
                },
            },
    };
}

[[nodiscard]] bool has_diagnostic_code(const std::vector<mirakana::MavgClusterGraphDiagnostic>& diagnostics,
                                       mirakana::MavgClusterGraphDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("mavg cluster graph validates a deterministic cooked descriptor") {
    const auto document = make_valid_document();
    const auto validation = mirakana::validate_mavg_cluster_graph(document);
    MK_REQUIRE(validation.diagnostics.empty());
    MK_REQUIRE(mirakana::mavg_cluster_graph_format_v1() == "GameEngine.MavgClusterGraph.v1");
}

MK_TEST("mavg cluster graph canonicalizes pages partitions clusters and children") {
    const auto canonical = mirakana::canonicalize_mavg_cluster_graph(make_valid_document());
    MK_REQUIRE(canonical.pages.size() == 3U);
    MK_REQUIRE(canonical.pages[0].page_index == 0U);
    MK_REQUIRE(canonical.pages[1].page_index == 1U);
    MK_REQUIRE(canonical.pages[2].page_index == 2U);
    MK_REQUIRE(canonical.material_partitions.size() == 2U);
    MK_REQUIRE(canonical.material_partitions[0].first_cluster == 0U);
    MK_REQUIRE(canonical.material_partitions[1].first_cluster == 1U);
    MK_REQUIRE(canonical.clusters.size() == 3U);
    MK_REQUIRE(canonical.clusters[0].cluster_index == 0U);
    MK_REQUIRE(canonical.clusters[0].children.size() == 2U);
    MK_REQUIRE(canonical.clusters[0].children[0] == 1U);
    MK_REQUIRE(canonical.clusters[0].children[1] == 2U);

    const auto serialized = mirakana::serialize_mavg_cluster_graph_document(canonical);
    MK_REQUIRE(serialized.contains("format=GameEngine.MavgClusterGraph.v1\n"));
    MK_REQUIRE(serialized.contains("asset.kind=mavg_cluster_graph\n"));
    MK_REQUIRE(serialized.contains("cluster.0.children=1,2\n"));

    const auto parsed = mirakana::deserialize_mavg_cluster_graph_document(serialized);
    MK_REQUIRE(parsed.pages[0].page_index == 0U);
    MK_REQUIRE(parsed.material_partitions[0].first_cluster == 0U);
    MK_REQUIRE(parsed.clusters[0].children[0] == 1U);
    MK_REQUIRE(parsed.clusters[0].children[1] == 2U);
}

MK_TEST("mavg cluster graph serializes runtime lod hierarchy draw ranges and fallback metadata") {
    const auto serialized = mirakana::serialize_mavg_cluster_graph_document(make_valid_document());
    MK_REQUIRE(serialized.contains("cluster.0.has_parent=0\n"));
    MK_REQUIRE(serialized.contains("cluster.0.parent=0\n"));
    MK_REQUIRE(serialized.contains("cluster.0.resident_fallback=0\n"));
    MK_REQUIRE(serialized.contains("cluster.0.geometric_error=8\n"));
    MK_REQUIRE(serialized.contains("cluster.0.first_index=0\n"));
    MK_REQUIRE(serialized.contains("cluster.0.index_count=384\n"));
    MK_REQUIRE(serialized.contains("cluster.0.vertex_base=0\n"));
    MK_REQUIRE(serialized.contains("cluster.1.has_parent=1\n"));
    MK_REQUIRE(serialized.contains("cluster.1.parent=0\n"));
    MK_REQUIRE(serialized.contains("cluster.1.resident_fallback=0\n"));
    MK_REQUIRE(serialized.contains("cluster.1.geometric_error=1.5\n"));
    MK_REQUIRE(serialized.contains("cluster.1.first_index=384\n"));
    MK_REQUIRE(serialized.contains("cluster.1.index_count=192\n"));
    MK_REQUIRE(serialized.contains("cluster.1.vertex_base=192\n"));
    MK_REQUIRE(serialized.contains("cluster.2.has_parent=1\n"));
    MK_REQUIRE(serialized.contains("cluster.2.parent=0\n"));
    MK_REQUIRE(serialized.contains("cluster.2.resident_fallback=0\n"));
    MK_REQUIRE(serialized.contains("cluster.2.geometric_error=1.25\n"));
    MK_REQUIRE(serialized.contains("cluster.2.first_index=576\n"));
    MK_REQUIRE(serialized.contains("cluster.2.index_count=192\n"));
    MK_REQUIRE(serialized.contains("cluster.2.vertex_base=288\n"));

    const auto parsed = mirakana::deserialize_mavg_cluster_graph_document(serialized);
    MK_REQUIRE(parsed.clusters[0].cluster_index == 0U);
    MK_REQUIRE(!parsed.clusters[0].has_parent);
    MK_REQUIRE(parsed.clusters[0].resident_fallback_cluster_index == 0U);
    MK_REQUIRE(parsed.clusters[0].geometric_error == 8.0F);
    MK_REQUIRE(parsed.clusters[0].first_index == 0U);
    MK_REQUIRE(parsed.clusters[0].index_count == 384U);
    MK_REQUIRE(parsed.clusters[0].vertex_base == 0);
    MK_REQUIRE(parsed.clusters[1].cluster_index == 1U);
    MK_REQUIRE(parsed.clusters[1].has_parent);
    MK_REQUIRE(parsed.clusters[1].parent_cluster_index == 0U);
    MK_REQUIRE(parsed.clusters[1].resident_fallback_cluster_index == 0U);
    MK_REQUIRE(parsed.clusters[1].geometric_error == 1.5F);
    MK_REQUIRE(parsed.clusters[1].first_index == 384U);
    MK_REQUIRE(parsed.clusters[1].index_count == 192U);
    MK_REQUIRE(parsed.clusters[1].vertex_base == 192);
    MK_REQUIRE(parsed.clusters[2].cluster_index == 2U);
    MK_REQUIRE(parsed.clusters[2].has_parent);
    MK_REQUIRE(parsed.clusters[2].parent_cluster_index == 0U);
    MK_REQUIRE(parsed.clusters[2].resident_fallback_cluster_index == 0U);
    MK_REQUIRE(parsed.clusters[2].geometric_error == 1.25F);
    MK_REQUIRE(parsed.clusters[2].first_index == 576U);
    MK_REQUIRE(parsed.clusters[2].index_count == 192U);
    MK_REQUIRE(parsed.clusters[2].vertex_base == 288);
}

MK_TEST("mavg cluster graph rejects duplicate pages and unknown child references") {
    auto document = make_valid_document();
    document.pages[1].page_index = document.pages[0].page_index;
    document.clusters[0].children.push_back(99U);

    const auto validation = mirakana::validate_mavg_cluster_graph(document);
    MK_REQUIRE(validation.diagnostics.size() >= 2U);

    MK_REQUIRE(
        has_diagnostic_code(validation.diagnostics, mirakana::MavgClusterGraphDiagnosticCode::duplicate_page_index));
    MK_REQUIRE(
        has_diagnostic_code(validation.diagnostics, mirakana::MavgClusterGraphDiagnosticCode::unknown_child_cluster));
}

MK_TEST("mavg cluster graph rejects invalid lod hierarchy fallback and draw metadata") {
    auto cycle = make_valid_document();
    cycle.clusters[2].has_parent = true;
    cycle.clusters[2].parent_cluster_index = 1;
    const auto cycle_validation = mirakana::validate_mavg_cluster_graph(cycle);
    MK_REQUIRE(has_diagnostic_code(cycle_validation.diagnostics,
                                   mirakana::MavgClusterGraphDiagnosticCode::missing_root_cluster));
    MK_REQUIRE(
        has_diagnostic_code(cycle_validation.diagnostics, mirakana::MavgClusterGraphDiagnosticCode::parent_cycle));

    auto parent_error = make_valid_document();
    parent_error.clusters[2].geometric_error = 0.5F;
    const auto parent_error_validation = mirakana::validate_mavg_cluster_graph(parent_error);
    MK_REQUIRE(has_diagnostic_code(parent_error_validation.diagnostics,
                                   mirakana::MavgClusterGraphDiagnosticCode::parent_error_less_than_child));

    auto missing_fallback = make_valid_document();
    missing_fallback.clusters[0].resident_fallback_cluster_index = 99;
    const auto missing_fallback_validation = mirakana::validate_mavg_cluster_graph(missing_fallback);
    MK_REQUIRE(has_diagnostic_code(missing_fallback_validation.diagnostics,
                                   mirakana::MavgClusterGraphDiagnosticCode::missing_resident_fallback));

    auto invalid_fallback = make_valid_document();
    invalid_fallback.clusters[0].resident_fallback_cluster_index = 1;
    const auto invalid_fallback_validation = mirakana::validate_mavg_cluster_graph(invalid_fallback);
    MK_REQUIRE(has_diagnostic_code(invalid_fallback_validation.diagnostics,
                                   mirakana::MavgClusterGraphDiagnosticCode::fallback_not_ancestor));

    auto invalid_draw_range = make_valid_document();
    invalid_draw_range.clusters[0].index_count = 0;
    const auto invalid_draw_range_validation = mirakana::validate_mavg_cluster_graph(invalid_draw_range);
    MK_REQUIRE(has_diagnostic_code(invalid_draw_range_validation.diagnostics,
                                   mirakana::MavgClusterGraphDiagnosticCode::invalid_cluster_draw_range));

    auto invalid_geometric_error = make_valid_document();
    invalid_geometric_error.clusters[0].geometric_error = -1.0F;
    const auto invalid_geometric_error_validation = mirakana::validate_mavg_cluster_graph(invalid_geometric_error);
    MK_REQUIRE(has_diagnostic_code(invalid_geometric_error_validation.diagnostics,
                                   mirakana::MavgClusterGraphDiagnosticCode::invalid_cluster_geometric_error));
}

MK_TEST("mavg cluster graph asset and dependency kinds are package contract rows") {
    const auto asset = mirakana::AssetId::from_name("mavg/cathedral_cluster_graph");
    const auto source_mesh = mirakana::AssetId::from_name("source/cathedral_high.glb");
    const auto material = mirakana::AssetId::from_name("materials/stone");

    MK_REQUIRE(mirakana::is_valid_asset_dependency_edge(mirakana::AssetDependencyEdge{
        .asset = asset,
        .dependency = source_mesh,
        .kind = mirakana::AssetDependencyKind::mavg_source_mesh,
        .path = "source/meshes/cathedral_high.glb",
    }));
    MK_REQUIRE(mirakana::is_valid_asset_dependency_edge(mirakana::AssetDependencyEdge{
        .asset = asset,
        .dependency = material,
        .kind = mirakana::AssetDependencyKind::mavg_material,
        .path = "materials/stone.material",
    }));

    std::vector<mirakana::AssetCookedArtifact> artifacts;
    artifacts.push_back(mirakana::AssetCookedArtifact{
        .asset = source_mesh,
        .kind = mirakana::AssetKind::mesh,
        .path = "runtime/source/cathedral_high.mesh",
        .content = "format=GameEngine.CookedMesh.v1\n",
        .source_revision = 4U,
        .dependencies = {},
    });
    artifacts.push_back(mirakana::AssetCookedArtifact{
        .asset = material,
        .kind = mirakana::AssetKind::material,
        .path = "runtime/materials/stone.material",
        .content = "format=GameEngine.Material.v1\n",
        .source_revision = 4U,
        .dependencies = {},
    });
    artifacts.push_back(mirakana::AssetCookedArtifact{
        .asset = asset,
        .kind = mirakana::AssetKind::mavg_cluster_graph,
        .path = "runtime/mavg/cathedral_cluster_graph.mavg",
        .content = mirakana::serialize_mavg_cluster_graph_document(make_valid_document()),
        .source_revision = 4U,
        .dependencies = {source_mesh, material},
    });
    std::vector<mirakana::AssetDependencyEdge> dependencies;
    dependencies.push_back(mirakana::AssetDependencyEdge{
        .asset = asset,
        .dependency = source_mesh,
        .kind = mirakana::AssetDependencyKind::mavg_source_mesh,
        .path = "source/meshes/cathedral_high.glb",
    });
    dependencies.push_back(mirakana::AssetDependencyEdge{
        .asset = asset,
        .dependency = material,
        .kind = mirakana::AssetDependencyKind::mavg_material,
        .path = "materials/stone.material",
    });

    const auto index = mirakana::build_asset_cooked_package_index(std::move(artifacts), std::move(dependencies));
    MK_REQUIRE(index.entries.size() == 3U);
    bool found_mavg_entry = false;
    for (const auto& entry : index.entries) {
        if (entry.asset == asset) {
            found_mavg_entry = true;
            MK_REQUIRE(entry.kind == mirakana::AssetKind::mavg_cluster_graph);
            MK_REQUIRE(entry.dependencies.size() == 2U);
        }
    }
    MK_REQUIRE(found_mavg_entry);
    MK_REQUIRE(index.dependencies.size() == 2U);
    bool found_source_edge = false;
    bool found_material_edge = false;
    for (const auto& edge : index.dependencies) {
        found_source_edge = found_source_edge || edge.kind == mirakana::AssetDependencyKind::mavg_source_mesh;
        found_material_edge = found_material_edge || edge.kind == mirakana::AssetDependencyKind::mavg_material;
    }
    MK_REQUIRE(found_source_edge);
    MK_REQUIRE(found_material_edge);

    const auto serialized = mirakana::serialize_asset_cooked_package_index(index);
    MK_REQUIRE(serialized.find("kind=mavg_cluster_graph\n") != std::string::npos);
    MK_REQUIRE(serialized.find("kind=mavg_source_mesh\n") != std::string::npos);
    const auto parsed = mirakana::deserialize_asset_cooked_package_index(serialized);
    found_mavg_entry = false;
    for (const auto& entry : parsed.entries) {
        found_mavg_entry = found_mavg_entry || entry.kind == mirakana::AssetKind::mavg_cluster_graph;
    }
    found_source_edge = false;
    for (const auto& edge : parsed.dependencies) {
        found_source_edge = found_source_edge || edge.kind == mirakana::AssetDependencyKind::mavg_source_mesh;
    }
    MK_REQUIRE(found_mavg_entry);
    MK_REQUIRE(found_source_edge);
}

int main() {
    return mirakana::test::run_all();
}
