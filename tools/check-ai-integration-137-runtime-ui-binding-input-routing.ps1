#requires -Version 7.0
#requires -PSEdition Core

# Chapter 13.7 for check-ai-integration.ps1 Runtime UI Binding And Input Routing v1 contracts.

$runtimeUiBindingHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/runtime_ui_binding.hpp"
$runtimeUiBindingSourceText = Get-AgentSurfaceText "engine/ui/src/runtime_ui_binding.cpp"
$runtimeUiCMakeText = Get-AgentSurfaceText "engine/ui/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeUiBindingTestsText = Get-AgentSurfaceText "tests/unit/runtime_ui_binding_tests.cpp"
$sample2dMainText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample2dManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$uiDocsText = Get-AgentSurfaceText "docs/ui.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-12-first-party-ui-runtime-editor-production-roadmap-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$runtimeBackendReadinessFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
$validationRecipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$gameCodeGuidanceFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RuntimeUiBindingValueType",
        "RuntimeUiBindingTarget",
        "RuntimeUiBindingPlanStatus",
        "RuntimeUiBindingDiagnosticCode",
        "RuntimeUiBindingDiagnostic",
        "RuntimeUiBindingValueRow",
        "RuntimeUiBindingRow",
        "RuntimeUiCommandAvailabilityRow",
        "RuntimeUiFocusScope",
        "RuntimeUiNavigationEdge",
        "RuntimeUiControllerGlyphRef",
        "RuntimeUiPointerCaptureRow",
        "RuntimeUiCommandInvocationRow",
        "RuntimeUiBindingDocument",
        "RuntimeUiInputRoutingPlan",
        "RuntimeUiBindingPlan",
        "runtime_ui_binding_value_type_name",
        "runtime_ui_binding_target_name",
        "runtime_ui_binding_plan_status_name",
        "plan_runtime_ui_binding",
        "plan_runtime_ui_input_routing"
    )) {
    Assert-ContainsText $runtimeUiBindingHeaderText $needle "engine/ui/include/mirakana/ui/runtime_ui_binding.hpp"
}

foreach ($needle in @(
        "missing_binding_id",
        "duplicate_binding_id",
        "missing_binding_key",
        "type_mismatch",
        "missing_element",
        "missing_command_id",
        "duplicate_command_id",
        "disabled_command_invocation",
        "missing_focus_scope",
        "missing_navigation_target",
        "navigation_cycle",
        "modal_focus_escape",
        "unknown_controller_glyph_ref",
        "pointer_capture_conflict"
    )) {
    Assert-ContainsText $runtimeUiBindingHeaderText $needle "runtime UI binding public header diagnostics"
}

foreach ($needle in @(
        "missing_binding_key",
        "type_mismatch",
        "disabled_command_invocation",
        "navigation_cycle",
        "modal_focus_escape",
        "unknown_controller_glyph_ref",
        "pointer_capture_conflict"
    )) {
    Assert-ContainsText $runtimeUiBindingTestsText $needle "tests/unit/runtime_ui_binding_tests.cpp"
}

foreach ($needle in @(
        "document.document.set_text",
        "document.document.set_enabled",
        "document.document.set_visible",
        "RuntimeUiBindingDiagnosticCode::disabled_command_invocation",
        "RuntimeUiBindingDiagnosticCode::navigation_cycle",
        "RuntimeUiBindingDiagnosticCode::modal_focus_escape",
        "RuntimeUiBindingDiagnosticCode::unknown_controller_glyph_ref",
        "RuntimeUiBindingDiagnosticCode::pointer_capture_conflict",
        "gameplay_commands_executed = 0U",
        "plan_runtime_ui_input_routing(document)"
    )) {
    Assert-ContainsText $runtimeUiBindingSourceText $needle "engine/ui/src/runtime_ui_binding.cpp"
}

Assert-ContainsText $runtimeUiCMakeText "src/runtime_ui_binding.cpp" "engine/ui/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_ui_binding_tests" "CMakeLists.txt"

foreach ($needle in @(
        "runtime ui binding applies typed values to document without executing gameplay commands",
        "runtime ui binding reports missing keys type mismatches and unsafe input routes",
        "runtime ui input routing publishes command focus navigation and capture rows",
        "plan_runtime_ui_binding",
        "plan_runtime_ui_input_routing"
    )) {
    Assert-ContainsText $runtimeUiBindingTestsText $needle "tests/unit/runtime_ui_binding_tests.cpp"
}

foreach ($needle in @(
        "mirakana/ui/runtime_ui_binding.hpp",
        "--require-runtime-ui-binding",
        "RuntimeUiBindingProbeResult",
        "validate_runtime_ui_binding_package_evidence",
        "plan_runtime_ui_binding",
        "runtime_ui_binding_status=",
        "runtime_ui_binding_ready=",
        "runtime_ui_binding_rows=",
        "runtime_ui_command_rows=",
        "runtime_ui_focus_scopes=",
        "runtime_ui_navigation_edges=",
        "runtime_ui_input_routing_ready=",
        "runtime_ui_binding_gameplay_commands_executed=",
        "runtime_ui_binding_diagnostics="
    )) {
    Assert-ContainsText $sample2dMainText $needle "games/sample_2d_desktop_runtime_package/main.cpp"
}

