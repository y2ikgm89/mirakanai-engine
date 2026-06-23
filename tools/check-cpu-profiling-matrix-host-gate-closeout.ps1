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
$hostGate = @($manifest.aiOperableProductionLoop.hostGates | Where-Object { $_.id -eq "cpu-profiling-matrix-host" })
if ($hostGate.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must define exactly one cpu-profiling-matrix-host gate."
}
if ($hostGate[0].status -ne "ready" -or $hostGate[0].residualClass -ne "ready") {
    Write-Error "cpu-profiling-matrix-host must be ready only after hosted Windows WPR/WPA evidence passes."
}
if (@($hostGate[0].validationRecipes) -notcontains "host-cpu-profiling-matrix") {
    Write-Error "cpu-profiling-matrix-host must reference host-cpu-profiling-matrix."
}

$hostGateNotes = [string]$hostGate[0].notes
foreach ($needle in @(
        "CPU profiling matrix host gate closeout",
        "Windows Performance Recorder",
        "Windows Performance Analyzer",
        "WPA Exporter",
        "host-cpu-profiling-matrix",
        "cpu_profiling_matrix_ready=1",
        "cpu_profiling_matrix_required_host_class_rows=1",
        "cpu_profiling_matrix_missing_host_class_rows=0",
        "cpu_profiling_matrix_broad_optimization=0",
        "cpu_profiling_matrix_linux_affinity_execution=0",
        "cpu_profiling_matrix_numa_execution=0",
        "broad CPU/GPU/memory optimization remains unclaimed"
    )) {
    Assert-ContainsText $hostGateNotes $needle "cpu-profiling-matrix-host notes"
}

foreach ($surface in @(
        @{
            Path = "tools/validate-cpu-profiling-matrix-host-gate.ps1"
            Needles = @("wpr -start CPU -filemode", "wpr -stop", "check-cpu-profiling-host-evidence.ps1", "-RequireReady", "cpu_profiling_matrix_ready=1")
        },
        @{
            Path = ".github/workflows/validate.yml"
            Needles = @("Validate CPU profiling matrix host gate", "./tools/validate-cpu-profiling-matrix-host-gate.ps1")
        },
        @{
            Path = "tools/check-ci-matrix.ps1"
            Needles = @("Validate CPU profiling matrix host gate", "./tools/validate-cpu-profiling-matrix-host-gate.ps1")
        },
        @{
            Path = "docs/current-capabilities.md"
            Needles = @("CPU profiling matrix host gate closeout", "cpu_profiling_matrix_ready=1", "broad CPU/GPU/memory optimization remains unclaimed")
        },
        @{
            Path = "docs/testing.md"
            Needles = @("CPU profiling matrix host gate closeout", "Windows Performance Recorder", "Windows Performance Analyzer")
        },
        @{
            Path = "docs/superpowers/plans/README.md"
            Needles = @("CPU Profiling Matrix Host Gate Closeout", "cpu-profiling-matrix-host", "cpu_profiling_matrix_missing_host_class_rows=0")
        },
        @{
            Path = "tools/validate.ps1"
            Needles = @("check-cpu-profiling-matrix-host-gate-closeout.ps1")
        }
    )) {
    $surfaceText = Get-Content -LiteralPath (Join-Path $root $surface.Path) -Raw
    foreach ($needle in @($surface.Needles)) {
        Assert-ContainsText $surfaceText $needle $surface.Path
    }
}

Write-Information "cpu-profiling-matrix-host-gate-closeout-check: ok" -InformationAction Continue
