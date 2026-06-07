// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
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

struct RuntimeMavgResidentPageMountRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    RuntimeResidentPackageMountIdV2 mount_id;
};

struct RuntimeMavgPageStreamingSelectedClusterRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
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
    bool invoked_eviction_plan{false};
    bool inferred_eviction_policy{false};
    bool planned_automatic_eviction_policy{false};
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

/// Reviews caller-provided resident page mounts and eviction candidates before a MAVG page-streaming safe point. The
/// function protects selected cluster pages plus resident fallback ancestors, then delegates to the existing resident
/// package eviction planner. It does not infer eviction policy, read files, mutate mounts, execute background
/// streaming, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimeMavgPageStreamingEvictionReviewResult
review_runtime_mavg_page_streaming_evictions(const RuntimeResidentPackageMountSetV2& mount_set,
                                             const RuntimeMavgPageStreamingEvictionReviewDesc& desc);

/// Builds a deterministic resident-page eviction candidate order when the caller has not supplied reviewed eviction
/// ids. The planner reuses selected/fallback page protection, skips protected mounts, sorts remaining resident page
/// mounts by page index descending then mount id ascending, and delegates to the resident package eviction planner. It
/// does not infer LRU/recency/frequency behavior, read files, mutate mounts, execute background streaming, or touch
/// renderer/RHI/native handles.
[[nodiscard]] RuntimeMavgPageStreamingEvictionReviewResult
plan_runtime_mavg_page_streaming_automatic_evictions(const RuntimeResidentPackageMountSetV2& mount_set,
                                                     const RuntimeMavgPageStreamingAutomaticEvictionPlanDesc& desc);

} // namespace mirakana::runtime
