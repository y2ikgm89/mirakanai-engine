---
name: gameengine-cursor-baseline
description: Runs GameEngine work inside Cursor by routing to AGENTS.md, thin Cursor skills, PowerShell validation wrappers, and targeted manifest fragments.
paths:
  - "**/*"
disable-model-invocation: false
---

# GameEngine Cursor baseline

## Authority

1. **`AGENTS.md`** is the shared baseline for validation, dependency bootstrap, language, Git Workflow, and Done Definition.
2. **Cursor global instructions** must not override this repository. If they conflict, align the global instruction or add a workspace override before relying on automation.
3. **Thin `.cursor/skills/gameengine-*` folders** name-match `.claude/skills/` and point to the Claude skill plus Codex twin; do not duplicate large shared procedures.
4. **Cursor project subagents** live in `.cursor/agents/*.md`, mirror `.claude/agents/` and `.codex/agents/`, use bounded scopes, keep read-only audit/review/explore roles on `readonly: true`, and set **`model: composer-2.5-fast`** via Cursor subagent frontmatter.

## Cursor-first discipline

- Hooks that require a Claude Code Skill tool do not apply verbatim in Cursor. Read the matching `SKILL.md` with Cursor's file reader instead.
- Keep `.cursor/rules/*.mdc` focused, actionable, scoped, and under Cursor's recommended 500-line rule budget; use `AGENTS.md` for root-wide plain Markdown guidance.
- Execute repository wrappers yourself when claiming readiness: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/<script>.ps1`. Use PowerShell 7 (`pwsh`), not Windows PowerShell 5.1.
- Before engine-facing API, `game.agent.json`, generated-game, manifest, or schema changes, read a targeted `engine/agent/manifest.fragments/*.json` slice or run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal|Standard`. Edit manifest fragments, then run `tools/compose-agent-manifest.ps1 -Write`.
- After touching one `.cpp`, use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files <path>` after `tools/cmake.ps1 --preset dev`; for public headers, pass the primary implementation TU or use clangd diagnostics.

## GitHub Flow

- Follow official GitHub Flow in `AGENTS.md` and `docs/workflows.md`; run `tools/check-publication-preflight.ps1` before staging/push/PR/merge.
- Cadence is purpose/checkpoint-based: push validated checkpoints, keep one PR per focused capability/gap-cluster/milestone, and avoid PRs per commit/checklist item.
- Use `tools/ready-task-pr.ps1` for guarded draft-to-ready conversion; prompt-gated `gh pr edit`, raw `gh pr ready`, or immediate `gh pr merge` need GitHub Web/Desktop or an approval-capable session when guarded automation blocks.
- Use `gh pr merge --auto --merge --match-head-commit <headRefOid>` only after validation checkpoints plus PR preflight. Check `state`, `isDraft`, `baseRefName`, `headRefName`, `headRefOid`, `mergeable`, `mergeStateStatus`, `reviewDecision`, `statusCheckRollup`, `autoMergeRequest`, and `url`.
- Runtime/C++/build/toolchain/public-contract PRs must wait for selected hosted checks to complete and `PR Gate` to report `SUCCESS`.
- Rerun preflight after every commit or push because a stale `headRefOid` invalidates the merge decision and any guarded ready decision.
- If `publication-preflight: blocked` appears, switch session/host context. After merge, use `tools/post-merge-task-cleanup.ps1` (preferred) or `tools/remove-merged-worktree.ps1 -DeleteLocalBranch -DeleteRemoteBranch` for guarded post-merge cleanup with Windows long-path fallback inside the guarded script; GitHub `delete_branch_on_merge` removes merged PR head branches automatically.
- docs/agent/rules/subagent-only PRs should use lightweight static validation instead of unrelated Windows/MSVC, macOS, or full repository clang-tidy lanes.

## Validation shorthand (details in AGENTS.md)

- Completion claim: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, or record the exact toolchain blocker.
- CMake/compiler/PATH: `tools/check-toolchain.ps1`; raw cmake lanes add `-RequireDirectCMake`.
- Default dev loop: `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev`, then `tools/ctest.ps1 --preset dev --output-on-failure`.
- Public API: `tools/check-public-api-boundaries.ps1`; shader/RHI: `tools/check-shader-toolchain.ps1`.
- Agent surfaces: `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-text-format.ps1`.
- Optional desktop runtime, package, Android, and iOS lanes are host-gated; treat missing SDKs as explicit blockers.

## Language

- **User-visible prose**: Japanese when the user writes Japanese (per project agent instructions).
- **Code, CLI, paths, `mirakana::` API names**: ASCII.

## Anti-patterns

- Lowering C++ standard or adding deps without `docs/legal-and-licensing.md`, `docs/dependencies.md`, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md`.
- Installing vcpkg packages from CMake configure; use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` only.
- Committing or pushing unrelated changes, pushing directly to protected/default branches, force-pushing without explicit user request, or creating/updating PRs without a reviewed branch/remote target, task-owned staged files, and validation evidence or a recorded blocker.
- Deleting `external/vcpkg` while cleaning ignored files; presets require that clone for `CMAKE_TOOLCHAIN_FILE`.
- Leaving stale test idioms in `tests/unit/*.cpp`; prefer `docs/cpp-style.md` Unit tests guidance before broad NOLINT.
