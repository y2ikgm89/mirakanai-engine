#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "release-package-artifacts.ps1")

$tools = Assert-CppBuildTools
if (-not $tools.CPack) {
    Write-Error "CPack is required but was not found. Install official CMake 3.30+ or Visual Studio Build Tools with C++ CMake tools."
}

Invoke-CheckedCommand $tools.CMake --preset release
Invoke-CheckedCommand $tools.CMake --build --preset release
Invoke-CheckedCommand $tools.CTest --preset release --output-on-failure

$installPrefix = Join-Path (Get-RepoRoot) "out/install/release"
Invoke-CheckedCommand $tools.CMake --install (Join-Path (Get-RepoRoot) "out/build/release") --config Release --prefix $installPrefix
& (Join-Path $PSScriptRoot "validate-installed-sdk.ps1") -InstallPrefix $installPrefix

Invoke-CheckedCommand $tools.CPack --preset release
Assert-ReleasePackageArtifacts -BuildDir (Join-Path (Get-RepoRoot) "out/build/release")
Write-Host "release-package-artifacts: ok"

Write-Host "package: ok"
