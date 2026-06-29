#requires -Version 7.0
#requires -PSEdition Core

# Chapter 6.9 for check-json-contracts.ps1 2D originality review static contracts.

$engine = Read-Json "engine/agent/manifest.json"
$geToolsModule = @($engine.modules | Where-Object { $_.name -eq "MK_tools" })
if ($geToolsModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_tools module for 2D originality review"
}

if (@($geToolsModule[0].publicHeaders) -notcontains "engine/tools/include/mirakana/tools/2d_originality_review.hpp") {
    Write-Error "engine manifest MK_tools publicHeaders must include 2d_originality_review.hpp"
}

$geToolsPurpose = [string]$geToolsModule[0].purpose
foreach ($needle in @(
        "review_2d_originality_sources",
        "review_2d_commercial_production_sources",
        "TwoDOriginalitySourceRow",
        "TwoDOriginalityReviewResult",
        "clean-room counters",
        "official_source_ledger_ready",
        "commercial_production_source_gate_ready",
        "counsel-review-required",
        "copied code/assets/documentation",
        "external engine schemas",
        "trademark surfaces",
        "external engine compatibility/equivalence/parity claims",
        "legal clearance automation"
    )) {
    if (-not $geToolsPurpose.Contains($needle)) {
        Write-Error "engine manifest MK_tools purpose must describe the 2D originality review gate and legal-clean-room non-claim: $needle"
    }
}

$twoDCommercialCleanRoomReviewInputSchema = Read-Json "schemas/2d-commercial-clean-room-review-input.schema.json"
if ([string]$twoDCommercialCleanRoomReviewInputSchema.'$schema' -ne "https://json-schema.org/draft/2020-12/schema") {
    Write-Error "2D commercial clean-room review input schema must use JSON Schema draft 2020-12."
}
if ([string]$twoDCommercialCleanRoomReviewInputSchema.properties.schema_version.const -ne "GameEngine.TwoDCommercialCleanRoomReviewInput.v1") {
    Write-Error "2D commercial clean-room review input schema must fix schema_version."
}
if ([string]$twoDCommercialCleanRoomReviewInputSchema.properties.validation_recipe.const -ne "2d-commercial-clean-room-review-input") {
    Write-Error "2D commercial clean-room review input schema must fix validation_recipe."
}
if ($twoDCommercialCleanRoomReviewInputSchema.properties.clean_room_rows.properties.external_engine_zero_material_review.properties.external_engine_schema_surface.const -ne $false) {
    Write-Error "2D commercial clean-room review input schema must reject external engine schema surfaces."
}
if ($twoDCommercialCleanRoomReviewInputSchema.properties.human_review.properties.legal_counsel_review_required.const -ne $true) {
    Write-Error "2D commercial clean-room review input schema must require legal counsel review."
}
if ($twoDCommercialCleanRoomReviewInputSchema.properties.non_claims.properties.legal_approval.const -ne $false) {
    Write-Error "2D commercial clean-room review input schema must not claim legal approval."
}
if ($twoDCommercialCleanRoomReviewInputSchema.properties.reference_sources.items.'$ref' -ne "#/`$defs/reference_source_row") {
    Write-Error "2D commercial clean-room review input schema must keep Context7 references separate from official sources."
}
if ([string]$twoDCommercialCleanRoomReviewInputSchema.'$defs'.reference_source_row.properties.source_class.const -ne "context7_documentation_mirror") {
    Write-Error "2D commercial clean-room review input schema must classify Context7 sources as reference mirrors."
}

$twoDCommercialOfficialSourceSummarySchema = Read-Json "schemas/2d-commercial-official-source-summary.schema.json"
if ([string]$twoDCommercialOfficialSourceSummarySchema.'$schema' -ne "https://json-schema.org/draft/2020-12/schema") {
    Write-Error "2D commercial official source summary schema must use JSON Schema draft 2020-12."
}
if ([string]$twoDCommercialOfficialSourceSummarySchema.properties.schema_version.const -ne "GameEngine.TwoDCommercialOfficialSourceSummary.v1") {
    Write-Error "2D commercial official source summary schema must fix schema_version."
}
if ([int]$twoDCommercialOfficialSourceSummarySchema.properties.official_source_count.minimum -lt 10) {
    Write-Error "2D commercial official source summary schema must require at least ten official source rows."
}
if ([int]$twoDCommercialOfficialSourceSummarySchema.properties.reference_source_count.minimum -lt 3) {
    Write-Error "2D commercial official source summary schema must require retained Context7 reference rows."
}
if ($twoDCommercialOfficialSourceSummarySchema.properties.non_claims.properties.commercial_ready.const -ne $false) {
    Write-Error "2D commercial official source summary schema must not claim commercial readiness."
}
