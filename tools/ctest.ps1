#requires -Version 7.0
#requires -PSEdition Core

param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Arguments
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$tools = Assert-CppBuildTools
Invoke-CheckedCommand $tools.CTest @Arguments
