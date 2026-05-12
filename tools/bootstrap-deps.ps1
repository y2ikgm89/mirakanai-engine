#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$vcpkgRoot = Join-Path $root "external/vcpkg"
$vcpkgExe = Join-Path $vcpkgRoot "vcpkg.exe"

if (-not (Test-Path $vcpkgRoot)) {
    Write-Error "Missing vcpkg checkout at external/vcpkg. This tree is the Microsoft vcpkg tool sources (gitignored), not CMake output—do not delete it as part of cleaning out/. Clone first: git clone https://github.com/microsoft/vcpkg.git external/vcpkg"
}

Set-MirakanaiVcpkgEnvironment | Out-Null
$triplet = Get-VcpkgDefaultTriplet

if (-not (Test-Path $vcpkgExe)) {
    $cmd = if ([string]::IsNullOrWhiteSpace($env:ComSpec)) { "cmd.exe" } else { $env:ComSpec }
    Invoke-CheckedCommand $cmd "/d" "/c" (Join-Path $vcpkgRoot "bootstrap-vcpkg.bat") "-disableMetrics"
}

Push-Location $root
try {
    Invoke-CheckedCommand $vcpkgExe install --x-feature=desktop-runtime --x-feature=desktop-gui --x-feature=asset-importers --triplet $triplet --disable-metrics
} finally {
    Pop-Location
}
