#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10AL

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Generate")]
    [string]$Mode = "Plan",

    [Parameter(Mandatory = $true)]
    [string]$RepoFullName,

    [string]$RepositoryJsonPath = "",

    [string]$WorkflowRelative = ".github/workflows/renderer-metal-memory-profiling-capable-host.yml",

    [string]$OutputRootRelative = "artifacts/renderer/public-runner-security-review/renderer-commercial-readiness",

    [string]$ApprovePublicRepoSelfHostedRunnerReview = "",

    [switch]$ReviewedPublicForkPrRisk,

    [switch]$ReviewedRunnerIsolation,

    [switch]$ReviewedSecretExposure,

    [switch]$ReviewedMetalProbeTruth,

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$requiredLabels = @("self-hosted", "macOS", "ARM64", "metal-residency-set")
$allowedWorkflows = @("renderer-metal-memory-profiling-capable-host.yml")
$reviewRelative = "$OutputRootRelative/public-runner-security-review.json"

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function ConvertTo-CounterValue {
    param([Parameter(Mandatory = $true)][string]$Value)

    return ($Value -replace '[^A-Za-z0-9_.-]', '_')
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

function Test-AllowedOutputRoot {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/public-runner-security-review/",
        [System.StringComparison]::Ordinal) -or
        $normalizedPath.StartsWith(
            "artifacts/renderer/commercial-readiness-evidence/",
            [System.StringComparison]::Ordinal)
}

function Resolve-RepoRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-SafeRepoRelativePath -RelativePath $RelativePath)) {
        Write-Error "unsafe_relative_path: $Label must be repo-relative without absolute, drive-qualified, colon, backslash, or '..' segments."
    }
    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $root.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "unsafe_relative_path: $Label must resolve under the repository root."
    }
    return $fullPath
}

function Get-JsonPropertyValue {
    param(
        [AllowNull()]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if ($null -eq $JsonObject) {
        return $null
    }
    $property = $JsonObject.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    return $property.Value
}

function Read-JsonFile {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $fullPath = Resolve-RepoRelativePath -RelativePath $RelativePath -Label $Label
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        Write-Error "$Label does not exist: $RelativePath"
    }
    return Get-Content -LiteralPath $fullPath -Raw | ConvertFrom-Json
}

function New-WorkflowSecurityAudit {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string[]]$ExpectedLabels
    )

    $fullPath = Resolve-RepoRelativePath -RelativePath $RelativePath -Label "WorkflowRelative"
    $workflowExists = Test-Path -LiteralPath $fullPath -PathType Leaf
    $workflowText = if ($workflowExists) {
        Get-Content -LiteralPath $fullPath -Raw
    } else {
        ""
    }

    $workflowDispatchPresent = $workflowText -match '(?m)^\s*workflow_dispatch\s*:'
    $untrustedTriggerMatches = [regex]::Matches(
        $workflowText,
        '(?m)^\s*(pull_request|pull_request_target|push|schedule)\s*:'
    )
    $untrustedPrTriggersPresent = $untrustedTriggerMatches.Count -gt 0
    $workflowDispatchOnly = $workflowDispatchPresent -and -not $untrustedPrTriggersPresent
    $contentsPermissionReadOnly = $workflowText -match '(?m)^\s*contents\s*:\s*read\s*$' -and
        $workflowText -notmatch '(?m)^\s*contents\s*:\s*write\s*$'
    $expectedRunsOn = "runs-on: [$([string]::Join(', ', $ExpectedLabels))]"
    $requiredLabelsMatch = $workflowText.Contains($expectedRunsOn)
    $checkoutUseCount = [regex]::Matches(
        $workflowText,
        '(?m)^\s*uses\s*:\s*actions/checkout@[0-9a-f]{40}\s*(?:#.*)?$'
    ).Count
    $checkoutPersistFalseCount = [regex]::Matches(
        $workflowText,
        '(?m)^\s*persist-credentials\s*:\s*false\s*$'
    ).Count
    $checkoutActionPinned = $checkoutUseCount -gt 0
    $checkoutPersistCredentialsDisabled = $checkoutUseCount -gt 0 -and
        $checkoutPersistFalseCount -ge $checkoutUseCount
    $confirmInputRequired = $workflowText.Contains("confirm_capable_apple_host") -and
        $workflowText.Contains("MTLGPUFamilyApple6")
    $ready = $workflowExists -and
        $workflowDispatchOnly -and
        $contentsPermissionReadOnly -and
        $requiredLabelsMatch -and
        $checkoutActionPinned -and
        $checkoutPersistCredentialsDisabled -and
        $confirmInputRequired

    return [ordered]@{
        workflow_file = $RelativePath
        workflow_file_exists = $workflowExists
        workflow_dispatch_only = $workflowDispatchOnly
        untrusted_pr_triggers_present = $untrustedPrTriggersPresent
        contents_permission_read_only = $contentsPermissionReadOnly
        required_labels = $ExpectedLabels
        required_labels_match = $requiredLabelsMatch
        checkout_action_pinned = $checkoutActionPinned
        checkout_persist_credentials_disabled = $checkoutPersistCredentialsDisabled
        confirm_input_required = $confirmInputRequired
        ready = $ready
    }
}

