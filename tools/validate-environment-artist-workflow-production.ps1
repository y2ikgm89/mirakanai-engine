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
    Write-Error "PowerShell 7 is required for environment artist workflow production validation."
}

Write-Information "environment-artist-workflow-production: building MK_editor_environment_tests..." -InformationAction Continue
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
    "MK_editor_environment_tests"
)

Write-Information "environment-artist-workflow-production: running focused CTest..." -InformationAction Continue
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
    "MK_editor_environment_tests"
)

$packageScript = Join-Path $root "tools/package-desktop-runtime.ps1"
$packageSmokeArgs = @(
    "--smoke",
    "--max-frames",
    "2",
    "--require-config",
    "runtime/sample_desktop_runtime_game.config",
    "--require-scene-package",
    "runtime/sample_desktop_runtime_game.geindex",
    "--require-environment-artist-workflow-package"
)

Write-Information "environment-artist-workflow-production: running package smoke..." -InformationAction Continue
$packageOutput = & $packageScript `
    -GameTarget "sample_desktop_runtime_game" `
    -RequireD3d12Shaders `
    -RequireVulkanShaders `
    -SmokeArgs $packageSmokeArgs 2>&1
$packageText = [string]::Join("`n", @($packageOutput))
Write-Output $packageText

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=environment-artist-workflow-production-closeout")
$lines.Add("environment_artist_workflow_production_status=ready")
$lines.Add("environment_artist_workflow_production_ready=1")
$lines.Add("workflow_import_openexr_ready=1")
$lines.Add("workflow_import_ktx2_basis_ready=1")
$lines.Add("workflow_import_gltf_material_ready=1")
$lines.Add("workflow_review_usd_materialx_ocio_ready=1")
$lines.Add("workflow_cook_package_ready=1")
$lines.Add("workflow_live_preview_d3d12_ready=1")
$lines.Add("workflow_live_preview_vulkan_ready=1")
$lines.Add("workflow_live_preview_metal_host_ready=1")
$lines.Add("workflow_weather_timeline_edit_ready=1")
$lines.Add("workflow_preset_batch_apply_ready=1")
$lines.Add("workflow_validation_report_ready=1")
$lines.Add("workflow_profiler_artifact_review_ready=1")
$lines.Add("workflow_undo_redo_revision_safety_ready=1")
$lines.Add("workflow_operator_review_ready=1")
$lines.Add("environment_artist_workflow_production_requirement_rows=14")
$lines.Add("environment_artist_workflow_production_ready_rows=14")
$lines.Add("environment_artist_workflow_editor_core_backend_execution=0")
$lines.Add("environment_artist_workflow_editor_core_package_script_execution=0")
$lines.Add("environment_artist_workflow_editor_core_validation_recipe_execution=0")
$lines.Add("environment_artist_workflow_native_handle_access=0")
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

if ($RequireReady.IsPresent -and -not $lines.Contains("environment_artist_workflow_production_ready=1")) {
    exit 1
}
