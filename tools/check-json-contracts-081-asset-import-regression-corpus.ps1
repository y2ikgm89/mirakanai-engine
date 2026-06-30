#requires -Version 7.0
#requires -PSEdition Core

# Chapter 8.1 for check-json-contracts.ps1 asset import regression corpus/report schemas.

$corpusSchemaText = Get-JsonContractSurfaceText "schemas/asset-import-regression-corpus.schema.json"
$reportSchemaText = Get-JsonContractSurfaceText "schemas/asset-import-regression-report.schema.json"
$corpusSchema = $corpusSchemaText | ConvertFrom-Json
$reportSchema = $reportSchemaText | ConvertFrom-Json

foreach ($needle in @(
        "https://json-schema.org/draft/2020-12/schema",
        "GameEngine.AssetImportRegressionCorpus.v1",
        "additionalProperties",
        "accepted_for_source_tree",
        "accepted_for_host_corpus_only",
        "external_engine_material",
        "notice_complete",
        "CC-BY-NC",
        "CC-BY-ND",
        "gltf_scene",
        "gltf_mesh",
        "gltf_animation",
        "png_texture",
        "openexr_texture",
        "ktx2_basis_texture",
        "material_document",
        "audio_source",
        "unit_scale",
        "up_axis",
        "source_references",
        "^sha256:",
        "(?!.*(^|/)\\.\\.($|/))",
        "(?!.*\\\\)"
    )) {
    Assert-ContainsText $corpusSchemaText $needle "asset import regression corpus schema"
}

foreach ($needle in @(
        "https://json-schema.org/draft/2020-12/schema",
        "GameEngine.AssetImportRegressionReport.v1",
        "GameEngine.AssetImportRegressionCorpus.v1",
        "additionalProperties",
        "asset_count",
        "succeeded_count",
        "failed_count",
        "legal_blocked_count",
        "nondeterministic_count",
        "ready_for_commercial_evidence",
        "parser_error",
        "validator_error",
        "unsafe_external_resource_path",
        "unsupported_skin_or_morph_combination",
        "coordinate_normalization_failed",
        "texture_transcode_failed",
        "nondeterministic_output",
        "manifest",
        "legal",
        "source_hash",
        "parser",
        "validator",
        "external_resource",
        "preset_review",
        "source_document_import",
        "cook",
        "output_hash",
        "preview"
    )) {
    Assert-ContainsText $reportSchemaText $needle "asset import regression report schema"
}

if ($corpusSchema.properties.schema_version.const -ne "GameEngine.AssetImportRegressionCorpus.v1") {
    Write-Error "asset import regression corpus schema_version must be const v1"
}
if ($corpusSchema.properties.corpus_id.const -ne "GameEngine.AssetImportRegressionCorpus.v1") {
    Write-Error "asset import regression corpus corpus_id must be const v1"
}
foreach ($property in @("schema_version", "corpus_id", "corpus_version", "root_path", "row_budget", "assets")) {
    if (@($corpusSchema.required) -notcontains $property) {
        Write-Error "asset import regression corpus schema missing required property: $property"
    }
}
if ($corpusSchema.properties.assets.items.'$ref' -ne "#/`$defs/corpus_asset") {
    Write-Error "asset import regression corpus assets must reference corpus_asset"
}
if ($corpusSchema.'$defs'.corpus_asset.properties.license_policy.enum -contains "rejected") {
    Write-Error "asset import regression corpus JSON schema must reject license_policy=rejected"
}
if ($corpusSchema.'$defs'.provenance.properties.external_engine_material.const -ne $false) {
    Write-Error "asset import regression corpus provenance must require external_engine_material=false"
}
if ($corpusSchema.'$defs'.provenance.properties.notice_complete.const -ne $true) {
    Write-Error "asset import regression corpus provenance must require notice_complete=true"
}

if ($reportSchema.properties.schema_version.const -ne "GameEngine.AssetImportRegressionReport.v1") {
    Write-Error "asset import regression report schema_version must be const v1"
}
if ($reportSchema.properties.corpus_id.const -ne "GameEngine.AssetImportRegressionCorpus.v1") {
    Write-Error "asset import regression report corpus_id must point at corpus v1"
}
foreach ($property in @(
        "schema_version",
        "corpus_id",
        "run_id",
        "asset_count",
        "succeeded_count",
        "failed_count",
        "legal_blocked_count",
        "nondeterministic_count",
        "ready",
        "rows"
    )) {
    if (@($reportSchema.required) -notcontains $property) {
        Write-Error "asset import regression report schema missing required property: $property"
    }
}
$reportRow = $reportSchema.'$defs'.report_row
foreach ($property in @(
        "asset_id",
        "kind",
        "asset",
        "source_path",
        "source_sha256",
        "preset_sha256",
        "importer_id",
        "importer_version",
        "phase",
        "code",
        "message",
        "deterministic_output_hash",
        "succeeded",
        "ready_for_commercial_evidence"
    )) {
    if (@($reportRow.required) -notcontains $property) {
        Write-Error "asset import regression report row schema missing required property: $property"
    }
}
foreach ($forbiddenClaim in @(
        "unity_compatibility",
        "unreal_compatibility",
        "godot_compatibility",
        "runtime_source_parsing",
        "native_handle"
    )) {
    Assert-DoesNotContainText $corpusSchemaText $forbiddenClaim "asset import regression corpus schema"
    Assert-DoesNotContainText $reportSchemaText $forbiddenClaim "asset import regression report schema"
}
