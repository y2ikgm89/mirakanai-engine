#requires -Version 7.0
#requires -PSEdition Core

# Chapter 6.4 for check-json-contracts.ps1 AI validation remediation recipe contracts.

function Get-JsonValidationRemediationPropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-JsonValidationRemediationString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-JsonValidationRemediationIdMap($rows, [string]$label) {
    $ids = @{}
    foreach ($row in @($rows)) {
        Assert-JsonValidationRemediationString $row.id "$label.id"
        if ($ids.ContainsKey([string]$row.id)) {
            Write-Error "$label has duplicate id: $($row.id)"
        }
        $ids[[string]$row.id] = $true
    }

    return $ids
}

function Assert-JsonValidationRemediationStringArray($rows, [string]$label) {
    if (@($rows).Count -lt 1) {
        Write-Error "$label must not be empty"
    }
    foreach ($row in @($rows)) {
        Assert-JsonValidationRemediationString $row $label
    }
}

function Assert-JsonValidationRemediationRecipes {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $recipes = Get-JsonValidationRemediationPropertyValue $Game.aiWorkflow "validationRemediationRecipes"
    if ($null -eq $recipes) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.validationRemediationRecipes missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $recipes @(
        "schemaVersion",
        "capabilityId",
        "recipeSetId",
        "playtestLoopId",
        "mutationLedgerId",
        "recipes",
        "unsupportedClaims"
    ) "$Label aiWorkflow.validationRemediationRecipes"

    if ($recipes.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.validationRemediationRecipes schemaVersion must be 1"
    }
    if ($recipes.capabilityId -ne "ai-validation-remediation-recipes-v1") {
        Write-Error "$Label aiWorkflow.validationRemediationRecipes capabilityId must be ai-validation-remediation-recipes-v1"
    }

    $gameName = [string]$Game.name
    $expectedRecipeSetId = "$gameName-validation-remediation-recipes"
    if ([string]$recipes.recipeSetId -ne $expectedRecipeSetId) {
        Write-Error "$Label aiWorkflow.validationRemediationRecipes recipeSetId must be $expectedRecipeSetId"
    }

    $playtestLoop = Get-JsonValidationRemediationPropertyValue $Game.aiWorkflow "generatedGamePlaytestLoop"
    if ($null -eq $playtestLoop) {
        Write-Error "$Label aiWorkflow.validationRemediationRecipes requires aiWorkflow.generatedGamePlaytestLoop"
    } elseif ([string]$recipes.playtestLoopId -ne [string]$playtestLoop.loopId) {
        Write-Error "$Label aiWorkflow.validationRemediationRecipes playtestLoopId must match aiWorkflow.generatedGamePlaytestLoop.loopId"
    }

    $mutationLedger = Get-JsonValidationRemediationPropertyValue $Game.aiWorkflow "contentMutationLedger"
    if ($null -eq $mutationLedger) {
        Write-Error "$Label aiWorkflow.validationRemediationRecipes requires aiWorkflow.contentMutationLedger"
    } elseif ([string]$recipes.mutationLedgerId -ne [string]$mutationLedger.ledgerId) {
        Write-Error "$Label aiWorkflow.validationRemediationRecipes mutationLedgerId must match aiWorkflow.contentMutationLedger.ledgerId"
    }

    $classificationIds = Assert-JsonValidationRemediationIdMap $playtestLoop.failureClassifications "$Label aiWorkflow.generatedGamePlaytestLoop.failureClassifications"
    $actionIds = @{}
    $actionSurfaceIds = @{}
    foreach ($action in @($mutationLedger.remediationActions)) {
        Assert-JsonValidationRemediationString $action.id "$Label aiWorkflow.contentMutationLedger.remediationActions.id"
        $actionIds[[string]$action.id] = $true
        $actionSurfaceIds[[string]$action.id] = [string]$action.reviewedCommandSurfaceId
    }
    $surfaceIds = Assert-JsonValidationRemediationIdMap $mutationLedger.reviewedCommandSurfaces "$Label aiWorkflow.contentMutationLedger.reviewedCommandSurfaces"
    $recipeIds = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $recipeIds[[string]$recipe.name] = $true
    }

    $coveredClassificationIds = @{}
    foreach ($recipe in @($recipes.recipes)) {
        Assert-Properties $recipe @(
            "id",
            "failureClassificationId",
            "remediationActionId",
            "reviewedCommandSurfaceId",
            "mode",
            "validationRecipeIds",
            "requiredEvidence",
            "stopConditions",
            "rerunPolicy"
        ) "$Label aiWorkflow.validationRemediationRecipes.recipes"

        Assert-JsonValidationRemediationString $recipe.id "$Label validation remediation recipe id"
        if (-not $classificationIds.ContainsKey([string]$recipe.failureClassificationId)) {
            Write-Error "$Label remediation recipe '$($recipe.id)' references unknown failureClassificationId: $($recipe.failureClassificationId)"
        }
        if (-not $actionIds.ContainsKey([string]$recipe.remediationActionId)) {
            Write-Error "$Label remediation recipe '$($recipe.id)' references unknown remediationActionId: $($recipe.remediationActionId)"
        }
        if (-not $surfaceIds.ContainsKey([string]$recipe.reviewedCommandSurfaceId)) {
            Write-Error "$Label remediation recipe '$($recipe.id)' references unknown reviewedCommandSurfaceId: $($recipe.reviewedCommandSurfaceId)"
        }
        if ($actionSurfaceIds[[string]$recipe.remediationActionId] -ne [string]$recipe.reviewedCommandSurfaceId) {
            Write-Error "$Label remediation recipe '$($recipe.id)' reviewedCommandSurfaceId must match its mutation-ledger remediation action"
        }
        if (@("repair-through-reviewed-surface", "record-host-gate", "developer-handoff") -notcontains [string]$recipe.mode) {
            Write-Error "$Label remediation recipe '$($recipe.id)' mode is unsupported: $($recipe.mode)"
        }
        if ([string]$recipe.rerunPolicy -ne "rerun-selected-validation-recipe") {
            Write-Error "$Label remediation recipe '$($recipe.id)' must rerun selected validation recipes after remediation"
        }
        foreach ($validationRecipeId in @($recipe.validationRecipeIds)) {
            if (-not $recipeIds.ContainsKey([string]$validationRecipeId)) {
                Write-Error "$Label remediation recipe '$($recipe.id)' references undeclared validation recipe: $validationRecipeId"
            }
        }
        Assert-JsonValidationRemediationStringArray $recipe.validationRecipeIds "$Label remediation recipe '$($recipe.id)'.validationRecipeIds"
        Assert-JsonValidationRemediationStringArray $recipe.requiredEvidence "$Label remediation recipe '$($recipe.id)'.requiredEvidence"
        Assert-JsonValidationRemediationStringArray $recipe.stopConditions "$Label remediation recipe '$($recipe.id)'.stopConditions"

        $coveredClassificationIds[[string]$recipe.failureClassificationId] = $true
    }

    foreach ($requiredClassification in @(
            "missing-package-file",
            "invalid-reference",
            "host-gated",
            "shader-tool-gap",
            "counter-mismatch",
            "runtime-package-load"
        )) {
        if (-not $coveredClassificationIds.ContainsKey($requiredClassification)) {
            Write-Error "$Label aiWorkflow.validationRemediationRecipes missing recipe for $requiredClassification"
        }
    }

    foreach ($unsupportedClaim in @(
            "validation-weakening",
            "evidence-deletion",
            "host-gate-bypass",
            "arbitrary-shell",
            "raw-manifest-command-evaluation",
            "cooked-package-mutation",
            "engine-internal-edits",
            "native-handles",
            "broad-quality-claim"
        )) {
        if (@($recipes.unsupportedClaims) -notcontains $unsupportedClaim) {
            Write-Error "$Label aiWorkflow.validationRemediationRecipes unsupportedClaims missing $unsupportedClaim"
        }
    }
}

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    $requiresValidationRemediation = @("2d-desktop-runtime-package", "3d-playable-desktop-package") -contains $game.gameplayContract.productionRecipe
    Assert-JsonValidationRemediationRecipes $game $relative $requiresValidationRemediation
}
