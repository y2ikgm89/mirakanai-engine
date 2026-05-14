#requires -Version 7.0
#requires -PSEdition Core

# Shared helpers and section dispatcher for check-ai-integration.ps1.

function Resolve-RequiredAgentPath($relativePath) {
    if ([System.IO.Path]::IsPathRooted($relativePath)) {
        $path = $relativePath
    }
    else {
        $path = Join-Path $root $relativePath
    }
    if (-not (Test-Path -LiteralPath $path)) {
        Write-Error "Missing required AI integration file: $relativePath"
    }
    return $path
}

function Get-AgentSurfaceText([Parameter(Mandatory)][string]$relativePath) {
    $path = Resolve-RequiredAgentPath $relativePath
    $cacheKey = [System.IO.Path]::GetFullPath($path)
    if ($script:agentSurfaceTextCache.ContainsKey($cacheKey)) {
        return $script:agentSurfaceTextCache[$cacheKey]
    }

    $parts = [System.Collections.Generic.List[string]]::new()
    $parts.Add((Get-Content -LiteralPath $path -Raw))

    $referenceRoot = Join-Path (Split-Path -Parent $path) "references"
    if (Test-Path -LiteralPath $referenceRoot) {
        $leafName = Split-Path -Leaf $path
        if ($leafName -eq "SKILL.md") {
            $referenceFiles = Get-ChildItem -LiteralPath $referenceRoot -Filter "*.md" -File | Sort-Object FullName
        }
        else {
            $baseName = [System.IO.Path]::GetFileNameWithoutExtension($path)
            $specificReference = Join-Path $referenceRoot "$baseName.md"
            $referenceFiles = @()
            if (Test-Path -LiteralPath $specificReference) {
                $referenceFiles = @(Get-Item -LiteralPath $specificReference)
            }
        }

        foreach ($referenceFile in $referenceFiles) {
            $parts.Add((Get-Content -LiteralPath $referenceFile.FullName -Raw))
        }
    }

    $text = $parts -join "`n`n"
    $script:agentSurfaceTextCache[$cacheKey] = $text
    return $text
}

function Assert-SkillFrontmatter($skillFile) {
    $head = (Get-Content -LiteralPath $skillFile -TotalCount 8) -join "`n"
    if ($head -notmatch "---\s*\nname:\s*[a-z0-9-]+" -or $head -notmatch "description:\s*.+") {
        Write-Error "Skill frontmatter must include lowercase name and description: $skillFile"
    }
}

function Assert-ClaudeAgentFrontmatter($agentFile) {
    $head = (Get-Content -LiteralPath $agentFile -TotalCount 8) -join "`n"
    if ($head -notmatch "---\s*\nname:\s*[a-z0-9-]+" -or $head -notmatch "description:\s*.+") {
        Write-Error "Claude agent frontmatter must include lowercase name and description: $agentFile"
    }
}

