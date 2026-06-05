#requires -Version 7.0
#requires -PSEdition Core
# Chapter 110 for check-ai-integration.ps1 static contracts.

$d3d12TestsText = Get-AgentSurfaceText "tests/unit/d3d12_rhi_tests.cpp"
$mavgGpuCullingPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-d3d12-gpu-culling-execution-v1.md"
$mavgComputePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-d3d12-compute-generated-indirect-execution-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "d3d12 rhi device executes mavg gpu culling dispatch into indexed indirect draw",
        "compile_mavg_gpu_culling_compaction_compute_shader",
        "create_mavg_gpu_culling_reference_rows",
        "plan_mavg_gpu_culling_indirect_commands",
        "encode_mavg_gpu_culling_candidate_rows",
        "wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence)",
        "draw_indexed_indirect(mirakana::rhi::IndexedIndirectDrawDesc",
        "require_black_probe_pixel(bytes, 8, 8)",
        "visible_cluster_count == 1",
        "culled_cluster_count == 1",
        "output_indirect_count == 1",
        "indexed_indirect_gpu_generated_count_buffer_uses == 1",
        "indexed_indirect_commands_executed == 0"
    )) {
    Assert-ContainsText $d3d12TestsText $needle "tests/unit/d3d12_rhi_tests.cpp MAVG D3D12 GPU culling execution coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgGpuCullingPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-d3d12-gpu-culling-execution-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-d3d12-gpu-culling-execution-v1",
            "D3D12",
            "MAVG",
            "GPU culling dispatch",
            "indexed indirect",
            "WARP-backed",
            "visible",
            "culled",
            "BufferUsage::storage | BufferUsage::indirect"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 GPU culling execution evidence"
    }
    foreach ($needle in @(
            "generic GPU-generated count-buffer systems",
            "generic storage-buffer state management",
            "Vulkan indirect draw execution",
            "Metal",
            "mesh shaders",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 GPU culling execution non-claim evidence"
    }
}

foreach ($needle in @(
        "mavg-d3d12-compute-generated-indirect-execution-v1",
        "Published stacked draft PR #451"
    )) {
    Assert-ContainsText $mavgComputePlanText $needle "docs/superpowers/plans/2026-06-05-mavg-d3d12-compute-generated-indirect-execution-v1.md completed prerequisite evidence"
}

foreach ($needle in @(
        "mavg-d3d12-gpu-culling-execution-v1",
        "docs/superpowers/plans/2026-06-05-mavg-d3d12-gpu-culling-execution-v1.md",
        "MAVG D3D12 GPU Culling Execution v1",
        "MavgGpuCullingIndirectPlan",
        "GPU culling dispatch",
        "same-buffer argument/count",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json active MAVG D3D12 GPU culling evidence"
}

foreach ($needle in @(
        "MAVG D3D12 GPU Culling Execution v1",
        "MavgGpuCullingIndirectPlan",
        "GPU culling dispatch",
        "visible WARP-backed texture readback proof",
        "generic storage-buffer state management",
        "Vulkan indirect draw execution",
        "Nanite"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_rhi MAVG D3D12 GPU culling evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG D3D12 GPU Culling Execution v1",
        "MavgGpuCullingIndirectPlan",
        "GPU culling dispatch",
        "visible WARP-backed texture readback proof",
        "no CPU decode/stat overclaim",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi MAVG D3D12 GPU culling evidence"
}
