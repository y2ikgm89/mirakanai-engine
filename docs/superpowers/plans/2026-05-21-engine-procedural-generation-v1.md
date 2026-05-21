# Engine Procedural Generation v1 (2026-05-21)

**Plan ID:** `engine-procedural-generation-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is gameplay-family-enabler developer-owned capability work, not a reopened Engine 1.0 production gap.

## Goal

Add deterministic first-party procedural generation primitives so generated 2D/3D games can use seed-driven maps, encounters, loot, and object placement with validation, reproducible output rows, and package-visible evidence without external services or game-specific rules in the engine.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- `renderer-scene-scale-v1` completed through PR #159 for backend-neutral scene-scale policy diagnostics, package-visible scene-scale counters, selected D3D12 instanced draw execution evidence, hosted checks, and full validation evidence.
- The developer-owned capability backlog lists `engine-procedural-generation-v1` as a `gameplay-family-enabler` for replayable, sandbox, procedural, and systemic games that need seed-driven maps, encounters, loot, and object placement.
- Existing foundations include runtime scene gameplay binding/interactions, deterministic construction placement rows, inventory/item/crafting rows, quest/dialogue rows, behavior authoring rows, navigation/physics package evidence, source asset registry rows, and desktop runtime package validation.
- This plan starts with value-only and package-evidence contracts. Broad content quality, live world streaming, script runtimes, external generation services, marketplace-style content pipelines, and measured performance claims stay future work unless a later phase validates them explicitly.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If procedural generation must become an Engine 1.0 blocker to proceed, stop.
- Keep generator inputs/outputs versioned, deterministic, and first-party. Do not add external service dependencies.
- Keep game-specific generator rules, content taste, biome design, enemy balance, narrative pacing, and loot tables in game-owned code/data unless a reusable multi-family engine primitive is proven.
- Start behavior/API/regression-risk changes with a RED test or static guard.
- Do not claim broad generated-game production readiness, content quality, streaming scale, nav/physics runtime execution, or renderer performance from value-only procedural rows.
- Public contracts must remain backend-neutral and avoid native handles.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Close the completed renderer scene-scale active pointer and select `engine-procedural-generation-v1` as the next active developer-owned gameplay-family-enabler capability.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `renderer-scene-scale-v1` as completed.
- `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, and `docs/roadmap.md` identify this plan as active without reopening production gaps.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: Procedural Seed and Plan Contract

**Status:** Completed.

### Goal

Add the smallest deterministic value contract for seed-driven generation requests, stable generator streams, output rows, budget/extent diagnostics, and reproducible result hashes.

### Done When

- RED tests fail first for missing procedural generation seed/plan contracts.
- Public value rows distinguish valid requests, invalid seeds, invalid map/object budgets, duplicate output ids, unsupported content categories, and deterministic replay mismatches.
- Focused tests prove identical inputs produce identical rows and hashes without filesystem mutation, external services, native handles, package mutation, live scripting, or content-quality claims.

## Phase 2: Scene/Object Placement Bridge

**Status:** Completed.

### Goal

Connect procedural output rows to existing scene/runtime-scene and construction-placement primitives so generated games can review object placement, encounter anchors, loot anchors, and blocked/occupied placement diagnostics before runtime use.

### Done When

- RED tests fail first for missing procedural-to-scene/object placement evidence.
- Generated placement rows can be validated against existing scene node/component or construction-placement contracts without game-specific placement heuristics in the engine.
- Diagnostics fail closed for missing anchor ids, duplicate generated ids, invalid transforms, occupied placement cells, unsupported component requirements, and package-invisible output rows.

## Phase 3: Package Evidence and Agent Surface Closeout

**Status:** Pending.

### Goal

Expose selected procedural generation counters in a desktop runtime package lane and close the AI-operable contract surfaces for the supported narrow claim.

### Done When

- Selected package smokes report deterministic procedural generation counters, diagnostics, object/encounter/loot row counts, replay/hash evidence, and package-visible adoption fields.
- Docs, manifest fragments, schemas/static checks, skills/rules/subagents, and generated-game guidance are checked for drift and updated only where durable behavior or workflow changed.
- Full `tools/validate.ps1` passes at the coherent runtime/public-contract gate, with `unsupportedProductionGaps = []`.

## Validation Evidence

- Phase 0 started after `renderer-scene-scale-v1` completed through PR #159, merge commit `e459ef612b7ce34146ed9dad3369428647791038`, hosted PR Gate, Windows MSVC, Full Repository Static Analysis shards, Linux, CodeQL, iOS, and macOS Metal CMake checks, plus local full `tools/validate.ps1` evidence while `unsupportedProductionGaps = []` stayed empty.
- Phase 0 pointer sync selected this plan in `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, `docs/roadmap.md`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`; composed `engine/agent/manifest.json` reports `currentActivePlan=docs/superpowers/plans/2026-05-21-engine-procedural-generation-v1.md`, `recommendedNextPlan.id=engine-procedural-generation-v1`, and `unsupportedProductionGaps = []`.
- Phase 0 static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, and `git diff --check` passed with `unsupported_gaps=0`.
- Phase 1 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_procedural_generation_tests` failed first because `mirakana/runtime/procedural_generation.hpp` was missing.
- Phase 1 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_procedural_generation_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_procedural_generation_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/procedural_generation.cpp,tests/unit/runtime_procedural_generation_tests.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1` passed after adding `RuntimeProceduralSeedStream`, `RuntimeProceduralGenerationRequest`, `RuntimeProceduralGenerationContext`, `RuntimeProceduralGenerationPlan`, `make_runtime_procedural_seed_stream`, `advance_runtime_procedural_seed`, and `plan_runtime_procedural_generation`.
- Phase 1 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with CTest 68/68 and `production-readiness-audit: unsupported_gaps=0`.
- Phase 2 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_tests` failed first because `RuntimeSceneProceduralConstructionPlacementIntentDesc`, `plan_runtime_scene_procedural_construction_placement_intents`, and the procedural placement diagnostics were missing.
- Phase 2 implementation evidence: `MK_runtime_scene` now exposes `RuntimeSceneProceduralConstructionPlacementIntentDesc` plus `plan_runtime_scene_procedural_construction_placement_intents`, preserving existing construction placement intent planning while attaching procedural output id, anchor id, content kind, and package-visible evidence to rows and diagnostics. The bridge accepts reviewed object/encounter/loot rows and fails closed for invalid procedural plans, missing or duplicate procedural output ids, missing anchors, unsupported map-tile placement attempts, package-invisible output rows, invalid transforms, occupied placement cells, and invalid node/component requirements without game-specific placement heuristics or implicit scene mutation.
- Phase 2 focused/static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_scene_tests|MK_runtime_procedural_generation_tests"`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_scene/src/runtime_scene.cpp,tests/unit/runtime_scene_tests.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, and `git diff --check` passed with `unsupported_gaps=0`.
- Phase 2 review-hardening evidence: additional RED tests failed first for mixed procedural batches that should preserve valid placement rows and for duplicate-node diagnostics that must keep the second row's procedural output metadata; the bridge now merges procedural prevalidation rows with forwarded construction-placement results by source index instead of guessing by `(candidate_index,node_name)`.
- Phase 2 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with CTest 68/68 and `production-readiness-audit: unsupported_gaps=0`; Apple/Metal checks remained host-gated or diagnostic-only on this Windows host.
