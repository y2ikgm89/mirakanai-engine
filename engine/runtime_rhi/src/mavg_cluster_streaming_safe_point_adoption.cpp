// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_cluster_streaming_safe_point_adoption.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgClusterStreamingSafePointAdoptionResult& result,
                    RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode code, AssetId graph_asset,
                    std::uint32_t page_index, runtime::RuntimeResidentPackageMountIdV2 mount_id, std::string message) {
    result.diagnostics.push_back(RuntimeMavgClusterStreamingSafePointAdoptionDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .mount_id = mount_id,
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_mount_id(const runtime::RuntimeResidentPackageMountSetV2& mount_set,
                                     runtime::RuntimeResidentPackageMountIdV2 mount_id) noexcept {
    return std::ranges::any_of(mount_set.mounts(), [mount_id](const runtime::RuntimeResidentPackageMountRecordV2& row) {
        return row.id == mount_id;
    });
}

[[nodiscard]] bool contains_mount_id(std::span<const runtime::RuntimeResidentPackageMountIdV2> mount_ids,
                                     runtime::RuntimeResidentPackageMountIdV2 mount_id) noexcept {
    return std::ranges::find(mount_ids, mount_id) != mount_ids.end();
}

[[nodiscard]] const RuntimeMavgClusterStreamingSafePointAdoptionMountRow*
find_mount_row(std::span<const RuntimeMavgClusterStreamingSafePointAdoptionMountRow> mount_rows, AssetId graph_asset,
               std::uint32_t page_index) noexcept {
    const auto found = std::ranges::find_if(mount_rows, [graph_asset, page_index](const auto& row) {
        return row.graph_asset == graph_asset && row.page_index == page_index;
    });
    if (found == mount_rows.end()) {
        return nullptr;
    }
    return &*found;
}

[[nodiscard]] bool contains_page_index(std::span<const std::uint32_t> page_indices, std::uint32_t page_index) noexcept {
    return std::ranges::find(page_indices, page_index) != page_indices.end();
}

void copy_eviction_plan_diagnostics(RuntimeMavgClusterStreamingSafePointAdoptionResult& result, AssetId graph_asset) {
    for (const auto& diagnostic : result.eviction_plan.diagnostics) {
        add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::eviction_plan_failed,
                       graph_asset, 0, diagnostic.mount,
                       diagnostic.message.empty() ? diagnostic.code : diagnostic.message);
    }
    if (!result.eviction_plan.succeeded() && result.diagnostics.empty()) {
        add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::eviction_plan_failed,
                       graph_asset, 0, {}, "MAVG safe-point adoption eviction plan failed");
    }
}

void copy_catalog_refresh_diagnostics(RuntimeMavgClusterStreamingSafePointAdoptionResult& result, AssetId graph_asset) {
    for (const auto& diagnostic : result.catalog_refresh.catalog_build.diagnostics) {
        add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::catalog_refresh_failed,
                       graph_asset, 0, {}, diagnostic.diagnostic);
    }
    for (const auto& diagnostic : result.catalog_refresh.budget_execution.diagnostics) {
        add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::catalog_refresh_failed,
                       graph_asset, 0, {}, diagnostic.message.empty() ? diagnostic.code : diagnostic.message);
    }
    if (!result.catalog_refresh.succeeded() && result.diagnostics.empty()) {
        add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::catalog_refresh_failed,
                       graph_asset, 0, {}, "MAVG safe-point adoption catalog refresh failed");
    }
}

} // namespace

