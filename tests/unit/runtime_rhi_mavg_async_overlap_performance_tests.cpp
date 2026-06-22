// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_async_overlap_performance.hpp"

#include <string>
#include <vector>

namespace {

constexpr std::string_view kPackageHash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

[[nodiscard]] std::vector<mirakana::runtime_rhi::RuntimeMavgAsyncOverlapTimingSample>
make_samples(std::uint64_t frame_us, std::uint64_t stall_us, std::uint64_t memory_peak_bytes, bool overlap) {
    std::vector<mirakana::runtime_rhi::RuntimeMavgAsyncOverlapTimingSample> samples;
    samples.reserve(120);
    for (std::size_t index = 0; index < 120U; ++index) {
        samples.push_back(mirakana::runtime_rhi::RuntimeMavgAsyncOverlapTimingSample{
            .frame_us = frame_us,
            .cpu_frame_us = frame_us - 2000U,
            .gpu_frame_us = frame_us - 3000U,
            .io_us = overlap ? 3000U : 5000U,
            .upload_us = overlap ? 2400U : 4000U,
            .streaming_stall_us = stall_us,
            .memory_peak_bytes = memory_peak_bytes,
            .queue_overlap_observed = overlap,
            .io_or_cpu_load_overlap_with_gpu_upload_or_draw = overlap,
        });
    }
    return samples;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgAsyncOverlapRunEvidence
make_run(std::string_view run_id, std::span<const mirakana::runtime_rhi::RuntimeMavgAsyncOverlapTimingSample> samples,
         std::string_view replay_hash, bool timing_window_only = false) {
    return mirakana::runtime_rhi::RuntimeMavgAsyncOverlapRunEvidence{
        .run_id = run_id,
        .workload_id = "mavg.stress.package.streaming",
        .package_hash_sha256 = kPackageHash,
        .camera_script_id = "mavg-camera-loop-a",
        .backend = "d3d12",
        .host_class = "windows-d3d12-nvidia",
        .adapter_id = "ven_10de_dev_2684",
        .driver_version = "32.0.15.7652",
        .replay_hash = replay_hash,
        .warmup_frame_count = 30,
        .measured_frame_count = samples.size(),
        .measured_samples = samples,
        .internal_engine_counters_from_non_captured_run = true,
        .timing_window_only_evidence = timing_window_only,
    };
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgAsyncOverlapProfilerArtifactEvidence make_profiler() {
    return mirakana::runtime_rhi::RuntimeMavgAsyncOverlapProfilerArtifactEvidence{
        .tool = mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceProfilerTool::pix_timing_capture,
        .artifact_id = "pix://mavg/async-overlap/perf-proof",
        .artifact_path = "artifacts/mavg/async-overlap-performance/2026-06-22-d3d12-pix/d3d12/"
                         "mavg.stress.package.streaming/pix-timing.wpix",
        .trace_event_json_path = "artifacts/mavg/async-overlap-performance/2026-06-22-d3d12-pix/d3d12/"
                                 "mavg.stress.package.streaming/trace-events.json",
        .workload_id = "mavg.stress.package.streaming",
        .package_hash_sha256 = kPackageHash,
        .backend = "d3d12",
        .host_class = "windows-d3d12-nvidia",
        .adapter_id = "ven_10de_dev_2684",
        .driver_version = "32.0.15.7652",
        .tool_version = "2505.12",
        .capture_mode = "sequential",
        .profiler_overhead_basis_points = 150,
        .capture_duration_us = 2'000'000,
        .reviewed = true,
        .official_tool = true,
        .symbols_or_debug_info_available = true,
        .queue_timeline_available = true,
        .io_timeline_available = true,
        .gpu_timeline_available = true,
        .memory_timeline_available = true,
        .overlap_with_gpu_upload_or_draw_observed = true,
        .representative_capture = true,
    };
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDesc
make_desc(std::span<const mirakana::runtime_rhi::RuntimeMavgAsyncOverlapTimingSample> baseline_samples,
          std::span<const mirakana::runtime_rhi::RuntimeMavgAsyncOverlapTimingSample> async_samples) {
    return mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDesc{
        .sync_baseline = make_run("sync_baseline", baseline_samples, "replay-hash-a"),
        .async_scheduler = make_run("async_scheduler", async_samples, "replay-hash-a"),
        .profiler_artifact = make_profiler(),
        .memory_budget_bytes = 600'000'000,
    };
}

} // namespace

MK_TEST("runtime rhi mavg async overlap measured performance promotes only comparable profiler backed evidence") {
    const auto baseline_samples = make_samples(20'000, 5'000, 540'000'000, false);
    const auto async_samples = make_samples(18'000, 3'500, 520'000'000, true);
    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_async_overlap_measured_performance(
        make_desc(baseline_samples, async_samples));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.comparable_runs);
    MK_REQUIRE(result.internal_non_captured_counters_ready);
    MK_REQUIRE(result.external_profiler_artifact_reviewed);
    MK_REQUIRE(result.profiler_overlap_evidence_ready);
    MK_REQUIRE(result.replay_hash_match);
    MK_REQUIRE(result.memory_budget_ready);
    MK_REQUIRE(result.mavg_async_overlap_measured_performance_ready);
    MK_REQUIRE(result.frame_p95_improvement_basis_points == 1000U);
    MK_REQUIRE(result.streaming_stall_p95_improvement_basis_points == 3000U);
    MK_REQUIRE(result.p99_regression_basis_points == 0U);
    MK_REQUIRE(result.sync_baseline_sample_count == 120U);
    MK_REQUIRE(result.async_scheduler_sample_count == 120U);
    MK_REQUIRE(!result.touched_native_handles);
    MK_REQUIRE(!result.claimed_broad_optimization);
}

MK_TEST("runtime rhi mavg async overlap measured performance rejects timing window only evidence") {
    const auto baseline_samples = make_samples(20'000, 5'000, 540'000'000, false);
    const auto async_samples = make_samples(18'000, 3'500, 520'000'000, true);
    auto desc = make_desc(baseline_samples, async_samples);
    desc.async_scheduler.timing_window_only_evidence = true;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_async_overlap_measured_performance(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.mavg_async_overlap_measured_performance_ready);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_measured_performance_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::timing_window_only_evidence));
}

