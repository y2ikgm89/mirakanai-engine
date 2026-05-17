#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$VcpkgRoot = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Get-GitOutput {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $output = & git -C $RepoRoot @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "git $($Arguments -join ' ') failed with exit code $LASTEXITCODE`: $(@($output) -join "`n")"
    }

    return @($output)
}

function ConvertTo-ComparablePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    return $fullPath.TrimEnd([char[]]@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
}

function Test-GitIgnoredPath {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string]$RelativePath
    )

    & git -C $RepoRoot check-ignore -q -- $RelativePath
    return $LASTEXITCODE -eq 0
}

function Test-VcpkgCheckout {
    param(
        [Parameter(Mandatory = $true)][string]$Path
    )

    $toolchainPath = Join-Path $Path "scripts/buildsystems/vcpkg.cmake"
    return (Test-Path -LiteralPath $toolchainPath -PathType Leaf)
}

function Resolve-WorktreeDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string]$CurrentRoot,
        [Parameter(Mandatory = $true)][string]$RelativePath
    )

    $currentComparableRoot = ConvertTo-ComparablePath -Path $CurrentRoot
    foreach ($line in Get-GitOutput -RepoRoot $RepoRoot -Arguments @("worktree", "list", "--porcelain")) {
        if (-not $line.StartsWith("worktree ")) {
            continue
        }

        $worktreeRoot = $line.Substring("worktree ".Length)
        if ((ConvertTo-ComparablePath -Path $worktreeRoot) -eq $currentComparableRoot) {
            continue
        }

        $candidate = Join-Path $worktreeRoot $RelativePath
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return $null
}

function Resolve-VcpkgCheckout {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string]$CurrentRoot,
        [string]$RequestedVcpkgRoot = ""
    )

    if (-not [string]::IsNullOrWhiteSpace($RequestedVcpkgRoot)) {
        $resolvedRequestedRoot = (Resolve-Path -LiteralPath $RequestedVcpkgRoot).Path
        if (-not (Test-VcpkgCheckout -Path $resolvedRequestedRoot)) {
            Write-Error "Requested VcpkgRoot is not a vcpkg checkout with scripts/buildsystems/vcpkg.cmake: $resolvedRequestedRoot"
        }

        return $resolvedRequestedRoot
    }

    $currentComparableRoot = ConvertTo-ComparablePath -Path $CurrentRoot
    foreach ($line in Get-GitOutput -RepoRoot $RepoRoot -Arguments @("worktree", "list", "--porcelain")) {
        if (-not $line.StartsWith("worktree ")) {
            continue
        }

        $worktreeRoot = $line.Substring("worktree ".Length)
        if ((ConvertTo-ComparablePath -Path $worktreeRoot) -eq $currentComparableRoot) {
            continue
        }

        $candidate = Join-Path $worktreeRoot "external/vcpkg"
        if (Test-VcpkgCheckout -Path $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return $null
}

$root = Get-RepoRoot
$externalDir = Join-Path $root "external"
$localVcpkgRoot = Join-Path $externalDir "vcpkg"
$localVcpkgInstalledRoot = Join-Path $root "vcpkg_installed"

foreach ($ignoredPath in @(".worktrees/", ".claude/worktrees/", "external/", "vcpkg_installed/")) {
    if (-not (Test-GitIgnoredPath -RepoRoot $root -RelativePath $ignoredPath)) {
        Write-Error "Required worktree-local path is not ignored by Git: $ignoredPath"
    }
}

$gitDir = (Get-GitOutput -RepoRoot $root -Arguments @("rev-parse", "--path-format=absolute", "--git-dir") | Select-Object -First 1)
$gitCommonDir = (Get-GitOutput -RepoRoot $root -Arguments @("rev-parse", "--path-format=absolute", "--git-common-dir") | Select-Object -First 1)
$isLinkedWorktree = (ConvertTo-ComparablePath -Path $gitDir) -ne (ConvertTo-ComparablePath -Path $gitCommonDir)

Write-Information "prepare-worktree: repo-root=$root" -InformationAction Continue
Write-Information "prepare-worktree: linked-worktree=$($isLinkedWorktree.ToString().ToLowerInvariant())" -InformationAction Continue
Write-Information "prepare-worktree: gitignore=ready" -InformationAction Continue

if (Test-VcpkgCheckout -Path $localVcpkgRoot) {
    Write-Information "prepare-worktree: external-vcpkg=ready" -InformationAction Continue
} else {
    $sourceVcpkgRoot = Resolve-VcpkgCheckout -RepoRoot $root -CurrentRoot $root -RequestedVcpkgRoot $VcpkgRoot
    if ([string]::IsNullOrWhiteSpace($sourceVcpkgRoot)) {
        $cloneMessage = "Missing external/vcpkg. Clone the official Microsoft vcpkg repository into external/vcpkg, bootstrap it, then run tools/bootstrap-deps.ps1. Linked worktrees may instead run this script after the main worktree has a valid external/vcpkg checkout."
        Write-Error $cloneMessage
    }

    if (-not (Test-Path -LiteralPath $externalDir -PathType Container)) {
        if ($PSCmdlet.ShouldProcess($externalDir, "Create ignored external tool checkout directory")) {
            $null = New-Item -ItemType Directory -Path $externalDir
        }
    }

    if (Test-Path -LiteralPath $localVcpkgRoot) {
        Write-Error "external/vcpkg exists but does not contain scripts/buildsystems/vcpkg.cmake: $localVcpkgRoot"
    }

    $linkKind = if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        "Junction"
    } else {
        "SymbolicLink"
    }

    if ($PSCmdlet.ShouldProcess($localVcpkgRoot, "Link to vcpkg checkout at $sourceVcpkgRoot")) {
        $null = New-Item -ItemType $linkKind -Path $localVcpkgRoot -Target $sourceVcpkgRoot
    }

    if (-not (Test-VcpkgCheckout -Path $localVcpkgRoot)) {
        Write-Error "Failed to prepare external/vcpkg at $localVcpkgRoot"
    }

    Write-Information "prepare-worktree: external-vcpkg=linked" -InformationAction Continue
    Write-Information "prepare-worktree: external-vcpkg-source=$sourceVcpkgRoot" -InformationAction Continue
}

