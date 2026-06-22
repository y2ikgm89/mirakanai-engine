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

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG broad optimization validation."
}

Write-Information "mavg-broad-optimization: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/cmake.ps1"), "--preset", "dev"
)

Write-Information "mavg-broad-optimization: building focused runtime RHI test target..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/cmake.ps1"), "--build",
    "--preset", "dev", "--target", "MK_runtime_rhi_mavg_broad_optimization_evidence_tests"
)

Write-Information "mavg-broad-optimization: running focused runtime RHI CTest lane..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/ctest.ps1"), "--preset",
    "dev", "--output-on-failure", "-R", "MK_runtime_rhi_mavg_broad_optimization_evidence_tests"
)

$schemaText = Get-Content -LiteralPath (Join-Path $root "schemas/mavg-broad-optimization-artifacts.schema.json") -Raw
foreach ($needle in @(
        "GameEngine.MavgBroadOptimizationArtifacts.v1",
        "mavg-broad-optimization-evidence-v1",
        "pix_timing_capture",
        "nsight_graphics_gpu_trace",
        "radeon_gpu_profiler",
        "intel_gpa",
        "apple_metal_tools",
        "profiler_tool_support_source_id",
        "profiler_capture_overhead_basis_points",
        "cpu_frame_p95_us",
        "gpu_frame_p95_us",
        "vram_peak_bytes",
        "claims_broad_backend_readiness"
    )) {
    if (-not $schemaText.Contains($needle)) {
        Write-Error "MAVG broad optimization schema must contain '$needle'."
    }
}

$ready = $false
$status = "host_evidence_required"
$requiredRows = 14
$retainedRows = 0

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-broad-optimization")
$lines.Add("mavg_broad_cpu_gpu_memory_optimization_status=$status")
$lines.Add("mavg_broad_cpu_gpu_memory_optimization_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_broad_optimization_required_rows=$requiredRows")
$lines.Add("mavg_broad_optimization_retained_rows=$retainedRows")
$lines.Add("mavg_broad_optimization_ready_rows=0")
$lines.Add("mavg_broad_optimization_d3d12_nvidia_rows=0")
$lines.Add("mavg_broad_optimization_d3d12_amd_rows=0")
$lines.Add("mavg_broad_optimization_d3d12_intel_rows=0")
$lines.Add("mavg_broad_optimization_vulkan_nvidia_rows=0")
$lines.Add("mavg_broad_optimization_vulkan_amd_rows=0")
$lines.Add("mavg_broad_optimization_vulkan_intel_rows=0")
$lines.Add("mavg_broad_optimization_metal_apple_rows=0")
$lines.Add("mavg_broad_optimization_mavg_stress_package_rows=0")
$lines.Add("mavg_broad_optimization_gameplay_package_rows=0")
$lines.Add("mavg_broad_optimization_internal_counter_rows=0")
$lines.Add("mavg_broad_optimization_official_profiler_rows=0")
$lines.Add("mavg_broad_optimization_cpu_metric_rows=0")
$lines.Add("mavg_broad_optimization_gpu_metric_rows=0")
$lines.Add("mavg_broad_optimization_memory_metric_rows=0")
$lines.Add("mavg_broad_optimization_io_metric_rows=0")
$lines.Add("mavg_broad_optimization_profiler_timeline_rows=0")
$lines.Add("mavg_broad_optimization_eol_or_unsupported_profiler_rows=0")
$lines.Add("mavg_broad_optimization_profiler_backend_vendor_mismatch_rows=0")
$lines.Add("mavg_broad_optimization_intel_gpa_eol=1")
$lines.Add("mavg_broad_optimization_intel_vulkan_supported_profiler_rows=0")
$lines.Add("mavg_broad_optimization_synthetic_only_rows=0")
$lines.Add("mavg_broad_optimization_native_handles_exposed=0")
$lines.Add("mavg_broad_optimization_package_visible_backend_readiness=0")
$lines.Add("mavg_broad_optimization_environment_ready=0")
$lines.Add("mavg_broad_optimization_renderer_ready=0")
$lines.Add("mavg_broad_optimization_nanite_compatible=0")
$lines.Add("mavg_broad_optimization_nanite_equivalent=0")
$lines.Add("mavg_broad_optimization_nanite_superior=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG broad CPU/GPU/memory optimization is incomplete; 14 retained official-profiler rows across D3D12/Vulkan/Metal and NVIDIA/AMD/Intel/Apple are required before mavg_broad_cpu_gpu_memory_optimization_ready can be 1."
}

Write-Information "mavg-broad-optimization-check: ok" -InformationAction Continue
