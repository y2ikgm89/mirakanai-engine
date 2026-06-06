// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_lod_selection.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_lod_document() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/selector_cluster_graph");
    const auto source_mesh = mirakana::AssetId::from_name("source/selector_high.glb");
    const auto material = mirakana::AssetId::from_name("materials/stone");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/meshes/selector_high.glb",
        .cluster_payload_uri = "runtime/mavg/selector_cluster_pages.mavgpayload",
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
                    .byte_size = 256,
                    .first_cluster = 1,
                    .cluster_count = 1,
                },
                mirakana::MavgClusterGraphPage{
                    .page_index = 2,
                    .byte_offset = 768,
                    .byte_size = 256,
                    .first_cluster = 2,
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
                    .children = {2, 1},
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

[[nodiscard]] mirakana::MavgClusterGraphDocument make_two_root_lod_document() {
    auto document = make_lod_document();
    document.pages.push_back(mirakana::MavgClusterGraphPage{
        .page_index = 3,
        .byte_offset = 1024,
        .byte_size = 256,
        .first_cluster = 3,
        .cluster_count = 1,
    });
    document.material_partitions.push_back(mirakana::MavgClusterGraphMaterialPartition{
        .material = mirakana::AssetId::from_name("materials/brick"),
        .first_cluster = 3,
        .cluster_count = 1,
    });
    document.clusters.push_back(mirakana::MavgClusterGraphCluster{
        .cluster_index = 3,
        .page_index = 3,
        .local_cluster_index = 0,
        .lod_level = 0,
        .triangle_count = 64,
        .vertex_count = 96,
        .bounds =
            mirakana::MavgBounds3f{
                .min = {.x = 10.0F, .y = -2.0F, .z = -2.0F},
                .max = {.x = 14.0F, .y = 2.0F, .z = 2.0F},
            },
        .material_partition = 1,
        .parent_cluster_index = 3,
        .has_parent = false,
        .resident_fallback_cluster_index = 3,
        .geometric_error = 2.0F,
        .first_index = 768,
        .index_count = 192,
        .vertex_base = 384,
        .children = {},
    });
    return document;
}

[[nodiscard]] mirakana::MavgLodViewDesc make_view(float camera_z) {
    return mirakana::MavgLodViewDesc{
        .camera_world_position = {.x = 0.0F, .y = 0.0F, .z = camera_z},
        .viewport_height_pixels = 100.0F,
        .target_error_pixels = 1.0F,
        .hysteresis_pixels = 0.25F,
        .max_selected_clusters = 16,
    };
}

[[nodiscard]] mirakana::MavgLodResidentPageSet resident_pages(std::vector<std::uint32_t> page_indices) {
    return mirakana::MavgLodResidentPageSet{.page_indices = std::move(page_indices)};
}

[[nodiscard]] bool has_diagnostic_code(const std::vector<mirakana::MavgLodSelectionDiagnostic>& diagnostics,
                                       mirakana::MavgLodSelectionDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const mirakana::MavgLodSelectionDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

} // namespace

MK_TEST("mavg lod selection chooses resident child clusters for near cameras") {
    const auto result =
        mirakana::select_mavg_lod_clusters(make_lod_document(), make_view(-10.0F), resident_pages({0, 1, 2}));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.selected_clusters.size() == 2U);
    MK_REQUIRE(result.selected_clusters[0].cluster_index == 1U);
    MK_REQUIRE(result.selected_clusters[0].lod_level == 1U);
    MK_REQUIRE(result.selected_clusters[0].first_index == 384U);
    MK_REQUIRE(result.selected_clusters[0].index_count == 192U);
    MK_REQUIRE(result.selected_clusters[0].vertex_base == 192);
    MK_REQUIRE(!result.selected_clusters[0].fallback_substitution);
    MK_REQUIRE(result.selected_clusters[1].cluster_index == 2U);
    MK_REQUIRE(result.page_requests.empty());
}

MK_TEST("mavg lod selection chooses the root fallback cluster for far cameras") {
    const auto result =
        mirakana::select_mavg_lod_clusters(make_lod_document(), make_view(-1000.0F), resident_pages({0, 1, 2}));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.selected_clusters.size() == 1U);
    MK_REQUIRE(result.selected_clusters[0].cluster_index == 0U);
    MK_REQUIRE(result.selected_clusters[0].lod_level == 0U);
    MK_REQUIRE(result.selected_clusters[0].first_index == 0U);
    MK_REQUIRE(result.selected_clusters[0].index_count == 384U);
    MK_REQUIRE(result.page_requests.empty());
}

