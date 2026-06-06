#requires -Version 7.0
#requires -PSEdition Core

# SPDX-FileCopyrightText: 2026 GameEngine contributors
# SPDX-License-Identifier: LicenseRef-Proprietary
#
# Recompiles the Vulkan physical-sky test shaders to SPIR-V for the
# backend_scaffold_tests physical-sky readback proof. After a HLSL change, set
# the printed MK_VULKAN_TEST_*_SPV variables before running the configured
# Vulkan test lane.

param(
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$skySource = Join-Path $root "tests/shaders/environment_physical_sky.hlsl"
if (-not (Test-Path -LiteralPath $skySource -PathType Leaf)) {
    throw "Missing shader source: $skySource"
}

if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $root "out\vulkan-physical-sky-test-artifacts"
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$dxc = Find-CommandOnCombinedPath "dxc"
if ([string]::IsNullOrWhiteSpace($dxc)) {
    $dxc = Get-FirstExistingFile (Get-WindowsSdkDxcCandidates)
}

if ([string]::IsNullOrWhiteSpace($dxc)) {
    throw "dxc not found on PATH or under Windows Kits 10\bin\<version>\x64. Install the Windows SDK or Vulkan SDK DXC."
}

$skyVsOut = Join-Path $OutDir "environment_physical_sky.vs.spv"
$skyFsOut = Join-Path $OutDir "environment_physical_sky.fs.spv"

foreach ($artifact in @($skyVsOut, $skyFsOut)) {
    if (Test-Path -LiteralPath $artifact -PathType Leaf) {
        Remove-Item -LiteralPath $artifact -Force
    }
}

$commonArgs = @("-spirv", "-fspv-target-env=vulkan1.3", "-HV", "2021")
$skyVsArgs = $commonArgs + @(
    "-T", "vs_6_7",
    "-E", "vs_main",
    "-D", "MK_PHYSICAL_SKY_VULKAN_BINDINGS",
    "-Fo", $skyVsOut,
    $skySource
)
$skyFsArgs = $commonArgs + @(
    "-T", "ps_6_7",
    "-E", "ps_main",
    "-D", "MK_PHYSICAL_SKY_VULKAN_BINDINGS",
    "-Fo", $skyFsOut,
    $skySource
)

Write-Information "dxc: $dxc" -InformationAction Continue
& $dxc @skyVsArgs
if ($LASTEXITCODE -ne 0) { throw "DXC physical-sky vertex SPIR-V compile failed (exit $LASTEXITCODE)." }
& $dxc @skyFsArgs
if ($LASTEXITCODE -ne 0) { throw "DXC physical-sky fragment SPIR-V compile failed (exit $LASTEXITCODE)." }

$spirvVal = Find-CommandOnCombinedPath "spirv-val"
if (-not [string]::IsNullOrWhiteSpace($spirvVal)) {
    foreach ($artifact in @($skyVsOut, $skyFsOut)) {
        & $spirvVal --target-env vulkan1.3 $artifact
        if ($LASTEXITCODE -ne 0) { throw "spirv-val rejected SPIR-V artifact $artifact (exit $LASTEXITCODE)." }
    }
    Write-Information "spirv-val: OK" -InformationAction Continue
} else {
    Write-Information "spirv-val: not on PATH (skipped)" -InformationAction Continue
}

Write-Information "" -InformationAction Continue
Write-Information "Wrote:" -InformationAction Continue
Write-Information "  $skyVsOut" -InformationAction Continue
Write-Information "  $skyFsOut" -InformationAction Continue
Write-Information "" -InformationAction Continue
Write-Information "Example (PowerShell, session scope):" -InformationAction Continue
Write-Information ('  $env:MK_VULKAN_TEST_PHYSICAL_SKY_VERTEX_SPV = "' + $skyVsOut + '"') -InformationAction Continue
Write-Information ('  $env:MK_VULKAN_TEST_PHYSICAL_SKY_FRAGMENT_SPV = "' + $skyFsOut + '"') -InformationAction Continue
