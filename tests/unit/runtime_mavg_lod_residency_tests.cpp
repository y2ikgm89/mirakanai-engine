// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/mavg_lod_residency.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_graph_document() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/resident_cluster_graph");
    const auto source_mesh = mirakana::AssetId::from_name("source/resident_high.glb");
    const auto material = mirakana::AssetId::from_name("materials/resident_stone");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/meshes/resident_high.glb",
        .cluster_payload_uri = "runtime/mavg/resident_cluster_pages.mavgpayload",
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
                    .page_index = 0,
                    .byte_offset = 0,
                    .byte_size = 512,
                    .first_cluster = 0,
                    .cluster_count = 1,
                },
                mirakana::MavgClusterGraphPage{
                    .page_index = 1,
                    .byte_offset = 512,
                    .byte_size = 256,
                    .first_cluster = 1,
                    .cluster_count = 1,
                },
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material,
                    .first_cluster = 0,
                    .cluster_count = 3,
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
                    .bounds =
                        mirakana::MavgBounds3f{
                            .min = {.x = -2.0F, .y = -2.0F, .z = -2.0F},
                            .max = {.x = 2.0F, .y = 2.0F, .z = 2.0F},
                        },
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 8.0F,
                    .first_index = 0,
                    .index_count = 384,
                    .vertex_base = 0,
                    .children = {1, 2},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds =
                        mirakana::MavgBounds3f{
                            .min = {.x = -2.0F, .y = -2.0F, .z = -2.0F},
                            .max = {.x = 0.0F, .y = 2.0F, .z = 2.0F},
                        },
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 0.0F,
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
                            .min = {.x = 0.0F, .y = -2.0F, .z = -2.0F},
                            .max = {.x = 2.0F, .y = 2.0F, .z = 2.0F},
                        },
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 0.0F,
                    .first_index = 576,
                    .index_count = 192,
                    .vertex_base = 288,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_graph_package(const mirakana::MavgClusterGraphDocument& graph) {
    return mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
        .asset = graph.asset,
        .kind = mirakana::AssetKind::mavg_cluster_graph,
        .path = "runtime/mavg/resident_cluster_graph.mavg",
        .content_hash = graph.asset.value + 1U,
        .source_revision = 1,
        .dependencies = {graph.source_mesh},
        .content = mirakana::serialize_mavg_cluster_graph_document(graph),
    }});
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_non_graph_package_for_asset(mirakana::AssetId asset) {
    return mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
        .asset = asset,
        .kind = mirakana::AssetKind::texture,
        .path = "runtime/textures/not_a_graph.texture",
        .content_hash = asset.value + 2U,
        .source_revision = 1,
        .content = "texture payload",
    }});
}

[[nodiscard]] mirakana::runtime::RuntimeResourceCatalogV2
make_catalog(const mirakana::MavgClusterGraphDocument& graph) {
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    const auto build = mirakana::runtime::build_runtime_resource_catalog_v2(catalog, make_graph_package(graph));
    MK_REQUIRE(build.succeeded());
    return catalog;
}

[[nodiscard]] bool has_diagnostic(const std::vector<std::string>& diagnostics, std::string_view needle) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.contains(needle)) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime mavg lod residency maps resident graph package records to page ids") {
    const auto graph = make_graph_document();
    const auto catalog = make_catalog(graph);

    const auto result = mirakana::runtime::build_runtime_mavg_lod_residency(
        mirakana::runtime::RuntimeMavgLodResidencyDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .catalog = &catalog,
        },
        {});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.resident_pages.page_indices.size() == 3U);
    MK_REQUIRE(result.resident_pages.page_indices[0] == 0U);
    MK_REQUIRE(result.resident_pages.page_indices[1] == 1U);
    MK_REQUIRE(result.resident_pages.page_indices[2] == 2U);
    MK_REQUIRE(result.reviewed_page_requests.empty());
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime mavg lod residency reports missing catalog evidence") {
    const auto graph = make_graph_document();
    const mirakana::runtime::RuntimeResourceCatalogV2 empty_catalog;

    const auto result = mirakana::runtime::build_runtime_mavg_lod_residency(
        mirakana::runtime::RuntimeMavgLodResidencyDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .catalog = &empty_catalog,
        },
        {});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.resident_pages.page_indices.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, "missing-catalog"));
}

MK_TEST("runtime mavg lod residency rejects non graph catalog evidence") {
    const auto graph = make_graph_document();
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    const auto build =
        mirakana::runtime::build_runtime_resource_catalog_v2(catalog, make_non_graph_package_for_asset(graph.asset));
    MK_REQUIRE(build.succeeded());

    const auto result = mirakana::runtime::build_runtime_mavg_lod_residency(
        mirakana::runtime::RuntimeMavgLodResidencyDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .catalog = &catalog,
        },
        {});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.resident_pages.page_indices.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, "invalid-catalog-kind"));
}

