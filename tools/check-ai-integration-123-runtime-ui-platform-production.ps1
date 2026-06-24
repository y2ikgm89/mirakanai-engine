#requires -Version 7.0
#requires -PSEdition Core

# Chapter 12.3 for check-ai-integration.ps1 First-Party Runtime UI And Editor Platform Production v1 contracts.

$runtimeUiPlatformProductionPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-first-party-runtime-ui-and-editor-platform-production-v1.md"
$runtimeUiCleanRoomLedgerText = Get-AgentSurfaceText "docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md"
$runtimeUiCleanRoomCheckText = Get-AgentSurfaceText "tools/check-first-party-ui-clean-room.ps1"
$runtimeUiPlatformProductionHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/runtime_ui_platform_production.hpp"
$runtimeUiPlatformProductionTestText = Get-AgentSurfaceText "tests/unit/runtime_ui_platform_production_tests.cpp"
$runtimeUiWin32TextHeaderText = Get-AgentSurfaceText "engine/platform/win32/include/mirakana/platform/win32/win32_ui_text_font.hpp"
$runtimeUiWin32TextSourceText = Get-AgentSurfaceText "engine/platform/win32/src/win32_ui_text_font.cpp"
$runtimeUiWin32TextTestText = Get-AgentSurfaceText "tests/unit/win32_ui_text_font_tests.cpp"
$runtimeUiWin32TextInputHeaderText = Get-AgentSurfaceText "engine/platform/win32/include/mirakana/platform/win32/win32_text_input.hpp"
$runtimeUiWin32TextInputSourceText = Get-AgentSurfaceText "engine/platform/win32/src/win32_text_input.cpp"
$runtimeUiWin32AccessibilityHeaderText = Get-AgentSurfaceText "engine/platform/win32/include/mirakana/platform/win32/win32_ui_accessibility.hpp"
$runtimeUiWin32AccessibilitySourceText = Get-AgentSurfaceText "engine/platform/win32/src/win32_ui_accessibility.cpp"
$win32PlatformTestText = Get-AgentSurfaceText "tests/unit/win32_platform_tests.cpp"
$sample2dPackageText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample2dManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$runtimeUiCMakeText = Get-AgentSurfaceText "engine/ui/CMakeLists.txt"
$win32PlatformCMakeText = Get-AgentSurfaceText "engine/platform/win32/CMakeLists.txt"
$gamesCMakeText = Get-AgentSurfaceText "games/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$moduleManifestText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$runtimeBackendReadinessText = Get-AgentSurfaceText "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
$validationRecipesText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$gameCodeGuidanceText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$dependencyDocsText = Get-AgentSurfaceText "docs/dependencies.md"
$legalDocsText = Get-AgentSurfaceText "docs/legal-and-licensing.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"

foreach ($needle in @(
        "First-Party Runtime UI And Editor Platform Production v1",
        "Clean-Room Source Ledger And Static Guard",
        "tools/check-first-party-ui-clean-room.ps1",
        "docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md"
    )) {
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan"
}

foreach ($needle in @(
        "RuntimeUiPlatformProductionFeature",
        "RuntimeUiPlatformProductionProofKind",
        "RuntimeUiPlatformProductionEvidenceRow",
        "RuntimeUiPlatformProductionResult",
        "evaluate_runtime_ui_platform_production"
    )) {
    Assert-ContainsText $runtimeUiPlatformProductionHeaderText $needle "runtime UI platform production public header"
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 2"
}

foreach ($needle in @(
        "visible_ui_editor",
        "production_text_shaping",
        "real_font_loading",
        "font_rasterization",
        "native_ime_session",
        "os_accessibility_publication",
        "renderer_texture_upload_execution",
        "clean_room_provenance",
        "external_engine_parity_non_claim"
    )) {
    Assert-ContainsText $runtimeUiPlatformProductionHeaderText $needle "runtime UI platform production feature enum"
}

foreach ($needle in @(
        "first_party_contract",
        "official_sdk_adapter",
        "audited_dependency_adapter",
        "selected_package_counter",
        "visible_editor_shell",
        "host_gate",
        "dependency_gate",
        "unsupported_non_claim"
    )) {
    Assert-ContainsText $runtimeUiPlatformProductionHeaderText $needle "runtime UI platform production proof enum"
}