function Assert-CodexReadOnlyAgent($relativePath) {
    $path = Resolve-RequiredAgentPath $relativePath
    $content = Get-Content -LiteralPath $path -Raw
    if ($content -notmatch '(?m)^sandbox_mode\s*=\s*"read-only"\s*$') {
        Write-Error "Read-only Codex agent must declare sandbox_mode = `"read-only`": $relativePath"
    }
}

function Assert-ContainsText($text, $needle, $label) {
    if (-not $text.Contains($needle)) {
        Write-Error "$label did not contain expected text: $needle"
    }
}

function Assert-DoesNotContainText($text, $needle, $label) {
    if ($text.Contains($needle)) {
        Write-Error "$label contained forbidden text: $needle"
    }
}

function Assert-MatchesText($text, $pattern, $label) {
    if (-not [System.Text.RegularExpressions.Regex]::IsMatch($text, $pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)) {
        Write-Error "$label did not match expected pattern: $pattern"
    }
}

function Assert-JsonProperty($object, [string[]]$properties, $label) {
    foreach ($property in $properties) {
        if (-not $object.PSObject.Properties.Name.Contains($property)) {
            Write-Error "$label missing required property: $property"
        }
    }
}

function Assert-NoGameSourceRawAssetIdFromName {
    $rawAssetIdMatches = @()
    $gamesRoot = Resolve-RequiredAgentPath "games"
    foreach ($sourceFile in Get-ChildItem -LiteralPath $gamesRoot -Filter "*.cpp" -Recurse -File) {
        foreach ($match in Select-String -LiteralPath $sourceFile.FullName -Pattern "AssetId::from_name(" -SimpleMatch) {
            $rawAssetIdMatches += "$(Get-RelativeRepoPath $match.Path):$($match.LineNumber)"
        }
    }

    $newGameScript = Resolve-RequiredAgentPath "tools/new-game.ps1"
    foreach ($match in Select-String -LiteralPath $newGameScript -Pattern "AssetId::from_name(" -SimpleMatch) {
        $rawAssetIdMatches += "$(Get-RelativeRepoPath $match.Path):$($match.LineNumber)"
    }

    if ($rawAssetIdMatches.Count -gt 0) {
        Write-Error "Game sources and tools/new-game.ps1 must derive asset references from AssetKeyV2 via asset_id_from_key_v2 instead of AssetId::from_name: $($rawAssetIdMatches -join ', ')"
    }
}

function Get-ActiveChildProductionPlan {
    $masterPlanPath = "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md"
    $plansRoot = Resolve-RequiredAgentPath "docs/superpowers/plans"
    $activePlans = @()

    Get-ChildItem -LiteralPath $plansRoot -Filter "2026-*.md" -File | ForEach-Object {
        $relativePath = Get-RelativeRepoPath $_.FullName
        if ($relativePath -eq $masterPlanPath) {
            return
        }

        $text = Get-Content -LiteralPath $_.FullName -Raw
        if ($text.Contains("**Status:** Active.")) {
            $planId = ""
            $planIdMatch = [regex]::Match($text, '\*\*Plan ID:\*\*\s*`([^`]+)`')
            if ($planIdMatch.Success) {
                $planId = $planIdMatch.Groups[1].Value
            }
            $activePlans += [pscustomobject]@{
                path = $relativePath
                fileName = $_.Name
                planId = $planId
            }
        }
    }

    return @($activePlans)
}

function Assert-ActiveProductionPlanDrift($productionLoop) {
    $masterPlanPath = "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md"
    $planRegistryPath = "docs/superpowers/plans/README.md"
    $planRegistryText = Get-AgentSurfaceText $planRegistryPath
    $activeSliceRow = [regex]::Match($planRegistryText, '(?m)^\| Active slice \(`currentActivePlan`\) \|.*$')
    if (-not $activeSliceRow.Success) {
        Write-Error "$planRegistryPath must contain an Active slice currentActivePlan row"
    }

    $activeChildPlans = @(Get-ActiveChildProductionPlan)
    if ($activeChildPlans.Count -gt 1) {
        Write-Error "Only one active child production plan is allowed: $(@($activeChildPlans | ForEach-Object { $_.path }) -join ', ')"
    }

    if ($activeChildPlans.Count -eq 1) {
        $activePlan = $activeChildPlans[0]
        if ($productionLoop.currentActivePlan -ne $activePlan.path) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop.currentActivePlan must match active child plan: $($activePlan.path)"
        }
        if (-not $activeSliceRow.Value.Contains($activePlan.fileName) -and
            ([string]::IsNullOrWhiteSpace($activePlan.planId) -or -not $activeSliceRow.Value.Contains($activePlan.planId))) {
            Write-Error "$planRegistryPath Active slice row must mention active child plan id or path: $($activePlan.path)"
        }
        return
    }

    if ($productionLoop.currentActivePlan -ne $masterPlanPath) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop.currentActivePlan must be the master plan when no child plan is active"
    }
    if ($productionLoop.recommendedNextPlan.id -ne "next-production-gap-selection") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop.recommendedNextPlan.id must be next-production-gap-selection when no child plan is active"
    }
}

function Assert-NewGameFailure($arguments, $label) {
    try {
        & (Join-Path $PSScriptRoot "new-game.ps1") @arguments | Out-Null
    } catch {
        return
    }
    Write-Error "$label should have failed."
}

function Assert-RegisterRuntimePackageFileFailure($arguments, $label) {
    try {
        & (Join-Path $PSScriptRoot "register-runtime-package-files.ps1") @arguments | Out-Null
    } catch {
        return
    }
    Write-Error "$label should have failed."
}

