#requires -Version 7.0
#requires -PSEdition Core
# Chapter 111 for check-ai-integration.ps1 static contracts.

$mavgGpuCullingHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp"
$mavgGpuCullingSourceText = Get-AgentSurfaceText "engine/renderer/src/mavg_gpu_culling.cpp"
$mavgGpuCullingTestsText = Get-AgentSurfaceText "tests/unit/mavg_gpu_culling_tests.cpp"
$mavgGpuCullingDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md"
$mavgGpuCullingIndirectPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-gpu-culling-indirect-v1.md"
$mavgVulkanCountBufferPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "MavgGpuCullingIndirectPlan",
        "MavgGpuCullingSyncRequirement",
        "MavgGpuCullingIndirectCommandLayout",
        "plan_mavg_gpu_culling_indirect_commands",
        "executed_gpu_culling",
        "compute_shader"
    )) {
    Assert-ContainsText $mavgGpuCullingHeaderText $needle "engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp GPU culling dispatch activation fail-closed contract"
}

foreach ($needle in @(
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "append_compute_sync_requirements",
        "indexed_indirect_record_stride_bytes",
        "MavgGpuCullingProducer::compute_shader",
        "fail_closed(plan)"
    )) {
    Assert-ContainsText $mavgGpuCullingSourceText $needle "engine/renderer/src/mavg_gpu_culling.cpp GPU culling dispatch activation fail-closed evidence"
}

foreach ($needle in @(
        "mavg gpu culling indirect planning emits packed indexed command rows",
        "MK_REQUIRE(!plan.executed_gpu_culling)",
        "mavg gpu culling indirect planning records backend sync requirements for compute produced commands"
    )) {
    Assert-ContainsText $mavgGpuCullingTestsText $needle "tests/unit/mavg_gpu_culling_tests.cpp GPU culling dispatch activation RED coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgGpuCullingDispatchPlanText; Label = "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-gpu-culling-dispatch-v1",
            "MavgGpuCullingIndirectPlan",
            "D3D12",
            "compute dispatch",
            "fail-closed"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG GPU culling dispatch activation evidence"
    }
    foreach ($needle in @(
            "Vulkan compute dispatch",
            "compute-generated indirect consumption",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG GPU culling dispatch non-claim evidence"
    }
}

foreach ($needle in @(
        "**Status:** Active.",
        "mavg-gpu-culling-dispatch-v1",
        "executed_gpu_culling=false",
        "fail-closed",
        "mavg-vulkan-count-buffer-indirect-execution-v1",
        "PR #553"
    )) {
    Assert-ContainsText $mavgGpuCullingDispatchPlanText $needle "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md activation contract"
}

foreach ($needle in @(
        'active child plan `mavg-gpu-culling-dispatch-v1`',
        "Follow-up actual GPU culling dispatch"
    )) {
    Assert-ContainsText $mavgVulkanCountBufferPlanText $needle "docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md sibling transition"
}

foreach ($needle in @(
        "active child plan",
        "mavg-gpu-culling-dispatch-v1"
    )) {
    Assert-ContainsText $mavgGpuCullingIndirectPlanText $needle "docs/superpowers/plans/2026-06-05-mavg-gpu-culling-indirect-v1.md sibling transition"
}

foreach ($needle in @(
        "mavg-gpu-culling-dispatch-v1",
        "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md",
        "MAVG GPU Culling Dispatch v1",
        "MavgGpuCullingIndirectPlan",
        "mavg-vulkan-count-buffer-indirect-execution-v1",
        "PR #552",
        "PR #553"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG GPU culling dispatch activation evidence"
}

foreach ($needle in @(
        "MAVG GPU Culling Dispatch v1",
        "MavgGpuCullingIndirectPlan",
        "executed_gpu_culling=false",
        "compute-write-to-indirect-read",
        "PR #553"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_renderer GPU culling dispatch activation evidence"
}

$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module" }
$rendererManifestText = ((@($rendererModule[0].recentEvidence) -join " "), [string]$rendererModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG GPU Culling Dispatch v1",
        "MavgGpuCullingIndirectPlan",
        "executed_gpu_culling=false",
        "compute-write-to-indirect-read",
        "PR #553"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer GPU culling dispatch activation evidence"
}

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must point at MAVG GPU culling dispatch during activation"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "mavg-gpu-culling-dispatch-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must be mavg-gpu-culling-dispatch-v1 during activation"
}
foreach ($needle in @("MAVG GPU Culling Dispatch v1", "mavg-gpu-culling-dispatch-v1", "MavgGpuCullingIndirectPlan", "mavg-vulkan-count-buffer-indirect-execution-v1", "PR #552", "PR #553", "executed_gpu_culling=false", "compute-write-to-indirect-read", "unsupportedProductionGaps = []")) {
    Assert-ContainsText ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.latestCloseoutEvidence) $needle "engine/agent/manifest.json recommendedNextPlan.latestCloseoutEvidence MAVG GPU culling dispatch activation evidence"
}
