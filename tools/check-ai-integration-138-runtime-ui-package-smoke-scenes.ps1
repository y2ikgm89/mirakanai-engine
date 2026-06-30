#requires -Version 7.0
#requires -PSEdition Core

# Chapter 13.8 for check-ai-integration.ps1 Runtime UI Package Smoke Scenes v1 contracts.

$runtimeUiPackageSmokeHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/runtime_ui_package_smoke_scene.hpp"
$runtimeUiPackageSmokeSourceText = Get-AgentSurfaceText "engine/ui/src/runtime_ui_package_smoke_scene.cpp"
$runtimeUiPackageSmokeTestsText = Get-AgentSurfaceText "tests/unit/runtime_ui_package_smoke_scene_tests.cpp"
$runtimeUiCMakeText = Get-AgentSurfaceText "engine/ui/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$sample2dMainText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample2dCMakeText = Get-AgentSurfaceText "games/CMakeLists.txt"
$sample2dManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$workloadValidationText = Get-AgentSurfaceText "tools/validate-2d-production-workloads.ps1"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$runtimeBackendReadinessFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
$validationRecipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$gameCodeGuidanceFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$uiDocsText = Get-AgentSurfaceText "docs/ui.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$twoDCommercialPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"

foreach ($needle in @(
        "RuntimeUiPackageSmokeSceneKind",
        "RuntimeUiPackageSmokeSceneDiagnosticCode",
        "RuntimeUiPackageSmokeSceneRow",
        "RuntimeUiPackageSmokeSceneDiagnostic",
        "RuntimeUiPackageSmokeSceneReview",
        "runtime_ui_package_smoke_scene_kind_name",
        "review_runtime_ui_package_smoke_scenes",
        "multilingual_glyph_fallback",
        "long_label_layout",
        "controller_only_navigation",
        "accessibility_tree_review",
        "missing_multilingual_scene",
        "missing_long_label_scene",
        "missing_controller_only_scene",
        "missing_accessibility_tree_scene",
        "missing_supporting_evidence",
        "public_native_handles",
        "ui_middleware_claim",
        "external_engine_compatibility_claim",
        "row_budget_overflow"
    )) {
    Assert-ContainsText $runtimeUiPackageSmokeHeaderText $needle "engine/ui/include/mirakana/ui/runtime_ui_package_smoke_scene.hpp"
}

foreach ($needle in @(
        "append_missing_scene_diagnostics",
        "RuntimeUiPackageSmokeSceneDiagnosticCode::missing_glyph_fallback",
        "RuntimeUiPackageSmokeSceneDiagnosticCode::missing_long_label_wrap",
        "RuntimeUiPackageSmokeSceneDiagnosticCode::missing_controller_navigation",
        "RuntimeUiPackageSmokeSceneDiagnosticCode::missing_controller_glyph",
        "RuntimeUiPackageSmokeSceneDiagnosticCode::missing_accessibility_reading_order",
        "RuntimeUiPackageSmokeSceneDiagnosticCode::public_native_handles",
        "RuntimeUiPackageSmokeSceneDiagnosticCode::ui_middleware_claim",
        "RuntimeUiPackageSmokeSceneDiagnosticCode::external_engine_compatibility_claim",
        "RuntimeUiPackageSmokeSceneDiagnosticCode::row_budget_overflow"
    )) {
    Assert-ContainsText $runtimeUiPackageSmokeSourceText $needle "engine/ui/src/runtime_ui_package_smoke_scene.cpp"
}

foreach ($needle in @(
        "runtime ui package smoke scenes require every selected scene family",
        "runtime ui package smoke scenes require multilingual fallback and long label wrapping evidence",
        "runtime ui package smoke scenes reject missing controller and accessibility evidence",
        "runtime ui package smoke scenes reject unsafe API and external engine claims",
        "runtime ui package smoke scenes become ready with selected package-visible UI evidence",
        "controller_navigation_edges == 4U",
        "accessibility_reading_order_rows == 5U"
    )) {
    Assert-ContainsText $runtimeUiPackageSmokeTestsText $needle "tests/unit/runtime_ui_package_smoke_scene_tests.cpp"
}

Assert-ContainsText $runtimeUiCMakeText "src/runtime_ui_package_smoke_scene.cpp" "engine/ui/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_ui_package_smoke_scene_tests" "CMakeLists.txt"

