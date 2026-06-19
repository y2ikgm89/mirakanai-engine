#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$InstallPrefix = "",
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

function Test-HostLinux {
    return [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Linux)
}

function Get-CounterValue {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $pattern = "(^|\s)" + [System.Text.RegularExpressions.Regex]::Escape($Name) + "=([^\s]+)"
    $match = [System.Text.RegularExpressions.Regex]::Match($Text, $pattern)
    if (-not $match.Success) {
        return $null
    }
    return $match.Groups[2].Value
}

if ([string]::IsNullOrWhiteSpace($InstallPrefix)) {
    $InstallPrefix = Join-Path $root "out/install/linux-vulkan-runtime-release"
}

$installPrefixPath = [System.IO.Path]::GetFullPath($InstallPrefix)
$binaryPath = Join-Path $installPrefixPath "bin/linux_vulkan_runtime_probe"
$shaderPath = Join-Path $installPrefixPath "bin/shaders/linux_vulkan_runtime_probe_environment_weather_solver.cs.spv"

$hostMatches = Test-HostLinux
$binaryReady = $hostMatches -and (Test-Path -LiteralPath $binaryPath -PathType Leaf)
$shaderReady = $hostMatches -and (Test-Path -LiteralPath $shaderPath -PathType Leaf)
$smokeExitCode = -1
$smokeText = ""

if ($binaryReady -and $shaderReady) {
    $output = & $binaryPath --require-linux-vulkan-runtime-probe --weather-solver-spv $shaderPath 2>&1
    $smokeExitCode = $LASTEXITCODE
    $smokeText = [string]::Join("`n", @($output))
    foreach ($line in @($output)) {
        Write-Output $line
    }
}

$probeReady = $smokeExitCode -eq 0 -and (Get-CounterValue -Text $smokeText -Name "linux_vulkan_runtime_probe_ready") -eq "1"
$readbackReady = $smokeExitCode -eq 0 -and (Get-CounterValue -Text $smokeText -Name "linux_vulkan_runtime_readback_ready") -eq "1"
$surfaceFamily = Get-CounterValue -Text $smokeText -Name "linux_vulkan_runtime_probe_surface_family"
if ([string]::IsNullOrWhiteSpace($surfaceFamily)) {
    $surfaceFamily = "missing"
}
$firstPartyHostReady = $smokeExitCode -eq 0 -and (Get-CounterValue -Text $smokeText -Name "first_party_linux_runtime_host_ready") -eq "1"
$nativeHandleAccess = Get-CounterValue -Text $smokeText -Name "native_handle_access"
if ([string]::IsNullOrWhiteSpace($nativeHandleAccess)) {
    $nativeHandleAccess = "1"
}

$installedValidatorReady = $hostMatches -and $binaryReady -and $shaderReady -and $probeReady -and $readbackReady -and
    $firstPartyHostReady -and $surfaceFamily -eq "offscreen_compute" -and $nativeHandleAccess -eq "0"

$actualCounters = @(
    "validation_recipe=environment-platform-linux-vulkan-installed-runtime",
    "host=linux",
    "host_matches=$(ConvertTo-CounterBit $hostMatches)",
    "linux_installed_validator_ready=$(ConvertTo-CounterBit $installedValidatorReady)",
    "linux_vulkan_runtime_probe_binary_ready=$(ConvertTo-CounterBit $binaryReady)",
    "linux_vulkan_runtime_probe_shader_ready=$(ConvertTo-CounterBit $shaderReady)",
    "linux_vulkan_runtime_probe_ready=$(ConvertTo-CounterBit $probeReady)",
    "linux_vulkan_runtime_readback_ready=$(ConvertTo-CounterBit $readbackReady)",
    "linux_vulkan_runtime_probe_surface_family=$surfaceFamily",
    "first_party_linux_runtime_host_ready=$(ConvertTo-CounterBit $firstPartyHostReady)",
    "native_handle_access=$nativeHandleAccess",
    "environment_all_platform_unconditional_ready=0",
    "environment_commercial_ready=0",
    "environment_ready=0"
)
$actualCounterLine = [string]::Join(" ", $actualCounters)
Write-Output $actualCounterLine

$missingExpectedCounters = @()
foreach ($counter in @($ExpectedEvidenceCounters)) {
    if ([string]::IsNullOrWhiteSpace($counter)) {
        continue
    }
    if (-not $actualCounterLine.Contains($counter) -and -not $smokeText.Contains($counter)) {
        $missingExpectedCounters += $counter
    }
}
if ($missingExpectedCounters.Count -gt 0) {
    Write-Error "installed-linux-runtime-validation is missing expected counters: $($missingExpectedCounters -join ', ')"
}

if ($RequireReady -and -not $installedValidatorReady) {
    Write-Error "installed-linux-runtime-validation requires installed linux_vulkan_runtime_probe binary, packaged SPIR-V shader, offscreen compute readback, first-party Linux runtime host evidence, and zero native-handle access."
}
