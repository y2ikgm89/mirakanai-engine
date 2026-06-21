#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [ValidateSet("desktop-runtime", "asset-importers", "physics-jolt", "network-enet", "directstorage-win32")]
    [string[]]$Feature = @(),

    [switch]$All
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$vcpkgRoot = Get-VcpkgRoot
$vcpkgExe = Get-VcpkgExecutablePath
$bootstrapScript = Get-VcpkgBootstrapScriptPath

if (-not (Test-Path -LiteralPath $vcpkgRoot -PathType Container)) {
    Write-Error "Missing vcpkg checkout at external/vcpkg. This tree is the Microsoft vcpkg tool sources (gitignored), not CMake output—do not delete it as part of cleaning out/. Clone first: git clone https://github.com/microsoft/vcpkg.git external/vcpkg"
}

Set-MirakanaiVcpkgEnvironment | Out-Null
$triplet = Get-VcpkgDefaultTriplet

if (-not (Test-Path -LiteralPath $vcpkgExe -PathType Leaf)) {
    if (-not (Test-Path -LiteralPath $bootstrapScript -PathType Leaf)) {
        Write-Error "Missing vcpkg bootstrap script (bootstrap-vcpkg.bat/bootstrap-vcpkg.sh): $bootstrapScript"
    }

    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        $cmd = if ([string]::IsNullOrWhiteSpace($env:ComSpec)) { "cmd.exe" } else { $env:ComSpec }
        Invoke-CheckedCommand $cmd "/d" "/c" $bootstrapScript "-disableMetrics"
    } else {
        Invoke-CheckedCommand "sh" $bootstrapScript "-disableMetrics"
    }
}

Push-Location $root
try {
    $allFeatures = @("desktop-runtime", "asset-importers", "physics-jolt", "network-enet", "directstorage-win32")
    $selectedFeatures = if ($All -or $Feature.Count -eq 0) { $allFeatures } else { $Feature }
    $installArgs = @("install")
    foreach ($selectedFeature in $selectedFeatures) {
        $installArgs += "--x-feature=$selectedFeature"
    }
    $installArgs += @("--triplet", $triplet, "--disable-metrics")
    Invoke-CheckedCommand $vcpkgExe @installArgs
} finally {
    Pop-Location
}
