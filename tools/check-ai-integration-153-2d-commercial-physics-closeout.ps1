#requires -Version 7.0
#requires -PSEdition Core

# Chapter 153 for check-ai-integration.ps1 static contracts.
# Runtime 2D Commercial Physics Closeout v1 package/manifest/plan alignment.

$physicsHeader = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/two_d_commercial_physics_closeout.hpp"
$physicsSource = Get-AgentSurfaceText "engine/runtime/src/two_d_commercial_physics_closeout.cpp"
$physicsTests = Get-AgentSurfaceText "tests/unit/runtime_2d_commercial_physics_closeout_tests.cpp"
$rootCMake = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeCMake = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$sample2dMain = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample2dManifest = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$validatorScript = Get-AgentSurfaceText "tools/validate-2d-commercial-physics-closeout.ps1"
$validationRecipesManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$gameCodeGuidanceManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$composedManifest = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilities = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmap = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistry = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"

foreach ($needle in @(
        "Runtime2DCommercialPhysicsFeatureKind",
        "Runtime2DCommercialPhysicsSourceKind",
        "Runtime2DCommercialPhysicsDiagnosticCode",
        "Runtime2DCommercialPhysicsFeatureRow",
        "Runtime2DCommercialPhysicsSourceRow",
        "Runtime2DCommercialPhysicsCloseoutDesc",
        "Runtime2DCommercialPhysicsCloseoutResult",
        "evaluate_runtime_2d_commercial_physics_closeout",
        "time_of_impact_ccd",
        "joint_constraints",
        "trigger_area_events",
        "kinematic_contact_resolution",
        "deterministic_replay",
        "package_counters",
        "broad_physics_parity_claim",
        "legal_approval_claim"
    )) {
    Assert-ContainsText $physicsHeader $needle "two_d_commercial_physics_closeout.hpp"
}

foreach ($needle in @(
        "kRequiredFeatureKinds",
        "kRequiredSourceKinds",
        "feature_not_ready",
        "source_not_ready",
        "selected_package_claim_missing",
        "dynamic_vs_dynamic_ccd_broad_claim",
        "physics_middleware_claim",
        "external_engine_compatibility_claim",
        "cross_platform_parity_claim",
        "legal_approval_claim"
    )) {
    Assert-ContainsText $physicsSource $needle "two_d_commercial_physics_closeout.cpp"
}

foreach ($needle in @(
        "runtime 2d commercial physics closeout accepts selected package evidence",
        "runtime 2d commercial physics closeout rejects duplicate feature and source rows",
        "runtime 2d commercial physics closeout rejects unsafe broad claims",
        "feature_rows == 6U",
        "source_rows == 4U"
    )) {
    Assert-ContainsText $physicsTests $needle "runtime_2d_commercial_physics_closeout_tests.cpp"
}

foreach ($needle in @(
        "src/two_d_commercial_physics_closeout.cpp",
        "tests/unit/runtime_2d_commercial_physics_closeout_tests.cpp",
        "MK_runtime_2d_commercial_physics_closeout_tests"
    )) {
    Assert-ContainsText ($runtimeCMake + "`n" + $rootCMake) $needle "2d commercial physics closeout CMake registration"
}

foreach ($needle in @(
        "mirakana/runtime/two_d_commercial_physics_closeout.hpp",
        "--require-2d-commercial-physics-closeout",
        "make_2d_commercial_physics_feature_rows",
        "make_2d_commercial_physics_source_rows",
        "validate_2d_commercial_physics_closeout_package_evidence",
        "2d_commercial_physics_closeout_ready=",
        "2d_commercial_physics_feature_rows=",
        "2d_commercial_physics_source_rows=",
        "2d_commercial_physics_time_of_impact_rows=",
        "2d_commercial_physics_exact_sweep_shape_pair_rows=",
        "2d_commercial_physics_kinematic_contact_rows=",
        "2d_commercial_physics_joint_rows=",
        "2d_commercial_physics_trigger_event_rows=",
        "2d_commercial_physics_dynamic_vs_dynamic_ccd_claim_rows=",
        "2d_commercial_physics_physics_middleware_claim_rows=",
        "2d_commercial_physics_broad_physics_parity_claim_rows=",
        "2d_commercial_physics_external_engine_claim_rows=",
        "2d_commercial_physics_cross_platform_parity_claim_rows=",
        "2d_commercial_physics_legal_approval_claim_rows=",
        "required_2d_commercial_physics_closeout_unavailable"
    )) {
    Assert-ContainsText $sample2dMain $needle "sample_2d_desktop_runtime_package/main.cpp"
}

