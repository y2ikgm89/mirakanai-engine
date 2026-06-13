#requires -Version 7.0
#requires -PSEdition Core
# Chapter 114 for check-ai-integration.ps1 static contracts.

$vulkanDispatchHeaderText = Get-AgentSurfaceText "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_gpu_culling_dispatch.hpp"
$vulkanBackendSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
$indirectDrawHeaderText = Get-AgentSurfaceText "engine/rhi/include/mirakana/rhi/indirect_draw.hpp"
$mavgVulkanConsumptionTestsText = Get-AgentSurfaceText "tests/unit/mavg_vulkan_compute_generated_indirect_consumption_tests.cpp"
$cmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-vulkan-compute-generated-indirect-consumption-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$mavgProductionLoopFragmentSurface = if ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "environment-commercial-excellence-v1") {
    @{ Text = $aiLoopFragmentText; Label = "production loop fragment" }
} else {
    @{ Text = (([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.completedContext), $planText) -join " "; Label = "production loop completed context" }
}
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "external_argument_buffer",
        "external_count_buffer",
        "leave_indirect_argument_state_for_consumption",
        "dispatch_mavg_gpu_culling_indirect(IRhiDevice& device"
    )) {
    Assert-ContainsText $vulkanDispatchHeaderText $needle "Vulkan MAVG compute-generated indirect consumption public RHI-owned buffer contract"
}

foreach ($needle in @(
        "is_compute_generated_indexed_indirect_buffer"
    )) {
    Assert-ContainsText $indirectDrawHeaderText $needle "shared compute-generated indexed indirect buffer classifier"
}

foreach ($needle in @(
        "dispatch_mavg_gpu_culling_indirect(IRhiDevice& device",
        "vulkan_pipeline_stage2_compute_shader_bit",
        "vulkan_access2_shader_write_bit",
        "vulkan_pipeline_stage2_draw_indirect_bit",
        "vulkan_access2_indirect_command_read_bit",
        "record_runtime_buffer_memory_barrier2",
        "is_compute_generated_indexed_indirect_buffer",
        "indexed_indirect_count_buffer_reads",
        "last_indexed_indirect_executed_draw_count = 0U"
    )) {
    Assert-ContainsText $vulkanBackendSourceText $needle "Vulkan RHI compute-generated indirect consumption implementation"
}

foreach ($needle in @(
        "mavg vulkan dispatch plus draw consumes compute generated indirect buffers",
        "MK_VULKAN_TEST_MAVG_GPU_CULLING_DISPATCH_SPV",
        "MK_VULKAN_TEST_VERTEX_SPV",
        "MK_VULKAN_TEST_FRAGMENT_SPV",
        "BufferUsage::indirect | mirakana::rhi::BufferUsage::storage",
        "leave_indirect_argument_state_for_consumption = true",
        "draw_indexed_indirect"
    )) {
    Assert-ContainsText $mavgVulkanConsumptionTestsText $needle "MAVG Vulkan compute-generated indirect consumption test coverage"
}

foreach ($needle in @(
        "MK_mavg_vulkan_compute_generated_indirect_consumption_tests",
        "tests/unit/mavg_vulkan_compute_generated_indirect_consumption_tests.cpp",
        "MK_rhi_vulkan"
    )) {
    Assert-ContainsText $cmakeText $needle "MAVG Vulkan compute-generated indirect consumption CMake target"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "implementation plan" },
        @{ Text = $planRegistryText; Label = "plan registry" },
        @{ Text = $currentCapabilitiesText; Label = "current capabilities" },
        @{ Text = $roadmapText; Label = "roadmap" },
        @{ Text = $mavgArchitectureSpecText; Label = "MAVG architecture spec" },
        @{ Text = $masterPlanText; Label = "MAVG master plan" },
        $mavgProductionLoopFragmentSurface,
        @{ Text = $modulesFragmentText; Label = "modules fragment" }
    )) {
    foreach ($needle in @(
            "mavg-vulkan-compute-generated-indirect-consumption-v1",
            "MAVG Vulkan Compute-Generated Indirect Consumption v1",
            "MK_mavg_vulkan_compute_generated_indirect_consumption_tests",
            "is_compute_generated_indexed_indirect_buffer",
            "leave_indirect_argument_state_for_consumption",
            "mesh shaders",
            "Metal readiness",
            "Nanite",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Vulkan compute-generated indirect consumption evidence and non-claims"
    }
}

if ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "environment-commercial-excellence-v1") {
    if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md") {
        Write-Error "engine/agent/manifest.json currentActivePlan must return to the production-completion master plan after MAVG Vulkan compute-generated indirect consumption closeout"
    }
    if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "next-production-gap-selection") {
        Write-Error "engine/agent/manifest.json recommendedNextPlan.id must be next-production-gap-selection after MAVG Vulkan compute-generated indirect consumption closeout"
    }
}
