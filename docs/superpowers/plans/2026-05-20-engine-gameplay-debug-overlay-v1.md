# Engine Gameplay Debug Overlay v1 (2026-05-20)

**Plan ID:** `engine-gameplay-debug-overlay-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is a developer-owned capability milestone, not a reopened Engine 1.0 production gap.

## Goal

Add a thin first-party gameplay debug overlay primitive so generated games can expose reviewed runtime diagnostics for gameplay bindings, interactions, input context state, audio cues, physics/navigation/AI counters, packages, and objective/session state without renderer-native handles or editor dependencies.

## Context

- `gameplay-authoring-foundation-v1`, save/settings/profile, UI menu/HUD, input action contexts, audio gameplay mixer, and asset placeholder generation are completed developer-owned milestones.
- The developer-owned backlog lists `engine-gameplay-debug-overlay-v1` as a foundational unblocker for AI and developer runtime-visible diagnostics.
- Existing runtime UI/HUD intent, diagnostics counters, gameplay binding/interaction, input context, audio mix planning, physics/navigation/AI, and package status surfaces already provide data rows. This plan should add a reusable overlay model over those rows, not game-specific debug UI behavior.

## Constraints

- Keep the overlay host-independent and value-oriented. Do not depend on Dear ImGui, SDL3, editor code, renderer internals, native handles, or platform UI APIs.
- Keep game-specific labels, filtering, styling, and commands in game-owned code/data unless they become reusable row contracts.
- Reuse existing public runtime/UI/diagnostic rows where possible.
- Preserve `unsupportedProductionGaps = []`. If this work requires reopening an Engine 1.0 production gap, stop.
- Use RED tests before behavior/API changes.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Make the manifest pointers, master-plan ledger, and plan registry agree that `engine-gameplay-debug-overlay-v1` is the next active developer-owned capability after asset placeholder generation closeout.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active `currentActivePlan` milestone and records `engine-asset-placeholder-generation-v1` as completed.
- The production master plan and readiness ledger name this milestone as developer-owned capability work, not an Engine 1.0 unsupported gap.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Overlay Row Contract

**Status:** Completed.

### Goal

Add the smallest host-independent public API that can plan deterministic debug overlay rows from caller-owned gameplay/system status inputs.

### Done When

- RED tests describe overlay row planning, deterministic ordering, fail-closed diagnostics for invalid/duplicate row ids, and explicit non-goals.
- Focused runtime/UI build and tests pass.
- Docs, manifest fragments, skills, and static checks describe the overlay contract without claiming renderer/native UI execution.

## Phase 2: Sample Adoption Evidence

**Status:** Pending.

### Goal

Adopt the overlay row contract in one source-tree or package sample that already reports gameplay/input/audio/package counters.

### Done When

- At least one sample uses the overlay planner before reporting debug counters or HUD/debug rows.
- Focused sample build/test or package validation proves the adoption.
- Agent-surface drift is checked and updated if durable guidance changed.

## Validation Evidence

- Phase 0 pointer sync: plan registry, production master-plan index, readiness ledger, manifest fragments, and composed manifest point `currentActivePlan` / `recommendedNextPlan` at this plan while keeping `unsupportedProductionGaps = []`.
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` failed before implementation because `RuntimeGameplayDebugOverlayRowDesc`, `RuntimeGameplayDebugOverlayCategory`, `RuntimeGameplayDebugOverlayRowKind`, `RuntimeGameplayDebugOverlayDiagnosticCode`, `RuntimeGameplayDebugOverlayPlan`, and `plan_runtime_gameplay_debug_overlay` were undefined.
- Phase 1 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` passed after adding `RuntimeGameplayDebugOverlayRow`, `RuntimeGameplayDebugOverlayDiagnostic`, `RuntimeGameplayDebugOverlayPlan`, and fail-closed validation in `MK_ui`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` passed.
- Phase 1 surface gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-public-api-boundaries.ps1` passed after docs, manifest fragments, skills, and static needles were synchronized.
- Phase 1 full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `production-readiness-audit: unsupported_gaps=0` and 65/65 CTest tests passing; Metal/Apple host diagnostics remained host-gated on this Windows host.
