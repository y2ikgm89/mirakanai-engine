#requires -Version 7.0
#requires -PSEdition Core

# Chapter 3 for check-json-contracts.ps1 static contracts.

foreach ($commandId in $scenePrefabAuthoringCommandIds) {
    $scenePrefabCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq $commandId })
    if ($scenePrefabCommand.Count -ne 1 -or $scenePrefabCommand[0].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop must expose one ready Scene/Prefab v2 authoring command surface: $commandId"
    } else {
        $scenePrefabModes = @{}
        foreach ($mode in @($scenePrefabCommand[0].requestModes)) {
            $scenePrefabModes[$mode.id] = $mode
        }
        if (-not $scenePrefabModes.ContainsKey("dry-run") -or $scenePrefabModes["dry-run"].status -ne "ready" -or
            -not $scenePrefabModes.ContainsKey("apply") -or $scenePrefabModes["apply"].status -ne "ready") {
            Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' must keep dry-run and apply ready"
        }
        foreach ($module in @("MK_scene", "MK_tools")) {
            if (@($scenePrefabCommand[0].requiredModules) -notcontains $module) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' missing required module: $module"
            }
        }
        foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
            if (@($scenePrefabCommand[0].resultShape.dryRunFields) -notcontains $field) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' dryRunFields missing: $field"
            }
        }
        foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
            if (@($scenePrefabCommand[0].resultShape.applyFields) -notcontains $field) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' applyFields missing: $field"
            }
        }
        $scenePrefabPolicyText = "$($scenePrefabCommand[0].summary) $($scenePrefabCommand[0].requestShape.pathPolicy) $($scenePrefabCommand[0].notes)"
        foreach ($needle in @(
                "GameEngine.Scene.v2",
                "GameEngine.Prefab.v2",
                "plan_scene_prefab_authoring",
                "apply_scene_prefab_authoring",
                "safe repository-relative",
                "does not evaluate arbitrary shell",
                "free-form edits are not supported",
                "Scene v2 runtime package migration",
                "editor productization",
                "nested prefab merge/resolution UX"
            )) {
            if (-not $scenePrefabPolicyText.Contains($needle)) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' must document reviewed helper/policy text: $needle"
            }
        }
    }
}
$sourceAssetCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "register-source-asset" })
if ($sourceAssetCommand.Count -ne 1 -or $sourceAssetCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready register-source-asset command surface"
} else {
    $sourceAssetModes = @{}
    foreach ($mode in @($sourceAssetCommand[0].requestModes)) {
        $sourceAssetModes[$mode.id] = $mode
    }
    if (-not $sourceAssetModes.ContainsKey("dry-run") -or $sourceAssetModes["dry-run"].status -ne "ready" -or
        -not $sourceAssetModes.ContainsKey("apply") -or $sourceAssetModes["apply"].status -ne "ready") {
        Write-Error "engine manifest register-source-asset must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_assets", "MK_tools")) {
        if (@($sourceAssetCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest register-source-asset missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "importMetadata", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($sourceAssetCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest register-source-asset dryRunFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("cookedOutputHint", "packageIndexPath", "backend", "shaderArtifactRequirements", "renderer", "rhi", "metal", "descriptor", "pipeline", "nativeHandle")) {
        if (@($sourceAssetCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($sourceAssetCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest register-source-asset must not expose package/renderer/native field: $forbiddenField"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($sourceAssetCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine manifest register-source-asset applyFields missing: $field"
        }
    }
    $sourceAssetPolicyText = "$($sourceAssetCommand[0].summary) $($sourceAssetCommand[0].requestShape.pathPolicy) $($sourceAssetCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.AssetIdentity.v2",
            "GameEngine.SourceAssetRegistry.v1",
            "plan_source_asset_registration",
            "apply_source_asset_registration",
            "safe repository-relative",
            "does not evaluate arbitrary shell",
            "free-form edits are not supported",
            "external importer execution is not supported",
            "does not write cooked artifacts",
            "does not update .geindex",
            "package cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles"
        )) {
        if (-not $sourceAssetPolicyText.Contains($needle)) {
            Write-Error "engine manifest register-source-asset must document reviewed helper/policy text: $needle"
        }
    }
    $sourceAssetHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/source_asset_registration_tool.hpp"
    $sourceAssetSourcePath = Join-Path $root "engine/tools/asset/source_asset_registration_tool.cpp"
    foreach ($requiredPath in @($sourceAssetHeaderPath, $sourceAssetSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "register-source-asset reviewed helper file is missing: $requiredPath"
        }
    }
    foreach ($helperPath in @($sourceAssetHeaderPath, $sourceAssetSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/tools/asset_import_tool.hpp",
                "mirakana/tools/asset_package_tool.hpp",
                "mirakana/assets/asset_package.hpp",
                "mirakana/renderer/",
                "mirakana/rhi/",
                "execute_asset_import_plan",
                "assemble_asset_cooked_package",
                "write_asset_cooked_package_index"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use package/import execution or renderer/RHI surface: $forbiddenText"
                }
            }
        }
    }
}
$registeredCookCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "cook-registered-source-assets" })
if ($registeredCookCommand.Count -ne 1 -or $registeredCookCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready cook-registered-source-assets command surface"
} else {
    $registeredCookModes = @{}
    foreach ($mode in @($registeredCookCommand[0].requestModes)) {
        $registeredCookModes[$mode.id] = $mode
    }
    if (-not $registeredCookModes.ContainsKey("dry-run") -or $registeredCookModes["dry-run"].status -ne "ready" -or
        -not $registeredCookModes.ContainsKey("apply") -or $registeredCookModes["apply"].status -ne "ready") {
        Write-Error "engine manifest cook-registered-source-assets must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_assets", "MK_tools")) {
        if (@($registeredCookCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest cook-registered-source-assets missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($registeredCookCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest cook-registered-source-assets dryRunFields missing: $field"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($registeredCookCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine manifest cook-registered-source-assets applyFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice")) {
        if (@($registeredCookCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($registeredCookCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest cook-registered-source-assets must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $registeredCookPolicyText = "$($registeredCookCommand[0].summary) $($registeredCookCommand[0].requestShape.pathPolicy) $($registeredCookCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.SourceAssetRegistry.v1",
            "explicitly selected",
            "plan_registered_source_asset_cook_package",
            "apply_registered_source_asset_cook_package",
            "build_asset_import_plan",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package",
            "safe repository-relative",
            "package-relative",
            "external importer execution is not supported",
            "broad dependency cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $registeredCookPolicyText.Contains($needle)) {
            Write-Error "engine manifest cook-registered-source-assets must document reviewed helper/policy text: $needle"
        }
    }
    $registeredCookHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/registered_source_asset_cook_package_tool.hpp"
    $registeredCookSourcePath = Join-Path $root "engine/tools/asset/registered_source_asset_cook_package_tool.cpp"
    foreach ($requiredPath in @($registeredCookHeaderPath, $registeredCookSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "cook-registered-source-assets reviewed helper file is missing: $requiredPath"
        }
    }
    if (Test-Path -LiteralPath $registeredCookSourcePath -PathType Leaf) {
        $helperText = Get-Content -LiteralPath $registeredCookSourcePath -Raw
        foreach ($requiredText in @(
            "mirakana/tools/asset_import_tool.hpp",
            "mirakana/tools/asset_package_tool.hpp",
            "build_asset_import_plan",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package"
        )) {
            if (-not $helperText.Contains($requiredText)) {
                Write-Error "$registeredCookSourcePath must reuse selected source asset import/package helper: $requiredText"
            }
        }
    }
    foreach ($helperPath in @($registeredCookHeaderPath, $registeredCookSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/renderer/",
                "mirakana/rhi/",
                "IRhiDevice",
                "ID3D12",
                "VkDevice",
                "MTLDevice"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use renderer/RHI/native surfaces: $forbiddenText"
                }
            }
        }
    }
}
$sceneMigrationCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "migrate-scene-v2-runtime-package" })
if ($sceneMigrationCommand.Count -ne 1 -or $sceneMigrationCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready migrate-scene-v2-runtime-package command surface"
} else {
    $sceneMigrationModes = @{}
    foreach ($mode in @($sceneMigrationCommand[0].requestModes)) {
        $sceneMigrationModes[$mode.id] = $mode
    }
    if (-not $sceneMigrationModes.ContainsKey("dry-run") -or $sceneMigrationModes["dry-run"].status -ne "ready" -or
        -not $sceneMigrationModes.ContainsKey("apply") -or $sceneMigrationModes["apply"].status -ne "ready") {
        Write-Error "engine manifest migrate-scene-v2-runtime-package must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_scene", "MK_assets", "MK_tools")) {
        if (@($sceneMigrationCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($sceneMigrationCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package dryRunFields missing: $field"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($sceneMigrationCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package applyFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice")) {
        if (@($sceneMigrationCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($sceneMigrationCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $sceneMigrationPolicyText = "$($sceneMigrationCommand[0].summary) $($sceneMigrationCommand[0].requestShape.pathPolicy) $($sceneMigrationCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.Scene.v2",
            "GameEngine.SourceAssetRegistry.v1",
            "GameEngine.Scene.v1",
            "plan_scene_v2_runtime_package_migration",
            "apply_scene_v2_runtime_package_migration",
            "plan_scene_package_update",
            "apply_scene_package_update",
            "safe repository-relative",
            "package-relative",
            "external importer execution is not supported",
            "package cooking",
            "dependent asset cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $sceneMigrationPolicyText.Contains($needle)) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package must document reviewed helper/policy text: $needle"
        }
    }
    $sceneMigrationHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/scene_v2_runtime_package_migration_tool.hpp"
    $sceneMigrationSourcePath = Join-Path $root "engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp"
    foreach ($requiredPath in @($sceneMigrationHeaderPath, $sceneMigrationSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "migrate-scene-v2-runtime-package reviewed helper file is missing: $requiredPath"
        }
    }
    foreach ($helperPath in @($sceneMigrationHeaderPath, $sceneMigrationSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/tools/asset_import_tool.hpp",
                "mirakana/tools/asset_package_tool.hpp",
                "mirakana/renderer/",
                "mirakana/rhi/",
                "execute_asset_import_plan",
                "assemble_asset_cooked_package",
                "write_asset_cooked_package_index",
                "IRhiDevice",
                "ID3D12",
                "VkDevice",
                "MTLDevice"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use importer/package execution or renderer/RHI/native surfaces: $forbiddenText"
                }
            }
        }
    }
    if (Test-Path -LiteralPath $sceneMigrationSourcePath -PathType Leaf) {
        $sceneMigrationSourceText = Get-Content -LiteralPath $sceneMigrationSourcePath -Raw
        foreach ($requiredCall in @(
                "plan_scene_package_update(",
                "apply_scene_package_update("
            )) {
            if (-not $sceneMigrationSourceText.Contains($requiredCall)) {
                Write-Error "migrate-scene-v2-runtime-package source must reuse existing scene package helper call: $requiredCall"
            }
        }
    }
}
$runtimeSceneValidationCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "validate-runtime-scene-package" })
if ($runtimeSceneValidationCommand.Count -ne 1 -or $runtimeSceneValidationCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready validate-runtime-scene-package command surface"
} else {
    $runtimeSceneValidationModes = @{}
    foreach ($mode in @($runtimeSceneValidationCommand[0].requestModes)) {
        $runtimeSceneValidationModes[$mode.id] = $mode
    }
    if (-not $runtimeSceneValidationModes.ContainsKey("dry-run") -or
        $runtimeSceneValidationModes["dry-run"].status -ne "ready" -or
        -not $runtimeSceneValidationModes.ContainsKey("execute") -or
        $runtimeSceneValidationModes["execute"].status -ne "ready") {
        Write-Error "engine manifest validate-runtime-scene-package must keep dry-run and execute ready"
    }
    if ($runtimeSceneValidationModes.ContainsKey("apply") -and $runtimeSceneValidationModes["apply"].status -eq "ready") {
        Write-Error "engine manifest validate-runtime-scene-package must remain non-mutating and must not expose ready apply"
    }
    foreach ($module in @("MK_runtime", "MK_runtime_scene", "MK_tools")) {
        if (@($runtimeSceneValidationCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest validate-runtime-scene-package missing required module: $module"
        }
    }
    foreach ($field in @("packageIndexPath", "sceneAssetKey")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.requiredFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package requestShape requiredFields missing: $field"
        }
    }
    foreach ($field in @("contentRoot", "validateAssetReferences", "requireUniqueNodeNames")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.optionalFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package requestShape optionalFields missing: $field"
        }
    }
    foreach ($field in @("packageSummary", "sceneAsset", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($runtimeSceneValidationCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package dryRunFields missing: $field"
        }
    }
    foreach ($field in @("packageSummary", "sceneSummary", "references", "packageRecordCount", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($runtimeSceneValidationCommand[0].resultShape.executeFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package executeFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice", "shaderArtifactRequirements")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($runtimeSceneValidationCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest validate-runtime-scene-package must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $runtimeSceneValidationPolicyText = "$($runtimeSceneValidationCommand[0].summary) $($runtimeSceneValidationCommand[0].requestShape.pathPolicy) $($runtimeSceneValidationCommand[0].notes)"
    foreach ($needle in @(
            "plan_runtime_scene_package_validation",
            "execute_runtime_scene_package_validation",
            "mirakana::runtime::load_runtime_asset_package",
            "mirakana::runtime_scene::instantiate_runtime_scene",
            "safe package-relative",
            "does not mutate",
            "runtime source parsing is not supported",
            "package cooking is not supported",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $runtimeSceneValidationPolicyText.Contains($needle)) {
            Write-Error "engine manifest validate-runtime-scene-package must document reviewed helper/policy text: $needle"
        }
    }
    $runtimeSceneValidationHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/runtime_scene_package_validation_tool.hpp"
    $runtimeSceneValidationSourcePath = Join-Path $root "engine/tools/scene/runtime_scene_package_validation_tool.cpp"
    foreach ($requiredPath in @($runtimeSceneValidationHeaderPath, $runtimeSceneValidationSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "validate-runtime-scene-package reviewed helper file is missing: $requiredPath"
        }
    }
    if (Test-Path -LiteralPath $runtimeSceneValidationSourcePath -PathType Leaf) {
        $helperText = Get-Content -LiteralPath $runtimeSceneValidationSourcePath -Raw
        foreach ($requiredText in @(
            "mirakana/runtime/asset_runtime.hpp",
            "mirakana/runtime_scene/runtime_scene.hpp",
            "load_runtime_asset_package",
            "instantiate_runtime_scene"
        )) {
            if (-not $helperText.Contains($requiredText)) {
                Write-Error "$runtimeSceneValidationSourcePath must reuse runtime package/scene validation helper: $requiredText"
            }
        }
        foreach ($forbiddenText in @(
            "mirakana/tools/asset_import_tool.hpp",
            "mirakana/tools/asset_package_tool.hpp",
            "mirakana/renderer/",
            "mirakana/rhi/",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package",
            "write_text(",
            "IRhiDevice",
            "ID3D12",
            "VkDevice",
            "MTLDevice"
        )) {
            if ($helperText.Contains($forbiddenText)) {
                Write-Error "$runtimeSceneValidationSourcePath must not mutate, import, package, or use renderer/RHI/native surfaces: $forbiddenText"
            }
        }
    }
}
$validationRunnerCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "run-validation-recipe" })
if ($validationRunnerCommand.Count -ne 1 -or $validationRunnerCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready run-validation-recipe command surface"
} else {
    $runnerModes = @{}
    foreach ($mode in @($validationRunnerCommand[0].requestModes)) {
        $runnerModes[$mode.id] = $mode
    }
    if (-not $runnerModes.ContainsKey("dry-run") -or $runnerModes["dry-run"].status -ne "ready" -or
        -not $runnerModes.ContainsKey("execute") -or $runnerModes["execute"].status -ne "ready") {
        Write-Error "engine manifest run-validation-recipe must keep dry-run and execute ready"
    }
    foreach ($requiredField in @("mode", "validationRecipe")) {
        if (@($validationRunnerCommand[0].requestShape.requiredFields) -notcontains $requiredField) {
            Write-Error "engine manifest run-validation-recipe requestShape missing required field: $requiredField"
        }
    }
    foreach ($optionalField in @("gameTarget", "strictBackend", "hostGateAcknowledgements", "timeoutSeconds")) {
        if (@($validationRunnerCommand[0].requestShape.optionalFields) -notcontains $optionalField) {
            Write-Error "engine manifest run-validation-recipe requestShape missing optional field: $optionalField"
        }
    }
    $runnerPolicyText = "$($validationRunnerCommand[0].requestShape.pathPolicy) $($validationRunnerCommand[0].notes)"
    foreach ($needle in @(
            "tools/run-validation-recipe.ps1",
            "Get-ValidationRecipeCommandPlan",
            "Invoke-ValidationRecipeCommandPlan",
            "does not evaluate raw manifest command strings",
            "free-form arguments are rejected",
            "not arbitrary shell"
        )) {
        if (-not $runnerPolicyText.Contains($needle)) {
            Write-Error "engine manifest run-validation-recipe must document runner path/helper/policy text: $needle"
        }
    }
    foreach ($field in @("recipe", "status", "command", "argv", "hostGates", "diagnostics", "blockedBy")) {
        if (@($validationRunnerCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest run-validation-recipe dryRunFields missing: $field"
        }
    }
    foreach ($field in @("recipe", "status", "exitCode", "durationSeconds", "stdoutSummary", "stderrSummary", "hostGates", "diagnostics")) {
        if (@($validationRunnerCommand[0].resultShape.executeFields) -notcontains $field) {
            Write-Error "engine manifest run-validation-recipe executeFields missing: $field"
        }
    }
    foreach ($recipe in @(
            "agent-contract",
            "default",
            "public-api-boundary",
            "shader-toolchain",
            "desktop-game-runtime",
            "desktop-runtime-sample-game-scene-gpu-package",
            "desktop-runtime-generated-material-shader-scaffold-package",
            "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict",
            "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package",
            "dev-windows-editor-game-module-driver-load-tests"
        )) {
        if (@($validationRunnerCommand[0].validationRecipes) -notcontains $recipe) {
            Write-Error "engine manifest run-validation-recipe validationRecipes missing allowlisted recipe: $recipe"
        }
    }
    if (@($validationRunnerCommand[0].validationRecipes).Count -ne 10) {
        Write-Error "engine manifest run-validation-recipe validationRecipes must be exactly the reviewed allowlist"
    }
    if (@($validationRunnerCommand[0].requestModes | Where-Object { $_.id -eq "apply" -and $_.status -eq "ready" }).Count -gt 0) {
        Write-Error "engine manifest run-validation-recipe must not expose a ready apply mode"
    }
    $runnerScriptPath = Join-Path $root "tools/run-validation-recipe.ps1"
    if (-not (Test-Path -LiteralPath $runnerScriptPath -PathType Leaf)) {
        Write-Error "engine manifest run-validation-recipe references missing tools/run-validation-recipe.ps1"
    }
    $runnerText = Get-Content -LiteralPath $runnerScriptPath -Raw
    foreach ($needle in @("validation-recipe-core.ps1", "Get-ValidationRecipeCommandPlan", "Invoke-ValidationRecipeCommandPlan")) {
        if (-not $runnerText.Contains($needle)) {
            Write-Error "tools/run-validation-recipe.ps1 missing reviewed helper name: $needle"
        }
    }
    $runnerCoreScriptPath = Join-Path $root "tools/validation-recipe-core.ps1"
    if (-not (Test-Path -LiteralPath $runnerCoreScriptPath -PathType Leaf)) {
        Write-Error "engine manifest run-validation-recipe references missing tools/validation-recipe-core.ps1"
    }
    $runnerCoreText = Get-Content -LiteralPath $runnerCoreScriptPath -Raw
    foreach ($needle in @("Test-ValidationRecipeRequest", "New-ValidationRecipeDryRunResult", "New-ValidationRecipeRejectedResult")) {
        if (-not $runnerCoreText.Contains($needle)) {
            Write-Error "tools/validation-recipe-core.ps1 missing reviewed helper name: $needle"
        }
    }
    foreach ($forbiddenRunnerPattern in @(
        "\bInvoke-Expression\b",
        "\biex\b",
        "\[scriptblock\]::Create",
        "\bcmd\s*/c\b",
        "\bbash\s+-c\b",
        "\bpwsh\b[^\r\n]*-Command\b",
        "\bpowershell\b[^\r\n]*-Command\b"
    )) {
        foreach ($runnerFile in @(
            @{ Label = "tools/run-validation-recipe.ps1"; Text = $runnerText },
            @{ Label = "tools/validation-recipe-core.ps1"; Text = $runnerCoreText }
        )) {
            if ([System.Text.RegularExpressions.Regex]::IsMatch($runnerFile.Text, $forbiddenRunnerPattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
                Write-Error "$($runnerFile.Label) must not contain shell-eval pattern: $forbiddenRunnerPattern"
            }
        }
    }
}
foreach ($commandId in $expectedCommandSurfaceIds) {
    if (-not $commandSurfaceIds.ContainsKey($commandId)) {
        Write-Error "engine manifest aiOperableProductionLoop missing command surface id: $commandId"
    }
}

$aiGameDevelopmentText = Get-Content -LiteralPath (Join-Path $root "docs/ai-game-development.md") -Raw
$aiIntegrationText = Get-Content -LiteralPath (Join-Path $root "docs/ai-integration.md") -Raw
$generatedScenariosText = Get-Content -LiteralPath (Join-Path $root "docs/specs/generated-game-validation-scenarios.md") -Raw
$promptPackText = Get-Content -LiteralPath (Join-Path $root "docs/specs/game-prompt-pack.md") -Raw
$handoffPromptText = Get-Content -LiteralPath (Join-Path $root "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md") -Raw
$roadmapText = Get-Content -LiteralPath (Join-Path $root "docs/roadmap.md") -Raw
$authoredRuntimeWorkflowRequiredText = @(
    "validated authored-to-runtime workflow",
    "register-source-asset -> cook-registered-source-assets -> migrate-scene-v2-runtime-package -> mirakana::runtime::load_runtime_asset_package -> mirakana::runtime_scene::instantiate_runtime_scene"
)
foreach ($workflowDoc in @(
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" }
)) {
    foreach ($requiredText in $authoredRuntimeWorkflowRequiredText) {
        if (-not $workflowDoc.Text.Contains($requiredText)) {
            Write-Error "$($workflowDoc.Label) did not contain expected text: $requiredText"
        }
    }
}
$runtimeScenePackageValidationRequiredText = @(
    "validate-runtime-scene-package",
    "plan_runtime_scene_package_validation",
    "execute_runtime_scene_package_validation",
    "non-mutating runtime scene package validation"
)
foreach ($runtimeSceneValidationDoc in @(
    @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" }
)) {
    foreach ($requiredText in $runtimeScenePackageValidationRequiredText) {
        if (-not $runtimeSceneValidationDoc.Text.Contains($requiredText)) {
            Write-Error "$($runtimeSceneValidationDoc.Label) did not contain expected text: $requiredText"
        }
    }
}
foreach ($forbiddenScenePrefabAuthoringClaim in @(
    "Scene/Prefab v2 authoring makes Scene v2 runtime package migration ready",
    "Scene/Prefab v2 authoring alone makes Scene v2 package migration ready",
    "editor productization is ready",
    "nested prefab merge/resolution UX is ready",
    "arbitrary free-form scene edits are supported",
    "arbitrary free-form prefab edits are supported",
    "Scene/Prefab v2 authoring runs arbitrary shell"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenScenePrefabAuthoringClaim)) {
            Write-Error "$($doc.Label) contains forbidden Scene/Prefab v2 authoring claim: $forbiddenScenePrefabAuthoringClaim"
        }
    }
}
foreach ($forbiddenSceneMigrationClaim in @(
    "migrate-scene-v2-runtime-package executes external importers",
    "Scene v2 runtime package migration executes external importers",
    "migrate-scene-v2-runtime-package cooks dependent assets",
    "Scene v2 runtime package migration cooks dependent assets",
    "migrate-scene-v2-runtime-package performs broad package cooking",
    "Scene v2 runtime package migration performs broad package cooking",
    "migrate-scene-v2-runtime-package makes renderer/RHI residency ready",
    "Scene v2 runtime package migration makes renderer/RHI residency ready",
    "migrate-scene-v2-runtime-package makes package streaming ready",
    "Scene v2 runtime package migration makes package streaming ready",
    "migrate-scene-v2-runtime-package supports material graphs",
    "Scene v2 runtime package migration supports material graphs",
    "migrate-scene-v2-runtime-package supports shader graphs",
    "Scene v2 runtime package migration supports shader graphs",
    "migrate-scene-v2-runtime-package supports live shader generation",
    "Scene v2 runtime package migration supports live shader generation",
    "migrate-scene-v2-runtime-package makes editor productization ready",
    "Scene v2 runtime package migration makes editor productization ready",
    "migrate-scene-v2-runtime-package makes Metal ready",
    "Scene v2 runtime package migration makes Metal ready",
    "migrate-scene-v2-runtime-package exposes public native/RHI handles",
    "Scene v2 runtime package migration exposes public native/RHI handles",
    "migrate-scene-v2-runtime-package makes general production renderer quality ready",
    "Scene v2 runtime package migration makes general production renderer quality ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenSceneMigrationClaim)) {
            Write-Error "$($doc.Label) contains forbidden Scene v2 runtime package migration claim: $forbiddenSceneMigrationClaim"
        }
    }
}
foreach ($forbiddenSourceAssetRegistrationClaim in @(
    "source asset registration executes external importers",
    "register-source-asset executes external importers",
    "source asset registration cooks runtime packages",
    "register-source-asset cooks runtime packages",
    "source asset registration makes renderer/RHI residency ready",
    "source asset registration makes package streaming ready",
    "source asset registration supports material graphs",
    "source asset registration supports shader graphs",
    "source asset registration supports live shader generation",
    "source asset registration makes editor productization ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenSourceAssetRegistrationClaim)) {
            Write-Error "$($doc.Label) contains forbidden source asset registration claim: $forbiddenSourceAssetRegistrationClaim"
        }
    }
}
foreach ($forbiddenRegisteredCookClaim in @(
    "cook-registered-source-assets performs broad dependency cooking",
    "cook-registered-source-assets cooks unselected dependencies",
    "cook-registered-source-assets makes renderer/RHI residency ready",
    "cook-registered-source-assets makes package streaming ready",
    "cook-registered-source-assets supports material graphs",
    "cook-registered-source-assets supports shader graphs",
    "cook-registered-source-assets supports live shader generation",
    "cook-registered-source-assets makes editor productization ready",
    "cook-registered-source-assets makes Metal ready",
    "cook-registered-source-assets exposes public native/RHI handles",
    "cook-registered-source-assets makes general production renderer quality ready",
    "cook-registered-source-assets executes arbitrary shell",
    "cook-registered-source-assets supports free-form edits"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenRegisteredCookClaim)) {
            Write-Error "$($doc.Label) contains forbidden registered source asset cook/package claim: $forbiddenRegisteredCookClaim"
        }
    }
}
foreach ($forbiddenAuthoredRuntimeWorkflowClaim in @(
    "authored-to-runtime workflow performs broad package cooking",
    "authored-to-runtime workflow cooks unselected dependencies",
    "authored-to-runtime workflow parses source assets at runtime",
    "authored-to-runtime workflow makes renderer/RHI residency ready",
    "authored-to-runtime workflow makes package streaming ready",
    "authored-to-runtime workflow supports material graphs",
    "authored-to-runtime workflow supports shader graphs",
    "authored-to-runtime workflow supports live shader generation",
    "authored-to-runtime workflow makes editor productization ready",
    "authored-to-runtime workflow makes Metal ready",
    "authored-to-runtime workflow exposes public native/RHI handles",
    "authored-to-runtime workflow makes general production renderer quality ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenAuthoredRuntimeWorkflowClaim)) {
            Write-Error "$($doc.Label) contains forbidden authored-to-runtime workflow claim: $forbiddenAuthoredRuntimeWorkflowClaim"
        }
    }
}
foreach ($forbiddenRuntimeSceneValidationClaim in @(
    "validate-runtime-scene-package performs package cooking",
    "validate-runtime-scene-package parses source assets at runtime",
    "validate-runtime-scene-package executes external importers",
    "validate-runtime-scene-package makes renderer/RHI residency ready",
    "validate-runtime-scene-package makes package streaming ready",
    "validate-runtime-scene-package supports material graphs",
    "validate-runtime-scene-package supports shader graphs",
    "validate-runtime-scene-package supports live shader generation",
    "validate-runtime-scene-package makes editor productization ready",
    "validate-runtime-scene-package makes Metal ready",
    "validate-runtime-scene-package exposes public native/RHI handles",
    "validate-runtime-scene-package makes general production renderer quality ready",
    "validate-runtime-scene-package executes arbitrary shell",
    "validate-runtime-scene-package supports free-form edits"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenRuntimeSceneValidationClaim)) {
            Write-Error "$($doc.Label) contains forbidden runtime scene package validation claim: $forbiddenRuntimeSceneValidationClaim"
        }
    }
}