foreach ($needle in @(
        "missing_feature_row",
        "duplicate_row_id",
        "public_native_handles",
        "dependency_not_recorded",
        "host_evidence_missing",
        "renderer_upload_missing",
        "external_engine_parity_claim",
        "middleware_api_exposure",
        "copied_external_source",
        "copied_external_asset",
        "row_budget_overflow"
    )) {
    Assert-ContainsText $runtimeUiPlatformProductionHeaderText $needle "runtime UI platform production diagnostics"
    Assert-ContainsText $runtimeUiPlatformProductionTestText $needle "runtime UI platform production tests"
}

foreach ($needle in @(
        "src/runtime_ui_platform_production.cpp",
        "runtime_ui_platform_production.cpp"
    )) {
    Assert-ContainsText $runtimeUiCMakeText $needle "engine/ui CMake runtime UI platform production source"
}

foreach ($needle in @(
        "MK_runtime_ui_platform_production_tests",
        "tests/unit/runtime_ui_platform_production_tests.cpp"
    )) {
    Assert-ContainsText $rootCMakeText $needle "root CMake runtime UI platform production tests"
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan validation"
}

foreach ($needle in @(
        "Win32UiTextShapeRequest",
        "Win32UiTextShapeResult",
        "Win32UiTextGlyphRow",
        "Win32UiTextBoundaryRow",
        "Win32UiTextFallbackFamilyRow",
        "shape_win32_ui_text_with_directwrite",
        "validate_win32_ui_text_shape_result_rows",
        "make_win32_directwrite_text_shaping_production_evidence"
    )) {
    Assert-ContainsText $runtimeUiWin32TextHeaderText $needle "Win32 DirectWrite runtime UI text shaping public header"
    Assert-ContainsText $runtimeUiWin32TextTestText $needle "Win32 DirectWrite runtime UI text shaping tests"
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 3"
}

foreach ($needle in @(
        "IDWriteFactory",
        "IDWriteTextLayout",
        "IDWriteTextAnalyzer",
        "DWriteCreateFactory"
    )) {
    Assert-ContainsText $runtimeUiWin32TextSourceText $needle "Win32 DirectWrite runtime UI text shaping source"
}

foreach ($needle in @(
        "invalid_utf8",
        "missing_font_family",
        "row_budget_exceeded",
        "duplicate_cluster_row",
        "public_native_handles_exposed"
    )) {
    Assert-ContainsText $runtimeUiWin32TextHeaderText $needle "Win32 DirectWrite runtime UI text shaping diagnostics"
    Assert-ContainsText $runtimeUiWin32TextTestText $needle "Win32 DirectWrite runtime UI text shaping tests"
}

foreach ($needle in @(
        "src/win32_ui_text_font.cpp",
        "dwrite"
    )) {
    Assert-ContainsText $win32PlatformCMakeText $needle "Win32 platform CMake DirectWrite runtime UI text shaping"
}

foreach ($needle in @(
        "MK_win32_ui_text_font_tests",
        "tests/unit/win32_ui_text_font_tests.cpp"
    )) {
    Assert-ContainsText $rootCMakeText $needle "root CMake Win32 DirectWrite runtime UI text shaping tests"
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 3 validation"
}

foreach ($needle in @(
        "Win32UiFontSourceKind",
        "Win32UiFontLicenseStatus",
        "Win32UiFontLoadRequest",
        "Win32UiFontFaceRow",
        "Win32UiGlyphRasterRequest",
        "Win32UiGlyphRasterResult",
        "Win32UiFontDiagnostic",
        "load_win32_ui_font_face",
        "rasterize_win32_ui_glyph",
        "validate_win32_ui_font_load_result_rows",
        "validate_win32_ui_glyph_raster_result_rows",
        "make_win32_directwrite_font_loading_production_evidence",
        "make_win32_directwrite_font_rasterization_production_evidence"
    )) {
    Assert-ContainsText $runtimeUiWin32TextHeaderText $needle "Win32 DirectWrite runtime UI font loading/rasterization public header"
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 4"
}

