#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$RequireAndroid,
    [switch]$RequireApple
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "apple-host-helpers.ps1")

$root = Get-RepoRoot

function Assert-TemplateFile {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $path = Join-Path $root $RelativePath
    if (-not (Test-Path $path)) {
        Write-Error "Missing mobile packaging template file: $RelativePath"
    }
}

function Assert-TemplateText {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string[]]$Needles
    )

    $path = Join-Path $root $RelativePath
    if (-not (Test-Path $path)) {
        Write-Error "Missing mobile packaging template file: $RelativePath"
    }

    $content = Get-Content -LiteralPath $path -Raw
    foreach ($needle in $Needles) {
        if (-not $content.Contains($needle)) {
            Write-Error "Mobile packaging template $RelativePath missing required text: $needle"
        }
    }
}

function Get-MobilePackagingBlocker {
    param(
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$Message
    )

    return [pscustomobject]@{
        Kind = $Kind
        Message = $Message
    }
}

function Add-MobilePackagingBlocker {
    param(
        [Parameter(Mandatory = $true)]$Blockers,
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $Blockers.Add((Get-MobilePackagingBlocker -Kind $Kind -Message $Message)) | Out-Null
}

function Write-MobilePackagingBlocker {
    param(
        [Parameter(Mandatory = $true)][string]$Prefix,
        [Parameter(Mandatory = $true)]$Blockers
    )

    foreach ($blocker in $Blockers) {
        Write-Information "${Prefix}: blocker kind=$($blocker.Kind) - $($blocker.Message)" -InformationAction Continue
    }
}

function Find-AndroidSdk {
    $localAppData = Get-LocalApplicationDataRoot
    $candidates = @(
        $env:ANDROID_HOME,
        $env:ANDROID_SDK_ROOT,
        (Get-EnvironmentVariableAnyScope "ANDROID_HOME"),
        (Get-EnvironmentVariableAnyScope "ANDROID_SDK_ROOT"),
        (Join-OptionalPath $localAppData "Android\Sdk")
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

function Find-AndroidNdk {
    param([string]$SdkRoot)

    $candidates = @(
        $env:ANDROID_NDK_HOME,
        $env:ANDROID_NDK_ROOT,
        (Get-EnvironmentVariableAnyScope "ANDROID_NDK_HOME"),
        (Get-EnvironmentVariableAnyScope "ANDROID_NDK_ROOT")
    ) | Where-Object {
        -not [string]::IsNullOrWhiteSpace($_)
    }
    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($SdkRoot)) {
        $ndkRoot = Join-Path $SdkRoot "ndk"
        if (Test-Path -LiteralPath $ndkRoot) {
            $latest = Get-ChildItem -LiteralPath $ndkRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
            if ($latest) {
                return $latest.FullName
            }
        }
    }

    return $null
}

function Find-GradleCommand {
    $gradle = Find-CommandOnCombinedPath "gradle"
    if ($gradle) {
        return $gradle
    }

    $toolchainsRoot = Join-OptionalPath (Get-LocalApplicationDataRoot) "GameEngineToolchains"
    if (-not [string]::IsNullOrWhiteSpace($toolchainsRoot) -and (Test-Path -LiteralPath $toolchainsRoot)) {
        $candidate = Get-ChildItem -LiteralPath $toolchainsRoot -Directory -Filter "gradle-*" |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName "bin\gradle.bat" } |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
            Select-Object -First 1
        if ($candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return $null
}

function Find-JavaCommand {
    $javaHome = Get-EnvironmentVariableAnyScope "JAVA_HOME"
    if (-not [string]::IsNullOrWhiteSpace($javaHome)) {
        $candidate = Join-Path $javaHome "bin\java.exe"
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    $java = Find-CommandOnCombinedPath "java"
    if ($java) {
        return $java
    }

    $adoptiumRoot = Join-OptionalPath (Get-EnvironmentVariableAnyScope "ProgramFiles") "Eclipse Adoptium"
    if (-not [string]::IsNullOrWhiteSpace($adoptiumRoot) -and (Test-Path -LiteralPath $adoptiumRoot)) {
        $candidate = Get-ChildItem -LiteralPath $adoptiumRoot -Directory -Filter "jdk-*" |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName "bin\java.exe" } |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
            Select-Object -First 1
        if ($candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return $null
}

function Test-EnvironmentSet {
    param([Parameter(Mandatory = $true)][string[]]$Names)

    foreach ($name in $Names) {
        if ([string]::IsNullOrWhiteSpace((Get-EnvironmentVariableAnyScope $name))) {
            return $false
        }
    }

    return $true
}

function Test-MobilePackagingDeviceProbeEnabled {
    param([bool]$RequireAndroidPackaging = $false)

    $override = Get-EnvironmentVariableAnyScope "MK_MOBILE_DEVICE_PROBE"
    if ($override -match "^(1|true|yes|on)$") {
        return $true
    }
    if ($override -match "^(0|false|no|off)$") {
        return $false
    }

    if ($RequireAndroidPackaging) {
        return $true
    }

    return (Get-EnvironmentVariableAnyScope "GITHUB_ACTIONS") -ine "true"
}

function Invoke-MobilePackagingProbe {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$ArgumentList = @(),
        [int]$TimeoutSeconds = 5,
        [int]$OutputWaitMilliseconds = 1000
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    foreach ($argument in $ArgumentList) {
        if ($null -ne $argument) {
            $null = $startInfo.ArgumentList.Add([string]$argument)
        }
    }

    $timeoutMilliseconds = [Math]::Max(1, $TimeoutSeconds) * 1000
    $childProcess = [System.Diagnostics.Process]::new()
    $childProcess.StartInfo = $startInfo

    try {
        $null = $childProcess.Start()
        $childProcess.StandardInput.Close()
        $standardOutputTask = $childProcess.StandardOutput.ReadToEndAsync()
        $standardErrorTask = $childProcess.StandardError.ReadToEndAsync()
        $timedOut = -not $childProcess.WaitForExit($timeoutMilliseconds)
        if ($timedOut) {
            try {
                $childProcess.Kill($true)
            }
            catch {
                $null = $_.Exception
            }

            if (-not $childProcess.WaitForExit(2000)) {
                return [pscustomobject]@{
                    TimedOut  = $true
                    ExitCode  = $null
                    Lines     = @()
                    ErrorText = ""
                }
            }
        }
        $standardOutputCompleted = $standardOutputTask.Wait($OutputWaitMilliseconds)
        $standardErrorCompleted = $standardErrorTask.Wait($OutputWaitMilliseconds)
        if (-not $standardOutputCompleted) {
            try {
                $childProcess.StandardOutput.Dispose()
            }
            catch {
                $null = $_.Exception
            }
        }
        if (-not $standardErrorCompleted) {
            try {
                $childProcess.StandardError.Dispose()
            }
            catch {
                $null = $_.Exception
            }
        }
        $standardOutput = if ($standardOutputCompleted) { $standardOutputTask.Result } else { "" }
        $standardError = if ($standardErrorCompleted) { $standardErrorTask.Result } else { "" }
        $probeTimedOut = $timedOut -or -not $standardOutputCompleted -or -not $standardErrorCompleted
        $lines = @($standardOutput -split "\r?\n" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })

        return [pscustomobject]@{
            TimedOut  = $probeTimedOut
            ExitCode  = $(if ($probeTimedOut) { $null } else { $childProcess.ExitCode })
            Lines     = $lines
            ErrorText = $standardError
        }
    }
    catch {
        return [pscustomobject]@{
            TimedOut  = $false
            ExitCode  = $null
            Lines     = @()
            ErrorText = $_.Exception.Message
        }
    }
    finally {
        $childProcess.Dispose()
    }
}

function Get-MobilePackagingAndroidAvdNameList {
    param(
        [string]$Emulator,
        [int]$TimeoutSeconds = 5
    )

    if ([string]::IsNullOrWhiteSpace($Emulator)) {
        return [pscustomobject]@{
            TimedOut = $false
            Names    = @()
        }
    }

    Set-AndroidAvdHomeEnvironment | Out-Null
    $probe = Invoke-MobilePackagingProbe -FilePath $Emulator -ArgumentList @("-list-avds") -TimeoutSeconds $TimeoutSeconds
    if ($probe.TimedOut -or $probe.ExitCode -ne 0) {
        return [pscustomobject]@{
            TimedOut = $probe.TimedOut
            Names    = @()
        }
    }

    return [pscustomobject]@{
        TimedOut = $false
        Names    = @($probe.Lines | ForEach-Object { ([string]$_).Trim() } | Where-Object {
                -not [string]::IsNullOrWhiteSpace($_)
            })
    }
}

foreach ($file in @(
    "platform/android/settings.gradle.kts",
    "platform/android/build.gradle.kts",
    "platform/android/app/build.gradle.kts",
    "platform/android/app/src/main/AndroidManifest.xml",
    "platform/android/app/src/main/cpp/CMakeLists.txt",
    "platform/android/app/src/main/cpp/android_audio_output.cpp",
    "platform/android/app/src/main/cpp/android_audio_output.hpp",
    "platform/android/app/src/main/cpp/game_activity_bridge.cpp",
    "platform/android/app/src/main/cpp/android_vulkan_surface_bridge.cpp",
    "platform/android/app/src/main/java/dev/mirakanai/android/MirakanaiActivity.java",
    "platform/android/app/src/main/assets/.gitkeep",
    "platform/ios/CMakeLists.txt",
    "platform/ios/Info.plist.in",
    "platform/ios/Sources/MirakanaiIOSApp/AppDelegate.mm",
    "platform/ios/Resources/.gitkeep"
)) {
    Assert-TemplateFile $file
}

Assert-TemplateText "tools/build-mobile-apple.ps1" @(
    "-DBUILD_TESTING=OFF",
    "--target `"MirakanaiIOS`""
)
Assert-TemplateText "platform/ios/Info.plist.in" @(
    '<key>CFBundleExecutable</key>',
    '${MACOSX_BUNDLE_EXECUTABLE_NAME}'
)

$androidBlockers = [System.Collections.Generic.List[object]]::new()
$appleBlockers = [System.Collections.Generic.List[object]]::new()

$androidSdk = Find-AndroidSdk
if (-not $androidSdk) {
    Add-MobilePackagingBlocker -Blockers $androidBlockers -Kind "missing_android_sdk" -Message "Android SDK not found; set ANDROID_HOME or ANDROID_SDK_ROOT"
}

$androidNdk = Find-AndroidNdk $androidSdk
if (-not $androidNdk) {
    Add-MobilePackagingBlocker -Blockers $androidBlockers -Kind "missing_android_ndk" -Message "Android NDK not found; install it through the Android SDK manager or set ANDROID_NDK_HOME"
}

if (-not (Find-GradleCommand)) {
    Add-MobilePackagingBlocker -Blockers $androidBlockers -Kind "missing_gradle" -Message "Gradle not found on PATH; install official Gradle or add a reviewed Gradle wrapper later"
}

if (-not (Find-JavaCommand)) {
    Add-MobilePackagingBlocker -Blockers $androidBlockers -Kind "missing_jdk" -Message "JDK not found on PATH; Android Gradle Plugin requires a compatible JDK"
}

$adb = Find-AndroidPlatformToolCommand "adb"
if (-not $adb) {
    Add-MobilePackagingBlocker -Blockers $androidBlockers -Kind "missing_adb" -Message "adb not found; install Android SDK Platform Tools for device/emulator smoke"
}

$apksigner = Find-AndroidBuildToolCommand "apksigner"
if (-not $apksigner) {
    Add-MobilePackagingBlocker -Blockers $androidBlockers -Kind "missing_apksigner" -Message "apksigner not found; install Android SDK Build Tools for Release signing verification"
}

$emulator = Find-AndroidEmulatorCommand
$androidAvdProbeTimedOut = $false
$androidAvds = @()
if ($emulator) {
    $androidAvdProbe = Get-MobilePackagingAndroidAvdNameList -Emulator $emulator
    $androidAvdProbeTimedOut = $androidAvdProbe.TimedOut
    $androidAvds = @($androidAvdProbe.Names)
}

$androidDeviceReady = $false
$androidDeviceProbeTimedOut = $false
$androidDeviceProbeSkipped = $false
if ($adb -and (Test-MobilePackagingDeviceProbeEnabled -RequireAndroidPackaging:$RequireAndroid.IsPresent)) {
    $deviceProbe = Invoke-MobilePackagingProbe -FilePath $adb -ArgumentList @("devices")
    $androidDeviceProbeTimedOut = $deviceProbe.TimedOut
    if (-not $deviceProbe.TimedOut -and $deviceProbe.ExitCode -eq 0) {
        $androidDeviceReady = @($deviceProbe.Lines | Select-Object -Skip 1 | Where-Object {
                [string]$_ -match "^\S+\s+device$"
            }).Count -gt 0
    }
} elseif ($adb) {
    $androidDeviceProbeSkipped = $true
}

$androidReleaseSigningEnvironment = @(
    "MK_ANDROID_KEYSTORE",
    "MK_ANDROID_KEYSTORE_PASSWORD",
    "MK_ANDROID_KEY_ALIAS",
    "MK_ANDROID_KEY_PASSWORD"
)
$androidReleaseSigningReady = Test-EnvironmentSet $androidReleaseSigningEnvironment
$androidKeystore = Get-EnvironmentVariableAnyScope "MK_ANDROID_KEYSTORE"
if ($androidReleaseSigningReady -and -not (Test-Path $androidKeystore)) {
    Add-MobilePackagingBlocker -Blockers $androidBlockers -Kind "missing_android_keystore" -Message "MK_ANDROID_KEYSTORE does not point to an existing keystore file"
    $androidReleaseSigningReady = $false
}

$hostIsAppleOs = Test-IsMacOS
$xcodebuild = Find-CommandOnCombinedPath "xcodebuild"
$xcrun = Find-CommandOnCombinedPath "xcrun"
$appleSimulatorReady = $false

if (-not $hostIsAppleOs) {
    Add-MobilePackagingBlocker -Blockers $appleBlockers -Kind "not_macos" -Message "Apple packaging requires macOS with Xcode command line tools"
}

if (-not $xcodebuild) {
    Add-MobilePackagingBlocker -Blockers $appleBlockers -Kind "missing_xcodebuild" -Message "xcodebuild not found"
}

if (-not $xcrun) {
    Add-MobilePackagingBlocker -Blockers $appleBlockers -Kind "missing_xcrun" -Message "xcrun not found"
}

if ($hostIsAppleOs -and $xcodebuild -and $xcrun) {
    $developerDirectory = Get-AppleDeveloperDirectory
    if (-not (Test-FullXcodeDeveloperDirectory $developerDirectory)) {
        Add-MobilePackagingBlocker -Blockers $appleBlockers -Kind "full_xcode_not_selected" -Message "Full Xcode must be selected as the active developer directory; Command Line Tools alone are not enough for iOS packaging"
    }

    if (-not (Test-XcrunSdkAvailable -Xcrun $xcrun -SdkName "iphonesimulator")) {
        Add-MobilePackagingBlocker -Blockers $appleBlockers -Kind "missing_iphonesimulator_sdk" -Message "iPhone Simulator SDK not available; install iOS platform support through Xcode components"
    }

    if (-not (Test-XcrunSdkAvailable -Xcrun $xcrun -SdkName "iphoneos")) {
        Add-MobilePackagingBlocker -Blockers $appleBlockers -Kind "missing_iphoneos_sdk" -Message "iPhoneOS SDK not available; install iOS platform support through Xcode components"
    }

    $appleSimulatorReady = Test-IosSimulatorRuntimeAvailable -Xcrun $xcrun
}

$appleSigningReady = -not [string]::IsNullOrWhiteSpace($env:MK_IOS_DEVELOPMENT_TEAM)

if ($androidBlockers.Count -eq 0) {
    Write-Information "mobile-packaging: android=ready" -InformationAction Continue
}
else {
    Write-Information "mobile-packaging: android=blocked" -InformationAction Continue
    Write-MobilePackagingBlocker -Prefix "mobile-packaging" -Blockers $androidBlockers
}

if ($appleBlockers.Count -eq 0) {
    Write-Information "mobile-packaging: apple=ready" -InformationAction Continue
}
else {
    Write-Information "mobile-packaging: apple=blocked" -InformationAction Continue
    Write-MobilePackagingBlocker -Prefix "mobile-packaging" -Blockers $appleBlockers
}

if ($androidReleaseSigningReady) {
    Write-Information "mobile-packaging: android-release-signing=ready" -InformationAction Continue
}
else {
    Write-Information "mobile-packaging: android-release-signing=not-configured" -InformationAction Continue
}

if ($emulator) {
    Write-Information "mobile-packaging: android-emulator=ready" -InformationAction Continue
}
else {
    Write-Information "mobile-packaging: android-emulator=not-installed" -InformationAction Continue
}

if ($androidAvdProbeTimedOut) {
    Write-Information "mobile-packaging: android-avd=probe-timeout" -InformationAction Continue
}
elseif ($androidAvds.Count -gt 0) {
    Write-Information "mobile-packaging: android-avd=ready ($($androidAvds -join ', '))" -InformationAction Continue
}
else {
    Write-Information "mobile-packaging: android-avd=not-configured" -InformationAction Continue
}

if ($androidDeviceProbeTimedOut) {
    Write-Information "mobile-packaging: android-device-smoke=probe-timeout" -InformationAction Continue
}
elseif ($androidDeviceProbeSkipped) {
    Write-Information "mobile-packaging: android-device-smoke=probe-skipped-ci" -InformationAction Continue
}
elseif ($androidDeviceReady) {
    Write-Information "mobile-packaging: android-device-smoke=ready" -InformationAction Continue
}
else {
    Write-Information "mobile-packaging: android-device-smoke=not-connected" -InformationAction Continue
}

if ($appleSimulatorReady) {
    Write-Information "mobile-packaging: apple-simulator=ready" -InformationAction Continue
}
else {
    Write-Information "mobile-packaging: apple-simulator=not-installed" -InformationAction Continue
}

if ($appleSigningReady) {
    Write-Information "mobile-packaging: apple-signing=ready" -InformationAction Continue
}
else {
    Write-Information "mobile-packaging: apple-signing=not-configured" -InformationAction Continue
}

if ($RequireAndroid -and $androidBlockers.Count -gt 0) {
    Write-Error "Android mobile packaging is blocked by missing local tools."
}

if ($RequireApple -and $appleBlockers.Count -gt 0) {
    Write-Error "Apple mobile packaging is blocked by missing local tools."
}

Write-Information "mobile-packaging-check: diagnostic-only" -InformationAction Continue
