// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimePackageStreamingExecutionMode : std::uint8_t {
    planning_only = 0,
    host_gated_safe_point,
};

enum class RuntimePackageStreamingExecutionStatus : std::uint8_t {
    invalid_descriptor = 0,
    validation_preflight_required,
    package_load_failed,
    residency_hint_failed,
    over_budget_intent,
    safe_point_replacement_failed,
    committed,
    resident_mount_failed,
    resident_unmount_failed,
    resident_replace_failed,
    resident_catalog_refresh_failed,
    resident_eviction_plan_failed,
};

enum class RuntimePackageResidencyPolicyStatus : std::uint8_t {
    invalid_descriptor = 0,
    within_budget,
    eviction_required,
    budget_unreachable,
    unsupported_request,
};

struct RuntimePackageStreamingExecutionDiagnostic {
    std::string code;
    std::string message;
};

struct RuntimePackageResidencyPolicyDiagnostic {
    RuntimeResidentPackageMountIdV2 mount_id;
    std::string code;
    std::string message;
};

struct RuntimePackageResidencyTelemetryRow {
    RuntimeResidentPackageMountIdV2 mount_id;
    std::uint64_t last_touched_frame{0};
    std::uint64_t io_bytes_read{0};
    std::uint64_t decompressed_bytes{0};
    std::uint64_t cpu_time_us{0};
    std::uint64_t gpu_upload_bytes{0};
    std::uint32_t asset_miss_count{0};
    std::uint32_t pop_in_count{0};
};

struct RuntimePackageResidencyPolicyDesc {
    std::uint64_t max_resident_content_bytes{0};
    std::size_t max_resident_asset_records{0};
    std::uint32_t max_resident_packages{0};
    bool safe_point_required{true};
    std::vector<RuntimePackageResidencyTelemetryRow> telemetry_rows;
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids;
    bool request_background_read_execution{false};
    bool request_package_script_execution{false};
    bool request_external_process{false};
    bool request_runtime_source_parsing{false};
    bool request_renderer_rhi_residency{false};
    bool request_native_handle_access{false};
};

struct RuntimePackageResidencyPolicyRow {
    RuntimeResidentPackageMountIdV2 mount_id;
    std::string label;
    std::uint64_t resident_bytes{0};
    std::size_t resident_asset_records{0};
    std::uint64_t last_touched_frame{0};
    bool protected_from_eviction{false};
    bool eviction_candidate{false};
    bool recommended_eviction{false};
};

