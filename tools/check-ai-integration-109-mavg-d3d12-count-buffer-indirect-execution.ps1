#requires -Version 7.0
#requires -PSEdition Core
# Chapter 109 for check-ai-integration.ps1 static contracts.

$d3d12BackendSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_backend.cpp"
$indirectDrawHeaderText = Get-AgentSurfaceText "engine/rhi/include/mirakana/rhi/indirect_draw.hpp"
$indirectDrawSourceText = Get-AgentSurfaceText "engine/rhi/src/indirect_draw.cpp"
$d3d12RhiTestsText = Get-AgentSurfaceText "tests/unit/d3d12_rhi_tests.cpp"
$mavgCountBufferPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-08-mavg-d3d12-count-buffer-indirect-execution-v1.md"
$mavgVulkanPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-08-mavg-vulkan-indexed-indirect-draw-execution-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "ExecuteIndirect",
        "d3d12 rhi indexed indirect draw count buffer requires copy_source upload usage in v1",
        "d3d12 rhi indexed indirect draw count buffer offset must be 4-byte aligned",
        "d3d12 rhi indexed indirect draw count range is outside the count buffer"
    )) {
    Assert-ContainsText $d3d12BackendSourceText $needle "engine/rhi/d3d12/src/d3d12_backend.cpp D3D12 count-buffer ExecuteIndirect evidence"
}

foreach ($needle in @(
        "decode_indexed_indirect_count_buffer_value",
        "effective_indexed_indirect_draw_count",
        "decode_indexed_indirect_draw_commands"
    )) {
    Assert-ContainsText $indirectDrawHeaderText $needle "engine/rhi/include/mirakana/rhi/indirect_draw.hpp count-buffer helper contract"
}

foreach ($needle in @(
        "decode_indexed_indirect_count_buffer_value",
        "effective_indexed_indirect_draw_count"
    )) {
    Assert-ContainsText $indirectDrawSourceText $needle "engine/rhi/src/indirect_draw.cpp count-buffer helper implementation"
}

foreach ($needle in @(
        "d3d12 rhi device executes count-buffer-limited indexed indirect draw into texture readback bytes",
        "d3d12 rhi device executes zero-count indexed indirect draw without submitting visible draws",
        "d3d12 rhi device rejects count buffer without upload copy_source usage",
        "indexed_indirect_count_buffer_reads",
        "last_indexed_indirect_count_buffer_value",
        "last_indexed_indirect_executed_draw_count"
    )) {
    Assert-ContainsText $d3d12RhiTestsText $needle "tests/unit/d3d12_rhi_tests.cpp D3D12 count-buffer indirect execution coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgCountBufferPlanText; Label = "docs/superpowers/plans/2026-06-08-mavg-d3d12-count-buffer-indirect-execution-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-d3d12-count-buffer-indirect-execution-v1",
            "D3D12",
            "ExecuteIndirect",
            "CPU-generated upload",
            "count-buffer"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 count-buffer indirect execution evidence"
    }
    foreach ($needle in @(
            "count-buffer Vulkan execution",
            "actual GPU culling dispatch",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 count-buffer indirect non-claim evidence"
    }
}

foreach ($needle in @(
        "Completed through PR",
        "mavg-vulkan-indexed-indirect-draw-execution-v1",
        "is now owned by the follow-up D3D12 count-buffer execution plan"
    )) {
    Assert-ContainsText $mavgVulkanPlanText $needle "docs/superpowers/plans/2026-06-08-mavg-vulkan-indexed-indirect-draw-execution-v1.md prerequisite transition"
}

foreach ($needle in @(
        "mavg-d3d12-count-buffer-indirect-execution-v1",
        "docs/superpowers/plans/2026-06-08-mavg-d3d12-count-buffer-indirect-execution-v1.md",
        "ID3D12GraphicsCommandList::ExecuteIndirect",
        "CountBufferOffset",
        "count-buffer Vulkan execution"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json active MAVG D3D12 count-buffer indirect draw evidence"
}

foreach ($needle in @(
        "MAVG D3D12 Count-Buffer Indirect Execution v1",
        "ExecuteIndirect",
        "CountBufferOffset",
        "decode_indexed_indirect_count_buffer_value",
        "count-buffer Vulkan execution"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_rhi D3D12 count-buffer indirect draw evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG D3D12 Count-Buffer Indirect Execution v1",
        "ExecuteIndirect",
        "CountBufferOffset",
        "decode_indexed_indirect_count_buffer_value",
        "count-buffer Vulkan execution"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi D3D12 count-buffer indirect draw evidence"
}

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-08-mavg-d3d12-count-buffer-indirect-execution-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must point at the active MAVG D3D12 count-buffer indirect execution plan during execution"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "mavg-d3d12-count-buffer-indirect-execution-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must be mavg-d3d12-count-buffer-indirect-execution-v1 during active MAVG D3D12 count-buffer indirect execution"
}
foreach ($needle in @("MAVG D3D12 Count-Buffer Indirect Execution v1", "mavg-vulkan-indexed-indirect-draw-execution-v1", "ExecuteIndirect", "CountBufferOffset", "count-buffer Vulkan execution", "unsupportedProductionGaps = []")) {
    Assert-ContainsText ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.latestCloseoutEvidence) $needle "engine/agent/manifest.json recommendedNextPlan.latestCloseoutEvidence MAVG D3D12 count-buffer active evidence"
}
