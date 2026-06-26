#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Assert-LinePresent {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$ExpectedLine,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Lines.Contains($ExpectedLine)) {
        Write-Error "$Context missing expected line: $ExpectedLine"
    }
}

$producerScript = Join-Path $root "tools/generate-renderer-d3d12-commercial-quality-host-supplement.ps1"
if (-not (Test-Path -LiteralPath $producerScript -PathType Leaf)) {
    Write-Error "tools/generate-renderer-d3d12-commercial-quality-host-supplement.ps1 must exist for retained D3D12 supplemental host proof generation."
}

$probeSource = Join-Path $root "tests/fixtures/d3d12_commercial_quality_host_supplement_probe.cpp"
if (-not (Test-Path -LiteralPath $probeSource -PathType Leaf)) {
    Write-Error "tests/fixtures/d3d12_commercial_quality_host_supplement_probe.cpp must exist for isolated D3D12 debug/GBV host probing."
}

$cmakeText = Get-Content -LiteralPath (Join-Path $root "CMakeLists.txt") -Raw
foreach ($needle in @(
        "MK_d3d12_commercial_quality_host_supplement_probe",
        "d3d12_commercial_quality_host_supplement_probe.cpp",
        "MK_rhi_d3d12"
    )) {
    if (-not $cmakeText.Contains($needle)) {
        Write-Error "CMakeLists.txt missing D3D12 host supplement probe needle: $needle"
    }
}

$probeText = Get-Content -LiteralPath $probeSource -Raw
foreach ($needle in @(
        "collect_commercial_quality_host_supplement",
        "ID3D12CommandQueue::GetClockCalibration",
        "ID3D12Device3::EnqueueMakeResident",
        "IDXGIAdapter3::QueryVideoMemoryInfo",
        "debug_message_count",
        "gpu_based_validation_message_count",
        "D3D12_RESOURCE_BARRIER",
        "native_handles_exposed"
    )) {
    if (-not $probeText.Contains($needle)) {
        Write-Error "D3D12 host supplement probe missing official evidence needle: $needle"
    }
}

$planLines = @(& $producerScript -Mode Plan `
        -OutputRootRelative "artifacts/renderer/d3d12-commercial-quality-host-evidence/supplement-contract-$PID")
foreach ($expectedLine in @(
        "validation_recipe=renderer-d3d12-commercial-quality-host-supplement",
        "renderer_d3d12_commercial_quality_host_supplement_mode=Plan",
        "renderer_d3d12_commercial_quality_host_supplement_ready=0",
        "renderer_d3d12_commercial_quality_host_supplement_written=0",
        "renderer_backend_parity_ready=0",
        "renderer_metal_broad_readiness=0",
        "renderer_broad_quality_ready=0",
        "renderer_commercial_readiness=0",
        "renderer_environment_ready=0"
    )) {
    Assert-LinePresent $planLines $expectedLine "D3D12 host supplement producer Plan mode"
}

Write-Information "renderer-d3d12-commercial-quality-host-supplement-check: ok" -InformationAction Continue
