#requires -Version 7.0
#requires -PSEdition Core
# Chapter 135 for check-ai-integration.ps1 static contracts.

$rendererHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/mavg_gpu_visible_cluster_packets.hpp"
$rendererSourceText = Get-AgentSurfaceText "engine/renderer/src/mavg_gpu_visible_cluster_packets.cpp"
$rendererCMakeText = Get-AgentSurfaceText "engine/renderer/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$testsText = Get-AgentSurfaceText "tests/unit/mavg_gpu_visible_cluster_packets_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-gpu-visible-cluster-packets-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "MavgGpuVisibleClusterPacketDiagnosticCode",
        "MavgGpuVisibleClusterPacketDiagnostic",
        "MavgGpuVisibleClusterPacketDesc",
        "MavgGpuVisibleClusterPacketRow",
        "MavgGpuVisibleClusterPacketPlan",
        "plan_mavg_gpu_visible_cluster_packets",
        "has_mavg_gpu_visible_cluster_packet_diagnostic",
        "missing_cluster_format_plan",
        "invalid_cluster_format_plan",
        "missing_culling_plan",
        "invalid_culling_plan",
        "duplicate_cluster_format_row",
        "missing_cluster_format_row",
        "stale_visible_command",
        "visible_command_count_mismatch",
        "meshlet_not_ready",
        "max_packet_count_exceeded",
        "payload_byte_span_overflow",
        "allocated_rhi_resources{false}",
        "submitted_gpu_work{false}",
        "executed_gpu_traversal{false}",
        "wrote_gpu_visibility_buffer{false}",
        "executed_mesh_shader{false}",
        "executed_indirect_draw{false}",
        "used_gpu_decompression{false}",
        "touched_native_handles{false}",
        "claimed_nanite_compatibility{false}"
    )) {
    Assert-ContainsText $rendererHeaderText $needle "mavg_gpu_visible_cluster_packets.hpp backend-neutral packet contract"
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
    Assert-DoesNotContainText $rendererHeaderText $needle "mavg_gpu_visible_cluster_packets.hpp must not expose native graphics or DirectStorage symbols"
    Assert-DoesNotContainText $rendererSourceText $needle "mavg_gpu_visible_cluster_packets.cpp must stay backend-neutral"
}

foreach ($needle in @(
        "same_cluster",
        "matching_format_row_count",
        "find_format_row",
        "duplicate_format_row_exists",
        "stale_visible_command",
        "fail_closed",
        "visible_command_count",
        "MavgGpuVisibleClusterPacketDiagnosticCode::missing_cluster_format_plan",
        "MavgGpuVisibleClusterPacketDiagnosticCode::invalid_cluster_format_plan",
        "MavgGpuVisibleClusterPacketDiagnosticCode::missing_culling_plan",
        "MavgGpuVisibleClusterPacketDiagnosticCode::invalid_culling_plan",
        "MavgGpuVisibleClusterPacketDiagnosticCode::duplicate_cluster_format_row",
        "MavgGpuVisibleClusterPacketDiagnosticCode::missing_cluster_format_row",
        "MavgGpuVisibleClusterPacketDiagnosticCode::stale_visible_command",
        "MavgGpuVisibleClusterPacketDiagnosticCode::visible_command_count_mismatch",
        "MavgGpuVisibleClusterPacketDiagnosticCode::meshlet_not_ready",
        "MavgGpuVisibleClusterPacketDiagnosticCode::max_packet_count_exceeded",
        "MavgGpuVisibleClusterPacketDiagnosticCode::payload_byte_span_overflow",
        "plan.packets.push_back",
        ".fallback_substitution = command.fallback_substitution",
        ".payload_page_byte_offset = row->payload_page_byte_offset",
        ".bounds_center = row->bounds_center",
        "plan.payload_byte_span_offset",
        "plan.payload_byte_span_size"
    )) {
    Assert-ContainsText $rendererSourceText $needle "mavg_gpu_visible_cluster_packets.cpp deterministic packet planning implementation"
}

