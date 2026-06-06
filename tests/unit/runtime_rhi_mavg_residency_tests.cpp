// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/mavg_page_streaming.hpp"
#include "mirakana/runtime_rhi/mavg_residency.hpp"

#include <cstdint>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_mavg_residency_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/runtime-rhi-residency");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/runtime-rhi-residency-source");
    const auto material = mirakana::AssetId::from_name("materials/runtime-rhi-residency-material");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/runtime-rhi-residency.glb",
        .cluster_payload_uri = "runtime/runtime-rhi-residency.mavg_payload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 4096,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 256, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 256, .byte_size = 256, .first_cluster = 1, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 2, .byte_offset = 512, .byte_size = 256, .first_cluster = 2, .cluster_count = 1},
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

[[nodiscard]] bool has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgPageResidencyActionResult& result,
                                  mirakana::runtime_rhi::RuntimeMavgPageResidencyActionDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime rhi mavg page residency makes selected pages resident and evicts reviewed unprotected pages") {
    const auto graph = make_mavg_residency_graph();
    const auto validation = mirakana::validate_mavg_cluster_graph(graph);
    MK_REQUIRE(validation.valid());
    mirakana::rhi::NullRhiDevice device;
    const auto root = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::vertex,
    });
    const auto selected = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 512,
        .usage = mirakana::rhi::BufferUsage::vertex,
    });
    const auto cold = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const std::vector<mirakana::runtime_rhi::RuntimeMavgResidentPageResourceRow> resources{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resource = {.kind = mirakana::rhi::RhiResidencyResourceKind::buffer, .buffer = root},
         .estimated_gpu_resident_bytes = 256},
        {.graph_asset = graph.asset,
         .page_index = 1,
         .mount_id = {.value = 11},
         .resource = {.kind = mirakana::rhi::RhiResidencyResourceKind::buffer, .buffer = selected},
         .estimated_gpu_resident_bytes = 512},
        {.graph_asset = graph.asset,
         .page_index = 2,
         .mount_id = {.value = 12},
         .resource = {.kind = mirakana::rhi::RhiResidencyResourceKind::texture, .texture = cold},
         .estimated_gpu_resident_bytes = 1024},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 1},
    };
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> protected_mounts{{.value = 10}};
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> eviction_order{{.value = 10}, {.value = 12}};

    const auto result = mirakana::runtime_rhi::execute_runtime_mavg_page_residency_actions(
        device, mirakana::runtime_rhi::RuntimeMavgPageResidencyActionDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .selected_clusters = selected_clusters,
                    .resident_page_resources = resources,
                    .protected_mount_ids = protected_mounts,
                    .eviction_candidate_unmount_order = eviction_order,
                });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.input_resident_page_resource_count == 3U);
    MK_REQUIRE(result.selected_page_resource_count == 1U);
    MK_REQUIRE(result.protected_page_resource_count == 1U);
    MK_REQUIRE(result.eviction_candidate_resource_count == 1U);
    MK_REQUIRE(result.made_resident_count == 2U);
    MK_REQUIRE(result.evicted_count == 1U);
    MK_REQUIRE(result.protected_skip_count == 1U);
    MK_REQUIRE(result.invoked_rhi_residency_action);
    MK_REQUIRE(result.invoked_make_resident_action);
    MK_REQUIRE(result.invoked_evict_action);
    MK_REQUIRE(!result.invoked_native_make_resident);
    MK_REQUIRE(!result.invoked_native_evict);
    MK_REQUIRE(!result.exposed_native_handles);
    MK_REQUIRE(!result.enforced_allocator_budget);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.used_directstorage_resource_destination);
    MK_REQUIRE(!result.used_gpu_decompression);
}

MK_TEST("runtime rhi mavg page residency rejects duplicate and missing resource rows before rhi calls") {
    const auto graph = make_mavg_residency_graph();
    mirakana::rhi::NullRhiDevice device;
    const auto page0 = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::vertex,
    });
    const std::vector<mirakana::runtime_rhi::RuntimeMavgResidentPageResourceRow> duplicate_resources{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resource = {.kind = mirakana::rhi::RhiResidencyResourceKind::buffer, .buffer = page0},
         .estimated_gpu_resident_bytes = 256},
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 11},
         .resource = {.kind = mirakana::rhi::RhiResidencyResourceKind::buffer, .buffer = page0},
         .estimated_gpu_resident_bytes = 256},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 1},
    };
    const auto duplicate = mirakana::runtime_rhi::execute_runtime_mavg_page_residency_actions(
        device, mirakana::runtime_rhi::RuntimeMavgPageResidencyActionDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .selected_clusters = selected_clusters,
                    .resident_page_resources = duplicate_resources,
                });

    MK_REQUIRE(!duplicate.succeeded());
    MK_REQUIRE(has_diagnostic(
        duplicate,
        mirakana::runtime_rhi::RuntimeMavgPageResidencyActionDiagnosticCode::duplicate_resident_page_resource));
    MK_REQUIRE(!duplicate.invoked_rhi_residency_action);

    const std::vector<mirakana::runtime_rhi::RuntimeMavgResidentPageResourceRow> missing_page_resource{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resource = {.kind = mirakana::rhi::RhiResidencyResourceKind::buffer, .buffer = page0},
         .estimated_gpu_resident_bytes = 256},
    };
    const auto missing = mirakana::runtime_rhi::execute_runtime_mavg_page_residency_actions(
        device, mirakana::runtime_rhi::RuntimeMavgPageResidencyActionDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .selected_clusters = selected_clusters,
                    .resident_page_resources = missing_page_resource,
                });

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing, mirakana::runtime_rhi::RuntimeMavgPageResidencyActionDiagnosticCode::missing_resident_page_resource));
    MK_REQUIRE(!missing.invoked_rhi_residency_action);
}

int main() {
    return mirakana::test::run_all();
}
