#requires -Version 7.0
#requires -PSEdition Core
# Chapter 133 for check-ai-integration.ps1 static contracts.

$runtimeRhiHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_page_gpu_resource_residency_execution.hpp"
$runtimeRhiSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_page_gpu_resource_residency_execution.cpp"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeRhiTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_page_gpu_resource_residency_execution_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-page-gpu-resource-residency-execution-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode",
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnostic",
        "RuntimeMavgPageGpuResourceResidencyExecutionDesc",
        "RuntimeMavgPageGpuResourceResidencyExecutionResult",
        "execute_runtime_mavg_page_gpu_resource_residency_actions",
        "RuntimeMavgPageGpuResourceUpdateReadinessResult",
        "RuntimeMavgPageResidencyActionResult",
        "missing_readiness_result",
        "invalid_readiness_result",
        "readiness_not_ready",
        "residency_action_failed",
        "consumed_gpu_resource_update_readiness{false}",
        "submitted_native_queue{false}",
        "allocated_rhi_resources{false}",
        "used_gpu_decompression{false}",
        "exposed_native_handles{false}"
    )) {
    Assert-ContainsText $runtimeRhiHeaderText $needle "mavg_page_gpu_resource_residency_execution.hpp handoff contract"
}

foreach ($needle in @(
        "ID3D12",
        "IDStorage",
        "DSTORAGE_",
        "#include <windows.h>",
        "#include <dstorage.h>",
        "ComPtr"
    )) {
    Assert-DoesNotContainText $runtimeRhiHeaderText $needle "mavg_page_gpu_resource_residency_execution.hpp must not expose native DirectStorage/D3D12 symbols"
    Assert-DoesNotContainText $runtimeRhiSourceText $needle "mavg_page_gpu_resource_residency_execution.cpp must stay native-handle-free"
}

foreach ($needle in @(
        "copy_readiness_evidence",
        "readiness_has_unsupported_side_effects",
        "readiness_has_completed_resource_destination",
        "copy_residency_evidence",
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::missing_readiness_result",
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::invalid_readiness_result",
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::readiness_not_ready",
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::residency_action_failed",
        "execute_runtime_mavg_page_residency_actions",
        ".resident_page_resources = readiness->resident_page_resources",
        "readiness.invoked_rhi_residency_action",
        "readiness.used_directstorage_resource_destination",
        "result.submitted_native_queue = false",
        "readiness->ready_resource_count != readiness->resident_page_resources.size()"
    )) {
    Assert-ContainsText $runtimeRhiSourceText $needle "mavg_page_gpu_resource_residency_execution.cpp fail-closed handoff implementation"
}

foreach ($needle in @(
        "src/mavg_page_gpu_resource_residency_execution.cpp"
    )) {
    Assert-ContainsText $runtimeRhiCMakeText $needle "engine/runtime_rhi/CMakeLists.txt MAVG page GPU resource residency execution source"
}

foreach ($needle in @(
        "MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests",
        "tests/unit/runtime_rhi_mavg_page_gpu_resource_residency_execution_tests.cpp",
        "MK_runtime_rhi"
    )) {
    Assert-ContainsText $rootCMakeText $needle "CMakeLists.txt MAVG page GPU resource residency execution test target"
}

foreach ($needle in @(
        "runtime rhi mavg page gpu resource residency execution consumes readiness and delegates residency actions",
        "runtime rhi mavg page gpu resource residency execution rejects missing or invalid readiness evidence",
        "runtime rhi mavg page gpu resource residency execution reports delegated residency validation failures",
        "execute_runtime_mavg_page_gpu_resource_residency_actions",
        "RuntimeMavgPageGpuResourceResidencyExecutionDesc",
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::missing_readiness_result",
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::invalid_readiness_result",
        "unsupported_readiness.allocated_rhi_resources = true",
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::readiness_not_ready",
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::residency_action_failed",
        "RuntimeMavgPageResidencyActionDiagnosticCode::missing_resident_page_resource",
        "observed_native_queue_submission",
        "submitted_native_queue",
        "allocated_rhi_resources",
        "enforced_allocator_budget",
        "used_gpu_decompression",
        "exposed_native_handles"
    )) {
    Assert-ContainsText $runtimeRhiTestsText $needle "MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests coverage"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-06-mavg-page-gpu-resource-residency-execution-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" }
    )) {
    foreach ($needle in @(
            "MAVG Page GPU Resource Residency Execution v1",
            "mavg_page_gpu_resource_residency_execution.hpp",
            "RuntimeMavgPageGpuResourceResidencyExecutionDesc",
            "RuntimeMavgPageGpuResourceResidencyExecutionResult",
            "execute_runtime_mavg_page_gpu_resource_residency_actions"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG page GPU resource residency execution evidence"
    }
    foreach ($needle in @(
            "GPU decompression",
            "allocator/GPU budget",
            "Vulkan/Metal",
            "async-overlap/performance",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG page GPU resource residency execution non-claims"
    }
}

Assert-ContainsText $aiLoopFragmentText "docs/superpowers/plans/2026-06-06-mavg-page-gpu-resource-residency-execution-v1.md" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG page GPU resource residency execution retained path"

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-page-gpu-resource-residency-execution-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG Page GPU Resource Residency Execution v1"
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
        "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_page_gpu_resource_residency_execution.hpp",
        "MAVG Page GPU Resource Residency Execution v1",
        "RuntimeMavgPageGpuResourceResidencyExecutionDesc",
        "RuntimeMavgPageGpuResourceResidencyExecutionResult",
        "RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode",
        "execute_runtime_mavg_page_gpu_resource_residency_actions",
        "RuntimeMavgPageGpuResourceUpdateReadinessResult",
        "execute_runtime_mavg_page_residency_actions",
        "MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests",
        "submitted_native_queue=false for this helper",
        "no helper-owned file IO",
        "no RHI allocation",
        "no allocator/GPU budget enforcement",
        "no mount mutation",
        "no GPU decompression",
        "no native handle exposure",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi page GPU resource residency execution evidence"
}
