#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

# `tools/check-ai-integration.ps1` runs once below (equivalent to `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`).
# `check-validation-recipe-runner.ps1` exercises DryRun/Execute rejection paths without duplicating that pass.

function Invoke-ValidateToolScript {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$ScriptFileName
    )

    Write-Host "validate: running $ScriptFileName"
    & (Join-Path $PSScriptRoot $ScriptFileName)
}

foreach ($scriptFileName in @(
        "check-license.ps1",
        "check-agents.ps1",
        "check-json-contracts.ps1",
        "check-validation-recipe-runner.ps1",
        "check-installed-sdk-validation.ps1",
        "check-release-package-artifacts.ps1",
        "check-ai-integration.ps1",
        "check-production-readiness-audit.ps1",
        "check-ci-matrix.ps1",
        "check-dependency-policy.ps1",
        "check-vcpkg-environment.ps1",
        "check-toolchain.ps1",
        "check-cpp-standard-policy.ps1",
        "check-coverage-thresholds.ps1",
        "check-shader-toolchain.ps1",
        "check-mobile-packaging.ps1",
        "check-apple-host-evidence.ps1",
        "check-public-api-boundaries.ps1"
    )) {
    Invoke-ValidateToolScript -ScriptFileName $scriptFileName
}

Write-Host "validate: running check-tidy.ps1"
& (Join-Path $PSScriptRoot "check-tidy.ps1") -MaxFiles 1

foreach ($scriptFileName in @(
        "build.ps1",
        "check-generated-msvc-cxx23-mode.ps1",
        "test.ps1"
    )) {
    Invoke-ValidateToolScript -ScriptFileName $scriptFileName
}

Write-Host "validate: ok"
exit 0
