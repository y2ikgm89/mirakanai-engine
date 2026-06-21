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

$tools = Assert-CppBuildTools

$mode = if ($RequireReady.IsPresent) { "ready" } else { "default" }
Write-Information "2d-sprite-throughput: running $mode validation lane" -InformationAction Continue

Invoke-CheckedCommand $tools.CMake --preset dev
Invoke-CheckedCommand $tools.CMake --build --preset dev --target `
    MK_renderer_2d_sprite_throughput_tests `
    MK_runtime_entity_scale_culling_tests
Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R `
    "MK_renderer_2d_sprite_throughput_tests|MK_runtime_entity_scale_culling_tests"

$smokeArgs = @(
    "--smoke",
    "--max-frames",
    "3",
    "--require-config",
    "runtime/sample_2d_desktop_runtime_package.config",
    "--require-scene-package",
    "runtime/sample_2d_desktop_runtime_package.geindex",
    "--require-win32-runtime-host",
    "--require-d3d12-shaders",
    "--require-d3d12-renderer",
    "--require-2d-sprite-throughput"
)

& (Join-Path $PSScriptRoot "package-desktop-runtime.ps1") `
    -GameTarget "sample_2d_desktop_runtime_package" `
    -RequireD3d12Shaders `
    -SmokeArgs $smokeArgs

Write-Information "2d-sprite-throughput: ok" -InformationAction Continue
