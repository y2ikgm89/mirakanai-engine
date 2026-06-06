#requires -Version 7.0
#requires -PSEdition Core
# Chapter 127 for check-ai-integration.ps1 static contracts.

$publicHeaderText = Get-AgentSurfaceText "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp"
$directStorageSourceText = Get-AgentSurfaceText "engine/runtime_host/win32/src/win32_mavg_directstorage_payload_io.cpp"
$win32SourceText = Get-AgentSurfaceText "engine/runtime_host/win32/src/win32_mavg_payload_io.cpp"
$runtimeHostWin32CmakeText = Get-AgentSurfaceText "engine/runtime_host/win32/CMakeLists.txt"
$rootCmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$packageConfigText = Get-AgentSurfaceText "cmake/MirakanaiConfig.cmake.in"
$directStorageSdkTestsText = Get-AgentSurfaceText "tests/unit/runtime_host_win32_directstorage_sdk_tests.cpp"
$runtimeHostWin32TestsText = Get-AgentSurfaceText "tests/unit/runtime_host_win32_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgDirectStorageNativePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-directstorage-native-execution-v1.md"
$mavgDirectStorageFencePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-directstorage-native-fence-signal-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$validationRecipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "#if defined(MK_RUNTIME_HOST_WIN32_ENABLE_DIRECTSTORAGE_SDK)",
        "Win32MavgPayloadDirectStorageDispatcherDesc",
        "Win32MavgPayloadDirectStorageDispatcher",
        "std::filesystem::path root_path",
        "std::size_t max_inflight_submissions",
        "std::unique_ptr<Impl> impl_"
    )) {
    Assert-ContainsText $publicHeaderText $needle "win32_mavg_payload_io.hpp optional DirectStorage public PIMPL contract"
}

foreach ($needle in @(
        "#include <dstorage.h>",
        "#include <dstorageerr.h>",
        "IDStorage",
        "DSTORAGE_",
        "DStorageGetFactory",
        "ID3D12",
        "HANDLE",
        "OVERLAPPED",
        "ComPtr",
        "Microsoft::WRL",
        "#include <windows.h>"
    )) {
    Assert-DoesNotContainText $publicHeaderText $needle "win32_mavg_payload_io.hpp must not expose native DirectStorage/D3D12/Win32 symbols"
}

foreach ($needle in @(
        "#include <windows.h>",
        "#include <d3d12.h>",
        "#include <dstorage.h>",
        "#include <dstorageerr.h>",
        "#include <dxgi1_6.h>",
        "DStorageGetFactory(IID_PPV_ARGS",
        "Microsoft::WRL::ComPtr<IDStorageFactory>",
        "Microsoft::WRL::ComPtr<IDStorageQueue>",
        "Microsoft::WRL::ComPtr<ID3D12Fence>",
        "CreateQueue",
        "OpenFile",
        "CreateStatusArray",
        "queue->Close();",
        "EnqueueRequest",
        "EnqueueSignal",
        "EnqueueStatus",
        "Submit()",
        "IDStorageStatusArray",
        "IsComplete",
        "GetHResult",
        "D3D12CreateDevice",
        "CreateDXGIFactory2",
        "EnumWarpAdapter",
        "CreateFence",
        "reserve_pending_submission",
        "SubmissionReservation",
        "has_root_name()",
        "has_root_directory()",
        "signal_fence_after_requests",
        "native_fence_signal_value",
        "native_fence_completed_value",
        "used_native_directstorage = true",
        "enqueued_native_requests = true",
        "submitted_native_queue = true",
        "enqueued_status_write = true"
    )) {
    Assert-ContainsText $directStorageSourceText $needle "win32_mavg_directstorage_payload_io.cpp private DirectStorage execution contract"
}

foreach ($needle in @(
        "has_root_name()",
        "has_root_directory()"
    )) {
    Assert-ContainsText $win32SourceText $needle "win32_mavg_payload_io.cpp rooted/drive-relative source path rejection"
}

