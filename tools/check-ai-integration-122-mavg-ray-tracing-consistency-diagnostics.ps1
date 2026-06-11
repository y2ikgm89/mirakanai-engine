#requires -Version 7.0
#requires -PSEdition Core
# Chapter 122 for check-ai-integration.ps1 static contracts.

$headerText = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/mavg_ray_tracing_consistency.hpp"
$sourceText = Get-AgentSurfaceText "engine/assets/src/mavg_ray_tracing_consistency.cpp"
$testsText = Get-AgentSurfaceText "tests/unit/mavg_ray_tracing_consistency_tests.cpp"
$assetsCmakeText = Get-AgentSurfaceText "engine/assets/CMakeLists.txt"
$cmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-12-mavg-ray-tracing-consistency-diagnostics-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "MavgRayTracingPayloadPolicy",
        "MavgRayTracingDeformationTier",
        "MavgRayTracingConsistencyDiagnosticCode",
        "MavgRasterClusterPayloadRow",
        "MavgRayTracingClusterPayloadRow",
        "MavgRayTracingConsistencyDesc",
        "MavgRayTracingConsistencyRow",
        "MavgRayTracingConsistencyResult",
        "plan_mavg_ray_tracing_consistency",
        "has_mavg_ray_tracing_consistency_diagnostic",
        "executed_ray_tracing",
        "exposed_native_handles"
    )) {
    Assert-ContainsText $headerText $needle "MAVG ray tracing consistency public contract"
}

foreach ($needle in @(
        "duplicate_raster_payload_row",
        "duplicate_ray_tracing_payload_row",
        "missing_raster_payload",
        "missing_ray_tracing_payload",
        "payload_draw_range_mismatch",
        "fallback_mismatch",
        "unsupported_dynamic_displacement",
        "unsupported_deformation_tier",
        "sort_result"
    )) {
    Assert-ContainsText $sourceText $needle "MAVG ray tracing consistency implementation"
}

foreach ($needle in @(
        "mavg ray tracing consistency accepts matching raster and rt payload rows",
        "mavg ray tracing consistency rejects missing and duplicate evidence",
        "mavg ray tracing consistency rejects mismatched raster and rt payload provenance",
        "mavg ray tracing consistency rejects unsupported deformation tiers",
        "MK_REQUIRE(!result.executed_ray_tracing)",
        "MK_REQUIRE(!result.exposed_native_handles)"
    )) {
    Assert-ContainsText $testsText $needle "MAVG ray tracing consistency tests"
}

foreach ($needle in @(
        "src/mavg_ray_tracing_consistency.cpp"
    )) {
    Assert-ContainsText $assetsCmakeText $needle "MAVG ray tracing consistency asset target source"
}

foreach ($needle in @(
        "MK_mavg_ray_tracing_consistency_tests",
        "tests/unit/mavg_ray_tracing_consistency_tests.cpp"
    )) {
    Assert-ContainsText $cmakeText $needle "MAVG ray tracing consistency test target"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "implementation plan" },
        @{ Text = $planRegistryText; Label = "plan registry" },
        @{ Text = $currentCapabilitiesText; Label = "current capabilities" },
        @{ Text = $roadmapText; Label = "roadmap" },
        @{ Text = $mavgArchitectureSpecText; Label = "MAVG architecture spec" },
        @{ Text = $masterPlanText; Label = "MAVG master plan" },
        @{ Text = $aiLoopFragmentText; Label = "production loop fragment" },
        @{ Text = $modulesFragmentText; Label = "modules fragment" }
    )) {
    foreach ($needle in @(
            "mavg-ray-tracing-consistency-diagnostics-v1",
            "MAVG Ray Tracing Consistency Diagnostics v1",
            "mavg_ray_tracing_consistency.hpp",
            "MavgRasterClusterPayloadRow",
            "MavgRayTracingClusterPayloadRow",
            "MavgRayTracingConsistencyResult",
            "plan_mavg_ray_tracing_consistency"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG ray tracing consistency evidence"
    }
    foreach ($needle in @(
            "acceleration-structure",
            "backend RT execution",
            "native handles",
            "Metal readiness",
            "Nanite",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG ray tracing consistency non-claims"
    }
}
