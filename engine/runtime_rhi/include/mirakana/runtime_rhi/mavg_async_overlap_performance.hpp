// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgAsyncOverlapMeasuredPerformanceProfilerTool : std::uint8_t {
    unknown = 0,
    pix_timing_capture,
    nsight_graphics_gpu_trace,
    radeon_gpu_profiler,
    intel_gpa,
    apple_metal_tools,
};

enum class RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_workload_id,
    invalid_package_hash,
    invalid_camera_script_id,
    invalid_backend,
    invalid_host_class,
    invalid_adapter_id,
    missing_sync_baseline,
    missing_async_scheduler,
    comparable_run_mismatch,
    insufficient_warmup_frames,
    insufficient_measured_frames,
    missing_internal_counters,
    timing_window_only_evidence,
    invalid_timing_sample,
    missing_profiler_artifact,
    profiler_artifact_not_reviewed,
    profiler_artifact_not_official,
    profiler_capture_not_representative,
    profiler_missing_required_timelines,
    profiler_missing_overlap,
    replay_hash_mismatch,
    memory_budget_exceeded,
    insufficient_frame_p95_improvement,
    insufficient_stall_p95_improvement,
    excessive_p99_regression,
    native_handle_claimed,
    broad_optimization_claimed,
};

struct RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnostic {
    RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode code{
        RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::none};
    std::string message;
};

struct RuntimeMavgAsyncOverlapTimingSample {
    std::uint64_t frame_us{0};
    std::uint64_t cpu_frame_us{0};
    std::uint64_t gpu_frame_us{0};
    std::uint64_t io_us{0};
    std::uint64_t upload_us{0};
    std::uint64_t streaming_stall_us{0};
    std::uint64_t memory_peak_bytes{0};
    bool queue_overlap_observed{false};
    bool io_or_cpu_load_overlap_with_gpu_upload_or_draw{false};
};

struct RuntimeMavgAsyncOverlapRunEvidence {
    std::string_view run_id;
    std::string_view workload_id;
    std::string_view package_hash_sha256;
    std::string_view camera_script_id;
    std::string_view backend;
    std::string_view host_class;
    std::string_view adapter_id;
    std::string_view driver_version;
    std::string_view replay_hash;
    std::size_t warmup_frame_count{0};
    std::size_t measured_frame_count{0};
    std::span<const RuntimeMavgAsyncOverlapTimingSample> measured_samples;
    bool internal_engine_counters_from_non_captured_run{false};
    bool timing_window_only_evidence{false};
    bool touched_native_handles{false};
    bool claimed_broad_optimization{false};
};

struct RuntimeMavgAsyncOverlapProfilerArtifactEvidence {
    RuntimeMavgAsyncOverlapMeasuredPerformanceProfilerTool tool{
        RuntimeMavgAsyncOverlapMeasuredPerformanceProfilerTool::unknown};
    std::string_view artifact_id;
    std::string_view artifact_path;
    std::string_view trace_event_json_path;
    std::string_view workload_id;
    std::string_view package_hash_sha256;
    std::string_view backend;
    std::string_view host_class;
    std::string_view adapter_id;
    std::string_view driver_version;
    std::string_view tool_version;
    std::string_view capture_mode;
    std::uint32_t profiler_overhead_basis_points{0};
    std::uint64_t capture_duration_us{0};
    bool reviewed{false};
    bool official_tool{false};
    bool symbols_or_debug_info_available{false};
    bool dropped_timestamps_or_overflow{false};
    bool queue_timeline_available{false};
    bool io_timeline_available{false};
    bool gpu_timeline_available{false};
    bool memory_timeline_available{false};
    bool overlap_with_gpu_upload_or_draw_observed{false};
    bool representative_capture{false};
};

struct RuntimeMavgAsyncOverlapMeasuredPerformanceDesc {
    RuntimeMavgAsyncOverlapRunEvidence sync_baseline;
    RuntimeMavgAsyncOverlapRunEvidence async_scheduler;
    RuntimeMavgAsyncOverlapProfilerArtifactEvidence profiler_artifact;
    std::size_t required_warmup_frames{30};
    std::size_t required_measured_frames{120};
    std::uint32_t minimum_frame_p95_improvement_basis_points{500};
    std::uint32_t minimum_stall_p95_improvement_basis_points{2000};
    std::uint32_t maximum_p99_regression_basis_points{200};
    std::uint32_t maximum_profiler_overhead_basis_points{500};
    std::uint64_t memory_budget_bytes{0};
};

struct RuntimeMavgAsyncOverlapMeasuredPerformanceResult {
    std::vector<RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnostic> diagnostics;
    std::string workload_id;
    std::string package_hash_sha256;
    std::string camera_script_id;
    std::string backend;
    std::string host_class;
    std::string adapter_id;
    std::size_t sync_baseline_sample_count{0};
    std::size_t async_scheduler_sample_count{0};
    std::uint64_t sync_baseline_frame_p95_us{0};
    std::uint64_t async_scheduler_frame_p95_us{0};
    std::uint64_t sync_baseline_frame_p99_us{0};
    std::uint64_t async_scheduler_frame_p99_us{0};
    std::uint64_t sync_baseline_streaming_stall_p95_us{0};
    std::uint64_t async_scheduler_streaming_stall_p95_us{0};
    std::uint64_t async_scheduler_memory_peak_bytes{0};
    std::uint32_t frame_p95_improvement_basis_points{0};
    std::uint32_t streaming_stall_p95_improvement_basis_points{0};
    std::uint32_t p99_regression_basis_points{0};
    bool comparable_runs{false};
    bool internal_non_captured_counters_ready{false};
    bool external_profiler_artifact_reviewed{false};
    bool profiler_overlap_evidence_ready{false};
    bool replay_hash_match{false};
    bool memory_budget_ready{false};
    bool timing_window_only_evidence_rejected{true};
    bool touched_native_handles{false};
    bool claimed_broad_optimization{false};
    bool mavg_async_overlap_measured_performance_ready{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && mavg_async_overlap_measured_performance_ready;
    }
};

/// Evaluates official-profiler-backed MAVG async overlap performance evidence. This function does not capture live
/// profiler data or expose native handles; it only promotes the measured row when caller-owned internal non-captured
/// counters and a reviewed external profiler artifact prove the exact workload/backend/host identity.
[[nodiscard]] RuntimeMavgAsyncOverlapMeasuredPerformanceResult
evaluate_runtime_mavg_async_overlap_measured_performance(const RuntimeMavgAsyncOverlapMeasuredPerformanceDesc& desc);

[[nodiscard]] bool has_runtime_mavg_async_overlap_measured_performance_diagnostic(
    const RuntimeMavgAsyncOverlapMeasuredPerformanceResult& result,
    RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
