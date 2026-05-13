#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$Game = "sample_headless",
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [ValidateSet("Simulator", "Device")]
    [string]$Platform = "Simulator",
    [string]$BundleIdentifier = "",
    [string]$DevelopmentTeam = $env:MK_IOS_DEVELOPMENT_TEAM,
    [string]$CodeSignIdentity = $env:MK_IOS_CODE_SIGN_IDENTITY
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$gameManifest = Join-Path $root "games\$Game\game.agent.json"
if (-not (Test-Path $gameManifest)) {
    Write-Error "Game manifest not found for Apple packaging: games/$Game/game.agent.json"
}

if ([string]::IsNullOrWhiteSpace($BundleIdentifier)) {
    $BundleIdentifier = $env:MK_IOS_BUNDLE_IDENTIFIER
}

if ([string]::IsNullOrWhiteSpace($BundleIdentifier)) {
    $BundleIdentifier = "dev.mirakanai.ios"
}

if ($Configuration -eq "Release" -and [string]::IsNullOrWhiteSpace($DevelopmentTeam)) {
    Write-Error "Apple Release packaging requires MK_IOS_DEVELOPMENT_TEAM or -DevelopmentTeam for Xcode signing."
}

& (Join-Path $PSScriptRoot "check-mobile-packaging.ps1") -RequireApple

$tools = Assert-CppBuildTools
$sdk = if ($Platform -eq "Device") { "iphoneos" } else { "iphonesimulator" }
$codeSigningAllowed = if ($Platform -eq "Device" -or -not [string]::IsNullOrWhiteSpace($DevelopmentTeam)) { "YES" } else { "NO" }
$buildDir = Join-Path $root "out\build\ios-$Platform-$Game-$Configuration"
$installDir = Join-Path $root "out\mobile\ios\$Platform\$Game\$Configuration"

Invoke-CheckedCommand $tools.CMake `
    "-S" (Join-Path $root "platform\ios") `
    "-B" $buildDir `
    "-G" "Xcode" `
    "-DCMAKE_SYSTEM_NAME=iOS" `
    "-DCMAKE_OSX_SYSROOT=$sdk" `
    "-DMK_ENABLE_CXX_MODULE_SCANNING=OFF" `
    "-DMK_ENABLE_IMPORT_STD=OFF" `
    "-DMK_IOS_GAME_NAME=$Game" `
    "-DMK_IOS_GAME_MANIFEST=$gameManifest" `
    "-DMK_IOS_BUNDLE_IDENTIFIER=$BundleIdentifier" `
    "-DMK_IOS_DEVELOPMENT_TEAM=$DevelopmentTeam" `
    "-DMK_IOS_CODE_SIGN_IDENTITY=$CodeSignIdentity" `
    "-DMK_IOS_CODE_SIGNING_ALLOWED=$codeSigningAllowed" `
    "-DCMAKE_INSTALL_PREFIX=$installDir"

Invoke-CheckedCommand $tools.CMake --build $buildDir --config $Configuration

