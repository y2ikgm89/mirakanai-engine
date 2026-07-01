#requires -Version 7.0
#requires -PSEdition Core

# Chapter 153 for check-ai-integration.ps1 static contracts.
# 2D Commercial Production Excellence v1 closeout pointer and docs alignment.

function Assert-2DCommercialCloseoutProductionLoopPointer {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    $productionLoopDocument = $Text | ConvertFrom-Json -Depth 100
    $productionLoop = $productionLoopDocument.aiOperableProductionLoop
    $expectedMasterPlanPath = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
    if ($productionLoop.currentActivePlan -ne $expectedMasterPlanPath) {
        Write-Error "$Label currentActivePlan must return to the production-completion master plan"
    }

    if ($productionLoop.recommendedNextPlan.id -ne "next-production-gap-selection") {
        Write-Error "$Label recommendedNextPlan.id must return to next-production-gap-selection"
    }

    if ($productionLoop.recommendedNextPlan.status -ne "selection-gate") {
        Write-Error "$Label recommendedNextPlan.status must be selection-gate"
    }

    if ($productionLoop.recommendedNextPlan.path -ne $expectedMasterPlanPath) {
        Write-Error "$Label recommendedNextPlan.path must point at the production-completion master plan"
    }

    if (@($productionLoop.unsupportedProductionGaps).Count -ne 0) {
        Write-Error "$Label unsupportedProductionGaps must remain empty after 2D commercial closeout"
    }

    Assert-ContainsText $Text "twoDCommercialProductionExcellenceCloseoutPhase10Evidence" $Label
    Assert-ContainsText $Text "2D Commercial Production Excellence v1 milestone is closed through Phase 10" $Label
    Assert-ContainsText $Text "PR #928" $Label
    Assert-ContainsText $Text "389af051390852fc29606cbedb1987067d16263e" $Label
    Assert-ContainsText $Text "unsupportedProductionGaps remains []" $Label
}

$productionLoopManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$composedManifest = Get-AgentSurfaceText "engine/agent/manifest.json"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"
$planRegistry = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilities = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmap = Get-AgentSurfaceText "docs/roadmap.md"

Assert-2DCommercialCloseoutProductionLoopPointer $productionLoopManifest "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
Assert-2DCommercialCloseoutProductionLoopPointer $composedManifest "engine/agent/manifest.json"

foreach ($needle in @(
        '**Status:** Completed.',
        '- [x] Update `docs/current-capabilities.md`, `docs/roadmap.md`, plan registry, game manifests, validation recipes, schemas, static checks, and manifest fragments only for completed and validated surfaces.',
        '- [x] Compose `engine/agent/manifest.json` if fragments change.',
        '- [x] Run targeted public API, package, JSON, agent-surface, legal/dependency, and static guards.',
        '- [x] Run `tools/validate.ps1` for runtime/build/public-contract changes.',
        '- [x] Record remaining host-gated rows, dependency-gated rows, unsupported broad claims, and recommended next plan.',
        'Phase 10 validation evidence:',
        'PR #928',
        '389af051390852fc29606cbedb1987067d16263e',
        'currentActivePlan` returns to `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`',
        'recommendedNextPlan.id = next-production-gap-selection',
        'unsupportedProductionGaps = []',
        'broad all-feature/all-platform 2D engine readiness',
        'legal approval remains unclaimed'
    )) {
    Assert-ContainsText $planText $needle "2d commercial production excellence closeout plan"
}

foreach ($needle in @(
        '| Selection gate (`currentActivePlan`) | [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md) (`next-production-gap-selection`) | 2D Commercial Production Excellence v1 is completed through Phase 10 closeout',
        '## Recent Completed 2D Commercial Production Excellence Work',
        '2D Commercial Release Legal Gate v1',
        'PR #928',
        '389af051390852fc29606cbedb1987067d16263e',
        '`unsupportedProductionGaps = []`'
    )) {
    Assert-ContainsText $planRegistry $needle "docs/superpowers/plans/README.md 2d commercial closeout"
}

Assert-DoesNotContainText $planRegistry '| Active milestone (`currentActivePlan`) | [2D Commercial Production Excellence v1]' `
    "docs/superpowers/plans/README.md stale 2d commercial active milestone"

foreach ($needle in @(
        'currentActivePlan` points at `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`',
        'recommendedNextPlan.id = next-production-gap-selection',
        '2D Commercial Production Excellence v1 is completed through Phase 10 closeout',
        '2D Commercial Release Legal Gate v1',
        'PR #928',
        '389af051390852fc29606cbedb1987067d16263e',
        'broad all-feature/all-platform 2D engine readiness',
        'legal approval remains unclaimed',
        'Unity/Unreal/Godot compatibility'
    )) {
    Assert-ContainsText ($currentCapabilities + "`n" + $roadmap) $needle "current capabilities and roadmap 2d commercial closeout"
}

Assert-DoesNotContainText $currentCapabilities 'recommendedNextPlan.id = 2d-commercial-production-excellence-v1' `
    "docs/current-capabilities.md stale 2d commercial recommendedNextPlan"
Assert-DoesNotContainText $roadmap 'Live execution is currently on `docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md`' `
    "docs/roadmap.md stale 2d commercial live execution"
