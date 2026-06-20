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
    [int]$BootTimeoutSeconds = 180,
    [ValidateSet("", "arm64-v8a", "x86_64")]
    [string]$AndroidAbi = "",
    [string]$ValidationLayerJniLibs = "",
    [switch]$RequirePackagedValidationLayer
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$packageName = "dev.mirakanai.android"
$activity = "$packageName/.MirakanaiActivity"
$startedEmulator = $false
$startedEmulatorSerial = ""

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

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

function Test-ApkContainsPackagedValidationLayer {
    param(
        [Parameter(Mandatory = $true)][string]$ApkPath,
        [Parameter(Mandatory = $true)][string]$AndroidAbi
    )

    $jar = Find-JavaToolCommand "jar"
    if (-not $jar) {
        return $false
    }

    $entries = @(& $jar "-tf" $ApkPath 2>$null)
    if ($LASTEXITCODE -ne 0) {
        return $false
    }
    return $entries -contains "lib/$AndroidAbi/libVkLayer_khronos_validation.so"
}

function Get-AndroidDevicePrimaryAbi {
    param([Parameter(Mandatory = $true)][string]$Serial)

    $abi = @(& $script:adb "-s" $Serial "shell" "getprop" "ro.product.cpu.abi" 2>$null)
    if ($LASTEXITCODE -ne 0 -or $abi.Count -eq 0) {
        Write-Error "Android smoke failed to read ro.product.cpu.abi from $Serial."
    }
    return ([string]$abi[0]).Trim()
}

function Resolve-AndroidPackageAbi {
    param(
        [Parameter(Mandatory = $true)][string]$Serial,
        [string]$RequestedAbi = ""
    )

    $primaryAbi = Get-AndroidDevicePrimaryAbi -Serial $Serial
    if ($primaryAbi -ne "arm64-v8a" -and $primaryAbi -ne "x86_64") {
        Write-Error "Android smoke supports arm64-v8a and x86_64 primary ABIs; device $Serial reported '$primaryAbi'."
    }
    if (-not [string]::IsNullOrWhiteSpace($RequestedAbi) -and $RequestedAbi -ne $primaryAbi) {
        Write-Error "AndroidAbi '$RequestedAbi' must match device primary ABI '$primaryAbi' for packaged validation layers."
    }
    return $primaryAbi
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
    $resolvedAndroidAbi = Resolve-AndroidPackageAbi -Serial $DeviceSerial -RequestedAbi $AndroidAbi

    if (-not $SkipBuild) {
        $buildArguments = @{
            Game = $Game
            Configuration = $Configuration
            AndroidAbi = $resolvedAndroidAbi
        }
        if (-not [string]::IsNullOrWhiteSpace($ValidationLayerJniLibs)) {
            $buildArguments.ValidationLayerJniLibs = $ValidationLayerJniLibs
        }
        & (Join-Path $PSScriptRoot "build-mobile-android.ps1") @buildArguments
    }

    $apkName = if ($Configuration -eq "Release") { "app-release.apk" } else { "app-debug.apk" }
    $variant = $Configuration.ToLowerInvariant()
    $apk = Join-Path $root "platform\android\app\build\outputs\apk\$variant\$apkName"
    if (-not (Test-Path -LiteralPath $apk -PathType Leaf)) {
        Write-Error "Android APK was not found for smoke: $apk"
    }

    $packagedValidationLayerReady = Test-ApkContainsPackagedValidationLayer -ApkPath $apk -AndroidAbi $resolvedAndroidAbi
    if ($RequirePackagedValidationLayer -and -not $packagedValidationLayerReady) {
        Write-Error "Android APK does not contain lib/$resolvedAndroidAbi/libVkLayer_khronos_validation.so."
    }

    [void](Invoke-AdbAllowFailure @("-s", $DeviceSerial, "uninstall", $packageName))
    if ((Invoke-AdbAllowFailure @("-s", $DeviceSerial, "logcat", "-c")) -ne 0) {
        Write-Error "Android smoke failed to clear logcat output before launch on $DeviceSerial."
    }
    Invoke-CheckedCommand $adb "-s" $DeviceSerial "install" "-r" $apk
    Invoke-CheckedCommand $adb "-s" $DeviceSerial "shell" "am" "start" "-W" "-n" $activity
    Start-Sleep -Seconds 3

    $mirakanaiLog = @(& $adb "-s" $DeviceSerial "logcat" "-d" "-s" "Mirakanai:I" "*:S")
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Android smoke failed to read Mirakanai logcat output from $DeviceSerial."
    }
    $readbackLines = @($mirakanaiLog | Where-Object { ([string]$_).Contains("android_vulkan_readback_ready=") })
    if ($readbackLines.Count -eq 0) {
        Write-Error "Android smoke did not find android_vulkan_readback_ready in Mirakanai logcat output."
    }
    $validationLayerLines = @($mirakanaiLog | Where-Object {
        ([string]$_).Contains("android_vulkan_validation_layer_enumerated=")
    })
    if ($validationLayerLines.Count -eq 0) {
        Write-Error "Android smoke did not find android_vulkan_validation_layer_enumerated in Mirakanai logcat output."
    }
    foreach ($line in $readbackLines) {
        Write-Host "android-smoke-log: $line"
    }
    if (-not (($readbackLines -join "`n").Contains("android_vulkan_readback_ready=1"))) {
        Write-Error "Android Vulkan readback smoke did not report android_vulkan_readback_ready=1."
    }
    if (-not (($validationLayerLines -join "`n").Contains("android_vulkan_validation_layer_enumerated=1"))) {
        Write-Error "Android Vulkan smoke did not enumerate VK_LAYER_KHRONOS_validation."
    }

    $validationIssues = @(& $adb "-s" $DeviceSerial "logcat" "-d" "-v" "brief" "-s" "VALIDATION:W" "*:S" |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Android smoke failed to read Vulkan VALIDATION logcat output from $DeviceSerial."
    }
    if ($validationIssues.Count -gt 0) {
        foreach ($line in $validationIssues) {
            Write-Host "android-validation-log: $line"
        }
        Write-Error "Android Vulkan validation produced warning or error output."
    }

    $processIds = @(& $adb "-s" $DeviceSerial "shell" "pidof" $packageName)
    if ($processIds.Count -eq 0 -or [string]::IsNullOrWhiteSpace(([string]$processIds[0]).Trim())) {
        Write-Error "Android smoke launch did not leave $packageName running on $DeviceSerial."
    }

    Invoke-CheckedCommand $adb "-s" $DeviceSerial "shell" "am" "force-stop" $packageName
    Write-Host "android-smoke: device=$DeviceSerial"
    Write-Host "android-smoke: abi=$resolvedAndroidAbi"
    Write-Host "android-smoke: apk=$apk"
    Write-Host "android_validation_layer_apk_packaged=$(ConvertTo-CounterBit $packagedValidationLayerReady)"
    Write-Host "android_vulkan_readback_ready=1"
    Write-Host "android_vulkan_validation_layer_enumerated=1"
    Write-Host "android_vulkan_validation_log_clean=1"
    Write-Host "VK_LAYER_KHRONOS_validation_ready=1"
    Write-Host "android-smoke: ok"
}
finally {
    if ($startedEmulator -and -not [string]::IsNullOrWhiteSpace($startedEmulatorSerial)) {
        [void](Invoke-AdbAllowFailure @("-s", $startedEmulatorSerial, "emu", "kill"))
    }
}
