#requires -Version 7.0
#requires -PSEdition Core

# Chapter 13.2 for check-ai-integration.ps1 2D originality review contracts.

$twoDOriginalityReviewHeaderText = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/2d_originality_review.hpp"
$twoDOriginalityReviewSourceText = Get-AgentSurfaceText "engine/tools/asset/2d_originality_review.cpp"
$twoDOriginalityReviewTestsText = Get-AgentSurfaceText "tests/unit/tools_2d_originality_review_tests.cpp"
$twoDOriginalityReviewRootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$twoDOriginalityReviewToolsCMakeText = Get-AgentSurfaceText "engine/tools/asset/CMakeLists.txt"
$twoDOriginalityReviewManifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$twoDOriginalityReviewCurrentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$twoDOriginalityReviewPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-original-2d-commercial-authoring-live-iteration-v1.md"

foreach ($needle in @(
        "TwoDOriginalitySourceKind",
        "TwoDOriginalitySourceRow",
        "TwoDOriginalityDiagnostic",
        "TwoDOriginalityReviewResult",
        "review_2d_originality_sources"
    )) {
    Assert-ContainsText $twoDOriginalityReviewHeaderText $needle "2D originality review public header"
}

foreach ($needle in @(
        "prohibited_external_engine_code",
        "copied_documentation_text",
        "public_surface_uses_external_engine_mark",
        "missing_source_rows"
    )) {
    Assert-ContainsText $twoDOriginalityReviewSourceText $needle "2D originality review source"
}

foreach ($needle in @(
        "2d originality review accepts first party official category docs and platform sdk rows",
        "2d originality review rejects unity unreal godot code assets schema or copied docs",
        "2d originality review rejects external engine trademarks in public surfaces",
        "2d originality review requires legal counsel review even when engineering gate is ready"
    )) {
    Assert-ContainsText $twoDOriginalityReviewTestsText $needle "2D originality review tests"
}

Assert-ContainsText $twoDOriginalityReviewRootCMakeText "MK_tools_2d_originality_review_tests" "root CMake 2D originality review test target"
Assert-ContainsText $twoDOriginalityReviewToolsCMakeText "2d_originality_review.cpp" "MK_tools asset CMake 2D originality source"
Assert-ContainsText $twoDOriginalityReviewManifestText "engine/tools/include/mirakana/tools/2d_originality_review.hpp" "engine manifest 2D originality review public header"
Assert-ContainsText $twoDOriginalityReviewManifestText "review_2d_originality_sources" "engine manifest 2D originality review purpose"
Assert-ContainsText $twoDOriginalityReviewCurrentCapabilitiesText "2D Originality Review v1" "docs/current-capabilities.md 2D originality review"
Assert-ContainsText $twoDOriginalityReviewCurrentCapabilitiesText "legal clearance automation" "docs/current-capabilities.md 2D originality non-claim"
Assert-ContainsText $twoDOriginalityReviewPlanText "Phase 1 validation evidence" "Original 2D authoring plan Phase 1 evidence"
