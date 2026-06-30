#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$Game = "sample_headless",
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [string]$BundleIdentifier = "",
    [string]$DeviceUdid = "",
    [string]$DeviceName = "",
    [switch]$SkipBuild,
    [int]$BootTimeoutSeconds = 420,
    [int]$BootAttempts = 2,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

if ($Help) {
    Write-Output "ios-smoke: builds an iOS Simulator app, installs it with xcrun simctl, launches it, and terminates it."
    Write-Output "ios-smoke: parameters: -Game <game_name> -Configuration <Debug|Release> [-BundleIdentifier <id>] [-DeviceUdid <udid>] [-DeviceName <name>] [-SkipBuild] [-BootTimeoutSeconds <seconds>] [-BootAttempts <count>]"
    return
}

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$bootedByScript = $false
$selectedDevice = $null

function Invoke-XcrunCapture {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [int]$TimeoutSeconds = 120
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $script:xcrun
    $startInfo.WorkingDirectory = $root
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in @($Arguments)) {
        $startInfo.ArgumentList.Add($argument) | Out-Null
    }
    $startInfo.Environment.Clear()
    foreach ($entry in Get-NormalizedProcessEnvironment) {
        $startInfo.Environment[$entry.Key] = $entry.Value
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        $null = $process.Start()
        $standardOutput = $process.StandardOutput.ReadToEndAsync()
        $standardError = $process.StandardError.ReadToEndAsync()
        $completed = $process.WaitForExit($TimeoutSeconds * 1000)
        $timedOut = -not $completed
        if ($timedOut) {
            try {
                $process.Kill($true)
            } catch {
                Write-Information "ios-smoke: failed to terminate timed-out xcrun process: $($_.Exception.Message)" -InformationAction Continue
            }
            $null = $process.WaitForExit(5000)
        }

        $null = $standardOutput.Wait(5000)
        $null = $standardError.Wait(5000)
        $output = if ($standardOutput.IsCompleted) { $standardOutput.Result } else { "" }
        $errorOutput = if ($standardError.IsCompleted) { $standardError.Result } else { "" }
        $combinedOutput = [string]::Join("`n", @($output, $errorOutput)).Trim()
        if ($timedOut) {
            Write-Error "xcrun $($Arguments -join ' ') timed out after $TimeoutSeconds second(s): $combinedOutput"
        }
        if ($process.ExitCode -ne 0) {
            Write-Error "xcrun $($Arguments -join ' ') failed with exit code $($process.ExitCode): $combinedOutput"
        }

        return $output.Trim()
    } finally {
        $process.Dispose()
    }
}

function Invoke-XcrunAllowFailure {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [int]$TimeoutSeconds = 60
    )

    try {
        [void](Invoke-XcrunCapture -Arguments $Arguments -TimeoutSeconds $TimeoutSeconds)
        return 0
    } catch {
        Write-Information "ios-smoke: allowed xcrun failure: $($_.Exception.Message)" -InformationAction Continue
        return 1
    }
}

function Convert-IosRuntimeVersion {
    param([Parameter(Mandatory = $true)][string]$RuntimeIdentifier)

    if ($RuntimeIdentifier -match "iOS-([0-9]+)(?:-([0-9]+))?(?:-([0-9]+))?") {
        $minor = if ([string]::IsNullOrWhiteSpace($Matches[2])) { "0" } else { $Matches[2] }
        $patch = if ([string]::IsNullOrWhiteSpace($Matches[3])) { "0" } else { $Matches[3] }
        return [version]"$($Matches[1]).$minor.$patch"
    }

    return [version]"0.0.0"
}

function Get-AvailableIosSimulators {
    $jsonText = @(& $script:xcrun "simctl" "list" "--json" "devices" "available" 2>$null) -join "`n"
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($jsonText)) {
        Write-Error "xcrun simctl list --json devices available failed."
    }

    $json = $jsonText | ConvertFrom-Json
    foreach ($runtimeProperty in $json.devices.PSObject.Properties) {
        $runtimeIdentifier = [string]$runtimeProperty.Name
        if ($runtimeIdentifier -notmatch "iOS") {
            continue
        }

        foreach ($device in @($runtimeProperty.Value)) {
            if (-not $device.isAvailable) {
                continue
            }

            [pscustomobject]@{
                Udid = [string]$device.udid
                Name = [string]$device.name
                State = [string]$device.state
                RuntimeIdentifier = $runtimeIdentifier
                RuntimeVersion = Convert-IosRuntimeVersion $runtimeIdentifier
            }
        }
    }
}

