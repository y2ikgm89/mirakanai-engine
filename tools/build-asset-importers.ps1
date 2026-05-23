#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$null = Assert-VcpkgExecutable -Purpose "the asset importer build"

Set-MirakanaiVcpkgEnvironment | Out-Null

$tools = Assert-CppBuildTools

Invoke-CheckedCommand $tools.CMake --preset asset-importers
Invoke-CheckedCommand $tools.CMake --build --preset asset-importers
Invoke-CheckedCommand $tools.CTest --preset asset-importers --output-on-failure

$installPrefix = Join-Path $root "out/install/asset-importers"
$vcpkgInstalled = Join-Path $root "vcpkg_installed/x64-windows"
Invoke-CheckedCommand $tools.CMake --install (Join-Path $root "out/build/asset-importers") --config Debug --prefix $installPrefix
& (Join-Path $PSScriptRoot "validate-installed-sdk.ps1") `
    -InstallPrefix $installPrefix `
    -BuildDir (Join-Path $root "out/build/installed-asset-importers-consumer") `
    -BuildConfig Debug `
    -AdditionalRuntimePaths @((Join-Path $vcpkgInstalled "debug/bin")) `
    -AdditionalCMakeArgs @("-DCMAKE_PREFIX_PATH=$vcpkgInstalled")
