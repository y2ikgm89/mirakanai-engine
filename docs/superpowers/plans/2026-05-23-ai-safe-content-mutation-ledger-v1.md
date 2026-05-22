# AI Safe Content Mutation Ledger v1 (2026-05-23)

**Plan ID:** `ai-safe-content-mutation-ledger-v1`
**Status:** Completed.
**Capability row:** `ai-safe-content-mutation-ledger-v1`

## Goal

Add a fail-closed per-game `game.agent.json.aiWorkflow.contentMutationLedger` contract for generated 2D/3D package games so AI game-creation work records game-owned write scopes, reviewed mutation surfaces, generated files, forbidden shared paths, and remediation actions before editing content.

## Context

- The production master plan is active with `unsupportedProductionGaps = []` and `recommendedNextPlan.id = next-production-gap-selection`.
- `ai-game-design-spec-v1` and `ai-game-generation-orchestrator-v1` are implemented, so generated 2D/3D package scaffolds now have reviewed design input and deterministic creation.
- The canonical backlog marks `ai-safe-content-mutation-ledger-v1` as the next AI game-creation foundational unblocker: per-game ledger rows, schema/static checks, and failure on AI mutation-surface drift.

## Constraints

- Keep the ledger descriptor-only and game-surface-only. It must not execute commands, apply fixes, mutate cooked packages, or grant engine-internal write authority.
- Require all mutation paths to stay under `games/<game_name>/` unless a separate developer-owned engine task changes shared surfaces.
- Preserve clean breaking behavior: no compatibility aliases, duplicate legacy fields, or broad permissive fallback for manifests missing this ledger on generated 2D/3D recipes.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/*.ps1` entrypoints only.
- Update schema, static checks, generated scaffolds, docs, skills, manifest fragments, and the composed manifest because this changes the durable AI game workflow.

## Official Practice Check

- OpenAI Codex official docs expose Codex configuration, permissions, rules, AGENTS.md, worktrees, and subagents as explicit workflow surfaces: <https://developers.openai.com/codex/cli>. This slice records game-local mutation boundaries in repository data instead of relying on implicit agent behavior.
- Claude Code settings docs define project-scoped `.claude/settings.json` and permission settings as shared repository configuration: <https://code.claude.com/docs/en/settings>. This slice does not broaden those permissions.
- Claude Code subagent docs describe read-only and write-capable tool scoping and subagent tool restrictions: <https://code.claude.com/docs/en/sub-agents>. This slice keeps reviewer/explorer behavior read-only and makes game mutation scopes explicit for future write-capable game workers.

## Phase Checklist

- [x] Select `ai-safe-content-mutation-ledger-v1` from the developer-owned AI game-creation backlog.
- [x] Add a failing static contract requiring `aiWorkflow.contentMutationLedger` for generated 2D/3D package manifests and scaffold outputs.
- [x] Add the schema and static validation for ledger rows, cross-references, game-local paths, reviewed command surfaces, forbidden shared paths, and remediation actions.
- [x] Update 2D/3D generated scaffolds and committed generated/sample manifests with descriptor-only ledger rows.
- [x] Update docs, game-development skills, manifest fragments, and composed manifest without broadening engine readiness claims.
- [x] Run focused static validation and then full `tools/validate.ps1`.
- [x] Close this plan by marking it Completed, returning `currentActivePlan` to the master plan, and keeping `recommendedNextPlan.id = next-production-gap-selection`.

## Done When

- Generated 2D/3D package manifests fail static validation without a complete `contentMutationLedger`.
- Ledger rows name AI-owned game-local roots, generated files, reviewed command surfaces, forbidden shared paths, and remediation actions with no absolute paths, parent traversal, command chaining, native/backend contract leaks, or shared-surface write grants.
- `tools/new-game.ps1` and `tools/create-game-recipe.ps1` scaffold 2D/3D package games with the ledger and deterministic dry-run/apply evidence.
- Durable docs, manifest fragments, and Codex/Claude/Cursor skills describe the supported boundary without claiming arbitrary shell execution, engine-internal edits, cooked-package mutation, external asset generation, or broad generated-game quality.
- Validation evidence is recorded below.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Expected FAIL | RED check failed on missing `games/sample_2d_desktop_runtime_package/game.agent.json aiWorkflow.contentMutationLedger`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Proved schema/static ledger rows, cross-references, game-local path checks, generated file rows, forbidden shared paths, remediation rows, and manifest compose sync. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Proved scaffold 2D/3D content mutation ledger rows and agent-surface needles. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Proved Codex/Claude/Cursor game-development skill parity and script surface budgets. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Proved tracked text/tool formatting after normalizing generated JSON line endings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full static/build/test validation passed; Apple/Metal host blockers remained diagnostic or host-gated only. |