Assert-ContainsText $rendererCMakeText "src/mavg_gpu_visible_cluster_packets.cpp" "engine/renderer/CMakeLists.txt MAVG visible packet source"

foreach ($needle in @(
        "MK_mavg_gpu_visible_cluster_packets_tests",
        "tests/unit/mavg_gpu_visible_cluster_packets_tests.cpp",
        "MK_renderer"
    )) {
    Assert-ContainsText $rootCMakeText $needle "CMakeLists.txt MAVG visible packet test target"
}

foreach ($needle in @(
        "mavg gpu visible cluster packets preserve visible command order and join cluster format rows",
        "mavg gpu visible cluster packets fail closed on missing or invalid source plans",
        "mavg gpu visible cluster packets fail closed on duplicate missing or stale format rows",
        "mavg gpu visible cluster packets enforce meshlet readiness and packet budgets",
        "mavg gpu visible cluster packets allow zero visible commands and reject count mismatches",
        "MavgGpuVisibleClusterPacketDesc",
        "plan_mavg_gpu_visible_cluster_packets",
        "missing_cluster_format_plan",
        "invalid_cluster_format_plan",
        "missing_culling_plan",
        "invalid_culling_plan",
        "duplicate_cluster_format_row",
        "missing_cluster_format_row",
        "stale_visible_command",
        "meshlet_not_ready",
        "max_packet_count_exceeded",
        "visible_command_count_mismatch",
        "wrote_gpu_visibility_buffer",
        "executed_gpu_traversal",
        "executed_mesh_shader",
        "executed_indirect_draw",
        "used_gpu_decompression",
        "touched_native_handles",
        "claimed_nanite_compatibility"
    )) {
    Assert-ContainsText $testsText $needle "MK_mavg_gpu_visible_cluster_packets_tests coverage"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-06-mavg-gpu-visible-cluster-packets-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $mavgSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" }
    )) {
    foreach ($needle in @(
            "MAVG GPU Visible Cluster Packet Rows v1",
            "mavg_gpu_visible_cluster_packets.hpp",
            "MavgGpuVisibleClusterPacketDesc",
            "MavgGpuVisibleClusterPacketRow",
            "MavgGpuVisibleClusterPacketPlan",
            "plan_mavg_gpu_visible_cluster_packets"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG visible packet evidence"
    }
    foreach ($needle in @(
            "GPU traversal",
            "visibility",
            "mesh shader",
            "GPU decompression",
            "Vulkan/Metal",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG visible packet non-claims"
    }
}

Assert-ContainsText $aiLoopFragmentText "docs/superpowers/plans/2026-06-06-mavg-gpu-visible-cluster-packets-v1.md" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG visible packet retained path"
Assert-ContainsText $aiLoopFragmentText "MAVG GPU Visible Cluster Packet Rows v1" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG visible packet retained evidence"
Assert-ContainsText $aiLoopFragmentText "wrote_gpu_visibility_buffer=false" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG visible packet visibility-buffer non-claim"
Assert-ContainsText $aiLoopFragmentText "executed_gpu_traversal=false" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG visible packet traversal non-claim"

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-gpu-visible-cluster-packets-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG GPU Visible Cluster Packet Rows v1"
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
        "engine/renderer/include/mirakana/renderer/mavg_gpu_visible_cluster_packets.hpp",
        "MAVG GPU Visible Cluster Packet Rows v1",
        "MavgGpuVisibleClusterPacketDesc",
        "MavgGpuVisibleClusterPacketRow",
        "MavgGpuVisibleClusterPacketPlan",
        "MavgGpuVisibleClusterPacketDiagnosticCode",
        "plan_mavg_gpu_visible_cluster_packets",
        "MK_mavg_gpu_visible_cluster_packets_tests",
        "allocated_rhi_resources=false",
        "submitted_gpu_work=false",
        "executed_gpu_traversal=false",
        "wrote_gpu_visibility_buffer=false",
        "executed_mesh_shader=false",
        "executed_indirect_draw=false",
        "used_gpu_decompression=false",
        "touched_native_handles=false",
        "no Vulkan/Metal parity claim",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer MAVG visible packet evidence"
}
