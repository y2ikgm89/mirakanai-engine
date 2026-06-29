#requires -Version 7.0
#requires -PSEdition Core

# Chapter 7.8 for check-json-contracts.ps1 selected 2D commercial large-project fixture contracts.

function Get-Json2DCommercialLargeProjectFixturePropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-Json2DCommercialLargeProjectFixtureString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-Json2DCommercialLargeProjectFixturePath([string]$value, [string]$label) {
    Assert-Json2DCommercialLargeProjectFixtureString $value $label
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

function Assert-Json2DCommercialLargeProjectFixtureHash([string]$value, [string]$label) {
    if ($value -notmatch "^sha256:[0-9a-f]{64}$") {
        Write-Error "$label must be a lowercase sha256:<64 hex> hash"
    }
}

function Get-Json2DCommercialLargeProjectFixtureGameRoot($game, [string]$label) {
    $entryPoint = ([string]$game.entryPoint).Replace("\", "/")
    $entryPointMatch = [regex]::Match($entryPoint, "^games/([a-z][a-z0-9_]*)/main\.cpp$")
    if (-not $entryPointMatch.Success) {
        Write-Error "$label entryPoint must be games/<game_name>/main.cpp for 2D commercial large-project fixture validation"
    }

    return "games/$($entryPointMatch.Groups[1].Value)"
}

function Assert-Json2DCommercialLargeProjectFixtureGamePath([string]$value, [string]$gameRoot, [string]$label) {
    Assert-Json2DCommercialLargeProjectFixturePath $value $label
    $normalized = $value.Replace("\", "/")
    if ($normalized -ne $gameRoot -and -not $normalized.StartsWith("$gameRoot/")) {
        Write-Error "$label must stay under ${gameRoot}: $value"
    }
}

function Assert-Json2DCommercialLargeProjectFixtureIdMap($rows, [string]$label) {
    $ids = @{}
    foreach ($row in @($rows)) {
        Assert-Json2DCommercialLargeProjectFixtureString $row.id "$label.id"
        if ($ids.ContainsKey([string]$row.id)) {
            Write-Error "$label has duplicate id: $($row.id)"
        }
        $ids[[string]$row.id] = $row
    }

    return $ids
}

function Assert-Json2DCommercialLargeProjectFixtureRecipeIds($recipeIds, $recipeNames, [string]$label) {
    foreach ($recipeId in @($recipeIds)) {
        if (-not $recipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "$label references undeclared validation recipe: $recipeId"
        }
    }
}

function Assert-Json2DCommercialLargeProjectFixtureSourceIds($sourceManifestIds, $sourceManifestRows, [string]$label) {
    foreach ($sourceManifestId in @($sourceManifestIds)) {
        if (-not $sourceManifestRows.ContainsKey([string]$sourceManifestId)) {
            Write-Error "$label references unknown source manifest: $sourceManifestId"
        }
    }
}

function Assert-Json2DCommercialLargeProjectFixtureCookIds($cookOutputIds, $cookOutputRows, [string]$label) {
    foreach ($cookOutputId in @($cookOutputIds)) {
        if (-not $cookOutputRows.ContainsKey([string]$cookOutputId)) {
            Write-Error "$label references unknown deterministic cook output: $cookOutputId"
        }
    }
}

function Assert-Json2DCommercialLargeProjectFixtures {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $fixtureSet = Get-Json2DCommercialLargeProjectFixturePropertyValue $Game.aiWorkflow "twoDCommercialLargeProjectFixtures"
    if ($null -eq $fixtureSet) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.twoDCommercialLargeProjectFixtures missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $fixtureSet @(
        "schemaVersion",
        "capabilityId",
        "fixtureSetId",
        "sourceAssetManifestSetId",
        "deterministicCookSetId",
        "productionAtlasPackingSetId",
        "runtimeSourceParserExclusionSetId",
        "packageIndexPath",
        "scaleFixture",
        "edgeCaseRows",
        "stableOutput",
        "cleanRoomPolicy",
        "unsupportedClaims"
    ) "$Label aiWorkflow.twoDCommercialLargeProjectFixtures"

    if ($fixtureSet.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.twoDCommercialLargeProjectFixtures schemaVersion must be 1"
    }
    if ($fixtureSet.capabilityId -ne "2d-commercial-large-project-fixtures-v1") {
        Write-Error "$Label aiWorkflow.twoDCommercialLargeProjectFixtures capabilityId must be 2d-commercial-large-project-fixtures-v1"
    }

    $gameRoot = Get-Json2DCommercialLargeProjectFixtureGameRoot $Game $Label
    $expectedFixtureSetId = "$($Game.name)-2d-commercial-large-project-fixtures"
    if ($fixtureSet.fixtureSetId -ne $expectedFixtureSetId) {
        Write-Error "$Label aiWorkflow.twoDCommercialLargeProjectFixtures fixtureSetId must be $expectedFixtureSetId"
    }

    $sourceManifestSet = Get-Json2DCommercialLargeProjectFixturePropertyValue $Game.aiWorkflow "twoDCommercialSourceAssetManifests"
    $cookSet = Get-Json2DCommercialLargeProjectFixturePropertyValue $Game.aiWorkflow "twoDCommercialDeterministicCookOutputs"
    $atlasSet = Get-Json2DCommercialLargeProjectFixturePropertyValue $Game.aiWorkflow "twoDCommercialProductionAtlasPacking"
    $runtimeSourceParserExclusionSet = Get-Json2DCommercialLargeProjectFixturePropertyValue $Game.aiWorkflow "twoDCommercialRuntimeSourceParserExclusion"
    if ($null -eq $sourceManifestSet -or $null -eq $cookSet -or $null -eq $atlasSet -or $null -eq $runtimeSourceParserExclusionSet) {
        Write-Error "$Label aiWorkflow.twoDCommercialLargeProjectFixtures requires source, cook, atlas, and runtime parser exclusion descriptors"
    } else {
        if ($fixtureSet.sourceAssetManifestSetId -ne $sourceManifestSet.manifestSetId) {
            Write-Error "$Label large-project fixture sourceAssetManifestSetId must match twoDCommercialSourceAssetManifests.manifestSetId"
        }
        if ($fixtureSet.deterministicCookSetId -ne $cookSet.cookSetId) {
            Write-Error "$Label large-project fixture deterministicCookSetId must match twoDCommercialDeterministicCookOutputs.cookSetId"
        }
        if ($fixtureSet.productionAtlasPackingSetId -ne $atlasSet.packingSetId) {
            Write-Error "$Label large-project fixture productionAtlasPackingSetId must match twoDCommercialProductionAtlasPacking.packingSetId"
        }
        if ($fixtureSet.runtimeSourceParserExclusionSetId -ne $runtimeSourceParserExclusionSet.exclusionSetId) {
            Write-Error "$Label large-project fixture runtimeSourceParserExclusionSetId must match twoDCommercialRuntimeSourceParserExclusion.exclusionSetId"
        }
        if ($fixtureSet.packageIndexPath -ne $cookSet.packageIndexPath -or
            $fixtureSet.packageIndexPath -ne $atlasSet.packageIndexPath -or
            $fixtureSet.packageIndexPath -ne $runtimeSourceParserExclusionSet.packageIndexPath) {
            Write-Error "$Label large-project fixture packageIndexPath must match cook, atlas, and runtime parser exclusion descriptors"
        }
    }

    Assert-Json2DCommercialLargeProjectFixtureGamePath ([string]$fixtureSet.packageIndexPath) $gameRoot "$Label aiWorkflow.twoDCommercialLargeProjectFixtures.packageIndexPath"

    $recipeNames = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $recipeNames[[string]$recipe.name] = $true
    }
    $runtimePackageFileSet = @{}
    foreach ($runtimePackageFile in @($Game.runtimePackageFiles)) {
        $runtimePackageFileSet[[string]$runtimePackageFile] = $true
    }

    $sourceManifestRows = Assert-Json2DCommercialLargeProjectFixtureIdMap $sourceManifestSet.assetManifests "$Label source manifests"
    $cookOutputRows = Assert-Json2DCommercialLargeProjectFixtureIdMap $cookSet.cookOutputs "$Label deterministic cook outputs"

    Assert-Properties $fixtureSet.scaleFixture @(
        "mode",
        "assetCount",
        "sourceManifestRowCount",
        "cookOutputRowCount",
        "dependencyEdgeRowCount",
        "duplicateNameProbeCount",
        "invalidMetadataProbeCount",
        "deletionRenameChurnCycles",
        "runtimePackageFiles",
        "sampledSourceManifestIds",
        "sampledCookOutputIds",
        "validationRecipeIds",
        "evidence"
    ) "$Label large-project scaleFixture"
    if ([string]$fixtureSet.scaleFixture.mode -ne "first-party-large-project-fixture") {
        Write-Error "$Label large-project scaleFixture.mode must be first-party-large-project-fixture"
    }
    if ([int]$fixtureSet.scaleFixture.assetCount -lt 256) {
        Write-Error "$Label large-project scaleFixture.assetCount must be >= 256"
    }
    if ([int]$fixtureSet.scaleFixture.sourceManifestRowCount -lt [int]$fixtureSet.scaleFixture.assetCount) {
        Write-Error "$Label large-project scaleFixture.sourceManifestRowCount must cover assetCount"
    }
    if ([int]$fixtureSet.scaleFixture.cookOutputRowCount -lt 64) {
        Write-Error "$Label large-project scaleFixture.cookOutputRowCount must be >= 64"
    }
    if ([int]$fixtureSet.scaleFixture.dependencyEdgeRowCount -lt [int]$fixtureSet.scaleFixture.sourceManifestRowCount) {
        Write-Error "$Label large-project scaleFixture.dependencyEdgeRowCount must cover sourceManifestRowCount"
    }
    if ([int]$fixtureSet.scaleFixture.duplicateNameProbeCount -lt 2) {
        Write-Error "$Label large-project scaleFixture.duplicateNameProbeCount must be >= 2"
    }
    if ([int]$fixtureSet.scaleFixture.invalidMetadataProbeCount -lt 2) {
        Write-Error "$Label large-project scaleFixture.invalidMetadataProbeCount must be >= 2"
    }
    if ([int]$fixtureSet.scaleFixture.deletionRenameChurnCycles -lt 2) {
        Write-Error "$Label large-project scaleFixture.deletionRenameChurnCycles must be >= 2"
    }
    foreach ($runtimePackageFile in @($fixtureSet.scaleFixture.runtimePackageFiles)) {
        if (-not $runtimePackageFileSet.ContainsKey([string]$runtimePackageFile)) {
            Write-Error "$Label large-project scaleFixture.runtimePackageFiles references undeclared runtime package file: $runtimePackageFile"
        }
    }
    foreach ($runtimePackageFile in @($Game.runtimePackageFiles)) {
        if (@($fixtureSet.scaleFixture.runtimePackageFiles) -notcontains [string]$runtimePackageFile) {
            Write-Error "$Label large-project scaleFixture.runtimePackageFiles must cover every runtimePackageFiles row: $runtimePackageFile"
        }
    }
    Assert-Json2DCommercialLargeProjectFixtureSourceIds $fixtureSet.scaleFixture.sampledSourceManifestIds $sourceManifestRows "$Label large-project scaleFixture.sampledSourceManifestIds"
    Assert-Json2DCommercialLargeProjectFixtureCookIds $fixtureSet.scaleFixture.sampledCookOutputIds $cookOutputRows "$Label large-project scaleFixture.sampledCookOutputIds"
    Assert-Json2DCommercialLargeProjectFixtureRecipeIds $fixtureSet.scaleFixture.validationRecipeIds $recipeNames "$Label large-project scaleFixture"
    Assert-Json2DCommercialLargeProjectFixtureString $fixtureSet.scaleFixture.evidence "$Label large-project scaleFixture.evidence"

    $edgeCaseRows = Assert-Json2DCommercialLargeProjectFixtureIdMap $fixtureSet.edgeCaseRows "$Label large-project edgeCaseRows"
    $requiredEdgeCases = @{
        "many-assets" = @{ CaseKind = "many-assets"; Outcome = "stable-output"; Diagnostic = "none" }
        "duplicate-names" = @{ CaseKind = "duplicate-names"; Outcome = "fail-closed-diagnostic"; Diagnostic = "duplicate-asset-name" }
        "missing-dependencies" = @{ CaseKind = "missing-dependencies"; Outcome = "fail-closed-diagnostic"; Diagnostic = "missing-dependency" }
        "invalid-metadata" = @{ CaseKind = "invalid-metadata"; Outcome = "fail-closed-diagnostic"; Diagnostic = "invalid-metadata" }
        "asset-deletion-churn" = @{ CaseKind = "asset-deletion-churn"; Outcome = "stable-output"; Diagnostic = "asset-deletion-churn-reviewed" }
        "asset-rename-churn" = @{ CaseKind = "asset-rename-churn"; Outcome = "stable-output"; Diagnostic = "asset-rename-churn-reviewed" }
        "stable-output-replay" = @{ CaseKind = "stable-output-replay"; Outcome = "stable-output"; Diagnostic = "deterministic-replay" }
    }
    foreach ($requiredEdgeCaseId in @($requiredEdgeCases.Keys)) {
        if (-not $edgeCaseRows.ContainsKey($requiredEdgeCaseId)) {
            Write-Error "$Label large-project edgeCaseRows missing $requiredEdgeCaseId"
        }
    }
    foreach ($row in @($fixtureSet.edgeCaseRows)) {
        Assert-Properties $row @(
            "id",
            "caseKind",
            "expectedOutcome",
            "diagnosticId",
            "sourceManifestIds",
            "cookOutputIds",
            "validationRecipeIds",
            "evidence"
        ) "$Label large-project edgeCaseRows '$($row.id)'"

        $edgeCaseRequirement = $requiredEdgeCases[[string]$row.id]
        if ($null -eq $edgeCaseRequirement) {
            Write-Error "$Label large-project edgeCaseRows contains unsupported id: $($row.id)"
        } else {
            $requiredCaseKind = [string]$edgeCaseRequirement["CaseKind"]
            $requiredOutcome = [string]$edgeCaseRequirement["Outcome"]
            $requiredDiagnostic = [string]$edgeCaseRequirement["Diagnostic"]
            if ([string]$row.caseKind -ne $requiredCaseKind) {
                Write-Error "$Label large-project edgeCaseRows '$($row.id)' caseKind must be $requiredCaseKind"
            }
            if ([string]$row.expectedOutcome -ne $requiredOutcome) {
                Write-Error "$Label large-project edgeCaseRows '$($row.id)' expectedOutcome must be $requiredOutcome"
            }
            if ([string]$row.diagnosticId -ne $requiredDiagnostic) {
                Write-Error "$Label large-project edgeCaseRows '$($row.id)' diagnosticId must be $requiredDiagnostic"
            }
        }

        Assert-Json2DCommercialLargeProjectFixtureSourceIds $row.sourceManifestIds $sourceManifestRows "$Label large-project edgeCaseRows '$($row.id)'.sourceManifestIds"
        Assert-Json2DCommercialLargeProjectFixtureCookIds $row.cookOutputIds $cookOutputRows "$Label large-project edgeCaseRows '$($row.id)'.cookOutputIds"
        Assert-Json2DCommercialLargeProjectFixtureRecipeIds $row.validationRecipeIds $recipeNames "$Label large-project edgeCaseRows '$($row.id)'"
        Assert-Json2DCommercialLargeProjectFixtureString $row.evidence "$Label large-project edgeCaseRows '$($row.id)'.evidence"
    }

    Assert-Properties $fixtureSet.stableOutput @(
        "mode",
        "baselineReplayHash",
        "postChurnReplayHash",
        "deterministicReplayReady",
        "unchangedCookOutputIds",
        "changedCookOutputIds",
        "deletedSourceManifestIds",
        "renamedSourceManifestIds",
        "diagnosticIds",
        "validationRecipeIds",
        "evidence"
    ) "$Label large-project stableOutput"
    if ([string]$fixtureSet.stableOutput.mode -ne "deterministic-large-project-replay") {
        Write-Error "$Label large-project stableOutput.mode must be deterministic-large-project-replay"
    }
    Assert-Json2DCommercialLargeProjectFixtureHash ([string]$fixtureSet.stableOutput.baselineReplayHash) "$Label large-project stableOutput.baselineReplayHash"
    Assert-Json2DCommercialLargeProjectFixtureHash ([string]$fixtureSet.stableOutput.postChurnReplayHash) "$Label large-project stableOutput.postChurnReplayHash"
    if ([string]$fixtureSet.stableOutput.baselineReplayHash -ne [string]$fixtureSet.stableOutput.postChurnReplayHash) {
        Write-Error "$Label large-project stableOutput replay hashes must remain stable after deletion/rename churn"
    }
    if ($fixtureSet.stableOutput.deterministicReplayReady -ne $true) {
        Write-Error "$Label large-project stableOutput.deterministicReplayReady must be true"
    }
    if (@($fixtureSet.stableOutput.unchangedCookOutputIds).Count -lt 3) {
        Write-Error "$Label large-project stableOutput.unchangedCookOutputIds must include at least three cook outputs"
    }
    Assert-Json2DCommercialLargeProjectFixtureCookIds $fixtureSet.stableOutput.unchangedCookOutputIds $cookOutputRows "$Label large-project stableOutput.unchangedCookOutputIds"
    Assert-Json2DCommercialLargeProjectFixtureCookIds $fixtureSet.stableOutput.changedCookOutputIds $cookOutputRows "$Label large-project stableOutput.changedCookOutputIds"
    if (@($fixtureSet.stableOutput.deletedSourceManifestIds).Count -lt 1) {
        Write-Error "$Label large-project stableOutput.deletedSourceManifestIds must include deletion churn evidence"
    }
    if (@($fixtureSet.stableOutput.renamedSourceManifestIds).Count -lt 1) {
        Write-Error "$Label large-project stableOutput.renamedSourceManifestIds must include rename churn evidence"
    }
    Assert-Json2DCommercialLargeProjectFixtureSourceIds $fixtureSet.stableOutput.deletedSourceManifestIds $sourceManifestRows "$Label large-project stableOutput.deletedSourceManifestIds"
    Assert-Json2DCommercialLargeProjectFixtureSourceIds $fixtureSet.stableOutput.renamedSourceManifestIds $sourceManifestRows "$Label large-project stableOutput.renamedSourceManifestIds"
    foreach ($diagnosticId in @("duplicate-asset-name", "missing-dependency", "invalid-metadata", "deterministic-replay")) {
        if (@($fixtureSet.stableOutput.diagnosticIds) -notcontains $diagnosticId) {
            Write-Error "$Label large-project stableOutput.diagnosticIds missing $diagnosticId"
        }
    }
    Assert-Json2DCommercialLargeProjectFixtureRecipeIds $fixtureSet.stableOutput.validationRecipeIds $recipeNames "$Label large-project stableOutput"
    Assert-Json2DCommercialLargeProjectFixtureString $fixtureSet.stableOutput.evidence "$Label large-project stableOutput.evidence"

    Assert-Properties $fixtureSet.cleanRoomPolicy @(
        "sourceMaterial",
        "thirdPartyDependenciesAdded",
        "externalEngineCodeAssetsSamplesUi",
        "compatibilityParityEndorsementClaims",
        "legalReviewStatus",
        "evidence"
    ) "$Label large-project cleanRoomPolicy"
    if ([string]$fixtureSet.cleanRoomPolicy.sourceMaterial -ne "first-party-fixture-descriptors-only") {
        Write-Error "$Label large-project cleanRoomPolicy.sourceMaterial must be first-party-fixture-descriptors-only"
    }
    if ($fixtureSet.cleanRoomPolicy.thirdPartyDependenciesAdded -ne $false) {
        Write-Error "$Label large-project cleanRoomPolicy.thirdPartyDependenciesAdded must be false"
    }
    foreach ($propertyName in @("externalEngineCodeAssetsSamplesUi", "compatibilityParityEndorsementClaims")) {
        if ([string](Get-Json2DCommercialLargeProjectFixturePropertyValue $fixtureSet.cleanRoomPolicy $propertyName) -ne "blocked") {
            Write-Error "$Label large-project cleanRoomPolicy.$propertyName must be blocked"
        }
    }
    if ([string]$fixtureSet.cleanRoomPolicy.legalReviewStatus -ne "not-legal-advice-no-new-third-party-material") {
        Write-Error "$Label large-project cleanRoomPolicy.legalReviewStatus must be not-legal-advice-no-new-third-party-material"
    }
    Assert-Json2DCommercialLargeProjectFixtureString $fixtureSet.cleanRoomPolicy.evidence "$Label large-project cleanRoomPolicy.evidence"

    foreach ($unsupportedClaim in @(
            "runtime-source-parsing",
            "runtime-source-image-decoding",
            "external-engine-project-import",
            "external-asset-download",
            "marketplace-asset-ingestion",
            "arbitrary-script-execution",
            "package-script-execution",
            "native-handle-access",
            "renderer-rhi-residency",
            "broad-large-project-readiness",
            "external-engine-compatibility-claim",
            "legal-approval-automation",
            "broad-commercial-2d-readiness"
        )) {
        if (@($fixtureSet.unsupportedClaims) -notcontains $unsupportedClaim) {
            Write-Error "$Label aiWorkflow.twoDCommercialLargeProjectFixtures unsupportedClaims missing $unsupportedClaim"
        }
    }

    $schemaText = Get-JsonContractSurfaceText "schemas/game-agent.schema.json"
    foreach ($needle in @(
            '"twoDCommercialLargeProjectFixtures"',
            '"2d-commercial-large-project-fixtures-v1"',
            '"first-party-large-project-fixture"',
            '"deterministic-large-project-replay"',
            '"duplicate-names"'
        )) {
        Assert-ContainsText $schemaText $needle "schemas/game-agent.schema.json 2D commercial large-project fixtures schema"
    }

    $planText = Get-JsonContractSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"
    foreach ($needle in @(
            "twoDCommercialLargeProjectFixtures",
            "2d-commercial-large-project-fixtures-v1",
            "large-project fixtures",
            "duplicate names, missing dependencies, invalid metadata",
            "asset deletion/rename churn"
        )) {
        Assert-ContainsText $planText $needle "2D Commercial Production Excellence v1 Phase 2 large-project fixture evidence"
    }

    $currentCapabilitiesText = Get-JsonContractSurfaceText "docs/current-capabilities.md"
    foreach ($needle in @(
            "2D Commercial Large-Project Fixtures v1",
            "twoDCommercialLargeProjectFixtures",
            "stable output under many assets",
            "broad commercial 2D readiness remains unclaimed"
        )) {
        Assert-ContainsText $currentCapabilitiesText $needle "docs/current-capabilities.md 2D commercial large-project fixture evidence"
    }

    $gameGuidanceText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
    Assert-ContainsText $gameGuidanceText "twoDCommercialLargeProjectFixtures" "engine/agent/manifest.fragments/014-gameCodeGuidance.json large-project fixture guidance"

    if ($edgeCaseRows.Count -lt @($fixtureSet.edgeCaseRows).Count) {
        Write-Error "$Label large-project fixture edge case ids must remain unique"
    }
}

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    $requiresLargeProjectFixtures = $game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package"
    Assert-Json2DCommercialLargeProjectFixtures $game $relative $requiresLargeProjectFixtures
}