foreach ($needle in @(
        "Win32UiFontLoadRequest",
        "Win32UiGlyphRasterRequest",
        "load_win32_ui_font_face",
        "rasterize_win32_ui_glyph",
        "validate_win32_ui_font_load_result_rows",
        "validate_win32_ui_glyph_raster_result_rows",
        "make_win32_directwrite_font_loading_production_evidence",
        "make_win32_directwrite_font_rasterization_production_evidence"
    )) {
    Assert-ContainsText $runtimeUiWin32TextTestText $needle "Win32 DirectWrite runtime UI font loading/rasterization tests"
}

foreach ($needle in @(
        "GetSystemFontCollection",
        "GetFirstMatchingFont",
        "CreateFontFace",
        "GetDesignGlyphMetrics",
        "CreateGlyphRunAnalysis",
        "CreateAlphaTexture"
    )) {
    Assert-ContainsText $runtimeUiWin32TextSourceText $needle "Win32 DirectWrite runtime UI font loading/rasterization source"
}

foreach ($needle in @(
        "missing_provenance",
        "missing_license_status",
        "unsupported_project_font_asset",
        "missing_face_id",
        "invalid_pixel_size",
        "invalid_atlas_padding",
        "missing_bitmap_pixels",
        "invalid_metrics",
        "atlas_overflow",
        "public_native_handles_exposed"
    )) {
    Assert-ContainsText $runtimeUiWin32TextHeaderText $needle "Win32 DirectWrite runtime UI font loading/rasterization diagnostics"
    Assert-ContainsText $runtimeUiWin32TextTestText $needle "Win32 DirectWrite runtime UI font loading/rasterization tests"
}

foreach ($needle in @(
        "--require-runtime-ui-font-rasterization",
        "runtime_ui_font_loading_rows",
        "runtime_ui_glyph_raster_rows",
        "runtime_ui_glyph_bitmap_rows",
        "runtime_ui_glyph_metrics_rows",
        "runtime_ui_font_license_rows",
        "runtime_ui_font_native_handles_exposed",
        "runtime_ui_font_rasterization_diagnostics"
    )) {
    Assert-ContainsText $sample2dPackageText $needle "sample_2d_desktop_runtime_package runtime UI font rasterization counters"
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 4 package counters"
}

foreach ($needle in @(
        "Win32TsfTextSessionDesc",
        "Win32TsfCompositionRow",
        "Win32TsfCandidateIntentRow",
        "Win32TsfTextAreaRow",
        "Win32TsfTextSessionResult",
        "plan_win32_tsf_text_session",
        "make_win32_tsf_native_ime_production_evidence"
    )) {
    Assert-ContainsText $runtimeUiWin32TextInputHeaderText $needle "Win32 TSF runtime UI native IME public header"
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 5"
}

foreach ($needle in @(
        "plan_win32_tsf_text_session",
        "make_win32_tsf_native_ime_production_evidence",
        "RuntimeUiPlatformProductionFeature::native_ime_session"
    )) {
    Assert-ContainsText $win32PlatformTestText $needle "Win32 TSF runtime UI native IME tests"
}

foreach ($needle in @(
        "ITfThreadMgr",
        "ITfDocumentMgr",
        "ITextStoreACP",
        "ITfContextOwnerCompositionSink",
        "CLSID_TF_ThreadMgr",
        "CreateDocumentMgr",
        "CreateContext",
        "RequestLock"
    )) {
    Assert-ContainsText $runtimeUiWin32TextInputSourceText $needle "Win32 TSF runtime UI native IME source"
}

foreach ($needle in @(
        "missing_active_target",
        "invalid_caret_rect",
        "invalid_surrounding_text",
        "composition_update_without_begin",
        "committed_text_target_mismatch",
        "candidate_rows_without_session",
        "public_native_handles_exposed",
        "broad_ime_parity_claim"
    )) {
    Assert-ContainsText $runtimeUiWin32TextInputHeaderText $needle "Win32 TSF runtime UI native IME diagnostics"
    Assert-ContainsText $win32PlatformTestText $needle "Win32 TSF runtime UI native IME tests"
}

foreach ($needle in @(
        "tsf_thread_manager_unavailable",
        "tsf_document_manager_unavailable",
        "tsf_context_unavailable"
    )) {
    Assert-ContainsText $runtimeUiWin32TextInputHeaderText $needle "Win32 TSF runtime UI native IME diagnostics"
}

