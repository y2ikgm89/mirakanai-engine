#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Assert-LinePresent {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$ExpectedLine,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Lines.Contains($ExpectedLine)) {
        Write-Error "$Context missing expected line: $ExpectedLine"
    }
}

function Assert-TextContains {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Text.Contains($Expected)) {
        Write-Error "$Context missing expected text: $Expected"
    }
}

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

function Write-TextFile {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Text
    )

    $path = ConvertTo-LocalPath $RelativePath
    $parent = Split-Path -Parent $path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    Set-Content -LiteralPath $path -Value $Text -Encoding utf8NoBOM
}

$diagnosticsScript = Join-Path $root "tools/check-2d-commercial-source-diagnostics.ps1"
if (-not (Test-Path -LiteralPath $diagnosticsScript -PathType Leaf)) {
    Write-Error "tools/check-2d-commercial-source-diagnostics.ps1 must exist for 2D commercial fail-closed source diagnostics."
}

$contractRootRelative = "out/2d-commercial-source-diagnostics-contract-$PID"
$cleanRootRelative = "$contractRootRelative/clean"
$badRootRelative = "$contractRootRelative/bad"
$contractRootPath = ConvertTo-LocalPath $contractRootRelative

try {
    if (Test-Path -LiteralPath $contractRootPath) {
        Remove-Item -LiteralPath $contractRootPath -Recurse -Force
    }

    Write-TextFile `
        -RelativePath "$cleanRootRelative/evidence/clean.json" `
        -Text @'
{
  "schema_version": "GameEngine.TwoDCommercialCleanRoomReviewInput.v1",
  "external_code_copied": false,
  "external_assets_copied": false,
  "copied_documentation_text": false,
  "external_engine_schema_surface": false,
  "third_party_trademark_public_surface": false,
  "missing_notice_record": false,
  "unapproved_dependency_source": false
}
'@

    $cleanLines = @(& $diagnosticsScript -ScanRootRelative $cleanRootRelative 6>&1)
    foreach ($expectedLine in @(
            "2d_commercial_source_diagnostics_ready=1",
            "2d_commercial_source_diagnostics_scope=retained_markers_and_public_surface_tokens",
            "external_code_copied_marker_rows=0",
            "external_assets_copied_marker_rows=0",
            "copied_documentation_text_marker_rows=0",
            "external_engine_schema_surface_rows=0",
            "third_party_trademark_public_surface_rows=0",
            "missing_notice_marker_rows=0",
            "unapproved_dependency_source_marker_rows=0",
            "external_engine_compatibility_claim_rows=0",
            "external_engine_equivalence_claim_rows=0",
            "external_engine_parity_claim_rows=0",
            "2d-commercial-source-diagnostics: ok"
        )) {
        Assert-LinePresent $cleanLines $expectedLine "clean 2D commercial source diagnostics"
    }

    Write-TextFile `
        -RelativePath "$badRootRelative/evidence/copied.json" `
        -Text @'
{
  "external_code_copied": true,
  "external_assets_copied": true,
  "copied_documentation_text": true,
  "external_engine_schema_surface": true,
  "third_party_trademark_public_surface": true,
  "missing_notice_record": true,
  "unapproved_dependency_source": true,
  "external_engine_equivalence_claim": true,
  "external_engine_parity_claim": true,
  "public_label": "Unity SDK compatible importer",
  "runtimePackageFiles": ["levels/main.tscn"]
}
'@

    $badRejected = $false
    $badMessage = ""
    try {
        $null = & $diagnosticsScript -ScanRootRelative $badRootRelative 2>&1
    } catch {
        $badRejected = $true
        $badMessage = [string]$_.Exception.Message
    }
    if (-not $badRejected) {
        Write-Error "2D commercial source diagnostics must reject copied material, trademark, notice, dependency, claim, and schema markers."
    }

    foreach ($expected in @(
            "2d_commercial_source_diagnostic=external_code_copied",
            "2d_commercial_source_diagnostic=external_assets_copied",
            "2d_commercial_source_diagnostic=copied_documentation_text",
            "2d_commercial_source_diagnostic=external_engine_schema_surface",
            "2d_commercial_source_diagnostic=third_party_trademark_public_surface",
            "2d_commercial_source_diagnostic=missing_notice_record",
            "2d_commercial_source_diagnostic=unapproved_dependency_source",
            "2d_commercial_source_diagnostic=external_engine_compatibility_claim",
            "external_engine_equivalence_claim",
            "external_engine_parity_claim"
        )) {
        Assert-TextContains $badMessage $expected "bad 2D commercial source diagnostics"
    }

    $unsafeRejected = $false
    try {
        $null = & $diagnosticsScript -ScanRootRelative "../unsafe" 2>&1
    } catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "2D commercial source diagnostics must reject unsafe scan roots."
    }

    $defaultLines = @(& $diagnosticsScript 6>&1)
    Assert-LinePresent $defaultLines "2d-commercial-source-diagnostics: ok" "repository 2D commercial source diagnostics"
}
finally {
    if (Test-Path -LiteralPath $contractRootPath) {
        Remove-Item -LiteralPath $contractRootPath -Recurse -Force
    }
}

Write-Information "2d-commercial-source-diagnostics-contract-check: ok" -InformationAction Continue
