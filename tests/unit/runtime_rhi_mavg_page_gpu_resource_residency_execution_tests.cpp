// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/mavg_page_streaming.hpp"
#include "mirakana/runtime_rhi/mavg_page_gpu_resource_residency_execution.hpp"

#include <cstdint>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_mavg_residency_execution_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/runtime-rhi-residency-execution");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/runtime-rhi-residency-execution-source");
    const auto material = mirakana::AssetId::from_name("materials/runtime-rhi-residency-execution-material");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/runtime-rhi-residency-execution.glb",
        .cluster_payload_uri = "runtime/runtime-rhi-residency-execution.mavg_payload",
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

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessResult
make_ready_resource_update(mirakana::rhi::NullRhiDevice& device, mirakana::AssetId graph_asset) {
    const auto root = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::vertex,
    });
    const auto selected = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 512,
        .usage = mirakana::rhi::BufferUsage::vertex,
    });
    const auto cold = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 1024,
        .usage = mirakana::rhi::BufferUsage::vertex,
    });

    return mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessResult{
        .resident_page_resources =
            {
                mirakana::runtime_rhi::RuntimeMavgResidentPageResourceRow{
                    .graph_asset = graph_asset,
                    .page_index = 0,
                    .mount_id = {.value = 10},
                    .resource = {.kind = mirakana::rhi::RhiResidencyResourceKind::buffer, .buffer = root},
                    .estimated_gpu_resident_bytes = 256},
                mirakana::runtime_rhi::RuntimeMavgResidentPageResourceRow{
                    .graph_asset = graph_asset,
                    .page_index = 1,
                    .mount_id = {.value = 11},
                    .resource = {.kind = mirakana::rhi::RhiResidencyResourceKind::buffer, .buffer = selected},
                    .estimated_gpu_resident_bytes = 512},
                mirakana::runtime_rhi::RuntimeMavgResidentPageResourceRow{
                    .graph_asset = graph_asset,
                    .page_index = 2,
                    .mount_id = {.value = 12},
                    .resource = {.kind = mirakana::rhi::RhiResidencyResourceKind::buffer, .buffer = cold},
                    .estimated_gpu_resident_bytes = 1024},
            },
        .update_rows = {},
        .diagnostics = {},
        .input_destination_row_count = 3,
        .ready_resource_count = 3,
        .duplicate_destination_row_count = 0,
        .ready_destination_bytes = 1792,
        .ready_estimated_gpu_resident_bytes = 1792,
        .used_directstorage_resource_destination = true,
        .used_directstorage_caller_owned_rhi_resource_destination = true,
        .directstorage_status_complete = true,
        .observed_native_queue_submission = true,
        .ready_for_residency_actions = true,
        .invoked_file_io = false,
        .submitted_native_queue = false,
        .allocated_rhi_resources = false,
        .invoked_rhi_residency_action = false,
        .invoked_native_make_resident = false,
        .invoked_native_evict = false,
        .enforced_allocator_budget = false,
        .mutated_mount_set = false,
        .used_gpu_decompression = false,
        .exposed_native_handles = false,
    };
}

[[nodiscard]] bool
has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionResult& result,
               mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool
has_residency_diagnostic(const mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionResult& result,
                         mirakana::runtime_rhi::RuntimeMavgPageResidencyActionDiagnosticCode code) {
    for (const auto& diagnostic : result.residency_action_result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime rhi mavg page gpu resource residency execution consumes readiness and delegates residency actions") {
    const auto graph = make_mavg_residency_execution_graph();
    const auto validation = mirakana::validate_mavg_cluster_graph(graph);
    MK_REQUIRE(validation.valid());
    mirakana::rhi::NullRhiDevice device;
    const auto readiness = make_ready_resource_update(device, graph.asset);
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 1},
    };
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> protected_mounts{{.value = 10}};
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> eviction_order{{.value = 10}, {.value = 12}};

    const auto result = mirakana::runtime_rhi::execute_runtime_mavg_page_gpu_resource_residency_actions(
        device, mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .readiness_result = &readiness,
                    .selected_clusters = selected_clusters,
                    .protected_mount_ids = protected_mounts,
                    .eviction_candidate_unmount_order = eviction_order,
                });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.consumed_gpu_resource_update_readiness);
    MK_REQUIRE(result.input_ready_resource_count == 3U);
    MK_REQUIRE(result.selected_page_resource_count == 1U);
    MK_REQUIRE(result.protected_page_resource_count == 1U);
    MK_REQUIRE(result.eviction_candidate_resource_count == 1U);
    MK_REQUIRE(result.made_resident_count == 2U);
    MK_REQUIRE(result.evicted_count == 1U);
    MK_REQUIRE(result.protected_skip_count == 1U);
    MK_REQUIRE(result.invoked_rhi_residency_action);
    MK_REQUIRE(result.invoked_make_resident_action);
    MK_REQUIRE(result.invoked_evict_action);
    MK_REQUIRE(result.used_directstorage_resource_destination);
    MK_REQUIRE(result.used_directstorage_caller_owned_rhi_resource_destination);
    MK_REQUIRE(result.directstorage_status_complete);
    MK_REQUIRE(result.observed_native_queue_submission);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.submitted_native_queue);
    MK_REQUIRE(!result.allocated_rhi_resources);
    MK_REQUIRE(!result.invoked_native_make_resident);
    MK_REQUIRE(!result.invoked_native_evict);
    MK_REQUIRE(!result.enforced_allocator_budget);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.used_gpu_decompression);
    MK_REQUIRE(!result.exposed_native_handles);
}

