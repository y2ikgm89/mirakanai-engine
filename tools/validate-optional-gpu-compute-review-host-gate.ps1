#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string[]]$ExpectedEvidenceCounters = @(),

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)

function Assert-LinePresent {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$ExpectedLine,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Lines.Contains($ExpectedLine)) {
        Write-Error "$Context missing expected line: $ExpectedLine"
    }
}

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

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

$evidenceRootRelative = "artifacts/performance/optional-gpu-compute-review/2026-06-23-rhi-compute-vulkan-weather-simulation"
$candidateId = "runtime-rendering-or-simulation-rhi-compute"
$candidateEvidencePath = "$evidenceRootRelative/$candidateId/evidence.json"
$timestampArtifactPath = "$evidenceRootRelative/$candidateId/vulkan-timestamp-validation.csv"
$traceArtifactPath = "$evidenceRootRelative/$candidateId/trace-events.json"
$sourceTimestampArtifactPath = "artifacts/environment/optimization/2026-06-19-vulkan-strict-validation-smoke/vulkan_strict/weather_simulation_stress/vulkan-timestamp-validation.csv"

foreach ($relativePath in @($candidateEvidencePath, $timestampArtifactPath, $traceArtifactPath, $sourceTimestampArtifactPath)) {
    if (-not (Test-Path -LiteralPath (ConvertTo-LocalPath $relativePath) -PathType Leaf)) {
        Write-Error "Required optional GPU compute review artifact is missing: $relativePath"
    }
}

$requiredCounters = @(
    "validation_recipe=host-optional-gpu-compute-review",
    "optional_gpu_compute_review_status=ready",
    "optional_gpu_compute_review_ready=1",
    "optional_gpu_compute_review_host_gated=0",
    "optional_gpu_compute_review_required_candidate_rows=1",
    "optional_gpu_compute_review_missing_candidate_rows=0",
    "optional_gpu_compute_review_profiler_artifact_rows=2",
    "optional_gpu_compute_review_dependency_install_gate_rows=1",
    "optional_gpu_compute_review_fallback_rows=1",
    "optional_gpu_compute_review_synchronization_evidence_rows=1",
    "optional_gpu_compute_review_default_validation_dependency=0",
    "optional_gpu_compute_review_vcpkg_feature_added=0",
    "optional_gpu_compute_review_cmake_linkage_added=0",
    "optional_gpu_compute_review_runtime_dependency_added=0",
    "optional_gpu_compute_review_public_native_handles_exposed=0",
    "optional_gpu_compute_review_broad_gpu_compute_claim=0",
    "optional_gpu_compute_review_async_overlap_claim=0",
    "optional_gpu_compute_review_cross_vendor_parity_claim=0",
    "optional_gpu_compute_review_cross_backend_parity_claim=0",
    "optional_gpu_compute_review_broad_optimization_claim=0"
)

$validationLines = @(& (Join-Path $PSScriptRoot "check-optional-gpu-compute-review-evidence.ps1") `
        -EvidenceRoot $evidenceRootRelative `
        -RequiredCandidateIds $candidateId `
        -RequireReady `
        -ExpectedEvidenceCounters (@($requiredCounters) + @($ExpectedEvidenceCounters)))

foreach ($line in $validationLines) {
    Write-Output $line
}

foreach ($counter in $requiredCounters) {
    Assert-LinePresent $validationLines $counter "optional GPU compute review host gate"
}

$evidence = Get-Content -LiteralPath (ConvertTo-LocalPath $candidateEvidencePath) -Raw | ConvertFrom-Json
if ([string]$evidence.candidate_id -ne $candidateId) {
    Write-Error "Optional GPU compute review evidence candidate_id mismatch."
}
if ([string]$evidence.classification -ne "rhi_compute") {
    Write-Error "Optional GPU compute review evidence classification must be rhi_compute."
}
if ([string]$evidence.selected_backend -ne "vulkan_strict") {
    Write-Error "Optional GPU compute review selected backend must be vulkan_strict."
}

$timestampHash = (Get-FileHash -LiteralPath (ConvertTo-LocalPath $timestampArtifactPath) -Algorithm SHA256).Hash.ToLowerInvariant()
$sourceTimestampHash = (Get-FileHash -LiteralPath (ConvertTo-LocalPath $sourceTimestampArtifactPath) -Algorithm SHA256).Hash.ToLowerInvariant()
if ($timestampHash -ne [string]$evidence.source_artifact_hash_sha256 -or $timestampHash -ne $sourceTimestampHash) {
    Write-Error "Optional GPU compute review timestamp artifact hash does not match the retained source artifact."
}

$traceText = Get-Content -LiteralPath (ConvertTo-LocalPath $traceArtifactPath) -Raw
$trace = $traceText | ConvertFrom-Json
Assert-ContainsText $traceText "Vulkan timestamp query + VK_LAYER_KHRONOS_validation" "optional GPU compute review trace artifact"
if (@($trace.traceEvents).Count -lt 2) {
    Write-Error "Optional GPU compute review trace artifact must contain workload and metric events."
}

Write-Output "optional_gpu_compute_review_host_gate_ready=1"
Write-Output "optional_gpu_compute_review_host_gate_selected_candidate=$candidateId"
Write-Output "optional_gpu_compute_review_host_gate_artifact_hash=$timestampHash"
Write-Information "optional-gpu-compute-review-host-gate-validation: ok" -InformationAction Continue
