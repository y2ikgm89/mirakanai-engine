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
    Write-Error "PowerShell 7 is required for MAVG deformation integration validation."
}

Write-Information "mavg-deformation-integration: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--preset",
    "dev"
)

Write-Information "mavg-deformation-integration: building focused runtime scene RHI test target..." `
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
    "MK_runtime_scene_rhi_mavg_deformation_integration_tests"
)

Write-Information "mavg-deformation-integration: running focused runtime scene RHI CTest lane..." `
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
    "MK_runtime_scene_rhi_mavg_deformation_integration_tests"
)

$policyReady = $true
$integrationReady = $false
$status = "backend_execution_required"

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-deformation-integration")
$lines.Add("mavg_deformation_policy_ready=$(ConvertTo-CounterBit $policyReady)")
$lines.Add("mavg_deformation_integration_status=$status")
$lines.Add("mavg_deformation_integration_ready=$(ConvertTo-CounterBit $integrationReady)")
$lines.Add("mavg_deformation_reviewed_cluster_rows=3")
$lines.Add("mavg_deformation_policy_ready_cluster_rows=3")
$lines.Add("mavg_deformation_backend_execution_rows=0")
$lines.Add("mavg_deformation_backend_execution_ready_rows=0")
$lines.Add("mavg_deformation_backend_execution_status=host_evidence_required")
$lines.Add("mavg_deformation_topology_changing_rows=0")
$lines.Add("mavg_deformation_runtime_generated_triangle_topology=0")
$lines.Add("mavg_deformation_unbounded_vertex_displacement=0")
$lines.Add("mavg_deformation_native_handles_exposed=0")
$lines.Add("mavg_deformation_ray_tracing_integration=0")
$lines.Add("mavg_deformation_mesh_shader_execution=0")
$lines.Add("mavg_deformation_metal_readiness=0")
$lines.Add("mavg_deformation_nanite_equivalence=0")
$lines.Add("mavg_deformation_broad_readiness=0")
$lines.Add("mavg_deformation_broad_backend_readiness=0")
$lines.Add("mavg_deformation_broad_optimization=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $integrationReady) {
    Write-Error "MAVG deformation integration is incomplete; selected backend execution evidence rows are required before mavg_deformation_integration_ready can be 1."
}

Write-Information "mavg-deformation-integration-check: ok" -InformationAction Continue
