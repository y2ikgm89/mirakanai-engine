#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)]
    [string]$WorktreePath,

    [string]$HeadRefOid = "",

    [string]$BaseRef = "origin/main",

    [string]$BaseBranch = "main",

    [string]$Remote = "origin",

    [string]$LocalCheckoutPath = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Invoke-GitCommand {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $output = & git -C $RepoRoot @Arguments 2>&1
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        Write-Error "git $($Arguments -join ' ') failed with exit code $exitCode`: $(@($output) -join "`n")"
    }

    return @($output)
}

$root = Get-RepoRoot
$resolvedLocalCheckoutPath = if ([string]::IsNullOrWhiteSpace($LocalCheckoutPath)) {
    $root
} else {
    (Resolve-Path -LiteralPath $LocalCheckoutPath).Path
}

if (-not [string]::IsNullOrWhiteSpace($HeadRefOid)) {
    $null = Invoke-GitCommand -RepoRoot $resolvedLocalCheckoutPath -Arguments @("fetch", "--prune", $Remote)
    $remainingCommits = Invoke-GitCommand -RepoRoot $resolvedLocalCheckoutPath -Arguments @("log", "--oneline", "${BaseRef}..${HeadRefOid}")
    $nonEmptyLines = @($remainingCommits | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($nonEmptyLines.Count -gt 0) {
        Write-Error @"
Refusing post-merge cleanup because head '$HeadRefOid' is not fully reachable from '$BaseRef'.
Remaining commits:
$($nonEmptyLines -join "`n")
"@
    }
}

$removeMergedWorktreeScript = Join-Path $root "tools/remove-merged-worktree.ps1"
if ($PSCmdlet.ShouldProcess($WorktreePath, "Run guarded post-merge cleanup with local and remote branch deletion")) {
    & pwsh -NoProfile -ExecutionPolicy Bypass -File $removeMergedWorktreeScript `
        -WorktreePath $WorktreePath `
        -BaseRef $BaseRef `
        -BaseBranch $BaseBranch `
        -Remote $Remote `
        -LocalCheckoutPath $resolvedLocalCheckoutPath `
        -DeleteLocalBranch `
        -DeleteRemoteBranch
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

Write-Information "post-merge-task-cleanup: ok worktree=$WorktreePath" -InformationAction Continue
