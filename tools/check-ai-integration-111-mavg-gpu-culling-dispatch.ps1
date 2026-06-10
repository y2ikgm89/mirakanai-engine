#requires -Version 7.0
#requires -PSEdition Core
# Chapter 111 for check-ai-integration.ps1 static contracts.

$mavgGpuCullingHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp"
$mavgGpuCullingSourceText = Get-AgentSurfaceText "engine/renderer/src/mavg_gpu_culling.cpp"
$mavgGpuCullingTestsText = Get-AgentSurfaceText "tests/unit/mavg_gpu_culling_tests.cpp"
$mavgGpuCullingDispatchTestsText = Get-AgentSurfaceText "tests/unit/mavg_gpu_culling_dispatch_tests.cpp"
$d3d12MavgGpuCullingDispatchHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_mavg_gpu_culling_dispatch.hpp"
$d3d12MavgGpuCullingDispatchSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp"
$mavgGpuCullingDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md"
$mavgGpuCullingIndirectPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-gpu-culling-indirect-v1.md"
$mavgVulkanCountBufferPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "MavgGpuCullingDispatchClusterRow",
        "MavgGpuCullingIndirectPlan",
        "MavgGpuCullingSyncRequirement",
        "MavgGpuCullingIndirectCommandLayout",
        "plan_mavg_gpu_culling_indirect_commands",
        "build_mavg_gpu_culling_dispatch_cluster_rows",
        "encode_mavg_gpu_culling_indirect_argument_buffer_bytes",
        "encode_mavg_gpu_culling_indirect_count_buffer_bytes",
        "executed_gpu_culling"
    )) {
    Assert-ContainsText $mavgGpuCullingHeaderText $needle "engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp MAVG GPU culling dispatch contract"
}

foreach ($needle in @(
        "encode_indexed_indirect_draw_command",
        "build_mavg_gpu_culling_dispatch_cluster_rows",
        "encode_mavg_gpu_culling_indirect_argument_buffer_bytes",
        "encode_mavg_gpu_culling_indirect_count_buffer_bytes"
    )) {
    Assert-ContainsText $mavgGpuCullingSourceText $needle "engine/renderer/src/mavg_gpu_culling.cpp MAVG GPU culling dispatch encode helpers"
}

foreach ($needle in @(
        "mavg gpu culling indirect planning emits packed indexed command rows",
        "MK_REQUIRE(!plan.executed_gpu_culling)",
        "mavg gpu culling indirect planning records backend sync requirements for compute produced commands"
    )) {
    Assert-ContainsText $mavgGpuCullingTestsText $needle "tests/unit/mavg_gpu_culling_tests.cpp MAVG GPU culling value-only planner coverage"
}

foreach ($needle in @(
        "dispatch_mavg_gpu_culling_indirect",
        "D3d12MavgGpuCullingDispatchDesc",
        "D3d12MavgGpuCullingDispatchResult",
        "executed_gpu_culling"
    )) {
    Assert-ContainsText $d3d12MavgGpuCullingDispatchHeaderText $needle "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_mavg_gpu_culling_dispatch.hpp MAVG GPU culling dispatch API"
}

foreach ($needle in @(
        "dispatch_mavg_gpu_culling_indirect",
        "command_list->Dispatch",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "D3D12_RESOURCE_BARRIER_TYPE_UAV",
        "resource_barriers_recorded",
        "executed_gpu_culling = true"
    )) {
    Assert-ContainsText $d3d12MavgGpuCullingDispatchSourceText $needle "engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp D3D12 MAVG GPU culling dispatch evidence"
}

foreach ($needle in @(
        "mavg gpu culling dispatch writes visible cluster indirect bytes on d3d12 warp",
        "mavg gpu culling dispatch reduces count for culled clusters on d3d12 warp",
        "MK_REQUIRE(dispatch.executed_gpu_culling)",
        "encode_mavg_gpu_culling_indirect_argument_buffer_bytes",
        "encode_mavg_gpu_culling_indirect_count_buffer_bytes",
        "dispatch_mavg_gpu_culling_indirect",
        "d3d12_warp_available"
    )) {
    Assert-ContainsText $mavgGpuCullingDispatchTestsText $needle "tests/unit/mavg_gpu_culling_dispatch_tests.cpp MAVG GPU culling dispatch WARP proof"
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
            "MK_mavg_gpu_culling_dispatch_tests",
            "dispatch_mavg_gpu_culling_indirect"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG GPU culling dispatch implementation evidence"
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
        "MK_mavg_gpu_culling_dispatch_tests",
        "dispatch_mavg_gpu_culling_indirect",
        "mavg-vulkan-count-buffer-indirect-execution-v1",
        "PR #553"
    )) {
    Assert-ContainsText $mavgGpuCullingDispatchPlanText $needle "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md implementation contract"
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
        "dispatch_mavg_gpu_culling_indirect",
        "MK_mavg_gpu_culling_dispatch_tests",
        "mavg-vulkan-count-buffer-indirect-execution-v1",
        "PR #552",
        "PR #553"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG GPU culling dispatch implementation evidence"
}

foreach ($needle in @(
        "MAVG GPU Culling Dispatch v1",
        "MavgGpuCullingIndirectPlan",
        "dispatch_mavg_gpu_culling_indirect",
        "MK_mavg_gpu_culling_dispatch_tests",
        "compute-write-to-indirect-read",
        "PR #553"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_renderer GPU culling dispatch implementation evidence"
}

$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module" }
$rendererManifestText = ((@($rendererModule[0].recentEvidence) -join " "), [string]$rendererModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG GPU Culling Dispatch v1",
        "MavgGpuCullingIndirectPlan",
        "dispatch_mavg_gpu_culling_indirect",
        "MK_mavg_gpu_culling_dispatch_tests",
        "compute-write-to-indirect-read",
        "PR #553"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer GPU culling dispatch implementation evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG GPU Culling Dispatch v1",
        "dispatch_mavg_gpu_culling_indirect",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "MK_mavg_gpu_culling_dispatch_tests"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi GPU culling dispatch implementation evidence"
}

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must point at MAVG GPU culling dispatch during implementation"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "mavg-gpu-culling-dispatch-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must be mavg-gpu-culling-dispatch-v1 during implementation"
}
foreach ($needle in @("MAVG GPU Culling Dispatch v1", "mavg-gpu-culling-dispatch-v1", "MavgGpuCullingIndirectPlan", "dispatch_mavg_gpu_culling_indirect", "MK_mavg_gpu_culling_dispatch_tests", "mavg-vulkan-count-buffer-indirect-execution-v1", "PR #552", "PR #553", "compute-write-to-indirect-read", "unsupportedProductionGaps = []")) {
    Assert-ContainsText ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.latestCloseoutEvidence) $needle "engine/agent/manifest.json recommendedNextPlan.latestCloseoutEvidence MAVG GPU culling dispatch implementation evidence"
}
