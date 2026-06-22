#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$Game = "sample_headless",
    [switch]$RunPackageCommands,
    [switch]$RequireReady,
    [string]$DeviceSerial = "",
    [switch]$StartEmulator,
    [string]$AvdName = "GameEngine_API36",
    [int]$EmulatorPort = 5588,
    [int]$BootTimeoutSeconds = 180,
    [ValidateSet("", "arm64-v8a", "x86_64")]
    [string]$AndroidAbi = "",
    [string]$ValidationLayerJniLibs = "",
    [string[]]$ExpectedEvidenceCounters = @(),
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$RemainingArguments = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
if ($RemainingArguments.Count -gt 0) {
    $ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($RemainingArguments)
}

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Test-TextCounters {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string[]]$Counters
    )

    foreach ($counter in $Counters) {
        if (-not $Text.Contains($counter)) {
            return $false
        }
    }
    return $true
}

function Resolve-HostPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }
    return Join-Path $root $Path
}

function Test-ValidationLayerJniLibsReady {
    param(
        [AllowNull()][string]$JniLibs,
        [Parameter(Mandatory = $true)][string]$Abi
    )

    if ([string]::IsNullOrWhiteSpace($JniLibs)) {
        return $false
    }
    $layer = Join-Path (Resolve-HostPath $JniLibs) (Join-Path $Abi "libVkLayer_khronos_validation.so")
    return Test-Path -LiteralPath $layer -PathType Leaf
}

function Invoke-ToolCapture {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @(),
        [int]$TimeoutSeconds = 20,
        [hashtable]$Environment = @{}
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
    foreach ($entry in $Environment.GetEnumerator()) {
        $startInfo.Environment[$entry.Key] = [string]$entry.Value
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        $null = $process.Start()
        $standardOutput = $process.StandardOutput.ReadToEndAsync()
        $standardError = $process.StandardError.ReadToEndAsync()
        $completed = $process.WaitForExit($TimeoutSeconds * 1000)
        if (-not $completed) {
            try {
                $process.Kill($true)
            } catch {
                Write-Verbose "failed to kill timed-out process '$FilePath': $_"
            }
            $null = $process.WaitForExit(5000)
            $null = $standardOutput.Wait(5000)
            $null = $standardError.Wait(5000)
            return [pscustomobject]@{
                ExitCode = -1
                Output = if ($standardOutput.IsCompleted) { $standardOutput.Result } else { "" }
                Error = [string]::Join("`n", @(
                        "timeout",
                        $(if ($standardError.IsCompleted) { $standardError.Result } else { "" })
                    )).Trim()
            }
        }
        $null = $standardOutput.Wait(5000)
        $null = $standardError.Wait(5000)
        return [pscustomobject]@{
            ExitCode = $process.ExitCode
            Output = if ($standardOutput.IsCompleted) { $standardOutput.Result } else { "" }
            Error = if ($standardError.IsCompleted) { $standardError.Result } else { "" }
        }
    } finally {
        $process.Dispose()
    }
}

function Get-ToolText {
    param([Parameter(Mandatory = $true)][pscustomobject]$Result)

    return [string]::Join("`n", @([string]$Result.Output, [string]$Result.Error)).Trim()
}

function Write-ToolFailureLines {
    param(
        [Parameter(Mandatory = $true)][string]$Prefix,
        [Parameter(Mandatory = $true)][string]$Text
    )

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return
    }
    Write-Output "$Prefix-output-begin"
    foreach ($line in ($Text -split "`r?`n")) {
        if (-not [string]::IsNullOrWhiteSpace($line)) {
            Write-Output "${Prefix}-output: $line"
        }
    }
    Write-Output "$Prefix-output-end"
}

function Invoke-MobilePackagingProbe {
    $scriptPath = Join-Path $PSScriptRoot "check-mobile-packaging.ps1"
    $arguments = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $scriptPath)
    $environment = @{}
    if ($RunPackageCommands.IsPresent -or $RequireReady.IsPresent) {
        $arguments += "-RequireAndroid"
    } else {
        $environment["MK_MOBILE_DEVICE_PROBE"] = "0"
    }

    $result = Invoke-ToolCapture -FilePath "pwsh" -Arguments $arguments -TimeoutSeconds 120 -Environment $environment
    $text = Get-ToolText $result
    return [pscustomobject]@{
        Ready = $result.ExitCode -eq 0 -and $text.Contains("mobile-packaging: android=ready")
        DiagnosticOnly = $text.Contains("mobile-packaging-check: diagnostic-only")
        Text = $text
    }
}

