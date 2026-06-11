#requires -Version 7.0
#requires -PSEdition Core
# Chapter 121 for check-ai-integration.ps1 MAVG deformation tier diagnostics evidence.

$mavgDeformationHeaderText = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/mavg_deformation.hpp"
$mavgDeformationSourceText = Get-AgentSurfaceText "engine/assets/src/mavg_deformation.cpp"
$mavgDeformationTestsText = Get-AgentSurfaceText "tests/unit/mavg_deformation_tests.cpp"
$assetsCmakeText = Get-AgentSurfaceText "engine/assets/CMakeLists.txt"
$cmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgDeformationPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-12-mavg-deformation-tier-diagnostics-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "MavgDeformationTier",
        "MavgDeformationClusterRow",
        "MavgSkinnedClusterBoundsRow",
        "MavgMorphClusterDeltaBoundsRow",
        "MavgDeformationTierDiagnosticsDesc",
        "MavgDeformationTierDiagnosticsResult",
        "plan_mavg_deformation_tier_diagnostics",
        "has_mavg_deformation_diagnostic",
        "supported_by_mavg_cluster_graph",
        "requires_conventional_fallback",
        "requires_runtime_refit",
        "touched_renderer_rhi_handles",
        "executed_runtime_upload",
        "executed_mesh_shader",
        "executed_directstorage",
        "executed_background_streaming",
        "claimed_broad_optimization"
    )) {
    Assert-ContainsText $mavgDeformationHeaderText $needle "mavg_deformation.hpp public contract"
}

foreach ($needle in @(
        "duplicate_tier_row",
        "unknown_cluster",
        "missing_skinned_bone_bounds",
        "invalid_skinned_bone_bounds",
        "missing_morph_delta_bounds",
        "invalid_morph_delta_bounds",
        "unsupported_dynamic_tier",
        "std::ranges::sort",
        "MavgDeformationTier::dynamic_displacement"
    )) {
    Assert-ContainsText $mavgDeformationSourceText $needle "mavg_deformation.cpp fail-closed diagnostics"
}

foreach ($needle in @(
        "mavg deformation static and rigid tiers are ready without runtime refit",
        "mavg deformation skinned and morph tiers require reviewed bounds rows",
        "mavg deformation missing bounds and dynamic tiers fail closed to conventional fallback",
        "mavg deformation rows are stable and duplicate tier rows are diagnosed",
        "mavg deformation invalid reviewed bounds rows fail closed",
        "has_mavg_deformation_diagnostic",
        "MK_REQUIRE(!result.touched_renderer_rhi_handles)",
        "MK_REQUIRE(!result.executed_runtime_upload)",
        "MK_REQUIRE(!result.executed_mesh_shader)"
    )) {
    Assert-ContainsText $mavgDeformationTestsText $needle "MK_mavg_deformation_tests coverage"
}

foreach ($needle in @(
        "src/mavg_deformation.cpp"
    )) {
    Assert-ContainsText $assetsCmakeText $needle "MK_assets mavg deformation source registration"
}

foreach ($needle in @(
        "MK_mavg_deformation_tests",
        "tests/unit/mavg_deformation_tests.cpp"
    )) {
    Assert-ContainsText $cmakeText $needle "MAVG deformation CMake test target"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgDeformationPlanText; Label = "docs/superpowers/plans/2026-06-12-mavg-deformation-tier-diagnostics-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" }
    )) {
    foreach ($needle in @(
            "mavg-deformation-tier-diagnostics-v1",
            "MAVG Deformation Tier Diagnostics v1",
            "mavg_deformation.hpp",
            "MavgDeformationTierDiagnosticsResult",
            "plan_mavg_deformation_tier_diagnostics",
            "Tier 0 static",
            "Tier 1 rigid",
            "Tier 2 skinned",
            "Tier 3 morph",
            "Tier 4 dynamic"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) deformation tier diagnostics evidence"
    }
    foreach ($needle in @(
            "upload/refit",
            "renderer/RHI execution",
            "DirectStorage",
            "background streaming",
            "mesh shader",
            "Metal readiness",
            "Nanite",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) deformation tier diagnostics non-claims"
    }
}

$assetModule = @($manifest.modules | Where-Object { $_.name -eq "MK_assets" })
if ($assetModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_assets module" }
$assetManifestText = ((@($assetModule[0].publicHeaders) -join " "), (@($assetModule[0].recentEvidence) -join " "), [string]$assetModule[0].purpose) -join " "
foreach ($needle in @(
        "engine/assets/include/mirakana/assets/mavg_deformation.hpp",
        "MAVG Deformation Tier Diagnostics v1",
        "MavgDeformationTierDiagnosticsResult",
        "plan_mavg_deformation_tier_diagnostics",
        "dynamic displacement",
        "without runtime upload/refit"
    )) {
    Assert-ContainsText $assetManifestText $needle "engine/agent/manifest.json MK_assets deformation evidence"
}

$recommendedPlanText = (([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.localSliceEvidence),
    ([string]$manifest.aiOperableProductionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG Deformation Tier Diagnostics v1",
        "mavg-deformation-tier-diagnostics-v1",
        "mavg_deformation.hpp",
        "MavgDeformationTierDiagnosticsResult",
        "runtime deformation upload/refit"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json production-loop deformation evidence"
}

Assert-ContainsText $aiLoopFragmentText '"unsupportedProductionGaps": []' "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json zero-gap truth"
