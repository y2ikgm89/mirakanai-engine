#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady,
    [string[]]$ExpectedEvidenceCounters = @(),
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)

$root = Get-RepoRoot
Set-Location $root

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Assert-TextContains {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Text.Contains($Needle)) {
        Write-Error "$Context must contain '$Needle'."
    }
}

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG autonomous streaming scheduler validation."
}

Write-Information "mavg-autonomous-streaming-scheduler: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--preset",
    "dev"
)

Write-Information "mavg-autonomous-streaming-scheduler: building focused runtime RHI test target..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--build",
    "--preset",
    "dev",
    "--target",
    "MK_runtime_rhi_mavg_autonomous_streaming_scheduler_tests"
)

Write-Information "mavg-autonomous-streaming-scheduler: running focused runtime RHI CTest lane..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/ctest.ps1"),
    "--preset",
    "dev",
    "--output-on-failure",
    "-R",
    "MK_runtime_rhi_mavg_autonomous_streaming_scheduler_tests"
)

$headerText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_autonomous_streaming_scheduler.hpp") -Raw
$sourceText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_rhi/src/mavg_autonomous_streaming_scheduler.cpp") -Raw
$testText = Get-Content -LiteralPath (Join-Path $root "tests/unit/runtime_rhi_mavg_autonomous_streaming_scheduler_tests.cpp") -Raw
$planText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md") -Raw
$manifestText = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw

foreach ($needle in @(
        "RuntimeMavgAutonomousStreamingSchedulerState",
        "RuntimeMavgAutonomousStreamingSchedulerDesc",
        "RuntimeMavgAutonomousStreamingSchedulerResult",
        "tick_runtime_mavg_autonomous_streaming_scheduler",
        "RuntimeMavgAutonomousStreamingIoBackendKind",
        "RuntimeMavgAutonomousStreamingSafePointPolicy",
        "mavg_autonomous_streaming_scheduler_ready",
        "exposed_native_handles",
        "proved_async_overlap_performance"
    )) {
    Assert-TextContains -Text $headerText -Needle $needle -Context "mavg_autonomous_streaming_scheduler.hpp"
}

foreach ($needle in @(
        "select_mavg_lod_clusters",
        "plan_runtime_mavg_page_streaming_requests",
        "load_runtime_mavg_payload_pages_from_filesystem",
        "load_runtime_mavg_payload_pages_from_direct_storage",
        "tick_runtime_mavg_page_streaming_background_service",
        "plan_runtime_mavg_gpu_memory_pressure_residency",
        "execute_runtime_mavg_cluster_streaming_safe_point_adoption",
        "result.exposed_native_handles = false",
        "result.proved_async_overlap_performance = false"
    )) {
    Assert-TextContains -Text $sourceText -Needle $needle -Context "mavg_autonomous_streaming_scheduler.cpp"
}

foreach ($needle in @(
        "selects dispatches adopts and evicts without page requests",
        "keeps pending rows and coalesces duplicates across frames",
        "responds to camera movement and page heat priority",
        "routes payload reads through directstorage executor",
        "fails closed on directstorage executor failure",
        "cancels selected pages before io",
        "preserves safe point atomicity on invalid mount rows"
    )) {
    Assert-TextContains -Text $testText -Needle $needle -Context "MK_runtime_rhi_mavg_autonomous_streaming_scheduler_tests"
}

foreach ($docCheck in @(
        @{ Text = $planText; Context = "MAVG advanced backend evidence plan" },
        @{ Text = $manifestText; Context = "engine/agent/manifest.json" }
    )) {
    Assert-TextContains -Text $docCheck.Text -Needle "mavg-autonomous-streaming-scheduler-v1" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "RuntimeMavgAutonomousStreamingSchedulerResult" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "mavg_autonomous_streaming_scheduler_ready=1" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "native handles" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "async-overlap/performance proof" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "broad MAVG backend readiness" -Context $docCheck.Context
}

$ready = $true

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-autonomous-streaming-scheduler")
$lines.Add("mavg_autonomous_streaming_scheduler_status=$(if ($ready) { 'ready' } else { 'blocked' })")
$lines.Add("mavg_autonomous_streaming_scheduler_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_autonomous_streaming_scheduler_tests_ready=1")
$lines.Add("mavg_autonomous_streaming_scheduler_native_handles_exposed=0")
$lines.Add("mavg_autonomous_streaming_scheduler_renderer_rhi_handles_touched=0")
$lines.Add("mavg_autonomous_streaming_scheduler_async_overlap_performance_proof=0")
$lines.Add("mavg_autonomous_streaming_scheduler_broad_backend_readiness=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG autonomous streaming scheduler is incomplete; focused CTest and docs/manifest/static evidence are required."
}
