# Engine Inventory Items Crafting v1 (2026-05-20)

**Plan ID:** `engine-inventory-items-crafting-v1`
**Status:** Completed.
**Current pointer rule:** Do not keep `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` pointed at this completed plan. Keep `unsupportedProductionGaps = []`; this was developer-owned gameplay-family capability work, not a reopened Engine 1.0 production gap.

## Goal

Add first-party item, inventory, and crafting primitives so AI-generated item-driven, survival, crafting, economy, progression, and sandbox games can validate reusable item catalogs, advance deterministic inventory state, and apply recipe transitions from game-owned rules without arbitrary scripting or engine-specific game logic.

## Context

- `engine-quest-dialogue-state-v1` completed deterministic quest/dialogue documents, save-state validation, transition rows, returned action/reward ids, and package-visible 2D gameplay-system counters.
- The developer-owned capability backlog lists `engine-inventory-items-crafting-v1` as the next gameplay-family enabler after quest/dialogue state for item definitions, inventory rules, recipes, and placement/cost validation.
- Generated games already have save/profile, input, UI/HUD intent, audio mix planning, placeholder assets, debug overlay rows, sprite authoring/rendering/animation, behavior authoring, collision query, navmesh/crowd, advanced physics controller, and quest/dialogue state foundations.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep item art, economy balance, reward execution, shop logic, loot tables, equipment semantics, and game-specific progression rules in game-owned data/code.
- Keep the engine surface value-only and deterministic: catalog validation rows, inventory-state rows, recipe transition rows, and explicit diagnostics only.
- Do not add scripting, networking, marketplace/economy services, editor item tooling, persistence mutation, renderer assets, or package format changes in this milestone.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the next active developer-owned gameplay-family capability after `engine-quest-dialogue-state-v1` closeout.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `engine-quest-dialogue-state-v1` as completed.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- The 1.0 readiness ledger records this plan as future gameplay capability work, not an Engine 1.0 unsupported gap.

## Phase 1: Item Catalog Contract

**Status:** Completed.

### Goal

Define the smallest reusable item/catalog contract for item ids, stack limits, categories/tags, optional placement/cost metadata, and deterministic validation diagnostics.

### Done When

- RED tests fail first for missing item catalog validation behavior.
- Public value types validate duplicate ids, invalid stack limits, missing references, unsupported tags/categories, and unsafe localization keys without game-specific execution.
- Focused tests prove deterministic diagnostic ordering and fail-closed invalid catalogs.

## Phase 2: Inventory And Crafting Transition Contract

**Status:** Completed.

### Goal

Add value-only inventory and recipe transition helpers over validated catalogs so game-owned systems can add/remove items, clamp stacks, consume recipe costs, produce outputs, and report blocked/invalid transitions deterministically.

### Done When

- RED tests fail first for missing inventory and crafting transition behavior.
- Runtime rows distinguish accepted, ignored, blocked, completed, and invalid transitions with deterministic diagnostics.
- Sample or package evidence demonstrates item pickup, inventory state validation, and one recipe path through public APIs only.
- Docs, manifest fragments, schemas/static checks, and agent surfaces are updated for durable AI-operable contract changes.
- Full `tools/validate.ps1` passes at the coherent phase gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 pointer sync selected this plan after `engine-quest-dialogue-state-v1` completed deterministic quest/dialogue state validation, transition rows, action/reward ids, package counters, hosted PR #137, and merge commit `cad53e3cc7e6221769b4b15ac56793ac6bc490d5`, while `unsupportedProductionGaps = []` stayed empty.
- Phase 1 adds Runtime Item Catalog v1 in `MK_runtime` through `RuntimeItemCatalogDocument`, `RuntimeItemDesc`, `RuntimeItemCostDesc`, `RuntimeItemCatalogValidationContext`, `RuntimeItemCatalogValidationResult`, `RuntimeItemCatalogDiagnostic`, `RuntimeItemCatalogValidationRow`, and `validate_runtime_item_catalog_document`.
- Phase 1 RED test evidence: `MK_runtime_inventory_items_tests` failed before implementation because `mirakana/runtime/inventory_items.hpp` was missing.
- Phase 1 focused GREEN evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_inventory_items_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_inventory_items_tests"` passed for deterministic valid rows and fail-closed diagnostics.
- Phase 2 adds value-only inventory/crafting transition contracts through `RuntimeInventoryState`, `RuntimeInventoryStackDesc`, `RuntimeInventoryStateValidationResult`, `validate_runtime_inventory_state`, `RuntimeCraftingRecipeDocument`, `RuntimeCraftingRecipeDesc`, `RuntimeInventoryTransitionRequest`, `RuntimeInventoryTransitionResult`, `RuntimeInventoryTransitionStatus`, and `advance_runtime_inventory_state`.
- Phase 2 RED test evidence: `MK_runtime_inventory_items_tests` failed before implementation because the inventory state, crafting recipe, and transition request/result symbols were missing.
- Phase 2 focused GREEN evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_inventory_items_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_inventory_items_tests"`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "sample_2d_desktop_runtime_package_smoke"` passed.
- Phase 2 package evidence: `sample_2d_desktop_runtime_package --smoke --require-gameplay-systems` emitted `gameplay_systems_inventory_items_ready=1`, `gameplay_systems_inventory_items_diagnostics=0`, `gameplay_systems_inventory_items_catalog_rows=2`, `gameplay_systems_inventory_items_state_rows=2`, `gameplay_systems_inventory_items_transition_rows=2`, `gameplay_systems_inventory_items_accepted_rows=1`, `gameplay_systems_inventory_items_completed_rows=1`, and `gameplay_systems_inventory_items_final_workbench_quantity=1`.
- Phase 2 agent-surface/static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/inventory_items.cpp,tests/unit/runtime_inventory_items_tests.cpp,games/sample_2d_desktop_runtime_package/main.cpp`, and `git diff --check` passed.
- Phase 2 coherent gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `unsupported_gaps=0`, all 67 tests passing, and only existing host-gated/diagnostic-only Apple/Metal/mobile notes.
