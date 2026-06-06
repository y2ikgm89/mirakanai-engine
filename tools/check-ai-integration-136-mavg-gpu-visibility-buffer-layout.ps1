#requires -Version 7.0
#requires -PSEdition Core
# Chapter 136 for check-ai-integration.ps1 static contracts.

$rendererHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/mavg_gpu_visibility_buffer_layout.hpp"
$rendererSourceText = Get-AgentSurfaceText "engine/renderer/src/mavg_gpu_visibility_buffer_layout.cpp"
$rendererCMakeText = Get-AgentSurfaceText "engine/renderer/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$testsText = Get-AgentSurfaceText "tests/unit/mavg_gpu_visibility_buffer_layout_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-layout-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "MavgGpuVisibilityBufferLayoutDiagnosticCode",
        "MavgGpuVisibilityBufferLayoutDiagnostic",
        "MavgGpuVisibilityBufferLayoutDesc",
        "MavgGpuVisibilityBufferSlotRow",
        "MavgGpuVisibilityBufferByteRangeRow",
        "MavgGpuVisibilityBufferLayoutRow",
        "MavgGpuVisibilityBufferSyncIntentRow",
        "MavgGpuVisibilityBufferLayoutPlan",
        "MavgGpuVisibilityBufferSyncProducer",
        "MavgGpuVisibilityBufferSyncConsumer",
        "plan_mavg_gpu_visibility_buffer_layout",
        "has_mavg_gpu_visibility_buffer_layout_diagnostic",
        "missing_visible_cluster_packet_plan",
        "invalid_visible_cluster_packet_plan",
        "source_packet_count_mismatch",
        "duplicate_packet_index",
        "non_dense_packet_index",
        "meshlet_not_ready",
        "invalid_slot_stride",
        "invalid_slot_alignment",
        "max_slot_count_exceeded",
        "slot_byte_range_overflow",
        "layout_byte_size_overflow",
        "visibility_buffer_write",
        "visibility_resolve_read",
        "minimum_slot_record_stride_bytes",
        "allocated_rhi_resources{false}",
        "wrote_gpu_visibility_buffer{false}",
        "submitted_gpu_work{false}",
        "executed_gpu_traversal{false}",
        "executed_mesh_shader{false}",
        "executed_indirect_draw{false}",
        "used_gpu_decompression{false}",
        "touched_native_handles{false}",
        "claimed_vulkan_parity{false}",
        "claimed_metal_parity{false}",
        "claimed_nanite_compatibility{false}"
    )) {
    Assert-ContainsText $rendererHeaderText $needle "mavg_gpu_visibility_buffer_layout.hpp backend-neutral layout contract"
}

foreach ($needle in @(
        "ID3D12",
        "IDStorage",
        "DSTORAGE_",
        "#include <windows.h>",
        "#include <dstorage.h>",
        "ComPtr",
        "vkCmd",
        "MTL",
        "BufferHandle",
        "IRhiDevice"
    )) {
    Assert-DoesNotContainText $rendererHeaderText $needle "mavg_gpu_visibility_buffer_layout.hpp must not expose native graphics or DirectStorage symbols"
    Assert-DoesNotContainText $rendererSourceText $needle "mavg_gpu_visibility_buffer_layout.cpp must stay backend-neutral"
}

foreach ($needle in @(
        "is_power_of_two",
        "align_up",
        "fail_closed",
        "packet_index_is_duplicate",
        "cluster_key_of",
        "next_slot_range",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::missing_visible_cluster_packet_plan",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::invalid_visible_cluster_packet_plan",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::source_packet_count_mismatch",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::duplicate_packet_index",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::non_dense_packet_index",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::meshlet_not_ready",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::invalid_slot_stride",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::invalid_slot_alignment",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::max_slot_count_exceeded",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::slot_byte_range_overflow",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode::layout_byte_size_overflow",
        "plan.layout_rows.push_back",
        "plan.sync_intents.push_back",
        "MavgGpuVisibilityBufferSyncIntentRow{}",
        ".slot_byte_offset = byte_offset",
        ".cluster_key = cluster_key_of(packet)"
    )) {
    Assert-ContainsText $rendererSourceText $needle "mavg_gpu_visibility_buffer_layout.cpp deterministic layout implementation"
}

Assert-ContainsText $rendererCMakeText "src/mavg_gpu_visibility_buffer_layout.cpp" "engine/renderer/CMakeLists.txt MAVG visibility buffer layout source"

