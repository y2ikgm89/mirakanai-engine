#requires -Version 7.0
#requires -PSEdition Core
# Chapter 138 for check-ai-integration.ps1 static contracts.

$runtimeRhiWriteHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_visibility_buffer_write_readback.hpp"
$runtimeRhiWriteSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_gpu_visibility_buffer_write_readback.cpp"
$runtimeRhiResourceHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_visibility_buffer_resource.hpp"
$runtimeRhiResourceSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_gpu_visibility_buffer_resource.cpp"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$writeTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests.cpp"
$resourceTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_gpu_visibility_buffer_resource_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-rhi-write-readback-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnostic",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDesc",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackRow",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackResult",
        "execute_runtime_mavg_gpu_visibility_buffer_write_readback",
        "has_runtime_mavg_gpu_visibility_buffer_write_readback_diagnostic",
        "missing_rhi_device",
        "missing_layout_plan",
        "invalid_layout_plan",
        "missing_resource_result",
        "invalid_resource_result",
        "resource_device_mismatch",
        "missing_resource_row",
        "invalid_visibility_buffer_handle",
        "visibility_buffer_size_mismatch",
        "missing_storage_usage",
        "missing_copy_source_usage_for_readback_copy",
        "missing_copy_destination_usage_for_staged_write",
        "slot_record_stride_mismatch",
        "slot_byte_range_overflow",
        "field_offset_out_of_range",
        "max_write_size_exceeded",
        "rhi_upload_allocation_failed",
        "rhi_readback_allocation_failed",
        "rhi_upload_write_failed",
        "rhi_copy_recording_failed",
        "rhi_submit_failed",
        "rhi_readback_failed",
        "readback_mismatch",
        "encoded_bytes",
        "readback_bytes",
        "wrote_gpu_visibility_buffer{false}",
        "submitted_copy_work{false}",
        "readback_bytes_match_encoded_records{false}",
        "created_descriptor_bindings{false}",
        "created_compute_pipeline{false}",
        "dispatched_compute_shader{false}",
        "executed_gpu_traversal{false}",
        "executed_mesh_shader{false}",
        "executed_indirect_draw{false}",
        "used_gpu_decompression{false}",
        "enforced_allocator_budget{false}",
        "exposed_native_handles{false}",
        "claimed_vulkan_parity{false}",
        "claimed_metal_parity{false}",
        "claimed_nanite_compatibility{false}",
        "claimed_benchmark_superiority{false}",
        "claimed_async_overlap_or_performance{false}"
    )) {
    Assert-ContainsText $runtimeRhiWriteHeaderText $needle "mavg_gpu_visibility_buffer_write_readback.hpp staged write/readback public API"
}

foreach ($needle in @(
        "owner_device"
    )) {
    Assert-ContainsText $runtimeRhiResourceHeaderText $needle "mavg_gpu_visibility_buffer_resource.hpp owner-device provenance"
    Assert-ContainsText $runtimeRhiResourceSourceText $needle "mavg_gpu_visibility_buffer_resource.cpp owner-device provenance"
    Assert-ContainsText $resourceTestsText $needle "MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests owner-device provenance"
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
    Assert-DoesNotContainText $runtimeRhiWriteHeaderText $needle "mavg_gpu_visibility_buffer_write_readback.hpp must not expose native graphics or DirectStorage symbols"
    Assert-DoesNotContainText $runtimeRhiWriteSourceText $needle "mavg_gpu_visibility_buffer_write_readback.cpp must stay backend-neutral"
}

foreach ($needle in @(
        "layout_shape_valid",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_rhi_device",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_layout_plan",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_layout_plan",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_resource_result",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_resource_result",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::resource_device_mismatch",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_resource_row",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_visibility_buffer_handle",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::visibility_buffer_size_mismatch",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_storage_usage",
        "missing_copy_source_usage_for_readback_copy",
        "missing_copy_destination_usage_for_staged_write",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::slot_record_stride_mismatch",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::slot_byte_range_overflow",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::field_offset_out_of_range",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::max_write_size_exceeded",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_submit_failed",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_readback_failed",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::readback_mismatch",
        "resource->owner_device != desc.device",
        "resource_row.owner_device != desc.device",
        "layout->slot_buffer_size_bytes > desc.max_write_size_bytes",
        "rhi::has_flag(resource_row.buffer_desc.usage, rhi::BufferUsage::storage)",
        "rhi::has_flag(resource_row.buffer_desc.usage, rhi::BufferUsage::copy_source)",
        "rhi::has_flag(resource_row.buffer_desc.usage, rhi::BufferUsage::copy_destination)",
        "write_u32_le",
        "write_u64_le",
        "slot_flags",
        ".usage = rhi::BufferUsage::copy_source",
        ".usage = rhi::BufferUsage::copy_destination",
        "desc.device->write_buffer",
        "commands->copy_buffer",
        "commands->close()",
        "desc.device->submit",
        "desc.device->wait",
        "desc.device->read_buffer",
        "result.readback_bytes_match_encoded_records = result.encoded_bytes == result.readback_bytes",
        "result.wrote_gpu_visibility_buffer = true"
    )) {
    Assert-ContainsText $runtimeRhiWriteSourceText $needle "mavg_gpu_visibility_buffer_write_readback.cpp deterministic staged write/readback implementation"
}