foreach ($needle in @(
        "--require-runtime-ui-tsf-session",
        "runtime_ui_tsf_session_ready",
        "runtime_ui_tsf_composition_rows",
        "runtime_ui_tsf_candidate_intent_rows",
        "runtime_ui_tsf_text_area_rows",
        "runtime_ui_ime_native_candidate_ui_ready",
        "runtime_ui_ime_cross_platform_ready",
        "runtime_ui_tsf_diagnostics"
    )) {
    Assert-ContainsText $sample2dPackageText $needle "sample_2d_desktop_runtime_package runtime UI TSF native IME counters"
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 5 package counters"
    Assert-ContainsText $sample2dManifestText $needle "sample_2d_desktop_runtime_package runtime UI TSF native IME recipe"
}

foreach ($needle in @(
        "runtime-ui-win32-tsf-native-ime-session",
        "native_ime_session"
    )) {
    Assert-ContainsText $runtimeBackendReadinessText $needle "runtime backend readiness manifest runtime UI TSF native IME"
    Assert-ContainsText $validationRecipesText $needle "validation recipes manifest runtime UI TSF native IME"
}

Assert-ContainsText $runtimeBackendReadinessText "Runtime UI Windows TSF Native IME Session v1" "runtime backend readiness manifest runtime UI TSF native IME"
Assert-ContainsText $validationRecipesText "selected Windows TSF runtime UI native IME session evidence" "validation recipes manifest runtime UI TSF native IME"
Assert-ContainsText $gameCodeGuidanceText "currentRuntimeUiWin32TsfNativeImeSession" "game code guidance manifest runtime UI TSF native IME"
Assert-ContainsText $gameCodeGuidanceText "plan_win32_tsf_text_session" "game code guidance manifest runtime UI TSF native IME"
Assert-ContainsText $dependencyDocsText "Windows TSF runtime UI IME session adapter" "docs/dependencies.md runtime UI TSF adapter"
Assert-ContainsText $legalDocsText "Windows TSF native IME session adapter" "docs/legal-and-licensing.md runtime UI TSF adapter"
Assert-ContainsText $currentCapabilitiesText "Runtime UI Windows TSF Native IME Session v1" "docs/current-capabilities.md"
Assert-ContainsText $planRegistryText "selected Windows TSF native IME session evidence" "docs/superpowers/plans/README.md"

foreach ($needle in @(
        "Win32UiaRuntimeNodeRow",
        "Win32UiaProviderPublicationDesc",
        "Win32UiaProviderPublicationResult",
        "publish_runtime_ui_to_win32_uia",
        "make_win32_uia_accessibility_publication_production_evidence"
    )) {
    Assert-ContainsText $runtimeUiWin32AccessibilityHeaderText $needle "Win32 UIA runtime UI accessibility publication public header"
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 6"
}

foreach ($needle in @(
        "publish_runtime_ui_to_win32_uia",
        "make_win32_uia_accessibility_publication_production_evidence",
        "RuntimeUiPlatformProductionFeature::os_accessibility_publication"
    )) {
    Assert-ContainsText $win32PlatformTestText $needle "Win32 UIA runtime UI accessibility publication tests"
}

foreach ($needle in @(
        "IRawElementProviderSimple",
        "IInvokeProvider",
        "GetPatternProvider",
        "GetPropertyValue",
        "UiaClientsAreListening",
        "UiaRaiseAutomationEvent",
        "UIA_InvokePatternId",
        "ProviderOptions_ServerSideProvider"
    )) {
    Assert-ContainsText $runtimeUiWin32AccessibilitySourceText $needle "Win32 UIA runtime UI accessibility publication source"
}

foreach ($needle in @(
        "missing_accessible_name",
        "duplicate_runtime_id",
        "invalid_bounds",
        "focusable_without_action_pattern",
        "child_without_parent",
        "unsupported_pattern_claim",
        "event_claim_without_provider_root",
        "public_native_handles_exposed",
        "broad_accessibility_parity_claim"
    )) {
    Assert-ContainsText $runtimeUiWin32AccessibilityHeaderText $needle "Win32 UIA runtime UI accessibility publication diagnostics"
    Assert-ContainsText $win32PlatformTestText $needle "Win32 UIA runtime UI accessibility publication tests"
}

foreach ($needle in @(
        "src/win32_ui_accessibility.cpp",
        "uiautomationcore",
        "oleaut32"
    )) {
    Assert-ContainsText $win32PlatformCMakeText $needle "Win32 platform CMake UIA runtime UI accessibility publication"
}

