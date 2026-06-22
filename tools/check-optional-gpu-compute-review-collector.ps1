#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

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

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

$collectorScript = Join-Path $root "tools/collect-optional-gpu-compute-review-evidence.ps1"
if (-not (Test-Path -LiteralPath $collectorScript -PathType Leaf)) {
    Write-Error "tools/collect-optional-gpu-compute-review-evidence.ps1 must exist for host-optional-gpu-compute-review evidence collection"
}

$evidenceRootRelative = "out/optional-gpu-compute-review-collector-contract/$PID"
$artifactDirectoryRelative = "$evidenceRootRelative/runtime-rendering-or-simulation-rhi-compute"
$artifactDirectory = ConvertTo-LocalPath $artifactDirectoryRelative
$null = New-Item -ItemType Directory -Path $artifactDirectory -Force

$profilerArtifact = "$artifactDirectoryRelative/pix-timing-capture.txt"
Set-Content -LiteralPath (ConvertTo-LocalPath $profilerArtifact) -Value "synthetic PIX timing capture placeholder" -Encoding utf8NoBOM

$planLines = @(& $collectorScript `
        -Mode Plan `
        -EvidenceRoot $evidenceRootRelative `
        -CandidateId "runtime-rendering-or-simulation-rhi-compute" `
        -Classification "rhi_compute" `
        -SelectedBackend "D3D12" `
        -WorkloadRecipe "collector-contract-self-test" `
        -DependencyInstallGate "reviewed-first-party-rhi" `
        -ScalarOrRhiFallback "scalar-reference-available" `
        -SynchronizationEvidence "queue-fence-reviewed" `
        -StreamEventUsage "rhi-queue-fence" `
        -NoWrite)

Assert-LinePresent $planLines "optional_gpu_compute_review_collector_mode=Plan" "collector Plan mode"
Assert-LinePresent $planLines "optional_gpu_compute_review_collector_plan_ready=1" "collector Plan mode"
Assert-LinePresent $planLines "optional_gpu_compute_review_collector_writes_evidence=0" "collector Plan mode"
Assert-LinePresent $planLines "optional_gpu_compute_review_collector_broad_gpu_compute_claim=0" "collector Plan mode"

$importLines = @(& $collectorScript `
        -Mode Import `
        -EvidenceRoot $evidenceRootRelative `
        -CandidateId "runtime-rendering-or-simulation-rhi-compute" `
        -Classification "rhi_compute" `
        -SelectedBackend "D3D12" `
        -WorkloadRecipe "collector-contract-self-test" `
        -DependencyInstallGate "reviewed-first-party-rhi" `
        -ScalarOrRhiFallback "scalar-reference-available" `
        -SynchronizationEvidence "queue-fence-reviewed" `
        -StreamEventUsage "rhi-queue-fence" `
        -ProfilerArtifactRelativePath $profilerArtifact)

Assert-LinePresent $importLines "optional_gpu_compute_review_collector_mode=Import" "collector Import mode"
Assert-LinePresent $importLines "optional_gpu_compute_review_collector_written=1" "collector Import mode"
Assert-LinePresent $importLines "optional_gpu_compute_review_collector_default_validation_dependency=0" "collector Import mode"
Assert-LinePresent $importLines "optional_gpu_compute_review_collector_runtime_dependency_added=0" "collector Import mode"
Assert-LinePresent $importLines "optional_gpu_compute_review_collector_public_native_handles_exposed=0" "collector Import mode"
Assert-LinePresent $importLines "optional_gpu_compute_review_collector_broad_gpu_compute_claim=0" "collector Import mode"

$evidenceJsonPath = ConvertTo-LocalPath "$artifactDirectoryRelative/evidence.json"
if (-not (Test-Path -LiteralPath $evidenceJsonPath -PathType Leaf)) {
    Write-Error "collector Import mode did not write evidence.json"
}

$evidence = Get-Content -LiteralPath $evidenceJsonPath -Raw | ConvertFrom-Json
if ([string]$evidence.candidate_id -ne "runtime-rendering-or-simulation-rhi-compute") {
    Write-Error "collector evidence candidate_id mismatch"
}
if ([string]$evidence.classification -ne "rhi_compute") {
    Write-Error "collector evidence classification mismatch"
}
if ([string]$evidence.synchronization -ne "queue-fence-reviewed") {
    Write-Error "collector evidence synchronization mismatch"
}

$validationLines = @(& (Join-Path $PSScriptRoot "check-optional-gpu-compute-review-evidence.ps1") `
        -EvidenceRoot $evidenceRootRelative `
        -RequiredCandidateIds "runtime-rendering-or-simulation-rhi-compute" `
        -ExpectedEvidenceCounters `
        "optional_gpu_compute_review_evidence_rows=1" `
        "optional_gpu_compute_review_valid_rows=1" `
        "optional_gpu_compute_review_ready=1" `
        "optional_gpu_compute_review_missing_candidate_rows=0" `
        "optional_gpu_compute_review_broad_gpu_compute_claim=0")

Assert-LinePresent $validationLines "optional_gpu_compute_review_status=ready" "collector generated evidence validation"

Write-Information "optional-gpu-compute-review-collector-check: ok" -InformationAction Continue
