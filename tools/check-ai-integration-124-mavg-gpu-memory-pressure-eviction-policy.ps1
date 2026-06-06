#requires -Version 7.0
#requires -PSEdition Core
# Chapter 124 for check-ai-integration.ps1 static contracts.

$mavgPageStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
$mavgPageStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_page_streaming.cpp"
$mavgPageStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_page_streaming_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$mavgPressurePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-gpu-memory-pressure-eviction-policy-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "caller_supplied_gpu_memory_pressure",
        "RuntimeMavgPageStreamingGpuMemoryPressureRow",
        "eviction_pressure_score",
        "estimated_gpu_resident_bytes",
        "std::span<const RuntimeMavgPageStreamingGpuMemoryPressureRow> gpu_memory_pressure_rows",
        "planned_gpu_memory_pressure_eviction_policy",
        "applied_caller_supplied_gpu_memory_pressure_policy",
        "gpu_memory_pressure_eviction_candidate_count",
        "gpu_memory_pressure_candidate_estimated_bytes",
        "gpu_memory_pressure_protected_estimated_bytes",
        "missing_gpu_memory_pressure_row_count",
        "duplicate_gpu_memory_pressure_row_count",
        "gpu_memory_pressure_counter_overflow",
        "missing_gpu_memory_pressure_row",
        "duplicate_gpu_memory_pressure_row"
    )) {
    Assert-ContainsText $mavgPageStreamingHeaderText $needle "mavg_page_streaming.hpp GPU memory pressure public contract"
}

foreach ($needle in @(
        "uses_gpu_memory_pressure_eviction_order",
        "caller_supplied_gpu_memory_pressure",
        "find_gpu_memory_pressure_row",
        "validate_gpu_memory_pressure_rows",
        "add_gpu_memory_pressure_estimated_bytes",
        "result.applied_caller_supplied_gpu_memory_pressure_policy = true",
        "result.planned_gpu_memory_pressure_eviction_policy = true",
        "result.gpu_memory_pressure_eviction_candidate_count = eviction_candidates.size()",
        "lhs.gpu_memory_eviction_pressure_score > rhs.gpu_memory_eviction_pressure_score",
        "lhs.gpu_memory_estimated_resident_bytes > rhs.gpu_memory_estimated_resident_bytes",
        "RuntimeMavgPageStreamingDiagnosticCode::missing_gpu_memory_pressure_row",
        "RuntimeMavgPageStreamingDiagnosticCode::duplicate_gpu_memory_pressure_row",
        "RuntimeMavgPageStreamingDiagnosticCode::gpu_memory_pressure_counter_overflow"
    )) {
    Assert-ContainsText $mavgPageStreamingSourceText $needle "mavg_page_streaming.cpp GPU memory pressure implementation"
}

foreach ($needle in @(
        "runtime mavg page streaming caller supplied gpu memory pressure orders high pressure unprotected pages first",
        "runtime mavg page streaming caller supplied gpu memory pressure uses estimated bytes tie breaker",
        "runtime mavg page streaming caller supplied gpu memory pressure rejects missing candidate row",
        "runtime mavg page streaming caller supplied gpu memory pressure rejects duplicate rows",
        "runtime mavg page streaming caller supplied gpu memory pressure rejects estimated byte overflow",
        "caller_supplied_gpu_memory_pressure",
        "RuntimeMavgPageStreamingGpuMemoryPressureRow",
        "applied_caller_supplied_gpu_memory_pressure_policy",
        "gpu_memory_pressure_eviction_candidate_count",
        "gpu_memory_pressure_candidate_estimated_bytes",
        "gpu_memory_pressure_protected_estimated_bytes",
        "missing_gpu_memory_pressure_row_count",
        "duplicate_gpu_memory_pressure_row_count",
        "gpu_memory_pressure_counter_overflow",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $mavgPageStreamingTestsText $needle "MK_runtime_mavg_page_streaming_tests GPU memory pressure coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgPressurePlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-gpu-memory-pressure-eviction-policy-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $productionMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-gpu-memory-pressure-eviction-policy-v1",
            "MAVG GPU Memory Pressure Eviction Policy v1",
            "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_gpu_memory_pressure",
            "RuntimeMavgPageStreamingGpuMemoryPressureRow",
            "planned_gpu_memory_pressure_eviction_policy",
            "applied_caller_supplied_gpu_memory_pressure_policy",
            "gpu_memory_pressure_eviction_candidate_count",
            "gpu_memory_pressure_counter_overflow"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) GPU memory pressure evidence"
    }
    foreach ($needle in @(
            "real GPU residency enforcement",
            "renderer/RHI handles",
            "DirectStorage",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) GPU memory pressure remaining non-claims"
    }
}

foreach ($needle in @(
        "MAVG GPU Memory Pressure Eviction Policy v1",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_gpu_memory_pressure",
        "RuntimeMavgPageStreamingGpuMemoryPressureRow",
        "gpu_memory_pressure_rows",
        "planned_gpu_memory_pressure_eviction_policy",
        "applied_caller_supplied_gpu_memory_pressure_policy",
        "gpu_memory_pressure_eviction_candidate_count",
        "gpu_memory_pressure_candidate_estimated_bytes",
        "gpu_memory_pressure_protected_estimated_bytes",
        "missing_gpu_memory_pressure_row_count",
        "duplicate_gpu_memory_pressure_row_count",
        "gpu_memory_pressure_counter_overflow",
        "eviction_pressure_score",
        "estimated_gpu_resident_bytes",
        "no real GPU residency enforcement",
        "no allocator/GPU budget enforcement",
        "no DirectStorage file IO",
        "no renderer/RHI handles"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json GPU memory pressure module evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json GPU memory pressure plan evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-gpu-memory-pressure-eviction-policy-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG GPU Memory Pressure Eviction Policy v1"
}

$recommendedPlanText = (([string]$productionLoop.recommendedNextPlan.retainedCompletedPlanEvidence), ([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG GPU Memory Pressure Eviction Policy v1",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_gpu_memory_pressure",
        "RuntimeMavgPageStreamingGpuMemoryPressureRow",
        "gpu_memory_pressure_eviction_candidate_count",
        "gpu_memory_pressure_counter_overflow",
        "no real GPU residency enforcement",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan GPU memory pressure evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG GPU Memory Pressure Eviction Policy v1",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_gpu_memory_pressure",
        "RuntimeMavgPageStreamingGpuMemoryPressureRow",
        "gpu_memory_pressure_eviction_candidate_count",
        "planned_gpu_memory_pressure_eviction_policy",
        "applied_caller_supplied_gpu_memory_pressure_policy",
        "gpu_memory_pressure_counter_overflow",
        "no real GPU residency enforcement"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime GPU memory pressure evidence"
}
