#requires -Version 7.0
#requires -PSEdition Core

# Chapter 149 for check-ai-integration.ps1 static contracts.
# Runtime 2D Commercial Input Closeout v1 package/manifest/plan alignment.

$inputCloseoutHeader = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/two_d_commercial_input_closeout.hpp"
$inputCloseoutSource = Get-AgentSurfaceText "engine/runtime/src/two_d_commercial_input_closeout.cpp"
$inputCloseoutTests = Get-AgentSurfaceText "tests/unit/runtime_2d_commercial_input_closeout_tests.cpp"
$rootCMake = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeCMake = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$sample2dMain = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample2dManifest = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$validatorScript = Get-AgentSurfaceText "tools/validate-2d-commercial-input-closeout.ps1"
$validationRecipesManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$gameCodeGuidanceManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$composedManifest = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilities = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmap = Get-AgentSurfaceText "docs/roadmap.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"

foreach ($needle in @(
        "Runtime2DCommercialInputOfficialSourceKind",
        "Runtime2DCommercialInputOfficialSourceRow",
        "Runtime2DCommercialInputNavigationRow",
        "Runtime2DCommercialInputCloseoutDesc",
        "Runtime2DCommercialInputCloseoutResult",
        "evaluate_runtime_2d_commercial_input_closeout",
        "microsoft_gameinput",
        "microsoft_raw_input",
        "microsoft_pointer_input",
        "microsoft_uia",
        "repository_legal_policy",
        "legal_approval_claim"
    )) {
    Assert-ContainsText $inputCloseoutHeader $needle "two_d_commercial_input_closeout.hpp"
}

foreach ($needle in @(
        "serialize_runtime_input_actions",
        "apply_runtime_input_rebinding_profile",
        "make_runtime_input_rebinding_presentation",
        "RuntimeInputDeviceProductionUxDiagnosticCode::glyph_rendering_requested",
        "accessibility_navigation_not_ready",
        "official_source_not_ready",
        "external_engine_compatibility_claim",
        "cross_platform_parity_claim",
        "legal_approval_claim"
    )) {
    Assert-ContainsText $inputCloseoutSource $needle "two_d_commercial_input_closeout.cpp"
}

foreach ($needle in @(
        "runtime 2d commercial input closeout accepts selected package evidence",
        "runtime 2d commercial input closeout rejects missing navigation and official source rows",
        "runtime 2d commercial input closeout rejects unsafe platform and legal claims",
        "external_engine_claim_rows == 1U",
        "legal_approval_claim_rows == 1U"
    )) {
    Assert-ContainsText $inputCloseoutTests $needle "runtime_2d_commercial_input_closeout_tests.cpp"
}

foreach ($needle in @(
        "src/two_d_commercial_input_closeout.cpp",
        "tests/unit/runtime_2d_commercial_input_closeout_tests.cpp",
        "MK_runtime_2d_commercial_input_closeout_tests"
    )) {
    Assert-ContainsText ($runtimeCMake + "`n" + $rootCMake) $needle "2d commercial input closeout CMake registration"
}

foreach ($needle in @(
        "mirakana/runtime/two_d_commercial_input_closeout.hpp",
        "--require-2d-commercial-input-closeout",
        "validate_2d_commercial_input_closeout_package_evidence",
        "make_2d_commercial_input_closeout_official_source_rows",
        "2d_commercial_input_closeout_ready=",
        "2d_commercial_input_action_map_ready=",
        "2d_commercial_input_rebinding_ready=",
        "2d_commercial_input_device_ux_ready=",
        "2d_commercial_input_accessibility_navigation_ready=",
        "2d_commercial_input_official_source_ready=",
        "2d_commercial_input_official_source_rows=",
        "2d_commercial_input_external_engine_claim_rows=",
        "2d_commercial_input_cross_platform_parity_claim_rows=",
        "2d_commercial_input_legal_approval_claim_rows=",
        "required_2d_commercial_input_closeout_unavailable"
    )) {
    Assert-ContainsText $sample2dMain $needle "sample_2d_desktop_runtime_package/main.cpp"
}

foreach ($needle in @(
        "installed-2d-commercial-input-closeout-smoke",
        "input-commercial-closeout-2d",
        "inputCommercialCloseout2D",
        "evaluate_runtime_2d_commercial_input_closeout",
        "2d_commercial_input_closeout_ready=1",
        "2d_commercial_input_official_source_rows=5",
        "2d_commercial_input_external_engine_claim_rows=0",
        "2d_commercial_input_cross_platform_parity_claim_rows=0",
        "2d_commercial_input_legal_approval_claim_rows=0",
        "external engine compatibility",
        "legal approval"
    )) {
    Assert-ContainsText $sample2dManifest $needle "sample_2d_desktop_runtime_package/game.agent.json"
}

foreach ($needle in @(
        "2d_commercial_input_closeout_status",
        "2d_commercial_input_closeout_ready",
        "2d_commercial_input_official_source_rows",
        "2d_commercial_input_external_engine_claim_rows",
        "2d_commercial_input_cross_platform_parity_claim_rows",
        "2d_commercial_input_legal_approval_claim_rows",
        "package-desktop-runtime.ps1",
        "installed-2d-commercial-input-closeout-smoke",
        "input-commercial-closeout-2d"
    )) {
    Assert-ContainsText $validatorScript $needle "tools/validate-2d-commercial-input-closeout.ps1"
}

foreach ($needle in @(
        "desktop-runtime-2d-commercial-input-closeout",
        "tools/validate-2d-commercial-input-closeout.ps1 -RequireReady",
        "Runtime2DCommercialInputCloseoutDesc",
        "zero external-engine compatibility",
        "zero legal-approval claims"
    )) {
    Assert-ContainsText $validationRecipesManifest $needle "engine/agent/manifest.fragments/009-validationRecipes.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json validation recipes"
}

foreach ($needle in @(
        "twoDCommercialInputCloseoutPhase7Evidence",
        "2d-commercial-input-closeout-v1",
        "Runtime2DCommercialInputOfficialSourceKind",
        "Microsoft GameInput, Raw Input, Pointer Input, UI Automation",
        "zero legal-approval claims"
    )) {
    Assert-ContainsText $productionLoopManifest $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json production loop"
}

foreach ($needle in @(
        "current2DCommercialInputCloseoutPhase7",
        "evaluate_runtime_2d_commercial_input_closeout",
        "--require-2d-commercial-input-closeout",
        "public Microsoft GameInput, Raw Input, Pointer Input, UI Automation",
        "external engine API/layout/asset/project import compatibility"
    )) {
    Assert-ContainsText $gameCodeGuidanceManifest $needle "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json game code guidance"
}

foreach ($needle in @(
        "2D Commercial Input Closeout v1",
        "tools/validate-2d-commercial-input-closeout.ps1",
        "https://learn.microsoft.com/en-us/gaming/gdk/docs/features/common/input/overviews/input-overview",
        "https://learn.microsoft.com/en-us/windows/win32/inputdev/raw-input",
        "https://learn.microsoft.com/en-us/windows/win32/inputmsg/messages",
        "https://learn.microsoft.com/en-us/windows/win32/winauto/entry-uiauto-win32",
        "zero external-engine",
        "zero legal-approval",
        "external engine compatibility"
    )) {
    Assert-ContainsText ($currentCapabilities + "`n" + $roadmap + "`n" + $planText) $needle "2d commercial input closeout docs and plan"
}