function Invoke-GitHubRepositoryMetadata {
    param([Parameter(Mandatory = $true)][string]$RepositoryFullName)

    $endpoint = "/repos/$RepositoryFullName"
    $output = & gh api $endpoint -H "Accept: application/vnd.github+json" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "gh api $endpoint failed: $(@($output) -join "`n")"
    }
    return (@($output) -join "`n") | ConvertFrom-Json
}

function New-PublicRunnerSecurityReview {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryFullName,
        [Parameter(Mandatory = $true)][string]$RepositoryVisibility,
        [Parameter(Mandatory = $true)][string]$GeneratedUtc,
        [Parameter(Mandatory = $true)][string]$ReviewedWorkflowFile,
        [Parameter(Mandatory = $true)]$WorkflowAudit
    )

    return [ordered]@{
        schema_version = "GameEngine.RendererPublicSelfHostedRunnerSecurityReview.v1"
        validation_recipe = "renderer-public-runner-security-review"
        generated_utc = $GeneratedUtc
        repo_full_name = $RepositoryFullName
        repository_visibility = $RepositoryVisibility
        review_status = "approved"
        reviewed_public_fork_pr_risk = $true
        reviewed_runner_isolation = $true
        reviewed_secret_exposure = $true
        reviewed_allowed_workflows = $allowedWorkflows
        reviewed_required_labels = $requiredLabels
        reviewed_metal_probe_truth = $true
        reviewed_workflow_file = $ReviewedWorkflowFile
        workflow_audit = $WorkflowAudit
        official_source_rows = [ordered]@{
            github_actions_self_hosted_runner_security = "https://docs.github.com/en/actions/security-guides/security-hardening-for-github-actions"
            github_actions_secure_use_reference = "https://docs.github.com/en/actions/reference/secure-use-reference"
            github_actions_runner_labels = "https://docs.github.com/en/actions/how-tos/write-workflows/choose-where-workflows-run/choose-the-runner-for-a-job"
            github_rest_repository_runner_registration_token = "https://docs.github.com/en/rest/actions/self-hosted-runners#create-a-registration-token-for-a-repository"
            context7_actions_library_id = "/websites/github_en_actions"
            context7_rest_library_id = "/websites/github_en_rest"
        }
        non_actions = [ordered]@{
            registration_token_fetched = $false
            registration_token_printed = $false
            workflow_dispatched = $false
            runner_registered = $false
            gpu_workload_executed = $false
        }
        token_handoff = [ordered]@{
            endpoint = "/repos/$RepositoryFullName/actions/runners/registration-token"
            expires_minutes = 60
            token_value_retained = $false
        }
        non_claims = [ordered]@{
            runner_available = $false
            metal_memory_profiling_host_evidence_ready = $false
            renderer_backend_parity_ready = $false
            renderer_metal_broad_readiness = $false
            renderer_broad_quality_ready = $false
            renderer_commercial_readiness = $false
            renderer_environment_ready = $false
        }
    }
}