function Select-IosSimulator {
    $devices = @(Get-AvailableIosSimulators | Where-Object { $_.Name -like "iPhone*" })
    if ($devices.Count -eq 0) {
        Write-Error "No available iPhone Simulator device was found. Install an iOS Simulator runtime with Xcode components before running iOS smoke."
    }

    if (-not [string]::IsNullOrWhiteSpace($DeviceUdid)) {
        $byUdid = $devices | Where-Object { $_.Udid -eq $DeviceUdid } | Select-Object -First 1
        if (-not $byUdid) {
            Write-Error "Requested iOS Simulator UDID was not found or is unavailable: $DeviceUdid"
        }
        return $byUdid
    }

    $candidates = $devices
    if (-not [string]::IsNullOrWhiteSpace($DeviceName)) {
        $candidates = @($devices | Where-Object { $_.Name -eq $DeviceName })
        if ($candidates.Count -eq 0) {
            Write-Error "Requested iOS Simulator device name was not found or is unavailable: $DeviceName"
        }
    }

    return $candidates |
        Sort-Object @{ Expression = { Get-IosSimulatorPreference $_ }; Ascending = $true },
        @{ Expression = "RuntimeVersion"; Descending = $true },
        @{ Expression = "Name"; Ascending = $true } |
        Select-Object -First 1
}

function Get-IosSimulatorPreference {
    param([Parameter(Mandatory = $true)]$Device)

    $name = [string]$Device.Name
    if ($name -match "^iPhone [0-9]+$") {
        return 0
    }
    if ($name -like "iPhone SE*") {
        return 1
    }
    if ($name -notmatch "Pro|Max|Plus") {
        return 2
    }
    return 3
}

function Wait-IosSimulatorBootOnce {
    param([Parameter(Mandatory = $true)][string]$Udid)

    $process = Start-Process -FilePath $script:xcrun -ArgumentList @("simctl", "bootstatus", $Udid, "-b") -NoNewWindow -PassThru
    if (-not $process.WaitForExit($BootTimeoutSeconds * 1000)) {
        try {
            $process.Kill()
            $process.WaitForExit(5000) | Out-Null
        }
        catch {
            Write-Information "ios-smoke: failed to terminate timed-out simctl bootstatus process: $($_.Exception.Message)" -InformationAction Continue
        }
        return $false
    }

    return $process.ExitCode -eq 0
}

function Wait-IosSimulatorBoot {
    param(
        [Parameter(Mandatory = $true)][string]$Udid,
        [switch]$AllowRestart
    )

    for ($attempt = 1; $attempt -le $BootAttempts; $attempt++) {
        Write-Information "ios-smoke: waiting for simulator boot ($attempt/$BootAttempts): $Udid" -InformationAction Continue
        if (Wait-IosSimulatorBootOnce $Udid) {
            return
        }

        if ($attempt -lt $BootAttempts -and $AllowRestart) {
            Write-Information "ios-smoke: simulator boot wait timed out; restarting simulator: $Udid" -InformationAction Continue
            [void](Invoke-XcrunAllowFailure @("simctl", "shutdown", $Udid))
            Invoke-CheckedCommand $script:xcrun "simctl" "boot" $Udid
        }
    }

    Write-Error "iOS Simulator did not finish booting after $BootAttempts attempt(s) of $BootTimeoutSeconds seconds: $Udid"
}

function Invoke-IosSimulatorInstall {
    param(
        [Parameter(Mandatory = $true)]$Device,
        [Parameter(Mandatory = $true)][string]$AppBundle
    )

    for ($attempt = 1; $attempt -le 2; $attempt++) {
        try {
            if ($attempt -gt 1) {
                Write-Information "ios-smoke: install retry start" -InformationAction Continue
            }
            [void](Invoke-XcrunCapture -Arguments @("simctl", "install", $Device.Udid, $AppBundle) -TimeoutSeconds 240)
            return
        } catch {
            if ($attempt -ge 2) {
                throw
            }

            Write-Information "ios-smoke: install retry after failure: $($_.Exception.Message)" -InformationAction Continue
            [void](Invoke-XcrunAllowFailure @("simctl", "shutdown", $Device.Udid))
            Invoke-CheckedCommand $script:xcrun "simctl" "boot" $Device.Udid
            $script:bootedByScript = $true
            Wait-IosSimulatorBoot $Device.Udid -AllowRestart
        }
    }
}

function Find-IosAppBundle {
    $buildDir = Join-Path $root "out\build\ios-Simulator-$Game-$Configuration"
    if (-not (Test-Path -LiteralPath $buildDir -PathType Container)) {
        Write-Error "iOS build directory was not found for smoke: $buildDir"
    }

    $configurationDirectory = "$Configuration-iphonesimulator"
    $bundle = Get-ChildItem -LiteralPath $buildDir -Directory -Filter "MirakanaiIOS.app" -Recurse |
        Where-Object { $_.FullName -match [regex]::Escape($configurationDirectory) } |
        Select-Object -First 1
    if (-not $bundle) {
        Write-Error "iOS app bundle was not found below $buildDir for $configurationDirectory."
    }

    return $bundle.FullName
}

if ([string]::IsNullOrWhiteSpace($BundleIdentifier)) {
    $BundleIdentifier = $env:MK_IOS_BUNDLE_IDENTIFIER
}

if ([string]::IsNullOrWhiteSpace($BundleIdentifier)) {
    $BundleIdentifier = "dev.mirakanai.ios"
}

