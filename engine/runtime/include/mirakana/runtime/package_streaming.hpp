// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/job_execution.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
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
    telemetry_budget_exceeded,
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
    std::uint64_t decompression_time_us{0};
    std::uint64_t cpu_time_us{0};
    std::uint64_t gpu_upload_bytes{0};
    std::uint64_t memory_high_water_bytes{0};
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
    std::uint64_t max_io_bytes_read{0};
    std::uint64_t max_decompressed_bytes{0};
    std::uint64_t max_decompression_time_us{0};
    std::uint64_t max_cpu_time_us{0};
    std::uint64_t max_gpu_upload_bytes{0};
    std::uint64_t max_memory_high_water_bytes{0};
    std::uint32_t max_asset_miss_count{0};
    std::uint32_t max_pop_in_count{0};
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
    std::uint64_t decompression_time_us{0};
    std::uint64_t cpu_time_us{0};
    std::uint64_t gpu_upload_bytes{0};
    std::uint64_t memory_high_water_bytes{0};
    std::uint32_t asset_miss_count{0};
    std::uint32_t pop_in_count{0};
    std::uint64_t io_byte_budget{0};
    std::uint64_t decompressed_byte_budget{0};
    std::uint64_t decompression_time_budget_us{0};
    std::uint64_t cpu_time_budget_us{0};
    std::uint64_t gpu_upload_byte_budget{0};
    std::uint64_t memory_high_water_budget_bytes{0};
    std::uint32_t asset_miss_budget_count{0};
    std::uint32_t pop_in_budget_count{0};
    bool safe_point_required{true};
    bool telemetry_budget_exceeded{false};
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

struct RuntimePackageBackgroundReadDiagnostic {
    std::size_t row_index{0};
    std::string target_id;
    std::string package_index_path;
    std::string code;
    std::string message;
};

struct RuntimePackageBackgroundReadQueueDesc {
    std::span<const RuntimePackageStreamingExecutionDesc> reviewed_rows;
    std::uint64_t frame_index{0};
    std::uint64_t scratch_bytes_per_task{64};
};

struct RuntimePackageBackgroundLoadedRow {
    std::size_t row_index{0};
    RuntimePackageStreamingExecutionDesc desc;
    RuntimePackageCandidateLoadResultV2 candidate_load;
    std::uint32_t worker_id{0};
    bool invoked_candidate_load{false};
};

struct RuntimePackageBackgroundReadQueueResult {
    std::vector<RuntimePackageBackgroundLoadedRow> loaded_rows;
    JobExecutionRunResult execution;
    std::vector<RuntimePackageBackgroundReadDiagnostic> diagnostics;
    std::size_t input_row_count{0};
    std::size_t dispatched_row_count{0};
    std::size_t loaded_row_count{0};
    std::size_t failed_row_count{0};
    bool invoked_file_io{false};
    bool invoked_candidate_load{false};
    bool executed_streaming{false};
    bool executed_background_worker{false};
    bool committed{false};
    bool mutated_mount_set{false};
    bool invoked_catalog_refresh{false};
    bool executed_package_scripts{false};
    bool spawned_external_process{false};
    bool parsed_runtime_source{false};
    bool touched_renderer_or_rhi_handles{false};
    bool invoked_direct_storage{false};
    bool applied_gpu_memory_pressure_policy{false};
    bool proved_async_overlap_performance{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && execution.ready() && failed_row_count == 0U;
    }
};

struct RuntimePackageBackgroundReadServiceState {
    std::vector<RuntimePackageStreamingExecutionDesc> pending_rows;
};

struct RuntimePackageBackgroundReadServiceTickDesc {
    std::span<const RuntimePackageStreamingExecutionDesc> reviewed_rows;
    std::size_t max_pending_rows{0};
    std::size_t max_dispatch_rows{0};
    std::uint64_t frame_index{0};
    std::uint64_t scratch_bytes_per_task{64};
};

struct RuntimePackageBackgroundReadServiceTickResult {
    RuntimePackageBackgroundReadQueueResult dispatch;
    std::vector<RuntimePackageBackgroundLoadedRow> loaded_rows;
    std::vector<RuntimePackageBackgroundReadDiagnostic> diagnostics;
    std::size_t input_row_count{0};
    std::size_t accepted_row_count{0};
    std::size_t duplicate_pending_row_count{0};
    std::size_t budget_dropped_request_count{0};
    std::size_t pending_row_count_before{0};
    std::size_t pending_row_count_after{0};
    std::size_t dispatched_row_count{0};
    std::size_t loaded_row_count{0};
    std::size_t failed_row_count{0};
    bool budget_degraded{false};
    bool invoked_file_io{false};
    bool invoked_candidate_load{false};
    bool executed_streaming{false};
    bool executed_background_worker{false};
    bool committed{false};
    bool mutated_mount_set{false};
    bool invoked_catalog_refresh{false};
    bool executed_package_scripts{false};
    bool spawned_external_process{false};
    bool parsed_runtime_source{false};
    bool touched_renderer_or_rhi_handles{false};
    bool invoked_direct_storage{false};
    bool applied_gpu_memory_pressure_policy{false};
    bool proved_async_overlap_performance{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && failed_row_count == 0U;
    }
};

/// Plans package-residency pressure counters and caller-reviewed LRU eviction order for already-mounted cooked
/// packages. This value-only helper never reads package files, launches background IO, executes scripts, touches
/// renderer/RHI residency, or exposes native handles.
[[nodiscard]] RuntimePackageResidencyPolicyPlan
plan_runtime_package_residency_policy(const RuntimeResidentPackageMountSetV2& mount_set,
                                      const RuntimePackageResidencyPolicyDesc& desc);

/// Dispatches reviewed package descriptors onto a caller-owned first-party job execution pool. The helper performs
/// package candidate file IO on workers and returns loaded package rows for a later caller-owned safe point; it does
/// not mount packages, refresh catalogs, execute scripts/processes, parse runtime source, execute DirectStorage,
/// integrate GPU memory pressure, prove async overlap/performance, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageBackgroundReadQueueResult
dispatch_runtime_package_background_reads(IFileSystem& filesystem, JobExecutionPool& execution_pool,
                                          RuntimePackageBackgroundReadQueueDesc desc);

/// Maintains caller-owned pending package descriptors across frames and dispatches a bounded subset through the
/// reviewed background read queue. Loaded rows are returned for later caller-owned safe-point adoption. The service
/// does not mount packages, refresh catalogs, execute scripts/processes, parse runtime source, execute DirectStorage,
/// integrate GPU memory pressure, prove async overlap/performance, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageBackgroundReadServiceTickResult
tick_runtime_package_background_read_service(IFileSystem& filesystem, JobExecutionPool& execution_pool,
                                             RuntimePackageBackgroundReadServiceState& state,
                                             RuntimePackageBackgroundReadServiceTickDesc desc);

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
