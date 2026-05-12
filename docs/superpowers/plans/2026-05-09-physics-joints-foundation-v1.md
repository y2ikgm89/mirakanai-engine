# Physics Joints Foundation Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic first-party 3D joints foundation for production-minimum generated-game constraints over the now-stable collision/query/contact/CCD/character-policy surface.

**Architecture:** Keep joints as dependency-free `mirakana_physics` value types and deterministic solver helpers. Do not add middleware, native handles, hidden backend state, or broad physics readiness. Start with explicit generated-game constraints that can be replay-tested before benchmark gates or optional Jolt/native adapter work.

**Tech Stack:** C++23, `mirakana_physics`, `mirakana_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.

---

**Plan ID:** `physics-joints-foundation-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `physics-1-0-collision-system` Phase P1.  
**Previous Slice:** [2026-05-09-physics-character-dynamic-interaction-policy-v1.md](2026-05-09-physics-character-dynamic-interaction-policy-v1.md)

## Context

- Scene/package collision authoring, exact 3D shape sweeps, contact manifolds, CCD foundation, and character/dynamic policy are complete.
- `PhysicsWorld3D::step` remains the deterministic discrete default; `PhysicsWorld3D::step_continuous` remains opt-in translational dynamic-vs-static primitive CCD.
- Generated-game physics can now express collision filtering, trigger reports, conservative controller movement, explicit dynamic push/step/ground policy rows, and package-authored collision scenes without middleware.
- The master plan recommends first-party joints before physics benchmark/determinism gates and optional Jolt/native adapter evaluation.
- This child plan is intentionally a medium-sized capability phase inside the `physics-1-0-collision-system` gap burn-down, not an isolated one-function slice and not a broad all-physics completion plan.
- After this child completes, stay on the same gap for `physics-benchmark-determinism-gates-v1`, the Jolt/native adapter gate or explicit 1.0 exclusion decision, and `physics-1-0-collision-system-closeout-v1`.

## Constraints

- Keep `engine/physics` independent from OS APIs, GPU APIs, asset formats, editor code, and middleware.
- Do not add Jolt/native backends, broad middleware parity, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, oriented boxes, mesh/convex casts, vehicles, ragdoll authoring, or broad physics readiness.
- Do not change default integration or collision behavior unless the new explicit joint API is called.
- Prefer explicit request/config/result rows with stable diagnostics over implicit world mutation.
- Joint solver behavior must be deterministic under body creation order and replay.

## Done When

- Public joint value types describe the accepted generated-game joint cases, including body ids, anchors or rest distance, solver settings, status, diagnostics, and deterministic per-joint rows.
- Tests prove valid constraints, invalid requests, disabled/static/dynamic combinations, deterministic ordering, reciprocal body lookup, no hidden middleware/native state, and unchanged default stepping when no joint API is called.
- Docs, manifest, skills, registry, and the master-plan ledger say joints foundation is implemented while benchmark gates, optional Jolt/native backends, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, oriented boxes, mesh/convex casts, vehicles, ragdolls, and broad physics readiness remain unsupported.
- The `physics-1-0-collision-system` unsupported gap remains non-ready after this phase unless benchmark/determinism gates and closeout evidence have also landed.
- Focused build/test commands, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/tool blocker.

