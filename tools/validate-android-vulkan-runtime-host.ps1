#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$RequireReady,
    [string[]]$ExpectedEvidenceCounters = @(),
    [string]$DeviceSerial = "",
    [switch]$StartEmulator,
    [string]$AvdName = "GameEngine_API36",
    [int]$EmulatorPort = 5586,
    [int]$BootTimeoutSeconds = 180,
    [switch]$ConfigureGpuDebugLayers,
    [string]$GpuDebugLayerPackage = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$packageName = "dev.mirakanai.android"
$startedEmulator = $false
$startedEmulatorSerial = ""

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Join-IfSet {
    param(
        [AllowNull()][string]$Base,
        [Parameter(Mandatory = $true)][string]$Child
    )

    if ([string]::IsNullOrWhiteSpace($Base)) {
        return $null
    }
    return Join-Path $Base $Child
}

function Get-FirstExistingDirectory {
    param([string[]]$Candidates)

    foreach ($candidate in @($Candidates)) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    return $null
}

function Find-AndroidNdkRoot {
    $sdk = Find-AndroidSdkRoot
    $candidates = @(
        (Get-EnvironmentVariableAnyScope "ANDROID_NDK_ROOT"),
        (Get-EnvironmentVariableAnyScope "ANDROID_NDK_HOME"),
        (Get-EnvironmentVariableAnyScope "NDK_ROOT"),
        (Join-IfSet $sdk "ndk-bundle")
    )

    if ($sdk) {
        $ndkRoot = Join-Path $sdk "ndk"
        if (Test-Path -LiteralPath $ndkRoot -PathType Container) {
            $latest = Get-ChildItem -LiteralPath $ndkRoot -Directory |
                Sort-Object Name -Descending |
                Select-Object -First 1
            if ($latest) {
                $candidates += $latest.FullName
            }
        }
    }

    return Get-FirstExistingDirectory $candidates
}

function Invoke-ToolCapture {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @(),
        [int]$TimeoutSeconds = 20
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = $root
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in @($Arguments)) {
        $startInfo.ArgumentList.Add($argument)
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        $null = $process.Start()
        $completed = $process.WaitForExit($TimeoutSeconds * 1000)
        if (-not $completed) {
            try {
                $process.Kill($true)
            } catch {
                Write-Verbose "failed to kill timed-out process '$FilePath': $_"
            }
            return [pscustomobject]@{
                ExitCode = -1
                Output = ""
                Error = "timeout"
            }
        }
        return [pscustomobject]@{
            ExitCode = $process.ExitCode
            Output = $process.StandardOutput.ReadToEnd()
            Error = $process.StandardError.ReadToEnd()
        }
    } finally {
        $process.Dispose()
    }
}

function Test-AndroidDeviceReady {
    param(
        [AllowNull()][string]$Adb,
        [string]$Serial = ""
    )

    if ([string]::IsNullOrWhiteSpace($Adb)) {
        return $false
    }
    if (-not [string]::IsNullOrWhiteSpace($Serial)) {
        $state = Invoke-ToolCapture -FilePath $Adb -Arguments @("-s", $Serial, "get-state") -TimeoutSeconds 15
        return $state.ExitCode -eq 0 -and ([string]$state.Output).Trim() -eq "device"
    }
    $devices = Invoke-ToolCapture -FilePath $Adb -Arguments @("devices") -TimeoutSeconds 15
    if ($devices.ExitCode -ne 0) {
        return $false
    }
    foreach ($line in ($devices.Output -split "`r?`n")) {
        if ($line -match '^\S+\s+device$') {
            return $true
        }
    }
    return $false
}

function Get-OnlineAndroidDevices {
    param([AllowNull()][string]$Adb)

    if ([string]::IsNullOrWhiteSpace($Adb)) {
        return @()
    }
    $devices = Invoke-ToolCapture -FilePath $Adb -Arguments @("devices") -TimeoutSeconds 15
    if ($devices.ExitCode -ne 0) {
        return @()
    }
    return @($devices.Output -split "`r?`n" | ForEach-Object {
        $line = [string]$_
        $deviceMatch = [regex]::Match($line, '^(\S+)\s+device$')
        if ($deviceMatch.Success) {
            $deviceMatch.Groups[1].Value
        }
    } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
}

function Wait-AndroidBoot {
    param(
        [Parameter(Mandatory = $true)][string]$Adb,
        [Parameter(Mandatory = $true)][string]$Serial,
        [int]$TimeoutSeconds = 180
    )

    $wait = Invoke-ToolCapture -FilePath $Adb -Arguments @("-s", $Serial, "wait-for-device") -TimeoutSeconds $TimeoutSeconds
    if ($wait.ExitCode -ne 0) {
        return $false
    }
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $boot = Invoke-ToolCapture -FilePath $Adb -Arguments @("-s", $Serial, "shell", "getprop", "sys.boot_completed") -TimeoutSeconds 15
        if ($boot.ExitCode -eq 0 -and ([string]$boot.Output).Trim() -eq "1") {
            return $true
        }
        Start-Sleep -Seconds 2
    }
    return $false
}

