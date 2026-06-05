#requires -Version 7.0
#requires -PSEdition Core
# Chapter 115 for check-ai-integration.ps1 static contracts.

$runtimeMavgPayloadPagesHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp"
$runtimeMavgPayloadPagesSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_payload_pages.cpp"
$runtimeMavgPayloadPagesTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_payload_pages_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgNativeIoStatusPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-native-directstorage-win32-async-io-dispatch-status-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "RuntimeMavgPayloadNativeIoBackend",
        "RuntimeMavgPayloadNativeIoStatus",
        "RuntimeMavgPayloadNativeIoDiagnosticCode",
        "RuntimeMavgPayloadNativeIoDiagnostic",
        "RuntimeMavgPayloadNativeIoDispatchBackendDesc",
        "RuntimeMavgPayloadNativeIoDispatchBackendResult",
        "RuntimeMavgPayloadNativeIoStatusBackendResult",
        "IRuntimeMavgPayloadNativeIoDispatcher",
        "RuntimeMavgPayloadNativeIoDispatchDesc",
        "RuntimeMavgPayloadNativeIoDispatchResult",
        "RuntimeMavgPayloadNativeIoStatusPollDesc",
        "RuntimeMavgPayloadNativeIoStatusPollResult",
        "dispatch_runtime_mavg_payload_native_io_requests",
        "poll_runtime_mavg_payload_native_io_status",
        "enqueued_native_requests",
        "submitted_native_queue",
        "used_win32_async_io"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesHeaderText $needle "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp native IO status API"
}

foreach ($needle in @(
        "missing_dispatcher",
        "invalid_request_plan",
        "unsupported_backend",
        "dispatch_failed",
        "missing_ticket",
        "status_failed",
        "desc.dispatcher->dispatch",
        "desc.dispatcher->poll_status",
        "require_native_directstorage",
        "mutated_mount_set = false",
        "executed_background_worker = false",
        "touched_renderer_or_rhi_handles = false"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesSourceText $needle "engine/runtime/src/mavg_payload_pages.cpp native IO status implementation"
}

foreach ($needle in @(
        "RecordingNativeIoDispatcher",
        "runtime mavg payload native io dispatch submits request plan through caller adapter",
        "runtime mavg payload native io dispatch rejects invalid inputs before adapter calls",
        "runtime mavg payload native io status polling reports submitted then complete",
        "runtime mavg payload native io status polling reports backend failures",
        "RuntimeMavgPayloadNativeIoBackend::test_adapter",
        "dispatch_runtime_mavg_payload_native_io_requests",
        "poll_runtime_mavg_payload_native_io_status",
        "status_failed"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesTestsText $needle "tests/unit/runtime_mavg_payload_pages_tests.cpp native IO status coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgNativeIoStatusPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-native-directstorage-win32-async-io-dispatch-status-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-native-directstorage-win32-async-io-dispatch-status-v1",
            "RuntimeMavgPayloadNativeIoBackend",
            "IRuntimeMavgPayloadNativeIoDispatcher",
            "RuntimeMavgPayloadNativeIoDispatchResult",
            "RuntimeMavgPayloadNativeIoStatusPollResult",
            "dispatch_runtime_mavg_payload_native_io_requests",
            "poll_runtime_mavg_payload_native_io_status",
            "dstorage.h",
            "DirectStorage SDK",
            "async-overlap/performance",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG native IO status evidence"
    }
}

foreach ($needle in @(
        "mavg-native-directstorage-win32-async-io-dispatch-status-v1",
        "docs/superpowers/plans/2026-06-06-mavg-native-directstorage-win32-async-io-dispatch-status-v1.md",
        "MAVG Native DirectStorage/Win32 Async IO Dispatch Status v1",
        "RuntimeMavgPayloadNativeIoBackend",
        "RuntimeMavgPayloadNativeIoStatus",
        "RuntimeMavgPayloadNativeIoDiagnostic",
        "IRuntimeMavgPayloadNativeIoDispatcher",
        "RuntimeMavgPayloadNativeIoDispatchResult",
        "RuntimeMavgPayloadNativeIoStatusPollResult",
        "dispatch_runtime_mavg_payload_native_io_requests",
        "poll_runtime_mavg_payload_native_io_status",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG native IO status active pointer"
}

foreach ($needle in @(
        "MAVG Native DirectStorage/Win32 Async IO Dispatch Status v1",
        "RuntimeMavgPayloadNativeIoBackend",
        "RuntimeMavgPayloadNativeIoStatus",
        "RuntimeMavgPayloadNativeIoDiagnostic",
        "IRuntimeMavgPayloadNativeIoDispatcher",
        "RuntimeMavgPayloadNativeIoDispatchResult",
        "RuntimeMavgPayloadNativeIoStatusPollResult",
        "dispatch_runtime_mavg_payload_native_io_requests",
        "poll_runtime_mavg_payload_native_io_status",
        "no dstorage.h",
        "no async-overlap/performance claim"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MAVG native IO status module evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
if (@($runtimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders missing mavg_payload_pages.hpp"
}
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Native DirectStorage/Win32 Async IO Dispatch Status v1",
        "RuntimeMavgPayloadNativeIoBackend",
        "RuntimeMavgPayloadNativeIoStatus",
        "RuntimeMavgPayloadNativeIoDiagnostic",
        "IRuntimeMavgPayloadNativeIoDispatcher",
        "RuntimeMavgPayloadNativeIoDispatchResult",
        "RuntimeMavgPayloadNativeIoStatusPollResult",
        "dispatch_runtime_mavg_payload_native_io_requests",
        "poll_runtime_mavg_payload_native_io_status",
        "no dstorage.h",
        "no async-overlap/performance claim"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime native IO status evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if ([string]$productionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-06-mavg-native-directstorage-win32-async-io-dispatch-status-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must select MAVG Native DirectStorage/Win32 Async IO Dispatch Status v1"
}
if ([string]$productionLoop.recommendedNextPlan.id -ne "mavg-native-directstorage-win32-async-io-dispatch-status-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must select mavg-native-directstorage-win32-async-io-dispatch-status-v1"
}
