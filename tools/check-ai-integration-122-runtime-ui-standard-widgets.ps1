#requires -Version 7.0
#requires -PSEdition Core

# Chapter 12.2 for check-ai-integration.ps1 First-Party Runtime UI Standard Widgets v1 contracts.

$runtimeUiStandardWidgetsHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/runtime_ui_standard_widgets.hpp"
$runtimeUiStandardWidgetsSourceText = Get-AgentSurfaceText "engine/ui/src/runtime_ui_standard_widgets.cpp"
$runtimeUiPublicHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/ui.hpp"
$runtimeUiCMakeText = Get-AgentSurfaceText "engine/ui/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeUiStandardWidgetsTestsText = Get-AgentSurfaceText "tests/unit/runtime_ui_standard_widgets_tests.cpp"
$sample2dMainText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample3dMainText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/main.cpp"
$sample2dManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample3dManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$uiDocsText = Get-AgentSurfaceText "docs/ui.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-first-party-runtime-ui-standard-widgets-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$runtimeBackendReadinessFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$gameCodeGuidanceFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

function Assert-NoRuntimeUiExternalPublicToken {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($token in @(
            "uGUI",
            "UMG",
            "Slate",
            "Widget Blueprint",
            "Control",
            "CanvasLayer",
            "UIDocument",
            "VisualElement",
            "UXML",
            "USS",
            "Blueprint"
        )) {
        $pattern = "(?<![A-Za-z0-9_])$([Regex]::Escape($token))(?![A-Za-z0-9_])"
        if ($Text -cmatch $pattern) {
            Write-Error "$Label contained forbidden external runtime UI public token: $token"
        }
    }
}

foreach ($needle in @(
        "RuntimeUiSourceReferenceKind",
        "RuntimeUiStandardWidgetProvenanceDesc",
        "RuntimeUiStandardWidgetProvenancePlan",
        "review_runtime_ui_standard_widget_provenance",
        "RuntimeUiMeterKind",
        "RuntimeUiMeterFillDirection",
        "RuntimeUiMeterDesc",
        "RuntimeUiMeterRow",
        "RuntimeUiMeterPlan",
        "plan_runtime_ui_meters",
        "build_runtime_ui_meter_document",
        "RuntimeUiMenuActionIntent",
        "RuntimeUiMenuActionDesc",
        "RuntimeUiMenuScreenDesc",
        "RuntimeUiMenuStackDesc",
        "RuntimeUiMenuStackPlan",
        "plan_runtime_ui_menu_stack",
        "build_runtime_ui_menu_stack_document",
        "RuntimeUiStandardHudDesc",
        "RuntimeUiStandardHudPlan",
        "plan_runtime_ui_standard_hud"
    )) {
    Assert-ContainsText $runtimeUiStandardWidgetsHeaderText $needle "engine/ui/include/mirakana/ui/runtime_ui_standard_widgets.hpp"
}

foreach ($needle in @(
        "copied_external_expression",
        "third_party_code_without_notice",
        "third_party_asset_without_notice",
        "marketplace_asset_without_review",
        "external_engine_public_name",
        "unsupported_ui_middleware_reference"
    )) {
    Assert-ContainsText $runtimeUiStandardWidgetsHeaderText $needle "engine/ui/include/mirakana/ui/runtime_ui_standard_widgets.hpp"
}

foreach ($needle in @(
        "contains_external_engine_public_token",
        "contains_bounded_token",
        "uGUI",
        "UMG",
        "Slate",
        "Widget Blueprint",
        "Control",
        "CanvasLayer",
        "UIDocument",
        "VisualElement",
        "RuntimeUiProvenanceDiagnosticCode::copied_external_expression",
        "RuntimeUiProvenanceDiagnosticCode::third_party_code_without_notice",
        "RuntimeUiProvenanceDiagnosticCode::third_party_asset_without_notice",
        "RuntimeUiProvenanceDiagnosticCode::marketplace_asset_without_review",
        "RuntimeUiProvenanceDiagnosticCode::external_engine_public_name",
        "RuntimeUiProvenanceDiagnosticCode::unsupported_ui_middleware_reference"
    )) {
    Assert-ContainsText $runtimeUiStandardWidgetsSourceText $needle "engine/ui/src/runtime_ui_standard_widgets.cpp"
}

Assert-ContainsText $runtimeUiPublicHeaderText "meter" "engine/ui/include/mirakana/ui/ui.hpp"
Assert-ContainsText $runtimeUiCMakeText "src/runtime_ui_standard_widgets.cpp" "engine/ui/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_ui_standard_widgets_tests" "CMakeLists.txt"

