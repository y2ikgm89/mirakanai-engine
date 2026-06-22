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

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG ray tracing integration validation."
}

Write-Information "mavg-ray-tracing-integration: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--preset",
    "dev"
)

Write-Information "mavg-ray-tracing-integration: building focused runtime scene RHI test target..." `
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
    "MK_runtime_scene_rhi_mavg_ray_tracing_integration_tests"
)

Write-Information "mavg-ray-tracing-integration: running focused runtime scene RHI CTest lane..." `
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
    "MK_runtime_scene_rhi_mavg_ray_tracing_integration_tests"
)

$policyReady = $true
$integrationReady = $false
$status = "backend_acceleration_structure_build_required"

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-ray-tracing-integration")
$lines.Add("mavg_ray_tracing_policy_ready=$(ConvertTo-CounterBit $policyReady)")
$lines.Add("mavg_ray_tracing_integration_status=$status")
$lines.Add("mavg_ray_tracing_integration_ready=$(ConvertTo-CounterBit $integrationReady)")
$lines.Add("mavg_ray_tracing_reviewed_blas_input_rows=2")
$lines.Add("mavg_ray_tracing_policy_ready_blas_input_rows=2")
$lines.Add("mavg_ray_tracing_backend_execution_rows=0")
$lines.Add("mavg_ray_tracing_backend_execution_ready_rows=0")
$lines.Add("mavg_ray_tracing_backend_execution_status=host_evidence_required")
$lines.Add("mavg_ray_tracing_d3d12_dxr_feature_rows=0")
$lines.Add("mavg_ray_tracing_d3d12_acceleration_structure_build_rows=0")
$lines.Add("mavg_ray_tracing_vulkan_acceleration_structure_feature_rows=0")
$lines.Add("mavg_ray_tracing_vulkan_acceleration_structure_build_rows=0")
$lines.Add("mavg_ray_tracing_implicit_mode_switches=0")
$lines.Add("mavg_ray_tracing_fallback_mode_mismatches=0")
$lines.Add("mavg_ray_tracing_native_handles_exposed=0")
$lines.Add("mavg_ray_tracing_metal_readiness=0")
$lines.Add("mavg_ray_tracing_mesh_shader_execution=0")
$lines.Add("mavg_ray_tracing_nanite_equivalence=0")
$lines.Add("mavg_ray_tracing_broad_readiness=0")
$lines.Add("mavg_ray_tracing_broad_backend_readiness=0")
$lines.Add("mavg_ray_tracing_broad_optimization=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $integrationReady) {
    Write-Error "MAVG ray tracing integration is incomplete; selected backend acceleration-structure build evidence rows are required before mavg_ray_tracing_integration_ready can be 1."
}

Write-Information "mavg-ray-tracing-integration-check: ok" -InformationAction Continue
