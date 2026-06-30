#requires -Version 7.0
#requires -PSEdition Core

# Chapter 147 for check-ai-integration.ps1 static contracts.
# Asset Import Regression Corpus v1 manifest/schema/API alignment.

$assetRegressionHeader = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/asset_import_regression_corpus.hpp"
$assetRegressionSource = Get-AgentSurfaceText "engine/assets/src/asset_import_regression_corpus.cpp"
$assetRegressionRunnerHeader = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/asset_import_regression_runner.hpp"
$assetRegressionRunnerSource = Get-AgentSurfaceText "engine/tools/asset/asset_import_regression_runner.cpp"
$assetRegressionRunnerCli = Get-AgentSurfaceText "engine/tools/asset/asset_import_regression_runner_cli.cpp"
$assetRegressionTests = Get-AgentSurfaceText "tests/unit/asset_import_regression_tests.cpp"
$assetRegressionCheckScript = Get-AgentSurfaceText "tools/check-asset-import-regression-corpus.ps1"
$assetRegressionValidateScript = Get-AgentSurfaceText "tools/validate-asset-import-regression-corpus.ps1"
$assetRegressionGenerateScript = Get-AgentSurfaceText "tools/generate-asset-import-regression-corpus-manifest.ps1"
$assetRegressionRunScript = Get-AgentSurfaceText "tools/run-asset-import-regression-corpus.ps1"
$assetRegressionFixtureReadme = Get-AgentSurfaceText "tests/fixtures/asset_import_regression/README.md"
$assetRegressionFixtureCorpus = Get-AgentSurfaceText "tests/fixtures/asset_import_regression/first_party_corpus.gecorpus"
$assetCMake = Get-AgentSurfaceText "engine/assets/CMakeLists.txt"
$toolsAssetCMake = Get-AgentSurfaceText "engine/tools/asset/CMakeLists.txt"
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
$architectureDoc = Get-AgentSurfaceText "docs/architecture.md"
$currentCapabilitiesDoc = Get-AgentSurfaceText "docs/current-capabilities.md"
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
        "asset import regression runner rejects invalid manifests without touching source files",
        "asset import regression runner maps corpus assets to deterministic success and failure rows",
        "asset.0.mesh.unit_scale=0.01",
        "asset.2.third_party_missing_expected_sha256",
        "first_party_corpus.gecorpus"
    )) {
    Assert-ContainsText $assetRegressionTests $needle "asset_import_regression_tests.cpp"
}

foreach ($needle in @(
        "AssetImportRegressionRunnerOptions",
        "AssetImportPresetsDocumentV1 project_presets",
        "AssetImportExecutionOptions import_execution_options",
        "run_asset_import_regression_corpus"
    )) {
    Assert-ContainsText $assetRegressionRunnerHeader $needle "asset_import_regression_runner.hpp"
}

foreach ($needle in @(
        "OverlayFileSystem",
        "sha256:",
        "review_asset_import_preset_for_asset",
        "execute_asset_import_plan",
        "ExternalAssetImportAdapters",
        "nondeterministic_output",
        "source_hash_mismatch",
        "unsupported_format",
        "parser_error",
        "import_gltf_node_transform_animation_quaternion_clip"
    )) {
    Assert-ContainsText $assetRegressionRunnerSource $needle "asset_import_regression_runner.cpp"
}

foreach ($needle in @(
        "--corpus-root",
        "--output-root",
        "--write-report",
        "--row-budget",
        "network_url_rejected",
        "reparse_point_rejected",
        "asset_import_regression_replay_hash",
        "serialize_asset_import_regression_report_v1",
        "run_asset_import_regression_corpus"
    )) {
    Assert-ContainsText $assetRegressionRunnerCli $needle "asset_import_regression_runner_cli.cpp"
}

