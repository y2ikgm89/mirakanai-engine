#requires -Version 7.0
#requires -PSEdition Core
# Chapter 116 for check-ai-integration.ps1 static contracts.

$runtimeMavgPayloadPagesHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp"
$runtimeMavgPayloadPagesSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_payload_pages.cpp"
$runtimeMavgPayloadPagesTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_payload_pages_tests.cpp"
$win32MavgPayloadIoHeaderText = Get-AgentSurfaceText "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp"
$win32MavgPayloadIoSourceText = Get-AgentSurfaceText "engine/runtime_host/win32/src/win32_mavg_payload_io.cpp"
$win32RuntimeHostTestsText = Get-AgentSurfaceText "tests/unit/runtime_host_win32_tests.cpp"
$win32RuntimeHostCmakeText = Get-AgentSurfaceText "engine/runtime_host/win32/CMakeLists.txt"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgWin32AsyncFileIoPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-win32-async-file-io-adapter-v1.md"
$mavgNativeIoStatusPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-native-directstorage-win32-async-io-dispatch-status-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "win32_async_file",
        "std::span<std::uint8_t> destination_memory",
        "std::string source_file_path",
        "used_win32_async_io"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesHeaderText $needle "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp Win32 async file IO contract"
}

foreach ($needle in @(
        ".source_file_path = std::string(desc.payload_blob_path)",
        ".destination_memory = desc.destination_memory",
        "backend_result.used_win32_async_io"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesSourceText $needle "engine/runtime/src/mavg_payload_pages.cpp Win32 async file IO contract forwarding"
}

foreach ($needle in @(
        "last_destination_memory_bytes",
        "destination_memory = destination_memory",
        'source_file_path == "runtime/mavg/runtime-page-addressable.pages"',
        "dispatcher.last_destination_memory_bytes == destination_memory.size()"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesTestsText $needle "tests/unit/runtime_mavg_payload_pages_tests.cpp Win32 async file IO runtime contract coverage"
}

foreach ($needle in @(
        "Win32MavgPayloadAsyncFileIoDispatcherDesc",
        "Win32MavgPayloadAsyncFileIoDispatcher final",
        "runtime::IRuntimeMavgPayloadNativeIoDispatcher",
        "poll_status(std::uint64_t ticket)"
    )) {
    Assert-ContainsText $win32MavgPayloadIoHeaderText $needle "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp public adapter contract"
}

foreach ($forbiddenNeedle in @(
        "HANDLE",
        "OVERLAPPED",
        "windows.h",
        "IDStorage",
        "ID3D12"
    )) {
    Assert-DoesNotContainText $win32MavgPayloadIoHeaderText $forbiddenNeedle "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp must not expose native handles"
}

foreach ($needle in @(
        "#include <windows.h>",
        "CreateFileW",
        "FILE_FLAG_OVERLAPPED",
        "OVERLAPPED",
        "CreateEventW",
        "ReadFile",
        "GetOverlappedResult",
        "ERROR_IO_PENDING",
        "ERROR_IO_INCOMPLETE",
        "CancelIoEx",
        "destination_memory",
        "source_file_path",
        "used_win32_async_io = true",
        "RuntimeMavgPayloadNativeIoBackend::win32_async_file"
    )) {
    Assert-ContainsText $win32MavgPayloadIoSourceText $needle "engine/runtime_host/win32/src/win32_mavg_payload_io.cpp Win32 overlapped implementation"
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
        "win32 mavg payload async file io dispatcher reads planned byte ranges into destination memory",
        "win32 mavg payload async file io dispatcher rejects destination overflow before ticket",
        "Win32MavgPayloadAsyncFileIoDispatcher",
        "make_single_win32_payload_request_plan",
        "poll_until_win32_payload_io_complete",
        "destination_memory = destination",
        "used_win32_async_io"
    )) {
    Assert-ContainsText $win32RuntimeHostTestsText $needle "tests/unit/runtime_host_win32_tests.cpp Win32 MAVG adapter coverage"
}

foreach ($needle in @(
        "src/win32_mavg_payload_io.cpp",
        "MK_runtime",
        "MK_runtime_host",
        "MK_platform_win32"
    )) {
    Assert-ContainsText $win32RuntimeHostCmakeText $needle "engine/runtime_host/win32/CMakeLists.txt Win32 MAVG adapter target wiring"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgNativeIoStatusPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-native-directstorage-win32-async-io-dispatch-status-v1.md" },
        @{ Text = $mavgWin32AsyncFileIoPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-win32-async-file-io-adapter-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-win32-async-file-io-adapter-v1",
            "Win32MavgPayloadAsyncFileIoDispatcher",
            "DirectStorage SDK",
            "async-overlap/performance",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Win32 async file IO adapter evidence"
    }
}

foreach ($needle in @(
        "mavg-win32-async-file-io-adapter-v1",
        "docs/superpowers/plans/2026-06-06-mavg-win32-async-file-io-adapter-v1.md",
        "MAVG Win32 Async File IO Adapter v1",
        "RuntimeMavgPayloadDirectStorageRequestRow::source_file_path",
        "destination_memory",
        "Win32MavgPayloadAsyncFileIoDispatcher",
        "CreateFileW",
        "ReadFile",
        "OVERLAPPED",
        "GetOverlappedResult",
        "CancelIoEx",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG Win32 async file IO active pointer"
}

foreach ($needle in @(
        "MAVG Win32 Async File IO Adapter v1",
        "RuntimeMavgPayloadDirectStorageRequestRow::source_file_path",
        "destination_memory",
        "Win32MavgPayloadAsyncFileIoDispatcher",
        "CreateFileW",
        "FILE_FLAG_OVERLAPPED",
        "OVERLAPPED",
        "ReadFile",
        "GetOverlappedResult",
        "CancelIoEx",
        "no DirectStorage SDK",
        "no async-overlap/performance claim"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MAVG Win32 async file IO module evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if ([string]$productionLoop.currentActivePlan -eq "docs/superpowers/plans/2026-06-06-mavg-win32-async-file-io-adapter-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must not keep completed MAVG Win32 Async File IO Adapter v1 active"
}
if ([string]$productionLoop.recommendedNextPlan.id -eq "mavg-win32-async-file-io-adapter-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must not keep completed mavg-win32-async-file-io-adapter-v1 active"
}
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-native-directstorage-win32-async-io-dispatch-status-v1.md") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan retainedCompletedPlanPaths must retain MAVG Native DirectStorage/Win32 Async IO Dispatch Status v1"
}
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-win32-async-file-io-adapter-v1.md") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan retainedCompletedPlanPaths must retain completed MAVG Win32 Async File IO Adapter v1"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
if (@($runtimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders missing mavg_payload_pages.hpp"
}
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Win32 Async File IO Adapter v1",
        "source_file_path",
        "destination_memory",
        "RuntimeMavgPayloadNativeIoDispatchDesc",
        "RuntimeMavgPayloadNativeIoDispatchBackendDesc"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime Win32 async file IO evidence"
}

$win32RuntimeHostModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_host_win32" })
if ($win32RuntimeHostModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_host_win32 module" }
if (@($win32RuntimeHostModule[0].dependencies) -notcontains "MK_runtime") {
    Write-Error "engine/agent/manifest.json MK_runtime_host_win32 dependencies missing MK_runtime"
}
if (@($win32RuntimeHostModule[0].publicHeaders) -notcontains "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime_host_win32 publicHeaders missing win32_mavg_payload_io.hpp"
}
$win32RuntimeHostManifestText = ((@($win32RuntimeHostModule[0].recentEvidence) -join " "), [string]$win32RuntimeHostModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Win32 Async File IO Adapter v1",
        "Win32MavgPayloadAsyncFileIoDispatcher",
        "CreateFileW",
        "FILE_FLAG_OVERLAPPED",
        "OVERLAPPED",
        "ReadFile",
        "GetOverlappedResult",
        "CancelIoEx",
        "no DirectStorage SDK",
        "no async-overlap/performance claim"
    )) {
    Assert-ContainsText $win32RuntimeHostManifestText $needle "engine/agent/manifest.json MK_runtime_host_win32 Win32 async file IO evidence"
}
