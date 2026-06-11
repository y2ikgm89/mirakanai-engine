// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/core/job_execution.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeMavgPageStreamingDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    missing_graph,
    missing_resident_pages,
    graph_asset_mismatch,
    invalid_graph,
    request_graph_mismatch,
    unknown_page,
    invalid_priority,
    invalid_reason,
    invalid_candidate,
    duplicate_candidate,
    missing_candidate,
    safe_point_failed,
    selected_graph_mismatch,
    unknown_cluster,
    page_mount_graph_mismatch,
    invalid_page_mount,
    duplicate_page_mount,
    missing_page_mount,
    invalid_protected_mount,
    duplicate_protected_mount,
    eviction_plan_failed,
    recency_graph_mismatch,
    invalid_recency_row,
    duplicate_recency_row,
    missing_recency_row,
    non_monotonic_use_generation,
    missing_background_load_rows,
    background_dispatch_failed,
    background_load_failed,
};

struct RuntimeMavgPageStreamingDiagnostic {
    RuntimeMavgPageStreamingDiagnosticCode code{RuntimeMavgPageStreamingDiagnosticCode::invalid_graph_asset};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    std::string message;
};

struct RuntimeMavgPageStreamingCandidateRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    RuntimePackageIndexDiscoveryCandidateV2 candidate;
};

struct RuntimeMavgPageStreamingPlanDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    const MavgLodResidentPageSet* resident_pages{nullptr};
    std::size_t max_queued_pages{0};
};

struct RuntimeMavgPageStreamingPlanRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    float priority{0.0F};
    std::string reason;
    RuntimePackageIndexDiscoveryCandidateV2 candidate;
    std::size_t duplicate_count{0};
};

struct RuntimeMavgPageStreamingPlanResult {
    std::vector<RuntimeMavgPageStreamingPlanRow> queued_page_requests;
    std::vector<RuntimeMavgPageStreamingDiagnostic> diagnostics;
    std::size_t input_request_count{0};
    std::size_t resident_skip_count{0};
    std::size_t duplicate_request_count{0};
    std::size_t missing_candidate_count{0};
    std::size_t budget_dropped_request_count{0};
    bool budget_degraded{false};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool executed_streaming{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RuntimeMavgPageStreamingDrainDesc {
    RuntimeMavgPageStreamingPlanRow row;
    RuntimeResidentPackageMountIdV2 mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudgetV2 budget{};
    std::vector<RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids;
};

struct RuntimeMavgPageStreamingDrainResult {
    RuntimeMavgPageStreamingPlanRow row;
    RuntimePackageCandidateResidentMountReviewedEvictionsResultV2 mount_result;
    std::vector<RuntimeMavgPageStreamingDiagnostic> diagnostics;
    bool invoked_candidate_load{false};
    bool invoked_eviction_plan{false};
    bool invoked_catalog_refresh{false};
    bool committed{false};
    bool executed_safe_point{false};
    bool executed_background_worker{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && mount_result.succeeded();
    }
};

struct RuntimeMavgPageStreamingBackgroundLoadDesc {
    std::span<const RuntimeMavgPageStreamingPlanRow> rows;
    std::uint64_t frame_index{0};
    std::uint64_t scratch_bytes_per_task{64};
};

struct RuntimeMavgPageStreamingBackgroundLoadedRow {
    RuntimeMavgPageStreamingPlanRow row;
    RuntimePackageCandidateLoadResultV2 candidate_load;
    std::uint32_t worker_id{0};
    bool invoked_candidate_load{false};
};

struct RuntimeMavgPageStreamingBackgroundLoadResult {
    std::vector<RuntimeMavgPageStreamingBackgroundLoadedRow> loaded_rows;
    JobExecutionRunResult execution;
    std::vector<RuntimeMavgPageStreamingDiagnostic> diagnostics;
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
    bool touched_renderer_or_rhi_handles{false};
    bool invoked_direct_storage{false};
    bool applied_gpu_memory_pressure_policy{false};
    bool proved_async_overlap_performance{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && execution.ready() && failed_row_count == 0U;
    }
};

struct RuntimeMavgPageStreamingBackgroundServiceState {
    AssetId graph_asset;
    std::vector<RuntimeMavgPageStreamingPlanRow> pending_rows;
};

struct RuntimeMavgPageStreamingBackgroundServiceTickDesc {
    AssetId graph_asset;
    std::span<const RuntimeMavgPageStreamingPlanRow> reviewed_rows;
    std::size_t max_pending_pages{0};
    std::size_t max_dispatch_pages{0};
    std::uint64_t frame_index{0};
    std::uint64_t scratch_bytes_per_task{64};
};

struct RuntimeMavgPageStreamingBackgroundServiceTickResult {
    RuntimeMavgPageStreamingBackgroundLoadResult dispatch;
    std::vector<RuntimeMavgPageStreamingBackgroundLoadedRow> loaded_rows;
    std::vector<RuntimeMavgPageStreamingDiagnostic> diagnostics;
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
    bool touched_renderer_or_rhi_handles{false};
    bool invoked_direct_storage{false};
    bool applied_gpu_memory_pressure_policy{false};
    bool proved_async_overlap_performance{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && failed_row_count == 0U;
    }
};

struct RuntimeMavgResidentPageMountRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    RuntimeResidentPackageMountIdV2 mount_id;
};

struct RuntimeMavgPageStreamingSelectedClusterRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
};

