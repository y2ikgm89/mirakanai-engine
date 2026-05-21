// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeWorldRegionStreamingActionKind : std::uint8_t {
    keep_resident = 0,
    load_region,
    unload_region,
};

enum class RuntimeWorldRegionStreamingPlanStatus : std::uint8_t {
    planned = 0,
    no_changes,
    invalid_request,
    budget_exceeded,
};

enum class RuntimeWorldRegionStreamingDiagnosticCode : std::uint8_t {
    invalid_region_id = 0,
    duplicate_region,
    invalid_package_index_path,
    duplicate_active_region,
    missing_active_region,
    missing_desired_region,
    duplicate_desired_region,
    missing_protected_region,
    duplicate_protected_region,
    protected_active_region_unload_requested,
    invalid_mount_id,
    duplicate_mount_id,
    resident_region_count_exceeded,
    resident_content_budget_exceeded,
    resident_asset_record_budget_exceeded,
    missing_package_candidate,
    package_streaming_execution_failed,
};

struct RuntimeWorldRegionPackageDesc {
    std::string region_id;
    RuntimePackageIndexDiscoveryCandidateV2 candidate;
    RuntimeResidentPackageMountIdV2 mount_id;
    std::uint64_t estimated_resident_bytes{0};
    std::size_t estimated_asset_records{0};
    std::vector<AssetId> required_preload_assets;
    std::vector<AssetKind> resident_resource_kinds;
};

struct RuntimeWorldRegionStreamingPlanRequest {
    std::vector<RuntimeWorldRegionPackageDesc> regions;
    std::vector<std::string> active_region_ids;
    std::vector<std::string> desired_region_ids;
    std::vector<std::string> protected_region_ids;
    RuntimeResourceResidencyBudgetV2 budget;
    std::size_t max_resident_regions{0};
};

struct RuntimeWorldRegionStreamingDiagnostic {
    RuntimeWorldRegionStreamingDiagnosticCode code{RuntimeWorldRegionStreamingDiagnosticCode::invalid_region_id};
    std::string region_id;
    std::string message;
};

struct RuntimeWorldRegionStreamingPlanRow {
    RuntimeWorldRegionStreamingActionKind action{RuntimeWorldRegionStreamingActionKind::keep_resident};
    std::string region_id;
    RuntimeResidentPackageMountIdV2 mount_id;
    std::string package_index_path;
    std::string content_root;
    std::uint64_t estimated_resident_bytes{0};
    std::size_t estimated_asset_records{0};
    std::size_t required_preload_asset_count{0};
    std::size_t resident_resource_kind_count{0};
    bool protected_region{false};
};

struct RuntimeWorldRegionStreamingPlan {
    RuntimeWorldRegionStreamingPlanStatus status{RuntimeWorldRegionStreamingPlanStatus::invalid_request};
    std::vector<RuntimeWorldRegionStreamingDiagnostic> diagnostics;
    std::vector<RuntimeWorldRegionStreamingPlanRow> rows;
    std::size_t projected_resident_region_count{0};
    std::uint64_t projected_resident_bytes{0};
    std::size_t projected_resident_asset_records{0};
    std::size_t load_count{0};
    std::size_t keep_count{0};
    std::size_t unload_count{0};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans deterministic world-region load/unload intent rows over caller-reviewed package candidates.
/// This value-only helper does not read files, load packages, mutate resident mounts, refresh catalogs, stream in the
/// background, own renderer/RHI resources, or touch native handles.
[[nodiscard]] RuntimeWorldRegionStreamingPlan
plan_runtime_world_region_streaming(const RuntimeWorldRegionStreamingPlanRequest& request);

enum class RuntimeWorldRegionStreamingSafePointStatus : std::uint8_t {
    invalid_plan = 0,
    no_changes,
    failed,
    completed,
};

struct RuntimeWorldRegionStreamingSafePointDesc {
    RuntimeWorldRegionStreamingPlan plan;
    std::vector<RuntimeWorldRegionPackageDesc> regions;
    std::string target_id;
    std::string runtime_scene_validation_target_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudgetV2 budget;
    std::uint32_t max_resident_packages{0};
    bool safe_point_required{true};
    bool runtime_scene_validation_succeeded{false};
    std::vector<RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids;
};

struct RuntimeWorldRegionStreamingSafePointRowResult {
    RuntimeWorldRegionStreamingActionKind action{RuntimeWorldRegionStreamingActionKind::keep_resident};
    std::string region_id;
    RuntimeResidentPackageMountIdV2 mount_id;
    RuntimePackageStreamingExecutionResult streaming;
    bool committed{false};
};

struct RuntimeWorldRegionStreamingSafePointResult {
    RuntimeWorldRegionStreamingSafePointStatus status{RuntimeWorldRegionStreamingSafePointStatus::invalid_plan};
    std::vector<RuntimeWorldRegionStreamingDiagnostic> diagnostics;
    std::vector<RuntimeWorldRegionStreamingSafePointRowResult> rows;
    std::size_t load_count{0};
    std::size_t keep_count{0};
    std::size_t unload_count{0};
    std::size_t committed_count{0};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Executes reviewed world-region load/unload plan rows through the existing package streaming safe-point primitives.
/// The live mount set and catalog cache are updated only after every row succeeds on a projected view. The helper does
/// not discover packages, infer eviction policy, stream in the background, upload renderer resources, or touch native
/// handles.
[[nodiscard]] RuntimeWorldRegionStreamingSafePointResult
execute_runtime_world_region_streaming_safe_point(IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set,
                                                  RuntimeResidentCatalogCacheV2& catalog_cache,
                                                  const RuntimeWorldRegionStreamingSafePointDesc& desc);

} // namespace mirakana::runtime
