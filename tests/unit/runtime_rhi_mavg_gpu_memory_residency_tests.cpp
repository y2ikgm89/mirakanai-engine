// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/gpu_memory_policy.hpp"
#include "mirakana/runtime_rhi/mavg_gpu_memory_residency.hpp"

#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_gpu_memory_residency_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/gpu-memory-residency");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/gpu-memory-residency-source");
    const auto material = mirakana::AssetId::from_name("materials/gpu-memory-residency-material");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/gpu-memory-residency.glb",
        .cluster_payload_uri = "runtime/gpu-memory-residency.mavg_payload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 4096,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 128, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 128, .byte_size = 128, .first_cluster = 1, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 2, .byte_offset = 256, .byte_size = 128, .first_cluster = 2, .cluster_count = 1},
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

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_runtime_package(mirakana::AssetId asset, std::uint32_t handle_value, std::string content) {
    return mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = handle_value},
        .asset = asset,
        .kind = mirakana::AssetKind::mesh,
        .path = "runtime/mavg/gpu-memory-residency-" + std::to_string(handle_value) + ".geasset",
        .content_hash = asset.value + handle_value,
        .source_revision = handle_value,
        .dependencies = {},
        .content = std::move(content),
    }});
}

void mount_resident_page(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set, std::uint32_t mount_id,
                         mirakana::AssetId asset, std::string content) {
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
                       .label = "mavg-gpu-memory-page-" + std::to_string(mount_id),
                       .package = make_runtime_package(asset, mount_id, std::move(content)),
                   })
                   .succeeded());
}

[[nodiscard]] mirakana::GpuMemoryPolicyPlan make_gpu_memory_policy(std::uint64_t target_bytes,
                                                                   bool residency_pressure_ready = true) {
    const std::vector<mirakana::GpuMemoryRequestDesc> requests{
        mirakana::GpuMemoryRequestDesc{.residency = mirakana::GpuMemoryResidencyClass::placed,
                                       .requested_bytes = target_bytes,
                                       .transient_heap = mirakana::GpuMemoryTransientHeapPolicy::none,
                                       .upload_pressure = mirakana::GpuMemoryUploadPressureKind::none,
                                       .scene_resources_available = true,
                                       .request_background_streaming = false,
                                       .request_automatic_eviction = false,
                                       .require_declared_budget_evidence = true,
                                       .require_residency_pressure_evidence = true,
                                       .require_package_counter_evidence = true,
                                       .source_index = 7}};
    return mirakana::plan_gpu_memory_policy(mirakana::GpuMemoryPolicyDesc{
        .requests = requests,
        .declared_local_budget_bytes = 64,
        .declared_non_local_budget_bytes = 0,
        .os_video_memory_budget_available = residency_pressure_ready,
        .os_local_budget_bytes = 64,
        .os_local_usage_bytes = residency_pressure_ready ? 48U : 0U,
        .os_non_local_budget_bytes = 0,
        .os_non_local_usage_bytes = 0,
        .committed_byte_estimate_available = false,
        .committed_byte_estimate = 0,
        .transient_heap_allocations = 0,
        .transient_placed_allocations = 0,
        .transient_placed_resources_alive = 0,
        .upload_bytes_written = 0,
        .residency_pressure_event_count = residency_pressure_ready ? 1U : 0U,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .require_backend_memory_evidence = false,
        .backend_memory_evidence_ready = false,
        .require_os_video_memory_budget = false,
        .package_counter_evidence_ready = true,
    });
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgGpuMemoryResidencyResult& result,
                                  mirakana::runtime_rhi::RuntimeMavgGpuMemoryResidencyDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime rhi mavg gpu memory pressure residency plans protected recency eviction without mutation") {
    const auto graph = make_gpu_memory_residency_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/gpu-memory-residency/root"), "root-data");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/gpu-memory-residency/cold"),
                        "cold-page-data");
    mount_resident_page(mount_set, 12, mirakana::AssetId::from_name("mavg/gpu-memory-residency/hot"), "hot-page-data");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        mirakana::runtime::RuntimeMavgResidentPageMountRow{
            .graph_asset = graph.asset,
            .page_index = 0,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 10}},
        mirakana::runtime::RuntimeMavgResidentPageMountRow{
            .graph_asset = graph.asset,
            .page_index = 1,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11}},
        mirakana::runtime::RuntimeMavgResidentPageMountRow{
            .graph_asset = graph.asset,
            .page_index = 2,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow{.graph_asset = graph.asset, .cluster_index = 0},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingRecencyRow> recency_rows{
        mirakana::runtime::RuntimeMavgPageStreamingRecencyRow{
            .graph_asset = graph.asset,
            .page_index = 1,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11},
            .resident_page_last_used_generation = 1},
        mirakana::runtime::RuntimeMavgPageStreamingRecencyRow{
            .graph_asset = graph.asset,
            .page_index = 2,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12},
            .resident_page_last_used_generation = 7},
    };
    const auto gpu_policy = make_gpu_memory_policy(22);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_gpu_memory_pressure_residency(
        mount_set,
        mirakana::runtime_rhi::RuntimeMavgGpuMemoryResidencyDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .gpu_memory_policy = &gpu_policy,
            .selected_clusters = selected_clusters,
            .resident_page_mounts = page_mounts,
            .recency_rows = recency_rows,
            .caller_protected_mount_ids = {},
            .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
            .policy_kind =
                mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.target_budget.max_resident_content_bytes.has_value());
    MK_REQUIRE(*result.target_budget.max_resident_content_bytes == 22U);
    MK_REQUIRE(result.target_resident_content_bytes == 22U);
    MK_REQUIRE(result.applied_gpu_memory_pressure_policy);
    MK_REQUIRE(result.invoked_eviction_plan);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(mount_set.mounts().size() == 3U);
    MK_REQUIRE(result.protected_mount_count == 1U);
    MK_REQUIRE(result.eviction_candidate_count == 2U);
    MK_REQUIRE(result.evicted_mount_count == 1U);
    MK_REQUIRE(result.eviction_review.eviction_plan.steps.size() == 1U);
    MK_REQUIRE(result.eviction_review.eviction_plan.steps[0].mount_id ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11});
}

MK_TEST("runtime rhi mavg gpu memory pressure residency fail closes without policy evidence") {
    const auto graph = make_gpu_memory_residency_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, graph.asset, "root-data");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        mirakana::runtime::RuntimeMavgResidentPageMountRow{
            .graph_asset = graph.asset,
            .page_index = 0,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 10}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow{.graph_asset = graph.asset, .cluster_index = 0},
    };
    const auto gpu_policy = make_gpu_memory_policy(8, false);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_gpu_memory_pressure_residency(
        mount_set, mirakana::runtime_rhi::RuntimeMavgGpuMemoryResidencyDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .gpu_memory_policy = &gpu_policy,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .recency_rows = {},
                       .caller_protected_mount_ids = {},
                       .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                   });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.target_budget.max_resident_content_bytes.has_value());
    MK_REQUIRE(!result.applied_gpu_memory_pressure_policy);
    MK_REQUIRE(!result.invoked_eviction_plan);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(has_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgGpuMemoryResidencyDiagnosticCode::gpu_memory_policy_failed));
    MK_REQUIRE(has_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgGpuMemoryResidencyDiagnosticCode::missing_residency_pressure_evidence));
}

int main() {
    return mirakana::test::run_all();
}