function Assert-RuntimeSceneValidationTarget($manifest, [string]$label, [string]$id, [string]$packageIndexPath, [string]$sceneAssetKey) {
    if (-not $manifest.PSObject.Properties.Name.Contains("runtimeSceneValidationTargets")) {
        Write-Error "$label must declare runtimeSceneValidationTargets"
    }
    $targets = @($manifest.runtimeSceneValidationTargets | Where-Object { $_.id -eq $id })
    if ($targets.Count -ne 1) {
        Write-Error "$label runtimeSceneValidationTargets must contain exactly one '$id' row"
    }
    if ($targets[0].packageIndexPath -ne $packageIndexPath) {
        Write-Error "$label runtimeSceneValidationTargets '$id' packageIndexPath must be $packageIndexPath"
    }
    if ($targets[0].sceneAssetKey -ne $sceneAssetKey) {
        Write-Error "$label runtimeSceneValidationTargets '$id' sceneAssetKey must be $sceneAssetKey"
    }
    if ($targets[0].validateAssetReferences -ne $true) {
        Write-Error "$label runtimeSceneValidationTargets '$id' must validate asset references"
    }
    if ($targets[0].requireUniqueNodeNames -ne $true) {
        Write-Error "$label runtimeSceneValidationTargets '$id' must require unique node names"
    }
    if ($targets[0].PSObject.Properties.Name.Contains("contentRoot")) {
        Write-Error "$label runtimeSceneValidationTargets '$id' must omit contentRoot when package index entries already include runtime-relative paths"
    }
    foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourceScenePath", "sourceAssetPath", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice")) {
        if ($targets[0].PSObject.Properties.Name.Contains($forbiddenField)) {
            Write-Error "$label runtimeSceneValidationTargets '$id' must not expose unsupported field: $forbiddenField"
        }
    }
}