Write-Information "ios-smoke: packaging preflight start" -InformationAction Continue
& (Join-Path $PSScriptRoot "check-mobile-packaging.ps1") -RequireApple
Write-Information "ios-smoke: packaging preflight ok" -InformationAction Continue

$xcrun = Find-CommandOnCombinedPath "xcrun"
if (-not $xcrun) {
    Write-Error "xcrun is required for iOS Simulator smoke. Install full Xcode and select it with xcode-select."
}
$script:xcrun = $xcrun
Write-Information "ios-smoke: xcrun=$xcrun" -InformationAction Continue

if (-not $SkipBuild) {
    Write-Information "ios-smoke: build start game=$Game configuration=$Configuration platform=Simulator" -InformationAction Continue
    & (Join-Path $PSScriptRoot "build-mobile-apple.ps1") `
        -Game $Game `
        -Configuration $Configuration `
        -Platform Simulator `
        -BundleIdentifier $BundleIdentifier
    Write-Information "ios-smoke: build done" -InformationAction Continue
} else {
    Write-Information "ios-smoke: build skipped" -InformationAction Continue
}

$appBundle = Find-IosAppBundle
Write-Information "ios-smoke: app bundle=$appBundle" -InformationAction Continue
$selectedDevice = Select-IosSimulator
Write-Information "ios-smoke: selected device=$($selectedDevice.Name) udid=$($selectedDevice.Udid) runtime=$($selectedDevice.RuntimeIdentifier) state=$($selectedDevice.State)" -InformationAction Continue

try {
    if ($selectedDevice.State -ne "Booted") {
        Write-Information "ios-smoke: boot request device=$($selectedDevice.Udid)" -InformationAction Continue
        Invoke-CheckedCommand $xcrun "simctl" "boot" $selectedDevice.Udid
        $bootedByScript = $true
    }

    Wait-IosSimulatorBoot $selectedDevice.Udid -AllowRestart:($selectedDevice.State -ne "Booted")

    Write-Information "ios-smoke: install start" -InformationAction Continue
    Invoke-IosSimulatorInstall -Device $selectedDevice -AppBundle $appBundle
    Write-Information "ios-smoke: install done" -InformationAction Continue
    Write-Information "ios-smoke: verify app container start" -InformationAction Continue
    [void](Invoke-XcrunCapture -Arguments @("simctl", "get_app_container", $selectedDevice.Udid, $BundleIdentifier, "app") -TimeoutSeconds 60)
    Write-Information "ios-smoke: launch start" -InformationAction Continue
    [void](Invoke-XcrunCapture -Arguments @("simctl", "launch", $selectedDevice.Udid, $BundleIdentifier) -TimeoutSeconds 120)
    Write-Information "ios-smoke: launch done" -InformationAction Continue
    Start-Sleep -Seconds 2
    Write-Information "ios-smoke: evidence container lookup start" -InformationAction Continue
    $dataContainer = Invoke-XcrunCapture -Arguments @("simctl", "get_app_container", $selectedDevice.Udid, $BundleIdentifier, "data") -TimeoutSeconds 60
    $iosMetalEvidencePath = Join-Path $dataContainer "Library/Caches/mirakanai_ios_metal_evidence.txt"
    if (-not (Test-Path -LiteralPath $iosMetalEvidencePath -PathType Leaf)) {
        Write-Error "iOS Metal evidence file was not written by the launched app: $iosMetalEvidencePath"
    }
    $iosMetalEvidence = Get-Content -LiteralPath $iosMetalEvidencePath -Raw
    foreach ($needle in @(
            "ios_metal_feature_set_checked=1",
            "ios_metal_command_queue_ready=1",
            "ios_metal_pipeline_ready=1",
            "ios_metal_command_buffer_ready=1",
            "ios_metal_readback_ready=1")) {
        if (-not $iosMetalEvidence.Contains($needle)) {
            Write-Error "iOS Metal evidence did not contain required counter '$needle': $iosMetalEvidencePath"
        }
    }
    [void](Invoke-XcrunAllowFailure @("simctl", "terminate", $selectedDevice.Udid, $BundleIdentifier))

    Write-Output "ios-smoke: device=$($selectedDevice.Name)"
    Write-Output "ios-smoke: udid=$($selectedDevice.Udid)"
    Write-Output "ios-smoke: runtime=$($selectedDevice.RuntimeIdentifier)"
    Write-Output "ios-smoke: app=$appBundle"
    Write-Output "ios-smoke: ios_metal_evidence=$iosMetalEvidencePath"
    foreach ($line in ($iosMetalEvidence -split "`r?`n")) {
        if (-not [string]::IsNullOrWhiteSpace($line)) {
            Write-Output "ios-smoke: $line"
        }
    }
    Write-Output "ios-smoke: ok"
}
finally {
    if ($bootedByScript -and $null -ne $selectedDevice) {
        [void](Invoke-XcrunAllowFailure @("simctl", "shutdown", $selectedDevice.Udid))
    }
}
