#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$null = Assert-VcpkgExecutable -Purpose "the desktop GUI build"

Set-MirakanaiVcpkgEnvironment | Out-Null

$tools = Assert-CppBuildTools

Write-Information "desktop-gui: visible editor shell is deferred; verifying editor-core coverage with MK_ENABLE_DESKTOP_GUI=OFF." -InformationAction Continue
Invoke-CheckedCommand $tools.CMake --preset desktop-gui
Invoke-CheckedCommand $tools.CMake --build --preset desktop-gui
Invoke-CheckedCommand $tools.CTest --preset desktop-gui --output-on-failure
