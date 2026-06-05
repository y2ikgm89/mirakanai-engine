#requires -Version 7.0
#requires -PSEdition Core
# Chapter 109 for check-ai-integration.ps1 static contracts.

$d3d12BackendHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
$d3d12BackendSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_backend.cpp"
$d3d12TestsText = Get-AgentSurfaceText "tests/unit/d3d12_rhi_tests.cpp"
$mavgD3d12ComputePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-d3d12-compute-generated-indirect-execution-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "buffer_transitions",
        "storage_buffer_uav_write_marks",
        "storage_buffer_uav_transitions",
        "storage_buffer_uav_barriers",
        "indexed_indirect_gpu_generated_draw_calls",
        "indexed_indirect_gpu_generated_count_buffer_uses",
        "indexed_indirect_argument_buffer_transitions",
        "device_context_stats(IRhiDevice& device)",
        "prepare_storage_buffer_uav_write",
        "bool gpu_generated_buffers"
    )) {
    Assert-ContainsText $d3d12BackendHeaderText $needle "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp D3D12 compute-generated indirect contract"
}

foreach ($needle in @(
        "record_indexed_indirect_gpu_generated_draw_stats",
        "D3D12_RESOURCE_STATE_UNORDERED_ACCESS",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "D3D12_RESOURCE_BARRIER_TYPE_UAV",
        "prepare_indirect_buffer_for_execute_indirect",
        "BufferUsage::storage",
        "BufferUsage::indirect",
        "bound_compute_storage_buffers_",
        "has_flag(desc.usage, BufferUsage::indirect)",
        "d3d12 rhi indexed indirect draw argument and count buffers must use the same generation path",
        "!argument_gpu_generated",
        "ExecuteIndirect(signature, desc.max_draw_count"
    )) {
    Assert-ContainsText $d3d12BackendSourceText $needle "engine/rhi/d3d12/src/d3d12_backend.cpp D3D12 compute-generated indirect evidence"
}

foreach ($needle in @(
        "d3d12 rhi device executes compute generated indexed indirect count buffer draw into texture readback bytes",
        "d3d12 rhi device rejects compute generated indexed indirect buffers missing required usage",
        "compile_indexed_indirect_command_count_compute_shader",
        "wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence)",
        "storage_buffer_uav_barriers == 1",
        "indexed_indirect_gpu_generated_draw_calls == 1",
        "indexed_indirect_commands_executed == 0"
    )) {
    Assert-ContainsText $d3d12TestsText $needle "tests/unit/d3d12_rhi_tests.cpp D3D12 compute-generated indirect coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgD3d12ComputePlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-d3d12-compute-generated-indirect-execution-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-d3d12-compute-generated-indirect-execution-v1",
            "D3D12",
            "compute-generated",
            "BufferUsage::storage | BufferUsage::indirect",
            "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
            "WARP-backed",
            "fail-closed",
            "no CPU decode/stat overclaim"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 compute-generated indirect evidence"
    }
    foreach ($needle in @(
            "generic GPU-generated count-buffer systems",
            "generic storage-buffer state management",
            "Vulkan indirect draw execution",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 compute-generated indirect non-claim evidence"
    }
}

foreach ($needle in @(
        "mavg-d3d12-compute-generated-indirect-execution-v1",
        "docs/superpowers/plans/2026-06-05-mavg-d3d12-compute-generated-indirect-execution-v1.md",
        "MAVG D3D12 Compute-Generated Indirect Execution v1",
        "BufferUsage::storage | BufferUsage::indirect",
        "D3D12_RESOURCE_STATE_UNORDERED_ACCESS",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "generic storage-buffer state management",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json active MAVG D3D12 compute-generated indirect evidence"
}

foreach ($needle in @(
        "MAVG D3D12 Compute-Generated Indirect Execution v1",
        "D3D12_RESOURCE_STATE_UNORDERED_ACCESS",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "UAV barrier evidence",
        "same-buffer argument/count offset execution",
        "generic storage-buffer state management",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_rhi D3D12 compute-generated indirect evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG D3D12 Compute-Generated Indirect Execution v1",
        "D3D12_RESOURCE_STATE_UNORDERED_ACCESS",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "visible WARP-backed texture readback proof",
        "no CPU decode/stat overclaim",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi D3D12 compute-generated indirect evidence"
}

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-05-mavg-d3d12-compute-generated-indirect-execution-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must point at mavg-d3d12-compute-generated-indirect-execution-v1"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "mavg-d3d12-compute-generated-indirect-execution-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must be mavg-d3d12-compute-generated-indirect-execution-v1"
}
