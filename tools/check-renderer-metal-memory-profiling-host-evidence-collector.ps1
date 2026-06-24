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

function Get-LineByPrefix {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$Prefix,
        [Parameter(Mandatory = $true)][string]$Context
    )

    $matchesForPrefix = @($Lines | Where-Object { [string]$_ -like "$Prefix*" })
    if ($matchesForPrefix.Count -ne 1) {
        Write-Error "$Context expected one line with prefix '$Prefix' but found $($matchesForPrefix.Count)."
    }
    return [string]$matchesForPrefix[0]
}

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

$collectorScript = Join-Path $root "tools/collect-renderer-metal-memory-profiling-host-evidence.ps1"
if (-not (Test-Path -LiteralPath $collectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-metal-memory-profiling-host-evidence.ps1 must exist for renderer Metal memory/profiling evidence collection"
}

$evidenceRootRelative = "out/renderer-metal-memory-profiling-host-evidence-collector-contract/$PID"
$artifactDirectoryRelative = "$evidenceRootRelative/sample_desktop_runtime_game"
$artifactDirectory = ConvertTo-LocalPath $artifactDirectoryRelative
$null = New-Item -ItemType Directory -Path $artifactDirectory -Force

$captureArtifact = "$artifactDirectoryRelative/capture-summary.txt"
Set-Content -LiteralPath (ConvertTo-LocalPath $captureArtifact) -Encoding utf8NoBOM -Value @(
    "schema=GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
    "claim=renderer-metal-memory-profiling-host-evidence-v1",
    "api=MTLHeap",
    "api=MTLResidencySet",
    "api=MTLCaptureManager",
    "api=MTLCaptureScope",
    "proof=memory_residency",
    "proof=profiling_capture"
)

$commonArgs = @{
    EvidenceRoot = $evidenceRootRelative
    WorkloadId = "sample_desktop_runtime_game.renderer_metal_memory_profiling_collector_self_test"
    CaptureArtifactRelativePath = $captureArtifact
    MacosVersion = "macOS collector self-test"
    XcodeVersion = "Xcode collector self-test"
    HeapResourceRows = 2
    HeapAllocatedBytes = 4194304
    ResidentBytes = 3145728
    BudgetBytes = 8388608
    ResidencySetAllocationRows = 1
    MemoryPressureSampleRows = 2
    MemoryPressureBudgetStatus = "within_budget"
    CaptureScopeLabel = "RendererMetalMemoryProfilingCollectorSelfTest"
    CaptureArtifactRows = 1
}

$planLines = @(& $collectorScript -Mode Plan @commonArgs -NoWrite)
Assert-LinePresent $planLines "renderer_metal_memory_profiling_host_evidence_collector_mode=Plan" "collector Plan mode"
Assert-LinePresent $planLines "renderer_metal_memory_profiling_host_evidence_collector_plan_ready=1" "collector Plan mode"
Assert-LinePresent $planLines "renderer_metal_memory_profiling_host_evidence_collector_writes_evidence=0" "collector Plan mode"
Assert-LinePresent $planLines "renderer_metal_memory_profiling_host_evidence_collector_broad_backend_parity=0" "collector Plan mode"
Assert-LinePresent $planLines "renderer_metal_memory_profiling_host_evidence_collector_broad_metal_readiness=0" "collector Plan mode"
Assert-LinePresent $planLines "renderer_metal_memory_profiling_host_evidence_collector_commercial_renderer=0" "collector Plan mode"
Assert-LinePresent $planLines "renderer_metal_memory_profiling_host_evidence_collector_broad_renderer_quality=0" "collector Plan mode"

$importLines = @(& $collectorScript -Mode Import @commonArgs)
Assert-LinePresent $importLines "renderer_metal_memory_profiling_host_evidence_collector_mode=Import" "collector Import mode"
Assert-LinePresent $importLines "renderer_metal_memory_profiling_host_evidence_collector_written=1" "collector Import mode"
Assert-LinePresent $importLines "renderer_metal_memory_profiling_host_evidence_collector_native_handles_exposed=0" "collector Import mode"
Assert-LinePresent $importLines "renderer_metal_memory_profiling_host_evidence_collector_external_engine_api_parity=0" "collector Import mode"
Assert-LinePresent $importLines "renderer_metal_memory_profiling_host_evidence_collector_broad_backend_parity=0" "collector Import mode"
Assert-LinePresent $importLines "renderer_metal_memory_profiling_host_evidence_collector_broad_metal_readiness=0" "collector Import mode"
Assert-LinePresent $importLines "renderer_metal_memory_profiling_host_evidence_collector_commercial_renderer=0" "collector Import mode"
Assert-LinePresent $importLines "renderer_metal_memory_profiling_host_evidence_collector_broad_renderer_quality=0" "collector Import mode"

$hashLine = Get-LineByPrefix `
    -Lines $importLines `
    -Prefix "renderer_metal_memory_profiling_host_evidence_collector_capture_artifact_hash=" `
    -Context "collector Import mode"
$hashValue = $hashLine.Substring("renderer_metal_memory_profiling_host_evidence_collector_capture_artifact_hash=".Length)
if ($hashValue -cnotmatch "^[0-9a-f]{64}$") {
    Write-Error "collector Import mode emitted invalid capture artifact hash: $hashValue"
}

$evidenceJsonPath = ConvertTo-LocalPath "$artifactDirectoryRelative/evidence.json"
if (-not (Test-Path -LiteralPath $evidenceJsonPath -PathType Leaf)) {
    Write-Error "collector Import mode did not write evidence.json"
}

$evidence = Get-Content -LiteralPath $evidenceJsonPath -Raw | ConvertFrom-Json
if ([string]$evidence.schema_version -ne "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1") {
    Write-Error "collector evidence schema_version mismatch"
}
if ([string]$evidence.claim_id -ne "renderer-metal-memory-profiling-host-evidence-v1") {
    Write-Error "collector evidence claim_id mismatch"
}
if ([string]$evidence.memory_residency_row.proof_row_id -ne "memory_residency") {
    Write-Error "collector evidence memory_residency proof row mismatch"
}
if ([string]$evidence.profiling_capture_row.proof_row_id -ne "profiling_capture") {
    Write-Error "collector evidence profiling_capture proof row mismatch"
}
if ([string]$evidence.profiling_capture_row.capture_artifact_path -ne "capture-summary.txt") {
    Write-Error "collector evidence capture_artifact_path mismatch"
}
if ([string]$evidence.profiling_capture_row.capture_artifact_hash_sha256 -ne $hashValue) {
    Write-Error "collector evidence capture_artifact_hash_sha256 mismatch"
}

$validationLines = @(& (Join-Path $PSScriptRoot "check-renderer-metal-memory-profiling-host-evidence.ps1") `
        -SkipFocusedRendererBuild `
        -ArtifactRootRelative $evidenceRootRelative `
        -RequireReady `
        -ExpectedEvidenceCounters `
        "renderer_metal_memory_profiling_status=ready" `
        "renderer_metal_memory_profiling_ready=1" `
        "renderer_backend_parity_ready=0" `
        "renderer_metal_broad_readiness=0" `
        "renderer_commercial_readiness=0" `
        "renderer_broad_quality_ready=0")

Assert-LinePresent $validationLines "renderer_metal_memory_profiling_status=ready" "collector generated evidence validation"
Assert-LinePresent $validationLines "renderer_metal_memory_profiling_ready=1" "collector generated evidence validation"
Assert-LinePresent $validationLines "renderer_backend_parity_ready=0" "collector generated evidence validation"
Assert-LinePresent $validationLines "renderer_metal_broad_readiness=0" "collector generated evidence validation"
Assert-LinePresent $validationLines "renderer_commercial_readiness=0" "collector generated evidence validation"
Assert-LinePresent $validationLines "renderer_broad_quality_ready=0" "collector generated evidence validation"

Write-Information "renderer-metal-memory-profiling-host-evidence-collector-check: ok" -InformationAction Continue
