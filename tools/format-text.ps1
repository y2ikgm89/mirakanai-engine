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

$changed = Repair-MKTextFormat -Root $Root
Write-Host "text-format: normalized $changed tracked text file(s)"
