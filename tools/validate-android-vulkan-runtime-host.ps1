#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$RequireReady,
    [string[]]$ExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$packageName = "dev.mirakanai.android"

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
    param([AllowNull()][string]$Adb)

    if ([string]::IsNullOrWhiteSpace($Adb)) {
        return $false
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

function Test-AndroidGpuDebugLayerSettings {
    param(
        [AllowNull()][string]$Adb,
        [Parameter(Mandatory = $true)][string]$PackageName
    )

    if ([string]::IsNullOrWhiteSpace($Adb)) {
        return [pscustomobject]@{
            SettingsReady = $false
            LayerAppInstalled = $false
        }
    }
    $enabled = Invoke-ToolCapture -FilePath $Adb -Arguments @("shell", "settings", "get", "global", "enable_gpu_debug_layers") -TimeoutSeconds 15
    $debugApp = Invoke-ToolCapture -FilePath $Adb -Arguments @("shell", "settings", "get", "global", "gpu_debug_app") -TimeoutSeconds 15
    $layerApp = Invoke-ToolCapture -FilePath $Adb -Arguments @("shell", "settings", "get", "global", "gpu_debug_layer_app") -TimeoutSeconds 15
    $layers = Invoke-ToolCapture -FilePath $Adb -Arguments @("shell", "settings", "get", "global", "gpu_debug_layers") -TimeoutSeconds 15
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
        $layerAppPath = Invoke-ToolCapture -FilePath $Adb -Arguments @("shell", "pm", "path", $layerAppText) -TimeoutSeconds 15
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
        [bool]$PrerequisitesReady
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
    $smoke = Invoke-ToolCapture -FilePath "pwsh" -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $smokeScript,
        "-Game",
        "sample_headless"
    ) -TimeoutSeconds 600
    $smokeText = [string]::Join("`n", @($smoke.Output, $smoke.Error))
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

$androidSdk = Find-AndroidSdkRoot
$androidNdk = Find-AndroidNdkRoot
$adb = Find-AndroidPlatformToolCommand "adb"

$androidSdkReady = -not [string]::IsNullOrWhiteSpace($androidSdk)
$androidNdkReady = -not [string]::IsNullOrWhiteSpace($androidNdk)
$adbDeviceReady = Test-AndroidDeviceReady -Adb $adb
$androidVulkanProfileReady = Test-AndroidManifestVulkanProfileReady
$androidGpuDebuggableReady = Test-AndroidGpuDebuggableReady
$gpuDebugLayerSettings = Test-AndroidGpuDebugLayerSettings -Adb $adb -PackageName $packageName
$androidGpuDebugLayerSettingsReady = [bool]$gpuDebugLayerSettings.SettingsReady
$androidGpuDebugLayerAppInstalled = [bool]$gpuDebugLayerSettings.LayerAppInstalled

$preSmokeReady = $androidSdkReady -and $androidNdkReady -and $adbDeviceReady -and $androidVulkanProfileReady -and
    $androidGpuDebuggableReady -and $androidGpuDebugLayerSettingsReady -and $androidGpuDebugLayerAppInstalled
$smokeEvidence = Invoke-AndroidPackageSmokeIfRequired -ShouldRun:$RequireReady.IsPresent -PrerequisitesReady:$preSmokeReady

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
