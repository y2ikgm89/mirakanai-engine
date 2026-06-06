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
                    .cluster_count = 1,
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
                    .bounds = mirakana::MavgBounds3f{.min = {-1.0F, -2.0F, -3.0F}, .max = {1.0F, 2.0F, 3.0F}},
                    .material_partition = 0,
                    .children = {},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 0,
                    .page_index = 0,
                    .local_cluster_index = 0,
                    .lod_level = 0,
                    .triangle_count = 128,
                    .vertex_count = 192,
                    .bounds = mirakana::MavgBounds3f{.min = {-2.0F, -4.0F, -6.0F}, .max = {2.0F, 4.0F, 6.0F}},
                    .material_partition = 1,
                    .children = {1},
                },
            },
    };
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
    MK_REQUIRE(canonical.pages.size() == 2U);
    MK_REQUIRE(canonical.pages[0].page_index == 0U);
    MK_REQUIRE(canonical.pages[1].page_index == 1U);
    MK_REQUIRE(canonical.material_partitions.size() == 2U);
    MK_REQUIRE(canonical.material_partitions[0].first_cluster == 0U);
    MK_REQUIRE(canonical.material_partitions[1].first_cluster == 1U);
    MK_REQUIRE(canonical.clusters.size() == 2U);
    MK_REQUIRE(canonical.clusters[0].cluster_index == 0U);
    MK_REQUIRE(canonical.clusters[0].children.size() == 1U);
    MK_REQUIRE(canonical.clusters[0].children[0] == 1U);

    const auto serialized = mirakana::serialize_mavg_cluster_graph_document(canonical);
    MK_REQUIRE(serialized.find("format=GameEngine.MavgClusterGraph.v1\n") != std::string::npos);
    MK_REQUIRE(serialized.find("asset.kind=mavg_cluster_graph\n") != std::string::npos);
    MK_REQUIRE(serialized.find("cluster.0.children=1\n") != std::string::npos);

    const auto parsed = mirakana::deserialize_mavg_cluster_graph_document(serialized);
    MK_REQUIRE(parsed.pages[0].page_index == 0U);
    MK_REQUIRE(parsed.material_partitions[0].first_cluster == 0U);
    MK_REQUIRE(parsed.clusters[0].children[0] == 1U);
}

MK_TEST("mavg cluster graph rejects duplicate pages and unknown child references") {
    auto document = make_valid_document();
    document.pages[1].page_index = document.pages[0].page_index;
    document.clusters[0].children.push_back(99U);

    const auto validation = mirakana::validate_mavg_cluster_graph(document);
    MK_REQUIRE(validation.diagnostics.size() >= 2U);

    bool found_duplicate_page = false;
    bool found_unknown_child = false;
    for (const auto& diagnostic : validation.diagnostics) {
        found_duplicate_page =
            found_duplicate_page || diagnostic.code == mirakana::MavgClusterGraphDiagnosticCode::duplicate_page_index;
        found_unknown_child =
            found_unknown_child || diagnostic.code == mirakana::MavgClusterGraphDiagnosticCode::unknown_child_cluster;
    }
    MK_REQUIRE(found_duplicate_page);
    MK_REQUIRE(found_unknown_child);
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