function Assert-MaterialShaderAuthoringTarget($manifest, [string]$label, [string]$id, [string]$sourceMaterialPath, [string]$runtimeMaterialPath, [string]$packageIndexPath) {
    if (-not $manifest.PSObject.Properties.Name.Contains("materialShaderAuthoringTargets")) {
        Write-Error "$label must declare materialShaderAuthoringTargets"
    }
    $targets = @($manifest.materialShaderAuthoringTargets | Where-Object { $_.id -eq $id })
    if ($targets.Count -ne 1) {
        Write-Error "$label materialShaderAuthoringTargets must contain exactly one '$id' row"
    }
    if ($targets[0].sourceMaterialPath -ne $sourceMaterialPath) {
        Write-Error "$label materialShaderAuthoringTargets '$id' sourceMaterialPath must be $sourceMaterialPath"
    }
    if ($targets[0].runtimeMaterialPath -ne $runtimeMaterialPath) {
        Write-Error "$label materialShaderAuthoringTargets '$id' runtimeMaterialPath must be $runtimeMaterialPath"
    }
    if ($targets[0].packageIndexPath -ne $packageIndexPath) {
        Write-Error "$label materialShaderAuthoringTargets '$id' packageIndexPath must be $packageIndexPath"
    }
    foreach ($shaderSourcePath in @("shaders/runtime_scene.hlsl", "shaders/runtime_postprocess.hlsl")) {
        if (@($targets[0].shaderSourcePaths) -notcontains $shaderSourcePath) {
            Write-Error "$label materialShaderAuthoringTargets '$id' shaderSourcePaths missing $shaderSourcePath"
        }
        if (@($manifest.runtimePackageFiles) -contains $shaderSourcePath) {
            Write-Error "$label materialShaderAuthoringTargets '$id' shader source must not be in runtimePackageFiles: $shaderSourcePath"
        }
    }
    if (@($manifest.runtimePackageFiles) -contains $sourceMaterialPath) {
        Write-Error "$label materialShaderAuthoringTargets '$id' sourceMaterialPath must not be in runtimePackageFiles"
    }
    foreach ($runtimeFile in @($runtimeMaterialPath, $packageIndexPath)) {
        if (@($manifest.runtimePackageFiles) -notcontains $runtimeFile) {
            Write-Error "$label materialShaderAuthoringTargets '$id' runtime file must be in runtimePackageFiles: $runtimeFile"
        }
    }
    foreach ($artifactPath in @($targets[0].d3d12ShaderArtifactPaths)) {
        if (-not ([string]$artifactPath).EndsWith(".dxil")) {
            Write-Error "$label materialShaderAuthoringTargets '$id' D3D12 artifact must end in .dxil: $artifactPath"
        }
    }
    foreach ($artifactPath in @($targets[0].vulkanShaderArtifactPaths)) {
        if (-not ([string]$artifactPath).EndsWith(".spv")) {
            Write-Error "$label materialShaderAuthoringTargets '$id' Vulkan artifact must end in .spv: $artifactPath"
        }
    }
    if ($targets[0].validateMaterialTextures -ne $true) {
        Write-Error "$label materialShaderAuthoringTargets '$id' must validate material textures"
    }
    if ($targets[0].validateShaderArtifacts -ne $true) {
        Write-Error "$label materialShaderAuthoringTargets '$id' must validate shader artifacts"
    }
    foreach ($forbiddenField in @("command", "shell", "argv", "shaderGraph", "materialGraph", "liveShaderGeneration", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice")) {
        if ($targets[0].PSObject.Properties.Name.Contains($forbiddenField)) {
            Write-Error "$label materialShaderAuthoringTargets '$id' must not expose unsupported field: $forbiddenField"
        }
    }
}

function Assert-AtlasTilemapAuthoringTarget($manifest, [string]$label, [string]$id, [string]$packageIndexPath, [string]$tilemapPath, [string]$atlasTexturePath) {
    if (-not $manifest.PSObject.Properties.Name.Contains("atlasTilemapAuthoringTargets")) {
        Write-Error "$label must declare atlasTilemapAuthoringTargets"
    }
    $targets = @($manifest.atlasTilemapAuthoringTargets | Where-Object { $_.id -eq $id })
    if ($targets.Count -ne 1) {
        Write-Error "$label atlasTilemapAuthoringTargets must contain exactly one '$id' row"
    }
    if ($targets[0].packageIndexPath -ne $packageIndexPath) {
        Write-Error "$label atlasTilemapAuthoringTargets '$id' packageIndexPath must be $packageIndexPath"
    }
    if ($targets[0].tilemapPath -ne $tilemapPath) {
        Write-Error "$label atlasTilemapAuthoringTargets '$id' tilemapPath must be $tilemapPath"
    }
    if ($targets[0].atlasTexturePath -ne $atlasTexturePath) {
        Write-Error "$label atlasTilemapAuthoringTargets '$id' atlasTexturePath must be $atlasTexturePath"
    }
    foreach ($runtimeFile in @($packageIndexPath, $tilemapPath, $atlasTexturePath)) {
        if (@($manifest.runtimePackageFiles) -notcontains $runtimeFile) {
            Write-Error "$label atlasTilemapAuthoringTargets '$id' runtime file must be in runtimePackageFiles: $runtimeFile"
        }
    }
    if ($targets[0].mode -ne "deterministic-package-data") {
        Write-Error "$label atlasTilemapAuthoringTargets '$id' mode must be deterministic-package-data"
    }
    if ($targets[0].sourceDecoding -ne "unsupported") {
        Write-Error "$label atlasTilemapAuthoringTargets '$id' sourceDecoding must remain unsupported"
    }
    if ($targets[0].atlasPacking -ne "unsupported") {
        Write-Error "$label atlasTilemapAuthoringTargets '$id' atlasPacking must remain unsupported"
    }
    if ($targets[0].nativeGpuSpriteBatching -ne "unsupported") {
        Write-Error "$label atlasTilemapAuthoringTargets '$id' nativeGpuSpriteBatching must remain unsupported"
    }
    foreach ($recipeId in @($targets[0].preflightRecipeIds)) {
        if (@($manifest.validationRecipes | ForEach-Object { $_.name }) -notcontains $recipeId) {
            Write-Error "$label atlasTilemapAuthoringTargets '$id' preflightRecipeIds must reference validationRecipes: $recipeId"
        }
    }
    foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourceImagePath", "sourceTilemapPath", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice", "runtimeImageDecoding", "productionAtlasPacking", "tilemapEditorUX", "productionSpriteBatching", "nativeGpuOutput", "packageStreamingReady", "rendererQualityClaim", "metalReady")) {
        if ($targets[0].PSObject.Properties.Name.Contains($forbiddenField)) {
            Write-Error "$label atlasTilemapAuthoringTargets '$id' must not expose unsupported field: $forbiddenField"
        }
    }
}

