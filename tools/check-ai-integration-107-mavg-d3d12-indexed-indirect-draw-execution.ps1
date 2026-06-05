#requires -Version 7.0
#requires -PSEdition Core
# Chapter 107 for check-ai-integration.ps1 static contracts.

$d3d12BackendHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
$d3d12BackendSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_backend.cpp"
$d3d12TestsText = Get-AgentSurfaceText "tests/unit/d3d12_rhi_tests.cpp"
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
    Assert-ContainsText $d3d12BackendHeaderText $needle "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp D3D12 indexed indirect contract"
}

foreach ($needle in @(
        "IndexedIndirectDrawSignatureRecord",
        "ensure_indexed_indirect_draw_signature",
        "D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED",
        "CreateCommandSignature",
        "ExecuteIndirect",
        "D3D12.IndexedIndirectDrawSignature",
        "d3d12 rhi indexed indirect draw argument buffer requires copy_source upload or storage usage",
        "record_indexed_indirect_draw_stats"
    )) {
    Assert-ContainsText $d3d12BackendSourceText $needle "engine/rhi/d3d12/src/d3d12_backend.cpp D3D12 ExecuteIndirect evidence"
}

foreach ($needle in @(
        "d3d12 rhi device executes indexed indirect draw into texture readback bytes",
        "draw_indexed_indirect",
        "indexed_indirect_commands_executed",
        "indexed_indirect_count_buffer_reads"
    )) {
    Assert-ContainsText $d3d12TestsText $needle "tests/unit/d3d12_rhi_tests.cpp D3D12 indexed indirect execution coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgD3d12PlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-draw-execution-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-d3d12-indexed-indirect-draw-execution-v1",
            "D3D12",
            "ExecuteIndirect",
            "CPU-generated upload",
            "count-buffer"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 indexed indirect draw evidence"
    }
    foreach ($needle in @(
            "Vulkan indirect draw execution",
            "generic GPU culling frameworks",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 indexed indirect non-claim evidence"
    }
}

foreach ($needle in @(
        "Completed stacked prerequisite",
        "mavg-d3d12-indexed-indirect-draw-execution-v1",
        "Native D3D12 `ExecuteIndirect` is now owned by the follow-up D3D12-only execution plan"
    )) {
    Assert-ContainsText $mavgRhiPlanText $needle "docs/superpowers/plans/2026-06-05-mavg-rhi-indirect-draw-v1.md prerequisite transition"
}

foreach ($needle in @(
        "mavg-d3d12-indexed-indirect-draw-execution-v1",
        "docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-draw-execution-v1.md",
        "D3D12 ExecuteIndirect execution",
        "D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED",
        "mavg-d3d12-indexed-indirect-count-buffer-execution-v1",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json active MAVG D3D12 indirect draw evidence"
}

foreach ($needle in @(
        "MAVG D3D12 Indexed Indirect Draw Execution v1",
        "D3D12 ExecuteIndirect execution",
        "D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED",
        "public buffer-state tracking",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_rhi D3D12 indirect draw evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG D3D12 Indexed Indirect Draw Execution v1",
        "D3D12 ExecuteIndirect execution",
        "D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED",
        "visible WARP-backed texture readback proof",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi D3D12 indexed indirect draw evidence"
}

if ($manifest.aiOperableProductionLoop.recommendedNextPlan.completedContext -notlike "*mavg-d3d12-indexed-indirect-draw-execution-v1*") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.completedContext must retain mavg-d3d12-indexed-indirect-draw-execution-v1 prerequisite evidence"
}
