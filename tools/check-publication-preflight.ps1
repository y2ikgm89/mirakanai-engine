#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$Remote = "origin",

    [string]$Branch = "",

    [ValidateRange(0, [int]::MaxValue)]
    [int]$PullRequest = 0,

    [string]$NetworkHost = "github.com",

    [ValidateRange(1, 65535)]
    [int]$NetworkPort = 443,

    [switch]$RequireClean
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

. (Join-Path $PSScriptRoot "common.ps1")

enum PublicationPreflightStatus {
    Ok
    Blocked
}

function New-PublicationPreflightResult {
    param()

    return [pscustomobject]@{
        Status = [PublicationPreflightStatus]::Ok
        Blockers = [System.Collections.Generic.List[string]]::new()
    }
}

function Add-PublicationPreflightBlocker {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$Detail
    )

    $Result.Status = [PublicationPreflightStatus]::Blocked
    $Result.Blockers.Add("kind=$Kind detail=$Detail") | Out-Null
}

function Invoke-PublicationPreflightProcess {
    param(
        [Parameter(Mandatory = $true)][string]$Command,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $processOutput = & $Command @Arguments 2>&1
    return [pscustomobject]@{
        ExitCode = $LASTEXITCODE
        Output = @($processOutput)
    }
}

function Invoke-PublicationPreflightGit {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    return Invoke-PublicationPreflightProcess -Command "git" -Arguments (@("-C", $RepoRoot) + $Arguments)
}

function Invoke-PublicationPreflightGh {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    return Invoke-PublicationPreflightProcess -Command "gh" -Arguments $Arguments
}

function Get-PublicationPreflightIdentity {
    param()

    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
    }

    return [System.Environment]::UserName
}

function Test-PublicationPreflightDirectoryWritable {
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][ref]$FailureDetail
    )

    $probePath = Join-Path $Directory ("publication-preflight-" + [guid]::NewGuid().ToString("N") + ".tmp")
    try {
        $bytes = [System.Text.Encoding]::ASCII.GetBytes("publication-preflight")
        $stream = [System.IO.File]::Open($probePath,
                                         [System.IO.FileMode]::CreateNew,
                                         [System.IO.FileAccess]::Write,
                                         [System.IO.FileShare]::None)
        try {
            $stream.Write($bytes, 0, $bytes.Length)
        } finally {
            $stream.Dispose()
        }
        [System.IO.File]::Delete($probePath)
        return $true
    } catch {
        $FailureDetail.Value = $_.Exception.Message
        try {
            if (Test-Path -LiteralPath $probePath -PathType Leaf) {
                [System.IO.File]::Delete($probePath)
            }
        } catch {
            $FailureDetail.Value = "$($FailureDetail.Value); cleanup failed: $($_.Exception.Message)"
        }
        return $false
    }
}

function Test-PublicationPreflightReadableFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][ref]$FailureDetail
    )

    try {
        $stream = [System.IO.File]::Open($Path,
                                         [System.IO.FileMode]::Open,
                                         [System.IO.FileAccess]::Read,
                                         [System.IO.FileShare]::ReadWrite)
        $stream.Dispose()
        return $true
    } catch {
        $FailureDetail.Value = $_.Exception.Message
        return $false
    }
}

function Get-PublicationPreflightGhConfigPath {
    param()

    $explicitConfigRoot = [Environment]::GetEnvironmentVariable("GH_CONFIG_DIR", "Process")
    if (-not [string]::IsNullOrWhiteSpace($explicitConfigRoot)) {
        return Join-Path $explicitConfigRoot "config.yml"
    }

    $appData = [Environment]::GetEnvironmentVariable("APPDATA", "Process")
    if (-not [string]::IsNullOrWhiteSpace($appData)) {
        return Join-Path (Join-Path $appData "GitHub CLI") "config.yml"
    }

    $configHome = [Environment]::GetEnvironmentVariable("XDG_CONFIG_HOME", "Process")
    if ([string]::IsNullOrWhiteSpace($configHome)) {
        $homePath = [Environment]::GetEnvironmentVariable("HOME", "Process")
        if ([string]::IsNullOrWhiteSpace($homePath)) {
            return $null
        }
        $configHome = Join-Path $homePath ".config"
    }

    return Join-Path (Join-Path $configHome "gh") "config.yml"
}

