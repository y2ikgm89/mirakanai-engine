#requires -Version 7.0
#requires -PSEdition Core
# Chapter 112 for check-ai-integration.ps1 static contracts.

$d3d12BackendSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_backend.cpp"
$d3d12MavgGpuCullingDispatchSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp"
$mavgComputeGeneratedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md"
$mavgGpuCullingDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$computeGeneratedTestsPath = Join-Path $root "tests/unit/mavg_d3d12_compute_generated_indirect_consumption_tests.cpp"

foreach ($needle in @(
        "draw_indexed_indirect",
        "d3d12 rhi indexed indirect draw argument buffer requires copy_source upload usage in v1",
        "d3d12 rhi indexed indirect draw count buffer requires copy_source upload usage in v1"
    )) {
    Assert-ContainsText $d3d12BackendSourceText $needle "engine/rhi/d3d12/src/d3d12_backend.cpp D3D12 compute-generated consumption activation fail-closed copy_source contract"
}

foreach ($needle in @(
        "dispatch_mavg_gpu_culling_indirect",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "D3D12_RESOURCE_BARRIER_TYPE_UAV",
        "executed_gpu_culling = true"
    )) {
    Assert-ContainsText $d3d12MavgGpuCullingDispatchSourceText $needle "engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp completed GPU culling dispatch baseline"
}

if (Test-Path -LiteralPath $computeGeneratedTestsPath) {
    Write-Error "tests/unit/mavg_d3d12_compute_generated_indirect_consumption_tests.cpp must not exist during activation-only slice"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgComputeGeneratedPlanText; Label = "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-d3d12-compute-generated-indirect-consumption-v1",
            "D3D12",
            "compute-generated",
            "dispatch_mavg_gpu_culling_indirect",
            "fail-closed",
            "copy_source"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 compute-generated indirect consumption activation evidence"
    }
    foreach ($needle in @(
            "Vulkan compute dispatch",
            "Vulkan compute-generated",
            "mesh shaders",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 compute-generated indirect consumption non-claim evidence"
    }
}

foreach ($needle in @(
        "**Status:** Active.",
        "mavg-d3d12-compute-generated-indirect-consumption-v1",
        "copy_source",
        "fail-closed",
        "mavg-gpu-culling-dispatch-v1",
        "PR #556",
        "PR #557",
        "PR #558",
        "MK_mavg_d3d12_compute_generated_indirect_consumption_tests"
    )) {
    Assert-ContainsText $mavgComputeGeneratedPlanText $needle "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md activation contract"
}

foreach ($needle in @(
        "active follow-up child",
        "mavg-d3d12-compute-generated-indirect-consumption-v1",
        "PR #558"
    )) {
    Assert-ContainsText $mavgGpuCullingDispatchPlanText $needle "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md sibling transition"
}

foreach ($needle in @(
        "mavg-d3d12-compute-generated-indirect-consumption-v1",
        "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md",
        "MAVG D3D12 Compute-Generated Indirect Consumption v1",
        "dispatch_mavg_gpu_culling_indirect",
        "mavg-gpu-culling-dispatch-v1",
        "PR #556",
        "PR #557",
        "PR #558",
        "copy_source"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG D3D12 compute-generated indirect consumption activation evidence"
}

foreach ($needle in @(
        "MAVG D3D12 Compute-Generated Indirect Consumption v1",
        "dispatch_mavg_gpu_culling_indirect",
        "copy_source",
        "fail-closed",
        "PR #557"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_renderer compute-generated consumption activation evidence"
}

$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module" }
$rendererManifestText = ((@($rendererModule[0].recentEvidence) -join " "), [string]$rendererModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG D3D12 Compute-Generated Indirect Consumption v1",
        "dispatch_mavg_gpu_culling_indirect",
        "copy_source",
        "fail-closed",
        "PR #557"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer compute-generated consumption activation evidence"
}

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must point at MAVG D3D12 compute-generated indirect consumption during activation"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "mavg-d3d12-compute-generated-indirect-consumption-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must be mavg-d3d12-compute-generated-indirect-consumption-v1 during activation"
}
foreach ($needle in @(
        "MAVG D3D12 Compute-Generated Indirect Consumption v1",
        "mavg-d3d12-compute-generated-indirect-consumption-v1",
        "dispatch_mavg_gpu_culling_indirect",
        "mavg-gpu-culling-dispatch-v1",
        "PR #556",
        "PR #557",
        "PR #558",
        "copy_source",
        "fail-closed",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.latestCloseoutEvidence) $needle "engine/agent/manifest.json recommendedNextPlan.latestCloseoutEvidence MAVG D3D12 compute-generated indirect consumption activation evidence"
}
