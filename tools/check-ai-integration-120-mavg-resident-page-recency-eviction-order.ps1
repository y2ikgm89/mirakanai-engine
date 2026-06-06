#requires -Version 7.0
#requires -PSEdition Core
# Chapter 120 for check-ai-integration.ps1 static contracts.

$mavgPageStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
$mavgPageStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_page_streaming.cpp"
$mavgPageStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_page_streaming_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$mavgRecencyPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-resident-page-recency-eviction-order-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind",
        "caller_supplied_recency",
        "RuntimeMavgPageStreamingRecencyRow",
        "resident_page_last_used_generation",
        "std::span<const RuntimeMavgPageStreamingRecencyRow> recency_rows",
        "recency_eviction_candidate_count",
        "missing_recency_row_count",
        "duplicate_recency_row_count",
        "applied_caller_supplied_recency_policy",
        "recency_graph_mismatch",
        "invalid_recency_row",
        "duplicate_recency_row",
        "missing_recency_row"
    )) {
    Assert-ContainsText $mavgPageStreamingHeaderText $needle "mavg_page_streaming.hpp recency eviction public contract"
}

foreach ($needle in @(
        "validate_recency_rows",
        "find_recency_row",
        "matches_page_mount",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency",
        "candidate.resident_page_last_used_generation = recency->resident_page_last_used_generation",
        "lhs.resident_page_last_used_generation < rhs.resident_page_last_used_generation",
        "RuntimeMavgPageStreamingDiagnosticCode::duplicate_recency_row",
        "RuntimeMavgPageStreamingDiagnosticCode::missing_recency_row"
    )) {
    Assert-ContainsText $mavgPageStreamingSourceText $needle "mavg_page_streaming.cpp recency eviction implementation"
}

foreach ($needle in @(
        "runtime mavg page streaming caller supplied recency orders older resident pages first",
        "runtime mavg page streaming caller supplied recency rejects duplicate rows before planning",
        "runtime mavg page streaming caller supplied recency requires one row per eviction candidate",
        "applied_caller_supplied_recency_policy",
        "duplicate_recency_row_count",
        "missing_recency_row_count",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $mavgPageStreamingTestsText $needle "MK_runtime_mavg_page_streaming_tests recency eviction coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgRecencyPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-resident-page-recency-eviction-order-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $productionMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-resident-page-recency-eviction-order-v1",
            "caller-supplied recency",
            "RuntimeMavgPageStreamingRecencyRow",
            "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency",
            "resident_page_last_used_generation"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) recency eviction evidence"
    }
    foreach ($needle in @(
            "runtime-inferred frequency",
            "GPU memory pressure",
            "DirectStorage",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) recency eviction non-claims"
    }
}

foreach ($needle in @(
        "MAVG Resident Page Recency Eviction Order v1",
        "RuntimeMavgPageStreamingRecencyRow",
        "caller_supplied_recency",
        "resident_page_last_used_generation",
        "duplicate_recency_row_count",
        "missing_recency_row_count",
        "runtime-inferred frequency eviction policy was out of scope for that child"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json recency eviction module evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json recency eviction plan evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-resident-page-recency-eviction-order-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG Resident Page Recency Eviction Order v1"
}

$recommendedPlanText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG Resident Page Recency Eviction Order v1",
        "RuntimeMavgPageStreamingRecencyRow",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency",
        "resident_page_last_used_generation",
        "duplicate_recency_row_count",
        "missing_recency_row_count",
        "MAVG Runtime-Inferred Frequency Eviction Policy v1",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan recency eviction evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Resident Page Recency Eviction Order v1",
        "RuntimeMavgPageStreamingRecencyRow",
        "caller_supplied_recency",
        "resident_page_last_used_generation",
        "duplicate_recency_row",
        "missing_recency_row",
        "runtime-inferred frequency eviction policy was out of scope for that child"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime recency eviction evidence"
}
