// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/core/job_execution.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/renderer/gpu_memory_policy.hpp"
#include "mirakana/runtime_rhi/mavg_cluster_streaming_residency_closeout.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_closeout_graph(std::string_view payload) {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/cluster-streaming-closeout");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/cluster-streaming-closeout-source");
    const auto material = mirakana::AssetId::from_name("materials/cluster-streaming-closeout-material");
    const auto page0 = payload.find("page0-cluster-bytes");
    const auto page1 = payload.find("page1-cluster-bytes");
    const auto page2 = payload.find("page2-cluster-bytes");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/cluster-streaming-closeout.glb",
        .cluster_payload_uri = "runtime/mavg/cluster-streaming-closeout.mavgpayload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 64,
        .pages =
            {
                mirakana::MavgClusterGraphPage{.page_index = 0,
                                               .byte_offset = static_cast<std::uint64_t>(page0),
                                               .byte_size = 19,
                                               .first_cluster = 0,
                                               .cluster_count = 1},
                mirakana::MavgClusterGraphPage{.page_index = 1,
                                               .byte_offset = static_cast<std::uint64_t>(page1),
                                               .byte_size = 19,
                                               .first_cluster = 1,
                                               .cluster_count = 1},
                mirakana::MavgClusterGraphPage{.page_index = 2,
                                               .byte_offset = static_cast<std::uint64_t>(page2),
                                               .byte_size = 19,
                                               .first_cluster = 2,
                                               .cluster_count = 1},
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
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = -1.0F, .y = -1.0F, .z = 0.0F},
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
                                                     .max = mirakana::MavgVec3f{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
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

[[nodiscard]] mirakana::MavgLodPageRequest make_request(mirakana::AssetId graph_asset, std::uint32_t page_index,
                                                        float priority, std::string_view reason) {
    return mirakana::MavgLodPageRequest{
        .graph_asset = graph_asset,
        .page_index = page_index,
        .priority = priority,
        .reason = std::string(reason),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPageStreamingCandidateRow make_candidate_row(mirakana::AssetId graph_asset,
                                                                                         std::uint32_t page_index) {
    return mirakana::runtime::RuntimeMavgPageStreamingCandidateRow{
        .graph_asset = graph_asset,
        .page_index = page_index,
        .candidate =
            mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                .package_index_path = "runtime/mavg/page" + std::to_string(page_index) + ".geindex",
                .content_root = "runtime/mavg",
                .label = "mavg-closeout-page-" + std::to_string(page_index),
            },
    };
}

void write_page_package(mirakana::MemoryFileSystem& filesystem, std::uint32_t page_index, mirakana::AssetId asset,
                        std::string_view content) {
    const auto package_path = "page" + std::to_string(page_index) + ".payload";
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = mirakana::AssetKind::mesh,
                                                                      .path = package_path,
                                                                      .content = std::string(content),
                                                                      .source_revision = page_index + 1U,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text("runtime/mavg/page" + std::to_string(page_index) + ".geindex",
                          mirakana::serialize_asset_cooked_package_index(index));
    filesystem.write_text("runtime/mavg/" + package_path, content);
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_runtime_package(mirakana::AssetId asset, std::uint32_t handle_value, std::string_view content) {
    return mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = handle_value},
        .asset = asset,
        .kind = mirakana::AssetKind::mesh,
        .path = "runtime/mavg/resident-closeout-" + std::to_string(handle_value) + ".geasset",
        .content_hash = asset.value + handle_value,
        .source_revision = handle_value,
        .dependencies = {},
        .content = std::string(content),
    }});
}

void mount_resident_page(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set, std::uint32_t mount_id,
                         mirakana::AssetId asset, std::string_view content) {
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
                       .label = "mavg-closeout-resident-" + std::to_string(mount_id),
                       .package = make_runtime_package(asset, mount_id, content),
                   })
                   .succeeded());
}

[[nodiscard]] mirakana::GpuMemoryPolicyPlan make_gpu_memory_policy(std::uint64_t target_bytes) {
    const std::vector<mirakana::GpuMemoryRequestDesc> requests{
        mirakana::GpuMemoryRequestDesc{.residency = mirakana::GpuMemoryResidencyClass::placed,
                                       .requested_bytes = target_bytes,
                                       .require_declared_budget_evidence = true,
                                       .require_residency_pressure_evidence = true,
                                       .require_package_counter_evidence = true,
                                       .source_index = 31}};
    return mirakana::plan_gpu_memory_policy(mirakana::GpuMemoryPolicyDesc{
        .requests = requests,
        .declared_local_budget_bytes = 64,
        .os_video_memory_budget_available = true,
        .os_local_budget_bytes = 64,
        .os_local_usage_bytes = 48,
        .residency_pressure_event_count = 1,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .package_counter_evidence_ready = true,
    });
}

