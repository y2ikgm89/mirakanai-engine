#requires -Version 7.0
#requires -PSEdition Core

# Chapter 91 for check-ai-integration.ps1 AI Codex/Claude/Cursor agent-surface alignment contracts.

$agentSurfaceAlignmentChecks = @(
    @{
        Path = "engine/agent/manifest.json"
        Needles = @("aiSurfaces", "crossToolAlignment", "ai-codex-claude-agent-surface-v1", "openai-codex-agents-md", "anthropic-claude-code-settings", "anthropic-claude-code-subagents", "cursor-rules-agents-md", "game-local source assets", "tools/create-game-recipe.ps1", "tools/run-validation-recipe.ps1", "direct default-branch push", "arbitrary shell or raw manifest command evaluation", "backend/native handle public game API", "validation weakening")
    },
    @{
        Path = "engine/agent/manifest.fragments/011-aiSurfaces.json"
        Needles = @("crossToolAlignment", "officialDocs", "gameOwnedWriteScopes", "reviewedCommandSurfaces", "forbiddenBroadGrants", "unsupportedClaims")
    },
    @{
        Path = "schemas/engine-agent.schema.json"
        Needles = @("crossToolAlignment", "ai-codex-claude-agent-surface-v1", "officialDocs", "toolSurfaces", "forbiddenBroadGrants")
    },
    @{
        Path = "tools/check-json-contracts-066-agent-surface-alignment.ps1"
        Needles = @("crossToolAlignment", "openai-codex-agents-md", "tools/check-ai-integration.ps1", "raw gh pr ready", "backend/native handle public game API")
    },
    @{
        Path = "docs/ai-integration.md"
        Needles = @("AI Codex/Claude/Cursor Agent Surface v1", "aiSurfaces.crossToolAlignment", "games/<game_name>/", "tools/create-game-recipe.ps1", "gh pr merge --auto --merge --match-head-commit <headRefOid>", "direct default-branch push", "arbitrary shell or raw manifest command evaluation", "backend/native handle public game API", "validation weakening", "model: composer-2.5-fast", "500-line rule guidance", "OpenAI developer docs MCP", "Close completed, obsolete, or no-longer-needed Codex subagents promptly after consuming their results")
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @("AI Codex/Claude/Cursor Agent Surface v1", "aiSurfaces.crossToolAlignment", "game-owned write scopes", "forbidden broad grants", "broad production readiness")
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @("AI Codex/Claude/Cursor Agent Surface v1", "aiSurfaces.crossToolAlignment", "required read-only roles", "native handle public API leakage")
    },
    @{
        Path = "docs/superpowers/plans/2026-05-23-ai-codex-claude-agent-surface-v1.md"
        Needles = @("ai-codex-claude-agent-surface-v1", "Official docs reviewed", "OpenAI Codex AGENTS.md guide", "Claude Code settings and permissions", "Cursor rules and AGENTS.md", 'no `*.cpp` or `*.hpp` implementation is expected')
    },
    @{
        Path = "docs/superpowers/plans/README.md"
        Needles = @("AI Codex/Claude/Cursor Agent Surface v1", "2026-05-23-ai-codex-claude-agent-surface-v1.md")
    },
    @{
        Path = "AGENTS.md"
        Needles = @("close completed/obsolete/no-longer-needed agents promptly after their result is consumed", "before spawning replacements")
    },
    @{
        Path = "docs/agent-operational-reference.md"
        Needles = @("Close completed, obsolete, or no-longer-needed agents promptly after consuming their results", "do not wait for the session to approach its subagent limit")
    },
    @{
        Path = "docs/workflows.md"
        Needles = @("Close completed, obsolete, or no-longer-needed delegated agents promptly after consuming their final output", "do not wait for the subagent limit to force cleanup")
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
        Needles = @("ai-codex-claude-agent-surface-v1", "implemented-1x-foundation", "aiSurfaces.crossToolAlignment", "official docs anchors")
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md"
        Needles = @("Agent-surface alignment", "ai-codex-claude-agent-surface-v1", "aiSurfaces.crossToolAlignment", "ai-validation-remediation-recipes-v1", "cross-tool agent-surface alignment")
    },
    @{
        Path = ".agents/skills/gameengine-agent-integration/SKILL.md"
        Needles = @("aiSurfaces.crossToolAlignment", "AI Codex/Claude/Cursor Agent Surface v1", "forbidden broad grants", ".codex/rules", ".claude/settings.json", "after their results are consumed")
    },
    @{
        Path = ".claude/skills/gameengine-agent-integration/SKILL.md"
        Needles = @("aiSurfaces.crossToolAlignment", "AI Codex/Claude/Cursor Agent Surface v1", "forbidden broad grants", ".codex/rules", ".claude/settings.json", "after their results are consumed")
    },
    @{
        Path = ".cursor/skills/gameengine-agent-integration/SKILL.md"
        Needles = @("aiSurfaces.crossToolAlignment", "AI Codex/Claude/Cursor Agent Surface v1", "forbidden broad grants", ".codex/rules", ".claude/settings.json", "after their results are consumed")
    },
    @{
        Path = ".codex/rules/gameengine.rules"
        Needles = @("Direct pushes to default or protected branches", "gh pr merge --auto --merge --match-head-commit", "branch cleanup stays in tools/remove-merged-worktree.ps1", "gh pr ready")
    },
    @{
        Path = ".claude/settings.json"
        Needles = @("gh pr merge --auto --merge --match-head-commit", "tools/remove-merged-worktree.ps1", "Bash(git push origin main:*)", "Bash(gh pr ready:*)")
    },
    @{
        Path = ".cursor/rules/mirakana-repository-baseline.mdc"
        Needles = @("AGENTS.md", ".cursor/skills/", "tools/*.ps1", "never push directly to the default branch", "500-line budget")
    },
    @{
        Path = ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
        Needles = @("model: composer-2.5-fast", "500-line rule budget", "Cursor subagent frontmatter")
    },
    @{
        Path = "tools/check-agents.ps1"
        Needles = @('Cursor rule exceeds official 500-line guidance', 'model: ${RequiredModel}', 'composer-2.5-fast')
    }
)

foreach ($check in $agentSurfaceAlignmentChecks) {
    $agentSurfaceAlignmentText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $agentSurfaceAlignmentText $needle $check.Path
    }
}
