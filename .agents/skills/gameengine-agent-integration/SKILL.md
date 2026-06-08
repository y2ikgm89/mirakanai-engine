---
name: gameengine-agent-integration
description: Keeps Codex, Claude, Cursor, manifests, skills, rules, and validation scripts aligned with the engine agent contract. Use for AGENTS.md, manifest fragments, check-ai-integration needles, or cross-tool agent surfaces.
paths:
  - "AGENTS.md"
  - "CLAUDE.md"
  - "engine/agent/**"
  - "schemas/**"
  - "tools/agent-context.ps1"
  - "tools/check-ai-integration*.ps1"
  - "tools/check-json-contracts*.ps1"
  - "tools/static-contract-ledger.ps1"
  - "tools/check-agents.ps1"
  - "tools/check-text-format.ps1"
  - "tools/check-text-format-contract.ps1"
  - "tools/format-text.ps1"
  - "tools/text-format-core.ps1"
  - "tools/prepare-worktree.ps1"
  - "tools/check-publication-preflight.ps1"
  - "tools/post-merge-task-cleanup.ps1"
  - "tools/remove-merged-worktree.ps1"
  - "tools/ready-task-pr.ps1"
  - "tools/compose-agent-manifest.ps1"
  - ".agents/**"
  - ".claude/**"
  - ".cursor/**"
  - ".codex/**"
  - "docs/workflows.md"
  - "docs/ai-integration.md"
  - "docs/agent-operational-reference.md"
---

# GameEngine Agent Integration

## Scope

Use this skill when changing agent-facing instructions, Codex/Claude/Cursor skills, rules, subagents, manifest fragments, schemas, validation needles, publication workflow, worktree workflow, or AI-operable engine/game contracts.

## Router Rules

- `AGENTS.md` is the shared durable baseline; `CLAUDE.md` imports it for Claude Code, and Cursor uses root `AGENTS.md`, `.cursor/rules/*.mdc`, `.cursor/skills/`, and `.cursor/agents/`.
- Keep tracked instructions concise and durable. Put long procedures in skill-local `references/*.md` or docs, path-specific guidance in rules, specialized behavior in subagents, and machine-readable capability/status claims in `engine/agent/manifest.json`.
- Do not hand-edit `engine/agent/manifest.json`. Edit `engine/agent/manifest.fragments/*.json`, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`; `tools/check-json-contracts.ps1` verifies committed compose output.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal` or `Standard` for targeted engine-agent context. Use `Full` only when the decision needs full manifest-shaped output.
- Every implementation, architecture, workflow, validation, packaging, or toolchain change includes a targeted agent-surface drift check before completion. If durable guidance or AI-operable contracts are stale, update the affected `AGENTS.md`, `CLAUDE.md`, docs, Codex/Claude/Cursor skills, rules, settings, subagents, manifest fragments plus compose output, schemas, validation checks, and tracked `.clangd` in the same task.
- `tools/check-ai-integration.ps1` validates agent-facing contracts; the static-contract ledger lives in `tools/static-contract-ledger.ps1`. Use `references/static-contract-chapters.md` before editing scoped chapter checks.
- `engine/agent/manifest.json.aiSurfaces.crossToolAlignment` is the AI Codex/Claude/Cursor Agent Surface v1 contract for game-owned write scopes, reviewed command surfaces, required read-only roles, forbidden broad grants, unsupported claims, `.codex/rules`, and `.claude/settings.json`.
- `tools/check-agents.ps1` enforces `AGENTS.md` size, selected `SKILL.md` router budgets, Codex/Claude skill twin parity through `claudeToCodexSkillMap`, Cursor thin-pointer skills, subagent budgets, PowerShell script hygiene, UTF-8 without BOM, LF, and final-newline contracts.
- Every tracked `tools/*.ps1` starts with `#requires -Version 7.0` then `#requires -PSEdition Core`, uses `pwsh`, stays PowerShell-parseable, avoids automatic-variable reuse, uses approved verbs, and follows `tools/common.ps1` / `tools/static-contract-common.ps1` ownership.
- Keep `.codex/rules/*.rules` and `.claude/settings.json` narrow command/permission gates. Do not broaden permissions, bypass GitHub Flow through REST/MCP object writes, or weaken prompt-gated operations to save time.
- Publication follows official GitHub Flow: before staging/push/PR/merge, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1`; if `publication-preflight: blocked`, stop with evidence and switch session or host context.
- Use validated phase commits, checkpoint pushes after remote-state inspection, one PR per focused capability/gap-cluster/milestone, early draft PRs, guarded `tools/ready-task-pr.ps1` conversion, `gh pr merge --auto --merge --match-head-commit <headRefOid>` registration only after PR preflight, and guarded `tools/post-merge-task-cleanup.ps1` cleanup (delegates to `tools/remove-merged-worktree.ps1` with branch deletion flags).
- Direct default-branch pushes are forbidden. Force-push, immediate merge, raw `gh pr ready`, raw worktree cleanup, and other PR state changes stay prompt-gated.
- Prefer Codex app Worktree/Handoff or Claude Code `--worktree` / subagent `isolation: worktree` for parallel work. Keep ignored roots, run `tools/prepare-worktree.ps1`, close completed/obsolete/no-longer-needed delegated agents after their results are consumed, and leave useful work running.
- Use official current sources for agent/platform behavior: OpenAI developer documentation MCP or official OpenAI docs for Codex/OpenAI API/OpenAI model questions; official Anthropic documentation for Claude Code memory, settings, permissions, hooks, skills, and subagents; official Cursor docs for Cursor rules, skills, agents, and 500-line rule guidance. Use Context7 MCP for live library, SDK, build-system, and toolchain documentation.
- Game source naming guidance must stay synchronized: `game_name` and `new-game -Name` values match `^[a-z][a-z0-9_]*$`; source-tree game directories and `runtimePackageFiles` path segments stay lowercase snake_case; JSON manifest IDs, display names, and external package identifiers may keep ecosystem formats including kebab-case.
- For production-completion work, drive from `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`, `recommendedNextPlan`, and `unsupportedProductionGaps`; keep `oneDotZeroCloseoutTier` honest in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and compose output.
- Keep `docs/superpowers/plans/README.md` as the plan registry, keep the live plan stack shallow, and prefer one dated capability/gap-cluster/milestone plan. Put phase gates, docs/manifest/static-check sync, validation-only follow-up, cleanup, and checklist substeps inside the active plan.

## Validation

- While iterating, run the smallest relevant checks: usually `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- For repository consistency after agent-surface changes, follow `docs/workflows.md` Repository consistency checklist and finish with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` when the slice touches durable tooling, manifests, workflow, runtime, build, packaging, or public contracts.

## References

- `references/static-contract-chapters.md`: chapter ownership for scoped static-contract files.
- `references/integration-change-matrix.md`: detailed update matrix for manifest, tooling, diagnostics, plans, publication, worktrees, mobile, and generated-game surfaces.
