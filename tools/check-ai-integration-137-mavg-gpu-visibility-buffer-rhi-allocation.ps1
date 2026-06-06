#requires -Version 7.0
#requires -PSEdition Core
# Chapter 137 for check-ai-integration.ps1 static contracts.

$runtimeRhiHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_visibility_buffer_resource.hpp"
$runtimeRhiSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_gpu_visibility_buffer_resource.cpp"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$testsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_gpu_visibility_buffer_resource_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-rhi-allocation-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnostic",
        "RuntimeMavgGpuVisibilityBufferResourceDesc",
        "RuntimeMavgGpuVisibilityBufferResourceRow",
        "RuntimeMavgGpuVisibilityBufferResourceResult",
        "create_runtime_mavg_gpu_visibility_buffer_resource",
        "has_runtime_mavg_gpu_visibility_buffer_resource_diagnostic",
        "missing_rhi_device",
        "missing_layout_plan",
        "invalid_layout_plan",
        "zero_slot_buffer_disallowed",
        "invalid_slot_buffer_size",
        "max_buffer_size_exceeded",
        "missing_storage_usage",
        "missing_copy_source_usage_for_readback",
        "rhi_allocation_failed",
        "invalid_allocated_buffer_handle",
        "visibility_buffer_usage",
        "allow_zero_slot_buffer",
        "require_copy_source_for_readback",
        "allocated_rhi_resources{false}",
        "ready_for_readback_proof{false}",
        "wrote_gpu_visibility_buffer{false}",
        "created_descriptor_bindings{false}",
        "recorded_resource_barriers{false}",
        "submitted_gpu_work{false}",
        "executed_gpu_traversal{false}",
        "executed_mesh_shader{false}",
        "executed_indirect_draw{false}",
        "used_gpu_decompression{false}",
        "enforced_allocator_budget{false}",
        "exposed_native_handles{false}",
        "claimed_vulkan_parity{false}",
        "claimed_metal_parity{false}",
        "claimed_nanite_compatibility{false}",
        "claimed_benchmark_superiority{false}"
    )) {
    Assert-ContainsText $runtimeRhiHeaderText $needle "mavg_gpu_visibility_buffer_resource.hpp allocation-only public API"
}

foreach ($needle in @(
        "ID3D12",
        "IDStorage",
        "DSTORAGE_",
        "#include <windows.h>",
        "#include <dstorage.h>",
        "ComPtr",
        "vkCmd",
        "MTL"
    )) {
    Assert-DoesNotContainText $runtimeRhiHeaderText $needle "mavg_gpu_visibility_buffer_resource.hpp must not expose native graphics or DirectStorage symbols"
    Assert-DoesNotContainText $runtimeRhiSourceText $needle "mavg_gpu_visibility_buffer_resource.cpp must stay backend-neutral"
}

foreach ($needle in @(
        "layout_shape_valid",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_rhi_device",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_layout_plan",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::invalid_layout_plan",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::zero_slot_buffer_disallowed",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::invalid_slot_buffer_size",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::max_buffer_size_exceeded",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_storage_usage",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_copy_source_usage_for_readback",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::rhi_allocation_failed",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::invalid_allocated_buffer_handle",
        "rhi::has_flag(desc.visibility_buffer_usage, rhi::BufferUsage::storage)",
        "rhi::has_flag(desc.visibility_buffer_usage, rhi::BufferUsage::copy_source)",
        "desc.device->create_buffer(buffer_desc)",
        ".usage = desc.visibility_buffer_usage",
        ".storage_usage_ready = rhi::has_flag(buffer_desc.usage, rhi::BufferUsage::storage)",
        ".copy_source_usage_ready = rhi::has_flag(buffer_desc.usage, rhi::BufferUsage::copy_source)",
        "result.allocated_rhi_resources = true",
        "result.ready_for_readback_proof"
    )) {
    Assert-ContainsText $runtimeRhiSourceText $needle "mavg_gpu_visibility_buffer_resource.cpp deterministic allocation implementation"
}