function Assert-PackageStreamingResidencyTarget($manifest, [string]$label, [string]$id, [string]$packageIndexPath, [string]$runtimeSceneValidationTargetId) {
    if (-not $manifest.PSObject.Properties.Name.Contains("packageStreamingResidencyTargets")) {
        Write-Error "$label must declare packageStreamingResidencyTargets"
    }
    $targets = @($manifest.packageStreamingResidencyTargets | Where-Object { $_.id -eq $id })
    if ($targets.Count -ne 1) {
        Write-Error "$label packageStreamingResidencyTargets must contain exactly one '$id' row"
    }
    if ($targets[0].packageIndexPath -ne $packageIndexPath) {
        Write-Error "$label packageStreamingResidencyTargets '$id' packageIndexPath must be $packageIndexPath"
    }
    if ($targets[0].runtimeSceneValidationTargetId -ne $runtimeSceneValidationTargetId) {
        Write-Error "$label packageStreamingResidencyTargets '$id' runtimeSceneValidationTargetId must be $runtimeSceneValidationTargetId"
    }
    if ($targets[0].mode -ne "host-gated-safe-point") {
        Write-Error "$label packageStreamingResidencyTargets '$id' mode must be host-gated-safe-point"
    }
    if (-not ($targets[0].residentBudgetBytes -is [int] -or $targets[0].residentBudgetBytes -is [long]) -or
        [int64]$targets[0].residentBudgetBytes -lt 1) {
        Write-Error "$label packageStreamingResidencyTargets '$id' residentBudgetBytes must be positive"
    }
    if ($targets[0].safePointRequired -ne $true) {
        Write-Error "$label packageStreamingResidencyTargets '$id' safePointRequired must be true"
    }
    if (@($manifest.runtimePackageFiles) -notcontains $packageIndexPath) {
        Write-Error "$label packageStreamingResidencyTargets '$id' packageIndexPath must be in runtimePackageFiles"
    }
    if (@($manifest.runtimeSceneValidationTargets | Where-Object { $_.id -eq $runtimeSceneValidationTargetId }).Count -ne 1) {
        Write-Error "$label packageStreamingResidencyTargets '$id' must reference runtimeSceneValidationTargets"
    }
    if ($targets[0].PSObject.Properties.Name.Contains("contentRoot")) {
        Write-Error "$label packageStreamingResidencyTargets '$id' must omit contentRoot when package index entries already include runtime-relative paths"
    }
    foreach ($recipeId in @($targets[0].preflightRecipeIds)) {
        if (@($manifest.validationRecipes | ForEach-Object { $_.name }) -notcontains $recipeId) {
            Write-Error "$label packageStreamingResidencyTargets '$id' preflightRecipeIds must reference validationRecipes: $recipeId"
        }
    }
    foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourcePackagePath", "sourceAssetPath", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice", "asyncEviction", "evictionCommand", "streamingThread", "backgroundStreaming", "executionCommand", "allocatorHandle", "allocatorBudgetEnforcement", "gpuBudgetEnforcement", "rendererQualityClaim", "metalReady")) {
        if ($targets[0].PSObject.Properties.Name.Contains($forbiddenField)) {
            Write-Error "$label packageStreamingResidencyTargets '$id' must not expose unsupported field: $forbiddenField"
        }
    }
}

