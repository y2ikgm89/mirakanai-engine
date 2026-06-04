// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/platform/filesystem.hpp"
#include "mirakana/tools/mavg_cluster_cook.hpp"

#include <string>

namespace {

[[nodiscard]] mirakana::MavgClusterCookRequest make_request() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/cathedral_cluster_graph");
    const auto source_mesh = mirakana::AssetId::from_name("source/cathedral_high.glb");
    const auto material_stone = mirakana::AssetId::from_name("materials/stone");
    const auto material_glass = mirakana::AssetId::from_name("materials/glass");

    mirakana::MavgClusterCookRequest request{
        .graph_asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/meshes/cathedral_high.glb",
        .graph_output_path = "runtime/mavg/cathedral_cluster_graph.mavg",
        .payload_output_path = "runtime/mavg/cathedral_cluster_pages.mavgpayload",
        .package_index_path = "runtime/package.index",
        .source_revision = 9U,
        .target_cluster_triangles = 2U,
        .page_size_bytes = 128U,
        .triangles =
            {
                mirakana::MavgClusterCookTriangle{
                    .bounds = mirakana::MavgBounds3f{.min = {0.0F, 0.0F, 0.0F}, .max = {1.0F, 1.0F, 1.0F}},
                },
                mirakana::MavgClusterCookTriangle{
                    .bounds = mirakana::MavgBounds3f{.min = {1.0F, 0.0F, 0.0F}, .max = {2.0F, 1.0F, 1.0F}},
                },
                mirakana::MavgClusterCookTriangle{
                    .bounds = mirakana::MavgBounds3f{.min = {2.0F, 0.0F, 0.0F}, .max = {3.0F, 1.0F, 1.0F}},
                },
            },
        .material_partitions =
            {
                mirakana::MavgClusterCookMaterialPartition{
                    .material = material_stone,
                    .first_triangle = 0U,
                    .triangle_count = 2U,
                },
                mirakana::MavgClusterCookMaterialPartition{
                    .material = material_glass,
                    .first_triangle = 2U,
                    .triangle_count = 1U,
                },
            },
        .package_rows =
            {
                mirakana::MavgClusterCookPackageRow{
                    .asset = source_mesh,
                    .kind = mirakana::AssetKind::mesh,
                    .path = "runtime/source/cathedral_high.mesh",
                    .content = "format=GameEngine.CookedMesh.v1\n",
                    .source_revision = 9U,
                },
                mirakana::MavgClusterCookPackageRow{
                    .asset = material_stone,
                    .kind = mirakana::AssetKind::material,
                    .path = "runtime/materials/stone.material",
                    .content = "format=GameEngine.Material.v1\n",
                    .source_revision = 9U,
                },
                mirakana::MavgClusterCookPackageRow{
                    .asset = material_glass,
                    .kind = mirakana::AssetKind::material,
                    .path = "runtime/materials/glass.material",
                    .content = "format=GameEngine.Material.v1\n",
                    .source_revision = 9U,
                },
            },
    };
    return request;
}

} // namespace

MK_TEST("mavg cluster cook planner produces deterministic graph payload and package rows") {
    const auto result = mirakana::plan_mavg_cluster_graph_cook_package(make_request());
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.graph.clusters.size() == 2U);
    MK_REQUIRE(result.graph.pages.size() == 1U);
    MK_REQUIRE(result.graph.material_partitions.size() == 2U);
    MK_REQUIRE(result.graph_content.find("format=GameEngine.MavgClusterGraph.v1\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("format=GameEngine.MavgClusterPayload.v1\n") != std::string::npos);
    MK_REQUIRE(result.package_index_content.find("mavg_cluster_graph") != std::string::npos);
    MK_REQUIRE(result.changed_files.size() == 3U);

    bool found_source_edge = false;
    bool found_material_edge = false;
    for (const auto& edge : result.package_index.dependencies) {
        found_source_edge = found_source_edge || edge.kind == mirakana::AssetDependencyKind::mavg_source_mesh;
        found_material_edge = found_material_edge || edge.kind == mirakana::AssetDependencyKind::mavg_material;
    }
    MK_REQUIRE(found_source_edge);
    MK_REQUIRE(found_material_edge);
}

MK_TEST("mavg cluster cook apply writes all planned changed files") {
    mirakana::MemoryFileSystem filesystem;
    const auto result = mirakana::apply_mavg_cluster_graph_cook_package(filesystem, make_request());
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(filesystem.exists("runtime/mavg/cathedral_cluster_graph.mavg"));
    MK_REQUIRE(filesystem.exists("runtime/mavg/cathedral_cluster_pages.mavgpayload"));
    MK_REQUIRE(filesystem.exists("runtime/package.index"));
    MK_REQUIRE(filesystem.read_text("runtime/mavg/cathedral_cluster_graph.mavg") == result.graph_content);
    MK_REQUIRE(filesystem.read_text("runtime/package.index") == result.package_index_content);
}

MK_TEST("mavg cluster cook planner rejects empty triangle input") {
    auto request = make_request();
    request.triangles.clear();
    const auto result = mirakana::plan_mavg_cluster_graph_cook_package(request);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.changed_files.empty());
}

int main() {
    return mirakana::test::run_all();
}
