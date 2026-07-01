#requires -Version 7.0
#requires -PSEdition Core

# Chapter 8.2 for check-json-contracts.ps1 2D commercial release legal review schemas.

$reviewSchemaText = Get-JsonContractSurfaceText "schemas/2d-commercial-release-legal-review-input.schema.json"
$summarySchemaText = Get-JsonContractSurfaceText "schemas/2d-commercial-release-official-source-summary.schema.json"
$reviewSchema = $reviewSchemaText | ConvertFrom-Json
$summarySchema = $summarySchemaText | ConvertFrom-Json
$generatorText = Get-JsonContractSurfaceText "tools/generate-2d-commercial-release-legal-review-input.ps1"
$checkerText = Get-JsonContractSurfaceText "tools/check-2d-commercial-release-legal-review-input.ps1"
$validatorText = Get-JsonContractSurfaceText "tools/validate-2d-commercial-release-legal-gate.ps1"
$sampleManifestText = Get-JsonContractSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sampleManifest = $sampleManifestText | ConvertFrom-Json
$planText = Get-JsonContractSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"
$currentCapabilitiesText = Get-JsonContractSurfaceText "docs/current-capabilities.md"
$gameGuidanceText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"

foreach ($needle in @(
        "https://json-schema.org/draft/2020-12/schema",
        "GameEngine 2D Commercial Release Legal Review Input v1",
        "2d-commercial-release-legal-gate-v1",
        "2d-commercial-release-legal-review-input",
        "package_content_inventory",
        "third_party_notice_record",
        "dependency_manifest_record",
        "source_provenance_summary",
        "clean_room_static_guard",
        "trademark_surface_guard",
        "distribution_artifact_inventory",
        "generated_asset_review",
        "legal_review_input_record",
        "windows_msix_signing",
        "macos_notarization",
        "android_play_signing",
        "legal_approval",
        "external_engine_compatibility",
        "store_certification_complete",
        "notarization_complete",
        "signing_complete"
    )) {
    Assert-ContainsText $reviewSchemaText $needle "2d commercial release legal review schema"
}

foreach ($needle in @(
        "GameEngine 2D Commercial Release Official Source Summary v1",
        "microsoft_msix_signing",
        "apple_notarization_distribution",
        "android_app_signing",
        "repository_clean_room_ledger",
        "repository_legal_policy",
        "implementation_input",
        "copied_expression_allowed"
    )) {
    Assert-ContainsText $summarySchemaText $needle "2d commercial release official source summary schema"
}

if ($reviewSchema.properties.schema_version.const -ne 1) {
    Write-Error "2d commercial release legal review schema_version must be const 1"
}
if ($reviewSchema.properties.claim_id.const -ne "2d-commercial-release-legal-gate-v1") {
    Write-Error "2d commercial release legal review claim_id must be const 2d-commercial-release-legal-gate-v1"
}
if ($reviewSchema.properties.legal_advice.const -ne $false) {
    Write-Error "2d commercial release legal review legal_advice must be false"
}
if ($reviewSchema.properties.engineering_review_input.const -ne $true) {
    Write-Error "2d commercial release legal review engineering_review_input must be true"
}
if ($reviewSchema.properties.counsel_review_required.const -ne $true) {
    Write-Error "2d commercial release legal review counsel_review_required must be true"
}
foreach ($property in @(
        "schema_version",
        "claim_id",
        "validation_recipe",
        "legal_advice",
        "engineering_review_input",
        "counsel_review_required",
        "selected_package",
        "package_rows",
        "release_blockers",
        "platform_gate_rows",
        "official_source_rows",
        "non_claims"
    )) {
    if (@($reviewSchema.required) -notcontains $property) {
        Write-Error "2d commercial release legal review schema missing required property: $property"
    }
}
foreach ($kind in @(
        "package_content_inventory",
        "third_party_notice_record",
        "dependency_manifest_record",
        "source_provenance_summary",
        "clean_room_static_guard",
        "trademark_surface_guard",
        "distribution_artifact_inventory",
        "generated_asset_review",
        "legal_review_input_record"
    )) {
    if (@($reviewSchema.properties.package_rows.items.properties.kind.enum) -notcontains $kind) {
        Write-Error "2d commercial release legal review package_rows kind enum missing $kind"
    }
}
foreach ($blocker in @(
        "missing_notices",
        "unknown_license_rows",
        "unapproved_dependencies",
        "external_engine_marks",
        "copied_assets",
        "unreviewed_generated_assets"
    )) {
    if ($reviewSchema.properties.release_blockers.properties.$blocker.const -ne 0) {
        Write-Error "2d commercial release legal review release_blockers.$blocker must be const 0"
    }
}