foreach ($needle in @(
        "MK_mavg_gpu_visibility_buffer_layout_tests",
        "tests/unit/mavg_gpu_visibility_buffer_layout_tests.cpp",
        "MK_renderer"
    )) {
    Assert-ContainsText $rootCMakeText $needle "CMakeLists.txt MAVG visibility buffer layout test target"
}

foreach ($needle in @(
        "mavg gpu visibility buffer layout derives deterministic slot rows from packet rows",
        "mavg gpu visibility buffer layout fails closed on missing invalid or inconsistent packet plans",
        "mavg gpu visibility buffer layout rejects duplicate or stale packet indexes",
        "mavg gpu visibility buffer layout validates stride alignment meshlet and budget gates",
        "mavg gpu visibility buffer layout allows zero packets",
        "MavgGpuVisibilityBufferLayoutDesc",
        "plan_mavg_gpu_visibility_buffer_layout",
        "missing_visible_cluster_packet_plan",
        "invalid_visible_cluster_packet_plan",
        "source_packet_count_mismatch",
        "duplicate_packet_index",
        "non_dense_packet_index",
        "meshlet_not_ready",
        "invalid_slot_stride",
        "invalid_slot_alignment",
        "max_slot_count_exceeded",
        "wrote_gpu_visibility_buffer",
        "executed_gpu_traversal",
        "executed_mesh_shader",
        "executed_indirect_draw",
        "used_gpu_decompression",
        "touched_native_handles",
        "claimed_vulkan_parity",
        "claimed_metal_parity",
        "claimed_nanite_compatibility"
    )) {
    Assert-ContainsText $testsText $needle "MK_mavg_gpu_visibility_buffer_layout_tests coverage"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-layout-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $mavgSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" }
    )) {
    foreach ($needle in @(
            "MAVG GPU Visibility Buffer Layout v1",
            "mavg_gpu_visibility_buffer_layout.hpp",
            "MavgGpuVisibilityBufferLayoutDesc",
            "MavgGpuVisibilityBufferSlotRow",
            "MavgGpuVisibilityBufferByteRangeRow",
            "MavgGpuVisibilityBufferLayoutRow",
            "MavgGpuVisibilityBufferSyncIntentRow",
            "MavgGpuVisibilityBufferLayoutPlan",
            "plan_mavg_gpu_visibility_buffer_layout"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG visibility buffer layout evidence"
    }
    foreach ($needle in @(
            "GPU visibility-buffer",
            "GPU traversal",
            "mesh shader",
            "GPU decompression",
            "Vulkan/Metal",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG visibility buffer layout non-claims"
    }
}

Assert-ContainsText $aiLoopFragmentText "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-layout-v1.md" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG visibility buffer layout retained path"
Assert-ContainsText $aiLoopFragmentText "MAVG GPU Visibility Buffer Layout v1" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG visibility buffer layout retained evidence"
Assert-ContainsText $aiLoopFragmentText "wrote_gpu_visibility_buffer=false" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG visibility-buffer write non-claim"
Assert-ContainsText $aiLoopFragmentText "executed_gpu_traversal=false" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG traversal non-claim"

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-gpu-visibility-buffer-layout-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG GPU Visibility Buffer Layout v1"
}

$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module"
}
$rendererManifestText = (
    (@($rendererModule[0].dependencies) -join " "),
    (@($rendererModule[0].publicHeaders) -join " "),
    (@($rendererModule[0].recentEvidence) -join " "),
    [string]$rendererModule[0].purpose
) -join " "
foreach ($needle in @(
        "engine/renderer/include/mirakana/renderer/mavg_gpu_visibility_buffer_layout.hpp",
        "MAVG GPU Visibility Buffer Layout v1",
        "MavgGpuVisibilityBufferLayoutDesc",
        "MavgGpuVisibilityBufferSlotRow",
        "MavgGpuVisibilityBufferByteRangeRow",
        "MavgGpuVisibilityBufferLayoutRow",
        "MavgGpuVisibilityBufferSyncIntentRow",
        "MavgGpuVisibilityBufferLayoutPlan",
        "MavgGpuVisibilityBufferLayoutDiagnosticCode",
        "plan_mavg_gpu_visibility_buffer_layout",
        "MK_mavg_gpu_visibility_buffer_layout_tests",
        "allocated_rhi_resources=false",
        "wrote_gpu_visibility_buffer=false",
        "submitted_gpu_work=false",
        "executed_gpu_traversal=false",
        "executed_mesh_shader=false",
        "executed_indirect_draw=false",
        "used_gpu_decompression=false",
        "touched_native_handles=false",
        "claimed_vulkan_parity=false",
        "claimed_metal_parity=false",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer MAVG visibility buffer layout evidence"
}
