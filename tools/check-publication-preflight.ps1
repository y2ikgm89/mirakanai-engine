#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$Branch = "",
    [switch]$SkipRemote,
    [switch]$SkipGhAuth
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Invoke-GitOutput {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $output = & git -C $RepoRoot @Arguments 2>&1
    return [pscustomobject]@{
        ExitCode = $LASTEXITCODE
        Output   = @($output)
    }
}

function Add-PreflightFailure {
    param(
        [System.Collections.Generic.List[string]]$Failures,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $Failures.Add($Message)
}

function Format-CommandOutput {
    param([object[]]$Output)

    $text = (@($Output) -join "`n").Trim()
    if ([string]::IsNullOrWhiteSpace($text)) {
        return "<no output>"
    }

    return $text
}

function Test-GitAdminWrite {
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [System.Collections.Generic.List[string]]$Failures
    )

    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) {
        Add-PreflightFailure -Failures $Failures -Message "Git admin directory does not exist: $Directory"
        return
    }

    $probePath = Join-Path $Directory "codex-publication-preflight-$([guid]::NewGuid().ToString("N")).tmp"
    try {
        [System.IO.File]::WriteAllText($probePath, "publication-preflight`n", [System.Text.UTF8Encoding]::new($false))
        Remove-Item -LiteralPath $probePath -Force -ErrorAction Stop
        Write-Information "publication-preflight: git-admin-write=ready" -InformationAction Continue
    } catch {
        if (Test-Path -LiteralPath $probePath) {
            try {
                Remove-Item -LiteralPath $probePath -Force -ErrorAction Stop
            } catch {
                Write-Information "publication-preflight: probe-cleanup-blocked=$probePath" -InformationAction Continue
            }
        }

        Add-PreflightFailure -Failures $Failures -Message "Cannot create and remove a temporary file in Git admin directory '$Directory': $($_.Exception.Message)"
    }
}

$root = Get-RepoRoot
$failures = [System.Collections.Generic.List[string]]::new()

Write-Information "publication-preflight: repo-root=$root" -InformationAction Continue

$insideWorktree = Invoke-GitOutput -RepoRoot $root -Arguments @("rev-parse", "--is-inside-work-tree")
if ($insideWorktree.ExitCode -ne 0 -or ((Format-CommandOutput $insideWorktree.Output) -ne "true")) {
    Write-Error "publication-preflight: not inside a Git worktree rooted at $root"
}

if ([string]::IsNullOrWhiteSpace($Branch)) {
    $branchResult = Invoke-GitOutput -RepoRoot $root -Arguments @("branch", "--show-current")
    if ($branchResult.ExitCode -eq 0) {
        $Branch = (Format-CommandOutput $branchResult.Output)
    }
}

if ([string]::IsNullOrWhiteSpace($Branch) -or $Branch -eq "<no output>") {
    Add-PreflightFailure -Failures $failures -Message "Current HEAD is detached or branch detection failed; pass -Branch <topic-branch>."
} else {
    Write-Information "publication-preflight: branch=$Branch" -InformationAction Continue
}

$gitDirResult = Invoke-GitOutput -RepoRoot $root -Arguments @("rev-parse", "--path-format=absolute", "--git-dir")
$gitCommonDirResult = Invoke-GitOutput -RepoRoot $root -Arguments @("rev-parse", "--path-format=absolute", "--git-common-dir")
$indexLockResult = Invoke-GitOutput -RepoRoot $root -Arguments @("rev-parse", "--path-format=absolute", "--git-path", "index.lock")

if ($gitDirResult.ExitCode -ne 0) {
    Add-PreflightFailure -Failures $failures -Message "Cannot resolve Git dir: $(Format-CommandOutput $gitDirResult.Output)"
}
if ($gitCommonDirResult.ExitCode -ne 0) {
    Add-PreflightFailure -Failures $failures -Message "Cannot resolve Git common dir: $(Format-CommandOutput $gitCommonDirResult.Output)"
}
if ($indexLockResult.ExitCode -ne 0) {
    Add-PreflightFailure -Failures $failures -Message "Cannot resolve Git index.lock path: $(Format-CommandOutput $indexLockResult.Output)"
} else {
    $gitDir = Format-CommandOutput $gitDirResult.Output
    $gitCommonDir = Format-CommandOutput $gitCommonDirResult.Output
    $indexLockPath = Format-CommandOutput $indexLockResult.Output
    $indexLockDirectory = Split-Path -Parent $indexLockPath
    $isLinkedWorktree = (ConvertTo-ComparablePath -Path $gitDir) -ne (ConvertTo-ComparablePath -Path $gitCommonDir)

    Write-Information "publication-preflight: git-dir=$gitDir" -InformationAction Continue
    Write-Information "publication-preflight: git-common-dir=$gitCommonDir" -InformationAction Continue
    Write-Information "publication-preflight: linked-worktree=$($isLinkedWorktree.ToString().ToLowerInvariant())" -InformationAction Continue
    Write-Information "publication-preflight: index-lock=$indexLockPath" -InformationAction Continue

    if (Test-Path -LiteralPath $indexLockPath) {
        Add-PreflightFailure -Failures $failures -Message "Git index lock already exists: $indexLockPath"
    }

    Test-GitAdminWrite -Directory $indexLockDirectory -Failures $failures
}

$originResult = Invoke-GitOutput -RepoRoot $root -Arguments @("remote", "get-url", "origin")
if ($originResult.ExitCode -ne 0) {
    Add-PreflightFailure -Failures $failures -Message "Cannot resolve origin remote: $(Format-CommandOutput $originResult.Output)"
} else {
    Write-Information "publication-preflight: origin=$(Format-CommandOutput $originResult.Output)" -InformationAction Continue
}

if ($SkipRemote) {
    Write-Information "publication-preflight: remote=skipped" -InformationAction Continue
} elseif ([string]::IsNullOrWhiteSpace($Branch) -or $Branch -eq "<no output>") {
    Add-PreflightFailure -Failures $failures -Message "Remote probe requires a topic branch; pass -Branch <topic-branch>."
} else {
    $remoteResult = Invoke-GitOutput -RepoRoot $root -Arguments @("ls-remote", "--heads", "origin", $Branch)
    if ($remoteResult.ExitCode -ne 0) {
        Add-PreflightFailure -Failures $failures -Message "Cannot reach origin for branch '$Branch': $(Format-CommandOutput $remoteResult.Output)"
    } else {
        Write-Information "publication-preflight: remote=ready" -InformationAction Continue
    }
}

if ($SkipGhAuth) {
    Write-Information "publication-preflight: gh-auth=skipped" -InformationAction Continue
} else {
    $ghOutput = & gh auth status 2>&1
    if ($LASTEXITCODE -ne 0) {
        Add-PreflightFailure -Failures $failures -Message "GitHub CLI auth unavailable: $(Format-CommandOutput @($ghOutput))"
    } else {
        Write-Information "publication-preflight: gh-auth=ready" -InformationAction Continue
    }
}

if ($failures.Count -gt 0) {
    foreach ($failure in $failures) {
        Write-Information "publication-preflight: blocker=$failure" -InformationAction Continue
    }

    Write-Error "publication-preflight: failed with $($failures.Count) blocker(s). Switch to a trusted local/full-access session with Git metadata write access, remote network access, and host-local GitHub authentication before commit/push/PR."
}

Write-Information "publication-preflight: ok" -InformationAction Continue
