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
$twoDCommercialPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"
$twoDCommercialCleanRoomSpecText = Get-AgentSurfaceText "docs/specs/2026-06-30-2d-commercial-clean-room-source-ledger-v1.md"
$twoDCommercialCleanRoomCheckText = Get-AgentSurfaceText "tools/check-2d-commercial-clean-room.ps1"
$twoDCommercialCleanRoomContractText = Get-AgentSurfaceText "tools/check-2d-commercial-clean-room-contract.ps1"
$twoDCommercialCleanRoomGeneratorText = Get-AgentSurfaceText "tools/generate-2d-commercial-clean-room-review-input.ps1"
$twoDCommercialCleanRoomReviewInputSchemaText = Get-AgentSurfaceText "schemas/2d-commercial-clean-room-review-input.schema.json"
$twoDCommercialOfficialSourceSummarySchemaText = Get-AgentSurfaceText "schemas/2d-commercial-official-source-summary.schema.json"
$twoDCommercialLegalText = Get-AgentSurfaceText "docs/legal-and-licensing.md"
$twoDCommercialDependenciesText = Get-AgentSurfaceText "docs/dependencies.md"

foreach ($needle in @(
        "TwoDOriginalitySourceKind",
        "TwoDOriginalitySourceRow",
        "TwoDOriginalityDiagnostic",
        "TwoDOriginalityReviewResult",
        "review_2d_originality_sources",
        "review_2d_commercial_production_sources",
        "external_engine_compatibility_claim",
        "external_engine_equivalence_claim",
        "external_engine_parity_claim",
        "commercial_production_source_gate_ready"
    )) {
    Assert-ContainsText $twoDOriginalityReviewHeaderText $needle "2D originality review public header"
}

foreach ($needle in @(
        "prohibited_external_engine_code",
        "copied_documentation_text",
        "public_surface_uses_external_engine_mark",
        "external_engine_compatibility_claim",
        "external_engine_equivalence_claim",
        "external_engine_parity_claim",
        "missing_official_documentation_category_source",
        "missing_official_platform_sdk_source",
        "missing_source_rows"
    )) {
    Assert-ContainsText $twoDOriginalityReviewSourceText $needle "2D originality review source"
}

foreach ($needle in @(
        "2d originality review accepts first party official category docs and platform sdk rows",
        "2d originality review rejects unity unreal godot code assets schema or copied docs",
        "2d originality review rejects external engine trademarks in public surfaces",
        "2d originality review requires legal counsel review even when engineering gate is ready",
        "2d commercial production source review requires first party official docs and platform sdk rows",
        "2d commercial production source review accepts official ledger while preserving counsel review",
        "2d commercial production source review rejects compatibility equivalence and parity claims"
    )) {
    Assert-ContainsText $twoDOriginalityReviewTestsText $needle "2D originality review tests"
}

Assert-ContainsText $twoDOriginalityReviewRootCMakeText "MK_tools_2d_originality_review_tests" "root CMake 2D originality review test target"
Assert-ContainsText $twoDOriginalityReviewToolsCMakeText "2d_originality_review.cpp" "MK_tools asset CMake 2D originality source"
Assert-ContainsText $twoDOriginalityReviewManifestText "engine/tools/include/mirakana/tools/2d_originality_review.hpp" "engine manifest 2D originality review public header"
Assert-ContainsText $twoDOriginalityReviewManifestText "review_2d_originality_sources" "engine manifest 2D originality review purpose"
Assert-ContainsText $twoDOriginalityReviewManifestText "review_2d_commercial_production_sources" "engine manifest 2D commercial production review purpose"
Assert-ContainsText $twoDOriginalityReviewManifestText "commercial_production_source_gate_ready" "engine manifest 2D commercial production gate"
Assert-ContainsText $twoDOriginalityReviewCurrentCapabilitiesText "2D Originality Review v1" "docs/current-capabilities.md 2D originality review"
Assert-ContainsText $twoDOriginalityReviewCurrentCapabilitiesText "review_2d_commercial_production_sources" "docs/current-capabilities.md 2D commercial production source review"
Assert-ContainsText $twoDOriginalityReviewCurrentCapabilitiesText "legal clearance automation" "docs/current-capabilities.md 2D originality non-claim"
Assert-ContainsText $twoDOriginalityReviewPlanText "Phase 1 validation evidence" "Original 2D authoring plan Phase 1 evidence"

