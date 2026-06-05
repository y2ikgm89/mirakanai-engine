// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/platform/filesystem.hpp"
#include "mirakana/tools/mavg_cluster_cook.hpp"

#include <bit>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace {

void append_le_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
}

void append_le_f32(std::vector<std::uint8_t>& bytes, float value) {
    append_le_u32(bytes, std::bit_cast<std::uint32_t>(value));
}

void append_vertex(std::vector<std::uint8_t>& bytes, mirakana::MavgVec3f position, mirakana::MavgVec3f normal, float u,
                   float v) {
    append_le_f32(bytes, position.x);
    append_le_f32(bytes, position.y);
    append_le_f32(bytes, position.z);
    append_le_f32(bytes, normal.x);
    append_le_f32(bytes, normal.y);
    append_le_f32(bytes, normal.z);
    append_le_f32(bytes, u);
    append_le_f32(bytes, v);
}

[[nodiscard]] std::string hex_encode(const std::vector<std::uint8_t>& bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(bytes.size() * 2U);
    for (const auto byte : bytes) {
        encoded.push_back(digits[(byte >> 4U) & 0x0fU]);
        encoded.push_back(digits[byte & 0x0fU]);
    }
    return encoded;
}

[[nodiscard]] std::string expected_vertex_data_hex() {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(9U * 32U);
    const auto normal = mirakana::MavgVec3f{.x = 0.0F, .y = 0.0F, .z = 1.0F};
    append_vertex(bytes, {.x = 0.0F, .y = 0.0F, .z = 0.0F}, normal, 0.0F, 0.0F);
    append_vertex(bytes, {.x = 1.0F, .y = 0.0F, .z = 0.0F}, normal, 1.0F, 0.0F);
    append_vertex(bytes, {.x = 0.0F, .y = 1.0F, .z = 0.0F}, normal, 0.0F, 1.0F);
    append_vertex(bytes, {.x = 1.0F, .y = 0.0F, .z = 0.0F}, normal, 0.0F, 0.0F);
    append_vertex(bytes, {.x = 2.0F, .y = 0.0F, .z = 0.0F}, normal, 1.0F, 0.0F);
    append_vertex(bytes, {.x = 1.0F, .y = 1.0F, .z = 0.0F}, normal, 0.0F, 1.0F);
    append_vertex(bytes, {.x = 2.0F, .y = 0.0F, .z = 0.0F}, normal, 0.0F, 0.0F);
    append_vertex(bytes, {.x = 3.0F, .y = 0.0F, .z = 0.0F}, normal, 1.0F, 0.0F);
    append_vertex(bytes, {.x = 2.0F, .y = 1.0F, .z = 0.0F}, normal, 0.0F, 1.0F);
    return hex_encode(bytes);
}

[[nodiscard]] std::string expected_index_data_hex() {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(9U * 4U);
    for (std::uint32_t index = 0; index < 9U; ++index) {
        append_le_u32(bytes, index);
    }
    return hex_encode(bytes);
}