foreach ($needle in @(
        "installed-2d-commercial-physics-closeout-smoke",
        "physics-commercial-closeout-2d",
        "commercialPhysicsCloseout2D",
        "evaluate_runtime_2d_commercial_physics_closeout",
        "2d_commercial_physics_closeout_ready=1",
        "2d_commercial_physics_feature_rows=6",
        "2d_commercial_physics_source_rows=4",
        "2d_commercial_physics_time_of_impact_rows=5",
        "2d_commercial_physics_joint_rows=4",
        "2d_commercial_physics_trigger_event_rows=2",
        "2d_commercial_physics_native_handle_access_rows=0",
        "2d_commercial_physics_external_engine_claim_rows=0",
        "2d_commercial_physics_cross_platform_parity_claim_rows=0",
        "2d_commercial_physics_legal_approval_claim_rows=0",
        "external commercial engine compatibility",
        "legal approval"
    )) {
    Assert-ContainsText $sample2dManifest $needle "sample_2d_desktop_runtime_package/game.agent.json"
}

foreach ($needle in @(
        "2d_commercial_physics_closeout_status",
        "2d_commercial_physics_closeout_ready",
        "2d_commercial_physics_feature_rows",
        "2d_commercial_physics_source_rows",
        "2d_commercial_physics_time_of_impact_rows",
        "2d_commercial_physics_exact_sweep_shape_pair_rows",
        "2d_commercial_physics_kinematic_contact_rows",
        "2d_commercial_physics_joint_rows",
        "2d_commercial_physics_trigger_event_rows",
        "package-desktop-runtime.ps1",
        "installed-2d-commercial-physics-closeout-smoke",
        "physics-commercial-closeout-2d"
    )) {
    Assert-ContainsText $validatorScript $needle "tools/validate-2d-commercial-physics-closeout.ps1"
}

foreach ($needle in @(
        "desktop-runtime-2d-commercial-physics-closeout",
        "tools/validate-2d-commercial-physics-closeout.ps1 -RequireReady",
        "Runtime2DCommercialPhysicsCloseoutDesc",
        "zero broad physics parity",
        "zero legal-approval claims"
    )) {
    Assert-ContainsText $validationRecipesManifest $needle "engine/agent/manifest.fragments/009-validationRecipes.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json validation recipes"
}

foreach ($needle in @(
        "twoDCommercialPhysicsCloseoutPhase7Evidence",
        "2d-commercial-physics-closeout-v1",
        "Runtime2DCommercialPhysicsFeatureKind",
        "TOI/CCD rows, exact sweep shape pairs",
        "zero broad physics parity rows"
    )) {
    Assert-ContainsText $productionLoopManifest $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json production loop"
}

foreach ($needle in @(
        "current2DCommercialPhysicsCloseoutPhase7",
        "evaluate_runtime_2d_commercial_physics_closeout",
        "--require-2d-commercial-physics-closeout",
        "first-party physics2d runtime extension API",
        "external engine API/layout/asset/project import compatibility"
    )) {
    Assert-ContainsText $gameCodeGuidanceManifest $needle "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json game code guidance"
}

foreach ($needle in @(
        "2D Commercial Physics Closeout v1",
        "tools/validate-2d-commercial-physics-closeout.ps1",
        "TOI/CCD",
        "joint/constraint",
        "trigger/area",
        "kinematic contact",
        "deterministic replay",
        "zero broad physics parity",
        "Unity/Unreal/Godot compatibility",
        "/kitware/cmake"
    )) {
    Assert-ContainsText ($currentCapabilities + "`n" + $roadmap + "`n" + $planRegistry + "`n" + $planText) $needle "2d commercial physics closeout docs and plan"
}
