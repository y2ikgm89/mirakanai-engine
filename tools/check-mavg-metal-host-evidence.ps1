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
. (Join-Path $PSScriptRoot "apple-host-helpers.ps1")

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

function Get-MavgMetalHostLabel {
    if (Test-IsMacOS) {
        return "macos"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return "windows"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Linux)) {
        return "linux"
    }
    return "unknown"
}

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG Metal host evidence validation."
}

Write-Information "mavg-metal-host-evidence: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--preset",
    "dev"
)

Write-Information "mavg-metal-host-evidence: building focused Metal MAVG mesh LOD test target..." `
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
    "MK_mavg_metal_mesh_lod_tests"
)

Write-Information "mavg-metal-host-evidence: running focused Metal MAVG mesh LOD CTest lane..." `
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
    "MK_mavg_metal_mesh_lod_tests"
)

$hostLabel = Get-MavgMetalHostLabel
$developerDirectory = Get-AppleDeveloperDirectory
$fullXcodeSelected = Test-FullXcodeDeveloperDirectory -DeveloperDirectory $developerDirectory
$xcrun = Find-CommandOnCombinedPath "xcrun"
$metalToolReady = Test-XcrunToolAvailable -Xcrun $xcrun -SdkName "macosx" -ToolName "metal"
$metallibToolReady = Test-XcrunToolAvailable -Xcrun $xcrun -SdkName "macosx" -ToolName "metallib"

$ready = $false
$hostGated = $true
$status = "host_evidence_required"

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-metal-mesh-lod-host-evidence")
$lines.Add("mavg_metal_mesh_lod_status=$status")
$lines.Add("mavg_metal_mesh_lod_host=$hostLabel")
$lines.Add("mavg_metal_mesh_lod_host_gated=$(ConvertTo-CounterBit $hostGated)")
$lines.Add("mavg_metal_mesh_lod_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_mesh_shader_lod_ready=0")
$lines.Add("mavg_metal_ray_tracing_ready=0")
$lines.Add("mavg_metal_mesh_lod_feature_set_table_source=Apple-Metal-Feature-Set-Tables-2026-05-21")
$lines.Add("mavg_metal_mesh_lod_feature_set_table_row_reviewed=1")
$lines.Add("mavg_metal_mesh_lod_full_xcode_selected=$(ConvertTo-CounterBit $fullXcodeSelected)")
$lines.Add("mavg_metal_mesh_lod_metal_tool_ready=$(ConvertTo-CounterBit $metalToolReady)")
$lines.Add("mavg_metal_mesh_lod_metallib_tool_ready=$(ConvertTo-CounterBit $metallibToolReady)")
$lines.Add("mavg_metal_mesh_lod_apple_gpu_family_rows=0")
$lines.Add("mavg_metal_mesh_lod_mesh_shader_feature_rows=0")
$lines.Add("mavg_metal_mesh_lod_object_shader_feature_rows=0")
$lines.Add("mavg_metal_mesh_lod_first_party_workload_rows=0")
$lines.Add("mavg_metal_mesh_lod_object_mesh_pipeline_rows=0")
$lines.Add("mavg_metal_mesh_lod_object_mesh_dispatch_rows=0")
$lines.Add("mavg_metal_mesh_lod_readback_hash_rows=0")
$lines.Add("mavg_metal_mesh_lod_package_visible_output_rows=0")
$lines.Add("mavg_metal_mesh_lod_simulator_only_evidence=0")
$lines.Add("mavg_metal_mesh_lod_cross_backend_inference=0")
$lines.Add("mavg_metal_mesh_lod_ray_tracing_pipeline_conflation=0")
$lines.Add("mavg_metal_mesh_lod_native_handles_exposed=0")
$lines.Add("mavg_metal_mesh_lod_nanite_compatible=0")
$lines.Add("mavg_metal_mesh_lod_nanite_equivalent=0")
$lines.Add("mavg_metal_mesh_lod_nanite_superior=0")
$lines.Add("mavg_metal_mesh_lod_broad_backend_readiness=0")
$lines.Add("mavg_metal_mesh_lod_broad_optimization=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG Metal mesh LOD readiness is incomplete; Apple-host object/mesh shader execution, readback hash, and package-visible output evidence are required before mavg_metal_mesh_lod_ready can be 1."
}

Write-Information "mavg-metal-host-evidence-check: ok" -InformationAction Continue
