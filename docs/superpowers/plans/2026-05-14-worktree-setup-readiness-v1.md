# Worktree Setup Readiness v1 (2026-05-14)

**Plan ID:** `worktree-setup-readiness-v1`

## Goal

Make manual linked worktrees a supported, repeatable repository workflow without changing the active production gap or adding compatibility shims.

## Context

The repository already ignores `.worktrees/`, `.claude/worktrees/`, `external/`, and `vcpkg_installed/`. Manual linked worktrees still fail configure when the ignored `external/vcpkg` checkout is missing, and Windows direct configure/build can inherit duplicate `PATH` / `Path` variables unless presets normalize both configure and build environments.

Official-source checks:

- Git worktree documentation defines `git worktree add/list/remove/prune` as the supported linked-worktree lifecycle and recommends `git worktree remove` instead of deleting directories manually.
- CMake Presets documentation supports inherited `environment` maps, `$penv{...}` for parent environment values, and `null` values to unset variables; this repository uses that to keep one effective Windows path key while reading `CMakePresets.json` with a case-preserving parser.
- The repository vcpkg policy keeps dependency install/update in `tools/bootstrap-deps.ps1`; CMake configure must remain configure-only.

## Constraints

- Keep `external/vcpkg` as an ignored local Microsoft vcpkg tool checkout referenced by `CMakePresets.json`.
- Do not move vcpkg acquisition into CMake configure.
- Do not share `vcpkg_installed/` across worktrees by default.
- Keep cleanup through `git worktree remove` / `git worktree prune`.
- Keep Codex, Claude, Cursor, manifest, docs, and static checks synchronized.

## Implementation Checklist

- [x] Add a PowerShell 7 worktree preparation entrypoint that verifies ignore rules and prepares linked-worktree `external/vcpkg` from an existing local checkout.
- [x] Normalize checked-in CMake configure presets as well as build presets.
- [x] Extend `tools/check-toolchain.ps1` to enforce configure/build preset normalization and report worktree/vcpkg readiness.
- [x] Update agent-facing docs, skills, rules, subagents, manifest fragments, and static checks.
- [x] Compose the engine agent manifest and run focused validation before the full slice gate.

## Done When

- `tools/prepare-worktree.ps1` is tracked and non-destructive.
- `CMakePresets.json` visible configure/build presets inherit the relevant hidden normalization presets.
- Static checks prevent regression of the worktree/toolchain guidance.
- `prepare-worktree`, `check-toolchain`, agent checks, JSON contract checks, full `validate.ps1`, and `build.ps1` are run or blockers are recorded.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` | Passed | Linked worktree reported `external-vcpkg=ready` and verified ignore rules. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireVcpkgToolchain` | Passed | Enforced normalized configure/build presets, reported linked worktree, `external-vcpkg=ready`, and later `vcpkg-installed=ready` after bootstrap. |
| `. tools/common.ps1; Invoke-CheckedCommand $tools.CMake --fresh --preset release` | Passed | Repository wrapper environment configured `release` successfully in an ordinary Windows shell. Raw direct `cmake --fresh --preset release` in the same host still failed to find the compiler, so docs keep repository wrappers as the recommended entrypoint. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` | Passed | First sandboxed run failed on network download; approved network run restored the worktree-local `vcpkg_installed/` through the official bootstrap entrypoint. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed | Rewrote the composed manifest from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Also reported `agent-manifest-compose: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent instruction and skill budgets stayed within limits. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Worktree/toolchain needles are enforced across docs, manifest, skills, rules, and subagents. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full repository validation passed; diagnostic host blockers remain only for Apple/Metal lanes on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Dev configure/build completed after validation. |
| `Invoke-ScriptAnalyzer ...` | Blocked | `Invoke-ScriptAnalyzer` is not installed on this host. |
