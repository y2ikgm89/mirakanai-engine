# Tidy Targeted File Lane v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party targeted-file lane to `tools/check-tidy.ps1` so agents can run clang-tidy on the files they changed without invoking the full repository warning stream.

**Architecture:** Keep the default `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` behavior unchanged for full-repository analysis. Add an opt-in `-Files` filter to the existing PowerShell wrapper after compile database generation, validate requested paths against the repository and compile database, and keep `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` on its existing bounded smoke lane.

**Tech Stack:** PowerShell 7, repository CMake File API compile database synthesis, clang-tidy, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Reduce the `full-repository-quality-gate` developer-efficiency loophole found during Physics Exact Sphere Cast v1: full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` can be too slow and verbose for coherent slice closure, while direct ad hoc clang-tidy commands bypass repository tool discovery and compile database rules.

## Context

- `tools/check-tidy.ps1` already owns clang-tidy discovery, `.clang-tidy` verification, CMake File API compile database synthesis, and `-MaxFiles`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` intentionally uses `check-tidy.ps1 -MaxFiles 1` as a fast smoke gate.
- Agents currently need direct `clang-tidy` commands for changed-file confidence when full tidy times out, which is less consistent than using the repository wrapper.

## Constraints

- Do not change the default full-repository behavior of `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- Do not loosen `.clang-tidy`, validation, CMake, or compile database requirements.
- Do not touch pre-existing unrelated dirty files such as `AGENTS.md`, `.agents/skills/cmake-build-system/SKILL.md`, `.claude/skills/gameengine-cmake-build-system/SKILL.md`, or build-fixer agent files in this slice.
- Keep `full-repository-quality-gate` `partly-ready`; this slice narrows local changed-file analysis, not full analyzer/profile completion.
- Keep implementation in the existing PowerShell wrapper; do not introduce new dependencies.

## Done When

- `tools/check-tidy.ps1` accepts `-Files <path>[,<path>]` and only analyzes requested compile-database source files.
- Invalid, out-of-repository, non-source, or compile-database-missing files fail with explicit errors.
- Static checks assert that the wrapper, docs, and manifest describe the targeted lane without claiming full repository quality readiness.
- Focused positive and negative wrapper commands pass/fail as expected.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before committing this slice.

## Files

- Modify: `tools/check-tidy.ps1`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/testing.md`
- Modify: `docs/workflows.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`

## Tasks

### Task 1: RED Static Contract

- [x] Add assertions to `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` that require:
  - `tools/check-tidy.ps1` to expose a `Files` parameter.
  - `docs/testing.md` to document `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp`.
  - `engine/agent/manifest.json` `full-repository-quality-gate` notes to mention targeted changed-file clang-tidy.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and record the expected failure before implementing the wrapper/docs/manifest updates.

### Task 2: Tidy Wrapper

- [x] Add `[string[]]$Files = @()` to `tools/check-tidy.ps1`.
- [x] Normalize comma-separated and repeated `-Files` values to full repository paths.
- [x] Reject missing paths, paths outside the repository, and non-`.cc`/`.cpp`/`.cxx` source paths with explicit `Write-Error` messages.
- [x] Filter the compile database file list to requested files, and reject requested files absent from the compile database before applying `-MaxFiles`.
- [x] Preserve the current no-`-Files` behavior.

### Task 3: Docs And Manifest

- [x] Update `docs/testing.md`, `docs/workflows.md`, and `docs/current-capabilities.md` with the targeted-file lane and its boundaries.
- [x] Update `engine/agent/manifest.json` `full-repository-quality-gate` notes and recommended context without changing the gap to ready.
- [x] Update the master plan and registry so this completed slice is discoverable after validation.

### Task 4: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp -MaxFiles 1` and record success.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/include/mirakana/physics/physics3d.hpp -MaxFiles 1` and record the expected non-source or compile-database failure.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `build: add targeted tidy file lane`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Failed as expected before wrapper/docs/manifest updates | RED static contract rejected missing `docs/testing.md` text: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Static integration contract accepted `tools/check-tidy.ps1` `Files` parameter, docs text, and manifest `full-repository-quality-gate` targeted changed-file notes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest JSON remained schema-valid after targeted tidy notes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | `full-repository-quality-gate` stayed `partly-ready`; targeted tidy did not claim full analyzer readiness. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp,tests/unit/core_tests.cpp -MaxFiles 1` | Passed | Only the first requested source was analyzed, preserving requested `-Files` order before `-MaxFiles`; output ended with `tidy-check: ok (1 files)`. Existing clang-tidy warnings were emitted as warnings, not errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp -MaxFiles 1` | Passed | Targeted wrapper-owned clang-tidy analyzed one compile-database source and ended with `tidy-check: ok (1 files)`. Existing repository warnings remained warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/include/mirakana/physics/physics3d.hpp -MaxFiles 1` | Failed as expected | Rejected a header with `requested file is not a clang-tidy source file (.cc, .cpp, .cxx)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/not_real.cpp -MaxFiles 1` | Failed as expected | Rejected a missing requested source with `requested file does not exist`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files C:\tmp\outside.cpp -MaxFiles 1` | Failed as expected | Rejected an out-of-repository path before analysis. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/src/main.cpp -MaxFiles 1` | Failed as expected | Rejected an existing `.cpp` absent from the active `dev` compile database. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Repository formatting wrapper accepted the slice. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed; host-gated Metal/Apple diagnostics remained diagnostic-only, and CTest reported 29/29 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Default development build completed after validation. |

## Status

**Status:** Completed.
