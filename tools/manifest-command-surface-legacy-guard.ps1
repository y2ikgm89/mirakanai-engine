#requires -Version 7.0
#requires -PSEdition Core
# Dot-source from check-json-contracts.ps1 / check-ai-integration.ps1 only.
# Rejects legacy top-level fields on aiOperableProductionLoop command surfaces.

$ErrorActionPreference = "Stop"

function Get-LegacyManifestCommandSurfaceFieldNames {
    return @("inputContract", "outputContract", "dryRun", "apply", "validation")
}

function Assert-ManifestCommandSurfaceHasNoLegacyTopLevelFields {
    param(
        [Parameter(Mandatory = $true)]$CommandSurface,
        [Parameter(Mandatory = $true)][string]$MessagePrefix
    )

    foreach ($legacyField in (Get-LegacyManifestCommandSurfaceFieldNames)) {
        if ($CommandSurface.PSObject.Properties.Name.Contains($legacyField)) {
            Write-Error "${MessagePrefix} '$($CommandSurface.id)' must use descriptor fields instead of legacy field: $legacyField"
        }
    }
}