function Write-JsonObject {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][object]$Value
    )

    $fullPath = Resolve-RepoRelativePath -RelativePath $RelativePath -Label "OutputRootRelative"
    $parent = Split-Path -Parent $fullPath
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    $json = $Value | ConvertTo-Json -Depth 16
    Set-Content -LiteralPath $fullPath -Value $json -Encoding utf8NoBOM
}

if ($RepoFullName -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
    Write-Error "RepoFullName must be in owner/repo form."
}
if (-not (Test-SafeRepoRelativePath -RelativePath $WorkflowRelative)) {
    Write-Error "WorkflowRelative must be repo-relative without absolute, drive-qualified, colon, backslash, or '..' segments."
}
if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative) -or
    -not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/public-runner-security-review/ or artifacts/renderer/commercial-readiness-evidence/."
}
if (-not [string]::IsNullOrWhiteSpace($ApprovePublicRepoSelfHostedRunnerReview) -and
    $ApprovePublicRepoSelfHostedRunnerReview -cne "public-repo-self-hosted-runner-risk-reviewed") {
    Write-Error "ApprovePublicRepoSelfHostedRunnerReview must equal public-repo-self-hosted-runner-risk-reviewed when provided."
}

$repositoryMetadata = if (-not [string]::IsNullOrWhiteSpace($RepositoryJsonPath)) {
    Read-JsonFile -RelativePath $RepositoryJsonPath -Label "RepositoryJsonPath"
} else {
    Invoke-GitHubRepositoryMetadata -RepositoryFullName $RepoFullName
}

$repositoryVisibility = "unknown"
$visibilityValue = Get-JsonPropertyValue -JsonObject $repositoryMetadata -Name "visibility"
if (-not [string]::IsNullOrWhiteSpace([string]$visibilityValue)) {
    $repositoryVisibility = [string]$visibilityValue
}
$repositoryPrivateKnown = $false
$repositoryPrivate = $false
$privateValue = Get-JsonPropertyValue -JsonObject $repositoryMetadata -Name "private"
if ($null -ne $privateValue) {
    $repositoryPrivateKnown = $true
    $repositoryPrivate = [bool]$privateValue
}
$isPublicRepository = $repositoryVisibility -ceq "public" -or
    ($repositoryPrivateKnown -and -not $repositoryPrivate)

$workflowAudit = New-WorkflowSecurityAudit -RelativePath $WorkflowRelative -ExpectedLabels $requiredLabels

$blockers = [System.Collections.Generic.List[string]]::new()
if (-not $isPublicRepository) {
    $blockers.Add("public_repository_metadata_required") | Out-Null
}
if (-not [bool]$workflowAudit.ready) {
    $blockers.Add("workflow_security_audit_required") | Out-Null
}
if ($ApprovePublicRepoSelfHostedRunnerReview -cne "public-repo-self-hosted-runner-risk-reviewed") {
    $blockers.Add("approval_confirmation_required") | Out-Null
}
if (-not $ReviewedPublicForkPrRisk) {
    $blockers.Add("public_fork_pr_risk_review_required") | Out-Null
}
if (-not $ReviewedRunnerIsolation) {
    $blockers.Add("runner_isolation_review_required") | Out-Null
}
if (-not $ReviewedSecretExposure) {
    $blockers.Add("secret_exposure_review_required") | Out-Null
}
if (-not $ReviewedMetalProbeTruth) {
    $blockers.Add("metal_probe_truth_review_required") | Out-Null
}

