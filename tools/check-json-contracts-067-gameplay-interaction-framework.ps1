#requires -Version 7.0
#requires -PSEdition Core

# Chapter 6.7 for check-json-contracts.ps1 gameplay interaction framework contracts.

function Assert-JsonGameplayInteractionSignal {
    param(
        [Parameter(Mandatory)]$Values,
        [Parameter(Mandatory)][string]$Needle,
        [Parameter(Mandatory)][string]$Label
    )

    if (@($Values) -notcontains $Needle) {
        Write-Error "$Label missing required gameplay interaction signal: $Needle"
    }
}

function Get-JsonGameplayInteractionQualityGate {
    param(
        [Parameter(Mandatory)]$Game,
        [Parameter(Mandatory)][string]$GateId,
        [Parameter(Mandatory)][string]$Label
    )

    $matchingGates = @($Game.aiWorkflow.generatedGameQualityRubric.gateResults | Where-Object { $_.id -eq $GateId })
    if ($matchingGates.Count -ne 1) {
        Write-Error "$Label aiWorkflow.generatedGameQualityRubric must contain exactly one gate '$GateId'"
        return $null
    }

    return $matchingGates[0]
}

function Assert-JsonGameplayInteractionQualityRubric {
    param(
        [Parameter(Mandatory)][string]$RelativePath
    )

    $path = Join-Path $root $RelativePath
    $game = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $feedbackGate = Get-JsonGameplayInteractionQualityGate $game "feedback-quality-gate" $RelativePath
    $restartGate = Get-JsonGameplayInteractionQualityGate $game "fail-restart-quality-gate" $RelativePath
    if ($null -eq $feedbackGate -or $null -eq $restartGate) {
        return
    }

    Assert-JsonGameplayInteractionSignal $feedbackGate.requiredSignals "gameplay_systems_interaction_ready=1" $RelativePath
    Assert-JsonGameplayInteractionSignal $feedbackGate.requiredSignals "gameplay_systems_interaction_feedback_rows=10" $RelativePath
    Assert-JsonGameplayInteractionSignal $feedbackGate.requiredSignals "gameplay_systems_interaction_final_session_state=running" $RelativePath
    Assert-JsonGameplayInteractionSignal $restartGate.requiredSignals "gameplay_systems_interaction_ready=1" $RelativePath
    Assert-JsonGameplayInteractionSignal $restartGate.requiredSignals "gameplay_systems_interaction_rows=10" $RelativePath
    Assert-JsonGameplayInteractionSignal $restartGate.requiredSignals "gameplay_systems_interaction_final_session_state=running" $RelativePath
}

Assert-JsonGameplayInteractionQualityRubric "games/sample_2d_desktop_runtime_package/game.agent.json"
Assert-JsonGameplayInteractionQualityRubric "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
