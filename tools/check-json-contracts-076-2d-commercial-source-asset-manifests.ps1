#requires -Version 7.0
#requires -PSEdition Core

# Chapter 7.4 for check-json-contracts.ps1 2D commercial source asset manifest contracts.

function Get-Json2DCommercialSourceManifestPropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-Json2DCommercialSourceManifestString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-Json2DCommercialSourceManifestPath([string]$value, [string]$label) {
    Assert-Json2DCommercialSourceManifestString $value $label
    $normalized = $value.Replace("\", "/")
    if ($normalized.StartsWith("/") -or $normalized -match "^[A-Za-z]:") {
        Write-Error "$label must be repository-relative: $value"
    }
    if ($normalized -match "(^|/)\.\.($|/)") {
        Write-Error "$label must not contain parent traversal: $value"
    }
    if ($normalized.Contains(";")) {
        Write-Error "$label must not contain command separators: $value"
    }
}

function Get-Json2DCommercialSourceManifestGameRoot($game, [string]$label) {
    $entryPoint = ([string]$game.entryPoint).Replace("\", "/")
    $entryPointMatch = [regex]::Match($entryPoint, "^games/([a-z][a-z0-9_]*)/main\.cpp$")
    if (-not $entryPointMatch.Success) {
        Write-Error "$label entryPoint must be games/<game_name>/main.cpp for 2D commercial source manifest validation"
    }

    return "games/$($entryPointMatch.Groups[1].Value)"
}

function Assert-Json2DCommercialSourceManifestGamePath([string]$value, [string]$gameRoot, [string]$label) {
    Assert-Json2DCommercialSourceManifestPath $value $label
    $normalized = $value.Replace("\", "/")
    if ($normalized -ne $gameRoot -and -not $normalized.StartsWith("$gameRoot/")) {
        Write-Error "$label must stay under ${gameRoot}: $value"
    }
}

function Assert-Json2DCommercialSourceManifestIdMap($rows, [string]$label) {
    $ids = @{}
    foreach ($row in @($rows)) {
        Assert-Json2DCommercialSourceManifestString $row.id "$label.id"
        if ($ids.ContainsKey([string]$row.id)) {
            Write-Error "$label has duplicate id: $($row.id)"
        }
        $ids[[string]$row.id] = $true
    }

    return $ids
}

function Assert-Json2DCommercialSourceAssetManifests {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $manifestSet = Get-Json2DCommercialSourceManifestPropertyValue $Game.aiWorkflow "twoDCommercialSourceAssetManifests"
    if ($null -eq $manifestSet) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $manifestSet @(
        "schemaVersion",
        "capabilityId",
        "manifestSetId",
        "sourceRegistryPath",
        "packageIndexPath",
        "assetManifests",
        "packageProvenance",
        "cookingPolicy",
        "unsupportedClaims"
    ) "$Label aiWorkflow.twoDCommercialSourceAssetManifests"

    if ($manifestSet.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests schemaVersion must be 1"
    }
    if ($manifestSet.capabilityId -ne "2d-commercial-source-asset-manifests-v1") {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests capabilityId must be 2d-commercial-source-asset-manifests-v1"
    }

    $gameRoot = Get-Json2DCommercialSourceManifestGameRoot $Game $Label
    $expectedManifestSetId = "$($Game.name)-2d-commercial-source-assets"
    if ($manifestSet.manifestSetId -ne $expectedManifestSetId) {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests manifestSetId must be $expectedManifestSetId"
    }

    Assert-Json2DCommercialSourceManifestGamePath ([string]$manifestSet.sourceRegistryPath) $gameRoot "$Label aiWorkflow.twoDCommercialSourceAssetManifests.sourceRegistryPath"
    Assert-Json2DCommercialSourceManifestGamePath ([string]$manifestSet.packageIndexPath) $gameRoot "$Label aiWorkflow.twoDCommercialSourceAssetManifests.packageIndexPath"
    if (-not ([string]$manifestSet.sourceRegistryPath).Replace("\", "/").StartsWith("$gameRoot/source/")) {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests sourceRegistryPath must stay under the game source root"
    }
    if (-not ([string]$manifestSet.packageIndexPath).Replace("\", "/").StartsWith("$gameRoot/runtime/")) {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests packageIndexPath must stay under the game runtime root"
    }

    $recipeNames = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $recipeNames[[string]$recipe.name] = $true
    }
    $runtimePackageFiles = @($Game.runtimePackageFiles | ForEach-Object { [string]$_ })
    $designAssetRequestIds = @{}
    foreach ($assetRequest in @($Game.aiWorkflow.gameDesignSpec.assetRequests)) {
        $designAssetRequestIds[[string]$assetRequest.id] = $true
    }

    $requiredKinds = @(
        "sprite",
        "sprite-sheet",
        "atlas-page",
        "tile-chunk",
        "animation-clip",
        "audio-cue",
        "localization-text",
        "ui-glyph-input"
    )
    $assetManifestIds = Assert-Json2DCommercialSourceManifestIdMap $manifestSet.assetManifests "$Label aiWorkflow.twoDCommercialSourceAssetManifests.assetManifests"
    $kindRows = @{}
    foreach ($row in @($manifestSet.assetManifests)) {
        Assert-Properties $row @(
            "id",
            "kind",
            "designAssetRequestId",
            "assetKey",
            "sourcePath",
            "sourceFormat",
            "delivery",
            "license",
            "provenance",
            "contentHash",
            "validationRecipeIds"
        ) "$Label aiWorkflow.twoDCommercialSourceAssetManifests.assetManifests"

        if ($requiredKinds -notcontains [string]$row.kind) {
            Write-Error "$Label 2D commercial source asset manifest '$($row.id)' has unsupported kind: $($row.kind)"
        }
        $kindRows[[string]$row.kind] = $true
        if (-not $designAssetRequestIds.ContainsKey([string]$row.designAssetRequestId)) {
            Write-Error "$Label 2D commercial source asset manifest '$($row.id)' references unknown designAssetRequestId: $($row.designAssetRequestId)"
        }
        Assert-Json2DCommercialSourceManifestString $row.assetKey "$Label 2D commercial source asset manifest '$($row.id)'.assetKey"
        Assert-Json2DCommercialSourceManifestGamePath ([string]$row.sourcePath) $gameRoot "$Label 2D commercial source asset manifest '$($row.id)'.sourcePath"
        if (-not ([string]$row.sourcePath).Replace("\", "/").StartsWith("$gameRoot/source/")) {
            Write-Error "$Label 2D commercial source asset manifest '$($row.id)' sourcePath must stay under source/"
        }
        if (@("texture-source", "sprite-sheet-source", "atlas-page-source", "tile-chunk-source", "animation-clip-source", "audio-cue-source", "localization-text-source", "ui-glyph-input-source") -notcontains [string]$row.sourceFormat) {
            Write-Error "$Label 2D commercial source asset manifest '$($row.id)' sourceFormat is unsupported: $($row.sourceFormat)"
        }
        if (@("reviewed-source-authoring", "reviewed-cook-package") -notcontains [string]$row.delivery) {
            Write-Error "$Label 2D commercial source asset manifest '$($row.id)' delivery is unsupported: $($row.delivery)"
        }
        if ([string]$row.license -ne "LicenseRef-Proprietary") {
            Write-Error "$Label 2D commercial source asset manifest '$($row.id)' must record first-party proprietary license"
        }
        if ([string]$row.provenance -ne "first-party-created") {
            Write-Error "$Label 2D commercial source asset manifest '$($row.id)' must record first-party-created provenance"
        }
        if ([string]$row.contentHash -notmatch "^sha256:[0-9a-f]{64}$") {
            Write-Error "$Label 2D commercial source asset manifest '$($row.id)' contentHash must be sha256:<64 lowercase hex>"
        }
        foreach ($recipeId in @($row.validationRecipeIds)) {
            if (-not $recipeNames.ContainsKey([string]$recipeId)) {
                Write-Error "$Label 2D commercial source asset manifest '$($row.id)' references undeclared validation recipe: $recipeId"
            }
        }
    }
    foreach ($requiredKind in $requiredKinds) {
        if (-not $kindRows.ContainsKey($requiredKind)) {
            Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests assetManifests missing $requiredKind coverage"
        }
    }

    Assert-Properties $manifestSet.packageProvenance @(
        "mode",
        "assetManifestCount",
        "provenanceRowCount",
        "licenseStatus",
        "runtimePackageFiles",
        "validationRecipeIds",
        "evidence"
    ) "$Label aiWorkflow.twoDCommercialSourceAssetManifests.packageProvenance"
    if ([string]$manifestSet.packageProvenance.mode -ne "reviewed-first-party-source-to-cooked-package") {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests packageProvenance.mode must be reviewed-first-party-source-to-cooked-package"
    }
    if ([int]$manifestSet.packageProvenance.assetManifestCount -ne @($manifestSet.assetManifests).Count) {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests packageProvenance.assetManifestCount must match assetManifests count"
    }
    if ([int]$manifestSet.packageProvenance.provenanceRowCount -ne @($manifestSet.assetManifests).Count) {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests packageProvenance.provenanceRowCount must match assetManifests count"
    }
    if ([string]$manifestSet.packageProvenance.licenseStatus -ne "first-party-proprietary-reviewed") {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests packageProvenance.licenseStatus must be first-party-proprietary-reviewed"
    }
    foreach ($runtimeFile in @($manifestSet.packageProvenance.runtimePackageFiles)) {
        if ($runtimePackageFiles -notcontains [string]$runtimeFile) {
            Write-Error "$Label packageProvenance.runtimePackageFiles references undeclared runtime package file: $runtimeFile"
        }
    }
    foreach ($recipeId in @($manifestSet.packageProvenance.validationRecipeIds)) {
        if (-not $recipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "$Label packageProvenance references undeclared validation recipe: $recipeId"
        }
    }
    Assert-Json2DCommercialSourceManifestString $manifestSet.packageProvenance.evidence "$Label packageProvenance.evidence"

    Assert-Properties $manifestSet.cookingPolicy @(
        "runtimeSourceParsing",
        "sourceDecodingLane",
        "packageMutation",
        "externalDownloads",
        "arbitraryScripts",
        "nativeHandles",
        "dependencyExpansion",
        "evidence"
    ) "$Label aiWorkflow.twoDCommercialSourceAssetManifests.cookingPolicy"
    foreach ($propertyName in @("runtimeSourceParsing", "externalDownloads", "arbitraryScripts", "nativeHandles")) {
        if ([string](Get-Json2DCommercialSourceManifestPropertyValue $manifestSet.cookingPolicy $propertyName) -ne "unsupported") {
            Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests cookingPolicy.$propertyName must be unsupported"
        }
    }
    if ([string]$manifestSet.cookingPolicy.sourceDecodingLane -ne "tools-editor-only-reviewed") {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests cookingPolicy.sourceDecodingLane must be tools-editor-only-reviewed"
    }
    if ([string]$manifestSet.cookingPolicy.packageMutation -ne "reviewed-safe-package-handoff") {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests cookingPolicy.packageMutation must be reviewed-safe-package-handoff"
    }
    if ([string]$manifestSet.cookingPolicy.dependencyExpansion -ne "manifest-declared-source-closure") {
        Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests cookingPolicy.dependencyExpansion must be manifest-declared-source-closure"
    }
    Assert-Json2DCommercialSourceManifestString $manifestSet.cookingPolicy.evidence "$Label cookingPolicy.evidence"

    foreach ($unsupportedClaim in @(
            "runtime-source-parsing",
            "external-engine-project-import",
            "external-asset-download",
            "marketplace-asset-ingestion",
            "arbitrary-script-execution",
            "native-handle-access",
            "broad-production-atlas-packing",
            "broad-audio-codec-readiness",
            "broad-localization-platform-parity",
            "broad-commercial-2d-readiness"
        )) {
        if (@($manifestSet.unsupportedClaims) -notcontains $unsupportedClaim) {
            Write-Error "$Label aiWorkflow.twoDCommercialSourceAssetManifests unsupportedClaims missing $unsupportedClaim"
        }
    }

    $schemaText = Get-JsonContractSurfaceText "schemas/game-agent.schema.json"
    foreach ($needle in @(
            '"twoDCommercialSourceAssetManifests"',
            '"2d-commercial-source-asset-manifests-v1"',
            '"sprite-sheet"',
            '"ui-glyph-input"',
            '"reviewed-first-party-source-to-cooked-package"',
            '"manifest-declared-source-closure"'
        )) {
        Assert-ContainsText $schemaText $needle "schemas/game-agent.schema.json 2D commercial source asset manifest schema"
    }

    $planText = Get-JsonContractSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"
    foreach ($needle in @(
            "twoDCommercialSourceAssetManifests",
            "2d-commercial-source-asset-manifests-v1",
            "first-party source asset manifests",
            "sprite sheets, atlas pages, tile chunks, animation clips, audio cues, localization text, UI glyph inputs",
            "runtime source parsing remains unsupported"
        )) {
        Assert-ContainsText $planText $needle "2D Commercial Production Excellence v1 Phase 2 source manifest evidence"
    }

    $currentCapabilitiesText = Get-JsonContractSurfaceText "docs/current-capabilities.md"
    foreach ($needle in @(
            "2D Commercial Source Asset Manifests v1",
            "twoDCommercialSourceAssetManifests",
            "runtime source parsing remains unsupported",
            "broad commercial 2D readiness remains unclaimed"
        )) {
        Assert-ContainsText $currentCapabilitiesText $needle "docs/current-capabilities.md 2D commercial source asset manifest evidence"
    }
}

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    $requiresCommercial2DSourceManifests = $game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package"
    Assert-Json2DCommercialSourceAssetManifests $game $relative $requiresCommercial2DSourceManifests
}