enum class RuntimeMavgPageStreamingAutomaticEvictionPolicyKind : std::uint8_t {
    deterministic_page_index = 0,
    caller_supplied_recency,
};

struct RuntimeMavgPageStreamingRecencyRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    RuntimeResidentPackageMountIdV2 mount_id;
    std::uint64_t resident_page_last_used_generation{0};
};

struct RuntimeMavgResidentPageUseGenerationDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    std::span<const RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters;
    std::span<const RuntimeMavgResidentPageMountRow> resident_page_mounts;
    std::span<const RuntimeMavgPageStreamingRecencyRow> previous_recency_rows;
    std::uint64_t current_use_generation{0};
};

struct RuntimeMavgResidentPageUseGenerationResult {
    std::vector<RuntimeMavgPageStreamingRecencyRow> recency_rows;
    std::vector<RuntimeMavgPageStreamingDiagnostic> diagnostics;
    std::size_t input_resident_page_mount_count{0};
    std::size_t input_selected_cluster_count{0};
    std::size_t output_recency_row_count{0};
    std::size_t touched_resident_page_count{0};
    std::size_t carried_recency_row_count{0};
    std::size_t new_resident_page_count{0};
    std::size_t dropped_nonresident_recency_row_count{0};
    std::size_t duplicate_recency_row_count{0};
    std::size_t missing_page_mount_count{0};
    bool inferred_resident_page_use_generation{false};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RuntimeMavgPageStreamingEvictionReviewDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    std::span<const RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters;
    std::span<const RuntimeMavgResidentPageMountRow> resident_page_mounts;
    std::span<const RuntimeResidentPackageMountIdV2> reviewed_candidate_unmount_order;
    std::span<const RuntimeResidentPackageMountIdV2> caller_protected_mount_ids;
    RuntimeResourceResidencyBudgetV2 target_budget{};
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
};

struct RuntimeMavgPageStreamingAutomaticEvictionPlanDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    std::span<const RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters;
    std::span<const RuntimeMavgResidentPageMountRow> resident_page_mounts;
    RuntimeMavgPageStreamingAutomaticEvictionPolicyKind policy_kind{
        RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::deterministic_page_index};
    std::span<const RuntimeMavgPageStreamingRecencyRow> recency_rows;
    std::span<const RuntimeResidentPackageMountIdV2> caller_protected_mount_ids;
    RuntimeResourceResidencyBudgetV2 target_budget{};
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
};

struct RuntimeMavgPageStreamingEvictionReviewResult {
    std::vector<RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids;
    RuntimeResidentPackageEvictionPlanResultV2 eviction_plan;
    std::vector<RuntimeMavgPageStreamingDiagnostic> diagnostics;
    std::size_t protected_visible_page_count{0};
    std::size_t protected_fallback_page_count{0};
    std::size_t duplicate_protected_mount_count{0};
    std::size_t protected_eviction_candidate_skip_count{0};
    std::size_t automatic_eviction_candidate_count{0};
    std::size_t recency_eviction_candidate_count{0};
    std::size_t missing_recency_row_count{0};
    std::size_t duplicate_recency_row_count{0};
    bool invoked_eviction_plan{false};
    bool inferred_eviction_policy{false};
    bool planned_automatic_eviction_policy{false};
    bool applied_caller_supplied_recency_policy{false};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && eviction_plan.succeeded();
    }
};

