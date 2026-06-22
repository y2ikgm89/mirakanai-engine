#requires -Version 7.0
#requires -PSEdition Core

# Chapter 6.3 for check-json-contracts.ps1 AI generated-game playtest loop contracts.

function Get-JsonPlaytestLoopPropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-JsonPlaytestLoopString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-JsonPlaytestLoopPath([string]$value, [string]$label) {
    Assert-JsonPlaytestLoopString $value $label
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

function Get-JsonPlaytestLoopGameRoot($game, [string]$label) {
    $entryPoint = ([string]$game.entryPoint).Replace("\", "/")
    $entryPointMatch = [regex]::Match($entryPoint, "^games/([a-z][a-z0-9_]*)/main\.cpp$")
    if (-not $entryPointMatch.Success) {
        Write-Error "$label entryPoint must be games/<game_name>/main.cpp for generated-game playtest loop validation"
    }

    return "games/$($entryPointMatch.Groups[1].Value)"
}

function Assert-JsonPlaytestLoopGamePath([string]$value, [string]$gameRoot, [string]$label) {
    Assert-JsonPlaytestLoopPath $value $label
    $normalized = $value.Replace("\", "/")
    if ($normalized -ne $gameRoot -and -not $normalized.StartsWith("$gameRoot/")) {
        Write-Error "$label must stay under ${gameRoot}: $value"
    }
}

function Assert-JsonPlaytestLoopIdMap($rows, [string]$label) {
    $ids = @{}
    foreach ($row in @($rows)) {
        Assert-JsonPlaytestLoopString $row.id "$label.id"
        if ($ids.ContainsKey([string]$row.id)) {
            Write-Error "$label has duplicate id: $($row.id)"
        }
        $ids[[string]$row.id] = $true
    }

    return $ids
}

function Assert-JsonGeneratedGamePlaytestLoop {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $loop = Get-JsonPlaytestLoopPropertyValue $Game.aiWorkflow "generatedGamePlaytestLoop"
    if ($null -eq $loop) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $loop @(
        "schemaVersion",
        "capabilityId",
        "loopId",
        "evidenceRoot",
        "reviewedRecipeSurfaces",
        "selectedRecipes",
        "failureClassifications",
        "remediationPolicy",
        "unsupportedClaims"
    ) "$Label aiWorkflow.generatedGamePlaytestLoop"

    if ($loop.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop schemaVersion must be 1"
    }
    if ($loop.capabilityId -ne "ai-generated-game-playtest-loop-v1") {
        Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop capabilityId must be ai-generated-game-playtest-loop-v1"
    }

    $gameRoot = Get-JsonPlaytestLoopGameRoot $Game $Label
    $expectedLoopId = "$($Game.name)-playtest-loop"
    if ($loop.loopId -ne $expectedLoopId) {
        Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop loopId must be $expectedLoopId"
    }
    Assert-JsonPlaytestLoopGamePath ([string]$loop.evidenceRoot) $gameRoot "$Label aiWorkflow.generatedGamePlaytestLoop.evidenceRoot"
    if (-not ([string]$loop.evidenceRoot).Replace("\", "/").StartsWith("$gameRoot/reports/playtest")) {
        Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop evidenceRoot must stay under reports/playtest"
    }

    $recipeNames = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $recipeNames[[string]$recipe.name] = $true
    }
    $mutationLedger = Get-JsonPlaytestLoopPropertyValue $Game.aiWorkflow "contentMutationLedger"
    $remediationActionIds = @{}
    foreach ($action in @($mutationLedger.remediationActions)) {
        $remediationActionIds[[string]$action.id] = $true
    }

    $surfaceIds = Assert-JsonPlaytestLoopIdMap $loop.reviewedRecipeSurfaces "$Label aiWorkflow.generatedGamePlaytestLoop.reviewedRecipeSurfaces"
    foreach ($requiredSurfaceId in @("run-validation-recipe-dry-run", "run-validation-recipe-execute", "package-smoke-evidence-review")) {
        if (-not $surfaceIds.ContainsKey($requiredSurfaceId)) {
            Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop reviewedRecipeSurfaces missing $requiredSurfaceId"
        }
    }
    foreach ($surface in @($loop.reviewedRecipeSurfaces)) {
        Assert-Properties $surface @("id", "mode", "commandSurfaceId", "evidence") "$Label aiWorkflow.generatedGamePlaytestLoop.reviewedRecipeSurfaces"
        if (@("dry-run", "execute", "review-only") -notcontains [string]$surface.mode) {
            Write-Error "$Label playtest surface '$($surface.id)' mode is unsupported: $($surface.mode)"
        }
        if (@("run-validation-recipe", "package-smoke-output") -notcontains [string]$surface.commandSurfaceId) {
            Write-Error "$Label playtest surface '$($surface.id)' commandSurfaceId is unsupported: $($surface.commandSurfaceId)"
        }
        Assert-JsonPlaytestLoopString $surface.evidence "$Label playtest surface '$($surface.id)'.evidence"
    }

    $classificationIds = Assert-JsonPlaytestLoopIdMap $loop.failureClassifications "$Label aiWorkflow.generatedGamePlaytestLoop.failureClassifications"
    foreach ($requiredClassification in @(
            "missing-package-file",
            "invalid-reference",
            "host-gated",
            "shader-tool-gap",
            "counter-mismatch",
            "runtime-package-load",
            "unsafe-mutation-request"
        )) {
        if (-not $classificationIds.ContainsKey($requiredClassification)) {
            Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop failureClassifications missing $requiredClassification"
        }
    }
    foreach ($classification in @($loop.failureClassifications)) {
        Assert-Properties $classification @("id", "trigger", "remediationActionId", "evidence") "$Label aiWorkflow.generatedGamePlaytestLoop.failureClassifications"
        Assert-JsonPlaytestLoopString $classification.trigger "$Label failureClassification '$($classification.id)'.trigger"
        Assert-JsonPlaytestLoopString $classification.evidence "$Label failureClassification '$($classification.id)'.evidence"
        if (-not $remediationActionIds.ContainsKey([string]$classification.remediationActionId)) {
            Write-Error "$Label failureClassification '$($classification.id)' references unknown remediationActionId: $($classification.remediationActionId)"
        }
    }

    $selectedRecipeIds = @{}
    foreach ($recipe in @($loop.selectedRecipes)) {
        Assert-Properties $recipe @(
            "id",
            "validationRecipeId",
            "reviewedRecipeSurfaceId",
            "evidenceKind",
            "expectedSignals",
            "failureClassificationIds",
            "hostGatePolicy"
        ) "$Label aiWorkflow.generatedGamePlaytestLoop.selectedRecipes"
        Assert-JsonPlaytestLoopString $recipe.id "$Label selected recipe id"
        $selectedRecipeIds[[string]$recipe.validationRecipeId] = $true
        if (-not $recipeNames.ContainsKey([string]$recipe.validationRecipeId)) {
            Write-Error "$Label selected recipe '$($recipe.id)' references undeclared validation recipe: $($recipe.validationRecipeId)"
        }
        if (-not $surfaceIds.ContainsKey([string]$recipe.reviewedRecipeSurfaceId)) {
            Write-Error "$Label selected recipe '$($recipe.id)' references unknown reviewedRecipeSurfaceId: $($recipe.reviewedRecipeSurfaceId)"
        }
        if (@("recipe-summary", "package-smoke-log", "gameplay-counter-summary", "package-smoke-counter-import") -notcontains [string]$recipe.evidenceKind) {
            Write-Error "$Label selected recipe '$($recipe.id)' evidenceKind is unsupported: $($recipe.evidenceKind)"
        }
        if ([string]$recipe.hostGatePolicy -ne "respect-manifest-host-gates") {
            Write-Error "$Label selected recipe '$($recipe.id)' must respect manifest host gates"
        }
        if (@($recipe.expectedSignals).Count -lt 1) {
            Write-Error "$Label selected recipe '$($recipe.id)' expectedSignals must not be empty"
        }
        foreach ($classificationId in @($recipe.failureClassificationIds)) {
            if (-not $classificationIds.ContainsKey([string]$classificationId)) {
                Write-Error "$Label selected recipe '$($recipe.id)' references unknown failureClassificationId: $classificationId"
            }
        }
    }

    foreach ($requiredRecipeId in @("desktop-game-runtime", "desktop-runtime-release-target")) {
        if (-not $selectedRecipeIds.ContainsKey($requiredRecipeId)) {
            Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop selectedRecipes missing $requiredRecipeId"
        }
    }
    if ($Game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package" -and
        -not $selectedRecipeIds.ContainsKey("installed-2d-package-smoke")) {
        Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop selectedRecipes missing installed-2d-package-smoke"
    }
    if ($Game.gameplayContract.productionRecipe -eq "3d-playable-desktop-package") {
        $hasD3d12PackageSmoke = $false
        foreach ($recipeId in $selectedRecipeIds.Keys) {
            if ($recipeId -match "^installed-d3d12.*smoke$") {
                $hasD3d12PackageSmoke = $true
            }
        }
        if (-not $hasD3d12PackageSmoke) {
            Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop selectedRecipes must include selected D3D12 package smoke evidence"
        }
    }

    Assert-Properties $loop.remediationPolicy @("mode", "mutationLedgerId", "remediationActionIds", "evidence") "$Label aiWorkflow.generatedGamePlaytestLoop.remediationPolicy"
    if ([string]$loop.remediationPolicy.mode -ne "mutation-ledger-remediation") {
        Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop remediationPolicy.mode must be mutation-ledger-remediation"
    }
    if ([string]$loop.remediationPolicy.mutationLedgerId -ne [string]$mutationLedger.ledgerId) {
        Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop remediationPolicy.mutationLedgerId must match aiWorkflow.contentMutationLedger.ledgerId"
    }
    foreach ($actionId in @($loop.remediationPolicy.remediationActionIds)) {
        if (-not $remediationActionIds.ContainsKey([string]$actionId)) {
            Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop remediationPolicy references unknown remediation action: $actionId"
        }
    }
    Assert-JsonPlaytestLoopString $loop.remediationPolicy.evidence "$Label aiWorkflow.generatedGamePlaytestLoop.remediationPolicy.evidence"

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
        if (@($loop.unsupportedClaims) -notcontains $unsupportedClaim) {
            Write-Error "$Label aiWorkflow.generatedGamePlaytestLoop unsupportedClaims missing $unsupportedClaim"
        }
    }
}

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    $requiresPlaytestLoop = @("2d-desktop-runtime-package", "3d-playable-desktop-package") -contains $game.gameplayContract.productionRecipe
    Assert-JsonGeneratedGamePlaytestLoop $game $relative $requiresPlaytestLoop
}
