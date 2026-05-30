#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9 for check-ai-integration.ps1 generated-game quality rubric contracts.

$generatedGameQualityRubricChecks = @(
    @{
        Path = "schemas/game-agent.schema.json"
        Needles = @("generatedGameQualityRubric", "ai-generated-game-quality-rubric-v1", "deterministic-package-smoke", "budget-evidence", "unsupportedRows")
    },
    @{
        Path = "tools/check-json-contracts-065-generated-game-quality-rubric.ps1"
        Needles = @("Assert-JsonGeneratedGameQualityRubric", "aiWorkflow.generatedGameQualityRubric", "subjective-fun", "broad-production-readiness")
    },
    @{
        Path = "tools/new-game-templates.ps1"
        Needles = @("New-AiGeneratedGameQualityRubric", "generatedGameQualityRubric = New-AiGeneratedGameQualityRubric", "objective-quality-gate", "package-quality-report")
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @("Generated Game Quality Rubric", "game.agent.json.aiWorkflow.generatedGameQualityRubric", "deterministic package smoke", "broad production readiness")
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @("AI Generated Game Quality Rubric v1", "game.agent.json.aiWorkflow.generatedGameQualityRubric", "objective, controls, feedback")
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
        Needles = @("ai-generated-game-quality-rubric-v1", "implemented-1x-foundation", "game.agent.json.aiWorkflow.generatedGameQualityRubric")
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @("ai-generated-game-quality-rubric-v1", "aiWorkflow.generatedGameQualityRubric", "objective, controls, feedback")
    },
    @{
        Path = ".agents/skills/gameengine-game-development/SKILL.md"
        Needles = @("aiWorkflow.generatedGameQualityRubric", "subjective fun", "broad production readiness")
    },
    @{
        Path = ".claude/skills/gameengine-game-development/SKILL.md"
        Needles = @("aiWorkflow.generatedGameQualityRubric", "subjective fun", "broad production readiness")
    }
)

foreach ($check in $generatedGameQualityRubricChecks) {
    $qualityRubricText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $qualityRubricText $needle $check.Path
    }
}
