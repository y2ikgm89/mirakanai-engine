#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$EvidenceRoot = "artifacts/performance/cpu-profiling-matrix",

    [string[]]$RequiredHostClassIds = @(),

    [switch]$RequireReady,

    [string[]]$ExpectedEvidenceCounters = @(),

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root
$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Test-HostIsWindows {
    return [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Windows)
}

function Test-HostIsLinux {
    return [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Linux)
}

function Get-HostLabel {
    if (Test-HostIsWindows) {
        return "windows"
    }
    if (Test-HostIsLinux) {
        return "linux"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::OSX)) {
        return "macos"
    }
    return "unknown"
}

function Get-JsonPropertyValue {
    param(
        [Parameter(Mandatory = $true)][object]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $property = $JsonObject.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    return $property.Value
}

function Add-Diagnostic {
    param(
        [Parameter(Mandatory = $true)]$Diagnostics,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $Diagnostics.Add($Message) | Out-Null
}

function Test-NonEmptyJsonString {
    param(
        [Parameter(Mandatory = $true)][object]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    return -not [string]::IsNullOrWhiteSpace([string]$value)
}

function Test-SafeEvidencePath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$EvidenceRootFull
    )

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        return $false
    }
    if ($RelativePath.Contains("\")) {
        return $false
    }
    if ($RelativePath.StartsWith("/", [System.StringComparison]::Ordinal)) {
        return $false
    }
    if ($RelativePath -match "^[A-Za-z]:") {
        return $false
    }
    if ($RelativePath.Contains(":")) {
        return $false
    }
    if ($RelativePath -match "(^|/)\.\.(/|$)") {
        return $false
    }

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $EvidenceRootFull.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    return $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)
}

function Test-CommandAvailable {
    param([Parameter(Mandatory = $true)][string]$Command)

    return $null -ne (Find-CommandOnCombinedPath $Command)
}

function Test-WprPmcSourceReady {
    $wpr = Find-CommandOnCombinedPath "wpr"
    if (-not $wpr) {
        return $false
    }

    $output = & $wpr "-pmcsources" 2>&1
    if ($LASTEXITCODE -ne 0) {
        return $false
    }

    $text = (@($output) -join "`n")
    return -not [string]::IsNullOrWhiteSpace($text)
}

function Get-StringSet {
    param([object[]]$Values)

    $set = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($value in @($Values)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$value)) {
            $null = $set.Add([string]$value)
        }
    }
    return $set
}

$manifest = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw | ConvertFrom-Json
$matrix = $manifest.aiOperableProductionLoop.cpuProfilingMatrix
$hostClassIds = @(($matrix.hostClasses | ForEach-Object { [string]$_.id }))
if ($RequiredHostClassIds.Count -eq 0) {
    $RequiredHostClassIds = $hostClassIds
}

$knownHostClasses = Get-StringSet $hostClassIds
$knownTraceRecipes = Get-StringSet @($matrix.traceRecipes | ForEach-Object { [string]$_.id })
$requiredCpuFields = @($matrix.requiredCpuFields | ForEach-Object { [string]$_ })
$comparisonFields = @($matrix.beforeAfterTracePair.comparisonFields | ForEach-Object { [string]$_ })
$budgetFields = @($matrix.regressionBudget.budgetFields | ForEach-Object { [string]$_ })

$evidenceRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $EvidenceRoot))
$evidenceFiles = @()
if (Test-Path -LiteralPath $evidenceRootFull -PathType Container) {
    $evidenceFiles = @(Get-ChildItem -LiteralPath $evidenceRootFull -Recurse -File -Filter "evidence.json")
}

$validRows = 0
$readyRows = 0
$diagnosticCount = 0
$beforeAfterPairs = 0
$regressionBudgetRows = 0
$profilerArtifactRows = 0
$traceArtifactRows = 0
$pathEscapeCount = 0
$invalidJsonCount = 0
$readyHostClasses = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)