## Task 1: Joint Contract

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`

- [x] Add RED tests named `3d physics distance joints solve deterministic body pairs`, `3d physics joints reject invalid requests without mutation`, and `3d physics joints keep default step unchanged when no joint solve is called`.
- [x] Declare the public opt-in API in `physics3d.hpp` before `PhysicsWorld3D`: `PhysicsJoint3DStatus`, `PhysicsJoint3DDiagnostic`, `PhysicsDistanceJoint3DDesc`, `PhysicsJointSolve3DConfig`, `PhysicsJointSolve3DDesc`, `PhysicsJointSolve3DRow`, and `PhysicsJointSolve3DResult`.
- [x] Declare `[[nodiscard]] PhysicsJointSolve3DResult solve_physics_joints_3d(PhysicsWorld3D& world, const PhysicsJointSolve3DDesc& desc);` after the `PhysicsWorld3D` class so joint solving is explicit and opt-in.
- [x] Keep the accepted v1 joint case to first-party distance constraints over current 3D body ids and world-space anchors derived from `local_anchor_first` / `local_anchor_second`; do not introduce persistent joint storage in `PhysicsWorld3D`.
- [x] Run `cmake --build --preset dev --target mirakana_core_tests --config Debug`.
- [x] Expected RED: compile or test failure mentioning the new joint API/types before implementation.

## Task 2: Deterministic Constraint Evaluation

**Files:**
- Modify: `engine/physics/src/physics3d.cpp`
- Modify: `tests/unit/core_tests.cpp`

- [x] Implement validation first: reject non-finite anchors/rest distance, negative rest distance, missing body ids, both bodies static, zero iterations, and non-finite tolerance with stable diagnostics.
- [x] Implement deterministic row ordering from `desc.distance_joints` source index. Each row records source index, body ids, previous distance, target distance, residual distance, applied correction for each body, and per-row diagnostic.
- [x] Solve distance error by inverse-mass weighted positional correction. Static bodies receive zero correction; dynamic/dynamic bodies split correction by inverse mass. Preserve velocity until a later benchmark/determinism gate explicitly accepts velocity projection.
- [x] Keep body mutation scoped to `solve_physics_joints_3d`; `PhysicsWorld3D::step`, `step_continuous`, `contacts`, and `contact_manifolds` behavior must remain unchanged when no joint solve is called.
- [x] Run `ctest --preset dev -R mirakana_core_tests --output-on-failure`.
- [x] Expected GREEN: focused physics tests pass.

## Task 3: Integration And Scope Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`

- [x] Add a dynamic/static distance-joint test proving the dynamic body moves and the static body does not.
- [x] Add a dynamic/dynamic replay test that builds two worlds with the same bodies/joints and verifies matching positions and joint rows after the same solve.
- [x] Add a missing-body and both-static invalid-request test proving no body positions mutate.
- [x] Add a default-step regression test that creates the same bodies without calling `solve_physics_joints_3d`, calls `step`, and proves no joint correction is applied implicitly.
- [x] Keep Jolt/native backends, persistent joint assets, ragdolls, vehicles, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, mesh/convex casts, oriented boxes, and broad physics readiness outside this plan.
- [x] Run `cmake --build --preset dev --target mirakana_core_tests --config Debug` and `ctest --preset dev -R mirakana_core_tests --output-on-failure`.

## Task 4: Docs, Manifest, Skills, And Validation

**Files:**
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
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Update capability language only after behavior is green. Mark joints foundation implemented, but keep `physics-1-0-collision-system` non-ready until benchmark/determinism gates and closeout evidence land.
- [x] Update `engine/agent/manifest.json` notes/status honestly: the active gap remains `partly-ready`; `recommendedNextPlan` should move to `physics-benchmark-determinism-gates-v1` only after this child plan is complete.
- [x] Update the master plan Physics gap burn-down table from P1 active to P1 completed and P2 next after implementation validation passes.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Record validation evidence in this plan and then mark this plan completed.

## Self-Review

- Scope is first-party generated-game joint foundation, not physics middleware parity.
- This plan intentionally avoids optional Jolt/native adapters, benchmark promotion, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, mesh/convex casts, vehicles, ragdolls, and broad physics readiness.
- The active pointer must remain synchronized with `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_core_tests --config Debug` | PASS | Focused build passed after the joint API and solver implementation. |
| `ctest --preset dev -R mirakana_core_tests --output-on-failure` | PASS | Focused unit test lane passed after review fixes for self-joint, invalid rest distance, non-finite tolerance, and local-anchor coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest/schema synchronization check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-facing docs, manifest, registry, and static integration checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Production readiness audit still keeps `physics-1-0-collision-system` non-ready with benchmark and adapter gates remaining. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public `mirakana_physics` API boundary accepted the explicit joint value types and solver entrypoint. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Repository formatting check passed. |
| `git diff --check` | PASS | No whitespace errors in the working-tree diff. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Coherent slice validation passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Slice-closing build passed. |
