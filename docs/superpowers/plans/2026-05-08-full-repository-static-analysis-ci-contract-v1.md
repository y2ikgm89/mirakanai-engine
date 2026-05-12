# Full Repository Static Analysis CI Contract v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed GitHub Actions static-analysis lane that runs full-repository `tools/check-tidy.ps1 -Strict` on `ubuntu-latest` and is guarded by the local CI matrix/static contract checks.

**Plan ID:** `full-repository-static-analysis-ci-contract-v1`

**Status:** Completed.

**Architecture:** Keep this as workflow/static-contract work. The lane uses existing repository tooling (`tools/check-tidy.ps1`) rather than introducing a second clang-tidy entrypoint, and local validation remains a static drift guard instead of claiming GitHub Actions executed locally.

**Tech Stack:** GitHub Actions YAML, PowerShell, `tools/check-ci-matrix.ps1`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`.

---

## Context

- Target unsupported gap: `full-repository-quality-gate`, currently `partly-ready`.
- Existing CI Matrix Contract Check v1 statically verifies Windows/Linux/sanitizer/macOS/iOS lanes but does not require a full-repository static-analysis job.
- `tools/check-tidy.ps1 -Strict` is the canonical wrapper for full-repository clang-tidy and compile database synthesis.
- GitHub Actions official structure is `jobs.<job_id>.runs-on` plus ordered `steps`; artifact upload should use the supported `actions/upload-artifact` major already used by this repository.

## Constraints

- Do not introduce a new analyzer wrapper or duplicate clang-tidy command construction.
- Do not claim local execution of GitHub Actions.
- Keep `full-repository-quality-gate` `partly-ready`; broader build/package execution evidence and broader analyzer profile work remain follow-up.
- Keep workflow changes focused on the new static-analysis lane and matching drift checks.
- Add RED static checks before adding the workflow job.

## Done When

- `tools/check-ci-matrix.ps1` fails before the workflow job exists, requiring a `static-analysis` job with `ubuntu-latest`, clang-tidy installation, `./tools/check-tidy.ps1 -Strict`, `actions/upload-artifact@v4`, and `static-analysis-tidy-logs`.
- `.github/workflows/validate.yml` has a focused `static-analysis` job using the existing tidy wrapper.
- `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` guard the workflow, docs, manifest notes, and completed plan status.
- Docs, manifest notes, plan registry, and master plan mention Full Repository Static Analysis CI Contract v1 while preserving remaining unsupported claims.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact blockers.

## File Plan

- Modify `tools/check-ci-matrix.ps1`: add RED assertions for the new `static-analysis` job.
- Modify `.github/workflows/validate.yml`: add the `static-analysis` job.
- Modify `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1`: add drift checks for static-analysis CI contract evidence.
- Modify `engine/agent/manifest.json`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`, `docs/workflows.md`, `docs/superpowers/plans/README.md`, and the master plan to synchronize the capability claim.

## Tasks

### Task 1: RED Static Check

- [x] Add `static-analysis` job assertions to `tools/check-ci-matrix.ps1`.
- [x] Require `name: Full Repository Static Analysis`, `runs-on: ubuntu-latest`, `actions/checkout@v4`, `sudo apt-get update && sudo apt-get install -y clang-tidy`, `./tools/check-tidy.ps1 -Strict`, `actions/upload-artifact@v4`, `name: static-analysis-tidy-logs`, `out/build/dev/compile_commands.json`, and `if-no-files-found: warn`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` and record the expected failure because `.github/workflows/validate.yml` has no `static-analysis` job yet.

### Task 2: Workflow Implementation

- [x] Add a `static-analysis` job to `.github/workflows/validate.yml` after the sanitizer lane.
- [x] Checkout the repository, install `clang-tidy`, run `./tools/check-tidy.ps1 -Strict` with `shell: pwsh`, and upload the compile database / CMake File API reply data under `static-analysis-tidy-logs` with `if: always()`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` and confirm it passes.

### Task 3: Docs, Manifest, And Static Contract

- [x] Update `engine/agent/manifest.json` notes for `full-repository-quality-gate` with Full Repository Static Analysis CI Contract v1 while keeping the gap `partly-ready`.
- [x] Update docs to say the workflow now includes a static-analysis lane, while local `ci-matrix-check` remains a static drift guard that does not execute GitHub Actions.
- [x] Add `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` assertions for the workflow, docs, manifest notes, and completed plan status.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit the coherent slice after validation and build pass.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | FAIL expected | Failed because `.github/workflows/validate.yml` was missing required job `static-analysis`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | PASS | Static CI matrix contract passed after adding the job. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`; read-only reviewer found closeout drift, which this completed pointer/doc sync resolves. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `unsupported_gaps=11`; `full-repository-quality-gate` remains `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Repository format check passed. |
| `git diff --check` | PASS | Only local LF-to-CRLF warnings were reported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed; CTest 29/29, with Metal/Apple lanes reported as host-gated diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Full development build passed after validation. |
