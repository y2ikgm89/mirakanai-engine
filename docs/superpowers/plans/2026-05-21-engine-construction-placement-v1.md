# Engine Construction Placement v1 (2026-05-21)

**Plan ID:** `engine-construction-placement-v1`
**Status:** Completed.
**Current pointer rule:** Historical implementation evidence. Do not leave `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` pointing here after closeout. Keep `unsupportedProductionGaps = []`; this was developer-owned gameplay-family capability work, not a reopened Engine 1.0 production gap.

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

**Status:** Completed.

### Goal

Define the smallest reusable construction placement contract over item catalog placement metadata, candidate footprint rows, supported placement surfaces, optional cost rows, and deterministic diagnostics.

### Done When

- RED tests fail first for missing placement-rule validation behavior.
- Public value types validate supported placement ids, item references, finite grid/world positions, positive footprints, duplicate occupied cells, missing costs, and unsupported placement surfaces without scene mutation.
- Focused tests prove deterministic valid rows and fail-closed diagnostics.

## Phase 2: Reviewed Scene Placement Intent

**Status:** Completed.

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
- Phase 1 RED test evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_inventory_items_tests` failed before implementation because `RuntimeConstructionPlacementSurfaceDesc`, `RuntimeConstructionPlacementCandidateDesc`, `RuntimeConstructionPlacementValidationContext`, `RuntimeConstructionPlacementDiagnosticCode`, `RuntimeConstructionPlacementValidationRowKind`, and `validate_runtime_construction_placement` were missing.
- Phase 1 implementation adds `RuntimeConstructionPlacementSurfaceDesc`, `RuntimeConstructionPlacementCellDesc`, `RuntimeConstructionPlacementCandidateDesc`, `RuntimeConstructionPlacementValidationContext`, `RuntimeConstructionPlacementDiagnostic`, `RuntimeConstructionPlacementValidationRow`, `RuntimeConstructionPlacementValidationResult`, and `validate_runtime_construction_placement` in `MK_runtime`. The validator returns deterministic candidate and occupied-cell rows only when all candidates pass, and otherwise fails closed for invalid catalogs, missing item references, non-placeable items, unsupported placement ids, missing/unsupported surfaces, non-finite grid/world positions, invalid footprints, duplicate occupied cells, and missing placement costs without scene mutation.
- Phase 1 focused GREEN evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_inventory_items_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_inventory_items_tests"` passed.
- Phase 1 agent-surface/static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/inventory_items.cpp,tests/unit/runtime_inventory_items_tests.cpp`, and `git diff --check` passed.
- Phase 1 full slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `production-readiness-audit: unsupported_gaps=0`, all 67 tests passing, and only diagnostic/host-gated Apple/Metal blockers on this Windows host.
- Phase 2 RED test evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_tests` failed before implementation because `RuntimeSceneConstructionPlacementIntentDesc`, `RuntimeSceneConstructionPlacementIntentContext`, `RuntimeSceneConstructionPlacementIntentPlan`, and `plan_runtime_scene_construction_placement_intents` were not present in `MK_runtime_scene`.
- Phase 2 implementation adds reviewed runtime-scene placement intent rows in `MK_runtime_scene`: `RuntimeSceneConstructionPlacementIntentDesc`, `RuntimeSceneConstructionPlacementIntentContext`, `RuntimeSceneConstructionPlacementOccupiedCell`, `RuntimeSceneConstructionPlacementIntentRow`, `RuntimeSceneConstructionPlacementIntentDiagnostic`, `RuntimeSceneConstructionPlacementIntentPlan`, `RuntimeSceneConstructionPlacementIntentStatus`, `RuntimeSceneConstructionPlacementIntentDiagnosticCode`, and `plan_runtime_scene_construction_placement_intents`. The planner consumes successful `validate_runtime_construction_placement` rows, requires explicit review, validates node names, components, finite/non-zero-scale transforms, and validation-world-position matches, rejects duplicate intent/existing scene names, reports existing-context and same-batch already-occupied cells, and classifies rows as accepted, blocked, invalid, or already_occupied without mutating scene state. `RuntimeConstructionPlacementValidationRow` now preserves candidate grid/world origin fields so reviewed scene-intent transforms can be checked against placement validation evidence.
- Phase 2 review hardening RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_tests` failed after adding tests for invalid/non-matching transforms and same-batch occupied-cell conflicts because `RuntimeSceneConstructionPlacementIntentDiagnosticCode::invalid_transform` and `mismatched_transform_position` were not implemented.
- Phase 2 sample/package evidence: `sample_2d_desktop_runtime_package --require-gameplay-systems` now reports `gameplay_systems_construction_placement_ready=1`, `gameplay_systems_construction_placement_diagnostics=0`, `gameplay_systems_construction_placement_validation_rows=3`, `gameplay_systems_construction_placement_intent_rows=1`, `gameplay_systems_construction_placement_intent_accepted_rows=1`, and `gameplay_systems_construction_placement_intent_occupied_cells=2`.
- Phase 2 focused GREEN evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_inventory_items_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_scene_tests|MK_runtime_inventory_items_tests|sample_2d_desktop_runtime_package_smoke"`, and a direct `sample_2d_desktop_runtime_package.exe --smoke --require-gameplay-systems` smoke passed.
- Phase 2 agent-surface/static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/inventory_items.cpp,engine/runtime_scene/src/runtime_scene.cpp,tests/unit/runtime_inventory_items_tests.cpp,tests/unit/runtime_scene_tests.cpp,games/sample_2d_desktop_runtime_package/main.cpp`, and `git diff --check` passed.
- Phase 2 installed package evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -SmokeArgs @('--smoke','--require-config','runtime/sample_2d_desktop_runtime_package.config','--require-scene-package','runtime/sample_2d_desktop_runtime_package.geindex','--require-gameplay-systems') }"` passed, including installed SDK validation, installed desktop runtime validation, CPack, and construction placement counters.
- Phase 2 full slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `production-readiness-audit: unsupported_gaps=0`; host-gated Apple/Metal diagnostics remain non-blocking on this Windows host.
- Hosted closeout: PR #145 merged into `main` with merge commit `8ab437c694709c0f193246255c3d553a8188476c`; hosted PR Gate, Windows MSVC, Full Repository Static Analysis shards, Linux, CodeQL, iOS, and macOS Metal CMake checks passed. The next active developer-owned capability is `renderer-modern-materials-v1`.
