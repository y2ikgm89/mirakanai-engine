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

function Assert-TextContains {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Text.Contains($Needle)) {
        Write-Error "$Context must contain '$Needle'."
    }
}

$isWindowsHost = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [System.Runtime.InteropServices.OSPlatform]::Windows)

$dependencyPaths = @(
    "vcpkg_installed/x64-windows/include/dstorage.h",
    "vcpkg_installed/x64-windows/include/dstorageerr.h",
    "vcpkg_installed/x64-windows/bin/dstorage.dll",
    "vcpkg_installed/x64-windows/bin/dstoragecore.dll",
    "vcpkg_installed/x64-windows/share/dstorage/dstorage-config.cmake"
)
$missingDependencyPaths = @()
foreach ($relativeDependencyPath in $dependencyPaths) {
    $dependencyPath = Join-Path $root $relativeDependencyPath
    if (-not (Test-Path -LiteralPath $dependencyPath -PathType Leaf)) {
        $missingDependencyPaths += $relativeDependencyPath
    }
}

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG Win32 DirectStorage SDK adapter validation."
}

if ($isWindowsHost -and $missingDependencyPaths.Count -eq 0) {
    Write-Information "mavg-win32-directstorage-sdk-adapter: configuring DirectStorage preset..." `
        -InformationAction Continue
    Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/cmake.ps1"),
        "--preset",
        "directstorage-win32"
    )

    Write-Information "mavg-win32-directstorage-sdk-adapter: building adapter test and package smoke target..." `
        -InformationAction Continue
    Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/cmake.ps1"),
        "--build",
        "--preset",
        "directstorage-win32",
        "--target",
        "MK_win32_directstorage_byte_range_io_tests",
        "sample_desktop_runtime_game"
    )

    Write-Information "mavg-win32-directstorage-sdk-adapter: running focused adapter CTest..." `
        -InformationAction Continue
    Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/ctest.ps1"),
        "--preset",
        "directstorage-win32",
        "--output-on-failure",
        "-R",
        "MK_win32_directstorage_byte_range_io_tests"
    )
}

$packageSmokeOutput = @()
$packageSmokeExitCode = 1
if ($isWindowsHost -and $missingDependencyPaths.Count -eq 0) {
    $packageSmokeExe = Join-Path $root "out/build/directstorage-win32/games/Debug/sample_desktop_runtime_game/sample_desktop_runtime_game.exe"
    if (Test-Path -LiteralPath $packageSmokeExe -PathType Leaf) {
        $packageSmokeOutput = & $packageSmokeExe `
            "--smoke" `
            "--max-frames" `
            "1" `
            "--require-mavg-win32-directstorage-sdk-adapter" 2>&1
        $packageSmokeExitCode = $LASTEXITCODE
    }
}
$packageSmokeText = [string]::Join("`n", @($packageSmokeOutput))

$hostReady = $isWindowsHost
$dependencyReady = $missingDependencyPaths.Count -eq 0
$packageSmokeReady = $packageSmokeExitCode -eq 0
$status = Get-CounterValueFromText -Text $packageSmokeText -Name "mavg_win32_directstorage_sdk_adapter_status" -DefaultValue "missing"
$adapterReadyCounter = Get-CounterValueFromText -Text $packageSmokeText -Name "mavg_win32_directstorage_sdk_adapter_ready" -DefaultValue "0"
$sdkVersion = Get-CounterValueFromText -Text $packageSmokeText -Name "mavg_win32_directstorage_sdk_adapter_sdk_version" -DefaultValue "missing"
$requestCount = Get-CounterValueFromText -Text $packageSmokeText -Name "mavg_win32_directstorage_sdk_adapter_requests" -DefaultValue "0"
$completedRanges = Get-CounterValueFromText -Text $packageSmokeText -Name "mavg_win32_directstorage_sdk_adapter_completed_ranges" -DefaultValue "0"
$nativeHandles = Get-CounterValueFromText -Text $packageSmokeText -Name "mavg_win32_directstorage_sdk_adapter_native_handles_exposed" -DefaultValue "1"
$gpuDestinations = Get-CounterValueFromText -Text $packageSmokeText -Name "mavg_win32_directstorage_sdk_adapter_gpu_destinations" -DefaultValue "1"
$gdeflate = Get-CounterValueFromText -Text $packageSmokeText -Name "mavg_win32_directstorage_sdk_adapter_gdeflate" -DefaultValue "1"
$asyncOverlapProof = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_win32_directstorage_sdk_adapter_async_overlap_performance_proof" `
    -DefaultValue "1"

