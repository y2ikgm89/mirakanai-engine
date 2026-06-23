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
$hostGate = @($manifest.aiOperableProductionLoop.hostGates | Where-Object { $_.id -eq "optional-gpu-compute-review-host" })
if ($hostGate.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must define exactly one optional-gpu-compute-review-host gate."
}
if ($hostGate[0].status -ne "ready" -or $hostGate[0].residualClass -ne "ready") {
    Write-Error "optional-gpu-compute-review-host must be ready only after retained real RHI compute review artifacts pass."
}
if (@($hostGate[0].validationRecipes) -notcontains "host-optional-gpu-compute-review") {
    Write-Error "optional-gpu-compute-review-host must reference host-optional-gpu-compute-review."
}

$hostGateNotes = [string]$hostGate[0].notes
foreach ($needle in @(
        "Optional GPU compute review host gate closeout",
        "runtime-rendering-or-simulation-rhi-compute",
        "Vulkan timestamp query",
        "VK_LAYER_KHRONOS_validation",
        "optional_gpu_compute_review_ready=1",
        "optional_gpu_compute_review_required_candidate_rows=1",
        "optional_gpu_compute_review_missing_candidate_rows=0",
        "optional_gpu_compute_review_profiler_artifact_rows=2",
        "optional_gpu_compute_review_broad_gpu_compute_claim=0",
        "CUDA/HIP/SYCL runtime remains unclaimed",
        "broad CPU/GPU/memory optimization remains unclaimed"
    )) {
    Assert-ContainsText $hostGateNotes $needle "optional-gpu-compute-review-host notes"
}

foreach ($surface in @(
        @{
            Path = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
            Needles = @('"id": "optional-gpu-compute-review-host"', '"status": "ready"', '"residualClass": "ready"', "Optional GPU compute review host gate closeout")
        },
        @{
            Path = "engine/agent/manifest.fragments/009-validationRecipes.json"
            Needles = @("tools/validate-optional-gpu-compute-review-host-gate.ps1", "runtime-rendering-or-simulation-rhi-compute", "-RequiredCandidateIds")
        },
        @{
            Path = "tools/validate-optional-gpu-compute-review-host-gate.ps1"
            Needles = @("2026-06-23-rhi-compute-vulkan-weather-simulation", "optional_gpu_compute_review_ready=1", "source_artifact_hash_sha256")
        },
        @{
            Path = "docs/current-capabilities.md"
            Needles = @("Optional GPU compute review host gate closeout", "runtime-rendering-or-simulation-rhi-compute", "CUDA/HIP/SYCL runtime remains unclaimed")
        },
        @{
            Path = "docs/testing.md"
            Needles = @("Optional GPU compute review host gate closeout", "tools/validate-optional-gpu-compute-review-host-gate.ps1", "optional_gpu_compute_review_ready=1")
        },
        @{
            Path = "docs/superpowers/plans/README.md"
            Needles = @("Optional GPU Compute Review Host Gate Closeout", "optional-gpu-compute-review-host", "optional_gpu_compute_review_missing_candidate_rows=0")
        },
        @{
            Path = "tools/validate.ps1"
            Needles = @("check-optional-gpu-compute-review-host-gate-closeout.ps1")
        }
    )) {
    $surfaceText = Get-Content -LiteralPath (Join-Path $root $surface.Path) -Raw
    foreach ($needle in @($surface.Needles)) {
        Assert-ContainsText $surfaceText $needle $surface.Path
    }
}

$validationLines = @(& (Join-Path $PSScriptRoot "validate-optional-gpu-compute-review-host-gate.ps1"))
foreach ($line in $validationLines) {
    Write-Output $line
}
if (-not $validationLines.Contains("optional_gpu_compute_review_host_gate_ready=1")) {
    Write-Error "optional GPU compute review host gate validator did not report ready."
}

Write-Information "optional-gpu-compute-review-host-gate-closeout-check: ok" -InformationAction Continue
