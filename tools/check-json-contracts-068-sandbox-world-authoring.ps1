#requires -Version 7.0
#requires -PSEdition Core

function Assert-SandboxWorldAuthoringTargets($game, [string]$relativePath, [bool]$required) {
    $gameDirectory = Get-GameDirectoryFromManifestPath $relativePath
    $packageFileSet = @{}
    foreach ($packageFile in (Get-NormalizedRuntimePackageFiles $game $relativePath)) {
        $packageFileSet[$packageFile.Substring($gameDirectory.Length + 1)] = $true
    }

    if (-not $game.PSObject.Properties.Name.Contains("sandboxWorldAuthoringTargets")) {
        if ($required) {
            Write-Error "$relativePath 2D package manifests must declare sandboxWorldAuthoringTargets"
        }
        return
    }
    if ($null -eq $game.sandboxWorldAuthoringTargets -or $game.sandboxWorldAuthoringTargets -isnot [System.Array]) {
        Write-Error "$relativePath sandboxWorldAuthoringTargets must be an array"
    }
    if ($required -and @($game.sandboxWorldAuthoringTargets).Count -lt 1) {
        Write-Error "$relativePath sandboxWorldAuthoringTargets must contain at least one target"
    }

    $validationRecipeIds = @{}
    foreach ($recipe in @($game.validationRecipes)) {
        $validationRecipeIds[[string]$recipe.name] = $true
    }
    $requiredFields = @(
        "id", "mode", "reviewApi", "applyApi", "packageIndexPath", "tilemapPath", "reviewDocumentPath",
        "atlasTexturePath", "tilemapAssetKey", "atlasTextureAssetKey", "tileDefinitionRows", "paletteBrushRows",
        "chunkTemplateRows", "proceduralSeedRows", "changedFileRows", "tilemapPackageChangedFileRows",
        "packageDependencyEdges", "externalImageDecoding", "externalDownload", "importerPluginExecution",
        "runtimeSourceParsing", "rendererRhiResidency", "packageApplyDuringReview", "nativeHandles",
        "preflightRecipeIds"
    )
    $unsupportedSentinelFields = @(
        "externalImageDecoding", "externalDownload", "importerPluginExecution", "runtimeSourceParsing",
        "rendererRhiResidency", "packageApplyDuringReview", "nativeHandles"
    )
    $seenIds = @{}
    foreach ($target in @($game.sandboxWorldAuthoringTargets)) {
        Assert-Properties $target $requiredFields "$relativePath sandboxWorldAuthoringTargets"
        $id = [string]$target.id
        if ($id -notmatch "^[a-z][a-z0-9-]*$") {
            Write-Error "$relativePath sandboxWorldAuthoringTargets id must be kebab-case: $id"
        }
        if ($seenIds.ContainsKey($id)) {
            Write-Error "$relativePath sandboxWorldAuthoringTargets id is duplicated: $id"
        }
        $seenIds[$id] = $true

        foreach ($pathRow in @(
                @{ Field = "packageIndexPath"; Path = ConvertTo-RepoPath ([string]$target.packageIndexPath); Extension = ".geindex"; RuntimeFile = $true; MustExist = $true },
                @{ Field = "tilemapPath"; Path = ConvertTo-RepoPath ([string]$target.tilemapPath); Extension = ".tilemap"; RuntimeFile = $true; MustExist = $true },
                @{ Field = "reviewDocumentPath"; Path = ConvertTo-RepoPath ([string]$target.reviewDocumentPath); Extension = ".sandbox_authoring"; RuntimeFile = $false; MustExist = $false },
                @{ Field = "atlasTexturePath"; Path = ConvertTo-RepoPath ([string]$target.atlasTexturePath); Extension = ".geasset"; RuntimeFile = $true; MustExist = $true }
            )) {
            if (-not (Test-SafeGameRelativePath $pathRow.Path) -or -not $pathRow.Path.EndsWith($pathRow.Extension)) {
                Write-Error "$relativePath sandboxWorldAuthoringTargets $($pathRow.Field) must be a safe game-relative $($pathRow.Extension) path: $($pathRow.Path)"
            }
            if ($pathRow.RuntimeFile -and -not $packageFileSet.ContainsKey($pathRow.Path)) {
                Write-Error "$relativePath sandboxWorldAuthoringTargets $($pathRow.Field) must also be declared in runtimePackageFiles: $($pathRow.Path)"
            }
            if (-not $pathRow.RuntimeFile -and $packageFileSet.ContainsKey($pathRow.Path)) {
                Write-Error "$relativePath sandboxWorldAuthoringTargets $($pathRow.Field) must not be declared in runtimePackageFiles: $($pathRow.Path)"
            }
            if ($pathRow.MustExist -and -not (Test-Path -LiteralPath (Join-Path $root "$gameDirectory/$($pathRow.Path)"))) {
                Write-Error "$relativePath sandboxWorldAuthoringTargets $($pathRow.Field) does not exist: $($pathRow.Path)"
            }
        }

        foreach ($assetKey in @([string]$target.tilemapAssetKey, [string]$target.atlasTextureAssetKey)) {
            if (-not (Test-SafeAssetKey $assetKey)) {
                Write-Error "$relativePath sandboxWorldAuthoringTargets asset keys must be safe AssetKeyV2 values: $assetKey"
            }
        }
        if ([string]$target.mode -ne "reviewed-tilemap-sandbox-world-authoring") {
            Write-Error "$relativePath sandboxWorldAuthoringTargets mode must be reviewed-tilemap-sandbox-world-authoring"
        }
        if ([string]$target.reviewApi -ne "review_sandbox_world_authoring_package") {
            Write-Error "$relativePath sandboxWorldAuthoringTargets reviewApi must be review_sandbox_world_authoring_package"
        }
        if ([string]$target.applyApi -ne "apply_sandbox_world_authoring_package") {
            Write-Error "$relativePath sandboxWorldAuthoringTargets applyApi must be apply_sandbox_world_authoring_package"
        }
        foreach ($field in $unsupportedSentinelFields) {
            if ([string]$target.$field -ne "unsupported") {
                Write-Error "$relativePath sandboxWorldAuthoringTargets $field must remain unsupported"
            }
        }
        foreach ($countField in @(
                "tileDefinitionRows", "paletteBrushRows", "chunkTemplateRows", "proceduralSeedRows",
                "changedFileRows", "tilemapPackageChangedFileRows", "packageDependencyEdges"
            )) {
            if ([int64]$target.$countField -lt 1) {
                Write-Error "$relativePath sandboxWorldAuthoringTargets $countField must be positive"
            }
        }
        foreach ($recipeId in @($target.preflightRecipeIds)) {
            if (-not $validationRecipeIds.ContainsKey([string]$recipeId)) {
                Write-Error "$relativePath sandboxWorldAuthoringTargets preflightRecipeIds must reference validationRecipes names: $recipeId"
            }
        }
        foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourceImagePath", "sourceDownloadUrl", "importerPlugin", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice", "runtimeImageDecoding", "productionAtlasPacking", "tilemapEditorUX", "productionSpriteBatching", "nativeGpuOutput", "packageStreamingReady", "rendererQualityClaim", "metalReady")) {
            if ($target.PSObject.Properties.Name.Contains($forbiddenField)) {
                Write-Error "$relativePath sandboxWorldAuthoringTargets must not expose unsupported production, renderer, native-handle, download, or importer field: $forbiddenField"
            }
        }
    }
}

