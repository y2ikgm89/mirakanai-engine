#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$CorpusRoot = "",
    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$pwshCommand = (Get-Command pwsh -ErrorAction Stop).Source

function Invoke-RepoPwshScript {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RelativeScript,

        [string[]]$ScriptArguments = @()
    )

    $scriptPath = Join-Path $PSScriptRoot $RelativeScript
    $arguments = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $scriptPath) + $ScriptArguments
    Invoke-CheckedCommand -FilePath $pwshCommand -Arguments $arguments
}

Invoke-RepoPwshScript -RelativeScript "check-json-contracts.ps1"

$validatorArguments = [System.Collections.Generic.List[string]]::new()
if (-not [string]::IsNullOrWhiteSpace($CorpusRoot)) {
    $validatorArguments.Add("-CorpusRoot") | Out-Null
    $validatorArguments.Add($CorpusRoot) | Out-Null
}
if ($RequireReady) {
    $validatorArguments.Add("-RequireReady") | Out-Null
}
Invoke-RepoPwshScript -RelativeScript "validate-asset-import-regression-corpus.ps1" -ScriptArguments ([string[]]$validatorArguments)

Invoke-RepoPwshScript -RelativeScript "cmake.ps1" -ScriptArguments @("--preset", "dev")
Invoke-RepoPwshScript -RelativeScript "cmake.ps1" -ScriptArguments @(
    "--build",
    "--preset",
    "dev",
    "--target",
    "MK_asset_import_regression_tests"
)
Invoke-RepoPwshScript -RelativeScript "ctest.ps1" -ScriptArguments @(
    "--preset",
    "dev",
    "--output-on-failure",
    "-R",
    "MK_asset_import_regression_tests"
)

Write-Information "asset-import-regression-corpus-check: ok" -InformationAction Continue
