#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$EvidenceRoot = "out/cpu-profiling-matrix-host-gate",

    [ValidateRange(250, 10000)]
    [int]$TraceDurationMilliseconds = 1000
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function ConvertTo-RepositoryRelativePath {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$Path)

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $rootPrefix = ([System.IO.Path]::GetFullPath($root)).TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "Path is outside repository root: $Path"
    }
    return $fullPath.Substring($rootPrefix.Length).Replace([System.IO.Path]::DirectorySeparatorChar, "/")
}

function Resolve-RequiredCommand {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$Command,
        [Parameter(Mandatory = $true)][string]$Purpose
    )

    $commandPath = Find-CommandOnCombinedPath $Command
    if (-not $commandPath) {
        Write-Error "$Purpose requires '$Command' on PATH."
    }
    return [string]$commandPath
}

function Invoke-WprCancelQuietly {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$WprPath)

    & $WprPath -cancel | Out-Null 2>&1
}

function Invoke-CpuSampleWorkload {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][int]$DurationMilliseconds)

    $deadline = [DateTimeOffset]::UtcNow.AddMilliseconds($DurationMilliseconds)
    $accumulator = [int64]146959810
    while ([DateTimeOffset]::UtcNow -lt $deadline) {
        for ($index = 0; $index -lt 4096; $index += 1) {
            $accumulator = [int64]((($accumulator * 1103515245) + 12345 + $index) % 2147483647)
        }
    }
    Write-Information "cpu-profiling-matrix-host-gate-workload-hash=$accumulator" -InformationAction Continue
}

function Invoke-WprCpuTrace {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$WprPath,
        [Parameter(Mandatory = $true)][string]$OutputPath,
        [Parameter(Mandatory = $true)][int]$DurationMilliseconds
    )

    Invoke-WprCancelQuietly -WprPath $WprPath
    try {
        Write-Information "wpr -start CPU -filemode" -InformationAction Continue
        & $WprPath -start CPU -filemode
        if ($LASTEXITCODE -ne 0) {
            Write-Error "WPR failed to start the CPU filemode profile."
        }
        Invoke-CpuSampleWorkload -DurationMilliseconds $DurationMilliseconds
        Write-Information "wpr -stop $OutputPath" -InformationAction Continue
        & $WprPath -stop $OutputPath
        if ($LASTEXITCODE -ne 0) {
            Write-Error "WPR failed to stop and write the trace: $OutputPath"
        }
    } finally {
        Invoke-WprCancelQuietly -WprPath $WprPath
    }

    if (-not (Test-Path -LiteralPath $OutputPath -PathType Leaf)) {
        Write-Error "WPR did not create the expected trace: $OutputPath"
    }
    if ((Get-Item -LiteralPath $OutputPath).Length -le 0) {
        Write-Error "WPR trace is empty: $OutputPath"
    }
}

if (-not [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Windows)) {
    Write-Error "CPU profiling matrix host gate closeout currently requires a Windows host with Windows Performance Toolkit."
}

$wprPath = Resolve-RequiredCommand -Command "wpr" -Purpose "Windows Performance Recorder host evidence"
$wpaPath = Resolve-RequiredCommand -Command "wpa" -Purpose "Windows Performance Analyzer host evidence"
$wpaExporterPath = Resolve-RequiredCommand -Command "wpaexporter" -Purpose "WPA Exporter host evidence"
$xperfPath = Resolve-RequiredCommand -Command "xperf" -Purpose "Windows Performance Toolkit compatibility evidence"

$evidenceDirectory = Join-Path $root ($EvidenceRoot -replace "/", [System.IO.Path]::DirectorySeparatorChar)
$traceDirectory = Join-Path $evidenceDirectory "windows-ci-host/cpu-frame-time"
$null = New-Item -ItemType Directory -Path $traceDirectory -Force

