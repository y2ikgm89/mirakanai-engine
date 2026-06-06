#requires -Version 7.0
#requires -PSEdition Core
# Chapter 121 for check-ai-integration.ps1 static contracts.

$mavgPageStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
$mavgPageStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_page_streaming.cpp"
$mavgPageStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_page_streaming_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$mavgUseGenerationPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-page-use-generation-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "non_monotonic_use_generation",
        "RuntimeMavgResidentPageUseGenerationDesc",
        "std::span<const RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters",
        "std::span<const RuntimeMavgResidentPageMountRow> resident_page_mounts",
        "std::span<const RuntimeMavgPageStreamingRecencyRow> previous_recency_rows",
        "current_use_generation",
        "RuntimeMavgResidentPageUseGenerationResult",
        "inferred_resident_page_use_generation",
        "touched_resident_page_count",
        "carried_recency_row_count",
        "new_resident_page_count",
        "dropped_nonresident_recency_row_count",
        "infer_runtime_mavg_resident_page_use_generations"
    )) {
    Assert-ContainsText $mavgPageStreamingHeaderText $needle "mavg_page_streaming.hpp use-generation public contract"
}

foreach ($needle in @(
        "contains_page_index",
        "infer_runtime_mavg_resident_page_use_generations",
        "selected_page_indices",
        "dropped_nonresident_recency_row_count",
        "RuntimeMavgPageStreamingDiagnosticCode::non_monotonic_use_generation",
        "result.inferred_resident_page_use_generation = true",
        "find_recency_row(desc.previous_recency_rows, mount)"
    )) {
    Assert-ContainsText $mavgPageStreamingSourceText $needle "mavg_page_streaming.cpp use-generation implementation"
}

foreach ($needle in @(
        "runtime mavg page streaming infers selected page generations and carries unselected rows",
        "runtime mavg page streaming generated use generations feed recency eviction order",
        "runtime mavg page streaming use generation inference drops nonresident rows and initializes cold pages",
        "runtime mavg page streaming use generation inference rejects duplicate previous rows",
        "runtime mavg page streaming use generation inference rejects nonmonotonic generations",
        "inferred_resident_page_use_generation",
        "dropped_nonresident_recency_row_count",
        "non_monotonic_use_generation",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $mavgPageStreamingTestsText $needle "MK_runtime_mavg_page_streaming_tests use-generation coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgUseGenerationPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-page-use-generation-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $productionMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-runtime-inferred-page-use-generation-v1",
            "MAVG Runtime-Inferred Page Use Generation v1",
            "RuntimeMavgResidentPageUseGenerationDesc",
            "RuntimeMavgResidentPageUseGenerationResult",
            "infer_runtime_mavg_resident_page_use_generations",
            "inferred_resident_page_use_generation"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) use-generation evidence"
    }
    foreach ($needle in @(
            "runtime-inferred frequency",
            "GPU memory pressure",
            "DirectStorage",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) use-generation non-claims"
    }
}

foreach ($needle in @(
        "MAVG Runtime-Inferred Page Use Generation v1",
        "RuntimeMavgResidentPageUseGenerationDesc",
        "RuntimeMavgResidentPageUseGenerationResult",
        "infer_runtime_mavg_resident_page_use_generations",
        "inferred_resident_page_use_generation",
        "dropped_nonresident_recency_row_count",
        "non_monotonic_use_generation",
        "runtime-inferred frequency eviction policy was out of scope for that child"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json use-generation module evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json use-generation plan evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-runtime-inferred-page-use-generation-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG Runtime-Inferred Page Use Generation v1"
}

$recommendedPlanText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG Runtime-Inferred Page Use Generation v1",
        "RuntimeMavgResidentPageUseGenerationDesc",
        "RuntimeMavgResidentPageUseGenerationResult",
        "infer_runtime_mavg_resident_page_use_generations",
        "inferred_resident_page_use_generation",
        "non_monotonic_use_generation",
        "MAVG Runtime-Inferred Frequency Eviction Policy v1",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan use-generation evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Runtime-Inferred Page Use Generation v1",
        "RuntimeMavgResidentPageUseGenerationDesc",
        "RuntimeMavgResidentPageUseGenerationResult",
        "infer_runtime_mavg_resident_page_use_generations",
        "inferred_resident_page_use_generation",
        "non_monotonic_use_generation",
        "runtime-inferred frequency eviction policy was out of scope for that child"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime use-generation evidence"
}
