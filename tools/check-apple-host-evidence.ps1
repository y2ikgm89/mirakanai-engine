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

function Get-AppleHostEvidenceBlocker {
    param(
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$Message
    )

    return [pscustomobject]@{
        Kind = $Kind
        Message = $Message
    }
}

function Add-AppleHostEvidenceBlocker {
    param(
        [Parameter(Mandatory = $true)]$Blockers,
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $Blockers.Add((Get-AppleHostEvidenceBlocker -Kind $Kind -Message $Message)) | Out-Null
}

foreach ($file in @(
    "platform/ios/CMakeLists.txt",
    "platform/ios/Info.plist.in",
    "platform/ios/Sources/MirakanaiIOSApp/AppDelegate.mm",
    "platform/ios/Sources/MirakanaiIOSApp/IosMetalEvidence.metal",
    "tools/build-mobile-apple.ps1",
    "tools/smoke-ios-package.ps1",
    "tools/check-mobile-packaging.ps1"
)) {
    Assert-RequiredFile $file
}

Assert-FileContainsText "platform/ios/CMakeLists.txt" @(
    "IosMetalEvidence.metal",
    "default.metallib",
    "xcrun",
    "metallib"
)

Assert-FileContainsText "platform/ios/Sources/MirakanaiIOSApp/AppDelegate.mm" @(
    "newCommandQueue",
    "newComputePipelineStateWithFunction",
    "mirakanai_ios_metal_evidence.txt",
    "ios_metal_command_queue_ready=",
    "ios_metal_pipeline_ready=",
    "ios_metal_readback_ready="
)

Assert-FileContainsText "tools/smoke-ios-package.ps1" @(
    "ios_metal_evidence",
    "ios_metal_command_queue_ready=1",
    "ios_metal_pipeline_ready=1",
    "ios_metal_command_buffer_ready=1",
    "ios_metal_readback_ready=1"
)

Assert-FileContainsText ".github/workflows/ios-validate.yml" @(
    "runs-on: macos-26",
    "xcodebuild -version",
    "xcrun --sdk iphonesimulator --show-sdk-path",
    "./tools/check-mobile-packaging.ps1 -RequireApple",
    "./tools/validate-apple-metal-platform-host.ps1 -Platform ios -RequireReady -ExpectedEvidenceCounters `$expected",
    "ios_metal_command_buffer_ready=1"
)

Assert-FileContainsText ".github/workflows/validate.yml" @(
    "name: macOS Metal CMake",
    "runs-on: macos-latest",
    "name: Ensure build tools are available",
    'command -v ninja >/dev/null 2>&1 || { echo "ninja is required on macos-latest runner images"; exit 1; }',
    "ninja --version",
    "brew install ccache",
    "cmake --preset ci-macos-appleclang",
    'cmake --build --preset ci-macos-appleclang --parallel "$(sysctl -n hw.logicalcpu)"',
    'ctest --preset ci-macos-appleclang --output-on-failure --parallel "$(sysctl -n hw.logicalcpu)"'
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

$blockers = [System.Collections.Generic.List[object]]::new()
if (-not $hostIsMacOS) {
    Add-AppleHostEvidenceBlocker -Blockers $blockers -Kind "not_macos" -Message "Apple host evidence requires macOS."
}
if (-not $xcodeBuild) {
    Add-AppleHostEvidenceBlocker -Blockers $blockers -Kind "missing_xcodebuild" -Message "xcodebuild not found; install full Xcode and select it with xcode-select."
}
if (-not $xcrun) {
    Add-AppleHostEvidenceBlocker -Blockers $blockers -Kind "missing_xcrun" -Message "xcrun not found; full Xcode is required for simctl and Metal tool resolution."
}
if (-not $fullXcodeSelected) {
    Add-AppleHostEvidenceBlocker -Blockers $blockers -Kind "full_xcode_not_selected" -Message "Full Xcode is not selected as the active developer directory; Command Line Tools alone are insufficient for iOS Simulator validation."
}
if (-not $simulatorSdkAvailable) {
    Add-AppleHostEvidenceBlocker -Blockers $blockers -Kind "missing_iphonesimulator_sdk" -Message "iphonesimulator SDK is unavailable through xcrun."
}
if (-not $deviceSdkAvailable) {
    Add-AppleHostEvidenceBlocker -Blockers $blockers -Kind "missing_iphoneos_sdk" -Message "iphoneos SDK is unavailable through xcrun."
}
if (-not $simulatorRuntimeAvailable) {
    Add-AppleHostEvidenceBlocker -Blockers $blockers -Kind "missing_ios_simulator_runtime" -Message "No available iOS Simulator runtime was reported by xcrun simctl."
}
if (-not $metalCompilerAvailable) {
    Add-AppleHostEvidenceBlocker -Blockers $blockers -Kind "missing_metal" -Message "Metal compiler tool 'metal' is unavailable through xcrun."
}
if (-not $metallibAvailable) {
    Add-AppleHostEvidenceBlocker -Blockers $blockers -Kind "missing_metallib" -Message "Metal library tool 'metallib' is unavailable through xcrun."
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
    Write-Host "apple-host-evidence: blocker kind=$($blocker.Kind) - $($blocker.Message)"
}

if ($status -eq "ready") {
    Write-Host "apple-host-evidence-check: ready"
} else {
    Write-Host "apple-host-evidence-check: host-gated"
}

if ($RequireReady -and $status -ne "ready") {
    Write-Error "Apple host evidence is host-gated; run on a macOS host with full Xcode, iOS Simulator runtime, and Metal compiler tools."
}