foreach ($needle in @(
        "2D Commercial Clean-Room Source Ledger v1",
        "official documentation to inform only broad functional categories",
        "External engine project/scene/schema imports",
        "MIRAIKANAI-owned first-party file extensions with the same generic English word"
    )) {
    Assert-ContainsText $twoDCommercialCleanRoomSpecText $needle "2D commercial clean-room source ledger spec"
}

foreach ($needle in @(
        "forbidden_2d_commercial_token",
        "ScanRootRelative",
        "schemas/game-agent.schema.json",
        "schemas/engine-agent.schema.json",
        "2d-commercial-clean-room: ok"
    )) {
    Assert-ContainsText $twoDCommercialCleanRoomCheckText $needle "2D commercial clean-room static guard"
}

foreach ($needle in @(
        "tools/check-2d-commercial-clean-room.ps1 must exist",
        "schemas/2d-commercial-clean-room-review-input.schema.json",
        "schemas/2d-commercial-official-source-summary.schema.json",
        "Test-Json",
        "2d-commercial-clean-room-contract-check: ok"
    )) {
    Assert-ContainsText $twoDCommercialCleanRoomContractText $needle "2D commercial clean-room guard contract"
}

foreach ($needle in @(
        "GameEngine.TwoDCommercialCleanRoomReviewInput.v1",
        "GameEngine.TwoDCommercialOfficialSourceSummary.v1",
        "Context7-Vulkan-Docs-2026-06-30",
        "context7_documentation_mirror",
        "Unity-Trademark-Guidelines-2026-06-30",
        "USPTO-Trademark-Basics-2026-06-30",
        "2d_commercial_clean_room_public_docs_only=1",
        "2d_commercial_clean_room_external_engine_zero_material_ready=1",
        "requires_legal_counsel_review=1",
        "2d_commercial_clean_room_commercial_ready=0",
        "2d_commercial_clean_room_legal_approval=0"
    )) {
    Assert-ContainsText $twoDCommercialCleanRoomGeneratorText $needle "2D commercial clean-room review input generator"
}

foreach ($needle in @(
        "GameEngine.TwoDCommercialCleanRoomReviewInput.v1",
        "2d-commercial-clean-room-source-gate-v1",
        "reference_sources",
        "context7_documentation_mirror",
        "implementation_source_copied",
        "external_engine_schema_surface",
        "legal_counsel_review_required",
        "legal_approval"
    )) {
    Assert-ContainsText $twoDCommercialCleanRoomReviewInputSchemaText $needle "2D commercial clean-room review input schema"
}

foreach ($needle in @(
        "GameEngine.TwoDCommercialOfficialSourceSummary.v1",
        "official-public-documentation-category-research-only",
        "reference_source_count",
        "rejected_external_material",
        "external_engine_schema",
        "parity_claim"
    )) {
    Assert-ContainsText $twoDCommercialOfficialSourceSummarySchemaText $needle "2D commercial official source summary schema"
}

foreach ($needle in @(
        "tools/check-2d-commercial-clean-room.ps1",
        "tools/generate-2d-commercial-clean-room-review-input.ps1",
        "schemas/2d-commercial-clean-room-review-input.schema.json",
        "without drawing legal conclusions"
    )) {
    Assert-ContainsText $twoDOriginalityReviewCurrentCapabilitiesText $needle "docs/current-capabilities.md 2D commercial clean-room guard"
}

foreach ($needle in @(
        "tools/check-2d-commercial-clean-room.ps1",
        "tools/generate-2d-commercial-clean-room-review-input.ps1",
        "docs/specs/2026-06-30-2d-commercial-clean-room-source-ledger-v1.md",
        "does not add third-party code, assets"
    )) {
    Assert-ContainsText $twoDCommercialLegalText $needle "docs/legal-and-licensing.md 2D commercial clean-room source gate"
    Assert-ContainsText $twoDCommercialDependenciesText $needle "docs/dependencies.md 2D commercial clean-room source gate"
}

foreach ($needle in @(
        "2d_commercial_clean_room_public_docs_only=1",
        "external_engine_schema_surface=0",
        "requires_legal_counsel_review=1",
        "without implying legal clearance"
    )) {
    Assert-ContainsText $twoDCommercialPlanText $needle "2D commercial production plan Phase 1 Done when"
}
