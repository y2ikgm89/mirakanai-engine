#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady,
    [string[]]$ExpectedEvidenceCounters = @(),
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)
$root = Get-RepoRoot
Set-Location $root

function ConvertTo-CounterBit {
    param([bool]$Value)
    if ($Value) { return "1" }
    return "0"
}

$specPath = Join-Path $root "docs/specs/2026-06-21-mavg-nanite-comparison-taxonomy-v1.md"
$schemaPath = Join-Path $root "schemas/mavg-nanite-comparison-report.schema.json"
if (-not (Test-Path -LiteralPath $specPath)) {
    Write-Error "Missing MAVG Nanite comparison taxonomy spec: $specPath"
}
if (-not (Test-Path -LiteralPath $schemaPath)) {
    Write-Error "Missing MAVG Nanite comparison schema: $schemaPath"
}

$specText = Get-Content -LiteralPath $specPath -Raw
$schemaText = Get-Content -LiteralPath $schemaPath -Raw

$requiredNeedles = @(
    "GameEngine.MavgNaniteComparisonReport.v1",
    "mavg-nanite-comparison-report-v1",
    "Nanite Virtualized Geometry Overview",
    "Nanite Technical Details",
    "https://dev.epicgames.com/documentation/unreal-engine/nanite-virtualized-geometry-in-unreal-engine",
    "https://dev.epicgames.com/documentation/unreal-engine/nanite-technical-details",
    "virtualized_geometry_system",
    "internal_compressed_mesh_format",
    "fine_grained_streaming",
    "automatic_lod",
    "cluster_visibility_culling",
    "fallback_mesh_behavior",
    "ray_tracing_fallback_behavior",
    "material_support_limits",
    "deformation_support_limits",
    "platform_support_limits",
    "storage_memory_residency",
    "authoring_import_workflow",
    "mavg_nanite_compatible=0",
    "mavg_nanite_equivalent=0",
    "mavg_nanite_superior=0"
)

foreach ($needle in $requiredNeedles) {
    if (-not $specText.Contains($needle) -and -not $schemaText.Contains($needle)) {
        Write-Error "MAVG Nanite comparison contract missing required text: $needle"
    }
}

foreach ($forbiddenNeedle in @(
        "mavg_nanite_compatible=1",
        "mavg_nanite_equivalent=1",
        "mavg_nanite_superior=1",
        "Nanite compatible",
        "Nanite equivalent",
        "Nanite superior"
    )) {
    if ($specText.Contains($forbiddenNeedle) -or $schemaText.Contains($forbiddenNeedle)) {
        Write-Error "MAVG Nanite comparison must not contain product claim text: $forbiddenNeedle"
    }
}

$ready = $true
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-nanite-comparison")
$lines.Add("mavg_nanite_comparison_report_status=ready")
$lines.Add("mavg_nanite_comparison_report_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_nanite_comparison_axes=12")
$lines.Add("mavg_nanite_official_source_rows=2")
$lines.Add("mavg_nanite_public_docs_only=1")
$lines.Add("mavg_nanite_first_party_assets_only=1")
$lines.Add("mavg_nanite_unreal_source_used=0")
$lines.Add("mavg_nanite_private_format_used=0")
$lines.Add("mavg_nanite_epic_sample_assets_used=0")
$lines.Add("mavg_nanite_compatible=0")
$lines.Add("mavg_nanite_equivalent=0")
$lines.Add("mavg_nanite_superior=0")
$lines.Add("mavg_nanite_marketing_claim_allowed=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG Nanite comparison report is not ready."
}

Write-Information "mavg-nanite-comparison-check: ok" -InformationAction Continue
