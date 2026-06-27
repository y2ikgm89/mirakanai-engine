#requires -Version 7.0
#requires -PSEdition Core
# Contract script: check-renderer-commercial-readiness-final-run-discovery.ps1

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

function Assert-LineAbsentPattern {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$UnexpectedPattern,
        [Parameter(Mandatory = $true)][string]$Context
    )

    foreach ($line in @($Lines)) {
        if ([string]$line -match $UnexpectedPattern) {
            Write-Error "$Context contained unexpected pattern '$UnexpectedPattern' in line: $line"
        }
    }
}

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

function Write-JsonObject {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][object]$Value
    )

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    $json = $Value | ConvertTo-Json -Depth 16
    Set-Content -LiteralPath $Path -Value $json -Encoding utf8NoBOM
}

function Remove-TestRoot {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $fullPath = [System.IO.Path]::GetFullPath((ConvertTo-LocalPath $RelativePath))
    $allowedRoot = [System.IO.Path]::GetFullPath((ConvertTo-LocalPath "out/renderer-commercial-readiness-final-run-discovery"))
    $allowedPrefix = $allowedRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if ($fullPath -ne $allowedRoot -and
        -not $fullPath.StartsWith($allowedPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "Refusing to remove final run discovery test root outside out/renderer-commercial-readiness-final-run-discovery: $fullPath"
    }
    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
    }
}

$discoveryScript = Join-Path $root "tools/plan-renderer-commercial-readiness-final-run-discovery.ps1"
if (-not (Test-Path -LiteralPath $discoveryScript -PathType Leaf)) {
    Write-Error "tools/plan-renderer-commercial-readiness-final-run-discovery.ps1 must exist so final-from-runs handoff inputs can be discovered without manual run-id inference."
}

$discoveryScriptText = Get-Content -LiteralPath $discoveryScript -Raw
foreach ($needle in @(
        "/actions/runs",
        "/actions/runs/{run_id}/artifacts",
        "renderer-commercial-readiness-current-run-artifact-intake",
        "renderer-metal-memory-profiling-host-artifacts",
        "renderer-commercial-readiness-final-from-runs.yml",
        "workflow_dispatched=0",
        "artifacts_downloaded=0",
        "renderer_commercial_readiness=0",
        "/websites/github_en_rest",
        "/websites/github_en_actions"
    )) {
    if (-not $discoveryScriptText.Contains($needle)) {
        Write-Error "final run discovery script missing required contract needle: $needle"
    }
}
foreach ($forbiddenNeedle in @(
        "gh run download",
        "registration-token",
        "actions/runners",
        "ghs_"
    )) {
    if ($discoveryScriptText.Contains($forbiddenNeedle)) {
        Write-Error "final run discovery script must not contain forbidden token/runner/download surface: $forbiddenNeedle"
    }
}

$fixtureRootRelative = "out/renderer-commercial-readiness-final-run-discovery/$PID"
$fixtureRoot = ConvertTo-LocalPath $fixtureRootRelative