function Invoke-AndroidDebugBuildIfRequired {
    param([bool]$ShouldRun)

    if (-not $ShouldRun) {
        return [pscustomobject]@{
            Ready = $false
            Text = ""
        }
    }

    $scriptPath = Join-Path $PSScriptRoot "build-mobile-android.ps1"
    $arguments = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $scriptPath,
        "-Game",
        $Game,
        "-Configuration",
        "Debug"
    )
    if (-not [string]::IsNullOrWhiteSpace($AndroidAbi)) {
        $arguments += @("-AndroidAbi", $AndroidAbi)
    }
    if (-not [string]::IsNullOrWhiteSpace($ValidationLayerJniLibs)) {
        $arguments += @("-ValidationLayerJniLibs", $ValidationLayerJniLibs)
    }

    $result = Invoke-ToolCapture -FilePath "pwsh" -Arguments $arguments -TimeoutSeconds 1800
    $text = Get-ToolText $result
    $apk = Join-Path $root "platform\android\app\build\outputs\apk\debug\app-debug.apk"
    return [pscustomobject]@{
        Ready = $result.ExitCode -eq 0 -and (Test-Path -LiteralPath $apk -PathType Leaf)
        Text = $text
    }
}

function Invoke-AndroidReleasePackageIfRequired {
    param([bool]$ShouldRun)

    if (-not $ShouldRun) {
        return [pscustomobject]@{
            Ready = $false
            LocalValidationKeyReady = $false
            Text = ""
        }
    }

    $scriptPath = Join-Path $PSScriptRoot "check-android-release-package.ps1"
    $arguments = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $scriptPath,
        "-Game",
        $Game,
        "-UseLocalValidationKey"
    )
    if (-not [string]::IsNullOrWhiteSpace($AndroidAbi)) {
        $arguments += @("-AndroidAbi", $AndroidAbi)
    }
    if (-not [string]::IsNullOrWhiteSpace($ValidationLayerJniLibs)) {
        $arguments += @("-ValidationLayerJniLibs", $ValidationLayerJniLibs)
    }

    $result = Invoke-ToolCapture -FilePath "pwsh" -Arguments $arguments -TimeoutSeconds 2400
    $text = Get-ToolText $result
    $apk = Join-Path $root "platform\android\app\build\outputs\apk\release\app-release.apk"
    $ready = $result.ExitCode -eq 0 -and $text.Contains("android-release-package-check: ok") -and
        (Test-Path -LiteralPath $apk -PathType Leaf)
    return [pscustomobject]@{
        Ready = $ready
        LocalValidationKeyReady = $ready
        Text = $text
    }
}

function Invoke-AndroidReleaseSmokeIfRequired {
    param([bool]$ShouldRun)

    if (-not $ShouldRun) {
        return [pscustomobject]@{
            Ready = $false
            Text = ""
        }
    }

    $scriptPath = Join-Path $PSScriptRoot "smoke-android-package.ps1"
    $arguments = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $scriptPath,
        "-Game",
        $Game,
        "-Configuration",
        "Release",
        "-SkipBuild",
        "-BootTimeoutSeconds",
        [string]$BootTimeoutSeconds
    )
    if (-not [string]::IsNullOrWhiteSpace($DeviceSerial)) {
        $arguments += @("-DeviceSerial", $DeviceSerial)
    }
    if ($StartEmulator.IsPresent) {
        $arguments += @("-StartEmulator", "-AvdName", $AvdName, "-EmulatorPort", [string]$EmulatorPort)
    }
    if (-not [string]::IsNullOrWhiteSpace($AndroidAbi)) {
        $arguments += @("-AndroidAbi", $AndroidAbi)
    }
    if (-not [string]::IsNullOrWhiteSpace($ValidationLayerJniLibs)) {
        $arguments += @("-ValidationLayerJniLibs", $ValidationLayerJniLibs)
        $arguments += "-RequirePackagedValidationLayer"
    }

    $result = Invoke-ToolCapture -FilePath "pwsh" -Arguments $arguments -TimeoutSeconds 900
    $text = Get-ToolText $result
    return [pscustomobject]@{
        Ready = $result.ExitCode -eq 0 -and $text.Contains("android-smoke: ok")
        Text = $text
    }
}

$defaultFailClosedCounters = @(
    "android_gameactivity_run_package_commands=0",
    "android_gameactivity_requires_host_operation=1",
    "android_gameactivity_debug_build_ready=0",
    "android_gameactivity_release_package_ready=0",
    "android_gameactivity_release_smoke_ready=0",
    "android_gameactivity_host_ready=0",
    "android_gameactivity_validation_layer_jni_libs_ready=0",
    "android_gameactivity_production_signing_material=0",
    "android_gameactivity_play_upload_ready=0",
    "android_gameactivity_native_handle_access=0"
)

