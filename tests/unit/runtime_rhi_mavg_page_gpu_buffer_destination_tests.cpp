// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/assets/mavg_cluster_payload.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/mavg_payload_pages.hpp"
#include "mirakana/runtime_rhi/mavg_page_gpu_buffer_destination.hpp"

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

[[nodiscard]] mirakana::MavgClusterGraphDocument make_buffer_destination_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/page-gpu-buffer-destination");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/page-gpu-buffer-destination-source");
    const auto material = mirakana::AssetId::from_name("materials/page-gpu-buffer-destination-material");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/page-gpu-buffer-destination.gltf",
        .cluster_payload_uri = "runtime/page-gpu-buffer-destination.mavg_payload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 64,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 64, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 64, .byte_size = 64, .first_cluster = 1, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 2, .byte_offset = 128, .byte_size = 64, .first_cluster = 2, .cluster_count = 1},
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material, .first_cluster = 0, .cluster_count = 3},
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
                    .children = {1, 2},
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
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 2,
                    .page_index = 2,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 1,
                    .vertex_count = 3,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                                                     .max = mirakana::MavgVec3f{.x = 1.0F, .y = 0.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 9,
                    .index_count = 3,
                    .vertex_base = 7,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] std::string make_payload_text(const mirakana::MavgClusterGraphDocument& graph) {
    return mirakana::serialize_mavg_cluster_payload_document(mirakana::MavgClusterPayloadDocument{
        .asset = graph.asset,
        .vertex_count = 10,
        .vertex_stride_bytes = 32,
        .vertex_data_hex = repeated_hex_byte(0x11U, 10U * 32U),
        .index_count = 12,
        .index_format = "uint32",
        .index_data_hex = repeated_hex_byte(0x22U, 12U * 4U),
        .page_data_hex = repeated_hex_byte(0x30U, 64U) + repeated_hex_byte(0x40U, 64U) + repeated_hex_byte(0x50U, 64U),
        .pages =
            {
                mirakana::MavgClusterPayloadPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 64, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterPayloadPage{
                    .page_index = 1, .byte_offset = 64, .byte_size = 64, .first_cluster = 1, .cluster_count = 1},
                mirakana::MavgClusterPayloadPage{
                    .page_index = 2, .byte_offset = 128, .byte_size = 64, .first_cluster = 2, .cluster_count = 1},
            },
        .clusters =
            {
                mirakana::MavgClusterPayloadCluster{
                    .cluster_index = 0, .page_index = 0, .first_index = 0, .index_count = 6, .vertex_base = 0},
                mirakana::MavgClusterPayloadCluster{
                    .cluster_index = 1, .page_index = 1, .first_index = 6, .index_count = 3, .vertex_base = 4},
                mirakana::MavgClusterPayloadCluster{
                    .cluster_index = 2, .page_index = 2, .first_index = 9, .index_count = 3, .vertex_base = 7},
            },
    });
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanResult
make_request_plan(const mirakana::MavgClusterGraphDocument& graph, const std::string& payload_text) {
    const std::vector<std::uint32_t> page_indices{2, 0};
    return mirakana::runtime::plan_runtime_mavg_payload_directstorage_requests(
        mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanDesc{
            .graph = &graph,
            .payload_text = payload_text,
            .payload_blob_path = "runtime/page-gpu-buffer-destination.pages",
            .page_indices = page_indices,
            .destination_base_offset = 1024U,
            .fence_wait_point = mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_gpu_work,
            .synchronize_with_fence = true,
        });
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationPlanResult& result,
                                  mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime rhi mavg page buffer destination planner maps directstorage requests to caller owned rhi buffer") {
    const auto graph = make_buffer_destination_graph();
    const auto payload_text = make_payload_text(graph);
    const auto request_plan = make_request_plan(graph, payload_text);
    MK_REQUIRE(request_plan.succeeded());

    const mirakana::rhi::BufferHandle payload_buffer{.value = 7};
    const std::vector<mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationRow> destinations{
        {.graph_asset = graph.asset,
         .page_index = 2,
         .mount_id = {.value = 22},
         .buffer = payload_buffer,
         .destination_offset = 1024U,
         .destination_size = 64U,
         .estimated_gpu_resident_bytes = 64U},
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 20},
         .buffer = payload_buffer,
         .destination_offset = 1088U,
         .destination_size = 64U,
         .estimated_gpu_resident_bytes = 64U},
    };

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_page_buffer_destinations(
        mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .request_plan = &request_plan,
            .destination_rows = destinations,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.requested_page_count == 2U);
    MK_REQUIRE(result.planned_destination_count == 2U);
    MK_REQUIRE(result.input_destination_row_count == 2U);
    MK_REQUIRE(result.total_destination_bytes == 128U);
    MK_REQUIRE(result.total_estimated_gpu_resident_bytes == 128U);
    MK_REQUIRE(result.destination_rows.size() == 2U);
    MK_REQUIRE(result.destination_rows[0].request_index == 0U);
    MK_REQUIRE(result.destination_rows[0].page_index == 2U);
    MK_REQUIRE(result.destination_rows[0].mount_id.value == 22U);
    MK_REQUIRE(result.destination_rows[0].buffer.value == payload_buffer.value);
    MK_REQUIRE(result.destination_rows[0].destination_offset == 1024U);
    MK_REQUIRE(result.destination_rows[0].destination_size == 64U);
    MK_REQUIRE(result.destination_rows[0].fence_wait_point ==
               mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_gpu_work);
    MK_REQUIRE(result.destination_rows[0].synchronized_with_fence);
    MK_REQUIRE(result.destination_rows[1].request_index == 1U);
    MK_REQUIRE(result.destination_rows[1].page_index == 0U);
    MK_REQUIRE(result.destination_rows[1].mount_id.value == 20U);
    MK_REQUIRE(result.destination_rows[1].destination_offset == 1088U);
    MK_REQUIRE(result.destination_rows[1].destination_size == 64U);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.used_native_directstorage);
    MK_REQUIRE(!result.submitted_native_queue);
    MK_REQUIRE(!result.used_directstorage_resource_destination);
    MK_REQUIRE(!result.used_gpu_decompression);
    MK_REQUIRE(!result.allocated_rhi_resources);
    MK_REQUIRE(!result.enforced_allocator_budget);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.exposed_native_handles);
}

