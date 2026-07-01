#requires -Version 7.0
#requires -PSEdition Core

# Chapter 147 for check-ai-integration.ps1 static contracts.
# Asset Import Regression Corpus v1 manifest/schema/API alignment.

$assetRegressionHeader = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/asset_import_regression_corpus.hpp"
$assetRegressionSource = Get-AgentSurfaceText "engine/assets/src/asset_import_regression_corpus.cpp"
$assetRegressionTriageHeader = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/asset_import_regression_triage.hpp"
$assetRegressionTriageSource = Get-AgentSurfaceText "engine/assets/src/asset_import_regression_triage.cpp"
$assetRegressionRunnerHeader = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/asset_import_regression_runner.hpp"
$assetRegressionRunnerSource = Get-AgentSurfaceText "engine/tools/asset/asset_import_regression_runner.cpp"
$assetRegressionRunnerCli = Get-AgentSurfaceText "engine/tools/asset/asset_import_regression_runner_cli.cpp"
$assetRegressionTests = Get-AgentSurfaceText "tests/unit/asset_import_regression_tests.cpp"
$assetRegressionCheckScript = Get-AgentSurfaceText "tools/check-asset-import-regression-corpus.ps1"
$assetRegressionValidateScript = Get-AgentSurfaceText "tools/validate-asset-import-regression-corpus.ps1"
$assetRegressionGenerateScript = Get-AgentSurfaceText "tools/generate-asset-import-regression-corpus-manifest.ps1"
$assetRegressionRunScript = Get-AgentSurfaceText "tools/run-asset-import-regression-corpus.ps1"
$assetRegressionHandoffPlannerScript = Get-AgentSurfaceText "tools/plan-asset-import-regression-corpus-handoff.ps1"
$assetRegressionHandoffCheckScript = Get-AgentSurfaceText "tools/check-asset-import-regression-corpus-handoff.ps1"
$assetRegressionOperatorLoopScript = Get-AgentSurfaceText "tools/check-asset-import-regression-operator-loop.ps1"
$editorAssetRegressionWorkflowHeader = Get-AgentSurfaceText "editor/core/include/mirakana/editor/asset_import_regression_workflow.hpp"
$editorAssetRegressionWorkflowSource = Get-AgentSurfaceText "editor/core/src/asset_import_regression_workflow.cpp"
$editorCoreTests = Get-AgentSurfaceText "tests/unit/editor_core_tests.cpp"
$nativeEditorLaunchHeader = Get-AgentSurfaceText "editor/src/native_editor_launch.hpp"
$nativeEditorLaunchSource = Get-AgentSurfaceText "editor/src/native_editor_launch.cpp"
$nativeEditorAppSource = Get-AgentSurfaceText "editor/src/native_editor_app.cpp"
$firstPartyEditorDocumentHeader = Get-AgentSurfaceText "editor/src/first_party_editor_document.hpp"
$firstPartyEditorDocumentSource = Get-AgentSurfaceText "editor/src/first_party_editor_document.cpp"
$nativeEditorMainSource = Get-AgentSurfaceText "editor/src/main.cpp"
$editorNativeShellTests = Get-AgentSurfaceText "tests/unit/editor_native_shell_tests.cpp"
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
        "GameEngine.AssetImportRegressionTriage.v1",
        "AssetImportRegressionTriageRowV1",
        "AssetImportRegressionTriageDocumentV1",
        "AssetImportRegressionRecommendedAction",
        "AssetImportRegressionReimportDecision",
        "failed_count",
        "legal_blocked_count",
        "nondeterministic_count",
        "recommended_action_for_asset_import_regression_code",
        "make_asset_import_regression_triage_v1",
        "serialize_asset_import_regression_triage_v1",
        "deserialize_asset_import_regression_triage_v1"
    )) {
    Assert-ContainsText $assetRegressionTriageHeader $needle "asset_import_regression_triage.hpp"
}

