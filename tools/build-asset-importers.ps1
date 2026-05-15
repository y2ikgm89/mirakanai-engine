#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$vcpkgExe = Join-Path $root "external/vcpkg/vcpkg.exe"

if (-not (Test-Path $vcpkgExe)) {
    Write-Error "vcpkg is required for the asset importer build. Run 'pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1' first."
}

Set-MirakanaiVcpkgEnvironment | Out-Null

$tools = Assert-CppBuildTools

Invoke-CheckedCommand $tools.CMake --preset asset-importers
Invoke-CheckedCommand $tools.CMake --build --preset asset-importers
Invoke-CheckedCommand $tools.CTest --preset asset-importers --output-on-failure

$installPrefix = Join-Path (Get-RepoRoot) "out/install/asset-importers"
$vcpkgInstalled = Join-Path (Get-RepoRoot) "vcpkg_installed/x64-windows"
Invoke-CheckedCommand $tools.CMake --install (Join-Path (Get-RepoRoot) "out/build/asset-importers") --config Debug --prefix $installPrefix
& (Join-Path $PSScriptRoot "validate-installed-sdk.ps1") `
    -InstallPrefix $installPrefix `
    -BuildDir (Join-Path (Get-RepoRoot) "out/build/installed-asset-importers-consumer") `
    -BuildConfig Debug `
    -AdditionalRuntimePaths @((Join-Path $vcpkgInstalled "debug/bin")) `
    -AdditionalCMakeArgs @("-DCMAKE_PREFIX_PATH=$vcpkgInstalled")
