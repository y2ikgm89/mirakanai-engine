#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [ValidateNotNullOrEmpty()]
    [string]$Repository = "",

    [ValidateNotNullOrEmpty()]
    [string]$Branch = "main",

    [ValidateNotNullOrEmpty()]
    [string[]]$RequiredStatusCheck = @("PR Gate")
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Invoke-GitHubCliJson {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [switch]$AllowNotFound
    )

    $output = & gh @Arguments 2>&1
    $exitCode = $LASTEXITCODE
    $text = ($output | Out-String).Trim()

    if ($exitCode -ne 0) {
        $isNotFound =
            $text.Contains("HTTP 404") -or
            $text.Contains('"status":"404"') -or
            $text.Contains("Branch not protected")

        if ($AllowNotFound -and $isNotFound) {
            return $null
        }

        Write-Error "gh $($Arguments -join ' ') failed: $text"
    }

    if ([string]::IsNullOrWhiteSpace($text)) {
        return $null
    }

    return $text | ConvertFrom-Json
}

function Add-PolicyFailure {
    [CmdletBinding()]
    param(
        [System.Collections.Generic.List[string]]$Failures,

        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    $Failures.Add($Message)
}

function Get-RuleByType {
    [CmdletBinding()]
    param(
        [AllowNull()]
        $Rules,

        [Parameter(Mandatory = $true)]
        [string]$Type
    )

    foreach ($rule in @($Rules)) {
        if ($null -eq $rule) {
            continue
        }
        if ((Test-JsonProperty $rule "type") -and $rule.type -eq $Type) {
            return $rule
        }
    }

    return $null
}

function Get-StatusCheckContextsFromRule {
    [CmdletBinding()]
    param([AllowNull()]$Rule)

    if ($null -eq $Rule -or -not (Test-JsonProperty $Rule "parameters")) {
        return @()
    }

    $parameters = $Rule.parameters
    if (-not (Test-JsonProperty $parameters "required_status_checks")) {
        return @()
    }

    $contexts = @()
    foreach ($check in @($parameters.required_status_checks)) {
        if ((Test-JsonProperty $check "context") -and -not [string]::IsNullOrWhiteSpace($check.context)) {
            $contexts += $check.context
        }
    }

    return $contexts
}

function Get-StatusCheckContextsFromProtection {
    [CmdletBinding()]
    param([AllowNull()]$Protection)

    if ($null -eq $Protection -or -not (Test-JsonProperty $Protection "required_status_checks")) {
        return @()
    }

    $statusChecks = $Protection.required_status_checks
    $contexts = @()
    if (Test-JsonProperty $statusChecks "contexts") {
        $contexts += @($statusChecks.contexts)
    }
    if (Test-JsonProperty $statusChecks "checks") {
        foreach ($check in @($statusChecks.checks)) {
            if ((Test-JsonProperty $check "context") -and -not [string]::IsNullOrWhiteSpace($check.context)) {
                $contexts += $check.context
            }
        }
    }

    return @($contexts | Select-Object -Unique)
}

function Test-RulesetPolicy {
    [CmdletBinding()]
    param(
        [AllowNull()]$Rules,
        [string[]]$RequiredChecks
    )

    $pullRequestRule = Get-RuleByType -Rules $Rules -Type "pull_request"
    $statusRule = Get-RuleByType -Rules $Rules -Type "required_status_checks"
    $deletionRule = Get-RuleByType -Rules $Rules -Type "deletion"
    $nonFastForwardRule = Get-RuleByType -Rules $Rules -Type "non_fast_forward"

    if ($null -eq $pullRequestRule -or $null -eq $statusRule -or $null -eq $deletionRule -or $null -eq $nonFastForwardRule) {
        return $false
    }

    if (-not (Test-JsonProperty $statusRule "parameters") -or
        -not (Test-JsonProperty $statusRule.parameters "strict_required_status_checks_policy") -or
        -not [bool]$statusRule.parameters.strict_required_status_checks_policy) {
        return $false
    }

    $contexts = Get-StatusCheckContextsFromRule -Rule $statusRule
    foreach ($requiredCheck in $RequiredChecks) {
        if ($contexts -notcontains $requiredCheck) {
            return $false
        }
    }

    return $true
}

function Test-BranchProtectionPolicy {
    [CmdletBinding()]
    param(
        [AllowNull()]$Protection,
        [string[]]$RequiredChecks
    )

    if ($null -eq $Protection) {
        return $false
    }

    if (-not (Test-JsonProperty $Protection "required_pull_request_reviews") -or
        $null -eq $Protection.required_pull_request_reviews) {
        return $false
    }

    if (-not (Test-JsonProperty $Protection "required_status_checks") -or
        $null -eq $Protection.required_status_checks) {
        return $false
    }

    if ((Test-JsonProperty $Protection.required_status_checks "strict") -and
        -not [bool]$Protection.required_status_checks.strict) {
        return $false
    }

    foreach ($requiredCheck in $RequiredChecks) {
        if ((Get-StatusCheckContextsFromProtection -Protection $Protection) -notcontains $requiredCheck) {
            return $false
        }
    }

    if ((Test-JsonProperty $Protection "allow_force_pushes") -and
        (Test-JsonProperty $Protection.allow_force_pushes "enabled") -and
        [bool]$Protection.allow_force_pushes.enabled) {
        return $false
    }

    if ((Test-JsonProperty $Protection "allow_deletions") -and
        (Test-JsonProperty $Protection.allow_deletions "enabled") -and
        [bool]$Protection.allow_deletions.enabled) {
        return $false
    }

    return $true
}

if ([string]::IsNullOrWhiteSpace($Repository)) {
    $repoView = Invoke-GitHubCliJson -Arguments @("repo", "view", "--json", "nameWithOwner")
    $Repository = $repoView.nameWithOwner
}

if ([string]::IsNullOrWhiteSpace($Repository)) {
    Write-Error "Could not resolve GitHub repository. Pass -Repository owner/name."
}

$failures = [System.Collections.Generic.List[string]]::new()

$repo = Invoke-GitHubCliJson -Arguments @("api", "repos/$Repository")
$activeRules = Invoke-GitHubCliJson -Arguments @("api", "repos/$Repository/rules/branches/$Branch")
$branchProtection = Invoke-GitHubCliJson -Arguments @("api", "repos/$Repository/branches/$Branch/protection") -AllowNotFound

if (-not (Test-JsonProperty $repo "default_branch") -or $repo.default_branch -ne $Branch) {
    Add-PolicyFailure $failures "repository default_branch must be $Branch"
}
if (-not (Test-JsonProperty $repo "allow_auto_merge") -or -not [bool]$repo.allow_auto_merge) {
    Add-PolicyFailure $failures "repository allow_auto_merge must be true"
}
if (-not (Test-JsonProperty $repo "delete_branch_on_merge") -or -not [bool]$repo.delete_branch_on_merge) {
    Add-PolicyFailure $failures "repository delete_branch_on_merge must be true"
}
if (-not (Test-JsonProperty $repo "allow_merge_commit") -or -not [bool]$repo.allow_merge_commit) {
    Add-PolicyFailure $failures "repository allow_merge_commit must be true"
}
if ((Test-JsonProperty $repo "allow_squash_merge") -and [bool]$repo.allow_squash_merge) {
    Add-PolicyFailure $failures "repository allow_squash_merge must be false for the project merge-commit policy"
}
if ((Test-JsonProperty $repo "allow_rebase_merge") -and [bool]$repo.allow_rebase_merge) {
    Add-PolicyFailure $failures "repository allow_rebase_merge must be false for the project merge-commit policy"
}

$rulesetPolicyOk = Test-RulesetPolicy -Rules $activeRules -RequiredChecks $RequiredStatusCheck
$branchProtectionPolicyOk = Test-BranchProtectionPolicy -Protection $branchProtection -RequiredChecks $RequiredStatusCheck

if (-not $rulesetPolicyOk -and -not $branchProtectionPolicyOk) {
    Add-PolicyFailure $failures "$Branch must be protected by either an active ruleset or branch protection requiring pull requests, strict required status checks ($($RequiredStatusCheck -join ', ')), force-push blocking, and deletion blocking"
}

if ($failures.Count -gt 0) {
    foreach ($failure in $failures) {
        Write-Information "github-repository-policy-check: FAIL: $failure" -InformationAction Continue
    }
    Write-Error "github-repository-policy-check failed for $Repository $Branch"
}

Write-Information "github-repository-policy-check: ok" -InformationAction Continue