foreach ($needle in @(
        "--require-runtime-ui-uia-publication",
        "runtime_ui_uia_provider_ready",
        "runtime_ui_accessibility_nodes",
        "runtime_ui_accessibility_action_rows",
        "runtime_ui_accessibility_event_rows",
        "runtime_ui_accessibility_cross_platform_ready",
        "runtime_ui_accessibility_native_handles_exposed",
        "runtime_ui_uia_diagnostics"
    )) {
    Assert-ContainsText $sample2dPackageText $needle "sample_2d_desktop_runtime_package runtime UI UIA accessibility publication counters"
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 6 package counters"
    Assert-ContainsText $sample2dManifestText $needle "sample_2d_desktop_runtime_package runtime UI UIA accessibility publication recipe"
}

foreach ($needle in @(
        "runtime-ui-win32-uia-accessibility-publication",
        "os_accessibility_publication"
    )) {
    Assert-ContainsText $runtimeBackendReadinessText $needle "runtime backend readiness manifest runtime UI UIA accessibility publication"
    Assert-ContainsText $validationRecipesText $needle "validation recipes manifest runtime UI UIA accessibility publication"
}

Assert-ContainsText $runtimeBackendReadinessText "Runtime UI Windows UIA Accessibility Publication v1" "runtime backend readiness manifest runtime UI UIA accessibility publication"
Assert-ContainsText $validationRecipesText "selected Windows UIA runtime UI accessibility publication evidence" "validation recipes manifest runtime UI UIA accessibility publication"
Assert-ContainsText $moduleManifestText "win32_ui_accessibility.hpp" "module manifest MK_platform_win32 public header"
Assert-ContainsText $gameCodeGuidanceText "currentRuntimeUiWin32UiaAccessibilityPublication" "game code guidance manifest runtime UI UIA accessibility publication"
Assert-ContainsText $gameCodeGuidanceText "publish_runtime_ui_to_win32_uia" "game code guidance manifest runtime UI UIA accessibility publication"
Assert-ContainsText $dependencyDocsText "Windows UI Automation runtime UI accessibility adapter" "docs/dependencies.md runtime UI UIA adapter"
Assert-ContainsText $legalDocsText "Windows UI Automation runtime UI accessibility adapter" "docs/legal-and-licensing.md runtime UI UIA adapter"
Assert-ContainsText $currentCapabilitiesText "Runtime UI Windows UIA Accessibility Publication v1" "docs/current-capabilities.md"
Assert-ContainsText $planRegistryText "selected Windows UIA accessibility publication evidence" "docs/superpowers/plans/README.md"

Assert-ContainsText $gamesCMakeText "MK_platform_win32" "games CMake sample runtime UI font rasterization link"

foreach ($section in @(
        "## Allowed Sources",
        "## Forbidden Inputs",
        "## Forbidden Public Names",
        "## Dependency Entry Gate",
        "## Trademark And Marketing Non-Claims",
        "## Review Evidence"
    )) {
    Assert-ContainsText $runtimeUiCleanRoomLedgerText $section "first-party UI clean-room source ledger"
}

foreach ($needle in @(
        "official Unity/Unreal/Godot category docs only",
        "Microsoft SDK docs",
        "Apple SDK docs",
        "HarfBuzz docs",
        "FreeType docs",
        "AT-SPI2 docs",
        "Vulkan docs",
        "W3C accessibility practices",
        "repository-owned code"
    )) {
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 1"
}

foreach ($needle in @(
        "Unity official category documentation",
        "Unreal Engine official category documentation",
        "Godot official category documentation",
        "Microsoft SDK documentation",
        "Apple SDK documentation",
        "HarfBuzz documentation",
        "FreeType documentation",
        "AT-SPI2 documentation",
        "Vulkan documentation",
        "W3C accessibility practices",
        "Repository-owned code"
    )) {
    Assert-ContainsText $runtimeUiCleanRoomLedgerText $needle "first-party UI clean-room source ledger"
}

