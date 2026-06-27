#requires -Version 7.0
#requires -PSEdition Core
# Contract script: check-renderer-public-runner-security-review.ps1

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
    $allowedRoot = [System.IO.Path]::GetFullPath((ConvertTo-LocalPath "artifacts/renderer/public-runner-security-review"))
    $allowedPrefix = $allowedRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if ($fullPath -ne $allowedRoot -and
        -not $fullPath.StartsWith($allowedPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "Refusing to remove public runner review test root outside artifacts/renderer/public-runner-security-review: $fullPath"
    }
    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
    }
}

$reviewScript = Join-Path $root "tools/generate-renderer-public-runner-security-review.ps1"
if (-not (Test-Path -LiteralPath $reviewScript -PathType Leaf)) {
    Write-Error "tools/generate-renderer-public-runner-security-review.ps1 must exist so public-repo self-hosted runner review can be retained before runner registration handoff."
}

$handoffScript = Join-Path $root "tools/validate-renderer-commercial-readiness-final-handoff.ps1"
if (-not (Test-Path -LiteralPath $handoffScript -PathType Leaf)) {
    Write-Error "tools/validate-renderer-commercial-readiness-final-handoff.ps1 must exist."
}

$fixtureRootRelative = "artifacts/renderer/public-runner-security-review/check-$PID"
$fixtureRoot = ConvertTo-LocalPath $fixtureRootRelative

try {
    Remove-TestRoot -RelativePath $fixtureRootRelative
    $null = New-Item -ItemType Directory -Force -Path $fixtureRoot

    $publicRepoJson = "$fixtureRootRelative/public-repo.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $publicRepoJson) -Value ([ordered]@{
            full_name = "owner/repo"
            private = $false
            visibility = "public"
        })

    $missingRunnerJson = "$fixtureRootRelative/missing-runners.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $missingRunnerJson) -Value ([ordered]@{
            total_count = 0
            runners = @()
        })

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
                quality_vfx_host_evidence = [ordered]@{
                    dependent_missing_inputs = @("metal_memory_profiling_host_evidence")
                }
            }
            assembler_handoff = [ordered]@{ ready = $false }
            final_preflight_handoff = [ordered]@{ ready = $false }
        })

    $outputRootRelative = "$fixtureRootRelative/generated-review"
    $reviewArtifactRelative = "$outputRootRelative/public-runner-security-review.json"

    $planLines = @(& $reviewScript `
            -Mode Plan `
            -RepoFullName "owner/repo" `
            -RepositoryJsonPath $publicRepoJson `
            -OutputRootRelative $outputRootRelative)
    foreach ($expectedLine in @(
            "validation_recipe=renderer-public-runner-security-review",
            "renderer_public_runner_security_review_status=review_required",
            "renderer_public_runner_security_review_ready=0",
            "renderer_public_runner_security_review_repository_visibility=public",
            "renderer_public_runner_security_review_required_labels=self-hosted,macOS,ARM64,metal-residency-set",
            "renderer_public_runner_security_review_allowed_workflows=renderer-metal-memory-profiling-capable-host.yml",
            "renderer_public_runner_security_review_artifact_written=0",
            "renderer_public_runner_security_review_registration_token_fetched=0",
            "renderer_public_runner_security_review_workflow_dispatched=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $planLines $expectedLine "public runner review plan"
    }
    Assert-LineAbsentPattern $planLines "registration-token-command|ghs_" "public runner review plan"

    $missingApprovalLines = @(& $reviewScript `
            -Mode Generate `
            -RepoFullName "owner/repo" `
            -RepositoryJsonPath $publicRepoJson `
            -OutputRootRelative $outputRootRelative `
            -NoWrite)
    foreach ($expectedLine in @(
            "renderer_public_runner_security_review_status=review_required",
            "renderer_public_runner_security_review_ready=0",
            "renderer_public_runner_security_review_blockers=approval_confirmation_required,public_fork_pr_risk_review_required,runner_isolation_review_required,secret_exposure_review_required,metal_probe_truth_review_required",
            "renderer_public_runner_security_review_artifact_written=0",
            "renderer_public_runner_security_review_registration_token_fetched=0",
            "renderer_public_runner_security_review_workflow_dispatched=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $missingApprovalLines $expectedLine "public runner review missing approvals"
    }

    $approvedLines = @(& $reviewScript `
            -Mode Generate `
            -RepoFullName "owner/repo" `
            -RepositoryJsonPath $publicRepoJson `
            -OutputRootRelative $outputRootRelative `
            -ApprovePublicRepoSelfHostedRunnerReview public-repo-self-hosted-runner-risk-reviewed `
            -ReviewedPublicForkPrRisk `
            -ReviewedRunnerIsolation `
            -ReviewedSecretExposure `
            -ReviewedMetalProbeTruth)
    foreach ($expectedLine in @(
            "renderer_public_runner_security_review_status=approved",
            "renderer_public_runner_security_review_ready=1",
            "renderer_public_runner_security_review_artifact=$reviewArtifactRelative",
            "renderer_public_runner_security_review_artifact_written=1",
            "renderer_public_runner_security_review_registration_token_endpoint=/repos/owner/repo/actions/runners/registration-token",
            "renderer_public_runner_security_review_registration_token_expires_minutes=60",
            "renderer_public_runner_security_review_registration_token_fetched=0",
            "renderer_public_runner_security_review_workflow_dispatched=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $approvedLines $expectedLine "public runner review approved"
    }
    Assert-LineAbsentPattern $approvedLines "registration-token-command|ghs_" "public runner review approved"

    $reviewArtifactPath = ConvertTo-LocalPath $reviewArtifactRelative
    if (-not (Test-Path -LiteralPath $reviewArtifactPath -PathType Leaf)) {
        Write-Error "public runner review approved artifact was not written: $reviewArtifactRelative"
    }
    $review = Get-Content -LiteralPath $reviewArtifactPath -Raw | ConvertFrom-Json
    if ([string]$review.schema_version -cne "GameEngine.RendererPublicSelfHostedRunnerSecurityReview.v1" -or
        [string]$review.review_status -cne "approved" -or
        -not [bool]$review.reviewed_public_fork_pr_risk -or
        -not [bool]$review.reviewed_runner_isolation -or
        -not [bool]$review.reviewed_secret_exposure -or
        -not [bool]$review.reviewed_metal_probe_truth) {
        Write-Error "public runner review artifact did not preserve approved review rows."
    }

    $handoffLines = @(& $handoffScript `
            -RepoFullName "owner/repo" `
            -RunnersJsonPath $missingRunnerJson `
            -RepositoryJsonPath $publicRepoJson `
            -PublicRepoRunnerSecurityReviewRelative $reviewArtifactRelative `
            -IntakeManifestRelative $blockedManifestRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_handoff_status=capable_host_runner_required",
            "renderer_commercial_readiness_final_handoff_next_action=provision_capable_host_runner",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_present=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_valid=1",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_status=approved",
            "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $handoffLines $expectedLine "public runner review handoff"
    }
}
finally {
    Remove-TestRoot -RelativePath $fixtureRootRelative
}

Write-Information "renderer-public-runner-security-review-check: ok" -InformationAction Continue