foreach ($evidenceFile in $evidenceFiles) {
    $rowDiagnostics = [System.Collections.Generic.List[string]]::new()
    try {
        $evidence = Get-Content -LiteralPath $evidenceFile.FullName -Raw | ConvertFrom-Json
    } catch {
        $invalidJsonCount += 1
        $diagnosticCount += 1
        continue
    }

    $hostClassId = [string](Get-JsonPropertyValue -JsonObject $evidence -Name "host_class_id")
    if (-not $knownHostClasses.Contains($hostClassId)) {
        Add-Diagnostic $rowDiagnostics "unknown_host_class_id"
    }

    $traceRecipeId = [string](Get-JsonPropertyValue -JsonObject $evidence -Name "trace_recipe_id")
    if (-not $knownTraceRecipes.Contains($traceRecipeId)) {
        Add-Diagnostic $rowDiagnostics "unknown_trace_recipe_id"
    }

    foreach ($field in $requiredCpuFields) {
        if (-not (Test-NonEmptyJsonString -JsonObject $evidence -Name $field)) {
            Add-Diagnostic $rowDiagnostics "missing_cpu_field:$field"
        }
    }

    $traceArtifacts = @(Get-JsonPropertyValue -JsonObject $evidence -Name "trace_artifacts")
    if ($traceArtifacts.Count -eq 0) {
        Add-Diagnostic $rowDiagnostics "missing_trace_artifacts"
    } else {
        $traceArtifactRows += $traceArtifacts.Count
        foreach ($artifact in $traceArtifacts) {
            $relativePath = [string](Get-JsonPropertyValue -JsonObject $artifact -Name "path")
            if (-not (Test-SafeEvidencePath -RelativePath $relativePath -EvidenceRootFull $evidenceRootFull)) {
                $pathEscapeCount += 1
                Add-Diagnostic $rowDiagnostics "unsafe_trace_artifact_path"
                continue
            }
            $artifactFullPath = [System.IO.Path]::GetFullPath((Join-Path $root $relativePath))
            if (-not (Test-Path -LiteralPath $artifactFullPath -PathType Leaf)) {
                Add-Diagnostic $rowDiagnostics "missing_trace_artifact_file"
            }
        }
    }

    $profilerArtifacts = @(Get-JsonPropertyValue -JsonObject $evidence -Name "profiler_artifacts")
    if ($profilerArtifacts.Count -eq 0) {
        Add-Diagnostic $rowDiagnostics "missing_profiler_artifacts"
    } else {
        $profilerArtifactRows += $profilerArtifacts.Count
    }

    $beforeAfter = Get-JsonPropertyValue -JsonObject $evidence -Name "before_after_trace_pair"
    if ($null -eq $beforeAfter) {
        Add-Diagnostic $rowDiagnostics "missing_before_after_trace_pair"
    } else {
        foreach ($field in @("baselineArtifact", "candidateArtifact")) {
            if (-not (Test-NonEmptyJsonString -JsonObject $beforeAfter -Name $field)) {
                Add-Diagnostic $rowDiagnostics "missing_before_after_field:$field"
            }
        }
        foreach ($field in $comparisonFields) {
            if (-not (Test-NonEmptyJsonString -JsonObject $beforeAfter -Name $field)) {
                Add-Diagnostic $rowDiagnostics "missing_comparison_field:$field"
            }
        }
        if ($rowDiagnostics.Count -eq 0) {
            $beforeAfterPairs += 1
        }
    }

    $budget = Get-JsonPropertyValue -JsonObject $evidence -Name "regression_budget"
    if ($null -eq $budget) {
        Add-Diagnostic $rowDiagnostics "missing_regression_budget"
    } else {
        $budgetReady = $true
        foreach ($field in $budgetFields) {
            $value = Get-JsonPropertyValue -JsonObject $budget -Name $field
            if ($null -eq $value) {
                $budgetReady = $false
                Add-Diagnostic $rowDiagnostics "missing_budget_field:$field"
                continue
            }
            try {
                if ([decimal]$value -lt 0) {
                    $budgetReady = $false
                    Add-Diagnostic $rowDiagnostics "negative_budget_field:$field"
                }
            } catch {
                $budgetReady = $false
                Add-Diagnostic $rowDiagnostics "invalid_budget_field:$field"
            }
        }
        if ($budgetReady) {
            $regressionBudgetRows += 1
        }
    }

    if ($rowDiagnostics.Count -eq 0) {
        $validRows += 1
        $readyRows += 1
        $null = $readyHostClasses.Add($hostClassId)
    } else {
        $diagnosticCount += $rowDiagnostics.Count
    }
}

