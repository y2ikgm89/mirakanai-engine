#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)]
    [string]$WorktreePath,

    [string]$BaseRef = "origin/main",

    [string]$BaseBranch = "main",

    [string]$Remote = "origin",

    [string]$LocalCheckoutPath = "",

    [switch]$DeleteLocalBranch
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Invoke-GitCommand {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [switch]$AllowFailure
    )

    $output = & git -C $RepoRoot @Arguments 2>&1
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0 -and -not $AllowFailure.IsPresent) {
        Write-Error "git $($Arguments -join ' ') failed with exit code $exitCode`: $(@($output) -join "`n")"
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = @($output)
    }
}

function ConvertTo-ComparablePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    return $fullPath.TrimEnd([char[]]@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
}

function Test-PathUnderDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Directory
    )

    $comparison = if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        [System.StringComparison]::OrdinalIgnoreCase
    } else {
        [System.StringComparison]::Ordinal
    }

    $candidate = ConvertTo-ComparablePath -Path $Path
    $rootDirectory = ConvertTo-ComparablePath -Path $Directory
    if ($candidate.Equals($rootDirectory, $comparison)) {
        return $true
    }

    $separator = [System.IO.Path]::DirectorySeparatorChar
    return $candidate.StartsWith("$rootDirectory$separator", $comparison)
}

function Resolve-RepoPath {
    param(
        [Parameter(Mandatory = $true)][string]$RepoPath,
        [Parameter(Mandatory = $true)][string]$GitPath
    )

    if ([System.IO.Path]::IsPathRooted($GitPath)) {
        return (Resolve-Path -LiteralPath $GitPath).Path
    }

    return (Resolve-Path -LiteralPath (Join-Path $RepoPath $GitPath)).Path
}

function Get-GitWorktreeRecords {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot
    )

    $lines = (Invoke-GitCommand -RepoRoot $RepoRoot -Arguments @("worktree", "list", "--porcelain")).Output
    $records = @()
    $current = $null

    foreach ($line in $lines) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            if ($null -ne $current) {
                $records += [pscustomobject]$current
                $current = $null
            }
            continue
        }

        if ($line.StartsWith("worktree ")) {
            if ($null -ne $current) {
                $records += [pscustomobject]$current
            }
            $current = [ordered]@{
                Path = $line.Substring("worktree ".Length)
                Branch = ""
                Head = ""
                Detached = $false
                Bare = $false
                Locked = $false
            }
            continue
        }

        if ($null -eq $current) {
            Write-Error "Unexpected git worktree porcelain line before worktree record: $line"
        }

        if ($line.StartsWith("branch ")) {
            $branchRef = $line.Substring("branch ".Length)
            if ($branchRef.StartsWith("refs/heads/")) {
                $current.Branch = $branchRef.Substring("refs/heads/".Length)
            } else {
                $current.Branch = $branchRef
            }
        } elseif ($line.StartsWith("HEAD ")) {
            $current.Head = $line.Substring("HEAD ".Length)
        } elseif ($line -eq "detached") {
            $current.Detached = $true
        } elseif ($line -eq "bare") {
            $current.Bare = $true
        } elseif ($line.StartsWith("locked")) {
            $current.Locked = $true
        }
    }

    if ($null -ne $current) {
        $records += [pscustomobject]$current
    }

    return $records
}

$root = Get-RepoRoot
$resolvedWorktreePath = (Resolve-Path -LiteralPath $WorktreePath).Path
$resolvedLocalCheckoutPath = if ([string]::IsNullOrWhiteSpace($LocalCheckoutPath)) {
    $root
} else {
    (Resolve-Path -LiteralPath $LocalCheckoutPath).Path
}

$comparableRoot = ConvertTo-ComparablePath -Path $root
$comparableWorktreePath = ConvertTo-ComparablePath -Path $resolvedWorktreePath
$comparableLocalCheckoutPath = ConvertTo-ComparablePath -Path $resolvedLocalCheckoutPath

if ($comparableWorktreePath -eq $comparableRoot) {
    Write-Error "Refusing to remove the current repository root as a worktree: $resolvedWorktreePath"
}
if ($comparableWorktreePath -eq $comparableLocalCheckoutPath) {
    Write-Error "Refusing to update and remove the same worktree path: $resolvedWorktreePath"
}

$standardWorktreeRoots = @(
    (Join-Path $root ".worktrees"),
    (Join-Path (Join-Path $root ".claude") "worktrees"),
    (Join-Path $resolvedLocalCheckoutPath ".worktrees"),
    (Join-Path (Join-Path $resolvedLocalCheckoutPath ".claude") "worktrees")
)

$isUnderStandardRoot = $false
foreach ($standardWorktreeRoot in $standardWorktreeRoots) {
    if (Test-PathUnderDirectory -Path $resolvedWorktreePath -Directory $standardWorktreeRoot) {
        $isUnderStandardRoot = $true
        break
    }
}

if (-not $isUnderStandardRoot) {
    Write-Error "Refusing post-merge cleanup outside standard ignored worktree roots (.worktrees/ or .claude/worktrees/): $resolvedWorktreePath"
}

$matchingRecords = @(Get-GitWorktreeRecords -RepoRoot $root | Where-Object {
        (ConvertTo-ComparablePath -Path $_.Path) -eq $comparableWorktreePath
    })

if ($matchingRecords.Count -ne 1) {
    Write-Error "Expected exactly one Git worktree record for $resolvedWorktreePath; found $($matchingRecords.Count)."
}

$record = $matchingRecords[0]
if ($record.Bare) {
    Write-Error "Refusing to remove a bare worktree record: $resolvedWorktreePath"
}
if ($record.Locked) {
    Write-Error "Refusing to remove a locked worktree: $resolvedWorktreePath"
}
if ([string]::IsNullOrWhiteSpace($record.Branch)) {
    Write-Error "Refusing post-merge cleanup for detached worktree without a local branch: $resolvedWorktreePath"
}
if ($record.Branch -in @("main", "master", $BaseBranch)) {
    Write-Error "Refusing to remove default branch worktree for branch '$($record.Branch)'"
}

$statusLines = (Invoke-GitCommand -RepoRoot $resolvedWorktreePath -Arguments @("status", "--porcelain")).Output
if ($statusLines.Count -ne 0) {
    Write-Error "Refusing to remove dirty worktree $resolvedWorktreePath. Commit, discard, or move these changes first:`n$($statusLines -join "`n")"
}

