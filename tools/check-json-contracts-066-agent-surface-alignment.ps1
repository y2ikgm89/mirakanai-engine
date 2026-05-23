#requires -Version 7.0
#requires -PSEdition Core

# Chapter 66 for check-json-contracts.ps1 AI agent-surface alignment contracts.

$engine = Read-Json "engine/agent/manifest.json"
$engineAgentSchema = Read-Json "schemas/engine-agent.schema.json"

Assert-Properties $engine.aiSurfaces @("crossToolAlignment") "engine manifest aiSurfaces"
$alignment = $engine.aiSurfaces.crossToolAlignment
Assert-Properties $alignment @(
    "schemaVersion",
    "capabilityId",
    "status",
    "officialDocs",
    "toolSurfaces",
    "gameOwnedWriteScopes",
    "sharedSurfaceWritePolicy",
    "reviewedCommandSurfaces",
    "validationGuards",
    "forbiddenBroadGrants",
    "requiredReadOnlyRoles",
    "unsupportedClaims"
) "engine manifest aiSurfaces.crossToolAlignment"

if ($alignment.schemaVersion -ne 1) {
    Write-Error "engine manifest aiSurfaces.crossToolAlignment.schemaVersion must be 1"
}
if ($alignment.capabilityId -ne "ai-codex-claude-agent-surface-v1") {
    Write-Error "engine manifest aiSurfaces.crossToolAlignment.capabilityId must be ai-codex-claude-agent-surface-v1"
}
if ($alignment.status -ne "implemented-1x-foundation") {
    Write-Error "engine manifest aiSurfaces.crossToolAlignment.status must be implemented-1x-foundation"
}

$officialDocIds = @($alignment.officialDocs | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "url", "appliesTo") "engine manifest aiSurfaces.crossToolAlignment officialDocs"
        [string]$_.id
    })
foreach ($requiredDocId in @(
        "openai-codex-agents-md",
        "anthropic-claude-code-settings",
        "anthropic-claude-code-subagents",
        "cursor-rules-agents-md"
    )) {
    if ($officialDocIds -notcontains $requiredDocId) {
        Write-Error "engine manifest aiSurfaces.crossToolAlignment officialDocs missing: $requiredDocId"
    }
}

$toolSurfaceIds = @($alignment.toolSurfaces | ForEach-Object {
        Assert-Properties $_ @("id", "instructions", "skills", "agents", "writeScope", "validationGuards") "engine manifest aiSurfaces.crossToolAlignment toolSurfaces"
        [string]$_.id
    })
foreach ($requiredToolId in @("codex", "claudeCode", "cursor")) {
    if ($toolSurfaceIds -notcontains $requiredToolId) {
        Write-Error "engine manifest aiSurfaces.crossToolAlignment toolSurfaces missing: $requiredToolId"
    }
}

$joinedAlignmentText = ($alignment | ConvertTo-Json -Depth 20)
foreach ($needle in @(
        "games/<game_name>/",
        "tools/create-game-recipe.ps1",
        "tools/run-validation-recipe.ps1",
        "tools/check-agents.ps1",
        "tools/check-ai-integration.ps1",
        "tools/check-json-contracts.ps1",
        "gh pr merge --auto --merge --match-head-commit <headRefOid>",
        "direct default-branch push",
        "raw gh pr ready",
        "arbitrary shell or raw manifest command evaluation",
        "backend/native handle public game API",
        "validation weakening"
    )) {
    Assert-ContainsText $joinedAlignmentText $needle "engine manifest aiSurfaces.crossToolAlignment"
}

foreach ($role in @("agent-surface-auditor", "explorer", "cpp-reviewer", "engine-architect", "planning-auditor", "rendering-auditor")) {
    if (@($alignment.requiredReadOnlyRoles) -notcontains $role) {
        Write-Error "engine manifest aiSurfaces.crossToolAlignment.requiredReadOnlyRoles missing: $role"
    }
}

Assert-Properties $engineAgentSchema.properties.aiSurfaces @("required", "properties") "schemas/engine-agent.schema.json aiSurfaces"
if (@($engineAgentSchema.properties.aiSurfaces.required) -notcontains "crossToolAlignment") {
    Write-Error "schemas/engine-agent.schema.json aiSurfaces.required must include crossToolAlignment"
}
Assert-Properties $engineAgentSchema.properties.aiSurfaces.properties @("crossToolAlignment") "schemas/engine-agent.schema.json aiSurfaces.properties"
