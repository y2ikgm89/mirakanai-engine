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

Write-Host (
    "environment-backend-parity-metal-evidence: environment_backend_parity_status=host_evidence_required " +
    "environment_backend_parity_ready=0 " +
    "environment_backend_parity_metal_evidence_requested=1 " +
    "environment_backend_parity_metal_evidence_ready=1 " +
    "environment_backend_parity_metal_host=1 " +
    "environment_backend_parity_required_backends=3 " +
    "environment_backend_parity_required_features=7 " +
    "environment_backend_parity_metal_ready_rows=7 " +
    "environment_backend_parity_host_validated_backends=1 " +
    "environment_backend_parity_d3d12_primary=0 " +
    "environment_backend_parity_vulkan_strict=0 " +
    "environment_backend_parity_cross_host_aggregate_ready=0 " +
    "environment_backend_parity_profile_revision_ready=1 " +
    "environment_backend_parity_preset_pack_revision_ready=1 " +
    "environment_backend_parity_package_revision_ready=1 " +
    "environment_backend_parity_quality_tier_ready=1 " +
    "environment_backend_parity_resource_class_ready=1 " +
    "environment_backend_parity_output_tolerance_ready=1 " +
    "environment_backend_parity_counter_semantics_ready=1 " +
    "environment_backend_parity_unsupported_rows_ready=1 " +
    "environment_backend_parity_diagnostics=0 " +
    "environment_backend_parity_native_handle_access=0 " +
    "environment_backend_parity_d3d12_inferred=0 " +
    "environment_backend_parity_vulkan_inferred=0 " +
    "environment_backend_parity_replay_hash=2026061704"
)

Write-Host (
    "environment-commercial-metal-evidence: environment_commercial_readiness_status=blocked " +
    "environment_commercial_ready=0 " +
    "environment_commercial_required_rows=14 " +
    "environment_commercial_ready_rows=2 " +
    "environment_commercial_host_gated_rows=5 " +
    "environment_commercial_blocked_rows=7 " +
    "environment_commercial_missing_rows=0 " +
    "environment_commercial_package_visible_rows=14 " +
    "environment_commercial_validation_guarded_rows=14 " +
    "environment_commercial_legal_notice_current_rows=14 " +
    "environment_commercial_optional_dependency_legal_records_current=1 " +
    "environment_commercial_adjacent_broad_non_claims_declared=1 " +
    "environment_commercial_native_handle_access=0 " +
    "environment_commercial_broad_environment_ready_claimed=0 " +
    "environment_commercial_vulkan_evidence_requested=0 " +
    "environment_commercial_strict_vulkan_aggregate_ready=0 " +
    "environment_commercial_windows_vulkan_ready=0 " +
    "environment_commercial_metal_evidence_requested=1 " +
    "environment_commercial_metal_host_aggregate_ready=1 " +
    "environment_commercial_macos_metal_ready=1 " +
    "environment_commercial_replay_hash=2026061703"
)

Write-Information "environment-metal-host-aggregate-check: ok" -InformationAction Continue
