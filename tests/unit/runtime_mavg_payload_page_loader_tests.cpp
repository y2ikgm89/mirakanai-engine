// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/mavg_payload_page_loader.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] std::span<const std::byte> bytes_view(const std::string& text) noexcept {
    return std::as_bytes(std::span<const char>(text.data(), text.size()));
}

[[nodiscard]] std::vector<std::byte> byte_copy(std::string_view text) {
    const auto bytes = std::as_bytes(std::span<const char>(text.data(), text.size()));
    return std::vector<std::byte>(bytes.begin(), bytes.end());
}

[[nodiscard]] mirakana::MavgClusterGraphDocument make_graph(const std::string& payload) {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/cathedral_cluster_graph");
    const auto source_mesh = mirakana::AssetId::from_name("source/cathedral_high");
    const auto material = mirakana::AssetId::from_name("materials/stone");
    const auto page0 = payload.find("page0-cluster-bytes");
    const auto page1 = payload.find("page1-cluster-bytes");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/meshes/cathedral_high.mesh",
        .cluster_payload_uri = "runtime/mavg/cathedral_cluster_pages.mavgpayload",
        .target_cluster_triangles = 64,
        .page_size_bytes = 64,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0,
                    .byte_offset = static_cast<std::uint64_t>(page0),
                    .byte_size = 19,
                    .first_cluster = 0,
                    .cluster_count = 1,
                },
                mirakana::MavgClusterGraphPage{
                    .page_index = 1,
                    .byte_offset = static_cast<std::uint64_t>(page1),
                    .byte_size = 19,
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
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds = {.min = {.x = -1.0F, .y = -1.0F, .z = -1.0F}, .max = {.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 4.0F,
                    .first_index = 0,
                    .index_count = 192,
                    .vertex_base = 0,
                    .children = {1},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 32,
                    .vertex_count = 48,
                    .bounds = {.min = {.x = 0.0F, .y = 0.0F, .z = 0.0F}, .max = {.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 192,
                    .index_count = 96,
                    .vertex_base = 96,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] bool
has_diagnostic_code(const std::vector<mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnostic>& diagnostics,
                    mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime mavg payload page loader copies requested byte ranges without side effects") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    const auto graph = make_graph(payload);
    const std::array<std::uint32_t, 2> requested_pages{1, 0};

    const auto result =
        mirakana::runtime::load_runtime_mavg_payload_pages(mirakana::runtime::RuntimeMavgPayloadPageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .payload_bytes = bytes_view(payload),
            .page_indices = requested_pages,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.loaded_pages.size() == 2);
    MK_REQUIRE(result.loaded_pages[0].page_index == 1);
    MK_REQUIRE(result.loaded_pages[0].bytes == byte_copy("page1-cluster-bytes"));
    MK_REQUIRE(result.loaded_pages[1].page_index == 0);
    MK_REQUIRE(result.loaded_pages[1].bytes == byte_copy("page0-cluster-bytes"));
    MK_REQUIRE(result.requested_page_count == 2);
    MK_REQUIRE(result.loaded_page_count == 2);
    MK_REQUIRE(result.payload_byte_count == payload.size());
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.executed_direct_storage);
    MK_REQUIRE(!result.touched_gpu_memory_policy);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg payload page loader rejects duplicate missing and out of bounds pages") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    auto graph = make_graph(payload);
    graph.pages[1].byte_offset = static_cast<std::uint64_t>(payload.size() + 1U);
    const std::array<std::uint32_t, 3> requested_pages{0, 0, 1};

    const auto result =
        mirakana::runtime::load_runtime_mavg_payload_pages(mirakana::runtime::RuntimeMavgPayloadPageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .payload_bytes = bytes_view(payload),
            .page_indices = requested_pages,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.loaded_pages.empty());
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::duplicate_page_request));
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::page_range_out_of_bounds));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg payload page loader rejects invalid payload format") {
    const std::string payload = "format=GameEngine.UnknownPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    const auto graph = make_graph(payload);
    const std::array<std::uint32_t, 1> requested_pages{0};

    const auto result =
        mirakana::runtime::load_runtime_mavg_payload_pages(mirakana::runtime::RuntimeMavgPayloadPageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .payload_bytes = bytes_view(payload),
            .page_indices = requested_pages,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.loaded_pages.empty());
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_payload_format));
}

int main() {
    return mirakana::test::run_all();
}