function Assert-PrefabScenePackageAuthoringTarget($manifest, [string]$label, [string]$id, [string]$packageIndexPath, [string]$outputScenePath, [string]$runtimeSceneValidationTargetId) {
    if (-not $manifest.PSObject.Properties.Name.Contains("prefabScenePackageAuthoringTargets")) {
        Write-Error "$label must declare prefabScenePackageAuthoringTargets"
    }
    $targets = @($manifest.prefabScenePackageAuthoringTargets | Where-Object { $_.id -eq $id })
    if ($targets.Count -ne 1) {
        Write-Error "$label prefabScenePackageAuthoringTargets must contain exactly one '$id' row"
    }
    Assert-JsonProperty $targets[0] @("id", "mode", "sceneAuthoringPath", "prefabAuthoringPath", "sourceRegistryPath", "packageIndexPath", "outputScenePath", "sceneAssetKey", "runtimeSceneValidationTargetId", "authoringCommandRows", "selectedSourceAssetKeys", "sourceCookMode", "sceneMigration", "runtimeSceneValidation", "hostGatedSmokeRecipeIds", "broadImporterExecution", "broadDependencyCooking", "runtimeSourceParsing", "materialGraph", "shaderGraph", "liveShaderGeneration", "skeletalAnimation", "gpuSkinning", "publicNativeRhiHandles", "metalReadiness", "rendererQuality") "$label prefabScenePackageAuthoringTargets '$id'"
    if ($targets[0].packageIndexPath -ne $packageIndexPath) {
        Write-Error "$label prefabScenePackageAuthoringTargets '$id' packageIndexPath must be $packageIndexPath"
    }
    if ($targets[0].outputScenePath -ne $outputScenePath) {
        Write-Error "$label prefabScenePackageAuthoringTargets '$id' outputScenePath must be $outputScenePath"
    }
    if ($targets[0].runtimeSceneValidationTargetId -ne $runtimeSceneValidationTargetId) {
        Write-Error "$label prefabScenePackageAuthoringTargets '$id' runtimeSceneValidationTargetId must be $runtimeSceneValidationTargetId"
    }
    if ($targets[0].mode -ne "deterministic-3d-prefab-scene-package-data") {
        Write-Error "$label prefabScenePackageAuthoringTargets '$id' mode must be deterministic-3d-prefab-scene-package-data"
    }
    if ($targets[0].sourceCookMode -ne "selected-source-registry-rows") {
        Write-Error "$label prefabScenePackageAuthoringTargets '$id' sourceCookMode must be selected-source-registry-rows"
    }
    if ($targets[0].sceneMigration -ne "migrate-scene-v2-runtime-package") {
        Write-Error "$label prefabScenePackageAuthoringTargets '$id' sceneMigration must be migrate-scene-v2-runtime-package"
    }
    if ($targets[0].runtimeSceneValidation -ne "validate-runtime-scene-package") {
        Write-Error "$label prefabScenePackageAuthoringTargets '$id' runtimeSceneValidation must be validate-runtime-scene-package"
    }
    foreach ($runtimeFile in @($packageIndexPath, $outputScenePath)) {
        if (@($manifest.runtimePackageFiles) -notcontains $runtimeFile) {
            Write-Error "$label prefabScenePackageAuthoringTargets '$id' runtime file must be in runtimePackageFiles: $runtimeFile"
        }
    }
    foreach ($sourcePath in @($targets[0].sceneAuthoringPath, $targets[0].prefabAuthoringPath, $targets[0].sourceRegistryPath)) {
        if (@($manifest.runtimePackageFiles) -contains $sourcePath) {
            Write-Error "$label prefabScenePackageAuthoringTargets '$id' source authoring file must not be in runtimePackageFiles: $sourcePath"
        }
    }
    $expectedOperations = @("create-scene", "add-scene-node", "add-or-update-component", "create-prefab", "instantiate-prefab")
    $actualOperations = @($targets[0].authoringCommandRows | ForEach-Object { $_.operation })
    if (($actualOperations -join "|") -ne ($expectedOperations -join "|")) {
        Write-Error "$label prefabScenePackageAuthoringTargets '$id' authoringCommandRows must be: $($expectedOperations -join ', ')"
    }
    foreach ($row in @($targets[0].authoringCommandRows)) {
        $expectedSurface = if ($row.operation -eq "create-prefab") { "GameEngine.Prefab.v2" } else { "GameEngine.Scene.v2" }
        if ($row.surface -ne $expectedSurface) {
            Write-Error "$label prefabScenePackageAuthoringTargets '$id' operation '$($row.operation)' must use $expectedSurface"
        }
    }
    $sceneTarget = @($manifest.runtimeSceneValidationTargets | Where-Object { $_.id -eq $runtimeSceneValidationTargetId })
    if ($sceneTarget.Count -ne 1) {
        Write-Error "$label prefabScenePackageAuthoringTargets '$id' must reference one runtimeSceneValidationTargets row: $runtimeSceneValidationTargetId"
    } else {
        if ($sceneTarget[0].packageIndexPath -ne $targets[0].packageIndexPath) {
            Write-Error "$label prefabScenePackageAuthoringTargets '$id' packageIndexPath must match runtimeSceneValidationTargets"
        }
        if ($sceneTarget[0].sceneAssetKey -ne $targets[0].sceneAssetKey) {
            Write-Error "$label prefabScenePackageAuthoringTargets '$id' sceneAssetKey must match runtimeSceneValidationTargets"
        }
    }
    foreach ($assetKey in @($targets[0].selectedSourceAssetKeys)) {
        if ([string]::IsNullOrWhiteSpace($assetKey) -or ([string]$assetKey).Contains(" ")) {
            Write-Error "$label prefabScenePackageAuthoringTargets '$id' selectedSourceAssetKeys must be safe AssetKeyV2 values: $assetKey"
        }
    }
    foreach ($recipeId in @($targets[0].hostGatedSmokeRecipeIds)) {
        if (@($manifest.validationRecipes | ForEach-Object { $_.name }) -notcontains $recipeId) {
            Write-Error "$label prefabScenePackageAuthoringTargets '$id' hostGatedSmokeRecipeIds must reference validationRecipes: $recipeId"
        }
    }
    foreach ($field in @("broadImporterExecution", "broadDependencyCooking", "runtimeSourceParsing", "materialGraph", "shaderGraph", "liveShaderGeneration", "skeletalAnimation", "gpuSkinning", "publicNativeRhiHandles", "rendererQuality")) {
        if ($targets[0].$field -ne "unsupported") {
            Write-Error "$label prefabScenePackageAuthoringTargets '$id' $field must remain unsupported"
        }
    }
    if ($targets[0].metalReadiness -ne "host-gated") {
        Write-Error "$label prefabScenePackageAuthoringTargets '$id' metalReadiness must remain host-gated"
    }
    foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourcePackagePath", "runtimeSourceFile", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice", "broadImporterReady", "broadCookingReady", "runtimeSourceParsingReady", "materialGraphReady", "shaderGraphReady", "liveShaderGenerationReady", "skeletalAnimationReady", "gpuSkinningReady", "rendererQualityClaim", "metalReady")) {
        if ($targets[0].PSObject.Properties.Name.Contains($forbiddenField)) {
            Write-Error "$label prefabScenePackageAuthoringTargets '$id' must not expose unsupported field: $forbiddenField"
        }
    }
}

