#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$excludedDirectories = @(".cache", ".cxx", ".git", ".gradle", ".vs", "build", "external", "out", "vcpkg_installed")
$missing = @()

function Test-IsExcludedPath($path) {
    $relative = [System.IO.Path]::GetRelativePath($root, $path)
    $parts = $relative -split '[\\/]'
    foreach ($directory in $excludedDirectories) {
        if ($parts -contains $directory) {
            return $true
        }
    }
    return $false
}

function Get-LicenseCheckedSourceFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    $extensions = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $extensions.Add(".hpp") | Out-Null
    $extensions.Add(".cpp") | Out-Null

    $pendingDirectories = [System.Collections.Generic.Queue[System.IO.DirectoryInfo]]::new()
    $pendingDirectories.Enqueue((Get-Item -LiteralPath $Root))
    while ($pendingDirectories.Count -gt 0) {
        $directory = $pendingDirectories.Dequeue()
        foreach ($file in Get-ChildItem -LiteralPath $directory.FullName -File) {
            if ($extensions.Contains($file.Extension)) {
                $file
            }
        }

        foreach ($childDirectory in Get-ChildItem -LiteralPath $directory.FullName -Directory -Force) {
            if (-not (Test-IsExcludedPath $childDirectory.FullName)) {
                $pendingDirectories.Enqueue($childDirectory)
            }
        }
    }
}

Get-LicenseCheckedSourceFile -Root $root |
    ForEach-Object {
        $content = (Get-Content -LiteralPath $_.FullName -TotalCount 5) -join "`n"
        if ($content -notmatch "SPDX-License-Identifier:") {
            $missing += $_.FullName
        }
    }

if ($missing.Count -gt 0) {
    Write-Error ("Missing SPDX-License-Identifier in:`n" + ($missing -join "`n"))
}

Write-Host "license-check: ok"
