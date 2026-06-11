// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_rhi/mavg_cluster_streaming_residency_closeout.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_graph_asset,
    missing_closeout,
    closeout_failed,
    missing_loaded_rows,
    loaded_row_failed,
    invalid_mount_row,
    duplicate_mount_row,
    missing_mount_row,
    invalid_mount_id,
    duplicate_mount_id,
    resident_mount_failed,
    eviction_plan_failed,
    catalog_refresh_failed,
};

struct RuntimeMavgClusterStreamingSafePointAdoptionDiagnostic {
    RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode code{
        RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode::none};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    std::string message;
};

struct RuntimeMavgClusterStreamingSafePointAdoptionMountRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
};

struct RuntimeMavgClusterStreamingSafePointAdoptionDesc {
    AssetId graph_asset;
    const RuntimeMavgClusterStreamingResidencyCloseoutResult* closeout{nullptr};
    std::span<const RuntimeMavgClusterStreamingSafePointAdoptionMountRow> mount_rows;
    runtime::RuntimePackageMountOverlay overlay{runtime::RuntimePackageMountOverlay::last_mount_wins};
};

struct RuntimeMavgClusterStreamingSafePointAdoptionRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    runtime::RuntimeResidentPackageMountResultV2 resident_mount;
};

struct RuntimeMavgClusterStreamingSafePointAdoptionResult {
    std::vector<RuntimeMavgClusterStreamingSafePointAdoptionRow> adopted_rows;
    runtime::RuntimeResidentPackageEvictionPlanResultV2 eviction_plan;
    runtime::RuntimeResidentCatalogCacheRefreshResultV2 catalog_refresh;
    std::vector<RuntimeMavgClusterStreamingSafePointAdoptionDiagnostic> diagnostics;
    std::size_t input_loaded_row_count{0};
    std::size_t adopted_page_count{0};
    std::size_t evicted_mount_count{0};
    std::size_t mounted_package_count{0};
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    bool invoked_candidate_load{false};
    bool invoked_eviction_plan{false};
    bool invoked_catalog_refresh{false};
    bool committed{false};
    bool mutated_mount_set{false};
    bool invoked_direct_storage{false};
    bool invoked_gpu_upload{false};
    bool executed_backend{false};
    bool touched_renderer_or_rhi_handles{false};
    bool proved_async_overlap_performance{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && committed;
    }
};

/// Adopts already background-loaded MAVG page packages from the Phase 5 closeout result into a caller-owned resident
/// mount set/catalog cache safe point. This helper uses only public runtime resident mount/catalog primitives and does
/// not reload package candidates, execute DirectStorage, upload to GPU, execute a backend, touch renderer/RHI handles,
/// run autonomous streaming, or prove async-overlap/performance.
[[nodiscard]] RuntimeMavgClusterStreamingSafePointAdoptionResult
execute_runtime_mavg_cluster_streaming_safe_point_adoption(
    runtime::RuntimeResidentPackageMountSetV2& mount_set, runtime::RuntimeResidentCatalogCacheV2& catalog_cache,
    const RuntimeMavgClusterStreamingSafePointAdoptionDesc& desc);

[[nodiscard]] bool has_runtime_mavg_cluster_streaming_safe_point_adoption_diagnostic(
    const RuntimeMavgClusterStreamingSafePointAdoptionResult& result,
    RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
