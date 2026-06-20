#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady,
    [string[]]$ExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$artifactRootRelative = "artifacts/environment/optimization"
$artifactRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $artifactRootRelative))
$requiredBackends = @("d3d12", "vulkan_strict", "metal_apple_host")
$requiredWorkloads = @(
    "preset_pack_flythrough",
    "storm_precipitation",
    "dense_volumetric_fog",
    "volumetric_cloud_sunset",
    "snowfield_material_weathering",
    "weather_simulation_stress",
    "asset_library_cold_load"
)
$beforeAfterMetricPairs = @(
    @("cpu_frame_p95_before_us", "cpu_frame_p95_after_us"),
    @("gpu_frame_p95_before_us", "gpu_frame_p95_after_us"),
    @("memory_peak_before_bytes", "memory_peak_after_bytes"),
    @("upload_before_bytes", "upload_after_bytes"),
    @("barrier_count_before", "barrier_count_after"),
    @("shader_compile_or_pipeline_cache_before_ms", "shader_compile_or_pipeline_cache_after_ms"),
    @("stutter_frames_before", "stutter_frames_after")
)
$budgetChecks = @(
    @("cpu_frame_p95_after_us", "cpu_frame_p95_budget_us"),
    @("gpu_frame_p95_after_us", "gpu_frame_p95_budget_us"),
    @("memory_peak_after_bytes", "memory_peak_budget_bytes"),
    @("upload_after_bytes", "upload_budget_bytes"),
    @("stutter_frames_after", "stutter_frames_budget"),
    @("shader_compile_or_pipeline_cache_after_ms", "shader_compile_or_pipeline_cache_budget_ms")
)

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Get-LeafName {
    param([Parameter(Mandatory = $true)][string]$Path)

    return [System.IO.Path]::GetFileName($Path.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
}

function Get-ParentPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $parent = [System.IO.Path]::GetDirectoryName($Path)
    if ($null -eq $parent) {
        return ""
    }
    return $parent
}

function Test-SafeEnvironmentOptimizationArtifactPath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$ExpectedPrefix
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
    if (-not $RelativePath.StartsWith($ExpectedPrefix, [System.StringComparison]::Ordinal)) {
        return $false
    }

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $artifactRootFull.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    return $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)
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

function Get-RequiredDecimalProperty {
    param(
        [Parameter(Mandatory = $true)][object]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][System.Collections.Generic.List[string]]$Diagnostics
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($null -eq $value) {
        $Diagnostics.Add("missing_metric:$Name")
        return $null
    }

    try {
        $decimalValue = [decimal]$value
    } catch {
        $Diagnostics.Add("invalid_metric:$Name")
        return $null
    }

    if ($decimalValue -lt 0) {
        $Diagnostics.Add("negative_metric:$Name")
        return $null
    }
    return $decimalValue
}

function Get-EvidenceFilesForPair {
    param(
        [Parameter(Mandatory = $true)][string]$Backend,
        [Parameter(Mandatory = $true)][string]$Workload
    )

    if (-not (Test-Path -LiteralPath $artifactRootFull -PathType Container)) {
        return @()
    }

    $files = Get-ChildItem -LiteralPath $artifactRootFull -Recurse -File -Filter "evidence.json" |
        Where-Object {
            $workloadDirectory = Get-ParentPath -Path $_.FullName
            $backendDirectory = Get-ParentPath -Path $workloadDirectory
            (Get-LeafName -Path $workloadDirectory) -eq $Workload -and
                (Get-LeafName -Path $backendDirectory) -eq $Backend
        }
    return @($files)
}

$requiredPairCount = $requiredBackends.Count * $requiredWorkloads.Count
$validRowCount = 0
$beforeAfterPairCount = 0
$profilerArtifactCount = 0
$traceEventJsonCount = 0
$budgetRowCount = 0
$overBudgetCount = 0
$missingArtifactCount = 0
$invalidHashCount = 0
$pathEscapeCount = 0
$invalidJsonCount = 0
$duplicateRowCount = 0
$diagnosticCount = 0
$readyBackends = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)

