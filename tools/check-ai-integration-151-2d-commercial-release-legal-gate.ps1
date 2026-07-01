#requires -Version 7.0
#requires -PSEdition Core

# Chapter 151 for check-ai-integration.ps1 static contracts.
# Runtime 2D Commercial Release Legal Gate v1 package/manifest/plan alignment.

$releaseHeader = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/two_d_commercial_release_legal_gate.hpp"
$releaseSource = Get-AgentSurfaceText "engine/runtime/src/two_d_commercial_release_legal_gate.cpp"
$releaseTests = Get-AgentSurfaceText "tests/unit/runtime_2d_commercial_release_legal_gate_tests.cpp"
$rootCMake = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeCMake = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$sample2dMain = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample2dManifest = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$generatorScript = Get-AgentSurfaceText "tools/generate-2d-commercial-release-legal-review-input.ps1"
$checkerScript = Get-AgentSurfaceText "tools/check-2d-commercial-release-legal-review-input.ps1"
$validatorScript = Get-AgentSurfaceText "tools/validate-2d-commercial-release-legal-gate.ps1"
$legalReviewSchema = Get-AgentSurfaceText "schemas/2d-commercial-release-legal-review-input.schema.json"
$officialSourceSchema = Get-AgentSurfaceText "schemas/2d-commercial-release-official-source-summary.schema.json"
$validationRecipesManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$gameCodeGuidanceManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$composedManifest = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilities = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmap = Get-AgentSurfaceText "docs/roadmap.md"
$legalPolicy = Get-AgentSurfaceText "docs/legal-and-licensing.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"

foreach ($needle in @(
        "Runtime2DCommercialReleaseEvidenceKind",
        "Runtime2DCommercialReleaseOfficialSourceKind",
        "Runtime2DCommercialReleasePlatformGateKind",
        "Runtime2DCommercialReleaseDiagnosticCode",
        "Runtime2DCommercialReleaseEvidenceRow",
        "Runtime2DCommercialReleaseOfficialSourceRow",
        "Runtime2DCommercialReleasePlatformGateRow",
        "Runtime2DCommercialReleaseGateDesc",
        "Runtime2DCommercialReleaseGateResult",
        "evaluate_runtime_2d_commercial_release_legal_gate",
        "microsoft_msix_signing",
        "apple_notarization_distribution",
        "android_app_signing",
        "external_engine_compatibility_claim",
        "legal_approval_claim"
    )) {
    Assert-ContainsText $releaseHeader $needle "two_d_commercial_release_legal_gate.hpp"
}

foreach ($needle in @(
        "kRequiredEvidenceKinds",
        "kRequiredOfficialSourceKinds",
        "kRequiredPlatformGateKinds",
        "evidence_row_ready",
        "official_source_row_ready",
        "platform_gate_row_ready",
        "missing_notice",
        "unknown_license",
        "unapproved_dependency",
        "external_engine_mark",
        "copied_asset",
        "unreviewed_generated_asset",
        "external_engine_compatibility_claim",
        "legal_approval_claim"
    )) {
    Assert-ContainsText $releaseSource $needle "two_d_commercial_release_legal_gate.cpp"
}

foreach ($needle in @(
        "runtime 2d commercial release legal gate accepts counsel ready engineering input",
        "runtime 2d commercial release legal gate rejects missing notices and unknown licenses",
        "runtime 2d commercial release legal gate rejects compatibility and approval claims",
        "official_source_rows == 5U",
        "platform_gate_rows == 3U"
    )) {
    Assert-ContainsText $releaseTests $needle "runtime_2d_commercial_release_legal_gate_tests.cpp"
}

foreach ($needle in @(
        "src/two_d_commercial_release_legal_gate.cpp",
        "tests/unit/runtime_2d_commercial_release_legal_gate_tests.cpp",
        "MK_runtime_2d_commercial_release_legal_gate_tests"
    )) {
    Assert-ContainsText ($runtimeCMake + "`n" + $rootCMake) $needle "2d commercial release legal gate CMake registration"
}

foreach ($needle in @(
        "mirakana/runtime/two_d_commercial_release_legal_gate.hpp",
        "--require-2d-commercial-release-legal-gate",
        "make_2d_commercial_release_evidence_rows",
        "make_2d_commercial_release_official_source_rows",
        "make_2d_commercial_release_platform_gate_rows",
        "validate_2d_commercial_release_legal_gate_package_evidence",
        "2d_commercial_release_legal_gate_ready=",
        "2d_commercial_release_evidence_rows=",
        "2d_commercial_release_official_source_rows=",
        "2d_commercial_release_platform_gate_rows=",
        "2d_commercial_release_host_gated_platform_rows=",
        "2d_commercial_release_external_engine_claim_rows=",
        "2d_commercial_release_legal_approval_claim_rows=",
        "required_2d_commercial_release_legal_gate_unavailable"
    )) {
    Assert-ContainsText $sample2dMain $needle "sample_2d_desktop_runtime_package/main.cpp"
}

