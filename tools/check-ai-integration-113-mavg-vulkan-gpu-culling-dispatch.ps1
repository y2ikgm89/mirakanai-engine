#requires -Version 7.0
#requires -PSEdition Core
# Chapter 113 for check-ai-integration.ps1 static contracts.

$mavgGpuCullingHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp"
$mavgGpuCullingSourceText = Get-AgentSurfaceText "engine/renderer/src/mavg_gpu_culling.cpp"
$mavgGpuCullingTestsText = Get-AgentSurfaceText "tests/unit/mavg_gpu_culling_tests.cpp"
$mavgGpuCullingDispatchTestsText = Get-AgentSurfaceText "tests/unit/mavg_gpu_culling_dispatch_tests.cpp"
$d3d12MavgGpuCullingDispatchSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp"
$vulkanMavgGpuCullingDispatchHeaderText = Get-AgentSurfaceText "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_gpu_culling_dispatch.hpp"
$vulkanMavgGpuCullingDispatchSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_mavg_gpu_culling_dispatch.cpp"
$vulkanBackendSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
$mavgVulkanGpuCullingDispatchTestsText = Get-AgentSurfaceText "tests/unit/mavg_vulkan_gpu_culling_dispatch_tests.cpp"
$mavgVulkanGpuCullingDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md"
$mavgGpuCullingDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "build_mavg_gpu_culling_dispatch_cluster_rows",
        "encode_mavg_gpu_culling_indirect_argument_buffer_bytes",
        "encode_mavg_gpu_culling_indirect_count_buffer_bytes"
    )) {
    Assert-ContainsText $mavgGpuCullingHeaderText $needle "engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp MAVG GPU culling dispatch encode contract"
    Assert-ContainsText $mavgGpuCullingSourceText $needle "engine/renderer/src/mavg_gpu_culling.cpp MAVG GPU culling dispatch encode helpers"
}

foreach ($needle in @(
        "MK_REQUIRE(!plan.executed_gpu_culling)",
        "mavg gpu culling indirect planning records backend sync requirements for compute produced commands"
    )) {
    Assert-ContainsText $mavgGpuCullingTestsText $needle "tests/unit/mavg_gpu_culling_tests.cpp MAVG GPU culling value-only planner coverage"
}

foreach ($needle in @(
        "dispatch_mavg_gpu_culling_indirect",
        "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        "executed_gpu_culling = true"
    )) {
    Assert-ContainsText $d3d12MavgGpuCullingDispatchSourceText $needle "engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp completed D3D12 GPU culling dispatch baseline"
}

foreach ($needle in @(
        "dispatch_mavg_gpu_culling_indirect",
        "VulkanMavgGpuCullingDispatchDesc",
        "VulkanMavgGpuCullingDispatchResult",
        "executed_gpu_culling"
    )) {
    Assert-ContainsText $vulkanMavgGpuCullingDispatchHeaderText $needle "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_gpu_culling_dispatch.hpp Vulkan MAVG GPU culling dispatch API"
}

foreach ($needle in @(
        "dispatch_mavg_gpu_culling_indirect",
        "record_runtime_buffer_memory_barrier2",
        "record_runtime_compute_dispatch",
        "VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT",
        "VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT",
        "resource_barriers_recorded",
        "executed_gpu_culling = true"
    )) {
    Assert-ContainsText $vulkanMavgGpuCullingDispatchSourceText $needle "engine/rhi/vulkan/src/vulkan_mavg_gpu_culling_dispatch.cpp Vulkan MAVG GPU culling dispatch evidence"
}

foreach ($needle in @(
        "record_runtime_buffer_memory_barrier2",
        "vulkan_pipeline_stage2_draw_indirect_bit",
        "vulkan_access2_indirect_command_read_bit",
        "buffer_memory_barrier_count"
    )) {
    Assert-ContainsText $vulkanBackendSourceText $needle "engine/rhi/vulkan/src/vulkan_backend.cpp Vulkan synchronization2 buffer barrier support"
}

foreach ($needle in @(
        "mavg vulkan gpu culling dispatch writes visible cluster indirect bytes with configured spir-v",
        "mavg vulkan gpu culling dispatch reduces count for culled clusters with configured spir-v",
        "MK_REQUIRE(dispatch.executed_gpu_culling)",
        "MK_VULKAN_TEST_MAVG_GPU_CULLING_DISPATCH_SPV",
        "encode_mavg_gpu_culling_indirect_argument_buffer_bytes",
        "encode_mavg_gpu_culling_indirect_count_buffer_bytes",
        "dispatch_mavg_gpu_culling_indirect"
    )) {
    Assert-ContainsText $mavgVulkanGpuCullingDispatchTestsText $needle "tests/unit/mavg_vulkan_gpu_culling_dispatch_tests.cpp Vulkan MAVG GPU culling dispatch SPIR-V proof"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgVulkanGpuCullingDispatchPlanText; Label = "docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-vulkan-gpu-culling-dispatch-v1",
            "Vulkan",
            "dispatch_mavg_gpu_culling_indirect",
            "MK_mavg_vulkan_gpu_culling_dispatch_tests",
            "VK_KHR_synchronization2",
            "compute dispatch"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Vulkan GPU culling dispatch implementation evidence"
    }
    foreach ($needle in @(
            "Vulkan compute-generated indirect consumption",
            "compute-generated indirect consumption",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Vulkan GPU culling dispatch non-claim evidence"
    }
}

foreach ($needle in @(
        "**Status:** Active.",
        "mavg-vulkan-gpu-culling-dispatch-v1",
        "dispatch_mavg_gpu_culling_indirect",
        "MK_mavg_vulkan_gpu_culling_dispatch_tests",
        "MK_VULKAN_TEST_MAVG_GPU_CULLING_DISPATCH_SPV",
        "mavg-gpu-culling-dispatch-v1",
        "PR #556"
    )) {
    Assert-ContainsText $mavgVulkanGpuCullingDispatchPlanText $needle "docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md implementation contract"
}

foreach ($needle in @(
        "mavg-vulkan-gpu-culling-dispatch-v1",
        "Vulkan compute dispatch",
        "mavg-gpu-culling-dispatch-v1",
        "PR #556"
    )) {
    Assert-ContainsText $mavgGpuCullingDispatchPlanText $needle "docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md sibling transition"
}

foreach ($needle in @(
        "MAVG Vulkan GPU Culling Dispatch v1",
        "dispatch_mavg_gpu_culling_indirect",
        "MK_mavg_vulkan_gpu_culling_dispatch_tests",
        "VK_KHR_synchronization2",
        "mavg-vulkan-gpu-culling-dispatch-v1"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_rhi Vulkan GPU culling dispatch implementation evidence"
}
