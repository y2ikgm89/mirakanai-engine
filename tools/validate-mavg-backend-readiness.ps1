#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady,
    [string[]]$ExpectedEvidenceCounters = @(),
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)

$root = Get-RepoRoot
Set-Location $root

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Get-CounterValueFromText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$DefaultValue
    )

    $match = [regex]::Match($Text, "(^|\s)$([regex]::Escape($Name))=([^\s]+)")
    if ($match.Success) {
        return $match.Groups[2].Value
    }
    return $DefaultValue
}

$isWindowsHost = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [System.Runtime.InteropServices.OSPlatform]::Windows)

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG backend readiness validation."
}

Write-Information "mavg-backend-readiness: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--preset",
    "dev"
)

Write-Information "mavg-backend-readiness: building focused runtime scene RHI closeout test target..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--build",
    "--preset",
    "dev",
    "--target",
    "MK_runtime_scene_rhi_mavg_backend_readiness_closeout_tests"
)

Write-Information "mavg-backend-readiness: building streamed backend draw evidence test target..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--build",
    "--preset",
    "dev",
    "--target",
    "MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests"
)

Write-Information "mavg-backend-readiness: running focused runtime scene RHI CTest lane..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/ctest.ps1"),
    "--preset",
    "dev",
    "--output-on-failure",
    "-R",
    "MK_runtime_scene_rhi_mavg_backend_readiness_closeout_tests|MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests"
)

if ($isWindowsHost) {
    Write-Information "mavg-backend-readiness: configuring desktop-runtime preset..." -InformationAction Continue
    Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/cmake.ps1"),
        "--preset",
        "desktop-runtime"
    )

    Write-Information "mavg-backend-readiness: building package smoke target..." -InformationAction Continue
    Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/cmake.ps1"),
        "--build",
        "--preset",
        "desktop-runtime",
        "--target",
        "sample_desktop_runtime_game"
    )
}

$packageSmokeOutput = @()
$packageSmokeExitCode = 1
if ($isWindowsHost) {
    $packageSmokeExe = Join-Path $root "out/build/desktop-runtime/games/Debug/sample_desktop_runtime_game/sample_desktop_runtime_game.exe"
    if (Test-Path -LiteralPath $packageSmokeExe -PathType Leaf) {
        $packageSmokeOutput = & $packageSmokeExe `
            "--smoke" `
            "--max-frames" `
            "1" `
            "--require-mavg-backend-readiness" 2>&1
        $packageSmokeExitCode = $LASTEXITCODE
    }
}
$packageSmokeText = [string]::Join("`n", @($packageSmokeOutput))

$hostReady = $isWindowsHost
$packageSmokeReady = $packageSmokeExitCode -eq 0
$status = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_package_visible_backend_readiness_status" `
    -DefaultValue "missing"
$readinessCounter = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_package_visible_backend_readiness_ready" `
    -DefaultValue "0"
$requiredRows = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_package_visible_backend_readiness_required_rows" `
    -DefaultValue "0"
$readyRows = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_package_visible_backend_readiness_ready_rows" `
    -DefaultValue "0"
$diagnostics = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_package_visible_backend_readiness_diagnostics" `
    -DefaultValue "1"
$metalHostGatedRows = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_package_visible_backend_readiness_metal_host_gated_rows" `
    -DefaultValue "1"
$nativeHandles = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_package_visible_backend_readiness_native_handles_exposed" `
    -DefaultValue "1"
$metalInference = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_package_visible_backend_readiness_metal_inference" `
    -DefaultValue "1"
$broadBackendReadiness = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_package_visible_backend_readiness_broad_backend_readiness" `
    -DefaultValue "1"

$ready = $hostReady -and
    $packageSmokeReady -and
    $status -eq "ready" -and
    $readinessCounter -eq "1" -and
    $requiredRows -eq "9" -and
    $readyRows -eq "9" -and
    $diagnostics -eq "0" -and
    $metalHostGatedRows -eq "0" -and
    $nativeHandles -eq "0" -and
    $metalInference -eq "0" -and
    $broadBackendReadiness -eq "0"

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-backend-readiness")
$lines.Add("mavg_package_visible_backend_readiness_status=$(if ($ready) { 'ready' } else { 'blocked' })")
$lines.Add("mavg_package_visible_backend_readiness_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_package_visible_backend_readiness_windows_host=$(ConvertTo-CounterBit $hostReady)")
$lines.Add("mavg_package_visible_backend_readiness_package_smoke_ready=$(ConvertTo-CounterBit $packageSmokeReady)")
$lines.Add("mavg_package_visible_backend_readiness_required_rows=$requiredRows")
$lines.Add("mavg_package_visible_backend_readiness_ready_rows=$readyRows")
$lines.Add("mavg_package_visible_backend_readiness_diagnostics=$diagnostics")
$lines.Add("mavg_package_visible_backend_readiness_metal_host_gated_rows=$metalHostGatedRows")
$lines.Add("mavg_package_visible_backend_readiness_native_handles_exposed=$nativeHandles")
$lines.Add("mavg_package_visible_backend_readiness_metal_inference=$metalInference")
$lines.Add("mavg_package_visible_backend_readiness_broad_backend_readiness=$broadBackendReadiness")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG backend readiness is incomplete; focused closeout CTest, package smoke counters, D3D12/Vulkan row evidence, and non-claim counters are required."
}
