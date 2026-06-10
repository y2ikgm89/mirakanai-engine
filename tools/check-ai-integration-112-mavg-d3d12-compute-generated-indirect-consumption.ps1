#requires -Version 7.0
#requires -PSEdition Core
# Chapter 112 for check-ai-integration.ps1 static contracts.

$indirectDrawHeaderText = Get-AgentSurfaceText "engine/rhi/include/mirakana/rhi/indirect_draw.hpp"
$indirectDrawSourceText = Get-AgentSurfaceText "engine/rhi/src/indirect_draw.cpp"
$d3d12BackendSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_backend.cpp"
$d3d12MavgGpuCullingDispatchHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_mavg_gpu_culling_dispatch.hpp"
$d3d12MavgGpuCullingDispatchSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp"
$computeGeneratedTestsText = Get-AgentSurfaceText "tests/unit/mavg_d3d12_compute_generated_indirect_consumption_tests.cpp"
$mavgComputeGeneratedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md"
$mavgVulkanGpuCullingDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md"
$mavgGpuCullingDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "is_cpu_upload_indexed_indirect_buffer",
        "is_compute_generated_indexed_indirect_buffer"
    )) {
    Assert-ContainsText $indirectDrawHeaderText $needle "engine/rhi/include/mirakana/rhi/indirect_draw.hpp compute-generated indirect buffer contract"
    Assert-ContainsText $indirectDrawSourceText $needle "engine/rhi/src/indirect_draw.cpp compute-generated indirect buffer contract"
}

foreach ($needle in @(
        "is_compute_generated_indexed_indirect_buffer",
        "compute_generated_indirect",
        "D3D12_RESOURCE_BARRIER_TYPE_UAV",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "native_buffer_resource",
        "compute-generated indirect|storage usage"
    )) {
    Assert-ContainsText $d3d12BackendSourceText $needle "engine/rhi/d3d12/src/d3d12_backend.cpp D3D12 compute-generated indirect consumption evidence"
}

foreach ($needle in @(
        "external_argument_buffer",
        "external_count_buffer",
        "leave_indirect_argument_state_for_consumption",
        "dispatch_mavg_gpu_culling_indirect(IRhiDevice& device"
    )) {
    Assert-ContainsText $d3d12MavgGpuCullingDispatchHeaderText $needle "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_mavg_gpu_culling_dispatch.hpp RHI dispatch integration contract"
}

foreach ($needle in @(
        "leave_indirect_argument_state_for_consumption",
        "dispatch_mavg_gpu_culling_indirect_on_rhi_device",
        "dispatch_mavg_gpu_culling_indirect_environment",
        "executed_gpu_culling = true"
    )) {
    Assert-ContainsText $d3d12MavgGpuCullingDispatchSourceText $needle "engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp RHI dispatch integration evidence"
}

foreach ($needle in @(
        "mavg d3d12 dispatch plus draw renders visible geometry from compute generated indirect buffers",
        "mavg d3d12 dispatch plus draw respects culled cluster count on compute generated indirect buffers",
        "dispatch_mavg_gpu_culling_indirect",
        "native_buffer_resource",
        "leave_indirect_argument_state_for_consumption",
        "BufferUsage::indirect | mirakana::rhi::BufferUsage::storage",
        "d3d12_warp_available"
    )) {
    Assert-ContainsText $computeGeneratedTestsText $needle "tests/unit/mavg_d3d12_compute_generated_indirect_consumption_tests.cpp WARP end-to-end proof"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgComputeGeneratedPlanText; Label = "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-d3d12-compute-generated-indirect-consumption-v1",
            "D3D12",
            "compute-generated",
            "dispatch_mavg_gpu_culling_indirect",
            "MK_mavg_d3d12_compute_generated_indirect_consumption_tests",
            "PR #560"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 compute-generated indirect consumption implementation evidence"
    }
    foreach ($needle in @(
            "Vulkan compute dispatch",
            "Vulkan compute-generated",
            "mesh shaders",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG D3D12 compute-generated indirect consumption non-claim evidence"
    }
}

foreach ($needle in @(
        "**Status:** Completed.",
        "mavg-d3d12-compute-generated-indirect-consumption-v1",
        "MK_mavg_d3d12_compute_generated_indirect_consumption_tests",
        "leave_indirect_argument_state_for_consumption",
        "native_buffer_resource",
        "is_compute_generated_indexed_indirect_buffer",
        "PR #560",
        "mavg-vulkan-gpu-culling-dispatch-v1",
        "PR #561"
    )) {
    Assert-ContainsText $mavgComputeGeneratedPlanText $needle "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md closeout contract"
}

foreach ($needle in @(
        "mavg-d3d12-compute-generated-indirect-consumption-v1",
        "PR #560"
    )) {
    Assert-ContainsText $mavgGpuCullingDispatchPlanText $needle "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md sibling transition"
}

foreach ($needle in @(
        "mavg-vulkan-gpu-culling-dispatch-v1",
        "PR #561"
    )) {
    Assert-ContainsText $mavgComputeGeneratedPlanText $needle "docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md Vulkan sibling transition"
}

foreach ($needle in @(
        "MAVG D3D12 Compute-Generated Indirect Consumption v1",
        "MK_mavg_d3d12_compute_generated_indirect_consumption_tests",
        "dispatch_mavg_gpu_culling_indirect",
        "leave_indirect_argument_state_for_consumption",
        "native_buffer_resource",
        "compute-generated",
        "PR #560"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_renderer compute-generated consumption closeout evidence"
}

$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module" }
$rendererManifestText = ((@($rendererModule[0].recentEvidence) -join " "), [string]$rendererModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG D3D12 Compute-Generated Indirect Consumption v1",
        "MK_mavg_d3d12_compute_generated_indirect_consumption_tests",
        "dispatch_mavg_gpu_culling_indirect",
        "leave_indirect_argument_state_for_consumption",
        "compute-generated",
        "PR #560"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer compute-generated consumption closeout evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG D3D12 Compute-Generated Indirect Consumption v1",
        "native_buffer_resource",
        "is_compute_generated_indexed_indirect_buffer",
        "MK_mavg_d3d12_compute_generated_indirect_consumption_tests"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi compute-generated consumption closeout evidence"
}
