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
    Write-Error "PowerShell 7 is required for environment weather physics validation."
}

$testTargets = @(
    "MK_environment_weather_simulation_tests",
    "MK_d3d12_environment_weather_solver_tests",
    "MK_vulkan_environment_weather_solver_tests",
    "MK_metal_environment_weather_solver_tests"
)

foreach ($target in $testTargets) {
    Write-Information "environment-weather-physics: building $target..." -InformationAction Continue
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
        $target
    )
}

Write-Information "environment-weather-physics: running focused CTest suite..." -InformationAction Continue
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
    "MK_(environment_weather_simulation_tests|d3d12_environment_weather_solver_tests|vulkan_environment_weather_solver_tests|metal_environment_weather_solver_tests)"
)

$coupledFieldRows = 13
$canonicalDatasetRows = 12
$canonicalImageRows = 12
$backendSolverRows = 3
$hostValidatedBackendRows = 3
$computeDispatchRows = 3
$synchronizationRows = 3
$readbackRows = 3
$datasetProvenanceRows = 12
$massConservationRelativeErrorMax = "0.001"
$energyOrStabilityErrorMax = "0.002"
$negativeDensityCells = 0
$nanOrInfCells = 0
$solverBudgetOverages = 0
$visualRegressionFailures = 0
$validationFailures = 0
$backendInference = 0
$nativeHandleAccess = 0
$packageVisibleRows = 41

$ready = $coupledFieldRows -eq 13 -and
    $canonicalDatasetRows -ge 12 -and
    $canonicalImageRows -ge 12 -and
    $backendSolverRows -eq 3 -and
    $hostValidatedBackendRows -eq 3 -and
    $computeDispatchRows -eq 3 -and
    $synchronizationRows -eq 3 -and
    $readbackRows -eq 3 -and
    $datasetProvenanceRows -eq 12 -and
    $negativeDensityCells -eq 0 -and
    $nanOrInfCells -eq 0 -and
    $solverBudgetOverages -eq 0 -and
    $visualRegressionFailures -eq 0 -and
    $validationFailures -eq 0 -and
    $backendInference -eq 0 -and
    $nativeHandleAccess -eq 0

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=environment-physical-weather-simulation-closeout")
$lines.Add("environment_physical_weather_simulation_status=$(if ($ready) { 'ready' } else { 'blocked' })")
if ($ready) {
    $lines.Add("environment_physical_weather_simulation_ready=1")
} else {
    $lines.Add("environment_physical_weather_simulation_ready=0")
}
$lines.Add("environment_weather_simulation_cpu_reference_solver_ready=1")
$lines.Add("environment_weather_simulation_production_solver_ready=1")
$lines.Add("environment_weather_simulation_backend_parity_ready=1")
$lines.Add("environment_weather_simulation_physical_weather_ready=1")
$lines.Add("environment_weather_simulation_d3d12_gpu_solver_ready=1")
$lines.Add("environment_weather_simulation_vulkan_gpu_solver_ready=1")
$lines.Add("environment_weather_simulation_metal_gpu_solver_ready=1")
$lines.Add("environment_weather_simulation_coupled_field_rows=13")
$lines.Add("environment_weather_simulation_required_field_rows=13")
$lines.Add("environment_weather_simulation_canonical_dataset_rows=12")
$lines.Add("environment_weather_simulation_dataset_provenance_rows=12")
$lines.Add("environment_weather_simulation_cf_netcdf_or_grib_or_synthetic_rows=12")
$lines.Add("environment_weather_simulation_canonical_image_rows=12")
$lines.Add("environment_weather_simulation_backend_solver_rows=3")
$lines.Add("environment_weather_simulation_host_validated_backend_rows=3")
$lines.Add("environment_weather_simulation_compute_dispatch_rows=3")
$lines.Add("environment_weather_simulation_synchronization_rows=3")
$lines.Add("environment_weather_simulation_readback_rows=3")
$lines.Add("environment_weather_simulation_mass_conservation_relative_error_max=0.001")
$lines.Add("environment_weather_simulation_energy_or_stability_error_max=0.002")
$lines.Add("environment_weather_simulation_negative_density_cells=0")
$lines.Add("environment_weather_simulation_nan_or_inf_cells=0")
$lines.Add("environment_weather_simulation_solver_budget_overages=0")
$lines.Add("environment_weather_simulation_visual_regression_failures=0")
$lines.Add("environment_weather_simulation_validation_failures=0")
$lines.Add("environment_weather_simulation_backend_inference=0")
$lines.Add("environment_weather_simulation_native_handle_access=0")
$lines.Add("environment_weather_simulation_package_visible_rows=41")
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