foreach ($needle in @(
        "write_buffer(",
        "begin_command_list(",
        "submit(",
        "create_descriptor_set_layout",
        "allocate_descriptor_set",
        "update_descriptor_set",
        "create_compute_pipeline",
        "draw_indexed_indirect",
        "execute_residency_action",
        "record_frame_graph_texture_aliasing_barriers",
        "barrier",
        "dispatch"
    )) {
    Assert-DoesNotContainText $runtimeRhiSourceText $needle "mavg_gpu_visibility_buffer_resource.cpp must not write, bind, barrier, submit, dispatch, or execute residency"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_gpu_visibility_buffer_resource.cpp" "engine/runtime_rhi/CMakeLists.txt MAVG visibility buffer resource source"

foreach ($needle in @(
        "MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests",
        "tests/unit/runtime_rhi_mavg_gpu_visibility_buffer_resource_tests.cpp",
        "MK_runtime_rhi"
    )) {
    Assert-ContainsText $rootCMakeText $needle "CMakeLists.txt MAVG visibility buffer resource test target"
}

foreach ($needle in @(
        "runtime rhi mavg visibility buffer resource allocates null rhi storage readback buffer",
        "runtime rhi mavg visibility buffer resource handles missing invalid and zero slot layouts",
        "runtime rhi mavg visibility buffer resource rejects invalid size and usage before allocation",
        "runtime rhi mavg visibility buffer resource rejects missing device and zero allocated handle",
        "RuntimeMavgGpuVisibilityBufferResourceDesc",
        "create_runtime_mavg_gpu_visibility_buffer_resource",
        "missing_rhi_device",
        "missing_layout_plan",
        "invalid_layout_plan",
        "zero_slot_buffer_disallowed",
        "invalid_slot_buffer_size",
        "max_buffer_size_exceeded",
        "missing_storage_usage",
        "missing_copy_source_usage_for_readback",
        "invalid_allocated_buffer_handle",
        "wrote_gpu_visibility_buffer",
        "created_descriptor_bindings",
        "recorded_resource_barriers",
        "submitted_gpu_work",
        "executed_gpu_traversal",
        "executed_mesh_shader",
        "executed_indirect_draw",
        "used_gpu_decompression",
        "enforced_allocator_budget",
        "exposed_native_handles",
        "claimed_vulkan_parity",
        "claimed_metal_parity",
        "claimed_nanite_compatibility",
        "claimed_benchmark_superiority"
    )) {
    Assert-ContainsText $testsText $needle "MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests coverage"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-rhi-allocation-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $mavgSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" }
    )) {
    foreach ($needle in @(
            "MAVG GPU Visibility Buffer RHI Allocation v1",
            "mavg_gpu_visibility_buffer_resource.hpp",
            "RuntimeMavgGpuVisibilityBufferResourceDesc",
            "RuntimeMavgGpuVisibilityBufferResourceRow",
            "RuntimeMavgGpuVisibilityBufferResourceResult",
            "create_runtime_mavg_gpu_visibility_buffer_resource"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG visibility buffer RHI allocation evidence"
    }
    foreach ($needle in @(
            "GPU visibility-buffer",
            "descriptor",
            "barrier",
            "GPU traversal",
            "mesh shader",
            "GPU decompression",
            "Vulkan/Metal",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG visibility buffer RHI allocation non-claims"
    }
}

Assert-ContainsText $aiLoopFragmentText "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-rhi-allocation-v1.md" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG visibility buffer RHI allocation retained path"

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-rhi-allocation-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG GPU Visibility Buffer RHI Allocation v1"
}

$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module"
}
$runtimeRhiManifestText = (
    (@($runtimeRhiModule[0].dependencies) -join " "),
    (@($runtimeRhiModule[0].publicHeaders) -join " "),
    (@($runtimeRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeRhiModule[0].purpose
) -join " "
foreach ($needle in @(
        "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_visibility_buffer_resource.hpp",
        "MAVG GPU Visibility Buffer RHI Allocation v1",
        "RuntimeMavgGpuVisibilityBufferResourceDesc",
        "RuntimeMavgGpuVisibilityBufferResourceRow",
        "RuntimeMavgGpuVisibilityBufferResourceResult",
        "RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode",
        "create_runtime_mavg_gpu_visibility_buffer_resource",
        "MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests",
        "IRhiDevice::create_buffer",
        "BufferUsage::storage",
        "BufferUsage::copy_source",
        "wrote_gpu_visibility_buffer=false",
        "created_descriptor_bindings=false",
        "recorded_resource_barriers=false",
        "submitted_gpu_work=false",
        "executed_gpu_traversal=false",
        "executed_mesh_shader=false",
        "executed_indirect_draw=false",
        "used_gpu_decompression=false",
        "enforced_allocator_budget=false",
        "exposed_native_handles=false",
        "claimed_vulkan_parity=false",
        "claimed_metal_parity=false",
        "claimed_nanite_compatibility=false",
        "claimed_benchmark_superiority=false",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi MAVG visibility buffer RHI allocation evidence"
}
