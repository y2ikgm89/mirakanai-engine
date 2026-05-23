#requires -Version 7.0
#requires -PSEdition Core

# Chapter 6.5 for check-json-contracts.ps1 generated game quality rubric contracts.

function Get-JsonQualityRubricPropertyValue($object, [string]$propertyName) {
    if ($null -eq $object) {
        return $null
    }

    $property = $object.PSObject.Properties[$propertyName]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Assert-JsonQualityRubricString($value, [string]$label) {
    if ([string]::IsNullOrWhiteSpace([string]$value)) {
        Write-Error "$label must be a non-empty string"
    }
}

function Assert-JsonQualityRubricStringArray($values, [string]$label) {
    if (@($values).Count -lt 1) {
        Write-Error "$label must contain at least one value"
    }
    foreach ($value in @($values)) {
        Assert-JsonQualityRubricString $value $label
    }
}

function Get-JsonQualityRubricIdMap($rows, [string]$label) {
    $map = @{}
    foreach ($row in @($rows)) {
        Assert-JsonQualityRubricString $row.id "$label.id"
        if ($map.ContainsKey([string]$row.id)) {
            Write-Error "$label contains duplicate id: $($row.id)"
        }
        $map[[string]$row.id] = $true
    }
    return $map
}

function Assert-JsonGeneratedGameQualityRubric {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][bool]$Required
    )

    $rubric = Get-JsonQualityRubricPropertyValue $Game.aiWorkflow "generatedGameQualityRubric"
    if ($null -eq $rubric) {
        if ($Required) {
            Write-Error "$Label aiWorkflow.generatedGameQualityRubric missing for gameplay recipe '$($Game.gameplayContract.productionRecipe)'"
        }
        return
    }

    Assert-Properties $rubric @(
        "schemaVersion",
        "capabilityId",
        "rubricId",
        "designSpecId",
        "playtestLoopId",
        "remediationRecipeSetId",
        "reportId",
        "reportPath",
        "gateResults",
        "sampleReports",
        "unsupportedRows",
        "unsupportedClaims"
    ) "$Label aiWorkflow.generatedGameQualityRubric"

    if ($rubric.schemaVersion -ne 1) {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric schemaVersion must be 1"
    }
    if ($rubric.capabilityId -ne "ai-generated-game-quality-rubric-v1") {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric capabilityId must be ai-generated-game-quality-rubric-v1"
    }

    $gameName = [string]$Game.name
    if ([string]$rubric.rubricId -ne "$gameName-quality-rubric") {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric rubricId must be $gameName-quality-rubric"
    }
    if ([string]$rubric.reportId -ne "$gameName-quality-rubric-report") {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric reportId must be $gameName-quality-rubric-report"
    }
    if ([string]$rubric.reportPath -ne "games/$($Game.target)/reports/quality/$($Game.target)-quality-rubric.json") {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric reportPath must stay under the game-local quality report root"
    }

    $designSpec = Get-JsonQualityRubricPropertyValue $Game.aiWorkflow "gameDesignSpec"
    if ($null -eq $designSpec) {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric requires aiWorkflow.gameDesignSpec"
    } elseif ([string]$rubric.designSpecId -ne [string]$designSpec.designId) {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric designSpecId must match aiWorkflow.gameDesignSpec.designId"
    }

    $playtestLoop = Get-JsonQualityRubricPropertyValue $Game.aiWorkflow "generatedGamePlaytestLoop"
    if ($null -eq $playtestLoop) {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric requires aiWorkflow.generatedGamePlaytestLoop"
    } elseif ([string]$rubric.playtestLoopId -ne [string]$playtestLoop.loopId) {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric playtestLoopId must match aiWorkflow.generatedGamePlaytestLoop.loopId"
    }

    $remediation = Get-JsonQualityRubricPropertyValue $Game.aiWorkflow "validationRemediationRecipes"
    if ($null -eq $remediation) {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric requires aiWorkflow.validationRemediationRecipes"
    } elseif ([string]$rubric.remediationRecipeSetId -ne [string]$remediation.recipeSetId) {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric remediationRecipeSetId must match aiWorkflow.validationRemediationRecipes.recipeSetId"
    }

    $validationRecipeIds = @{}
    foreach ($recipe in @($Game.validationRecipes)) {
        $validationRecipeIds[[string]$recipe.name] = $true
    }
    $remediationRecipeIds = Get-JsonQualityRubricIdMap $remediation.recipes "$Label aiWorkflow.validationRemediationRecipes.recipes"

    $coveredCategories = @{}
    $gateResultIds = @{}
    foreach ($gate in @($rubric.gateResults)) {
        Assert-Properties $gate @(
            "id",
            "category",
            "status",
            "evidence",
            "requiredSignals",
            "validationRecipeIds",
            "remediationRecipeIds"
        ) "$Label aiWorkflow.generatedGameQualityRubric.gateResults"

        Assert-JsonQualityRubricString $gate.id "$Label quality gate result id"
        if ($gateResultIds.ContainsKey([string]$gate.id)) {
            Write-Error "$Label aiWorkflow.generatedGameQualityRubric duplicate gate result id: $($gate.id)"
        }
        $gateResultIds[[string]$gate.id] = $true

        if (@("objective", "controls", "feedback", "fail-restart", "deterministic-package-smoke", "budget-evidence") -notcontains [string]$gate.category) {
            Write-Error "$Label quality gate '$($gate.id)' category is unsupported: $($gate.category)"
        }
        if (@("passed-with-evidence", "host-gated", "unsupported") -notcontains [string]$gate.status) {
            Write-Error "$Label quality gate '$($gate.id)' status is unsupported: $($gate.status)"
        }

        Assert-JsonQualityRubricStringArray $gate.evidence "$Label quality gate '$($gate.id)'.evidence"
        Assert-JsonQualityRubricStringArray $gate.requiredSignals "$Label quality gate '$($gate.id)'.requiredSignals"
        Assert-JsonQualityRubricStringArray $gate.validationRecipeIds "$Label quality gate '$($gate.id)'.validationRecipeIds"
        Assert-JsonQualityRubricStringArray $gate.remediationRecipeIds "$Label quality gate '$($gate.id)'.remediationRecipeIds"

        foreach ($validationRecipeId in @($gate.validationRecipeIds)) {
            if (-not $validationRecipeIds.ContainsKey([string]$validationRecipeId)) {
                Write-Error "$Label quality gate '$($gate.id)' references undeclared validation recipe: $validationRecipeId"
            }
        }
        foreach ($remediationRecipeId in @($gate.remediationRecipeIds)) {
            if (-not $remediationRecipeIds.ContainsKey([string]$remediationRecipeId)) {
                Write-Error "$Label quality gate '$($gate.id)' references undeclared remediation recipe: $remediationRecipeId"
            }
        }

        $coveredCategories[[string]$gate.category] = $true
    }

    foreach ($requiredCategory in @("objective", "controls", "feedback", "fail-restart", "deterministic-package-smoke", "budget-evidence")) {
        if (-not $coveredCategories.ContainsKey($requiredCategory)) {
            Write-Error "$Label aiWorkflow.generatedGameQualityRubric missing gate category: $requiredCategory"
        }
    }

    $coveredReportKinds = @{}
    foreach ($report in @($rubric.sampleReports)) {
        Assert-Properties $report @("id", "kind", "path", "gateResultIds", "requiredSignals") "$Label aiWorkflow.generatedGameQualityRubric.sampleReports"
        Assert-JsonQualityRubricString $report.id "$Label quality rubric sampleReports.id"
        if (@("headless-assertion", "package-assertion") -notcontains [string]$report.kind) {
            Write-Error "$Label quality rubric sample report '$($report.id)' has unsupported kind: $($report.kind)"
        }
        if ([string]$report.path -notlike "games/$($Game.target)/reports/quality/*.json") {
            Write-Error "$Label quality rubric sample report '$($report.id)' path must stay under games/$($Game.target)/reports/quality"
        }
        Assert-JsonQualityRubricStringArray $report.gateResultIds "$Label quality rubric sample report '$($report.id)'.gateResultIds"
        Assert-JsonQualityRubricStringArray $report.requiredSignals "$Label quality rubric sample report '$($report.id)'.requiredSignals"
        foreach ($gateResultId in @($report.gateResultIds)) {
            if (-not $gateResultIds.ContainsKey([string]$gateResultId)) {
                Write-Error "$Label quality rubric sample report '$($report.id)' references unknown gate result id: $gateResultId"
            }
        }
        $coveredReportKinds[[string]$report.kind] = $true
    }
    foreach ($requiredReportKind in @("headless-assertion", "package-assertion")) {
        if (-not $coveredReportKinds.ContainsKey($requiredReportKind)) {
            Write-Error "$Label aiWorkflow.generatedGameQualityRubric missing sample report kind: $requiredReportKind"
        }
    }

    $unsupportedRowsByClaim = @{}
    foreach ($row in @($rubric.unsupportedRows)) {
        Assert-Properties $row @("id", "claimId", "reason", "escalation") "$Label aiWorkflow.generatedGameQualityRubric.unsupportedRows"
        Assert-JsonQualityRubricString $row.id "$Label quality rubric unsupportedRows.id"
        Assert-JsonQualityRubricString $row.claimId "$Label quality rubric unsupportedRows.claimId"
        Assert-JsonQualityRubricString $row.reason "$Label quality rubric unsupportedRows.reason"
        Assert-JsonQualityRubricString $row.escalation "$Label quality rubric unsupportedRows.escalation"
        $unsupportedRowsByClaim[[string]$row.claimId] = $true
    }

    foreach ($unsupportedClaim in @(
            "subjective-fun",
            "commercial-quality",
            "platform-parity",
            "unbounded-performance",
            "unreviewed-content",
            "autonomous-balancing",
            "native-handles",
            "validation-weakening",
            "broad-production-readiness"
        )) {
        if (@($rubric.unsupportedClaims) -notcontains $unsupportedClaim) {
            Write-Error "$Label aiWorkflow.generatedGameQualityRubric unsupportedClaims missing $unsupportedClaim"
        }
        if (-not $unsupportedRowsByClaim.ContainsKey($unsupportedClaim)) {
            Write-Error "$Label aiWorkflow.generatedGameQualityRubric unsupportedRows missing $unsupportedClaim"
        }
    }
}

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    $requiresQualityRubric = @("2d-desktop-runtime-package", "3d-playable-desktop-package") -contains $game.gameplayContract.productionRecipe
    Assert-JsonGeneratedGameQualityRubric $game $relative $requiresQualityRubric
}
