# Engine Navmesh Crowd v1 (2026-05-20)

**Plan ID:** `engine-navmesh-crowd-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is developer-owned navigation/gameplay capability work, not a reopened Engine 1.0 production gap.

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

**Status:** Pending.

### Goal

Define the smallest reusable crowd/navigation contract over existing route, agent, dynamic-obstacle, and local-avoidance primitives, with deterministic ordering, explicit diagnostics, and no middleware/native-handle exposure.

### Done When

- RED tests or static guards fail first for missing crowd batch planning behavior.
- `MK_navigation` exposes value-only request/result/diagnostic rows for generated-game crowd movement authoring.
- Focused tests prove deterministic agent ordering, duplicate/invalid agent diagnostics, dynamic-obstacle interaction, local-avoidance aggregation, max-agent budget rejection, and stable replay counters.

## Phase 2: Package Crowd Evidence

**Status:** Pending.

### Goal

Adopt the crowd/navigation contract in selected generated package paths so package smokes can report deterministic navmesh/crowd counters.

### Done When

- A committed sample or generated package path reports package-visible navmesh/crowd counters through existing validation recipes.
- Docs, manifest fragments, schemas/static checks, and agent surfaces are updated for durable workflow or AI-operable contract changes.
- Full `tools/validate.ps1` passes at the coherent phase gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 pointer sync: this plan was selected after `physics-collision-query-v1` completed deterministic collision query batches and package-visible `collision_query_batch_*` counters, while `unsupportedProductionGaps = []` stayed empty.
