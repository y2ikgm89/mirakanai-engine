# Worktree Post-Merge Cleanup v1 (2026-05-15)

**Plan ID:** `worktree-post-merge-cleanup-v1`

## Goal

Make completed task worktrees removable automatically after their PR is actually merged into `main`, while keeping raw destructive worktree removal reviewed.

## Context

Git documents `git worktree remove` and `git worktree prune` as the supported linked-worktree cleanup path. GitHub Flow says to delete the branch after a pull request is merged. Documentation alone cannot delete local directories, so the repository needs a guarded PowerShell 7 entrypoint that agents can call only after merge evidence exists.

## Constraints

- Do not use `Remove-Item` or ad hoc directory deletion for worktree cleanup.
- Refuse cleanup outside ignored `.worktrees/` and `.claude/worktrees/` roots.
- Refuse dirty, locked, detached, default-branch, unknown, or not-merged worktrees.
- Keep raw `git worktree remove` / `git worktree prune` prompt-gated.
- Keep Codex, Claude, Cursor, manifest, schema, and static checks synchronized.

## Implementation Checklist

- [x] Add a failing static guard for the new cleanup command and guidance.
- [x] Add `tools/remove-merged-worktree.ps1` with merge/clean/root safety checks.
- [x] Expose the command in manifest fragments and compose `engine/agent/manifest.json`.
- [x] Update AGENTS, workflow docs, AI integration docs, Codex/Claude/Cursor skills, rules, and settings.
- [x] Run focused agent/static validation and record evidence.
- [x] Run full slice validation or record blockers.

## Done When

- Agents have one canonical automatic cleanup command for merged task worktrees.
- Raw worktree removal remains reviewed.
- Static checks fail if the command or guidance drifts.
- Validation evidence is recorded here and in the completion report.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | RED | Failed before implementation because `AGENTS.md` lacked `tools/remove-merged-worktree.ps1`. |
| `git worktree add .worktrees/codex-cleanup-smoke -b codex/cleanup-smoke main` | Passed | Created a temporary clean linked worktree for script smoke validation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath .worktrees/codex-cleanup-smoke -BaseRef main -DeleteLocalBranch` | RED then Passed | RED exposed array output handling in `Get-GitWorktreeRecords`; after the fix, the script removed the temporary worktree, deleted the merged local branch, and pruned metadata. |
| `git worktree list --porcelain`; `git branch --list codex/cleanup-smoke`; `Test-Path -LiteralPath '.worktrees/codex-cleanup-smoke'` | Passed | Temporary worktree and branch were gone after cleanup. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed | Rewrote the canonical manifest from fragments with LF output. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Also reported `agent-manifest-compose: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | RED then Passed | RED first caught CRLF manifest output, then AGENTS budget, then the new allow-rule whitelist; fixes landed in `compose-agent-manifest.ps1`, `AGENTS.md`, and `check-agents.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Worktree cleanup needles are enforced across docs, manifest, skills, rules, and settings. |
| `git diff --check` | Passed | No whitespace errors. |
| `Get-Command Invoke-ScriptAnalyzer -ErrorAction SilentlyContinue` | Blocked | `Invoke-ScriptAnalyzer` is not installed on this host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed; diagnostic-only Apple/Metal host gates remain expected on Windows, and 51 CTest tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Standalone dev configure/build completed after validation. |
