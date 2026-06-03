# AI Game Generation Orchestrator v1 (2026-05-23)

**Plan ID:** `ai-game-generation-orchestrator-v1`
**Status:** Completed.
**Capability row:** `ai-game-generation-orchestrator-v1`

## Goal

Implement a reviewed, deterministic dry-run/apply path that starts from a fail-closed `aiWorkflow.gameDesignSpec` row and creates a game-owned 2D or 3D generated package scaffold through the supported `tools/new-game.ps1` workflow.

## Context

- The production master plan is active with `unsupportedProductionGaps = []` and `recommendedNextPlan.id = next-production-gap-selection`.
- `ai-game-design-spec-v1` is implemented and gives generated 2D/3D package work a structured design descriptor.
- The canonical backlog marks `ai-game-generation-orchestrator-v1` as the next AI game-creation foundational unblocker: reviewed dry-run/apply flow, deterministic file lists, generated 2D/3D package evidence, and no arbitrary shell execution.

## Constraints

- Keep the surface reviewed and narrow: `DesktopRuntime2DPackage` and `DesktopRuntime3DPackage` only for this foundation slice.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/*.ps1` entrypoints only; do not introduce raw shell execution or free-form manifest command evaluation.
- Keep all generated changes under `games/<game_name>/` plus the existing `games/CMakeLists.txt` registration path that `tools/new-game.ps1` already owns.
- Preserve first-party/backend-neutral public contracts; do not expose native handles, SDL3, Dear ImGui, renderer/RHI internals, backend terms, or middleware types as game-facing API.
- Update docs, manifest fragments, static checks, and game-development skills because this changes the durable AI game workflow.

## Official Practice Check

- OpenAI Codex CLI official docs describe Codex as a local coding agent that can read, change, and run code in a selected directory, and its workflow surfaces include local code review, subagents, web search, cloud tasks, MCP, scripting, and approval modes: <https://developers.openai.com/codex/cli>.
- Anthropic Claude Code official docs describe subagents as task-specific assistants with separate context windows and configurable tool access; project subagents live under `.claude/agents/` and can be shared with a repository: <https://docs.anthropic.com/en/docs/claude-code/sub-agents>.
- Anthropic Claude Code settings docs state that `settings.json` is the official mechanism for configuring permissions, environment variables, and tool behavior; this slice does not broaden Claude permissions or add new arbitrary execution rights: <https://docs.anthropic.com/en/docs/claude-code/settings>.
- Repository AI/operator command-surface guidance requires typed dry-run/apply rows, deterministic diagnostics, host-gate acknowledgement where needed, and manifest/static-check synchronization. This plan implements one reviewed PowerShell orchestrator over the existing `tools/new-game.ps1`; it does not evaluate raw manifest commands.

## Phase Checklist

- [x] Select `ai-game-generation-orchestrator-v1` from the developer-owned AI game-creation backlog.
- [x] Add a failing static/scaffold check requiring a ready `create-game-recipe` command surface and `tools/create-game-recipe.ps1` dry-run/apply behavior.
- [x] Implement `tools/create-game-recipe.ps1` as a reviewed wrapper over `tools/new-game.ps1` with JSON result rows, fail-closed design-spec validation, deterministic changed-file output, and no arbitrary shell execution.
- [x] Promote the manifest `create-game-recipe` command surface to ready dry-run/apply and compose `engine/agent/manifest.json` from fragments.
- [x] Update docs/current capabilities/game-development skills/static integration needles for the supported boundary.
- [x] Run focused static/script validation and then full `tools/validate.ps1`.
- [x] Close this plan by marking it Completed, returning `currentActivePlan` to the master plan, and keeping `recommendedNextPlan.id = next-production-gap-selection`.

## Done When

- `tools/create-game-recipe.ps1 -Mode DryRun` validates a design spec, reports deterministic planned files/recipes, and does not write files.
- `tools/create-game-recipe.ps1 -Mode Apply` requires the same reviewed design spec, calls `tools/new-game.ps1` directly with fixed arguments, writes only the generated game scaffold and `games/CMakeLists.txt`, then replaces the generated manifest design spec with the reviewed input spec.
- Static checks prove 2D and 3D dry-run/apply behavior, no broad shell execution, and command-surface drift alignment.
- Durable docs, manifest fragments, and Codex/Claude/Cursor skills describe the supported boundary without claiming autonomous game design, arbitrary asset generation, package cooking beyond scaffold output, broad quality, or native/backend API access.
- Validation evidence is recorded below.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Expected FAIL | RED check failed on missing `tools/create-game-recipe.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Proved 2D/3D create-game-recipe dry-run/apply scaffold smokes and agent-surface needles. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Proved manifest command-surface contract, apply-ready allowlist, and JSON schema/static contract synchronization. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Proved Codex/Claude/Cursor game-development skill surface parity. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Proved tracked text/tool formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full static/build/test validation passed; host-gated Apple/Metal diagnostics remained diagnostic-only. |
