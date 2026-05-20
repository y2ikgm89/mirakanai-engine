#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

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
Invoke-CheckedCommand $tools.CMake --build --preset dev
