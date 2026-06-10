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
        "cmd_draw_indexed_indirect_count",
        "vkCmdDrawIndexedIndirectCount",
        "vulkan rhi indexed indirect count buffer requires copy_source upload usage in v1",
        "vulkan rhi indexed indirect count buffer offset must be 4-byte aligned",
        "vulkan rhi indexed indirect count range is outside the count buffer",
        "supports_indexed_indirect_count_buffer_draw",
        "decode_indexed_indirect_draw_commands",
        "read_runtime_buffer"
    )) {
    Assert-ContainsText $vulkanBackendSourceText $needle "engine/rhi/vulkan/src/vulkan_backend.cpp Vulkan count-buffer vkCmdDrawIndexedIndirectCount evidence"
}

foreach ($needle in @(
        "decode_indexed_indirect_count_buffer_value",
        "effective_indexed_indirect_draw_count"
    )) {
    Assert-ContainsText $indirectDrawHeaderText $needle "engine/rhi/include/mirakana/rhi/indirect_draw.hpp count-buffer helper contract"
    Assert-ContainsText $indirectDrawSourceText $needle "engine/rhi/src/indirect_draw.cpp count-buffer helper implementation"
}

foreach ($needle in @(
        "vulkan rhi device executes count-buffer-limited indexed indirect draw into texture readback bytes",
        "vulkan rhi device executes zero-count indexed indirect draw without submitting visible draws",
        "vulkan rhi device rejects count buffer without upload copy_source usage",
        "indexed_indirect_count_buffer_reads",
        "last_indexed_indirect_count_buffer_value",
        "last_indexed_indirect_executed_draw_count",
        "MK_VULKAN_TEST_VERTEX_SPV",
        "MK_VULKAN_TEST_FRAGMENT_SPV"
    )) {
    Assert-ContainsText $backendScaffoldTestsText $needle "tests/unit/backend_scaffold_tests.cpp Vulkan count-buffer indirect execution coverage"
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
            "compute-generated",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Vulkan count-buffer indirect non-claim evidence"
    }
}

foreach ($needle in @(
        "**Status:** Completed.",
        "Completed through PR #552",
        "mavg-gpu-culling-dispatch-v1",
        "PR #556",
        "vkCmdDrawIndexedIndirectCount",
        "mavg-d3d12-count-buffer-indirect-execution-v1"
    )) {
    Assert-ContainsText $mavgVulkanCountBufferPlanText $needle "docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md closeout contract"
}

foreach ($needle in @(
        "Completed through PR",
        "mavg-vulkan-count-buffer-indirect-execution-v1",
        "PR #552"
    )) {
    Assert-ContainsText $mavgD3d12CountBufferPlanText $needle "docs/superpowers/plans/2026-06-08-mavg-d3d12-count-buffer-indirect-execution-v1.md sibling transition"
}

foreach ($needle in @(
        "mavg-vulkan-count-buffer-indirect-execution-v1",
        "docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md",
        "MAVG Vulkan Count-Buffer Indirect Execution v1",
        "vkCmdDrawIndexedIndirectCount",
        "PR #552"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG Vulkan count-buffer closeout evidence"
}

foreach ($needle in @(
        "MAVG Vulkan Count-Buffer Indirect Execution v1",
        "vkCmdDrawIndexedIndirectCount",
        "decode_indexed_indirect_count_buffer_value",
        "PR #552"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_rhi Vulkan count-buffer execution evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Vulkan Count-Buffer Indirect Execution v1",
        "vkCmdDrawIndexedIndirectCount",
        "decode_indexed_indirect_count_buffer_value",
        "PR #552"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi Vulkan count-buffer execution evidence"
}

foreach ($needle in @("MAVG Vulkan Count-Buffer Indirect Execution v1", "PR #552", "mavg-vulkan-count-buffer-indirect-execution-v1", "vkCmdDrawIndexedIndirectCount", "mavg-d3d12-count-buffer-indirect-execution-v1", "PR #547", "unsupportedProductionGaps = []")) {
    Assert-ContainsText ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.latestCloseoutEvidence) $needle "engine/agent/manifest.json recommendedNextPlan.latestCloseoutEvidence MAVG Vulkan count-buffer closeout evidence"
}
