#requires -Version 7.0
#requires -PSEdition Core
# Chapter 118 for check-ai-integration.ps1 static contracts.

$vcpkgManifestText = Get-AgentSurfaceText "vcpkg.json"
$cmakeListsText = Get-AgentSurfaceText "CMakeLists.txt"
$cmakePresetsText = Get-AgentSurfaceText "CMakePresets.json"
$bootstrapDepsText = Get-AgentSurfaceText "tools/bootstrap-deps.ps1"
$validateDirectStorageSdkText = Get-AgentSurfaceText "tools/validate-directstorage-sdk.ps1"
$directStorageSdkSmokeText = Get-AgentSurfaceText "tests/unit/runtime_host_win32_directstorage_sdk_tests.cpp"
$dependencyPolicyText = Get-AgentSurfaceText "tools/check-dependency-policy.ps1"
$nativeDesktopContractsText = Get-AgentSurfaceText "tools/check-native-desktop-contracts.ps1"
$publicApiBoundaryText = Get-AgentSurfaceText "tools/check-public-api-boundaries.ps1"
$thirdPartyNoticesText = Get-AgentSurfaceText "THIRD_PARTY_NOTICES.md"
$dependencyDocsText = Get-AgentSurfaceText "docs/dependencies.md"
$legalDocsText = Get-AgentSurfaceText "docs/legal-and-licensing.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgDirectStorageSdkPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-directstorage-sdk-dependency-gate-v1.md"
$mavgWin32IocpPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-win32-iocp-file-io-worker-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        '"directstorage-sdk"',
        '"dstorage"',
        "Optional Windows DirectStorage SDK dependency gate"
    )) {
    Assert-ContainsText $vcpkgManifestText $needle "vcpkg.json DirectStorage SDK feature gate"
}

foreach ($needle in @(
        "MK_ENABLE_DIRECTSTORAGE_SDK",
        "find_package(dstorage CONFIG REQUIRED)",
        "MK_runtime_host_win32_directstorage_sdk_tests",
        "Microsoft::DirectStorage",
        "Microsoft::DirectStorageCore",
        "copy_if_different"
    )) {
    Assert-ContainsText $cmakeListsText $needle "CMakeLists.txt DirectStorage SDK smoke target"
}

foreach ($needle in @(
        '"name": "directstorage-sdk"',
        "MK_ENABLE_DIRECTSTORAGE_SDK",
        "VCPKG_MANIFEST_INSTALL",
        "VCPKG_INSTALLED_DIR",
        "x64-windows"
    )) {
    Assert-ContainsText $cmakePresetsText $needle "CMakePresets.json DirectStorage SDK preset"
}

Assert-ContainsText $bootstrapDepsText "--x-feature=directstorage-sdk" "tools/bootstrap-deps.ps1 DirectStorage SDK bootstrap"

