# Sprite Atlas Authoring v1 (2026-05-20)

**Plan ID:** `sprite-atlas-authoring-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is developer-owned 2D capability work, not a reopened Engine 1.0 production gap.

## Goal

Add the first source-authoring primitive for generated-game sprite atlases so AI and developer workflows can describe reviewed sprite frames, pack them through existing deterministic atlas policy, and produce package-ready source/cook rows without inventing game-specific asset pipelines.

## Context

- The foundational unblockers are complete: gameplay binding/interactions, save/settings/profile, runtime menu/HUD intent, input contexts, gameplay audio mix planning, asset placeholder generation, and gameplay debug overlay rows.
- Existing `MK_assets` already exposes deterministic RGBA8 sprite atlas packing, and existing package/UI atlas work proves adjacent metadata/cooked-package patterns.
- The next backlog priority is 2D strengthening. `sprite-atlas-authoring-v1` should bridge generated-game source sprite rows to reviewed atlas metadata and cook/package planning before renderer batching or flipbook animation work expands the runtime side.

## Constraints

- Keep the work host-independent and deterministic. Do not require native GPU handles, renderer residency, runtime source image decoding, package streaming, editor-only UI, or broad production atlas tooling.
- Reuse existing first-party image decode, atlas packing, source asset registry, and registered source cook/package contracts where they fit.
- Game-specific art direction, frame naming policy, animation semantics, and replacement workflows stay in game-owned code/data unless they become reusable engine contracts.
- Preserve `unsupportedProductionGaps = []`. If the work needs a reopened Engine 1.0 blocker, stop.
- Use RED tests or static guards before behavior/API changes.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Make the manifest pointers, master-plan ledger, and plan registry select `sprite-atlas-authoring-v1` as the next active developer-owned capability after debug overlay closeout.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active `currentActivePlan` milestone and records `engine-gameplay-debug-overlay-v1` as completed.
- The production master plan and readiness ledger name this milestone as developer-owned 2D capability work, not an Engine 1.0 unsupported gap.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Source Atlas Authoring Contract

**Status:** Pending.

### Goal

Add a narrow public contract that validates generated sprite atlas source rows and produces deterministic atlas metadata/provenance rows ready for the existing cook/package path.

### Done When

- RED tests describe valid sprite atlas source rows, duplicate/invalid frame ids, unsupported dimensions or formats, deterministic packing order, and fail-closed diagnostics.
- Focused `MK_assets` / `MK_tools` build and tests pass.
- Docs, manifest fragments, and static checks describe the source atlas authoring contract without claiming runtime image decoding, renderer residency, production batching, or animation semantics.

## Phase 2: Package Adoption Evidence

**Status:** Pending.

### Goal

Adopt the sprite atlas authoring contract in a generated 2D source/package path that already consumes cooked 2D assets.

### Done When

- At least one sample or scaffold uses the atlas authoring planner before package registration or runtime consumption.
- Focused sample/package validation proves the adoption with deterministic atlas metadata counters.
- Agent-surface drift is checked and updated if durable guidance changed.

## Validation Evidence

- Phase 0 pointer sync: plan registry, production master-plan index, readiness ledger, manifest fragments, and composed manifest point `currentActivePlan` / `recommendedNextPlan` at this plan while keeping `unsupportedProductionGaps = []`.
- Phase 0 surface/full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, and `tools/validate.ps1` passed while selecting this plan; `production-readiness-audit` reported `unsupported_gaps=0` and 65/65 CTest tests passed.
