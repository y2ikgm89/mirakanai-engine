#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$OutputRootRelative = "out/validation/2d-commercial-release-legal-review-input"
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$generator = Join-Path $PSScriptRoot "generate-2d-commercial-release-legal-review-input.ps1"
if (-not (Test-Path -LiteralPath $generator -PathType Leaf)) {
    Write-Error "2d-commercial-release-legal-review-input: missing generator $generator"
}

function Assert-ReleaseLegalProperty($Object, [string]$Name, [string]$Label) {
    if ($null -eq $Object.PSObject.Properties[$Name]) {
        Write-Error "$Label missing required property: $Name"
    }
}

function Assert-ReleaseLegalText($Text, [string]$Needle, [string]$Label) {
    if (-not ([string]$Text).Contains($Needle)) {
        Write-Error "$Label missing expected text: $Needle"
    }
}

$generatorOutput = & $generator -OutputRootRelative $OutputRootRelative 2>&1
$generatorExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
if ($generatorExitCode -ne 0) {
    Write-Error "2d-commercial-release-legal-review-input: generator failed with exit code $generatorExitCode`n$(@($generatorOutput) -join "`n")"
}

$reviewPath = Join-Path $root (Join-Path $OutputRootRelative "2d-commercial-release-legal-review.json")
$summaryPath = Join-Path $root (Join-Path $OutputRootRelative "2d-commercial-release-official-source-summary.json")
foreach ($path in @($reviewPath, $summaryPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        Write-Error "2d-commercial-release-legal-review-input: expected output missing: $path"
    }
}

$review = Get-Content -LiteralPath $reviewPath -Raw | ConvertFrom-Json
$summary = Get-Content -LiteralPath $summaryPath -Raw | ConvertFrom-Json

Assert-ReleaseLegalProperty $review "schema_version" "release legal review"
Assert-ReleaseLegalProperty $review "claim_id" "release legal review"
Assert-ReleaseLegalProperty $review "validation_recipe" "release legal review"
Assert-ReleaseLegalProperty $review "legal_advice" "release legal review"
Assert-ReleaseLegalProperty $review "engineering_review_input" "release legal review"
Assert-ReleaseLegalProperty $review "counsel_review_required" "release legal review"
Assert-ReleaseLegalProperty $review "package_rows" "release legal review"
Assert-ReleaseLegalProperty $review "release_blockers" "release legal review"
Assert-ReleaseLegalProperty $review "platform_gate_rows" "release legal review"
Assert-ReleaseLegalProperty $review "official_source_rows" "release legal review"
Assert-ReleaseLegalProperty $review "non_claims" "release legal review"

if ($review.schema_version -ne 1) {
    Write-Error "release legal review schema_version must be 1"
}
if ($review.claim_id -ne "2d-commercial-release-legal-gate-v1") {
    Write-Error "release legal review claim_id must be 2d-commercial-release-legal-gate-v1"
}
if ($review.validation_recipe -ne "2d-commercial-release-legal-review-input") {
    Write-Error "release legal review validation_recipe must be 2d-commercial-release-legal-review-input"
}
if ($review.legal_advice -ne $false -or $review.engineering_review_input -ne $true -or
    $review.counsel_review_required -ne $true) {
    Write-Error "release legal review must be counsel-ready engineering input and not legal advice"
}

if (@($review.package_rows).Count -ne 9) {
    Write-Error "release legal review package_rows must contain exactly 9 rows"
}
foreach ($expectedKind in @(
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
    if (@($review.package_rows | Where-Object { [string]$_.kind -eq $expectedKind }).Count -ne 1) {
        Write-Error "release legal review package_rows must contain exactly one $expectedKind row"
    }
}

foreach ($blockerName in @(
        "missing_notices",
        "unknown_license_rows",
        "unapproved_dependencies",
        "external_engine_marks",
        "copied_assets",
        "unreviewed_generated_assets"
    )) {
    Assert-ReleaseLegalProperty $review.release_blockers $blockerName "release legal review release_blockers"
    if ([int]$review.release_blockers.$blockerName -ne 0) {
        Write-Error "release legal review release_blockers.$blockerName must be 0"
    }
}

if (@($review.platform_gate_rows).Count -ne 3) {
    Write-Error "release legal review platform_gate_rows must contain exactly 3 rows"
}
foreach ($gateId in @("windows-msix-signing", "macos-notarization", "android-play-signing")) {
    if (@($review.platform_gate_rows | Where-Object { [string]$_.id -eq $gateId -and $_.host_gated -eq $true }).Count -ne 1) {
        Write-Error "release legal review platform_gate_rows must contain host-gated $gateId"
    }
}

if (@($review.official_source_rows).Count -ne 5) {
    Write-Error "release legal review official_source_rows must contain exactly 5 rows"
}
foreach ($sourceId in @(
        "microsoft.msix.signing",
        "apple.notarization.distribution",
        "android.app.signing",
        "repository.2d-clean-room-ledger",
        "repository.legal-policy"
    )) {
    if (@($review.official_source_rows | Where-Object { [string]$_.id -eq $sourceId -and $_.official -eq $true }).Count -ne 1) {
        Write-Error "release legal review official_source_rows must contain $sourceId"
    }
}

foreach ($flag in @("legal_approval", "external_engine_compatibility", "external_engine_parity", "native_handles")) {
    Assert-ReleaseLegalProperty $review.non_claims $flag "release legal review non_claims"
    if ($review.non_claims.$flag -ne $false) {
        Write-Error "release legal review non_claims.$flag must be false"
    }
}

Assert-ReleaseLegalProperty $summary "schema_version" "release legal official source summary"
Assert-ReleaseLegalProperty $summary "validation_recipe" "release legal official source summary"
Assert-ReleaseLegalProperty $summary "legal_advice" "release legal official source summary"
Assert-ReleaseLegalProperty $summary "official_source_rows" "release legal official source summary"
if ($summary.validation_recipe -ne "2d-commercial-release-legal-review-input") {
    Write-Error "release legal official source summary validation_recipe mismatch"
}
if ($summary.legal_advice -ne $false) {
    Write-Error "release legal official source summary must not be legal advice"
}
if (@($summary.official_source_rows).Count -ne 5) {
    Write-Error "release legal official source summary official_source_rows must contain exactly 5 rows"
}

$summaryText = Get-Content -LiteralPath $summaryPath -Raw
foreach ($needle in @(
        "https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview",
        "https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution",
        "https://developer.android.com/studio/publish/app-signing",
        "docs/legal-and-licensing.md"
    )) {
    Assert-ReleaseLegalText $summaryText $needle "release legal official source summary"
}

Write-Information "2d-commercial-release-legal-review-input: ok" -InformationAction Continue