MK_TEST("mavg lod selection substitutes resident fallbacks and requests missing child pages") {
    const auto result = mirakana::select_mavg_lod_clusters(make_lod_document(), make_view(-10.0F), resident_pages({0}));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.selected_clusters.size() == 1U);
    MK_REQUIRE(result.selected_clusters[0].cluster_index == 0U);
    MK_REQUIRE(result.selected_clusters[0].fallback_substitution);
    MK_REQUIRE(result.page_requests.size() == 2U);
    MK_REQUIRE(result.page_requests[0].page_index == 1U);
    MK_REQUIRE(result.page_requests[1].page_index == 2U);
    MK_REQUIRE(result.fallback_substitution_count == 2U);
    MK_REQUIRE(result.missing_page_count == 2U);
    MK_REQUIRE(has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::missing_page));
}

MK_TEST("mavg lod selection collapses mixed residency to one fallback cover") {
    const auto result =
        mirakana::select_mavg_lod_clusters(make_lod_document(), make_view(-10.0F), resident_pages({0, 2}));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.selected_clusters.size() == 1U);
    MK_REQUIRE(result.selected_clusters[0].cluster_index == 0U);
    MK_REQUIRE(result.selected_clusters[0].fallback_substitution);
    MK_REQUIRE(result.page_requests.size() == 1U);
    MK_REQUIRE(result.page_requests[0].page_index == 1U);
    MK_REQUIRE(result.fallback_substitution_count == 1U);
    MK_REQUIRE(result.missing_page_count == 1U);
    MK_REQUIRE(has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::missing_page));
}

MK_TEST("mavg lod selection reports missing resident fallback before selecting invalid fallback rows") {
    auto document = make_lod_document();
    document.clusters[1].resident_fallback_cluster_index = 99;

    const auto result = mirakana::select_mavg_lod_clusters(document, make_view(-10.0F), resident_pages({0}));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.selected_clusters.empty());
    MK_REQUIRE(
        has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::missing_resident_fallback));
    MK_REQUIRE(!has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::invalid_graph));
}

MK_TEST("mavg lod selection rejects invalid viewport height without selecting clusters") {
    auto view = make_view(-10.0F);
    view.viewport_height_pixels = 0.0F;

    const auto result = mirakana::select_mavg_lod_clusters(make_lod_document(), view, resident_pages({0, 1, 2}));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.selected_clusters.empty());
    MK_REQUIRE(has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::invalid_view));
}

MK_TEST("mavg lod selection rejects invalid target error without selecting clusters") {
    auto view = make_view(-10.0F);
    view.target_error_pixels = -1.0F;

    const auto result = mirakana::select_mavg_lod_clusters(make_lod_document(), view, resident_pages({0, 1, 2}));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.selected_clusters.empty());
    MK_REQUIRE(has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::invalid_view));
}

MK_TEST("mavg lod selection degrades over budget selections to deterministic fallback ancestors") {
    auto view = make_view(-10.0F);
    view.max_selected_clusters = 1;

    const auto result = mirakana::select_mavg_lod_clusters(make_lod_document(), view, resident_pages({0, 1, 2}));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.budget_degraded);
    MK_REQUIRE(result.selected_clusters.size() == 1U);
    MK_REQUIRE(result.selected_clusters[0].cluster_index == 0U);
    MK_REQUIRE(result.selected_clusters[0].lod_level == 0U);
    MK_REQUIRE(has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::budget_degraded));
}

MK_TEST("mavg lod selection reports unsatisfied budgets instead of returning over-budget success") {
    auto view = make_view(-10.0F);
    view.max_selected_clusters = 1;

    const auto result =
        mirakana::select_mavg_lod_clusters(make_two_root_lod_document(), view, resident_pages({0, 1, 2, 3}));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.selected_clusters.size() == 2U);
    MK_REQUIRE(result.selected_clusters[0].cluster_index == 0U);
    MK_REQUIRE(result.selected_clusters[1].cluster_index == 3U);
    MK_REQUIRE(result.budget_degraded);
    MK_REQUIRE(has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::budget_degraded));
    MK_REQUIRE(has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::budget_unsatisfied));
}

MK_TEST("mavg lod selection uses hysteresis to keep previous rows near the threshold") {
    auto view = make_view(-76.19048F);
    view.target_error_pixels = 10.0F;
    view.hysteresis_pixels = 1.0F;

    const auto result = mirakana::select_mavg_lod_clusters(make_lod_document(), view, resident_pages({0, 1, 2}),
                                                           mirakana::MavgLodPreviousSelection{
                                                               .cluster_indices = {0},
                                                           });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.selected_clusters.size() == 1U);
    MK_REQUIRE(result.selected_clusters[0].cluster_index == 0U);
    MK_REQUIRE(
        has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::hysteresis_reused_previous));
}

MK_TEST("mavg lod selection rejects invalid graphs without selecting clusters") {
    auto document = make_lod_document();
    document.clusters[1].page_index = 99;

    const auto result = mirakana::select_mavg_lod_clusters(document, make_view(-10.0F), resident_pages({0, 1, 2}));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.selected_clusters.empty());
    MK_REQUIRE(has_diagnostic_code(result.diagnostics, mirakana::MavgLodSelectionDiagnosticCode::invalid_graph));
}

int main() {
    return mirakana::test::run_all();
}