foreach ($needle in @(
        "mirakana/ui/runtime_ui_package_smoke_scene.hpp",
        "RuntimeUiPackageSmokeSceneProbeResult",
        "validate_runtime_ui_package_smoke_scene_evidence",
        "review_runtime_ui_package_smoke_scenes",
        "--require-runtime-ui-package-smoke-scenes",
        "runtime_ui_package_smoke_scene_ready=",
        "runtime_ui_package_smoke_scene_rows=",
        "runtime_ui_package_smoke_scene_selected_rows=",
        "runtime_ui_package_smoke_scene_ready_rows=",
        "runtime_ui_package_smoke_scene_language_rows=",
        "runtime_ui_package_smoke_scene_glyph_fallback_rows=",
        "runtime_ui_package_smoke_scene_long_label_rows=",
        "runtime_ui_package_smoke_scene_long_label_max_code_units=",
        "runtime_ui_package_smoke_scene_controller_only_rows=",
        "runtime_ui_package_smoke_scene_controller_navigation_edges=",
        "runtime_ui_package_smoke_scene_controller_glyph_rows=",
        "runtime_ui_package_smoke_scene_accessibility_tree_rows=",
        "runtime_ui_package_smoke_scene_accessibility_nodes=",
        "runtime_ui_package_smoke_scene_accessibility_action_rows=",
        "runtime_ui_package_smoke_scene_accessibility_reading_order_rows=",
        "runtime_ui_package_smoke_scene_supporting_evidence_rows=",
        "runtime_ui_package_smoke_scene_native_handle_rows=",
        "runtime_ui_package_smoke_scene_ui_middleware_claim_rows=",
        "runtime_ui_package_smoke_scene_external_engine_compatibility_claim_rows=",
        "runtime_ui_package_smoke_scene_diagnostics=",
        "required_runtime_ui_package_smoke_scenes_unavailable"
    )) {
    Assert-ContainsText $sample2dMainText $needle "games/sample_2d_desktop_runtime_package/main.cpp"
}

foreach ($needle in @(
        "--require-runtime-ui-package-smoke-scenes",
        "--require-runtime-ui-standard-widgets",
        "--require-runtime-ui-widgets",
        "--require-runtime-ui-binding",
        "--require-runtime-ui-workbench",
        "--require-runtime-ui-production-stack",
        "--require-runtime-ui-platform-package",
        "--require-runtime-ui-font-rasterization",
        "--require-runtime-ui-tsf-session",
        "--require-runtime-ui-uia-publication",
        "--require-runtime-ui-atlas-upload",
        "--require-runtime-ui-renderer-atlas-handoff",
        "installed-2d-runtime-ui-package-smoke-scenes",
        "runtime-ui-package-smoke-scenes",
        "runtime_ui_package_smoke_scene_ready=1"
    )) {
    Assert-ContainsText $sample2dManifestText $needle "games/sample_2d_desktop_runtime_package/game.agent.json"
}

