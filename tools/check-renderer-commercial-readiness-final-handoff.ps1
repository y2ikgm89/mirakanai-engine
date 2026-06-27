#requires -Version 7.0
#requires -PSEdition Core
# Contract script: check-renderer-commercial-readiness-final-handoff.ps1

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$handoffScript = Join-Path $root "tools/validate-renderer-commercial-readiness-final-handoff.ps1"
if (-not (Test-Path -LiteralPath $handoffScript -PathType Leaf)) {
    Write-Error "tools/validate-renderer-commercial-readiness-final-handoff.ps1 must exist."
}
$handoffScriptText = Get-Content -LiteralPath $handoffScript -Raw
if ($handoffScriptText -match '\[Parameter\(Mandatory = \$true\)\]\[string\]\$RepositoryFullName') {
    Write-Error "Invoke-RunnerPreflight must allow an empty RepositoryFullName so RunnersJsonPath-only fixture preflight does not fail during PowerShell binding."
}
if ($handoffScriptText -match '\[Parameter\(Mandatory = \$true\)\]\[string\]\$RunnerJsonPath') {
    Write-Error "Invoke-RunnerPreflight must allow an empty RunnerJsonPath so RepoFullName-only API preflight does not fail during PowerShell binding."
}

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
    $allowedRoot = [System.IO.Path]::GetFullPath((ConvertTo-LocalPath "out/renderer-commercial-readiness-final-handoff"))
    $allowedPrefix = $allowedRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if ($fullPath -ne $allowedRoot -and
        -not $fullPath.StartsWith($allowedPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "Refusing to remove final handoff test root outside out/renderer-commercial-readiness-final-handoff: $fullPath"
    }
    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
    }
}

$fixtureRootRelative = "out/renderer-commercial-readiness-final-handoff/$PID"
$fixtureRoot = ConvertTo-LocalPath $fixtureRootRelative

