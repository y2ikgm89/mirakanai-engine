#requires -Version 7.0
#requires -PSEdition Core

# Chapter 6.2 for check-json-contracts.ps1 AI placeholder asset pipeline contracts.

function Get-JsonPlaceholderPipelinePropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-JsonPlaceholderPipelineString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-JsonPlaceholderPipelinePath([string]$value, [string]$label) {
    Assert-JsonPlaceholderPipelineString $value $label
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

function Get-JsonPlaceholderPipelineGameRoot($game, [string]$label) {
    $entryPoint = ([string]$game.entryPoint).Replace("\", "/")
    $entryPointMatch = [regex]::Match($entryPoint, "^games/([a-z][a-z0-9_]*)/main\.cpp$")
    if (-not $entryPointMatch.Success) {
        Write-Error "$label entryPoint must be games/<game_name>/main.cpp for placeholder pipeline validation"
    }

    return "games/$($entryPointMatch.Groups[1].Value)"
}

function Assert-JsonPlaceholderPipelineGamePath([string]$value, [string]$gameRoot, [string]$label) {
    Assert-JsonPlaceholderPipelinePath $value $label
    $normalized = $value.Replace("\", "/")
    if ($normalized -ne $gameRoot -and -not $normalized.StartsWith("$gameRoot/")) {
        Write-Error "$label must stay under ${gameRoot}: $value"
    }
}

function Assert-JsonPlaceholderPipelineIdMap($rows, [string]$label) {
    $ids = @{}
    foreach ($row in @($rows)) {
        Assert-JsonPlaceholderPipelineString $row.id "$label.id"
        if ($ids.ContainsKey([string]$row.id)) {
            Write-Error "$label has duplicate id: $($row.id)"
        }
        $ids[[string]$row.id] = $true
    }

    return $ids
}

function Assert-JsonPlaceholderPipeline {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $pipeline = Get-JsonPlaceholderPipelinePropertyValue $Game.aiWorkflow "placeholderAssetPipeline"
    if ($null -eq $pipeline) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.placeholderAssetPipeline missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $pipeline @(
        "schemaVersion",
        "capabilityId",
        "pipelineId",
        "sourceRegistryPath",
        "packageIndexPath",
        "reviewedToolSurfaces",
        "plannedAssets",
        "packageHandoff",
        "replacementWorkflow",
        "unsupportedClaims"
    ) "$Label aiWorkflow.placeholderAssetPipeline"

    if ($pipeline.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline schemaVersion must be 1"
    }
    if ($pipeline.capabilityId -ne "ai-placeholder-asset-pipeline-v1") {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline capabilityId must be ai-placeholder-asset-pipeline-v1"
    }

    $gameRoot = Get-JsonPlaceholderPipelineGameRoot $Game $Label
    $expectedPipelineId = "$($Game.name)-placeholder-assets"
    if ($pipeline.pipelineId -ne $expectedPipelineId) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline pipelineId must be $expectedPipelineId"
    }

    Assert-JsonPlaceholderPipelineGamePath ([string]$pipeline.sourceRegistryPath) $gameRoot "$Label aiWorkflow.placeholderAssetPipeline.sourceRegistryPath"
    Assert-JsonPlaceholderPipelineGamePath ([string]$pipeline.packageIndexPath) $gameRoot "$Label aiWorkflow.placeholderAssetPipeline.packageIndexPath"
    if (-not ([string]$pipeline.sourceRegistryPath).Replace("\", "/").StartsWith("$gameRoot/source/")) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline sourceRegistryPath must stay under the game source root"
    }
    if (-not ([string]$pipeline.packageIndexPath).Replace("\", "/").StartsWith("$gameRoot/runtime/")) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline packageIndexPath must stay under the game runtime root"
    }

    $recipeNames = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $recipeNames[[string]$recipe.name] = $true
    }
    $runtimePackageFiles = @($Game.runtimePackageFiles | ForEach-Object { [string]$_ })
    $assetRequestIds = @{}
    foreach ($assetRequest in @($Game.aiWorkflow.gameDesignSpec.assetRequests)) {
        $assetRequestIds[[string]$assetRequest.id] = $true
    }

    $toolSurfaceIds = Assert-JsonPlaceholderPipelineIdMap $pipeline.reviewedToolSurfaces "$Label aiWorkflow.placeholderAssetPipeline.reviewedToolSurfaces"
    foreach ($requiredToolId in @("plan-placeholder-asset-bundle", "plan-placeholder-asset-cook-package")) {
        if (-not $toolSurfaceIds.ContainsKey($requiredToolId)) {
            Write-Error "$Label aiWorkflow.placeholderAssetPipeline reviewedToolSurfaces missing $requiredToolId"
        }
    }
    foreach ($surface in @($pipeline.reviewedToolSurfaces)) {
        Assert-Properties $surface @("id", "mode", "api", "evidence") "$Label aiWorkflow.placeholderAssetPipeline.reviewedToolSurfaces"
        if ([string]$surface.mode -ne "review-only") {
            Write-Error "$Label placeholder surface '$($surface.id)' must be review-only"
        }
        if (@("mirakana::plan_placeholder_asset_bundle", "mirakana::plan_placeholder_asset_cook_package") -notcontains [string]$surface.api) {
            Write-Error "$Label placeholder surface '$($surface.id)' api is unsupported: $($surface.api)"
        }
        Assert-JsonPlaceholderPipelineString $surface.evidence "$Label placeholder surface '$($surface.id)'.evidence"
    }

    $placeholderRoles = @{}
    foreach ($asset in @($pipeline.plannedAssets)) {
        Assert-Properties $asset @(
            "id",
            "designAssetRequestId",
            "placeholderRole",
            "assetKind",
            "assetKey",
            "sourcePath",
            "importedPath",
            "reviewedToolSurfaceId",
            "generator",
            "license",
            "delivery",
            "validationRecipeIds"
        ) "$Label aiWorkflow.placeholderAssetPipeline.plannedAssets"
        Assert-JsonPlaceholderPipelineString $asset.id "$Label placeholder asset id"
        if (-not $assetRequestIds.ContainsKey([string]$asset.designAssetRequestId)) {
            Write-Error "$Label placeholder asset '$($asset.id)' references unknown designAssetRequestId: $($asset.designAssetRequestId)"
        }
        if (@("sprite", "mesh", "material", "audio", "ui", "scene-prop") -notcontains [string]$asset.placeholderRole) {
            Write-Error "$Label placeholder asset '$($asset.id)' placeholderRole is unsupported: $($asset.placeholderRole)"
        }
        $placeholderRoles[[string]$asset.placeholderRole] = $true
        if (@("texture", "mesh", "material", "audio") -notcontains [string]$asset.assetKind) {
            Write-Error "$Label placeholder asset '$($asset.id)' assetKind is unsupported: $($asset.assetKind)"
        }
        Assert-JsonPlaceholderPipelineString $asset.assetKey "$Label placeholder asset '$($asset.id)'.assetKey"
        Assert-JsonPlaceholderPipelineGamePath ([string]$asset.sourcePath) $gameRoot "$Label placeholder asset '$($asset.id)'.sourcePath"
        Assert-JsonPlaceholderPipelineGamePath ([string]$asset.importedPath) $gameRoot "$Label placeholder asset '$($asset.id)'.importedPath"
        if (-not ([string]$asset.sourcePath).Replace("\", "/").StartsWith("$gameRoot/source/")) {
            Write-Error "$Label placeholder asset '$($asset.id)' sourcePath must stay under source/"
        }
        if (-not (([string]$asset.importedPath).Replace("\", "/").StartsWith("$gameRoot/runtime/") -or
                ([string]$asset.importedPath).Replace("\", "/").StartsWith("$gameRoot/intermediate/"))) {
            Write-Error "$Label placeholder asset '$($asset.id)' importedPath must stay under runtime/ or intermediate/"
        }
        if (-not $toolSurfaceIds.ContainsKey([string]$asset.reviewedToolSurfaceId)) {
            Write-Error "$Label placeholder asset '$($asset.id)' references unknown reviewedToolSurfaceId: $($asset.reviewedToolSurfaceId)"
        }
        if ([string]$asset.generator -ne "mirakana-placeholder-asset-tool-v1") {
            Write-Error "$Label placeholder asset '$($asset.id)' must use mirakana-placeholder-asset-tool-v1"
        }
        if ([string]$asset.license -ne "LicenseRef-Proprietary") {
            Write-Error "$Label placeholder asset '$($asset.id)' must record first-party proprietary placeholder license"
        }
        if (@("source-document", "cook-package") -notcontains [string]$asset.delivery) {
            Write-Error "$Label placeholder asset '$($asset.id)' delivery is unsupported: $($asset.delivery)"
        }
        if ([string]$asset.delivery -eq "cook-package") {
            $gameRelativeImportedPath = ([string]$asset.importedPath).Replace("\", "/").Substring($gameRoot.Length + 1)
            if ($runtimePackageFiles -notcontains $gameRelativeImportedPath) {
                Write-Error "$Label placeholder asset '$($asset.id)' cook-package importedPath must be listed in runtimePackageFiles: $gameRelativeImportedPath"
            }
        }
        foreach ($recipeId in @($asset.validationRecipeIds)) {
            if (-not $recipeNames.ContainsKey([string]$recipeId)) {
                Write-Error "$Label placeholder asset '$($asset.id)' references undeclared validation recipe: $recipeId"
            }
        }
    }
    if (@($pipeline.plannedAssets).Count -lt 1) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline plannedAssets must contain placeholder rows"
    }
    foreach ($requiredRole in @("sprite", "audio")) {
        if ($Game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package" -and -not $placeholderRoles.ContainsKey($requiredRole)) {
            Write-Error "$Label aiWorkflow.placeholderAssetPipeline must include $requiredRole placeholder coverage"
        }
    }
    foreach ($requiredRole in @("mesh", "material", "ui", "scene-prop")) {
        if ($Game.gameplayContract.productionRecipe -eq "3d-playable-desktop-package" -and -not $placeholderRoles.ContainsKey($requiredRole)) {
            Write-Error "$Label aiWorkflow.placeholderAssetPipeline must include $requiredRole placeholder coverage"
        }
    }

    Assert-Properties $pipeline.packageHandoff @("mode", "sourceRevision", "plannedAssetCount", "sourceDocumentCount", "provenanceRowCount", "runtimePackageFileCount", "runtimePackageFiles", "validationRecipeIds", "evidence") "$Label aiWorkflow.placeholderAssetPipeline.packageHandoff"
    if ([string]$pipeline.packageHandoff.mode -ne "reviewed-cook-package") {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline packageHandoff.mode must be reviewed-cook-package"
    }
    if ([int]$pipeline.packageHandoff.sourceRevision -lt 1) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline packageHandoff.sourceRevision must be positive"
    }
    if ([int]$pipeline.packageHandoff.plannedAssetCount -ne @($pipeline.plannedAssets).Count) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline packageHandoff.plannedAssetCount must match plannedAssets count"
    }
    if ([int]$pipeline.packageHandoff.sourceDocumentCount -ne @($pipeline.plannedAssets).Count) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline packageHandoff.sourceDocumentCount must match plannedAssets count"
    }
    if ([int]$pipeline.packageHandoff.provenanceRowCount -ne @($pipeline.plannedAssets).Count) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline packageHandoff.provenanceRowCount must match plannedAssets count"
    }
    if ([int]$pipeline.packageHandoff.runtimePackageFileCount -ne @($pipeline.packageHandoff.runtimePackageFiles).Count) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline packageHandoff.runtimePackageFileCount must match runtimePackageFiles count"
    }
    foreach ($runtimeFile in @($pipeline.packageHandoff.runtimePackageFiles)) {
        Assert-JsonPlaceholderPipelineString $runtimeFile "$Label packageHandoff.runtimePackageFiles"
        if ($runtimePackageFiles -notcontains [string]$runtimeFile) {
            Write-Error "$Label packageHandoff.runtimePackageFiles references undeclared runtime package file: $runtimeFile"
        }
    }
    foreach ($recipeId in @($pipeline.packageHandoff.validationRecipeIds)) {
        if (-not $recipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "$Label packageHandoff references undeclared validation recipe: $recipeId"
        }
    }
    Assert-JsonPlaceholderPipelineString $pipeline.packageHandoff.evidence "$Label packageHandoff.evidence"

    Assert-Properties $pipeline.replacementWorkflow @("mode", "reviewedToolSurfaceIds", "allowedReplacementSources", "provenanceRequirements", "packageHandoffRequired", "validationRecipeIds", "evidence") "$Label aiWorkflow.placeholderAssetPipeline.replacementWorkflow"
    if ([string]$pipeline.replacementWorkflow.mode -ne "reviewed-placeholder-replacement") {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline replacementWorkflow.mode must be reviewed-placeholder-replacement"
    }
    foreach ($requiredToolId in @("plan-placeholder-asset-bundle", "plan-placeholder-asset-cook-package")) {
        if (@($pipeline.replacementWorkflow.reviewedToolSurfaceIds) -notcontains $requiredToolId) {
            Write-Error "$Label aiWorkflow.placeholderAssetPipeline replacementWorkflow.reviewedToolSurfaceIds missing $requiredToolId"
        }
    }
    foreach ($toolSurfaceId in @($pipeline.replacementWorkflow.reviewedToolSurfaceIds)) {
        if (-not $toolSurfaceIds.ContainsKey([string]$toolSurfaceId)) {
            Write-Error "$Label replacementWorkflow references unknown reviewedToolSurfaceId: $toolSurfaceId"
        }
    }
    foreach ($source in @("first-party-placeholder-regeneration", "game-owned-source-document")) {
        if (@($pipeline.replacementWorkflow.allowedReplacementSources) -notcontains $source) {
            Write-Error "$Label aiWorkflow.placeholderAssetPipeline replacementWorkflow.allowedReplacementSources missing $source"
        }
    }
    foreach ($requirement in @("stable-asset-key", "LicenseRef-Proprietary", "content-hash", "source-revision")) {
        if (@($pipeline.replacementWorkflow.provenanceRequirements) -notcontains $requirement) {
            Write-Error "$Label aiWorkflow.placeholderAssetPipeline replacementWorkflow.provenanceRequirements missing $requirement"
        }
    }
    if (-not [bool]$pipeline.replacementWorkflow.packageHandoffRequired) {
        Write-Error "$Label aiWorkflow.placeholderAssetPipeline replacementWorkflow.packageHandoffRequired must be true"
    }
    foreach ($recipeId in @($pipeline.replacementWorkflow.validationRecipeIds)) {
        if (-not $recipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "$Label replacementWorkflow references undeclared validation recipe: $recipeId"
        }
    }
    Assert-JsonPlaceholderPipelineString $pipeline.replacementWorkflow.evidence "$Label replacementWorkflow.evidence"

    foreach ($unsupportedClaim in @(
            "external-asset-download",
            "arbitrary-image-generation",
            "runtime-source-parsing",
            "renderer-rhi-residency",
            "native-handles",
            "middleware-contracts",
            "broad-art-direction",
            "broad-commercial-quality"
        )) {
        if (@($pipeline.unsupportedClaims) -notcontains $unsupportedClaim) {
            Write-Error "$Label aiWorkflow.placeholderAssetPipeline unsupportedClaims missing $unsupportedClaim"
        }
    }
}

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    $requiresPlaceholderPipeline = @("2d-desktop-runtime-package", "3d-playable-desktop-package") -contains $game.gameplayContract.productionRecipe
    Assert-JsonPlaceholderPipeline $game $relative $requiresPlaceholderPipeline
}

$selectedPipelineRoles = @{}
foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $pipeline = Get-JsonPlaceholderPipelinePropertyValue $gameManifestEntry.Game.aiWorkflow "placeholderAssetPipeline"
    if ($null -eq $pipeline) {
        continue
    }
    foreach ($asset in @($pipeline.plannedAssets)) {
        $selectedPipelineRoles[[string]$asset.placeholderRole] = $true
    }
}
foreach ($requiredRole in @("sprite", "material", "audio")) {
    if (-not $selectedPipelineRoles.ContainsKey($requiredRole)) {
        Write-Error "AI placeholder asset pipeline selected manifests must include $requiredRole placeholder coverage"
    }
}