if ($summarySchema.properties.claim_id.const -ne "2d-commercial-release-legal-gate-v1") {
    Write-Error "2d commercial release official source summary claim_id must be const 2d-commercial-release-legal-gate-v1"
}
if ($summarySchema.properties.legal_advice.const -ne $false) {
    Write-Error "2d commercial release official source summary legal_advice must be false"
}
foreach ($kind in @(
        "microsoft_msix_signing",
        "apple_notarization_distribution",
        "android_app_signing",
        "repository_clean_room_ledger",
        "repository_legal_policy"
    )) {
    if (@($summarySchema.properties.official_source_rows.items.properties.kind.enum) -notcontains $kind) {
        Write-Error "2d commercial release official source summary kind enum missing $kind"
    }
}

foreach ($needle in @(
        "2d-commercial-release-legal-review.json",
        "2d-commercial-release-official-source-summary.json",
        "legal_advice = `$false",
        "engineering_review_input = `$true",
        "counsel_review_required = `$true",
        "release_blocker_rows=0",
        "platform_gate_rows=3",
        "official_source_rows=5",
        "https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview",
        "https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution",
        "https://developer.android.com/studio/publish/app-signing"
    )) {
    Assert-ContainsText $generatorText $needle "2d commercial release legal review generator"
}

foreach ($needle in @(
        "2d-commercial-release-legal-gate-v1",
        "missing_notices",
        "unknown_license_rows",
        "unapproved_dependencies",
        "external_engine_marks",
        "copied_assets",
        "unreviewed_generated_assets",
        "legal_advice",
        "not legal advice"
    )) {
    Assert-ContainsText ($checkerText + "`n" + $validatorText) $needle "2d commercial release legal review checker and validator"
}

foreach ($needle in @(
        "installed-2d-commercial-release-legal-gate-smoke",
        "release-legal-gate-2d",
        "commercialReleaseLegalGate2D",
        "2d_commercial_release_legal_gate_ready=1",
        "2d_commercial_release_evidence_rows=9",
        "2d_commercial_release_official_source_rows=5",
        "2d_commercial_release_platform_gate_rows=3"
    )) {
    Assert-ContainsText $sampleManifestText $needle "sample_2d_desktop_runtime_package release legal gate manifest contract"
}

$recipeNames = @($sampleManifest.validationRecipes | ForEach-Object { [string]$_.name })
if ($recipeNames -notcontains "installed-2d-commercial-release-legal-gate-smoke") {
    Write-Error "sample_2d_desktop_runtime_package validationRecipes missing installed-2d-commercial-release-legal-gate-smoke"
}
$qualityGateIds = @($sampleManifest.aiWorkflow.gameDesignSpec.qualityGates | ForEach-Object { [string]$_.id })
if ($qualityGateIds -notcontains "release-legal-gate-2d") {
    Write-Error "sample_2d_desktop_runtime_package qualityGates missing release-legal-gate-2d"
}

foreach ($needle in @(
        "2D Commercial Release Legal Gate v1",
        "tools/validate-2d-commercial-release-legal-gate.ps1",
        "2d-commercial-release-legal-gate-v1",
        "counsel-ready engineering review input",
        "not legal advice",
        "legal approval remains unclaimed"
    )) {
    Assert-ContainsText ($planText + "`n" + $currentCapabilitiesText + "`n" + $gameGuidanceText) $needle "2d commercial release legal gate retained docs and guidance"
}
