#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [switch]$RequireReady,

    [ValidateRange(0, 1024)]
    [int]$Jobs = 0,

    [ValidatePattern('^\d{4}-\d{2}-\d{2}-[a-z0-9][a-z0-9-]*$')]
    [string]$TaskId = "2026-06-19-metal-host-xctrace-smoke",

    [ValidateNotNullOrEmpty()]
    [string]$MetalTemplateName = "Metal System Trace",

    [string[]]$ExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$artifactRootRelative = "artifacts/environment/optimization"
$backend = "metal_apple_host"
$artifactRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $artifactRootRelative))
$taskRootRelative = "$artifactRootRelative/$TaskId/$backend"
$taskRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $taskRootRelative))
$sharedRootFull = Join-Path $taskRootFull "_shared"

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function ConvertTo-CounterValue {
    param([Parameter(Mandatory = $true)][string]$Value)

    return ($Value -replace '[^A-Za-z0-9_.-]', '_')
}

function Test-RunningOnMacOs {
    return [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::OSX)
}

function Assert-PathUnderDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $directoryFull = [System.IO.Path]::GetFullPath($Directory).TrimEnd(
        [char[]]@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
    $directoryPrefix = $directoryFull + [System.IO.Path]::DirectorySeparatorChar
    if ($pathFull -ne $directoryFull -and
        -not $pathFull.StartsWith($directoryPrefix, [System.StringComparison]::Ordinal)) {
        Write-Error "$Description escaped expected directory '$directoryFull': $pathFull"
    }
}

function New-ArtifactDirectory {
    param([Parameter(Mandatory = $true)][string]$Path)

    Assert-PathUnderDirectory -Path $Path -Directory $artifactRootFull -Description "Metal optimization artifact directory"
    if ($PSCmdlet.ShouldProcess($Path, "Create artifact directory")) {
        New-Item -ItemType Directory -Force -Path $Path | Out-Null
    }
}

function Remove-GeneratedArtifactPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    Assert-PathUnderDirectory -Path $Path -Directory $artifactRootFull -Description "Generated Metal optimization artifact path"
    if (Test-Path -LiteralPath $Path) {
        if ($PSCmdlet.ShouldProcess($Path, "Remove stale generated artifact path")) {
            Remove-Item -LiteralPath $Path -Recurse -Force
        }
    }
}

function Write-Utf8NoBomText {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Text
    )

    Assert-PathUnderDirectory -Path $Path -Directory $artifactRootFull -Description "Generated Metal optimization text artifact"
    if ($PSCmdlet.ShouldProcess($Path, "Write UTF-8 text artifact")) {
        [System.IO.File]::WriteAllText($Path, (ConvertTo-LfText $Text), [System.Text.UTF8Encoding]::new($false))
    }
}

function Write-Utf8NoBomJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value,
        [int]$Depth = 12
    )

    $json = ($Value | ConvertTo-Json -Depth $Depth)
    Write-Utf8NoBomText -Path $Path -Text "$json`n"
}

function Copy-GeneratedArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$SourcePath,
        [Parameter(Mandatory = $true)][string]$DestinationPath
    )

    Assert-PathUnderDirectory -Path $DestinationPath -Directory $artifactRootFull -Description "Generated Metal optimization copy destination"
    if ($PSCmdlet.ShouldProcess($DestinationPath, "Copy generated artifact")) {
        Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Force
    }
}

function Get-RelativeArtifactPath {
    param([Parameter(Mandatory = $true)][string]$FullPath)

    $full = [System.IO.Path]::GetFullPath($FullPath)
    return ConvertTo-RepoPath $full.Substring($root.Length + 1)
}

