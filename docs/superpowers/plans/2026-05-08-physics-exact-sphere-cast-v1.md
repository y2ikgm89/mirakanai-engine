# Physics Exact Sphere Cast v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent exact 3D sphere cast query for the current first-party `PhysicsWorld3D` primitives.

**Architecture:** Keep the slice inside dependency-free `mirakana_physics` C++23 value contracts. Add a diagnostic result API on `PhysicsWorld3D` that sweeps a sphere exactly against current AABB, sphere, and vertical capsule target primitives without replacing the existing conservative `shape_sweep` query. This does not add joints, CCD, scene/package collision assets, native backend handles, Jolt integration, or middleware.

**Tech Stack:** C++23, `mirakana_physics`, `mirakana_math`, `mirakana_core_tests`, repository CMake/CTest plus PowerShell 7 scripts under `tools/`.

---

## Goal

Reduce the master-plan physics exact-query loophole by proving one exact primitive cast that generated games can call without middleware or backend-specific handles.

## Context

- `PhysicsWorld3D::shape_sweep` is explicitly conservative and sweeps collision bounds, so it can report false positives for rounded primitives.
- `mirakana_physics` already owns all first-party 3D primitive metadata needed for AABB, sphere, and vertical capsule targets.
- The current manifest and docs still classify exact primitive casts as follow-up work.

## Constraints

- Keep `PhysicsWorld3D::shape_sweep` unchanged as the conservative compatibility query.
- Add only exact sphere cast coverage for current first-party 3D target primitives; exact capsule casts, exact box casts, mesh casts, arbitrary convex casts, joints, CCD, scene/package collision assets, dynamic push/step policies, Jolt/native backends, and middleware remain out of scope.
- Keep query filtering consistent with existing 3D sweeps: collision mask, ignored body, disabled bodies, and trigger inclusion are honored.
- Keep public API names in `mirakana::`, with value-type descriptors/results and no public native handles.

## Done When

- `PhysicsWorld3D` exposes `PhysicsExactSphereCast3DDesc`, `PhysicsExactSphereCast3DHit`, `PhysicsExactSphereCast3DStatus`, `PhysicsExactSphereCast3DDiagnostic`, `PhysicsExactSphereCast3DResult`, and `exact_sphere_cast`.
- Unit tests prove exact sphere hits, initial overlaps, trigger/filter behavior, false-positive rejection compared with bounds sweeps, AABB target hits, capsule target hits, and invalid request diagnostics.
- Docs, plan registry, master plan, manifest, and static checks describe exact sphere cast as implemented while preserving unsupported claims for broader exact casts, joints, CCD, scene/package collision assets, dynamic push/step policies, and Jolt/native backends.
- Focused `mirakana_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, format checks, changed-file `clang-tidy`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before committing this slice. Full-repository `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` must pass or have a concrete host/time blocker recorded with targeted tidy evidence.

## Files

- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`
- Modify: `engine/physics/src/physics3d.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `docs/architecture.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`

## Tasks

### Task 1: Red Tests

- [x] Add exact sphere cast tests to `tests/unit/core_tests.cpp`:
  - `3d physics exact sphere cast hits sphere surface without bounds inflation`
  - `3d physics exact sphere cast rejects conservative bounds false positive`
  - `3d physics exact sphere cast reports initial overlaps and honors filters`
  - `3d physics exact sphere cast supports aabb and capsule targets`
  - `3d physics exact sphere cast handles rounded edges caps and deterministic ties`
  - `3d physics exact sphere cast rejects invalid requests`
- [x] Run focused `mirakana_core_tests` build and record the expected compile failure because the new exact sphere cast API does not exist.

### Task 2: Physics Runtime Contract

- [x] Add public exact sphere cast descriptor, hit, status, diagnostic, result, and `PhysicsWorld3D::exact_sphere_cast`.
- [x] Implement finite-input validation, deterministic filtering, nearest-hit selection, and stable tie-breaking by body id.
- [x] Implement exact moving-sphere intersections against current AABB, sphere, and vertical capsule target primitives.

### Task 3: Documentation And Contract Checks

- [x] Update manifest/docs/master plan/registry to describe the new exact sphere cast path and remaining unsupported physics work.
- [x] Update static checks to assert the exact sphere cast API and honest unsupported boundaries.

### Task 4: Validation And Commit

- [x] Run focused `mirakana_core_tests` build and CTest.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` or record a concrete blocker with changed-file tidy evidence.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and format if needed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `feat: add physics exact sphere cast`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` | Expected failure | RED build failed because `PhysicsExactSphereCast3DDesc`, `PhysicsWorld3D::exact_sphere_cast`, and related result/status types did not exist yet. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` | Passed | GREEN focused build after implementation and after adding the loophole coverage tests. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R mirakana_core_tests` | Passed | `mirakana_core_tests` passed with exact sphere cast coverage for sphere, false-positive rejection, initial overlap, filters, disabled body skip, AABB face/corner, capsule side/cap, deterministic tie, and invalid requests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | `production-readiness-audit-check: ok`; unsupported gaps remain intentionally tracked outside this slice. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public API boundary check accepted the new `mirakana_physics` value contracts. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Failed, then passed | Initial format check caught new `physics3d.cpp` formatting; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` fixed it and the rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | Blocked by time | Full-repository tidy generated very large existing warning output and timed out after 300 seconds and again after 900 seconds before reaching completion. |
| Changed-file `clang-tidy` for `engine/physics/src/physics3d.cpp` and `tests/unit/core_tests.cpp` | Passed | Used the repository compile database and `.clang-tidy`; the command exited 0 for the changed implementation and tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed: license/config/schema/agent/production readiness/CI/dependency/toolchain/API boundary/bounded tidy/build/test. Windows host-gated Metal/iOS diagnostics were reported as expected. CTest passed 29/29. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Full dev preset build completed after validation. |

## Status

**Status:** Completed.
