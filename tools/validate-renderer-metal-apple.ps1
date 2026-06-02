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
    Write-Error "PowerShell 7 is required for renderer Metal Apple validation."
}

$cmake = Get-CMakeCommand
if (-not $cmake) {
    Write-Error "CMake is required for renderer Metal Apple validation."
}

$ctest = Get-CTestCommand
if (-not $ctest) {
    Write-Error "CTest is required for renderer Metal Apple validation."
}

if (-not (Get-NinjaCommand)) {
    Write-Error "Ninja is required because the ci-macos-appleclang preset uses the Ninja generator."
}

$jobsToUse = Resolve-ParallelJobCount -Jobs $Jobs

Write-Information "renderer-metal-apple: checking Apple host evidence..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/check-apple-host-evidence.ps1"),
    "-RequireReady"
)

Write-Information "renderer-metal-apple: configuring ci-macos-appleclang..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $cmake -Arguments @("--preset", "ci-macos-appleclang")

Write-Information "renderer-metal-apple: building Metal renderer evidence tests..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $cmake -Arguments @(
    "--build",
    "--preset",
    "ci-macos-appleclang",
    "--target",
    "MK_backend_scaffold_tests",
    "MK_renderer_quality_matrix_tests",
    "--parallel",
    [string]$jobsToUse
)

Write-Information "renderer-metal-apple: running focused CTest evidence..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $ctest -Arguments @(
    "--preset",
    "ci-macos-appleclang",
    "--output-on-failure",
    "-R",
    "MK_backend_scaffold_tests|MK_renderer_quality_matrix_tests",
    "--parallel",
    [string]$jobsToUse
)

Write-Information "renderer-metal-apple-check: ok" -InformationAction Continue
