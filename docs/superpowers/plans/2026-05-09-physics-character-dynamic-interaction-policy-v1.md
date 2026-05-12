# Physics Character/Dynamic Interaction Policy Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Define a deterministic generated-game 3D character/dynamic interaction policy over the existing first-party query, contact manifold, and CCD surface.

**Architecture:** Keep policy data in dependency-free `mirakana_physics` value types and pure helper functions. Do not introduce middleware, native handles, or hidden runtime state. The policy should make generated-game choices explicit for push, step, slope, grounded, trigger, and dynamic-body interaction semantics rather than silently expanding the conservative character controller.

**Tech Stack:** C++23, `mirakana_physics`, `mirakana_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.

---

**Plan ID:** `physics-character-dynamic-interaction-policy-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Previous Slice:** [2026-05-09-physics-ccd-foundation-v1.md](2026-05-09-physics-ccd-foundation-v1.md)

## Context

- Physics Scene Package Collision Authoring v1, Broader Exact Casts And Sweeps v1, Contact Manifold Stability v1, and CCD Foundation v1 are complete.
- `PhysicsWorld3D::shape_sweep` and `move_physics_character_controller_3d` remain conservative bounds-based controller helpers.
- `PhysicsWorld3D::exact_shape_sweep`, `PhysicsWorld3D::contact_manifolds`, and `PhysicsWorld3D::step_continuous` now provide the query/contact/CCD substrate needed to define generated-game interaction policy.
- The master plan still needs a narrow production policy before joints, benchmark gates, or optional Jolt/native adapter work.

## Constraints

- Keep `engine/physics` independent from OS APIs, GPU APIs, asset formats, editor code, and middleware.
- Do not add dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, joints, optional Jolt/native backends, oriented boxes, mesh/convex casts, or broad physics readiness.
- Do not change default `PhysicsWorld3D::step` replay behavior.
- Do not make a full platformer, vehicle, or character-action middleware claim.
- Prefer explicit value-type requests/results with deterministic diagnostics over implicit controller mutation.

## Done When

- A public policy contract defines generated-game push, step, slope, grounded, trigger, and dynamic-body interaction semantics for accepted 3D controller cases.
- Tests prove deterministic results, invalid-request diagnostics, layer/mask and trigger behavior, no hidden middleware/native state, and unchanged default stepping.
- Docs, manifest, skills, registry, and this master-plan ledger say the dynamic interaction policy is implemented while joints, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, optional Jolt/native backends, mesh/convex casts, benchmarks, and broad physics readiness remain unsupported.
- Focused build/test commands, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/tool blocker.

## Task 1: Policy Contract

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`

- [x] Add RED tests for explicit generated-game character/dynamic policy rows.
- [x] Add public request/result/config value types with stable diagnostics.
- [x] Run focused build and confirm RED before implementation.

## Task 2: Deterministic Policy Evaluation

**Files:**
- Modify: `engine/physics/src/physics3d.cpp`
- Modify: `tests/unit/core_tests.cpp`

- [x] Implement the minimal accepted policy over existing query/contact/CCD helpers.
- [x] Preserve default `PhysicsWorld3D::step` and existing controller behavior unless the new explicit policy API is called.
- [x] Prove deterministic ordering, layer/mask filtering, trigger behavior, and invalid-request non-mutation.

## Task 3: Scope And Integration Evidence

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: docs and manifest guidance after behavior is green.

- [x] Add tests for step/slope/grounded/push decisions in accepted generated-game cases.
- [x] Keep dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, joints, optional Jolt/native backends, mesh/convex casts, and broad physics readiness outside the ready claim.
- [x] Run focused validation.

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

- [x] Update capability language only after behavior is green.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Record validation evidence and then mark this plan completed.

## Self-Review

- Scope is a policy contract over existing first-party physics, not a middleware parity claim.
- The plan intentionally avoids joints, optional Jolt/native adapters, mesh/convex casts, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, and broad physics readiness.
- The active pointer must remain synchronized with `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Implementation Summary

`PhysicsCharacterDynamicPolicy3DDesc`, `PhysicsCharacterDynamicPolicy3DRowKind`,
`PhysicsCharacterDynamicPolicy3DRow`, `PhysicsCharacterDynamicPolicy3DResult`, and
`evaluate_physics_character_dynamic_policy_3d` now define a pure, non-mutating generated-game
character/dynamic policy over existing first-party 3D physics primitives. The policy reports
deterministic solid contact, trigger overlap, dynamic push proposal, step-up, and ground-probe rows;
uses reciprocal layer/mask filtering; keeps triggers non-blocking in the policy path; preserves
dynamic body state; validates invalid requests without mutation; and rejects steps whose final
lowered capsule would land inside a blocker. `PhysicsWorld3D::step` and the conservative
`move_physics_character_controller_3d` path remain unchanged. The CCD config was also cleaned by
removing the unused `PhysicsContinuousStep3DConfig::max_iterations` field; CCD remains a
single-row translational dynamic-vs-static primitive CCD foundation.

Still unsupported: dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, joints, optional Jolt/native
backends, oriented boxes, mesh/convex casts, physics benchmarks, middleware parity, and broad
physics readiness.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_core_tests --config Debug` | PASS | Focused build after policy implementation and CCD config cleanup. |
| `ctest --preset dev -R mirakana_core_tests --output-on-failure` | PASS | Covers policy rows, trigger opt-in, reciprocal masks, dynamic push proposals, step acceptance/blocking, grounded propagation, invalid requests, and existing 3D physics regression tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary accepted after adding policy value types and helper. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting clean after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest/schema contract sync accepted with `physics-joints-foundation-v1` as the active child plan. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration guidance accepted after adding policy API names and removing stale dynamic push/step unsupported claims. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Unsupported production gaps remain explicitly counted; this slice does not promote broad physics readiness. |
| `git diff --check` | PASS | No whitespace errors; Git reported CRLF conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed; Metal/iOS diagnostics remain host-gated on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default dev build completed after validation. |