foreach ($needle in @(
        "runtime-ui-binding",
        "installed-2d-runtime-ui-binding-smoke",
        "--require-runtime-ui-binding",
        "runtime_ui_binding_ready=1"
    )) {
    Assert-ContainsText $sample2dManifestText $needle "games/sample_2d_desktop_runtime_package/game.agent.json"
}

foreach ($needle in @(
        'requiresRuntimeUiBinding',
        '--require-runtime-ui-binding',
        'runtime_ui_binding_status',
        'runtime_ui_binding_ready',
        'runtime_ui_binding_rows',
        'runtime_ui_command_rows',
        'runtime_ui_focus_scopes',
        'runtime_ui_navigation_edges',
        'runtime_ui_input_routing_ready',
        'runtime_ui_binding_gameplay_commands_executed',
        'runtime_ui_binding_diagnostics'
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}

foreach ($docsSurface in @(
        @{ Label = "docs/ui.md"; Text = $uiDocsText },
        @{ Label = "docs/current-capabilities.md"; Text = $currentCapabilitiesText },
        @{ Label = "docs/roadmap.md"; Text = $roadmapText },
        @{ Label = "docs/ai-game-development.md"; Text = $aiGameDevelopmentText },
        @{ Label = "docs/superpowers/plans/README.md"; Text = $planRegistryText },
        @{ Label = "docs/superpowers/plans/2026-06-12-first-party-ui-runtime-editor-production-roadmap-v1.md"; Text = $planText }
    )) {
    foreach ($needle in @(
            "Runtime UI Binding And Input Routing v1",
            "RuntimeUiBindingDocument",
            "RuntimeUiBindingRow",
            "RuntimeUiCommandAvailabilityRow",
            "RuntimeUiFocusScope",
            "RuntimeUiNavigationEdge",
            "RuntimeUiInputRoutingPlan",
            "plan_runtime_ui_binding",
            "plan_runtime_ui_input_routing",
            "--require-runtime-ui-binding",
            "runtime_ui_binding_ready",
            "runtime_ui_input_routing_ready",
            "runtime_ui_binding_gameplay_commands_executed=0"
        )) {
        Assert-ContainsText $docsSurface.Text $needle $docsSurface.Label
    }
}

foreach ($needle in @(
        "engine/ui/include/mirakana/ui/runtime_ui_binding.hpp",
        "Runtime UI Binding And Input Routing v1",
        "RuntimeUiBindingDocument",
        "plan_runtime_ui_binding",
        "plan_runtime_ui_input_routing",
        "runtime_ui_binding_ready=1",
        "runtime_ui_input_routing_ready=1"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json"
}

foreach ($needle in @(
        "runtime-ui-binding-input-routing",
        "RuntimeUiBindingDocument",
        "RuntimeUiBindingValueRow",
        "RuntimeUiCommandAvailabilityRow",
        "RuntimeUiFocusScope",
        "RuntimeUiNavigationEdge",
        "RuntimeUiInputRoutingPlan",
        "plan_runtime_ui_binding",
        "plan_runtime_ui_input_routing",
        "--require-runtime-ui-binding",
        "runtime_ui_binding_status=ready",
        "runtime_ui_binding_ready=1",
        "runtime_ui_binding_rows=3",
        "runtime_ui_command_rows=2",
        "runtime_ui_focus_scopes=2",
        "runtime_ui_navigation_edges=3",
        "runtime_ui_input_routing_ready=1",
        "runtime_ui_binding_gameplay_commands_executed=0",
        "runtime_ui_binding_diagnostics=0"
    )) {
    Assert-ContainsText $runtimeBackendReadinessFragmentText $needle "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
}

foreach ($needle in @(
        "runtime-ui-binding-input-routing",
        "installed-2d-runtime-ui-binding-smoke",
        "MK_runtime_ui_binding_tests",
        "--require-runtime-ui-binding",
        "runtime_ui_binding_status/runtime_ui_binding_ready"
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle "engine/agent/manifest.fragments/009-validationRecipes.json"
}

foreach ($needle in @(
        "runtime-ui-binding",
        "runtime-ui-binding-input-routing",
        "installed-2d-runtime-ui-binding-smoke",
        "Runtime UI Binding And Input Routing v1",
        "gameplay command execution"
    )) {
    Assert-ContainsText $productionLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
}

foreach ($needle in @(
        "currentRuntimeUiBindingInputRoutingV1",
        "RuntimeUiBindingDocument",
        "plan_runtime_ui_binding",
        "plan_runtime_ui_input_routing",
        "--require-runtime-ui-binding",
        "runtime_ui_binding_ready=1",
        "runtime_ui_binding_gameplay_commands_executed=0"
    )) {
    Assert-ContainsText $gameCodeGuidanceFragmentText $needle "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
}

foreach ($needle in @(
        "runtime-ui-binding",
        "runtime-ui-binding-input-routing",
        "currentRuntimeUiBindingInputRoutingV1",
        "RuntimeUiBindingDocument",
        "plan_runtime_ui_binding",
        "plan_runtime_ui_input_routing",
        "--require-runtime-ui-binding",
        "runtime_ui_binding_ready=1"
    )) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}
