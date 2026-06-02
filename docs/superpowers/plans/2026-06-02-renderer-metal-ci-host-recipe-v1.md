# 2026-06-02 Renderer Metal CI Host Recipe v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `renderer-metal-ci-host-recipe-v1`
**Status:** Completed
**Owner:** `tools` / CI / agent-surface governance
**Parent:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)
**Published:** PR #386 / merge commit `12fde1bf7ea369e97a45d95a0ee271b4e2515b3d`

**Goal:** Make the reviewed renderer Metal Apple host-evidence recipe run in the hosted macOS validation lane without weakening existing full macOS build/CTest coverage or promoting broad Metal readiness.

**Architecture:** Keep `tools/validate-renderer-metal-apple.ps1` as the single reviewed recipe entrypoint. Add a `pwsh` run step to the existing `macOS Metal CMake` job before the full `ci-macos-appleclang` configure/build/CTest sequence, then update the CI static contract and docs so drift is detected locally.

**Tech Stack:** GitHub Actions, PowerShell 7, CMake/CTest presets, Apple Xcode/Metal command-line tooling.

---

## Context

`renderer-metal-apple-host-evidence` already exists as a reviewed `run-validation-recipe` entry. The gap is that hosted `macOS Metal CMake` currently proves the `ci-macos-appleclang` build/test lane, but it does not execute the reviewed recipe entrypoint that also runs `check-apple-host-evidence.ps1 -RequireReady`.

Official/current-doc checks used for this slice:

- GitHub Actions workflow syntax supports workflow YAML under `.github/workflows`, `run` steps, and explicit `jobs.<job_id>.steps[*].shell`.
- Apple Xcode command-line tooling requires Xcode to be installed and selected as the active developer directory for Xcode-only tools such as `xcodebuild`.
- Apple command-line tools docs distinguish full Xcode from the Command Line Tools package; the recipe intentionally asks for full Xcode/Metal host evidence rather than inferring readiness from another backend.
- Apple Metal docs expose `metal`/`metallib` command-line library generation through `xcrun`, and Metal capability support is platform/GPU-family specific.
- Context7 `/kitware/cmake` confirms `ctest --preset` uses CMake test presets and supports focused regex/parallel options; the repository keeps CMake/CTest preset lanes as the reproducible CI surface.

## Constraints

- Do not remove or weaken the existing full macOS configure/build/CTest steps.
- Do not mark `metal-apple` ready.
- Do not claim Metal visible presentation, Apple package readiness, or broad renderer quality.
- Do not expose public native Metal/RHI handles.
- Keep the workflow least-privilege and pinned-action posture unchanged.
- Keep `tools/check-ci-matrix.ps1` as the local CI drift guard.

## Tasks

- [x] Add a `Renderer Metal Apple host evidence recipe` step to `.github/workflows/validate.yml` under the existing macOS job.
- [x] Pass the hosted logical CPU count to `tools/validate-renderer-metal-apple.ps1 -Jobs`.
- [x] Preserve the subsequent full `cmake --preset ci-macos-appleclang`, build, and CTest steps.
- [x] Update `tools/check-ci-matrix.ps1` so local static validation catches removal of the recipe step.
- [x] Update `docs/workflows.md`, `docs/testing.md`, the production backlog row, and this registry with the new hosted evidence boundary.
- [x] Run focused static validation.
- [x] Run full validation.

## Done When

- `tools/check-ci-matrix.ps1` proves the macOS job contains `Renderer Metal Apple host evidence recipe` and `tools/validate-renderer-metal-apple.ps1 -Jobs`.
- Local docs/static checks pass.
- The repository still reports `unsupportedProductionGaps = []`, while `metal-apple` remains host-gated and broad renderer quality remains unclaimed.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | Passed | Proved the macOS job contains the reviewed recipe step. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed | Text/docs workflow formatting gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest compose and JSON contracts agree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent-surface drift gate for workflow/documentation sync. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | Passed | Reviewed recipe allowlist and host-gate behavior stayed green. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | `unsupported_gaps=0`; no readiness promotion was introduced. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent surface and script hygiene stayed green. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe renderer-metal-apple-host-evidence` | Passed | Returned the reviewed `validate-renderer-metal-apple.ps1` command plan. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe renderer-metal-apple-host-evidence` | Rejected as expected | Failed closed with `missing-host-gate-acknowledgement`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe renderer-metal-apple-host-evidence -HostGateAcknowledgements metal-apple -TimeoutSeconds 30` | Failed as expected on Windows | Reported `apple-host-evidence-check: host-gated` with missing macOS/Xcode/Metal blockers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full local validation passed: 19 static checks, build, tidy smoke, and 85/85 CTest tests. |
| PR #386 hosted checks | Passed | `PR Gate`, `macOS Metal CMake`, `Windows MSVC`, Linux lanes, full repository static analysis, iOS Simulator smoke, and CodeQL passed for head `2079ca34050f930bad98d8775661cd5d37f4cb7f`; merged as `12fde1bf7ea369e97a45d95a0ee271b4e2515b3d`. |
| PR #386 macOS recipe log | Passed | The hosted `macOS Metal CMake` job ran `Renderer Metal Apple host evidence recipe`, reported `apple-host-evidence-check: ready`, built Metal renderer evidence tests, ran `MK_backend_scaffold_tests` and `MK_renderer_quality_matrix_tests`, and ended with `renderer-metal-apple-check: ok`. |
