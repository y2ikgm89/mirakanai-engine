# Sprite Batching Renderer v1 (2026-05-20)

**Plan ID:** `sprite-batching-renderer-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is developer-owned 2D capability work, not a reopened Engine 1.0 production gap.

## Goal

Strengthen the reusable renderer-facing 2D sprite batching primitive so generated games can safely submit repeated atlas-backed sprite rows through public engine contracts and get deterministic planning/execution evidence without exposing native/RHI handles or inventing game-specific renderer code.

## Context

- `sprite-atlas-authoring-v1` is complete and gives generated 2D workflows reviewed source atlas rows plus source registry handoff before runtime consumption.
- Historical 2D batching slices already provide `plan_sprite_batches`, scene packet telemetry, and a narrow native RHI execution proof for adjacent compatible `SpriteCommand` runs.
- The backlog still calls out `sprite-batching-renderer-v1` as a developer-owned 2D strengthening milestone because AI-generated games need a clearer renderer contract over atlas-backed repeated sprite submission, package-visible counters, and backend gates before flipbook animation or broader 2D renderer quality work expands.

## Constraints

- Preserve sprite draw order and alpha-composition semantics; do not sort by material/texture/atlas unless a validated policy row explicitly allows it.
- Keep public gameplay APIs free of `IRhiDevice`, backend handles, OS handles, D3D12/Vulkan/Metal objects, and renderer internals.
- Do not claim runtime source image decoding, package streaming execution, Metal readiness, broad renderer quality, tilemap production editing, animation semantics, or broad generated 2D production readiness.
- Build on existing `SpriteCommand`, atlas metadata, scene-renderer packet, and package smoke surfaces instead of adding game-specific renderer paths.
- Preserve `unsupportedProductionGaps = []`. If the work needs a reopened Engine 1.0 blocker, stop.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the next active developer-owned 2D capability after `sprite-atlas-authoring-v1` closeout.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `sprite-atlas-authoring-v1` as completed.
- The production master plan and readiness ledger name this milestone as developer-owned 2D capability work, not an Engine 1.0 unsupported gap.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Atlas-Backed Batch Contract

**Status:** Completed.

### Goal

Define the narrow reusable contract for atlas-backed repeated sprite submission and deterministic batch evidence across renderer/package surfaces.

### Done When

- RED coverage or static guards fail first for missing atlas-backed batch policy/evidence rows.
- Renderer-facing API or diagnostics distinguish valid adjacent compatible atlas-backed batches from unsupported ordering, material, or texture-policy requests.
- Focused renderer/scene-renderer tests prove order preservation, fail-closed diagnostics, and counter stability.

## Phase 2: Package Evidence

**Status:** Pending.

### Goal

Adopt the contract in a generated 2D package path so package smokes can verify deterministic sprite batch counters without native handle exposure.

### Done When

- `DesktopRuntime2DPackage` or a committed 2D sample reports package-visible atlas-backed batch counters through existing validation recipes.
- D3D12/Vulkan host gates are explicit and only claimed when the relevant local or hosted lane proves them.
- Docs, manifest fragments, schemas/static checks, and agent surfaces are updated for any durable workflow or AI-operable contract change.

## Validation Evidence

- Phase 0 pointer sync: plan registry, readiness ledger, production master-plan index, manifest fragments, and composed manifest select this plan as the next active developer-owned capability while keeping `unsupportedProductionGaps = []`.
- Phase 0 static/full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, and `tools/validate.ps1` passed after selecting this plan; `production-readiness-audit` reported `unsupported_gaps=0` and 65/65 CTest tests passed.
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` failed as expected while tests referenced missing `SpriteBatchPlanDesc`, `SpriteBatchPlanOptions`, `atlas_backed_batch_count`, `repeated_atlas_batch_count`, `repeated_atlas_sprite_count`, `unsupported_reordering_policy`, and `untextured_sprite_disallowed`.
- Phase 1 implementation: `MK_renderer` now exposes `SpriteBatchPlanDesc` and `SpriteBatchPlanOptions` so atlas-backed repeated sprite batch planning can remain order-preserving by default, fail closed on unsupported reorder requests, optionally reject untextured rows when an atlas-backed-only policy is requested, and report `atlas_backed_batch_count`, `repeated_atlas_batch_count`, and `repeated_atlas_sprite_count` without exposing native/RHI handles.
- Phase 1 focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` passed for the new sprite batching renderer tests.
- Phase 1 surface/full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/validate.ps1` passed; `production-readiness-audit` reported `unsupported_gaps=0` and 65/65 CTest tests passed.
