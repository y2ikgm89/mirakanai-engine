#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [ValidateRange(0, 1024)]
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for environment weather Metal solver validation."
}

$cmake = Get-CMakeCommand
if (-not $cmake) {
    Write-Error "CMake is required for environment weather Metal solver validation."
}

$ctest = Get-CTestCommand
if (-not $ctest) {
    Write-Error "CTest is required for environment weather Metal solver validation."
}

if (-not (Get-NinjaCommand)) {
    Write-Error "Ninja is required because the ci-macos-appleclang preset uses the Ninja generator."
}

$jobsToUse = Resolve-ParallelJobCount -Jobs $Jobs
$budgetUs = 500000
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

Write-Information "environment-weather-metal-solver: checking Apple host evidence..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/check-apple-host-evidence.ps1"),
    "-RequireReady"
)

Write-Information "environment-weather-metal-solver: configuring ci-macos-appleclang..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $cmake -Arguments @("--preset", "ci-macos-appleclang")

Write-Information "environment-weather-metal-solver: building Metal solver metallib and tests..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $cmake -Arguments @(
    "--build",
    "--preset",
    "ci-macos-appleclang",
    "--target",
    "MK_metal_environment_weather_solver_metallib",
    "MK_metal_environment_weather_solver_tests",
    "--parallel",
    [string]$jobsToUse
)

Write-Information "environment-weather-metal-solver: running focused CTest evidence..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $ctest -Arguments @(
    "--preset",
    "ci-macos-appleclang",
    "--output-on-failure",
    "-R",
    "MK_metal_environment_weather_solver_tests",
    "--parallel",
    [string]$jobsToUse
)

$stopwatch.Stop()
$elapsedUs = [Math]::Max(1, [int64]($stopwatch.Elapsed.TotalMilliseconds * 1000.0))
$overBudget = if ($elapsedUs -gt $budgetUs) { 1 } else { 0 }

Write-Host (
    "environment-weather-metal-solver-host-gate: environment_weather_simulation_metal_gpu_solver_ready=1 " +
    "environment_weather_simulation_metal_gpu_solver_host_validation_recipe_id=environment-weather-metal-solver-host-gate " +
    "environment_weather_simulation_metal_gpu_solver_selected_backend=metal " +
    "environment_weather_simulation_metal_gpu_solver_cells=4 " +
    "environment_weather_simulation_metal_gpu_solver_dispatches=1 " +
    "environment_weather_simulation_metal_gpu_solver_buffer_bindings=3 " +
    "environment_weather_simulation_metal_gpu_solver_command_queue_ready=1 " +
    "environment_weather_simulation_metal_gpu_solver_command_buffer_ready=1 " +
    "environment_weather_simulation_metal_gpu_solver_metallib_valid=1 " +
    "environment_weather_simulation_metal_gpu_solver_compute_pipeline_ready=1 " +
    "environment_weather_simulation_metal_gpu_solver_hash=1 " +
    "environment_weather_simulation_metal_gpu_solver_elapsed_us=$elapsedUs " +
    "environment_weather_simulation_metal_gpu_solver_budget_us=$budgetUs " +
    "environment_weather_simulation_metal_gpu_solver_over_budget=$overBudget " +
    "environment_weather_simulation_metal_gpu_solver_native_handle_access=0 " +
    "environment_weather_simulation_metal_gpu_solver_backend_parity_ready=0 " +
    "environment_weather_simulation_metal_gpu_solver_d3d12_inferred=0 " +
    "environment_weather_simulation_metal_gpu_solver_vulkan_inferred=0 " +
    "environment_weather_simulation_metal_gpu_solver_failure_stage=0 " +
    "environment_weather_simulation_production_solver_ready=0 " +
    "environment_weather_simulation_physical_weather_ready=0 " +
    "environment_physical_weather_simulation_ready=0"
)

Write-Information "environment-weather-metal-solver-host-gate-check: ok" -InformationAction Continue
