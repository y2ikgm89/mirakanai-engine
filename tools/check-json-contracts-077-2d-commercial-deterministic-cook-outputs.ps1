#requires -Version 7.0
#requires -PSEdition Core

# Chapter 7.5 for check-json-contracts.ps1 2D commercial deterministic cook output contracts.

function Get-Json2DCommercialDeterministicCookPropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-Json2DCommercialDeterministicCookString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-Json2DCommercialDeterministicCookPath([string]$value, [string]$label) {
    Assert-Json2DCommercialDeterministicCookString $value $label
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

function Get-Json2DCommercialDeterministicCookGameRoot($game, [string]$label) {
    $entryPoint = ([string]$game.entryPoint).Replace("\", "/")
    $entryPointMatch = [regex]::Match($entryPoint, "^games/([a-z][a-z0-9_]*)/main\.cpp$")
    if (-not $entryPointMatch.Success) {
        Write-Error "$label entryPoint must be games/<game_name>/main.cpp for 2D commercial deterministic cook validation"
    }

    return "games/$($entryPointMatch.Groups[1].Value)"
}

function Assert-Json2DCommercialDeterministicCookGamePath([string]$value, [string]$gameRoot, [string]$label) {
    Assert-Json2DCommercialDeterministicCookPath $value $label
    $normalized = $value.Replace("\", "/")
    if ($normalized -ne $gameRoot -and -not $normalized.StartsWith("$gameRoot/")) {
        Write-Error "$label must stay under ${gameRoot}: $value"
    }
}

function Assert-Json2DCommercialDeterministicCookRuntimeOutputPath(
    [string]$value,
    [string]$gameRoot,
    [string[]]$runtimePackageFiles,
    [string]$label
) {
    Assert-Json2DCommercialDeterministicCookGamePath $value $gameRoot $label
    $normalized = $value.Replace("\", "/")
    if (-not $normalized.StartsWith("$gameRoot/runtime/")) {
        Write-Error "$label must stay under the game runtime root: $value"
    }

    $gameRelativePath = $normalized.Substring($gameRoot.Length + 1)
    if ($runtimePackageFiles -notcontains $gameRelativePath) {
        Write-Error "$label must reference a declared runtimePackageFiles row: $gameRelativePath"
    }
}

function Assert-Json2DCommercialDeterministicCookIdMap($rows, [string]$label) {
    $ids = @{}
    foreach ($row in @($rows)) {
        Assert-Json2DCommercialDeterministicCookString $row.id "$label.id"
        if ($ids.ContainsKey([string]$row.id)) {
            Write-Error "$label has duplicate id: $($row.id)"
        }
        $ids[[string]$row.id] = $true
    }

    return $ids
}

function Assert-Json2DCommercialDeterministicCookRecipeIds($recipeIds, $recipeNames, [string]$label) {
    foreach ($recipeId in @($recipeIds)) {
        if (-not $recipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "$label references undeclared validation recipe: $recipeId"
        }
    }
}

function Assert-Json2DCommercialDeterministicCookOutputs {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $cookSet = Get-Json2DCommercialDeterministicCookPropertyValue $Game.aiWorkflow "twoDCommercialDeterministicCookOutputs"
    if ($null -eq $cookSet) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $cookSet @(
        "schemaVersion",
        "capabilityId",
        "cookSetId",
        "sourceAssetManifestSetId",
        "packageIndexPath",
        "cookOutputs",
        "dependencyGraphRows",
        "incrementalRecook",
        "packageBudget",
        "replay",
        "unsupportedClaims"
    ) "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs"

    if ($cookSet.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs schemaVersion must be 1"
    }
    if ($cookSet.capabilityId -ne "2d-commercial-deterministic-cook-outputs-v1") {
        Write-Error "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs capabilityId must be 2d-commercial-deterministic-cook-outputs-v1"
    }

    $gameRoot = Get-Json2DCommercialDeterministicCookGameRoot $Game $Label
    $expectedCookSetId = "$($Game.name)-2d-commercial-deterministic-cook"
    if ($cookSet.cookSetId -ne $expectedCookSetId) {
        Write-Error "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs cookSetId must be $expectedCookSetId"
    }

    $sourceManifestSet = Get-Json2DCommercialDeterministicCookPropertyValue $Game.aiWorkflow "twoDCommercialSourceAssetManifests"
    if ($null -eq $sourceManifestSet) {
        Write-Error "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs requires aiWorkflow.twoDCommercialSourceAssetManifests"
    } else {
        if ($cookSet.sourceAssetManifestSetId -ne $sourceManifestSet.manifestSetId) {
            Write-Error "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs sourceAssetManifestSetId must match twoDCommercialSourceAssetManifests.manifestSetId"
        }
        if ($cookSet.packageIndexPath -ne $sourceManifestSet.packageIndexPath) {
            Write-Error "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs packageIndexPath must match twoDCommercialSourceAssetManifests.packageIndexPath"
        }
    }
    Assert-Json2DCommercialDeterministicCookGamePath ([string]$cookSet.packageIndexPath) $gameRoot "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.packageIndexPath"
    if (-not ([string]$cookSet.packageIndexPath).Replace("\", "/").StartsWith("$gameRoot/runtime/")) {
        Write-Error "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs packageIndexPath must stay under runtime/"
    }

    $recipeNames = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $recipeNames[[string]$recipe.name] = $true
    }
    $runtimePackageFiles = @($Game.runtimePackageFiles | ForEach-Object { [string]$_ })
    $sourceManifestIds = Assert-Json2DCommercialDeterministicCookIdMap $sourceManifestSet.assetManifests "$Label aiWorkflow.twoDCommercialSourceAssetManifests.assetManifests"

    $cookOutputIds = Assert-Json2DCommercialDeterministicCookIdMap $cookSet.cookOutputs "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.cookOutputs"
    $sourceCoverage = @{}
    foreach ($row in @($cookSet.cookOutputs)) {
        Assert-Properties $row @(
            "id",
            "sourceManifestIds",
            "assetKey",
            "outputPath",
            "outputKind",
            "contentHash",
            "sourceProvenance",
            "licenseStatus",
            "packageBudgetRowId",
            "validationRecipeIds"
        ) "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.cookOutputs"

        foreach ($sourceManifestId in @($row.sourceManifestIds)) {
            if (-not $sourceManifestIds.ContainsKey([string]$sourceManifestId)) {
                Write-Error "$Label deterministic cook output '$($row.id)' references unknown source manifest: $sourceManifestId"
            }
            $sourceCoverage[[string]$sourceManifestId] = $true
        }
        Assert-Json2DCommercialDeterministicCookString $row.assetKey "$Label deterministic cook output '$($row.id)'.assetKey"
        Assert-Json2DCommercialDeterministicCookRuntimeOutputPath ([string]$row.outputPath) $gameRoot $runtimePackageFiles "$Label deterministic cook output '$($row.id)'.outputPath"
        if (@("texture", "tilemap", "animation-clip", "audio", "ui-atlas") -notcontains [string]$row.outputKind) {
            Write-Error "$Label deterministic cook output '$($row.id)' outputKind is unsupported: $($row.outputKind)"
        }
        if ([string]$row.contentHash -notmatch "^sha256:[0-9a-f]{64}$") {
            Write-Error "$Label deterministic cook output '$($row.id)' contentHash must be sha256:<64 lowercase hex>"
        }
        if ([string]$row.sourceProvenance -ne "first-party-created") {
            Write-Error "$Label deterministic cook output '$($row.id)' must preserve first-party-created source provenance"
        }
        if ([string]$row.licenseStatus -ne "first-party-proprietary-reviewed") {
            Write-Error "$Label deterministic cook output '$($row.id)' must preserve first-party-proprietary-reviewed license status"
        }
        Assert-Json2DCommercialDeterministicCookRecipeIds $row.validationRecipeIds $recipeNames "$Label deterministic cook output '$($row.id)'"
    }
    foreach ($sourceManifestId in $sourceManifestIds.Keys) {
        if (-not $sourceCoverage.ContainsKey($sourceManifestId)) {
            Write-Error "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs missing cook output coverage for source manifest: $sourceManifestId"
        }
    }

    $dependencyGraphIds = Assert-Json2DCommercialDeterministicCookIdMap $cookSet.dependencyGraphRows "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.dependencyGraphRows"
    $dependencyKinds = @{}
    $dependencySourceCoverage = @{}
    foreach ($row in @($cookSet.dependencyGraphRows)) {
        Assert-Properties $row @("id", "fromCookOutputId", "toSourceManifestId", "dependencyKind", "replayRelevant") "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.dependencyGraphRows"
        if (-not $cookOutputIds.ContainsKey([string]$row.fromCookOutputId)) {
            Write-Error "$Label dependency graph row '$($row.id)' references unknown cook output: $($row.fromCookOutputId)"
        }
        if (-not $sourceManifestIds.ContainsKey([string]$row.toSourceManifestId)) {
            Write-Error "$Label dependency graph row '$($row.id)' references unknown source manifest: $($row.toSourceManifestId)"
        }
        if (@("content-hash", "dependency-edge", "source-provenance", "license-status", "package-budget", "replay-hash") -notcontains [string]$row.dependencyKind) {
            Write-Error "$Label dependency graph row '$($row.id)' has unsupported dependencyKind: $($row.dependencyKind)"
        }
        $dependencyKinds[[string]$row.dependencyKind] = $true
        $dependencySourceCoverage[[string]$row.toSourceManifestId] = $true
        if ($row.replayRelevant -ne $true) {
            Write-Error "$Label dependency graph row '$($row.id)' replayRelevant must be true"
        }
    }
    foreach ($requiredKind in @("content-hash", "dependency-edge", "source-provenance", "license-status", "package-budget", "replay-hash")) {
        if (-not $dependencyKinds.ContainsKey($requiredKind)) {
            Write-Error "$Label dependency graph rows missing $requiredKind coverage"
        }
    }
    foreach ($sourceManifestId in $sourceManifestIds.Keys) {
        if (-not $dependencySourceCoverage.ContainsKey($sourceManifestId)) {
            Write-Error "$Label dependency graph rows missing source manifest coverage for: $sourceManifestId"
        }
    }

    Assert-Properties $cookSet.incrementalRecook @(
        "mode",
        "invalidationKey",
        "invalidatedSourceManifestIds",
        "impactedCookOutputIds",
        "unchangedCookOutputIds",
        "safePointRequired",
        "packageMutation",
        "evidence"
    ) "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.incrementalRecook"
    if ([string]$cookSet.incrementalRecook.mode -ne "content-hash-and-dependency-graph") {
        Write-Error "$Label incrementalRecook.mode must be content-hash-and-dependency-graph"
    }
    if ([string]$cookSet.incrementalRecook.invalidationKey -notmatch "^sha256:[0-9a-f]{64}$") {
        Write-Error "$Label incrementalRecook.invalidationKey must be sha256:<64 lowercase hex>"
    }
    if ($cookSet.incrementalRecook.safePointRequired -ne $true) {
        Write-Error "$Label incrementalRecook.safePointRequired must be true"
    }
    if ([string]$cookSet.incrementalRecook.packageMutation -ne "reviewed-safe-package-handoff") {
        Write-Error "$Label incrementalRecook.packageMutation must be reviewed-safe-package-handoff"
    }
    foreach ($sourceManifestId in @($cookSet.incrementalRecook.invalidatedSourceManifestIds)) {
        if (-not $sourceManifestIds.ContainsKey([string]$sourceManifestId)) {
            Write-Error "$Label incrementalRecook references unknown invalidatedSourceManifestId: $sourceManifestId"
        }
    }
    foreach ($cookOutputId in @($cookSet.incrementalRecook.impactedCookOutputIds + $cookSet.incrementalRecook.unchangedCookOutputIds)) {
        if (-not $cookOutputIds.ContainsKey([string]$cookOutputId)) {
            Write-Error "$Label incrementalRecook references unknown cook output id: $cookOutputId"
        }
    }
    Assert-Json2DCommercialDeterministicCookString $cookSet.incrementalRecook.evidence "$Label incrementalRecook.evidence"

    Assert-Properties $cookSet.packageBudget @("mode", "maxCookedBytes", "actualCookedBytes", "overBudgetRows", "budgetRows", "evidence") "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.packageBudget"
    if ([string]$cookSet.packageBudget.mode -ne "fail-closed-package-budget") {
        Write-Error "$Label packageBudget.mode must be fail-closed-package-budget"
    }
    if ([int]$cookSet.packageBudget.maxCookedBytes -le 0) {
        Write-Error "$Label packageBudget.maxCookedBytes must be positive"
    }
    if ([int]$cookSet.packageBudget.actualCookedBytes -le 0 -or [int]$cookSet.packageBudget.actualCookedBytes -gt [int]$cookSet.packageBudget.maxCookedBytes) {
        Write-Error "$Label packageBudget.actualCookedBytes must be positive and <= maxCookedBytes"
    }
    if ([int]$cookSet.packageBudget.overBudgetRows -ne 0) {
        Write-Error "$Label packageBudget.overBudgetRows must be 0"
    }
    $budgetRowIds = Assert-Json2DCommercialDeterministicCookIdMap $cookSet.packageBudget.budgetRows "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.packageBudget.budgetRows"
    $budgetCookOutputCoverage = @{}
    foreach ($row in @($cookSet.packageBudget.budgetRows)) {
        Assert-Properties $row @("id", "cookOutputId", "budgetClass", "packageFile", "byteCount", "maxBytes") "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.packageBudget.budgetRows"
        if (-not $cookOutputIds.ContainsKey([string]$row.cookOutputId)) {
            Write-Error "$Label package budget row '$($row.id)' references unknown cook output: $($row.cookOutputId)"
        }
        if (@("texture", "tilemap", "animation", "audio", "ui") -notcontains [string]$row.budgetClass) {
            Write-Error "$Label package budget row '$($row.id)' has unsupported budgetClass: $($row.budgetClass)"
        }
        if ([int]$row.byteCount -le 0 -or [int]$row.byteCount -gt [int]$row.maxBytes) {
            Write-Error "$Label package budget row '$($row.id)' byteCount must be positive and <= maxBytes"
        }
        Assert-Json2DCommercialDeterministicCookRuntimeOutputPath ([string]$row.packageFile) $gameRoot $runtimePackageFiles "$Label package budget row '$($row.id)'.packageFile"
        $budgetCookOutputCoverage[[string]$row.cookOutputId] = $true
    }
    foreach ($row in @($cookSet.cookOutputs)) {
        if (-not $budgetRowIds.ContainsKey([string]$row.packageBudgetRowId)) {
            Write-Error "$Label deterministic cook output '$($row.id)' references unknown packageBudgetRowId: $($row.packageBudgetRowId)"
        }
        if (-not $budgetCookOutputCoverage.ContainsKey([string]$row.id)) {
            Write-Error "$Label package budget rows missing cook output coverage for: $($row.id)"
        }
    }
    Assert-Json2DCommercialDeterministicCookString $cookSet.packageBudget.evidence "$Label packageBudget.evidence"

    Assert-Properties $cookSet.replay @("mode", "replayHash", "deterministicReplayReady", "dependencyGraphRowCount", "validationRecipeIds", "evidence") "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs.replay"
    if ([string]$cookSet.replay.mode -ne "deterministic-cook-replay-hash") {
        Write-Error "$Label replay.mode must be deterministic-cook-replay-hash"
    }
    if ([string]$cookSet.replay.replayHash -notmatch "^sha256:[0-9a-f]{64}$") {
        Write-Error "$Label replay.replayHash must be sha256:<64 lowercase hex>"
    }
    if ($cookSet.replay.deterministicReplayReady -ne $true) {
        Write-Error "$Label replay.deterministicReplayReady must be true"
    }
    if ([int]$cookSet.replay.dependencyGraphRowCount -ne @($cookSet.dependencyGraphRows).Count) {
        Write-Error "$Label replay.dependencyGraphRowCount must match dependencyGraphRows count"
    }
    Assert-Json2DCommercialDeterministicCookRecipeIds $cookSet.replay.validationRecipeIds $recipeNames "$Label replay"
    Assert-Json2DCommercialDeterministicCookString $cookSet.replay.evidence "$Label replay.evidence"

    foreach ($unsupportedClaim in @(
            "runtime-source-parsing",
            "external-engine-project-import",
            "external-asset-download",
            "marketplace-asset-ingestion",
            "arbitrary-script-execution",
            "native-handle-access",
            "nondeterministic-cook-output",
            "runtime-package-mutation",
            "broad-production-atlas-packing",
            "broad-commercial-2d-readiness"
        )) {
        if (@($cookSet.unsupportedClaims) -notcontains $unsupportedClaim) {
            Write-Error "$Label aiWorkflow.twoDCommercialDeterministicCookOutputs unsupportedClaims missing $unsupportedClaim"
        }
    }

    $schemaText = Get-JsonContractSurfaceText "schemas/game-agent.schema.json"
    foreach ($needle in @(
            '"twoDCommercialDeterministicCookOutputs"',
            '"2d-commercial-deterministic-cook-outputs-v1"',
            '"content-hash-and-dependency-graph"',
            '"fail-closed-package-budget"',
            '"deterministic-cook-replay-hash"',
            '"nondeterministic-cook-output"'
        )) {
        Assert-ContainsText $schemaText $needle "schemas/game-agent.schema.json 2D commercial deterministic cook output schema"
    }

    $planText = Get-JsonContractSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"
    foreach ($needle in @(
            "twoDCommercialDeterministicCookOutputs",
            "2d-commercial-deterministic-cook-outputs-v1",
            "deterministic cook outputs",
            "dependency graph rows",
            "incremental recook invalidation",
            "runtime package mutation remains unsupported"
        )) {
        Assert-ContainsText $planText $needle "2D Commercial Production Excellence v1 Phase 2 deterministic cook evidence"
    }

    $currentCapabilitiesText = Get-JsonContractSurfaceText "docs/current-capabilities.md"
    foreach ($needle in @(
            "2D Commercial Deterministic Cook Outputs v1",
            "twoDCommercialDeterministicCookOutputs",
            "dependency graph rows",
            "runtime package mutation remains unsupported",
            "broad commercial 2D readiness remains unclaimed"
        )) {
        Assert-ContainsText $currentCapabilitiesText $needle "docs/current-capabilities.md 2D commercial deterministic cook output evidence"
    }

    if ($dependencyGraphIds.Count -lt @($cookSet.dependencyGraphRows).Count) {
        Write-Error "$Label dependency graph ids must remain unique"
    }
}

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    $requiresCommercial2DDeterministicCook = $game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package"
    Assert-Json2DCommercialDeterministicCookOutputs $game $relative $requiresCommercial2DDeterministicCook
}
