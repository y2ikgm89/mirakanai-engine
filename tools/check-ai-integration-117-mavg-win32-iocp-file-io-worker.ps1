#requires -Version 7.0
#requires -PSEdition Core
# Chapter 117 for check-ai-integration.ps1 static contracts.

$win32MavgPayloadIoHeaderText = Get-AgentSurfaceText "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp"
$win32MavgPayloadIoSourceText = Get-AgentSurfaceText "engine/runtime_host/win32/src/win32_mavg_payload_io.cpp"
$win32RuntimeHostTestsText = Get-AgentSurfaceText "tests/unit/runtime_host_win32_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgWin32IocpPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-win32-iocp-file-io-worker-v1.md"
$mavgWin32AsyncFileIoPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-win32-async-file-io-adapter-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "Win32MavgPayloadIocpFileIoDispatcherDesc",
        "std::size_t worker_thread_count",
        "Win32MavgPayloadIocpFileIoDispatcher final",
        "runtime::IRuntimeMavgPayloadNativeIoDispatcher",
        "poll_status(std::uint64_t ticket)"
    )) {
    Assert-ContainsText $win32MavgPayloadIoHeaderText $needle "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp IOCP worker public adapter contract"
}

foreach ($forbiddenNeedle in @(
        "HANDLE",
        "OVERLAPPED",
        "windows.h",
        "IDStorage",
        "ID3D12"
    )) {
    Assert-DoesNotContainText $win32MavgPayloadIoHeaderText $forbiddenNeedle "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp must not expose IOCP/native handles"
}

foreach ($needle in @(
        "CreateIoCompletionPort",
        "GetQueuedCompletionStatus",
        "PostQueuedCompletionStatus",
        "CancelIoEx",
        "std::thread",
        "std::condition_variable",
        "try_reserve_submission",
        "cancel_and_release_submission",
        "max_worker_thread_count",
        "executed_background_worker = true",
        "used_win32_async_io = true",
        "RuntimeMavgPayloadNativeIoBackend::win32_async_file"
    )) {
    Assert-ContainsText $win32MavgPayloadIoSourceText $needle "engine/runtime_host/win32/src/win32_mavg_payload_io.cpp Win32 IOCP worker implementation"
}

foreach ($forbiddenNeedle in @(
        "IDStorageFactory",
        "IDStorageQueue",
        "IDStorageStatusArray",
        "ID3D12Fence"
    )) {
    Assert-DoesNotContainText $win32MavgPayloadIoSourceText $forbiddenNeedle "engine/runtime_host/win32/src/win32_mavg_payload_io.cpp must not claim DirectStorage SDK execution"
}

foreach ($needle in @(
        "win32 mavg payload iocp file io worker reads multiple planned byte ranges into destination memory",
        "win32 mavg payload iocp file io worker rejects destination overflow before ticket",
        "win32 mavg payload iocp file io worker releases inflight slot after read failure",
        "win32 mavg payload iocp file io worker keeps concurrent dispatch under inflight limit",
        "Win32MavgPayloadIocpFileIoDispatcher",
        "worker_thread_count = 1U",
        "max_inflight_submissions = 1U",
        "executed_background_worker"
    )) {
    Assert-ContainsText $win32RuntimeHostTestsText $needle "tests/unit/runtime_host_win32_tests.cpp Win32 IOCP worker coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgWin32IocpPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-win32-iocp-file-io-worker-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-win32-iocp-file-io-worker-v1",
            "Win32MavgPayloadIocpFileIoDispatcher",
            "CreateIoCompletionPort",
            "GetQueuedCompletionStatus",
            "PostQueuedCompletionStatus",
            "executed_background_worker",
            "DirectStorage SDK",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Win32 IOCP file IO worker evidence"
    }
}

foreach ($needle in @(
        "Completed/published as draft PR #463",
        "mavg-win32-iocp-file-io-worker-v1",
        "Win32MavgPayloadAsyncFileIoDispatcher"
    )) {
    Assert-ContainsText $mavgWin32AsyncFileIoPlanText $needle "docs/superpowers/plans/2026-06-06-mavg-win32-async-file-io-adapter-v1.md completed retained evidence"
}

foreach ($needle in @(
        "mavg-win32-iocp-file-io-worker-v1",
        "docs/superpowers/plans/2026-06-06-mavg-win32-iocp-file-io-worker-v1.md",
        "MAVG Win32 IOCP File IO Worker v1",
        "Win32MavgPayloadIocpFileIoDispatcher",
        "CreateIoCompletionPort",
        "GetQueuedCompletionStatus",
        "PostQueuedCompletionStatus",
        "executed_background_worker=true",
        "mavg-win32-async-file-io-adapter-v1",
        "draft PR #463",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG Win32 IOCP file IO active pointer"
}

foreach ($needle in @(
        "MAVG Win32 IOCP File IO Worker v1",
        "Win32MavgPayloadIocpFileIoDispatcher",
        "CreateIoCompletionPort",
        "GetQueuedCompletionStatus",
        "PostQueuedCompletionStatus",
        "executed_background_worker=true",
        "no DirectStorage SDK dependency",
        "Nanite compatibility/equivalence/superiority"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MAVG Win32 IOCP file IO module evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if ([string]$productionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-06-mavg-win32-iocp-file-io-worker-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must select MAVG Win32 IOCP File IO Worker v1"
}
if ([string]$productionLoop.recommendedNextPlan.id -ne "mavg-win32-iocp-file-io-worker-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must select mavg-win32-iocp-file-io-worker-v1"
}
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-win32-async-file-io-adapter-v1.md") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan retainedCompletedPlanPaths must retain MAVG Win32 Async File IO Adapter v1"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Win32 IOCP File IO Worker v1",
        "executed_background_worker=true",
        "IRuntimeMavgPayloadNativeIoDispatcher",
        "no dstorage.h",
        "no renderer/RHI handles"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime IOCP worker boundary evidence"
}

$win32RuntimeHostModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_host_win32" })
if ($win32RuntimeHostModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_host_win32 module" }
if (@($win32RuntimeHostModule[0].publicHeaders) -notcontains "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime_host_win32 publicHeaders missing win32_mavg_payload_io.hpp"
}
$win32RuntimeHostManifestText = ((@($win32RuntimeHostModule[0].recentEvidence) -join " "), [string]$win32RuntimeHostModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Win32 IOCP File IO Worker v1",
        "Win32MavgPayloadIocpFileIoDispatcher",
        "CreateIoCompletionPort",
        "GetQueuedCompletionStatus",
        "PostQueuedCompletionStatus",
        "executed_background_worker=true",
        "no DirectStorage SDK dependency",
        "Nanite compatibility/equivalence/superiority"
    )) {
    Assert-ContainsText $win32RuntimeHostManifestText $needle "engine/agent/manifest.json MK_runtime_host_win32 IOCP worker evidence"
}
