#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$SkipBuild,
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$tools = Assert-CppBuildTools
$effectiveJobs = Resolve-ParallelJobCount -Jobs $Jobs
if (-not $SkipBuild.IsPresent) {
    Write-Information "test: cmake parallel jobs=$effectiveJobs" -InformationAction Continue
    Invoke-CheckedCommand $tools.CMake --build --preset dev --parallel $effectiveJobs
}
Write-Information "test: ctest parallel jobs=$effectiveJobs" -InformationAction Continue
Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure --timeout 300 --parallel $effectiveJobs
