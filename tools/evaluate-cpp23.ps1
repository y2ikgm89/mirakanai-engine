#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$Gui,
    [switch]$Release
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "release-package-artifacts.ps1")

$tools = Assert-CppBuildTools

Invoke-CheckedCommand $tools.CMake --preset cpp23-eval
Invoke-CheckedCommand $tools.CMake --build --preset cpp23-eval
Invoke-CheckedCommand $tools.CTest --preset cpp23-eval --output-on-failure

if ($Release) {
    if (-not $tools.CPack) {
        Write-Error "CPack is required for the C++23 release package verification."
    }

    $releaseBuildDir = Join-Path (Get-RepoRoot) "out/build/cpp23-release-preset-eval"
    Invoke-CheckedCommand $tools.CMake --preset cpp23-release-eval
    Invoke-CheckedCommand $tools.CMake --build --preset cpp23-release-eval
    Invoke-CheckedCommand $tools.CTest --preset cpp23-release-eval --output-on-failure
    $installPrefix = Join-Path (Get-RepoRoot) "out/install/cpp23-release-eval"
    Invoke-CheckedCommand $tools.CMake --install $releaseBuildDir --config Release --prefix $installPrefix
    & (Join-Path $PSScriptRoot "validate-installed-sdk.ps1") -InstallPrefix $installPrefix -BuildDir (Join-Path (Get-RepoRoot) "out/build/installed-consumer-cpp23-release-eval")
    Invoke-CheckedCommand $tools.CPack --preset cpp23-release-eval
    Assert-ReleasePackageArtifacts -BuildDir $releaseBuildDir
}

if ($Gui) {
    $root = Get-RepoRoot
    $vcpkgExe = Join-Path $root "external/vcpkg/vcpkg.exe"
    if (-not (Test-Path $vcpkgExe)) {
        Write-Error "vcpkg is required for the C++23 desktop GUI verification. Run 'pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1' first."
    }

    Set-MirakanaiVcpkgEnvironment | Out-Null

    Invoke-CheckedCommand $tools.CMake --preset cpp23-desktop-gui-eval
    Invoke-CheckedCommand $tools.CMake --build --preset cpp23-desktop-gui-eval
    Invoke-CheckedCommand $tools.CTest --preset cpp23-desktop-gui-eval --output-on-failure
}

Write-Host "cpp23-verification: ok"
