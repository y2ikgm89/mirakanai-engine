#requires -Version 7.0
#requires -PSEdition Core
# Chapter 123 for check-ai-integration.ps1 static contracts.

$mavgPageStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
$mavgPageStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_page_streaming.cpp"
$mavgPageStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_page_streaming_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$mavgFrequencyPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-frequency-eviction-policy-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "runtime_inferred_frequency",
        "RuntimeMavgPageStreamingFrequencyRow",
        "resident_page_selection_count",
        "RuntimeMavgResidentPageFrequencyDesc",
        "std::span<const RuntimeMavgPageStreamingFrequencyRow> previous_frequency_rows",
        "RuntimeMavgResidentPageFrequencyResult",
        "inferred_resident_page_frequency",
        "carried_frequency_row_count",
        "dropped_nonresident_frequency_row_count",
        "duplicate_frequency_row_count",
        "missing_frequency_row_count",
        "frequency_counter_overflow",
        "infer_runtime_mavg_resident_page_frequencies"
    )) {
    Assert-ContainsText $mavgPageStreamingHeaderText $needle "mavg_page_streaming.hpp runtime-inferred frequency public contract"
}

foreach ($needle in @(
        "uses_frequency_eviction_order",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency",
        "find_frequency_row",
        "copy_frequency_evidence",
        "infer_runtime_mavg_resident_page_frequencies",
        "result.inferred_frequency_eviction_policy = true",
        "result.runtime_inferred_frequency_eviction_candidate_count = eviction_candidates.size()",
        "RuntimeMavgPageStreamingDiagnosticCode::frequency_counter_overflow",
        "lhs.resident_page_selection_count < rhs.resident_page_selection_count"
    )) {
    Assert-ContainsText $mavgPageStreamingSourceText $needle "mavg_page_streaming.cpp runtime-inferred frequency implementation"
}

foreach ($needle in @(
        "runtime mavg page streaming runtime inferred frequency policy orders least selected unprotected pages first",
        "runtime mavg page streaming runtime inferred frequency policy evicts cold residents first",
        "runtime mavg page streaming runtime inferred frequency policy rejects counter overflow",
        "runtime mavg page streaming frequency inference drops nonresident rows and deduplicates selected pages",
        "runtime mavg page streaming frequency inference rejects duplicate previous rows",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency",
        "inferred_frequency_eviction_policy",
        "runtime_inferred_frequency_eviction_candidate_count",
        "frequency_counter_overflow",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $mavgPageStreamingTestsText $needle "MK_runtime_mavg_page_streaming_tests runtime-inferred frequency coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgFrequencyPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-frequency-eviction-policy-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $productionMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-runtime-inferred-frequency-eviction-policy-v1",
            "MAVG Runtime-Inferred Frequency Eviction Policy v1",
            "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency",
            "RuntimeMavgPageStreamingFrequencyRow",
            "RuntimeMavgResidentPageFrequencyDesc",
            "RuntimeMavgResidentPageFrequencyResult",
            "infer_runtime_mavg_resident_page_frequencies",
            "inferred_frequency_eviction_policy",
            "runtime_inferred_frequency_eviction_candidate_count",
            "frequency_counter_overflow"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) runtime-inferred frequency evidence"
    }
    foreach ($needle in @(
            "GPU memory pressure",
            "DirectStorage",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) runtime-inferred frequency remaining non-claims"
    }
}

foreach ($needle in @(
        "MAVG Runtime-Inferred Frequency Eviction Policy v1",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency",
        "RuntimeMavgPageStreamingFrequencyRow",
        "RuntimeMavgResidentPageFrequencyDesc",
        "RuntimeMavgResidentPageFrequencyResult",
        "infer_runtime_mavg_resident_page_frequencies",
        "inferred_frequency_eviction_policy",
        "inferred_resident_page_frequency",
        "runtime_inferred_frequency_eviction_candidate_count",
        "duplicate_frequency_row_count",
        "missing_frequency_row_count",
        "frequency_counter_overflow"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json runtime-inferred frequency module evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json runtime-inferred frequency plan evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-frequency-eviction-policy-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG Runtime-Inferred Frequency Eviction Policy v1"
}

$recommendedPlanText = (([string]$productionLoop.recommendedNextPlan.retainedCompletedPlanEvidence), ([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG Runtime-Inferred Frequency Eviction Policy v1",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency",
        "RuntimeMavgPageStreamingFrequencyRow",
        "infer_runtime_mavg_resident_page_frequencies",
        "runtime_inferred_frequency_eviction_candidate_count",
        "frequency_counter_overflow",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan runtime-inferred frequency evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Runtime-Inferred Frequency Eviction Policy v1",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency",
        "RuntimeMavgPageStreamingFrequencyRow",
        "RuntimeMavgResidentPageFrequencyDesc",
        "RuntimeMavgResidentPageFrequencyResult",
        "infer_runtime_mavg_resident_page_frequencies",
        "inferred_frequency_eviction_policy",
        "runtime_inferred_frequency_eviction_candidate_count",
        "frequency_counter_overflow"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime runtime-inferred frequency evidence"
}
