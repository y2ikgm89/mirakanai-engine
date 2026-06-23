#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$Repository = "",

    [string]$RulesetName = "main-pr-gate",

    [string]$RequiredStatusCheck = "PR Gate",

    [string]$DefaultBranch = "main"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

. (Join-Path $PSScriptRoot "common.ps1")

function Invoke-GitHubRulesetCheckGh {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    $processOutput = & gh @Arguments 2>&1
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        Write-Error "github-ruleset-check: gh $($Arguments -join ' ') failed with exit code $exitCode`: $(@($processOutput) -join "`n")"
    }

    return @($processOutput) -join "`n"
}

function Assert-GitHubRulesetCheck {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if (-not $Condition) {
        Write-Error "github-ruleset-check: $Message"
    }
}

function Get-GitHubRulesetCheckProperty {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Test-GitHubRulesetCheckHasRule {
    param(
        [Parameter(Mandatory = $true)][object[]]$Rules,
        [Parameter(Mandatory = $true)][string]$Type
    )

    return @($Rules | Where-Object { [string]$_.type -eq $Type }).Count -gt 0
}

function Get-GitHubRulesetCheckRule {
    param(
        [Parameter(Mandatory = $true)][object[]]$Rules,
        [Parameter(Mandatory = $true)][string]$Type
    )

    return @($Rules | Where-Object { [string]$_.type -eq $Type } | Select-Object -First 1)
}

if ([string]::IsNullOrWhiteSpace($Repository)) {
    $Repository = (Invoke-GitHubRulesetCheckGh -Arguments @("repo", "view", "--json", "nameWithOwner", "--jq", ".nameWithOwner")).Trim()
}
Assert-GitHubRulesetCheck -Condition (-not [string]::IsNullOrWhiteSpace($Repository)) -Message "repository could not be resolved"

$repositoryJson = Invoke-GitHubRulesetCheckGh -Arguments @("api", "repos/$Repository")
$repositoryInfo = $repositoryJson | ConvertFrom-Json
Assert-GitHubRulesetCheck -Condition ([string]$repositoryInfo.default_branch -eq $DefaultBranch) -Message "expected default_branch=$DefaultBranch but got $($repositoryInfo.default_branch)"
Assert-GitHubRulesetCheck -Condition ([bool]$repositoryInfo.allow_auto_merge) -Message "expected allow_auto_merge=true"
Assert-GitHubRulesetCheck -Condition ([bool]$repositoryInfo.allow_update_branch) -Message "expected allow_update_branch=true"
Assert-GitHubRulesetCheck -Condition ([bool]$repositoryInfo.delete_branch_on_merge) -Message "expected delete_branch_on_merge=true"

$rulesetsJson = Invoke-GitHubRulesetCheckGh -Arguments @("api", "repos/$Repository/rulesets")
$rulesets = @($rulesetsJson | ConvertFrom-Json)
$ruleset = @($rulesets | Where-Object { [string]$_.name -eq $RulesetName } | Select-Object -First 1)
Assert-GitHubRulesetCheck -Condition ($ruleset.Count -eq 1) -Message "missing repository ruleset '$RulesetName'"

$rulesetId = [int](Get-GitHubRulesetCheckProperty -Object $ruleset[0] -Name "id")
$rulesetDetailJson = Invoke-GitHubRulesetCheckGh -Arguments @("api", "repos/$Repository/rulesets/$rulesetId")
$rulesetDetail = $rulesetDetailJson | ConvertFrom-Json
Assert-GitHubRulesetCheck -Condition ([string]$rulesetDetail.name -eq $RulesetName) -Message "ruleset id $rulesetId has unexpected name $($rulesetDetail.name)"
Assert-GitHubRulesetCheck -Condition ([string]$rulesetDetail.target -eq "branch") -Message "ruleset '$RulesetName' must target branches"
Assert-GitHubRulesetCheck -Condition ([string]$rulesetDetail.enforcement -eq "active") -Message "ruleset '$RulesetName' must be active"
Assert-GitHubRulesetCheck -Condition (@($rulesetDetail.bypass_actors).Count -eq 0) -Message "ruleset '$RulesetName' must not have bypass actors"

$includedRefs = @($rulesetDetail.conditions.ref_name.include)
Assert-GitHubRulesetCheck -Condition ($includedRefs -contains "~DEFAULT_BRANCH") -Message "ruleset '$RulesetName' must include ~DEFAULT_BRANCH"
Assert-GitHubRulesetCheck -Condition (@($rulesetDetail.conditions.ref_name.exclude).Count -eq 0) -Message "ruleset '$RulesetName' must not exclude refs"

$rules = @($rulesetDetail.rules)
Assert-GitHubRulesetCheck -Condition (Test-GitHubRulesetCheckHasRule -Rules $rules -Type "deletion") -Message "ruleset '$RulesetName' must block branch deletion"
Assert-GitHubRulesetCheck -Condition (Test-GitHubRulesetCheckHasRule -Rules $rules -Type "non_fast_forward") -Message "ruleset '$RulesetName' must block non-fast-forward updates"

$pullRequestRule = Get-GitHubRulesetCheckRule -Rules $rules -Type "pull_request"
Assert-GitHubRulesetCheck -Condition ($pullRequestRule.Count -eq 1) -Message "ruleset '$RulesetName' must require pull requests"
$pullRequestParameters = $pullRequestRule[0].parameters
Assert-GitHubRulesetCheck -Condition (@($pullRequestParameters.allowed_merge_methods) -contains "merge") -Message "pull request rule must allow merge commits"
Assert-GitHubRulesetCheck -Condition ([int]$pullRequestParameters.required_approving_review_count -eq 0) -Message "pull request rule must not require human approvals for this single-operator repo"
Assert-GitHubRulesetCheck -Condition ([bool]$pullRequestParameters.required_review_thread_resolution) -Message "pull request rule must require review thread resolution"
Assert-GitHubRulesetCheck -Condition (-not [bool]$pullRequestParameters.require_code_owner_review) -Message "pull request rule must not require code owner review"
Assert-GitHubRulesetCheck -Condition (-not [bool]$pullRequestParameters.dismiss_stale_reviews_on_push) -Message "pull request rule must not dismiss stale reviews when approvals are not required"
Assert-GitHubRulesetCheck -Condition (-not [bool]$pullRequestParameters.require_last_push_approval) -Message "pull request rule must not require last-push approval for this single-operator repo"

$statusRule = Get-GitHubRulesetCheckRule -Rules $rules -Type "required_status_checks"
Assert-GitHubRulesetCheck -Condition ($statusRule.Count -eq 1) -Message "ruleset '$RulesetName' must require status checks"
$statusParameters = $statusRule[0].parameters
Assert-GitHubRulesetCheck -Condition ([bool]$statusParameters.strict_required_status_checks_policy) -Message "required status checks must require the latest default branch"
Assert-GitHubRulesetCheck -Condition ([bool]$statusParameters.do_not_enforce_on_create) -Message "required status checks should not block branch creation"
$requiredChecks = @($statusParameters.required_status_checks)
Assert-GitHubRulesetCheck -Condition (@($requiredChecks | Where-Object { [string]$_.context -eq $RequiredStatusCheck }).Count -eq 1) -Message "ruleset '$RulesetName' must require '$RequiredStatusCheck'"
Assert-GitHubRulesetCheck -Condition (@($requiredChecks | Where-Object { [string]$_.context -ne $RequiredStatusCheck }).Count -eq 0) -Message "ruleset '$RulesetName' must not require individual heavy CI lanes"

$branchRulesJson = Invoke-GitHubRulesetCheckGh -Arguments @("api", "repos/$Repository/rules/branches/$DefaultBranch")
$branchRules = @($branchRulesJson | ConvertFrom-Json)
Assert-GitHubRulesetCheck -Condition (@($branchRules | Where-Object { [int]$_.ruleset_id -eq $rulesetId -and [string]$_.type -eq "required_status_checks" }).Count -eq 1) -Message "ruleset '$RulesetName' is not applied to branch '$DefaultBranch'"

Write-Information "github-ruleset-check: repo=$Repository" -InformationAction Continue
Write-Information "github-ruleset-check: ruleset=$RulesetName id=$rulesetId" -InformationAction Continue
Write-Information "github-ruleset-check: required_status_check=$RequiredStatusCheck" -InformationAction Continue
Write-Information "github-ruleset-check: ok" -InformationAction Continue