MK_TEST("runtime rhi mavg page gpu resource residency execution rejects missing or invalid readiness evidence") {
    const auto graph = make_mavg_residency_execution_graph();
    mirakana::rhi::NullRhiDevice device;

    const auto missing = mirakana::runtime_rhi::execute_runtime_mavg_page_gpu_resource_residency_actions(
        device, mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                });

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing,
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::missing_readiness_result));
    MK_REQUIRE(!missing.consumed_gpu_resource_update_readiness);
    MK_REQUIRE(!missing.invoked_rhi_residency_action);

    auto invalid_readiness = make_ready_resource_update(device, graph.asset);
    invalid_readiness.diagnostics.push_back(mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDiagnostic{
        .code = mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::failed_status_result,
        .graph_asset = graph.asset,
        .page_index = 1,
        .mount_id = {.value = 11},
        .message = "status failed"});

    const auto invalid = mirakana::runtime_rhi::execute_runtime_mavg_page_gpu_resource_residency_actions(
        device, mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .readiness_result = &invalid_readiness,
                });

    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(has_diagnostic(
        invalid,
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::invalid_readiness_result));
    MK_REQUIRE(!invalid.invoked_rhi_residency_action);

    auto unsupported_readiness = make_ready_resource_update(device, graph.asset);
    unsupported_readiness.allocated_rhi_resources = true;

    const auto unsupported = mirakana::runtime_rhi::execute_runtime_mavg_page_gpu_resource_residency_actions(
        device, mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .readiness_result = &unsupported_readiness,
                });

    MK_REQUIRE(!unsupported.succeeded());
    MK_REQUIRE(has_diagnostic(
        unsupported,
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::invalid_readiness_result));
    MK_REQUIRE(!unsupported.invoked_rhi_residency_action);
    MK_REQUIRE(!unsupported.allocated_rhi_resources);

    auto not_ready = make_ready_resource_update(device, graph.asset);
    not_ready.ready_for_residency_actions = false;

    const auto rejected = mirakana::runtime_rhi::execute_runtime_mavg_page_gpu_resource_residency_actions(
        device, mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .readiness_result = &not_ready,
                });

    MK_REQUIRE(!rejected.succeeded());
    MK_REQUIRE(has_diagnostic(
        rejected,
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::readiness_not_ready));
    MK_REQUIRE(!rejected.invoked_rhi_residency_action);
}

MK_TEST("runtime rhi mavg page gpu resource residency execution reports delegated residency validation failures") {
    const auto graph = make_mavg_residency_execution_graph();
    mirakana::rhi::NullRhiDevice device;
    auto readiness = make_ready_resource_update(device, graph.asset);
    readiness.resident_page_resources.resize(1);
    readiness.ready_resource_count = readiness.resident_page_resources.size();
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 1},
    };

    const auto result = mirakana::runtime_rhi::execute_runtime_mavg_page_gpu_resource_residency_actions(
        device, mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .readiness_result = &readiness,
                    .selected_clusters = selected_clusters,
                });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.consumed_gpu_resource_update_readiness);
    MK_REQUIRE(has_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::residency_action_failed));
    MK_REQUIRE(has_residency_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgPageResidencyActionDiagnosticCode::missing_resident_page_resource));
    MK_REQUIRE(!result.invoked_rhi_residency_action);
}

int main() {
    return mirakana::test::run_all();
}
