#requires -Version 7.0
#requires -PSEdition Core

# Chapter 10.1 for check-ai-integration.ps1 Physics/Navigation production breadth review gates.

$physicsHeaderText = Get-AgentSurfaceText "engine/physics/include/mirakana/physics/physics_production_breadth.hpp"
$physicsSourceText = Get-AgentSurfaceText "engine/physics/src/physics_production_breadth.cpp"
$navigationHeaderText = Get-AgentSurfaceText "engine/navigation/include/mirakana/navigation/navigation_production_breadth.hpp"
$navigationSourceText = Get-AgentSurfaceText "engine/navigation/src/navigation_production_breadth.cpp"
$physicsCMakeText = Get-AgentSurfaceText "engine/physics/CMakeLists.txt"
$navigationCMakeText = Get-AgentSurfaceText "engine/navigation/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$testText = Get-AgentSurfaceText "tests/unit/physics_navigation_production_breadth_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$gameGuidanceFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "PhysicsProductionBreadthEvidenceRow",
        "PhysicsProductionBreadthReviewRequest",
        "PhysicsProductionBreadthReview",
        "oriented_box_query",
        "persistent_joint_asset",
        "optional_native_adapter",
        "host_evidence_required",
        "native_handles_exposed",
        "unsupported_broad_claim",
        "review_physics_production_breadth"
    )) {
    Assert-ContainsText $physicsHeaderText $needle "engine/physics/include/mirakana/physics/physics_production_breadth.hpp"
}

foreach ($needle in @(
        "PhysicsProductionBreadthDiagnostic::missing_official_source",
        "PhysicsProductionBreadthDiagnostic::missing_budget",
        "PhysicsProductionBreadthDiagnostic::missing_required_feature",
        "PhysicsProductionBreadthDiagnostic::missing_dependency_legal_record",
        "PhysicsProductionBreadthStatus::host_evidence_required",
        "claims_broad_middleware_parity",
        "native_handles_exposed",
        "ready_features"
    )) {
    Assert-ContainsText $physicsSourceText $needle "engine/physics/src/physics_production_breadth.cpp"
}

foreach ($needle in @(
        "NavigationProductionBreadthEvidenceRow",
        "NavigationProductionBreadthReviewRequest",
        "NavigationProductionBreadthReview",
        "navmesh_source_import",
        "string_pulling_path",
        "optional_recast_detour_adapter",
        "host_evidence_required",
        "source_mutation_claimed",
        "unsupported_broad_claim",
        "review_navigation_production_breadth"
    )) {
    Assert-ContainsText $navigationHeaderText $needle "engine/navigation/include/mirakana/navigation/navigation_production_breadth.hpp"
}

foreach ($needle in @(
        "NavigationProductionBreadthDiagnostic::missing_official_source",
        "NavigationProductionBreadthDiagnostic::missing_budget",
        "NavigationProductionBreadthDiagnostic::missing_required_feature",
        "NavigationProductionBreadthDiagnostic::missing_dependency_legal_record",
        "NavigationProductionBreadthStatus::host_evidence_required",
        "claims_arbitrary_runtime_bake",
        "exposes_native_recast_detour_handles",
        "mutates_source_geometry"
    )) {
    Assert-ContainsText $navigationSourceText $needle "engine/navigation/src/navigation_production_breadth.cpp"
}

Assert-ContainsText $physicsCMakeText "src/physics_production_breadth.cpp" "engine/physics/CMakeLists.txt"
Assert-ContainsText $navigationCMakeText "src/navigation_production_breadth.cpp" "engine/navigation/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_physics_navigation_production_breadth_tests" "CMakeLists.txt"

foreach ($needle in @(
        "physics production breadth review accepts complete first party evidence",
        "physics production breadth review fails closed on missing and unsafe evidence",
        "physics production breadth review separates host gated optional adapter evidence",
        "navigation production breadth review accepts complete first party evidence",
        "navigation production breadth review fails closed on source mutation and native claims",
        "navigation production breadth review separates host gated recast detour evidence",
        "https://jrouwe.github.io/JoltPhysics/",
        "https://recastnav.com/"
    )) {
    Assert-ContainsText $testText $needle "tests/unit/physics_navigation_production_breadth_tests.cpp"
}

foreach ($docSurface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $gameGuidanceFragmentText; Label = "engine/agent/manifest.fragments/014-gameCodeGuidance.json" }
    )) {
    Assert-ContainsText $docSurface.Text "review_physics_production_breadth" $docSurface.Label
    Assert-ContainsText $docSurface.Text "review_navigation_production_breadth" $docSurface.Label
    Assert-ContainsText $docSurface.Text "host-gated" $docSurface.Label
    Assert-ContainsText $docSurface.Text "native handle" $docSurface.Label
}

foreach ($needle in @(
        "engine/physics/include/mirakana/physics/physics_production_breadth.hpp",
        "Physics Production Breadth Gate v1",
        "review_physics_production_breadth",
        "engine/navigation/include/mirakana/navigation/navigation_production_breadth.hpp",
        "Navigation Production Breadth Gate v1",
        "review_navigation_production_breadth"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json"
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}
