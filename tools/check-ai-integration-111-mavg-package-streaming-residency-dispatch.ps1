#requires -Version 7.0
#requires -PSEdition Core
# Chapter 111 for check-ai-integration.ps1 static contracts.

$runtimeMavgPageStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
$runtimeMavgPageStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_page_streaming.cpp"
$runtimeMavgPageStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_page_streaming_tests.cpp"
$mavgDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-package-streaming-residency-dispatch-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "RuntimeMavgPageStreamingDispatchMode",
        "RuntimeMavgPageStreamingDispatchDesc",
        "RuntimeMavgPageStreamingDispatchRow",
        "RuntimeMavgPageStreamingDispatchPlan",
        "caller_owned_safe_point",
        "caller_owned_background_queue",
        "plan_runtime_mavg_page_streaming_dispatches",
        "invalid_dispatch_row",
        "missing_dispatch_mount_ids",
        "dispatch_mount_id_count_mismatch",
        "invalid_dispatch_mount_id",
        "duplicate_dispatch_mount_id",
        "unsafe_dispatch_mode"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingHeaderText $needle "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp MAVG dispatch public API"
}

foreach ($needle in @(
        "valid_dispatch_mode",
        "valid_dispatch_row",
        "max_dispatch_rows",
        "RuntimeMavgPageStreamingDrainDesc",
        "background_worker_owned_by_caller",
        "executed_background_worker"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingSourceText $needle "engine/runtime/src/mavg_page_streaming.cpp MAVG dispatch implementation"
}

foreach ($needle in @(
        "runtime mavg page streaming dispatch planner builds safe point rows deterministically",
        "runtime mavg page streaming dispatch planner applies deterministic max dispatch budget",
        "runtime mavg page streaming dispatch planner records caller owned background queue without executing worker",
        "runtime mavg page streaming dispatch planner rejects invalid dispatch rows before side effects",
        "runtime mavg page streaming dispatch planner rejects unsafe no safe point mutation boundary"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingTestsText $needle "tests/unit/runtime_mavg_page_streaming_tests.cpp MAVG dispatch coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgDispatchPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-package-streaming-residency-dispatch-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-package-streaming-residency-dispatch-v1",
            "RuntimeMavgPageStreamingDispatchPlan",
            "plan_runtime_mavg_page_streaming_dispatches",
            "RuntimeMavgPageStreamingDrainDesc",
            "caller-owned",
            "safe-point",
            "background"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG package streaming residency dispatch evidence"
    }
    foreach ($needle in @(
            "autonomous background",
            "async",
            "partial",
            "GPU memory pressure",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG package streaming residency dispatch non-claims"
    }
    if ($surface.Label -eq "docs/superpowers/plans/2026-06-05-mavg-package-streaming-residency-dispatch-v1.md") {
        Assert-ContainsText $surface.Text "automatic eviction policy" "$($surface.Label) MAVG package streaming residency dispatch historical non-claims"
    } else {
        Assert-ContainsText $surface.Text "runtime-inferred LRU/frequency" "$($surface.Label) MAVG package streaming residency dispatch remaining eviction non-claims"
    }
}

foreach ($needle in @(
        "mavg-package-streaming-residency-dispatch-v1",
        "docs/superpowers/plans/2026-06-05-mavg-package-streaming-residency-dispatch-v1.md",
        "MAVG Package Streaming Residency Dispatch v1",
        "RuntimeMavgPageStreamingDispatchPlan",
        "plan_runtime_mavg_page_streaming_dispatches",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG dispatch active pointer"
}

foreach ($needle in @(
        "MAVG Package Streaming Residency Dispatch v1",
        "RuntimeMavgPageStreamingDispatchPlan",
        "plan_runtime_mavg_page_streaming_dispatches",
        "RuntimeMavgPageStreamingDrainDesc",
        "autonomous background worker execution",
        "DirectStorage/Win32 IO integration"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_runtime MAVG dispatch evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Package Streaming Residency Dispatch v1",
        "RuntimeMavgPageStreamingDispatchPlan",
        "plan_runtime_mavg_page_streaming_dispatches",
        "RuntimeMavgPageStreamingDrainDesc",
        "max_dispatch_rows",
        "autonomous background worker execution",
        "DirectStorage/Win32 IO integration"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime MAVG dispatch evidence"
}
