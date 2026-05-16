# Windows CMake/MSBuild Path Environment v1

**Status:** Completed

## Goal

Root-fix the recurring Windows Visual Studio CMake/MSBuild `PATH`/`Path` environment collision so local focused configure/build/test loops no longer depend on manually repairing `out/build/dev` or starting a perfectly cased shell.

## Context

- A direct `cmake --preset dev` from the current Windows shell reproduced the known failure before compiler detection completed: CMake/Visual Studio/MSBuild inherited an uppercase `PATH` shape and `CL.exe` failed during compiler-id probing.
- Official CMake preset documentation supports explicit inherited environments through `$penv{...}` and `null` entries, but Visual Studio generator startup still depends on the process environment CMake passes to MSBuild during configure.
- This repository already centralizes build tooling in PowerShell entrypoints and `Invoke-CheckedCommand`, so the clean fix is to make repository-supported CMake/CTest launches normalize their child environment before CMake, MSBuild, or CTest starts.

## Constraints

- No backward-compatibility shim that keeps raw `cmake --preset dev` as the recommended local path.
- Do not move vcpkg package installation back into CMake configure.
- Keep linked worktrees ready without duplicating the Microsoft `external/vcpkg` checkout or downloaded packages.
- Keep Codex/Claude/Cursor skills, rules, subagents, manifest fragments, and static integration checks synchronized.

## Implementation

- Add `tools/cmake.ps1` and `tools/ctest.ps1` as the supported focused local wrappers over resolved official CMake/CTest paths.
- Update `tools/common.ps1` so child processes receive exactly one `Path` key on Windows (`PATH` elsewhere), built from parent `Path` and `PATH` values with deterministic de-duplication.
- Update `tools/check-toolchain.ps1` so raw direct CMake requirements verify `cmake` discovery and preset normalization instead of treating uppercase-only `PATH` as a blocker after the preset fix.
- Extend `tools/prepare-worktree.ps1` to link an existing local `vcpkg_installed/` package tree in linked worktrees when available.
- Update durable docs, skills, rules, subagents, manifest fragments, and static checks to make the wrappers the normal CMake/CTest path.

## Done When

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` configures the linked worktree successfully from the problematic shell.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` reports wrapper and worktree readiness, and `-RequireDirectCMake` passes when raw direct CMake is on `PATH` and the checked-in preset normalization is valid.
- Focused build/test and repository static checks pass.
- `engine/agent/manifest.json` is regenerated from fragments.
- The branch is staged, committed, pushed, and opened as a PR.

## Validation Evidence

| Check | Status | Evidence |
| --- | --- | --- |
| Direct raw `cmake --preset dev` reproduction | PASS | Reproduced compiler detection failure before implementation in the uppercase-`PATH` shell. |
| `tools/prepare-worktree.ps1` | PASS | Reported linked-worktree readiness with `external-vcpkg=ready`, `vcpkg-installed=ready`, and `prepare-worktree: ok`. |
| `tools/cmake.ps1 --preset dev` | PASS | Configured `out/build/dev` successfully after wrapper normalization and package-tree link. |
| Raw `cmake --preset dev` after preset fix | PASS | Configured `out/build/dev` successfully from the same shell, proving the CMake Presets environment fix covers direct preset configure. |
| `tools/check-toolchain.ps1` | PASS | Reported supported tool versions, preset environment readiness, parent path casing diagnostics, and linked-worktree vcpkg readiness. |
| `tools/check-toolchain.ps1 -RequireDirectCMake` | PASS | Verified raw `cmake` is available on `PATH` without treating uppercase-only `PATH` as a blocker after preset normalization. |
| Focused build/test | PASS | `tools/cmake.ps1 --build --preset dev --target MK_core_tests` passed; `tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` passed 1/1 tests. |
| `tools/compose-agent-manifest.ps1 -Write` | PASS | Regenerated `engine/agent/manifest.json` from fragments after manifest guidance changes. |
| `tools/check-agents.ps1` / `tools/check-ai-integration.ps1` / `tools/check-json-contracts.ps1` | PASS | Agent-surface parity, static integration needles, and JSON contracts passed after docs/skills/rules/subagent/manifest updates. |
| `tools/validate.ps1` | PASS | Full validation passed after restoring the missing official `external/vcpkg` checkout required by the linked worktree. |
| `tools/build.ps1` | PASS | Standalone dev configure/build completed successfully through the repository-supported wrapper path. |