foreach ($token in @(
        "UXML",
        "USS",
        "UMG",
        "Slate",
        "WidgetBlueprint",
        "UnityEngine",
        "UnityEditor",
        "Godot",
        "CanvasLayer",
        "ControlNode",
        "UnrealEd",
        "BlueprintGraph",
        "DearImGui",
        "ImGui",
        "RmlUi",
        "Noesis",
        "Slint",
        "Qt"
    )) {
    Assert-ContainsText $runtimeUiCleanRoomLedgerText $token "first-party UI clean-room source ledger forbidden public names"
    Assert-ContainsText $runtimeUiCleanRoomCheckText "`"$token`"" "tools/check-first-party-ui-clean-room.ps1 forbidden token list"
}

foreach ($needle in @(
        "Default themes, fonts, icons, screenshots",
        "Unity UXML/USS imports",
        "Unreal UMG/Widget Blueprint/Slate imports",
        "Godot scene/control tree imports",
        "Visual parity, API parity, workflow parity"
    )) {
    Assert-ContainsText $runtimeUiCleanRoomLedgerText $needle "first-party UI clean-room source ledger forbidden inputs"
}

Assert-ContainsText $dependencyDocsText "First-party runtime UI clean-room source gate" "docs/dependencies.md runtime UI clean-room gate"
Assert-ContainsText $legalDocsText "First-Party Runtime UI Clean-Room Source Gate" "docs/legal-and-licensing.md runtime UI clean-room gate"
foreach ($needle in @(
        "docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md",
        "tools/check-first-party-ui-clean-room.ps1"
    )) {
    Assert-ContainsText $dependencyDocsText $needle "docs/dependencies.md runtime UI clean-room gate"
    Assert-ContainsText $legalDocsText $needle "docs/legal-and-licensing.md runtime UI clean-room gate"
}

Assert-ContainsText $currentCapabilitiesText "First-Party UI Clean-Room Source Ledger v1" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "Runtime UI Platform Production Gate v1" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "Runtime UI Windows DirectWrite Text And Font Adapter v1" "docs/current-capabilities.md"
Assert-ContainsText $runtimeBackendReadinessText "runtime-ui-platform-production-gate" "runtime backend readiness manifest"
Assert-ContainsText $runtimeBackendReadinessText "runtime-ui-win32-directwrite-text-shaping" "runtime backend readiness manifest"
Assert-ContainsText $runtimeBackendReadinessText "runtime-ui-win32-directwrite-font-rasterization" "runtime backend readiness manifest"
Assert-ContainsText $validationRecipesText "runtime-ui-win32-directwrite-text-shaping" "validation recipes manifest"
Assert-ContainsText $validationRecipesText "runtime-ui-win32-directwrite-font-rasterization" "validation recipes manifest"
Assert-ContainsText $moduleManifestText "win32_ui_text_font.hpp" "module manifest MK_platform_win32 public header"
Assert-ContainsText $gameCodeGuidanceText "currentRuntimeUiPlatformProductionGate" "game code guidance manifest"
Assert-ContainsText $gameCodeGuidanceText "shape_win32_ui_text_with_directwrite" "game code guidance manifest runtime UI Win32 DirectWrite"
Assert-ContainsText $gameCodeGuidanceText "currentRuntimeUiWin32DirectWriteFontRasterization" "game code guidance manifest runtime UI Win32 DirectWrite font rasterization"
Assert-ContainsText $gameCodeGuidanceText "rasterize_win32_ui_glyph" "game code guidance manifest runtime UI Win32 DirectWrite font rasterization"
Assert-ContainsText $dependencyDocsText "DirectWrite runtime UI text/font adapter" "docs/dependencies.md runtime UI DirectWrite adapter"
Assert-ContainsText $legalDocsText "runtime UI Windows DirectWrite text/font adapter" "docs/legal-and-licensing.md runtime UI DirectWrite adapter"
Assert-ContainsText $currentCapabilitiesText "first-party-ui-clean-room: ok" "docs/current-capabilities.md"
Assert-ContainsText $planRegistryText "First-Party Runtime UI And Editor Platform Production v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "selected Windows DirectWrite runtime UI text-shaping evidence" "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "selected Windows DirectWrite system-font loading and alpha8 glyph-rasterization evidence" "docs/superpowers/plans/README.md"

$cleanRoomCheckScriptPath = Resolve-RequiredAgentPath "tools/check-first-party-ui-clean-room.ps1"
& $cleanRoomCheckScriptPath
