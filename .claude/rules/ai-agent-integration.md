---
paths:
  - "AGENTS.md"
  - "CLAUDE.md"
  - ".agents/**"
  - ".codex/**"
  - ".claude/**"
  - ".cursor/**"
  - ".clangd"
  - "engine/agent/**"
  - "schemas/**"
  - "tools/agent-context.ps1"
  - "tools/compose-agent-manifest.ps1"
  - "tools/check-agents.ps1"
  - "tools/check-ai-integration*.ps1"
  - "tools/check-json-contracts*.ps1"
  - "tools/check-publication-preflight.ps1"
  - "tools/post-merge-task-cleanup.ps1"
  - "tools/remove-merged-worktree.ps1"
  - "tools/ready-task-pr.ps1"
  - "tools/static-contract-ledger.ps1"
  - "docs/workflows.md"
  - "docs/ai-integration.md"
  - "docs/agent-operational-reference.md"
---

# AI Agent Integration Rules

This is a startup-loaded router for Claude Code memory imports. Keep it short; put detailed workflow prose in `AGENTS.md`, `docs/workflows.md`, `docs/ai-integration.md`, `docs/agent-operational-reference.md`, or the `gameengine-agent-integration` skill.

## Authority

- `AGENTS.md` is the shared baseline for Codex, Claude Code, and Cursor. Resolve conflicts in favor of `AGENTS.md`.
- `CLAUDE.md` imports `AGENTS.md`, this file, and `.claude/rules/cpp-engine.md` with official memory imports; do not rely on undocumented automatic `.claude/rules/` loading.
- Keep always-loaded guidance specific, concise, verifiable, and durable. Do not put personal preferences, credentials, API keys, MCP connection state, stale status snapshots, or machine-local paths in tracked instructions.
- Reusable workflows belong in `.agents/skills/`, `.claude/skills/`, and `.cursor/skills/`; path-scoped guidance belongs in rules; specialized roles belong in subagents; machine-readable capability/status claims belong in `engine/agent/manifest.fragments/*.json` plus composed `engine/agent/manifest.json`.

## Drift And Manifest

- Every implementation change needs a targeted agent-surface drift check before completion.
- If durable behavior, workflow, validation, packaging, permissions, tool expectations, or AI-operable claims drift, update the owning `AGENTS.md`, `CLAUDE.md`, docs, skills, rules, settings, subagents, manifest fragments, schemas, and static checks in the same task.
- Do not broad-load every surface when no durable guidance changed. Use targeted reads and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal|Standard`.
- Edit `engine/agent/manifest.fragments/*.json`, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`; never hand-edit `engine/agent/manifest.json`.
- For static contract ownership, use `tools/static-contract-ledger.ps1` and the chapter guidance in `.agents/skills/gameengine-agent-integration/references/static-contract-chapters.md`.

## Permissions And Publishing

- `.codex/rules` and `.claude/settings.json` are narrow command/permission gates, not troubleshooting playbooks. Do not broaden them to save time.
- Follow official GitHub Flow from `AGENTS.md` and `docs/workflows.md`: validated checkpoint commits, topic-branch pushes, one focused PR, guarded draft-to-ready, and auto-merge registration only with the current `headRefOid`.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` before staging, push, PR creation/update, ready conversion, merge, or cleanup. If `publication-preflight: blocked` appears, switch session/host context instead of bypassing GitHub Flow.
- Publication temp clones are not the supported path. Use the task worktree so Git admin state, network, `gh auth status`, and PR head SHA evidence match the branch being published.
- Use `tools/ready-task-pr.ps1` instead of raw `gh pr ready`; use `gh pr merge --auto --merge --match-head-commit <headRefOid>` instead of immediate merge; keep `--delete-branch` out of linked-worktree auto-merge commands.
- Prefer `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/post-merge-task-cleanup.ps1` for guarded post-merge cleanup; `tools/remove-merged-worktree.ps1` remains the lower-level entrypoint. Raw worktree cleanup stays reviewed.
- Git/GitHub authentication stays host-local. Do not add repository requirements for `GITHUB_TOKEN`, personal access tokens, or checked-in credential helper overrides.

## Cross-Tool Surfaces

- Keep Codex, Claude Code, and Cursor behaviorally equivalent when shared workflows change.
- Game naming guidance must stay synchronized: `game_name` and `new-game -Name` values match `^[a-z][a-z0-9_]*$`; source-tree paths stay lowercase snake_case; JSON manifest IDs may use ecosystem formats such as kebab-case.
- Codex: `AGENTS.md`, `.agents/skills/`, `.codex/agents/`, `.codex/rules`.
- Claude Code: `CLAUDE.md`, `.claude/settings.json`, `.claude/skills/`, `.claude/agents/`, and this startup-loaded rule router.
- Cursor: `AGENTS.md`, `.cursor/rules/*.mdc`, `.cursor/skills/`, and `.cursor/agents/`. Keep Cursor `gameengine-*` skills thin except `gameengine-cursor-baseline` and `gameengine-plan-registry`.
- Read-only review, exploration, architecture, planning, rendering, and agent-surface audit subagents must stay read-only. Write-capable tools belong only on builder/fixer roles expected to change files.

## Documentation Sources

- Use Context7 for live library, SDK, build-system, and toolchain docs.
- Use the OpenAI developer documentation MCP or official OpenAI documentation for Codex/OpenAI behavior.
- Use official Anthropic docs for Claude Code memory, settings, permissions, hooks, skills, and subagents.
- Use official Cursor docs for Cursor rules, Agent Skills, and project subagents.

## Validation

- For agent-only changes, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- Use `docs/workflows.md` for the Repository consistency checklist and PR validation cost policy. Use `.claude/rules/cpp-engine.md` for C++/CMake/toolchain details such as `check-toolchain`, `prepare-worktree`, `VCPKG_MANIFEST_INSTALL=OFF`, and Windows diagnostics.
