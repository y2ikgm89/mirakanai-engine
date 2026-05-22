#requires -Version 7.0
#requires -PSEdition Core

# Chapter 6 for check-json-contracts.ps1 game design spec contracts.

function Get-JsonGameDesignSpecPropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-JsonGameDesignSpecString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-JsonGameDesignSpecStringArray($values, [string]$label) {
    $arrayValues = @($values)
    if ($arrayValues.Count -lt 1) {
        Write-Error "$label must contain at least one row"
        return
    }

    foreach ($value in $arrayValues) {
        Assert-JsonGameDesignSpecString $value $label
    }
}

function Assert-JsonGameDesignSpec {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $designSpec = Get-JsonGameDesignSpecPropertyValue $Game.aiWorkflow "gameDesignSpec"
    if ($null -eq $designSpec) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.gameDesignSpec missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $designSpec @(
        "schemaVersion",
        "capabilityId",
        "designId",
        "gameplayFamily",
        "template",
        "camera",
        "inputMap",
        "coreLoop",
        "sceneList",
        "assetRequests",
        "systems",
        "packageTargets",
        "validationRecipeIds",
        "qualityGates",
        "unsupportedClaims"
    ) "$Label aiWorkflow.gameDesignSpec"

    if ($designSpec.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.gameDesignSpec schemaVersion must be 1"
    }
    if ($designSpec.capabilityId -ne "ai-game-design-spec-v1") {
        Write-Error "$Label aiWorkflow.gameDesignSpec capabilityId must be ai-game-design-spec-v1"
    }
    if ([string]$designSpec.designId -notmatch "^[a-z][a-z0-9-]*$") {
        Write-Error "$Label aiWorkflow.gameDesignSpec designId must be kebab-case"
    }

    if (@(
            "headless-validation",
            "desktop-runtime-package",
            "desktop-runtime-cooked-scene",
            "desktop-runtime-material-shader",
            "2d-desktop-runtime-package",
            "3d-playable-desktop-package"
        ) -notcontains [string]$designSpec.gameplayFamily) {
        Write-Error "$Label aiWorkflow.gameDesignSpec gameplayFamily is not supported: $($designSpec.gameplayFamily)"
    }

    if (@(
            "Headless",
            "DesktopRuntimePackage",
            "DesktopRuntimeCookedScenePackage",
            "DesktopRuntimeMaterialShaderPackage",
            "DesktopRuntime2DPackage",
            "DesktopRuntime3DPackage"
        ) -notcontains [string]$designSpec.template) {
        Write-Error "$Label aiWorkflow.gameDesignSpec template is not supported: $($designSpec.template)"
    }

    Assert-Properties $designSpec.camera @("mode", "primary") "$Label aiWorkflow.gameDesignSpec.camera"
    if (@("none", "fixed-2d", "side-view-2d", "perspective-3d", "orbit-3d") -notcontains [string]$designSpec.camera.mode) {
        Write-Error "$Label aiWorkflow.gameDesignSpec camera mode is not supported: $($designSpec.camera.mode)"
    }
    Assert-JsonGameDesignSpecString $designSpec.camera.primary "$Label aiWorkflow.gameDesignSpec.camera.primary"

    foreach ($inputRow in @($designSpec.inputMap)) {
        Assert-Properties $inputRow @("action", "defaultBinding", "purpose") "$Label aiWorkflow.gameDesignSpec.inputMap"
        Assert-JsonGameDesignSpecString $inputRow.action "$Label aiWorkflow.gameDesignSpec.inputMap.action"
        Assert-JsonGameDesignSpecString $inputRow.defaultBinding "$Label aiWorkflow.gameDesignSpec.inputMap.defaultBinding"
        Assert-JsonGameDesignSpecString $inputRow.purpose "$Label aiWorkflow.gameDesignSpec.inputMap.purpose"
    }
    if (@($designSpec.inputMap).Count -lt 1) {
        Write-Error "$Label aiWorkflow.gameDesignSpec.inputMap must contain at least one row"
    }

    Assert-Properties $designSpec.coreLoop @("objective", "playerActions", "feedback", "winState", "failState", "restart") "$Label aiWorkflow.gameDesignSpec.coreLoop"
    Assert-JsonGameDesignSpecString $designSpec.coreLoop.objective "$Label aiWorkflow.gameDesignSpec.coreLoop.objective"
    Assert-JsonGameDesignSpecStringArray $designSpec.coreLoop.playerActions "$Label aiWorkflow.gameDesignSpec.coreLoop.playerActions"
    Assert-JsonGameDesignSpecString $designSpec.coreLoop.feedback "$Label aiWorkflow.gameDesignSpec.coreLoop.feedback"
    Assert-JsonGameDesignSpecString $designSpec.coreLoop.winState "$Label aiWorkflow.gameDesignSpec.coreLoop.winState"
    Assert-JsonGameDesignSpecString $designSpec.coreLoop.failState "$Label aiWorkflow.gameDesignSpec.coreLoop.failState"
    Assert-JsonGameDesignSpecString $designSpec.coreLoop.restart "$Label aiWorkflow.gameDesignSpec.coreLoop.restart"

    foreach ($sceneRow in @($designSpec.sceneList)) {
        Assert-Properties $sceneRow @("id", "kind", "path", "purpose") "$Label aiWorkflow.gameDesignSpec.sceneList"
        Assert-JsonGameDesignSpecString $sceneRow.id "$Label aiWorkflow.gameDesignSpec.sceneList.id"
        Assert-JsonGameDesignSpecString $sceneRow.kind "$Label aiWorkflow.gameDesignSpec.sceneList.kind"
        Assert-JsonGameDesignSpecString $sceneRow.path "$Label aiWorkflow.gameDesignSpec.sceneList.path"
        Assert-JsonGameDesignSpecString $sceneRow.purpose "$Label aiWorkflow.gameDesignSpec.sceneList.purpose"
    }
    if (@($designSpec.sceneList).Count -lt 1) {
        Write-Error "$Label aiWorkflow.gameDesignSpec.sceneList must contain at least one row"
    }

    foreach ($assetRow in @($designSpec.assetRequests)) {
        Assert-Properties $assetRow @("id", "kind", "purpose", "delivery") "$Label aiWorkflow.gameDesignSpec.assetRequests"
        Assert-JsonGameDesignSpecString $assetRow.id "$Label aiWorkflow.gameDesignSpec.assetRequests.id"
        Assert-JsonGameDesignSpecString $assetRow.kind "$Label aiWorkflow.gameDesignSpec.assetRequests.kind"
        Assert-JsonGameDesignSpecString $assetRow.purpose "$Label aiWorkflow.gameDesignSpec.assetRequests.purpose"
        Assert-JsonGameDesignSpecString $assetRow.delivery "$Label aiWorkflow.gameDesignSpec.assetRequests.delivery"
    }
    if (@($designSpec.assetRequests).Count -lt 1) {
        Write-Error "$Label aiWorkflow.gameDesignSpec.assetRequests must contain at least one row"
    }

    Assert-JsonGameDesignSpecStringArray $designSpec.systems "$Label aiWorkflow.gameDesignSpec.systems"

    $gamePackagingTargets = @($Game.packagingTargets | ForEach-Object { [string]$_ })
    foreach ($target in @($designSpec.packageTargets)) {
        Assert-JsonGameDesignSpecString $target "$Label aiWorkflow.gameDesignSpec.packageTargets"
        if ($gamePackagingTargets -notcontains [string]$target) {
            Write-Error "$Label aiWorkflow.gameDesignSpec packageTargets references undeclared packaging target: $target"
        }
    }
    if (@($designSpec.packageTargets).Count -lt 1) {
        Write-Error "$Label aiWorkflow.gameDesignSpec.packageTargets must contain at least one row"
    }

    $gameValidationRecipeNames = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $gameValidationRecipeNames[[string]$recipe.name] = $true
    }
    foreach ($recipeId in @($designSpec.validationRecipeIds)) {
        Assert-JsonGameDesignSpecString $recipeId "$Label aiWorkflow.gameDesignSpec.validationRecipeIds"
        if (-not $gameValidationRecipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "$Label aiWorkflow.gameDesignSpec validationRecipeIds references undeclared validation recipe: $recipeId"
        }
    }
    if (@($designSpec.validationRecipeIds).Count -lt 1) {
        Write-Error "$Label aiWorkflow.gameDesignSpec.validationRecipeIds must contain at least one row"
    }

    foreach ($qualityGate in @($designSpec.qualityGates)) {
        Assert-Properties $qualityGate @("id", "evidence", "recipeIds") "$Label aiWorkflow.gameDesignSpec.qualityGates"
        Assert-JsonGameDesignSpecString $qualityGate.id "$Label aiWorkflow.gameDesignSpec.qualityGates.id"
        Assert-JsonGameDesignSpecString $qualityGate.evidence "$Label aiWorkflow.gameDesignSpec.qualityGates.evidence"
        foreach ($recipeId in @($qualityGate.recipeIds)) {
            Assert-JsonGameDesignSpecString $recipeId "$Label aiWorkflow.gameDesignSpec.qualityGates.recipeIds"
            if (-not $gameValidationRecipeNames.ContainsKey([string]$recipeId)) {
                Write-Error "$Label aiWorkflow.gameDesignSpec quality gate '$($qualityGate.id)' references undeclared validation recipe: $recipeId"
            }
        }
        if (@($qualityGate.recipeIds).Count -lt 1) {
            Write-Error "$Label aiWorkflow.gameDesignSpec quality gate '$($qualityGate.id)' must reference at least one recipe"
        }
    }
    if (@($designSpec.qualityGates).Count -lt 1) {
        Write-Error "$Label aiWorkflow.gameDesignSpec.qualityGates must contain at least one row"
    }

    foreach ($unsupportedClaim in @("engine-internal-edits", "native-handles", "unreviewed-external-assets")) {
        if (@($designSpec.unsupportedClaims) -notcontains $unsupportedClaim) {
            Write-Error "$Label aiWorkflow.gameDesignSpec unsupportedClaims missing $unsupportedClaim"
        }
    }
}

Get-ChildItem -Path (Join-Path $root "games") -Recurse -Filter "game.agent.json" | ForEach-Object {
    $relative = Get-RelativeRepoPath $_.FullName
    $game = Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json
    $requiresGameDesignSpec = @("2d-desktop-runtime-package", "3d-playable-desktop-package") -contains $game.gameplayContract.productionRecipe
    Assert-JsonGameDesignSpec $game $relative $requiresGameDesignSpec
}
