#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)]
    [ValidateRange(1, [int]::MaxValue)]
    [int]$PullRequest,

    [string]$ExpectedBase = "main",

    [string]$Remote = "origin",

    [string]$RequiredGate = "PR Gate"
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Write-ReadyTaskPrError {
    param(
        [Parameter(Mandatory = $true)][string]$Message
    )

    Write-Error "Refusing to mark PR ready: $Message"
}

function Invoke-ProcessCommand {
    param(
        [Parameter(Mandatory = $true)][string]$Command,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $output = & $Command @Arguments 2>&1
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        Write-ReadyTaskPrError "$Command $($Arguments -join ' ') failed with exit code $exitCode`: $(@($output) -join "`n")"
    }

    return @($output)
}

function Invoke-GitCommand {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    return Invoke-ProcessCommand -Command "git" -Arguments (@("-C", $RepoRoot) + $Arguments)
}

function Invoke-GhCommand {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    return Invoke-ProcessCommand -Command "gh" -Arguments $Arguments
}

function Get-RequiredTextLine {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $line = @($Lines | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -First 1)
    if ($line.Count -eq 0) {
        Write-ReadyTaskPrError "missing $Description"
    }

    return $line[0].Trim()
}

function Read-PullRequest {
    param(
        [Parameter(Mandatory = $true)][int]$Number,
        [Parameter(Mandatory = $true)][string]$Fields
    )

    $json = (Invoke-GhCommand -Arguments @("pr", "view", $Number.ToString(), "--json", $Fields)) -join "`n"
    if ([string]::IsNullOrWhiteSpace($json)) {
        Write-ReadyTaskPrError "gh pr view returned no JSON for PR #$Number"
    }

    return $json | ConvertFrom-Json
}

function Test-TaskOwnedBranch {
    param(
        [Parameter(Mandatory = $true)][string]$Branch
    )

    return $Branch.StartsWith("codex/", [System.StringComparison]::Ordinal) -or
        $Branch.StartsWith("codex-", [System.StringComparison]::Ordinal)
}

function Get-CheckDisplayName {
    param(
        [Parameter(Mandatory = $true)]$Check
    )

    $name = ""
    if ($Check.PSObject.Properties.Name.Contains("name")) {
        $name = [string]$Check.name
    }
    if ([string]::IsNullOrWhiteSpace($name) -and $Check.PSObject.Properties.Name.Contains("context")) {
        $name = [string]$Check.context
    }
    if ([string]::IsNullOrWhiteSpace($name)) {
        $name = "<unnamed>"
    }

    return $name
}

function Test-SuccessfulStatusCheck {
    param(
        [Parameter(Mandatory = $true)]$Check,
        [Parameter(Mandatory = $true)][string]$RequiredGateName,
        [ref]$RequiredGateFound
    )

    $typeName = ""
    if ($Check.PSObject.Properties.Name.Contains("__typename")) {
        $typeName = [string]$Check.__typename
    }

    $name = Get-CheckDisplayName -Check $Check
    if ($typeName -eq "CheckRun") {
        if ($Check.status -ne "COMPLETED") {
            Write-ReadyTaskPrError "check '$name' is not completed: status=$($Check.status)"
        }

        $allowedConclusions = @("SUCCESS", "NEUTRAL", "SKIPPED")
        if ($allowedConclusions -notcontains $Check.conclusion) {
            Write-ReadyTaskPrError "check '$name' conclusion is not acceptable: conclusion=$($Check.conclusion)"
        }

        if ($name -eq $RequiredGateName -and $Check.conclusion -eq "SUCCESS") {
            $RequiredGateFound.Value = $true
        }

        return
    }

    if ($typeName -eq "StatusContext") {
        if ($Check.state -ne "SUCCESS") {
            Write-ReadyTaskPrError "status '$name' is not successful: state=$($Check.state)"
        }

        if ($name -eq $RequiredGateName) {
            $RequiredGateFound.Value = $true
        }

        return
    }

    Write-ReadyTaskPrError "unknown statusCheckRollup item '$name' type '$typeName'"
}

function Invoke-ReadyTaskPrPreflight {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][int]$Number,
        [Parameter(Mandatory = $true)][string]$ExpectedBaseBranch,
        [Parameter(Mandatory = $true)][string]$RemoteName,
        [Parameter(Mandatory = $true)][string]$RequiredGateName
    )

    $statusLines = Invoke-GitCommand -RepoRoot $RepoRoot -Arguments @("status", "--porcelain=v1")
    if (@($statusLines).Count -gt 0) {
        Write-ReadyTaskPrError "local worktree is not clean"
    }

    $currentBranch = Get-RequiredTextLine `
        -Lines (Invoke-GitCommand -RepoRoot $RepoRoot -Arguments @("branch", "--show-current")) `
        -Description "current branch"
    if (-not (Test-TaskOwnedBranch -Branch $currentBranch)) {
        Write-ReadyTaskPrError "current branch '$currentBranch' is not task-owned; expected codex/ or codex- prefix"
    }

    $localHead = Get-RequiredTextLine `
        -Lines (Invoke-GitCommand -RepoRoot $RepoRoot -Arguments @("rev-parse", "HEAD")) `
        -Description "local HEAD"

    $null = Invoke-GitCommand -RepoRoot $RepoRoot -Arguments @("fetch", "--prune", $RemoteName)
    $remoteHead = Get-RequiredTextLine `
        -Lines (Invoke-GitCommand -RepoRoot $RepoRoot -Arguments @("rev-parse", "--verify", "refs/remotes/$RemoteName/$currentBranch")) `
        -Description "remote tracking branch refs/remotes/$RemoteName/$currentBranch"
    if ($remoteHead -ne $localHead) {
        Write-ReadyTaskPrError "local HEAD $localHead does not match remote $RemoteName/$currentBranch $remoteHead"
    }

    $fields = "state,isDraft,baseRefName,headRefName,headRefOid,mergeable,mergeStateStatus,reviewDecision,statusCheckRollup,autoMergeRequest,url"
    $pullRequest = Read-PullRequest -Number $Number -Fields $fields

    if ($pullRequest.state -ne "OPEN") {
        Write-ReadyTaskPrError "PR #$Number state is $($pullRequest.state), expected OPEN"
    }
    if ($pullRequest.isDraft -ne $true) {
        Write-ReadyTaskPrError "PR #$Number is not a draft"
    }
    if ($pullRequest.baseRefName -ne $ExpectedBaseBranch) {
        Write-ReadyTaskPrError "PR #$Number base is '$($pullRequest.baseRefName)', expected '$ExpectedBaseBranch'"
    }
    if ($pullRequest.headRefName -ne $currentBranch) {
        Write-ReadyTaskPrError "PR #$Number head branch '$($pullRequest.headRefName)' does not match current branch '$currentBranch'"
    }
    if ($pullRequest.headRefOid -ne $localHead) {
        Write-ReadyTaskPrError "PR #$Number headRefOid $($pullRequest.headRefOid) does not match local HEAD $localHead"
    }
    if ($pullRequest.mergeable -ne "MERGEABLE") {
        Write-ReadyTaskPrError "PR #$Number mergeable is $($pullRequest.mergeable), expected MERGEABLE"
    }
    if ($pullRequest.mergeStateStatus -ne "CLEAN") {
        Write-ReadyTaskPrError "PR #$Number mergeStateStatus is $($pullRequest.mergeStateStatus), expected CLEAN"
    }
    if ($pullRequest.reviewDecision -eq "CHANGES_REQUESTED") {
        Write-ReadyTaskPrError "PR #$Number has CHANGES_REQUESTED reviewDecision"
    }

    $statusCheckRollup = @($pullRequest.statusCheckRollup)
    if ($statusCheckRollup.Count -eq 0) {
        Write-ReadyTaskPrError "PR #$Number has no statusCheckRollup entries"
    }

    $requiredGateFound = $false
    foreach ($check in $statusCheckRollup) {
        Test-SuccessfulStatusCheck -Check $check -RequiredGateName $RequiredGateName -RequiredGateFound ([ref]$requiredGateFound)
    }
    if (-not $requiredGateFound) {
        Write-ReadyTaskPrError "required status check '$RequiredGateName' did not report SUCCESS"
    }

    return [pscustomobject]@{
        PullRequest = $pullRequest
        Branch = $currentBranch
        Head = $localHead
    }
}

$root = Get-RepoRoot
$preflight = Invoke-ReadyTaskPrPreflight `
    -RepoRoot $root `
    -Number $PullRequest `
    -ExpectedBaseBranch $ExpectedBase `
    -RemoteName $Remote `
    -RequiredGateName $RequiredGate

$target = "PR #$PullRequest ($($preflight.PullRequest.url))"
if ($PSCmdlet.ShouldProcess($target, "Mark task-owned draft pull request ready for review after guarded preflight")) {
    $null = Invoke-GhCommand -Arguments @("pr", "ready", $PullRequest.ToString())
    $updatedPullRequest = Read-PullRequest -Number $PullRequest -Fields "state,isDraft,url"
    if ($updatedPullRequest.state -ne "OPEN" -or $updatedPullRequest.isDraft -eq $true) {
        Write-ReadyTaskPrError "gh pr ready did not leave PR #$PullRequest open and ready"
    }

    Write-Information "ready-task-pr: ready pr=$PullRequest branch=$($preflight.Branch) head=$($preflight.Head) url=$($updatedPullRequest.url)" -InformationAction Continue
}