MK_TEST("runtime mavg lod residency rejects invalid graph documents before marking pages resident") {
    auto graph = make_graph_document();
    const auto catalog = make_catalog(graph);
    graph.pages[1].page_index = graph.pages[0].page_index;

    const auto result = mirakana::runtime::build_runtime_mavg_lod_residency(
        mirakana::runtime::RuntimeMavgLodResidencyDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .catalog = &catalog,
        },
        {});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.resident_pages.page_indices.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, "invalid-graph"));
}

MK_TEST("runtime mavg lod residency reports invalid descriptor inputs") {
    const auto result =
        mirakana::runtime::build_runtime_mavg_lod_residency(mirakana::runtime::RuntimeMavgLodResidencyDesc{}, {});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.resident_pages.page_indices.empty());
    MK_REQUIRE(result.reviewed_page_requests.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, "invalid-graph-asset"));
    MK_REQUIRE(has_diagnostic(result.diagnostics, "missing-graph"));
    MK_REQUIRE(has_diagnostic(result.diagnostics, "missing-catalog"));
}

MK_TEST("runtime mavg lod residency reports graph asset mismatch") {
    const auto graph = make_graph_document();
    const auto catalog = make_catalog(graph);
    const auto other_graph_asset = mirakana::AssetId::from_name("mavg/other_cluster_graph");

    const auto result = mirakana::runtime::build_runtime_mavg_lod_residency(
        mirakana::runtime::RuntimeMavgLodResidencyDesc{
            .graph_asset = other_graph_asset,
            .graph = &graph,
            .catalog = &catalog,
        },
        {});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.resident_pages.page_indices.empty());
    MK_REQUIRE(result.reviewed_page_requests.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, "graph-asset-mismatch"));
}

MK_TEST("runtime mavg lod residency preserves reviewed page requests") {
    const auto graph = make_graph_document();
    const auto catalog = make_catalog(graph);
    const std::vector<mirakana::MavgLodPageRequest> requests{
        mirakana::MavgLodPageRequest{
            .graph_asset = graph.asset,
            .page_index = 2,
            .priority = 9.0F,
            .reason = "selector-missing-page",
        },
        mirakana::MavgLodPageRequest{
            .graph_asset = graph.asset,
            .page_index = 1,
            .priority = 4.0F,
            .reason = "budget-prefetch",
        },
    };

    const auto result = mirakana::runtime::build_runtime_mavg_lod_residency(
        mirakana::runtime::RuntimeMavgLodResidencyDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .catalog = &catalog,
        },
        requests);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.reviewed_page_requests.size() == 2U);
    MK_REQUIRE(result.reviewed_page_requests[0].page_index == 2U);
    MK_REQUIRE(result.reviewed_page_requests[0].reason == "selector-missing-page");
    MK_REQUIRE(result.reviewed_page_requests[1].page_index == 1U);
    MK_REQUIRE(result.reviewed_page_requests[1].reason == "budget-prefetch");
}

MK_TEST("runtime mavg lod residency rejects reviewed page requests for another graph") {
    const auto graph = make_graph_document();
    const auto catalog = make_catalog(graph);
    const std::vector<mirakana::MavgLodPageRequest> requests{
        mirakana::MavgLodPageRequest{
            .graph_asset = mirakana::AssetId::from_name("mavg/other_cluster_graph"),
            .page_index = 2,
            .priority = 9.0F,
            .reason = "selector-missing-page",
        },
    };

    const auto result = mirakana::runtime::build_runtime_mavg_lod_residency(
        mirakana::runtime::RuntimeMavgLodResidencyDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .catalog = &catalog,
        },
        requests);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.resident_pages.page_indices.empty());
    MK_REQUIRE(result.reviewed_page_requests.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, "invalid-page-request"));
}

MK_TEST("runtime mavg lod residency rejects reviewed page requests outside the graph") {
    const auto graph = make_graph_document();
    const auto catalog = make_catalog(graph);
    const std::vector<mirakana::MavgLodPageRequest> requests{
        mirakana::MavgLodPageRequest{
            .graph_asset = graph.asset,
            .page_index = 99,
            .priority = 9.0F,
            .reason = "selector-missing-page",
        },
    };

    const auto result = mirakana::runtime::build_runtime_mavg_lod_residency(
        mirakana::runtime::RuntimeMavgLodResidencyDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .catalog = &catalog,
        },
        requests);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.resident_pages.page_indices.empty());
    MK_REQUIRE(result.reviewed_page_requests.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, "invalid-page-request"));
}

MK_TEST("runtime mavg lod residency is a pure evidence bridge without execution side effects") {
    const auto graph = make_graph_document();
    const auto catalog = make_catalog(graph);

    const auto result = mirakana::runtime::build_runtime_mavg_lod_residency(
        mirakana::runtime::RuntimeMavgLodResidencyDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .catalog = &catalog,
        },
        {});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_streaming);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

int main() {
    return mirakana::test::run_all();
}