foreach ($manifest in Get-GameAgentManifests) {
    $requiresTargets = $manifest.Game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package"
    Assert-SandboxWorldAuthoringTargets $manifest.Game $manifest.RelativePath $requiresTargets
}

$sampleManifest = Get-GameAgentManifest "games/sample_2d_desktop_runtime_package/game.agent.json"
if ($sampleManifest) {
    if (@($sampleManifest.Game.engineModules) -notcontains "MK_tools") {
        Write-Error "$($sampleManifest.RelativePath) engineModules missing MK_tools"
    }
    foreach ($needle in @(
            "--require-sandbox-authoring-review", "installed-2d-sandbox-authoring-review-smoke",
            "sandboxWorldAuthoringTargets", "review_sandbox_world_authoring_package",
            "apply_sandbox_world_authoring_package", "sandbox_authoring_review_status=ready",
            "sandbox_authoring_review_external_image_decoding_invoked=0",
            "sandbox_authoring_review_external_download_invoked=0",
            "sandbox_authoring_review_importer_plugin_invoked=0",
            "sandbox_authoring_review_package_apply_invoked=0"
        )) {
        if (-not $sampleManifest.Text.Contains($needle)) {
            Write-Error "$($sampleManifest.RelativePath) missing sandbox authoring review contract text: $needle"
        }
    }
}

