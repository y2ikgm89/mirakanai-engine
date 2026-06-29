#requires -Version 7.0
#requires -PSEdition Core

# Chapter 7.6 for check-json-contracts.ps1 selected 2D commercial production atlas packing contracts.

function Get-Json2DCommercialProductionAtlasPropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-Json2DCommercialProductionAtlasString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-Json2DCommercialProductionAtlasPath([string]$value, [string]$label) {
    Assert-Json2DCommercialProductionAtlasString $value $label
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

function Get-Json2DCommercialProductionAtlasGameRoot($game, [string]$label) {
    $entryPoint = ([string]$game.entryPoint).Replace("\", "/")
    $entryPointMatch = [regex]::Match($entryPoint, "^games/([a-z][a-z0-9_]*)/main\.cpp$")
    if (-not $entryPointMatch.Success) {
        Write-Error "$label entryPoint must be games/<game_name>/main.cpp for 2D commercial production atlas validation"
    }

    return "games/$($entryPointMatch.Groups[1].Value)"
}

function Assert-Json2DCommercialProductionAtlasGamePath([string]$value, [string]$gameRoot, [string]$label) {
    Assert-Json2DCommercialProductionAtlasPath $value $label
    $normalized = $value.Replace("\", "/")
    if ($normalized -ne $gameRoot -and -not $normalized.StartsWith("$gameRoot/")) {
        Write-Error "$label must stay under ${gameRoot}: $value"
    }
}

function Assert-Json2DCommercialProductionAtlasRuntimeOutputPath(
    [string]$value,
    [string]$gameRoot,
    [string[]]$runtimePackageFiles,
    [string]$label
) {
    Assert-Json2DCommercialProductionAtlasGamePath $value $gameRoot $label
    $normalized = $value.Replace("\", "/")
    if (-not $normalized.StartsWith("$gameRoot/runtime/")) {
        Write-Error "$label must stay under the game runtime root: $value"
    }

    $gameRelativePath = $normalized.Substring($gameRoot.Length + 1)
    if ($runtimePackageFiles -notcontains $gameRelativePath) {
        Write-Error "$label must reference a declared runtimePackageFiles row: $gameRelativePath"
    }
}

function Assert-Json2DCommercialProductionAtlasIdMap($rows, [string]$label) {
    $ids = @{}
    foreach ($row in @($rows)) {
        Assert-Json2DCommercialProductionAtlasString $row.id "$label.id"
        if ($ids.ContainsKey([string]$row.id)) {
            Write-Error "$label has duplicate id: $($row.id)"
        }
        $ids[[string]$row.id] = $row
    }

    return $ids
}

function Assert-Json2DCommercialProductionAtlasRecipeIds($recipeIds, $recipeNames, [string]$label) {
    foreach ($recipeId in @($recipeIds)) {
        if (-not $recipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "$label references undeclared validation recipe: $recipeId"
        }
    }
}

function Test-Json2DCommercialProductionAtlasPowerOfTwo([int]$value) {
    return $value -gt 0 -and ($value -band ($value - 1)) -eq 0
}

function Assert-Json2DCommercialProductionAtlasPacking {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $atlasSet = Get-Json2DCommercialProductionAtlasPropertyValue $Game.aiWorkflow "twoDCommercialProductionAtlasPacking"
    if ($null -eq $atlasSet) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.twoDCommercialProductionAtlasPacking missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $atlasSet @(
        "schemaVersion",
        "capabilityId",
        "packingSetId",
        "sourceAssetManifestSetId",
        "deterministicCookSetId",
        "packageIndexPath",
        "atlasRows",
        "qualityGateRows",
        "packageInclusion",
        "unsupportedClaims"
    ) "$Label aiWorkflow.twoDCommercialProductionAtlasPacking"

    if ($atlasSet.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.twoDCommercialProductionAtlasPacking schemaVersion must be 1"
    }
    if ($atlasSet.capabilityId -ne "2d-commercial-production-atlas-packing-v1") {
        Write-Error "$Label aiWorkflow.twoDCommercialProductionAtlasPacking capabilityId must be 2d-commercial-production-atlas-packing-v1"
    }

    $gameRoot = Get-Json2DCommercialProductionAtlasGameRoot $Game $Label
    $expectedPackingSetId = "$($Game.name)-2d-commercial-production-atlas-packing"
    if ($atlasSet.packingSetId -ne $expectedPackingSetId) {
        Write-Error "$Label aiWorkflow.twoDCommercialProductionAtlasPacking packingSetId must be $expectedPackingSetId"
    }

    $sourceManifestSet = Get-Json2DCommercialProductionAtlasPropertyValue $Game.aiWorkflow "twoDCommercialSourceAssetManifests"
    $cookSet = Get-Json2DCommercialProductionAtlasPropertyValue $Game.aiWorkflow "twoDCommercialDeterministicCookOutputs"
    if ($null -eq $sourceManifestSet -or $null -eq $cookSet) {
        Write-Error "$Label aiWorkflow.twoDCommercialProductionAtlasPacking requires source manifest and deterministic cook descriptors"
    } else {
        if ($atlasSet.sourceAssetManifestSetId -ne $sourceManifestSet.manifestSetId) {
            Write-Error "$Label production atlas sourceAssetManifestSetId must match twoDCommercialSourceAssetManifests.manifestSetId"
        }
        if ($atlasSet.deterministicCookSetId -ne $cookSet.cookSetId) {
            Write-Error "$Label production atlas deterministicCookSetId must match twoDCommercialDeterministicCookOutputs.cookSetId"
        }
        if ($atlasSet.packageIndexPath -ne $cookSet.packageIndexPath) {
            Write-Error "$Label production atlas packageIndexPath must match deterministic cook packageIndexPath"
        }
    }

    Assert-Json2DCommercialProductionAtlasGamePath ([string]$atlasSet.packageIndexPath) $gameRoot "$Label aiWorkflow.twoDCommercialProductionAtlasPacking.packageIndexPath"
    $runtimePackageFiles = @($Game.runtimePackageFiles | ForEach-Object { [string]$_ })
    $recipeNames = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $recipeNames[[string]$recipe.name] = $true
    }

    $sourceManifestRows = Assert-Json2DCommercialProductionAtlasIdMap $sourceManifestSet.assetManifests "$Label aiWorkflow.twoDCommercialSourceAssetManifests.assetManifests"
    $cookOutputRows = Assert-Json2DCommercialProductionAtlasIdMap $cookSet.cookOutputs "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.cookOutputs"
    $atlasRows = Assert-Json2DCommercialProductionAtlasIdMap $atlasSet.atlasRows "$Label aiWorkflow.twoDCommercialProductionAtlasPacking.atlasRows"
    $multiPageEvidence = $false
    foreach ($row in @($atlasSet.atlasRows)) {
        Assert-Properties $row @(
            "id",
            "cookOutputId",
            "sourceManifestIds",
            "runtimePackageFile",
            "textureFormatTarget",
            "maxSide",
            "paddingPixels",
            "bleedPixels",
            "rotationPolicy",
            "powerOfTwoPolicy",
            "mipPolicy",
            "deterministicPlacement",
            "pages",
            "placements",
            "validationRecipeIds"
        ) "$Label production atlas row '$($row.id)'"

        if (-not $cookOutputRows.ContainsKey([string]$row.cookOutputId)) {
            Write-Error "$Label production atlas row '$($row.id)' references unknown cookOutputId: $($row.cookOutputId)"
        }
        $cookOutput = $cookOutputRows[[string]$row.cookOutputId]
        if (@("texture", "ui-atlas") -notcontains [string]$cookOutput.outputKind) {
            Write-Error "$Label production atlas row '$($row.id)' cookOutputId must point at texture or ui-atlas output"
        }
        $cookRuntimeFile = ([string]$cookOutput.outputPath).Replace("\", "/").Substring($gameRoot.Length + 1)
        if ([string]$row.runtimePackageFile -ne $cookRuntimeFile) {
            Write-Error "$Label production atlas row '$($row.id)' runtimePackageFile must match deterministic cook outputPath"
        }
        Assert-Json2DCommercialProductionAtlasRuntimeOutputPath ([string]$cookOutput.outputPath) $gameRoot $runtimePackageFiles "$Label production atlas row '$($row.id)'.cookOutput.outputPath"

        foreach ($sourceManifestId in @($row.sourceManifestIds)) {
            if (-not $sourceManifestRows.ContainsKey([string]$sourceManifestId)) {
                Write-Error "$Label production atlas row '$($row.id)' references unknown source manifest: $sourceManifestId"
            }
            if (@($cookOutput.sourceManifestIds) -notcontains [string]$sourceManifestId) {
                Write-Error "$Label production atlas row '$($row.id)' source manifest '$sourceManifestId' must be covered by its cook output"
            }
        }
        if ([string]$row.textureFormatTarget -ne "rgba8_unorm") {
            Write-Error "$Label production atlas row '$($row.id)' textureFormatTarget must be rgba8_unorm"
        }
        if ([int]$row.maxSide -le 0 -or [int]$row.maxSide -gt 16384) {
            Write-Error "$Label production atlas row '$($row.id)' maxSide must be positive and <= 16384"
        }
        if ([int]$row.paddingPixels -lt 1) {
            Write-Error "$Label production atlas row '$($row.id)' paddingPixels must be >= 1"
        }
        if ([int]$row.bleedPixels -lt 1 -or [int]$row.bleedPixels -gt [int]$row.paddingPixels) {
            Write-Error "$Label production atlas row '$($row.id)' bleedPixels must be in [1, paddingPixels]"
        }
        if ([string]$row.rotationPolicy -ne "disabled") {
            Write-Error "$Label production atlas row '$($row.id)' rotationPolicy must be disabled"
        }
        if (@("keep-tight-bounds", "require-power-of-two-pages") -notcontains [string]$row.powerOfTwoPolicy) {
            Write-Error "$Label production atlas row '$($row.id)' powerOfTwoPolicy is unsupported: $($row.powerOfTwoPolicy)"
        }
        if ([string]$row.mipPolicy -ne "base-level-only") {
            Write-Error "$Label production atlas row '$($row.id)' mipPolicy must be base-level-only"
        }
        if ($row.deterministicPlacement -ne $true) {
            Write-Error "$Label production atlas row '$($row.id)' deterministicPlacement must be true"
        }
        Assert-Json2DCommercialProductionAtlasRecipeIds $row.validationRecipeIds $recipeNames "$Label production atlas row '$($row.id)'"

        $pageRows = Assert-Json2DCommercialProductionAtlasIdMap $row.pages "$Label production atlas row '$($row.id)'.pages"
        if (@($row.pages).Count -ge 2) {
            $multiPageEvidence = $true
        }
        foreach ($page in @($row.pages)) {
            Assert-Properties $page @("id", "pageIndex", "width", "height", "powerOfTwo", "paddingPixels", "bleedPixels") "$Label production atlas page '$($page.id)'"
            if ([int]$page.width -le 0 -or [int]$page.height -le 0 -or [int]$page.width -gt [int]$row.maxSide -or [int]$page.height -gt [int]$row.maxSide) {
                Write-Error "$Label production atlas page '$($page.id)' dimensions must be positive and <= row.maxSide"
            }
            if ([string]$row.powerOfTwoPolicy -eq "require-power-of-two-pages" -and (-not (Test-Json2DCommercialProductionAtlasPowerOfTwo ([int]$page.width)) -or -not (Test-Json2DCommercialProductionAtlasPowerOfTwo ([int]$page.height)))) {
                Write-Error "$Label production atlas page '$($page.id)' must use power-of-two dimensions"
            }
            if ([int]$page.paddingPixels -ne [int]$row.paddingPixels -or [int]$page.bleedPixels -ne [int]$row.bleedPixels) {
                Write-Error "$Label production atlas page '$($page.id)' padding/bleed must match its atlas row"
            }
        }

        foreach ($placement in @($row.placements)) {
            Assert-Properties $placement @("id", "sourceManifestId", "pageId", "x", "y", "width", "height", "paddedX", "paddedY", "paddedWidth", "paddedHeight", "rotated") "$Label production atlas placement '$($placement.id)'"
            if (-not $sourceManifestRows.ContainsKey([string]$placement.sourceManifestId)) {
                Write-Error "$Label production atlas placement '$($placement.id)' references unknown source manifest: $($placement.sourceManifestId)"
            }
            if (-not $pageRows.ContainsKey([string]$placement.pageId)) {
                Write-Error "$Label production atlas placement '$($placement.id)' references unknown pageId: $($placement.pageId)"
            }
            if ($placement.rotated -ne $false) {
                Write-Error "$Label production atlas placement '$($placement.id)' rotated must be false"
            }
            if ([int]$placement.width -le 0 -or [int]$placement.height -le 0 -or [int]$placement.paddedWidth -lt [int]$placement.width -or [int]$placement.paddedHeight -lt [int]$placement.height) {
                Write-Error "$Label production atlas placement '$($placement.id)' dimensions must be positive and include padding"
            }
        }
    }
    if (-not $multiPageEvidence) {
        Write-Error "$Label aiWorkflow.twoDCommercialProductionAtlasPacking must include at least one multi-page atlas row"
    }

    $qualityGateRows = Assert-Json2DCommercialProductionAtlasIdMap $atlasSet.qualityGateRows "$Label aiWorkflow.twoDCommercialProductionAtlasPacking.qualityGateRows"
    foreach ($gate in @(
            "padding-and-bleed-control",
            "rotation-policy-disabled",
            "power-of-two-policy-recorded",
            "multiple-pages",
            "deterministic-placement",
            "texture-format-target",
            "mip-policy-base-level-only",
            "package-inclusion"
        )) {
        if (-not $qualityGateRows.ContainsKey($gate)) {
            Write-Error "$Label production atlas qualityGateRows missing $gate"
        }
        if ([string]$qualityGateRows[$gate].status -ne "ready") {
            Write-Error "$Label production atlas quality gate '$gate' must be ready"
        }
    }

    Assert-Properties $atlasSet.packageInclusion @("mode", "runtimePackageFiles", "cookOutputIds", "validationRecipeIds", "evidence") "$Label production atlas packageInclusion"
    if ([string]$atlasSet.packageInclusion.mode -ne "manifest-runtime-package-file-coverage") {
        Write-Error "$Label production atlas packageInclusion.mode must be manifest-runtime-package-file-coverage"
    }
    foreach ($runtimePackageFile in @($atlasSet.packageInclusion.runtimePackageFiles)) {
        if ($runtimePackageFiles -notcontains [string]$runtimePackageFile) {
            Write-Error "$Label production atlas package inclusion references undeclared runtimePackageFiles row: $runtimePackageFile"
        }
    }
    foreach ($cookOutputId in @($atlasSet.packageInclusion.cookOutputIds)) {
        if (-not $cookOutputRows.ContainsKey([string]$cookOutputId)) {
            Write-Error "$Label production atlas package inclusion references unknown cookOutputId: $cookOutputId"
        }
    }
    Assert-Json2DCommercialProductionAtlasRecipeIds $atlasSet.packageInclusion.validationRecipeIds $recipeNames "$Label production atlas packageInclusion"
    Assert-Json2DCommercialProductionAtlasString $atlasSet.packageInclusion.evidence "$Label production atlas packageInclusion.evidence"

    foreach ($unsupportedClaim in @(
            "runtime-source-parsing",
            "runtime-source-image-decoding",
            "external-engine-project-import",
            "external-asset-download",
            "marketplace-asset-ingestion",
            "arbitrary-script-execution",
            "renderer-rhi-residency",
            "native-handle-access",
            "broad-production-atlas-packing",
            "broad-commercial-2d-readiness"
        )) {
        if (@($atlasSet.unsupportedClaims) -notcontains $unsupportedClaim) {
            Write-Error "$Label aiWorkflow.twoDCommercialProductionAtlasPacking unsupportedClaims missing $unsupportedClaim"
        }
    }

    $schemaText = Get-JsonContractSurfaceText "schemas/game-agent.schema.json"
    foreach ($needle in @(
            '"twoDCommercialProductionAtlasPacking"',
            '"2d-commercial-production-atlas-packing-v1"',
            '"padding-and-bleed-control"',
            '"rotation-policy-disabled"',
            '"require-power-of-two-pages"',
            '"base-level-only"'
        )) {
        Assert-ContainsText $schemaText $needle "schemas/game-agent.schema.json 2D commercial production atlas packing schema"
    }

    $assetHeaderText = Get-JsonContractSurfaceText "engine/assets/include/mirakana/assets/sprite_atlas_packing.hpp"
    $assetSourceText = Get-JsonContractSurfaceText "engine/assets/src/sprite_atlas_packing.cpp"
    $assetTestsText = Get-JsonContractSurfaceText "tests/unit/assets_sprite_atlas_packing_tests.cpp"
    foreach ($needle in @(
            "ProductionSpriteAtlasPackingPolicy",
            "pack_production_sprite_atlas_rgba8",
            "ProductionSpriteAtlasPowerOfTwoPolicy",
            "ProductionSpriteAtlasMipPolicy"
        )) {
        Assert-ContainsText $assetHeaderText $needle "production sprite atlas packing header"
    }
    foreach ($needle in @("blit_production_rgba8_rect", "page_count_exceeds_limit", "require_power_of_two_pages")) {
        Assert-ContainsText $assetSourceText $needle "production sprite atlas packing implementation"
    }
    foreach ($needle in @("production sprite atlas packing creates deterministic padded power-of-two pages", "production sprite atlas packing fails closed for invalid bleed and page count")) {
        Assert-ContainsText $assetTestsText $needle "production sprite atlas packing tests"
    }

    $planText = Get-JsonContractSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"
    foreach ($needle in @(
            "twoDCommercialProductionAtlasPacking",
            "2d-commercial-production-atlas-packing-v1",
            "selected first-party production atlas packing",
            "deterministic multi-page atlas placement",
            "runtime source image decoding remains unsupported"
        )) {
        Assert-ContainsText $planText $needle "2D Commercial Production Excellence v1 Phase 2 production atlas evidence"
    }

    $currentCapabilitiesText = Get-JsonContractSurfaceText "docs/current-capabilities.md"
    foreach ($needle in @(
            "2D Commercial Production Atlas Packing v1",
            "twoDCommercialProductionAtlasPacking",
            "selected first-party production atlas packing",
            "broad commercial 2D readiness remains unclaimed"
        )) {
        Assert-ContainsText $currentCapabilitiesText $needle "docs/current-capabilities.md 2D commercial production atlas evidence"
    }

    if ($atlasRows.Count -lt @($atlasSet.atlasRows).Count) {
        Write-Error "$Label atlas row ids must remain unique"
    }
}

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    $requiresCommercial2DProductionAtlas = $game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package"
    Assert-Json2DCommercialProductionAtlasPacking $game $relative $requiresCommercial2DProductionAtlas
}