[[nodiscard]] bool
has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutResult& result,
               mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST(
    "runtime rhi mavg cluster streaming residency closeout composes payload background and memory pressure evidence") {
    const std::string payload =
        "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\npage2-cluster-bytes\n";
    const auto graph = make_closeout_graph(payload);
    auto filesystem = mirakana::MemoryFileSystem{};
    filesystem.write_text(graph.cluster_payload_uri, payload);
    write_page_package(filesystem, 1, mirakana::AssetId::from_name("mavg/closeout/page-1"), "page one package");
    write_page_package(filesystem, 2, mirakana::AssetId::from_name("mavg/closeout/page-2"), "page two package");
    auto pool = mirakana::JobExecutionPool(mirakana::JobExecutionPoolDesc{
        .name = "runtime_rhi.mavg.closeout",
        .logical_processor_count = 2,
        .worker_count = 2,
        .queue_capacity_per_worker = 4,
        .scratch_budget_bytes_per_worker = 512,
        .frame_index = 91,
    });

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/closeout/root"), "root-page");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/closeout/cold"), "cold-page");
    mount_resident_page(mount_set, 12, mirakana::AssetId::from_name("mavg/closeout/hot"), "hot-page");
    const auto resident_pages = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 1, 4.0F, "visible-middle"),
        make_request(graph.asset, 2, 8.0F, "visible-near"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_candidate_row(graph.asset, 1),
        make_candidate_row(graph.asset, 2),
    };
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
        mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow{.graph_asset = graph.asset, .cluster_index = 1},
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
            .resident_page_last_used_generation = 9},
    };
    const auto gpu_policy = make_gpu_memory_policy(22);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_cluster_streaming_residency_closeout(
        mount_set, mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .resident_pages = &resident_pages,
                       .page_requests = requests,
                       .candidates = candidates,
                       .filesystem = &filesystem,
                       .execution_pool = &pool,
                       .payload_path = graph.cluster_payload_uri,
                       .gpu_memory_policy = &gpu_policy,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .recency_rows = recency_rows,
                       .frame_index = 91,
                       .scratch_bytes_per_task = 64,
                   });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.queued_page_request_count == 2U);
    MK_REQUIRE(result.payload_loaded_page_count == 2U);
    MK_REQUIRE(result.background_loaded_row_count == 2U);
    MK_REQUIRE(result.protected_visible_page_count == 1U);
    MK_REQUIRE(result.protected_fallback_page_count == 1U);
    MK_REQUIRE(result.deterministic_degradation_row_count == 1U);
    MK_REQUIRE(result.applied_gpu_memory_pressure_policy);
    MK_REQUIRE(result.preserved_visible_geometry_without_holes);
    MK_REQUIRE(result.invoked_file_io);
    MK_REQUIRE(result.executed_background_worker);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.invoked_catalog_refresh);
    MK_REQUIRE(!result.invoked_direct_storage);
    MK_REQUIRE(!result.invoked_gpu_upload);
    MK_REQUIRE(!result.executed_backend);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(!result.proved_async_overlap_performance);
    MK_REQUIRE(mount_set.mounts().size() == 3U);
}

MK_TEST("runtime rhi mavg cluster streaming residency closeout fail closes payload range errors before mutation") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\n";
    auto graph = make_closeout_graph(payload + "page1-cluster-bytes\npage2-cluster-bytes\n");
    graph.pages[1].byte_offset = static_cast<std::uint64_t>(payload.size() + 64U);
    auto filesystem = mirakana::MemoryFileSystem{};
    filesystem.write_text(graph.cluster_payload_uri, payload);
    auto pool = mirakana::JobExecutionPool(mirakana::JobExecutionPoolDesc{
        .name = "runtime_rhi.mavg.closeout.failure",
        .logical_processor_count = 2,
        .worker_count = 2,
        .queue_capacity_per_worker = 4,
        .scratch_budget_bytes_per_worker = 512,
        .frame_index = 92,
    });

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/closeout/root"), "root-page");
    const auto resident_pages = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 1, 4.0F, "visible-middle"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_candidate_row(graph.asset, 1),
    };
    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        mirakana::runtime::RuntimeMavgResidentPageMountRow{
            .graph_asset = graph.asset,
            .page_index = 0,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 10}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow{.graph_asset = graph.asset, .cluster_index = 1},
    };
    const auto gpu_policy = make_gpu_memory_policy(8);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_cluster_streaming_residency_closeout(
        mount_set, mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .resident_pages = &resident_pages,
                       .page_requests = requests,
                       .candidates = candidates,
                       .filesystem = &filesystem,
                       .execution_pool = &pool,
                       .payload_path = graph.cluster_payload_uri,
                       .gpu_memory_policy = &gpu_policy,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .frame_index = 92,
                   });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.queued_page_request_count == 1U);
    MK_REQUIRE(result.payload_loaded_page_count == 0U);
    MK_REQUIRE(result.background_loaded_row_count == 0U);
    MK_REQUIRE(result.deterministic_degradation_row_count == 1U);
    MK_REQUIRE(has_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::payload_page_load_failed));
    MK_REQUIRE(result.invoked_file_io);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.applied_gpu_memory_pressure_policy);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.invoked_catalog_refresh);
    MK_REQUIRE(!result.invoked_direct_storage);
    MK_REQUIRE(!result.invoked_gpu_upload);
    MK_REQUIRE(!result.executed_backend);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(mount_set.mounts().size() == 1U);
}

int main() {
    return mirakana::test::run_all();
}