$sampleMainText = Get-Content -LiteralPath (Join-Path $root "games/sample_2d_desktop_runtime_package/main.cpp") -Raw
foreach ($needle in @(
        "mirakana/tools/sandbox_world_authoring.hpp", "--require-sandbox-authoring-review",
        "review_sandbox_world_authoring_package", "sandbox_authoring_review_status=", "sandbox_authoring_review_ready=",
        "sandbox_authoring_review_tile_definition_rows=", "sandbox_authoring_review_palette_brush_rows=",
        "sandbox_authoring_review_chunk_template_rows=", "sandbox_authoring_review_procedural_seed_rows=",
        "sandbox_authoring_review_changed_files=", "sandbox_authoring_review_tilemap_package_changed_files=",
        "sandbox_authoring_review_package_dependency_edges=", "sandbox_authoring_review_preview_hash=",
        "sandbox_authoring_review_external_image_decoding_invoked=",
        "sandbox_authoring_review_external_download_invoked=", "sandbox_authoring_review_importer_plugin_invoked=",
        "sandbox_authoring_review_package_apply_invoked=", "sandbox_authoring_review_diagnostics=",
        "required_sandbox_authoring_review_unavailable"
    )) {
    if (-not $sampleMainText.Contains($needle)) {
        Write-Error "games/sample_2d_desktop_runtime_package/main.cpp missing sandbox authoring review smoke field: $needle"
    }
}

$gamesCMakeText = Get-Content -LiteralPath (Join-Path $root "games/CMakeLists.txt") -Raw
foreach ($needle in @("--require-sandbox-authoring-review", "MK_tools")) {
    if (-not $gamesCMakeText.Contains($needle)) {
        Write-Error "games/CMakeLists.txt missing sandbox authoring review package metadata: $needle"
    }
}

$installedValidationText = Get-Content -LiteralPath (Join-Path $root "tools/validate-installed-desktop-runtime.ps1") -Raw
foreach ($needle in @(
        "--require-sandbox-authoring-review", "sandbox_authoring_review_status", "sandbox_authoring_review_ready",
        "sandbox_authoring_review_tile_definition_rows", "sandbox_authoring_review_palette_brush_rows",
        "sandbox_authoring_review_chunk_template_rows", "sandbox_authoring_review_procedural_seed_rows",
        "sandbox_authoring_review_changed_files", "sandbox_authoring_review_tilemap_package_changed_files",
        "sandbox_authoring_review_package_dependency_edges", "sandbox_authoring_review_preview_hash",
        "sandbox_authoring_review_external_image_decoding_invoked",
        "sandbox_authoring_review_external_download_invoked", "sandbox_authoring_review_importer_plugin_invoked",
        "sandbox_authoring_review_package_apply_invoked", "sandbox_authoring_review_diagnostics"
    )) {
    if (-not $installedValidationText.Contains($needle)) {
        Write-Error "tools/validate-installed-desktop-runtime.ps1 missing sandbox authoring review validation field: $needle"
    }
}
