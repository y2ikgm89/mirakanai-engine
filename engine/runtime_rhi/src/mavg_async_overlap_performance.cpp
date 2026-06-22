// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_async_overlap_performance.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgAsyncOverlapMeasuredPerformanceResult& result,
                    RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode code, std::string message) {
    result.diagnostics.push_back(RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnostic{
        .code = code,
        .message = std::move(message),
    });
}

[[nodiscard]] bool blank(std::string_view value) noexcept {
    return value.empty();
}

[[nodiscard]] bool valid_hash(std::string_view value) noexcept {
    if (value.size() != 64U) {
        return false;
    }
    return std::ranges::all_of(value, [](char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'); });
}

template <typename Selector>
[[nodiscard]] std::uint64_t percentile(std::span<const RuntimeMavgAsyncOverlapTimingSample> samples,
                                       std::size_t percent, Selector selector) {
    std::vector<std::uint64_t> values;
    values.reserve(samples.size());
    for (const auto& sample : samples) {
        values.push_back(selector(sample));
    }
    std::ranges::sort(values);
    const auto index = values.empty() ? 0U : ((values.size() * percent) + 99U) / 100U - 1U;
    return values[index];
}

[[nodiscard]] std::uint32_t improvement_basis_points(std::uint64_t baseline, std::uint64_t optimized) noexcept {
    if (baseline == 0U || optimized >= baseline) {
        return 0U;
    }
    return static_cast<std::uint32_t>(((baseline - optimized) * 10000U) / baseline);
}

[[nodiscard]] std::uint32_t regression_basis_points(std::uint64_t baseline, std::uint64_t candidate) noexcept {
    if (baseline == 0U || candidate <= baseline) {
        return 0U;
    }
    return static_cast<std::uint32_t>(((candidate - baseline) * 10000U) / baseline);
}

[[nodiscard]] std::uint64_t peak_memory(std::span<const RuntimeMavgAsyncOverlapTimingSample> samples) noexcept {
    std::uint64_t peak = 0;
    for (const auto& sample : samples) {
        peak = std::max(peak, sample.memory_peak_bytes);
    }
    return peak;
}

void validate_run(RuntimeMavgAsyncOverlapMeasuredPerformanceResult& result,
                  const RuntimeMavgAsyncOverlapRunEvidence& run, std::size_t required_warmup_frames,
                  std::size_t required_measured_frames, bool require_overlap) {
    if (run.warmup_frame_count < required_warmup_frames) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::insufficient_warmup_frames,
                       "MAVG measured async-overlap performance requires at least 30 warmup frames per run");
    }
    if (run.measured_frame_count < required_measured_frames || run.measured_samples.size() < required_measured_frames) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::insufficient_measured_frames,
                       "MAVG measured async-overlap performance requires at least 120 measured frames per run");
    }
    if (!run.internal_engine_counters_from_non_captured_run) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::missing_internal_counters,
                       "MAVG measured async-overlap performance requires non-captured internal engine counters");
    }
    if (run.timing_window_only_evidence) {
        result.timing_window_only_evidence_rejected = true;
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::timing_window_only_evidence,
                       "MAVG measured async-overlap performance rejects timing-window-only evidence");
    }
    if (run.touched_native_handles) {
        result.touched_native_handles = true;
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::native_handle_claimed,
                       "MAVG measured async-overlap performance must not expose native handles");
    }
    if (run.claimed_broad_optimization) {
        result.claimed_broad_optimization = true;
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::broad_optimization_claimed,
                       "MAVG measured async-overlap performance must not claim broad optimization");
    }

    for (const auto& sample : run.measured_samples) {
        if (sample.frame_us == 0U || sample.cpu_frame_us == 0U || sample.gpu_frame_us == 0U ||
            sample.streaming_stall_us == 0U || sample.memory_peak_bytes == 0U) {
            add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::invalid_timing_sample,
                           "MAVG measured async-overlap timing samples must include positive frame, CPU, GPU, stall, "
                           "and memory values");
        }
        if (require_overlap && !sample.io_or_cpu_load_overlap_with_gpu_upload_or_draw) {
            add_diagnostic(
                result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::profiler_missing_overlap,
                "MAVG async-scheduler samples must retain IO or CPU load overlap with GPU upload or draw work");
        }
    }
}

