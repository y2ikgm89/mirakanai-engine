#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Import")]
    [string]$Mode = "Plan",

    [string]$EvidenceRoot = "artifacts/performance/cpu-profiling-matrix",

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$HostClassId,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$TraceRecipeId,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$WorkloadRecipe,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$CompilerAndFlags,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$SelectedSimdLane,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$ThermalPowerState,

    [string]$BaselineArtifactRelativePath = "",

    [string]$CandidateArtifactRelativePath = "",

    [string]$ProfilerArtifactRelativePath = "",

    [decimal]$FrameTimeBudget = 0,

    [decimal]$MemoryGrowthBudget = 0,

    [decimal]$QueueWaitBudget = 0,

    [decimal]$CacheMissBudget = 0,

    [decimal]$BranchMissBudget = 0,

    [decimal]$MemoryBandwidthBudget = 0,

    [decimal]$DiagnosticsBudget = 0,

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

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

function Get-CommandPathOrEmpty {
    param([Parameter(Mandatory = $true)][string]$Command)

    $commandPath = Find-CommandOnCombinedPath $Command
    if ($commandPath) {
        return [string]$commandPath
    }
    return ""
}

function Get-WprPmcSourceSummary {
    $wpr = Find-CommandOnCombinedPath "wpr"
    if (-not $wpr) {
        return "wpr_not_found"
    }

    $output = & $wpr "-pmcsources" 2>&1
    if ($LASTEXITCODE -ne 0) {
        return "wpr_pmc_sources_unavailable"
    }

    $lines = @($output | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } | Select-Object -First 8)
    if ($lines.Count -eq 0) {
        return "wpr_pmc_sources_empty"
    }
    return ($lines -join "; ")
}

function Get-HostCpuFacts {
    $processorName = [System.Runtime.InteropServices.RuntimeInformation]::ProcessArchitecture.ToString()
    $socketCount = "1"
    $physicalCoreCount = "unknown"
    $logicalProcessorCount = [string][Environment]::ProcessorCount
    $numaNodeCount = "unknown"

    if (Test-HostIsWindows) {
        try {
            $processors = @(Get-CimInstance -ClassName Win32_Processor -ErrorAction Stop)
            if ($processors.Count -gt 0) {
                $processorName = [string]$processors[0].Name
                $socketCount = [string]$processors.Count
                $physicalCoreCount = [string](($processors | Measure-Object -Property NumberOfCores -Sum).Sum)
                $logicalProcessorCount = [string](($processors | Measure-Object -Property NumberOfLogicalProcessors -Sum).Sum)
            }
        } catch {
            $physicalCoreCount = "unknown"
        }

        try {
            $numaNodes = @(Get-CimInstance -ClassName Win32_NumaNode -ErrorAction Stop)
            if ($numaNodes.Count -gt 0) {
                $numaNodeCount = [string]$numaNodes.Count
            }
        } catch {
            $numaNodeCount = "unknown"
        }
    } elseif (Test-HostIsLinux -and (Test-Path -LiteralPath "/proc/cpuinfo" -PathType Leaf)) {
        $cpuInfo = Get-Content -LiteralPath "/proc/cpuinfo" -ErrorAction SilentlyContinue
        $modelLine = @($cpuInfo | Where-Object { [string]$_ -like "model name*" } | Select-Object -First 1)
        if ($modelLine.Count -gt 0) {
            $parts = ([string]$modelLine[0]).Split(":", 2)
            if ($parts.Count -eq 2) {
                $processorName = $parts[1].Trim()
            }
        }
    }

    $smtState = "unknown"
    try {
        if ([int]$physicalCoreCount -gt 0 -and [int]$logicalProcessorCount -gt [int]$physicalCoreCount) {
            $smtState = "enabled"
        } elseif ([int]$physicalCoreCount -gt 0) {
            $smtState = "disabled"
        }
    } catch {
        $smtState = "unknown"
    }

    return [pscustomobject]@{
        ExactCpuModel = $processorName
        SocketCount = $socketCount
        PhysicalCoreCount = $physicalCoreCount
        LogicalProcessorCount = $logicalProcessorCount
        SmtState = $smtState
        ProcessorGroups = "default"
        NumaNodeCount = $numaNodeCount
        NpsState = "not-collected"
        Topology = "sockets=$socketCount;physical_cores=$physicalCoreCount;logical_processors=$logicalProcessorCount;numa_nodes=$numaNodeCount"
    }
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

function Resolve-RequiredArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$EvidenceRootFull,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-SafeEvidencePath -RelativePath $RelativePath -EvidenceRootFull $EvidenceRootFull)) {
        Write-Error "$Label must be a safe repo-relative path under $EvidenceRoot"
    }

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        Write-Error "$Label does not exist: $RelativePath"
    }
}

