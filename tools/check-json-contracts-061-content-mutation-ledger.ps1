#requires -Version 7.0
#requires -PSEdition Core

# Chapter 6.1 for check-json-contracts.ps1 AI content mutation ledger contracts.

function Get-JsonMutationLedgerPropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-JsonMutationLedgerString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-JsonMutationLedgerPath([string]$value, [string]$label) {
    Assert-JsonMutationLedgerString $value $label
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

function Get-JsonMutationLedgerGameRoot($game, [string]$label) {
    $entryPoint = ([string]$game.entryPoint).Replace("\", "/")
    $entryPointMatch = [regex]::Match($entryPoint, "^games/([a-z][a-z0-9_]*)/main\.cpp$")
    if (-not $entryPointMatch.Success) {
        Write-Error "$label entryPoint must be games/<game_name>/main.cpp for content mutation ledger validation"
    }

    return "games/$($entryPointMatch.Groups[1].Value)"
}

function Assert-JsonMutationLedgerGamePath([string]$value, [string]$gameRoot, [string]$label) {
    Assert-JsonMutationLedgerPath $value $label
    $normalized = $value.Replace("\", "/")
    if ($normalized -ne $gameRoot -and -not $normalized.StartsWith("$gameRoot/")) {
        Write-Error "$label must stay under ${gameRoot}: $value"
    }
}

function Assert-JsonMutationLedgerIdMap($rows, [string]$label) {
    $ids = @{}
    foreach ($row in @($rows)) {
        Assert-JsonMutationLedgerString $row.id "$label.id"
        if ($ids.ContainsKey([string]$row.id)) {
            Write-Error "$label has duplicate id: $($row.id)"
        }
        $ids[[string]$row.id] = $true
    }

    return $ids
}

function Assert-JsonMutationLedgerCommandReference($ids, [string]$commandId, [string]$label) {
    Assert-JsonMutationLedgerString $commandId $label
    if (-not $ids.ContainsKey($commandId)) {
        Write-Error "$label references unknown reviewed command surface: $commandId"
    }
}

function Assert-JsonMutationLedger {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $ledger = Get-JsonMutationLedgerPropertyValue $Game.aiWorkflow "contentMutationLedger"
    if ($null -eq $ledger) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.contentMutationLedger missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $ledger @(
        "schemaVersion",
        "capabilityId",
        "ledgerId",
        "aiOwnedSourceRoots",
        "generatedFiles",
        "reviewedCommandSurfaces",
        "forbiddenSharedPaths",
        "remediationActions"
    ) "$Label aiWorkflow.contentMutationLedger"

    if ($ledger.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.contentMutationLedger schemaVersion must be 1"
    }
    if ($ledger.capabilityId -ne "ai-safe-content-mutation-ledger-v1") {
        Write-Error "$Label aiWorkflow.contentMutationLedger capabilityId must be ai-safe-content-mutation-ledger-v1"
    }

    $gameRoot = Get-JsonMutationLedgerGameRoot $Game $Label
    $expectedLedgerId = "$($Game.name)-ai-mutation-ledger"
    if ($ledger.ledgerId -ne $expectedLedgerId) {
        Write-Error "$Label aiWorkflow.contentMutationLedger ledgerId must be $expectedLedgerId"
    }

    $recipeNames = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $recipeNames[[string]$recipe.name] = $true
    }

    $commandIds = Assert-JsonMutationLedgerIdMap $ledger.reviewedCommandSurfaces "$Label aiWorkflow.contentMutationLedger.reviewedCommandSurfaces"
    foreach ($requiredCommandId in @("create-game-recipe", "register-runtime-package-files", "engine-capability-handoff")) {
        if (-not $commandIds.ContainsKey($requiredCommandId)) {
            Write-Error "$Label aiWorkflow.contentMutationLedger reviewedCommandSurfaces missing $requiredCommandId"
        }
    }

    foreach ($surface in @($ledger.reviewedCommandSurfaces)) {
        Assert-Properties $surface @("id", "mode", "command", "allowedPathRoots", "validationRecipeIds", "unsupportedActions") "$Label aiWorkflow.contentMutationLedger.reviewedCommandSurfaces"
        if (@("dry-run-apply", "review-only") -notcontains [string]$surface.mode) {
            Write-Error "$Label reviewed command surface '$($surface.id)' has unsupported mode: $($surface.mode)"
        }
        Assert-JsonMutationLedgerString $surface.command "$Label reviewed command surface '$($surface.id)'.command"
        if (($surface.mode -eq "dry-run-apply") -and ([string]$surface.command -notmatch "^pwsh -NoProfile -ExecutionPolicy Bypass -File tools/[a-z0-9-]+\.ps1$")) {
            Write-Error "$Label reviewed command surface '$($surface.id)' must use a supported tools/*.ps1 workflow command"
        }
        if ([string]$surface.command -match "(\|\||&&|;)") {
            Write-Error "$Label reviewed command surface '$($surface.id)' must not contain shell command chaining"
        }
        foreach ($pathRoot in @($surface.allowedPathRoots)) {
            Assert-JsonMutationLedgerGamePath ([string]$pathRoot) $gameRoot "$Label reviewed command surface '$($surface.id)'.allowedPathRoots"
        }
        foreach ($recipeId in @($surface.validationRecipeIds)) {
            Assert-JsonMutationLedgerString $recipeId "$Label reviewed command surface '$($surface.id)'.validationRecipeIds"
            if (-not $recipeNames.ContainsKey([string]$recipeId)) {
                Write-Error "$Label reviewed command surface '$($surface.id)' references undeclared validation recipe: $recipeId"
            }
        }
        foreach ($unsupportedAction in @(
                "arbitrary-shell",
                "engine-internal-edits",
                "native-handles",
                "middleware-contracts",
                "unreviewed-external-assets",
                "cooked-package-mutation",
                "renderer-rhi-residency",
                "broad-commercial-quality"
            )) {
            if (@($surface.unsupportedActions) -notcontains $unsupportedAction) {
                Write-Error "$Label reviewed command surface '$($surface.id)' unsupportedActions missing $unsupportedAction"
            }
        }
    }

    $aiOwnedRootIds = Assert-JsonMutationLedgerIdMap $ledger.aiOwnedSourceRoots "$Label aiWorkflow.contentMutationLedger.aiOwnedSourceRoots"
    foreach ($sourceRoot in @($ledger.aiOwnedSourceRoots)) {
        Assert-Properties $sourceRoot @("id", "path", "allowedOperations", "reviewedCommandSurfaceIds", "evidence") "$Label aiWorkflow.contentMutationLedger.aiOwnedSourceRoots"
        Assert-JsonMutationLedgerGamePath ([string]$sourceRoot.path) $gameRoot "$Label aiWorkflow.contentMutationLedger.aiOwnedSourceRoots '$($sourceRoot.id)'.path"
        foreach ($operation in @($sourceRoot.allowedOperations)) {
            if (@("create", "modify") -notcontains [string]$operation) {
                Write-Error "$Label AI-owned source root '$($sourceRoot.id)' has unsupported operation: $operation"
            }
        }
        foreach ($commandId in @($sourceRoot.reviewedCommandSurfaceIds)) {
            Assert-JsonMutationLedgerCommandReference $commandIds ([string]$commandId) "$Label AI-owned source root '$($sourceRoot.id)'.reviewedCommandSurfaceIds"
        }
        Assert-JsonMutationLedgerString $sourceRoot.evidence "$Label AI-owned source root '$($sourceRoot.id)'.evidence"
    }

    if (-not $aiOwnedRootIds.ContainsKey("game-root")) {
        Write-Error "$Label aiWorkflow.contentMutationLedger aiOwnedSourceRoots missing game-root"
    }

    $generatedFileIds = Assert-JsonMutationLedgerIdMap $ledger.generatedFiles "$Label aiWorkflow.contentMutationLedger.generatedFiles"
    foreach ($generatedFile in @($ledger.generatedFiles)) {
        Assert-Properties $generatedFile @("id", "path", "generatedBy", "reviewedCommandSurfaceId", "updatePolicy") "$Label aiWorkflow.contentMutationLedger.generatedFiles"
        Assert-JsonMutationLedgerGamePath ([string]$generatedFile.path) $gameRoot "$Label generated file '$($generatedFile.id)'.path"
        Assert-JsonMutationLedgerString $generatedFile.generatedBy "$Label generated file '$($generatedFile.id)'.generatedBy"
        Assert-JsonMutationLedgerCommandReference $commandIds ([string]$generatedFile.reviewedCommandSurfaceId) "$Label generated file '$($generatedFile.id)'.reviewedCommandSurfaceId"
        Assert-JsonMutationLedgerString $generatedFile.updatePolicy "$Label generated file '$($generatedFile.id)'.updatePolicy"
    }
    foreach ($requiredGeneratedFile in @("$gameRoot/game.agent.json", "$gameRoot/main.cpp", "$gameRoot/README.md")) {
        if (@($ledger.generatedFiles | Where-Object { ([string]$_.path).Replace("\", "/") -eq $requiredGeneratedFile }).Count -ne 1) {
            Write-Error "$Label aiWorkflow.contentMutationLedger generatedFiles missing $requiredGeneratedFile"
        }
    }
    foreach ($runtimeFile in @($Game.runtimePackageFiles)) {
        $requiredRuntimeFile = "$gameRoot/$runtimeFile"
        if (@($ledger.generatedFiles | Where-Object { ([string]$_.path).Replace("\", "/") -eq $requiredRuntimeFile }).Count -ne 1) {
            Write-Error "$Label aiWorkflow.contentMutationLedger generatedFiles missing runtime package file $requiredRuntimeFile"
        }
    }
    if ($generatedFileIds.Count -lt 3) {
        Write-Error "$Label aiWorkflow.contentMutationLedger generatedFiles must contain generated file rows"
    }

    $forbiddenPathIds = Assert-JsonMutationLedgerIdMap $ledger.forbiddenSharedPaths "$Label aiWorkflow.contentMutationLedger.forbiddenSharedPaths"
    foreach ($forbiddenPath in @($ledger.forbiddenSharedPaths)) {
        Assert-Properties $forbiddenPath @("id", "path", "reason") "$Label aiWorkflow.contentMutationLedger.forbiddenSharedPaths"
        Assert-JsonMutationLedgerPath ([string]$forbiddenPath.path) "$Label forbidden shared path '$($forbiddenPath.id)'.path"
        $normalized = ([string]$forbiddenPath.path).Replace("\", "/")
        if ($normalized -eq $gameRoot -or $normalized.StartsWith("$gameRoot/")) {
            Write-Error "$Label forbidden shared path '$($forbiddenPath.id)' must not point inside the game-owned root"
        }
        Assert-JsonMutationLedgerString $forbiddenPath.reason "$Label forbidden shared path '$($forbiddenPath.id)'.reason"
    }
    foreach ($requiredForbiddenId in @("engine", "editor", "shared-tools", "shared-schemas", "agent-surfaces", "repository-ci", "shared-docs", "shared-games-cmake")) {
        if (-not $forbiddenPathIds.ContainsKey($requiredForbiddenId)) {
            Write-Error "$Label aiWorkflow.contentMutationLedger forbiddenSharedPaths missing $requiredForbiddenId"
        }
    }

    $remediationIds = Assert-JsonMutationLedgerIdMap $ledger.remediationActions "$Label aiWorkflow.contentMutationLedger.remediationActions"
    foreach ($remediation in @($ledger.remediationActions)) {
        Assert-Properties $remediation @("id", "trigger", "reviewedCommandSurfaceId", "expectedResult", "validationRecipeIds") "$Label aiWorkflow.contentMutationLedger.remediationActions"
        Assert-JsonMutationLedgerString $remediation.trigger "$Label remediation action '$($remediation.id)'.trigger"
        Assert-JsonMutationLedgerCommandReference $commandIds ([string]$remediation.reviewedCommandSurfaceId) "$Label remediation action '$($remediation.id)'.reviewedCommandSurfaceId"
        Assert-JsonMutationLedgerString $remediation.expectedResult "$Label remediation action '$($remediation.id)'.expectedResult"
        foreach ($recipeId in @($remediation.validationRecipeIds)) {
            if (-not $recipeNames.ContainsKey([string]$recipeId)) {
                Write-Error "$Label remediation action '$($remediation.id)' references undeclared validation recipe: $recipeId"
            }
        }
    }
    foreach ($requiredRemediationId in @("missing-package-file", "unsafe-shared-path-request", "unsupported-engine-capability", "validation-failure")) {
        if (-not $remediationIds.ContainsKey($requiredRemediationId)) {
            Write-Error "$Label aiWorkflow.contentMutationLedger remediationActions missing $requiredRemediationId"
        }
    }
}

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    $requiresMutationLedger = @("2d-desktop-runtime-package", "3d-playable-desktop-package") -contains $game.gameplayContract.productionRecipe
    Assert-JsonMutationLedger $game $relative $requiresMutationLedger
}