function Start-AndroidEmulatorForValidation {
    param(
        [Parameter(Mandatory = $true)][string]$Adb,
        [Parameter(Mandatory = $true)][string]$Name,
        [int]$Port = 5586,
        [int]$TimeoutSeconds = 180
    )

    $emulator = Find-AndroidEmulatorCommand
    if ([string]::IsNullOrWhiteSpace($emulator)) {
        return ""
    }
    Set-AndroidAvdHomeEnvironment | Out-Null
    $available = Invoke-ToolCapture -FilePath $emulator -Arguments @("-list-avds") -TimeoutSeconds 20
    if ($available.ExitCode -ne 0 -or -not (@($available.Output -split "`r?`n") -contains $Name)) {
        return ""
    }

    $serial = "emulator-$Port"
    $process = Start-Process -FilePath $emulator -ArgumentList @(
        "-avd", $Name,
        "-port", "$Port",
        "-no-window",
        "-no-snapshot",
        "-no-boot-anim",
        "-gpu", "swiftshader_indirect"
    ) -WindowStyle Hidden -PassThru
    if ($process.HasExited) {
        return ""
    }

    $script:startedEmulator = $true
    $script:startedEmulatorSerial = $serial
    if (-not (Wait-AndroidBoot -Adb $Adb -Serial $serial -TimeoutSeconds $TimeoutSeconds)) {
        return ""
    }
    return $serial
}

function Resolve-AndroidDeviceSerial {
    param(
        [AllowNull()][string]$Adb,
        [string]$RequestedSerial = "",
        [bool]$AllowStartEmulator = $false
    )

    if ([string]::IsNullOrWhiteSpace($Adb)) {
        return ""
    }
    if (-not [string]::IsNullOrWhiteSpace($RequestedSerial)) {
        if (Test-AndroidDeviceReady -Adb $Adb -Serial $RequestedSerial) {
            return $RequestedSerial
        }
        return ""
    }

    $onlineDevices = @(Get-OnlineAndroidDevices -Adb $Adb)
    if ($onlineDevices.Count -eq 1) {
        return $onlineDevices[0]
    }
    if ($onlineDevices.Count -eq 0 -and $AllowStartEmulator) {
        return Start-AndroidEmulatorForValidation -Adb $Adb -Name $script:AvdName -Port $script:EmulatorPort -TimeoutSeconds $script:BootTimeoutSeconds
    }
    return ""
}

function Get-AdbDeviceArguments {
    param(
        [string]$Serial = "",
        [string[]]$Arguments = @()
    )

    if ([string]::IsNullOrWhiteSpace($Serial)) {
        return $Arguments
    }
    return @("-s", $Serial) + $Arguments
}

function Get-AndroidGpuDebugLayerPackage {
    param(
        [AllowNull()][string]$Adb,
        [string]$Serial = "",
        [string]$RequestedPackage = ""
    )

    if (-not [string]::IsNullOrWhiteSpace($RequestedPackage)) {
        return $RequestedPackage
    }
    if ([string]::IsNullOrWhiteSpace($Adb) -or [string]::IsNullOrWhiteSpace($Serial)) {
        return ""
    }
    $abi = Invoke-ToolCapture -FilePath $Adb -Arguments (Get-AdbDeviceArguments -Serial $Serial -Arguments @("shell", "getprop", "ro.product.cpu.abilist")) -TimeoutSeconds 15
    if ($abi.ExitCode -ne 0) {
        return ""
    }
    $abiText = [string]$abi.Output
    if ($abiText.Contains("arm64-v8a")) {
        return "com.google.android.gapid.arm64v8a"
    }
    if ($abiText.Contains("armeabi-v7a")) {
        return "com.google.android.gapid.armeabi-v7a"
    }
    if ($abiText.Contains("x86")) {
        return "com.google.android.gapid.x86"
    }
    return ""
}

