# SPDX-FileCopyrightText: 2026 GameEngine contributors
# SPDX-License-Identifier: LicenseRef-Proprietary
#
# Recompiles tests/shaders/vulkan_shadow_receiver.hlsl to SPIR-V for the
# backend_scaffold_tests Vulkan shadow-receiver readback proof. After a HLSL
# change, set MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV and
# MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV to the emitted file paths (or copy
# the artifacts into your CI secret store).

#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$source = Join-Path $root "tests/shaders/vulkan_shadow_receiver.hlsl"
if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Missing shader source: $source"
}

if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $root "out\vulkan-shadow-receiver-test-artifacts"
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$dxc = Find-CommandOnCombinedPath "dxc"
if ([string]::IsNullOrWhiteSpace($dxc)) {
    $dxc = Get-FirstExistingFile (Get-WindowsSdkDxcCandidates)
}

if ([string]::IsNullOrWhiteSpace($dxc)) {
    throw "dxc not found on PATH or under Windows Kits 10\bin\<version>\x64. Install the Windows SDK or Vulkan SDK DXC."
}

$vsOut = Join-Path $OutDir "vulkan_shadow_receiver.vs.spv"
$fsOut = Join-Path $OutDir "vulkan_shadow_receiver.fs.spv"
if (Test-Path -LiteralPath $vsOut -PathType Leaf) { Remove-Item -LiteralPath $vsOut -Force }
if (Test-Path -LiteralPath $fsOut -PathType Leaf) { Remove-Item -LiteralPath $fsOut -Force }

$commonArgs = @("-spirv", "-fspv-target-env=vulkan1.3", "-HV", "2021")
$vsArgs = $commonArgs + @("-T", "vs_6_7", "-E", "main", "-D", "MK_VULKAN_SHADOW_RECEIVER_VERTEX", "-Fo", $vsOut, $source)
$fsArgs = $commonArgs + @("-T", "ps_6_7", "-E", "main", "-D", "MK_VULKAN_SHADOW_RECEIVER_FRAGMENT", "-Fo", $fsOut, $source)

Write-Host "dxc: $dxc"
& $dxc @vsArgs
if ($LASTEXITCODE -ne 0) { throw "DXC vertex SPIR-V compile failed (exit $LASTEXITCODE)." }
& $dxc @fsArgs
if ($LASTEXITCODE -ne 0) { throw "DXC fragment SPIR-V compile failed (exit $LASTEXITCODE)." }

$spirvVal = Find-CommandOnCombinedPath "spirv-val"
if (-not [string]::IsNullOrWhiteSpace($spirvVal)) {
    & $spirvVal $vsOut
    if ($LASTEXITCODE -ne 0) { throw "spirv-val rejected vertex SPIR-V (exit $LASTEXITCODE)." }
    & $spirvVal $fsOut
    if ($LASTEXITCODE -ne 0) { throw "spirv-val rejected fragment SPIR-V (exit $LASTEXITCODE)." }
    Write-Host "spirv-val: OK"
} else {
    Write-Host "spirv-val: not on PATH (skipped)"
}

Write-Host ""
Write-Host "Wrote:"
Write-Host "  $vsOut"
Write-Host "  $fsOut"
Write-Host ""
Write-Host "Example (PowerShell, session scope):"
Write-Host ('  $env:MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV = "' + $vsOut + '"')
Write-Host ('  $env:MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV = "' + $fsOut + '"')

