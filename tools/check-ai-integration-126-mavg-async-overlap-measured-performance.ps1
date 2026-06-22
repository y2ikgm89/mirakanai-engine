#requires -Version 7.0
#requires -PSEdition Core
# Chapter 126 for check-ai-integration.ps1 MAVG measured async-overlap performance host-evidence contract.

$mavgMeasuredHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_async_overlap_performance.hpp"
$mavgMeasuredSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_async_overlap_performance.cpp"
$mavgMeasuredTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_async_overlap_performance_tests.cpp"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$schemaText = Get-AgentSurfaceText "schemas/mavg-async-overlap-performance.schema.json"
$validatorText = Get-AgentSurfaceText "tools/validate-mavg-async-overlap-performance.ps1"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgAdvancedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RuntimeMavgAsyncOverlapMeasuredPerformanceProfilerTool",
        "RuntimeMavgAsyncOverlapMeasuredPerformanceDiagnosticCode",
        "RuntimeMavgAsyncOverlapRunEvidence",
        "RuntimeMavgAsyncOverlapProfilerArtifactEvidence",
        "RuntimeMavgAsyncOverlapMeasuredPerformanceDesc",
        "RuntimeMavgAsyncOverlapMeasuredPerformanceResult",
        "evaluate_runtime_mavg_async_overlap_measured_performance",
        "has_runtime_mavg_async_overlap_measured_performance_diagnostic",
        "sync_baseline",
        "async_scheduler",
        "profiler_artifact",
        "required_warmup_frames{30}",
        "required_measured_frames{120}",
        "minimum_frame_p95_improvement_basis_points{500}",
        "minimum_stall_p95_improvement_basis_points{2000}",
        "maximum_p99_regression_basis_points{200}",
        "bool mavg_async_overlap_measured_performance_ready{false};",
        "pix_timing_capture",
        "nsight_graphics_gpu_trace",
        "radeon_gpu_profiler",
        "intel_gpa",
        "apple_metal_tools"
    )) {
    Assert-ContainsText $mavgMeasuredHeaderText $needle "mavg_async_overlap_performance.hpp public contract"
}

foreach ($needle in @(
        "insufficient_warmup_frames",
        "insufficient_measured_frames",
        "missing_internal_counters",
        "timing_window_only_evidence",
        "profiler_artifact_not_reviewed",
        "profiler_artifact_not_official",
        "profiler_capture_not_representative",
        "profiler_missing_required_timelines",
        "profiler_missing_overlap",
        "replay_hash_mismatch",
        "memory_budget_exceeded",
        "insufficient_frame_p95_improvement",
        "insufficient_stall_p95_improvement",
        "excessive_p99_regression",
        "native_handle_claimed",
        "broad_optimization_claimed",
        "result.mavg_async_overlap_measured_performance_ready = result.diagnostics.empty()"
    )) {
    Assert-ContainsText $mavgMeasuredSourceText $needle "mavg_async_overlap_performance.cpp fail-closed implementation"
}

