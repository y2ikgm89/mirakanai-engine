# Physics Benchmark Determinism Gates Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add deterministic, host-independent Physics 1.0 replay and budget gates over the first-party 3D collision/query/contact/CCD/character-policy/joint surface.

**Architecture:** Keep the gate dependency-free inside `mirakana_physics` and make it report deterministic counts, signatures, and diagnostics instead of wall-clock timings. Treat this as evidence for the first-party Physics 1.0 ready surface only; do not add middleware, native handles, persistent benchmark assets, or broad physics readiness.

**Tech Stack:** C++23, `mirakana_physics`, `mirakana_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.

---

**Plan ID:** `physics-benchmark-determinism-gates-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `physics-1-0-collision-system` Phase P2.  
**Previous Slice:** [2026-05-09-physics-joints-foundation-v1.md](2026-05-09-physics-joints-foundation-v1.md)

## Context

- Physics Scene Package Collision Authoring, broader exact casts/sweeps, contact manifold stability, CCD foundation, character/dynamic policy, and joints foundation are complete.
- `PhysicsWorld3D::step` remains the deterministic discrete default; CCD and joint solving remain explicit opt-in calls.
- This phase should prove the first-party surface is replayable and budgeted enough for small-to-medium generated games without relying on host timing noise.
- The next phase after this plan is the Jolt/native adapter gate or an explicit first-party 1.0 exclusion decision.

## Constraints

- Do not use wall-clock timing, threads, platform APIs, GPU APIs, middleware, native handles, or external benchmark harnesses.
- Do not mutate `PhysicsWorld3D` while evaluating a gate unless a caller explicitly passes copied worlds to a replay helper.
- Keep all counts and signatures stable across repeated runs with the same inputs.
- Keep dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, persistent joint assets, vehicles, ragdolls, oriented boxes, mesh/convex casts, Jolt/native backends, editor UX, and broad physics readiness outside this plan.

## Done When

- Public gate value types expose deterministic status, diagnostics, budget limits, observed counts, replay signatures, and per-surface rows.
- Tests prove deterministic replay signatures for duplicated worlds, budget pass/fail behavior, invalid request diagnostics, and no mutation during read-only gate evaluation.
- Docs, manifest, skills, registry, and the master-plan ledger say benchmark/determinism gates are implemented while the gap remains non-ready until the Jolt/native adapter gate or explicit exclusion and closeout land.
- Focused build/test commands, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/tool blocker.

## Task 1: Gate Contract

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`

- [x] Add RED tests named `3d physics determinism gate reports stable query and solver budgets`, `3d physics determinism gate rejects budget regressions without mutation`, and `3d physics replay signature is stable across duplicated worlds`.
- [x] Declare public value types before `PhysicsWorld3D`: `PhysicsDeterminismGate3DStatus`, `PhysicsDeterminismGate3DDiagnostic`, `PhysicsDeterminismGate3DConfig`, `PhysicsDeterminismGate3DCounts`, `PhysicsReplaySignature3D`, and `PhysicsDeterminismGate3DResult`.
- [x] Declare `[[nodiscard]] PhysicsReplaySignature3D make_physics_replay_signature_3d(const PhysicsWorld3D& world);` and `[[nodiscard]] PhysicsDeterminismGate3DResult evaluate_physics_determinism_gate_3d(const PhysicsWorld3D& world, const PhysicsDeterminismGate3DConfig& config);` after the `PhysicsWorld3D` class.
- [x] Keep the v1 contract count-based: bodies, broadphase pairs, contacts, contact manifolds, trigger overlaps, total contact points, and a replay signature over body ids, positions, velocities, inverse masses, enabled flags, trigger flags, shapes, layers, and masks.
- [x] Run `cmake --build --preset dev --target mirakana_core_tests --config Debug`.
- [x] Expected RED: compile or test failure mentioning the new gate API/types before implementation.

## Task 2: Deterministic Gate Evaluation

**Files:**
- Modify: `engine/physics/src/physics3d.cpp`
- Modify: `tests/unit/core_tests.cpp`

- [x] Implement validation first: reject non-finite budgets by using integer limits only, reject all-zero budget configs except explicit unlimited defaults, and report the first exceeded budget with stable diagnostics.
- [x] Compute observed counts through existing read-only public paths: `broadphase_pairs`, `contacts`, `contact_manifolds`, and `trigger_overlaps`.
- [x] Implement `make_physics_replay_signature_3d` as a deterministic hash over the current world rows using a fixed first-party hash routine and stable float bit representation; do not include pointer values, allocation addresses, wall-clock values, or host-specific data.
- [x] Prove `evaluate_physics_determinism_gate_3d` does not mutate body positions, velocities, accumulated forces, or enabled flags.
- [x] Run `ctest --preset dev -R mirakana_core_tests --output-on-failure`.
- [x] Expected GREEN: focused physics gate tests pass.

## Task 3: Cross-Surface Replay Coverage

**Files:**
- Modify: `tests/unit/core_tests.cpp`

- [x] Add a duplicated-world test that creates the same 3D world twice, evaluates broadphase/contact/manifold/trigger counts, solves the same explicit joint request on copied worlds, and verifies matching signatures after the same operations.
- [x] Add a budget-failure test that sets one low limit at a time for bodies, broadphase pairs, contacts, manifolds, trigger overlaps, and contact points; verify deterministic diagnostics and observed counts.
- [x] Add an invalid/default config test proving default config is unlimited and valid, while explicitly zeroing a required minimum budget row rejects only when the observed count exceeds that row.
- [x] Keep benchmark gates count-based and host-independent; do not add timing assertions or performance thresholds tied to this Windows host.
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

- [x] Update capability language only after behavior is green. Mark benchmark/determinism gates implemented, but keep `physics-1-0-collision-system` non-ready until the Jolt/native adapter gate or explicit exclusion and closeout evidence land.
- [x] Update `engine/agent/manifest.json` notes/status honestly: the active gap remains `partly-ready`; `recommendedNextPlan` should move to the Jolt/native adapter gate or explicit exclusion only after this child plan is complete.
- [x] Update the master plan Physics gap burn-down table from P2 active to P2 completed and P3 next after implementation validation passes.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Record validation evidence in this plan and then mark this plan completed.

## Self-Review

- Scope is deterministic replay and budget evidence, not benchmark timing or middleware parity.
- This plan intentionally avoids optional Jolt/native adapters, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, mesh/convex casts, vehicles, ragdolls, persistent joint assets, and broad physics readiness.
- The active pointer must remain synchronized with `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_core_tests --config Debug` | PASS | Built `mirakana_core_tests.exe` after P2 replay signature and determinism gate implementation. |
| `ctest --preset dev -R mirakana_core_tests --output-on-failure` | PASS | `1/1 Test #1: mirakana_core_tests` passed after P2 regression coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `physics-1-0-collision-system` remains `partly-ready` with `required=2`; `production-readiness-audit-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok` after running `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `git diff --check -- ...` | PASS | No whitespace errors; only CRLF normalization warnings from Git. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `29/29` CTest tests passed and `validate: ok`; Metal/iOS checks remain diagnostic host gates on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Debug preset configured and built all default targets. |