MK_TEST("runtime rhi mavg page buffer destination planner rejects duplicate destination pages before io") {
    const auto graph = make_buffer_destination_graph();
    const auto payload_text = make_payload_text(graph);
    const auto request_plan = make_request_plan(graph, payload_text);
    const mirakana::rhi::BufferHandle payload_buffer{.value = 7};
    const std::vector<mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationRow> duplicate_destinations{
        {.graph_asset = graph.asset,
         .page_index = 2,
         .mount_id = {.value = 22},
         .buffer = payload_buffer,
         .destination_offset = 1024U,
         .destination_size = 64U,
         .estimated_gpu_resident_bytes = 64U},
        {.graph_asset = graph.asset,
         .page_index = 2,
         .mount_id = {.value = 23},
         .buffer = payload_buffer,
         .destination_offset = 1088U,
         .destination_size = 64U,
         .estimated_gpu_resident_bytes = 64U},
    };

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_page_buffer_destinations(
        mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .request_plan = &request_plan,
            .destination_rows = duplicate_destinations,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.destination_rows.empty());
    MK_REQUIRE(has_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationDiagnosticCode::duplicate_destination_row));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.used_native_directstorage);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.exposed_native_handles);
}

MK_TEST("runtime rhi mavg page buffer destination planner rejects missing and too small destination rows") {
    const auto graph = make_buffer_destination_graph();
    const auto payload_text = make_payload_text(graph);
    const auto request_plan = make_request_plan(graph, payload_text);
    const mirakana::rhi::BufferHandle payload_buffer{.value = 7};
    const std::vector<mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationRow> missing_page_destination{
        {.graph_asset = graph.asset,
         .page_index = 2,
         .mount_id = {.value = 22},
         .buffer = payload_buffer,
         .destination_offset = 1024U,
         .destination_size = 64U,
         .estimated_gpu_resident_bytes = 64U},
    };
    const auto missing = mirakana::runtime_rhi::plan_runtime_mavg_page_buffer_destinations(
        mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .request_plan = &request_plan,
            .destination_rows = missing_page_destination,
        });

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing, mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationDiagnosticCode::missing_destination_row));

    const std::vector<mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationRow> too_small_destination{
        {.graph_asset = graph.asset,
         .page_index = 2,
         .mount_id = {.value = 22},
         .buffer = payload_buffer,
         .destination_offset = 1024U,
         .destination_size = 32U,
         .estimated_gpu_resident_bytes = 32U},
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 20},
         .buffer = payload_buffer,
         .destination_offset = 1088U,
         .destination_size = 64U,
         .estimated_gpu_resident_bytes = 64U},
    };
    const auto too_small = mirakana::runtime_rhi::plan_runtime_mavg_page_buffer_destinations(
        mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .request_plan = &request_plan,
            .destination_rows = too_small_destination,
        });

    MK_REQUIRE(!too_small.succeeded());
    MK_REQUIRE(has_diagnostic(
        too_small, mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationDiagnosticCode::destination_range_mismatch));
    MK_REQUIRE(!too_small.used_native_directstorage);
    MK_REQUIRE(!too_small.allocated_rhi_resources);
    MK_REQUIRE(!too_small.exposed_native_handles);
}

int main() {
    return mirakana::test::run_all();
}