$linkKind = if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
    "Junction"
} else {
    "SymbolicLink"
}

if (Test-Path -LiteralPath $localVcpkgInstalledRoot -PathType Container) {
    Write-Information "prepare-worktree: vcpkg-installed=ready" -InformationAction Continue
} elseif (Test-Path -LiteralPath $localVcpkgInstalledRoot) {
    Write-Error "vcpkg_installed exists but is not a directory: $localVcpkgInstalledRoot"
} else {
    $sourceVcpkgInstalledRoot = Resolve-WorktreeDirectory -RepoRoot $root -CurrentRoot $root -RelativePath "vcpkg_installed"
    if ([string]::IsNullOrWhiteSpace($sourceVcpkgInstalledRoot)) {
        Write-Information "prepare-worktree: vcpkg-installed=missing" -InformationAction Continue
    } else {
        if ($PSCmdlet.ShouldProcess($localVcpkgInstalledRoot, "Link to vcpkg installed tree at $sourceVcpkgInstalledRoot")) {
            $null = New-Item -ItemType $linkKind -Path $localVcpkgInstalledRoot -Target $sourceVcpkgInstalledRoot
        }

        if (-not (Test-Path -LiteralPath $localVcpkgInstalledRoot -PathType Container)) {
            Write-Error "Failed to prepare vcpkg_installed at $localVcpkgInstalledRoot"
        }

        Write-Information "prepare-worktree: vcpkg-installed=linked" -InformationAction Continue
        Write-Information "prepare-worktree: vcpkg-installed-source=$sourceVcpkgInstalledRoot" -InformationAction Continue
    }
}

Write-Information "prepare-worktree: ok" -InformationAction Continue
