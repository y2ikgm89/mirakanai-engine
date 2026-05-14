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
  - "tools/prepare-worktree.ps1"
  - "tools/compose-agent-manifest.ps1"
  - ".agents/**"
  - ".claude/**"
  - ".cursor/**"
  - ".codex/**"
  - "docs/workflows.md"
  - "docs/ai-integration.md"
  - "docs/agent-operational-reference.md"
---

# GameEngine agent integration (Cursor)

Full workflow lives in shared skills. Read these canonical files (ASCII paths):

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-agent-integration/SKILL.md` |
| Codex | `.agents/skills/gameengine-agent-integration/SKILL.md` |
| Baseline | `AGENTS.md` |
| Consistency checklist | `docs/workflows.md` (**Repository consistency checklist**) |

`tools/*.ps1` hygiene also includes **UTF-8 without BOM**, contiguous `#requires`, **approved-verb** `function` names, and **PSScriptAnalyzer-friendly** patterns (no automatic-variable reuse such as **`$input` / `$matches`**—including **`foreach ($input in ...)`** and **`$matches = [Regex]::Matches(...)`**—no shadowing **`$Is*`** platform automatics, **`$null =`** for intentional discard, **`Write-Information -InformationAction Continue`** instead of **`Write-Host`** for non-pipeline status, non-empty **`catch`**, **`ShouldProcess`** on host-mutating **`Set-*`** helpers—see checklist step 2 and `AGENTS.md` **Repository command entrypoints**). Run **`Invoke-ScriptAnalyzer`** on edited scripts when the module is installed.

Static contract ledger entrypoints stay thin; `tools/static-contract-ledger.ps1`.

Machine-readable **canonical** contract: `engine/agent/manifest.json` (compose output from `engine/agent/manifest.fragments/` via `tools/compose-agent-manifest.ps1`). New `.claude/skills/gameengine-*` topics require a Codex twin registered in `tools/check-agents.ps1` (`claudeToCodexSkillMap`) and a matching thin `.cursor/skills/gameengine-*` folder unless intentionally Cursor-only (`gameengine-cursor-baseline`, `gameengine-plan-registry`). Keep Codex/Claude/Cursor surfaces aligned when workflow commands change, including Git/GitHub commit, push, merge/delete-branch, auto-merge registration, post-merge remote-tracking cleanup, force-push, and PR publishing gates. Treat Codex command policy as session-scoped: `.codex/rules` edits may need policy reload or a new session before newly allowed commands are available. Agent publishing follows official GitHub Flow: task-owned `gh pr create` and `gh pr merge --auto --merge --delete-branch` may run automatically only after validation checkpoints and the `docs/workflows.md` PR preflight confirms `mergeStateStatus`, `statusCheckRollup`, and `reviewDecision`; a final completion report must not stop after local validation when task-owned changes can be published; push cadence is checkpoint-based, not commit-count-based; pending-only `UNSTABLE` / `BLOCKED` may register auto-merge, and use `--match-head-commit <headRefOid>` when available. Direct default-branch pushes are forbidden; prompt-gated PR state changes such as `gh pr edit` or immediate `gh pr merge` still require GitHub Web/Desktop or an approval-capable session when approvals are unavailable.

Keep `AGENTS.md` under Codex's default 32 KiB project-doc budget, keep shared `SKILL.md` bodies as concise routers, and keep subagents narrowly scoped; `tools/check-agents.ps1` enforces the repository budgets. Move detail to skill-local `references/*.md`, canonical docs, subagents, or manifest fragments instead of expanding always-loaded instructions.

Every implementation change, improvement, bug fix, refactor, and architecture/toolchain/workflow/validation/packaging change includes a targeted **agent-surface drift check** before completion. When durable guidance or AI-operable contracts are stale, update the relevant `AGENTS.md`, `CLAUDE.md`, docs, Codex/Claude/Cursor skills, rules, settings, subagents, manifest fragments plus compose output, schemas, validation checks, and tracked `.clangd` in the same task; do not leave that synchronization as a separate follow-up or load every surface when no durable guidance changed.

Parallel worktree guidance follows official Codex/Claude surfaces: prefer Codex app Worktree/Handoff or Claude Code `--worktree` / subagent `isolation: worktree`, keep `.worktrees/` and `.claude/worktrees/` ignored, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` inside manual linked worktrees, use Claude `worktree.baseRef = "head"` for isolated subagents that need current branch context, and clean with `git worktree remove` / `git worktree prune`.

Hosted PR failure hardening: update `AGENTS.md`, docs, Codex/Claude/Cursor skills, subagents, and scoped `tools/check-ai-integration.ps1` Needles together when a PR check exposes guidance/static-guard drift. For CI check selection, keep branch-protection-required checks always-running or aggregate-gated; do not make path-filtered required checks. Use lightweight static validation for docs/agent/rules/subagent-only PRs instead of unrelated Windows/MSVC, macOS, or full repository clang-tidy lanes. For static analysis, keep `.clang-tidy` `HeaderFilterRegex`, `--warnings-as-errors=*`, `NN warnings generated.` handling, and hosted `-Jobs 0` guidance synchronized. Keep rules and settings as command/permission gates, not troubleshooting playbooks.

Git/GitHub authentication stays host-local through Git Credential Manager, GitHub CLI, SSH agent, or a browser session; do not add repository requirements for `GITHUB_TOKEN` or personal access tokens. If warnings mention missing helpers such as `credential-manager-core`, inspect `git config --show-origin --get-all credential.helper`, prefer current Git for Windows GCM helper `manager`, and fix host/user Git config rather than adding repository overrides.

When adding retained editor UI ids or literals enforced by `tools/check-ai-integration.ps1`, extend scoped Needles and sibling skill/manifest text together (see canonical skill §When Changing Integration item 17).

When **`editor/core`** aggregates such as **`ScenePrefabInstanceRefreshPolicy`** gain fields, update **`tests/unit/editor_core_tests.cpp`** designated initializers in the same task (see canonical skill **Rules** and `docs/cpp-style.md` **Unit tests**).

Canonical skill also documents `oneDotZeroCloseoutTier` on each `unsupportedProductionGaps` row and the production-completion master plan **HTML comment** tail used as substring evidence for `check-ai-integration.ps1`; do not strip that tail without running full `validate`.

`MK_tools` implementation `.cpp` paths are under `engine/tools/{shader,gltf,asset,scene}/` (not a flat `engine/tools/src/`). Changing them requires updating `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `engine/tools/*/CMakeLists.txt` together; see `docs/specs/2026-05-11-directory-layout-target-v1.md`. New `mirakana/*` deps in a cluster require the matching `MK_*` on that cluster's `OBJECT` target (spec invariant 4).

Windows D3D12 GPU capture: default **operator PIX + AI analysis** pattern is `docs/ai-integration.md` § **Recommended workflow (operator PIX, AI analysis)** (see full `.claude/skills/gameengine-agent-integration/SKILL.md`).
