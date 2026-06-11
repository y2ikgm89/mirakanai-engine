// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_cluster_streaming_safe_point_adoption.hpp"

#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord
make_record(mirakana::AssetId asset, mirakana::runtime::RuntimeAssetHandle handle, std::string content) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = mirakana::AssetKind::mesh,
        .path = "runtime/mavg/safe-point/" + std::to_string(handle.value) + ".geasset",
        .content_hash = asset.value + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = std::move(content),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_package(mirakana::runtime::RuntimeAssetRecord record) {
    return mirakana::runtime::RuntimeAssetPackage({std::move(record)});
}

void mount_resident_page(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set, std::uint32_t mount_id,
                         mirakana::AssetId asset, mirakana::runtime::RuntimeAssetHandle handle, std::string content) {
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
                       .label = "mavg-safe-point-" + std::to_string(mount_id),
                       .package = make_package(make_record(asset, handle, std::move(content))),
                   })
                   .succeeded());
}

[[nodiscard]] mirakana::runtime::RuntimeResidentCatalogCacheV2
make_refreshed_cache(const mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set) {
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    MK_REQUIRE(catalog_cache
                   .refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                            mirakana::runtime::RuntimeResourceResidencyBudgetV2{})
                   .succeeded());
    return catalog_cache;
}

[[nodiscard]] mirakana::runtime::RuntimePackageCandidateLoadResultV2
make_loaded_candidate(mirakana::AssetId graph_asset, std::uint32_t page_index, mirakana::AssetId page_asset,
                      mirakana::runtime::RuntimeAssetHandle handle, std::string content) {
    auto loaded = mirakana::runtime::RuntimePackageCandidateLoadResultV2{
        .status = mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded,
        .candidate =
            mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                .package_index_path = "runtime/mavg/page" + std::to_string(page_index) + ".geindex",
                .content_root = "runtime/mavg",
                .label = "mavg-safe-point-page-" + std::to_string(page_index),
            },
        .loaded_package =
            mirakana::runtime::RuntimeAssetPackageLoadResult{
                .package = make_package(make_record(page_asset, handle, std::move(content))),
                .failures = {},
            },
        .loaded_record_count = 1,
        .estimated_resident_bytes = 8,
        .invoked_load = true,
    };
    loaded.package_desc.index_path = loaded.candidate.package_index_path;
    loaded.package_desc.content_root = loaded.candidate.content_root;
    (void)graph_asset;
    return loaded;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutResult
make_successful_closeout(mirakana::AssetId graph_asset, mirakana::AssetId new_page_asset) {
    auto closeout = mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutResult{};
    closeout.page_streaming_plan.queued_page_requests.push_back(mirakana::runtime::RuntimeMavgPageStreamingPlanRow{
        .graph_asset = graph_asset,
        .page_index = 1,
        .priority = 1.0F,
        .reason = "safe-point-adoption",
        .candidate =
            mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                .package_index_path = "runtime/mavg/page1.geindex",
                .content_root = "runtime/mavg",
                .label = "mavg-safe-point-page-1",
            },
    });
    closeout.payload_page_load.loaded_pages.push_back(mirakana::runtime::RuntimeMavgPayloadPageRow{
        .graph_asset = graph_asset,
        .page_index = 1,
        .byte_offset = 0,
        .byte_size = 8,
    });
    closeout.payload_page_load.loaded_page_count = 1;
    closeout.background_load.loaded_rows.push_back(mirakana::runtime::RuntimeMavgPageStreamingBackgroundLoadedRow{
        .row = closeout.page_streaming_plan.queued_page_requests[0],
        .candidate_load = make_loaded_candidate(graph_asset, 1, new_page_asset,
                                                mirakana::runtime::RuntimeAssetHandle{.value = 21}, "new-page"),
        .worker_id = 0,
        .invoked_candidate_load = true,
    });
    closeout.background_load.execution.status = mirakana::JobExecutionRunStatus::ready;
    closeout.background_load.loaded_row_count = 1;
    closeout.background_load.executed_background_worker = true;
    closeout.gpu_memory_residency.target_budget =
        mirakana::runtime::RuntimeResourceResidencyBudgetV2{.max_resident_content_bytes = 18};
    closeout.gpu_memory_residency.eviction_review.eviction_candidate_unmount_order.push_back(
        mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12});
    closeout.gpu_memory_residency.eviction_review.protected_mount_ids.push_back(
        mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 10});
    closeout.gpu_memory_residency.eviction_review.eviction_plan.status =
        mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::planned;
    closeout.gpu_memory_residency.eviction_review.invoked_eviction_plan = true;
    closeout.gpu_memory_residency.applied_gpu_memory_pressure_policy = true;
    closeout.gpu_memory_residency.invoked_eviction_plan = true;
    closeout.queued_page_request_count = 1;
    closeout.payload_loaded_page_count = 1;
    closeout.background_loaded_row_count = 1;
    closeout.applied_gpu_memory_pressure_policy = true;
    closeout.executed_background_worker = true;
    return closeout;
}

