#requires -Version 7.0
#requires -PSEdition Core
# Chapter 132 for check-ai-integration.ps1 static contracts.

$runtimeRhiHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_page_gpu_resource_update.hpp"
$runtimeRhiSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_page_gpu_resource_update.cpp"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeRhiTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_page_gpu_resource_update_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-page-gpu-resource-update-readiness-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnostic",
        "RuntimeMavgPageGpuResourceUpdateRow",
        "RuntimeMavgPageGpuResourceUpdateReadinessDesc",
        "RuntimeMavgPageGpuResourceUpdateReadinessResult",
        "make_runtime_mavg_page_gpu_resource_update_readiness",
        "RuntimeMavgResidentPageResourceRow",
        "incomplete_status_result",
        "failed_status_result",
        "resource_destination_not_used",
        "submitted_destination_bytes_mismatch",
        "duplicate_destination_row",
        "observed_native_queue_submission",
        "submitted_native_queue{false}",
        "invoked_rhi_residency_action{false}",
        "invoked_native_make_resident{false}",
        "invoked_native_evict{false}",
        "used_gpu_decompression{false}",
        "exposed_native_handles{false}"
    )) {
    Assert-ContainsText $runtimeRhiHeaderText $needle "mavg_page_gpu_resource_update.hpp readiness contract"
}

foreach ($needle in @(
        "ID3D12",
        "IDStorage",
        "DSTORAGE_",
        "#include <windows.h>",
        "#include <dstorage.h>",
        "ComPtr"
    )) {
    Assert-DoesNotContainText $runtimeRhiHeaderText $needle "mavg_page_gpu_resource_update.hpp must not expose native DirectStorage/D3D12 symbols"
    Assert-DoesNotContainText $runtimeRhiSourceText $needle "mavg_page_gpu_resource_update.cpp must stay value-only and native-handle-free"
}

foreach ($needle in @(
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::missing_buffer_destination_plan",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::invalid_buffer_destination_plan",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::missing_dispatch_result",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::invalid_dispatch_result",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::missing_status_result",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::incomplete_status_result",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::failed_status_result",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::resource_destination_not_used",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::submitted_request_count_mismatch",
        "submitted_destination_bytes_mismatch",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::status_destination_bytes_mismatch",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::invalid_destination_row",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::duplicate_destination_row",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::ticket_mismatch",
        "result.observed_native_queue_submission = true",
        "result.ready_for_residency_actions = true",
        "RhiResidencyResourceKind::buffer",
        "append_ready_row"
    )) {
    Assert-ContainsText $runtimeRhiSourceText $needle "mavg_page_gpu_resource_update.cpp fail-closed readiness implementation"
}

foreach ($needle in @(
        "src/mavg_page_gpu_resource_update.cpp"
    )) {
    Assert-ContainsText $runtimeRhiCMakeText $needle "engine/runtime_rhi/CMakeLists.txt MAVG page GPU resource update source"
}

foreach ($needle in @(
        "MK_runtime_rhi_mavg_page_gpu_resource_update_tests",
        "tests/unit/runtime_rhi_mavg_page_gpu_resource_update_tests.cpp",
        "MK_runtime_rhi"
    )) {
    Assert-ContainsText $rootCMakeText $needle "CMakeLists.txt MAVG page GPU resource update test target"
}

foreach ($needle in @(
        "runtime rhi mavg page gpu resource update readiness emits resident resource rows after completed",
        "runtime rhi mavg page gpu resource update readiness rejects incomplete and failed status evidence",
        "runtime rhi mavg page gpu resource update readiness rejects missing rhi destination evidence",
        "runtime rhi mavg page gpu resource update readiness rejects byte mismatch and duplicate destinations",
        "make_runtime_mavg_page_gpu_resource_update_readiness",
        "RuntimeMavgPageGpuResourceUpdateReadinessDesc",
        "incomplete_status_result",
        "failed_status_result",
        "resource_destination_not_used",
        "submitted_destination_bytes_mismatch",
        "duplicate_destination_row",
        "observed_native_queue_submission",
        "submitted_native_queue",
        "invoked_rhi_residency_action",
        "invoked_native_make_resident",
        "invoked_native_evict",
        "used_gpu_decompression",
        "exposed_native_handles"
    )) {
    Assert-ContainsText $runtimeRhiTestsText $needle "MK_runtime_rhi_mavg_page_gpu_resource_update_tests coverage"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-06-mavg-page-gpu-resource-update-readiness-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" }
    )) {
    foreach ($needle in @(
            "MAVG Page GPU Resource Update Readiness v1",
            "mavg_page_gpu_resource_update.hpp",
            "RuntimeMavgPageGpuResourceUpdateReadinessDesc",
            "RuntimeMavgPageGpuResourceUpdateReadinessResult",
            "RuntimeMavgPageGpuResourceUpdateRow",
            "make_runtime_mavg_page_gpu_resource_update_readiness",
            "RuntimeMavgResidentPageResourceRow"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG page GPU resource update readiness evidence"
    }
    foreach ($needle in @(
            "GPU decompression",
            "allocator/GPU budget",
            "Vulkan/Metal",
            "async-overlap/performance",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG page GPU resource update readiness non-claims"
    }
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-page-gpu-resource-update-readiness-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG Page GPU Resource Update Readiness v1"
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
        "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_page_gpu_resource_update.hpp",
        "MAVG Page GPU Resource Update Readiness v1",
        "RuntimeMavgPageGpuResourceUpdateReadinessDesc",
        "RuntimeMavgPageGpuResourceUpdateReadinessResult",
        "RuntimeMavgPageGpuResourceUpdateRow",
        "RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode",
        "make_runtime_mavg_page_gpu_resource_update_readiness",
        "RuntimeMavgResidentPageResourceRow",
        "MK_runtime_rhi_mavg_page_gpu_resource_update_tests",
        "submitted_native_queue=false for this helper",
        "no file IO",
        "no RHI allocation",
        "no invoked RHI residency action",
        "no native MakeResident/Evict invocation",
        "no allocator/GPU budget enforcement",
        "no mount mutation",
        "no GPU decompression",
        "no native handle exposure",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi page GPU resource update readiness evidence"
}
