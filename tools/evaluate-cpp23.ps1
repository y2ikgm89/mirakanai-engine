#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$Debug,
    [switch]$Gui,
    [switch]$Release,
    [ValidateRange(0, 1024)]
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "release-package-artifacts.ps1")

$tools = Assert-CppBuildTools
$effectiveJobs = Resolve-ParallelJobCount -Jobs $Jobs
Write-Information "cpp23-verification: cmake/ctest parallel jobs=$effectiveJobs" -InformationAction Continue

$runDebug = $Debug.IsPresent -or (-not $Release.IsPresent -and -not $Gui.IsPresent)

if ($runDebug) {
    Invoke-CheckedCommand $tools.CMake --preset cpp23-eval
    Invoke-CheckedCommand $tools.CMake --build --preset cpp23-eval --parallel $effectiveJobs
    Invoke-CheckedCommand $tools.CTest --preset cpp23-eval --output-on-failure --timeout 300 --parallel $effectiveJobs
}

if ($Release) {
    if (-not $tools.CPack) {
        Write-Error "CPack is required for the C++23 release package verification."
    }

    $releaseBuildDir = Join-Path (Get-RepoRoot) "out/build/cpp23-release-preset-eval"
    Invoke-CheckedCommand $tools.CMake --preset cpp23-release-eval
    Invoke-CheckedCommand $tools.CMake --build --preset cpp23-release-eval --parallel $effectiveJobs
    Invoke-CheckedCommand $tools.CTest --preset cpp23-release-eval --output-on-failure --timeout 300 --parallel $effectiveJobs
    $installPrefix = Join-Path (Get-RepoRoot) "out/install/cpp23-release-eval"
    Invoke-CheckedCommand $tools.CMake --install $releaseBuildDir --config Release --prefix $installPrefix
    & (Join-Path $PSScriptRoot "validate-installed-sdk.ps1") -InstallPrefix $installPrefix -BuildDir (Join-Path (Get-RepoRoot) "out/build/installed-consumer-cpp23-release-eval")
    Invoke-CheckedCommand $tools.CPack --preset cpp23-release-eval
    Assert-ReleasePackageArtifacts -BuildDir $releaseBuildDir
}

if ($Gui) {
    Write-Error "The C++23 GUI lane is deferred during SDL3 removal. MK_editor_core remains covered by the default Debug lane; a future visible editor shell must use first-party Win32/D3D12 adapters and must not depend on SDL3."
}

Write-Host "cpp23-verification: ok"
