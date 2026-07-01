#requires -Version 7.0
#requires -PSEdition Core
# Chapter 155 for check-ai-integration.ps1 MAVG DirectStorage GPU decompression policy gate.

$headerText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_directstorage_gpu_decompression_policy.hpp"
$sourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_directstorage_gpu_decompression_policy.cpp"
$testsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_directstorage_gpu_decompression_policy_tests.cpp"
$schemaText = Get-AgentSurfaceText "schemas/mavg-directstorage-gpu-decompression-policy.schema.json"
$validatorText = Get-AgentSurfaceText "tools/validate-mavg-directstorage-gpu-decompression-policy.ps1"
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
$policyPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-07-01-mavg-gpu-directstorage-gdeflate-policy-v1.md"

foreach ($needle in @(
        "RuntimeMavgDirectStorageGpuDecompressionPolicyRow",
        "RuntimeMavgDirectStorageGpuDecompressionPolicyResult",
        "RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode",
        "d3d12_multiple_subresources_range",
        "gdeflate",
        "zstd_preview",
        "evaluate_runtime_mavg_directstorage_gpu_decompression_policy",
        "has_runtime_mavg_directstorage_gpu_decompression_diagnostic",
        "runtime_mavg_directstorage_gpu_decompression_status_label",
        "bool mavg_directstorage_gpu_decompression_policy_ready{false};",
        "bool mavg_directstorage_gpu_destination_execution_ready{false};",
        "bool mavg_directstorage_gdeflate_execution_ready{false};",
        "bool mavg_directstorage_zstd_preview_ready{false};",
        "bool mavg_package_visible_backend_readiness_ready{false};",
        "bool mavg_directstorage_performance_ready{false};",
        "bool mavg_broad_cpu_gpu_memory_optimization_ready{false};",
        "bool mavg_nanite_compatible{false};",
        "bool mavg_nanite_equivalent{false};",
        "bool mavg_nanite_superior{false};"
    )) {
    Assert-ContainsText $headerText $needle "mavg_directstorage_gpu_decompression_policy.hpp public contract"
}

foreach ($forbiddenNeedle in @(
        "#include <dstorage.h>",
        "DStorageGetFactory",
        "IDStorageFactory",
        "IDStorageQueue",
        "IDStorageStatusArray",
        "IUnknown",
        "HANDLE",
        "void*"
    )) {
    Assert-DoesNotContainText $headerText $forbiddenNeedle "mavg_directstorage_gpu_decompression_policy.hpp native handle boundary"
    Assert-DoesNotContainText $sourceText $forbiddenNeedle "mavg_directstorage_gpu_decompression_policy.cpp native handle boundary"
}

foreach ($needle in @(
        "DirectStorage 1.4/Zstd/GACL rows are public preview",
        "DirectStorage Zstd preview requires explicitly authored Zstd/GACL assets",
        "usage before any future promotion",
        "MAVG DirectStorage/GDeflate policy rows must not promote package-visible backend readiness",
        "MAVG DirectStorage/GDeflate policy rows must not promote broad CPU/GPU/memory optimization",
        "MAVG DirectStorage/GDeflate policy rows must not claim Nanite compatibility, equivalence, or",
        "superiority",
        "execution claims require retained artifact identity",
        "retained artifact hashes must be lowercase SHA-256",
        "result.mavg_directstorage_gpu_decompression_policy_ready = true"
    )) {
    Assert-ContainsText $sourceText $needle "mavg_directstorage_gpu_decompression_policy.cpp fail-closed implementation"
}

foreach ($needle in @(
        "runtime rhi mavg directstorage gpu decompression policy accepts reviewed 1.3 gpu destination and gdeflate rows",
        "runtime rhi mavg directstorage gpu decompression policy host gates missing official review rows",
        "runtime rhi mavg directstorage gpu decompression policy leaves zstd preview host gated",
        "runtime rhi mavg directstorage gpu decompression policy blocks native handles and external claims",
        "runtime rhi mavg directstorage gpu decompression policy blocks execution claims without retained artifact",
        "missing_d3d12_synchronization_review",
        "directstorage_preview_selected",
        "zstd_preview_not_ready",
        "invalid_retained_artifact_hash"
    )) {
    Assert-ContainsText $testsText $needle "MK_runtime_rhi_mavg_ds_gpu_decomp_policy_tests coverage"
}

