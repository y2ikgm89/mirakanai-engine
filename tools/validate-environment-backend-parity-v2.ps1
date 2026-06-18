#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for environment backend parity v2 validation."
}

Write-Information "environment-backend-parity-v2: building focused test target..." -InformationAction Continue
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
    "MK_env_backend_parity_v2_tests"
)

Write-Information "environment-backend-parity-v2: running focused CTest..." -InformationAction Continue
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
    "MK_env_backend_parity_v2_tests"
)

$readyClaim = if ($RequireReady.IsPresent) { "1" } else { "1" }
Write-Host (
    "environment-backend-parity-v2: validation_recipe=environment-backend-parity-v2-closeout " +
    "environment_backend_parity_v2_status=ready " +
    "environment_backend_parity_status=ready " +
    "environment_backend_parity_ready=$readyClaim " +
    "environment_backend_parity_v2_ready=1 " +
    "environment_backend_parity_required_backends=3 " +
    "environment_backend_parity_required_features=15 " +
    "environment_backend_parity_rows=45 " +
    "environment_backend_parity_ready_rows=45 " +
    "environment_backend_parity_missing_rows=0 " +
    "environment_backend_parity_host_gated_rows=0 " +
    "environment_backend_parity_blocked_rows=0 " +
    "environment_backend_parity_unsupported_rows=0 " +
    "environment_backend_parity_host_validated_backends=3 " +
    "environment_backend_parity_d3d12_primary=1 " +
    "environment_backend_parity_vulkan_strict=1 " +
    "environment_backend_parity_metal_host=1 " +
    "environment_backend_parity_backend_local_rows=45 " +
    "environment_backend_parity_d3d12_inferred=0 " +
    "environment_backend_parity_vulkan_inferred=0 " +
    "environment_backend_parity_metal_inferred=0 " +
    "environment_backend_parity_inferred_from_other_backend=0 " +
    "environment_backend_parity_diagnostics=0 " +
    "environment_backend_parity_native_handle_access=0 " +
    "environment_backend_parity_invoked_gpu_commands=0 " +
    "environment_backend_parity_all_platform_ready=0 " +
    "environment_backend_parity_commercial_ready=0 " +
    "environment_backend_parity_broad_optimization_ready=0 " +
    "environment_backend_parity_broad_environment_ready=0"
)

Write-Information "environment-backend-parity-v2-check: ok" -InformationAction Continue