function Assert-RegisteredSourceAssetCookTarget($manifest, [string]$label, [string]$id, [string]$prefabScenePackageAuthoringTargetId, [string]$sourceRegistryPath, [string]$packageIndexPath, [string[]]$selectedAssetKeys, [string]$dependencyExpansion, [string]$dependencyCooking) {
    if (-not $manifest.PSObject.Properties.Name.Contains("registeredSourceAssetCookTargets")) {
        Write-Error "$label must declare registeredSourceAssetCookTargets"
    }
    $targets = @($manifest.registeredSourceAssetCookTargets | Where-Object { $_.id -eq $id })
    if ($targets.Count -ne 1) {
        Write-Error "$label registeredSourceAssetCookTargets must contain exactly one '$id' row"
    }
    Assert-JsonProperty $targets[0] @("id", "mode", "cookCommandId", "prefabScenePackageAuthoringTargetId", "sourceRegistryPath", "packageIndexPath", "selectedAssetKeys", "dependencyExpansion", "dependencyCooking", "externalImporterExecution", "rendererRhiResidency", "packageStreaming", "materialGraph", "shaderGraph", "liveShaderGeneration", "editorProductization", "metalReadiness", "publicNativeRhiHandles", "generalProductionRendererQuality", "arbitraryShell", "freeFormEdit") "$label registeredSourceAssetCookTargets '$id'"
    if ($targets[0].prefabScenePackageAuthoringTargetId -ne $prefabScenePackageAuthoringTargetId) {
        Write-Error "$label registeredSourceAssetCookTargets '$id' prefabScenePackageAuthoringTargetId must be $prefabScenePackageAuthoringTargetId"
    }
    if ($targets[0].sourceRegistryPath -ne $sourceRegistryPath) {
        Write-Error "$label registeredSourceAssetCookTargets '$id' sourceRegistryPath must be $sourceRegistryPath"
    }
    if ($targets[0].packageIndexPath -ne $packageIndexPath) {
        Write-Error "$label registeredSourceAssetCookTargets '$id' packageIndexPath must be $packageIndexPath"
    }
    if ($targets[0].mode -ne "descriptor-only-cook-registered-source-assets") {
        Write-Error "$label registeredSourceAssetCookTargets '$id' mode must be descriptor-only-cook-registered-source-assets"
    }
    if ($targets[0].cookCommandId -ne "cook-registered-source-assets") {
        Write-Error "$label registeredSourceAssetCookTargets '$id' cookCommandId must be cook-registered-source-assets"
    }
    if ($targets[0].dependencyExpansion -ne $dependencyExpansion) {
        Write-Error "$label registeredSourceAssetCookTargets '$id' dependencyExpansion must be $dependencyExpansion"
    }
    if ($targets[0].dependencyCooking -ne $dependencyCooking) {
        Write-Error "$label registeredSourceAssetCookTargets '$id' dependencyCooking must be $dependencyCooking"
    }
    $prefabTargets = @($manifest.prefabScenePackageAuthoringTargets | Where-Object { $_.id -eq $prefabScenePackageAuthoringTargetId })
    if ($prefabTargets.Count -ne 1) {
        Write-Error "$label registeredSourceAssetCookTargets '$id' must reference prefabScenePackageAuthoringTargets id $prefabScenePackageAuthoringTargetId"
    }
    if ($prefabTargets[0].sourceRegistryPath -ne $sourceRegistryPath) {
        Write-Error "$label registeredSourceAssetCookTargets '$id' sourceRegistryPath must match prefabScenePackageAuthoringTargets '$prefabScenePackageAuthoringTargetId'"
    }
    if ($prefabTargets[0].packageIndexPath -ne $packageIndexPath) {
        Write-Error "$label registeredSourceAssetCookTargets '$id' packageIndexPath must match prefabScenePackageAuthoringTargets '$prefabScenePackageAuthoringTargetId'"
    }
    $actualKeys = @($targets[0].selectedAssetKeys | ForEach-Object { [string]$_ }) | Sort-Object
    $expectedKeys = @($selectedAssetKeys | ForEach-Object { [string]$_ }) | Sort-Object
    if (($actualKeys -join "|") -ne ($expectedKeys -join "|")) {
        Write-Error "$label registeredSourceAssetCookTargets '$id' selectedAssetKeys must be: $($expectedKeys -join ', ')"
    }
    foreach ($field in @("externalImporterExecution", "rendererRhiResidency", "packageStreaming", "materialGraph", "shaderGraph", "liveShaderGeneration", "editorProductization", "publicNativeRhiHandles", "generalProductionRendererQuality", "arbitraryShell", "freeFormEdit")) {
        if ($targets[0].$field -ne "unsupported") {
            Write-Error "$label registeredSourceAssetCookTargets '$id' $field must remain unsupported"
        }
    }
    if ($targets[0].metalReadiness -ne "host-gated") {
        Write-Error "$label registeredSourceAssetCookTargets '$id' metalReadiness must remain host-gated"
    }
    foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourcePackagePath", "runtimeSourceFile", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice", "broadImporterReady", "broadCookingReady", "rendererQualityClaim", "metalReady")) {
        if ($targets[0].PSObject.Properties.Name.Contains($forbiddenField)) {
            Write-Error "$label registeredSourceAssetCookTargets '$id' must not expose unsupported field: $forbiddenField"
        }
    }
}