function Test-PublicationPreflight {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string]$RemoteName,
        [Parameter(Mandatory = $true)]
        [AllowEmptyString()]
        [string]$RequestedBranch,
        [int]$PullRequestNumber,
        [Parameter(Mandatory = $true)][string]$HostName,
        [int]$Port,
        [bool]$RequireCleanWorktree
    )

    $result = New-PublicationPreflightResult
    $identity = Get-PublicationPreflightIdentity
    Write-Information "publication-preflight: repo-root=$RepoRoot" -InformationAction Continue
    Write-Information "publication-preflight: user=$identity" -InformationAction Continue

    $topLevel = Invoke-PublicationPreflightGit -RepoRoot $RepoRoot -Arguments @("rev-parse", "--show-toplevel")
    if ($topLevel.ExitCode -ne 0) {
        Add-PublicationPreflightBlocker $result "git-repo-root" (@($topLevel.Output) -join "`n")
    }

    $resolvedBranch = $RequestedBranch
    if ([string]::IsNullOrWhiteSpace($resolvedBranch)) {
        $branchResult = Invoke-PublicationPreflightGit -RepoRoot $RepoRoot -Arguments @("branch", "--show-current")
        if ($branchResult.ExitCode -ne 0) {
            Add-PublicationPreflightBlocker $result "git-branch" (@($branchResult.Output) -join "`n")
        } else {
            $resolvedBranch = (@($branchResult.Output) | Select-Object -First 1).Trim()
        }
    }
    if ([string]::IsNullOrWhiteSpace($resolvedBranch)) {
        Add-PublicationPreflightBlocker $result "detached-head" "publication requires a named task branch"
    }
    Write-Information "publication-preflight: branch=$resolvedBranch" -InformationAction Continue

    $statusResult = Invoke-PublicationPreflightGit -RepoRoot $RepoRoot -Arguments @("status", "--short", "--branch")
    if ($statusResult.ExitCode -ne 0) {
        Add-PublicationPreflightBlocker $result "git-status" (@($statusResult.Output) -join "`n")
    } else {
        foreach ($statusLine in @($statusResult.Output)) {
            Write-Information "publication-preflight: git-status=$statusLine" -InformationAction Continue
        }
        if ($RequireCleanWorktree) {
            $dirtyLines = @($statusResult.Output | Where-Object { -not $_.StartsWith("##", [StringComparison]::Ordinal) })
            if ($dirtyLines.Count -gt 0) {
                Add-PublicationPreflightBlocker $result "dirty-worktree" (@($dirtyLines) -join "; ")
            }
        }
    }

    # Git index lock path probe intentionally runs: git rev-parse --path-format=absolute --git-path index.lock
    $indexLockResult = Invoke-PublicationPreflightGit -RepoRoot $RepoRoot -Arguments @(
        "rev-parse", "--path-format=absolute", "--git-path", "index.lock")
    if ($indexLockResult.ExitCode -ne 0) {
        Add-PublicationPreflightBlocker $result "git-index-lock-path" (@($indexLockResult.Output) -join "`n")
    } else {
        $indexLockPath = (@($indexLockResult.Output) | Select-Object -First 1).Trim()
        Write-Information "publication-preflight: git-index-lock=$indexLockPath" -InformationAction Continue
        if (Test-Path -LiteralPath $indexLockPath -PathType Leaf) {
            Add-PublicationPreflightBlocker $result "git-index-lock-present" $indexLockPath
        }
        $indexDirectory = Split-Path -Parent $indexLockPath
        $writeFailure = ""
        if (-not (Test-PublicationPreflightDirectoryWritable -Directory $indexDirectory -FailureDetail ([ref]$writeFailure))) {
            Add-PublicationPreflightBlocker $result "git-index-directory-acl" "$indexDirectory ($writeFailure)"
        }
    }

    # Git admin write probe intentionally runs: git update-index -q --refresh
    $refreshResult = Invoke-PublicationPreflightGit -RepoRoot $RepoRoot -Arguments @("update-index", "-q", "--refresh")
    if ($refreshResult.ExitCode -ne 0) {
        $refreshText = @($refreshResult.Output) -join "`n"
        if ($refreshText -match "index\.lock|Permission denied|Access is denied|Unable to create") {
            Add-PublicationPreflightBlocker $result "git-index-refresh-acl" $refreshText
        } else {
            Add-PublicationPreflightBlocker $result "git-index-refresh" $refreshText
        }
    }

    $remoteUrlResult = Invoke-PublicationPreflightGit -RepoRoot $RepoRoot -Arguments @("remote", "get-url", $RemoteName)
    if ($remoteUrlResult.ExitCode -ne 0) {
        Add-PublicationPreflightBlocker $result "git-remote" (@($remoteUrlResult.Output) -join "`n")
    } else {
        Write-Information "publication-preflight: remote=$RemoteName url=$((@($remoteUrlResult.Output) | Select-Object -First 1).Trim())" -InformationAction Continue
    }

    if (-not [string]::IsNullOrWhiteSpace($resolvedBranch)) {
        $remoteHeadResult = Invoke-PublicationPreflightGit -RepoRoot $RepoRoot -Arguments @(
            "ls-remote", "--heads", $RemoteName, $resolvedBranch)
        if ($remoteHeadResult.ExitCode -ne 0) {
            Add-PublicationPreflightBlocker $result "git-remote-network-or-auth" (@($remoteHeadResult.Output) -join "`n")
        } elseif (@($remoteHeadResult.Output).Count -eq 0) {
            Write-Information "publication-preflight: remote-head=missing branch=$resolvedBranch" -InformationAction Continue
        } else {
            Write-Information "publication-preflight: remote-head=present branch=$resolvedBranch" -InformationAction Continue
        }
    }

    $testNetConnectionCommand = Get-Command "Test-NetConnection" -ErrorAction SilentlyContinue
    if ($testNetConnectionCommand) {
        # Network port probe intentionally uses Test-NetConnection for github.com:443.
        $networkOk = Test-NetConnection -ComputerName $HostName -Port $Port -InformationLevel Quiet -WarningAction SilentlyContinue
        if ($networkOk) {
            Write-Information "publication-preflight: network=$HostName`:$Port reachable" -InformationAction Continue
        } else {
            Add-PublicationPreflightBlocker $result "github-network" "$HostName`:$Port is not reachable"
        }
    } else {
        Write-Information "publication-preflight: network-port-probe=unavailable command=Test-NetConnection" -InformationAction Continue
    }

    $ghConfigPath = Get-PublicationPreflightGhConfigPath
    if (-not [string]::IsNullOrWhiteSpace($ghConfigPath) -and (Test-Path -LiteralPath $ghConfigPath -PathType Leaf)) {
        $ghConfigFailure = ""
        if (-not (Test-PublicationPreflightReadableFile -Path $ghConfigPath -FailureDetail ([ref]$ghConfigFailure))) {
            Add-PublicationPreflightBlocker $result "gh-config-acl" "$ghConfigPath ($ghConfigFailure)"
        } else {
            Write-Information "publication-preflight: gh-config=readable" -InformationAction Continue
        }
    }

    # GitHub CLI auth probe intentionally runs: gh auth status
    $ghAuthResult = Invoke-PublicationPreflightGh -Arguments @("auth", "status")
    if ($ghAuthResult.ExitCode -ne 0) {
        Add-PublicationPreflightBlocker $result "gh-auth" (@($ghAuthResult.Output) -join "`n")
    } else {
        Write-Information "publication-preflight: gh-auth=ok" -InformationAction Continue
    }

    if ($PullRequestNumber -gt 0) {
        # PR publication probe intentionally runs: gh pr view --json headRefOid,statusCheckRollup,url
        $prResult = Invoke-PublicationPreflightGh -Arguments @(
            "pr", "view", $PullRequestNumber.ToString(), "--json", "headRefOid,statusCheckRollup,url")
        if ($prResult.ExitCode -ne 0) {
            Add-PublicationPreflightBlocker $result "gh-pr-view" (@($prResult.Output) -join "`n")
        } else {
            $prJson = @($prResult.Output) -join "`n"
            $pr = $prJson | ConvertFrom-Json
            Write-Information "publication-preflight: pr=$PullRequestNumber url=$($pr.url) headRefOid=$($pr.headRefOid)" -InformationAction Continue
        }
    }

    if ($result.Status -eq [PublicationPreflightStatus]::Blocked) {
        foreach ($blocker in $result.Blockers) {
            Write-Information "publication-preflight: blocked $blocker" -InformationAction Continue
        }
        Write-Error "publication-preflight: blocked blockers=$($result.Blockers.Count)"
    }

    Write-Information "publication-preflight: ok" -InformationAction Continue
}

$repoRoot = Get-RepoRoot
Test-PublicationPreflight -RepoRoot $repoRoot `
    -RemoteName $Remote `
    -RequestedBranch $Branch `
    -PullRequestNumber $PullRequest `
    -HostName $NetworkHost `
    -Port $NetworkPort `
    -RequireCleanWorktree $RequireClean.IsPresent
