#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Import")]
    [string]$Mode = "Plan",

    [string]$EvidenceRoot = "artifacts/performance/optional-gpu-compute-review",

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$CandidateId,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$Classification,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$SelectedBackend,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$WorkloadRecipe,

    [string]$DataTransferCost = "operator-reviewed",

    [string]$MemoryResidency = "operator-reviewed",

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$SynchronizationEvidence,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$StreamEventUsage,

    [string]$QueueProfilerVisibility = "operator-reviewed",

    [string]$DependencyBurden = "operator-reviewed",

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$DependencyInstallGate,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$ScalarOrRhiFallback,

    [string]$ProfilerArtifactRelativePath = "",

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

function Get-HostLabel {
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return "windows"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Linux)) {
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
$review = $manifest.aiOperableProductionLoop.optionalGpuComputeReview
$knownCandidates = @{}
foreach ($candidate in @($review.candidateRows)) {
    $knownCandidates[[string]$candidate.id] = $candidate
}
$knownClassifications = @($review.classifications | ForEach-Object { [string]$_ })

if (-not $knownCandidates.ContainsKey($CandidateId)) {
    Write-Error "Unknown optional GPU compute candidate id: $CandidateId"
}
if ($knownClassifications -notcontains $Classification) {
    Write-Error "Unknown optional GPU compute classification: $Classification"
}
if ([string]$knownCandidates[$CandidateId].classification -ne $Classification) {
    Write-Error "Classification mismatch for ${CandidateId}: expected $($knownCandidates[$CandidateId].classification), got $Classification"
}

$evidenceRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $EvidenceRoot))
$evidenceDirectoryRelative = "$EvidenceRoot/$CandidateId"
$evidencePathRelative = "$evidenceDirectoryRelative/evidence.json"
$evidenceDirectoryFull = [System.IO.Path]::GetFullPath((Join-Path $root $evidenceDirectoryRelative))
$evidencePathFull = [System.IO.Path]::GetFullPath((Join-Path $root $evidencePathRelative))
$willWrite = $Mode -eq "Import" -and -not $NoWrite.IsPresent
$requiresProfilerArtifact = $Classification -ne "non_goal"

$toolPaths = [ordered]@{
    dxc = Get-CommandPathOrEmpty "dxc"
    spirv_val = Get-CommandPathOrEmpty "spirv-val"
    vulkaninfo = Get-CommandPathOrEmpty "vulkaninfo"
    pix = Get-CommandPathOrEmpty "pix"
    nvidia_smi = Get-CommandPathOrEmpty "nvidia-smi"
    nvcc = Get-CommandPathOrEmpty "nvcc"
    nsys = Get-CommandPathOrEmpty "nsys"
    ncu = Get-CommandPathOrEmpty "ncu"
    hipcc = Get-CommandPathOrEmpty "hipcc"
    rocprof = Get-CommandPathOrEmpty "rocprof"
    rocprofv3 = Get-CommandPathOrEmpty "rocprofv3"
    sycl_ls = Get-CommandPathOrEmpty "sycl-ls"
}

