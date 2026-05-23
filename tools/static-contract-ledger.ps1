#requires -Version 7.0
#requires -PSEdition Core

$script:StaticContractLedgerToolsRoot = $PSScriptRoot

function Get-StaticContractSectionFile {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$Prefix
    )

    $escapedPrefix = [regex]::Escape($Prefix)
    $sectionNamePattern = "^$escapedPrefix-\d{3}-.+\.ps1$"
    $sectionFiles = @(
        Get-ChildItem -LiteralPath $script:StaticContractLedgerToolsRoot -Filter "$Prefix-*.ps1" -File |
            Where-Object { [regex]::IsMatch($_.Name, $sectionNamePattern) } |
            Sort-Object Name |
            ForEach-Object { $_.Name }
    )
    if ($sectionFiles.Count -eq 0) {
        Write-Error "No static contract section files found for prefix '$Prefix'"
    }

    return @($sectionFiles)
}

function Get-StaticContractLedger {
    return @(
        [pscustomobject]@{
            Id = "check-ai-integration"
            EntryScript = "tools/check-ai-integration.ps1"
            CoreScript = "tools/check-ai-integration-core.ps1"
            SectionFiles = Get-StaticContractSectionFile -Prefix "check-ai-integration"
            MaximumEntryLines = 80
            MaximumCoreLines = 1600
            MaximumSectionLines = 2600
        },
        [pscustomobject]@{
            Id = "check-json-contracts"
            EntryScript = "tools/check-json-contracts.ps1"
            CoreScript = "tools/check-json-contracts-core.ps1"
            SectionFiles = Get-StaticContractSectionFile -Prefix "check-json-contracts"
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

function Get-StaticContractLedgerRepoPath {
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