function Set-AndroidGpuDebugLayerSettings {
    param(
        [AllowNull()][string]$Adb,
        [string]$Serial = "",
        [Parameter(Mandatory = $true)][string]$PackageName,
        [Parameter(Mandatory = $true)][string]$LayerPackageName
    )

    if ([string]::IsNullOrWhiteSpace($Adb) -or [string]::IsNullOrWhiteSpace($Serial) -or
        [string]::IsNullOrWhiteSpace($LayerPackageName)) {
        return $false
    }
    $layerAppPath = Invoke-ToolCapture -FilePath $Adb -Arguments (Get-AdbDeviceArguments -Serial $Serial -Arguments @("shell", "pm", "path", $LayerPackageName)) -TimeoutSeconds 15
    if ($layerAppPath.ExitCode -ne 0 -or -not ([string]$layerAppPath.Output).Trim().StartsWith("package:")) {
        return $false
    }

    $commands = @(
        @("shell", "settings", "put", "global", "enable_gpu_debug_layers", "1"),
        @("shell", "settings", "put", "global", "gpu_debug_app", $PackageName),
        @("shell", "settings", "put", "global", "gpu_debug_layer_app", $LayerPackageName),
        @("shell", "settings", "put", "global", "gpu_debug_layers", "VK_LAYER_KHRONOS_validation")
    )
    foreach ($command in $commands) {
        $result = Invoke-ToolCapture -FilePath $Adb -Arguments (Get-AdbDeviceArguments -Serial $Serial -Arguments $command) -TimeoutSeconds 15
        if ($result.ExitCode -ne 0) {
            return $false
        }
    }
    return $true
}

function Test-AndroidGpuDebugLayerSettings {
    param(
        [AllowNull()][string]$Adb,
        [Parameter(Mandatory = $true)][string]$PackageName,
        [string]$Serial = ""
    )

    if ([string]::IsNullOrWhiteSpace($Adb)) {
        return [pscustomobject]@{
            SettingsReady = $false
            LayerAppInstalled = $false
        }
    }
    $enabled = Invoke-ToolCapture -FilePath $Adb -Arguments (Get-AdbDeviceArguments -Serial $Serial -Arguments @("shell", "settings", "get", "global", "enable_gpu_debug_layers")) -TimeoutSeconds 15
    $debugApp = Invoke-ToolCapture -FilePath $Adb -Arguments (Get-AdbDeviceArguments -Serial $Serial -Arguments @("shell", "settings", "get", "global", "gpu_debug_app")) -TimeoutSeconds 15
    $layerApp = Invoke-ToolCapture -FilePath $Adb -Arguments (Get-AdbDeviceArguments -Serial $Serial -Arguments @("shell", "settings", "get", "global", "gpu_debug_layer_app")) -TimeoutSeconds 15
    $layers = Invoke-ToolCapture -FilePath $Adb -Arguments (Get-AdbDeviceArguments -Serial $Serial -Arguments @("shell", "settings", "get", "global", "gpu_debug_layers")) -TimeoutSeconds 15
    if ($enabled.ExitCode -ne 0 -or $debugApp.ExitCode -ne 0 -or $layerApp.ExitCode -ne 0 -or $layers.ExitCode -ne 0) {
        return [pscustomobject]@{
            SettingsReady = $false
            LayerAppInstalled = $false
        }
    }
    $enabledReady = ([string]$enabled.Output).Trim() -eq "1"
    $debugAppReady = ([string]$debugApp.Output).Trim() -eq $PackageName
    $layerAppText = ([string]$layerApp.Output).Trim()
    $layerAppReady = $layerAppText.StartsWith("com.google.android.gapid.")
    $validationLayerReady = ([string]$layers.Output).Contains("VK_LAYER_KHRONOS_validation")
    $layerAppInstalled = $false
    if ($layerAppReady) {
        $layerAppPath = Invoke-ToolCapture -FilePath $Adb -Arguments (Get-AdbDeviceArguments -Serial $Serial -Arguments @("shell", "pm", "path", $layerAppText)) -TimeoutSeconds 15
        $layerAppInstalled = $layerAppPath.ExitCode -eq 0 -and ([string]$layerAppPath.Output).Trim().StartsWith("package:")
    }

    return [pscustomobject]@{
        SettingsReady = $enabledReady -and $debugAppReady -and $layerAppReady -and $validationLayerReady
        LayerAppInstalled = $layerAppInstalled
    }
}

