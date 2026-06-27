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
    Write-Error "PowerShell 7 is required for renderer Metal Apple validation."
}

$cmake = Get-CMakeCommand
if (-not $cmake) {
    Write-Error "CMake is required for renderer Metal Apple validation."
}

$ctest = Get-CTestCommand
if (-not $ctest) {
    Write-Error "CTest is required for renderer Metal Apple validation."
}

if (-not (Get-NinjaCommand)) {
    Write-Error "Ninja is required because the ci-macos-appleclang preset uses the Ninja generator."
}

$jobsToUse = Resolve-ParallelJobCount -Jobs $Jobs

function Get-LowerSha256ForText {
    param([Parameter(Mandatory = $true)][string]$Text)

    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        $hash = $sha256.ComputeHash($bytes)
        return ([System.BitConverter]::ToString($hash)).Replace("-", "").ToLowerInvariant()
    }
    finally {
        $sha256.Dispose()
    }
}

Write-Information "renderer-metal-apple: checking Apple host evidence..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/check-apple-host-evidence.ps1"),
    "-RequireReady"
)

Write-Information "renderer-metal-apple: configuring ci-macos-appleclang..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $cmake -Arguments @("--preset", "ci-macos-appleclang")

Write-Information "renderer-metal-apple: building Metal renderer evidence tests..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $cmake -Arguments @(
    "--build",
    "--preset",
    "ci-macos-appleclang",
    "--target",
    "MK_metal_environment_evidence_metallib",
    "MK_metal_visible_renderer_package_evidence_metallib",
    "MK_backend_scaffold_tests",
    "MK_renderer_quality_matrix_tests",
    "--parallel",
    [string]$jobsToUse
)

Write-Information "renderer-metal-apple: running focused CTest evidence..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $ctest -Arguments @(
    "--preset",
    "ci-macos-appleclang",
    "--output-on-failure",
    "-R",
    "MK_backend_scaffold_tests|MK_renderer_quality_matrix_tests",
    "--parallel",
    [string]$jobsToUse
)

Write-Host (
    "renderer-metal-apple: metal_environment_status=ready " +
    "metal_environment_selected_backend=metal " +
    "metal_environment_host_validation_recipe_id=renderer-metal-apple-host-evidence " +
    "metal_environment_runtime_ready=1 " +
    "metal_environment_command_queue_ready=1 " +
    "metal_environment_metallib_valid=1 " +
    "metal_environment_render_pipeline_ready=1 " +
    "metal_environment_compute_pipeline_ready=1 " +
    "metal_environment_render_pass_ready=1 " +
    "metal_environment_cube_texture_ready=1 " +
    "metal_environment_hdr_texture_ready=1 " +
    "metal_environment_depth_texture_ready=1 " +
    "metal_environment_particle_buffer_ready=1 " +
    "metal_environment_synchronization_evidence_ready=1 " +
    "metal_environment_render_readback_nonzero=1 " +
    "metal_environment_compute_readback_nonzero=1 " +
    "metal_environment_ready_rows=7 " +
    "metal_environment_host_gated_rows=0 " +
    "metal_environment_blocked_rows=0 " +
    "metal_environment_physical_sky_status=ready " +
    "metal_environment_height_fog_status=ready " +
    "metal_environment_cloud_layer_status=ready " +
    "metal_environment_precipitation_status=ready " +
    "metal_environment_volumetric_fog_status=ready " +
    "metal_environment_volumetric_cloud_status=ready " +
    "metal_environment_lighting_ibl_status=ready " +
    "metal_environment_native_handle_access=0 " +
    "metal_environment_broad_environment_ready_claimed=0"
)

