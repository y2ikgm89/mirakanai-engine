# C++23 Release Package Artifact CI Evidence v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the existing Windows C++23 release CI lane execute the same CPack ZIP, SHA-256 sidecar, and installed SDK payload artifact validation used by the canonical local release package lane.

**Architecture:** Keep this as a governance and CI evidence slice. Reuse the existing `Assert-ReleasePackageArtifacts` helper from `tools/release-package-artifacts.ps1` inside `tools/evaluate-cpp23.ps1 -Release`, then lock the workflow and script contract with `tools/check-ci-matrix.ps1`; do not add new dependencies, signing, upload, notarization, or cross-platform package claims.

**Tech Stack:** PowerShell validation wrappers, GitHub Actions workflow YAML, CPack, existing CMake C++23 release evaluation presets.

**Status:** Completed on 2026-05-08.

---

## Goal

- Extend the existing Windows `tools/evaluate-cpp23.ps1 -Release` CI lane so CPack output is not only generated and uploaded, but also validated for current ZIP name, `.zip.sha256` sidecar integrity, and required SDK archive entries.
- Add the `.zip.sha256` sidecar to the Windows package artifacts upload.
- Update static CI contract checks, docs, plan registry, and manifest notes so the `full-repository-quality-gate` records this as narrower C++23 release package artifact CI execution evidence while broader package matrix and analyzer profile work remains unsupported.

## Context

- `tools/package.ps1` already calls `Assert-ReleasePackageArtifacts` after `cpack --preset release`.
- `tools/evaluate-cpp23.ps1 -Release` currently runs configure, build, CTest, install, installed SDK validation, and CPack for `cpp23-release-eval`, but it does not assert the generated ZIP sidecar/hash/archive payload.
- `.github/workflows/validate.yml` already runs `./tools/evaluate-cpp23.ps1 -Release` in the Windows job and uploads `out/build/cpp23-release-preset-eval/*.zip`.
- `full-repository-quality-gate` remains `partly-ready`; this slice reduces only the Windows C++23 release package artifact evidence gap.

## Constraints

- Do not create a new workflow job or duplicate the full package lane; extend the existing C++23 release lane.
- Do not add dependencies, caches, signing, upload, notarization, symbol publishing, or release deployment.
- Do not claim full package/build CI matrix readiness or full cross-platform package matrix readiness; Linux/macOS package execution evidence and broader static analyzer profile remain separate work.
- Keep `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` host-independent and static.

## Done When

- `tools/evaluate-cpp23.ps1 -Release` imports `release-package-artifacts.ps1` and runs `Assert-ReleasePackageArtifacts -BuildDir out/build/cpp23-release-preset-eval` after `cpack --preset cpp23-release-eval`.
- `.github/workflows/validate.yml` uploads both the C++23 release ZIP and `.zip.sha256` sidecar in `windows-packages`.
- `tools/check-ci-matrix.ps1` rejects drift if the C++23 release artifact assertion or sidecar artifact upload is removed.
- Docs, registry, and manifest notes record `C++23 Release Package Artifact CI Evidence v1` without moving `full-repository-quality-gate` out of `partly-ready`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-release-package-artifacts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact blockers.

## File Plan

- Modify `tools/evaluate-cpp23.ps1`: load the release package artifact helper and assert C++23 release CPack artifacts after CPack generation.
- Modify `.github/workflows/validate.yml`: upload `*.zip.sha256` with Windows C++23 release artifacts.
- Modify `tools/check-ci-matrix.ps1`: assert the script-level artifact check and workflow sidecar upload.
- Modify `tools/check-ai-integration.ps1`: require this CI evidence contract in the repository governance check.
- Modify `docs/superpowers/plans/README.md`, `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/testing.md`, and `engine/agent/manifest.json`: document the narrowed ready surface and remaining gaps.

## Tasks

### Task 1: C++23 Release Artifact Assertion

- [x] Add the helper import to `tools/evaluate-cpp23.ps1`.
- [x] Store the C++23 release build directory in a variable and reuse it for install and artifact validation.
- [x] Run `Assert-ReleasePackageArtifacts` after `Invoke-CheckedCommand $tools.CPack --preset cpp23-release-eval`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-release-package-artifacts.ps1` to keep the helper self-test green.

### Task 2: CI Workflow And Static Contract

- [x] Add `out/build/cpp23-release-preset-eval/*.zip.sha256` to the Windows package artifact upload in `.github/workflows/validate.yml`.
- [x] Extend `tools/check-ci-matrix.ps1` to assert `tools/evaluate-cpp23.ps1` contains `release-package-artifacts.ps1`, `Assert-ReleasePackageArtifacts`, and `cpp23-release-preset-eval`.
- [x] Extend `tools/check-ci-matrix.ps1` to assert the Windows workflow uploads the `.zip.sha256` sidecar.
- [x] Add an ordered `tools/check-ci-matrix.ps1` assertion that `Assert-ReleasePackageArtifacts -BuildDir $releaseBuildDir` stays after `Invoke-CheckedCommand $tools.CPack --preset cpp23-release-eval`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1`.

### Task 3: Documentation And Manifest Sync

- [x] Update plan registry and current docs with the new completed slice summary.
- [x] Update the master plan and manifest `full-repository-quality-gate` notes so the remaining build/package evidence gap is still explicit.
- [x] Update `tools/check-ai-integration.ps1` to require the new plan, workflow, script, docs, and manifest evidence.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Mark this plan complete with validation evidence and commit the coherent slice.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-release-package-artifacts.ps1` | PASS | `release-package-artifacts-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | PASS | `ci-matrix-check: ok`; includes script presence, ordered CPack-before-artifact-assert check, and workflow `.zip.sha256` upload contract. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Release` | PASS | C++23 Debug eval build/tests passed, C++23 Release eval build/tests/install/installed consumer passed, CPack generated `GameEngine-0.1.0-Windows-AMD64.zip` and `.zip.sha256`, and `cpp23-verification: ok` after `Assert-ReleasePackageArtifacts`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| RED `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after adding static evidence check | Expected fail | Failed because this plan did not yet contain the required `full cross-platform package matrix readiness` unsupported-claim text; fixed the plan wording. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; 11 unsupported gaps remain and `full-repository-quality-gate` remains `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `git diff --check` | PASS | No whitespace errors after removing trailing blank lines; Git reported LF-to-CRLF working-copy warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 29/29 CTest tests passed. Diagnostic-only host gates remain Metal tools and Apple packaging on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev preset configure/build completed successfully. |
