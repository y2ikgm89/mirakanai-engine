#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady,
    [string]$ArtifactRootRelative = "artifacts/mavg/async-overlap-performance",
    [string[]]$ExpectedEvidenceCounters = @(),
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)
$root = Get-RepoRoot
Set-Location $root

$artifactRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $ArtifactRootRelative))
$acceptedProfilerTools = @("pix_timing_capture", "nsight_graphics_gpu_trace", "radeon_gpu_profiler", "intel_gpa", "apple_metal_tools")

function ConvertTo-CounterBit {
    param([bool]$Value)
    if ($Value) { return "1" }
    return "0"
}

function Get-JsonPropertyValue {
    param([Parameter(Mandatory = $true)][object]$JsonObject, [Parameter(Mandatory = $true)][string]$Name)
    if ($null -eq $JsonObject) { return $null }
    $property = $JsonObject.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-RequiredStringProperty {
    param([object]$JsonObject, [string]$Name, [System.Collections.Generic.List[string]]$Diagnostics)
    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        $Diagnostics.Add("missing_string:$Name")
        return ""
    }
    return [string]$value
}

function Get-RequiredDecimalProperty {
    param([object]$JsonObject, [string]$Name, [System.Collections.Generic.List[string]]$Diagnostics)
    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($null -eq $value) {
        $Diagnostics.Add("missing_number:$Name")
        return $null
    }
    try {
        $decimalValue = [decimal]$value
    } catch {
        $Diagnostics.Add("invalid_number:$Name")
        return $null
    }
    if ($decimalValue -lt 0) {
        $Diagnostics.Add("negative_number:$Name")
        return $null
    }
    return $decimalValue
}

function Get-RequiredBoolProperty {
    param([object]$JsonObject, [string]$Name, [System.Collections.Generic.List[string]]$Diagnostics)
    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($null -eq $value) {
        $Diagnostics.Add("missing_bool:$Name")
        return $false
    }
    if ($value -is [bool]) { return [bool]$value }
    $Diagnostics.Add("invalid_bool:$Name")
    return $false
}

function Test-Sha256Text {
    param([string]$Value)
    return $Value -cmatch "^[0-9a-f]{64}$"
}

function Get-BasisPointImprovement {
    param([decimal]$Baseline, [decimal]$Optimized)
    if ($Baseline -le 0 -or $Optimized -ge $Baseline) { return 0 }
    return [int][System.Math]::Floor((($Baseline - $Optimized) * 10000) / $Baseline)
}

function Get-BasisPointRegression {
    param([decimal]$Baseline, [decimal]$Candidate)
    if ($Baseline -le 0 -or $Candidate -le $Baseline) { return 0 }
    return [int][System.Math]::Floor((($Candidate - $Baseline) * 10000) / $Baseline)
}

function Get-LeafName {
    param([string]$Path)
    return [System.IO.Path]::GetFileName($Path.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
}

function Get-ParentPath {
    param([string]$Path)
    $parent = [System.IO.Path]::GetDirectoryName($Path)
    if ($null -eq $parent) { return "" }
    return $parent
}

function Test-SafeMavgAsyncOverlapArtifactPath {
    param([string]$RelativePath, [string]$ExpectedPrefix)
    if ([string]::IsNullOrWhiteSpace($RelativePath)) { return $false }
    if ($RelativePath.Contains("\")) { return $false }
    if ($RelativePath.StartsWith("/", [System.StringComparison]::Ordinal)) { return $false }
    if ($RelativePath -match "^[A-Za-z]:") { return $false }
    if ($RelativePath.Contains(":")) { return $false }
    if ($RelativePath -match "(^|/)\.\.(/|$)") { return $false }
    if (-not $RelativePath.StartsWith($ExpectedPrefix, [System.StringComparison]::Ordinal)) { return $false }
    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $artifactRootFull.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    return $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)
}

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) { Write-Error "PowerShell 7 is required for MAVG async-overlap measured performance validation." }

Write-Information "mavg-async-overlap-performance: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/cmake.ps1"), "--preset", "dev")
Write-Information "mavg-async-overlap-performance: building focused runtime RHI test target..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/cmake.ps1"), "--build", "--preset", "dev", "--target", "MK_runtime_rhi_mavg_async_overlap_performance_tests")
Write-Information "mavg-async-overlap-performance: running focused runtime RHI CTest lane..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/ctest.ps1"), "--preset", "dev", "--output-on-failure", "-R", "MK_runtime_rhi_mavg_async_overlap_performance_tests")

