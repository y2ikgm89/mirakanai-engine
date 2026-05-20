# Engine Navmesh Crowd v1 (2026-05-20)

**Plan ID:** `engine-navmesh-crowd-v1`
**Status:** Completed.
**Current pointer rule:** This milestone is complete. Keep `unsupportedProductionGaps = []`; this was developer-owned navigation/gameplay capability work, not a reopened Engine 1.0 production gap.

## Goal

Extend first-party navigation primitives so generated games can plan deterministic navmesh/crowd movement with validated scene-ref polygon routes, explicit crowd/local-avoidance rows, replayable diagnostics, and selected package-visible counters without exposing middleware or native navigation handles.

## Context

- `physics-collision-query-v1` completed deterministic collision query batches and selected generated 3D package counters.
- Existing `MK_navigation` work already covers grid A*, path following, agent movement ticks, grid dynamic-obstacle replanning, grid smoothing, single-agent local avoidance, generated-game route setup, and scene-ref navmesh path planning with dynamic-obstacle blocking.
- The developer-owned capability backlog lists `engine-navmesh-crowd-v1` as the next gameplay-family enabler for navmesh/crowd tests, dynamic obstacle diagnostics, package evidence, and middleware-free public API contracts.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep navmesh asset import, editor visualization parity, background navigation jobs, RVO/ORCA middleware, and public native handles out of scope unless a later dated optional-adapter plan accepts them.
- Keep game-specific squad tactics, formations, and encounter logic in game-owned code/data.
- Reuse existing `MK_navigation` route, agent, and local-avoidance primitives where possible; use clean-break changes if existing contracts block deterministic crowd rows.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the next active developer-owned navigation/gameplay capability after `physics-collision-query-v1` closeout.

### Done When

- `docs/superpowers/plans/README.md`, the readiness ledger, and the production master-plan index list this plan as the active developer-owned milestone.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Deterministic Crowd Query Contract

**Status:** Completed.

### Goal

Define the smallest reusable crowd/navigation contract over existing route, agent, dynamic-obstacle, and local-avoidance primitives, with deterministic ordering, explicit diagnostics, and no middleware/native-handle exposure.

### Done When

- RED tests or static guards fail first for missing crowd batch planning behavior.
- `MK_navigation` exposes value-only request/result/diagnostic rows for generated-game crowd movement authoring.
- Focused tests prove deterministic agent ordering, duplicate/invalid agent diagnostics, dynamic-obstacle interaction, local-avoidance aggregation, max-agent budget rejection, and stable replay counters.

## Phase 2: Package Crowd Evidence

**Status:** Completed.

### Goal

Adopt the crowd/navigation contract in selected generated package paths so package smokes can report deterministic navmesh/crowd counters.

### Done When

- A committed sample or generated package path reports package-visible navmesh/crowd counters through existing validation recipes.
- Docs, manifest fragments, schemas/static checks, and agent surfaces are updated for durable workflow or AI-operable contract changes.
- Full `tools/validate.ps1` passes at the coherent phase gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 pointer sync: this plan was selected after `physics-collision-query-v1` completed deterministic collision query batches and package-visible `collision_query_batch_*` counters, while `unsupportedProductionGaps = []` stayed empty.
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_navigation_tests` failed on missing `mirakana/navigation/navigation_crowd.hpp` after adding crowd query tests first.
- Phase 1 focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_navigation_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_navigation_tests` passed after adding `NavigationCrowdPlanRequest` / `plan_navigation_navmesh_crowd` value rows, deterministic ordering, dynamic-obstacle route propagation, local-avoidance aggregation, duplicate/invalid-agent diagnostics, max-agent budget rejection, and replay counter tests.
- Phase 1 static/full validation: `tools/check-agents.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-format.ps1` passed; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `unsupported_gaps=0` and only existing diagnostic-only Apple/Metal host gates.
- Phase 2 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-installed-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` failed first after the installed generated 3D package validation required package-visible `gameplay_systems_navigation_crowd_*` fields.
- Phase 2 focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_generated_desktop_runtime_3d_package` passed, and the direct source-tree smoke reported `gameplay_systems_navigation_crowd_status=success`, `gameplay_systems_navigation_crowd_source_order_ready=1`, `gameplay_systems_navigation_crowd_agents=2`, `gameplay_systems_navigation_crowd_route_successes=2`, `gameplay_systems_navigation_crowd_avoidance_successes=2`, `gameplay_systems_navigation_crowd_applied_neighbors=2`, and `gameplay_systems_navigation_crowd_dynamic_obstacles=2`.
- Phase 2 package evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` passed; installed validation reported `installed-desktop-runtime-validation: ok (sample_generated_desktop_runtime_3d_package)` with the same package-visible navmesh/crowd counters.
- Phase 2 static/agent evidence: `tools/check-tidy.ps1 -Files games/sample_generated_desktop_runtime_3d_package/main.cpp`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, `tools/check-agents.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-ai-integration.ps1` passed after updating docs, manifest fragments, generated manifest, scaffolding/static guards, and plan pointers.
- Phase 2 full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `unsupported_gaps=0`, all 65 dev tests passing, and only expected diagnostic-only Apple/Metal host gates.
