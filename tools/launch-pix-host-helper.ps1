#requires -Version 7.0
#requires -PSEdition Core

<#
.SYNOPSIS
  Optional Windows host helper: launch Microsoft PIX from a non-repository scratch directory.

.DESCRIPTION
  Creates a session directory under %LocalAppData%\MirakanaiEngine\pix-host-helper\ and starts the
  Microsoft PIX UI (`WinPix.exe` under `Program Files\\Microsoft PIX\\<version>\\`, or legacy
  `PIX.exe` when present) with that working directory when PIX is installed.   Does not write under
  the repository root. Optional `MK_editor` integration (Windows, D3D12 viewport, acknowledged PIX capture request)
  runs the same script with `-SkipLaunch` from a reviewed Resources panel control; see
  `docs/superpowers/plans/2026-05-11-editor-resource-capture-pix-host-handoff-evidence-v1.md`.

  Set MIRAKANA_PIX_EXE to override the PIX executable path (full path to `WinPix.exe` or `PIX.exe`).

.NOTES
  PIX on Windows is a host diagnostic; see AGENTS.md and manifest hostGates. Install PIX from
  Microsoft and Windows Graphics Tools / D3D12 debug layer as needed for GPU capture work.
#>

param(
    [switch]$PassThru,
    # Resolve PIX, create scratch, print paths, but do not start the PIX process (operator smoke / automation).
    [switch]$SkipLaunch
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Find-MicrosoftPixUiExecutableUnderProgramFiles {
    param([Parameter(Mandatory = $true)][string]$ProgramFilesRoot)

    if ([string]::IsNullOrWhiteSpace($ProgramFilesRoot)) {
        return $null
    }

    $pixRoot = Join-Path $ProgramFilesRoot "Microsoft PIX"
    if (-not (Test-Path -LiteralPath $pixRoot)) {
        return $null
    }

    $legacy = Join-Path $pixRoot "PIX.exe"
    if (Test-Path -LiteralPath $legacy -PathType Leaf) {
        return (Resolve-Path -LiteralPath $legacy).Path
    }

    $versionDirs = Get-ChildItem -LiteralPath $pixRoot -Directory -ErrorAction SilentlyContinue |
        Sort-Object { $_.Name } -Descending
    foreach ($dir in $versionDirs) {
        $winPix = Join-Path $dir.FullName "WinPix.exe"
        if (Test-Path -LiteralPath $winPix -PathType Leaf) {
            return (Resolve-Path -LiteralPath $winPix).Path
        }
    }

    return $null
}

function Resolve-PixUiExecutable {
    $override = Get-EnvironmentVariableAnyScope "MIRAKANA_PIX_EXE"
    if (-not [string]::IsNullOrWhiteSpace($override) -and (Test-Path -LiteralPath $override -PathType Leaf)) {
        return (Resolve-Path -LiteralPath $override).Path
    }

    $pf64 = [Environment]::GetEnvironmentVariable("ProgramFiles", "Machine")
    if ([string]::IsNullOrWhiteSpace($pf64)) {
        $pf64 = [Environment]::GetEnvironmentVariable("ProgramFiles", "Process")
    }
    if (-not [string]::IsNullOrWhiteSpace($pf64)) {
        $from64 = Find-MicrosoftPixUiExecutableUnderProgramFiles $pf64
        if (-not [string]::IsNullOrWhiteSpace($from64)) {
            return $from64
        }
    }

    $pf32 = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)", "Machine")
    if ([string]::IsNullOrWhiteSpace($pf32)) {
        $pf32 = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)", "Process")
    }
    if (-not [string]::IsNullOrWhiteSpace($pf32)) {
        $from32 = Find-MicrosoftPixUiExecutableUnderProgramFiles $pf32
        if (-not [string]::IsNullOrWhiteSpace($from32)) {
            return $from32
        }
    }

    $fromWinPixPath = Find-CommandOnCombinedPath "WinPix"
    if (-not [string]::IsNullOrWhiteSpace($fromWinPixPath)) {
        return $fromWinPixPath
    }

    $fromPath = Find-CommandOnCombinedPath "PIX"
    if (-not [string]::IsNullOrWhiteSpace($fromPath)) {
        return $fromPath
    }

    return $null
}

$pix = Resolve-PixUiExecutable
if ([string]::IsNullOrWhiteSpace($pix)) {
    Write-Error "Microsoft PIX (WinPix.exe or PIX.exe) was not found. Install PIX on Windows (`winget install --id Microsoft.PIX -e`), add it to PATH, or set MIRAKANA_PIX_EXE to the full path of WinPix.exe or PIX.exe."
}

$localApp = [Environment]::GetFolderPath([Environment+SpecialFolder]::LocalApplicationData)
$scratchRoot = Join-Path $localApp "MirakanaiEngine\pix-host-helper"
$sessionDir = Join-Path $scratchRoot (Get-Date -Format "yyyyMMdd-HHmmss-fff")
New-Item -ItemType Directory -Path $sessionDir -Force | Out-Null

$readme = @"
MirakanaiEngine PIX host-helper scratch (not part of the Git repository).
Safe to delete. Created for manual GPU capture workflows alongside MK_editor Resources capture rows.
"@
Set-Content -LiteralPath (Join-Path $sessionDir "README.txt") -Value $readme -Encoding utf8NoBOM

Write-Host "PIX executable: $pix"
Write-Host "Session scratch (not in repo): $sessionDir"

if ($PassThru) {
    return [pscustomobject]@{
        PixExecutable = $pix
        SessionDirectory = $sessionDir
    }
}

if ($SkipLaunch) {
    Write-Host "SkipLaunch: PIX not started."
    exit 0
}

Start-Process -FilePath $pix -WorkingDirectory $sessionDir
