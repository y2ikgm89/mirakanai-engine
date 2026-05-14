# 2026-05-15 Post-Merge Local Checkout Sync V1

## Goal

Make guarded post-merge worktree cleanup update the local base checkout before deleting the merged task worktree.

## Context

GitHub Flow says changes appear on the default branch after the pull request is merged and recommends deleting the branch afterward. Git documents `git worktree remove` / `git worktree prune` as the supported linked-worktree cleanup path, and `git pull --ff-only` / fast-forward-only integration as the safe update path when local history has not diverged.

The previous cleanup command proved the task branch was merged and the task worktree was clean before deletion, but it did not update the foreground local checkout. That left a mismatch between `origin/main` and the files visible in Local.

## Constraints

- Do not stash, discard, copy, or merge over local user work.
- Update only a clean local checkout on the expected base branch.
- Use `git fetch --prune <remote>` followed by a fast-forward-only update to `-BaseRef`.
- Keep raw `git worktree remove` / `git worktree prune` reviewed and keep the guarded script as the automatic path.
- Keep Codex, Claude, Cursor, manifest commands, static checks, and command policy synchronized.

## Done When

- `tools/remove-merged-worktree.ps1` fetches/prunes, verifies the local checkout is clean and on `-BaseBranch`, fast-forwards it to `-BaseRef`, then removes only clean merged worktrees.
- Dirty, detached, non-base, diverged, locked, detached task, default-branch task, unknown, or unmerged cases fail before deletion.
- Agent surfaces document that local checkout sync is automatic only when fast-forward-safe.
- Focused static checks and a smoke cleanup prove the behavior.

## Checklist

- [x] Extend guarded cleanup script with local checkout sync preflight and fast-forward update.
- [x] Update AGENTS, workflow docs, AI integration docs, Codex/Claude/Cursor skills/rules, manifest fragments, and static needles.
- [x] Run focused script smoke for fast-forward local checkout plus worktree deletion.
- [x] Run focused static validation.
- [x] Run slice-closing validation.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| Context7 `/git/htmldocs` docs lookup | Passed | Confirmed fetch prune, pull/fetch+merge, fast-forward-only, and worktree prune/remove behavior. |
| GitHub Flow docs lookup | Passed | Confirmed branch-based PR flow and deleting merged branches. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath .worktrees/local-sync-smoke-target -DeleteLocalBranch` | Passed | Fast-forwarded the smoke local checkout and removed a clean merged worktree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath .worktrees/local-sync-smoke-target-2 -BaseRef main -BaseBranch codex/local-sync-smoke-base -LocalCheckoutPath .worktrees/local-sync-smoke-base -DeleteLocalBranch` | Failed as expected | Exposed that `git branch -d` checks merge status against current `HEAD`, not the verified `-BaseRef`; fixed by deleting with `git branch -D` only after the script proves ancestry. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath .worktrees/local-sync-smoke-target-3 -BaseRef main -BaseBranch codex/local-sync-smoke-base -LocalCheckoutPath .worktrees/local-sync-smoke-base -DeleteLocalBranch` | Passed | Verified branch deletion after ancestry proof, target worktree removal, and local checkout fast-forward. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath .worktrees/local-sync-smoke-target-4 -BaseRef main -BaseBranch codex/local-sync-smoke-base -Remote . -LocalCheckoutPath .worktrees/local-sync-smoke-base -DeleteLocalBranch` | Passed | Re-ran after script tightening; verified local fast-forward, branch deletion, and target removal without network. |
| Escalated smoke using default remote | Blocked | Git for Windows rejected the sandbox-created smoke worktree as `safe.directory` for the escalated host user; local-remote smoke covered script behavior without changing global Git config. |
| Smoke cleanup checks | Passed | Removed smoke worktrees/branches and pruned stale worktree metadata. |
| `git diff --cached --check` | Passed | No whitespace errors in staged task diff. |
| `rg -n '^(<<<<<<<|=======|>>>>>>>)'` | Passed | No conflict markers remain. |
| `Get-Command Invoke-ScriptAnalyzer -ErrorAction SilentlyContinue` | Blocked | PSScriptAnalyzer is not installed in this host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest command string and generated manifest parity passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent config, skills, and instruction budgets passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | New local checkout sync needles passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting/text normalization passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` | Passed | Linked this worktree to the local `external/vcpkg` checkout. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` | Passed after escalation | First run was blocked by sandbox network when downloading vcpkg sources; escalated run installed the required packages. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed; host-gated Apple/Metal lanes remained diagnostic as expected on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Explicit pre-commit build gate passed after validation. |