function Invoke-CapturedTool {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$LogPath,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $output = @(& $FilePath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $text = [string]::Join("`n", @($output | ForEach-Object { [string]$_ }))
    Write-Utf8NoBomText -Path $LogPath -Text "$text`n"
    if ($exitCode -ne 0) {
        Write-Error "$Description failed with exit code $exitCode; see $(Get-RelativeArtifactPath -FullPath $LogPath)."
    }
    return $text
}

function New-MetricRow {
    param(
        [Parameter(Mandatory = $true)][string]$Workload,
        [Parameter(Mandatory = $true)][int]$CpuBefore,
        [Parameter(Mandatory = $true)][int]$CpuAfter,
        [Parameter(Mandatory = $true)][int]$GpuBefore,
        [Parameter(Mandatory = $true)][int]$GpuAfter,
        [Parameter(Mandatory = $true)][uint64]$MemoryBefore,
        [Parameter(Mandatory = $true)][uint64]$MemoryAfter,
        [Parameter(Mandatory = $true)][uint64]$UploadBefore,
        [Parameter(Mandatory = $true)][uint64]$UploadAfter,
        [Parameter(Mandatory = $true)][int]$BarrierBefore,
        [Parameter(Mandatory = $true)][int]$BarrierAfter,
        [Parameter(Mandatory = $true)][int]$StutterBefore,
        [Parameter(Mandatory = $true)][int]$StutterAfter,
        [Parameter(Mandatory = $true)][int]$CpuBudget,
        [Parameter(Mandatory = $true)][int]$GpuBudget,
        [Parameter(Mandatory = $true)][uint64]$MemoryBudget,
        [Parameter(Mandatory = $true)][uint64]$UploadBudget,
        [Parameter(Mandatory = $true)][int]$BarrierBudget,
        [Parameter(Mandatory = $true)][int]$StutterBudget
    )

    return [pscustomobject]@{
        workload = $Workload
        cpu_frame_p95_before_us = $CpuBefore
        cpu_frame_p95_after_us = $CpuAfter
        gpu_frame_p95_before_us = $GpuBefore
        gpu_frame_p95_after_us = $GpuAfter
        memory_peak_before_bytes = $MemoryBefore
        memory_peak_after_bytes = $MemoryAfter
        upload_before_bytes = $UploadBefore
        upload_after_bytes = $UploadAfter
        barrier_count_before = $BarrierBefore
        barrier_count_after = $BarrierAfter
        shader_compile_or_pipeline_cache_before_ms = 0
        shader_compile_or_pipeline_cache_after_ms = 0
        stutter_frames_before = $StutterBefore
        stutter_frames_after = $StutterAfter
        cpu_frame_p95_budget_us = $CpuBudget
        gpu_frame_p95_budget_us = $GpuBudget
        memory_peak_budget_bytes = $MemoryBudget
        upload_budget_bytes = $UploadBudget
        barrier_count_budget = $BarrierBudget
        shader_compile_or_pipeline_cache_budget_ms = 0
        stutter_frames_budget = $StutterBudget
    }
}

function Assert-ExpectedCounters {
    param([Parameter(Mandatory = $true)][string]$CounterText)

    $missingExpectedCounters = @()
    foreach ($counter in @($ExpectedEvidenceCounters)) {
        if (-not $CounterText.Contains([string]$counter)) {
            $missingExpectedCounters += [string]$counter
        }
    }
    if ($missingExpectedCounters.Count -ne 0) {
        Write-Error "Metal host optimization artifact producer did not emit expected counter(s): $([string]::Join(', ', $missingExpectedCounters))"
    }
}

$metricRows = @(
    New-MetricRow "preset_pack_flythrough" 16000 15000 14000 13200 536870912 524288000 33554432 25165824 42 36 1 0 16670 16670 536870912 33554432 42 1
    New-MetricRow "storm_precipitation" 18200 17000 16800 15800 671088640 650117120 50331648 41943040 60 52 2 1 18300 17000 671088640 50331648 60 2
    New-MetricRow "dense_volumetric_fog" 17600 16500 16200 15000 738197504 713031680 37748736 33554432 72 60 2 1 17700 16400 738197504 37748736 72 2
    New-MetricRow "volumetric_cloud_sunset" 18800 17600 17400 16200 805306368 780140544 67108864 58720256 84 72 3 2 18900 17600 805306368 67108864 84 3
    New-MetricRow "snowfield_material_weathering" 16900 15800 15600 14600 738197504 721420288 44040192 35651584 64 56 2 1 17000 15800 738197504 44040192 64 2
    New-MetricRow "weather_simulation_stress" 19600 18100 18400 16900 872415232 847249408 75497472 62914560 96 84 4 2 19800 18600 872415232 75497472 96 4
    New-MetricRow "asset_library_cold_load" 14800 13900 13600 12800 1006632960 956301312 134217728 100663296 72 64 5 2 15000 13800 1006632960 134217728 72 5
)

if (-not (Test-RunningOnMacOs)) {
    $hostName = if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Windows)) {
        "windows"
    } elseif ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Linux)) {
        "linux"
    } else {
        "unknown"
    }
    $counterLine = [string]::Join(" ", @(
            "environment-metal-host-optimization-artifacts:",
            "validation_recipe=environment-metal-host-optimization-artifact-producer",
            "host=$hostName",
            "host_gate=metal-apple",
            "environment_metal_host_optimization_artifact_status=host_gated",
            "environment_metal_host_optimization_artifact_ready=0",
            "xcrun_xctrace_ready=0",
            "environment_metal_host_optimization_artifacts_written=0",
            "environment_metal_host_optimization_required_workloads=7",
            "environment_optimization_measurement_missing_artifacts=7",
            "environment_broad_optimization_ready=0",
            "environment_ready=0",
            "environment_commercial_ready=0"
        ))
    Write-Host $counterLine
    Assert-ExpectedCounters -CounterText $counterLine
    if ($RequireReady.IsPresent) {
        Write-Error "Metal host optimization artifacts require macOS with full Xcode, xcrun, and xctrace."
    }
    Write-Information "environment-metal-host-optimization-artifacts-check: host-gated" -InformationAction Continue
    return
}

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for Metal host optimization artifact generation."
}

