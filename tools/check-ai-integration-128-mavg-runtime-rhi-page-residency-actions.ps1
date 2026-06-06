#requires -Version 7.0
#requires -PSEdition Core
# Chapter 128 for check-ai-integration.ps1 static contracts.

$publicHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_residency.hpp"
$sourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_residency.cpp"
$runtimeRhiCmakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeRhiTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_residency_tests.cpp"
$d3d12TestsText = Get-AgentSurfaceText "tests/unit/d3d12_rhi_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgRuntimeRhiResidencyPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-runtime-rhi-page-residency-actions-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgPageResidencyActionDiagnosticCode",
        "RuntimeMavgPageResidencyActionDiagnostic",
        "RuntimeMavgResidentPageResourceRow",
        "RuntimeMavgPageResidencyActionRow",
        "RuntimeMavgPageResidencyActionDesc",
        "RuntimeMavgPageResidencyActionResult",
        "execute_runtime_mavg_page_residency_actions",
        "rhi::RhiResidencyResourceRef",
        "invoked_rhi_residency_action",
        "invoked_make_resident_action",
        "invoked_evict_action",
        "invoked_native_make_resident",
        "invoked_native_evict",
        "exposed_native_handles",
        "enforced_allocator_budget",
        "invoked_file_io",
        "mutated_mount_set",
        "used_directstorage_resource_destination",
        "used_gpu_decompression"
    )) {
    Assert-ContainsText $publicHeaderText $needle "mavg_residency.hpp public MAVG residency action adapter contract"
}

foreach ($needle in @(
        "ID3D12",
        "DStorage",
        "dstorage",
        "IDXGI",
        "HANDLE",
        "windows.h"
    )) {
    Assert-DoesNotContainText $publicHeaderText $needle "mavg_residency.hpp must remain backend/native-handle free"
    Assert-DoesNotContainText $sourceText $needle "mavg_residency.cpp must call only the RHI abstraction"
}

foreach ($needle in @(
        "device.execute_residency_action",
        "RhiResidencyActionKind::make_resident",
        "RhiResidencyActionKind::evict",
        "missing_resident_page_resource",
        "duplicate_resident_page_resource",
        "invalid_resident_page_resource",
        "rhi_make_resident_failed",
        "rhi_evict_failed",
        "invoked_rhi_residency_action",
        "merge_rhi_evidence"
    )) {
    Assert-ContainsText $sourceText $needle "mavg_residency.cpp RHI action planning and diagnostics"
}

foreach ($needle in @(
        "src/mavg_residency.cpp"
    )) {
    Assert-ContainsText $runtimeRhiCmakeText $needle "engine/runtime_rhi/CMakeLists.txt MAVG residency source/header wiring"
}

foreach ($needle in @(
        "MK_runtime_rhi_mavg_residency_tests",
        "tests/unit/runtime_rhi_mavg_residency_tests.cpp",
        "MK_runtime_rhi"
    )) {
    Assert-ContainsText $rootCmakeText $needle "CMakeLists.txt MAVG runtime RHI residency test target"
}

foreach ($needle in @(
        "runtime rhi mavg page residency makes selected pages resident and evicts reviewed unprotected pages",
        "runtime rhi mavg page residency rejects duplicate and missing resource rows before rhi calls",
        "RuntimeMavgResidentPageResourceRow",
        "RuntimeMavgPageResidencyActionDesc",
        "duplicate_resident_page_resource",
        "missing_resident_page_resource",
        "protected_skip_count == 1U",
        "made_resident_count == 2U",
        "evicted_count == 1U",
        "!result.invoked_file_io",
        "!result.mutated_mount_set",
        "!result.used_directstorage_resource_destination",
        "!result.used_gpu_decompression"
    )) {
    Assert-ContainsText $runtimeRhiTestsText $needle "MK_runtime_rhi_mavg_residency_tests deterministic adapter coverage"
}

