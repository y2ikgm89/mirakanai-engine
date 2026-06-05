#requires -Version 7.0
#requires -PSEdition Core

# Chapter 10.4 for check-ai-integration.ps1 MAVG runtime LOD follow-up evidence.

$runtimeMavgPageStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
$runtimeMavgPageStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_page_streaming.cpp"
$runtimeMavgPageStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_page_streaming_tests.cpp"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgPageStreamingPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-page-streaming-queue-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$loopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RuntimeMavgPageStreamingCandidateRow",
        "RuntimeMavgPageStreamingPlanResult",
        "RuntimeMavgPageStreamingDrainResult",
        "plan_runtime_mavg_page_streaming_requests",
        "execute_runtime_mavg_page_streaming_request_safe_point",
        "executed_background_worker",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingHeaderText $needle "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
}

foreach ($needle in @(
        "commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2",
        "max_queued_pages",
        "missing_candidate",
        "safe_point_failed"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingSourceText $needle "engine/runtime/src/mavg_page_streaming.cpp"
}

foreach ($needle in @(
        "runtime mavg page streaming planner coalesces nonresident requests deterministically",
        "runtime mavg page streaming planner applies deterministic max page budget",
        "runtime mavg page streaming executes one queued row through reviewed safe point",
        "runtime mavg page streaming drain rejects invalid mount id before mutation"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingTestsText $needle "tests/unit/runtime_mavg_page_streaming_tests.cpp"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgPageStreamingPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-page-streaming-queue-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $loopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" }
    )) {
    foreach ($needle in @(
            "mavg_page_streaming.hpp",
            "RuntimeMavgPageStreamingPlanResult",
            "RuntimeMavgPageStreamingDrainResult",
            "plan_runtime_mavg_page_streaming_requests",
            "execute_runtime_mavg_page_streaming_request_safe_point",
            "autonomous"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG page streaming queue evidence"
    }
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
if (@($runtimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders missing mavg_page_streaming.hpp"
}

$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Page Streaming Queue v1",
        "RuntimeMavgPageStreamingCandidateRow",
        "RuntimeMavgPageStreamingPlanResult",
        "RuntimeMavgPageStreamingDrainResult",
        "plan_runtime_mavg_page_streaming_requests",
        "execute_runtime_mavg_page_streaming_request_safe_point",
        "autonomous background"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime MAVG page streaming queue evidence"
}
