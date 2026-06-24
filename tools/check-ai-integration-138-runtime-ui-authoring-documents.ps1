#requires -Version 7.0
#requires -PSEdition Core

# Chapter 13.8 for check-ai-integration.ps1 First-Party UI Authoring Documents v1 contracts.

$runtimeUiAuthoringHeader = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/runtime_ui_authoring.hpp"
$runtimeUiAuthoringSource = Get-AgentSurfaceText "engine/ui/src/runtime_ui_authoring.cpp"
$uiCMake = Get-AgentSurfaceText "engine/ui/CMakeLists.txt"
$rootCMake = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeUiAuthoringTests = Get-AgentSurfaceText "tests/unit/runtime_ui_authoring_tests.cpp"
$sampleManifest = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sampleUiDocument = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/runtime/ui/main_menu.uidoc"
$sampleUiTheme = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/runtime/ui/default.uitheme"
$uiDocs = Get-AgentSurfaceText "docs/ui.md"
$capabilitiesDocs = Get-AgentSurfaceText "docs/current-capabilities.md"
$aiGameDocs = Get-AgentSurfaceText "docs/ai-game-development.md"
$roadmapDocs = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistry = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$modulesFragment = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$readinessFragment = Get-AgentSurfaceText "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
$recipesFragment = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$guidanceFragment = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$composedManifest = Get-AgentSurfaceText "engine/agent/manifest.json"

function Assert-TextContainsAll {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string[]]$Needles
    )

    foreach ($needle in $Needles) {
        Assert-ContainsText $Text $needle $Name
    }
}

Assert-TextContainsAll `
    -Name "runtime UI authoring header exposes first-party document contracts" `
    -Text $runtimeUiAuthoringHeader `
    -Needles @(
        "RuntimeUiAuthoringDiagnosticCode",
        "RuntimeUiThemeTokenKind",
        "RuntimeUiAuthoringDiagnostic",
        "RuntimeUiAuthoredElementRow",
        "RuntimeUiAuthoredWidgetRow",
        "RuntimeUiAuthoredBindingRow",
        "RuntimeUiThemeTokenRow",
        "RuntimeUiAuthoringDocument",
        "RuntimeUiThemeDocument",
        "RuntimeUiAuthoringDocumentParseResult",
        "RuntimeUiThemeParseResult",
        "runtime_ui_theme_token_kind_name",
        "parse_runtime_ui_document",
        "write_runtime_ui_document",
        "parse_runtime_ui_theme",
        "write_runtime_ui_theme"
    )

Assert-TextContainsAll `
    -Name "runtime UI authoring source keeps deterministic first-party formats and fail-closed diagnostics" `
    -Text $runtimeUiAuthoringSource `
    -Needles @(
        "GameEngine.UiDocument.v1",
        "GameEngine.UiTheme.v1",
        "duplicate_element_id",
        "unknown_parent_id",
        "invalid_theme_token",
        "unsafe_token",
        "missing_localization_key",
        "missing_accessibility_name",
        "unknown_widget_id",
        "write_runtime_ui_document",
        "write_runtime_ui_theme",
        '"unity"',
        '"unreal"',
        '"godot"',
        '"uxml"',
        '"uss"',
        '"umg"',
        '"slate"',
        '"hwnd"',
        '"native_handle"'
    )

Assert-TextContainsAll `
    -Name "runtime UI authoring build and test targets are registered" `
    -Text ($uiCMake + "`n" + $rootCMake) `
    -Needles @(
        "src/runtime_ui_authoring.cpp",
        "MK_runtime_ui_authoring_tests",
        "*.uidoc",
        "*.uitheme"
    )

Assert-TextContainsAll `
    -Name "runtime UI authoring tests cover parse/write and fail-closed gates" `
    -Text $runtimeUiAuthoringTests `
    -Needles @(
        "runtime ui authoring parses and writes deterministic first-party document and theme files",
        "runtime ui authoring fails closed for document identity hierarchy metadata and widget errors",
        "runtime ui authoring rejects unsafe native middleware path and external engine tokens",
        "runtime ui authoring rejects invalid theme tokens",
        "RuntimeUiAuthoringDiagnosticCode::duplicate_element_id",
        "RuntimeUiAuthoringDiagnosticCode::unknown_parent_id",
        "RuntimeUiAuthoringDiagnosticCode::invalid_theme_token",
        "RuntimeUiAuthoringDiagnosticCode::unsafe_token",
        "RuntimeUiAuthoringDiagnosticCode::missing_localization_key",
        "RuntimeUiAuthoringDiagnosticCode::missing_accessibility_name",
        "RuntimeUiAuthoringDiagnosticCode::unknown_widget_id"
    )

Assert-TextContainsAll `
    -Name "sample package manifests first-party UI authoring files" `
    -Text ($sampleManifest + "`n" + $sampleUiDocument + "`n" + $sampleUiTheme) `
    -Needles @(
        "runtime/ui/main_menu.uidoc",
        "runtime/ui/default.uitheme",
        "GameEngine.UiDocument.v1",
        "GameEngine.UiTheme.v1"
    )

Assert-TextContainsAll `
    -Name "runtime UI authoring docs and plan registry describe scope and non-claims" `
    -Text ($uiDocs + "`n" + $capabilitiesDocs + "`n" + $aiGameDocs + "`n" + $roadmapDocs + "`n" + $planRegistry) `
    -Needles @(
        "First-Party UI Authoring Documents v1",
        "RuntimeUiAuthoringDocument",
        "RuntimeUiThemeDocument",
        "GameEngine.UiDocument.v1",
        "GameEngine.UiTheme.v1",
        "runtime/ui/main_menu.uidoc",
        "runtime/ui/default.uitheme",
        "Unity/Unreal/Godot",
        "visible UI editor",
        "renderer upload",
        "text shaping",
        "font rasterization",
        "native IME",
        "OS accessibility publication"
    )

Assert-TextContainsAll `
    -Name "runtime UI authoring manifest fragments and composed manifest are updated" `
    -Text ($modulesFragment + "`n" + $readinessFragment + "`n" + $recipesFragment + "`n" + $guidanceFragment + "`n" + $composedManifest) `
    -Needles @(
        "runtime_ui_authoring.hpp",
        "runtime-ui-authoring-documents",
        "First-Party UI Authoring Documents v1",
        "RuntimeUiAuthoringDocument",
        "RuntimeUiThemeDocument",
        "GameEngine.UiDocument.v1",
        "GameEngine.UiTheme.v1",
        "runtime/ui/main_menu.uidoc",
        "runtime/ui/default.uitheme",
        "Unity/Unreal/Godot",
        "visible editor execution",
        "renderer upload",
        "native handle"
    )
