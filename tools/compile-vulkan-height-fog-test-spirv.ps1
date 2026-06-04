#requires -Version 7.0
#requires -PSEdition Core

# SPDX-FileCopyrightText: 2026 GameEngine contributors
# SPDX-License-Identifier: LicenseRef-Proprietary
#
# Recompiles the Vulkan depth-write and environment height-fog test shaders to
# SPIR-V for the backend_scaffold_tests height-fog readback proof. After a HLSL
# change, set the printed MK_VULKAN_TEST_*_SPV variables before running the
# configured Vulkan test lane.

param(
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$depthSource = Join-Path $root "tests/shaders/vulkan_depth_sampling.hlsl"
$fogSource = Join-Path $root "tests/shaders/environment_height_fog.hlsl"
if (-not (Test-Path -LiteralPath $depthSource -PathType Leaf)) {
    throw "Missing shader source: $depthSource"
}
if (-not (Test-Path -LiteralPath $fogSource -PathType Leaf)) {
    throw "Missing shader source: $fogSource"
}

if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $root "out\vulkan-height-fog-test-artifacts"
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$dxc = Find-CommandOnCombinedPath "dxc"
if ([string]::IsNullOrWhiteSpace($dxc)) {
    $dxc = Get-FirstExistingFile (Get-WindowsSdkDxcCandidates)
}

if ([string]::IsNullOrWhiteSpace($dxc)) {
    throw "dxc not found on PATH or under Windows Kits 10\bin\<version>\x64. Install the Windows SDK or Vulkan SDK DXC."
}

$depthVsOut = Join-Path $OutDir "vulkan_depth_write.vs.spv"
$depthFsOut = Join-Path $OutDir "vulkan_depth_write.fs.spv"
$fogVsOut = Join-Path $OutDir "environment_height_fog.vs.spv"
$fogFsOut = Join-Path $OutDir "environment_height_fog.fs.spv"

foreach ($artifact in @($depthVsOut, $depthFsOut, $fogVsOut, $fogFsOut)) {
    if (Test-Path -LiteralPath $artifact -PathType Leaf) {
        Remove-Item -LiteralPath $artifact -Force
    }
}

$commonArgs = @("-spirv", "-fspv-target-env=vulkan1.3", "-HV", "2021")
$depthVsArgs = $commonArgs + @(
    "-T", "vs_6_7",
    "-E", "main",
    "-D", "MK_VULKAN_DEPTH_WRITE_VERTEX",
    "-Fo", $depthVsOut,
    $depthSource
)
$depthFsArgs = $commonArgs + @(
    "-T", "ps_6_7",
    "-E", "main",
    "-D", "MK_VULKAN_DEPTH_WRITE_FRAGMENT",
    "-Fo", $depthFsOut,
    $depthSource
)
$fogVsArgs = $commonArgs + @(
    "-T", "vs_6_7",
    "-E", "vs_main",
    "-D", "MK_HEIGHT_FOG_VULKAN_BINDINGS",
    "-Fo", $fogVsOut,
    $fogSource
)
$fogFsArgs = $commonArgs + @(
    "-T", "ps_6_7",
    "-E", "ps_main",
    "-D", "MK_HEIGHT_FOG_VULKAN_BINDINGS",
    "-Fo", $fogFsOut,
    $fogSource
)

Write-Information "dxc: $dxc" -InformationAction Continue
& $dxc @depthVsArgs
if ($LASTEXITCODE -ne 0) { throw "DXC depth vertex SPIR-V compile failed (exit $LASTEXITCODE)." }
& $dxc @depthFsArgs
if ($LASTEXITCODE -ne 0) { throw "DXC depth fragment SPIR-V compile failed (exit $LASTEXITCODE)." }
& $dxc @fogVsArgs
if ($LASTEXITCODE -ne 0) { throw "DXC height-fog vertex SPIR-V compile failed (exit $LASTEXITCODE)." }
& $dxc @fogFsArgs
if ($LASTEXITCODE -ne 0) { throw "DXC height-fog fragment SPIR-V compile failed (exit $LASTEXITCODE)." }

$spirvVal = Find-CommandOnCombinedPath "spirv-val"
if (-not [string]::IsNullOrWhiteSpace($spirvVal)) {
    foreach ($artifact in @($depthVsOut, $depthFsOut, $fogVsOut, $fogFsOut)) {
        & $spirvVal --target-env vulkan1.3 $artifact
        if ($LASTEXITCODE -ne 0) { throw "spirv-val rejected SPIR-V artifact $artifact (exit $LASTEXITCODE)." }
    }
    Write-Information "spirv-val: OK" -InformationAction Continue
} else {
    Write-Information "spirv-val: not on PATH (skipped)" -InformationAction Continue
}

Write-Information "" -InformationAction Continue
Write-Information "Wrote:" -InformationAction Continue
Write-Information "  $depthVsOut" -InformationAction Continue
Write-Information "  $depthFsOut" -InformationAction Continue
Write-Information "  $fogVsOut" -InformationAction Continue
Write-Information "  $fogFsOut" -InformationAction Continue
Write-Information "" -InformationAction Continue
Write-Information "Example (PowerShell, session scope):" -InformationAction Continue
Write-Information ('  $env:MK_VULKAN_TEST_HEIGHT_FOG_DEPTH_VERTEX_SPV = "' + $depthVsOut + '"') -InformationAction Continue
Write-Information ('  $env:MK_VULKAN_TEST_HEIGHT_FOG_DEPTH_FRAGMENT_SPV = "' + $depthFsOut + '"') -InformationAction Continue
Write-Information ('  $env:MK_VULKAN_TEST_HEIGHT_FOG_VERTEX_SPV = "' + $fogVsOut + '"') -InformationAction Continue
Write-Information ('  $env:MK_VULKAN_TEST_HEIGHT_FOG_FRAGMENT_SPV = "' + $fogFsOut + '"') -InformationAction Continue