foreach ($needle in @(
        "MK_runtime_host_win32_directstorage",
        "src/win32_mavg_directstorage_payload_io.cpp",
        "MK_ENABLE_DIRECTSTORAGE_SDK",
        "Microsoft::DirectStorage",
        "d3d12",
        "dxgi",
        "MK_RUNTIME_HOST_WIN32_ENABLE_DIRECTSTORAGE_SDK=1"
    )) {
    Assert-ContainsText $runtimeHostWin32CmakeText $needle "engine/runtime_host/win32/CMakeLists.txt optional DirectStorage target"
}

foreach ($needle in @(
        "find_package(dstorage CONFIG REQUIRED)",
        "MK_runtime_host_win32_directstorage_sdk_tests",
        "MK_runtime_host_win32_directstorage",
        "Microsoft::DirectStorage",
        "MK_set_export_name(MK_runtime_host_win32_directstorage runtime_host_win32_directstorage)",
        "MK_PACKAGE_HAS_DIRECTSTORAGE_SDK"
    )) {
    Assert-ContainsText $rootCmakeText $needle "CMakeLists.txt optional DirectStorage package/install/test wiring"
}

foreach ($needle in @(
        "Mirakanai_HAS_DIRECTSTORAGE_SDK",
        "find_dependency(dstorage CONFIG)"
    )) {
    Assert-ContainsText $packageConfigText $needle "MirakanaiConfig.cmake.in optional DirectStorage dependency forwarding"
}

foreach ($needle in @(
        "#include <dstorage.h>",
        "#include <dstorageerr.h>",
        "&DStorageGetFactory",
        "run_directstorage_file_to_memory_queue_status_execution_test",
        "run_directstorage_fence_signal_execution_test",
        "run_directstorage_rejects_unsafe_source_paths_before_factory_test",
        "Win32MavgPayloadDirectStorageDispatcher",
        "used_native_directstorage",
        "enqueued_native_requests",
        "submitted_native_queue",
        "enqueued_status_write",
        "signal_fence_after_requests = true",
        "native_fence_signal_value",
        "native_fence_completed_value",
        "C:payload.bin",
        "payload.bin"
    )) {
    Assert-ContainsText $directStorageSdkTestsText $needle "MK_runtime_host_win32_directstorage_sdk_tests native execution coverage"
}

foreach ($needle in @(
        "win32 mavg payload async file io dispatcher rejects drive relative source path before file open",
        "win32 mavg payload iocp file io worker rejects rooted source path before file open",
        "C:payload.bin",
        "relative single-line path"
    )) {
    Assert-ContainsText $runtimeHostWin32TestsText $needle "MK_runtime_host_win32_tests unsafe source path coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgDirectStorageNativePlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-directstorage-native-execution-v1.md" },
        @{ Text = $mavgDirectStorageFencePlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-directstorage-native-fence-signal-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $productionMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "MAVG DirectStorage Native Execution v1",
            "mavg-directstorage-native-execution-v1",
            "MK_runtime_host_win32_directstorage"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) DirectStorage native execution evidence"
    }
    foreach ($needle in @(
            "D3D12 resource",
            "GPU decompression",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) DirectStorage native execution non-claims"
    }
}