$authoringSurfaceIds = @{}
foreach ($authoringSurface in $productionLoop.authoringSurfaces) {
    Assert-Properties $authoringSurface @("id", "status", "owner", "notes") "engine manifest aiOperableProductionLoop authoringSurfaces"
    if ($authoringSurfaceIds.ContainsKey($authoringSurface.id)) {
        Write-Error "engine manifest aiOperableProductionLoop authoring surface id is duplicated: $($authoringSurface.id)"
    }
    $authoringSurfaceIds[$authoringSurface.id] = $true
    if ($allowedProductionStatuses -notcontains $authoringSurface.status) {
        Write-Error "engine manifest aiOperableProductionLoop authoring surface '$($authoringSurface.id)' has invalid status: $($authoringSurface.status)"
    }
}
$sceneAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "scene-component-prefab-schema-v2" })
if ($sceneAuthoringSurface.Count -ne 1 -or $sceneAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface scene-component-prefab-schema-v2 must be ready as a contract-only MK_scene surface"
}
if (-not ([string]$sceneAuthoringSurface[0].notes).Contains("Contract-only") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("nested prefab propagation/merge resolution UX") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("2D/3D vertical slices")) {
    Write-Error "engine manifest scene-component-prefab-schema-v2 authoring surface must keep contract-only follow-up limits explicit"
}
$assetIdentityAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "asset-identity-v2" })
if ($assetIdentityAuthoringSurface.Count -ne 1 -or $assetIdentityAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface asset-identity-v2 must be ready as a foundation-only MK_assets surface"
}
if (-not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Foundation-only") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("GameEngine.AssetIdentity.v2") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("plan_asset_identity_placements_v2") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Reviewed command-owned apply surfaces") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("placement_rows") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("ContentBrowserState") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("SourceAssetRegistryDocumentV1") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("ContentBrowserState::refresh_from") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("content_browser_import.assets") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("GameEngine.Project.v4 project.source_registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("refresh_content_browser_from_project_source_registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Reload Source Registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("audit_runtime_scene_asset_identity") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("AssetKeyV2 key-first") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("tools/new-game.ps1") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("runtime source registry parsing") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("2D/3D vertical slices")) {
    Write-Error "engine manifest asset-identity-v2 authoring surface must keep foundation-only follow-up limits explicit"
}
$uiAtlasAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "ui-atlas-metadata-authoring-tooling-v1" })
if ($uiAtlasAuthoringSurface.Count -ne 1 -or $uiAtlasAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface ui-atlas-metadata-authoring-tooling-v1 must be ready as a cooked-metadata-only MK_assets/MK_tools surface"
}
if (-not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("GameEngine.UiAtlas.v1") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_cooked_ui_atlas_metadata") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("verify_cooked_ui_atlas_package_metadata") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_packed_ui_atlas_from_decoded_images") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_packed_ui_glyph_atlas_from_rasterized_glyphs") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("RuntimeUiAtlasGlyph") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("GameEngine.CookedTexture.v1") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("renderer texture upload")) {
    Write-Error "engine manifest ui-atlas-metadata-authoring-tooling-v1 authoring surface must keep cooked metadata tooling, decoded/glyph atlas bridges, and renderer-upload limits explicit"
}

