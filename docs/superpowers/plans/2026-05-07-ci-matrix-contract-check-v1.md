# CI Matrix Contract Check v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a repository validation check that keeps the documented GitHub Actions build, package, sanitizer, coverage, macOS, and iOS lanes from drifting silently.

**Architecture:** Keep the check as a host-independent PowerShell contract test over checked-in workflow YAML, `engine/agent/manifest.json` command strings, and `tools/validate.ps1` wiring. The check does not execute CI, install dependencies, or broaden ready claims; it verifies that the reviewed workflow lanes and artifact handoff rows remain present.

**Tech Stack:** PowerShell validation scripts, `engine/agent/manifest.json` command strings, GitHub Actions workflow YAML, existing `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Goal

- Add `tools/check-ci-matrix.ps1` as a deterministic static CI workflow contract check.
- Wire it into `tools/validate.ps1` (and manifest command surfaces) so default validation fails when core CI lanes or artifact handoff rows are removed.
- Update current docs, manifest, and static checks so `full-repository-quality-gate` records this as CI matrix contract hardening while broader analyzer/profile work remains unsupported.

## Context

- Master plan gap: `full-repository-quality-gate`.
- Existing CI workflows:
  - `.github/workflows/validate.yml`
  - `.github/workflows/ios-validate.yml`
- Existing scripts already cover sanitizer, coverage, installed SDK, release package artifact, and production-readiness audits; this slice prevents the workflow matrix from drifting away from those reviewed lanes.

## Constraints

- Do not add a new dependency, GitHub Action, or package manager lane.
- Do not execute CI from local validation.
- Do not claim package/build CI matrix completion; this closes only the static workflow contract check.
- Keep Apple/iOS/Metal readiness host-gated on non-Apple hosts.
- Keep optional GUI/importer/bootstrap lanes out of default CI unless a future plan adds dependency bootstrap evidence.

## Done When

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` reports `ci-matrix-check: ok`.
- A RED run proves `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` fails before `tools/check-ci-matrix.ps1` exists or before the validate hook is wired.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact blockers.
- `full-repository-quality-gate` still remains `partly-ready`, with package/build CI matrix contract hardening recorded for reviewed Windows/Linux/sanitizer/macOS/iOS workflow lanes and broader static analyzer/profile work still unsupported.
- This slice does not execute CI locally or claim full package/build matrix readiness.

## File Plan

- Create `tools/check-ci-matrix.ps1`: static contract assertions for workflow jobs, core steps, artifact uploads, and script alias wiring.
- Modify `tools/validate.ps1`: run the new check during default validation before heavier local tool checks.
- Modify `engine/agent/manifest.json`: point active work at this plan, record the selected gap, and expose `commands.ciMatrixCheck` for `tools/check-ci-matrix.ps1`.
- Modify `docs/superpowers/plans/README.md`: set this plan as the active slice.
- Modify `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`: record the selected next child slice.
- Modify `docs/current-capabilities.md`, `docs/roadmap.md`, and `docs/testing.md`: describe the new contract check after implementation.
- Modify `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1`: require the new plan/script/docs/manifest evidence.

## Tasks

### Task 1: Select Active Slice

- [x] Create this plan.
- [x] Update `engine/agent/manifest.json` active plan and recommended next plan to this child slice.
- [x] Update the plan registry and master plan current pointer.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Commit the selection docs/manifest/static-check change.

### Task 2: RED Validation Hook

- [x] Add the `ci-matrix-check` entry (historically via `package.json`; now `tools/validate.ps1` invokes `check-ci-matrix.ps1` directly) and call it from `tools/validate.ps1` before creating `tools/check-ci-matrix.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and record the expected failure on missing `tools/check-ci-matrix.ps1`.

### Task 3: Implement CI Matrix Contract

- [x] Create `tools/check-ci-matrix.ps1`.
- [x] Assert `.github/workflows/validate.yml` has Windows, Linux, Linux sanitizer, and macOS jobs.
- [x] Assert the Windows job runs `./tools/validate.ps1`, `./tools/evaluate-cpp23.ps1 -Release`, uploads Windows test logs, and uploads Windows packages.
- [x] Assert the Linux job installs `lcov`, configures/builds/tests CMake, runs `./tools/check-coverage.ps1 -Strict`, and uploads Linux test/coverage artifacts.
- [x] Assert the sanitizer job installs Ninja, uses `clang-asan-ubsan` configure/build/test presets, sets `UBSAN_OPTIONS`, and uploads sanitizer logs.
- [x] Assert the macOS job configures with `xcrun` clang/clang++, builds/tests, and uploads macOS logs.
- [x] Assert `.github/workflows/ios-validate.yml` has manual dispatch, path filters, `./tools/check-mobile-packaging.ps1 -RequireApple`, `./tools/smoke-ios-package.ps1`, and iOS build artifact upload.
- [x] Assert `engine/agent/manifest.json` exposes `ciMatrixCheck` pointing at `tools/check-ci-matrix.ps1` (replaces historical `package.json` wiring).

### Task 4: Documentation And Static Contract Sync

- [x] Update current capabilities, roadmap, testing docs, master plan, registry, and manifest with CI Matrix Contract Check v1.
- [x] Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` to require the new script/docs/manifest evidence.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Expected fail | `tools/validate.ps1` failed at missing `tools/check-ci-matrix.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | Pass | `ci-matrix-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-readiness-audit-check: ok`; `full-repository-quality-gate` remains `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | `format-check: ok`. |
| `git diff --check` | Pass | No whitespace errors; Git reported LF-to-CRLF working-copy warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | `validate: ok`; default validation ran `ci-matrix-check: ok` and 29/29 CTest tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Dev preset build completed successfully. |
