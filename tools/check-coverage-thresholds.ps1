# Validates tools/coverage-thresholds.json structure for coverage gate policy (runs on any host).
#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$path = Join-Path $root "tools/coverage-thresholds.json"
if (-not (Test-Path -LiteralPath $path)) {
    Write-Error "Missing tools/coverage-thresholds.json"
}

$data = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
if ($data.schemaVersion -ne 1) {
    Write-Error "coverage-thresholds.json schemaVersion must be 1"
}

$minLine = $data.minLineCoveragePercent
if ($null -eq $minLine) {
    Write-Error "coverage-thresholds.json must set minLineCoveragePercent"
}
if ($minLine -lt 0 -or $minLine -gt 100) {
    Write-Error "coverage-thresholds.json minLineCoveragePercent must be between 0 and 100"
}

if ($null -ne $data.lcovRemovePatterns) {
    foreach ($entry in @($data.lcovRemovePatterns)) {
        if ($null -eq $entry -or [string]::IsNullOrWhiteSpace([string]$entry)) {
            Write-Error "coverage-thresholds.json lcovRemovePatterns entries must be non-empty strings"
        }
    }
}

Write-Host "check-coverage-thresholds: ok"
