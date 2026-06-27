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
foreach ($needle in @(
        "UseRunDiscovery",
        "plan-renderer-commercial-readiness-final-run-discovery.ps1",
        "renderer_commercial_readiness_final_handoff_run_discovery_known",
        "renderer_commercial_readiness_final_handoff_run_discovery_status"
    )) {
    if (-not $handoffScriptText.Contains($needle)) {
        Write-Error "final handoff script missing required run-discovery bridge needle: $needle"
    }
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

function Assert-LineAbsent {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$UnexpectedLine,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if ($Lines.Contains($UnexpectedLine)) {
        Write-Error "$Context contained unexpected line: $UnexpectedLine"
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

    $publicRepoJson = "$fixtureRootRelative/public-repo.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $publicRepoJson) -Value ([ordered]@{
            full_name = "owner/repo"
            private = $false
            visibility = "public"
        })

    $publicRunnerSecurityReviewJson = "$fixtureRootRelative/public-runner-security-review.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $publicRunnerSecurityReviewJson) -Value ([ordered]@{
            schema_version = "GameEngine.RendererPublicSelfHostedRunnerSecurityReview.v1"
            repo_full_name = "owner/repo"
            repository_visibility = "public"
            review_status = "approved"
            reviewed_public_fork_pr_risk = $true
            reviewed_runner_isolation = $true
            reviewed_secret_exposure = $true
            reviewed_allowed_workflows = @("renderer-metal-memory-profiling-capable-host.yml")
            reviewed_required_labels = @("self-hosted", "macOS", "ARM64", "metal-residency-set")
            reviewed_metal_probe_truth = $true
            reviewed_workflow_file = ".github/workflows/renderer-metal-memory-profiling-capable-host.yml"
            workflow_audit = [ordered]@{
                workflow_dispatch_only = $true
                untrusted_pr_triggers_present = $false
                contents_permission_read_only = $true
                required_labels = @("self-hosted", "macOS", "ARM64", "metal-residency-set")
                checkout_action_pinned = $true
                checkout_persist_credentials_disabled = $true
                confirm_input_required = $true
            }
        })

    $invalidPublicRunnerSecurityReviewJson = "$fixtureRootRelative/public-runner-security-review-pending.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $invalidPublicRunnerSecurityReviewJson) -Value ([ordered]@{
            schema_version = "GameEngine.RendererPublicSelfHostedRunnerSecurityReview.v1"
            repo_full_name = "owner/repo"
            repository_visibility = "public"
            review_status = "pending"
            reviewed_public_fork_pr_risk = $true
            reviewed_runner_isolation = $true
            reviewed_secret_exposure = $true
            reviewed_allowed_workflows = @("renderer-metal-memory-profiling-capable-host.yml")
            reviewed_required_labels = @("self-hosted", "macOS", "ARM64", "metal-residency-set")
            reviewed_metal_probe_truth = $true
            reviewed_workflow_file = ".github/workflows/renderer-metal-memory-profiling-capable-host.yml"
            workflow_audit = [ordered]@{
                workflow_dispatch_only = $true
                untrusted_pr_triggers_present = $false
                contents_permission_read_only = $true
                required_labels = @("self-hosted", "macOS", "ARM64", "metal-residency-set")
                checkout_action_pinned = $true
                checkout_persist_credentials_disabled = $true
                confirm_input_required = $true
            }
        })

    $invalidPublicRunnerWorkflowReviewJson = "$fixtureRootRelative/public-runner-security-review-invalid-workflow.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $invalidPublicRunnerWorkflowReviewJson) -Value ([ordered]@{
            schema_version = "GameEngine.RendererPublicSelfHostedRunnerSecurityReview.v1"
            repo_full_name = "owner/repo"
            repository_visibility = "public"
            review_status = "approved"
            reviewed_public_fork_pr_risk = $true
            reviewed_runner_isolation = $true
            reviewed_secret_exposure = $true
            reviewed_allowed_workflows = @("renderer-metal-memory-profiling-capable-host.yml")
            reviewed_required_labels = @("self-hosted", "macOS", "ARM64", "metal-residency-set")
            reviewed_metal_probe_truth = $true
            reviewed_workflow_file = ".github/workflows/renderer-metal-memory-profiling-capable-host.yml"
            workflow_audit = [ordered]@{
                workflow_dispatch_only = $false
                untrusted_pr_triggers_present = $true
                contents_permission_read_only = $true
                required_labels = @("self-hosted", "macOS", "ARM64", "metal-residency-set")
                checkout_action_pinned = $true
                checkout_persist_credentials_disabled = $true
                confirm_input_required = $true
            }
        })

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

    $missingMetalArtifactsJson = "$fixtureRootRelative/missing-metal-artifacts.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $missingMetalArtifactsJson) -Value ([ordered]@{
            total_count = 0
            artifacts = @()
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
            "renderer_commercial_readiness_final_handoff_runner_required_labels=self-hosted,macOS,ARM64,metal-residency-set",
            "renderer_commercial_readiness_final_handoff_runner_registration_token_endpoint=/repos/owner/repo/actions/runners/registration-token",
            "renderer_commercial_readiness_final_handoff_runner_registration_token_command=gh api -X POST -H `"Accept: application/vnd.github+json`" /repos/owner/repo/actions/runners/registration-token",
            "renderer_commercial_readiness_final_handoff_runner_registration_token_expires_minutes=60",
            "renderer_commercial_readiness_final_handoff_runner_config_command_template=./config.sh --url https://github.com/owner/repo --token <registration-token> --labels metal-residency-set",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_required=1",
            "renderer_commercial_readiness_final_handoff_runner_label_truth_requires_metal_probe=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $missingRunnerLines $expectedLine "final handoff missing runner"
    }

    $publicRepoUnreviewedLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $missingRunnerJson `
            -RepositoryJsonPath $publicRepoJson `
            -IntakeManifestRelative $blockedManifestRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=public_runner_security_review_required",
            "renderer_commercial_readiness_final_handoff_next_action=complete_public_runner_security_review",
            "renderer_commercial_readiness_final_handoff_runner_repository_metadata_known=1",
            "renderer_commercial_readiness_final_handoff_runner_repository_visibility=public",
            "renderer_commercial_readiness_final_handoff_runner_repository_private=0",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_required=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmed=0",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmation_required=public-repo-self-hosted-runner-risk-reviewed",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $publicRepoUnreviewedLines $expectedLine "final handoff public repo unreviewed"
    }
    Assert-LineAbsent $publicRepoUnreviewedLines `
        "renderer_commercial_readiness_final_handoff_runner_registration_token_command=gh api -X POST -H `"Accept: application/vnd.github+json`" /repos/owner/repo/actions/runners/registration-token" `
        "final handoff public repo unreviewed"

    $publicRepoReviewedLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $missingRunnerJson `
            -RepositoryJsonPath $publicRepoJson `
            -ConfirmPublicRepoSelfHostedRunnerSecurityReview public-repo-self-hosted-runner-risk-reviewed `
            -IntakeManifestRelative $blockedManifestRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=capable_host_runner_required",
            "renderer_commercial_readiness_final_handoff_next_action=provision_capable_host_runner",
            "renderer_commercial_readiness_final_handoff_runner_repository_metadata_known=1",
            "renderer_commercial_readiness_final_handoff_runner_repository_visibility=public",
            "renderer_commercial_readiness_final_handoff_runner_repository_private=0",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_required=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmed=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=0",
            "renderer_commercial_readiness_final_handoff_runner_registration_token_command=gh api -X POST -H `"Accept: application/vnd.github+json`" /repos/owner/repo/actions/runners/registration-token",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $publicRepoReviewedLines $expectedLine "final handoff public repo reviewed"
    }

    $publicRepoReviewArtifactLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $missingRunnerJson `
            -RepositoryJsonPath $publicRepoJson `
            -PublicRepoRunnerSecurityReviewRelative $publicRunnerSecurityReviewJson `
            -IntakeManifestRelative $blockedManifestRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=capable_host_runner_required",
            "renderer_commercial_readiness_final_handoff_next_action=provision_capable_host_runner",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_present=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_valid=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_status=approved",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_workflow_audit_valid=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmed=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=0",
            "renderer_commercial_readiness_final_handoff_runner_registration_token_command=gh api -X POST -H `"Accept: application/vnd.github+json`" /repos/owner/repo/actions/runners/registration-token",
            "renderer_commercial_readiness=0"
    )) {
        Assert-LinePresent $publicRepoReviewArtifactLines $expectedLine "final handoff public repo review artifact"
    }

    $publicRepoInvalidReviewArtifactLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $missingRunnerJson `
            -RepositoryJsonPath $publicRepoJson `
            -PublicRepoRunnerSecurityReviewRelative $invalidPublicRunnerSecurityReviewJson `
            -IntakeManifestRelative $blockedManifestRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=public_runner_security_review_required",
            "renderer_commercial_readiness_final_handoff_next_action=complete_public_runner_security_review",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_present=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_valid=0",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_status=pending",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmed=0",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $publicRepoInvalidReviewArtifactLines $expectedLine "final handoff public repo invalid review artifact"
    }
    Assert-LineAbsent $publicRepoInvalidReviewArtifactLines `
        "renderer_commercial_readiness_final_handoff_runner_registration_token_command=gh api -X POST -H `"Accept: application/vnd.github+json`" /repos/owner/repo/actions/runners/registration-token" `
        "final handoff public repo invalid review artifact"

    $publicRepoInvalidWorkflowReviewLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $missingRunnerJson `
            -RepositoryJsonPath $publicRepoJson `
            -PublicRepoRunnerSecurityReviewRelative $invalidPublicRunnerWorkflowReviewJson `
            -IntakeManifestRelative $blockedManifestRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=public_runner_security_review_required",
            "renderer_commercial_readiness_final_handoff_next_action=complete_public_runner_security_review",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_present=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_valid=0",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_status=approved",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_workflow_audit_valid=0",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $publicRepoInvalidWorkflowReviewLines $expectedLine "final handoff public repo invalid workflow review artifact"
    }
    Assert-LineAbsent $publicRepoInvalidWorkflowReviewLines `
        "renderer_commercial_readiness_final_handoff_runner_registration_token_command=gh api -X POST -H `"Accept: application/vnd.github+json`" /repos/owner/repo/actions/runners/registration-token" `
        "final handoff public repo invalid workflow review artifact"

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

    $discoveredMissingRunnerLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $missingRunnerJson `
            -RepositoryJsonPath $publicRepoJson `
            -UseRunDiscovery `
            -RunDiscoverySourceRunsJsonPath $sourceRunsJson `
            -RunDiscoverySourceArtifactsJsonPath $sourceArtifactsJson `
            -RunDiscoveryMetalRunsJsonPath $metalRunsJson `
            -RunDiscoveryMetalArtifactsJsonPath $missingMetalArtifactsJson)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=public_runner_security_review_required",
            "renderer_commercial_readiness_final_handoff_next_action=complete_public_runner_security_review",
            "renderer_commercial_readiness_final_handoff_intake_manifest_present=0",
            "renderer_commercial_readiness_final_handoff_run_discovery_known=1",
            "renderer_commercial_readiness_final_handoff_run_discovery_status=metal_memory_profiling_run_required",
            "renderer_commercial_readiness_final_handoff_run_discovery_source_run_ready=1",
            "renderer_commercial_readiness_final_handoff_run_discovery_metal_memory_profiling_run_ready=0",
            "renderer_commercial_readiness_final_handoff_source_run_ready=1",
            "renderer_commercial_readiness_final_handoff_source_run_id=111111",
            "renderer_commercial_readiness_final_handoff_metal_memory_profiling_run_ready=0",
            "renderer_commercial_readiness_final_handoff_runner_available=0",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $discoveredMissingRunnerLines $expectedLine "final handoff run discovery missing runner"
    }
    Assert-LineAbsent $discoveredMissingRunnerLines `
        "renderer_commercial_readiness_final_handoff_runner_registration_token_command=gh api -X POST -H `"Accept: application/vnd.github+json`" /repos/owner/repo/actions/runners/registration-token" `
        "final handoff run discovery missing runner"

    $discoveredReadyRunnerLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $readyRunnerJson `
            -RepositoryJsonPath $publicRepoJson `
            -UseRunDiscovery `
            -RunDiscoverySourceRunsJsonPath $sourceRunsJson `
            -RunDiscoverySourceArtifactsJsonPath $sourceArtifactsJson `
            -RunDiscoveryMetalRunsJsonPath $metalRunsJson `
            -RunDiscoveryMetalArtifactsJsonPath $missingMetalArtifactsJson)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=public_runner_security_review_required",
            "renderer_commercial_readiness_final_handoff_next_action=complete_public_runner_security_review",
            "renderer_commercial_readiness_final_handoff_run_discovery_known=1",
            "renderer_commercial_readiness_final_handoff_source_run_id=111111",
            "renderer_commercial_readiness_final_handoff_runner_available=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_required=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmed=0",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $discoveredReadyRunnerLines $expectedLine "final handoff run discovery ready public runner unreviewed"
    }
    Assert-LineAbsent $discoveredReadyRunnerLines `
        "renderer_commercial_readiness_final_handoff_capable_host_workflow_command=gh workflow run renderer-metal-memory-profiling-capable-host.yml -f confirm_capable_apple_host=MTLGPUFamilyApple6" `
        "final handoff run discovery ready public runner unreviewed"

    $discoveredReviewedReadyRunnerLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $readyRunnerJson `
            -RepositoryJsonPath $publicRepoJson `
            -PublicRepoRunnerSecurityReviewRelative $publicRunnerSecurityReviewJson `
            -UseRunDiscovery `
            -RunDiscoverySourceRunsJsonPath $sourceRunsJson `
            -RunDiscoverySourceArtifactsJsonPath $sourceArtifactsJson `
            -RunDiscoveryMetalRunsJsonPath $metalRunsJson `
            -RunDiscoveryMetalArtifactsJsonPath $missingMetalArtifactsJson)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=metal_memory_profiling_run_required",
            "renderer_commercial_readiness_final_handoff_next_action=run_metal_memory_profiling_capable_host_workflow",
            "renderer_commercial_readiness_final_handoff_run_discovery_known=1",
            "renderer_commercial_readiness_final_handoff_source_run_id=111111",
            "renderer_commercial_readiness_final_handoff_runner_available=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_valid=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_workflow_audit_valid=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmed=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=0",
            "renderer_commercial_readiness_final_handoff_capable_host_workflow_command=gh workflow run renderer-metal-memory-profiling-capable-host.yml -f confirm_capable_apple_host=MTLGPUFamilyApple6",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $discoveredReviewedReadyRunnerLines $expectedLine "final handoff run discovery ready public runner reviewed"
    }

    $discoveredFromRunsLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $missingRunnerJson `
            -UseRunDiscovery `
            -RunDiscoverySourceRunsJsonPath $sourceRunsJson `
            -RunDiscoverySourceArtifactsJsonPath $sourceArtifactsJson `
            -RunDiscoveryMetalRunsJsonPath $metalRunsJson `
            -RunDiscoveryMetalArtifactsJsonPath $metalArtifactsJson)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=ready_for_final_from_runs_workflow",
            "renderer_commercial_readiness_final_handoff_next_action=run_final_from_runs_workflow",
            "renderer_commercial_readiness_final_handoff_run_discovery_status=ready_for_final_from_runs_workflow",
            "renderer_commercial_readiness_final_handoff_run_discovery_metal_memory_profiling_run_ready=1",
            "renderer_commercial_readiness_final_handoff_source_run_id=111111",
            "renderer_commercial_readiness_final_handoff_metal_memory_profiling_run_id=222222",
            "renderer_commercial_readiness_final_handoff_final_from_runs_workflow_command=gh workflow run renderer-commercial-readiness-final-from-runs.yml -f source_artifact_run_id=111111 -f metal_memory_profiling_run_id=222222 -f confirm_final_retained_root_handoff=renderer-commercial-final-retained-root",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $discoveredFromRunsLines $expectedLine "final handoff run discovery from-runs ready"
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
