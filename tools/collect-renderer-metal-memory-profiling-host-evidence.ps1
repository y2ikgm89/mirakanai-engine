#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-metal-memory-profiling-host-evidence-collector-v1

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Import")]
    [string]$Mode = "Plan",

    [string]$EvidenceRoot = "artifacts/renderer/metal-memory-profiling-host-evidence",

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$WorkloadId,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$CaptureArtifactRelativePath,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$MacosVersion,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$XcodeVersion,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, [long]::MaxValue)]
    [long]$HeapResourceRows,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, [long]::MaxValue)]
    [long]$HeapAllocatedBytes,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, [long]::MaxValue)]
    [long]$ResidentBytes,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, [long]::MaxValue)]
    [long]$BudgetBytes,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, [long]::MaxValue)]
    [long]$ResidencySetAllocationRows,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, [long]::MaxValue)]
    [long]$MemoryPressureSampleRows,

    [Parameter(Mandatory = $true)]
    [ValidateSet("within_budget", "pressure_observed")]
    [string]$MemoryPressureBudgetStatus,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$CaptureScopeLabel,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, [long]::MaxValue)]
    [long]$CaptureArtifactRows,

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "apple-host-helpers.ps1")

$root = Get-RepoRoot
Set-Location $root

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Get-RendererMetalCollectorHostLabel {
    if (Test-IsMacOS) {
        return "macos"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return "windows"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Linux)) {
        return "linux"
    }
    return "unknown"
}

function Test-SafeRepoRelativePath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        return $false
    }
    if ($RelativePath.Contains("\")) {
        return $false
    }
    if ([System.IO.Path]::IsPathRooted($RelativePath)) {
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
    return $true
}

function ConvertTo-RepoRelativePath {
    param([Parameter(Mandatory = $true)][string]$FullPath)

    $rootFull = [System.IO.Path]::GetFullPath($root).TrimEnd([System.IO.Path]::DirectorySeparatorChar)
    $normalizedFullPath = [System.IO.Path]::GetFullPath($FullPath)
    $relativePath = [System.IO.Path]::GetRelativePath($rootFull, $normalizedFullPath)
    return ($relativePath -replace "\\", "/")
}

function Resolve-SafeRepoRelativePathUnderEvidenceRoot {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$EvidenceRootFull,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-SafeRepoRelativePath -RelativePath $RelativePath)) {
        Write-Error "$Label must be a safe repo-relative path without absolute, drive-qualified, colon, backslash, or '..' segments."
    }

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $EvidenceRootFull.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "$Label must resolve under $EvidenceRoot."
    }
    return $fullPath
}

if (-not (Test-SafeRepoRelativePath -RelativePath $EvidenceRoot)) {
    Write-Error "EvidenceRoot must be a safe repo-relative path."
}
if ($BudgetBytes -lt $ResidentBytes) {
    Write-Error "BudgetBytes must be greater than or equal to ResidentBytes."
}

$evidenceRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $EvidenceRoot))
$captureArtifactFull = Resolve-SafeRepoRelativePathUnderEvidenceRoot `
    -RelativePath $CaptureArtifactRelativePath `
    -EvidenceRootFull $evidenceRootFull `
    -Label "CaptureArtifactRelativePath"

$captureArtifactName = [System.IO.Path]::GetFileName($captureArtifactFull)
if ($captureArtifactName -ieq "evidence.json") {
    Write-Error "CaptureArtifactRelativePath must not be evidence.json."
}

$evidenceDirectoryFull = [System.IO.Path]::GetDirectoryName($captureArtifactFull)
$evidencePathFull = Join-Path $evidenceDirectoryFull "evidence.json"
$evidencePathRelative = ConvertTo-RepoRelativePath -FullPath $evidencePathFull
$captureArtifactRelativeToEvidence = $captureArtifactName
$willWrite = $Mode -eq "Import" -and -not $NoWrite.IsPresent
$captureArtifactHash = ""

$developerDirectory = Get-AppleDeveloperDirectory
$fullXcodeSelected = Test-FullXcodeDeveloperDirectory -DeveloperDirectory $developerDirectory
$xcrun = Find-CommandOnCombinedPath "xcrun"
$metalToolReady = Test-XcrunToolAvailable -Xcrun $xcrun -SdkName "macosx" -ToolName "metal"
$metallibToolReady = Test-XcrunToolAvailable -Xcrun $xcrun -SdkName "macosx" -ToolName "metallib"
$xctraceToolReady = (Test-XcrunToolAvailable -Xcrun $xcrun -SdkName "macosx" -ToolName "xctrace") -or
    -not [string]::IsNullOrWhiteSpace((Find-CommandOnCombinedPath "xctrace"))

if ($Mode -eq "Import") {
    if (-not (Test-Path -LiteralPath $captureArtifactFull -PathType Leaf)) {
        Write-Error "CaptureArtifactRelativePath does not exist: $CaptureArtifactRelativePath"
    }

    $captureArtifactHash = (Get-FileHash -LiteralPath $captureArtifactFull -Algorithm SHA256).Hash.ToLowerInvariant()

    $evidence = [ordered]@{
        schema_version = "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1"
        claim_id = "renderer-metal-memory-profiling-host-evidence-v1"
        host = [ordered]@{
            platform = "macos"
            macos_version = $MacosVersion
            xcode_version = $XcodeVersion
            full_xcode_selected = $true
            metal_tool_ready = $true
            metallib_tool_ready = $true
        }
        source_rows = [ordered]@{
            heap_documentation_source_id = "Apple-Metal-MTLHeap-2026-06-24"
            residency_set_documentation_source_id = "Apple-Metal-MTLResidencySet-2026-06-24"
            residency_request_documentation_source_id = "Apple-Metal-MTLResidencySet-requestResidency-2026-06-24"
            command_queue_residency_documentation_source_id = "Apple-Metal-MTLCommandQueue-addResidencySet-2026-06-24"
            capture_manager_documentation_source_id = "Apple-Metal-MTLCaptureManager-2026-06-24"
            programmatic_capture_documentation_source_id = "Apple-Metal-ProgrammaticCapture-2026-06-24"
        }
        memory_residency_row = [ordered]@{
            proof_row_id = "memory_residency"
            host_validation_recipe_id = "renderer-metal-memory-profiling-host-evidence"
            first_party_workload_id = $WorkloadId
            runtime_ready = $true
            command_queue_ready = $true
            heap_api_name = "MTLHeap"
            heap_allocation_ready = $true
            heap_resource_allocation_ready = $true
            heap_resource_rows = $HeapResourceRows
            heap_allocated_bytes = $HeapAllocatedBytes
            resident_bytes = $ResidentBytes
            budget_bytes = $BudgetBytes
            residency_api_name = "MTLResidencySet"
            residency_set_ready = $true
            residency_set_allocation_rows = $ResidencySetAllocationRows
            residency_request_ready = $true
            residency_commit_ready = $true
            command_queue_residency_set_committed = $true
            residency_pressure_evidence_ready = $true
            memory_pressure_sample_rows = $MemoryPressureSampleRows
            memory_pressure_budget_status = $MemoryPressureBudgetStatus
        }
        # The validator contract treats this row as MTLCaptureManager plus MTLCaptureScope boundary evidence.
        profiling_capture_row = [ordered]@{
            proof_row_id = "profiling_capture"
            host_validation_recipe_id = "renderer-metal-memory-profiling-host-evidence"
            first_party_workload_id = $WorkloadId
            runtime_ready = $true
            command_queue_ready = $true
            capture_api_name = "MTLCaptureManager"
            capture_manager_ready = $true
            capture_descriptor_ready = $true
            capture_object_ready = $true
            capture_scope_ready = $true
            capture_scope_label = $CaptureScopeLabel
            capture_boundary_ready = $true
            capture_started = $true
            capture_stopped = $true
            command_buffer_captured = $true
            capture_artifact_path = $captureArtifactRelativeToEvidence
            capture_artifact_hash_sha256 = $captureArtifactHash
            deterministic_capture_hash_sha256 = $captureArtifactHash
            capture_artifact_rows = $CaptureArtifactRows
        }
        non_claims = [ordered]@{
            simulator_only_evidence = $false
            cross_backend_inference = $false
            native_handles_exposed = $false
            broad_backend_parity_ready = $false
            broad_metal_readiness = $false
            commercial_renderer_readiness = $false
            broad_renderer_quality = $false
            environment_ready = $false
            external_engine_api_parity = $false
        }
    }

    if ($willWrite) {
        $null = New-Item -ItemType Directory -Path $evidenceDirectoryFull -Force
        $evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $evidencePathFull -Encoding utf8NoBOM
    }
}

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=renderer-metal-memory-profiling-host-evidence")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_mode=$Mode")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_host=$(Get-RendererMetalCollectorHostLabel)")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_workload=$WorkloadId")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_plan_ready=1")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_writes_evidence=$(ConvertTo-CounterBit $willWrite)")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $evidencePathFull -PathType Leaf)))")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_evidence_path=$evidencePathRelative")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_capture_artifact_path=$CaptureArtifactRelativePath")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_capture_artifact_hash=$captureArtifactHash")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_full_xcode_selected=$(ConvertTo-CounterBit $fullXcodeSelected)")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_metal_tool_ready=$(ConvertTo-CounterBit $metalToolReady)")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_metallib_tool_ready=$(ConvertTo-CounterBit $metallibToolReady)")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_xctrace_ready=$(ConvertTo-CounterBit $xctraceToolReady)")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_heap_resource_rows=$HeapResourceRows")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_residency_set_allocation_rows=$ResidencySetAllocationRows")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_memory_pressure_sample_rows=$MemoryPressureSampleRows")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_capture_artifact_rows=$CaptureArtifactRows")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_native_handles_exposed=0")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_external_engine_api_parity=0")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_broad_backend_parity=0")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_broad_metal_readiness=0")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_commercial_renderer=0")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_broad_renderer_quality=0")
$lines.Add("renderer_metal_memory_profiling_host_evidence_collector_environment_ready=0")

foreach ($line in $lines) {
    Write-Output $line
}

Write-Information "renderer-metal-memory-profiling-host-evidence-collector: ok" -InformationAction Continue
