#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Resolve-ParallelJobCount {
    [CmdletBinding()]
    param(
        [Parameter()][ValidateRange(0, 1024)][int]$Jobs = 0
    )

    if ($Jobs -eq 0) {
        return [Math]::Max(1, [Environment]::ProcessorCount)
    }

    return $Jobs
}

$tools = Assert-CppBuildTools
$repositoryRoot = Get-RepoRoot
$presets = Read-CMakePresets
$buildDirectory = Resolve-CMakeConfigurePresetBinaryDirectory `
    -Presets $presets `
    -ConfigurePresetName "dev" `
    -SourceDirectory $repositoryRoot
if ([string]::IsNullOrWhiteSpace($buildDirectory)) {
    Write-Error "build: could not resolve CMake preset binaryDir for dev"
}

New-CMakeFileApiCodemodelQuery -BuildDir $buildDirectory

Invoke-CheckedCommand $tools.CMake --preset dev
$effectiveJobs = Resolve-ParallelJobCount -Jobs $Jobs
Write-Information "build: cmake parallel jobs=$effectiveJobs" -InformationAction Continue
Invoke-CheckedCommand $tools.CMake --build --preset dev --parallel $effectiveJobs