RuntimeMavgClusterStreamingSafePointAdoptionResult execute_runtime_mavg_cluster_streaming_safe_point_adoption(
    runtime::RuntimeResidentPackageMountSetV2& mount_set, runtime::RuntimeResidentCatalogCacheV2& catalog_cache,
    const RuntimeMavgClusterStreamingSafePointAdoptionDesc& desc) {
    RuntimeMavgClusterStreamingSafePointAdoptionResult result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.mounted_package_count = mount_set.mounts().size();
    result.invoked_direct_storage = false;
    result.invoked_gpu_upload = false;
    result.executed_backend = false;
    result.touched_renderer_or_rhi_handles = false;
    result.proved_async_overlap_performance = false;

    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::invalid_graph_asset,
                       desc.graph_asset, 0, {}, "MAVG safe-point adoption graph asset id must be non-zero");
        return result;
    }
    if (desc.closeout == nullptr) {
        add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::missing_closeout,
                       desc.graph_asset, 0, {}, "MAVG safe-point adoption requires closeout evidence");
        return result;
    }

    const auto& closeout = *desc.closeout;
    result.input_loaded_row_count = closeout.background_load.loaded_rows.size();
    if (!closeout.succeeded()) {
        add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::closeout_failed,
                       desc.graph_asset, 0, {}, "MAVG safe-point adoption requires successful closeout evidence");
        return result;
    }
    if (closeout.background_load.loaded_rows.empty()) {
        add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::missing_loaded_rows,
                       desc.graph_asset, 0, {}, "MAVG safe-point adoption requires at least one background loaded row");
        return result;
    }

    std::vector<std::uint32_t> loaded_page_indices;
    for (const auto& loaded : closeout.background_load.loaded_rows) {
        if (loaded.row.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::loaded_row_failed,
                           loaded.row.graph_asset, loaded.row.page_index, {},
                           "MAVG safe-point adoption loaded row graph asset must match descriptor graph asset");
            continue;
        }
        if (!loaded.candidate_load.succeeded()) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::loaded_row_failed,
                           loaded.row.graph_asset, loaded.row.page_index, {},
                           "MAVG safe-point adoption loaded row must contain a successful package load result");
            continue;
        }
        if (contains_page_index(loaded_page_indices, loaded.row.page_index)) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::loaded_row_failed,
                           loaded.row.graph_asset, loaded.row.page_index, {},
                           "MAVG safe-point adoption loaded rows must be unique by page");
            continue;
        }
        loaded_page_indices.push_back(loaded.row.page_index);
    }

    std::vector<runtime::RuntimeResidentPackageMountIdV2> seen_mount_ids;
    std::vector<std::uint32_t> seen_mount_page_indices;
    for (const auto& mount_row : desc.mount_rows) {
        if (mount_row.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::invalid_mount_row,
                           mount_row.graph_asset, mount_row.page_index, mount_row.mount_id,
                           "MAVG safe-point adoption mount row graph asset must match descriptor graph asset");
            continue;
        }
        if (!contains_page_index(loaded_page_indices, mount_row.page_index)) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::invalid_mount_row,
                           mount_row.graph_asset, mount_row.page_index, mount_row.mount_id,
                           "MAVG safe-point adoption mount rows must reference loaded pages");
            continue;
        }
        if (mount_row.mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::invalid_mount_id,
                           desc.graph_asset, mount_row.page_index, mount_row.mount_id,
                           "MAVG safe-point adoption mount ids must be non-zero");
            continue;
        }
        if (contains_page_index(seen_mount_page_indices, mount_row.page_index)) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::duplicate_mount_row,
                           desc.graph_asset, mount_row.page_index, mount_row.mount_id,
                           "MAVG safe-point adoption mount rows must be unique by page");
            continue;
        }
        if (contains_mount_id(seen_mount_ids, mount_row.mount_id)) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::duplicate_mount_id,
                           desc.graph_asset, mount_row.page_index, mount_row.mount_id,
                           "MAVG safe-point adoption mount ids must be unique");
            continue;
        }
        if (contains_mount_id(mount_set, mount_row.mount_id)) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::duplicate_mount_id,
                           desc.graph_asset, mount_row.page_index, mount_row.mount_id,
                           "MAVG safe-point adoption mount id is already mounted");
            continue;
        }
        seen_mount_page_indices.push_back(mount_row.page_index);
        seen_mount_ids.push_back(mount_row.mount_id);
    }

    for (const auto page_index : loaded_page_indices) {
        const auto* const mount_row = find_mount_row(desc.mount_rows, desc.graph_asset, page_index);
        if (mount_row == nullptr) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::missing_mount_row,
                           desc.graph_asset, page_index, {},
                           "MAVG safe-point adoption requires a mount id row for every loaded page");
        }
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    runtime::RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    std::vector<RuntimeMavgClusterStreamingSafePointAdoptionRow> projected_adopted_rows;
    projected_adopted_rows.reserve(closeout.background_load.loaded_rows.size());
    for (const auto& loaded : closeout.background_load.loaded_rows) {
        const auto* const mount_row = find_mount_row(desc.mount_rows, desc.graph_asset, loaded.row.page_index);
        const auto mount_result = projected_mount_set.mount(runtime::RuntimeResidentPackageMountRecordV2{
            .id = mount_row->mount_id,
            .label = loaded.row.candidate.label,
            .package = loaded.candidate_load.loaded_package.package,
        });
        projected_adopted_rows.push_back(RuntimeMavgClusterStreamingSafePointAdoptionRow{
            .graph_asset = desc.graph_asset,
            .page_index = loaded.row.page_index,
            .mount_id = mount_row->mount_id,
            .resident_mount = mount_result,
        });
        if (!mount_result.succeeded()) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::resident_mount_failed,
                           desc.graph_asset, loaded.row.page_index, mount_row->mount_id,
                           mount_result.diagnostic.message.empty() ? mount_result.diagnostic.code
                                                                   : mount_result.diagnostic.message);
            projected_adopted_rows.clear();
            return result;
        }
    }

    auto protected_mounts = closeout.gpu_memory_residency.eviction_review.protected_mount_ids;
    for (const auto mount_id : seen_mount_ids) {
        if (!contains_mount_id(protected_mounts, mount_id)) {
            protected_mounts.push_back(mount_id);
        }
    }

    result.invoked_eviction_plan = true;
    result.eviction_plan = runtime::plan_runtime_resident_package_evictions_v2(
        projected_mount_set,
        runtime::RuntimeResidentPackageEvictionPlanDescV2{
            .target_budget = closeout.gpu_memory_residency.target_budget,
            .overlay = desc.overlay,
            .candidate_unmount_order = closeout.gpu_memory_residency.eviction_review.eviction_candidate_unmount_order,
            .protected_mount_ids = std::move(protected_mounts),
        });
    if (!result.eviction_plan.succeeded()) {
        copy_eviction_plan_diagnostics(result, desc.graph_asset);
        return result;
    }

    for (const auto& step : result.eviction_plan.steps) {
        const auto unmount = projected_mount_set.unmount(step.mount_id);
        if (!unmount.succeeded()) {
            add_diagnostic(result, RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::eviction_plan_failed,
                           desc.graph_asset, 0, step.mount_id,
                           unmount.diagnostic.message.empty() ? unmount.diagnostic.code : unmount.diagnostic.message);
            return result;
        }
    }
    const auto projected_evicted_mount_count = result.eviction_plan.steps.size();

    runtime::RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    result.invoked_catalog_refresh = true;
    result.catalog_refresh =
        projected_catalog_cache.refresh(projected_mount_set, desc.overlay, closeout.gpu_memory_residency.target_budget);
    if (!result.catalog_refresh.succeeded()) {
        copy_catalog_refresh_diagnostics(result, desc.graph_asset);
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.adopted_rows = std::move(projected_adopted_rows);
    result.adopted_page_count = result.adopted_rows.size();
    result.evicted_mount_count = projected_evicted_mount_count;
    result.mounted_package_count = mount_set.mounts().size();
    result.mount_generation = mount_set.generation();
    result.committed = true;
    result.mutated_mount_set = true;
    return result;
}

bool has_runtime_mavg_cluster_streaming_safe_point_adoption_diagnostic(
    const RuntimeMavgClusterStreamingSafePointAdoptionResult& result,
    RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana::runtime_rhi