foreach ($needle in @(
        "installed-2d-commercial-release-legal-gate-smoke",
        "release-legal-gate-2d",
        "commercialReleaseLegalGate2D",
        "evaluate_runtime_2d_commercial_release_legal_gate",
        "2d_commercial_release_legal_gate_ready=1",
        "2d_commercial_release_evidence_rows=9",
        "2d_commercial_release_official_source_rows=5",
        "2d_commercial_release_platform_gate_rows=3",
        "Microsoft MSIX signing",
        "Apple notarization",
        "Android app signing",
        "external commercial engine marks",
        "legal-approval claims",
        "not legal advice"
    )) {
    Assert-ContainsText $sample2dManifest $needle "sample_2d_desktop_runtime_package/game.agent.json"
}

foreach ($needle in @(
        "2d-commercial-release-legal-gate-v1",
        "2d-commercial-release-legal-review-input",
        "2d-commercial-release-legal-review.json",
        "2d-commercial-release-official-source-summary.json",
        "legal_advice = `$false",
        "engineering_review_input = `$true",
        "counsel_review_required = `$true",
        "https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview",
        "https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution",
        "https://developer.android.com/studio/publish/app-signing"
    )) {
    Assert-ContainsText ($generatorScript + "`n" + $checkerScript) $needle "2d commercial release legal review input scripts"
}

foreach ($needle in @(
        "2d_commercial_release_legal_gate_status",
        "2d_commercial_release_legal_gate_ready",
        "2d_commercial_release_evidence_rows",
        "2d_commercial_release_official_source_rows",
        "2d_commercial_release_platform_gate_rows",
        "2d_commercial_release_external_engine_claim_rows",
        "2d_commercial_release_legal_approval_claim_rows",
        "package-desktop-runtime.ps1",
        "installed-2d-commercial-release-legal-gate-smoke",
        "release-legal-gate-2d"
    )) {
    Assert-ContainsText $validatorScript $needle "tools/validate-2d-commercial-release-legal-gate.ps1"
}

foreach ($needle in @(
        "GameEngine 2D Commercial Release Legal Review Input v1",
        "2d-commercial-release-legal-gate-v1",
        "package_content_inventory",
        "third_party_notice_record",
        "clean_room_static_guard",
        "trademark_surface_guard",
        "generated_asset_review",
        "legal_review_input_record",
        "store_certification_complete",
        "signing_complete"
    )) {
    Assert-ContainsText $legalReviewSchema $needle "2d-commercial-release-legal-review-input.schema.json"
}

foreach ($needle in @(
        "GameEngine 2D Commercial Release Official Source Summary v1",
        "microsoft_msix_signing",
        "apple_notarization_distribution",
        "android_app_signing",
        "repository_clean_room_ledger",
        "repository_legal_policy",
        "copied_expression_allowed"
    )) {
    Assert-ContainsText $officialSourceSchema $needle "2d-commercial-release-official-source-summary.schema.json"
}

foreach ($needle in @(
        "desktop-runtime-2d-commercial-release-legal-gate",
        "tools/validate-2d-commercial-release-legal-gate.ps1 -RequireReady",
        "Runtime2DCommercialReleaseGateDesc",
        "counsel-ready engineering review input",
        "zero legal-approval claims"
    )) {
    Assert-ContainsText $validationRecipesManifest $needle "engine/agent/manifest.fragments/009-validationRecipes.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json validation recipes"
}

foreach ($needle in @(
        "twoDCommercialReleaseLegalGatePhase9Evidence",
        "2d-commercial-release-legal-gate-v1",
        "Runtime2DCommercialReleaseEvidenceKind",
        "MSIX package signing",
        "Apple notarization",
        "Android app signing"
    )) {
    Assert-ContainsText $productionLoopManifest $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json production loop"
}

foreach ($needle in @(
        "current2DCommercialReleaseLegalGatePhase9",
        "evaluate_runtime_2d_commercial_release_legal_gate",
        "--require-2d-commercial-release-legal-gate",
        "counsel-ready engineering review input",
        "separate host gates"
    )) {
    Assert-ContainsText $gameCodeGuidanceManifest $needle "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json game code guidance"
}

foreach ($needle in @(
        "2D Commercial Release Legal Gate v1",
        "tools/validate-2d-commercial-release-legal-gate.ps1",
        "https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview",
        "https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution",
        "https://developer.android.com/studio/publish/app-signing",
        "engineering review input",
        "not legal advice",
        "legal approval remains unclaimed"
    )) {
    Assert-ContainsText ($currentCapabilities + "`n" + $roadmap + "`n" + $legalPolicy + "`n" + $planText) $needle "2d commercial release legal gate docs and plan"
}