$missingHostClassCount = 0
foreach ($hostClassId in $RequiredHostClassIds) {
    if (-not $readyHostClasses.Contains($hostClassId)) {
        $missingHostClassCount += 1
    }
}

$ready = $evidenceFiles.Count -gt 0 -and
    $diagnosticCount -eq 0 -and
    $invalidJsonCount -eq 0 -and
    $pathEscapeCount -eq 0 -and
    $missingHostClassCount -eq 0
$status = if ($ready) { "ready" } else { "host-gated" }

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=host-cpu-profiling-matrix")
$lines.Add("cpu_profiling_matrix_status=$status")
$lines.Add("cpu_profiling_matrix_host=$(Get-HostLabel)")
$lines.Add("cpu_profiling_matrix_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("cpu_profiling_matrix_host_gated=$(ConvertTo-CounterBit (-not $ready))")
$lines.Add("cpu_profiling_matrix_evidence_rows=$($evidenceFiles.Count)")
$lines.Add("cpu_profiling_matrix_valid_rows=$validRows")
$lines.Add("cpu_profiling_matrix_ready_rows=$readyRows")
$lines.Add("cpu_profiling_matrix_required_host_class_rows=$($RequiredHostClassIds.Count)")
$lines.Add("cpu_profiling_matrix_ready_host_class_rows=$($readyHostClasses.Count)")
$lines.Add("cpu_profiling_matrix_missing_host_class_rows=$missingHostClassCount")
$lines.Add("cpu_profiling_matrix_trace_artifact_rows=$traceArtifactRows")
$lines.Add("cpu_profiling_matrix_profiler_artifact_rows=$profilerArtifactRows")
$lines.Add("cpu_profiling_matrix_before_after_pairs=$beforeAfterPairs")
$lines.Add("cpu_profiling_matrix_regression_budget_rows=$regressionBudgetRows")
$lines.Add("cpu_profiling_matrix_diagnostic_count=$diagnosticCount")
$lines.Add("cpu_profiling_matrix_invalid_json_count=$invalidJsonCount")
$lines.Add("cpu_profiling_matrix_path_escape_count=$pathEscapeCount")
$lines.Add("cpu_profiling_matrix_wpr_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'wpr'))")
$lines.Add("cpu_profiling_matrix_wpa_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'wpa'))")
$lines.Add("cpu_profiling_matrix_xperf_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'xperf'))")
$lines.Add("cpu_profiling_matrix_wpr_pmc_sources_ready=$(ConvertTo-CounterBit (Test-WprPmcSourceReady))")
$lines.Add("cpu_profiling_matrix_perf_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'perf'))")
$lines.Add("cpu_profiling_matrix_vtune_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'vtune'))")
$lines.Add("cpu_profiling_matrix_amduprof_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'AMDuProfCLI'))")
$lines.Add("cpu_profiling_matrix_linux_affinity_execution=0")
$lines.Add("cpu_profiling_matrix_numa_execution=0")
$lines.Add("cpu_profiling_matrix_pgo_lto_changed=0")
$lines.Add("cpu_profiling_matrix_data_layout_rewrite=0")
$lines.Add("cpu_profiling_matrix_broad_optimization=0")
$lines.Add("cpu_profiling_matrix_gpu_compute_claim=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "CPU profiling matrix readiness is incomplete; attach official profiler artifacts, complete before/after trace pairs, regression budgets, and every required host class row before cpu_profiling_matrix_ready can be 1."
}

Write-Information "cpu-profiling-host-evidence-check: ok" -InformationAction Continue