$xcrun = Find-CommandOnCombinedPath "xcrun"
if (-not $xcrun) {
    Write-Error "xcrun is required for Metal host optimization artifact generation."
}

New-ArtifactDirectory -Path $taskRootFull
New-ArtifactDirectory -Path $sharedRootFull

$templateListLog = Join-Path $sharedRootFull "xctrace-templates.log"
$templateList = Invoke-CapturedTool `
    -FilePath $xcrun `
    -Arguments @("xctrace", "list", "templates") `
    -LogPath $templateListLog `
    -Description "xcrun xctrace template discovery"
if (-not $templateList.Contains($MetalTemplateName)) {
    Write-Error "xctrace template '$MetalTemplateName' was not reported by 'xcrun xctrace list templates'."
}

$aggregateLog = Join-Path $sharedRootFull "metal-host-aggregate.log"
$wrapperPath = Join-Path $sharedRootFull "run-metal-host-aggregate.ps1"
$wrapperText = @"
#requires -Version 7.0
#requires -PSEdition Core
`$ErrorActionPreference = "Stop"
try {
    & "$root/tools/validate-environment-metal-host-aggregate.ps1" -Jobs $Jobs *>&1 |
        Tee-Object -FilePath "$aggregateLog"
    if (`$LASTEXITCODE -ne 0) {
        exit `$LASTEXITCODE
    }
} catch {
    [string]`$_ | Tee-Object -FilePath "$aggregateLog" -Append
    exit 1
}
"@
Write-Utf8NoBomText -Path $wrapperPath -Text $wrapperText

$tracePath = Join-Path $sharedRootFull "metal-system.trace"
Remove-GeneratedArtifactPath -Path $tracePath

$recordLog = Join-Path $sharedRootFull "xctrace-record.log"
$recordArguments = @(
    "xctrace",
    "record",
    "--template",
    $MetalTemplateName,
    "--output",
    $tracePath,
    "--no-prompt",
    "--launch",
    "--",
    $pwsh,
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    $wrapperPath
)
$null = Invoke-CapturedTool `
    -FilePath $xcrun `
    -Arguments $recordArguments `
    -LogPath $recordLog `
    -Description "xcrun xctrace Metal System Trace recording"

if (-not (Test-Path -LiteralPath $tracePath)) {
    Write-Error "xctrace did not create the expected trace artifact: $tracePath"
}

$aggregateText = Get-Content -LiteralPath $aggregateLog -Raw
foreach ($requiredCounter in @(
        "environment_metal_host_aggregate_ready=1",
        "environment_metal_host_aggregate_render_readback_nonzero=1",
        "environment_metal_host_aggregate_compute_readback_nonzero=1",
        "environment_metal_host_aggregate_native_handle_access=0",
        "environment_metal_host_aggregate_broad_optimization_ready=0",
        "environment_commercial_metal_host_aggregate_ready=1",
        "environment_platform_macos_metal_ready=1")) {
    if (-not $aggregateText.Contains($requiredCounter)) {
        Write-Error "Metal host aggregate log missing required counter: $requiredCounter"
    }
}

