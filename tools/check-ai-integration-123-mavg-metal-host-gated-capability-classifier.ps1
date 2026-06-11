#requires -Version 7.0
#requires -PSEdition Core
# Chapter 123 for check-ai-integration.ps1 MAVG Metal host-gated capability classifier evidence.

$headerText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/mavg_metal_capability_policy.hpp"
$sourceText = Get-AgentSurfaceText "engine/renderer/src/mavg_metal_capability_policy.cpp"
$testsText = Get-AgentSurfaceText "tests/unit/mavg_metal_capability_policy_tests.cpp"
$cmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-12-mavg-metal-host-gated-capability-classifier-v1.md"
$mavgSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "MavgMetalCapabilityRow",
        "MavgMetalCapabilityPlan",
        "MavgMetalCapabilityDiagnosticCode",
        "plan_mavg_metal_capabilities",
        "streamed_cluster_draw",
        "mesh_shader_execution",
        "ray_tracing_consistency",
        "benchmark_evidence",
        "unsupported_cross_backend_inference",
        "unsupported_native_handle_claim",
        "unsupported_gpu_execution_claim",
        "unsupported_nanite_claim",
        "executed_gpu_commands",
        "exposed_native_handles",
        "claimed_nanite_equivalence"
    )) {
    Assert-ContainsText $headerText $needle "mavg_metal_capability_policy.hpp public contract"
}

foreach ($needle in @(
        "is_reviewed_metal_host_validation_recipe",
        "renderer-metal-apple-host-evidence",
        "is_host_gated_row",
        "is_ready_row",
        "MavgMetalCapabilityStatus::host_evidence_required",
        "MavgMetalCapabilityDiagnosticCode::unsupported_cross_backend_inference",
        "MavgMetalCapabilityDiagnosticCode::unsupported_native_handle_claim",
        "MavgMetalCapabilityDiagnosticCode::unsupported_gpu_execution_claim",
        "MavgMetalCapabilityDiagnosticCode::unsupported_nanite_claim"
    )) {
    Assert-ContainsText $sourceText $needle "mavg_metal_capability_policy.cpp fail-closed implementation"
}

foreach ($needle in @(
        "keeps all rows host gated without Apple host evidence",
        "becomes ready only with reviewed Apple host evidence",
        "rejects cross backend inference and native handles",
        "rejects unsupported execution and Nanite claims",
        "replay hash is stable under input row order"
    )) {
    Assert-ContainsText $testsText $needle "MK_mavg_metal_capability_policy_tests coverage"
}

Assert-ContainsText $cmakeText "MK_mavg_metal_capability_policy_tests" "CMake MAVG Metal classifier test target"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-12-mavg-metal-host-gated-capability-classifier-v1.md" },
        @{ Text = $mavgSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" }
    )) {
    foreach ($needle in @(
            "mavg-metal-host-gated-capability-classifier-v1",
            "mavg_metal_capability_policy.hpp",
            "host-gated",
            "Apple-host evidence",
            "native handle",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Metal classifier evidence"
    }
}

$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module" }
$rendererManifestText = ((@($rendererModule[0].publicHeaders) -join " "), (@($rendererModule[0].recentEvidence) -join " "), [string]$rendererModule[0].purpose) -join " "
foreach ($needle in @(
        "engine/renderer/include/mirakana/renderer/mavg_metal_capability_policy.hpp",
        "MAVG Metal Host-Gated Capability Classifier v1",
        "MavgMetalCapabilityRow",
        "MavgMetalCapabilityPlan",
        "plan_mavg_metal_capabilities",
        "executed_gpu_commands",
        "exposed_native_handles",
        "claimed_nanite_equivalence"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer MAVG Metal classifier evidence"
}

$recommendedPlanText = (([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.localSliceEvidence),
    ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.latestCloseoutEvidence),
    ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.completedContext)) -join " "
foreach ($needle in @(
        "MAVG Metal Host-Gated Capability Classifier v1",
        "MavgMetalCapabilityDiagnosticCode",
        "MK_mavg_metal_capability_policy_tests",
        "non-Metal backend inference",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan MAVG Metal classifier evidence"
}
