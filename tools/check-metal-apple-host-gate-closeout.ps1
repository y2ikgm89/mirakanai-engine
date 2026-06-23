#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Assert-ContainsText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Text.Contains($Needle)) {
        Write-Error "$Context missing: $Needle"
    }
}

$manifest = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw | ConvertFrom-Json
$metalHostGate = @($manifest.aiOperableProductionLoop.hostGates | Where-Object { $_.id -eq "metal-apple" })
if ($metalHostGate.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must define exactly one metal-apple host gate."
}
if ($metalHostGate[0].status -ne "ready") {
    Write-Error "metal-apple host gate must be ready after hosted Apple Metal closeout evidence passed."
}
if ($metalHostGate[0].residualClass -ne "ready") {
    Write-Error "metal-apple residualClass must be ready after hosted Apple Metal closeout evidence passed."
}

foreach ($recipe in @(
        "shader-toolchain",
        "mobile-packaging",
        "renderer-metal-apple-host-evidence",
        "renderer-metal-environment-aggregate-apple-host-evidence",
        "environment-platform-ios-metal-package",
        "environment-weather-metal-solver-host-gate",
        "ios-simulator-smoke"
    )) {
    if (@($metalHostGate[0].validationRecipes) -notcontains $recipe) {
        Write-Error "metal-apple must reference $recipe."
    }
}

$metalNotes = [string]$metalHostGate[0].notes
foreach ($needle in @(
        "Apple Metal host gate closeout",
        "macOS Metal CMake",
        "iOS Metal Evidence",
        "renderer-metal-environment-aggregate-apple-host-evidence",
        "environment_metal_host_aggregate_ready=1",
        "environment_platform_macos_metal_ready=1",
        "environment_platform_requires_macos_metal_host_evidence=0",
        "environment-platform-ios-metal-package",
        "environment_platform_ios_metal_ready=1",
        "environment_platform_requires_ios_metal_host_evidence=0",
        "environment-weather-metal-solver-host-gate",
        "environment_weather_simulation_metal_gpu_solver_ready=1",
        "environment_weather_simulation_metal_gpu_solver_native_handle_access=0",
        "native_handle_access=0",
        "environment_ready=0"
    )) {
    Assert-ContainsText $metalNotes $needle "metal-apple host gate notes"
}

foreach ($surface in @(
        @{
            Path = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
            Needles = @('"id": "metal-apple"', '"status": "ready"', '"residualClass": "ready"', "Apple Metal host gate closeout")
        },
        @{
            Path = ".github/workflows/validate.yml"
            Needles = @("Validate Metal weather solver host gate", "./tools/validate-environment-weather-metal-solver-host-gate.ps1")
        },
        @{
            Path = "tools/check-ci-matrix.ps1"
            Needles = @("Validate Metal weather solver host gate", "./tools/validate-environment-weather-metal-solver-host-gate.ps1")
        },
        @{
            Path = "docs/current-capabilities.md"
            Needles = @("Apple Metal host gate closeout", "environment_metal_host_aggregate_ready=1", "environment_weather_simulation_metal_gpu_solver_ready=1")
        },
        @{
            Path = "docs/testing.md"
            Needles = @("Apple Metal host gate closeout", "macOS Metal CMake", "iOS Metal Evidence", "environment_platform_ios_metal_ready=1")
        },
        @{
            Path = "docs/superpowers/plans/README.md"
            Needles = @("Apple Metal host gate closeout", "metal-apple", "environment_platform_macos_metal_ready=1")
        },
        @{
            Path = "tools/validate.ps1"
            Needles = @("check-metal-apple-host-gate-closeout.ps1")
        }
    )) {
    $surfaceText = Get-Content -LiteralPath (Join-Path $root $surface.Path) -Raw
    foreach ($needle in @($surface.Needles)) {
        Assert-ContainsText $surfaceText $needle $surface.Path
    }
}

Write-Information "metal-apple-host-gate-closeout-check: ok" -InformationAction Continue
