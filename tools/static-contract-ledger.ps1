#requires -Version 7.0
#requires -PSEdition Core

function Get-StaticContractLedger {
    return @(
        [pscustomobject]@{
            Id = "check-ai-integration"
            EntryScript = "tools/check-ai-integration.ps1"
            CoreScript = "tools/check-ai-integration-core.ps1"
            SectionFiles = @(
                "check-ai-integration-010-agent-baseline.ps1",
                "check-ai-integration-020-engine-manifest.ps1",
                "check-ai-integration-030-runtime-rendering.ps1",
                "check-ai-integration-040-agent-surfaces.ps1",
                "check-ai-integration-050-game-generation.ps1",
                "check-ai-integration-060-editor-workflows.ps1",
                "check-ai-integration-070-production-ledger.ps1",
                "check-ai-integration-080-scaffold-smokes.ps1"
            )
            MaximumEntryLines = 80
            MaximumCoreLines = 1600
            MaximumSectionLines = 2500
        },
        [pscustomobject]@{
            Id = "check-json-contracts"
            EntryScript = "tools/check-json-contracts.ps1"
            CoreScript = "tools/check-json-contracts-core.ps1"
            SectionFiles = @(
                "check-json-contracts-010-engine-manifest.ps1",
                "check-json-contracts-020-game-contracts.ps1",
                "check-json-contracts-030-tooling-contracts.ps1",
                "check-json-contracts-040-agent-surfaces.ps1",
                "check-json-contracts-050-generated-games.ps1"
            )
            MaximumEntryLines = 80
            MaximumCoreLines = 1600
            MaximumSectionLines = 1500
        }
    )
}

function Get-StaticContractLedgerById {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Id
    )

    $matchingLedgers = @(Get-StaticContractLedger | Where-Object { $_.Id -eq $Id })
    if ($matchingLedgers.Count -ne 1) {
        Write-Error "Expected exactly one static contract ledger row for '$Id', found $($matchingLedgers.Count)"
    }

    return $matchingLedgers[0]
}

function Get-StaticContractLedgerRepoPaths {
    param(
        [Parameter(Mandatory = $true)]
        $Ledger
    )

    $paths = @($Ledger.CoreScript)
    foreach ($sectionFile in $Ledger.SectionFiles) {
        $paths += "tools/$sectionFile"
    }

    return @($paths)
}
