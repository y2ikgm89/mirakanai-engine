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

$collectorScript = Join-Path $root "tools/collect-cpu-profiling-host-evidence.ps1"
if (-not (Test-Path -LiteralPath $collectorScript -PathType Leaf)) {
    Write-Error "tools/collect-cpu-profiling-host-evidence.ps1 must exist for host-cpu-profiling-matrix evidence collection"
}

$evidenceRootRelative = "out/cpu-profiling-host-evidence-collector-contract/$PID"
$artifactDirectoryRelative = "$evidenceRootRelative/windows-ci-host/cpu-frame-time"
$artifactDirectory = ConvertTo-LocalPath $artifactDirectoryRelative
$null = New-Item -ItemType Directory -Path $artifactDirectory -Force

$baselineArtifact = "$artifactDirectoryRelative/baseline.etl"
$candidateArtifact = "$artifactDirectoryRelative/candidate.etl"
$profilerArtifact = "$artifactDirectoryRelative/wpa-summary.txt"

Set-Content -LiteralPath (ConvertTo-LocalPath $baselineArtifact) -Value "synthetic baseline ETL placeholder" -Encoding utf8NoBOM
Set-Content -LiteralPath (ConvertTo-LocalPath $candidateArtifact) -Value "synthetic candidate ETL placeholder" -Encoding utf8NoBOM
Set-Content -LiteralPath (ConvertTo-LocalPath $profilerArtifact) -Value "synthetic WPA summary placeholder" -Encoding utf8NoBOM

$planLines = @(& $collectorScript `
        -Mode Plan `
        -EvidenceRoot $evidenceRootRelative `
        -HostClassId "windows-ci-host" `
        -TraceRecipeId "cpu-frame-time" `
        -WorkloadRecipe "collector-contract-self-test" `
        -CompilerAndFlags "self-test" `
        -SelectedSimdLane "scalar" `
        -ThermalPowerState "self-test" `
        -NoWrite)

Assert-LinePresent $planLines "cpu_profiling_host_evidence_collector_mode=Plan" "collector Plan mode"
Assert-LinePresent $planLines "cpu_profiling_host_evidence_collector_plan_ready=1" "collector Plan mode"
Assert-LinePresent $planLines "cpu_profiling_host_evidence_collector_writes_evidence=0" "collector Plan mode"
Assert-LinePresent $planLines "cpu_profiling_host_evidence_collector_broad_optimization=0" "collector Plan mode"

$importLines = @(& $collectorScript `
        -Mode Import `
        -EvidenceRoot $evidenceRootRelative `
        -HostClassId "windows-ci-host" `
        -TraceRecipeId "cpu-frame-time" `
        -WorkloadRecipe "collector-contract-self-test" `
        -CompilerAndFlags "self-test" `
        -SelectedSimdLane "scalar" `
        -ThermalPowerState "self-test" `
        -BaselineArtifactRelativePath $baselineArtifact `
        -CandidateArtifactRelativePath $candidateArtifact `
        -ProfilerArtifactRelativePath $profilerArtifact `
        -FrameTimeBudget 16670 `
        -MemoryGrowthBudget 0 `
        -QueueWaitBudget 0 `
        -CacheMissBudget 0 `
        -BranchMissBudget 0 `
        -MemoryBandwidthBudget 0 `
        -DiagnosticsBudget 0)

Assert-LinePresent $importLines "cpu_profiling_host_evidence_collector_mode=Import" "collector Import mode"
Assert-LinePresent $importLines "cpu_profiling_host_evidence_collector_written=1" "collector Import mode"
Assert-LinePresent $importLines "cpu_profiling_host_evidence_collector_linux_affinity_execution=0" "collector Import mode"
Assert-LinePresent $importLines "cpu_profiling_host_evidence_collector_numa_execution=0" "collector Import mode"
Assert-LinePresent $importLines "cpu_profiling_host_evidence_collector_pgo_lto_changed=0" "collector Import mode"
Assert-LinePresent $importLines "cpu_profiling_host_evidence_collector_data_layout_rewrite=0" "collector Import mode"
Assert-LinePresent $importLines "cpu_profiling_host_evidence_collector_broad_optimization=0" "collector Import mode"

$evidenceJsonPath = ConvertTo-LocalPath "$artifactDirectoryRelative/evidence.json"
if (-not (Test-Path -LiteralPath $evidenceJsonPath -PathType Leaf)) {
    Write-Error "collector Import mode did not write evidence.json"
}

$evidence = Get-Content -LiteralPath $evidenceJsonPath -Raw | ConvertFrom-Json
if ([string]$evidence.host_class_id -ne "windows-ci-host") {
    Write-Error "collector evidence host_class_id mismatch"
}
if ([string]$evidence.trace_recipe_id -ne "cpu-frame-time") {
    Write-Error "collector evidence trace_recipe_id mismatch"
}
if ([string]$evidence.before_after_trace_pair.baselineArtifact -ne "baseline") {
    Write-Error "collector evidence baselineArtifact mismatch"
}
if ([string]$evidence.before_after_trace_pair.candidateArtifact -ne "candidate") {
    Write-Error "collector evidence candidateArtifact mismatch"
}
if ([decimal]$evidence.regression_budget.frame_time_budget -ne 16670) {
    Write-Error "collector evidence regression budget mismatch"
}

$validationLines = @(& (Join-Path $PSScriptRoot "check-cpu-profiling-host-evidence.ps1") `
        -EvidenceRoot $evidenceRootRelative `
        -RequiredHostClassIds "windows-ci-host" `
        -ExpectedEvidenceCounters `
        "cpu_profiling_matrix_evidence_rows=1" `
        "cpu_profiling_matrix_valid_rows=1" `
        "cpu_profiling_matrix_ready=1" `
        "cpu_profiling_matrix_missing_host_class_rows=0" `
        "cpu_profiling_matrix_broad_optimization=0")

Assert-LinePresent $validationLines "cpu_profiling_matrix_status=ready" "collector generated evidence validation"

Write-Information "cpu-profiling-host-evidence-collector-check: ok" -InformationAction Continue
