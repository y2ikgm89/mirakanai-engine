#requires -Version 7.0
#requires -PSEdition Core
# Chapter 105 for check-ai-integration.ps1 static contracts.

$mavgGpuCullingHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp"
$mavgGpuCullingSourceText = Get-AgentSurfaceText "engine/renderer/src/mavg_gpu_culling.cpp"
$mavgGpuCullingTestsText = Get-AgentSurfaceText "tests/unit/mavg_gpu_culling_tests.cpp"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgGpuCullingPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-gpu-culling-indirect-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "MavgGpuCullingClusterBoundsRow",
        "MavgGpuCullingIndirectCommand",
        "MavgGpuCullingIndirectCommandLayout",
        "MavgGpuCullingSyncRequirement",
        "MavgGpuCullingIndirectPlan",
        "MavgGpuCullingIndirectDesc",
        "plan_mavg_gpu_culling_indirect_commands",
        "executed_gpu_culling",
        "executed_indirect_draw",
        "executed_mesh_shader",
        "touched_native_handles"
    )) {
    Assert-ContainsText $mavgGpuCullingHeaderText $needle "engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp"
}

foreach ($needle in @(
        "VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT",
        "VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "indexed_indirect_record_stride_bytes",
        "MavgGpuCullingDiagnosticCode::max_command_count_exceeded",
        "fail_closed(plan)"
    )) {
    Assert-ContainsText $mavgGpuCullingSourceText $needle "engine/renderer/src/mavg_gpu_culling.cpp"
}

foreach ($needle in @(
        "mavg gpu culling indirect planning emits packed indexed command rows",
        "mavg gpu culling indirect planning filters invisible cluster bounds deterministically",
        "mavg gpu culling indirect planning fails closed on invalid culling bounds",
        "mavg gpu culling indirect planning fails closed when command budget is exceeded",
        "mavg gpu culling indirect planning records backend sync requirements for compute produced commands"
    )) {
    Assert-ContainsText $mavgGpuCullingTestsText $needle "tests/unit/mavg_gpu_culling_tests.cpp"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgGpuCullingPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-gpu-culling-indirect-v1.md" }
    )) {
    foreach ($needle in @("mavg_gpu_culling.hpp", "MavgGpuCullingIndirectPlan", "plan_mavg_gpu_culling_indirect_commands", "D3D12/Vulkan", "value-only")) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG GPU culling indirect planning evidence"
    }
    foreach ($needle in @("ExecuteIndirect", "Vulkan indirect draw execution", "mesh shaders", "Nanite")) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG GPU culling non-claim evidence"
    }
}

$aiLoopGpuCullingEvidenceText = $aiLoopFragmentText
if ($manifest.aiOperableProductionLoop.currentActivePlan -eq "docs/superpowers/plans/2026-06-08-mavg-vulkan-indexed-indirect-draw-execution-v1.md") {
    $aiLoopGpuCullingEvidenceText = (([string]$manifest.aiOperableProductionLoop.latestCloseoutEvidence), ([string]$manifest.aiOperableProductionLoop.completedContext), $aiLoopFragmentText) -join " "
}
foreach ($needle in @(
        "mavg-gpu-culling-indirect-v1",
        "docs/superpowers/plans/2026-06-05-mavg-gpu-culling-indirect-v1.md",
        "actual GPU culling dispatch",
        "D3D12 ExecuteIndirect",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $aiLoopGpuCullingEvidenceText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
}

foreach ($needle in @(
        "mavg-gpu-culling-indirect-v1",
        "completed stacked child for value-only packed indexed indirect command planning",
        "actual compute dispatch and backend indirect execution remain follow-up"
    )) {
    Assert-ContainsText $mavgRuntimeLodPlanText $needle "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
}

$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module" }
if (@($rendererModule[0].publicHeaders) -notcontains "engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp") {
    Write-Error "engine/agent/manifest.json MK_renderer publicHeaders missing mavg_gpu_culling.hpp"
}
$rendererManifestText = ((@($rendererModule[0].recentEvidence) -join " "), [string]$rendererModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG GPU Culling Indirect Planning v1",
        "MavgGpuCullingClusterBoundsRow",
        "MavgGpuCullingIndirectPlan",
        "plan_mavg_gpu_culling_indirect_commands",
        "20-byte five-field argument layout",
        "D3D12/Vulkan compute-write-to-indirect-read",
        "without executing GPU culling"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer MAVG GPU culling indirect planning evidence"
}
