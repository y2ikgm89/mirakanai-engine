#requires -Version 7.0
#requires -PSEdition Core

# SPDX-FileCopyrightText: 2026 GameEngine contributors
# SPDX-License-Identifier: LicenseRef-Proprietary

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Push-Location $root
try {
    if (-not $IsWindows) {
        Write-Error "MK_editor_game_module_driver_load_tests is registered only for WIN32 CMake targets; run on Windows with MSVC dev preset tooling."
    }

    $tools = Assert-CppBuildTools
    Invoke-CheckedCommand $tools.CMake --preset dev
    Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_editor_game_module_driver_probe MK_editor_game_module_driver_load_tests
    Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_editor_game_module_driver_load_tests
}
finally {
    Pop-Location
}