foreach ($needle in @(
        "--require-runtime-ui-package-smoke-scenes",
        "--require-runtime-ui-standard-widgets",
        "--require-runtime-ui-widgets",
        "--require-runtime-ui-binding",
        "--require-runtime-ui-workbench",
        "--require-runtime-ui-production-stack",
        "--require-runtime-ui-platform-package",
        "--require-runtime-ui-font-rasterization",
        "--require-runtime-ui-tsf-session",
        "--require-runtime-ui-uia-publication",
        "--require-runtime-ui-atlas-upload",
        "--require-runtime-ui-renderer-atlas-handoff",
        "runtime_ui_package_smoke_scene_ready",
        "runtime_ui_package_smoke_scene_rows",
        "runtime_ui_package_smoke_scene_diagnostics"
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
    Assert-ContainsText $workloadValidationText $needle "tools/validate-2d-production-workloads.ps1"
}
foreach ($needle in @(
        "runtime_ui_package_smoke_scene_selected_rows",
        "runtime_ui_package_smoke_scene_ready_rows",
        "runtime_ui_package_smoke_scene_long_label_max_code_units",
        "runtime_ui_package_smoke_scene_controller_navigation_edges",
        "runtime_ui_package_smoke_scene_controller_glyph_rows",
        "runtime_ui_package_smoke_scene_accessibility_nodes",
        "runtime_ui_package_smoke_scene_accessibility_action_rows",
        "runtime_ui_package_smoke_scene_accessibility_reading_order_rows",
        "runtime_ui_package_smoke_scene_supporting_evidence_rows"
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}
foreach ($needle in @(
        "--require-runtime-ui-standard-widgets",
        "--require-runtime-ui-widgets",
        "--require-runtime-ui-binding",
        "--require-runtime-ui-workbench",
        "--require-runtime-ui-production-stack",
        "--require-runtime-ui-platform-package",
        "--require-runtime-ui-font-rasterization",
        "--require-runtime-ui-tsf-session",
        "--require-runtime-ui-uia-publication",
        "--require-runtime-ui-atlas-upload",
        "--require-runtime-ui-renderer-atlas-handoff",
        "--require-runtime-ui-package-smoke-scenes"
    )) {
    Assert-ContainsText $sample2dCMakeText $needle "games/CMakeLists.txt"
}

foreach ($needle in @(
        "engine/ui/include/mirakana/ui/runtime_ui_package_smoke_scene.hpp",
        "Runtime UI Package Smoke Scenes v1",
        "RuntimeUiPackageSmokeSceneRow",
        "review_runtime_ui_package_smoke_scenes",
        "runtime_ui_controller_glyph_refs=1",
        "runtime_ui_package_smoke_scene_ready=1"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json"
}

foreach ($needle in @(
        "runtime-ui-package-smoke-scenes",
        "Runtime UI Package Smoke Scenes v1",
        "RuntimeUiPackageSmokeSceneKind",
        "review_runtime_ui_package_smoke_scenes",
        "--require-runtime-ui-package-smoke-scenes",
        "runtime_ui_package_smoke_scene_ready=1",
        "runtime_ui_package_smoke_scene_supporting_evidence_rows=10",
        "Unity/Unreal/Godot compatibility or parity"
    )) {
    Assert-ContainsText $runtimeBackendReadinessFragmentText $needle "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
}

foreach ($needle in @(
        "installed-2d-runtime-ui-package-smoke-scenes",
        "Runtime UI Package Smoke Scenes v1",
        "explicit prerequisite flags",
        "--require-runtime-ui-standard-widgets",
        "--require-runtime-ui-widgets",
        "--require-runtime-ui-binding",
        "--require-runtime-ui-workbench",
        "--require-runtime-ui-production-stack",
        "--require-runtime-ui-platform-package",
        "--require-runtime-ui-font-rasterization",
        "--require-runtime-ui-tsf-session",
        "--require-runtime-ui-uia-publication",
        "--require-runtime-ui-atlas-upload",
        "--require-runtime-ui-renderer-atlas-handoff",
        "--require-runtime-ui-package-smoke-scenes",
        "runtime_ui_package_smoke_scene_ready=1",
        "ten supporting evidence rows",
        "zero diagnostics/native-handle/UI-middleware/external-engine compatibility claim rows"
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle "engine/agent/manifest.fragments/009-validationRecipes.json"
}

foreach ($needle in @(
        "currentRuntimeUiPackageSmokeScenes",
        "RuntimeUiPackageSmokeSceneKind",
        "RuntimeUiPackageSmokeSceneRow",
        "review_runtime_ui_package_smoke_scenes",
        "--require-runtime-ui-package-smoke-scenes",
        "runtime_ui_package_smoke_scene_ready=1",
        "runtime_ui_package_smoke_scene_supporting_evidence_rows=10",
        "runtime_ui_package_smoke_scene_native_handle_rows=0",
        "Unity/Unreal/Godot compatibility or parity"
    )) {
    Assert-ContainsText $gameCodeGuidanceFragmentText $needle "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}

foreach ($docsSurface in @(
        @{ Label = "docs/ui.md"; Text = $uiDocsText },
        @{ Label = "docs/current-capabilities.md"; Text = $currentCapabilitiesText },
        @{ Label = "docs/roadmap.md"; Text = $roadmapText },
        @{ Label = "docs/ai-game-development.md"; Text = $aiGameDevelopmentText },
        @{ Label = "docs/superpowers/plans/README.md"; Text = $planRegistryText },
        @{ Label = "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"; Text = $twoDCommercialPlanText }
    )) {
    foreach ($needle in @(
            "Runtime UI Package Smoke Scenes v1",
            "RuntimeUiPackageSmokeSceneRow",
            "review_runtime_ui_package_smoke_scenes",
            "--require-runtime-ui-package-smoke-scenes",
            "supporting evidence",
            "native handles",
            "UI middleware",
            "Unity/Unreal/Godot compatibility"
        )) {
        Assert-ContainsText $docsSurface.Text $needle $docsSurface.Label
    }
}