foreach ($needle in @(
        "runtime rhi mavg async overlap measured performance promotes only comparable profiler backed evidence",
        "runtime rhi mavg async overlap measured performance rejects timing window only evidence",
        "runtime rhi mavg async overlap measured performance rejects missing profiler proof",
        "runtime rhi mavg async overlap measured performance rejects mismatched package or replay",
        "runtime rhi mavg async overlap measured performance enforces thresholds and memory budget",
        "runtime rhi mavg async overlap measured performance rejects profiler overhead and missing timelines",
        "frame_p95_improvement_basis_points == 1000U",
        "streaming_stall_p95_improvement_basis_points == 3000U",
        "!result.mavg_async_overlap_measured_performance_ready"
    )) {
    Assert-ContainsText $mavgMeasuredTestsText $needle "MK_runtime_rhi_mavg_async_overlap_performance_tests coverage"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_async_overlap_performance.cpp" "engine/runtime_rhi/CMakeLists.txt measured performance source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_rhi_mavg_async_overlap_performance_tests" "root CMake measured performance test target"

foreach ($needle in @(
        "GameEngine.MavgAsyncOverlapPerformanceEvidence.v1",
        "sync_baseline",
        "async_scheduler",
        "profiler_artifact",
        "warmup_frame_count",
        "minimum",
        "30",
        "measured_frame_count",
        "120",
        "internal_engine_counters_from_non_captured_run",
        "timing_window_only_evidence",
        "frame_p95_us",
        "frame_p99_us",
        "streaming_stall_p95_us",
        "memory_peak_bytes",
        "artifact_hash_sha256",
        "pix_timing_capture",
        "nsight_graphics_gpu_trace",
        "radeon_gpu_profiler",
        "intel_gpa",
        "apple_metal_tools",
        "overlap_with_gpu_upload_or_draw_observed",
        "representative_capture"
    )) {
    Assert-ContainsText $schemaText $needle "mavg-async-overlap-performance.schema.json"
}

foreach ($needle in @(
        "validation_recipe=mavg-async-overlap-performance",
        "MK_runtime_rhi_mavg_async_overlap_performance_tests",
        '"artifacts/mavg/async-overlap-performance"',
        'artifact_root=$ArtifactRootRelative',
        'mavg_async_overlap_measured_performance_status=$status',
        'mavg_async_overlap_measured_performance_ready=$(ConvertTo-CounterBit $ready)',
        'mavg_async_overlap_measured_performance_missing_artifacts=$missingArtifactCount',
        '$status = if ($ready) { "ready" } else { "host_evidence_required" }',
        "mavg_async_overlap_measured_performance_timing_window_only=0",
        "mavg_async_overlap_measured_performance_native_handles_exposed=0",
        "mavg_async_overlap_measured_performance_gpu_directstorage_destinations=0",
        "mavg_async_overlap_measured_performance_gdeflate=0",
        "mavg_async_overlap_measured_performance_mesh_shader_execution=0",
        "mavg_async_overlap_measured_performance_metal_readiness=0",
        "mavg_async_overlap_measured_performance_nanite_equivalence=0",
        "mavg_async_overlap_measured_performance_broad_optimization=0",
        "MAVG async-overlap measured performance proof is incomplete"
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-mavg-async-overlap-performance.ps1"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgAdvancedPlanText; Label = "MAVG advanced evidence plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "mavg-async-overlap-performance-v1",
            "mavg_async_overlap_performance.hpp",
            "RuntimeMavgAsyncOverlapMeasuredPerformanceDesc",
            "RuntimeMavgAsyncOverlapProfilerArtifactEvidence",
            "RuntimeMavgAsyncOverlapMeasuredPerformanceResult",
            "evaluate_runtime_mavg_async_overlap_measured_performance",
            "GameEngine.MavgAsyncOverlapPerformanceEvidence.v1",
            "mavg_async_overlap_measured_performance_status=host_evidence_required",
            "mavg_async_overlap_measured_performance_ready=0",
            "native handles",
            "GPU DirectStorage destinations",
            "GDeflate",
            "mesh shader",
            "Metal readiness",
            "Nanite",
            "broad MAVG backend readiness",
            "broad"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) measured performance host-gated contract"
    }
    foreach ($forbiddenNeedle in @(
            "mavg_async_overlap_measured_performance_ready=1",
            "mavg_async_overlap_measured_performance_status=ready"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden measured performance ready claim"
    }
}

$manifest = $manifestText | ConvertFrom-Json
$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module" }
$runtimeRhiManifestText = ((@($runtimeRhiModule[0].publicHeaders) -join " "),
    (@($runtimeRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_async_overlap_performance.hpp",
        "RuntimeMavgAsyncOverlapMeasuredPerformanceDesc",
        "RuntimeMavgAsyncOverlapProfilerArtifactEvidence",
        "RuntimeMavgAsyncOverlapMeasuredPerformanceResult",
        "evaluate_runtime_mavg_async_overlap_measured_performance",
        "mavg_async_overlap_measured_performance_status=host_evidence_required",
        "mavg_async_overlap_measured_performance_ready=0"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi measured performance evidence"
}