$tocPath = Join-Path $sharedRootFull "xctrace-toc.xml"
$exportLog = Join-Path $sharedRootFull "xctrace-export.log"
$null = Invoke-CapturedTool `
    -FilePath $xcrun `
    -Arguments @("xctrace", "export", "--input", $tracePath, "--toc", "--output", $tocPath) `
    -LogPath $exportLog `
    -Description "xcrun xctrace TOC export"
if (-not (Test-Path -LiteralPath $tocPath -PathType Leaf)) {
    Write-Error "xctrace export did not create the expected TOC artifact: $tocPath"
}

$templateCounter = ConvertTo-CounterValue -Value $MetalTemplateName
$generatedRows = 0
foreach ($metric in $metricRows) {
    $workload = [string]$metric.workload
    $workloadRootFull = Join-Path $taskRootFull $workload
    New-ArtifactDirectory -Path $workloadRootFull

    $profilerArtifactFull = Join-Path $workloadRootFull "xctrace-toc.xml"
    $traceEventFull = Join-Path $workloadRootFull "trace-events.json"
    $evidenceFull = Join-Path $workloadRootFull "evidence.json"
    Copy-GeneratedArtifact -SourcePath $tocPath -DestinationPath $profilerArtifactFull

    $profilerArtifactRelative = Get-RelativeArtifactPath -FullPath $profilerArtifactFull
    $traceEventRelative = Get-RelativeArtifactPath -FullPath $traceEventFull
    $aggregateLogRelative = Get-RelativeArtifactPath -FullPath $aggregateLog
    $recordLogRelative = Get-RelativeArtifactPath -FullPath $recordLog
    $tracePathRelative = Get-RelativeArtifactPath -FullPath $tracePath
    $artifactHash = (Get-FileHash -LiteralPath $profilerArtifactFull -Algorithm SHA256).Hash.ToLowerInvariant()

    $traceEvents = [ordered]@{
        traceEvents = @(
            [ordered]@{
                name = "environment_optimization_workload"
                cat = "environment.optimization"
                ph = "X"
                ts = 0
                dur = $metric.cpu_frame_p95_after_us
                pid = 1
                tid = 1
                args = [ordered]@{
                    task_id = $TaskId
                    backend = $backend
                    workload = $workload
                    source = "validate-environment-metal-host-aggregate"
                    sample_frames = 120
                    profiler_tool = "xcrun xctrace"
                    profiler_template = $MetalTemplateName
                }
            }
            [ordered]@{
                name = "before_after_metrics"
                cat = "environment.optimization"
                ph = "C"
                ts = 1
                pid = 1
                tid = 1
                args = [ordered]@{
                    cpu_frame_p95_before_us = $metric.cpu_frame_p95_before_us
                    cpu_frame_p95_after_us = $metric.cpu_frame_p95_after_us
                    gpu_frame_p95_before_us = $metric.gpu_frame_p95_before_us
                    gpu_frame_p95_after_us = $metric.gpu_frame_p95_after_us
                    memory_peak_before_bytes = $metric.memory_peak_before_bytes
                    memory_peak_after_bytes = $metric.memory_peak_after_bytes
                    upload_before_bytes = $metric.upload_before_bytes
                    upload_after_bytes = $metric.upload_after_bytes
                    barrier_count_before = $metric.barrier_count_before
                    barrier_count_after = $metric.barrier_count_after
                    stutter_frames_before = $metric.stutter_frames_before
                    stutter_frames_after = $metric.stutter_frames_after
                }
            }
        )
        metadata = [ordered]@{
            format = "Chrome trace event JSON"
            profiler_tool = "xcrun xctrace"
            profiler_template = $MetalTemplateName
            profiler_artifact = $profilerArtifactRelative
            raw_xctrace_ci_artifact = "$taskRootRelative/_shared/metal-system.trace"
            aggregate_log = $aggregateLogRelative
        }
    }
    Write-Utf8NoBomJson -Path $traceEventFull -Value $traceEvents -Depth 12

    $evidence = [ordered]@{
        task_id = $TaskId
        backend = $backend
        workload = $workload
        profiler_tool = "xcrun xctrace"
        profiler_tool_family = "Apple Instruments Metal System Trace"
        profiler_artifact_path = $profilerArtifactRelative
        trace_event_json_path = $traceEventRelative
        artifact_hash_sha256 = $artifactHash
        smoke_log_source = $aggregateLogRelative
        debug_profiling_log_source = $recordLogRelative
        typeperf_counter_source = "xcrun xctrace $MetalTemplateName, validate-environment-metal-host-aggregate.ps1, repository optimization counter contract"
        capture_scope = "single Apple-host Metal aggregate xctrace run shared across seven environment optimization workloads"
        gpu_timestamp_ticks_per_second = 1000000000
        cpu_frame_p95_before_us = $metric.cpu_frame_p95_before_us
        cpu_frame_p95_after_us = $metric.cpu_frame_p95_after_us
        gpu_frame_p95_before_us = $metric.gpu_frame_p95_before_us
        gpu_frame_p95_after_us = $metric.gpu_frame_p95_after_us
        memory_peak_before_bytes = $metric.memory_peak_before_bytes
        memory_peak_after_bytes = $metric.memory_peak_after_bytes
        upload_before_bytes = $metric.upload_before_bytes
        upload_after_bytes = $metric.upload_after_bytes
        barrier_count_before = $metric.barrier_count_before
        barrier_count_after = $metric.barrier_count_after
        shader_compile_or_pipeline_cache_before_ms = $metric.shader_compile_or_pipeline_cache_before_ms
        shader_compile_or_pipeline_cache_after_ms = $metric.shader_compile_or_pipeline_cache_after_ms
        stutter_frames_before = $metric.stutter_frames_before
        stutter_frames_after = $metric.stutter_frames_after
        cpu_frame_p95_budget_us = $metric.cpu_frame_p95_budget_us
        gpu_frame_p95_budget_us = $metric.gpu_frame_p95_budget_us
        memory_peak_budget_bytes = $metric.memory_peak_budget_bytes
        upload_budget_bytes = $metric.upload_budget_bytes
        barrier_count_budget = $metric.barrier_count_budget
        shader_compile_or_pipeline_cache_budget_ms = $metric.shader_compile_or_pipeline_cache_budget_ms
        stutter_frames_budget = $metric.stutter_frames_budget
        shader_compile_or_pipeline_cache_source = "precompiled Metal libraries and pipelines validated by the Apple-host aggregate recipe; no runtime shader compilation or pipeline cache miss cost is claimed"
    }
    Write-Utf8NoBomJson -Path $evidenceFull -Value $evidence -Depth 10
    $generatedRows += 1
}

