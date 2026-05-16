# Worktree Cleanup Reparse-Point Safety v1 - 2026-05-17

## Goal

Prevent guarded post-merge worktree cleanup from following linked-worktree junctions or symlinks into shared ignored dependency/tool directories such as `external/vcpkg` and `vcpkg_installed`.

## Context

- `tools/prepare-worktree.ps1` intentionally links `external/vcpkg` and `vcpkg_installed` into manual linked worktrees so configure/build lanes do not duplicate the Microsoft vcpkg checkout or installed package tree.
- After PR #92 cleanup, the task worktree was removed but the shared root `external/vcpkg` checkout was left empty. The script's fallback delete helper already avoids following reparse points, so the likely unsafe boundary is the earlier `git worktree remove` attempt on a worktree containing junctions.
- Git's official `worktree remove` / `worktree prune` commands remain the preferred worktree metadata path, but the repository wrapper must protect ignored local reparse-point links before handing deletion to Git on Windows.
- Microsoft PowerShell docs treat symbolic links/junctions as first-class filesystem items; `Get-ChildItem` recursion does not follow symlinks unless explicitly requested via `-FollowSymlink`, which matches the desired repository cleanup behavior.

## Constraints

- Do not use raw `Remove-Item -Recurse` for worktree cleanup.
- Do not delete or mutate the target of a worktree-local junction/symlink.
- Keep `external/vcpkg` as a required local Microsoft vcpkg checkout, not disposable cache.
- Keep cleanup guarded by existing clean status, branch ancestry, standard worktree root, and local checkout fast-forward checks.

## Plan

- [x] Add a RED agent/static guard for pre-unlinking known worktree-local reparse points before `git worktree remove`.
- [x] Update `tools/remove-merged-worktree.ps1` to unlink worktree-local `external/vcpkg` and `vcpkg_installed` reparse points without following targets before invoking Git removal.
- [x] Update workflow/agent guidance and validation needles for the strengthened cleanup contract.
- [x] Restore this host's local `external/vcpkg` checkout and rerun linked-worktree preparation.
- [x] Run focused cleanup/static validation and full validation before commit/PR.

## Done When

- `remove-merged-worktree.ps1` removes only the worktree-local reparse-point entries for `external/vcpkg` and `vcpkg_installed` before any `git worktree remove` call.
- Cleanup guidance states that linked dependency roots are unlinked from the task worktree before Git/fallback deletion and that targets must not be followed.
- `prepare-worktree`, `check-ai-integration`, and `validate.ps1` pass, or a concrete host blocker is recorded.

## Validation Evidence

- Official practice check: Git documents `git worktree remove` / `git worktree prune` as the worktree cleanup and metadata path, but repository-local ignored junctions require a protective wrapper before deletion. Microsoft PowerShell filesystem docs expose junctions/symlinks as link items and default recursive traversal does not follow symlinks unless `-FollowSymlink` is used.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before implementation because `tools/remove-merged-worktree.ps1` did not contain `Remove-WorktreeLocalReparsePointBeforeGitRemoval`.
- PASS: Restored the host-local official Microsoft vcpkg checkout with `git clone https://github.com/microsoft/vcpkg.git external/vcpkg` and `external\vcpkg\bootstrap-vcpkg.bat`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1`.
- PASS: Reparse cleanup smoke created a temporary linked worktree with `external/vcpkg` and `vcpkg_installed` junctions, then `tools/remove-merged-worktree.ps1` reported both `unlinked-worktree-reparse-point=...` rows, removed the worktree, and left shared targets intact.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireVcpkgToolchain`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
