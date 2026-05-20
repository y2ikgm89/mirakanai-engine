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
        if (Test-Path $ndkRoot) {
            $latest = Get-ChildItem -Path $ndkRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
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
    if (-not [string]::IsNullOrWhiteSpace($toolchainsRoot) -and (Test-Path $toolchainsRoot)) {
        $candidate = Get-ChildItem -Path $toolchainsRoot -Directory -Filter "gradle-*" |
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
    if (-not [string]::IsNullOrWhiteSpace($adoptiumRoot) -and (Test-Path $adoptiumRoot)) {
        $candidate = Get-ChildItem -Path $adoptiumRoot -Directory -Filter "jdk-*" |
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
        else {
            $childProcess.WaitForExit()
        }

        $standardOutputCompleted = $standardOutputTask.Wait($OutputWaitMilliseconds)
        $standardErrorCompleted = $standardErrorTask.Wait($OutputWaitMilliseconds)
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

function Get-MobilePackagingAndroidAvdNames {
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

$androidBlockers = [System.Collections.Generic.List[string]]::new()
$appleBlockers = [System.Collections.Generic.List[string]]::new()

$androidSdk = Find-AndroidSdk
if (-not $androidSdk) {
    $androidBlockers.Add("Android SDK not found; set ANDROID_HOME or ANDROID_SDK_ROOT") | Out-Null
}

$androidNdk = Find-AndroidNdk $androidSdk
if (-not $androidNdk) {
    $androidBlockers.Add("Android NDK not found; install it through the Android SDK manager or set ANDROID_NDK_HOME") |
        Out-Null
}

if (-not (Find-GradleCommand)) {
    $androidBlockers.Add("Gradle not found on PATH; install official Gradle or add a reviewed Gradle wrapper later") |
        Out-Null
}

if (-not (Find-JavaCommand)) {
    $androidBlockers.Add("JDK not found on PATH; Android Gradle Plugin requires a compatible JDK") | Out-Null
}

$adb = Find-AndroidPlatformToolCommand "adb"
if (-not $adb) {
    $androidBlockers.Add("adb not found; install Android SDK Platform Tools for device/emulator smoke") | Out-Null
}

$apksigner = Find-AndroidBuildToolCommand "apksigner"
if (-not $apksigner) {
    $androidBlockers.Add("apksigner not found; install Android SDK Build Tools for Release signing verification") |
        Out-Null
}

$emulator = Find-AndroidEmulatorCommand
$androidAvdProbeTimedOut = $false
$androidAvds = @()
if ($emulator) {
    $androidAvdProbe = Get-MobilePackagingAndroidAvdNames -Emulator $emulator
    $androidAvdProbeTimedOut = $androidAvdProbe.TimedOut
    $androidAvds = @($androidAvdProbe.Names)
}

$androidDeviceReady = $false
$androidDeviceProbeTimedOut = $false
if ($adb) {
    $deviceProbe = Invoke-MobilePackagingProbe -FilePath $adb -ArgumentList @("devices")
    $androidDeviceProbeTimedOut = $deviceProbe.TimedOut
    if (-not $deviceProbe.TimedOut -and $deviceProbe.ExitCode -eq 0) {
        $androidDeviceReady = @($deviceProbe.Lines | Select-Object -Skip 1 | Where-Object {
                [string]$_ -match "^\S+\s+device$"
            }).Count -gt 0
    }
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
    $androidBlockers.Add("MK_ANDROID_KEYSTORE does not point to an existing keystore file") | Out-Null
    $androidReleaseSigningReady = $false
}

$hostIsAppleOs = Test-IsMacOS
$xcodebuild = Find-CommandOnCombinedPath "xcodebuild"
$xcrun = Find-CommandOnCombinedPath "xcrun"
$appleSimulatorReady = $false

if (-not $hostIsAppleOs) {
    $appleBlockers.Add("Apple packaging requires macOS with Xcode command line tools") | Out-Null
}

if (-not $xcodebuild) {
    $appleBlockers.Add("xcodebuild not found") | Out-Null
}

if (-not $xcrun) {
    $appleBlockers.Add("xcrun not found") | Out-Null
}

if ($hostIsAppleOs -and $xcodebuild -and $xcrun) {
    $developerDirectory = Get-AppleDeveloperDirectory
    if (-not (Test-FullXcodeDeveloperDirectory $developerDirectory)) {
        $appleBlockers.Add("Full Xcode must be selected as the active developer directory; Command Line Tools alone are not enough for iOS packaging") |
            Out-Null
    }

    if (-not (Test-XcrunSdkAvailable -Xcrun $xcrun -SdkName "iphonesimulator")) {
        $appleBlockers.Add("iPhone Simulator SDK not available; install iOS platform support through Xcode components") |
            Out-Null
    }

    if (-not (Test-XcrunSdkAvailable -Xcrun $xcrun -SdkName "iphoneos")) {
        $appleBlockers.Add("iPhoneOS SDK not available; install iOS platform support through Xcode components") |
            Out-Null
    }

    $appleSimulatorReady = Test-IosSimulatorRuntimeAvailable -Xcrun $xcrun
}

$appleSigningReady = -not [string]::IsNullOrWhiteSpace($env:MK_IOS_DEVELOPMENT_TEAM)

if ($androidBlockers.Count -eq 0) {
    Write-Host "mobile-packaging: android=ready"
}
else {
    Write-Host "mobile-packaging: android=blocked"
    foreach ($blocker in $androidBlockers) {
        Write-Host "mobile-packaging: blocker - $blocker"
    }
}

if ($appleBlockers.Count -eq 0) {
    Write-Host "mobile-packaging: apple=ready"
}
else {
    Write-Host "mobile-packaging: apple=blocked"
    foreach ($blocker in $appleBlockers) {
        Write-Host "mobile-packaging: blocker - $blocker"
    }
}

if ($androidReleaseSigningReady) {
    Write-Host "mobile-packaging: android-release-signing=ready"
}
else {
    Write-Host "mobile-packaging: android-release-signing=not-configured"
}

if ($emulator) {
    Write-Host "mobile-packaging: android-emulator=ready"
}
else {
    Write-Host "mobile-packaging: android-emulator=not-installed"
}

if ($androidAvdProbeTimedOut) {
    Write-Host "mobile-packaging: android-avd=probe-timeout"
}
elseif ($androidAvds.Count -gt 0) {
    Write-Host "mobile-packaging: android-avd=ready ($($androidAvds -join ', '))"
}
else {
    Write-Host "mobile-packaging: android-avd=not-configured"
}

if ($androidDeviceProbeTimedOut) {
    Write-Host "mobile-packaging: android-device-smoke=probe-timeout"
}
elseif ($androidDeviceReady) {
    Write-Host "mobile-packaging: android-device-smoke=ready"
}
else {
    Write-Host "mobile-packaging: android-device-smoke=not-connected"
}

if ($appleSimulatorReady) {
    Write-Host "mobile-packaging: apple-simulator=ready"
}
else {
    Write-Host "mobile-packaging: apple-simulator=not-installed"
}

if ($appleSigningReady) {
    Write-Host "mobile-packaging: apple-signing=ready"
}
else {
    Write-Host "mobile-packaging: apple-signing=not-configured"
}

if ($RequireAndroid -and $androidBlockers.Count -gt 0) {
    Write-Error "Android mobile packaging is blocked by missing local tools."
}

if ($RequireApple -and $appleBlockers.Count -gt 0) {
    Write-Error "Apple mobile packaging is blocked by missing local tools."
}

Write-Host "mobile-packaging-check: diagnostic-only"
