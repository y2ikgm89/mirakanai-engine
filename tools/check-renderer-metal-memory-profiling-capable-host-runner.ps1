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

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

$preflightScript = Join-Path $root "tools/validate-renderer-metal-memory-profiling-capable-host-runner.ps1"
if (-not (Test-Path -LiteralPath $preflightScript -PathType Leaf)) {
    Write-Error "tools/validate-renderer-metal-memory-profiling-capable-host-runner.ps1 must exist for capable-host runner preflight"
}

$fixtureRootRelative = "out/renderer-metal-memory-profiling-capable-host-runner-preflight/$PID"
$fixtureRoot = ConvertTo-LocalPath $fixtureRootRelative
$null = New-Item -ItemType Directory -Force -Path $fixtureRoot

$readyJson = Join-Path $fixtureRoot "ready-runners.json"
Set-Content -LiteralPath $readyJson -Encoding utf8NoBOM -Value @'
{
  "total_count": 2,
  "runners": [
    {
      "id": 1001,
      "name": "metal-memory-ready",
      "os": "macOS",
      "status": "online",
      "busy": false,
      "labels": [
        { "name": "self-hosted" },
        { "name": "macOS" },
        { "name": "ARM64" },
        { "name": "metal-residency-set" }
      ]
    },
    {
      "id": 1002,
      "name": "metal-memory-busy",
      "os": "macOS",
      "status": "online",
      "busy": true,
      "labels": [
        { "name": "self-hosted" },
        { "name": "macOS" },
        { "name": "ARM64" },
        { "name": "metal-residency-set" }
      ]
    }
  ]
}
'@

$missingJson = Join-Path $fixtureRoot "missing-runners.json"
Set-Content -LiteralPath $missingJson -Encoding utf8NoBOM -Value @'
{
  "total_count": 0,
  "runners": []
}
'@

$readyLines = @(& $preflightScript -RepoFullName "owner/repo" -RunnersJsonPath $readyJson -RequireAvailable)
Assert-LinePresent $readyLines "validation_recipe=renderer-metal-memory-profiling-capable-host-runner-preflight" "ready runner preflight"
Assert-LinePresent $readyLines "renderer_metal_memory_profiling_capable_host_runner_api_endpoint=/repos/{owner}/{repo}/actions/runners" "ready runner preflight"
Assert-LinePresent $readyLines "renderer_metal_memory_profiling_capable_host_runner_status=ready" "ready runner preflight"
Assert-LinePresent $readyLines "renderer_metal_memory_profiling_capable_host_runner_available=1" "ready runner preflight"
Assert-LinePresent $readyLines "renderer_metal_memory_profiling_capable_host_runner_total=2" "ready runner preflight"
Assert-LinePresent $readyLines "renderer_metal_memory_profiling_capable_host_runner_matching_label_runners=2" "ready runner preflight"
Assert-LinePresent $readyLines "renderer_metal_memory_profiling_capable_host_runner_online_matching_runners=2" "ready runner preflight"
Assert-LinePresent $readyLines "renderer_metal_memory_profiling_capable_host_runner_idle_matching_runners=1" "ready runner preflight"
Assert-LinePresent $readyLines "renderer_metal_memory_profiling_capable_host_runner_required_labels=self-hosted,macOS,ARM64,metal-residency-set" "ready runner preflight"
Assert-LinePresent $readyLines "renderer_commercial_readiness=0" "ready runner preflight"

$missingLines = @(& $preflightScript -RepoFullName "owner/repo" -RunnersJsonPath $missingJson)
Assert-LinePresent $missingLines "renderer_metal_memory_profiling_capable_host_runner_status=host_evidence_required" "missing runner preflight"
Assert-LinePresent $missingLines "renderer_metal_memory_profiling_capable_host_runner_available=0" "missing runner preflight"
Assert-LinePresent $missingLines "renderer_metal_memory_profiling_capable_host_runner_total=0" "missing runner preflight"
Assert-LinePresent $missingLines "renderer_metal_memory_profiling_capable_host_runner_matching_label_runners=0" "missing runner preflight"
Assert-LinePresent $missingLines "renderer_metal_memory_profiling_capable_host_runner_idle_matching_runners=0" "missing runner preflight"
Assert-LinePresent $missingLines "renderer_commercial_readiness=0" "missing runner preflight"

$failedOutput = @(& pwsh -NoProfile -ExecutionPolicy Bypass -File $preflightScript `
        -RepoFullName "owner/repo" `
        -RunnersJsonPath $missingJson `
        -RequireAvailable 2>&1)
$failedExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { [int]$LASTEXITCODE }
$global:LASTEXITCODE = 0
if ($failedExitCode -eq 0) {
    Write-Error "missing runner preflight with -RequireAvailable unexpectedly succeeded"
}
if (-not (@($failedOutput) -match "Renderer Metal memory profiling capable host runner is not available")) {
    Write-Error "missing runner preflight with -RequireAvailable did not report the expected blocker"
}

Write-Information "renderer-metal-memory-profiling-capable-host-runner-check: ok" -InformationAction Continue
