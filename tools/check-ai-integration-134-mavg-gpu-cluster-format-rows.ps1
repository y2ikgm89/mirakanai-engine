#requires -Version 7.0
#requires -PSEdition Core
# Chapter 134 for check-ai-integration.ps1 static contracts.

$rendererHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/mavg_gpu_cluster_format.hpp"
$rendererSourceText = Get-AgentSurfaceText "engine/renderer/src/mavg_gpu_cluster_format.cpp"
$rendererCMakeText = Get-AgentSurfaceText "engine/renderer/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$testsText = Get-AgentSurfaceText "tests/unit/mavg_gpu_cluster_format_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-gpu-cluster-format-rows-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "MavgGpuClusterFormatDiagnosticCode",
        "MavgGpuClusterFormatDiagnostic",
        "MavgGpuClusterFormatDesc",
        "MavgGpuClusterFormatRow",
        "MavgGpuClusterFormatPlan",
        "plan_mavg_gpu_cluster_format_rows",
        "has_mavg_gpu_cluster_format_diagnostic",
        "invalid_graph",
        "invalid_payload",
        "unsupported_vertex_stride",
        "unsupported_index_format",
        "cluster_draw_range_out_of_payload",
        "allocated_rhi_resources{false}",
        "submitted_gpu_work{false}",
        "executed_mesh_shader{false}",
        "executed_gpu_traversal{false}",
        "used_gpu_decompression{false}",
        "touched_native_handles{false}",
        "claimed_nanite_compatibility{false}"
    )) {
    Assert-ContainsText $rendererHeaderText $needle "mavg_gpu_cluster_format.hpp backend-neutral GPU cluster format contract"
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
    Assert-DoesNotContainText $rendererHeaderText $needle "mavg_gpu_cluster_format.hpp must not expose native graphics or DirectStorage symbols"
    Assert-DoesNotContainText $rendererSourceText $needle "mavg_gpu_cluster_format.cpp must stay backend-neutral"
}

foreach ($needle in @(
        "supported_position_normal_uv_stride_bytes = 32",
        "supported_uint32_index_stride_bytes = 4",
        "validate_mavg_cluster_graph",
        "validate_mavg_cluster_payload",
        "canonicalize_mavg_cluster_graph",
        "cluster_draw_range_fits_payload",
        "center_of",
        "radius_of",
        "MavgGpuClusterFormatDiagnosticCode::invalid_graph",
        "MavgGpuClusterFormatDiagnosticCode::invalid_payload",
        "MavgGpuClusterFormatDiagnosticCode::unsupported_vertex_stride",
        "MavgGpuClusterFormatDiagnosticCode::unsupported_index_format",
        "MavgGpuClusterFormatDiagnosticCode::cluster_draw_range_out_of_payload",
        "plan.d3d12_mesh_shader_limit_triangles",
        "plan.vulkan_mesh_shader_limit_triangles",
        "plan.rows.push_back",
        ".payload_page_byte_offset",
        ".payload_page_byte_size",
        ".meshlet_ready = meshlet_ready"
    )) {
    Assert-ContainsText $rendererSourceText $needle "mavg_gpu_cluster_format.cpp deterministic row planning implementation"
}

Assert-ContainsText $rendererCMakeText "src/mavg_gpu_cluster_format.cpp" "engine/renderer/CMakeLists.txt MAVG GPU cluster format source"

foreach ($needle in @(
        "MK_mavg_gpu_cluster_format_tests",
        "tests/unit/mavg_gpu_cluster_format_tests.cpp",
        "MK_renderer",
        "MK_assets",
        "MK_core"
    )) {
    Assert-ContainsText $rootCMakeText $needle "CMakeLists.txt MAVG GPU cluster format test target"
}

foreach ($needle in @(
        "mavg gpu cluster format rows derive deterministic GPU layout from graph payload",
        "mavg gpu cluster format rows fail closed on invalid graph and payload",
        "mavg gpu cluster format rows reject unsupported payload layouts and draw overflow",
        "mavg gpu cluster format rows report clusters outside meshlet limits without executing GPU work",
        "MavgGpuClusterFormatDesc",
        "plan_mavg_gpu_cluster_format_rows",
        "MavgGpuClusterFormatDiagnosticCode::invalid_graph",
        "MavgGpuClusterFormatDiagnosticCode::invalid_payload",
        "MavgGpuClusterFormatDiagnosticCode::unsupported_vertex_stride",
        "MavgGpuClusterFormatDiagnosticCode::unsupported_index_format",
        "MavgGpuClusterFormatDiagnosticCode::cluster_draw_range_out_of_payload",
        "payload_page_byte_offset",
        "bounds_center",
        "meshlet_ready_cluster_count",
        "executed_mesh_shader",
        "executed_gpu_traversal",
        "used_gpu_decompression",
        "touched_native_handles",
        "claimed_nanite_compatibility"
    )) {
    Assert-ContainsText $testsText $needle "MK_mavg_gpu_cluster_format_tests coverage"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-06-mavg-gpu-cluster-format-rows-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" }
    )) {
    foreach ($needle in @(
            "MAVG GPU Cluster Format Rows v1",
            "mavg_gpu_cluster_format.hpp",
            "MavgGpuClusterFormatDesc",
            "MavgGpuClusterFormatRow",
            "MavgGpuClusterFormatPlan",
            "plan_mavg_gpu_cluster_format_rows"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG GPU cluster format evidence"
    }
    foreach ($needle in @(
            "GPU traversal",
            "mesh shader",
            "GPU decompression",
            "Vulkan/Metal",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG GPU cluster format non-claims"
    }
}

Assert-ContainsText $aiLoopFragmentText "docs/superpowers/plans/2026-06-06-mavg-gpu-cluster-format-rows-v1.md" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG GPU cluster format retained path"
Assert-ContainsText $aiLoopFragmentText "MAVG GPU Cluster Format Rows v1" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG GPU cluster format retained evidence"
Assert-ContainsText $aiLoopFragmentText "executed_gpu_traversal=false" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG GPU cluster format traversal non-claim"

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-gpu-cluster-format-rows-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG GPU Cluster Format Rows v1"
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
        "engine/renderer/include/mirakana/renderer/mavg_gpu_cluster_format.hpp",
        "MAVG GPU Cluster Format Rows v1",
        "MavgGpuClusterFormatDesc",
        "MavgGpuClusterFormatRow",
        "MavgGpuClusterFormatPlan",
        "MavgGpuClusterFormatDiagnosticCode",
        "plan_mavg_gpu_cluster_format_rows",
        "MK_mavg_gpu_cluster_format_tests",
        "allocated_rhi_resources=false",
        "submitted_gpu_work=false",
        "executed_mesh_shader=false",
        "executed_gpu_traversal=false",
        "used_gpu_decompression=false",
        "touched_native_handles=false",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer MAVG GPU cluster format evidence"
}
