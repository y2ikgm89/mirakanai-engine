#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "manifest-command-surface-legacy-guard.ps1")
. (Join-Path $PSScriptRoot "static-contract-ledger.ps1")

$root = Get-RepoRoot
$script:agentSurfaceTextCache = @{}

. (Join-Path $PSScriptRoot "check-ai-integration-core.ps1")

# Serialize concurrent runs via a machine-local named mutex (see `Initialize-RepoExclusiveToolMutex` in `tools/common.ps1`).
$checkAiIntegrationRepoExclusiveMutex = Initialize-RepoExclusiveToolMutex -RepositoryRoot $root -ToolId "check-ai-integration"
try {
    Write-Information "check-ai-integration: exclusive repository mutex acquired; running checks..." -InformationAction Continue
    Invoke-CheckAiIntegrationSections
}
finally {
    Clear-RepoExclusiveToolMutex -Mutex $checkAiIntegrationRepoExclusiveMutex
}

exit 0