function New-ScaffoldCheckRoot {
    [CmdletBinding(SupportsShouldProcess = $true)]
    param()

    $base = Join-Path $root "out/new-game-scaffold-checks"
    $rootPath = Join-Path $base ([System.Guid]::NewGuid().ToString("N"))
    if (-not $PSCmdlet.ShouldProcess($rootPath, "Create scaffold check root")) {
        return $rootPath
    }
    New-Item -ItemType Directory -Path $base -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $rootPath "games") -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $rootPath "games/CMakeLists.txt") -Value "" -NoNewline
    return $rootPath
}

function Remove-ScaffoldCheckRoot {
    [CmdletBinding(SupportsShouldProcess = $true)]
    param([string]$rootPath)

    if ([string]::IsNullOrWhiteSpace($rootPath) -or -not (Test-Path -LiteralPath $rootPath)) {
        return
    }
    $fullRoot = [System.IO.Path]::GetFullPath($rootPath).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
    $allowedRoot = [System.IO.Path]::GetFullPath((Join-Path $root "out/new-game-scaffold-checks")).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
    $allowedChildPrefix = $allowedRoot + [System.IO.Path]::DirectorySeparatorChar
    if ($fullRoot.Equals($allowedRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
        -not $fullRoot.StartsWith($allowedChildPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "Refusing to remove scaffold check root outside repository out directory: $fullRoot"
    }
    if ($PSCmdlet.ShouldProcess($fullRoot, "Remove scaffold check root")) {
        Remove-Item -LiteralPath $fullRoot -Recurse -Force
    }
}

function Assert-ClaudeReadOnlyAgent($relativePath) {
    $path = Resolve-RequiredAgentPath $relativePath
    $head = (Get-Content -LiteralPath $path -TotalCount 8) -join "`n"
    if ($head -notmatch '(?m)^tools:\s*Read,\s*Grep,\s*Glob,\s*LS\s*$') {
        Write-Error "Read-only Claude agent must declare tools: Read, Grep, Glob, LS: $relativePath"
    }
}
function Get-CheckAiIntegrationSectionFiles {
    $ledger = Get-StaticContractLedgerById -Id "check-ai-integration"
    return @($ledger.SectionFiles)
}

function Invoke-CheckAiIntegrationSections {
    foreach ($sectionFile in Get-CheckAiIntegrationSectionFiles) {
        . (Join-Path $PSScriptRoot $sectionFile)
    }
}
