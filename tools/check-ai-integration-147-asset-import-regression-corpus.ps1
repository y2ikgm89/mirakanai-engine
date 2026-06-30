#requires -Version 7.0
#requires -PSEdition Core

# Chapter 147 for check-ai-integration.ps1 static contracts.
# Asset Import Regression Corpus v1 manifest/schema/API alignment.

$assetRegressionHeader = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/asset_import_regression_corpus.hpp"
$assetRegressionSource = Get-AgentSurfaceText "engine/assets/src/asset_import_regression_corpus.cpp"
$assetRegressionTests = Get-AgentSurfaceText "tests/unit/asset_import_regression_tests.cpp"
$assetRegressionCheckScript = Get-AgentSurfaceText "tools/check-asset-import-regression-corpus.ps1"
$assetRegressionValidateScript = Get-AgentSurfaceText "tools/validate-asset-import-regression-corpus.ps1"
$assetRegressionFixtureReadme = Get-AgentSurfaceText "tests/fixtures/asset_import_regression/README.md"
$assetRegressionFixtureCorpus = Get-AgentSurfaceText "tests/fixtures/asset_import_regression/first_party_corpus.gecorpus"
$assetCMake = Get-AgentSurfaceText "engine/assets/CMakeLists.txt"
$rootCMake = Get-AgentSurfaceText "CMakeLists.txt"
$commandsManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$modulesManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$importerManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/007-importerCapabilities.json"
$validationRecipesManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$guidanceManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$composedManifest = Get-AgentSurfaceText "engine/agent/manifest.json"
$corpusSchema = Get-AgentSurfaceText "schemas/asset-import-regression-corpus.schema.json"
$reportSchema = Get-AgentSurfaceText "schemas/asset-import-regression-report.schema.json"
$jsonContractChapter = Get-AgentSurfaceText "tools/check-json-contracts-081-asset-import-regression-corpus.ps1"
$legalDoc = Get-AgentSurfaceText "docs/legal-and-licensing.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-30-asset-import-commercial-regression-workflow-v1.md"
$planReadme = Get-AgentSurfaceText "docs/superpowers/plans/README.md"

foreach ($needle in @(
        "GameEngine.AssetImportRegressionCorpus.v1",
        "AssetImportRegressionCorpusAssetV1",
        "AssetImportRegressionReportRowV1",
        "AssetImportRegressionDiagnosticCode",
        "validate_asset_import_regression_corpus_v1",
        "deserialize_asset_import_regression_report_v1",
        "gltf_scene",
        "ktx2_basis_texture",
        "external_engine_material"
    )) {
    Assert-ContainsText $assetRegressionHeader $needle "asset_import_regression_corpus.hpp"
}

foreach ($needle in @(
        "validate_asset_import_provenance_document",
        "GameEngine.AssetImportRegressionReport.v1",
        "duplicate_asset_id",
        "unsafe_source_path",
        "third_party_missing_expected_sha256",
        "external_engine_material",
        "row_budget_exceeded",
        "mesh.unit_scale",
        ".code="
    )) {
    Assert-ContainsText $assetRegressionSource $needle "asset_import_regression_corpus.cpp"
}

foreach ($needle in @(
        "asset import regression corpus serializes deterministically and round trips",
        "asset import regression corpus rejects unsafe paths and legal blockers",
        "asset import regression corpus deserialize rejects duplicate keys",
        "asset import regression report serializes deterministic failure rows",
        "asset import regression committed first-party corpus fixture validates expected coverage",
        "asset.0.mesh.unit_scale=0.01",
        "asset.2.third_party_missing_expected_sha256",
        "first_party_corpus.gecorpus"
    )) {
    Assert-ContainsText $assetRegressionTests $needle "asset_import_regression_tests.cpp"
}

foreach ($needle in @(
        "asset_import_regression_corpus.cpp",
        "MK_asset_import_regression_tests",
        "tests/unit/asset_import_regression_tests.cpp"
    )) {
    Assert-ContainsText ($assetCMake + "`n" + $rootCMake) $needle "asset import regression CMake registration"
}

foreach ($needle in @(
        "checkAssetImportRegressionCorpus",
        "validateAssetImportRegressionCorpus",
        "asset-import-regression-corpus",
        "tools/check-asset-import-regression-corpus.ps1",
        "tools/validate-asset-import-regression-corpus.ps1",
        "asset_import_regression_corpus.hpp",
        "Asset Import Regression Corpus v1",
        "GameEngine.AssetImportRegressionCorpus.v1",
        "GameEngine.AssetImportRegressionReport.v1",
        "schemas/asset-import-regression-corpus.schema.json",
        "schemas/asset-import-regression-report.schema.json",
        "commercial-license policy",
        "external_engine_material=false",
        "currentAssetImportRegressionCorpus"
    )) {
    Assert-ContainsText ($commandsManifest + "`n" + $modulesManifest + "`n" + $importerManifest + "`n" + $validationRecipesManifest + "`n" + $guidanceManifest + "`n" + $composedManifest) $needle "asset import regression manifest surfaces"
}

foreach ($needle in @(
        "first_party_corpus.gecorpus",
        "gltf.mesh.valid",
        "gltf.animation.valid",
        "diagnostic_unsafe_external_path",
        "diagnostic_unsupported_skin_morph_combination",
        "material.first_party",
        "expected_sha256=sha256:",
        "LicenseRef-Proprietary",
        "external_engine_material=false"
    )) {
    Assert-ContainsText ($assetRegressionFixtureCorpus + "`n" + $assetRegressionFixtureReadme) $needle "asset import regression first-party fixture corpus"
}

foreach ($needle in @(
        "asset_import_regression_corpus_ready",
        "asset_import_regression_large_corpus_present",
        "asset_import_regression_legal_blocked_count",
        "asset_import_regression_failed_count",
        "asset_import_regression_replay_hash",
        "Get-FileHash",
        "Resolve-Path -LiteralPath",
        "ReparsePoint",
        "require_ready.large_corpus_missing",
        "out/host-artifacts/asset-import-regression-corpus"
    )) {
    Assert-ContainsText ($assetRegressionCheckScript + "`n" + $assetRegressionValidateScript + "`n" + $assetRegressionFixtureReadme) $needle "asset import regression corpus validation scripts"
}

foreach ($needle in @(
        "https://json-schema.org/draft/2020-12/schema",
        "additionalProperties",
        "row_budget",
        "accepted_for_source_tree",
        "accepted_for_host_corpus_only",
        "notice_complete",
        "external_engine_material",
        "CC-BY-NC",
        "runtime_source_parsing",
        "Unity",
        "Unreal",
        "Godot"
    )) {
    Assert-ContainsText ($corpusSchema + "`n" + $reportSchema + "`n" + $jsonContractChapter + "`n" + $planText + "`n" + $legalDoc) $needle "asset import regression schema/plan/legal surfaces"
}

foreach ($needle in @(
        "Asset Import Commercial Regression Workflow v1",
        "importer regression corpus",
        "axis/unit preview",
        "preset diff",
        "reimport",
        "legal",
        "Unity",
        "Unreal",
        "Godot"
    )) {
    Assert-ContainsText ($planText + "`n" + $planReadme) $needle "asset import regression plan registry"
}