foreach ($publicTokenSurface in @(
        @{ Label = "engine/ui/include/mirakana/ui/runtime_ui_standard_widgets.hpp"; Text = $runtimeUiStandardWidgetsHeaderText },
        @{ Label = "games/sample_2d_desktop_runtime_package/main.cpp"; Text = $sample2dMainText },
        @{ Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp"; Text = $sample3dMainText },
        @{ Label = "games/sample_2d_desktop_runtime_package/game.agent.json"; Text = $sample2dManifestText },
        @{ Label = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"; Text = $sample3dManifestText },
        @{ Label = "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"; Text = $runtimeBackendReadinessFragmentText },
        @{ Label = "engine/agent/manifest.fragments/014-gameCodeGuidance.json"; Text = $gameCodeGuidanceFragmentText },
        @{ Label = "engine/agent/manifest.json"; Text = $manifestText }
    )) {
    Assert-NoRuntimeUiExternalPublicToken -Text $publicTokenSurface.Text -Label $publicTokenSurface.Label
}

foreach ($needle in @(
        "accept official docs and first party design only",
        "reject copied external expression",
        "reject external engine code and assets",
        "reject external public UI names and middleware",
        "bound external public UI name matching",
        "plan health mana and stamina meters",
        "mark depleted meter",
        "reject invalid and duplicate meter ids",
        "build meter document with accessibility labels",
        "plan pause menu stack",
        "reject invalid menu stack",
        "compose standard hud only with clean inputs",
        "reject dirty standard hud provenance"
    )) {
    Assert-ContainsText $runtimeUiStandardWidgetsTestsText $needle "tests/unit/runtime_ui_standard_widgets_tests.cpp"
}

foreach ($sampleSurface in @(
        @{ Label = "games/sample_2d_desktop_runtime_package/main.cpp"; Text = $sample2dMainText },
        @{ Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp"; Text = $sample3dMainText }
    )) {
    foreach ($needle in @(
            "mirakana/ui/runtime_ui_standard_widgets.hpp",
            "--require-runtime-ui-standard-widgets",
            "RuntimeUiStandardWidgetsProbeResult",
            "make_runtime_ui_standard_widgets_provenance",
            "make_runtime_ui_standard_widget_meters",
            "make_runtime_ui_standard_widget_menu_stack",
            "validate_runtime_ui_standard_widgets_package_evidence",
            "plan_runtime_ui_standard_hud",
            "runtime_ui_standard_widgets_ready=",
            "runtime_ui_standard_widgets_provenance_ready=",
            "runtime_ui_standard_widgets_official_documentation_rows=",
            "runtime_ui_standard_widgets_first_party_design_rows=",
            "runtime_ui_standard_widgets_meter_rows=",
            "runtime_ui_standard_widgets_menu_screens=",
            "runtime_ui_standard_widgets_menu_actions=",
            "runtime_ui_standard_widgets_document_elements=",
            "runtime_ui_meter_health_normalized=",
            "runtime_ui_meter_mana_normalized=",
            "runtime_ui_meter_stamina_normalized=",
            "runtime_ui_standard_widgets_health_accessibility_ready=",
            "runtime_ui_standard_widgets_mana_localization_ready=",
            "runtime_ui_standard_widgets_stamina_warning_style_ready=",
            "runtime_ui_standard_widgets_external_engine_code=",
            "runtime_ui_standard_widgets_external_engine_assets=",
            "runtime_ui_standard_widgets_ui_middleware=",
            "runtime_ui_standard_widgets_diagnostics="
        )) {
        Assert-ContainsText $sampleSurface.Text $needle $sampleSurface.Label
    }
}

foreach ($manifestSurface in @(
        @{ Label = "games/sample_2d_desktop_runtime_package/game.agent.json"; Text = $sample2dManifestText; Recipe = "installed-2d-runtime-ui-standard-widgets-smoke" },
        @{ Label = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"; Text = $sample3dManifestText; Recipe = "installed-3d-runtime-ui-standard-widgets-smoke" }
    )) {
    foreach ($needle in @(
            "runtime-ui-standard-widgets",
            $manifestSurface.Recipe,
            "--require-runtime-ui-standard-widgets",
            "runtime_ui_standard_widgets"
        )) {
        Assert-ContainsText $manifestSurface.Text $needle $manifestSurface.Label
    }
}

foreach ($needle in @(
        'requiresRuntimeUiStandardWidgets',
        '--require-runtime-ui-standard-widgets',
        'runtime_ui_standard_widgets_ready',
        'runtime_ui_standard_widgets_provenance_ready',
        'runtime_ui_standard_widgets_official_documentation_rows',
        'runtime_ui_standard_widgets_first_party_design_rows',
        'runtime_ui_standard_widgets_meter_rows',
        'runtime_ui_standard_widgets_menu_screens',
        'runtime_ui_standard_widgets_menu_actions',
        'runtime_ui_standard_widgets_document_elements',
        'runtime_ui_meter_health_normalized',
        'runtime_ui_meter_mana_normalized',
        'runtime_ui_meter_stamina_normalized',
        'runtime_ui_standard_widgets_health_accessibility_ready',
        'runtime_ui_standard_widgets_mana_localization_ready',
        'runtime_ui_standard_widgets_stamina_warning_style_ready',
        'runtime_ui_standard_widgets_external_engine_code',
        'runtime_ui_standard_widgets_external_engine_assets',
        'runtime_ui_standard_widgets_ui_middleware',
        'runtime_ui_standard_widgets_diagnostics'
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}

foreach ($docsSurface in @(
        @{ Label = "docs/ui.md"; Text = $uiDocsText },
        @{ Label = "docs/current-capabilities.md"; Text = $currentCapabilitiesText },
        @{ Label = "docs/roadmap.md"; Text = $roadmapText },
        @{ Label = "docs/ai-game-development.md"; Text = $aiGameDevelopmentText },
        @{ Label = "docs/superpowers/plans/README.md"; Text = $planRegistryText },
        @{ Label = "docs/superpowers/plans/2026-06-21-first-party-runtime-ui-standard-widgets-v1.md"; Text = $planText }
    )) {
    foreach ($needle in @(
            "First-Party Runtime UI Standard Widgets v1",
            "UI middleware"
        )) {
        Assert-ContainsText $docsSurface.Text $needle $docsSurface.Label
    }
}

foreach ($docsSurface in @(
        @{ Label = "docs/ui.md"; Text = $uiDocsText },
        @{ Label = "docs/roadmap.md"; Text = $roadmapText },
        @{ Label = "docs/ai-game-development.md"; Text = $aiGameDevelopmentText },
        @{ Label = "docs/superpowers/plans/2026-06-21-first-party-runtime-ui-standard-widgets-v1.md"; Text = $planText }
    )) {
    Assert-ContainsText $docsSurface.Text "--require-runtime-ui-standard-widgets" $docsSurface.Label
}

foreach ($docsSurface in @(
        @{ Label = "docs/ai-game-development.md"; Text = $aiGameDevelopmentText },
        @{ Label = "docs/superpowers/plans/2026-06-21-first-party-runtime-ui-standard-widgets-v1.md"; Text = $planText }
    )) {
    Assert-ContainsText $docsSurface.Text "runtime_ui_standard_widgets_ready" $docsSurface.Label
}

foreach ($needle in @(
        "engine/ui/include/mirakana/ui/runtime_ui_standard_widgets.hpp",
        "RuntimeUiStandardWidgetProvenanceDesc",
        "plan_runtime_ui_standard_hud",
        "external-engine-code"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json"
}

foreach ($needle in @(
        "runtime-ui-standard-widgets",
        "RuntimeUiStandardWidgetProvenanceDesc",
        "RuntimeUiMeterDesc",
        "RuntimeUiMenuStackDesc",
        "RuntimeUiStandardHudDesc",
        "review_runtime_ui_standard_widget_provenance",
        "plan_runtime_ui_standard_hud",
        "--require-runtime-ui-standard-widgets",
        "runtime_ui_standard_widgets_ready=1",
        "runtime_ui_standard_widgets_external_engine_code=0",
        "runtime_ui_standard_widgets_external_engine_assets=0",
        "runtime_ui_standard_widgets_ui_middleware=0"
    )) {
    Assert-ContainsText $runtimeBackendReadinessFragmentText $needle "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
}

foreach ($needle in @(
        "runtime-ui-standard-widgets",
        "installed-2d-runtime-ui-standard-widgets-smoke",
        "installed-3d-runtime-ui-standard-widgets-smoke"
    )) {
    Assert-ContainsText $productionLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
}

foreach ($needle in @(
        "currentRuntimeUiStandardWidgetsV1",
        "RuntimeUiStandardWidgetProvenanceDesc",
        "plan_runtime_ui_standard_hud",
        "--require-runtime-ui-standard-widgets",
        "runtime_ui_standard_widgets_ready=1",
        "Unity, Unreal Engine, and Godot documentation may be recorded only as category research"
    )) {
    Assert-ContainsText $gameCodeGuidanceFragmentText $needle "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
}

foreach ($needle in @(
        "runtime-ui-standard-widgets",
        "currentRuntimeUiStandardWidgetsV1",
        "RuntimeUiStandardWidgetProvenanceDesc",
        "plan_runtime_ui_standard_hud",
        "--require-runtime-ui-standard-widgets",
        "runtime_ui_standard_widgets_ready=1"
    )) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}
