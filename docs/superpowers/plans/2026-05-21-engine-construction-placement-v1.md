# Engine Construction Placement v1 (2026-05-21)

**Plan ID:** `engine-construction-placement-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is developer-owned gameplay-family capability work, not a reopened Engine 1.0 production gap.

## Goal

Add first-party construction and placement primitives so AI-generated sandbox, survival, farming, tactics, and base-building games can preview, validate, and review placeable object intent from game-owned rules without engine-specific game logic, arbitrary scripting, native handles, or direct unreviewed scene mutation.

## Context

- `engine-inventory-items-crafting-v1` completed reusable item catalogs, placement/cost metadata, inventory-state validation, and value-only add/remove/craft transitions.
- `gameplay-authoring-foundation-v1` completed scene gameplay binding and interaction rows that can identify gameplay-relevant scene nodes without runtime reflection.
- Physics collision query, navmesh/crowd, debug overlay, input contexts, runtime menu/HUD intent, save/settings/profile, and placeholder asset generation are already implemented as developer-owned foundations.
- The developer-owned capability backlog lists `engine-construction-placement-v1` after inventory/items/crafting for building and sandbox gameplay.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep building pieces, economy balance, biome rules, recipe semantics, art direction, UI presentation, and persistence storage in game-owned code/data.
- Keep engine primitives value-only and deterministic: placement rule rows, preview/validation rows, cost rows, occupancy/collision/nav diagnostics, and reviewed scene-update intent rows.
- Do not add arbitrary scripting, editor building tools, runtime source asset parsing, renderer/RHI residency, native handles, networking, or package format changes in the first phase.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the next active developer-owned gameplay-family capability after `engine-inventory-items-crafting-v1` merged.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `engine-inventory-items-crafting-v1` as completed.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: Placement Rule Validation Contract

**Status:** Pending.

### Goal

Define the smallest reusable construction placement contract over item catalog placement metadata, candidate footprint rows, supported placement surfaces, optional cost rows, and deterministic diagnostics.

### Done When

- RED tests fail first for missing placement-rule validation behavior.
- Public value types validate supported placement ids, item references, finite grid/world positions, positive footprints, duplicate occupied cells, missing costs, and unsupported placement surfaces without scene mutation.
- Focused tests prove deterministic valid rows and fail-closed diagnostics.

## Phase 2: Reviewed Scene Placement Intent

**Status:** Pending.

### Goal

Add reviewed scene update intent rows so game-owned code can convert accepted placement results into explicit node/component creation requests without mutating scene state implicitly.

### Done When

- RED tests fail first for missing scene placement intent behavior.
- Runtime rows distinguish accepted, blocked, invalid, and already-occupied placement outcomes with deterministic diagnostics.
- Sample or package evidence demonstrates one public-API placement preview plus one reviewed placement intent path.
- Docs, manifest fragments, schemas/static checks, and agent surfaces are updated for durable AI-operable contract changes.
- Full `tools/validate.ps1` passes at the coherent phase gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 pointer sync selected this plan after `engine-inventory-items-crafting-v1` completed Runtime Inventory Items Crafting v1 through PR #140, merge commit `8812d00ee158450aa46fff2d5bc8909532b66a9d`, hosted PR Gate success, and full local validation while `unsupportedProductionGaps = []` stayed empty.
