#requires -Version 7.0
#requires -PSEdition Core
# Chapter 116 for check-ai-integration.ps1 static contracts.

$vulkanMeshShaderLodHeaderText = Get-AgentSurfaceText "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_mesh_shader_lod.hpp"
$vulkanMeshShaderLodSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_mavg_mesh_shader_lod.cpp"
$vulkanBackendHeaderText = Get-AgentSurfaceText "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp"
$vulkanBackendSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
$backendScaffoldTestsText = Get-AgentSurfaceText "tests/unit/backend_scaffold_tests.cpp"
$vulkanMeshShaderLodTestsText = Get-AgentSurfaceText "tests/unit/vulkan_mavg_mesh_shader_lod_tests.cpp"
$vulkanMeshShaderLodTaskShaderText = Get-AgentSurfaceText "tests/shaders/vulkan_mavg_mesh_shader_lod.task.hlsl"
$vulkanMeshShaderLodMeshShaderText = Get-AgentSurfaceText "tests/shaders/vulkan_mavg_mesh_shader_lod.mesh.hlsl"
$vulkanMeshShaderLodFragmentShaderText = Get-AgentSurfaceText "tests/shaders/vulkan_mavg_mesh_shader_lod.frag.hlsl"
$cmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "VulkanMavgMeshShaderLodCapabilityResult",
        "probe_vulkan_mavg_mesh_shader_lod_capability",
        "mesh_shader_supported",
        "mesh_shader_enabled",
        "draw_indirect_count_enabled",
        "max_mesh_work_group_count_x",
        "max_mesh_work_group_count_y",
        "max_mesh_work_group_count_z",
        "task_shader_entry_point",
        "mesh_shader_entry_point",
        "fragment_shader_entry_point",
        "readback_hash",
        "mavg_mesh_shader_lod_vulkan_ready"
    )) {
    Assert-ContainsText $vulkanMeshShaderLodHeaderText $needle "Vulkan MAVG mesh shader LOD backend-private API"
}

foreach ($needle in @(
        "vulkan_structure_type_physical_device_mesh_shader_features_ext",
        "vulkan_structure_type_physical_device_mesh_shader_properties_ext",
        "query_mesh_shader_feature_support",
        "make_native_mesh_shader_features",
        "chain_native_device_features",
        "vkCmdDrawMeshTasksEXT",
        "vkCmdDrawMeshTasksIndirectEXT",
        "vulkan_shader_stage_task_bit_ext",
        "vulkan_shader_stage_mesh_bit_ext",
        "VulkanRuntimeMeshGraphicsPipeline",
        "VulkanRuntimeMeshGraphicsPipelineDesc",
        "create_runtime_mesh_graphics_pipeline",
        "record_runtime_texture_rendering_mesh_tasks_draw",
        "cmd_draw_mesh_tasks",
        "vertex_input_state = nullptr",
        "input_assembly_state = nullptr",
        "supports_mesh_shader_extension",
        "mesh_shader_feature_queried",
        "max_mesh_output_vertices",
        "max_mesh_output_primitives"
    )) {
    Assert-ContainsText $vulkanBackendSourceText $needle "Vulkan backend VK_EXT_mesh_shader feature/property probe"
}

foreach ($needle in @(
        "supports_mesh_shader_extension",
        "require_mesh_shader",
        "require_task_shader",
        "enable_mesh_shader_queries",
        "mesh_shader_enabled",
        "task_shader_enabled",
        "mesh_shader_feature_queried",
        "max_task_work_group_count_x",
        "max_task_work_group_count_y",
        "max_task_work_group_count_z",
        "max_mesh_work_group_count_x",
        "max_mesh_work_group_count_y",
        "max_mesh_work_group_count_z",
        "VulkanRuntimeMeshGraphicsPipeline",
        "VulkanRuntimeTextureRenderingMeshTasksDrawDesc"
    )) {
    Assert-ContainsText $vulkanBackendHeaderText $needle "Vulkan physical device snapshot mesh shader per-axis limits"
}

foreach ($needle in @(
        "vulkan logical device create plan enables mesh and task shader features only when requested",
        "require_mesh_shader",
        "require_task_shader",
        "vulkan runtime mesh graphics pipeline keeps mesh stages backend private",
        "Vulkan mesh shader SPIR-V bytecode is required",
        "vkCmdDrawMeshTasksEXT",
        "vkCmdDrawMeshTasksIndirectEXT"
    )) {
    Assert-ContainsText $backendScaffoldTestsText $needle "Vulkan logical-device mesh shader feature-enable scaffold tests"
}

