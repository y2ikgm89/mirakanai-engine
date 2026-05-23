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

    Get-ChildItem -Path $Root -Recurse -File -Include *.hpp, *.cpp |
        Where-Object { -not (Test-IsExcludedPath $_.FullName) }
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