foreach ($needle in @(
        "MAVG DirectStorage Native Execution v1",
        "MK_runtime_host_win32_directstorage",
        "Win32MavgPayloadDirectStorageDispatcher",
        "MK_RUNTIME_HOST_WIN32_ENABLE_DIRECTSTORAGE_SDK",
        "DStorageGetFactory",
        "IDStorageFactory::OpenFile",
        "IDStorageQueue::EnqueueRequest",
        "IDStorageStatusArray",
        "used_native_directstorage=true",
        "enqueued_native_requests",
        "submitted_native_queue",
        "enqueued_status_write",
        "signal_fence_after_requests",
        "native_fence_signal_value",
        "native_fence_completed_value",
        "no default DirectStorage dependency",
        "no public native handles",
        "no D3D12 resource destination",
        "no GPU decompression",
        "no Nanite compatibility/equivalence/superiority"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json DirectStorage native execution evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json DirectStorage native execution evidence"
}

foreach ($needle in @(
        '"name": "mavg-directstorage-native-execution"',
        "tools/validate-directstorage-sdk.ps1",
        "used_native_directstorage=true",
        "native_fence_signal_value",
        "unsafe source-path rejection",
        "no D3D12 resource destination",
        "no GPU decompression"
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle "engine/agent/manifest.fragments/009-validationRecipes.json DirectStorage native execution recipe"
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-directstorage-native-execution-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG DirectStorage Native Execution v1"
}
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-directstorage-native-fence-signal-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG DirectStorage Native Fence Signal v1"
}

$recommendedPlanText = (([string]$productionLoop.recommendedNextPlan.retainedCompletedPlanEvidence), ([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG DirectStorage Native Execution v1",
        "MAVG DirectStorage Native Fence Signal v1",
        "MK_runtime_host_win32_directstorage",
        "Win32MavgPayloadDirectStorageDispatcher",
        "DStorageGetFactory",
        "used_native_directstorage=true",
        "signal_fence_after_requests",
        "native_fence_signal_value",
        "no public native handles",
        "no D3D12 resource destination",
        "no GPU decompression",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan DirectStorage native execution evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG DirectStorage Native Execution v1",
        "no dstorage.h",
        "used_native_directstorage=true",
        "enqueued_native_requests",
        "native_fence_signal_value",
        "no default DirectStorage SDK dependency",
        "no D3D12 resource-destination IO",
        "no GPU decompression"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime DirectStorage native execution boundary"
}

$win32RuntimeHostModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_host_win32" })
if ($win32RuntimeHostModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_host_win32 module" }
$win32RuntimeHostManifestText = ((@($win32RuntimeHostModule[0].recentEvidence) -join " "), [string]$win32RuntimeHostModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG DirectStorage Native Execution v1",
        "MK_runtime_host_win32_directstorage",
        "Win32MavgPayloadDirectStorageDispatcher",
        "DStorageGetFactory",
        "IDStorageFactory::OpenFile",
        "used_native_directstorage=true",
        "signal_fence_after_requests",
        "IDStorageQueue::EnqueueSignal",
        "native_fence_signal_value",
        "no public native handles",
        "no D3D12 resource destination",
        "no GPU decompression"
    )) {
    Assert-ContainsText $win32RuntimeHostManifestText $needle "engine/agent/manifest.json MK_runtime_host_win32 DirectStorage native execution evidence"
}

$directStorageModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_host_win32_directstorage" })
if ($directStorageModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_host_win32_directstorage module" }
$directStorageManifestText = ((@($directStorageModule[0].dependencies) -join " "), (@($directStorageModule[0].publicHeaders) -join " "), (@($directStorageModule[0].recentEvidence) -join " "), [string]$directStorageModule[0].purpose) -join " "
foreach ($needle in @(
        "Microsoft::DirectStorage",
        "MK_RUNTIME_HOST_WIN32_ENABLE_DIRECTSTORAGE_SDK",
        "Win32MavgPayloadDirectStorageDispatcher",
        "DStorageGetFactory",
        "IDStorageQueue::EnqueueRequest",
        "IDStorageStatusArray",
        "used_native_directstorage=true",
        "enqueued_native_requests",
        "submitted_native_queue",
        "enqueued_status_write",
        "IDStorageQueue::EnqueueSignal",
        "native_fence_signal_value",
        "not a default dependency",
        "not D3D12 resource-destination IO",
        "not GPU decompression",
        "private DirectStorage fence signal"
    )) {
    Assert-ContainsText $directStorageManifestText $needle "engine/agent/manifest.json MK_runtime_host_win32_directstorage optional module"
}
