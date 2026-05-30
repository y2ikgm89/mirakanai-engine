#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$null = Assert-VcpkgExecutable -Purpose "the native desktop GUI build"
Set-MirakanaiVcpkgEnvironment | Out-Null
$tools = Assert-CppBuildTools

Invoke-CheckedCommand $tools.CMake --preset desktop-gui
Invoke-CheckedCommand $tools.CMake --build --preset desktop-gui
Invoke-CheckedCommand $tools.CTest --preset desktop-gui --output-on-failure