foreach ($needle in @(
        "fix_notice_or_remove_asset",
        "open_axis_unit_preview",
        "inspect_codec_dependency",
        "rerun_isolated_and_compare_hashes",
        "dry_run_allowed",
        "source_excerpt_hash",
        "failed_count=",
        "preset_diff_required",
        "axis_unit_preview_required",
        "legal_blocked_count=",
        "nondeterministic_count=",
        "legal_blocked",
        "nondeterministic"
    )) {
    Assert-ContainsText $assetRegressionTriageSource $needle "asset_import_regression_triage.cpp"
}

foreach ($needle in @(
        "const AssetImportRegressionTriageDocumentV1* triage",
        "workflow_row_id(`"failure`"",
        "triage_failure",
        "recommended_action=",
        "reimport_decision=",
        "preset_diff_required",
        "axis_unit_preview_required"
    )) {
    Assert-ContainsText ($editorAssetRegressionWorkflowHeader + "`n" + $editorAssetRegressionWorkflowSource) $needle "editor asset import regression workflow triage rows"
}

foreach ($needle in @(
        "asset import regression corpus serializes deterministically and round trips",
        "asset import regression corpus rejects unsafe paths and legal blockers",
        "asset import regression corpus deserialize rejects duplicate keys",
        "asset import regression report serializes deterministic failure rows",
        "asset import regression committed first-party corpus fixture validates expected coverage",
        "asset import regression runner rejects invalid manifests without touching source files",
        "asset import regression runner maps corpus assets to deterministic success and failure rows",
        "asset import regression triage maps every diagnostic to a deterministic operator action",
        "asset import regression triage serializes safe deterministic operator rows",
        "asset.0.mesh.unit_scale=0.01",
        "asset.2.third_party_missing_expected_sha256",
        "first_party_corpus.gecorpus"
    )) {
    Assert-ContainsText $assetRegressionTests $needle "asset_import_regression_tests.cpp"
}

foreach ($needle in @(
        "editor asset import regression triage rows drive retained commands without executing tools",
        "editor asset import regression triage legal blockers disable retained reimport command",
        "asset_browser.import_workflow.failure.mesh.axis",
        "asset_browser.import_workflow.failure.texture.codec",
        "asset_browser.import_workflow.failure.mesh.legal",
        "recommended_action=open_axis_unit_preview",
        "reimport_decision=dry_run_allowed",
        "recommended_action=fix_notice_or_remove_asset",
        "reimport_decision=blocked"
    )) {
    Assert-ContainsText $editorCoreTests $needle "editor_core_tests.cpp asset import regression triage"
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
        "asset_import_regression_triage.cpp",
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
        "planAssetImportRegressionCorpusHandoff",
        "checkAssetImportRegressionCorpusHandoff",
        "checkAssetImportRegressionOperatorLoop",
        "plan-asset-import-regression-corpus-handoff.ps1 [-CorpusRoot <path>] [-RequireReady]",
        "check-asset-import-regression-corpus-handoff.ps1",
        "check-asset-import-regression-operator-loop.ps1 [-ReportPath <path>] [-CorpusRoot <path>] [-OutputRoot <path>] [-SyntheticSmoke] [-RequireReady]",
        "generateAssetImportRegressionCorpusManifest",
        "runAssetImportRegressionCorpus",
        "validateAssetImportRegressionCorpus",
        "asset-import-regression-corpus",
        "asset-import-regression-corpus-handoff",
        "tools/check-asset-import-regression-corpus.ps1",
        "tools/plan-asset-import-regression-corpus-handoff.ps1",
        "tools/check-asset-import-regression-corpus-handoff.ps1",
        "tools/check-asset-import-regression-operator-loop.ps1",
        "tools/generate-asset-import-regression-corpus-manifest.ps1",
        "tools/run-asset-import-regression-corpus.ps1",
        "tools/validate-asset-import-regression-corpus.ps1",
        "asset_import_regression_corpus.hpp",
        "Asset Import Regression Corpus v1",
        "GameEngine.AssetImportRegressionCorpus.v1",
        "GameEngine.AssetImportRegressionReport.v1",
        "GameEngine.AssetImportRegressionTriage.v1",
        "mirakana::make_asset_import_regression_triage_v1",
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
        "plan-asset-import-regression-corpus-handoff.ps1",
        "check-asset-import-regression-corpus-handoff.ps1",
        "check-asset-import-regression-operator-loop.ps1",
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
        "asset_import_regression_operator_loop_ready",
        "asset_import_regression_operator_loop_report_rows",
        "asset_import_regression_operator_loop_blocked_rows",
        "asset_import_regression_operator_loop_reimport_candidates",
        "asset_import_regression_operator_loop_preset_diff_required",
        "asset_import_regression_operator_loop_axis_unit_preview_required",
        "asset_import_regression_operator_loop_legal_blocked_rows",
        "asset_import_regression_operator_loop_nondeterministic_rows",
        "failed_count=5",
        "legal_blocked_count=1",
        "nondeterministic_count=1",
        "asset_import_regression_operator_loop_corpus_retained_reports",
        "asset_import_regression_operator_loop_corpus_success_reports",
        "asset_import_regression_operator_loop_corpus_failure_reports",
        "asset_import_regression_operator_loop_require_ready",
        "asset_import_regression_operator_loop_corpus_ready",
        "asset_import_regression_operator_loop_editor_core_value_only",
        "asset_import_regression_operator_loop_external_engine_claim",
        "asset_import_regression_handoff_status",
        "asset_import_regression_handoff_ready",
        "asset_import_regression_handoff_next_action",
        "asset_import_regression_handoff_failed_count",
        "asset_import_regression_handoff_legal_blocked_count",
        "asset_import_regression_handoff_nondeterministic_count",
        "asset_import_regression_handoff_retained_success_reports",
        "asset_import_regression_handoff_retained_failure_reports",
        "asset_import_regression_handoff_operator_loop_input_ready",
        "asset_import_regression_handoff_external_engine_claim=0",
        "asset_import_regression_handoff_legal_approval_claim=0",
        "asset_import_regression_handoff_unity_unreal_godot_compatibility_claim=0",
        "asset_import_regression_handoff_diagnostic=require_ready.missing_corpus",
        "operator_loop_required",
        "require_ready.retained_success_report_missing",
        "require_ready.retained_failure_report_missing",
        "retained/success/report.gereport",
        "retained/failure/report.gereport",
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
    Assert-ContainsText ($assetRegressionCheckScript + "`n" + $assetRegressionValidateScript + "`n" + $assetRegressionGenerateScript + "`n" + $assetRegressionRunScript + "`n" + $assetRegressionHandoffPlannerScript + "`n" + $assetRegressionHandoffCheckScript + "`n" + $assetRegressionOperatorLoopScript + "`n" + $assetRegressionFixtureReadme) $needle "asset import regression corpus validation scripts"
}

foreach ($needle in @(
        "asset_import_regression_report_path",
        "--asset-import-regression-report",
        "make_visible_sanitized_asset_import_regression_report",
        "unsafe_source_path_redacted",
        "asset_import_regression_retained_ui_available",
        "editor_asset_import_regression_workflow_visible",
        "editor_asset_import_regression_workflow_rows",
        "editor_asset_import_regression_failed_rows",
        "editor_asset_import_regression_reimport_command_enabled",
        "editor_asset_import_regression_preset_diff_command_enabled",
        "editor_asset_import_regression_axis_unit_preview_command_enabled",
        "editor_asset_import_regression_importers_executed_in_core",
        "editor_asset_import_regression_native_handles_exposed",
        "editor_asset_import_regression_external_engine_claim",
        "editor first party document exposes retained asset import regression workflow from report path",
        "MK_editor_native_shell_tests",
        "asset-import-regression-visible-shell-smoke"
    )) {
    Assert-ContainsText ($nativeEditorLaunchHeader + "`n" + $nativeEditorLaunchSource + "`n" + $nativeEditorAppSource + "`n" + $firstPartyEditorDocumentHeader + "`n" + $firstPartyEditorDocumentSource + "`n" + $nativeEditorMainSource + "`n" + $editorNativeShellTests + "`n" + $importerManifest + "`n" + $validationRecipesManifest + "`n" + $planText + "`n" + $planReadme) $needle "asset import regression visible native shell smoke"
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