$schemaText = Get-Content -LiteralPath (Join-Path $root "schemas/mavg-async-overlap-performance.schema.json") -Raw
foreach ($needle in @("GameEngine.MavgAsyncOverlapPerformanceEvidence.v1", "sync_baseline", "async_scheduler", "profiler_artifact", "pix_timing_capture", "nsight_graphics_gpu_trace", "radeon_gpu_profiler", "internal_engine_counters_from_non_captured_run", "timing_window_only_evidence", "overlap_with_gpu_upload_or_draw_observed")) {
    if (-not $schemaText.Contains($needle)) { Write-Error "MAVG async-overlap performance schema must contain '$needle'." }
}

$evidenceFiles = @()
if (Test-Path -LiteralPath $artifactRootFull -PathType Container) {
    $evidenceFiles = @(Get-ChildItem -LiteralPath $artifactRootFull -Recurse -File -Filter "evidence.json")
}

$validRowCount = 0
$artifactRowCount = 0
$profilerArtifactCount = 0
$traceEventJsonCount = 0
$internalCounterRows = 0
$externalProfilerRows = 0
$comparableRunRows = 0
$profilerOverlapRows = 0
$memoryBudgetRows = 0
$replayHashRows = 0
$pathEscapeCount = 0
$invalidHashCount = 0
$invalidJsonCount = 0
$missingArtifactCount = 0
$duplicateRowCount = 0
$diagnosticCount = 0
$maxFrameP95ImprovementBasisPoints = 0
$maxStallP95ImprovementBasisPoints = 0
$maxP99RegressionBasisPoints = 0
$seenRows = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)

if ($evidenceFiles.Count -eq 0) {
    $missingArtifactCount = 1
}

