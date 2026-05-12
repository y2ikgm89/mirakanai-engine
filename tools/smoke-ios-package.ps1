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
    [int]$BootTimeoutSeconds = 180,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

if ($Help) {
    Write-Host "ios-smoke: builds an iOS Simulator app, installs it with xcrun simctl, launches it, and terminates it."
    Write-Host "ios-smoke: parameters: -Game <game_name> -Configuration <Debug|Release> [-BundleIdentifier <id>] [-DeviceUdid <udid>] [-DeviceName <name>] [-SkipBuild] [-BootTimeoutSeconds <seconds>]"
    return
}

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$bootedByScript = $false
$selectedDevice = $null

function Invoke-XcrunAllowFailure {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    & $script:xcrun @Arguments | Out-Null
    return $LASTEXITCODE
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
        Sort-Object @{ Expression = "RuntimeVersion"; Descending = $true }, @{ Expression = "Name"; Descending = $true } |
        Select-Object -First 1
}

function Wait-IosSimulatorBoot {
    param([Parameter(Mandatory = $true)][string]$Udid)

    $process = Start-Process -FilePath $script:xcrun -ArgumentList @("simctl", "bootstatus", $Udid, "-b") -NoNewWindow -PassThru
    if (-not $process.WaitForExit($BootTimeoutSeconds * 1000)) {
        try {
            $process.Kill()
        }
        catch {
        }
        Write-Error "iOS Simulator did not finish booting within $BootTimeoutSeconds seconds: $Udid"
    }

    if ($process.ExitCode -ne 0) {
        Write-Error "xcrun simctl bootstatus failed with exit code $($process.ExitCode) for simulator: $Udid"
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

& (Join-Path $PSScriptRoot "check-mobile-packaging.ps1") -RequireApple

$xcrun = Find-CommandOnCombinedPath "xcrun"
if (-not $xcrun) {
    Write-Error "xcrun is required for iOS Simulator smoke. Install full Xcode and select it with xcode-select."
}
$script:xcrun = $xcrun

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "build-mobile-apple.ps1") `
        -Game $Game `
        -Configuration $Configuration `
        -Platform Simulator `
        -BundleIdentifier $BundleIdentifier
}

$appBundle = Find-IosAppBundle
$selectedDevice = Select-IosSimulator

try {
    if ($selectedDevice.State -ne "Booted") {
        Invoke-CheckedCommand $xcrun "simctl" "boot" $selectedDevice.Udid
        $bootedByScript = $true
    }

    Wait-IosSimulatorBoot $selectedDevice.Udid

    Invoke-CheckedCommand $xcrun "simctl" "install" $selectedDevice.Udid $appBundle
    Invoke-CheckedCommand $xcrun "simctl" "get_app_container" $selectedDevice.Udid $BundleIdentifier "app"
    Invoke-CheckedCommand $xcrun "simctl" "launch" $selectedDevice.Udid $BundleIdentifier
    Start-Sleep -Seconds 2
    [void](Invoke-XcrunAllowFailure @("simctl", "terminate", $selectedDevice.Udid, $BundleIdentifier))

    Write-Host "ios-smoke: device=$($selectedDevice.Name)"
    Write-Host "ios-smoke: udid=$($selectedDevice.Udid)"
    Write-Host "ios-smoke: runtime=$($selectedDevice.RuntimeIdentifier)"
    Write-Host "ios-smoke: app=$appBundle"
    Write-Host "ios-smoke: ok"
}
finally {
    if ($bootedByScript -and $null -ne $selectedDevice) {
        [void](Invoke-XcrunAllowFailure @("simctl", "shutdown", $selectedDevice.Udid))
    }
}