foreach ($packageSurface in $productionLoop.packageSurfaces) {
    Assert-Properties $packageSurface @("id", "status", "targets", "validationRecipes", "notes") "engine manifest aiOperableProductionLoop packageSurfaces"
    if ($allowedProductionStatuses -notcontains $packageSurface.status) {
        Write-Error "engine manifest aiOperableProductionLoop package surface '$($packageSurface.id)' has invalid status: $($packageSurface.status)"
    }
    foreach ($target in @($packageSurface.targets)) {
        if (-not $packagingTargetNames.ContainsKey($target)) {
            Write-Error "engine manifest aiOperableProductionLoop package surface '$($packageSurface.id)' references unknown packaging target: $target"
        }
    }
    foreach ($validationRecipe in @($packageSurface.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop package surface '$($packageSurface.id)' references unknown validation recipe: $validationRecipe"
        }
    }
}

$requiredGapIds = @(
    "scene-component-prefab-schema-v2",
    "frame-graph-v1",
    "upload-staging-v1",
    "2d-playable-vertical-slice",
    "3d-playable-vertical-slice",
    "editor-productization",
    "production-ui-importer-platform-adapters",
    "full-repository-quality-gate"
)
$gapIds = @{}
foreach ($gap in $productionLoop.unsupportedProductionGaps) {
    Assert-Properties $gap @("id", "oneDotZeroCloseoutTier", "status", "requiredBeforeReadyClaim", "notes") "engine manifest aiOperableProductionLoop unsupportedProductionGaps"
    $tier = [string]$gap.oneDotZeroCloseoutTier
    if (@("foundation-follow-up", "package-evidence", "closeout-wedge") -notcontains $tier) {
        Write-Error "engine manifest aiOperableProductionLoop unsupported gap '$($gap.id)' has invalid oneDotZeroCloseoutTier '$tier'"
    }
    $gapIds[$gap.id] = $true
    if ($gap.status -eq "ready") {
        Write-Error "engine manifest aiOperableProductionLoop unsupported gap '$($gap.id)' must not be ready"
    }
}
foreach ($gapId in $requiredGapIds) {
    if (-not $gapIds.ContainsKey($gapId)) {
        Write-Error "engine manifest aiOperableProductionLoop missing unsupported gap id: $gapId"
    }
}
$sceneSchemaGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "scene-component-prefab-schema-v2" })
if ($sceneSchemaGap.Count -ne 1 -or $sceneSchemaGap[0].status -ne "implemented-contract-only") {
    Write-Error "engine manifest aiOperableProductionLoop scene-component-prefab-schema-v2 gap must be implemented-contract-only"
}
if (-not ([string]$sceneSchemaGap[0].notes).Contains("contract-only") -or
    -not ([string]$sceneSchemaGap[0].notes).Contains("broad/dependent package cooking") -or
    -not ([string]$sceneSchemaGap[0].notes).Contains("nested prefab propagation/merge resolution UX")) {
    Write-Error "engine manifest aiOperableProductionLoop scene-component-prefab-schema-v2 gap must keep remaining unsupported claims explicit"
}
$assetIdentityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "asset-identity-v2" })
if ($assetIdentityGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop asset-identity-v2 gap must leave unsupportedProductionGaps after reference cleanup closeout"
}
$runtimeResourceGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "runtime-resource-v2" })
if ($runtimeResourceGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop runtime-resource-v2 gap must leave unsupportedProductionGaps after 1.0 scope closeout"
}
$recommendedText = (([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
    "Frame Graph Transient Texture Alias Planning v1",
    "FrameGraphTransientTextureAliasPlan",
    "plan_frame_graph_transient_texture_aliases",
    "Frame Graph Shadow Scratch Color Target-State Ownership v1",
    "shadow_color",
    "6 pass callbacks/15 barrier steps",
    "Frame Graph Viewport Surface Color State Executor v1",
    "RhiViewportSurface",
    "viewport_color",
    "Frame Graph Texture Aliasing Barrier Command v1",
    "record_frame_graph_texture_aliasing_barriers",
    "automatic executor insertion",
    "Package Streaming Frame Graph Texture Binding Handoff v1",
    "make_runtime_package_streaming_frame_graph_texture_bindings",
    "Frame Graph Render Pass Envelope v1",
    "render_passes_recorded",
    "frame-graph-v1"
)) {
    if (-not $recommendedText.Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe frame-graph transient alias planning and next gap: $needle"
    }
}
$rendererRhiGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "renderer-rhi-resource-foundation" })
if ($rendererRhiGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop renderer-rhi-resource-foundation gap must leave unsupportedProductionGaps after 1.0 scope closeout"
}
