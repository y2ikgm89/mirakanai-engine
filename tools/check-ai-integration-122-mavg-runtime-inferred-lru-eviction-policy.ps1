#requires -Version 7.0
#requires -PSEdition Core
# Chapter 122 for check-ai-integration.ps1 static contracts.

$mavgPageStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
$mavgPageStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_page_streaming.cpp"
$mavgPageStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_page_streaming_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$mavgUseGenerationPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-page-use-generation-v1.md"
$mavgRuntimeInferredLruPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-lru-eviction-policy-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind",
        "runtime_inferred_lru",
        "std::span<const RuntimeMavgPageStreamingRecencyRow> previous_recency_rows",
        "current_use_generation",
        "inferred_eviction_policy",
        "inferred_lru_eviction_policy",
        "inferred_resident_page_use_generation",
        "runtime_inferred_lru_eviction_candidate_count",
        "touched_resident_page_count",
        "carried_recency_row_count",
        "new_resident_page_count",
        "dropped_nonresident_recency_row_count"
    )) {
    Assert-ContainsText $mavgPageStreamingHeaderText $needle "mavg_page_streaming.hpp runtime-inferred LRU public contract"
}

foreach ($needle in @(
        "uses_recency_eviction_order",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru",
        "infer_runtime_mavg_resident_page_use_generations",
        "copy_use_generation_evidence",
        "result.inferred_lru_eviction_policy = true",
        "result.runtime_inferred_lru_eviction_candidate_count = eviction_candidates.size()"
    )) {
    Assert-ContainsText $mavgPageStreamingSourceText $needle "mavg_page_streaming.cpp runtime-inferred LRU implementation"
}

foreach ($needle in @(
        "runtime mavg page streaming runtime inferred lru policy orders older unprotected pages first",
        "runtime mavg page streaming runtime inferred lru policy evicts cold residents first",
        "runtime mavg page streaming runtime inferred lru policy rejects nonmonotonic generations",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru",
        "inferred_lru_eviction_policy",
        "runtime_inferred_lru_eviction_candidate_count",
        "non_monotonic_use_generation",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $mavgPageStreamingTestsText $needle "MK_runtime_mavg_page_streaming_tests runtime-inferred LRU coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgUseGenerationPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-page-use-generation-v1.md" },
        @{ Text = $mavgRuntimeInferredLruPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-lru-eviction-policy-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $productionMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-runtime-inferred-lru-eviction-policy-v1",
            "MAVG Runtime-Inferred LRU Eviction Policy v1",
            "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru",
            "inferred_lru_eviction_policy",
            "runtime_inferred_lru_eviction_candidate_count"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) runtime-inferred LRU evidence"
    }
    foreach ($needle in @(
            "runtime-inferred frequency",
            "GPU memory pressure",
            "DirectStorage",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) runtime-inferred LRU non-claims"
    }
}

foreach ($needle in @(
        "MAVG Runtime-Inferred LRU Eviction Policy v1",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru",
        "inferred_eviction_policy",
        "inferred_lru_eviction_policy",
        "inferred_resident_page_use_generation",
        "runtime_inferred_lru_eviction_candidate_count",
        "older unprotected resident page ordering",
        "cold resident page ordering",
        "runtime-inferred frequency eviction policy was out of scope for that child"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json runtime-inferred LRU module evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json runtime-inferred LRU plan evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-lru-eviction-policy-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG Runtime-Inferred LRU Eviction Policy v1"
}

$recommendedPlanText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG Runtime-Inferred LRU Eviction Policy v1",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru",
        "inferred_eviction_policy",
        "inferred_lru_eviction_policy",
        "runtime_inferred_lru_eviction_candidate_count",
        "runtime-inferred LRU candidate ordering",
        "MAVG Runtime-Inferred Frequency Eviction Policy v1",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan runtime-inferred LRU evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Runtime-Inferred LRU Eviction Policy v1",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru",
        "inferred_lru_eviction_policy",
        "runtime_inferred_lru_eviction_candidate_count",
        "runtime-inferred frequency eviction policy was out of scope for that child"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime runtime-inferred LRU evidence"
}