void validate_identity(RuntimeMavgAsyncOverlapMeasuredPerformanceResult& result,
                       const RuntimeMavgAsyncOverlapMeasuredPerformanceDesc& desc) {
    const auto& baseline = desc.sync_baseline;
    const auto& async = desc.async_scheduler;

    if (blank(baseline.workload_id) || blank(async.workload_id)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::invalid_workload_id,
                       "MAVG measured async-overlap workload id must be present for both runs");
    }
    if (!valid_hash(baseline.package_hash_sha256) || !valid_hash(async.package_hash_sha256)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::invalid_package_hash,
                       "MAVG measured async-overlap package hash must be a lowercase SHA-256 hex string");
    }
    if (blank(baseline.camera_script_id) || blank(async.camera_script_id)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::invalid_camera_script_id,
                       "MAVG measured async-overlap camera script id must be present for both runs");
    }
    if (blank(baseline.backend) || blank(async.backend)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::invalid_backend,
                       "MAVG measured async-overlap backend must be present for both runs");
    }
    if (blank(baseline.host_class) || blank(async.host_class)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::invalid_host_class,
                       "MAVG measured async-overlap host class must be present for both runs");
    }
    if (blank(baseline.adapter_id) || blank(async.adapter_id)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::invalid_adapter_id,
                       "MAVG measured async-overlap adapter id must be present for both runs");
    }

    const auto comparable = baseline.workload_id == async.workload_id &&
                            baseline.package_hash_sha256 == async.package_hash_sha256 &&
                            baseline.camera_script_id == async.camera_script_id && baseline.backend == async.backend &&
                            baseline.host_class == async.host_class && baseline.adapter_id == async.adapter_id;
    if (!comparable) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::comparable_run_mismatch,
                       "MAVG measured async-overlap sync_baseline and async_scheduler runs must share package, camera, "
                       "backend, host, and adapter identity");
    }
    result.comparable_runs = comparable;

    if (blank(baseline.replay_hash) || baseline.replay_hash != async.replay_hash) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::replay_hash_mismatch,
                       "MAVG measured async-overlap visual replay hashes must match");
    } else {
        result.replay_hash_match = true;
    }
}

void validate_profiler(RuntimeMavgAsyncOverlapMeasuredPerformanceResult& result,
                       const RuntimeMavgAsyncOverlapMeasuredPerformanceDesc& desc) {
    const auto& artifact = desc.profiler_artifact;
    const auto& async = desc.async_scheduler;
    if (artifact.tool == RuntimeMavgAsyncOverlapMeasuredPerformanceProfilerTool::unknown ||
        blank(artifact.artifact_id) || blank(artifact.artifact_path) || blank(artifact.trace_event_json_path) ||
        blank(artifact.tool_version) || blank(artifact.capture_mode) || blank(artifact.driver_version)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::missing_profiler_artifact,
                       "MAVG measured async-overlap performance requires a concrete external profiler artifact row");
    }
    if (!artifact.reviewed) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::profiler_artifact_not_reviewed,
                       "MAVG measured async-overlap profiler artifact must be reviewed");
    }
    if (!artifact.official_tool) {
        add_diagnostic(
            result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::profiler_artifact_not_official,
            "MAVG measured async-overlap profiler artifact must come from an official or reviewed host profiler path");
    }
    const auto artifact_identity_match = artifact.workload_id == async.workload_id &&
                                         artifact.package_hash_sha256 == async.package_hash_sha256 &&
                                         artifact.backend == async.backend && artifact.host_class == async.host_class &&
                                         artifact.adapter_id == async.adapter_id;
    if (!artifact_identity_match) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::comparable_run_mismatch,
                       "MAVG measured async-overlap profiler artifact must match async-scheduler workload, package, "
                       "backend, host, and adapter");
    }
    if (artifact.dropped_timestamps_or_overflow || !artifact.representative_capture ||
        artifact.profiler_overhead_basis_points > desc.maximum_profiler_overhead_basis_points ||
        artifact.capture_duration_us == 0U) {
        add_diagnostic(result,
                       RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::profiler_capture_not_representative,
                       "MAVG measured async-overlap profiler capture must be representative and below overhead limits");
    }
    if (!artifact.queue_timeline_available || !artifact.io_timeline_available || !artifact.gpu_timeline_available ||
        !artifact.memory_timeline_available || !artifact.symbols_or_debug_info_available) {
        add_diagnostic(result,
                       RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::profiler_missing_required_timelines,
                       "MAVG measured async-overlap profiler artifact must retain queue, IO, GPU, memory, and "
                       "symbol/debug metadata");
    }
    if (!artifact.overlap_with_gpu_upload_or_draw_observed) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::profiler_missing_overlap,
                       "MAVG measured async-overlap profiler artifact must show IO or CPU load overlap with GPU upload "
                       "or draw work");
    }

    result.external_profiler_artifact_reviewed = artifact.reviewed && artifact.official_tool;
    result.profiler_overlap_evidence_ready =
        artifact.queue_timeline_available && artifact.io_timeline_available && artifact.gpu_timeline_available &&
        artifact.memory_timeline_available && artifact.overlap_with_gpu_upload_or_draw_observed &&
        artifact.representative_capture && !artifact.dropped_timestamps_or_overflow &&
        artifact.profiler_overhead_basis_points <= desc.maximum_profiler_overhead_basis_points;
}

} // namespace