$baselineTrace = Join-Path $traceDirectory "baseline.etl"
$candidateTrace = Join-Path $traceDirectory "candidate.etl"
$profilerSummary = Join-Path $traceDirectory "wpa-wpr-summary.txt"

$toolSummary = [System.Collections.Generic.List[string]]::new()
$toolSummary.Add("Windows Performance Recorder=$wprPath")
$toolSummary.Add("Windows Performance Analyzer=$wpaPath")
$toolSummary.Add("WPA Exporter=$wpaExporterPath")
$toolSummary.Add("Xperf=$xperfPath")
$toolSummary.Add("WPR status before collection:")
foreach ($summaryLine in @(& $wprPath -status 2>&1)) {
    $null = $toolSummary.Add([string]$summaryLine)
}
$toolSummary.Add("WPR PMU sources:")
foreach ($summaryLine in @(& $wprPath -pmcsources 2>&1)) {
    $null = $toolSummary.Add([string]$summaryLine)
}
$toolSummary.Add("WPA Exporter help probe:")
foreach ($summaryLine in @(& $wpaExporterPath -h 2>&1 | Select-Object -First 20)) {
    $null = $toolSummary.Add([string]$summaryLine)
}
$toolSummary | Set-Content -LiteralPath $profilerSummary -Encoding utf8NoBOM

Invoke-WprCpuTrace -WprPath $wprPath -OutputPath $baselineTrace -DurationMilliseconds $TraceDurationMilliseconds
Invoke-WprCpuTrace -WprPath $wprPath -OutputPath $candidateTrace -DurationMilliseconds $TraceDurationMilliseconds

$collectorLines = @(& (Join-Path $PSScriptRoot "collect-cpu-profiling-host-evidence.ps1") `
        -Mode Import `
        -EvidenceRoot $EvidenceRoot `
        -HostClassId "windows-ci-host" `
        -TraceRecipeId "cpu-frame-time" `
        -WorkloadRecipe "cpu-profiling-matrix-host-gate-closeout-wpr-cpu" `
        -CompilerAndFlags "windows-msvc-hosted-validation-no-codegen-change" `
        -SelectedSimdLane "scalar" `
        -ThermalPowerState "hosted-runner-default" `
        -BaselineArtifactRelativePath (ConvertTo-RepositoryRelativePath -Path $baselineTrace) `
        -CandidateArtifactRelativePath (ConvertTo-RepositoryRelativePath -Path $candidateTrace) `
        -ProfilerArtifactRelativePath (ConvertTo-RepositoryRelativePath -Path $profilerSummary) `
        -FrameTimeBudget 0 `
        -MemoryGrowthBudget 0 `
        -QueueWaitBudget 0 `
        -CacheMissBudget 0 `
        -BranchMissBudget 0 `
        -MemoryBandwidthBudget 0 `
        -DiagnosticsBudget 0)

$validationLines = @(& (Join-Path $PSScriptRoot "check-cpu-profiling-host-evidence.ps1") `
        -EvidenceRoot $EvidenceRoot `
        -RequiredHostClassIds "windows-ci-host" `
        -RequireReady `
        -ExpectedEvidenceCounters `
        "cpu_profiling_matrix_ready=1" `
        "cpu_profiling_matrix_required_host_class_rows=1" `
        "cpu_profiling_matrix_missing_host_class_rows=0" `
        "cpu_profiling_matrix_trace_artifact_rows=2" `
        "cpu_profiling_matrix_profiler_artifact_rows=1" `
        "cpu_profiling_matrix_before_after_pairs=1" `
        "cpu_profiling_matrix_regression_budget_rows=1" `
        "cpu_profiling_matrix_broad_optimization=0" `
        "cpu_profiling_matrix_linux_affinity_execution=0" `
        "cpu_profiling_matrix_numa_execution=0")

foreach ($line in @($collectorLines) + @($validationLines)) {
    Write-Output $line
}

Write-Information "cpu-profiling-matrix-host-gate-check: ok" -InformationAction Continue
