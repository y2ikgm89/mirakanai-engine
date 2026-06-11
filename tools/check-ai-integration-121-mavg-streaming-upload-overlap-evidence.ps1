#requires -Version 7.0
#requires -PSEdition Core
# Chapter 121 for check-ai-integration.ps1 MAVG streaming upload overlap evidence.

$mavgOverlapHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_streaming_upload_overlap_evidence.hpp"
$mavgOverlapSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_streaming_upload_overlap_evidence.cpp"
$mavgOverlapTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_streaming_upload_overlap_evidence_tests.cpp"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgOverlapPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-streaming-upload-overlap-evidence-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgStreamingUploadOverlapEvidenceDesc",
        "RuntimeMavgStreamingUploadOverlapEvidenceResult",
        "RuntimeMavgStreamingUploadEvidenceWindow",
        "RuntimeMavgStreamingUploadEvidenceClockDomain",
        "caller_monotonic_tick",
        "background_load_window",
        "gpu_upload_window",
        "recorded_temporal_overlap_evidence",
        "overlap_tick_count",
        "bool claimed_speedup{false};",
        "bool proved_async_overlap_performance{false};",
        "plan_runtime_mavg_streaming_upload_overlap_evidence",
        "has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic"
    )) {
    Assert-ContainsText $mavgOverlapHeaderText $needle "mavg_streaming_upload_overlap_evidence.hpp public contract"
}

foreach ($needle in @(
        "closeout.succeeded()",
        "closeout.background_load.loaded_rows.empty()",
        "adoption.adopted_rows.empty()",
        "upload.page_bindings.empty()",
        "source_row_graph_asset_mismatch",
        "valid_clock_domain",
        "same_clock_domain",
        "window_clock_domain_mismatch",
        "missing_window_row_evidence",
        "result.recorded_temporal_overlap_evidence = true"
    )) {
    Assert-ContainsText $mavgOverlapSourceText $needle "mavg_streaming_upload_overlap_evidence.cpp fail-closed implementation"
}

foreach ($needle in @(
        "runtime rhi mavg streaming upload overlap evidence records caller supplied overlap without broad claims",
        "runtime rhi mavg streaming upload overlap evidence rejects failed closeout even with background rows",
        "runtime rhi mavg streaming upload overlap evidence rejects source row graph mismatch",
        "runtime rhi mavg streaming upload overlap evidence rejects mismatched window clock domains",
        "runtime rhi mavg streaming upload overlap evidence rejects missing window row counters",
        "claimed_speedup",
        "proved_async_overlap_performance"
    )) {
    Assert-ContainsText $mavgOverlapTestsText $needle "MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests coverage"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_streaming_upload_overlap_evidence.cpp" "engine/runtime_rhi/CMakeLists.txt overlap evidence source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests" "root CMake overlap evidence test target"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $productionMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgOverlapPlanText; Label = "docs/superpowers/plans/2026-06-11-mavg-streaming-upload-overlap-evidence-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" }
    )) {
    foreach ($needle in @(
            "mavg-streaming-upload-overlap-evidence-v1",
            "RuntimeMavgStreamingUploadOverlapEvidenceDesc",
            "RuntimeMavgStreamingUploadOverlapEvidenceResult",
            "plan_runtime_mavg_streaming_upload_overlap_evidence",
            "recorded_temporal_overlap_evidence",
            "overlap_tick_count",
            "background_load_window",
            "gpu_upload_window"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) overlap evidence claim"
    }
    foreach ($needle in @(
            "caller-owned timing windows",
            "claimed_speedup=false",
            "proved_async_overlap_performance=false",
            "DirectStorage",
            "backend execution",
            "mesh shaders",
            "native handles",
            "Nanite",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) overlap evidence non-claims"
    }
}

$recommendedPlanText = ($manifest.aiOperableProductionLoop.recommendedNextPlan | ConvertTo-Json -Depth 8)
foreach ($needle in @(
        "MAVG Streaming Upload Overlap Evidence v1",
        "mavg_streaming_upload_overlap_evidence.hpp",
        "RuntimeMavgStreamingUploadOverlapEvidenceDesc",
        "RuntimeMavgStreamingUploadOverlapEvidenceResult",
        "plan_runtime_mavg_streaming_upload_overlap_evidence",
        "recorded_temporal_overlap_evidence",
        "overlap_tick_count",
        "caller-owned timing windows",
        "claimed_speedup=false",
        "proved_async_overlap_performance=false"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan overlap evidence"
}

$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module" }
$runtimeRhiManifestText = ((@($runtimeRhiModule[0].publicHeaders) -join " "),
    (@($runtimeRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_streaming_upload_overlap_evidence.hpp",
        "RuntimeMavgStreamingUploadOverlapEvidenceDesc",
        "RuntimeMavgStreamingUploadOverlapEvidenceResult",
        "plan_runtime_mavg_streaming_upload_overlap_evidence",
        "recorded_temporal_overlap_evidence",
        "overlap_tick_count",
        "claimed_speedup=false",
        "proved_async_overlap_performance=false"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi overlap evidence"
}
