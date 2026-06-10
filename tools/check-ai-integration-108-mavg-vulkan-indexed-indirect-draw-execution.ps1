#requires -Version 7.0
#requires -PSEdition Core
# Chapter 108 for check-ai-integration.ps1 static contracts.

$vulkanBackendHeaderText = Get-AgentSurfaceText "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp"
$vulkanBackendSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
$backendScaffoldTestsText = Get-AgentSurfaceText "tests/unit/backend_scaffold_tests.cpp"
$mavgVulkanPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-08-mavg-vulkan-indexed-indirect-draw-execution-v1.md"
$mavgD3d12PlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-draw-execution-v1.md"
$mavgRhiPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-rhi-indirect-draw-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "draw_indexed_indirect",
        "indexed_indirect_draw_calls",
        "indexed_indirect_commands_executed",
        "last_indexed_indirect_max_draw_count"
    )) {
    Assert-ContainsText $vulkanBackendHeaderText $needle "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp Vulkan indexed indirect contract"
}

foreach ($needle in @(
        "vkCmdDrawIndexedIndirect",
        "VkDrawIndexedIndirectCommand",
        "read_runtime_buffer",
        "decode_indexed_indirect_draw_commands",
        "vulkan rhi indexed indirect count buffer execution is not implemented",
        "vulkan rhi indexed indirect draw argument buffer requires copy_source upload usage in v1",
        "record_indexed_indirect_draw_stats"
    )) {
    Assert-ContainsText $vulkanBackendSourceText $needle "engine/rhi/vulkan/src/vulkan_backend.cpp Vulkan vkCmdDrawIndexedIndirect evidence"
}

foreach ($needle in @(
        "vulkan rhi device executes indexed indirect draw into texture readback bytes",
        "draw_indexed_indirect",
        "indexed_indirect_commands_executed",
        "indexed_indirect_count_buffer_reads",
        "MK_VULKAN_TEST_VERTEX_SPV",
        "MK_VULKAN_TEST_FRAGMENT_SPV"
    )) {
    Assert-ContainsText $backendScaffoldTestsText $needle "tests/unit/backend_scaffold_tests.cpp Vulkan indexed indirect execution coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgVulkanPlanText; Label = "docs/superpowers/plans/2026-06-08-mavg-vulkan-indexed-indirect-draw-execution-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-vulkan-indexed-indirect-draw-execution-v1",
            "Vulkan",
            "vkCmdDrawIndexedIndirect",
            "CPU-generated upload",
            "count-buffer"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Vulkan indexed indirect draw evidence"
    }
    foreach ($needle in @(
            "count-buffer Vulkan execution",
            "actual GPU culling dispatch",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Vulkan indexed indirect non-claim evidence"
    }
}

foreach ($needle in @(
        "Completed through PR",
        "mavg-d3d12-indexed-indirect-draw-execution-v1",
        'Native Vulkan `vkCmdDrawIndexedIndirect` completed through the Vulkan-only execution plan'
    )) {
    Assert-ContainsText $mavgD3d12PlanText $needle "docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-draw-execution-v1.md prerequisite transition"
}

foreach ($needle in @(
        "mavg-vulkan-indexed-indirect-draw-execution-v1",
        "docs/superpowers/plans/2026-06-08-mavg-vulkan-indexed-indirect-draw-execution-v1.md",
        "MAVG Vulkan Indexed Indirect Draw Execution v1",
        "vkCmdDrawIndexedIndirect",
        "count-buffer Vulkan execution",
        "PR #541"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG Vulkan indirect draw closeout evidence"
}

foreach ($needle in @(
        "MAVG Vulkan Indexed Indirect Draw Execution v1",
        "Vulkan vkCmdDrawIndexedIndirect execution",
        "vkCmdDrawIndexedIndirect",
        "public buffer-state tracking",
        "D3D12 changes"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_rhi Vulkan indirect draw evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Vulkan Indexed Indirect Draw Execution v1",
        "Vulkan vkCmdDrawIndexedIndirect execution",
        "vkCmdDrawIndexedIndirect",
        "SPIR-V environment-gated visible texture readback proof",
        "D3D12 changes"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi Vulkan indexed indirect draw evidence"
}

foreach ($needle in @("mavg-vulkan-indexed-indirect-draw-execution-v1", "vkCmdDrawIndexedIndirect", "count-buffer Vulkan execution", "mavg-d3d12-indexed-indirect-draw-execution-v1", "mavg-d3d12-count-buffer-indirect-execution-v1", "PR #541", "PR #547")) {
    Assert-ContainsText ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.latestCloseoutEvidence + " " + [string]$manifest.aiOperableProductionLoop.recommendedNextPlan.completedContext) $needle "engine/agent/manifest.json recommendedNextPlan MAVG Vulkan completed prerequisite evidence"
}
