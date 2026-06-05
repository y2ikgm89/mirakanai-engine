// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/assets/mavg_cluster_payload.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

[[nodiscard]] std::string repeated_hex_byte(std::uint8_t value, std::size_t count) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(count * 2U);
    for (std::size_t index = 0; index < count; ++index) {
        result.push_back(digits[(value >> 4U) & 0x0fU]);
        result.push_back(digits[value & 0x0fU]);
    }
    return result;
}

[[nodiscard]] mirakana::MavgClusterGraphDocument make_payload_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/page-addressable");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/page-addressable-source");
    const auto material = mirakana::AssetId::from_name("materials/page-addressable-material");
    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/page-addressable.glb",
        .cluster_payload_uri = "runtime/mavg/page-addressable.mavgpayload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 64,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 64, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 64, .byte_size = 64, .first_cluster = 1, .cluster_count = 1},
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material, .first_cluster = 0, .cluster_count = 2},
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
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = -1.0F, .y = 0.0F, .z = 0.0F},
                                                     .max = mirakana::MavgVec3f{.x = 0.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 6,
                    .index_count = 3,
                    .vertex_base = 4,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] mirakana::MavgClusterPayloadDocument make_payload_document() {
    const auto graph = make_payload_graph();
    return mirakana::MavgClusterPayloadDocument{
        .asset = graph.asset,
        .vertex_count = 4,
        .vertex_stride_bytes = 32,
        .vertex_data_hex = repeated_hex_byte(0x11U, 4U * 32U),
        .index_count = 9,
        .index_format = "uint32",
        .index_data_hex = repeated_hex_byte(0x22U, 9U * 4U),
        .page_data_hex = repeated_hex_byte(0x30U, 64U) + repeated_hex_byte(0x40U, 64U),
        .pages =
            {
                mirakana::MavgClusterPayloadPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 64, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterPayloadPage{
                    .page_index = 1, .byte_offset = 64, .byte_size = 64, .first_cluster = 1, .cluster_count = 1},
            },
        .clusters =
            {
                mirakana::MavgClusterPayloadCluster{
                    .cluster_index = 0, .page_index = 0, .first_index = 0, .index_count = 6, .vertex_base = 0},
                mirakana::MavgClusterPayloadCluster{
                    .cluster_index = 1, .page_index = 1, .first_index = 6, .index_count = 3, .vertex_base = 4},
            },
    };
}

[[nodiscard]] bool has_diagnostic(const std::vector<mirakana::MavgClusterPayloadDiagnostic>& diagnostics,
                                  mirakana::MavgClusterPayloadDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("mavg cluster payload validates page addressable byte ranges against graph") {
    const auto graph = make_payload_graph();
    const auto payload = make_payload_document();

    const auto validation = mirakana::validate_mavg_cluster_payload(payload, graph);
    MK_REQUIRE(validation.valid());
    MK_REQUIRE(mirakana::mavg_cluster_payload_format_v1() == "GameEngine.MavgClusterPayload.v1");

    const auto serialized = mirakana::serialize_mavg_cluster_payload_document(payload);
    MK_REQUIRE(serialized.contains("format=GameEngine.MavgClusterPayload.v1\n"));
    MK_REQUIRE(serialized.contains("page.data_hex=" + payload.page_data_hex + "\n"));
    MK_REQUIRE(serialized.contains("page.0.byte_offset=0\n"));
    MK_REQUIRE(serialized.contains("page.1.byte_offset=64\n"));
    MK_REQUIRE(serialized.contains("cluster.1.page=1\n"));

    const auto parsed = mirakana::deserialize_mavg_cluster_payload_document(serialized);
    MK_REQUIRE(parsed.asset == graph.asset);
    MK_REQUIRE(parsed.pages.size() == 2U);
    MK_REQUIRE(parsed.pages[1].byte_offset == 64U);
    MK_REQUIRE(parsed.clusters[1].page_index == 1U);
    MK_REQUIRE(mirakana::validate_mavg_cluster_payload(parsed, graph).valid());
}

MK_TEST("mavg cluster payload rejects duplicate overlapping and out of range pages") {
    const auto graph = make_payload_graph();
    auto payload = make_payload_document();
    payload.pages[1].page_index = payload.pages[0].page_index;
    payload.pages[1].byte_offset = 32;
    payload.pages[1].byte_size = 128;

    const auto validation = mirakana::validate_mavg_cluster_payload(payload, graph);
    MK_REQUIRE(!validation.valid());
    MK_REQUIRE(
        has_diagnostic(validation.diagnostics, mirakana::MavgClusterPayloadDiagnosticCode::duplicate_page_index));
    MK_REQUIRE(
        has_diagnostic(validation.diagnostics, mirakana::MavgClusterPayloadDiagnosticCode::overlapping_page_range));
    MK_REQUIRE(has_diagnostic(validation.diagnostics, mirakana::MavgClusterPayloadDiagnosticCode::invalid_page_range));
}

MK_TEST("mavg cluster payload rejects cluster page and draw range mismatches") {
    const auto graph = make_payload_graph();
    auto payload = make_payload_document();
    payload.clusters[1].page_index = 0;
    payload.clusters[1].first_index = 99;

    const auto validation = mirakana::validate_mavg_cluster_payload(payload, graph);
    MK_REQUIRE(!validation.valid());
    MK_REQUIRE(
        has_diagnostic(validation.diagnostics, mirakana::MavgClusterPayloadDiagnosticCode::cluster_page_mismatch));
    MK_REQUIRE(has_diagnostic(validation.diagnostics,
                              mirakana::MavgClusterPayloadDiagnosticCode::cluster_draw_range_mismatch));
}

int main() {
    return mirakana::test::run_all();
}