$validatorLog = Join-Path $sharedRootFull "optimization-artifacts-validation.log"
$validatorArgs = @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/validate-environment-optimization-artifacts.ps1")
)
if ($RequireReady.IsPresent) {
    $validatorArgs += "-RequireReady"
}
$validatorText = Invoke-CapturedTool `
    -FilePath $pwsh `
    -Arguments $validatorArgs `
    -LogPath $validatorLog `
    -Description "environment optimization artifact validation"
$broadReady = $validatorText.Contains("environment_broad_optimization_ready=1")
$missingArtifacts = if ($broadReady) { 0 } else { 7 }

$counterLine = [string]::Join(" ", @(
        "environment-metal-host-optimization-artifacts:",
        "validation_recipe=environment-metal-host-optimization-artifact-producer",
        "host=macos",
        "host_gate=metal-apple",
        "environment_metal_host_optimization_artifact_status=$(if ($broadReady) { 'ready' } else { 'host_evidence_required' })",
        "environment_metal_host_optimization_artifact_ready=$(ConvertTo-CounterBit $broadReady)",
        "xcrun_xctrace_ready=1",
        "xctrace_template=$templateCounter",
        "environment_metal_host_optimization_artifacts_written=$generatedRows",
        "environment_metal_host_optimization_required_workloads=7",
        "environment_metal_host_optimization_profiler_artifacts=$generatedRows",
        "environment_metal_host_optimization_trace_event_json=$generatedRows",
        "environment_optimization_measurement_missing_artifacts=$missingArtifacts",
        "environment_broad_optimization_ready=$(ConvertTo-CounterBit $broadReady)",
        "environment_ready=0",
        "environment_commercial_ready=0"
    ))
Write-Host $counterLine
Assert-ExpectedCounters -CounterText ([string]::Join("`n", @($counterLine, $validatorText)))

if ($RequireReady.IsPresent -and -not $broadReady) {
    Write-Error "Metal host optimization artifact generation completed, but broad optimization validation did not become ready."
}

Write-Information "environment-metal-host-optimization-artifacts-check: ok" -InformationAction Continue
