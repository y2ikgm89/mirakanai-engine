#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$vcpkgExe = Join-Path $root "external/vcpkg/vcpkg.exe"

if (-not (Test-Path $vcpkgExe)) {
    Write-Error "vcpkg is required for the desktop GUI build. Run 'pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1' first."
}

Set-MirakanaiVcpkgEnvironment | Out-Null

$tools = Assert-CppBuildTools

Invoke-CheckedCommand $tools.CMake --preset desktop-gui
Invoke-CheckedCommand $tools.CMake --build --preset desktop-gui
Invoke-CheckedCommand $tools.CTest --preset desktop-gui --output-on-failure

