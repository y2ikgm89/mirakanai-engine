// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/job_execution.hpp"
#include "mirakana/platform/byte_range_io.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/renderer/mavg_lod_selection.hpp"
#include "mirakana/runtime/mavg_page_streaming.hpp"
#include "mirakana/runtime/mavg_payload_page_loader.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_rhi/mavg_cluster_streaming_safe_point_adoption.hpp"
#include "mirakana/runtime_rhi/mavg_gpu_memory_residency.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgAutonomousStreamingIoBackendKind : std::uint8_t {
    filesystem_byte_range = 0,
    direct_storage_byte_range,
};

enum class RuntimeMavgAutonomousStreamingSafePointPolicy : std::uint8_t {
    defer = 0,
    adopt_when_loaded,
};

enum class RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_graph_asset,
    missing_graph,
    missing_resident_pages,
    missing_filesystem,
    missing_execution_pool,
    missing_gpu_memory_policy,
    missing_direct_storage_executor,
    state_graph_mismatch,
    lod_selection_failed,
    recency_inference_failed,
    page_streaming_plan_failed,
    payload_page_load_failed,
    background_service_failed,
    gpu_memory_residency_failed,
    safe_point_adoption_failed,
};

struct RuntimeMavgAutonomousStreamingSchedulerDiagnostic {
    RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode code{
        RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::none};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    std::string message;
};

struct RuntimeMavgAutonomousStreamingSchedulerViewState {
    MavgLodViewDesc lod_view;
};

struct RuntimeMavgAutonomousStreamingSchedulerPageHeatRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    float heat{0.0F};
    std::uint64_t last_used_frame{0};
};

struct RuntimeMavgAutonomousStreamingSchedulerState {
    AssetId graph_asset;
    runtime::RuntimeMavgPageStreamingBackgroundServiceState background_service;
    std::vector<runtime::RuntimeMavgPageStreamingRecencyRow> recency_rows;
    std::uint32_t next_mount_id{1};
    std::uint64_t last_frame_index{0};
};

struct RuntimeMavgAutonomousStreamingSchedulerDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    RuntimeMavgAutonomousStreamingSchedulerViewState view;
    const MavgLodResidentPageSet* resident_pages{nullptr};
    std::span<const runtime::RuntimeMavgPageStreamingCandidateRow> candidates;
    IFileSystem* filesystem{nullptr};
    JobExecutionPool* execution_pool{nullptr};
    std::span<const runtime::RuntimeMavgResidentPageMountRow> resident_page_mounts;
    const GpuMemoryPolicyPlan* gpu_memory_policy{nullptr};
    std::span<const runtime::RuntimeResidentPackageMountIdV2> caller_protected_mount_ids;
    std::span<const RuntimeMavgAutonomousStreamingSchedulerPageHeatRow> page_heat;
    std::span<const std::uint32_t> cancelled_page_indices;
    IByteRangeIoExecutor* direct_storage{nullptr};
    RuntimeMavgAutonomousStreamingIoBackendKind io_backend{
        RuntimeMavgAutonomousStreamingIoBackendKind::filesystem_byte_range};
    std::string_view payload_path;
    runtime::RuntimePackageMountOverlay overlay{runtime::RuntimePackageMountOverlay::last_mount_wins};
    std::size_t max_candidate_pages_per_frame{0};
    std::size_t max_queued_pages{0};
    std::size_t max_pending_pages{0};
    std::size_t max_dispatch_pages{0};
    std::size_t max_adopt_pages{0};
    std::uint64_t frame_index{0};
    std::uint64_t scratch_bytes_per_task{64};
    RuntimeMavgAutonomousStreamingSafePointPolicy safe_point_policy{
        RuntimeMavgAutonomousStreamingSafePointPolicy::defer};
};

struct RuntimeMavgAutonomousStreamingSchedulerResult {
    MavgLodSelectionResult lod_selection;
    std::vector<MavgLodPageRequest> generated_page_requests;
    runtime::RuntimeMavgResidentPageUseGenerationResult resident_page_use_generations;
    runtime::RuntimeMavgPageStreamingPlanResult page_streaming_plan;
    runtime::RuntimeMavgPayloadPageLoadResult payload_page_load;
    runtime::RuntimeMavgPageStreamingBackgroundServiceTickResult background_service;
    RuntimeMavgGpuMemoryResidencyResult gpu_memory_residency;
    RuntimeMavgClusterStreamingSafePointAdoptionResult safe_point_adoption;
    std::vector<RuntimeMavgAutonomousStreamingSchedulerDiagnostic> diagnostics;
    std::size_t selected_page_request_count{0};
    std::size_t cancelled_page_count{0};
    std::size_t queued_page_count{0};
    std::size_t dispatched_page_count{0};
    std::size_t adopted_page_count{0};
    std::size_t eviction_candidate_count{0};
    std::size_t pending_page_count_before{0};
    std::size_t pending_page_count_after{0};
    std::size_t duplicate_pending_page_count{0};
    std::size_t payload_loaded_page_count{0};
    bool selected_without_caller_precomputed_requests{false};
    bool used_filesystem_backend_row{false};
    bool used_direct_storage_backend_row{false};
    bool executed_filesystem_payload_io{false};
    bool executed_direct_storage_payload_io{false};
    bool invoked_file_io{false};
    bool executed_background_worker{false};
    bool applied_gpu_memory_pressure_policy{false};
    bool mutated_mount_set{false};
    bool invoked_catalog_refresh{false};
    bool touched_renderer_or_rhi_handles{false};
    bool exposed_native_handles{false};
    bool proved_async_overlap_performance{false};
    bool invoked_gpu_upload{false};
    bool executed_backend{false};
    bool bounded_per_frame_work{false};
    bool mavg_autonomous_streaming_scheduler_ready{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

/// Ticks the value-owned MAVG streaming controller for one frame: it derives page requests from runtime LOD selection,
/// plans bounded package streaming, reads reviewed payload byte ranges, maintains background streaming state, applies
/// value-only GPU memory pressure residency planning, and optionally adopts loaded packages at a caller-owned safe
/// point. The scheduler does not expose native handles, upload GPU data, execute renderer backends, or prove async
/// overlap/performance.
[[nodiscard]] RuntimeMavgAutonomousStreamingSchedulerResult tick_runtime_mavg_autonomous_streaming_scheduler(
    runtime::RuntimeResidentPackageMountSetV2& mount_set, runtime::RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeMavgAutonomousStreamingSchedulerState& state, const RuntimeMavgAutonomousStreamingSchedulerDesc& desc);

[[nodiscard]] bool has_runtime_mavg_autonomous_streaming_scheduler_diagnostic(
    const RuntimeMavgAutonomousStreamingSchedulerResult& result,
    RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