foreach ($needle in @(
        "asset_import_regression_corpus.cpp",
        "asset_import_regression_runner.cpp",
        "asset_import_regression_runner_cli.cpp",
        "MK_asset_import_regression_tests",
        "MK_asset_import_regression_runner",
        "tests/unit/asset_import_regression_tests.cpp",
        "MK_tools"
    )) {
    Assert-ContainsText ($assetCMake + "`n" + $toolsAssetCMake + "`n" + $rootCMake) $needle "asset import regression CMake registration"
}

foreach ($needle in @(
        "checkAssetImportRegressionCorpus",
        "generateAssetImportRegressionCorpusManifest",
        "runAssetImportRegressionCorpus",
        "validateAssetImportRegressionCorpus",
        "asset-import-regression-corpus",
        "tools/check-asset-import-regression-corpus.ps1",
        "tools/generate-asset-import-regression-corpus-manifest.ps1",
        "tools/run-asset-import-regression-corpus.ps1",
        "tools/validate-asset-import-regression-corpus.ps1",
        "asset_import_regression_corpus.hpp",
        "Asset Import Regression Corpus v1",
        "GameEngine.AssetImportRegressionCorpus.v1",
        "GameEngine.AssetImportRegressionReport.v1",
        "schemas/asset-import-regression-corpus.schema.json",
        "schemas/asset-import-regression-report.schema.json",
        "implemented-value-contract-and-mk-tools-runner-foundation",
        "runnerOwnerModule",
        "MK_asset_import_regression_runner",
        "mirakana::run_asset_import_regression_corpus",
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
        "generate-asset-import-regression-corpus-manifest.ps1",
        "FailOnMissingNotice",
        "asset_import_regression_manifest_generator_ready",
        "asset_import_regression_manifest_generator_downloaded_assets=0",
        "asset_import_regression_manifest_generator_license_inference=0",
        "asset_import_regression_fresh_process_match",
        "asset_import_regression_corpus_ready",
        "asset_import_regression_corpus_manifest_present",
        "asset_import_regression_large_corpus_present",
        "asset_import_regression_legal_blocked_count",
        "asset_import_regression_failed_count",
        "asset_import_regression_expected_hashes_present",
        "asset_import_regression_notices_present",
        "asset_import_regression_sources_gltf_present",
        "asset_import_regression_sources_textures_present",
        "asset_import_regression_sources_materials_present",
        "asset_import_regression_sources_audio_present",
        "asset_import_regression_source_paths_canonical",
        "asset_import_regression_official_source_ledger_present",
        "asset_import_regression_selection_summary_present",
        "asset_import_regression_minimum_composition_ready",
        "asset_import_regression_required_feature_categories_ready",
        "asset_import_regression_replay_hash",
        "retained-hashes.gehashes",
        "SkipCorpusCheck",
        "Get-FileHash",
        "Resolve-Path -LiteralPath",
        "ReparsePoint",
        "require_ready.corpus_manifest_missing",
        "require_ready.notices_missing",
        "require_ready.sources_textures_missing",
        "require_ready.source_paths_noncanonical",
        "require_ready.official_source_ledger_missing",
        "require_ready.selection_summary_missing",
        "require_ready.minimum_composition_not_met",
        "require_ready.required_feature_categories_missing",
        "require_ready.large_corpus_missing",
        "gltf.mesh_only",
        "texture.ktx2_basis",
        "animation.invalid_quaternion",
        "audio.loop_normalization_preset",
        "retained/official-source-ledger.md",
        "retained/corpus-selection-summary.md",
        "out/host-artifacts/asset-import-regression-corpus"
    )) {
    Assert-ContainsText ($assetRegressionCheckScript + "`n" + $assetRegressionValidateScript + "`n" + $assetRegressionGenerateScript + "`n" + $assetRegressionRunScript + "`n" + $assetRegressionFixtureReadme) $needle "asset import regression corpus validation scripts"
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
    Assert-ContainsText ($corpusSchema + "`n" + $reportSchema + "`n" + $jsonContractChapter + "`n" + $planText + "`n" + $legalDoc + "`n" + $architectureDoc + "`n" + $currentCapabilitiesDoc) $needle "asset import regression schema/plan/legal surfaces"
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
