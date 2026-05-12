#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$tools = Assert-CppBuildTools
Invoke-CheckedCommand $tools.CMake --build --preset dev
Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure --timeout 300