$manifest = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw | ConvertFrom-Json
$matrix = $manifest.aiOperableProductionLoop.cpuProfilingMatrix
$knownHostClassIds = @($matrix.hostClasses | ForEach-Object { [string]$_.id })
$knownTraceRecipeIds = @($matrix.traceRecipes | ForEach-Object { [string]$_.id })

if ($knownHostClassIds -notcontains $HostClassId) {
    Write-Error "Unknown CPU profiling host class id: $HostClassId"
}
if ($knownTraceRecipeIds -notcontains $TraceRecipeId) {
    Write-Error "Unknown CPU profiling trace recipe id: $TraceRecipeId"
}

$evidenceRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $EvidenceRoot))
$evidenceDirectoryRelative = "$EvidenceRoot/$HostClassId/$TraceRecipeId"
$evidencePathRelative = "$evidenceDirectoryRelative/evidence.json"
$evidenceDirectoryFull = [System.IO.Path]::GetFullPath((Join-Path $root $evidenceDirectoryRelative))
$evidencePathFull = [System.IO.Path]::GetFullPath((Join-Path $root $evidencePathRelative))
$willWrite = $Mode -eq "Import" -and -not $NoWrite.IsPresent

$wprPath = Get-CommandPathOrEmpty "wpr"
$wpaPath = Get-CommandPathOrEmpty "wpa"
$xperfPath = Get-CommandPathOrEmpty "xperf"
$perfPath = Get-CommandPathOrEmpty "perf"
$vtunePath = Get-CommandPathOrEmpty "vtune"
$amdUprofPath = Get-CommandPathOrEmpty "AMDuProfCLI"
$counterSet = Get-WprPmcSourceSummary