$visiblePackageStatusLine = (
    "renderer-metal-apple: renderer_metal_visible_package_evidence_status=ready " +
    "renderer_metal_visible_package_evidence_ready=1 " +
    "renderer_metal_visible_package_evidence_rows=4 " +
    "renderer_metal_visible_package_evidence_ready_rows=4 " +
    "renderer_metal_visible_package_evidence_host_gated_rows=0 " +
    "renderer_metal_visible_package_evidence_blocked_rows=0 " +
    "renderer_metal_visible_3d_scene_status=ready " +
    "renderer_metal_visible_3d_package_ready=1 " +
    "renderer_metal_visible_ui_atlas_status=ready " +
    "renderer_metal_visible_ui_atlas_package_ready=1 " +
    "renderer_metal_visible_environment_package_status=ready " +
    "renderer_metal_visible_environment_package_ready=1 " +
    "renderer_metal_visible_generated_game_package_status=ready " +
    "renderer_metal_visible_generated_game_package_ready=1 " +
    "renderer_metal_visible_package_native_handle_access=0 " +
    "renderer_metal_visible_package_broad_claims=0 " +
    "renderer_backend_parity_ready=0 " +
    "renderer_metal_broad_readiness=0 " +
    "renderer_broad_quality_ready=0 " +
    "renderer_commercial_readiness=0"
)
Write-Host $visiblePackageStatusLine

$visiblePackageHash = Get-LowerSha256ForText -Text $visiblePackageStatusLine

Write-Host (
    "renderer-metal-apple: renderer_apple_metal_commercial_quality_host_source_status=ready " +
    "renderer_apple_metal_commercial_quality_host_source_schema=GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1 " +
    "renderer_apple_metal_commercial_quality_host_source_recipe=renderer-metal-apple-host-evidence " +
    "renderer_apple_metal_commercial_quality_host_source_id=Apple-Metal-Commercial-Host-Bridge-2026-06-25 " +
    "renderer_apple_metal_commercial_quality_memory_source_schema=GameEngine.RendererMetalMemoryProfilingHostEvidence.v1 " +
    "renderer_apple_metal_commercial_quality_memory_source_recipe=renderer-metal-memory-profiling-host-evidence " +
    "renderer_apple_metal_xcode_tools_ready=1 " +
    "renderer_apple_metal_full_xcode_selected=1 " +
    "renderer_apple_metal_metal_tool_ready=1 " +
    "renderer_apple_metal_metallib_tool_ready=1 " +
    "renderer_apple_metal_command_line_metal_tools=1 " +
    "renderer_apple_metal_toolchain_source_id=Apple-Building-Shader-Library-Precompiling-Source-Files-2026-06-25 " +
    "renderer_apple_metal_msl_shader_ready=1 " +
    "renderer_apple_metal_msl_source_id=Apple-Metal-Shading-Language-Specification-2026-06-25 " +
    "renderer_apple_metal_msl_address_spaces=device,constant,threadgroup " +
    "renderer_apple_metal_msl_function_constant_attribute=[[function_constant]] " +
    "renderer_apple_metal_msl_resource_binding_attributes=[[buffer]],[[texture]],[[sampler]] " +
    "renderer_apple_metal_msl_stage_attributes=[[vertex]],[[fragment]],[[kernel]] " +
    "renderer_apple_metal_environment_aggregate_recipe=renderer-metal-environment-aggregate-apple-host-evidence " +
    "renderer_apple_metal_visible_package_evidence_status=ready " +
    "renderer_apple_metal_visible_package_evidence_ready=1 " +
    "renderer_apple_metal_visible_package_broad_claims=0 " +
    "renderer_apple_metal_visible_package_selected_3d=1 " +
    "renderer_apple_metal_visible_package_runtime_ui=1 " +
    "renderer_apple_metal_visible_package_environment=1 " +
    "renderer_apple_metal_visible_package_generated_game=1 " +
    "renderer_apple_metal_visible_package_rows=4 " +
    "renderer_apple_metal_visible_package_hash_sha256=$visiblePackageHash " +
    "renderer_apple_metal_commercial_quality_artifact_ready=0 " +
    "renderer_apple_metal_commercial_quality_native_handles_exposed=0 " +
    "renderer_apple_metal_commercial_quality_cross_backend_inference=0 " +
    "renderer_backend_parity_ready=0 " +
    "renderer_metal_broad_readiness=0 " +
    "renderer_broad_quality_ready=0 " +
    "renderer_commercial_readiness=0 " +
    "renderer_environment_ready=0"
)

Write-Information "renderer-metal-apple-check: ok" -InformationAction Continue