$effectiveAndroidAbi = if ([string]::IsNullOrWhiteSpace($AndroidAbi)) { "arm64-v8a" } else { $AndroidAbi }
$validationLayerJniLibsReady = Test-ValidationLayerJniLibsReady -JniLibs $ValidationLayerJniLibs -Abi $effectiveAndroidAbi
$mobileProbe = Invoke-MobilePackagingProbe
$debugBuild = Invoke-AndroidDebugBuildIfRequired -ShouldRun:($RunPackageCommands.IsPresent -and $mobileProbe.Ready)
if ($RunPackageCommands.IsPresent -and -not $debugBuild.Ready) {
    Write-ToolFailureLines -Prefix "android-gameactivity-debug-build" -Text $debugBuild.Text
}
$releasePackage = Invoke-AndroidReleasePackageIfRequired -ShouldRun:($RunPackageCommands.IsPresent -and $debugBuild.Ready)
if ($RunPackageCommands.IsPresent -and $debugBuild.Ready -and -not $releasePackage.Ready) {
    Write-ToolFailureLines -Prefix "android-gameactivity-release-package" -Text $releasePackage.Text
}
$releaseSmoke = Invoke-AndroidReleaseSmokeIfRequired -ShouldRun:($RunPackageCommands.IsPresent -and $releasePackage.Ready)
if ($RunPackageCommands.IsPresent -and $releasePackage.Ready -and -not $releaseSmoke.Ready) {
    Write-ToolFailureLines -Prefix "android-gameactivity-release-smoke" -Text $releaseSmoke.Text
}

$hostReady = $mobileProbe.Ready -and $debugBuild.Ready -and $releasePackage.Ready -and $releaseSmoke.Ready
$requiresHostOperation = -not $hostReady
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=android-gameactivity-host")
$lines.Add("host_gate=android-gameactivity")
$lines.Add("android_gameactivity_game=$Game")
$lines.Add("android_gameactivity_android_abi=$effectiveAndroidAbi")
$lines.Add("android_gameactivity_run_package_commands=$(ConvertTo-CounterBit $RunPackageCommands.IsPresent)")
$lines.Add("android_gameactivity_mobile_check_ready=$(ConvertTo-CounterBit $mobileProbe.Ready)")
$lines.Add("android_gameactivity_mobile_check_diagnostic_only=$(ConvertTo-CounterBit $mobileProbe.DiagnosticOnly)")
$lines.Add("android_gameactivity_validation_layer_jni_libs_ready=$(ConvertTo-CounterBit $validationLayerJniLibsReady)")
$lines.Add("android_gameactivity_requires_host_operation=$(ConvertTo-CounterBit $requiresHostOperation)")
$lines.Add("android_gameactivity_debug_build_ready=$(ConvertTo-CounterBit $debugBuild.Ready)")
$lines.Add("android_gameactivity_release_package_ready=$(ConvertTo-CounterBit $releasePackage.Ready)")
$lines.Add("android_gameactivity_local_validation_key_ready=$(ConvertTo-CounterBit $releasePackage.LocalValidationKeyReady)")
$lines.Add("android_gameactivity_release_smoke_ready=$(ConvertTo-CounterBit $releaseSmoke.Ready)")
$lines.Add("android_gameactivity_host_ready=$(ConvertTo-CounterBit $hostReady)")
$lines.Add("android_gameactivity_production_signing_material=0")
$lines.Add("android_gameactivity_play_upload_ready=0")
$lines.Add("android_gameactivity_physical_device_matrix_ready=0")
$lines.Add("android_gameactivity_native_handle_access=0")

foreach ($line in $lines) {
    Write-Output $line
}

$counterText = [string]::Join("`n", $lines)
if ($ExpectedEvidenceCounters.Count -gt 0 -and -not (Test-TextCounters -Text $counterText -Counters $ExpectedEvidenceCounters)) {
    foreach ($counter in $ExpectedEvidenceCounters) {
        if (-not $counterText.Contains($counter)) {
            Write-Error "Android GameActivity host validator missing expected evidence counter: $counter"
        }
    }
}

if ($RequireReady.IsPresent -and -not $hostReady) {
    Write-Error "Android GameActivity host gate is not ready. Re-run with -RunPackageCommands on a ready Android host and inspect the emitted counters."
}
