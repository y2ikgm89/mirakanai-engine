#requires -Version 7.0
#requires -PSEdition Core
# Chapter 121 for check-ai-integration.ps1 MAVG quality governor benchmark harness evidence.

$headerText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/mavg_quality_governor.hpp"
$sourceText = Get-AgentSurfaceText "engine/renderer/src/mavg_quality_governor.cpp"
$testsText = Get-AgentSurfaceText "tests/unit/mavg_quality_governor_tests.cpp"
$toolText = Get-AgentSurfaceText "tools/benchmark-mavg.ps1"
$cmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$rendererCmakeText = Get-AgentSurfaceText "engine/renderer/CMakeLists.txt"
$mavgDocsText = Get-AgentSurfaceText "docs/mavg.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-12-mavg-quality-governor-benchmark-harness-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgBenchmarkSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-benchmark-methodology-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "MavgQualityGovernorStatus",
        "MavgQualityGovernorFeatureKind",
        "MavgQualityGovernorDiagnosticCode",
        "MavgQualityGovernorLimits",
        "MavgQualityGovernorCounterRow",
        "MavgQualityGovernorRequest",
        "MavgQualityGovernorResult",
        "evaluate_mavg_quality_governor",
        "has_mavg_quality_governor_diagnostic",
        "selected_benchmark_harness_ready",
        "broad_mavg_benchmark_ready",
        "invoked_gpu_commands",
        "invoked_profiler_capture",
        "mutated_packages",
        "accessed_native_handles"
    )) {
    Assert-ContainsText $headerText $needle "mavg_quality_governor.hpp public contract"
}

foreach ($needle in @(
        "is_host_gated_row",
        "backend == rhi::BackendKind::metal",
        "row.host_evidence && !is_host_gated_row(row)",
        "hash_mix(hash, request.limits.max_cpu_frame_time_p95_us)",
        "hash_mix(hash, static_cast<std::uint64_t>(request.row_budget))",
        "request.require_rt_consistency_evidence",
        "append_rows_and_counts(result, request, false)",
        "forbidden_native_token",
        'token.starts_with("id3d12")',
        'token.starts_with("vk")'
    )) {
    Assert-ContainsText $sourceText $needle "mavg_quality_governor.cpp fail-closed implementation"
}

foreach ($needle in @(
        "mavg quality governor keeps metal and explicit host gates out of ready results",
        "mavg quality governor rejects native tokens across id separators",
        "mavg quality governor replay hash includes budget policy",
        "metal_result.status == mirakana::MavgQualityGovernorStatus::host_evidence_required",
        "result.row_count == 2U",
        "result.unsupported_claim_row_count == 2U",
        "base_result.replay_hash != relaxed_result.replay_hash"
    )) {
    Assert-ContainsText $testsText $needle "MK_mavg_quality_governor_tests review regression coverage"
}

foreach ($needle in @(
        "MK_mavg_quality_governor_tests",
        "tests/unit/mavg_quality_governor_tests.cpp"
    )) {
    Assert-ContainsText $cmakeText $needle "root CMake MAVG quality governor test target"
}
Assert-ContainsText $rendererCmakeText "src/mavg_quality_governor.cpp" "MK_renderer CMake MAVG quality governor source"

foreach ($needle in @(
        "mavg-benchmark-harness:",
        "executes_benchmark=false",
        "writes_artifacts=false",
        "invokes_profiler=false",
        "native_handles=false",
        "^[A-Za-z0-9._-]+$",
        "must not contain native/backend handle tokens",
        'StartsWith("id3d12")',
        'StartsWith("vk")',
        'StartsWith("mtl")'
    )) {
    Assert-ContainsText $toolText $needle "tools/benchmark-mavg.ps1 deterministic dry-run guard"
}

foreach ($surface in @(
        @{ Text = $mavgDocsText; Label = "docs/mavg.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-12-mavg-quality-governor-benchmark-harness-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgBenchmarkSpecText; Label = "docs/specs/2026-06-05-mavg-benchmark-methodology-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" }
    )) {
    foreach ($needle in @(
            "mavg-quality-governor-benchmark-harness-v1",
            "MAVG Quality Governor Benchmark Harness v1",
            "mavg_quality_governor.hpp",
            "MavgQualityGovernorResult",
            "MavgQualityGovernorCounterRow",
            "MavgQualityGovernorLimits",
            "evaluate_mavg_quality_governor",
            "tools/benchmark-mavg.ps1",
            "mavg-benchmark-harness:",
            "executes_benchmark=false",
            "writes_artifacts=false",
            "invokes_profiler=false",
            "native_handles=false"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG quality governor evidence"
    }
    foreach ($needle in @(
            "benchmark superiority",
            "Nanite",
            "broad CPU/GPU/memory optimization",
            "backend parity",
            "Metal readiness",
            "mesh shader execution",
            "deformation",
            "ray-tracing",
            "DirectStorage",
            "persistent/autonomous streaming services",
            "async-overlap"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG quality governor non-claims"
    }
}

if ([string]$manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must remain the production-completion master plan after MAVG quality governor closeout"
}
if ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "next-production-gap-selection") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must remain next-production-gap-selection after MAVG quality governor closeout"
}

$recommendedPlanText = (([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.localSliceEvidence),
    ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.latestCloseoutEvidence),
    ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.completedContext),
    ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG Quality Governor Benchmark Harness v1",
        "mavg-quality-governor-benchmark-harness-v1",
        "evaluate_mavg_quality_governor",
        "MavgQualityGovernorResult",
        "mavg-benchmark-harness:",
        "executes_benchmark=false",
        "native_handles=false",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan MAVG quality governor evidence"
}

$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module"
}
$rendererModuleText = ((@($rendererModule[0].publicHeaders) -join " "),
    (@($rendererModule[0].recentEvidence) -join " "),
    [string]$rendererModule[0].purpose) -join " "
foreach ($needle in @(
        "engine/renderer/include/mirakana/renderer/mavg_quality_governor.hpp",
        "MAVG Quality Governor Benchmark Harness v1",
        "MavgQualityGovernorResult",
        "evaluate_mavg_quality_governor",
        "tools/benchmark-mavg.ps1",
        "benchmark superiority",
        "Metal readiness",
        "broad CPU/GPU/memory optimization"
    )) {
    Assert-ContainsText $rendererModuleText $needle "engine/agent/manifest.json MK_renderer quality governor evidence"
}
