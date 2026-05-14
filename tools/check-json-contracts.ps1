#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "manifest-command-surface-legacy-guard.ps1")
. (Join-Path $PSScriptRoot "static-contract-ledger.ps1")

$root = Get-RepoRoot

$composeVerify = Join-Path $PSScriptRoot "compose-agent-manifest.ps1"
& $composeVerify -Verify

. (Join-Path $PSScriptRoot "check-json-contracts-core.ps1")

Invoke-JsonContractSections
