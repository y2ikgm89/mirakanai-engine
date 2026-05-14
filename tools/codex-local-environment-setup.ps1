#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$SourceRoot = (Get-Location).Path,
    [string]$WorktreeRoot = $env:CODEX_WORKTREE_PATH,
    [switch]$SkipToolchainCheck
)

$ErrorActionPreference = "Stop"

function Resolve-RootPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        Write-Error "$Description must not be empty."
    }

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $fullPath -PathType Container)) {
        Write-Error "$Description does not exist: $fullPath"
    }

    return (Resolve-Path -LiteralPath $fullPath).ProviderPath
}

function Join-RepoPath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$RelativePath
    )

    return [System.IO.Path]::GetFullPath((Join-Path $Root $RelativePath))
}

function New-RepositoryJunction {
    [CmdletBinding(SupportsShouldProcess = $true)]
    param(
        [Parameter(Mandatory = $true)][string]$SourceRootPath,
        [Parameter(Mandatory = $true)][string]$WorktreeRootPath,
        [Parameter(Mandatory = $true)][string]$RelativePath
    )

    $target = Join-RepoPath -Root $SourceRootPath -RelativePath $RelativePath
    $link = Join-RepoPath -Root $WorktreeRootPath -RelativePath $RelativePath

    if (-not (Test-Path -LiteralPath $target -PathType Container)) {
        Write-Information "codex-local-env: skip $RelativePath; source path is missing: $target" -InformationAction Continue
        return
    }

    if (Test-Path -LiteralPath $link) {
        Write-Information "codex-local-env: $RelativePath already exists in worktree" -InformationAction Continue
        return
    }

    $parent = Split-Path -Parent $link
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        if ($PSCmdlet.ShouldProcess($parent, "Create parent directory for $RelativePath junction")) {
            $null = New-Item -ItemType Directory -Path $parent -Force
        }
    }

    if ($PSCmdlet.ShouldProcess($link, "Create junction to $target")) {
        $null = New-Item -ItemType Junction -Path $link -Target $target
        Write-Information "codex-local-env: linked $RelativePath -> $target" -InformationAction Continue
    }
}

if ([string]::IsNullOrWhiteSpace($WorktreeRoot)) {
    $WorktreeRoot = $SourceRoot
}

$sourceRootPath = Resolve-RootPath -Path $SourceRoot -Description "Source root"
$worktreeRootPath = Resolve-RootPath -Path $WorktreeRoot -Description "Worktree root"

if (-not (Test-Path -LiteralPath (Join-RepoPath -Root $worktreeRootPath -RelativePath "tools/check-toolchain.ps1") -PathType Leaf)) {
    Write-Error "Worktree root does not look like the GameEngine repository: $worktreeRootPath"
}

if (-not $sourceRootPath.Equals($worktreeRootPath, [System.StringComparison]::OrdinalIgnoreCase)) {
    New-RepositoryJunction -SourceRootPath $sourceRootPath -WorktreeRootPath $worktreeRootPath -RelativePath "external/vcpkg"
    New-RepositoryJunction -SourceRootPath $sourceRootPath -WorktreeRootPath $worktreeRootPath -RelativePath "vcpkg_installed"
} else {
    Write-Information "codex-local-env: source root and worktree root are the same; no junctions needed" -InformationAction Continue
}

if (-not $SkipToolchainCheck.IsPresent) {
    Push-Location $worktreeRootPath
    try {
        & (Join-RepoPath -Root $worktreeRootPath -RelativePath "tools/check-toolchain.ps1")
    } finally {
        Pop-Location
    }
}

Write-Information "codex-local-env: setup complete" -InformationAction Continue