[[nodiscard]] mirakana::MavgClusterCookRequest make_request() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/cathedral_cluster_graph");
    const auto source_mesh = mirakana::AssetId::from_name("source/cathedral_high.glb");
    const auto material_stone = mirakana::AssetId::from_name("materials/stone");
    const auto material_glass = mirakana::AssetId::from_name("materials/glass");
    const auto normal = mirakana::MavgVec3f{.x = 0.0F, .y = 0.0F, .z = 1.0F};

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
        .vertices =
            {
                mirakana::MavgClusterCookVertex{
                    .position = {.x = 0.0F, .y = 0.0F, .z = 0.0F},
                    .normal = normal,
                    .u = 0.0F,
                    .v = 0.0F,
                },
                mirakana::MavgClusterCookVertex{
                    .position = {.x = 1.0F, .y = 0.0F, .z = 0.0F},
                    .normal = normal,
                    .u = 1.0F,
                    .v = 0.0F,
                },
                mirakana::MavgClusterCookVertex{
                    .position = {.x = 0.0F, .y = 1.0F, .z = 0.0F},
                    .normal = normal,
                    .u = 0.0F,
                    .v = 1.0F,
                },
                mirakana::MavgClusterCookVertex{
                    .position = {.x = 1.0F, .y = 0.0F, .z = 0.0F},
                    .normal = normal,
                    .u = 0.0F,
                    .v = 0.0F,
                },
                mirakana::MavgClusterCookVertex{
                    .position = {.x = 2.0F, .y = 0.0F, .z = 0.0F},
                    .normal = normal,
                    .u = 1.0F,
                    .v = 0.0F,
                },
                mirakana::MavgClusterCookVertex{
                    .position = {.x = 1.0F, .y = 1.0F, .z = 0.0F},
                    .normal = normal,
                    .u = 0.0F,
                    .v = 1.0F,
                },
                mirakana::MavgClusterCookVertex{
                    .position = {.x = 2.0F, .y = 0.0F, .z = 0.0F},
                    .normal = normal,
                    .u = 0.0F,
                    .v = 0.0F,
                },
                mirakana::MavgClusterCookVertex{
                    .position = {.x = 3.0F, .y = 0.0F, .z = 0.0F},
                    .normal = normal,
                    .u = 1.0F,
                    .v = 0.0F,
                },
                mirakana::MavgClusterCookVertex{
                    .position = {.x = 2.0F, .y = 1.0F, .z = 0.0F},
                    .normal = normal,
                    .u = 0.0F,
                    .v = 1.0F,
                },
            },
        .triangles =
            {
                mirakana::MavgClusterCookTriangle{
                    .i0 = 0U,
                    .i1 = 1U,
                    .i2 = 2U,
                    .bounds =
                        mirakana::MavgBounds3f{
                            .min = {.x = 0.0F, .y = 0.0F, .z = 0.0F},
                            .max = {.x = 1.0F, .y = 1.0F, .z = 0.0F},
                        },
                },
                mirakana::MavgClusterCookTriangle{
                    .i0 = 3U,
                    .i1 = 4U,
                    .i2 = 5U,
                    .bounds =
                        mirakana::MavgBounds3f{
                            .min = {.x = 1.0F, .y = 0.0F, .z = 0.0F},
                            .max = {.x = 2.0F, .y = 1.0F, .z = 0.0F},
                        },
                },
                mirakana::MavgClusterCookTriangle{
                    .i0 = 6U,
                    .i1 = 7U,
                    .i2 = 8U,
                    .bounds =
                        mirakana::MavgBounds3f{
                            .min = {.x = 2.0F, .y = 0.0F, .z = 0.0F},
                            .max = {.x = 3.0F, .y = 1.0F, .z = 0.0F},
                        },
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
    MK_REQUIRE(mirakana::validate_mavg_cluster_graph(result.graph).valid());
    MK_REQUIRE(result.graph.clusters.size() == 4U);
    MK_REQUIRE(result.graph.pages.size() == 2U);
    MK_REQUIRE(result.graph.material_partitions.size() == 2U);
    MK_REQUIRE(result.graph.clusters[0].resident_fallback_cluster_index == 0U);
    MK_REQUIRE(!result.graph.clusters[0].has_parent);
    MK_REQUIRE(result.graph.clusters[0].children.size() == 1U);
    MK_REQUIRE(result.graph.clusters[0].children[0] == 1U);
    MK_REQUIRE(result.graph.clusters[0].first_index == 0U);
    MK_REQUIRE(result.graph.clusters[0].index_count == 6U);
    MK_REQUIRE(result.graph.clusters[0].vertex_base == 0);
    MK_REQUIRE(result.graph.clusters[0].geometric_error > result.graph.clusters[1].geometric_error);
    MK_REQUIRE(result.graph.clusters[1].has_parent);
    MK_REQUIRE(result.graph.clusters[1].parent_cluster_index == 0U);
    MK_REQUIRE(result.graph.clusters[1].resident_fallback_cluster_index == 0U);
    MK_REQUIRE(result.graph.clusters[1].first_index == 0U);
    MK_REQUIRE(result.graph.clusters[1].index_count == 6U);
    MK_REQUIRE(result.graph.clusters[1].vertex_base == 0);
    MK_REQUIRE(result.graph.clusters[2].resident_fallback_cluster_index == 2U);
    MK_REQUIRE(result.graph.clusters[2].first_index == 6U);
    MK_REQUIRE(result.graph.clusters[2].index_count == 3U);
    MK_REQUIRE(result.graph.clusters[2].vertex_base == 0);
    MK_REQUIRE(result.graph.clusters[3].has_parent);
    MK_REQUIRE(result.graph.clusters[3].parent_cluster_index == 2U);
    MK_REQUIRE(result.graph.clusters[3].resident_fallback_cluster_index == 2U);
    MK_REQUIRE(result.graph.clusters[3].first_index == 6U);
    MK_REQUIRE(result.graph.clusters[3].index_count == 3U);
    MK_REQUIRE(result.graph.clusters[3].vertex_base == 0);
    MK_REQUIRE(result.graph_content.find("format=GameEngine.MavgClusterGraph.v1\n") != std::string::npos);
    MK_REQUIRE(result.graph_content.find("cluster.0.resident_fallback=0\n") != std::string::npos);
    MK_REQUIRE(result.graph_content.find("cluster.1.parent=0\n") != std::string::npos);
    MK_REQUIRE(result.graph_content.find("cluster.3.parent=2\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("format=GameEngine.MavgClusterPayload.v1\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("vertex.count=9\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("vertex.stride_bytes=32\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("vertex.data_hex=" + expected_vertex_data_hex() + "\n") !=
               std::string::npos);
    MK_REQUIRE(result.payload_content.find("index.count=9\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("index.format=uint32\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("index.data_hex=" + expected_index_data_hex() + "\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.count=2\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.data_hex=") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.0.index=0\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.0.byte_offset=0\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.0.byte_size=128\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.0.first_cluster=0\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.0.cluster_count=2\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.1.index=1\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.1.byte_offset=128\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.1.byte_size=128\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.1.first_cluster=2\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("page.1.cluster_count=2\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("cluster.0.page=0\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("cluster.1.page=0\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("cluster.2.page=1\n") != std::string::npos);
    MK_REQUIRE(result.payload_content.find("cluster.3.page=1\n") != std::string::npos);
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

MK_TEST("mavg cluster cook planner rejects invalid static payload inputs") {
    auto missing_vertices = make_request();
    missing_vertices.vertices.clear();
    const auto missing_vertex_result = mirakana::plan_mavg_cluster_graph_cook_package(missing_vertices);
    MK_REQUIRE(!missing_vertex_result.succeeded());
    MK_REQUIRE(missing_vertex_result.changed_files.empty());

    auto invalid_index = make_request();
    invalid_index.triangles[1].i2 = 99U;
    const auto invalid_index_result = mirakana::plan_mavg_cluster_graph_cook_package(invalid_index);
    MK_REQUIRE(!invalid_index_result.succeeded());
    MK_REQUIRE(invalid_index_result.changed_files.empty());

    auto invalid_material_range = make_request();
    invalid_material_range.material_partitions[1].first_triangle = 3U;
    const auto invalid_material_result = mirakana::plan_mavg_cluster_graph_cook_package(invalid_material_range);
    MK_REQUIRE(!invalid_material_result.succeeded());
    MK_REQUIRE(invalid_material_result.changed_files.empty());
}

int main() {
    return mirakana::test::run_all();
}
