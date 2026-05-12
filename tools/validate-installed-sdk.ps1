#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$InstallPrefix = "",
    [string]$BuildDir = "",
    [string]$BuildConfig = "Release",
    [string[]]$AdditionalRuntimePaths = @(),
    [string[]]$AdditionalCMakeArgs = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "installed-sdk-validation.ps1")

$root = Get-RepoRoot
$tools = Assert-CppBuildTools

if ([string]::IsNullOrWhiteSpace($InstallPrefix)) {
    $InstallPrefix = Join-Path $root "out/install/release"
}
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $root "out/build/installed-consumer"
}

$installPrefixPath = [System.IO.Path]::GetFullPath($InstallPrefix)
$buildPath = [System.IO.Path]::GetFullPath($BuildDir)
$configDir = Join-Path $installPrefixPath "lib/cmake/Mirakanai"
$exampleSource = Join-Path $root "examples/installed_consumer"

Assert-InstalledSdkMetadata -InstallPrefix $installPrefixPath
if (-not (Test-Path $exampleSource)) {
    Write-Error "Installed SDK consumer example source was not found: $exampleSource"
}

$configureArguments = @(
    "-S",
    $exampleSource,
    "-B",
    $buildPath,
    "-DMirakanai_DIR=$configDir",
    "-DCMAKE_BUILD_TYPE=$BuildConfig"
) + $AdditionalCMakeArgs

Invoke-CheckedCommand $tools.CMake @configureArguments
Invoke-CheckedCommand $tools.CMake --build $buildPath --config $BuildConfig

$runtimePaths = @()
$installedBin = Join-Path $installPrefixPath "bin"
if (Test-Path -LiteralPath $installedBin -PathType Container) {
    $runtimePaths += $installedBin
}
foreach ($runtimePath in $AdditionalRuntimePaths) {
    if (-not [string]::IsNullOrWhiteSpace($runtimePath)) {
        $runtimePaths += [System.IO.Path]::GetFullPath($runtimePath)
    }
}
if ($runtimePaths.Count -gt 0) {
    $env:PATH = (($runtimePaths + $env:PATH) -join [System.IO.Path]::PathSeparator)
}

Invoke-CheckedCommand $tools.CTest --test-dir $buildPath --build-config $BuildConfig --output-on-failure

Write-Host "installed-sdk-validation: ok"