$ready = $Mode -ceq "Generate" -and @($blockers).Count -eq 0
$status = if ($ready) { "approved" } else { "review_required" }
$artifactWritten = $false
if ($ready -and -not $NoWrite) {
    $review = New-PublicRunnerSecurityReview `
        -RepositoryFullName $RepoFullName `
        -RepositoryVisibility $repositoryVisibility `
        -GeneratedUtc ([System.DateTimeOffset]::UtcNow.ToString("o")) `
        -ReviewedWorkflowFile $WorkflowRelative `
        -WorkflowAudit $workflowAudit
    Write-JsonObject -RelativePath $reviewRelative -Value $review
    $artifactWritten = $true
}

Write-Output "validation_recipe=renderer-public-runner-security-review"
Write-Output "renderer_public_runner_security_review_repo=$RepoFullName"
Write-Output "renderer_public_runner_security_review_status=$status"
Write-Output "renderer_public_runner_security_review_ready=$(ConvertTo-CounterBit $ready)"
Write-Output "renderer_public_runner_security_review_repository_metadata_known=1"
Write-Output "renderer_public_runner_security_review_repository_visibility=$(ConvertTo-CounterValue -Value $repositoryVisibility)"
if ($repositoryPrivateKnown) {
    Write-Output "renderer_public_runner_security_review_repository_private=$(ConvertTo-CounterBit $repositoryPrivate)"
}
Write-Output "renderer_public_runner_security_review_required_labels=$([string]::Join(',', $requiredLabels))"
Write-Output "renderer_public_runner_security_review_allowed_workflows=$([string]::Join(',', $allowedWorkflows))"
Write-Output "renderer_public_runner_security_review_workflow_audit_file=$WorkflowRelative"
Write-Output "renderer_public_runner_security_review_workflow_audit_ready=$(ConvertTo-CounterBit ([bool]$workflowAudit.ready))"
Write-Output "renderer_public_runner_security_review_workflow_dispatch_only=$(ConvertTo-CounterBit ([bool]$workflowAudit.workflow_dispatch_only))"
Write-Output "renderer_public_runner_security_review_workflow_untrusted_pr_triggers=$(ConvertTo-CounterBit ([bool]$workflowAudit.untrusted_pr_triggers_present))"
Write-Output "renderer_public_runner_security_review_workflow_permissions_read_only=$(ConvertTo-CounterBit ([bool]$workflowAudit.contents_permission_read_only))"
Write-Output "renderer_public_runner_security_review_workflow_required_labels_match=$(ConvertTo-CounterBit ([bool]$workflowAudit.required_labels_match))"
Write-Output "renderer_public_runner_security_review_workflow_checkout_action_pinned=$(ConvertTo-CounterBit ([bool]$workflowAudit.checkout_action_pinned))"
Write-Output "renderer_public_runner_security_review_workflow_checkout_persist_credentials_disabled=$(ConvertTo-CounterBit ([bool]$workflowAudit.checkout_persist_credentials_disabled))"
Write-Output "renderer_public_runner_security_review_workflow_confirm_input_required=$(ConvertTo-CounterBit ([bool]$workflowAudit.confirm_input_required))"
Write-Output "renderer_public_runner_security_review_public_fork_pr_risk_reviewed=$(ConvertTo-CounterBit $ReviewedPublicForkPrRisk)"
Write-Output "renderer_public_runner_security_review_runner_isolation_reviewed=$(ConvertTo-CounterBit $ReviewedRunnerIsolation)"
Write-Output "renderer_public_runner_security_review_secret_exposure_reviewed=$(ConvertTo-CounterBit $ReviewedSecretExposure)"
Write-Output "renderer_public_runner_security_review_metal_probe_truth_reviewed=$(ConvertTo-CounterBit $ReviewedMetalProbeTruth)"
Write-Output "renderer_public_runner_security_review_confirmation_provided=$(ConvertTo-CounterBit ($ApprovePublicRepoSelfHostedRunnerReview -ceq 'public-repo-self-hosted-runner-risk-reviewed'))"
if (@($blockers).Count -gt 0) {
    Write-Output "renderer_public_runner_security_review_blockers=$([string]::Join(',', @($blockers)))"
}
if ($ready -or -not [string]::IsNullOrWhiteSpace($OutputRootRelative)) {
    Write-Output "renderer_public_runner_security_review_artifact=$reviewRelative"
}
Write-Output "renderer_public_runner_security_review_artifact_written=$(ConvertTo-CounterBit $artifactWritten)"
Write-Output "renderer_public_runner_security_review_registration_token_endpoint=/repos/$RepoFullName/actions/runners/registration-token"
Write-Output "renderer_public_runner_security_review_registration_token_expires_minutes=60"
Write-Output "renderer_public_runner_security_review_registration_token_fetched=0"
Write-Output "renderer_public_runner_security_review_registration_token_printed=0"
Write-Output "renderer_public_runner_security_review_workflow_dispatched=0"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

Write-Information "renderer-public-runner-security-review: ok" -InformationAction Continue
