#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$Game = "sample_headless",
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [string]$DeviceSerial = "",
    [switch]$SkipBuild,
    [switch]$StartEmulator,
[string]$AvdName = "Mirakanai_API36",
    [int]$EmulatorPort = 5584,
    [int]$BootTimeoutSeconds = 180
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$packageName = "dev.mirakanai.android"
$activity = "$packageName/.MirakanaiActivity"
$startedEmulator = $false
$startedEmulatorSerial = ""

function Invoke-AdbAllowFailure {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    & $script:adb @Arguments | Out-Null
    return $LASTEXITCODE
}

function Get-OnlineAndroidDevices {
    $lines = & $script:adb "devices"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "adb devices failed."
    }

    return @($lines | Select-Object -Skip 1 | ForEach-Object {
        $line = [string]$_
        if ($line -match "^(\S+)\s+device$") {
            $Matches[1]
        }
    } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
}

function Wait-AndroidBoot {
    param([Parameter(Mandatory = $true)][string]$Serial)

    Invoke-CheckedCommand $script:adb "-s" $Serial "wait-for-device"
    $deadline = (Get-Date).AddSeconds($BootTimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $boot = @(& $script:adb "-s" $Serial "shell" "getprop" "sys.boot_completed" 2>$null)
        if ($boot.Count -gt 0 -and ([string]$boot[0]).Trim() -eq "1") {
            return
        }
        Start-Sleep -Seconds 2
    }

    Write-Error "Android device did not finish booting within $BootTimeoutSeconds seconds: $Serial"
}

function Start-AndroidEmulatorForSmoke {
    $emulator = Find-AndroidEmulatorCommand
    if (-not $emulator) {
        Write-Error "Android Emulator is required for -StartEmulator. Install it with sdkmanager --install emulator."
    }

    $available = & $emulator "-list-avds"
    if ($LASTEXITCODE -ne 0 -or -not ($available -contains $AvdName)) {
        Write-Error "Android AVD '$AvdName' was not found. Create it with avdmanager before running emulator smoke."
    }

    $serial = "emulator-$EmulatorPort"
    $process = Start-Process -FilePath $emulator -ArgumentList @(
        "-avd", $AvdName,
        "-port", "$EmulatorPort",
        "-no-window",
        "-no-snapshot",
        "-no-boot-anim",
        "-gpu", "swiftshader_indirect"
    ) -WindowStyle Hidden -PassThru
    if ($process.HasExited) {
        Write-Error "Android Emulator exited during startup for AVD '$AvdName'."
    }

    $script:startedEmulator = $true
    $script:startedEmulatorSerial = $serial
    Wait-AndroidBoot $serial
    return $serial
}

Set-ProcessEnvironmentFromAnyScope "ANDROID_HOME"
Set-ProcessEnvironmentFromAnyScope "ANDROID_SDK_ROOT"
Set-ProcessEnvironmentFromAnyScope "JAVA_HOME"
Set-AndroidAvdHomeEnvironment | Out-Null

$adb = Find-AndroidPlatformToolCommand "adb"
if (-not $adb) {
    Write-Error "adb is required for Android device smoke. Install Android SDK Platform Tools."
}
$script:adb = $adb

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "build-mobile-android.ps1") -Game $Game -Configuration $Configuration
}

$apkName = if ($Configuration -eq "Release") { "app-release.apk" } else { "app-debug.apk" }
$variant = $Configuration.ToLowerInvariant()
$apk = Join-Path $root "platform\android\app\build\outputs\apk\$variant\$apkName"
if (-not (Test-Path -LiteralPath $apk -PathType Leaf)) {
    Write-Error "Android APK was not found for smoke: $apk"
}

try {
    Invoke-CheckedCommand $adb "start-server"

    if ([string]::IsNullOrWhiteSpace($DeviceSerial)) {
        $devices = Get-OnlineAndroidDevices
        if ($devices.Count -eq 0 -and $StartEmulator) {
            $DeviceSerial = Start-AndroidEmulatorForSmoke
        }
        elseif ($devices.Count -eq 1) {
            $DeviceSerial = $devices[0]
        }
        elseif ($devices.Count -gt 1) {
            Write-Error "Multiple Android devices are online. Pass -DeviceSerial to choose one: $($devices -join ', ')"
        }
        else {
            Write-Error "No online Android device or emulator was found. Connect a device or pass -StartEmulator."
        }
    }

    Wait-AndroidBoot $DeviceSerial

    [void](Invoke-AdbAllowFailure @("-s", $DeviceSerial, "uninstall", $packageName))
    Invoke-CheckedCommand $adb "-s" $DeviceSerial "install" "-r" $apk
    Invoke-CheckedCommand $adb "-s" $DeviceSerial "shell" "am" "start" "-W" "-n" $activity
    Start-Sleep -Seconds 3

    $processIds = @(& $adb "-s" $DeviceSerial "shell" "pidof" $packageName)
    if ($processIds.Count -eq 0 -or [string]::IsNullOrWhiteSpace(([string]$processIds[0]).Trim())) {
        Write-Error "Android smoke launch did not leave $packageName running on $DeviceSerial."
    }

    Invoke-CheckedCommand $adb "-s" $DeviceSerial "shell" "am" "force-stop" $packageName
    Write-Host "android-smoke: device=$DeviceSerial"
    Write-Host "android-smoke: apk=$apk"
    Write-Host "android-smoke: ok"
}
finally {
    if ($startedEmulator -and -not [string]::IsNullOrWhiteSpace($startedEmulatorSerial)) {
        [void](Invoke-AdbAllowFailure @("-s", $startedEmulatorSerial, "emu", "kill"))
    }
}
