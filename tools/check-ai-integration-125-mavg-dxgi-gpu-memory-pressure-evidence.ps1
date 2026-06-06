#requires -Version 7.0
#requires -PSEdition Core
# Chapter 125 for check-ai-integration.ps1 static contracts.

$mavgPressureHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_memory_pressure.hpp"
$mavgPressureSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_gpu_memory_pressure.cpp"
$runtimeRhiTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_tests.cpp"
$d3d12TestsText = Get-AgentSurfaceText "tests/unit/d3d12_rhi_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgDxgiPressurePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-dxgi-gpu-memory-pressure-evidence-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode",
        "RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc",
        "RuntimeMavgResidentPageGpuMemoryEstimateRow",
        "RuntimeMavgDxgiGpuMemoryPressureEvidenceResult",
        "build_runtime_mavg_dxgi_gpu_memory_pressure_rows",
        "std::span<const runtime::RuntimeMavgResidentPageMountRow> resident_page_mounts",
        "std::span<const RuntimeMavgResidentPageGpuMemoryEstimateRow> estimated_pages",
        "local_video_memory_pressure_score",
        "estimated_gpu_resident_byte_overflow",
        "used_dxgi_video_memory_budget_evidence",
        "touched_renderer_or_rhi_handles",
        "enforced_gpu_residency",
        "reserved_video_memory"
    )) {
    Assert-ContainsText $mavgPressureHeaderText $needle "mavg_gpu_memory_pressure.hpp DXGI GPU pressure public contract"
}

foreach ($needle in @(
        "desc.backend != rhi::BackendKind::d3d12",
        "missing_dxgi_video_memory_budget",
        "invalid_dxgi_video_memory_budget",
        "duplicate_resident_page_mount",
        "duplicate_estimate_row",
        "missing_estimate_row",
        "estimated_byte_overflow",
        "pressure_score_from_budget",
        "result.used_dxgi_video_memory_budget_evidence = true",
        "RuntimeMavgPageStreamingGpuMemoryPressureRow",
        "result.pressure_rows.clear()"
    )) {
    Assert-ContainsText $mavgPressureSourceText $needle "mavg_gpu_memory_pressure.cpp DXGI GPU pressure implementation"
}

foreach ($needle in @(
        "runtime rhi mavg dxgi gpu memory pressure builds rows from d3d12 budget evidence",
        "runtime rhi mavg dxgi gpu memory pressure rows feed mavg eviction ordering",
        "runtime rhi mavg dxgi gpu memory pressure requires d3d12 dxgi budget evidence",
        "runtime rhi mavg dxgi gpu memory pressure rejects missing estimate rows",
        "runtime rhi mavg dxgi gpu memory pressure rejects duplicate estimate rows",
        "runtime rhi mavg dxgi gpu memory pressure rejects estimated byte overflow",
        "build_runtime_mavg_dxgi_gpu_memory_pressure_rows",
        "RuntimeMavgResidentPageGpuMemoryEstimateRow",
        "caller_supplied_gpu_memory_pressure",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $runtimeRhiTestsText $needle "MK_runtime_rhi_tests DXGI GPU pressure coverage"
}

foreach ($needle in @(
        "d3d12 rhi memory diagnostics reports committed resource bytes and optional DXGI video memory",
        "os_video_memory_budget_available",
        "local_video_memory_budget_bytes > 0U"
    )) {
    Assert-ContainsText $d3d12TestsText $needle "MK_d3d12_rhi_tests host-safe DXGI memory diagnostics coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgDxgiPressurePlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-dxgi-gpu-memory-pressure-evidence-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "MAVG DXGI GPU Memory Pressure Evidence v1",
            "mavg-dxgi-gpu-memory-pressure-evidence-v1",
            "mavg_gpu_memory_pressure.hpp",
            "RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc",
            "RuntimeMavgResidentPageGpuMemoryEstimateRow",
            "RuntimeMavgDxgiGpuMemoryPressureEvidenceResult",
            "build_runtime_mavg_dxgi_gpu_memory_pressure_rows"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) DXGI GPU pressure evidence"
    }
    foreach ($needle in @(
            "real GPU residency enforcement",
            "allocator",
            "DirectStorage",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) DXGI GPU pressure non-claims"
    }
}

foreach ($needle in @(
        "MAVG DXGI GPU Memory Pressure Evidence v1",
        "mavg_gpu_memory_pressure.hpp",
        "RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc",
        "RuntimeMavgResidentPageGpuMemoryEstimateRow",
        "RuntimeMavgDxgiGpuMemoryPressureEvidenceResult",
        "RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode",
        "build_runtime_mavg_dxgi_gpu_memory_pressure_rows",
        "D3D12 RhiDeviceMemoryDiagnostics DXGI local_video_memory_budget_bytes",
        "caller_supplied_gpu_memory_pressure",
        "no real GPU residency enforcement",
        "no allocator/GPU budget enforcement",
        "no DirectStorage file IO",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json DXGI GPU pressure evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json DXGI GPU pressure evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-dxgi-gpu-memory-pressure-evidence-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG DXGI GPU Memory Pressure Evidence v1"
}

$recommendedPlanText = (([string]$productionLoop.recommendedNextPlan.retainedCompletedPlanEvidence), ([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG DXGI GPU Memory Pressure Evidence v1",
        "RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc",
        "RuntimeMavgResidentPageGpuMemoryEstimateRow",
        "RuntimeMavgDxgiGpuMemoryPressureEvidenceResult",
        "build_runtime_mavg_dxgi_gpu_memory_pressure_rows",
        "D3D12 RhiDeviceMemoryDiagnostics",
        "no real GPU residency enforcement",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan DXGI GPU pressure evidence"
}

$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module" }
$runtimeRhiManifestText = ((@($runtimeRhiModule[0].publicHeaders) -join " "), (@($runtimeRhiModule[0].recentEvidence) -join " "), [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_gpu_memory_pressure.hpp",
        "RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc",
        "RuntimeMavgResidentPageGpuMemoryEstimateRow",
        "RuntimeMavgDxgiGpuMemoryPressureEvidenceResult",
        "build_runtime_mavg_dxgi_gpu_memory_pressure_rows",
        "no real GPU residency enforcement",
        "no allocator/GPU budget enforcement",
        "no Nanite compatibility/equivalence/superiority"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi DXGI GPU pressure evidence"
}
