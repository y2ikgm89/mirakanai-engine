#requires -Version 7.0
#requires -PSEdition Core

# Chapter 7.7 for check-json-contracts.ps1 2D commercial runtime source parser exclusion contracts.

function Get-Json2DCommercialRuntimeSourceParserExclusionPropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-Json2DCommercialRuntimeSourceParserExclusionString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-Json2DCommercialRuntimeSourceParserExclusionPath([string]$value, [string]$label) {
    Assert-Json2DCommercialRuntimeSourceParserExclusionString $value $label
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

function Get-Json2DCommercialRuntimeSourceParserExclusionGameRoot($game, [string]$label) {
    $entryPoint = ([string]$game.entryPoint).Replace("\", "/")
    $entryPointMatch = [regex]::Match($entryPoint, "^games/([a-z][a-z0-9_]*)/main\.cpp$")
    if (-not $entryPointMatch.Success) {
        Write-Error "$label entryPoint must be games/<game_name>/main.cpp for 2D commercial runtime source parser exclusion validation"
    }

    return "games/$($entryPointMatch.Groups[1].Value)"
}

function Assert-Json2DCommercialRuntimeSourceParserExclusionGamePath([string]$value, [string]$gameRoot, [string]$label) {
    Assert-Json2DCommercialRuntimeSourceParserExclusionPath $value $label
    $normalized = $value.Replace("\", "/")
    if ($normalized -ne $gameRoot -and -not $normalized.StartsWith("$gameRoot/")) {
        Write-Error "$label must stay under ${gameRoot}: $value"
    }
}

function Assert-Json2DCommercialRuntimeSourceParserExclusionIdMap($rows, [string]$label) {
    $ids = @{}
    foreach ($row in @($rows)) {
        Assert-Json2DCommercialRuntimeSourceParserExclusionString $row.id "$label.id"
        if ($ids.ContainsKey([string]$row.id)) {
            Write-Error "$label has duplicate id: $($row.id)"
        }
        $ids[[string]$row.id] = $row
    }

    return $ids
}

function Assert-Json2DCommercialRuntimeSourceParserExclusionRecipeIds($recipeIds, $recipeNames, [string]$label) {
    foreach ($recipeId in @($recipeIds)) {
        if (-not $recipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "$label references undeclared validation recipe: $recipeId"
        }
    }
}

function Test-Json2DCommercialRuntimeSourceParserExclusionForbiddenRuntimePath([string]$runtimePath, [string[]]$forbiddenExtensions) {
    $normalized = $runtimePath.Replace("\", "/").ToLowerInvariant()
    if ($normalized.StartsWith("source/") -or $normalized.Contains("/source/")) {
        return $true
    }

    foreach ($extension in $forbiddenExtensions) {
        if ($normalized.EndsWith($extension.ToLowerInvariant())) {
            return $true
        }
    }

    return $false
}

function Assert-Json2DCommercialRuntimeSourceParserExclusion {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $exclusionSet = Get-Json2DCommercialRuntimeSourceParserExclusionPropertyValue $Game.aiWorkflow "twoDCommercialRuntimeSourceParserExclusion"
    if ($null -eq $exclusionSet) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.twoDCommercialRuntimeSourceParserExclusion missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $exclusionSet @(
        "schemaVersion",
        "capabilityId",
        "exclusionSetId",
        "sourceAssetManifestSetId",
        "deterministicCookSetId",
        "productionAtlasPackingSetId",
        "packageIndexPath",
        "runtimePackagePolicy",
        "parserExclusionRows",
        "runtimePackageCoverage",
        "cleanRoomPolicy",
        "unsupportedClaims"
    ) "$Label aiWorkflow.twoDCommercialRuntimeSourceParserExclusion"

    if ($exclusionSet.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.twoDCommercialRuntimeSourceParserExclusion schemaVersion must be 1"
    }
    if ($exclusionSet.capabilityId -ne "2d-commercial-runtime-source-parser-exclusion-v1") {
        Write-Error "$Label aiWorkflow.twoDCommercialRuntimeSourceParserExclusion capabilityId must be 2d-commercial-runtime-source-parser-exclusion-v1"
    }

    $gameRoot = Get-Json2DCommercialRuntimeSourceParserExclusionGameRoot $Game $Label
    $expectedExclusionSetId = "$($Game.name)-2d-commercial-runtime-source-parser-exclusion"
    if ($exclusionSet.exclusionSetId -ne $expectedExclusionSetId) {
        Write-Error "$Label aiWorkflow.twoDCommercialRuntimeSourceParserExclusion exclusionSetId must be $expectedExclusionSetId"
    }

    $sourceManifestSet = Get-Json2DCommercialRuntimeSourceParserExclusionPropertyValue $Game.aiWorkflow "twoDCommercialSourceAssetManifests"
    $cookSet = Get-Json2DCommercialRuntimeSourceParserExclusionPropertyValue $Game.aiWorkflow "twoDCommercialDeterministicCookOutputs"
    $atlasSet = Get-Json2DCommercialRuntimeSourceParserExclusionPropertyValue $Game.aiWorkflow "twoDCommercialProductionAtlasPacking"
    if ($null -eq $sourceManifestSet -or $null -eq $cookSet -or $null -eq $atlasSet) {
        Write-Error "$Label aiWorkflow.twoDCommercialRuntimeSourceParserExclusion requires source manifest, deterministic cook, and production atlas descriptors"
    } else {
        if ($exclusionSet.sourceAssetManifestSetId -ne $sourceManifestSet.manifestSetId) {
            Write-Error "$Label runtime source parser exclusion sourceAssetManifestSetId must match twoDCommercialSourceAssetManifests.manifestSetId"
        }
        if ($exclusionSet.deterministicCookSetId -ne $cookSet.cookSetId) {
            Write-Error "$Label runtime source parser exclusion deterministicCookSetId must match twoDCommercialDeterministicCookOutputs.cookSetId"
        }
        if ($exclusionSet.productionAtlasPackingSetId -ne $atlasSet.packingSetId) {
            Write-Error "$Label runtime source parser exclusion productionAtlasPackingSetId must match twoDCommercialProductionAtlasPacking.packingSetId"
        }
        if ($exclusionSet.packageIndexPath -ne $cookSet.packageIndexPath -or $exclusionSet.packageIndexPath -ne $atlasSet.packageIndexPath) {
            Write-Error "$Label runtime source parser exclusion packageIndexPath must match deterministic cook and production atlas descriptors"
        }
    }

    Assert-Json2DCommercialRuntimeSourceParserExclusionGamePath ([string]$exclusionSet.packageIndexPath) $gameRoot "$Label aiWorkflow.twoDCommercialRuntimeSourceParserExclusion.packageIndexPath"

    $recipeNames = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $recipeNames[[string]$recipe.name] = $true
    }

    $runtimePackageFiles = @($Game.runtimePackageFiles | ForEach-Object { [string]$_ })
    $runtimePackageFileSet = @{}
    foreach ($runtimePackageFile in $runtimePackageFiles) {
        $runtimePackageFileSet[$runtimePackageFile] = $true
    }

    Assert-Properties $exclusionSet.runtimePackagePolicy @(
        "mode",
        "runtimeSourceParsing",
        "sourceImageDecoding",
        "sourceAudioDecoding",
        "sourceFontLoading",
        "sourceRegistryRuntimeParsing",
        "packageScriptExecution",
        "runtimePackageFilesContainCookedOnly",
        "evidence"
    ) "$Label runtime source parser exclusion runtimePackagePolicy"
    if ([string]$exclusionSet.runtimePackagePolicy.mode -ne "cooked-runtime-package-only") {
        Write-Error "$Label runtimePackagePolicy.mode must be cooked-runtime-package-only"
    }
    foreach ($propertyName in @("runtimeSourceParsing", "sourceImageDecoding", "sourceAudioDecoding", "sourceFontLoading", "sourceRegistryRuntimeParsing", "packageScriptExecution")) {
        if ([string](Get-Json2DCommercialRuntimeSourceParserExclusionPropertyValue $exclusionSet.runtimePackagePolicy $propertyName) -ne "unsupported") {
            Write-Error "$Label runtimePackagePolicy.$propertyName must be unsupported"
        }
    }
    if ($exclusionSet.runtimePackagePolicy.runtimePackageFilesContainCookedOnly -ne $true) {
        Write-Error "$Label runtimePackagePolicy.runtimePackageFilesContainCookedOnly must be true"
    }
    Assert-Json2DCommercialRuntimeSourceParserExclusionString $exclusionSet.runtimePackagePolicy.evidence "$Label runtimePackagePolicy.evidence"

    $parserRows = Assert-Json2DCommercialRuntimeSourceParserExclusionIdMap $exclusionSet.parserExclusionRows "$Label parserExclusionRows"
    foreach ($requiredRow in @(
            "png-source-image-decoder",
            "jpeg-source-image-decoder",
            "source-audio-codec-decoder",
            "source-font-loader",
            "source-asset-registry-parser",
            "external-engine-project-importer",
            "marketplace-asset-ingestion",
            "arbitrary-package-script"
        )) {
        if (-not $parserRows.ContainsKey($requiredRow)) {
            Write-Error "$Label parserExclusionRows missing $requiredRow"
        }
    }
    foreach ($row in @($exclusionSet.parserExclusionRows)) {
        Assert-Properties $row @("id", "sourceCategory", "runtimeSurface", "status", "promotionRequirement", "evidence") "$Label parserExclusionRows '$($row.id)'"
        if ([string]$row.runtimeSurface -ne "shipping-runtime") {
            Write-Error "$Label parserExclusionRows '$($row.id)' runtimeSurface must be shipping-runtime"
        }
        if ([string]$row.status -ne "unsupported") {
            Write-Error "$Label parserExclusionRows '$($row.id)' status must be unsupported"
        }
        if ([string]$row.promotionRequirement -ne "separate-dependency-license-security-plan") {
            Write-Error "$Label parserExclusionRows '$($row.id)' promotionRequirement must be separate-dependency-license-security-plan"
        }
        Assert-Json2DCommercialRuntimeSourceParserExclusionString $row.evidence "$Label parserExclusionRows '$($row.id)'.evidence"
    }

    Assert-Properties $exclusionSet.runtimePackageCoverage @(
        "mode",
        "runtimePackageFiles",
        "deterministicCookOutputIds",
        "atlasRuntimePackageFiles",
        "forbiddenSourceExtensions",
        "validationRecipeIds",
        "evidence"
    ) "$Label runtimePackageCoverage"
    if ([string]$exclusionSet.runtimePackageCoverage.mode -ne "manifest-runtime-package-files-are-cooked-only") {
        Write-Error "$Label runtimePackageCoverage.mode must be manifest-runtime-package-files-are-cooked-only"
    }
    $forbiddenSourceExtensions = @($exclusionSet.runtimePackageCoverage.forbiddenSourceExtensions | ForEach-Object { [string]$_ })
    foreach ($requiredExtension in @(".png", ".jpg", ".jpeg", ".wav", ".ogg", ".mp3", ".flac", ".ttf", ".otf", ".woff", ".woff2", ".texture_source", ".sprite_sheet_source", ".atlas_page_source", ".tile_chunk_source", ".animation_clip_source", ".audio_cue_source", ".localization_text_source", ".ui_glyph_input_source", ".geassets")) {
        if ($forbiddenSourceExtensions -notcontains $requiredExtension) {
            Write-Error "$Label runtimePackageCoverage.forbiddenSourceExtensions missing $requiredExtension"
        }
    }
    foreach ($runtimePackageFile in @($exclusionSet.runtimePackageCoverage.runtimePackageFiles)) {
        $runtimePackageFile = [string]$runtimePackageFile
        if (-not $runtimePackageFileSet.ContainsKey($runtimePackageFile)) {
            Write-Error "$Label runtimePackageCoverage references undeclared runtimePackageFiles row: $runtimePackageFile"
        }
        if (Test-Json2DCommercialRuntimeSourceParserExclusionForbiddenRuntimePath $runtimePackageFile $forbiddenSourceExtensions) {
            Write-Error "$Label runtimePackageCoverage must not include source parser input in runtimePackageFiles: $runtimePackageFile"
        }
    }
    foreach ($runtimePackageFile in $runtimePackageFiles) {
        if (@($exclusionSet.runtimePackageCoverage.runtimePackageFiles) -notcontains $runtimePackageFile) {
            Write-Error "$Label runtimePackageCoverage must cover every runtimePackageFiles row: $runtimePackageFile"
        }
    }

    $sourceManifestRows = Assert-Json2DCommercialRuntimeSourceParserExclusionIdMap $sourceManifestSet.assetManifests "$Label source manifests"
    foreach ($sourceRow in @($sourceManifestSet.assetManifests)) {
        $sourceGameRelativePath = ([string]$sourceRow.sourcePath).Replace("\", "/").Substring($gameRoot.Length + 1)
        if ($runtimePackageFileSet.ContainsKey($sourceGameRelativePath)) {
            Write-Error "$Label source manifest '$($sourceRow.id)' sourcePath must not be listed in runtimePackageFiles"
        }
        if (-not ([string]$sourceRow.sourcePath).Replace("\", "/").StartsWith("$gameRoot/source/")) {
            Write-Error "$Label source manifest '$($sourceRow.id)' sourcePath must stay under source/"
        }
    }

    $cookOutputRows = Assert-Json2DCommercialRuntimeSourceParserExclusionIdMap $cookSet.cookOutputs "$Label deterministic cook outputs"
    foreach ($cookOutputId in @($exclusionSet.runtimePackageCoverage.deterministicCookOutputIds)) {
        if (-not $cookOutputRows.ContainsKey([string]$cookOutputId)) {
            Write-Error "$Label runtimePackageCoverage references unknown deterministic cook output: $cookOutputId"
        }
    }
    foreach ($cookOutput in @($cookSet.cookOutputs)) {
        if (@($exclusionSet.runtimePackageCoverage.deterministicCookOutputIds) -notcontains [string]$cookOutput.id) {
            Write-Error "$Label runtimePackageCoverage must include every deterministic cook output id: $($cookOutput.id)"
        }
        $cookGameRelativePath = ([string]$cookOutput.outputPath).Replace("\", "/").Substring($gameRoot.Length + 1)
        if (-not $runtimePackageFileSet.ContainsKey($cookGameRelativePath)) {
            Write-Error "$Label deterministic cook output '$($cookOutput.id)' must map to runtimePackageFiles: $cookGameRelativePath"
        }
    }

    foreach ($atlasRuntimePackageFile in @($exclusionSet.runtimePackageCoverage.atlasRuntimePackageFiles)) {
        if (-not $runtimePackageFileSet.ContainsKey([string]$atlasRuntimePackageFile)) {
            Write-Error "$Label runtimePackageCoverage atlasRuntimePackageFiles references undeclared runtime package file: $atlasRuntimePackageFile"
        }
        if (@($atlasSet.packageInclusion.runtimePackageFiles) -notcontains [string]$atlasRuntimePackageFile) {
            Write-Error "$Label runtimePackageCoverage atlasRuntimePackageFiles must match production atlas package inclusion: $atlasRuntimePackageFile"
        }
    }
    Assert-Json2DCommercialRuntimeSourceParserExclusionRecipeIds $exclusionSet.runtimePackageCoverage.validationRecipeIds $recipeNames "$Label runtimePackageCoverage"
    Assert-Json2DCommercialRuntimeSourceParserExclusionString $exclusionSet.runtimePackageCoverage.evidence "$Label runtimePackageCoverage.evidence"

    Assert-Properties $exclusionSet.cleanRoomPolicy @(
        "sourceMaterial",
        "thirdPartyDependenciesAdded",
        "externalEngineCodeAssetsSamplesUi",
        "compatibilityParityEndorsementClaims",
        "legalReviewStatus",
        "evidence"
    ) "$Label cleanRoomPolicy"
    if ([string]$exclusionSet.cleanRoomPolicy.sourceMaterial -ne "first-party-cooked-runtime-only") {
        Write-Error "$Label cleanRoomPolicy.sourceMaterial must be first-party-cooked-runtime-only"
    }
    if ($exclusionSet.cleanRoomPolicy.thirdPartyDependenciesAdded -ne $false) {
        Write-Error "$Label cleanRoomPolicy.thirdPartyDependenciesAdded must be false"
    }
    foreach ($propertyName in @("externalEngineCodeAssetsSamplesUi", "compatibilityParityEndorsementClaims")) {
        if ([string](Get-Json2DCommercialRuntimeSourceParserExclusionPropertyValue $exclusionSet.cleanRoomPolicy $propertyName) -ne "blocked") {
            Write-Error "$Label cleanRoomPolicy.$propertyName must be blocked"
        }
    }
    if ([string]$exclusionSet.cleanRoomPolicy.legalReviewStatus -ne "not-legal-advice-no-new-third-party-material") {
        Write-Error "$Label cleanRoomPolicy.legalReviewStatus must be not-legal-advice-no-new-third-party-material"
    }
    Assert-Json2DCommercialRuntimeSourceParserExclusionString $exclusionSet.cleanRoomPolicy.evidence "$Label cleanRoomPolicy.evidence"

    foreach ($unsupportedClaim in @(
            "runtime-source-parsing",
            "runtime-source-image-decoding",
            "runtime-source-audio-decoding",
            "runtime-source-font-loading",
            "runtime-source-registry-parsing",
            "external-engine-project-import",
            "external-asset-download",
            "marketplace-asset-ingestion",
            "arbitrary-script-execution",
            "package-script-execution",
            "native-handle-access",
            "renderer-rhi-residency",
            "broad-codec-readiness",
            "external-engine-compatibility-claim",
            "legal-approval-automation",
            "broad-commercial-2d-readiness"
        )) {
        if (@($exclusionSet.unsupportedClaims) -notcontains $unsupportedClaim) {
            Write-Error "$Label aiWorkflow.twoDCommercialRuntimeSourceParserExclusion unsupportedClaims missing $unsupportedClaim"
        }
    }

    $schemaText = Get-JsonContractSurfaceText "schemas/game-agent.schema.json"
    foreach ($needle in @(
            '"twoDCommercialRuntimeSourceParserExclusion"',
            '"2d-commercial-runtime-source-parser-exclusion-v1"',
            '"cooked-runtime-package-only"',
            '"separate-dependency-license-security-plan"',
            '"first-party-cooked-runtime-only"'
        )) {
        Assert-ContainsText $schemaText $needle "schemas/game-agent.schema.json 2D commercial runtime source parser exclusion schema"
    }

    $planText = Get-JsonContractSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"
    foreach ($needle in @(
            "twoDCommercialRuntimeSourceParserExclusion",
            "2d-commercial-runtime-source-parser-exclusion-v1",
            "shipping runtime consumes only reviewed cooked payloads",
            "separate dependency/license/security plan",
            "Unity/Unreal Engine/Godot compatibility, equivalence, parity, replacement, endorsement, and legal approval remain unclaimed"
        )) {
        Assert-ContainsText $planText $needle "2D Commercial Production Excellence v1 Phase 2 runtime source parser exclusion evidence"
    }

    $currentCapabilitiesText = Get-JsonContractSurfaceText "docs/current-capabilities.md"
    foreach ($needle in @(
            "2D Commercial Runtime Source Parser Exclusion v1",
            "twoDCommercialRuntimeSourceParserExclusion",
            "shipping runtime consumes only reviewed cooked payloads",
            "broad commercial 2D readiness remains unclaimed"
        )) {
        Assert-ContainsText $currentCapabilitiesText $needle "docs/current-capabilities.md 2D commercial runtime source parser exclusion evidence"
    }

    $gameGuidanceText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
    Assert-ContainsText $gameGuidanceText "twoDCommercialRuntimeSourceParserExclusion" "engine/agent/manifest.fragments/014-gameCodeGuidance.json runtime source parser exclusion guidance"

    if ($sourceManifestRows.Count -lt @($sourceManifestSet.assetManifests).Count) {
        Write-Error "$Label source manifest ids must remain unique"
    }
}

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    $requiresRuntimeSourceParserExclusion = $game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package"
    Assert-Json2DCommercialRuntimeSourceParserExclusion $game $relative $requiresRuntimeSourceParserExclusion
}
