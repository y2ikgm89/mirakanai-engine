# Engine Entity Scale and Culling v1 (2026-05-21)

**Plan ID:** `engine-entity-scale-and-culling-v1`
**Status:** Active.
**Current pointer rule:** `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points at this milestone while active. Keep `unsupportedProductionGaps = []`; this is a developer-owned scale-enabler capability, not a reopened Engine 1.0 production gap.

## Goal

Add first-party entity scale and culling primitives so generated games with many objects can describe stable entity rows, derive deterministic visibility/update/draw budget intent, and surface package-visible counters without exposing renderer internals, native handles, broad renderer-quality claims, or game-specific world rules.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- `engine-world-region-streaming-v1` completed deterministic world-region catalogs, safe-point region load/unload planning, reviewed package bridge evidence, selected package counters, and full validation while keeping `unsupportedProductionGaps = []`.
- The developer-owned capability backlog lists `engine-entity-scale-and-culling-v1` as the next `scale-enabler` for high-object-count games needing entity queries, culling, LOD, instancing, and update/draw budgets.
- Existing foundations include scene/runtime stable ids, runtime scene instance rows, package-visible 2D/3D counters, renderer scene-scale diagnostics, sprite batching package evidence, world-region package evidence, and desktop runtime package validation.
- This plan starts with host-independent value contracts and selected package evidence. Renderer-owned GPU culling, native occlusion queries, broad instancing execution, performance claims, ECS architecture replacement, streaming background workers, and platform-native handles stay future work unless a later phase validates them explicitly.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If entity scale/culling must become an Engine 1.0 blocker to proceed, stop.
- Keep public contracts deterministic, backend-neutral, and first-party. Do not introduce middleware, native handles, renderer-private types, broad ECS replacement, file watching, background workers, or hidden renderer/RHI ownership.
- Keep game-specific spawn rules, AI update policy, encounter pacing, world partition decisions, and save-game semantics in game-owned code/data unless a reusable multi-family primitive is proven.
- Reuse existing runtime scene, renderer scene-scale, sprite batching, package validation, and generated-game guidance surfaces where possible.
- Start behavior/API/regression-risk changes with a RED test or static guard.
- Do not claim broad high-object-count production readiness, GPU-driven culling, broad instancing performance, renderer quality, or world streaming readiness from value-only culling rows.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the active developer-owned scale-enabler capability after `engine-world-region-streaming-v1` closeout.

### Done When

- `docs/superpowers/plans/README.md`, the readiness ledger, the master-plan index, and `docs/roadmap.md` list this plan as the active milestone.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: Entity Query and Culling Plan Contract

**Status:** Completed.

### Goal

Add the smallest deterministic value contract for entity rows, visibility volumes, camera/view rows, culling decisions, and diagnostics.

### Done When

- RED tests fail first for missing entity scale/culling plan contracts.
- The selected engine module exposes backend-neutral value rows for entity ids, bounds, layers/masks, view frusta or orthographic regions, visibility decisions, update buckets, and diagnostics.
- Focused tests prove deterministic ordering, duplicate/missing entity diagnostics, layer/mask filtering, 2D/3D bounds handling, budget rejection, and no renderer/RHI/native-handle mutation.

## Phase 2: LOD and Budget Intent Rows

**Status:** Completed.

### Goal

Extend the host-independent contract with LOD, update frequency, draw intent, and budget rows that generated games can use before renderer-specific adoption.

### Done When

- RED tests fail first for missing LOD/update/draw budget behavior.
- Plan rows report stable LOD selection, update groups, culled/visible counts, projected draw/update costs, and budget diagnostics.
- Tests cover deterministic tie-breaking, invalid LOD thresholds, disabled/protected entities, over-budget diagnostics, and explicit non-goals for GPU-driven culling or renderer-owned residency.

## Phase 3: Selected Package Evidence and Agent Surface Closeout

**Status:** Pending.

### Goal

Expose selected entity scale/culling counters in an existing desktop runtime package lane and close the AI-operable contract surfaces for the supported narrow claim.

### Done When

- Selected package smokes report deterministic entity scale/culling counters, diagnostics, visible/culled/update/LOD budget rows, and package-visible adoption fields.
- Docs, manifest fragments, schemas/static checks, skills/rules/subagents, and generated-game guidance are checked for drift and updated only where durable behavior or workflow changed.
- Full `tools/validate.ps1` passes at the coherent runtime/public-contract gate, with `unsupportedProductionGaps = []`.

## Validation Evidence

- Phase 0 starts after `engine-world-region-streaming-v1` completed through `docs/superpowers/plans/2026-05-21-engine-world-region-streaming-v1.md` by adding deterministic `MK_runtime` world-region catalog/plan rows, reviewed package safe-point load/unload bridge evidence, selected `sample_2d_desktop_runtime_package --require-world-region-streaming` counters, package validation, and full `tools/validate.ps1` evidence while `unsupportedProductionGaps = []` stayed empty.
- Phase 0 pointer sync selected this plan in `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, `docs/roadmap.md`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`; composed `engine/agent/manifest.json` reports `currentActivePlan=docs/superpowers/plans/2026-05-21-engine-entity-scale-and-culling-v1.md`, `recommendedNextPlan.id=engine-entity-scale-and-culling-v1`, and `unsupportedProductionGaps = []`.
- Phase 0 static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed with `unsupported_gaps=0`.
- Phase 1 RED evidence: adding `tests/unit/runtime_entity_scale_culling_tests.cpp` and `MK_runtime_entity_scale_culling_tests` first failed at `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_entity_scale_culling_tests` because `mirakana/runtime/entity_scale_culling.hpp` did not exist.
- Phase 1 implementation adds `engine/runtime/include/mirakana/runtime/entity_scale_culling.hpp` and `engine/runtime/src/entity_scale_culling.cpp` to `MK_runtime`, exposing `plan_runtime_entity_scale_culling` value rows for stable entity ids, 2D/3D AABB bounds, layer masks, view bounds, visibility decisions, update buckets, projected counters, and fail-closed diagnostics without renderer/RHI/native-handle or scene mutation.
- Phase 1 focused validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_entity_scale_culling_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_entity_scale_culling_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/entity_scale_culling.cpp,tests/unit/runtime_entity_scale_culling_tests.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- Phase 1 agent-surface drift updated `engine/agent/manifest.fragments/004-modules.json`, `engine/agent/manifest.fragments/014-gameCodeGuidance.json`, composed `engine/agent/manifest.json`, `docs/ai-game-development.md`, and `docs/current-capabilities.md`; no durable workflow, permission, rule, or subagent behavior changed, so `AGENTS.md`, skills, rules, and subagents did not need edits.
- Phase 1 gate validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (70/70 tests passed, `production-readiness-audit: unsupported_gaps=0`).
- Phase 2 RED evidence: extending `tests/unit/runtime_entity_scale_culling_tests.cpp` first failed at `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_entity_scale_culling_tests` because `RuntimeEntityScaleCullingDrawIntentKind`, `RuntimeEntityScaleCullingLodBandDesc`, LOD row fields, draw/update budget fields, and new diagnostic codes did not exist.
- Phase 2 implementation extends `plan_runtime_entity_scale_culling` with deterministic nearest-threshold LOD selection, visible-row draw intent, draw/update cost and update interval rows, budget-protected row metadata, projected draw/update cost counters, and fail-closed invalid LOD plus visible/draw/update budget diagnostics without renderer/RHI/native-handle or scene mutation.
- Phase 2 focused validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_entity_scale_culling_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_entity_scale_culling_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/entity_scale_culling.cpp,tests/unit/runtime_entity_scale_culling_tests.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- Phase 2 agent-surface drift updated `engine/agent/manifest.fragments/004-modules.json`, `engine/agent/manifest.fragments/014-gameCodeGuidance.json`, composed `engine/agent/manifest.json`, `docs/ai-game-development.md`, and `docs/current-capabilities.md`; no durable workflow, permission, rule, or subagent behavior changed, so `AGENTS.md`, skills, rules, and subagents did not need edits.
- Phase 2 gate validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (70/70 tests passed, `production-readiness-audit: unsupported_gaps=0`; shader-toolchain Apple/Metal and Apple host checks remained diagnostic-only host-gated on this Windows host).
