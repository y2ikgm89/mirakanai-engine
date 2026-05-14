#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "apple-host-helpers.ps1")

$root = Get-RepoRoot

function Assert-RequiredFile {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $path = Join-Path $root $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        Write-Error "Apple host evidence requires repository file: $RelativePath"
    }
}

function Assert-FileContainsText {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string[]]$Needles
    )

    Assert-RequiredFile $RelativePath
    $text = Get-Content -LiteralPath (Join-Path $root $RelativePath) -Raw
    foreach ($needle in $Needles) {
        if (-not $text.Contains($needle)) {
            Write-Error "$RelativePath did not contain required Apple host evidence text: $needle"
        }
    }
}

foreach ($file in @(
    "platform/ios/CMakeLists.txt",
    "platform/ios/Info.plist.in",
    "platform/ios/Sources/MirakanaiIOSApp/AppDelegate.mm",
    "tools/build-mobile-apple.ps1",
    "tools/smoke-ios-package.ps1",
    "tools/check-mobile-packaging.ps1"
)) {
    Assert-RequiredFile $file
}

Assert-FileContainsText ".github/workflows/ios-validate.yml" @(
    "runs-on: macos-26",
    "xcodebuild -version",
    "xcrun --sdk iphonesimulator --show-sdk-path",
    "./tools/check-mobile-packaging.ps1 -RequireApple",
    "./tools/smoke-ios-package.ps1 -Game sample_headless -Configuration Debug"
)

Assert-FileContainsText ".github/workflows/validate.yml" @(
    "name: macOS Metal CMake",
    "runs-on: macos-latest",
    "name: Ensure Ninja is available",
    "command -v ninja",
    "ninja --version",
    "brew install ninja",
    "cmake --preset ci-macos-appleclang",
    "cmake --build --preset ci-macos-appleclang",
    "ctest --preset ci-macos-appleclang --output-on-failure"
)

$hostIsMacOS = Test-IsMacOS
$hostName = if ($hostIsMacOS) {
    "macos"
} elseif ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Windows)) {
    "windows"
} elseif ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Linux)) {
    "linux"
} else {
    "unknown"
}

$xcodeBuild = Find-CommandOnCombinedPath "xcodebuild"
$xcrun = Find-CommandOnCombinedPath "xcrun"
$developerDirectory = Get-AppleDeveloperDirectory
$fullXcodeSelected = Test-FullXcodeDeveloperDirectory $developerDirectory
$simulatorSdkAvailable = Test-XcrunSdkAvailable $xcrun "iphonesimulator"
$deviceSdkAvailable = Test-XcrunSdkAvailable $xcrun "iphoneos"
$simulatorRuntimeAvailable = Test-IosSimulatorRuntimeAvailable $xcrun
$metalCompilerAvailable = Test-XcrunToolAvailable $xcrun "macosx" "metal"
$metallibAvailable = Test-XcrunToolAvailable $xcrun "macosx" "metallib"
$iosSigningConfigured = -not [string]::IsNullOrWhiteSpace((Get-EnvironmentVariableAnyScope "MK_IOS_DEVELOPMENT_TEAM")) -and
    -not [string]::IsNullOrWhiteSpace((Get-EnvironmentVariableAnyScope "MK_IOS_CODE_SIGN_IDENTITY"))

$blockers = [System.Collections.Generic.List[string]]::new()
if (-not $hostIsMacOS) {
    $blockers.Add("Apple host evidence requires macOS.") | Out-Null
}
if (-not $xcodeBuild) {
    $blockers.Add("xcodebuild not found; install full Xcode and select it with xcode-select.") | Out-Null
}
if (-not $xcrun) {
    $blockers.Add("xcrun not found; full Xcode is required for simctl and Metal tool resolution.") | Out-Null
}
if (-not $fullXcodeSelected) {
    $blockers.Add("Full Xcode is not selected as the active developer directory; Command Line Tools alone are insufficient for iOS Simulator validation.") | Out-Null
}
if (-not $simulatorSdkAvailable) {
    $blockers.Add("iphonesimulator SDK is unavailable through xcrun.") | Out-Null
}
if (-not $deviceSdkAvailable) {
    $blockers.Add("iphoneos SDK is unavailable through xcrun.") | Out-Null
}
if (-not $simulatorRuntimeAvailable) {
    $blockers.Add("No available iOS Simulator runtime was reported by xcrun simctl.") | Out-Null
}
if (-not $metalCompilerAvailable) {
    $blockers.Add("Metal compiler tool 'metal' is unavailable through xcrun.") | Out-Null
}
if (-not $metallibAvailable) {
    $blockers.Add("Metal library tool 'metallib' is unavailable through xcrun.") | Out-Null
}

$xcodeReady = $hostIsMacOS -and $xcodeBuild -and $xcrun -and $fullXcodeSelected
$iosSimulatorReady = $xcodeReady -and $simulatorSdkAvailable -and $simulatorRuntimeAvailable
$metalLibraryReady = $xcodeReady -and $metalCompilerAvailable -and $metallibAvailable
$status = if ($xcodeReady -and $iosSimulatorReady -and $metalLibraryReady) { "ready" } else { "host-gated" }

Write-Host "apple-host-evidence: host=$hostName"
Write-Host "apple-host-evidence: xcode=$(if ($xcodeReady) { "ready" } else { "blocked" })"
Write-Host "apple-host-evidence: ios-simulator=$(if ($iosSimulatorReady) { "ready" } else { "blocked" })"
Write-Host "apple-host-evidence: metal-library=$(if ($metalLibraryReady) { "ready" } else { "blocked" })"
Write-Host "apple-host-evidence: ios-signing=$(if ($iosSigningConfigured) { "configured" } else { "not-configured" })"
Write-Host "apple-host-evidence: workflow-ios=present"
Write-Host "apple-host-evidence: workflow-macos-metal=present"
foreach ($blocker in $blockers) {
    Write-Host "apple-host-evidence: blocker - $blocker"
}

if ($status -eq "ready") {
    Write-Host "apple-host-evidence-check: ready"
} else {
    Write-Host "apple-host-evidence-check: host-gated"
}

if ($RequireReady -and $status -ne "ready") {
    Write-Error "Apple host evidence is host-gated; run on a macOS host with full Xcode, iOS Simulator runtime, and Metal compiler tools."
}