foreach ($backend in $requiredBackends) {
    foreach ($workload in $requiredWorkloads) {
        $rowDiagnostics = [System.Collections.Generic.List[string]]::new()
        $evidenceFiles = @(Get-EvidenceFilesForPair -Backend $backend -Workload $workload)
        if ($evidenceFiles.Count -eq 0) {
            $missingArtifactCount += 1
            $diagnosticCount += 1
            continue
        }
        if ($evidenceFiles.Count -ne 1) {
            $duplicateRowCount += 1
            $diagnosticCount += 1
            continue
        }

        $evidenceFile = $evidenceFiles[0]
        $workloadDirectory = Get-ParentPath -Path $evidenceFile.FullName
        $backendDirectory = Get-ParentPath -Path $workloadDirectory
        $taskDirectory = Get-ParentPath -Path $backendDirectory
        $taskId = Get-LeafName -Path $taskDirectory
        $expectedPathPrefix = "$artifactRootRelative/$taskId/$backend/$workload/"

        try {
            $evidence = Get-Content -LiteralPath $evidenceFile.FullName -Raw | ConvertFrom-Json
        } catch {
            $invalidJsonCount += 1
            $diagnosticCount += 1
            continue
        }

        if ([string](Get-JsonPropertyValue -JsonObject $evidence -Name "task_id") -ne $taskId) {
            $rowDiagnostics.Add("task_id_mismatch")
        }
        if ([string](Get-JsonPropertyValue -JsonObject $evidence -Name "backend") -ne $backend) {
            $rowDiagnostics.Add("backend_mismatch")
        }
        if ([string](Get-JsonPropertyValue -JsonObject $evidence -Name "workload") -ne $workload) {
            $rowDiagnostics.Add("workload_mismatch")
        }

        foreach ($metricPair in $beforeAfterMetricPairs) {
            $beforeMetric = Get-RequiredDecimalProperty -JsonObject $evidence -Name $metricPair[0] -Diagnostics $rowDiagnostics
            $afterMetric = Get-RequiredDecimalProperty -JsonObject $evidence -Name $metricPair[1] -Diagnostics $rowDiagnostics
            if ($null -ne $beforeMetric -and $null -ne $afterMetric) {
                continue
            }
        }

        $timestampFrequency = Get-RequiredDecimalProperty -JsonObject $evidence -Name "gpu_timestamp_ticks_per_second" -Diagnostics $rowDiagnostics
        if ($null -eq $timestampFrequency -or $timestampFrequency -le 0) {
            $rowDiagnostics.Add("invalid_gpu_timestamp_ticks_per_second")
        }

        foreach ($budgetCheck in $budgetChecks) {
            $afterValue = Get-RequiredDecimalProperty -JsonObject $evidence -Name $budgetCheck[0] -Diagnostics $rowDiagnostics
            $budgetValue = Get-RequiredDecimalProperty -JsonObject $evidence -Name $budgetCheck[1] -Diagnostics $rowDiagnostics
            if ($null -ne $afterValue -and $null -ne $budgetValue) {
                $budgetRowCount += 1
                if ($afterValue -gt $budgetValue) {
                    $overBudgetCount += 1
                    $rowDiagnostics.Add("over_budget:$($budgetCheck[0])")
                }
            }
        }

        $profilerArtifactPath = [string](Get-JsonPropertyValue -JsonObject $evidence -Name "profiler_artifact_path")
        $traceEventJsonPath = [string](Get-JsonPropertyValue -JsonObject $evidence -Name "trace_event_json_path")
        $artifactHash = [string](Get-JsonPropertyValue -JsonObject $evidence -Name "artifact_hash_sha256")

        $profilerPathSafe = Test-SafeEnvironmentOptimizationArtifactPath -RelativePath $profilerArtifactPath -ExpectedPrefix $expectedPathPrefix
        $tracePathSafe = Test-SafeEnvironmentOptimizationArtifactPath -RelativePath $traceEventJsonPath -ExpectedPrefix $expectedPathPrefix
        if (-not $profilerPathSafe -or -not $tracePathSafe) {
            $pathEscapeCount += 1
            $rowDiagnostics.Add("artifact_path_escape")
        }

        if ($artifactHash -cnotmatch "^[0-9a-f]{64}$") {
            $invalidHashCount += 1
            $rowDiagnostics.Add("invalid_artifact_hash_sha256")
        }

        if ($profilerPathSafe) {
            $profilerFullPath = [System.IO.Path]::GetFullPath((Join-Path $root $profilerArtifactPath))
            if (Test-Path -LiteralPath $profilerFullPath -PathType Leaf) {
                $profilerArtifactCount += 1
                if ($artifactHash -cmatch "^[0-9a-f]{64}$") {
                    $actualHash = (Get-FileHash -LiteralPath $profilerFullPath -Algorithm SHA256).Hash.ToLowerInvariant()
                    if ($actualHash -cne $artifactHash) {
                        $invalidHashCount += 1
                        $rowDiagnostics.Add("artifact_hash_mismatch")
                    }
                }
            } else {
                $missingArtifactCount += 1
                $rowDiagnostics.Add("missing_profiler_artifact")
            }
        }

        if ($tracePathSafe) {
            $traceFullPath = [System.IO.Path]::GetFullPath((Join-Path $root $traceEventJsonPath))
            if (Test-Path -LiteralPath $traceFullPath -PathType Leaf) {
                try {
                    $null = Get-Content -LiteralPath $traceFullPath -Raw | ConvertFrom-Json
                    $traceEventJsonCount += 1
                } catch {
                    $invalidJsonCount += 1
                    $rowDiagnostics.Add("invalid_trace_event_json")
                }
            } else {
                $missingArtifactCount += 1
                $rowDiagnostics.Add("missing_trace_event_json")
            }
        }

        if ($rowDiagnostics.Count -eq 0) {
            $validRowCount += 1
            $beforeAfterPairCount += 1
            $null = $readyBackends.Add($backend)
        } else {
            $diagnosticCount += $rowDiagnostics.Count
        }
    }
}