foreach ($needle in @(
        "create_descriptor_set_layout",
        "allocate_descriptor_set",
        "update_descriptor_set",
        "create_compute_pipeline",
        "bind_compute_pipeline",
        "dispatch(",
        "draw_indexed_indirect",
        "execute_residency_action",
        "DStorage"
    )) {
    Assert-DoesNotContainText $runtimeRhiWriteSourceText $needle "mavg_gpu_visibility_buffer_write_readback.cpp must not bind descriptors, create compute pipelines, dispatch, draw indirect, execute residency, or use DirectStorage"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_gpu_visibility_buffer_write_readback.cpp" "engine/runtime_rhi/CMakeLists.txt MAVG visibility buffer write/readback source"

foreach ($needle in @(
        "MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests",
        "tests/unit/runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests.cpp",
        "MK_runtime_rhi"
    )) {
    Assert-ContainsText $rootCMakeText $needle "CMakeLists.txt MAVG visibility buffer write/readback test target"
}

foreach ($needle in @(
        "runtime rhi mavg visibility buffer write readback copies deterministic records through null rhi",
        "runtime rhi mavg visibility buffer write readback rejects missing invalid and mismatched inputs",
        "runtime rhi mavg visibility buffer write readback rejects usage size and byte range gaps",
        "runtime rhi mavg visibility buffer write readback reports rhi operation failures",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDesc",
        "execute_runtime_mavg_gpu_visibility_buffer_write_readback",
        "read_u32_le",
        "read_u64_le",
        "FaultingRhiDevice",
        "resource_device_mismatch",
        "max_write_size_exceeded",
        "rhi_submit_failed",
        "rhi_readback_failed",
        "readback_mismatch",
        "created_descriptor_bindings",
        "created_compute_pipeline",
        "dispatched_compute_shader",
        "executed_gpu_traversal",
        "executed_mesh_shader",
        "executed_indirect_draw",
        "used_gpu_decompression",
        "enforced_allocator_budget",
        "exposed_native_handles",
        "claimed_vulkan_parity",
        "claimed_metal_parity",
        "claimed_nanite_compatibility",
        "claimed_benchmark_superiority",
        "claimed_async_overlap_or_performance"
    )) {
    Assert-ContainsText $writeTestsText $needle "MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests coverage"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-rhi-write-readback-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $mavgSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $productionMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" }
    )) {
    foreach ($needle in @(
            "MAVG GPU Visibility Buffer RHI Write Readback v1",
            "mavg_gpu_visibility_buffer_write_readback.hpp",
            "RuntimeMavgGpuVisibilityBufferWriteReadbackDesc",
            "RuntimeMavgGpuVisibilityBufferWriteReadbackRow",
            "RuntimeMavgGpuVisibilityBufferWriteReadbackResult",
            "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode",
            "execute_runtime_mavg_gpu_visibility_buffer_write_readback"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG visibility buffer write/readback evidence"
    }
    foreach ($needle in @(
            "descriptor",
            "compute",
            "GPU traversal",
            "mesh shader",
            "indirect draw",
            "GPU decompression",
            "DirectStorage",
            "native handle",
            "Vulkan/Metal",
            "Nanite",
            "benchmark",
            "async-overlap"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG visibility buffer write/readback non-claims"
    }
}

Assert-ContainsText $aiLoopFragmentText "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-rhi-write-readback-v1.md" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG visibility buffer write/readback retained path"

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-rhi-write-readback-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG GPU Visibility Buffer RHI Write Readback v1"
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
        "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_visibility_buffer_write_readback.hpp",
        "MAVG GPU Visibility Buffer RHI Write Readback v1",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDesc",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackRow",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackResult",
        "RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode",
        "execute_runtime_mavg_gpu_visibility_buffer_write_readback",
        "MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests",
        "IRhiDevice::write_buffer",
        "IRhiCommandList::copy_buffer",
        "IRhiDevice::submit",
        "IRhiDevice::wait",
        "IRhiDevice::read_buffer",
        "BufferUsage::storage",
        "BufferUsage::copy_source",
        "BufferUsage::copy_destination",
        "resource_device_mismatch",
        "max_write_size_exceeded",
        "rhi_submit_failed",
        "rhi_readback_failed",
        "readback_mismatch",
        "created_descriptor_bindings=false",
        "created_compute_pipeline=false",
        "dispatched_compute_shader=false",
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
        "claimed_async_overlap_or_performance=false",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi MAVG visibility buffer write/readback evidence"
}