foreach ($needle in @(
        "vulkan_mesh_shader_feature_enable_path_missing",
        "vulkan_mesh_shader_draw_command_unavailable",
        "create_runtime_device",
        "create_runtime_mesh_graphics_pipeline",
        "record_runtime_texture_rendering_mesh_tasks_draw",
        "read_runtime_buffer",
        "hash_readback_bytes",
        "workgroup_product_exceeds_limit",
        "vulkan_mesh_shader_lod_execution_ready",
        "indirect_range_valid",
        "fallback_indexed_draw_promoted_readiness",
        "mavg_mesh_shader_lod_vulkan_ready"
    )) {
    Assert-ContainsText $vulkanMeshShaderLodSourceText $needle "Vulkan MAVG mesh shader LOD fail-closed source"
}

foreach ($needle in @(
        "vulkan mavg mesh shader lod rejects empty task rows",
        "vulkan mavg mesh shader lod probe records mesh shader feature state",
        "vulkan mavg mesh shader lod host gates without mesh shader support",
        "vulkan mavg mesh shader lod rejects invalid workgroup counts",
        "vulkan mavg mesh shader lod rejects indirect range overflow",
        "vulkan mavg mesh shader lod does not promote fallback indexed draw",
        "vulkan mavg mesh shader lod executes mesh shader when host supports it",
        "task_shader_entry_point = ""task_main""",
        "mesh_shader_entry_point = ""mesh_main""",
        "fragment_shader_entry_point = ""fragment_main""",
        "vulkan_mavg_mesh_shader_lod diagnostic=",
        "MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_TASK_SPV",
        "MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_MESH_SPV",
        "MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_FRAGMENT_SPV"
    )) {
    Assert-ContainsText $vulkanMeshShaderLodTestsText $needle "Vulkan MAVG mesh shader LOD tests"
}

foreach ($needle in @(
        "groupshared Payload payload",
        "DispatchMesh(1, 1, 1, payload)"
    )) {
    Assert-ContainsText $vulkanMeshShaderLodTaskShaderText $needle "Vulkan MAVG mesh shader LOD task shader"
}

foreach ($needle in @(
        "SetMeshOutputCounts(3, 1)",
        "out vertices VertexOut vertices_out[3]",
        "out indices uint3 primitives_out[1]"
    )) {
    Assert-ContainsText $vulkanMeshShaderLodMeshShaderText $needle "Vulkan MAVG mesh shader LOD mesh shader"
}

foreach ($needle in @(
        "fragment_main",
        "SV_Target"
    )) {
    Assert-ContainsText $vulkanMeshShaderLodFragmentShaderText $needle "Vulkan MAVG mesh shader LOD fragment shader"
}

foreach ($needle in @(
        "MK_mavg_vulkan_mesh_shader_lod_tests",
        "tests/unit/vulkan_mavg_mesh_shader_lod_tests.cpp",
        "MK_rhi_vulkan"
    )) {
    Assert-ContainsText $cmakeText $needle "Vulkan MAVG mesh shader LOD CMake target"
}

foreach ($needle in @(
        "Vulkan Mesh Shader LOD Execution",
        "feature-enable slice evidence",
        "readback execution slice evidence",
        "VulkanRuntimeMeshGraphicsPipeline",
        "make_native_mesh_shader_features",
        "mesh_shader_enabled",
        "task_shader_enabled",
        "drawIndirectCount",
        "offset + stride * (drawCount - 1) + sizeof(VkDrawMeshTasksIndirectCommandEXT) <= buffer_size_bytes",
        "mavg_mesh_shader_lod_vulkan_ready=true",
        "host_gated"
    )) {
    Assert-ContainsText $planText $needle "Vulkan MAVG mesh shader LOD official gate plan"
}

foreach ($needle in @(
        "Vulkan MAVG Mesh Shader LOD",
        "VulkanRuntimeMeshGraphicsPipeline",
        "vkCmdDrawMeshTasksEXT deterministic readback proof",
        "mavg_mesh_shader_lod_vulkan_ready",
        "probe_vulkan_mavg_mesh_shader_lod_capability",
        "MK_mavg_vulkan_mesh_shader_lod_tests",
        "host-gated"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json Vulkan MAVG mesh shader LOD evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json Vulkan MAVG mesh shader LOD evidence"
}