$ready = $validRowCount -eq $requiredPairCount -and
    $beforeAfterPairCount -eq $requiredPairCount -and
    $profilerArtifactCount -eq $requiredPairCount -and
    $traceEventJsonCount -eq $requiredPairCount -and
    $budgetRowCount -eq ($requiredPairCount * $budgetChecks.Count) -and
    $overBudgetCount -eq 0 -and
    $missingArtifactCount -eq 0 -and
    $invalidHashCount -eq 0 -and
    $pathEscapeCount -eq 0 -and
    $invalidJsonCount -eq 0 -and
    $duplicateRowCount -eq 0 -and
    $readyBackends.Count -eq $requiredBackends.Count
$status = if ($ready) { "ready" } else { "host_evidence_required" }

$counterLine = [string]::Join(" ", @(
        "environment-optimization-artifacts:",
        "validation_recipe=environment-broad-optimization-cross-backend-measurement",
        "artifact_root=$artifactRootRelative",
        "environment_optimization_measurement_status=$status",
        "environment_optimization_measurement_ready=$(ConvertTo-CounterBit $ready)",
        "environment_optimization_measurement_workload_rows=$validRowCount",
        "environment_optimization_measurement_required_workload_rows=$requiredPairCount",
        "environment_optimization_measurement_required_workloads=$($requiredWorkloads.Count)",
        "environment_optimization_measurement_backend_rows=$($readyBackends.Count)",
        "environment_optimization_measurement_required_backend_rows=$($requiredBackends.Count)",
        "environment_optimization_measurement_required_backends=d3d12,vulkan_strict,metal_apple_host",
        "environment_optimization_measurement_before_after_pairs=$beforeAfterPairCount",
        "environment_optimization_measurement_profiler_artifacts=$profilerArtifactCount",
        "environment_optimization_measurement_trace_event_json=$traceEventJsonCount",
        "environment_optimization_measurement_budget_rows=$budgetRowCount",
        "environment_optimization_measurement_missing_artifacts=$missingArtifactCount",
        "environment_optimization_measurement_invalid_hashes=$invalidHashCount",
        "environment_optimization_measurement_path_escapes=$pathEscapeCount",
        "environment_optimization_measurement_invalid_json=$invalidJsonCount",
        "environment_optimization_measurement_duplicate_rows=$duplicateRowCount",
        "environment_optimization_measurement_over_budget=$overBudgetCount",
        "environment_optimization_measurement_diagnostics=$diagnosticCount",
        "environment_optimization_measurement_gpu_timestamp_frequency_rows=$validRowCount",
        "environment_optimization_measurement_profiler_artifact_root=artifacts/environment/optimization/<task-id>/<backend>/<workload>/",
        "environment_broad_optimization_ready=$(ConvertTo-CounterBit $ready)",
        "environment_ready=0",
        "environment_commercial_ready=0"
    ))

Write-Host $counterLine

$missingExpectedCounters = @()
foreach ($counter in @($ExpectedEvidenceCounters)) {
    if (-not $counterLine.Contains([string]$counter)) {
        $missingExpectedCounters += [string]$counter
    }
}

if ($missingExpectedCounters.Count -ne 0) {
    Write-Error "environment optimization artifact validation did not emit expected counter(s): $([string]::Join(', ', $missingExpectedCounters))"
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "environment optimization artifacts are incomplete; 21 retained profiler/trace/budget rows are required before environment_broad_optimization_ready can be 1."
}

Write-Information "environment-optimization-artifacts-check: ok" -InformationAction Continue
