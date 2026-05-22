# AI Codex/Claude/Cursor Agent Surface v1 - 2026-05-23

**Plan ID:** `ai-codex-claude-agent-surface-v1`
**Status:** Completed.
**Backlog row:** `ai-codex-claude-agent-surface-v1`
**Classification:** Standard

## Goal

Make Codex, Claude Code, and Cursor project surfaces share one machine-readable production alignment contract for AI-authored game work: game-owned write scopes, reviewed tool surfaces, validation guards, and forbidden broad permission grants.

## Context

The developer-owned backlog marks this as a foundational unblocker for AI-authored games. The current repository already has per-tool instruction roots, skills, agents, Codex rules, Claude settings, and Cursor rules, but the composed engine manifest does not yet expose the cross-tool alignment as a schema-checked contract.

Official docs reviewed:

- OpenAI Codex AGENTS.md guide: https://developers.openai.com/codex/guides/agents-md
- Claude Code settings and permissions: https://docs.anthropic.com/en/docs/claude-code/settings
- Claude Code subagents: https://docs.anthropic.com/en/docs/claude-code/sub-agents
- Cursor rules and AGENTS.md: https://docs.cursor.com/en/context

## Constraints

- Do not broaden `.codex/rules` or `.claude/settings.json` permissions.
- Keep `engine/agent/manifest.json` compose-only; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.
- Keep the contract backend-neutral and game-facing; do not expose native handles, renderer/RHI internals, SDL3, Dear ImGui, or middleware types as public game APIs.
- Keep generated-game automation on reviewed `tools/*.ps1` surfaces and selected validation recipes; no arbitrary shell or raw manifest command evaluation.
- This slice changes AI contracts/static guards/docs only; no `*.cpp` or `*.hpp` implementation is expected.

## Done When

- `engine/agent/manifest.json.aiSurfaces.crossToolAlignment` is composed from fragments and schema/static-guarded.
- Docs and skills describe the cross-tool alignment contract and official docs anchors.
- The backlog row is marked `implemented-1x-foundation` without claiming broad autonomous game creation readiness.
- Focused JSON/AI integration checks and final `tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Plan

1. Add a failing static guard for the missing `aiSurfaces.crossToolAlignment` contract.
2. Add the manifest fragment contract and schema shape, then compose the manifest.
3. Sync docs, skills, backlog/projection text, and static integration needles.
4. Run focused validation, final validation, and publish through GitHub Flow.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | RED first failed on missing `aiSurfaces.crossToolAlignment`; green after manifest/schema implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent surface drift guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Skill/rule/subagent parity and budgets. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Text/static formatting guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Slice-closing validation passed after closeout updates; Apple/Metal/Android host diagnostics remained expected host-gated evidence notes. |