foreach ($needle in @(
        "GameEngine.MavgDirectStorageGpuDecompressionPolicy.v1",
        "mavg-gpu-directstorage-gdeflate-policy-v1",
        "microsoft-directstorage-api-downloads",
        "microsoft-directstorage-1.3-release",
        "microsoft-directstorage-developer-guidance",
        "microsoft-directstorage-1.4-zstd-preview",
        "d3d12_multiple_subresources_range",
        "gdeflate",
        "zstd_preview",
        "gpuDestinationExecutionReady",
        "gdeflateExecutionReady",
        "zstdPreviewReady",
        "packageVisibleBackendReadinessReady",
        "broadCpuGpuMemoryOptimizationReady",
        "unityUnrealGodotCompatibility"
    )) {
    Assert-ContainsText $schemaText $needle "mavg-directstorage-gpu-decompression-policy schema"
}

foreach ($needle in @(
        "validation_recipe=mavg-directstorage-gpu-decompression-policy",
        "MK_runtime_rhi_mavg_ds_gpu_decomp_policy_tests",
        "mavg_directstorage_gpu_decompression_policy_status=policy_ready",
        "mavg_directstorage_gpu_decompression_policy_ready=1",
        "mavg_directstorage_gpu_destination_policy_ready=1",
        "mavg_directstorage_gdeflate_policy_ready=1",
        "mavg_directstorage_gpu_destination_execution_ready=0",
        "mavg_directstorage_gdeflate_execution_ready=0",
        "mavg_directstorage_zstd_preview_ready=0",
        "mavg_package_visible_backend_readiness_ready=0",
        "mavg_directstorage_performance_ready=0",
        "mavg_broad_cpu_gpu_memory_optimization_ready=0",
        "mavg_nanite_compatible=0",
        "mavg_nanite_equivalent=0",
        "mavg_nanite_superior=0",
        "MAVG DirectStorage GPU decompression policy is incomplete"
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-mavg-directstorage-gpu-decompression-policy.ps1"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_directstorage_gpu_decompression_policy.cpp" "engine/runtime_rhi/CMakeLists.txt MAVG DirectStorage policy source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_rhi_mavg_ds_gpu_decomp_policy_tests" "root CMake MAVG DirectStorage policy test target"
Assert-ContainsText $commandsFragmentText "mavgDirectStorageGpuDecompressionPolicyCheck" "manifest commands MAVG DirectStorage policy command"
Assert-ContainsText $productionLoopFragmentText "mavgDirectStorageGpuDecompressionPolicyEvidence" "production loop retained MAVG DirectStorage policy evidence"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgMasterPlanText; Label = "MAVG master plan" },
        @{ Text = $policyPlanText; Label = "MAVG DirectStorage policy plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $productionLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "mavg-gpu-directstorage-gdeflate-policy-v1",
            "mavg_directstorage_gpu_decompression_policy_ready=1",
            "mavg_directstorage_gpu_destination_execution_ready=0",
            "mavg_directstorage_gdeflate_execution_ready=0",
            "mavg_directstorage_zstd_preview_ready=0",
            "mavg_package_visible_backend_readiness_ready=0",
            "mavg_directstorage_performance_ready=0",
            "mavg_broad_cpu_gpu_memory_optimization_ready=0",
            "mavg_nanite_compatible=0",
            "mavg_nanite_equivalent=0",
            "mavg_nanite_superior=0",
            "RuntimeMavgDirectStorageGpuDecompressionPolicyResult",
            "evaluate_runtime_mavg_directstorage_gpu_decompression_policy",
            "DirectStorage 1.4/Zstd",
            "Unity",
            "Unreal",
            "Godot",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG DirectStorage policy evidence"
    }
    foreach ($forbiddenNeedle in @(
            "mavg_directstorage_gpu_destination_execution_ready=1",
            "mavg_directstorage_gdeflate_execution_ready=1",
            "mavg_directstorage_zstd_preview_ready=1",
            "mavg_directstorage_performance_ready=1",
            "mavg_directstorage_native_handles_exposed=1",
            "mavg_nanite_compatible=1",
            "mavg_nanite_equivalent=1",
            "mavg_nanite_superior=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden MAVG DirectStorage ready claim"
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
        "mavg_directstorage_gpu_decompression_policy.hpp",
        "RuntimeMavgDirectStorageGpuDecompressionPolicyResult",
        "GameEngine.MavgDirectStorageGpuDecompressionPolicy.v1",
        "mavg_directstorage_gpu_decompression_policy_ready=1",
        "mavg_directstorage_gpu_destination_execution_ready=0",
        "mavg_directstorage_gdeflate_execution_ready=0",
        "mavg_directstorage_zstd_preview_ready=0",
        "mavg_package_visible_backend_readiness_ready=0"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi MAVG DirectStorage policy evidence"
}