try {
    Remove-TestRoot -RelativePath $fixtureRootRelative
    $null = New-Item -ItemType Directory -Force -Path $fixtureRoot

    $sourceRunsJson = "$fixtureRootRelative/source-runs.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $sourceRunsJson) -Value ([ordered]@{
            total_count = 1
            workflow_runs = @(
                [ordered]@{
                    id = 111111
                    name = "Validate"
                    head_branch = "main"
                    status = "completed"
                    conclusion = "success"
                    created_at = "2026-06-27T17:00:00Z"
                }
            )
        })

    $sourceArtifactsJson = "$fixtureRootRelative/source-artifacts.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $sourceArtifactsJson) -Value ([ordered]@{
            total_count = 1
            artifacts = @(
                [ordered]@{
                    name = "renderer-commercial-readiness-current-run-artifact-intake"
                    expired = $false
                }
            )
        })

    $metalRunsJson = "$fixtureRootRelative/metal-runs.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $metalRunsJson) -Value ([ordered]@{
            total_count = 1
            workflow_runs = @(
                [ordered]@{
                    id = 222222
                    name = "Renderer Metal Memory Profiling Capable Host"
                    head_branch = "main"
                    status = "completed"
                    conclusion = "success"
                    created_at = "2026-06-27T18:00:00Z"
                }
            )
        })

    $metalArtifactsJson = "$fixtureRootRelative/metal-artifacts.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $metalArtifactsJson) -Value ([ordered]@{
            total_count = 1
            artifacts = @(
                [ordered]@{
                    name = "renderer-metal-memory-profiling-host-artifacts"
                    expired = $false
                }
            )
        })

    $readyLines = @(& $discoveryScript `
            -Mode Plan `
            -RepoFullName "owner/repo" `
            -SourceRunsJsonPath $sourceRunsJson `
            -SourceArtifactsJsonPath $sourceArtifactsJson `
            -MetalRunsJsonPath $metalRunsJson `
            -MetalArtifactsJsonPath $metalArtifactsJson)
    foreach ($expectedLine in @(
            "validation_recipe=renderer-commercial-readiness-final-run-discovery",
            "renderer_commercial_readiness_final_run_discovery_status=ready_for_final_from_runs_workflow",
            "renderer_commercial_readiness_final_run_discovery_next_action=run_final_from_runs_workflow",
            "renderer_commercial_readiness_final_run_discovery_source_run_ready=1",
            "renderer_commercial_readiness_final_run_discovery_source_run_id=111111",
            "renderer_commercial_readiness_final_run_discovery_source_artifact_present=1",
            "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_run_ready=1",
            "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_run_id=222222",
            "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_artifact_present=1",
            "renderer_commercial_readiness_final_run_discovery_final_from_runs_workflow_command=gh workflow run renderer-commercial-readiness-final-from-runs.yml -f source_artifact_run_id=111111 -f metal_memory_profiling_run_id=222222 -f confirm_final_retained_root_handoff=renderer-commercial-final-retained-root",
            "renderer_commercial_readiness_final_run_discovery_workflow_dispatched=0",
            "renderer_commercial_readiness_final_run_discovery_artifacts_downloaded=0",
            "renderer_commercial_readiness_final_run_discovery_gpu_workload_executed=0",
            "renderer_commercial_readiness_final_run_discovery_evidence_assembled=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $readyLines $expectedLine "final run discovery ready"
    }
    Assert-LineAbsentPattern $readyLines "registration-token|ghs_" "final run discovery ready"

    $missingMetalArtifactsJson = "$fixtureRootRelative/missing-metal-artifacts.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $missingMetalArtifactsJson) -Value ([ordered]@{
            total_count = 0
            artifacts = @()
        })

    $missingMetalLines = @(& $discoveryScript `
            -Mode Plan `
            -RepoFullName "owner/repo" `
            -SourceRunsJsonPath $sourceRunsJson `
            -SourceArtifactsJsonPath $sourceArtifactsJson `
            -MetalRunsJsonPath $metalRunsJson `
            -MetalArtifactsJsonPath $missingMetalArtifactsJson)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_run_discovery_status=metal_memory_profiling_run_required",
            "renderer_commercial_readiness_final_run_discovery_next_action=run_metal_memory_profiling_capable_host_workflow",
            "renderer_commercial_readiness_final_run_discovery_source_run_ready=1",
            "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_run_ready=0",
            "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_artifact_present=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $missingMetalLines $expectedLine "final run discovery missing metal"
    }
    Assert-LineAbsentPattern $missingMetalLines "renderer_commercial_readiness_final_run_discovery_final_from_runs_workflow_command=" "final run discovery missing metal"

    $missingSourceArtifactsJson = "$fixtureRootRelative/missing-source-artifacts.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $missingSourceArtifactsJson) -Value ([ordered]@{
            total_count = 0
            artifacts = @()
        })

    $missingSourceLines = @(& $discoveryScript `
            -Mode Plan `
            -RepoFullName "owner/repo" `
            -SourceRunsJsonPath $sourceRunsJson `
            -SourceArtifactsJsonPath $missingSourceArtifactsJson `
            -MetalRunsJsonPath $metalRunsJson `
            -MetalArtifactsJsonPath $metalArtifactsJson)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_run_discovery_status=source_run_required",
            "renderer_commercial_readiness_final_run_discovery_next_action=wait_for_current_run_artifact_intake",
            "renderer_commercial_readiness_final_run_discovery_source_run_ready=0",
            "renderer_commercial_readiness_final_run_discovery_source_artifact_present=0",
            "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_run_ready=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $missingSourceLines $expectedLine "final run discovery missing source"
    }
    Assert-LineAbsentPattern $missingSourceLines "renderer_commercial_readiness_final_run_discovery_final_from_runs_workflow_command=" "final run discovery missing source"
}
finally {
    Remove-TestRoot -RelativePath $fixtureRootRelative
}

Write-Information "renderer-commercial-readiness-final-run-discovery-check: ok" -InformationAction Continue
