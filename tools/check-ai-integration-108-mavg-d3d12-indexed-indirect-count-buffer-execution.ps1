#requires -Version 7.0
#requires -PSEdition Core
# Chapter 108 for check-ai-integration.ps1 static contracts.

$d3d12BackendHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
$d3d12BackendSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_backend.cpp"
$d3d12TestsText = Get-AgentSurfaceText "tests/unit/d3d12_rhi_tests.cpp"
$mavgD3d12CountPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-count-buffer-execution-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "draw_indexed_indirect",
        "NativeResourceHandle count_buffer",
        "indexed_indirect_count_buffer_reads",
        "last_indexed_indirect_count_buffer_value"
    )) {
    Assert-ContainsText $d3d12BackendHeaderText $needle "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp D3D12 indexed indirect count-buffer contract"
}

foreach ($needle in @(
        "indexed_indirect_count_range_end",
        "read_indexed_indirect_count_value",
        "std::min(count_buffer_value, desc.max_draw_count)",
        "ExecuteIndirect(signature, desc.max_draw_count",
        "desc.count_buffer_offset",
        "d3d12 rhi indexed indirect draw count buffer requires indirect usage",
        "d3d12 rhi indexed indirect draw count buffer requires copy_source upload usage in v1",
        "d3d12 rhi indexed indirect draw count range is outside the count buffer"
    )) {
    Assert-ContainsText $d3d12BackendSourceText $needle "engine/rhi/d3d12/src/d3d12_backend.cpp D3D12 count-buffer ExecuteIndirect evidence"
}
if ($d3d12BackendSourceText -like "*d3d12 rhi indexed indirect count buffer execution is not implemented*") {
    Write-Error "engine/rhi/d3d12/src/d3d12_backend.cpp must not retain the old count-buffer not-implemented gate"
}

foreach ($needle in @(
        "d3d12 rhi device executes indexed indirect count buffer draw into texture readback bytes",
        "d3d12 rhi device executes zero indexed indirect count buffer without drawing",
        "d3d12 rhi device clamps indexed indirect count buffer draw count to max draw count",
        "d3d12 rhi device rejects invalid indexed indirect count buffer descriptions",
        "require_black_center_pixel",
        "indexed_indirect_count_buffer_reads == 1",
        "last_indexed_indirect_count_buffer_value == 7"
    )) {
    Assert-ContainsText $d3d12TestsText $needle "tests/unit/d3d12_rhi_tests.cpp D3D12 indexed indirect count-buffer coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgD3d12CountPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-count-buffer-execution-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-d3d12-indexed-indirect-count-buffer-execution-v1",
            "D3D12",
            "ExecuteIndirect",
            "count-buffer execution",
            "CPU-generated upload",
            "zero-count execution",
            "min(count, MaxCommandCount)",
            "fail-closed count-buffer"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 indexed indirect count-buffer evidence"
    }
    foreach ($needle in @(
            "GPU-generated count buffers",
            "compute-generated indirect buffers",
            "Vulkan indirect draw execution",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 indexed indirect count-buffer non-claim evidence"
    }
}

foreach ($needle in @(
        "mavg-d3d12-indexed-indirect-count-buffer-execution-v1",
        "docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-count-buffer-execution-v1.md",
        "D3D12 ExecuteIndirect count-buffer execution",
        "zero-count execution",
        "min(count, MaxCommandCount)",
        "GPU-generated count buffers",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json active MAVG D3D12 count-buffer evidence"
}

foreach ($needle in @(
        "MAVG D3D12 Indexed Indirect Count Buffer Execution v1",
        "D3D12 ExecuteIndirect count-buffer execution",
        "zero-count execution",
        "min(count, MaxCommandCount)",
        "public buffer-state tracking",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_rhi D3D12 count-buffer evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG D3D12 Indexed Indirect Count Buffer Execution v1",
        "D3D12 ExecuteIndirect count-buffer execution",
        "zero-count execution",
        "min(count, MaxCommandCount)",
        "visible WARP-backed texture readback proof",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi D3D12 indexed indirect count-buffer evidence"
}

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-count-buffer-execution-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must point at mavg-d3d12-indexed-indirect-count-buffer-execution-v1"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "mavg-d3d12-indexed-indirect-count-buffer-execution-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must be mavg-d3d12-indexed-indirect-count-buffer-execution-v1"
}