foreach ($needle in @(
        '#include "mirakana/runtime_rhi/mavg_residency.hpp"',
        "d3d12 rhi mavg page residency adapter invokes native residency actions",
        "RuntimeMavgResidentPageResourceRow",
        "execute_runtime_mavg_page_residency_actions",
        "result.invoked_native_make_resident",
        "result.invoked_native_evict",
        "result.made_resident_count == 2U",
        "result.evicted_count == 1U",
        "result.protected_skip_count == 1U",
        "!result.exposed_native_handles",
        "!result.enforced_allocator_budget",
        "!result.used_directstorage_resource_destination",
        "!result.used_gpu_decompression"
    )) {
    Assert-ContainsText $d3d12TestsText $needle "MK_d3d12_rhi_tests native MAVG adapter coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgRuntimeRhiResidencyPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-runtime-rhi-page-residency-actions-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $productionMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "MAVG Runtime RHI Page Residency Actions v1",
            "mavg-runtime-rhi-page-residency-actions-v1",
            "RuntimeMavgResidentPageResourceRow",
            "RuntimeMavgPageResidencyActionDesc",
            "RuntimeMavgPageResidencyActionResult",
            "execute_runtime_mavg_page_residency_actions",
            "IRhiDevice::execute_residency_action",
            "ID3D12Device::MakeResident",
            "ID3D12Device::Evict"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG runtime RHI residency evidence"
    }
    foreach ($needle in @(
            "allocator/GPU budget enforcement",
            "DirectStorage native fence",
            "D3D12 resource-destination",
            "GPU decompression",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG runtime RHI residency non-claims"
    }
}

foreach ($needle in @(
        "MAVG Runtime RHI Page Residency Actions v1",
        "mavg_residency.hpp",
        "RuntimeMavgResidentPageResourceRow",
        "RuntimeMavgPageResidencyActionDesc",
        "RuntimeMavgPageResidencyActionResult",
        "execute_runtime_mavg_page_residency_actions",
        "IRhiDevice::execute_residency_action",
        "ID3D12Device::MakeResident",
        "ID3D12Device::Evict",
        "exposed_native_handles=false",
        "enforced_allocator_budget=false",
        "no DirectStorage native fence",
        "no D3D12 resource-destination IO",
        "no GPU decompression",
        "no async-overlap/performance claim",
        "no Nanite compatibility/equivalence/superiority"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MAVG runtime RHI residency evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG runtime RHI residency evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-runtime-rhi-page-residency-actions-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG Runtime RHI Page Residency Actions v1"
}

$recommendedPlanText = (([string]$productionLoop.recommendedNextPlan.retainedCompletedPlanEvidence), ([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG Runtime RHI Page Residency Actions v1",
        "mavg_residency.hpp",
        "RuntimeMavgResidentPageResourceRow",
        "RuntimeMavgPageResidencyActionDesc",
        "RuntimeMavgPageResidencyActionResult",
        "execute_runtime_mavg_page_residency_actions",
        "ID3D12Device::MakeResident",
        "ID3D12Device::Evict",
        "exposed_native_handles=false",
        "enforced_allocator_budget=false",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan MAVG runtime RHI residency evidence"
}

$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module" }
$runtimeRhiManifestText = ((@($runtimeRhiModule[0].publicHeaders) -join " "), (@($runtimeRhiModule[0].recentEvidence) -join " "), [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_residency.hpp",
        "MAVG Runtime RHI Page Residency Actions v1",
        "RuntimeMavgResidentPageResourceRow",
        "RuntimeMavgPageResidencyActionDesc",
        "RuntimeMavgPageResidencyActionResult",
        "execute_runtime_mavg_page_residency_actions",
        "IRhiDevice::execute_residency_action",
        "no DirectStorage resource destination",
        "no GPU decompression",
        "no allocator/GPU budget enforcement",
        "no Nanite compatibility/equivalence/superiority"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi MAVG runtime RHI residency evidence"
}