/// Pure planner over caller-reviewed MAVG page requests and package candidates. `candidates` may contain rows for
/// multiple graphs; only rows matching `desc.graph_asset` and the requested page participate. Matching candidates must
/// satisfy the same relative `.geindex`, optional relative content root, and non-empty relative label shape required by
/// `load_runtime_package_candidate_v2`. This function does not read files, mutate resident state, execute streaming, or
/// touch renderer/RHI/native handles.
[[nodiscard]] RuntimeMavgPageStreamingPlanResult
plan_runtime_mavg_page_streaming_requests(const RuntimeMavgPageStreamingPlanDesc& desc,
                                          std::span<const MavgLodPageRequest> page_requests,
                                          std::span<const RuntimeMavgPageStreamingCandidateRow> candidates);

[[nodiscard]] RuntimeMavgPageStreamingDrainResult execute_runtime_mavg_page_streaming_request_safe_point(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeMavgPageStreamingDrainDesc desc);

/// Dispatches reviewed MAVG page package candidate loads onto a caller-owned first-party job execution pool. The
/// helper performs package candidate file IO on workers and returns loaded package rows for a later caller-owned safe
/// point; it does not mount packages, refresh catalogs, execute DirectStorage, integrate GPU memory pressure, prove
/// async overlap/performance, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimeMavgPageStreamingBackgroundLoadResult
dispatch_runtime_mavg_page_streaming_background_loads(IFileSystem& filesystem, JobExecutionPool& execution_pool,
                                                      RuntimeMavgPageStreamingBackgroundLoadDesc desc);

/// Maintains caller-owned pending MAVG page rows across frames and dispatches a bounded subset through the existing
/// background package loader. The service validates graph ownership before mutating state, coalesces duplicate pending
/// pages, and returns loaded rows for later caller-owned safe-point adoption. It does not mount packages, refresh
/// catalogs, execute DirectStorage, integrate GPU memory pressure, prove async overlap/performance, or touch
/// renderer/RHI/native handles.
[[nodiscard]] RuntimeMavgPageStreamingBackgroundServiceTickResult
tick_runtime_mavg_page_streaming_background_service(IFileSystem& filesystem, JobExecutionPool& execution_pool,
                                                    RuntimeMavgPageStreamingBackgroundServiceState& state,
                                                    RuntimeMavgPageStreamingBackgroundServiceTickDesc desc);

/// Reviews caller-provided resident page mounts and eviction candidates before a MAVG page-streaming safe point. The
/// function protects selected cluster pages plus resident fallback ancestors, then delegates to the existing resident
/// package eviction planner. It does not infer eviction policy, read files, mutate mounts, execute background
/// streaming, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimeMavgPageStreamingEvictionReviewResult
review_runtime_mavg_page_streaming_evictions(const RuntimeResidentPackageMountSetV2& mount_set,
                                             const RuntimeMavgPageStreamingEvictionReviewDesc& desc);

/// Builds a deterministic resident-page eviction candidate order when the caller has not supplied reviewed eviction
/// ids. The planner reuses selected/fallback page protection, skips protected mounts, and delegates to the resident
/// package eviction planner. The default policy sorts by page index descending then mount id ascending; the
/// caller-supplied recency policy sorts older supplied use generations first. It does not infer LRU/frequency behavior,
/// read files, mutate mounts, execute background streaming, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimeMavgPageStreamingEvictionReviewResult
plan_runtime_mavg_page_streaming_automatic_evictions(const RuntimeResidentPackageMountSetV2& mount_set,
                                                     const RuntimeMavgPageStreamingAutomaticEvictionPlanDesc& desc);

/// Builds side-effect-free resident page use-generation rows from selected cluster pages, current resident page mounts,
/// and caller-retained previous recency rows. Selected resident pages receive `current_use_generation`, retained
/// unselected pages keep their previous generation, new resident pages start cold at zero, and nonresident previous
/// rows are dropped. This does not read files, mutate mounts, execute streaming, infer eviction order, or touch
/// renderer/RHI handles.
[[nodiscard]] RuntimeMavgResidentPageUseGenerationResult
infer_runtime_mavg_resident_page_use_generations(const RuntimeResidentPackageMountSetV2& mount_set,
                                                 const RuntimeMavgResidentPageUseGenerationDesc& desc);

} // namespace mirakana::runtime
