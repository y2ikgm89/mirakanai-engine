#requires -Version 7.0
#requires -PSEdition Core
# Chapter 113 for check-ai-integration.ps1 static contracts.

$vulkanBackendSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
$d3d12MavgGpuCullingDispatchSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp"
$mavgVulkanGpuCullingDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md"
$mavgComputeGeneratedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md"
$mavgGpuCullingDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$vulkanDispatchHeaderPath = Join-Path $root "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_gpu_culling_dispatch.hpp"
$vulkanDispatchSourcePath = Join-Path $root "engine/rhi/vulkan/src/vulkan_mavg_gpu_culling_dispatch.cpp"
$vulkanDispatchTestsPath = Join-Path $root "tests/unit/mavg_vulkan_gpu_culling_dispatch_tests.cpp"

foreach ($needle in @(
        "draw_indexed_indirect",
        "vulkan rhi indexed indirect draw argument buffer requires copy_source upload usage in v1",
        "vulkan rhi indexed indirect count buffer requires copy_source upload usage in v1"
    )) {
    Assert-ContainsText $vulkanBackendSourceText $needle "engine/rhi/vulkan/src/vulkan_backend.cpp Vulkan GPU culling dispatch activation fail-closed copy_source contract"
}

foreach ($needle in @(
        "dispatch_mavg_gpu_culling_indirect",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "D3D12_RESOURCE_BARRIER_TYPE_UAV",
        "executed_gpu_culling = true"
    )) {
    Assert-ContainsText $d3d12MavgGpuCullingDispatchSourceText $needle "engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp completed D3D12 GPU culling dispatch baseline"
}

if (Test-Path -LiteralPath $vulkanDispatchHeaderPath) {
    Write-Error "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_gpu_culling_dispatch.hpp must not exist during activation-only slice"
}
if (Test-Path -LiteralPath $vulkanDispatchSourcePath) {
    Write-Error "engine/rhi/vulkan/src/vulkan_mavg_gpu_culling_dispatch.cpp must not exist during activation-only slice"
}
if (Test-Path -LiteralPath $vulkanDispatchTestsPath) {
    Write-Error "tests/unit/mavg_vulkan_gpu_culling_dispatch_tests.cpp must not exist during activation-only slice"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgVulkanGpuCullingDispatchPlanText; Label = "docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-vulkan-gpu-culling-dispatch-v1",
            "Vulkan",
            "compute dispatch",
            "dispatch_mavg_gpu_culling_indirect",
            "fail-closed",
            "copy_source"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Vulkan GPU culling dispatch activation evidence"
    }
    foreach ($needle in @(
            "Vulkan compute-generated indirect consumption",
            "D3D12 changes",
            "mesh shaders",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Vulkan GPU culling dispatch non-claim evidence"
    }
}

foreach ($needle in @(
        "**Status:** Active.",
        "mavg-vulkan-gpu-culling-dispatch-v1",
        "copy_source",
        "fail-closed",
        "mavg-d3d12-compute-generated-indirect-consumption-v1",
        "PR #560",
        "PR #561",
        "MK_mavg_vulkan_gpu_culling_dispatch_tests"
    )) {
    Assert-ContainsText $mavgVulkanGpuCullingDispatchPlanText $needle "docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md activation contract"
}

foreach ($needle in @(
        "mavg-vulkan-gpu-culling-dispatch-v1",
        "PR #561"
    )) {
    Assert-ContainsText $mavgComputeGeneratedPlanText $needle "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md sibling transition"
}

foreach ($needle in @(
        "Planned Vulkan dispatch follow-up child",
        "mavg-vulkan-gpu-culling-dispatch-v1",
        "PR #560"
    )) {
    Assert-ContainsText $mavgGpuCullingDispatchPlanText $needle "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md sibling transition"
}

foreach ($needle in @(
        "mavg-vulkan-gpu-culling-dispatch-v1",
        "docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md",
        "MAVG Vulkan GPU Culling Dispatch v1",
        "dispatch_mavg_gpu_culling_indirect",
        "mavg-d3d12-compute-generated-indirect-consumption-v1",
        "PR #560",
        "PR #561",
        "copy_source"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG Vulkan GPU culling dispatch activation evidence"
}

foreach ($needle in @(
        "MAVG Vulkan GPU Culling Dispatch v1",
        "dispatch_mavg_gpu_culling_indirect",
        "copy_source",
        "fail-closed",
        "PR #560"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_renderer Vulkan GPU culling dispatch activation evidence"
}

$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module" }
$rendererManifestText = ((@($rendererModule[0].recentEvidence) -join " "), [string]$rendererModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Vulkan GPU Culling Dispatch v1",
        "dispatch_mavg_gpu_culling_indirect",
        "copy_source",
        "fail-closed",
        "PR #560"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer Vulkan GPU culling dispatch activation evidence"
}

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must point at MAVG Vulkan GPU culling dispatch during activation"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "mavg-vulkan-gpu-culling-dispatch-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must be mavg-vulkan-gpu-culling-dispatch-v1 during activation"
}
foreach ($needle in @(
        "MAVG Vulkan GPU Culling Dispatch v1",
        "mavg-vulkan-gpu-culling-dispatch-v1",
        "dispatch_mavg_gpu_culling_indirect",
        "mavg-d3d12-compute-generated-indirect-consumption-v1",
        "PR #560",
        "PR #561",
        "copy_source",
        "fail-closed",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.latestCloseoutEvidence) $needle "engine/agent/manifest.json recommendedNextPlan.latestCloseoutEvidence MAVG Vulkan GPU culling dispatch activation evidence"
}
