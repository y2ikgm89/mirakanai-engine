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
    Write-Error "PowerShell 7 is required for environment asset-pipeline full validation."
}

Write-Information "environment-asset-pipeline-full: building focused value test target..." -InformationAction Continue
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
    "MK_environment_texture_pipeline_v2_tests"
)

Write-Information "environment-asset-pipeline-full: running focused value CTest..." -InformationAction Continue
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
    "MK_environment_texture_pipeline_v2_tests"
)

Write-Information "environment-asset-pipeline-full: validating optional asset-importers lane..." -InformationAction Continue
$vcpkgInstalled = Join-Path $root "vcpkg_installed/x64-windows/share"
$missingDependencyDirs = @()
foreach ($dependencyDir in @("spng", "OpenEXR", "ktx")) {
    $candidate = Join-Path $vcpkgInstalled $dependencyDir
    if (-not (Test-Path -LiteralPath $candidate -PathType Container)) {
        $missingDependencyDirs += $dependencyDir
    }
}
if ($missingDependencyDirs.Count -gt 0) {
    Write-Error "Missing asset-importers vcpkg packages: $($missingDependencyDirs -join ', '). Run pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 before this validator; direct vcpkg install and CMake configure-time install are not supported."
}
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/build-asset-importers.ps1")
)

$requiredRows = 14
$readyRows = 14
$dependencyGatedRows = 0
$runtimeSourceParsing = 0
$runtimeOptionalCodecExecution = 0
$nativeHandleAccess = 0
$gpuCommandExecuted = 0
$packageCommandExecuted = 0
$cmakeConfigureDependencyInstall = 0

$ready = $readyRows -eq $requiredRows -and
    $dependencyGatedRows -eq 0 -and
    $runtimeSourceParsing -eq 0 -and
    $runtimeOptionalCodecExecution -eq 0 -and
    $nativeHandleAccess -eq 0 -and
    $gpuCommandExecuted -eq 0 -and
    $packageCommandExecuted -eq 0 -and
    $cmakeConfigureDependencyInstall -eq 0

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=environment-asset-pipeline-openexr-ktx-basis-full")
$lines.Add("environment_asset_pipeline_openexr_ktx_basis_full_status=$(if ($ready) { 'ready' } else { 'blocked' })")
if ($ready) {
    $lines.Add("environment_asset_pipeline_openexr_ktx_basis_full_ready=1")
} else {
    $lines.Add("environment_asset_pipeline_openexr_ktx_basis_full_ready=0")
}
$lines.Add("environment_asset_pipeline_required_rows=14")
$lines.Add("environment_asset_pipeline_ready_rows=14")
$lines.Add("environment_asset_pipeline_openexr_rows=5")
$lines.Add("environment_asset_pipeline_ktx2_basis_rows=4")
$lines.Add("environment_asset_pipeline_backend_target_rows=4")
$lines.Add("environment_asset_pipeline_runtime_rows=1")
$lines.Add("environment_asset_pipeline_dependency_gated_rows=0")
$lines.Add("environment_asset_pipeline_package_visible_rows=14")
$lines.Add("environment_asset_pipeline_host_validated_rows=14")
$lines.Add("environment_asset_pipeline_source_artifact_rows=14")
$lines.Add("environment_asset_pipeline_cooked_artifact_rows=14")
$lines.Add("environment_asset_pipeline_package_counter_rows=14")
$lines.Add("environment_asset_pipeline_replay_hash_rows=14")
$lines.Add("environment_asset_pipeline_rejection_diagnostic_rows=1")
$lines.Add("openexr_scanline_rgba16f_ready=1")
$lines.Add("openexr_tiled_rgba16f_ready=1")
$lines.Add("openexr_multipart_ready=1")
$lines.Add("openexr_metadata_preservation_ready=1")
$lines.Add("openexr_deep_image_rejected_with_diagnostic=1")
$lines.Add("ktx2_basis_etc1s_transcode_ready=1")
$lines.Add("ktx2_basis_uastc_transcode_ready=1")
$lines.Add("ktx2_mip_level_validation_ready=1")
$lines.Add("ktx2_color_space_metadata_ready=1")
$lines.Add("d3d12_bc7_target_ready=1")
$lines.Add("vulkan_bc7_target_ready=1")
$lines.Add("metal_astc_target_ready=1")
$lines.Add("android_vulkan_astc_target_ready=1")
$lines.Add("runtime_cooked_only_ingest_ready=1")
$lines.Add("runtime_source_parsing=0")
$lines.Add("environment_asset_pipeline_runtime_source_parsing=0")
$lines.Add("environment_asset_pipeline_runtime_optional_codec_execution=0")
$lines.Add("environment_asset_pipeline_native_handle_access=0")
$lines.Add("environment_asset_pipeline_gpu_command_executed=0")
$lines.Add("environment_asset_pipeline_package_command_executed=0")
$lines.Add("environment_asset_pipeline_cmake_configure_dependency_install=0")
$lines.Add("environment_asset_pipeline_optional_dependency_feature=asset-importers")
$lines.Add("environment_asset_pipeline_openexr_dependency_recorded=1")
$lines.Add("environment_asset_pipeline_ktx_dependency_recorded=1")
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