MK_TEST("runtime rhi mavg async overlap measured performance rejects missing profiler proof") {
    const auto baseline_samples = make_samples(20'000, 5'000, 540'000'000, false);
    const auto async_samples = make_samples(18'000, 3'500, 520'000'000, true);
    auto desc = make_desc(baseline_samples, async_samples);
    desc.profiler_artifact.reviewed = false;
    desc.profiler_artifact.official_tool = false;
    desc.profiler_artifact.overlap_with_gpu_upload_or_draw_observed = false;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_async_overlap_measured_performance(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.external_profiler_artifact_reviewed);
    MK_REQUIRE(!result.profiler_overlap_evidence_ready);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_measured_performance_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::
                    profiler_artifact_not_reviewed));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_measured_performance_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::
                    profiler_artifact_not_official));
}

MK_TEST("runtime rhi mavg async overlap measured performance rejects mismatched package or replay") {
    const auto baseline_samples = make_samples(20'000, 5'000, 540'000'000, false);
    const auto async_samples = make_samples(18'000, 3'500, 520'000'000, true);
    auto desc = make_desc(baseline_samples, async_samples);
    desc.async_scheduler.package_hash_sha256 = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    desc.async_scheduler.replay_hash = "different-replay";

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_async_overlap_measured_performance(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.comparable_runs);
    MK_REQUIRE(!result.replay_hash_match);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_measured_performance_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::comparable_run_mismatch));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_measured_performance_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::replay_hash_mismatch));
}

MK_TEST("runtime rhi mavg async overlap measured performance enforces thresholds and memory budget") {
    const auto baseline_samples = make_samples(20'000, 5'000, 540'000'000, false);
    const auto async_samples = make_samples(19'700, 4'300, 700'000'000, true);
    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_async_overlap_measured_performance(
        make_desc(baseline_samples, async_samples));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.frame_p95_improvement_basis_points == 150U);
    MK_REQUIRE(result.streaming_stall_p95_improvement_basis_points == 1400U);
    MK_REQUIRE(!result.memory_budget_ready);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_measured_performance_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::
                    insufficient_frame_p95_improvement));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_measured_performance_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::
                    insufficient_stall_p95_improvement));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_measured_performance_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::memory_budget_exceeded));
}

MK_TEST("runtime rhi mavg async overlap measured performance rejects profiler overhead and missing timelines") {
    const auto baseline_samples = make_samples(20'000, 5'000, 540'000'000, false);
    const auto async_samples = make_samples(18'000, 3'500, 520'000'000, true);
    auto desc = make_desc(baseline_samples, async_samples);
    desc.profiler_artifact.profiler_overhead_basis_points = 900;
    desc.profiler_artifact.io_timeline_available = false;
    desc.profiler_artifact.dropped_timestamps_or_overflow = true;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_async_overlap_measured_performance(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.profiler_overlap_evidence_ready);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_measured_performance_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::
                    profiler_capture_not_representative));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_measured_performance_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode::
                    profiler_missing_required_timelines));
}

int main() {
    return mirakana::test::run_all();
}