RuntimeMavgAsyncOverlapMeasuredPerformanceResult
evaluate_runtime_mavg_async_overlap_measured_performance(const RuntimeMavgAsyncOverlapMeasuredPerformanceDesc& desc) {
    RuntimeMavgAsyncOverlapMeasuredPerformanceResult result;
    result.workload_id = std::string(desc.async_scheduler.workload_id);
    result.package_hash_sha256 = std::string(desc.async_scheduler.package_hash_sha256);
    result.camera_script_id = std::string(desc.async_scheduler.camera_script_id);
    result.backend = std::string(desc.async_scheduler.backend);
    result.host_class = std::string(desc.async_scheduler.host_class);
    result.adapter_id = std::string(desc.async_scheduler.adapter_id);
    result.sync_baseline_sample_count = desc.sync_baseline.measured_samples.size();
    result.async_scheduler_sample_count = desc.async_scheduler.measured_samples.size();

    if (blank(desc.sync_baseline.run_id)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::missing_sync_baseline,
                       "MAVG measured async-overlap requires a sync_baseline run row");
    }
    if (blank(desc.async_scheduler.run_id)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::missing_async_scheduler,
                       "MAVG measured async-overlap requires an async_scheduler run row");
    }

    validate_identity(result, desc);
    validate_run(result, desc.sync_baseline, desc.required_warmup_frames, desc.required_measured_frames, false);
    validate_run(result, desc.async_scheduler, desc.required_warmup_frames, desc.required_measured_frames, true);
    validate_profiler(result, desc);

    result.internal_non_captured_counters_ready = desc.sync_baseline.internal_engine_counters_from_non_captured_run &&
                                                  desc.async_scheduler.internal_engine_counters_from_non_captured_run &&
                                                  !desc.sync_baseline.timing_window_only_evidence &&
                                                  !desc.async_scheduler.timing_window_only_evidence;

    if (desc.sync_baseline.measured_samples.size() >= desc.required_measured_frames &&
        desc.async_scheduler.measured_samples.size() >= desc.required_measured_frames) {
        result.sync_baseline_frame_p95_us =
            percentile(desc.sync_baseline.measured_samples, 95U, [](const auto& sample) { return sample.frame_us; });
        result.async_scheduler_frame_p95_us =
            percentile(desc.async_scheduler.measured_samples, 95U, [](const auto& sample) { return sample.frame_us; });
        result.sync_baseline_frame_p99_us =
            percentile(desc.sync_baseline.measured_samples, 99U, [](const auto& sample) { return sample.frame_us; });
        result.async_scheduler_frame_p99_us =
            percentile(desc.async_scheduler.measured_samples, 99U, [](const auto& sample) { return sample.frame_us; });
        result.sync_baseline_streaming_stall_p95_us = percentile(
            desc.sync_baseline.measured_samples, 95U, [](const auto& sample) { return sample.streaming_stall_us; });
        result.async_scheduler_streaming_stall_p95_us = percentile(
            desc.async_scheduler.measured_samples, 95U, [](const auto& sample) { return sample.streaming_stall_us; });
        result.async_scheduler_memory_peak_bytes = peak_memory(desc.async_scheduler.measured_samples);
        result.frame_p95_improvement_basis_points =
            improvement_basis_points(result.sync_baseline_frame_p95_us, result.async_scheduler_frame_p95_us);
        result.streaming_stall_p95_improvement_basis_points = improvement_basis_points(
            result.sync_baseline_streaming_stall_p95_us, result.async_scheduler_streaming_stall_p95_us);
        result.p99_regression_basis_points =
            regression_basis_points(result.sync_baseline_frame_p99_us, result.async_scheduler_frame_p99_us);
    }

    if (desc.memory_budget_bytes == 0U || result.async_scheduler_memory_peak_bytes == 0U ||
        result.async_scheduler_memory_peak_bytes > desc.memory_budget_bytes) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::memory_budget_exceeded,
                       "MAVG measured async-overlap async scheduler memory peak must stay within the selected budget");
    } else {
        result.memory_budget_ready = true;
    }
    if (result.frame_p95_improvement_basis_points < desc.minimum_frame_p95_improvement_basis_points) {
        add_diagnostic(result,
                       RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::insufficient_frame_p95_improvement,
                       "MAVG measured async-overlap p95 frame time improvement is below threshold");
    }
    if (result.streaming_stall_p95_improvement_basis_points < desc.minimum_stall_p95_improvement_basis_points) {
        add_diagnostic(result,
                       RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::insufficient_stall_p95_improvement,
                       "MAVG measured async-overlap p95 streaming stall improvement is below threshold");
    }
    if (result.p99_regression_basis_points > desc.maximum_p99_regression_basis_points) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::excessive_p99_regression,
                       "MAVG measured async-overlap p99 frame regression exceeds threshold");
    }

    result.mavg_async_overlap_measured_performance_ready =
        result.diagnostics.empty() && result.comparable_runs && result.internal_non_captured_counters_ready &&
        result.external_profiler_artifact_reviewed && result.profiler_overlap_evidence_ready &&
        result.replay_hash_match && result.memory_budget_ready && !result.touched_native_handles &&
        !result.claimed_broad_optimization;
    return result;
}

bool has_runtime_mavg_async_overlap_measured_performance_diagnostic(
    const RuntimeMavgAsyncOverlapMeasuredPerformanceResult& result,
    RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics,
                               [code](const RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnostic& diagnostic) {
                                   return diagnostic.code == code;
                               });
}

} // namespace mirakana::runtime_rhi
