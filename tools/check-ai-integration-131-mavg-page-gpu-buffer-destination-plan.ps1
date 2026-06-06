#requires -Version 7.0
#requires -PSEdition Core
# Chapter 131 for check-ai-integration.ps1 static contracts.

$runtimeRhiHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_page_gpu_buffer_destination.hpp"
$runtimeRhiSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_page_gpu_buffer_destination.cpp"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeRhiTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_page_gpu_buffer_destination_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-page-gpu-buffer-destination-plan-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgPageBufferDestinationDiagnosticCode",
        "RuntimeMavgPageBufferDestinationDiagnostic",
        "RuntimeMavgPageBufferDestinationRow",
        "RuntimeMavgPageBufferDestinationDesc",
        "RuntimeMavgPageBufferDestinationPlanRow",
        "RuntimeMavgPageBufferDestinationPlanResult",
        "plan_runtime_mavg_page_buffer_destinations",
        "duplicate_destination_row",
        "missing_destination_row",
        "destination_range_mismatch",
        "invoked_file_io",
        "used_native_directstorage",
        "submitted_native_queue",
        "used_directstorage_resource_destination",
        "used_gpu_decompression",
        "allocated_rhi_resources",
        "enforced_allocator_budget",
        "mutated_mount_set",
        "exposed_native_handles"
    )) {
    Assert-ContainsText $runtimeRhiHeaderText $needle "mavg_page_gpu_buffer_destination.hpp value-only planner contract"
}

foreach ($needle in @(
        "ID3D12",
        "IDStorage",
        "DSTORAGE_",
        "#include <windows.h>",
        "#include <dstorage.h>",
        "ComPtr"
    )) {
    Assert-DoesNotContainText $runtimeRhiHeaderText $needle "mavg_page_gpu_buffer_destination.hpp must not expose native DirectStorage/D3D12 symbols"
    Assert-DoesNotContainText $runtimeRhiSourceText $needle "mavg_page_gpu_buffer_destination.cpp must stay value-only and native-handle-free"
}

foreach ($needle in @(
        "validate_mavg_cluster_graph",
        "RuntimeMavgPageBufferDestinationDiagnosticCode::invalid_graph_asset",
        "RuntimeMavgPageBufferDestinationDiagnosticCode::missing_graph",
        "RuntimeMavgPageBufferDestinationDiagnosticCode::invalid_graph",
        "RuntimeMavgPageBufferDestinationDiagnosticCode::missing_request_plan",
        "RuntimeMavgPageBufferDestinationDiagnosticCode::invalid_request_plan",
        "RuntimeMavgPageBufferDestinationDiagnosticCode::invalid_destination_row",
        "RuntimeMavgPageBufferDestinationDiagnosticCode::duplicate_destination_row",
        "RuntimeMavgPageBufferDestinationDiagnosticCode::missing_destination_row",
        "RuntimeMavgPageBufferDestinationDiagnosticCode::destination_range_mismatch",
        "result.invoked_file_io = false",
        "result.used_native_directstorage = false",
        "result.submitted_native_queue = false",
        "result.used_directstorage_resource_destination = false",
        "result.used_gpu_decompression = false",
        "result.allocated_rhi_resources = false",
        "result.enforced_allocator_budget = false",
        "result.mutated_mount_set = false",
        "result.exposed_native_handles = false",
        "request_fits_destination",
        "find_destination_by_page"
    )) {
    Assert-ContainsText $runtimeRhiSourceText $needle "mavg_page_gpu_buffer_destination.cpp fail-closed planner implementation"
}

foreach ($needle in @(
        "src/mavg_page_gpu_buffer_destination.cpp"
    )) {
    Assert-ContainsText $runtimeRhiCMakeText $needle "engine/runtime_rhi/CMakeLists.txt MAVG page GPU buffer destination source"
}

foreach ($needle in @(
        "MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests",
        "tests/unit/runtime_rhi_mavg_page_gpu_buffer_destination_tests.cpp",
        "MK_runtime_rhi"
    )) {
    Assert-ContainsText $rootCMakeText $needle "CMakeLists.txt MAVG page GPU buffer destination test target"
}

foreach ($needle in @(
        "runtime rhi mavg page buffer destination planner maps directstorage requests to caller owned rhi buffer",
        "runtime rhi mavg page buffer destination planner rejects duplicate destination pages before io",
        "runtime rhi mavg page buffer destination planner rejects missing and too small destination rows",
        "RuntimeMavgPageBufferDestinationRow",
        "RuntimeMavgPageBufferDestinationDesc",
        "plan_runtime_mavg_page_buffer_destinations",
        "duplicate_destination_row",
        "missing_destination_row",
        "destination_range_mismatch",
        "used_native_directstorage",
        "submitted_native_queue",
        "used_directstorage_resource_destination",
        "used_gpu_decompression",
        "allocated_rhi_resources",
        "enforced_allocator_budget",
        "mutated_mount_set",
        "exposed_native_handles"
    )) {
    Assert-ContainsText $runtimeRhiTestsText $needle "MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests coverage"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-06-mavg-page-gpu-buffer-destination-plan-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" }
    )) {
    foreach ($needle in @(
            "MAVG Page GPU Buffer Destination Plan v1",
            "mavg_page_gpu_buffer_destination.hpp",
            "RuntimeMavgPageBufferDestinationRow",
            "RuntimeMavgPageBufferDestinationDesc",
            "RuntimeMavgPageBufferDestinationPlanResult",
            "plan_runtime_mavg_page_buffer_destinations",
            "BufferHandle"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG page GPU buffer destination evidence"
    }
    foreach ($needle in @(
            "GPU decompression",
            "allocator/GPU budget",
            "Vulkan/Metal",
            "async-overlap/performance",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG page GPU buffer destination non-claims"
    }
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-page-gpu-buffer-destination-plan-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG Page GPU Buffer Destination Plan v1"
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
        "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_page_gpu_buffer_destination.hpp",
        "MAVG Page GPU Buffer Destination Plan v1",
        "RuntimeMavgPageBufferDestinationRow",
        "RuntimeMavgPageBufferDestinationDesc",
        "RuntimeMavgPageBufferDestinationPlanResult",
        "plan_runtime_mavg_page_buffer_destinations",
        "MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests",
        "no native DirectStorage submission",
        "no DirectStorage resource destination execution in MK_runtime_rhi",
        "no GPU decompression",
        "no RHI allocation",
        "no allocator/GPU budget enforcement",
        "no native handle exposure",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi page GPU buffer destination evidence"
}
