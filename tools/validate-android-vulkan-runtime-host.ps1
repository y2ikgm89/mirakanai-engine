#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$RequireReady,
    [string[]]$ExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

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

function Test-AndroidDebugValidationLayerReady {
    param([AllowNull()][string]$Adb)

    if ([string]::IsNullOrWhiteSpace($Adb)) {
        return $false
    }
    $enabled = Invoke-ToolCapture -FilePath $Adb -Arguments @("shell", "settings", "get", "global", "enable_gpu_debug_layers") -TimeoutSeconds 15
    $layers = Invoke-ToolCapture -FilePath $Adb -Arguments @("shell", "settings", "get", "global", "gpu_debug_layers") -TimeoutSeconds 15
    if ($enabled.ExitCode -ne 0 -or $layers.ExitCode -ne 0) {
        return $false
    }
    return ([string]$enabled.Output).Trim() -eq "1" -and ([string]$layers.Output).Contains("VK_LAYER_KHRONOS_validation")
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

function Test-AndroidValidationLayerPackaged {
    $jniLibRoot = Join-Path $root "platform/android/app/src/main/jniLibs"
    if (-not (Test-Path -LiteralPath $jniLibRoot -PathType Container)) {
        return $false
    }
    $layer = Get-ChildItem -LiteralPath $jniLibRoot -Recurse -File -Filter "libVkLayer_khronos_validation.so" |
        Select-Object -First 1
    return $null -ne $layer
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
$androidValidationLayerPackaged = Test-AndroidValidationLayerPackaged
$debugValidationLayerReady = Test-AndroidDebugValidationLayerReady -Adb $adb
$validationLayerReady = $androidValidationLayerPackaged -or $debugValidationLayerReady

$preSmokeReady = $androidSdkReady -and $androidNdkReady -and $adbDeviceReady -and $androidVulkanProfileReady -and
    $androidValidationLayerPackaged -and $validationLayerReady
$smokeEvidence = Invoke-AndroidPackageSmokeIfRequired -ShouldRun:$RequireReady.IsPresent -PrerequisitesReady:$preSmokeReady

$androidVulkanReady = $preSmokeReady -and $smokeEvidence.PackageSmokeReady -and $smokeEvidence.VulkanReadbackReady

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
    "android_validation_layer_packaged=$(ConvertTo-CounterBit $androidValidationLayerPackaged)",
    "VK_LAYER_KHRONOS_validation_ready=$(ConvertTo-CounterBit $validationLayerReady)",
    "android_package_smoke_ready=$(ConvertTo-CounterBit $smokeEvidence.PackageSmokeReady)",
    "android_vulkan_readback_ready=$(ConvertTo-CounterBit $smokeEvidence.VulkanReadbackReady)",
    "environment_platform_android_vulkan_ready=$(ConvertTo-CounterBit $androidVulkanReady)",
    "environment_platform_requires_android_vulkan_host_evidence=$(if ($androidVulkanReady) { '0' } else { '1' })",
    "environment_all_platform_unconditional_ready=0",
    "desktop_vulkan_inferred=0",
    "linux_vulkan_inferred=0",
    "native_handle_access=0"
)
Write-Output ([string]::Join(" ", $actualCounters))

if ($RequireReady -and -not $androidVulkanReady) {
    Write-Error "environment-platform-android-vulkan-package requires Android SDK, NDK, adb device/emulator, Vulkan manifest feature declarations, packaged VK_LAYER_KHRONOS_validation, enabled validation layer evidence, package smoke, and Android Vulkan readback evidence."
}