$rootCommonDir = Resolve-RepoPath -RepoPath $root -GitPath ((Invoke-GitCommand -RepoRoot $root -Arguments @("rev-parse", "--git-common-dir")).Output | Select-Object -First 1)
$localCommonDir = Resolve-RepoPath -RepoPath $resolvedLocalCheckoutPath -GitPath ((Invoke-GitCommand -RepoRoot $resolvedLocalCheckoutPath -Arguments @("rev-parse", "--git-common-dir")).Output | Select-Object -First 1)
if ((ConvertTo-ComparablePath -Path $rootCommonDir) -ne (ConvertTo-ComparablePath -Path $localCommonDir)) {
    Write-Error "Refusing to update local checkout from a different Git repository: $resolvedLocalCheckoutPath"
}

$localBranch = (Invoke-GitCommand -RepoRoot $resolvedLocalCheckoutPath -Arguments @("branch", "--show-current")).Output | Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($localBranch)) {
    Write-Error "Refusing to update detached local checkout: $resolvedLocalCheckoutPath"
}
if ($localBranch -ne $BaseBranch) {
    Write-Error "Refusing to update local checkout on branch '$localBranch'. Switch it to '$BaseBranch' or pass the intended -BaseBranch explicitly."
}

$localStatusLines = (Invoke-GitCommand -RepoRoot $resolvedLocalCheckoutPath -Arguments @("status", "--porcelain")).Output
if ($localStatusLines.Count -ne 0) {
    Write-Error "Refusing to update dirty local checkout $resolvedLocalCheckoutPath. Commit, discard, or move these changes first:`n$($localStatusLines -join "`n")"
}

Write-Information "remove-merged-worktree: fetch-prune=$Remote" -InformationAction Continue
$null = Invoke-GitCommand -RepoRoot $root -Arguments @("fetch", "--prune", $Remote)

$baseCommit = (Invoke-GitCommand -RepoRoot $root -Arguments @("rev-parse", "--verify", "$BaseRef^{commit}")).Output | Select-Object -First 1
$branchCommit = (Invoke-GitCommand -RepoRoot $root -Arguments @("rev-parse", "--verify", "refs/heads/$($record.Branch)^{commit}")).Output | Select-Object -First 1
$mergeBaseResult = Invoke-GitCommand -RepoRoot $root -Arguments @("merge-base", "--is-ancestor", $branchCommit, $baseCommit) -AllowFailure
if ($mergeBaseResult.ExitCode -eq 1) {
    Write-Error "Refusing to remove worktree because branch '$($record.Branch)' is not fully merged into '$BaseRef'. Run git fetch --prune $Remote or use the correct -BaseRef after the PR is merged."
}
if ($mergeBaseResult.ExitCode -ne 0) {
    Write-Error "git merge-base --is-ancestor failed with exit code $($mergeBaseResult.ExitCode): $(@($mergeBaseResult.Output) -join "`n")"
}

Write-Information "remove-merged-worktree: repo-root=$root" -InformationAction Continue
Write-Information "remove-merged-worktree: worktree=$resolvedWorktreePath" -InformationAction Continue
Write-Information "remove-merged-worktree: branch=$($record.Branch)" -InformationAction Continue
Write-Information "remove-merged-worktree: base-ref=$BaseRef" -InformationAction Continue
Write-Information "remove-merged-worktree: local-checkout=$resolvedLocalCheckoutPath" -InformationAction Continue
Write-Information "remove-merged-worktree: local-branch=$localBranch" -InformationAction Continue

if ($PSCmdlet.ShouldProcess($resolvedLocalCheckoutPath, "Fast-forward local checkout to $BaseRef")) {
    $null = Invoke-GitCommand -RepoRoot $resolvedLocalCheckoutPath -Arguments @("merge", "--ff-only", $BaseRef)
}

if ($PSCmdlet.ShouldProcess($resolvedWorktreePath, "Remove merged Git worktree")) {
    $null = Invoke-GitCommand -RepoRoot $root -Arguments @("worktree", "remove", $resolvedWorktreePath)
}

if ($DeleteLocalBranch.IsPresent) {
    if ($PSCmdlet.ShouldProcess($record.Branch, "Delete merged local branch")) {
        # The script already proved ancestry against BaseRef; -D avoids branch -d using the runner's current HEAD as the merge target.
        $null = Invoke-GitCommand -RepoRoot $root -Arguments @("branch", "-D", $record.Branch)
    }
}

if ($PSCmdlet.ShouldProcess($root, "Prune stale Git worktree metadata")) {
    $null = Invoke-GitCommand -RepoRoot $root -Arguments @("worktree", "prune")
}

Write-Information "remove-merged-worktree: ok" -InformationAction Continue
