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
    if ($Value) { return "1" }
    return "0"
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

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG DirectStorage GPU decompression policy validation."
}

Write-Information "mavg-directstorage-gpu-decompression-policy: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/cmake.ps1"), "--preset", "dev"
)

Write-Information "mavg-directstorage-gpu-decompression-policy: building focused runtime RHI test target..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/cmake.ps1"), "--build",
    "--preset", "dev", "--target", "MK_runtime_rhi_mavg_ds_gpu_decomp_policy_tests"
)

Write-Information "mavg-directstorage-gpu-decompression-policy: running focused runtime RHI CTest lane..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/ctest.ps1"), "--preset",
    "dev", "--output-on-failure", "-R", "MK_runtime_rhi_mavg_ds_gpu_decomp_policy_tests"
)

$schemaText = Get-Content -LiteralPath (Join-Path $root "schemas/mavg-directstorage-gpu-decompression-policy.schema.json") -Raw
foreach ($needle in @(
        "GameEngine.MavgDirectStorageGpuDecompressionPolicy.v1",
        "mavg-gpu-directstorage-gdeflate-policy-v1",
        "microsoft-directstorage-api-downloads",
        "microsoft-directstorage-1.3-release",
        "microsoft-directstorage-developer-guidance",
        "microsoft-directstorage-1.4-zstd-preview",
        "d3d12_multiple_subresources_range",
        "gdeflate",
        "zstd_preview",
        "gpuDestinationExecutionReady",
        "gdeflateExecutionReady",
        "zstdPreviewReady",
        "packageVisibleBackendReadinessReady",
        "broadCpuGpuMemoryOptimizationReady",
        "unityUnrealGodotCompatibility"
    )) {
    Assert-TextContains -Text $schemaText -Needle $needle -Context "mavg DirectStorage GPU decompression schema"
}

$docPaths = @(
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/superpowers/plans/README.md",
    "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md",
    "engine/agent/manifest.json"
)
foreach ($docPath in $docPaths) {
    $docText = Get-Content -LiteralPath (Join-Path $root $docPath) -Raw
    foreach ($needle in @(
            "mavg-gpu-directstorage-gdeflate-policy-v1",
            "mavg_directstorage_gpu_decompression_policy_ready=1",
            "mavg_directstorage_gpu_destination_execution_ready=0",
            "mavg_directstorage_gdeflate_execution_ready=0",
            "mavg_directstorage_zstd_preview_ready=0",
            "mavg_package_visible_backend_readiness_ready=0",
            "mavg_directstorage_performance_ready=0",
            "mavg_broad_cpu_gpu_memory_optimization_ready=0",
            "mavg_nanite_compatible=0",
            "mavg_nanite_equivalent=0",
            "mavg_nanite_superior=0",
            "RuntimeMavgDirectStorageGpuDecompressionPolicyResult",
            "evaluate_runtime_mavg_directstorage_gpu_decompression_policy",
            "DirectStorage 1.4/Zstd",
            "Unity",
            "Unreal",
            "Godot",
            "Nanite"
        )) {
        Assert-TextContains -Text $docText -Needle $needle -Context $docPath
    }
}

$ready = $true
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-directstorage-gpu-decompression-policy")
$lines.Add("mavg_directstorage_gpu_decompression_policy_status=policy_ready")
$lines.Add("mavg_directstorage_gpu_decompression_policy_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_directstorage_gpu_destination_policy_ready=1")
$lines.Add("mavg_directstorage_gdeflate_policy_ready=1")
$lines.Add("mavg_directstorage_gpu_destination_execution_ready=0")
$lines.Add("mavg_directstorage_gdeflate_execution_ready=0")
$lines.Add("mavg_directstorage_zstd_preview_ready=0")
$lines.Add("mavg_directstorage_zstd_preview_selected=0")
$lines.Add("mavg_directstorage_native_handles_exposed=0")
$lines.Add("mavg_directstorage_performance_ready=0")
$lines.Add("mavg_package_visible_backend_readiness_ready=0")
$lines.Add("mavg_broad_cpu_gpu_memory_optimization_ready=0")
$lines.Add("mavg_nanite_compatible=0")
$lines.Add("mavg_nanite_equivalent=0")
$lines.Add("mavg_nanite_superior=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG DirectStorage GPU decompression policy is incomplete; focused tests, schema, docs, and manifest non-claim evidence are required."
}

Write-Information "mavg-directstorage-gpu-decompression-policy-check: ok" -InformationAction Continue
