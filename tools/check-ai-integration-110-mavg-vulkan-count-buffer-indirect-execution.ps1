#requires -Version 7.0
#requires -PSEdition Core
# Chapter 110 for check-ai-integration.ps1 static contracts.

$vulkanBackendSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
$indirectDrawHeaderText = Get-AgentSurfaceText "engine/rhi/include/mirakana/rhi/indirect_draw.hpp"
$indirectDrawSourceText = Get-AgentSurfaceText "engine/rhi/src/indirect_draw.cpp"
$backendScaffoldTestsText = Get-AgentSurfaceText "tests/unit/backend_scaffold_tests.cpp"
$mavgVulkanCountBufferPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md"
$mavgD3d12CountBufferPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-08-mavg-d3d12-count-buffer-indirect-execution-v1.md"
$mavgVulkanNoCountPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-08-mavg-vulkan-indexed-indirect-draw-execution-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "vulkan rhi indexed indirect count buffer execution is not implemented",
        "vkCmdDrawIndexedIndirect",
        "read_runtime_buffer",
        "decode_indexed_indirect_draw_commands"
    )) {
    Assert-ContainsText $vulkanBackendSourceText $needle "engine/rhi/vulkan/src/vulkan_backend.cpp Vulkan count-buffer activation fail-closed evidence"
}

foreach ($needle in @(
        "decode_indexed_indirect_count_buffer_value",
        "effective_indexed_indirect_draw_count"
    )) {
    Assert-ContainsText $indirectDrawHeaderText $needle "engine/rhi/include/mirakana/rhi/indirect_draw.hpp count-buffer helper contract"
    Assert-ContainsText $indirectDrawSourceText $needle "engine/rhi/src/indirect_draw.cpp count-buffer helper implementation"
}

foreach ($needle in @(
        "vulkan rhi device rejects indexed indirect count buffer execution until feature gate lands",
        "vulkan rhi indexed indirect count buffer execution is not implemented",
        "MK_VULKAN_TEST_VERTEX_SPV",
        "MK_VULKAN_TEST_FRAGMENT_SPV"
    )) {
    Assert-ContainsText $backendScaffoldTestsText $needle "tests/unit/backend_scaffold_tests.cpp Vulkan count-buffer activation RED coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgVulkanCountBufferPlanText; Label = "docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-vulkan-count-buffer-indirect-execution-v1",
            "Vulkan",
            "vkCmdDrawIndexedIndirectCount",
            "CPU-generated upload",
            "count-buffer"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Vulkan count-buffer indirect execution evidence"
    }
    foreach ($needle in @(
            "compute-generated count",
            "actual GPU culling dispatch",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Vulkan count-buffer indirect non-claim evidence"
    }
}

foreach ($needle in @(
        "**Status:** Active.",
        "mavg-vulkan-count-buffer-indirect-execution-v1",
        "vkCmdDrawIndexedIndirectCount",
        "fail-closed",
        "mavg-d3d12-count-buffer-indirect-execution-v1"
    )) {
    Assert-ContainsText $mavgVulkanCountBufferPlanText $needle "docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md activation contract"
}

foreach ($needle in @(
        "Completed through PR",
        "mavg-vulkan-count-buffer-indirect-execution-v1",
        "Follow-up Vulkan count-buffer execution"
    )) {
    Assert-ContainsText $mavgD3d12CountBufferPlanText $needle "docs/superpowers/plans/2026-06-08-mavg-d3d12-count-buffer-indirect-execution-v1.md sibling transition"
}

foreach ($needle in @(
        "mavg-vulkan-count-buffer-indirect-execution-v1",
        "docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md",
        "MAVG Vulkan Count-Buffer Indirect Execution v1",
        "vkCmdDrawIndexedIndirectCount",
        "mavg-d3d12-count-buffer-indirect-execution-v1",
        "PR #547"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG Vulkan count-buffer activation evidence"
}

foreach ($needle in @(
        "MAVG Vulkan Count-Buffer Indirect Execution v1",
        "vkCmdDrawIndexedIndirectCount",
        "decode_indexed_indirect_count_buffer_value",
        "count-buffer Vulkan execution",
        "D3D12 changes"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_rhi Vulkan count-buffer activation evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Vulkan Count-Buffer Indirect Execution v1",
        "vkCmdDrawIndexedIndirectCount",
        "decode_indexed_indirect_count_buffer_value",
        "count-buffer Vulkan execution",
        "D3D12 changes"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi Vulkan count-buffer activation evidence"
}

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must point at MAVG Vulkan count-buffer indirect execution during activation"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "mavg-vulkan-count-buffer-indirect-execution-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must be mavg-vulkan-count-buffer-indirect-execution-v1 during activation"
}
foreach ($needle in @("MAVG Vulkan Count-Buffer Indirect Execution v1", "mavg-vulkan-count-buffer-indirect-execution-v1", "vkCmdDrawIndexedIndirectCount", "mavg-d3d12-count-buffer-indirect-execution-v1", "PR #547", "count-buffer Vulkan execution", "unsupportedProductionGaps = []")) {
    Assert-ContainsText ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.latestCloseoutEvidence) $needle "engine/agent/manifest.json recommendedNextPlan.latestCloseoutEvidence MAVG Vulkan count-buffer activation evidence"
}
