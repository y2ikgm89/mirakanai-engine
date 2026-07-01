#requires -Version 7.0
#requires -PSEdition Core
# Chapter 156 for check-ai-integration.ps1 MAVG DirectStorage GPU destination execution evidence gate.

$headerText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_directstorage_gpu_destination_execution.hpp"
$sourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_directstorage_gpu_destination_execution.cpp"
$testsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_directstorage_gpu_destination_execution_tests.cpp"
$schemaText = Get-AgentSurfaceText "schemas/mavg-directstorage-gpu-destination-execution.schema.json"
$validatorText = Get-AgentSurfaceText "tools/validate-mavg-directstorage-gpu-destination-execution.ps1"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$destinationPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-07-01-mavg-directstorage-gpu-destination-execution-evidence-v1.md"

foreach ($needle in @(
        "RuntimeMavgDirectStorageGpuDestinationExecutionRow",
        "RuntimeMavgDirectStorageGpuDestinationExecutionResult",
        "RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode",
        "RuntimeMavgDirectStorageGpuDestinationKind",
        "d3d12_multiple_subresources_range",
        "evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence",
        "has_runtime_mavg_directstorage_gpu_destination_execution_diagnostic",
        "runtime_mavg_directstorage_gpu_destination_execution_status_label",
        "bool mavg_directstorage_gpu_destination_execution_ready{false};",
        "bool mavg_directstorage_multiple_subresources_range_execution_ready{false};",
        "bool mavg_directstorage_gdeflate_execution_ready{false};",
        "bool mavg_directstorage_zstd_preview_ready{false};",
        "bool mavg_package_visible_backend_readiness_ready{false};",
        "bool mavg_directstorage_performance_ready{false};",
        "bool mavg_broad_cpu_gpu_memory_optimization_ready{false};",
        "bool mavg_nanite_compatible{false};",
        "bool mavg_nanite_equivalent{false};",
        "bool mavg_nanite_superior{false};",
        "bool mavg_external_engine_compatibility{false};"
    )) {
    Assert-ContainsText $headerText $needle "mavg_directstorage_gpu_destination_execution.hpp public contract"
}

foreach ($forbiddenNeedle in @(
        "#include <dstorage.h>",
        "#include <d3d12.h>",
        "DStorageGetFactory",
        "IDStorageFactory",
        "IDStorageQueue",
        "IDStorageStatusArray",
        "ID3D12",
        "IUnknown",
        "HANDLE",
        "void*"
    )) {
    Assert-DoesNotContainText $headerText $forbiddenNeedle "mavg_directstorage_gpu_destination_execution.hpp native handle boundary"
    Assert-DoesNotContainText $sourceText $forbiddenNeedle "mavg_directstorage_gpu_destination_execution.cpp native handle boundary"
}

foreach ($needle in @(
        "DirectStorage 1.4/Zstd/GACL preview rows are future review input only",
        "requires the EnqueueRequests API",
        "requires D3D12 fence synchronization",
        "must start in D3D12_RESOURCE_STATE_COMMON",
        "deterministic readback hash evidence",
        "package-visible output evidence",
        "must not claim GDeflate execution readiness",
        "must not claim DirectStorage 1.4/Zstd preview",
        "must not promote package-visible backend",
        "must not claim Nanite compatibility",
        "must not claim Unity, Unreal, or Godot"
    )) {
    Assert-ContainsText $sourceText $needle "mavg_directstorage_gpu_destination_execution.cpp fail-closed implementation"
}

foreach ($needle in @(
        "runtime rhi mavg directstorage gpu destination execution accepts reviewed 1.3 range evidence",
        "runtime rhi mavg directstorage gpu destination execution host gates missing execution evidence",
        "runtime rhi mavg directstorage gpu destination execution blocks invalid artifacts and ranges",
        "runtime rhi mavg directstorage gpu destination execution keeps preview and decompression claims blocked",
        "runtime rhi mavg directstorage gpu destination execution accepts buffer evidence without range readiness",
        "runtime rhi mavg directstorage gpu destination execution blocks native handle exposure",
        "missing_d3d12_fence_synchronization",
        "directstorage_preview_selected",
        "gdeflate_execution_claim_not_allowed",
        "external_engine_claim_not_allowed"
    )) {
    Assert-ContainsText $testsText $needle "MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests coverage"
}