if ($Mode -eq "Import") {
    if ($requiresProfilerArtifact) {
        Resolve-RequiredArtifact `
            -RelativePath $ProfilerArtifactRelativePath `
            -EvidenceRootFull $evidenceRootFull `
            -Label "ProfilerArtifactRelativePath"
    } elseif (-not [string]::IsNullOrWhiteSpace($ProfilerArtifactRelativePath)) {
        Resolve-RequiredArtifact `
            -RelativePath $ProfilerArtifactRelativePath `
            -EvidenceRootFull $evidenceRootFull `
            -Label "ProfilerArtifactRelativePath"
    }

    $profilerArtifacts = @()
    if (-not [string]::IsNullOrWhiteSpace($ProfilerArtifactRelativePath)) {
        $profilerArtifacts += [ordered]@{
            id = "profiler-summary"
            path = $ProfilerArtifactRelativePath
        }
    }

    $evidence = [ordered]@{
        candidate_id = $CandidateId
        classification = $Classification
        selected_backend = $SelectedBackend
        workload_recipe = $WorkloadRecipe
        data_transfer_cost = $DataTransferCost
        memory_residency = $MemoryResidency
        synchronization = $SynchronizationEvidence
        stream_event_usage = $StreamEventUsage
        queue_profiler_visibility = $QueueProfilerVisibility
        dependency_burden = $DependencyBurden
        dependency_install_gate = $DependencyInstallGate
        scalar_or_rhi_fallback = $ScalarOrRhiFallback
        profiler_artifacts = $profilerArtifacts
        host = Get-HostLabel
        os_kernel_or_build = [System.Runtime.InteropServices.RuntimeInformation]::OSDescription
        tool_availability = $toolPaths
        default_validation_dependency = 0
        vcpkg_feature_added = 0
        cmake_linkage_added = 0
        runtime_dependency_added = 0
        public_native_handles_exposed = 0
        broad_gpu_compute_claim = 0
        async_overlap_claim = 0
        cross_vendor_parity_claim = 0
        cross_backend_parity_claim = 0
        broad_optimization_claim = 0
    }

    if ($willWrite) {
        $null = New-Item -ItemType Directory -Path $evidenceDirectoryFull -Force
        $evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $evidencePathFull -Encoding utf8NoBOM
    }
}

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=host-optional-gpu-compute-review")
$lines.Add("optional_gpu_compute_review_collector_mode=$Mode")
$lines.Add("optional_gpu_compute_review_collector_host=$(Get-HostLabel)")
$lines.Add("optional_gpu_compute_review_collector_candidate=$CandidateId")
$lines.Add("optional_gpu_compute_review_collector_classification=$Classification")
$lines.Add("optional_gpu_compute_review_collector_backend=$SelectedBackend")
$lines.Add("optional_gpu_compute_review_collector_plan_ready=1")
$lines.Add("optional_gpu_compute_review_collector_writes_evidence=$(ConvertTo-CounterBit $willWrite)")
$lines.Add("optional_gpu_compute_review_collector_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $evidencePathFull -PathType Leaf)))")
$lines.Add("optional_gpu_compute_review_collector_evidence_path=$evidencePathRelative")
$lines.Add("optional_gpu_compute_review_collector_dxc_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.dxc)))")
$lines.Add("optional_gpu_compute_review_collector_spirv_val_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.spirv_val)))")
$lines.Add("optional_gpu_compute_review_collector_vulkaninfo_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.vulkaninfo)))")
$lines.Add("optional_gpu_compute_review_collector_pix_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.pix)))")
$lines.Add("optional_gpu_compute_review_collector_nvidia_smi_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.nvidia_smi)))")
$lines.Add("optional_gpu_compute_review_collector_nvcc_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.nvcc)))")
$lines.Add("optional_gpu_compute_review_collector_nsys_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.nsys)))")
$lines.Add("optional_gpu_compute_review_collector_ncu_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.ncu)))")
$lines.Add("optional_gpu_compute_review_collector_hipcc_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.hipcc)))")
$lines.Add("optional_gpu_compute_review_collector_rocprof_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.rocprof)))")
$lines.Add("optional_gpu_compute_review_collector_rocprofv3_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.rocprofv3)))")
$lines.Add("optional_gpu_compute_review_collector_sycl_ls_ready=$(ConvertTo-CounterBit (-not [string]::IsNullOrWhiteSpace($toolPaths.sycl_ls)))")
$lines.Add("optional_gpu_compute_review_collector_default_validation_dependency=0")
$lines.Add("optional_gpu_compute_review_collector_vcpkg_feature_added=0")
$lines.Add("optional_gpu_compute_review_collector_cmake_linkage_added=0")
$lines.Add("optional_gpu_compute_review_collector_runtime_dependency_added=0")
$lines.Add("optional_gpu_compute_review_collector_public_native_handles_exposed=0")
$lines.Add("optional_gpu_compute_review_collector_broad_gpu_compute_claim=0")
$lines.Add("optional_gpu_compute_review_collector_async_overlap_claim=0")
$lines.Add("optional_gpu_compute_review_collector_cross_vendor_parity_claim=0")
$lines.Add("optional_gpu_compute_review_collector_cross_backend_parity_claim=0")
$lines.Add("optional_gpu_compute_review_collector_broad_optimization_claim=0")

foreach ($line in $lines) {
    Write-Output $line
}

Write-Information "optional-gpu-compute-review-collector: ok" -InformationAction Continue
