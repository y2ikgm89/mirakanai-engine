# Gameplay Authoring Foundation v1 (2026-05-19)

**Plan ID:** `gameplay-authoring-foundation-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is a developer-owned capability milestone, not a reopened Engine 1.0 production gap.

## Goal

Add first-party gameplay authoring primitives so AI-generated games can safely connect authored scene nodes/components to gameplay systems and basic interactions without editing engine internals or depending on native handles.

## Context

- Engine 1.0 closeout remains zero-gap after `gameplay-physics-navigation-ai-foundation-v1`.
- The developer-owned backlog identifies `engine-scene-gameplay-binding-v1`, `engine-gameplay-interaction-framework-v1`, and optional `engine-gameplay-debug-overlay-v1` as generally useful follow-up capabilities.
- Current runtime scene APIs can instantiate cooked Scene v1 payloads and resolve animation bindings by node name, but gameplay systems still rely on game-local conventions to find scene nodes.

## Constraints

- Keep `unsupportedProductionGaps` empty. If this work requires reopening an Engine 1.0 production gap, stop.
- Keep public gameplay contracts value-oriented and backend-neutral. No SDL3, editor, Dear ImGui, RHI, D3D12, Vulkan, Metal, or middleware handles in gameplay-facing APIs.
- Use RED tests before behavior/API changes and close C++/runtime/public-contract phases with focused checks plus one fresh `tools/validate.ps1`.
- Update docs, manifest fragments, schemas/static checks, and skills when durable AI-operable behavior changes.

## Phase 0: Zero-Gap Pointer Sync

**Status:** Completed.

### Goal

Make the master-plan ledger, plan registry, and manifest pointers agree that Engine 1.0 has no remaining `unsupportedProductionGaps` while this developer-owned capability starts.

### Done When

- `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md` no longer names closed gameplay rows as active unsupported gaps.
- `docs/superpowers/plans/README.md` lists this plan as the active `currentActivePlan` slice.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Scene Gameplay Binding Contract

**Status:** Completed.

### Goal

Add a small `MK_runtime_scene` public contract that resolves explicit authored gameplay binding rows to scene node ids and required component presence.

### Planned API Shape

- `RuntimeSceneGameplayBindingSourceRow`: authored binding id, gameplay system id, slot id, node name, and required component kind.
- `RuntimeSceneGameplayBindingResolution`: resolved rows plus diagnostics and `succeeded()`.
- `resolve_runtime_scene_gameplay_bindings`: fail-closed resolver over a `RuntimeSceneInstance`.

### Done When

- RED tests prove valid bindings resolve deterministically in authored order.
- RED tests prove missing nodes, duplicate node names, duplicate binding ids, invalid row ids, and missing required components produce diagnostics and no partial binding rows.
- Focused `MK_runtime_scene_tests` passes.
- Public API boundary, agent/static drift checks, composed manifest, docs, and full validation are updated.

## Phase 2: Basic Interaction Framework

**Status:** Completed.

### Goal

Layer a value-type interaction plan over resolved gameplay bindings so games can express triggers, pickups, damage/heal, objectives, and win/loss/restart transitions without engine-specific game rules.

### Implemented API Shape

- `RuntimeSceneGameplayInteractionSourceRow`: authored action id, interaction kind, source/target binding ids, optional objective id, and amount.
- `RuntimeSceneGameplayInteractionPlanRequest`: caller-provided session state.
- `RuntimeSceneGameplayInteractionPlan`: deterministic resolved interaction rows, diagnostics, final session state, and `succeeded()`.
- `plan_runtime_scene_gameplay_interactions`: fail-closed planner over Phase 1 binding rows.

### Done When

- Interaction request/result rows are deterministic and testable without a renderer or platform backend.
- The API can consume Phase 1 binding rows and report missing targets, duplicate action ids, and rejected transitions.
- Sample 2D and 3D usage demonstrates at least two gameplay-family consumers.

## Phase 3: Debug Overlay Rows (Optional)

**Status:** Not selected.

### Goal

Expose runtime-visible/headless debug rows for gameplay bindings and interactions if Phase 1/2 diagnostics are insufficient for generated-game remediation.

### Done When

- Debug overlay rows can be generated without renderer/native handles.
- Package-visible counters are added only if they tighten AI remediation.

## Validation Evidence

- Phase 0 pointer sync:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
  - Manifest check after compose: `currentActivePlan = docs/superpowers/plans/2026-05-19-gameplay-authoring-foundation-v1.md`, `recommendedNextPlan = gameplay-authoring-foundation-v1`, `unsupportedProductionGaps = 0`.
- Phase 1 RED:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_tests`
  - Expected failure before implementation: `RuntimeSceneGameplayBindingSourceRow`, `RuntimeSceneGameplayBindingComponentKind`, `RuntimeSceneGameplayBindingResolution`, and `resolve_runtime_scene_gameplay_bindings` were undefined.
- Phase 1 focused green:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_tests`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_scene_tests`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- Phase 1 full closeout:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; `production-readiness-audit: unsupported_gaps=0`.
- Phase 2 RED:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_tests`
  - Expected failure before implementation: `RuntimeSceneGameplayInteractionSourceRow`, `RuntimeSceneGameplayInteractionPlanRequest`, `RuntimeSceneGameplayInteractionPlan`, and `plan_runtime_scene_gameplay_interactions` were undefined.
- Phase 2 focused green:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_tests`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_scene_tests`
- Phase 2 sample usage:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_playable_foundation`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_gameplay_foundation`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "sample_2d_playable_foundation|sample_gameplay_foundation"`
- Phase 2 agent/static and public contract:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- Phase 2 full closeout:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; `production-readiness-audit: unsupported_gaps=0`.
- Phase 3 selection:
  - Not selected. Phase 1/2 diagnostics and sample counters were sufficient for generated-game remediation at this milestone boundary.