function Test-AndroidManifestVulkanProfileReady {
    $manifestPath = Join-Path $root "platform/android/app/src/main/AndroidManifest.xml"
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        return $false
    }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw
    return $manifest.Contains('android.hardware.vulkan.version') -and
        $manifest.Contains('android.hardware.vulkan.level') -and
        $manifest.Contains('android:required="true"')
}

function Test-AndroidGpuDebuggableReady {
    $gradlePath = Join-Path $root "platform/android/app/build.gradle.kts"
    if (-not (Test-Path -LiteralPath $gradlePath -PathType Leaf)) {
        return $false
    }
    $gradle = Get-Content -LiteralPath $gradlePath -Raw
    return $gradle.Contains('getByName("debug")') -and $gradle.Contains("isDebuggable = true")
}

function Invoke-AndroidPackageSmokeIfRequired {
    param(
        [bool]$ShouldRun,
        [bool]$PrerequisitesReady,
        [string]$Serial = ""
    )

    if (-not $ShouldRun -or -not $PrerequisitesReady) {
        return [pscustomobject]@{
            PackageSmokeReady = $false
            VulkanReadbackReady = $false
            ValidationLayerEnumerated = $false
            ValidationLogClean = $false
        }
    }

    $smokeScript = Join-Path $PSScriptRoot "smoke-android-package.ps1"
    $smokeArguments = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $smokeScript,
        "-Game",
        "sample_headless"
    )
    if (-not [string]::IsNullOrWhiteSpace($Serial)) {
        $smokeArguments += @("-DeviceSerial", $Serial)
    }
    $smoke = Invoke-ToolCapture -FilePath "pwsh" -Arguments $smokeArguments -TimeoutSeconds 600
    $smokeText = [string]::Join("`n", @($smoke.Output, $smoke.Error))
    if (-not [string]::IsNullOrWhiteSpace($smokeText)) {
        Write-Output $smokeText.TrimEnd()
    }
    return [pscustomobject]@{
        PackageSmokeReady = $smoke.ExitCode -eq 0 -and $smokeText.Contains("android-smoke: ok")
        VulkanReadbackReady = $smoke.ExitCode -eq 0 -and $smokeText.Contains("android_vulkan_readback_ready=1")
        ValidationLayerEnumerated = $smoke.ExitCode -eq 0 -and $smokeText.Contains("android_vulkan_validation_layer_enumerated=1")
        ValidationLogClean = $smoke.ExitCode -eq 0 -and $smokeText.Contains("android_vulkan_validation_log_clean=1")
    }
}

Set-ProcessEnvironmentFromAnyScope "ANDROID_HOME"
Set-ProcessEnvironmentFromAnyScope "ANDROID_SDK_ROOT"
Set-ProcessEnvironmentFromAnyScope "JAVA_HOME"
Set-AndroidAvdHomeEnvironment | Out-Null

$androidSdk = Find-AndroidSdkRoot
$androidNdk = Find-AndroidNdkRoot
$adb = Find-AndroidPlatformToolCommand "adb"

try {
    $script:AvdName = $AvdName
    $script:EmulatorPort = $EmulatorPort
    $script:BootTimeoutSeconds = $BootTimeoutSeconds
    $resolvedDeviceSerial = Resolve-AndroidDeviceSerial -Adb $adb -RequestedSerial $DeviceSerial -AllowStartEmulator:$StartEmulator.IsPresent

    if ($ConfigureGpuDebugLayers -and -not [string]::IsNullOrWhiteSpace($resolvedDeviceSerial)) {
        $layerPackage = Get-AndroidGpuDebugLayerPackage -Adb $adb -Serial $resolvedDeviceSerial -RequestedPackage $GpuDebugLayerPackage
        if (-not [string]::IsNullOrWhiteSpace($layerPackage)) {
            [void](Set-AndroidGpuDebugLayerSettings -Adb $adb -Serial $resolvedDeviceSerial -PackageName $packageName -LayerPackageName $layerPackage)
        }
    }

    $androidSdkReady = -not [string]::IsNullOrWhiteSpace($androidSdk)
    $androidNdkReady = -not [string]::IsNullOrWhiteSpace($androidNdk)
    $adbDeviceReady = Test-AndroidDeviceReady -Adb $adb -Serial $resolvedDeviceSerial
    $androidVulkanProfileReady = Test-AndroidManifestVulkanProfileReady
    $androidGpuDebuggableReady = Test-AndroidGpuDebuggableReady
    $gpuDebugLayerSettings = Test-AndroidGpuDebugLayerSettings -Adb $adb -PackageName $packageName -Serial $resolvedDeviceSerial
    $androidGpuDebugLayerSettingsReady = [bool]$gpuDebugLayerSettings.SettingsReady
    $androidGpuDebugLayerAppInstalled = [bool]$gpuDebugLayerSettings.LayerAppInstalled

    $preSmokeReady = $androidSdkReady -and $androidNdkReady -and $adbDeviceReady -and $androidVulkanProfileReady -and
        $androidGpuDebuggableReady -and $androidGpuDebugLayerSettingsReady -and $androidGpuDebugLayerAppInstalled
    $smokeEvidence = Invoke-AndroidPackageSmokeIfRequired -ShouldRun:$RequireReady.IsPresent -PrerequisitesReady:$preSmokeReady -Serial $resolvedDeviceSerial
} finally {
    if ($startedEmulator -and -not [string]::IsNullOrWhiteSpace($startedEmulatorSerial) -and -not [string]::IsNullOrWhiteSpace($adb)) {
        [void](Invoke-ToolCapture -FilePath $adb -Arguments @("-s", $startedEmulatorSerial, "emu", "kill") -TimeoutSeconds 15)
    }
}

