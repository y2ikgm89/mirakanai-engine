# Sprite Animation Flipbook v1 (2026-05-20)

**Plan ID:** `sprite-animation-flipbook-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is developer-owned 2D capability work, not a reopened Engine 1.0 production gap.

## Goal

Add a reusable first-party 2D flipbook animation primitive so AI-generated games can advance named sprite-frame clips over cooked atlas/package data, bind the sampled frame to scene sprite rows, and report deterministic package-visible counters without adding game-specific animation engines.

## Context

- `sprite-atlas-authoring-v1` provides reviewed source atlas rows and source registry handoff before cooked runtime consumption.
- `sprite-batching-renderer-v1` provides atlas-backed repeated scene sprite plan counters and package smoke evidence.
- The older `2d-sprite-animation-package-v1` slice proves cooked `sprite_animation` payload loading and frame application, but generated games still need a clearer reusable flipbook state/update contract for common idle/run/impact animation families.

## Constraints

- Keep game-specific animation graphs, state machines, combat logic, and art direction in game-owned code/data.
- Do not parse source images at runtime, generate renderer/RHI residency from source files, expose native/RHI handles, sort sprites, or claim broad production sprite animation tooling.
- Preserve deterministic sampling and fail-closed diagnostics for invalid clips, durations, target rows, and atlas-frame references.
- Build on existing cooked `GameEngine.CookedSpriteAnimation.v1`, scene renderer sprite rows, and generated 2D package smokes.
- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the next active developer-owned 2D capability after `sprite-batching-renderer-v1` closeout.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `sprite-batching-renderer-v1` as completed.
- The production master plan and readiness ledger name this milestone as developer-owned 2D capability work, not an Engine 1.0 unsupported gap.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Flipbook State Contract

**Status:** Completed.

### Goal

Define the smallest reusable flipbook state/update API over cooked sprite-frame rows, including deterministic looping/clamping, named clip rows, selected-frame evidence, and fail-closed diagnostics.

### Done When

- RED tests fail first for missing flipbook state/update behavior.
- Public engine APIs expose value-only flipbook descriptors/results without renderer, platform, editor, or native handles.
- Focused unit tests prove deterministic frame selection, invalid duration/frame diagnostics, and clean-break failure behavior.

## Phase 2: Package Evidence

**Status:** Pending.

### Goal

Adopt the flipbook contract in the generated 2D package path so package smokes can verify deterministic clip ticks and sprite-frame application counters over cooked atlas data.

### Done When

- `DesktopRuntime2DPackage` or a committed 2D sample reports package-visible flipbook counters through existing validation recipes.
- D3D12/Vulkan host gates remain explicit and only claimed when the relevant local or hosted lane proves them.
- Docs, manifest fragments, schemas/static checks, and agent surfaces are updated for any durable workflow or AI-operable contract change.

## Validation Evidence

- Phase 0 pointer sync: plan registry, readiness ledger, production master-plan index, manifest fragments, and composed manifest select this plan as the next active developer-owned capability after `sprite-batching-renderer-v1` while keeping `unsupportedProductionGaps = []`.
- Phase 0 static gate: after manifest compose, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed locally on 2026-05-20.
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_tests` failed first because `RuntimeSpriteFlipbookClipDesc`, `RuntimeSpriteFlipbookState`, `RuntimeSpriteFlipbookDesc`, and `advance_runtime_sprite_flipbook` did not exist.
- Phase 1 GREEN: `MK_scene_renderer` now exposes value-only `RuntimeSpriteFlipbookClipDesc`, `RuntimeSpriteFlipbookDesc`, `RuntimeSpriteFlipbookState`, `RuntimeSpriteFlipbookSampleResult`, and `advance_runtime_sprite_flipbook`; focused build and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_scene_renderer_tests` passed.