try {
    Remove-TestRoot -RelativePath $fixtureRootRelative
    $null = New-Item -ItemType Directory -Force -Path $fixtureRoot

    $blockedManifestRelative = "$fixtureRootRelative/blocked-intake-manifest.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $blockedManifestRelative) -Value ([ordered]@{
            schema_version = "GameEngine.RendererCommercialReadinessFinalRetainedRootArtifactImport.v1"
            validation_recipe = "renderer-commercial-readiness-final-retained-root-artifact-import"
            repo_full_name = "owner/repo"
            run_id = "111111"
            ready = $false
            missing_assembler_inputs = @(
                "metal_memory_profiling_host_evidence",
                "quality_vfx_host_evidence"
            )
            assembler_input_blockers = [ordered]@{
                metal_memory_profiling_host_evidence = [ordered]@{
                    host_gate_summary_count = 1
                    host_gate_reasons = @("mtlresidencyset_unsupported")
                    dependent_missing_inputs = @()
                    host_gate_summary_paths = @("blocked/renderer-metal-memory-profiling-host-artifacts/host-gate-summary.json")
                }
                quality_vfx_host_evidence = [ordered]@{
                    host_gate_summary_count = 1
                    host_gate_reasons = @("metal_memory_profiling_host_evidence_required")
                    dependent_missing_inputs = @("metal_memory_profiling_host_evidence")
                    host_gate_summary_paths = @("blocked/renderer-quality-vfx-commercial-artifacts/host-gate-summary.json")
                }
            }
            assembler_handoff = [ordered]@{
                ready = $false
                script = "tools/assemble-renderer-commercial-readiness-final-retained-root.ps1"
                output_root = "artifacts/renderer/commercial-readiness-evidence/final-from-runs/assembled-final-retained-root"
                command_arguments = @()
                require_ready_command_arguments = @()
            }
            final_preflight_handoff = [ordered]@{
                ready = $false
                script = "tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1"
                artifact_root = ""
                command_arguments = @()
            }
            quality_vfx_regenerate = [ordered]@{
                requested = $false
                ready = $false
            }
            auto_assemble = [ordered]@{
                requested = $false
                ready = $false
            }
        })

    $readyRunnerJson = "$fixtureRootRelative/ready-runners.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $readyRunnerJson) -Value ([ordered]@{
            total_count = 1
            runners = @(
                [ordered]@{
                    id = 1001
                    name = "metal-memory-ready"
                    os = "macOS"
                    status = "online"
                    busy = $false
                    labels = @(
                        [ordered]@{ name = "self-hosted" },
                        [ordered]@{ name = "macOS" },
                        [ordered]@{ name = "ARM64" },
                        [ordered]@{ name = "metal-residency-set" }
                    )
                }
            )
        })

    $missingRunnerJson = "$fixtureRootRelative/missing-runners.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $missingRunnerJson) -Value ([ordered]@{
            total_count = 0
            runners = @()
        })

    $missingRunnerLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $missingRunnerJson `
            -IntakeManifestRelative $blockedManifestRelative)
    foreach ($expectedLine in @(
            "validation_recipe=renderer-commercial-readiness-final-handoff",
            "renderer_commercial_readiness_final_handoff_status=capable_host_runner_required",
            "renderer_commercial_readiness_final_handoff_next_action=provision_capable_host_runner",
            "renderer_commercial_readiness_final_handoff_source_run_ready=1",
            "renderer_commercial_readiness_final_handoff_runner_preflight_known=1",
            "renderer_commercial_readiness_final_handoff_runner_available=0",
            "renderer_commercial_readiness_final_handoff_missing_assembler_inputs=2",
            "renderer_commercial_readiness_final_handoff_missing_assembler_input_names=metal_memory_profiling_host_evidence,quality_vfx_host_evidence",
            "renderer_commercial_readiness_final_handoff_quality_vfx_dependency_blockers=metal_memory_profiling_host_evidence",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $missingRunnerLines $expectedLine "final handoff missing runner"
    }

    $readyRunnerLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $readyRunnerJson `
            -IntakeManifestRelative $blockedManifestRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=metal_memory_profiling_run_required",
            "renderer_commercial_readiness_final_handoff_next_action=run_metal_memory_profiling_capable_host_workflow",
            "renderer_commercial_readiness_final_handoff_runner_available=1",
            "renderer_commercial_readiness_final_handoff_capable_host_workflow_command=gh workflow run renderer-metal-memory-profiling-capable-host.yml -f confirm_capable_apple_host=MTLGPUFamilyApple6",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $readyRunnerLines $expectedLine "final handoff ready runner"
    }

    $fixtureOnlyRunnerLines = @(& $handoffScript `
            -RunnersJsonPath $readyRunnerJson `
            -IntakeManifestRelative $blockedManifestRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=metal_memory_profiling_run_required",
            "renderer_commercial_readiness_final_handoff_next_action=run_metal_memory_profiling_capable_host_workflow",
            "renderer_commercial_readiness_final_handoff_runner_preflight_known=1",
            "renderer_commercial_readiness_final_handoff_runner_available=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $fixtureOnlyRunnerLines $expectedLine "final handoff fixture-only runner"
    }

    $fromRunsLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $readyRunnerJson `
            -IntakeManifestRelative $blockedManifestRelative `
            -MetalMemoryProfilingRunId "222222")
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=ready_for_final_from_runs_workflow",
            "renderer_commercial_readiness_final_handoff_next_action=run_final_from_runs_workflow",
            "renderer_commercial_readiness_final_handoff_source_run_id=111111",
            "renderer_commercial_readiness_final_handoff_metal_memory_profiling_run_id=222222",
            "renderer_commercial_readiness_final_handoff_final_from_runs_workflow_command=gh workflow run renderer-commercial-readiness-final-from-runs.yml -f source_artifact_run_id=111111 -f metal_memory_profiling_run_id=222222 -f confirm_final_retained_root_handoff=renderer-commercial-final-retained-root",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $fromRunsLines $expectedLine "final handoff from-runs ready"
    }

    $finalRootManifestRelative = "$fixtureRootRelative/final-root-intake-manifest.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $finalRootManifestRelative) -Value ([ordered]@{
            schema_version = "GameEngine.RendererCommercialReadinessFinalRetainedRootArtifactImport.v1"
            run_id = "333333"
            ready = $true
            missing_assembler_inputs = @()
            assembler_input_blockers = [ordered]@{}
            assembler_handoff = [ordered]@{
                ready = $false
                script = "tools/assemble-renderer-commercial-readiness-final-retained-root.ps1"
                output_root = ""
                command_arguments = @()
                require_ready_command_arguments = @()
            }
            final_preflight_handoff = [ordered]@{
                ready = $true
                script = "tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1"
                artifact_root = "artifacts/renderer/commercial-readiness-evidence/final-from-runs/renderer-commercial-readiness-final-retained-root"
                command_arguments = @(
                    "-ArtifactRootRelative",
                    "artifacts/renderer/commercial-readiness-evidence/final-from-runs/renderer-commercial-readiness-final-retained-root"
                )
            }
        })

    $finalRootLines = @(& $handoffScript -IntakeManifestRelative $finalRootManifestRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=ready_for_final_preflight",
            "renderer_commercial_readiness_final_handoff_next_action=run_final_preflight",
            "renderer_commercial_readiness_final_handoff_final_preflight_command=pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1 -ArtifactRootRelative artifacts/renderer/commercial-readiness-evidence/final-from-runs/renderer-commercial-readiness-final-retained-root",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $finalRootLines $expectedLine "final handoff final-root preflight"
    }
}
finally {
    Remove-TestRoot -RelativePath $fixtureRootRelative
}

Write-Information "renderer-commercial-readiness-final-handoff-check: ok" -InformationAction Continue
