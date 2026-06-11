// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime_rhi/mavg_cluster_streaming_residency_closeout.hpp"
#include "mirakana/runtime_rhi/mavg_cluster_streaming_safe_point_adoption.hpp"
#include "mirakana/runtime_rhi/mavg_streamed_cluster_gpu_upload.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_graph_asset,
    missing_closeout,
    closeout_failed,
    closeout_missing_background_load_evidence,
    missing_safe_point_adoption,
    safe_point_adoption_not_committed,
    missing_gpu_upload,
    gpu_upload_not_ready,
    missing_source_row_evidence,
    source_row_graph_asset_mismatch,
    invalid_background_window,
    invalid_gpu_upload_window,
    invalid_window_clock_domain,
    window_clock_domain_mismatch,
    missing_window_row_evidence,
    no_temporal_overlap,
    missing_measured_budget_evidence,
};

struct RuntimeMavgStreamingUploadOverlapEvidenceDiagnostic {
    RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode code{
        RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::none};
    AssetId graph_asset;
    std::string message;
};

enum class RuntimeMavgStreamingUploadEvidenceClockDomain : std::uint8_t {
    none = 0,
    caller_monotonic_tick,
};

struct RuntimeMavgStreamingUploadEvidenceWindow {
    RuntimeMavgStreamingUploadEvidenceClockDomain clock_domain{RuntimeMavgStreamingUploadEvidenceClockDomain::none};
    std::uint64_t timeline_id{0};
    std::uint64_t begin_tick{0};
    std::uint64_t end_tick{0};
    std::size_t completed_row_count{0};

    [[nodiscard]] std::uint64_t duration_ticks() const noexcept {
        return end_tick > begin_tick ? end_tick - begin_tick : 0U;
    }
};

struct RuntimeMavgStreamingUploadOverlapEvidenceDesc {
    AssetId graph_asset;
    const RuntimeMavgClusterStreamingResidencyCloseoutResult* closeout{nullptr};
    const RuntimeMavgClusterStreamingSafePointAdoptionResult* safe_point_adoption{nullptr};
    const RuntimeMavgStreamedClusterGpuUploadResult* gpu_upload{nullptr};
    RuntimeMavgStreamingUploadEvidenceWindow background_load_window;
    RuntimeMavgStreamingUploadEvidenceWindow gpu_upload_window;
    bool require_measured_budget_evidence{false};
    bool measured_budget_evidence_ready{false};
};

struct RuntimeMavgStreamingUploadOverlapEvidenceResult {
    std::vector<RuntimeMavgStreamingUploadOverlapEvidenceDiagnostic> diagnostics;
    std::size_t background_loaded_row_count{0};
    std::size_t adopted_page_count{0};
    std::size_t uploaded_page_count{0};
    std::size_t uploaded_cluster_count{0};
    std::uint64_t uploaded_bytes{0};
    std::uint64_t background_load_tick_count{0};
    std::uint64_t gpu_upload_tick_count{0};
    std::uint64_t overlap_tick_count{0};
    bool recorded_temporal_overlap_evidence{false};
    bool executed_background_worker{false};
    bool invoked_gpu_upload{false};
    bool invoked_persistent_streaming_service{false};
    bool invoked_direct_storage{false};
    bool executed_backend{false};
    bool executed_mesh_shader{false};
    bool touched_native_handles{false};
    bool claimed_speedup{false};
    bool proved_async_overlap_performance{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && recorded_temporal_overlap_evidence;
    }
};

/// Records value-only temporal overlap evidence between reviewed MAVG background package load rows and streamed-cluster
/// GPU upload rows. Timing windows must use the same caller-owned monotonic clock domain and timeline id; this helper
/// does not dispatch workers, upload GPU resources, execute a backend, run DirectStorage, mutate residency state, touch
/// native handles, or claim speedup.
[[nodiscard]] RuntimeMavgStreamingUploadOverlapEvidenceResult
plan_runtime_mavg_streaming_upload_overlap_evidence(const RuntimeMavgStreamingUploadOverlapEvidenceDesc& desc);

[[nodiscard]] bool has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
    const RuntimeMavgStreamingUploadOverlapEvidenceResult& result,
    RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
