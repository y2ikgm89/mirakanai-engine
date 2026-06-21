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

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG async overlap performance proof validation."
}

Write-Information "mavg-async-overlap-performance-proof: configuring dev preset..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--preset",
    "dev"
)

Write-Information "mavg-async-overlap-performance-proof: building focused runtime RHI test target..." `
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
    "MK_runtime_rhi_mavg_async_overlap_performance_proof_tests"
)

Write-Information "mavg-async-overlap-performance-proof: running focused runtime RHI CTest lane..." `
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
    "MK_runtime_rhi_mavg_async_overlap_performance_proof_tests|MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests"
)

if ($isWindowsHost) {
    Write-Information "mavg-async-overlap-performance-proof: configuring desktop-runtime preset..." `
        -InformationAction Continue
    Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/cmake.ps1"),
        "--preset",
        "desktop-runtime"
    )

    Write-Information "mavg-async-overlap-performance-proof: building package smoke target..." `
        -InformationAction Continue
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
            "--require-mavg-async-overlap-performance-proof" 2>&1
        $packageSmokeExitCode = $LASTEXITCODE
    }
}
$packageSmokeText = [string]::Join("`n", @($packageSmokeOutput))

$hostReady = $isWindowsHost
$packageSmokeReady = $packageSmokeExitCode -eq 0
$status = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_status" `
    -DefaultValue "missing"
$proofReadyCounter = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_ready" `
    -DefaultValue "0"
$sampleCount = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_samples" `
    -DefaultValue "0"
$overlappedSampleCount = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_overlapped_samples" `
    -DefaultValue "0"
$serialP95Ticks = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_serial_p95_ticks" `
    -DefaultValue "0"
$overlappedP95Ticks = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_overlapped_p95_ticks" `
    -DefaultValue "0"
$overlapTicks = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_overlap_ticks" `
    -DefaultValue "0"
$speedupBasisPoints = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_speedup_basis_points" `
    -DefaultValue "0"
$claimedSpeedup = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_claimed_speedup" `
    -DefaultValue "0"
$provedAsyncOverlapPerformance = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_proved_async_overlap_performance" `
    -DefaultValue "0"
$nativeHandles = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_native_handles_exposed" `
    -DefaultValue "1"
$gpuDirectStorageDestinations = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_gpu_directstorage_destinations" `
    -DefaultValue "1"
$gdeflate = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_gdeflate" `
    -DefaultValue "1"
$meshShaderExecution = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_mesh_shader_execution" `
    -DefaultValue "1"
$metalReadiness = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_metal_readiness" `
    -DefaultValue "1"
$naniteEquivalence = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_nanite_equivalence" `
    -DefaultValue "1"
$broadOptimization = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_async_overlap_performance_proof_broad_optimization" `
    -DefaultValue "1"

$planText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-06-21-mavg-async-overlap-performance-proof-v1.md") -Raw
$capabilitiesText = Get-Content -LiteralPath (Join-Path $root "docs/current-capabilities.md") -Raw
$roadmapText = Get-Content -LiteralPath (Join-Path $root "docs/roadmap.md") -Raw
$manifestText = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw

foreach ($docCheck in @(
        @{ Text = $planText; Context = "MAVG async overlap performance proof plan" },
        @{ Text = $capabilitiesText; Context = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Context = "docs/roadmap.md" },
        @{ Text = $manifestText; Context = "engine/agent/manifest.json" }
    )) {
    Assert-TextContains -Text $docCheck.Text -Needle "mavg-async-overlap-performance-proof-v1" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "selected measured-sample async-overlap performance proof" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "RuntimeMavgAsyncOverlapPerformanceProofResult" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "mavg_async_overlap_performance_proof_ready=1" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "broad optimization" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "native handles" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "Metal readiness" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "Nanite" -Context $docCheck.Context
}

$ready = $hostReady -and
    $packageSmokeReady -and
    $status -eq "ready" -and
    $proofReadyCounter -eq "1" -and
    $sampleCount -eq "10" -and
    $overlappedSampleCount -eq "5" -and
    $serialP95Ticks -eq "140" -and
    $overlappedP95Ticks -eq "105" -and
    $overlapTicks -eq "30" -and
    $speedupBasisPoints -eq "2500" -and
    $claimedSpeedup -eq "1" -and
    $provedAsyncOverlapPerformance -eq "1" -and
    $nativeHandles -eq "0" -and
    $gpuDirectStorageDestinations -eq "0" -and
    $gdeflate -eq "0" -and
    $meshShaderExecution -eq "0" -and
    $metalReadiness -eq "0" -and
    $naniteEquivalence -eq "0" -and
    $broadOptimization -eq "0"

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-async-overlap-performance-proof")
$lines.Add("mavg_async_overlap_performance_proof_status=$(if ($ready) { 'ready' } else { 'blocked' })")
$lines.Add("mavg_async_overlap_performance_proof_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_async_overlap_performance_proof_windows_host=$(ConvertTo-CounterBit $hostReady)")
$lines.Add("mavg_async_overlap_performance_proof_package_smoke_ready=$(ConvertTo-CounterBit $packageSmokeReady)")
$lines.Add("mavg_async_overlap_performance_proof_samples=$sampleCount")
$lines.Add("mavg_async_overlap_performance_proof_overlapped_samples=$overlappedSampleCount")
$lines.Add("mavg_async_overlap_performance_proof_serial_p95_ticks=$serialP95Ticks")
$lines.Add("mavg_async_overlap_performance_proof_overlapped_p95_ticks=$overlappedP95Ticks")
$lines.Add("mavg_async_overlap_performance_proof_overlap_ticks=$overlapTicks")
$lines.Add("mavg_async_overlap_performance_proof_speedup_basis_points=$speedupBasisPoints")
$lines.Add("mavg_async_overlap_performance_proof_claimed_speedup=$claimedSpeedup")
$lines.Add("mavg_async_overlap_performance_proof_proved_async_overlap_performance=$provedAsyncOverlapPerformance")
$lines.Add("mavg_async_overlap_performance_proof_native_handles_exposed=$nativeHandles")
$lines.Add("mavg_async_overlap_performance_proof_gpu_directstorage_destinations=$gpuDirectStorageDestinations")
$lines.Add("mavg_async_overlap_performance_proof_gdeflate=$gdeflate")
$lines.Add("mavg_async_overlap_performance_proof_mesh_shader_execution=$meshShaderExecution")
$lines.Add("mavg_async_overlap_performance_proof_metal_readiness=$metalReadiness")
$lines.Add("mavg_async_overlap_performance_proof_nanite_equivalence=$naniteEquivalence")
$lines.Add("mavg_async_overlap_performance_proof_broad_optimization=$broadOptimization")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG async overlap performance proof is incomplete; focused CTest, package smoke counters, selected-sample speedup evidence, and non-claim docs/manifest evidence are required."
}
