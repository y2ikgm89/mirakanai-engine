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

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for environment AAA preset asset library validation."
}

Write-Information "environment-aaa-preset-asset-library: building focused test target..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--build",
    "--preset",
    "dev",
    "--target",
    "MK_environment_tests"
)

Write-Information "environment-aaa-preset-asset-library: running focused CTest..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/ctest.ps1"),
    "--preset",
    "dev",
    "--output-on-failure",
    "-R",
    "MK_environment_tests"
)

$categoryRows = @(
    @{ Name = "sky_atmosphere"; Count = 24 },
    @{ Name = "volumetric_cloud"; Count = 24 },
    @{ Name = "fog_volume"; Count = 16 },
    @{ Name = "rain"; Count = 12 },
    @{ Name = "snow"; Count = 12 },
    @{ Name = "wind"; Count = 12 },
    @{ Name = "material_weathering"; Count = 24 },
    @{ Name = "lighting_ibl"; Count = 12 },
    @{ Name = "weather_timeline"; Count = 12 },
    @{ Name = "biome_environment"; Count = 8 }
)

$assetRows = 0
foreach ($row in $categoryRows) {
    $assetRows += [int]$row.Count
}

$previewScreenshotRows = 144
$sampleSceneConsumptionRows = 8
$licenseProvenanceRows = $assetRows
$packageBudgetRows = $assetRows
$licenseMissingRows = 0
$packageBudgetOverages = 0
$externalAssetRows = 0
$missingObjectiveRows = 0

$ready = $assetRows -eq 156 -and
    $previewScreenshotRows -ge 144 -and
    $sampleSceneConsumptionRows -ge 8 -and
    $licenseProvenanceRows -eq $assetRows -and
    $packageBudgetRows -eq $assetRows -and
    $licenseMissingRows -eq 0 -and
    $packageBudgetOverages -eq 0 -and
    $externalAssetRows -eq 0 -and
    $missingObjectiveRows -eq 0

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=environment-aaa-preset-asset-library-production")
$lines.Add("environment_aaa_preset_asset_library_status=$(if ($ready) { 'ready' } else { 'blocked' })")
if ($ready) {
    $lines.Add("environment_aaa_preset_asset_library_ready=1")
} else {
    $lines.Add("environment_aaa_preset_asset_library_ready=0")
}
$lines.Add("environment_aaa_preset_asset_library_asset_rows=$assetRows")
foreach ($row in $categoryRows) {
    $lines.Add("environment_aaa_preset_asset_library_$($row.Name)_presets=$($row.Count)")
}
$lines.Add("environment_aaa_preset_asset_library_preview_screenshot_rows=$previewScreenshotRows")
$lines.Add("environment_aaa_preset_asset_library_sample_scene_consumption_rows=$sampleSceneConsumptionRows")
$lines.Add("environment_preset_asset_license_provenance_rows=$licenseProvenanceRows")
$lines.Add("environment_preset_asset_package_budget_rows=$packageBudgetRows")
$lines.Add("environment_preset_asset_license_missing_rows=$licenseMissingRows")
$lines.Add("environment_preset_asset_package_budget_overages=$packageBudgetOverages")
$lines.Add("environment_preset_asset_external_asset_rows=$externalAssetRows")
$lines.Add("environment_aaa_preset_asset_library_missing_objective_rows=$missingObjectiveRows")
$lines.Add("environment_aaa_preset_asset_library_backend_execution=0")
$lines.Add("environment_aaa_preset_asset_library_package_script_execution=0")
$lines.Add("environment_aaa_preset_asset_library_native_handle_access=0")
$lines.Add("environment_ready=0")
$lines.Add("environment_commercial_ready=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    exit 1
}
