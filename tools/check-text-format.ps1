#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [ValidateNotNullOrEmpty()]
    [string]$Root = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "text-format-core.ps1")

if ([string]::IsNullOrWhiteSpace($Root)) {
    $Root = Get-RepoRoot
}

$issues = @(Test-MKTextFormat -Root $Root)
if ($issues.Count -gt 0) {
    $sample = ($issues | Select-Object -First 30 | ForEach-Object { "$($_.RepoPath): $($_.Reason)" }) -join "`n"
    Write-Error "text-format-check failed. Run pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1 to normalize tracked text files.`n$sample"
}

Write-Host "text-format-check: ok"
