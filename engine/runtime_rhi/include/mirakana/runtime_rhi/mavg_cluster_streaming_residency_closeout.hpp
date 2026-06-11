// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/core/job_execution.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/renderer/gpu_memory_policy.hpp"
#include "mirakana/runtime/mavg_page_streaming.hpp"
#include "mirakana/runtime/mavg_payload_page_loader.hpp"
#include "mirakana/runtime_rhi/mavg_gpu_memory_residency.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_graph_asset,
    missing_graph,
    missing_filesystem,
    missing_execution_pool,
    page_streaming_plan_failed,
    payload_page_load_failed,
    background_load_failed,
    gpu_memory_residency_failed,
};

struct RuntimeMavgClusterStreamingResidencyCloseoutDiagnostic {
    RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode code{
        RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::none};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    std::string message;
};

struct RuntimeMavgClusterStreamingResidencyCloseoutDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    const MavgLodResidentPageSet* resident_pages{nullptr};
    std::span<const MavgLodPageRequest> page_requests;
    std::span<const runtime::RuntimeMavgPageStreamingCandidateRow> candidates;
    IFileSystem* filesystem{nullptr};
    JobExecutionPool* execution_pool{nullptr};
    std::string_view payload_path;
    const GpuMemoryPolicyPlan* gpu_memory_policy{nullptr};
    std::span<const runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters;
    std::span<const runtime::RuntimeMavgResidentPageMountRow> resident_page_mounts;
    std::span<const runtime::RuntimeMavgPageStreamingRecencyRow> recency_rows;
    std::span<const runtime::RuntimeResidentPackageMountIdV2> caller_protected_mount_ids;
    runtime::RuntimePackageMountOverlay overlay{runtime::RuntimePackageMountOverlay::last_mount_wins};
    runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind eviction_policy_kind{
        runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency};
    std::size_t max_queued_pages{0};
    std::uint64_t frame_index{0};
    std::uint64_t scratch_bytes_per_task{64};
};

struct RuntimeMavgClusterStreamingResidencyCloseoutResult {
    runtime::RuntimeMavgPageStreamingPlanResult page_streaming_plan;
    runtime::RuntimeMavgPayloadPageLoadResult payload_page_load;
    runtime::RuntimeMavgPageStreamingBackgroundLoadResult background_load;
    RuntimeMavgGpuMemoryResidencyResult gpu_memory_residency;
    std::vector<RuntimeMavgClusterStreamingResidencyCloseoutDiagnostic> diagnostics;
    std::size_t queued_page_request_count{0};
    std::size_t payload_loaded_page_count{0};
    std::size_t background_loaded_row_count{0};
    std::size_t protected_visible_page_count{0};
    std::size_t protected_fallback_page_count{0};
    std::size_t deterministic_degradation_row_count{0};
    bool applied_gpu_memory_pressure_policy{false};
    bool preserved_visible_geometry_without_holes{false};
    bool invoked_file_io{false};
    bool executed_background_worker{false};
    bool mutated_mount_set{false};
    bool invoked_catalog_refresh{false};
    bool invoked_direct_storage{false};
    bool invoked_gpu_upload{false};
    bool executed_backend{false};
    bool touched_renderer_or_rhi_handles{false};
    bool proved_async_overlap_performance{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && page_streaming_plan.succeeded() && payload_page_load.succeeded() &&
               background_load.succeeded() && gpu_memory_residency.succeeded();
    }
};

/// Composes the reviewed Phase 5 MAVG streaming/residency evidence lane: page request planning, filesystem byte-range
/// payload reads, background package candidate loads, and GPU memory pressure resident byte-budget eviction planning.
/// This is a value-level closeout helper and does not execute safe-point resident mount mutation, catalog refresh,
/// DirectStorage, GPU upload, backend rendering, or async-overlap/performance proof.
[[nodiscard]] RuntimeMavgClusterStreamingResidencyCloseoutResult
plan_runtime_mavg_cluster_streaming_residency_closeout(const runtime::RuntimeResidentPackageMountSetV2& mount_set,
                                                       const RuntimeMavgClusterStreamingResidencyCloseoutDesc& desc);

[[nodiscard]] bool has_runtime_mavg_cluster_streaming_residency_closeout_diagnostic(
    const RuntimeMavgClusterStreamingResidencyCloseoutResult& result,
    RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
