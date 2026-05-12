# Physics Character Controller Collision Authoring v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the first narrow physics production gap by adding first-party 3D character-controller movement and authored collision scene rows over the existing deterministic `PhysicsWorld3D` sweep/contact foundation.

**Architecture:** Keep the slice inside `mirakana_physics` as dependency-free C++23 value contracts. Build a conservative capsule controller on top of `PhysicsWorld3D::shape_sweep` and a deterministic authored-collision builder that creates `PhysicsWorld3D` bodies from reviewed first-party rows. Do not add native backend handles, Jolt integration, exact primitive casts, joints, CCD, dynamic-body impulse exchange, scene/editor dependencies, or asset file IO.

**Tech Stack:** C++23, `mirakana_physics`, `mirakana_math`, `mirakana_core_tests`, repository CMake/CTest plus PowerShell 7 scripts under `tools/`.

---

## Goal

Reduce the master-plan physics loophole from "no character controller or authored collision path" to a narrow first-party production-minimum contract that generated games can use without middleware leakage.

## Context

- `mirakana_physics` already has deterministic 3D AABB/sphere/capsule contacts, collision filtering, trigger overlaps, conservative bounds-based shape sweeps, and iterative contact resolution.
- `engine/agent/manifest.json` still marks character controllers and authored collision assets as future work.
- Master plan row `Audio, physics, navigation, and AI` names `physics-character-controller-collision-authoring-v1` as a preferred child split.

## Constraints

- Character movement is a conservative capsule bounds sweep contract, not an exact capsule cast or full kinematic platform controller.
- Authored collision rows build in-memory `PhysicsWorld3D` bodies only; no asset serialization, scene import, editor GUI, runtime package files, or automatic middleware adapter selection.
- Native/Jolt backend requests must fail closed as unsupported.
- Do not add joints, CCD, step offsets, slope-limit locomotion policy beyond grounded contact reporting, dynamic body pushing, trigger events, scene/renderer/editor dependencies, public native handles, or third-party dependencies.
- Keep all new public API names in `mirakana::` and headers minimal.

## Done When

- `mirakana_physics` exposes a deterministic 3D character controller request/result with finite-input validation, conservative capsule sweep movement, contact rows, grounded reporting, and initial-overlap failure.
- `mirakana_physics` exposes first-party authored 3D collision scene rows that deterministically build a `PhysicsWorld3D`, reject duplicate/invalid names and invalid body descriptors, and fail closed for requested native backends.
- Unit tests prove wall constraints, grounded downward movement, initial-overlap rejection, authored collision world creation, duplicate-name rejection, and unsupported native backend rejection.
- Docs, plan registry, master plan, manifest, and static checks describe the new narrow physics path and preserve unsupported claims for exact casts, joints, CCD, Jolt/native backend, scene/editor asset serialization, and dynamic push policy.
- Focused `mirakana_core_tests`, static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before committing this slice.

## Files

- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`
- Modify: `engine/physics/src/physics3d.cpp`
- Modify: `tests/unit/core_tests.cpp`
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

- [x] Add character controller wall/ground/initial-overlap tests to `tests/unit/core_tests.cpp`.
- [x] Add authored collision scene success/duplicate/native-backend tests.
- [x] Run focused `mirakana_core_tests` and record expected compile failure before new APIs exist.

### Task 2: Physics Runtime Contracts

- [x] Add public character controller request/result/status/diagnostic/contact rows.
- [x] Implement conservative capsule movement using `PhysicsWorld3D::shape_sweep`, stable contact rows, finite-input validation, max-iteration guard, and grounded contact reporting.
- [x] Add public authored collision scene rows and deterministic world builder with fail-closed native backend request handling.

### Task 3: Documentation And Contract Checks

- [x] Update manifest/docs/master plan/registry to describe the completed narrow physics path.
- [x] Update static checks to assert the new physics APIs and unsupported boundaries.

### Task 4: Validation And Commit

- [x] Run focused `mirakana_core_tests` build and CTest.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and format if needed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `feat: add physics character controller collision authoring`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` | Expected RED | Failed on missing `PhysicsCharacterController3DDesc`, `PhysicsCharacterController3DStatus`, `move_physics_character_controller_3d`, `PhysicsAuthoredCollisionScene3DDesc`, and `build_physics_world_3d_from_authored_collision_scene` before implementation. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` | Passed | Focused `mirakana_core_tests` build passed after adding the physics controller and authored collision APIs. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R mirakana_core_tests` | Passed | Focused `mirakana_core_tests` passed with new controller and authored collision coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | JSON contract checks include the updated mirakana_physics manifest status/purpose. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration checks include the physics APIs, docs, skills, manifest guidance, and unsupported boundaries. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Audit still reports 11 known unsupported gaps and keeps the broader 1.0 gaps honest. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public `mirakana_physics` header additions stayed within the allowed API boundary. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Failed then Passed | Initial run flagged `engine/physics/src/physics3d.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied repository formatting and the final format check passed. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` | Passed | Focused build passed again after formatting. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R mirakana_core_tests` | Passed | Focused `mirakana_core_tests` passed again after formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed with known diagnostic-only Metal/Apple host gates and Android host readiness reported honestly. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Default dev build completed successfully. |

## Status

**Status:** Completed.