foreach ($needle in @(
        "GameEngine.MavgDirectStorageGpuDestinationExecutionEvidence.v1",
        "mavg-directstorage-gpu-destination-execution-evidence-v1",
        "microsoft-directstorage-api-downloads",
        "microsoft-directstorage-1.3-enqueue-requests",
        "microsoft-directstorage-1.3-multiple-subresources-range",
        "microsoft-directstorage-developer-guidance",
        "microsoft-directstorage-1.4-zstd-preview",
        "microsoft-d3d12-resource-barriers",
        "d3d12_multiple_subresources_range",
        "enqueueRequestsUsed",
        "d3d12FenceSynchronizationUsed",
        "destinationResourceStateCommon",
        "gdeflateExecutionReady",
        "zstdPreviewReady",
        "packageVisibleBackendReadinessReady",
        "broadCpuGpuMemoryOptimizationReady",
        "unityUnrealGodotCompatibility"
    )) {
    Assert-ContainsText $schemaText $needle "mavg-directstorage-gpu-destination-execution schema"
}

foreach ($needle in @(
        "validation_recipe=mavg-directstorage-gpu-destination-execution-evidence",
        "MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests",
        "mavg_directstorage_gpu_destination_execution_status=host_evidence_required",
        "mavg_directstorage_gpu_destination_execution_ready=0",
        "mavg_directstorage_multiple_subresources_range_execution_ready=0",
        "mavg_directstorage_gdeflate_execution_ready=0",
        "mavg_directstorage_zstd_preview_ready=0",
        "mavg_directstorage_native_handles_exposed=0",
        "mavg_directstorage_performance_ready=0",
        "mavg_package_visible_backend_readiness_ready=0",
        "mavg_broad_cpu_gpu_memory_optimization_ready=0",
        "mavg_nanite_compatible=0",
        "mavg_nanite_equivalent=0",
        "mavg_nanite_superior=0",
        "mavg_external_engine_compatibility=0",
        "requires retained host artifacts before -RequireReady can pass"
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-mavg-directstorage-gpu-destination-execution.ps1"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_directstorage_gpu_destination_execution.cpp" "engine/runtime_rhi/CMakeLists.txt MAVG DirectStorage GPU destination source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests" "root CMake MAVG DirectStorage GPU destination test target"
Assert-ContainsText $commandsFragmentText "mavgDirectStorageGpuDestinationExecutionCheck" "manifest commands MAVG DirectStorage GPU destination command"
Assert-ContainsText $productionLoopFragmentText "mavgDirectStorageGpuDestinationExecutionEvidence" "production loop retained MAVG DirectStorage GPU destination evidence"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgMasterPlanText; Label = "MAVG master plan" },
        @{ Text = $destinationPlanText; Label = "MAVG DirectStorage GPU destination execution plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $productionLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "mavg-directstorage-gpu-destination-execution-evidence-v1",
            "mavg_directstorage_gpu_destination_execution_ready=0",
            "mavg_directstorage_multiple_subresources_range_execution_ready=0",
            "mavg_directstorage_gdeflate_execution_ready=0",
            "mavg_directstorage_zstd_preview_ready=0",
            "mavg_package_visible_backend_readiness_ready=0",
            "mavg_directstorage_performance_ready=0",
            "mavg_broad_cpu_gpu_memory_optimization_ready=0",
            "mavg_nanite_compatible=0",
            "mavg_nanite_equivalent=0",
            "mavg_nanite_superior=0",
            "mavg_external_engine_compatibility=0",
            "RuntimeMavgDirectStorageGpuDestinationExecutionResult",
            "evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence",
            "DirectStorage 1.3",
            "DirectStorage 1.4/Zstd",
            "Unity",
            "Unreal",
            "Godot",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG DirectStorage GPU destination evidence"
    }
    foreach ($forbiddenNeedle in @(
            "mavg_directstorage_gdeflate_execution_ready=1",
            "mavg_directstorage_zstd_preview_ready=1",
            "mavg_directstorage_performance_ready=1",
            "mavg_directstorage_native_handles_exposed=1",
            "mavg_broad_cpu_gpu_memory_optimization_ready=1",
            "mavg_nanite_compatible=1",
            "mavg_nanite_equivalent=1",
            "mavg_nanite_superior=1",
            "mavg_external_engine_compatibility=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden MAVG DirectStorage GPU destination ready claim"
    }
}

$manifest = $manifestText | ConvertFrom-Json
$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module"
}
$runtimeRhiManifestText = ((@($runtimeRhiModule[0].publicHeaders) -join " "),
    (@($runtimeRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_directstorage_gpu_destination_execution.hpp",
        "RuntimeMavgDirectStorageGpuDestinationExecutionResult",
        "GameEngine.MavgDirectStorageGpuDestinationExecutionEvidence.v1",
        "mavg_directstorage_gpu_destination_execution_ready=0",
        "mavg_directstorage_multiple_subresources_range_execution_ready=0",
        "mavg_directstorage_gdeflate_execution_ready=0",
        "mavg_directstorage_zstd_preview_ready=0",
        "mavg_package_visible_backend_readiness_ready=0"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi MAVG DirectStorage GPU destination evidence"
}
