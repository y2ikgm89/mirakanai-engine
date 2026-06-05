#requires -Version 7.0
#requires -PSEdition Core
# Chapter 114 for check-ai-integration.ps1 static contracts.

$runtimeMavgPayloadPagesHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp"
$runtimeMavgPayloadPagesSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_payload_pages.cpp"
$runtimeMavgPayloadPagesTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_payload_pages_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgDirectStorageRequestPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-directstorage-request-plan-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode",
        "RuntimeMavgPayloadDirectStorageFenceWaitPoint",
        "RuntimeMavgPayloadDirectStorageRequestPlanDesc",
        "RuntimeMavgPayloadDirectStorageRequestRow",
        "RuntimeMavgPayloadDirectStorageRequestPlanResult",
        "requires_native_directstorage_sdk",
        "enqueued_native_requests",
        "submitted_native_queue",
        "signaled_native_fence",
        "plan_runtime_mavg_payload_directstorage_requests"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesHeaderText $needle "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp DirectStorage request-plan API"
}

foreach ($needle in @(
        "validate_mavg_cluster_graph",
        "deserialize_mavg_cluster_payload_document",
        "validate_mavg_cluster_payload",
        "duplicate_requested_page",
        "unknown_page",
        "missing_payload_page",
        "fits_directstorage_request_size",
        "destination_range_overflow",
        'debug_name = "mavg.payload.page."',
        "requires_native_directstorage_sdk = true"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesSourceText $needle "engine/runtime/src/mavg_payload_pages.cpp DirectStorage request-plan implementation"
}

foreach ($needle in @(
        "runtime mavg payload directstorage requests plan selected page ranges without executing io",
        "runtime mavg payload directstorage request planning rejects duplicate pages before io",
        "runtime mavg payload directstorage request planning rejects overflowing destination ranges",
        "RuntimeMavgPayloadDirectStorageRequestPlanDesc",
        "before_source_access",
        "requires_native_directstorage_sdk",
        "destination_range_overflow"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesTestsText $needle "tests/unit/runtime_mavg_payload_pages_tests.cpp DirectStorage request-plan coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgDirectStorageRequestPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-directstorage-request-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-directstorage-request-plan-v1",
            "RuntimeMavgPayloadDirectStorageRequestPlanDesc",
            "RuntimeMavgPayloadDirectStorageRequestRow",
            "RuntimeMavgPayloadDirectStorageRequestPlanResult",
            "RuntimeMavgPayloadDirectStorageFenceWaitPoint",
            "plan_runtime_mavg_payload_directstorage_requests",
            "requires_native_directstorage_sdk=true",
            "used_native_directstorage=false",
            "async-overlap/performance",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG DirectStorage request-plan evidence"
    }
}

foreach ($needle in @(
        "mavg-directstorage-request-plan-v1",
        "docs/superpowers/plans/2026-06-05-mavg-directstorage-request-plan-v1.md",
        "MAVG DirectStorage Request Plan v1",
        "RuntimeMavgPayloadDirectStorageRequestPlanDesc",
        "RuntimeMavgPayloadDirectStorageRequestRow",
        "RuntimeMavgPayloadDirectStorageRequestPlanResult",
        "RuntimeMavgPayloadDirectStorageFenceWaitPoint",
        "plan_runtime_mavg_payload_directstorage_requests",
        "requires_native_directstorage_sdk=true",
        "used_native_directstorage=false",
        "native DirectStorage queues/fences/status arrays",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG DirectStorage request-plan active pointer"
}

foreach ($needle in @(
        "MAVG DirectStorage Request Plan v1",
        "RuntimeMavgPayloadDirectStorageRequestPlanDesc",
        "RuntimeMavgPayloadDirectStorageRequestRow",
        "RuntimeMavgPayloadDirectStorageRequestPlanResult",
        "RuntimeMavgPayloadDirectStorageFenceWaitPoint",
        "plan_runtime_mavg_payload_directstorage_requests",
        "requires_native_directstorage_sdk=true",
        "used_native_directstorage=false",
        "no native enqueue",
        "no native submit",
        "no native fence signal",
        "no native DirectStorage queues/fences/status arrays"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MAVG DirectStorage request-plan module evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
if (@($runtimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders missing mavg_payload_pages.hpp"
}
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG DirectStorage Request Plan v1",
        "RuntimeMavgPayloadDirectStorageRequestPlanDesc",
        "RuntimeMavgPayloadDirectStorageRequestRow",
        "RuntimeMavgPayloadDirectStorageRequestPlanResult",
        "RuntimeMavgPayloadDirectStorageFenceWaitPoint",
        "plan_runtime_mavg_payload_directstorage_requests",
        "requires_native_directstorage_sdk=true",
        "used_native_directstorage=false",
        "no native enqueue",
        "no native submit",
        "no native fence signal",
        "no native DirectStorage queues/fences/status arrays"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime DirectStorage request-plan evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-05-mavg-directstorage-request-plan-v1.md") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan retainedCompletedPlanPaths must retain MAVG DirectStorage Request Plan v1"
}
if (-not ([string]$productionLoop.recommendedNextPlan.retainedCompletedPlanEvidence).Contains("MAVG DirectStorage Request Plan v1")) {
    Write-Error "engine/agent/manifest.json recommendedNextPlan retainedCompletedPlanEvidence must retain MAVG DirectStorage Request Plan v1"
}
