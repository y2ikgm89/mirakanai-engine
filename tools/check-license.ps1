#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$missing = @()

function Get-LicenseCheckedSourceFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    $relativePaths = @(& git -C $Root ls-files --cached --others --exclude-standard -- '*.cpp' '*.hpp')
    if ($LASTEXITCODE -ne 0) {
        Write-Error "license-check: git ls-files failed"
    }

    foreach ($relativePath in $relativePaths) {
        if ([string]::IsNullOrWhiteSpace($relativePath)) {
            continue
        }

        $fullPath = Join-Path $Root ($relativePath -replace '/', [System.IO.Path]::DirectorySeparatorChar)
        if (Test-Path -LiteralPath $fullPath -PathType Leaf) {
            Get-Item -LiteralPath $fullPath
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

Write-Information "license-check: ok" -InformationAction Continue
