#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Assert-Equal {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$Actual,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($Actual -ne $Expected) {
        Write-Error "$Label expected '$Expected' but got '$Actual'."
    }
}

function Assert-ContainsText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not $Text.Contains($Needle)) {
        Write-Error "$Label did not contain expected text: $Needle"
    }
}

$oldDownloads = [Environment]::GetEnvironmentVariable("VCPKG_DOWNLOADS", "Process")
$oldDefaultCache = [Environment]::GetEnvironmentVariable("VCPKG_DEFAULT_BINARY_CACHE", "Process")
$oldBinarySources = [Environment]::GetEnvironmentVariable("VCPKG_BINARY_SOURCES", "Process")
$oldDisableMetrics = [Environment]::GetEnvironmentVariable("VCPKG_DISABLE_METRICS", "Process")
$oldForceSystemBinaries = [Environment]::GetEnvironmentVariable("VCPKG_FORCE_SYSTEM_BINARIES", "Process")
$oldPath = [Environment]::GetEnvironmentVariable("Path", "Process")

try {
    [Environment]::SetEnvironmentVariable("VCPKG_DOWNLOADS", $null, "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_DEFAULT_BINARY_CACHE", $null, "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_BINARY_SOURCES", $null, "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_DISABLE_METRICS", $null, "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_FORCE_SYSTEM_BINARIES", $null, "Process")

    $cacheRoot = Join-Path (Get-RepoRoot) "out/vcpkg-environment-check"
    $state = Set-MirakanaiVcpkgEnvironment -CacheRoot $cacheRoot
    $expectedDownloads = [System.IO.Path]::GetFullPath((Join-Path $cacheRoot "downloads"))
    $expectedBinaryCache = [System.IO.Path]::GetFullPath((Join-Path $cacheRoot "binary-cache"))
    $expectedBinarySources = "clear;files,$expectedBinaryCache,readwrite"

    Assert-Equal ([Environment]::GetEnvironmentVariable("VCPKG_DOWNLOADS", "Process")) $expectedDownloads "VCPKG_DOWNLOADS"
    Assert-Equal ([Environment]::GetEnvironmentVariable("VCPKG_DEFAULT_BINARY_CACHE", "Process")) $expectedBinaryCache "VCPKG_DEFAULT_BINARY_CACHE"
    Assert-Equal ([Environment]::GetEnvironmentVariable("VCPKG_BINARY_SOURCES", "Process")) $expectedBinarySources "VCPKG_BINARY_SOURCES"
    Assert-Equal ([Environment]::GetEnvironmentVariable("VCPKG_DISABLE_METRICS", "Process")) "1" "VCPKG_DISABLE_METRICS"
    Assert-Equal $state.Downloads $expectedDownloads "vcpkg environment state downloads"
    Assert-Equal $state.BinaryCache $expectedBinaryCache "vcpkg environment state binary cache"
    if (-not (Test-Path -LiteralPath $expectedDownloads -PathType Container)) {
        Write-Error "Set-MirakanaiVcpkgEnvironment must create the downloads directory."
    }
    if (-not (Test-Path -LiteralPath $expectedBinaryCache -PathType Container)) {
        Write-Error "Set-MirakanaiVcpkgEnvironment must create the binary cache directory."
    }

    $fake7zipDir = Join-Path $expectedDownloads "tools/7zip-26.00-windows"
    $fake7zrDir = Join-Path $expectedDownloads "tools/7zr-26.00-windows"
    $fakeCmakeDir = Join-Path $expectedDownloads "tools/cmake-4.2.3-windows/cmake-4.2.3-windows-x86_64/bin"
    $fakeNinjaDir = Join-Path $expectedDownloads "tools/ninja-1.13.2-windows"
    foreach ($directory in @($fake7zipDir, $fake7zrDir, $fakeCmakeDir, $fakeNinjaDir)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
    foreach ($file in @(
            (Join-Path $fake7zipDir "7z.exe"),
            (Join-Path $fake7zrDir "7zr.exe"),
            (Join-Path $fakeCmakeDir "cmake.exe"),
            (Join-Path $fakeNinjaDir "ninja.exe")
        )) {
        Set-Content -LiteralPath $file -Value "" -NoNewline
    }
    Set-MirakanaiVcpkgEnvironment -CacheRoot $cacheRoot | Out-Null
    Assert-Equal ([Environment]::GetEnvironmentVariable("VCPKG_FORCE_SYSTEM_BINARIES", "Process")) "1" "VCPKG_FORCE_SYSTEM_BINARIES"
    $pathAfterToolDiscovery = [Environment]::GetEnvironmentVariable("Path", "Process")
    foreach ($directory in @($fake7zipDir, $fake7zrDir, $fakeCmakeDir, $fakeNinjaDir)) {
        if ($pathAfterToolDiscovery.IndexOf($directory, [System.StringComparison]::OrdinalIgnoreCase) -lt 0) {
            Write-Error "Set-MirakanaiVcpkgEnvironment must add existing vcpkg tool directory to Path: $directory"
        }
    }

    $pwsh = (Get-Process -Id $PID).Path
    $pathProbe = [System.IO.Path]::GetFullPath((Join-Path $cacheRoot "path-probe"))
    New-Item -ItemType Directory -Path $pathProbe -Force | Out-Null
    [Environment]::SetEnvironmentVariable("Path", "$pathProbe;$oldPath", "Process")
    $escapedPathProbe = $pathProbe.Replace("'", "''")
Invoke-CheckedCommand $pwsh -NoProfile -Command @"
`$pathValue = [Environment]::GetEnvironmentVariable('Path', 'Process')
if (`$pathValue.IndexOf('$escapedPathProbe', [System.StringComparison]::OrdinalIgnoreCase) -lt 0) { exit 33 }
"@

    $commonContent = Get-Content -LiteralPath (Join-Path $PSScriptRoot "common.ps1") -Raw
    if ($commonContent.Contains("RedirectStandardInput = `$true") -or $commonContent.Contains("StandardInput.Close()")) {
        Write-Error "Invoke-CheckedCommand must not synthesize stdin handles for child tools; vcpkg isolation belongs in explicit bootstrap and CMake manifest-install policy."
    }

    foreach ($script in @(
            "bootstrap-deps.ps1",
            "build-gui.ps1",
            "validate-desktop-game-runtime.ps1",
            "package-desktop-runtime.ps1",
            "build-asset-importers.ps1",
            "evaluate-cpp23.ps1"
        )) {
        $content = Get-Content -LiteralPath (Join-Path $PSScriptRoot $script) -Raw
        Assert-ContainsText $content "Set-MirakanaiVcpkgEnvironment" "$script vcpkg environment setup"
    }
} finally {
    [Environment]::SetEnvironmentVariable("VCPKG_DOWNLOADS", $oldDownloads, "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_DEFAULT_BINARY_CACHE", $oldDefaultCache, "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_BINARY_SOURCES", $oldBinarySources, "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_DISABLE_METRICS", $oldDisableMetrics, "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_FORCE_SYSTEM_BINARIES", $oldForceSystemBinaries, "Process")
    [Environment]::SetEnvironmentVariable("Path", $oldPath, "Process")
}

Write-Host "vcpkg-environment-check: ok"
