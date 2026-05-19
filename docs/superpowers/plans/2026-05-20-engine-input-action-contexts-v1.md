# Engine Input Action Contexts v1 (2026-05-20)

**Plan ID:** `engine-input-action-contexts-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is a developer-owned capability milestone, not a reopened Engine 1.0 production gap.

## Goal

Strengthen reusable runtime input action/context primitives so AI-generated games can switch between gameplay, menu, dialogue, rebinding, and capture states without platform-native handles or game-specific engine logic.

## Context

- `gameplay-authoring-foundation-v1` completed runtime scene gameplay binding and interaction planning.
- `engine-save-settings-profile-v1` completed reviewed runtime profile path/document primitives.
- `engine-ui-game-menu-hud-v1` completed a runtime menu/HUD intent model that needs reliable action contexts for menu/gameplay handoff.
- The developer-owned backlog lists `engine-input-action-contexts-v1` as a foundational unblocker for keyboard/mouse/gamepad/touch mappings, context switching, UI capture, and rebinding.

## Constraints

- Keep game-specific mappings, labels, and rebinding choices in game-owned code/data.
- Promote only reusable input context/profile primitives into engine APIs.
- Keep platform adapters behind `engine/platform`; no native input handles in public runtime contracts.
- Preserve `unsupportedProductionGaps = []`. If this work requires reopening an Engine 1.0 production gap, stop.
- Use RED tests before behavior/API changes.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Make the manifest pointers, master-plan ledger, and plan registry agree that `engine-input-action-contexts-v1` is the active developer-owned capability after UI game menu/HUD foundation, while keeping `unsupportedProductionGaps = []`.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active `currentActivePlan` slice.
- The production master plan and readiness ledger name this milestone as developer-owned capability work, not an Engine 1.0 unsupported gap.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Runtime Input Context Review And First Contract

**Status:** Completed.

### Goal

Read the existing runtime input action/context/rebinding APIs and select the smallest missing primitive that improves menu/gameplay/capture context switching for generated games.

### Done When

- Existing `MK_runtime` input context APIs and tests are reviewed before implementation.
- RED runtime tests describe the selected missing behavior.
- Focused runtime build/test and relevant public/agent static checks pass.

### Result

- Selected missing primitive: Runtime Input Context Stack Planner v1.
- Added `RuntimeInputContextStackRequest`, `RuntimeInputContextLayerDesc`, `RuntimeInputContextLayerKind`, `RuntimeInputContextStackDiagnostic`, `RuntimeInputContextStackPlan`, and `plan_runtime_input_context_stack`.
- The planner validates gameplay/menu/dialogue/rebinding/capture/overlay context layers before producing a `RuntimeInputContextStack`, fails closed for invalid or duplicate contexts, supports default-context fallback, supports modal lower-priority blocking, and reports `gameplay_input_available`, `gameplay_input_consumed`, `ui_context_active`, and `capture_context_active`.

## Phase 2: Generated Game Adoption Evidence

**Status:** Completed.

### Goal

Apply the context stack planner to one or more generated-game or sample-game paths so menu/HUD/capture handoff is package- or source-tree-visible, not just a unit-tested helper.

### Done When

- At least one existing generated-game/sample flow uses `plan_runtime_input_context_stack` before evaluating `RuntimeInputActionMap`.
- Focused build/test/package or source-tree validation proves the adoption.
- Docs, manifest fragments, and static checks describe the adopted generated-game flow while keeping `unsupportedProductionGaps = []`.

### Result

- `sample_2d_playable_foundation` now binds gameplay movement/jump actions in a `gameplay` context and a passive HUD overlay action in a `hud` overlay context.
- Each frame plans a passive HUD overlay plus gameplay stack with `plan_runtime_input_context_stack` before evaluating `RuntimeInputActionMap`.
- The sample smoke reports `input_contexts=6` across the three-frame headless proof and fails if the planner diagnostics, gameplay availability, or overlay evidence regress.

## Validation Evidence

- Phase 0 pointer sync: plan registry, production master-plan index, readiness ledger, manifest fragments, and composed manifest now point `currentActivePlan` / `recommendedNextPlan` at this plan while keeping `unsupportedProductionGaps = []`.
- Phase 0 static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Phase 0 full gate inherited from the checkpoint: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; `production-readiness-audit` reported `unsupported_gaps=0`.
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests` failed before implementation because `RuntimeInputContextStackRequest`, `RuntimeInputContextLayerDesc`, `RuntimeInputContextLayerKind`, and `plan_runtime_input_context_stack` were not defined.
- Phase 1 focused runtime build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests` passed.
- Phase 1 focused runtime test: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_tests` passed.
- Phase 1 static/public checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `git diff --check` passed.
- Phase 1 full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; `production-readiness-audit` reported `unsupported_gaps=0`.
- Phase 2 RED/static guard: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before sample adoption because `games/sample_2d_playable_foundation/main.cpp` did not contain `plan_runtime_input_context_stack`.
- Phase 2 focused sample build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_playable_foundation` passed after replacing an invalid `Key::tab` use with the existing `Key::escape`.
- Phase 2 focused sample test: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R sample_2d_playable_foundation` passed.
- Phase 2 static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `git diff --check` passed.
- Phase 2 full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; `production-readiness-audit` reported `unsupported_gaps=0`; CTest reported 65/65 tests passed.