struct RuntimePackageResidencyPolicyPlan {
    RuntimePackageResidencyPolicyStatus status{RuntimePackageResidencyPolicyStatus::invalid_descriptor};
    std::vector<RuntimePackageResidencyPolicyRow> rows;
    std::vector<RuntimeResidentPackageMountIdV2> lru_candidate_mount_ids;
    std::vector<RuntimeResidentPackageMountIdV2> recommended_eviction_mount_ids;
    std::vector<RuntimePackageResidencyPolicyDiagnostic> diagnostics;
    std::uint64_t estimated_resident_bytes{0};
    std::uint64_t content_budget_bytes{0};
    std::size_t resident_asset_record_count{0};
    std::size_t asset_record_budget_count{0};
    std::uint32_t resident_package_count{0};
    std::uint32_t package_budget_count{0};
    std::uint64_t io_bytes_read{0};
    std::uint64_t decompressed_bytes{0};
    std::uint64_t cpu_time_us{0};
    std::uint64_t gpu_upload_bytes{0};
    std::uint32_t asset_miss_count{0};
    std::uint32_t pop_in_count{0};
    bool safe_point_required{true};
    bool background_read_execution_invoked{false};
    bool package_script_execution_invoked{false};
    bool external_process_invoked{false};
    bool runtime_source_parsing_invoked{false};
    bool renderer_rhi_residency_invoked{false};
    bool native_handle_exposed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimePackageStreamingExecutionDesc {
    std::string target_id;
    std::string package_index_path;
    std::string content_root;
    std::string runtime_scene_validation_target_id;
    RuntimePackageStreamingExecutionMode mode{RuntimePackageStreamingExecutionMode::planning_only};
    std::uint64_t resident_budget_bytes{0};
    bool safe_point_required{true};
    bool runtime_scene_validation_succeeded{false};
    std::vector<AssetId> required_preload_assets;
    std::vector<AssetKind> resident_resource_kinds;
    std::uint32_t max_resident_packages{0};
};

struct RuntimePackageStreamingExecutionResult {
    RuntimePackageStreamingExecutionStatus status{RuntimePackageStreamingExecutionStatus::invalid_descriptor};
    std::string target_id;
    std::string package_index_path;
    std::string runtime_scene_validation_target_id;
    std::uint64_t estimated_resident_bytes{0};
    std::uint64_t resident_budget_bytes{0};
    std::uint32_t required_preload_asset_count{0};
    std::uint32_t resident_resource_kind_count{0};
    std::uint32_t resident_package_count{0};
    std::uint32_t resident_mount_generation{0};
    RuntimePackageSafePointReplacementResult replacement;
    RuntimePackageCandidateLoadResultV2 candidate_load;
    RuntimeResidentPackageMountResultV2 resident_mount;
    RuntimeResidentPackageUnmountCommitResultV2 resident_unmount;
    RuntimeResidentPackageReplaceCommitResultV2 resident_replace;
    RuntimeResidentPackageEvictionPlanResultV2 eviction_plan;
    RuntimeResidentCatalogCacheRefreshResultV2 resident_catalog_refresh;
    std::vector<RuntimePackageStreamingExecutionDiagnostic> diagnostics;
    std::size_t evicted_mount_count{0};
    bool invoked_eviction_plan{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans package-residency pressure counters and caller-reviewed LRU eviction order for already-mounted cooked
/// packages. This value-only helper never reads package files, launches background IO, executes scripts, touches
/// renderer/RHI residency, or exposes native handles.
[[nodiscard]] RuntimePackageResidencyPolicyPlan
plan_runtime_package_residency_policy(const RuntimeResidentPackageMountSetV2& mount_set,
                                      const RuntimePackageResidencyPolicyDesc& desc);

[[nodiscard]] RuntimePackageStreamingExecutionResult execute_selected_runtime_package_streaming_safe_point(
    RuntimeAssetPackageStore& store, RuntimeResourceCatalogV2& catalog,
    const RuntimePackageStreamingExecutionDesc& desc, RuntimeAssetPackageLoadResult loaded_package);

[[nodiscard]] RuntimePackageStreamingExecutionResult execute_selected_runtime_package_streaming_safe_point(
    RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc, RuntimeAssetPackageLoadResult loaded_package);

[[nodiscard]] RuntimePackageStreamingExecutionResult
execute_selected_runtime_package_streaming_candidate_resident_mount_safe_point(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc);

/// Builds the descriptor-selected package candidate, delegates projected resident mounting and caller-reviewed
/// evictions to the reviewed candidate helper, validates selected streaming residency hints, and commits the live
/// mount/cache state only after every projected step succeeds.
[[nodiscard]] RuntimePackageStreamingExecutionResult
execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc,
    std::vector<RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order,
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids);

[[nodiscard]] RuntimePackageStreamingExecutionResult
execute_selected_runtime_package_streaming_resident_replace_safe_point(RuntimeResidentPackageMountSetV2& mount_set,
                                                                       RuntimeResidentCatalogCacheV2& catalog_cache,
                                                                       RuntimeResidentPackageMountIdV2 mount_id,
                                                                       RuntimePackageMountOverlay overlay,
                                                                       const RuntimePackageStreamingExecutionDesc& desc,
                                                                       RuntimeAssetPackageLoadResult loaded_package);

[[nodiscard]] RuntimePackageStreamingExecutionResult
execute_selected_runtime_package_streaming_candidate_resident_replace_safe_point(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc);

/// Builds the descriptor-selected package candidate, delegates projected resident replacement and caller-reviewed
/// evictions to the reviewed candidate helper, validates selected streaming residency hints, and commits the live
/// mount/cache state only after every projected step succeeds.
[[nodiscard]] RuntimePackageStreamingExecutionResult
execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc,
    std::vector<RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order,
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids);

[[nodiscard]] RuntimePackageStreamingExecutionResult
execute_selected_runtime_package_streaming_resident_unmount_safe_point(
    RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc);

} // namespace mirakana::runtime