$planText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-06-21-mavg-win32-directstorage-sdk-adapter-v1.md") -Raw
$capabilitiesText = Get-Content -LiteralPath (Join-Path $root "docs/current-capabilities.md") -Raw
$roadmapText = Get-Content -LiteralPath (Join-Path $root "docs/roadmap.md") -Raw
$manifestText = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw

foreach ($docCheck in @(
        @{ Text = $planText; Context = "MAVG DirectStorage plan" },
        @{ Text = $capabilitiesText; Context = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Context = "docs/roadmap.md" },
        @{ Text = $manifestText; Context = "engine/agent/manifest.json" }
    )) {
    Assert-TextContains -Text $docCheck.Text -Needle "mavg-win32-directstorage-sdk-adapter-v1" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "Microsoft DirectStorage SDK 1.3.0" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "GPU destinations" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "GDeflate" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "async-overlap/performance proof" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "native handle" -Context $docCheck.Context
}

$ready = $hostReady -and
    $dependencyReady -and
    $packageSmokeReady -and
    $status -eq "ready" -and
    $adapterReadyCounter -eq "1" -and
    $sdkVersion -eq "1.3.0" -and
    $requestCount -eq "2" -and
    $completedRanges -eq "2" -and
    $nativeHandles -eq "0" -and
    $gpuDestinations -eq "0" -and
    $gdeflate -eq "0" -and
    $asyncOverlapProof -eq "0"

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-win32-directstorage-sdk-adapter")
$lines.Add("mavg_win32_directstorage_sdk_adapter_status=$(if ($ready) { 'ready' } else { 'blocked' })")
$lines.Add("mavg_win32_directstorage_sdk_adapter_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_win32_directstorage_sdk_adapter_windows_host=$(ConvertTo-CounterBit $hostReady)")
$lines.Add("mavg_win32_directstorage_sdk_adapter_dependency_ready=$(ConvertTo-CounterBit $dependencyReady)")
$lines.Add("mavg_win32_directstorage_sdk_adapter_package_smoke_ready=$(ConvertTo-CounterBit $packageSmokeReady)")
$lines.Add("mavg_win32_directstorage_sdk_adapter_sdk_version=$sdkVersion")
$lines.Add("mavg_win32_directstorage_sdk_adapter_requests=$requestCount")
$lines.Add("mavg_win32_directstorage_sdk_adapter_completed_ranges=$completedRanges")
$lines.Add("mavg_win32_directstorage_sdk_adapter_native_handles_exposed=$nativeHandles")
$lines.Add("mavg_win32_directstorage_sdk_adapter_gpu_destinations=$gpuDestinations")
$lines.Add("mavg_win32_directstorage_sdk_adapter_gdeflate=$gdeflate")
$lines.Add("mavg_win32_directstorage_sdk_adapter_async_overlap_performance_proof=$asyncOverlapProof")
$lines.Add("mavg_win32_directstorage_sdk_adapter_gpu_destination_ready=0")
$lines.Add("mavg_win32_directstorage_sdk_adapter_gdeflate_ready=0")
$lines.Add("mavg_win32_directstorage_sdk_adapter_async_overlap_ready=0")
$lines.Add("mavg_win32_directstorage_sdk_adapter_metal_ready=0")
$lines.Add("mavg_win32_directstorage_sdk_adapter_broad_optimization_ready=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($missingDependencyPaths.Count -gt 0) {
    Write-Output "mavg_win32_directstorage_sdk_adapter_missing_dependencies=$([string]::Join(',', $missingDependencyPaths))"
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG Win32 DirectStorage SDK adapter is incomplete; Windows host, dstorage files, adapter CTest, package smoke counters, and non-claim docs/manifest evidence are required."
}