foreach ($needle in @(
        "Assert-VcpkgExecutable",
        "Set-MirakanaiVcpkgEnvironment",
        "--preset directstorage-sdk",
        "--target MK_runtime_host_win32_directstorage_sdk_tests",
        "-R `"MK_runtime_host_win32_directstorage_sdk_tests`""
    )) {
    Assert-ContainsText $validateDirectStorageSdkText $needle "tools/validate-directstorage-sdk.ps1 wrapper"
}

foreach ($needle in @(
        "#include <dstorage.h>",
        "#include <dstorageerr.h>",
        "DSTORAGE_REQUEST",
        "DSTORAGE_CONFIGURATION1",
        "&DStorageGetFactory"
    )) {
    Assert-ContainsText $directStorageSdkSmokeText $needle "tests/unit/runtime_host_win32_directstorage_sdk_tests.cpp SDK compile/link smoke"
}
Assert-DoesNotContainText $directStorageSdkSmokeText "DStorageGetFactory(" "DirectStorage SDK smoke must not execute factory creation"
Assert-DoesNotContainText $directStorageSdkSmokeText "IDStorageFactory" "DirectStorage SDK smoke must not create native factories"
Assert-DoesNotContainText $directStorageSdkSmokeText "IDStorageQueue" "DirectStorage SDK smoke must not create native queues"
Assert-DoesNotContainText $directStorageSdkSmokeText "ID3D12Fence" "DirectStorage SDK smoke must not use D3D12 fences"

foreach ($needle in @(
        "directstorage-sdk",
        "dstorage",
        "Microsoft DirectStorage SDK",
        "tools/validate-directstorage-sdk.ps1",
        "MK_runtime_host_win32_directstorage_sdk_tests",
        "IDStorage",
        "--x-feature=directstorage-sdk"
    )) {
    Assert-ContainsText $dependencyPolicyText $needle "tools/check-dependency-policy.ps1 DirectStorage SDK policy"
}

foreach ($needle in @(
        "dstorage",
        "dstorageerr",
        "IDStorage",
        "DSTORAGE_",
        "DStorage"
    )) {
    Assert-ContainsText $nativeDesktopContractsText $needle "tools/check-native-desktop-contracts.ps1 DirectStorage public contract guard"
    Assert-ContainsText $publicApiBoundaryText $needle "tools/check-public-api-boundaries.ps1 DirectStorage public API guard"
}

foreach ($surface in @(
        @{ Text = $thirdPartyNoticesText; Label = "THIRD_PARTY_NOTICES.md" },
        @{ Text = $dependencyDocsText; Label = "docs/dependencies.md" },
        @{ Text = $legalDocsText; Label = "docs/legal-and-licensing.md" }
    )) {
    foreach ($needle in @(
            "Microsoft DirectStorage SDK",
            "directstorage-sdk",
            "dstorage",
            "LicenseRef-Microsoft-DirectStorage-SDK",
            "LICENSE-CODE.txt"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) DirectStorage SDK dependency/legal records"
    }
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgDirectStorageSdkPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-directstorage-sdk-dependency-gate-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-directstorage-sdk-dependency-gate-v1",
            "DirectStorage SDK Dependency Gate v1",
            "directstorage-sdk",
            "dstorage",
            "MK_ENABLE_DIRECTSTORAGE_SDK",
            "tools/validate-directstorage-sdk.ps1",
            "MK_runtime_host_win32_directstorage_sdk_tests",
            "Microsoft::DirectStorage",
            "DStorageGetFactory",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG DirectStorage SDK dependency gate evidence"
    }
}

foreach ($needle in @(
        "Completed/published as draft PR #466",
        "mavg-directstorage-sdk-dependency-gate-v1",
        "Win32MavgPayloadIocpFileIoDispatcher"
    )) {
    Assert-ContainsText $mavgWin32IocpPlanText $needle "docs/superpowers/plans/2026-06-06-mavg-win32-iocp-file-io-worker-v1.md completed retained evidence"
}

Assert-ContainsText $commandsFragmentText "validateDirectStorageSdk" "engine/agent/manifest.fragments/002-commands.json DirectStorage SDK command"
Assert-ContainsText $commandsFragmentText "tools/validate-directstorage-sdk.ps1" "engine/agent/manifest.fragments/002-commands.json DirectStorage SDK command"

foreach ($needle in @(
        "MAVG DirectStorage SDK Dependency Gate v1",
        "directstorage-sdk",
        "dstorage",
        "MK_ENABLE_DIRECTSTORAGE_SDK",
        "MK_runtime_host_win32_directstorage_sdk_tests",
        "Microsoft::DirectStorage",
        "Microsoft::DirectStorageCore",
        "no DStorageGetFactory call",
        "Nanite compatibility/equivalence/superiority"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json DirectStorage SDK module evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json DirectStorage SDK active pointer"
}

foreach ($needle in @(
        "mavg-win32-iocp-file-io-worker-v1",
        "docs/superpowers/plans/2026-06-06-mavg-win32-iocp-file-io-worker-v1.md",
        "draft PR #466",
        "executed_background_worker=true",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json retained IOCP evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if ([string]$productionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-06-mavg-directstorage-sdk-dependency-gate-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must select MAVG DirectStorage SDK Dependency Gate v1"
}
if ([string]$productionLoop.recommendedNextPlan.id -ne "mavg-directstorage-sdk-dependency-gate-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must select mavg-directstorage-sdk-dependency-gate-v1"
}
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-win32-iocp-file-io-worker-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG Win32 IOCP File IO Worker v1"
}

$commandSurface = $manifest.commands
if ([string]$commandSurface.validateDirectStorageSdk -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-directstorage-sdk.ps1") {
    Write-Error "engine/agent/manifest.json commands.validateDirectStorageSdk must point at tools/validate-directstorage-sdk.ps1"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG DirectStorage SDK Dependency Gate v1",
        "no dstorage.h",
        "no DirectStorage file IO",
        "no renderer/RHI handles",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime DirectStorage SDK boundary evidence"
}

$win32RuntimeHostModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_host_win32" })
if ($win32RuntimeHostModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_host_win32 module" }
$win32RuntimeHostManifestText = ((@($win32RuntimeHostModule[0].recentEvidence) -join " "), [string]$win32RuntimeHostModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG DirectStorage SDK Dependency Gate v1",
        "directstorage-sdk",
        "dstorage",
        "MK_runtime_host_win32_directstorage_sdk_tests",
        "Microsoft::DirectStorage",
        "no DStorageGetFactory call",
        "Nanite compatibility/equivalence/superiority"
    )) {
    Assert-ContainsText $win32RuntimeHostManifestText $needle "engine/agent/manifest.json MK_runtime_host_win32 DirectStorage SDK evidence"
}