[[nodiscard]] bool
has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionResult& result,
               mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime rhi mavg cluster streaming safe point adoption commits loaded rows with reviewed evictions") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/safe-point/graph");
    const auto root_asset = mirakana::AssetId::from_name("mavg/safe-point/root");
    const auto old_asset = mirakana::AssetId::from_name("mavg/safe-point/old");
    const auto new_asset = mirakana::AssetId::from_name("mavg/safe-point/new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, root_asset, mirakana::runtime::RuntimeAssetHandle{.value = 10}, "root");
    mount_resident_page(mount_set, 12, old_asset, mirakana::runtime::RuntimeAssetHandle{.value = 12}, "old-page-large");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_generation = mount_set.generation();
    const auto closeout = make_successful_closeout(graph_asset, new_asset);
    MK_REQUIRE(closeout.succeeded());
    const std::vector<mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionMountRow> mount_rows{
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionMountRow{
            .graph_asset = graph_asset,
            .page_index = 1,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11}},
    };

    const auto result = mirakana::runtime_rhi::execute_runtime_mavg_cluster_streaming_safe_point_adoption(
        mount_set, catalog_cache,
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .mount_rows = mount_rows,
            .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.adopted_page_count == 1U);
    MK_REQUIRE(result.evicted_mount_count == 1U);
    MK_REQUIRE(result.mounted_package_count == 2U);
    MK_REQUIRE(result.mutated_mount_set);
    MK_REQUIRE(result.invoked_catalog_refresh);
    MK_REQUIRE(!result.invoked_candidate_load);
    MK_REQUIRE(!result.invoked_direct_storage);
    MK_REQUIRE(!result.invoked_gpu_upload);
    MK_REQUIRE(!result.executed_backend);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(!result.proved_async_overlap_performance);
    MK_REQUIRE(mount_set.generation() > previous_generation);
    MK_REQUIRE(catalog_cache.has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), root_asset).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), new_asset).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), old_asset).has_value());
}

MK_TEST("runtime rhi mavg cluster streaming safe point adoption fails closed before mutation") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/safe-point/graph");
    const auto root_asset = mirakana::AssetId::from_name("mavg/safe-point/root");
    const auto new_asset = mirakana::AssetId::from_name("mavg/safe-point/new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, root_asset, mirakana::runtime::RuntimeAssetHandle{.value = 10}, "root");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    auto closeout = make_successful_closeout(graph_asset, new_asset);
    closeout.diagnostics.push_back(mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutDiagnostic{
        .code =
            mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::background_load_failed,
        .graph_asset = graph_asset,
        .page_index = 1,
        .message = "background failed",
    });

    const std::vector<mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionMountRow> mount_rows{
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionMountRow{
            .graph_asset = graph_asset,
            .page_index = 1,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 10}},
    };

    const auto result = mirakana::runtime_rhi::execute_runtime_mavg_cluster_streaming_safe_point_adoption(
        mount_set, catalog_cache,
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .mount_rows = mount_rows,
            .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(has_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::closeout_failed));
    MK_REQUIRE(result.adopted_page_count == 0U);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.invoked_catalog_refresh);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1U);
}

MK_TEST("runtime rhi mavg cluster streaming safe point adoption rejects duplicate mount rows before mutation") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/safe-point/graph");
    const auto root_asset = mirakana::AssetId::from_name("mavg/safe-point/root");
    const auto new_asset = mirakana::AssetId::from_name("mavg/safe-point/new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, root_asset, mirakana::runtime::RuntimeAssetHandle{.value = 10}, "root");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto closeout = make_successful_closeout(graph_asset, new_asset);
    const std::vector<mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionMountRow> mount_rows{
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionMountRow{
            .graph_asset = graph_asset,
            .page_index = 1,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11}},
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionMountRow{
            .graph_asset = graph_asset,
            .page_index = 1,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12}},
    };

    const auto result = mirakana::runtime_rhi::execute_runtime_mavg_cluster_streaming_safe_point_adoption(
        mount_set, catalog_cache,
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .mount_rows = mount_rows,
            .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(has_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::duplicate_mount_row));
    MK_REQUIRE(result.adopted_rows.empty());
    MK_REQUIRE(result.adopted_page_count == 0U);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.invoked_eviction_plan);
    MK_REQUIRE(!result.invoked_catalog_refresh);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1U);
}

MK_TEST("runtime rhi mavg cluster streaming safe point adoption keeps projected rows uncommitted on eviction failure") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/safe-point/graph");
    const auto root_asset = mirakana::AssetId::from_name("mavg/safe-point/root");
    const auto new_asset = mirakana::AssetId::from_name("mavg/safe-point/new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, root_asset, mirakana::runtime::RuntimeAssetHandle{.value = 10}, "root");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    auto closeout = make_successful_closeout(graph_asset, new_asset);
    closeout.gpu_memory_residency.target_budget =
        mirakana::runtime::RuntimeResourceResidencyBudgetV2{.max_resident_content_bytes = 1};
    closeout.gpu_memory_residency.eviction_review.eviction_candidate_unmount_order.clear();
    const std::vector<mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionMountRow> mount_rows{
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionMountRow{
            .graph_asset = graph_asset,
            .page_index = 1,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11}},
    };

    const auto result = mirakana::runtime_rhi::execute_runtime_mavg_cluster_streaming_safe_point_adoption(
        mount_set, catalog_cache,
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .mount_rows = mount_rows,
            .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(has_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::eviction_plan_failed));
    MK_REQUIRE(result.invoked_eviction_plan);
    MK_REQUIRE(!result.invoked_catalog_refresh);
    MK_REQUIRE(result.adopted_rows.empty());
    MK_REQUIRE(result.adopted_page_count == 0U);
    MK_REQUIRE(result.evicted_mount_count == 0U);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1U);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), root_asset).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), new_asset).has_value());
}

int main() {
    return mirakana::test::run_all();
}
