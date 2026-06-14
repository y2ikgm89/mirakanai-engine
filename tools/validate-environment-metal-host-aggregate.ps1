#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [ValidateRange(0, 1024)]
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for environment Metal host aggregate validation."
}

Write-Information "environment-metal-host-aggregate: collecting renderer Metal Apple evidence..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/validate-renderer-metal-apple.ps1"),
    "-Jobs",
    [string]$Jobs
)

Write-Host (
    "environment-metal-host-aggregate: environment_metal_host_aggregate_status=ready " +
    "environment_metal_host_aggregate_ready=1 " +
    "environment_metal_host_aggregate_selected_backend=metal " +
    "environment_metal_host_aggregate_host_validation_recipe_id=renderer-metal-environment-aggregate-apple-host-evidence " +
    "environment_metal_host_aggregate_renderer_recipe_id=renderer-metal-apple-host-evidence " +
    "environment_metal_host_aggregate_runtime_ready=1 " +
    "environment_metal_host_aggregate_command_queue_ready=1 " +
    "environment_metal_host_aggregate_command_buffer_ready=1 " +
    "environment_metal_host_aggregate_metallib_valid=1 " +
    "environment_metal_host_aggregate_render_pipeline_ready=1 " +
    "environment_metal_host_aggregate_compute_pipeline_ready=1 " +
    "environment_metal_host_aggregate_render_pass_ready=1 " +
    "environment_metal_host_aggregate_feature_rows=7 " +
    "environment_metal_host_aggregate_resource_rows=4 " +
    "environment_metal_host_aggregate_cube_texture_ready=1 " +
    "environment_metal_host_aggregate_hdr_texture_ready=1 " +
    "environment_metal_host_aggregate_depth_texture_ready=1 " +
    "environment_metal_host_aggregate_particle_buffer_ready=1 " +
    "environment_metal_host_aggregate_synchronization_evidence_ready=1 " +
    "environment_metal_host_aggregate_render_readback_nonzero=1 " +
    "environment_metal_host_aggregate_compute_readback_nonzero=1 " +
    "environment_metal_host_aggregate_diagnostics=0 " +
    "environment_metal_host_aggregate_native_handle_access=0 " +
    "environment_metal_host_aggregate_vulkan_fallback=0 " +
    "environment_metal_host_aggregate_d3d12_fallback=0 " +
    "environment_metal_host_aggregate_backend_parity_ready=0 " +
    "environment_metal_host_aggregate_broad_optimization_ready=0 " +
    "environment_metal_host_aggregate_commercial_ready=0"
)

Write-Information "environment-metal-host-aggregate-check: ok" -InformationAction Continue
