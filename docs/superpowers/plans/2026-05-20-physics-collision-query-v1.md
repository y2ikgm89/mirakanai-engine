# Physics Collision Query v1 (2026-05-20)

**Plan ID:** `physics-collision-query-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is developer-owned gameplay/physics capability work, not a reopened Engine 1.0 production gap.

## Goal

Expand first-party physics collision query primitives so generated games can ask deterministic scene-query questions with explicit filters, diagnostics, replayable query rows, and selected package-visible counters without exposing middleware/native physics handles.

## Context

- Engine AI Behavior Authoring v1 completed reusable behavior document validation and package-visible behavior authoring counters in `sample_2d_desktop_runtime_package`.
- Existing physics work already covers baseline 2D/3D contacts, exact 3D shape sweeps/casts, contact manifolds, CCD rows, character/dynamic policy rows, distance joints, determinism gates, and selected generated package evidence.
- The production completion advanced track lists `physics-collision-query-v1` as the next developer-owned gameplay/physics capability for deterministic query ordering, layer/filter tests, invalid-shape diagnostics, package-visible query counters, and first-party API contracts.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep native physics middleware, public native handles, editor visualization parity, and broad automatic collision authoring out of scope.
- Reuse existing `MK_physics` world/query primitives where possible; do not add compatibility shims.
- Query contracts must be deterministic, value-oriented, budgeted, and testable without renderer, editor, platform, or middleware dependencies.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the next active developer-owned gameplay/physics capability after `engine-ai-behavior-authoring-v1` closeout.

### Done When

- `docs/superpowers/plans/README.md`, the readiness ledger, and the production master-plan index list this plan as the active developer-owned milestone.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Deterministic Query Batch Contract

**Status:** Completed.

### Goal

Define the smallest reusable collision query batch contract over existing physics world/query concepts, with deterministic ordering, filter/layer semantics, invalid-query diagnostics, and no middleware/native-handle exposure.

### Done When

- RED tests or static guards fail first for missing query batch validation/execution behavior.
- `MK_physics` exposes value-only request/result/diagnostic rows for reusable generated-game collision query authoring.
- Focused tests prove deterministic ordering, layer/mask filtering, raycast trigger inclusion, shape-sweep trigger inclusion/exclusion, invalid shape/query diagnostics, default-unbounded query counts, and explicit budget rejection.

## Phase 2: Package Query Evidence

**Status:** Completed.

### Goal

Adopt the collision query contract in selected generated gameplay package paths so package smokes can report deterministic query counters.

### Done When

- A committed sample or generated package path reports package-visible collision query counters through existing validation recipes.
- Docs, manifest fragments, schemas/static checks, and agent surfaces are updated for durable workflow or AI-operable contract changes.
- Full `tools/validate.ps1` passes at the coherent phase gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 pointer sync: this plan was selected after `engine-ai-behavior-authoring-v1` completed package-visible behavior authoring counters, while `unsupportedProductionGaps = []` stayed empty.
- Phase 1 RED: adding batch raycast/sweep tests first failed because `PhysicsRaycastBatch2DDesc`, `PhysicsRaycastBatch3DDesc`, `PhysicsShapeSweepBatch2DDesc`, `PhysicsShapeSweepBatch3DDesc`, `PhysicsWorld2D::raycast_batch`, `PhysicsWorld3D::raycast_batch`, `PhysicsWorld2D::shape_sweep_batch`, and `PhysicsWorld3D::shape_sweep_batch` did not exist. A follow-up RED proved the initial default `max_queries = 64` contract rejected 65 `.queries` rows before the default was changed to unbounded.
- Phase 1 focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after docs, manifest fragments, composed manifest, skills, and static needles were synchronized. The review subagents found no blocking C++ issues and identified diagnostic/default-budget surface gaps that were fixed before the phase gate.
- Phase 1 full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on Windows with expected diagnostic-only host gates for unavailable Apple/Metal tooling; `production-readiness-audit` reported `unsupported_gaps=0`.
- Phase 2 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed after adding the static guard for package-visible `collision_query_batch_ready` evidence because `currentPhysics` did not yet expose the collision query batch package contract.
- Phase 2 focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_generated_desktop_runtime_3d_package`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime-release --target sample_generated_desktop_runtime_3d_package`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime-release --output-on-failure -R sample_generated_desktop_runtime_3d_package_smoke`, the installed `sample_generated_desktop_runtime_3d_package` package smoke with `--require-scene-collision-package`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed after docs, manifest fragments, composed manifest, game manifests, skills, and static needles were synchronized.
- Phase 2 PR static follow-up: hosted `Full Repository Static Analysis` failed on `performance-move-const-arg` for trivially copyable physics query batch rows and `modernize-loop-convert` for behavior authoring reverse traversal. The static root cause was fixed with value `push_back` rows and `std::views::reverse`; focused `tools/check-tidy.ps1 -Strict -Files engine/physics/src/physics2d.cpp,engine/physics/src/physics3d.cpp,engine/ai/src/behavior_tree.cpp`, `tools/check-format.ps1`, focused dev build, and focused `ctest` passed.
- Phase 2 full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on Windows with expected diagnostic-only or host-gated Apple/Metal checks; `production-readiness-audit` reported `unsupported_gaps=0`.