foreach ($evidenceFile in $evidenceFiles) {
    $rowDiagnostics = [System.Collections.Generic.List[string]]::new()
    $workloadDirectory = Get-ParentPath -Path $evidenceFile.FullName
    $backendDirectory = Get-ParentPath -Path $workloadDirectory
    $taskDirectory = Get-ParentPath -Path $backendDirectory
    $taskIdFromPath = Get-LeafName -Path $taskDirectory
    $backendFromPath = Get-LeafName -Path $backendDirectory
    $workloadFromPath = Get-LeafName -Path $workloadDirectory
    $rowKey = "$taskIdFromPath/$backendFromPath/$workloadFromPath"
    if (-not $seenRows.Add($rowKey)) {
        $duplicateRowCount += 1
        $rowDiagnostics.Add("duplicate_row")
    }

    try {
        $evidence = Get-Content -LiteralPath $evidenceFile.FullName -Raw | ConvertFrom-Json
    } catch {
        $invalidJsonCount += 1
        $diagnosticCount += 1
        continue
    }

    $artifactRowCount += 1
    $taskId = Get-RequiredStringProperty -JsonObject $evidence -Name "task_id" -Diagnostics $rowDiagnostics
    $backend = Get-RequiredStringProperty -JsonObject $evidence -Name "backend" -Diagnostics $rowDiagnostics
    $workload = Get-RequiredStringProperty -JsonObject $evidence -Name "workload_id" -Diagnostics $rowDiagnostics
    $packageHash = Get-RequiredStringProperty -JsonObject $evidence -Name "package_hash_sha256" -Diagnostics $rowDiagnostics
    $cameraScript = Get-RequiredStringProperty -JsonObject $evidence -Name "camera_script_id" -Diagnostics $rowDiagnostics
    $hostClass = Get-RequiredStringProperty -JsonObject $evidence -Name "host_class" -Diagnostics $rowDiagnostics
    $adapterId = Get-RequiredStringProperty -JsonObject $evidence -Name "adapter_id" -Diagnostics $rowDiagnostics
    $driverVersion = Get-RequiredStringProperty -JsonObject $evidence -Name "driver_version" -Diagnostics $rowDiagnostics
    $memoryBudget = Get-RequiredDecimalProperty -JsonObject $evidence -Name "memory_budget_bytes" -Diagnostics $rowDiagnostics
    $syncBaseline = Get-JsonPropertyValue -JsonObject $evidence -Name "sync_baseline"
    $asyncScheduler = Get-JsonPropertyValue -JsonObject $evidence -Name "async_scheduler"
    $profiler = Get-JsonPropertyValue -JsonObject $evidence -Name "profiler_artifact"

    if ($taskId -ne $taskIdFromPath) { $rowDiagnostics.Add("task_id_path_mismatch") }
    if ($backend -ne $backendFromPath) { $rowDiagnostics.Add("backend_path_mismatch") }
    if ($workload -ne $workloadFromPath) { $rowDiagnostics.Add("workload_path_mismatch") }
    if (-not (Test-Sha256Text -Value $packageHash)) { $invalidHashCount += 1; $rowDiagnostics.Add("invalid_package_hash_sha256") }
    if ([string]::IsNullOrWhiteSpace($cameraScript) -or [string]::IsNullOrWhiteSpace($hostClass) -or [string]::IsNullOrWhiteSpace($adapterId) -or [string]::IsNullOrWhiteSpace($driverVersion)) { $rowDiagnostics.Add("missing_identity") }

    foreach ($run in @($syncBaseline, $asyncScheduler)) {
        $runId = Get-RequiredStringProperty -JsonObject $run -Name "run_id" -Diagnostics $rowDiagnostics
        $runWorkload = Get-RequiredStringProperty -JsonObject $run -Name "workload_id" -Diagnostics $rowDiagnostics
        $runPackageHash = Get-RequiredStringProperty -JsonObject $run -Name "package_hash_sha256" -Diagnostics $rowDiagnostics
        $runCameraScript = Get-RequiredStringProperty -JsonObject $run -Name "camera_script_id" -Diagnostics $rowDiagnostics
        $runBackend = Get-RequiredStringProperty -JsonObject $run -Name "backend" -Diagnostics $rowDiagnostics
        $runHostClass = Get-RequiredStringProperty -JsonObject $run -Name "host_class" -Diagnostics $rowDiagnostics
        $runAdapterId = Get-RequiredStringProperty -JsonObject $run -Name "adapter_id" -Diagnostics $rowDiagnostics
        $null = Get-RequiredStringProperty -JsonObject $run -Name "driver_version" -Diagnostics $rowDiagnostics
        $null = Get-RequiredStringProperty -JsonObject $run -Name "replay_hash" -Diagnostics $rowDiagnostics
        $warmupFrameCount = Get-RequiredDecimalProperty -JsonObject $run -Name "warmup_frame_count" -Diagnostics $rowDiagnostics
        $measuredFrameCount = Get-RequiredDecimalProperty -JsonObject $run -Name "measured_frame_count" -Diagnostics $rowDiagnostics
        $internalCounters = Get-RequiredBoolProperty -JsonObject $run -Name "internal_engine_counters_from_non_captured_run" -Diagnostics $rowDiagnostics
        $timingWindowOnly = Get-RequiredBoolProperty -JsonObject $run -Name "timing_window_only_evidence" -Diagnostics $rowDiagnostics
        $null = Get-RequiredDecimalProperty -JsonObject $run -Name "frame_p95_us" -Diagnostics $rowDiagnostics
        $null = Get-RequiredDecimalProperty -JsonObject $run -Name "frame_p99_us" -Diagnostics $rowDiagnostics
        $null = Get-RequiredDecimalProperty -JsonObject $run -Name "streaming_stall_p95_us" -Diagnostics $rowDiagnostics
        $null = Get-RequiredDecimalProperty -JsonObject $run -Name "memory_peak_bytes" -Diagnostics $rowDiagnostics
        if ([string]::IsNullOrWhiteSpace($runId)) { $rowDiagnostics.Add("missing_run_id") }
        if ($runWorkload -ne $workload -or $runPackageHash -ne $packageHash -or $runCameraScript -ne $cameraScript -or $runBackend -ne $backend -or $runHostClass -ne $hostClass -or $runAdapterId -ne $adapterId) { $rowDiagnostics.Add("run_identity_mismatch") }
        if ($null -eq $warmupFrameCount -or $warmupFrameCount -lt 30) { $rowDiagnostics.Add("insufficient_warmup_frames") }
        if ($null -eq $measuredFrameCount -or $measuredFrameCount -lt 120) { $rowDiagnostics.Add("insufficient_measured_frames") }
        if (-not $internalCounters) { $rowDiagnostics.Add("missing_non_captured_internal_counters") }
        if ($timingWindowOnly) { $rowDiagnostics.Add("timing_window_only_evidence") }
    }

    $syncReplayHash = Get-RequiredStringProperty -JsonObject $syncBaseline -Name "replay_hash" -Diagnostics $rowDiagnostics
    $asyncReplayHash = Get-RequiredStringProperty -JsonObject $asyncScheduler -Name "replay_hash" -Diagnostics $rowDiagnostics
    if ($syncReplayHash -eq $asyncReplayHash -and -not [string]::IsNullOrWhiteSpace($syncReplayHash)) { $replayHashRows += 1 } else { $rowDiagnostics.Add("replay_hash_mismatch") }

    $baselineFrameP95 = Get-RequiredDecimalProperty -JsonObject $syncBaseline -Name "frame_p95_us" -Diagnostics $rowDiagnostics
    $asyncFrameP95 = Get-RequiredDecimalProperty -JsonObject $asyncScheduler -Name "frame_p95_us" -Diagnostics $rowDiagnostics
    $baselineFrameP99 = Get-RequiredDecimalProperty -JsonObject $syncBaseline -Name "frame_p99_us" -Diagnostics $rowDiagnostics
    $asyncFrameP99 = Get-RequiredDecimalProperty -JsonObject $asyncScheduler -Name "frame_p99_us" -Diagnostics $rowDiagnostics
    $baselineStallP95 = Get-RequiredDecimalProperty -JsonObject $syncBaseline -Name "streaming_stall_p95_us" -Diagnostics $rowDiagnostics
    $asyncStallP95 = Get-RequiredDecimalProperty -JsonObject $asyncScheduler -Name "streaming_stall_p95_us" -Diagnostics $rowDiagnostics
    $asyncMemoryPeak = Get-RequiredDecimalProperty -JsonObject $asyncScheduler -Name "memory_peak_bytes" -Diagnostics $rowDiagnostics
    if ($null -ne $baselineFrameP95 -and $null -ne $asyncFrameP95) { $maxFrameP95ImprovementBasisPoints = [System.Math]::Max($maxFrameP95ImprovementBasisPoints, (Get-BasisPointImprovement -Baseline $baselineFrameP95 -Optimized $asyncFrameP95)) }
    if ($null -ne $baselineStallP95 -and $null -ne $asyncStallP95) { $maxStallP95ImprovementBasisPoints = [System.Math]::Max($maxStallP95ImprovementBasisPoints, (Get-BasisPointImprovement -Baseline $baselineStallP95 -Optimized $asyncStallP95)) }
    if ($null -ne $baselineFrameP99 -and $null -ne $asyncFrameP99) { $maxP99RegressionBasisPoints = [System.Math]::Max($maxP99RegressionBasisPoints, (Get-BasisPointRegression -Baseline $baselineFrameP99 -Candidate $asyncFrameP99)) }
    if ($null -ne $memoryBudget -and $null -ne $asyncMemoryPeak -and $asyncMemoryPeak -le $memoryBudget) { $memoryBudgetRows += 1 } else { $rowDiagnostics.Add("memory_budget_exceeded") }

    $profilerTool = Get-RequiredStringProperty -JsonObject $profiler -Name "tool" -Diagnostics $rowDiagnostics
    $profilerArtifactPath = Get-RequiredStringProperty -JsonObject $profiler -Name "artifact_path" -Diagnostics $rowDiagnostics
    $traceEventJsonPath = Get-RequiredStringProperty -JsonObject $profiler -Name "trace_event_json_path" -Diagnostics $rowDiagnostics
    $artifactHash = Get-RequiredStringProperty -JsonObject $profiler -Name "artifact_hash_sha256" -Diagnostics $rowDiagnostics
    $profilerReviewed = Get-RequiredBoolProperty -JsonObject $profiler -Name "reviewed" -Diagnostics $rowDiagnostics
    $officialTool = Get-RequiredBoolProperty -JsonObject $profiler -Name "official_tool" -Diagnostics $rowDiagnostics
    $profilerOverhead = Get-RequiredDecimalProperty -JsonObject $profiler -Name "profiler_overhead_basis_points" -Diagnostics $rowDiagnostics
    $captureDuration = Get-RequiredDecimalProperty -JsonObject $profiler -Name "capture_duration_us" -Diagnostics $rowDiagnostics
    $droppedTimestamps = Get-RequiredBoolProperty -JsonObject $profiler -Name "dropped_timestamps_or_overflow" -Diagnostics $rowDiagnostics
    $symbolsAvailable = Get-RequiredBoolProperty -JsonObject $profiler -Name "symbols_or_debug_info_available" -Diagnostics $rowDiagnostics
    $queueTimeline = Get-RequiredBoolProperty -JsonObject $profiler -Name "queue_timeline_available" -Diagnostics $rowDiagnostics
    $ioTimeline = Get-RequiredBoolProperty -JsonObject $profiler -Name "io_timeline_available" -Diagnostics $rowDiagnostics
    $gpuTimeline = Get-RequiredBoolProperty -JsonObject $profiler -Name "gpu_timeline_available" -Diagnostics $rowDiagnostics
    $memoryTimeline = Get-RequiredBoolProperty -JsonObject $profiler -Name "memory_timeline_available" -Diagnostics $rowDiagnostics
    $overlapObserved = Get-RequiredBoolProperty -JsonObject $profiler -Name "overlap_with_gpu_upload_or_draw_observed" -Diagnostics $rowDiagnostics
    $representativeCapture = Get-RequiredBoolProperty -JsonObject $profiler -Name "representative_capture" -Diagnostics $rowDiagnostics
    $null = Get-RequiredStringProperty -JsonObject $profiler -Name "tool_version" -Diagnostics $rowDiagnostics
    $null = Get-RequiredStringProperty -JsonObject $profiler -Name "capture_mode" -Diagnostics $rowDiagnostics

    if (-not $acceptedProfilerTools.Contains($profilerTool)) { $rowDiagnostics.Add("unsupported_profiler_tool") }
    if (-not (Test-Sha256Text -Value $artifactHash)) { $invalidHashCount += 1; $rowDiagnostics.Add("invalid_artifact_hash_sha256") }
    if (-not $profilerReviewed -or -not $officialTool) { $rowDiagnostics.Add("profiler_not_reviewed_or_official") }
    if ($null -eq $profilerOverhead -or $profilerOverhead -gt 500 -or $null -eq $captureDuration -or $captureDuration -le 0 -or $droppedTimestamps -or -not $representativeCapture) { $rowDiagnostics.Add("profiler_capture_not_representative") }
    if (-not $symbolsAvailable -or -not $queueTimeline -or -not $ioTimeline -or -not $gpuTimeline -or -not $memoryTimeline) { $rowDiagnostics.Add("profiler_missing_required_timeline") }
    if (-not $overlapObserved) { $rowDiagnostics.Add("profiler_missing_overlap") }

    $expectedPathPrefix = "$ArtifactRootRelative/$taskId/$backend/$workload/"
    $profilerPathSafe = Test-SafeMavgAsyncOverlapArtifactPath -RelativePath $profilerArtifactPath -ExpectedPrefix $expectedPathPrefix
    $tracePathSafe = Test-SafeMavgAsyncOverlapArtifactPath -RelativePath $traceEventJsonPath -ExpectedPrefix $expectedPathPrefix
    if (-not $profilerPathSafe -or -not $tracePathSafe) {
        $pathEscapeCount += 1
        $rowDiagnostics.Add("artifact_path_escape")
    }
    if ($profilerPathSafe) {
        $profilerFullPath = [System.IO.Path]::GetFullPath((Join-Path $root $profilerArtifactPath))
        if (Test-Path -LiteralPath $profilerFullPath -PathType Leaf) {
            $profilerArtifactCount += 1
            if (Test-Sha256Text -Value $artifactHash) {
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

    $frameP95Ready = $maxFrameP95ImprovementBasisPoints -ge 500
    $stallP95Ready = $maxStallP95ImprovementBasisPoints -ge 2000
    $p99Ready = $maxP99RegressionBasisPoints -le 200
    if (-not $frameP95Ready) { $rowDiagnostics.Add("insufficient_frame_p95_improvement") }
    if (-not $stallP95Ready) { $rowDiagnostics.Add("insufficient_stall_p95_improvement") }
    if (-not $p99Ready) { $rowDiagnostics.Add("excessive_p99_regression") }

    if ($rowDiagnostics.Count -eq 0) {
        $validRowCount += 1
        $internalCounterRows += 1
        $externalProfilerRows += 1
        $comparableRunRows += 1
        $profilerOverlapRows += 1
    } else {
        $diagnosticCount += $rowDiagnostics.Count
    }
}

$ready = $validRowCount -ge 1 -and $profilerArtifactCount -ge 1 -and $traceEventJsonCount -ge 1 -and $internalCounterRows -ge 1 -and $externalProfilerRows -ge 1 -and $comparableRunRows -ge 1 -and $profilerOverlapRows -ge 1 -and $memoryBudgetRows -ge 1 -and $replayHashRows -ge 1 -and $pathEscapeCount -eq 0 -and $invalidHashCount -eq 0 -and $invalidJsonCount -eq 0 -and $missingArtifactCount -eq 0 -and $duplicateRowCount -eq 0 -and $maxFrameP95ImprovementBasisPoints -ge 500 -and $maxStallP95ImprovementBasisPoints -ge 2000 -and $maxP99RegressionBasisPoints -le 200
$status = if ($ready) { "ready" } else { "host_evidence_required" }

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-async-overlap-performance")
$lines.Add("artifact_root=$ArtifactRootRelative")
$lines.Add("mavg_async_overlap_measured_performance_status=$status")
$lines.Add("mavg_async_overlap_measured_performance_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_async_overlap_measured_performance_artifact_rows=$artifactRowCount")
$lines.Add("mavg_async_overlap_measured_performance_valid_rows=$validRowCount")
$lines.Add("mavg_async_overlap_measured_performance_internal_counter_rows=$internalCounterRows")
$lines.Add("mavg_async_overlap_measured_performance_external_profiler_rows=$externalProfilerRows")
$lines.Add("mavg_async_overlap_measured_performance_profiler_artifacts=$profilerArtifactCount")
$lines.Add("mavg_async_overlap_measured_performance_trace_event_json=$traceEventJsonCount")
$lines.Add("mavg_async_overlap_measured_performance_comparable_run_rows=$comparableRunRows")
$lines.Add("mavg_async_overlap_measured_performance_profiler_overlap_rows=$profilerOverlapRows")
$lines.Add("mavg_async_overlap_measured_performance_memory_budget_rows=$memoryBudgetRows")
$lines.Add("mavg_async_overlap_measured_performance_replay_hash_rows=$replayHashRows")
$lines.Add("mavg_async_overlap_measured_performance_frame_p95_improvement_basis_points=$maxFrameP95ImprovementBasisPoints")
$lines.Add("mavg_async_overlap_measured_performance_stall_p95_improvement_basis_points=$maxStallP95ImprovementBasisPoints")
$lines.Add("mavg_async_overlap_measured_performance_p99_regression_basis_points=$maxP99RegressionBasisPoints")
$lines.Add("mavg_async_overlap_measured_performance_missing_artifacts=$missingArtifactCount")
$lines.Add("mavg_async_overlap_measured_performance_invalid_hashes=$invalidHashCount")
$lines.Add("mavg_async_overlap_measured_performance_path_escapes=$pathEscapeCount")
$lines.Add("mavg_async_overlap_measured_performance_invalid_json=$invalidJsonCount")
$lines.Add("mavg_async_overlap_measured_performance_duplicate_rows=$duplicateRowCount")
$lines.Add("mavg_async_overlap_measured_performance_diagnostics=$diagnosticCount")
$lines.Add("mavg_async_overlap_measured_performance_timing_window_only=0")
$lines.Add("mavg_async_overlap_measured_performance_native_handles_exposed=0")
$lines.Add("mavg_async_overlap_measured_performance_gpu_directstorage_destinations=0")
$lines.Add("mavg_async_overlap_measured_performance_gdeflate=0")
$lines.Add("mavg_async_overlap_measured_performance_mesh_shader_execution=0")
$lines.Add("mavg_async_overlap_measured_performance_metal_readiness=0")
$lines.Add("mavg_async_overlap_measured_performance_nanite_equivalence=0")
$lines.Add("mavg_async_overlap_measured_performance_broad_optimization=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) { Write-Error "Expected evidence counter not emitted: $expected" }
}
foreach ($line in $lines) { Write-Output $line }

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG async-overlap measured performance proof is incomplete; retained internal counters plus reviewed official profiler artifact evidence are required before mavg_async_overlap_measured_performance_ready can be 1."
}

Write-Information "mavg-async-overlap-performance-check: ok" -InformationAction Continue