$validationLayerReady = $preSmokeReady -and $smokeEvidence.ValidationLayerEnumerated -and $smokeEvidence.ValidationLogClean
$androidVulkanReady = $preSmokeReady -and $smokeEvidence.PackageSmokeReady -and $smokeEvidence.VulkanReadbackReady -and
    $validationLayerReady

foreach ($counter in @($ExpectedEvidenceCounters)) {
    if (-not [string]::IsNullOrWhiteSpace($counter)) {
        Write-Output "environment-platform-android-vulkan-required-counter: $counter"
    }
}

$actualCounters = @(
    "validation_recipe=environment-platform-android-vulkan-package",
    "host_gate=android-vulkan-runtime-host",
    "host_has_android_sdk=$(ConvertTo-CounterBit $androidSdkReady)",
    "host_has_android_ndk=$(ConvertTo-CounterBit $androidNdkReady)",
    "adb_device_or_emulator_ready=$(ConvertTo-CounterBit $adbDeviceReady)",
    "android_vulkan_profile_ready=$(ConvertTo-CounterBit $androidVulkanProfileReady)",
    "android_gpu_debuggable_ready=$(ConvertTo-CounterBit $androidGpuDebuggableReady)",
    "android_gpu_debug_layer_settings_ready=$(ConvertTo-CounterBit $androidGpuDebugLayerSettingsReady)",
    "android_gpu_debug_layer_app_installed=$(ConvertTo-CounterBit $androidGpuDebugLayerAppInstalled)",
    "VK_LAYER_KHRONOS_validation_ready=$(ConvertTo-CounterBit $validationLayerReady)",
    "android_package_smoke_ready=$(ConvertTo-CounterBit $smokeEvidence.PackageSmokeReady)",
    "android_vulkan_readback_ready=$(ConvertTo-CounterBit $smokeEvidence.VulkanReadbackReady)",
    "android_vulkan_validation_layer_enumerated=$(ConvertTo-CounterBit $smokeEvidence.ValidationLayerEnumerated)",
    "android_vulkan_validation_log_clean=$(ConvertTo-CounterBit $smokeEvidence.ValidationLogClean)",
    "environment_platform_android_vulkan_ready=$(ConvertTo-CounterBit $androidVulkanReady)",
    "environment_platform_requires_android_vulkan_host_evidence=$(if ($androidVulkanReady) { '0' } else { '1' })",
    "environment_all_platform_unconditional_ready=0",
    "desktop_vulkan_inferred=0",
    "linux_vulkan_inferred=0",
    "native_handle_access=0"
)
$actualCounterLine = [string]::Join(" ", $actualCounters)
Write-Output $actualCounterLine

$missingExpectedCounters = @($ExpectedEvidenceCounters | Where-Object {
    -not [string]::IsNullOrWhiteSpace($_) -and -not $actualCounterLine.Contains([string]$_)
})
if ($missingExpectedCounters.Count -gt 0) {
    Write-Error "environment-platform-android-vulkan-package is missing expected actual counters: $($missingExpectedCounters -join ', ')"
}

if ($RequireReady -and -not $androidVulkanReady) {
    Write-Error "environment-platform-android-vulkan-package requires Android SDK, NDK, adb device/emulator, Vulkan manifest feature declarations, Android debug build instrumentation, AGI GPU debug layer settings, installed AGI layer APK, VK_LAYER_KHRONOS_validation enumeration, clean validation logcat output, package smoke, and Android Vulkan readback evidence."
}