if ($Mode -eq "Import") {
    Resolve-RequiredArtifact -RelativePath $BaselineArtifactRelativePath -EvidenceRootFull $evidenceRootFull -Label "BaselineArtifactRelativePath"
    Resolve-RequiredArtifact -RelativePath $CandidateArtifactRelativePath -EvidenceRootFull $evidenceRootFull -Label "CandidateArtifactRelativePath"
    Resolve-RequiredArtifact -RelativePath $ProfilerArtifactRelativePath -EvidenceRootFull $evidenceRootFull -Label "ProfilerArtifactRelativePath"

    $cpuFacts = Get-HostCpuFacts
    $profilerTools = @(
        "wpr=$wprPath",
        "wpa=$wpaPath",
        "xperf=$xperfPath",
        "perf=$perfPath",
        "vtune=$vtunePath",
        "amduprof=$amdUprofPath"
    ) -join "; "

    $evidence = [ordered]@{
        host_class_id = $HostClassId
        trace_recipe_id = $TraceRecipeId
        exact_cpu_model = $cpuFacts.ExactCpuModel
        os_kernel_or_build = [System.Runtime.InteropServices.RuntimeInformation]::OSDescription
        topology = $cpuFacts.Topology
        socket_count = $cpuFacts.SocketCount
        physical_core_count = $cpuFacts.PhysicalCoreCount
        logical_processor_count = $cpuFacts.LogicalProcessorCount
        smt_state = $cpuFacts.SmtState
        processor_groups = $cpuFacts.ProcessorGroups
        numa_node_count = $cpuFacts.NumaNodeCount
        nps_state = $cpuFacts.NpsState
        scheduler_context = "os-default"
        compiler_and_flags = $CompilerAndFlags
        selected_simd_lane = $SelectedSimdLane
        thermal_power_state = $ThermalPowerState
        profiler_name_version = $profilerTools
        counter_set = $counterSet
        workload_recipe = $WorkloadRecipe
        trace_artifact_id = "baseline,candidate"
        trace_artifacts = @(
            [ordered]@{ id = "baseline"; path = $BaselineArtifactRelativePath },
            [ordered]@{ id = "candidate"; path = $CandidateArtifactRelativePath }
        )
        profiler_artifacts = @(
            [ordered]@{ id = "profiler-summary"; path = $ProfilerArtifactRelativePath }
        )
        before_after_trace_pair = [ordered]@{
            baselineArtifact = "baseline"
            candidateArtifact = "candidate"
            frame_p95_p99_max = "operator-reviewed"
            over_budget_frames = "operator-reviewed"
            worker_waits = "operator-reviewed"
            cache_misses = "operator-reviewed"
            branch_misses = "operator-reviewed"
            memory_bandwidth = "operator-reviewed"
            local_remote_numa_ratio = "operator-reviewed"
            diagnostics_count = "operator-reviewed"
        }
        regression_budget = [ordered]@{
            frame_time_budget = $FrameTimeBudget
            memory_growth_budget = $MemoryGrowthBudget
            queue_wait_budget = $QueueWaitBudget
            cache_miss_budget = $CacheMissBudget
            branch_miss_budget = $BranchMissBudget
            memory_bandwidth_budget = $MemoryBandwidthBudget
            diagnostics_budget = $DiagnosticsBudget
        }
        linux_affinity_execution = 0
        numa_execution = 0
        pgo_lto_changed = 0
        data_layout_rewrite = 0
        broad_optimization = 0
        gpu_compute_claim = 0
    }

    if ($willWrite) {
        $null = New-Item -ItemType Directory -Path $evidenceDirectoryFull -Force
        $evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $evidencePathFull -Encoding utf8NoBOM
    }
}

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=host-cpu-profiling-matrix")
$lines.Add("cpu_profiling_host_evidence_collector_mode=$Mode")
$lines.Add("cpu_profiling_host_evidence_collector_host=$(Get-HostLabel)")
$lines.Add("cpu_profiling_host_evidence_collector_host_class=$HostClassId")
$lines.Add("cpu_profiling_host_evidence_collector_trace_recipe=$TraceRecipeId")
$lines.Add("cpu_profiling_host_evidence_collector_plan_ready=1")
$lines.Add("cpu_profiling_host_evidence_collector_writes_evidence=$(ConvertTo-CounterBit $willWrite)")
$lines.Add("cpu_profiling_host_evidence_collector_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $evidencePathFull -PathType Leaf)))")
$lines.Add("cpu_profiling_host_evidence_collector_evidence_path=$evidencePathRelative")
$lines.Add("cpu_profiling_host_evidence_collector_wpr_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($wprPath)))")
$lines.Add("cpu_profiling_host_evidence_collector_wpa_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($wpaPath)))")
$lines.Add("cpu_profiling_host_evidence_collector_xperf_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($xperfPath)))")
$lines.Add("cpu_profiling_host_evidence_collector_perf_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($perfPath)))")
$lines.Add("cpu_profiling_host_evidence_collector_vtune_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($vtunePath)))")
$lines.Add("cpu_profiling_host_evidence_collector_amduprof_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($amdUprofPath)))")
$lines.Add("cpu_profiling_host_evidence_collector_linux_affinity_execution=0")
$lines.Add("cpu_profiling_host_evidence_collector_numa_execution=0")
$lines.Add("cpu_profiling_host_evidence_collector_pgo_lto_changed=0")
$lines.Add("cpu_profiling_host_evidence_collector_data_layout_rewrite=0")
$lines.Add("cpu_profiling_host_evidence_collector_broad_optimization=0")
$lines.Add("cpu_profiling_host_evidence_collector_gpu_compute_claim=0")

foreach ($line in $lines) {
    Write-Output $line
}

Write-Information "cpu-profiling-host-evidence-collector: ok" -InformationAction Continue
