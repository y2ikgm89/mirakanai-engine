#requires -Version 7.0
#requires -PSEdition Core
# Chapter 119 for check-ai-integration.ps1 static contracts.

$mavgPageStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
$mavgPageStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_page_streaming.cpp"
$mavgPageStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_page_streaming_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgAutonomousWorkerPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-autonomous-page-streaming-worker-v1.md"
$mavgDirectStorageSdkPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-directstorage-sdk-dependency-gate-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgPageStreamingDispatchMode",
        "engine_owned_background_worker",
        "RuntimeMavgPageStreamingWorkerDesc",
        "RuntimeMavgPageStreamingWorkerResult",
        "execute_runtime_mavg_page_streaming_worker",
        "max_worker_rows",
        "executed_background_worker",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $mavgPageStreamingHeaderText $needle "mavg_page_streaming.hpp autonomous worker public contract"
}

foreach ($needle in @(
        "#include <thread>",
        "std::thread worker",
        "worker.join()",
        "RuntimeMavgPageStreamingDiagnosticCode::unsupported_worker_dispatch_mode",
        "execute_runtime_mavg_page_streaming_request_safe_point",
        "drain.executed_background_worker = true",
        "result.budget_degraded",
        "result.budget_dropped_row_count"
    )) {
    Assert-ContainsText $mavgPageStreamingSourceText $needle "mavg_page_streaming.cpp autonomous worker execution"
}

foreach ($needle in @(
        "runtime mavg page streaming dispatch planner records engine owned background worker without executing it",
        "runtime mavg page streaming worker executes engine owned dispatch rows",
        "runtime mavg page streaming worker rejects caller owned dispatch plans before mutation",
        "runtime mavg page streaming worker applies deterministic max row budget",
        "find_runtime_resource_v2",
        "unsupported_worker_dispatch_mode",
        "max_worker_rows"
    )) {
    Assert-ContainsText $mavgPageStreamingTestsText $needle "MK_runtime_mavg_page_streaming_tests autonomous worker coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $masterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgAutonomousWorkerPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-autonomous-page-streaming-worker-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-autonomous-page-streaming-worker-v1",
            "MAVG Autonomous Page Streaming Worker v1",
            "engine_owned_background_worker",
            "RuntimeMavgPageStreamingWorkerResult",
            "execute_runtime_mavg_page_streaming_worker",
            "DirectStorage",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG autonomous worker evidence"
    }
}

foreach ($needle in @(
        "Completed/published as draft PR #469",
        "tools/validate-directstorage-sdk.ps1",
        "bootstrap-deps.ps1",
        "dstorage"
    )) {
    Assert-ContainsText $mavgDirectStorageSdkPlanText $needle "DirectStorage SDK gate retained completed evidence"
}

foreach ($needle in @(
        "MAVG Autonomous Page Streaming Worker v1",
        "RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker",
        "RuntimeMavgPageStreamingWorkerDesc",
        "RuntimeMavgPageStreamingWorkerResult",
        "execute_runtime_mavg_page_streaming_worker",
        "no DirectStorage file IO",
        "no automatic eviction policy",
        "no GPU memory pressure integration",
        "no renderer/RHI handles",
        "no async-overlap/performance claim",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json autonomous worker module evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json autonomous worker active pointer"
}

$productionLoop = $manifest.aiOperableProductionLoop
if ([string]$productionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-06-mavg-autonomous-page-streaming-worker-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must select MAVG Autonomous Page Streaming Worker v1"
}
if ([string]$productionLoop.recommendedNextPlan.id -ne "mavg-autonomous-page-streaming-worker-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must select mavg-autonomous-page-streaming-worker-v1"
}
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-directstorage-sdk-dependency-gate-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG DirectStorage SDK Dependency Gate v1"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Autonomous Page Streaming Worker v1",
        "RuntimeMavgPageStreamingWorkerResult",
        "execute_runtime_mavg_page_streaming_worker",
        "executed_background_worker=true",
        "no DirectStorage file IO",
        "no automatic eviction policy",
        "no renderer/RHI handles",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime autonomous worker evidence"
}
