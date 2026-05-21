# Engine World Region Streaming v1 (2026-05-21)

**Plan ID:** `engine-world-region-streaming-v1`
**Status:** Active.
**Current pointer rule:** `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points at this milestone while active. Keep `unsupportedProductionGaps = []`; this is a developer-owned scale-enabler capability, not a reopened Engine 1.0 production gap.

## Goal

Add first-party world-region streaming primitives so larger generated games can describe region/chunk package candidates, plan deterministic safe-point load/unload work, enforce resource budgets, and produce package-visible diagnostics without background streaming, renderer/RHI residency ownership, native handles, or game-specific world rules in the engine.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- `engine-procedural-generation-v1` completed deterministic seed/plan rows, runtime-scene procedural placement bridging, and selected `sample_2d_desktop_runtime_package --require-procedural-generation` package evidence.
- The developer-owned capability backlog lists `engine-world-region-streaming-v1` as a `scale-enabler` for larger maps that need region/chunk packages, safe load/unload, resource budgets, world-state persistence hooks, and resource/nav/physics partition hooks.
- Existing foundations include reviewed package index discovery, package candidate load, resident package mount/replace/unmount, reviewed eviction plans, resident catalog cache refresh, runtime scene validation, procedural placement rows, nav/physics package evidence, and desktop runtime package validation.
- This plan starts with value-only planning and selected package evidence. Broad async/background streaming, arbitrary eviction policy, renderer-owned residency, GPU allocator enforcement, native handles, navmesh/physics runtime partition execution, world-state serialization, and performance claims stay future work unless a later phase validates them explicitly.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If world-region streaming must become an Engine 1.0 blocker to proceed, stop.
- Keep the first public contracts deterministic, backend-neutral, and first-party. Do not introduce middleware, native handles, file watching, background workers, or renderer/RHI ownership in this milestone.
- Keep game-specific biome rules, encounter pacing, procedural region content, quest state, and save-game semantics in game-owned code/data unless a reusable multi-family primitive is proven.
- Reuse existing `MK_runtime` package discovery, candidate load, resident mount, catalog cache, and reviewed eviction primitives where possible.
- Start behavior/API/regression-risk changes with a RED test or static guard.
- Do not claim broad generated-game production readiness, broad package streaming, world persistence, nav/physics partition execution, or renderer performance from value-only region rows.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the active developer-owned scale-enabler capability after `engine-procedural-generation-v1` closeout.

### Done When

- `docs/superpowers/plans/README.md`, the readiness ledger, the master-plan index, and `docs/roadmap.md` list this plan as the active milestone.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: Region Catalog and Streaming Plan Contract

**Status:** Completed.

### Goal

Add the smallest deterministic value contract for reviewed world-region catalogs, active-region requests, safe-point load/unload intent rows, residency budget counters, and missing/duplicate/over-budget diagnostics.

### Done When

- RED tests fail first for missing world-region streaming catalog/plan contracts.
- `MK_runtime` exposes backend-neutral value rows for region ids, package candidates, active/desired region sets, safe-point load/unload actions, required preload assets, resident budget projections, and diagnostics.
- Focused tests prove deterministic ordering, duplicate/missing region diagnostics, active/desired diff planning, over-budget rejection, protected active-region behavior, and no filesystem/package/renderer mutation.

## Phase 2: Reviewed Safe-Point Package Bridge

**Status:** Completed.

### Goal

Bridge region plan rows to existing reviewed package candidate load, resident mount/replace/unmount, catalog cache refresh, and eviction primitives without introducing background streaming or broad automatic eviction policy.

### Done When

- RED tests fail first for missing region-to-package safe-point bridge behavior.
- Reviewed region load/unload rows can drive existing resident package helpers with stable mount ids, protected regions, explicit eviction candidates, and live-state preservation on failure.
- Diagnostics fail closed for missing package candidates, failed runtime scene validation evidence, budget failure, invalid mount ids, insufficient reviewed evictions, and catalog refresh failure.

## Phase 3: Package Evidence and Agent Surface Closeout

**Status:** Pending.

### Goal

Expose selected world-region streaming counters in a desktop runtime package lane and close the AI-operable contract surfaces for the supported narrow claim.

### Done When

- Selected package smokes report deterministic world-region counters, diagnostics, region load/unload intent rows, budget evidence, missing-region diagnostics, and reviewed package adoption fields.
- Docs, manifest fragments, schemas/static checks, skills/rules/subagents, and generated-game guidance are checked for drift and updated only where durable behavior or workflow changed.
- Full `tools/validate.ps1` passes at the coherent runtime/public-contract gate, with `unsupportedProductionGaps = []`.

## Validation Evidence

- Phase 0 starts after `engine-procedural-generation-v1` completed through PR #160, merge commit `5aad091b`, hosted PR Gate, Windows MSVC, Full Repository Static Analysis shards, Linux, CodeQL, iOS, macOS Metal CMake checks, and local full `tools/validate.ps1` evidence while `unsupportedProductionGaps = []` stayed empty.
- Phase 0 pointer sync selected this plan in `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, `docs/roadmap.md`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`; composed `engine/agent/manifest.json` reports `currentActivePlan=docs/superpowers/plans/2026-05-21-engine-world-region-streaming-v1.md`, `recommendedNextPlan.id=engine-world-region-streaming-v1`, and `unsupportedProductionGaps = []`.
- Phase 0 static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed with `unsupported_gaps=0`.
- Phase 1 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_world_region_streaming_tests` first failed on missing `mirakana/runtime/world_region_streaming.hpp`; a later mount-id hardening test failed until `duplicate_mount_id` and desired-region mount validation were implemented.
- Phase 1 implementation added `engine/runtime/include/mirakana/runtime/world_region_streaming.hpp`, `engine/runtime/src/world_region_streaming.cpp`, and `MK_runtime_world_region_streaming_tests` for deterministic region catalog validation, active/desired/protected region diff rows, load/keep/unload/no-change status, non-zero unique resident mount ids, `.geindex` package candidates, projected resident region/content/asset-record budgets, and fail-closed duplicate/missing/protected-unload/over-budget diagnostics without filesystem, package, renderer/RHI, or native-handle mutation.
- Phase 1 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_world_region_streaming_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_world_region_streaming_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/world_region_streaming.cpp,tests/unit/runtime_world_region_streaming_tests.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed.
- Phase 1 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `production-readiness-audit: unsupported_gaps=0`; Apple/Metal diagnostics remained host-gated only.
- Phase 2 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_world_region_streaming_tests` first failed on missing `RuntimeWorldRegionStreamingSafePointDesc`, `RuntimeWorldRegionStreamingSafePointStatus`, and `execute_runtime_world_region_streaming_safe_point`.
- Phase 2 implementation added `RuntimeWorldRegionStreamingSafePointDesc`, `RuntimeWorldRegionStreamingSafePointResult`, `RuntimeWorldRegionStreamingSafePointRowResult`, `RuntimeWorldRegionStreamingSafePointStatus`, and `execute_runtime_world_region_streaming_safe_point` in `MK_runtime`, bridging reviewed world-region load/unload rows through existing package streaming candidate resident mount/replacement with reviewed evictions or resident unmount helpers over a projected mount/cache view. The bridge fails closed for invalid plans, missing package candidates, runtime scene validation preflight, package load, budget, reviewed eviction, and catalog refresh failures, and commits live `RuntimeResidentPackageMountSetV2` / `RuntimeResidentCatalogCacheV2` state only after every row succeeds.
- Phase 2 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_world_region_streaming_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_world_region_streaming_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/world_region_streaming.cpp,tests/unit/runtime_world_region_streaming_tests.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed with `unsupported_gaps=0`.
- Phase 2 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; `production-readiness-audit` reported `unsupported_gaps=0`, all 69 CTest tests passed, and Apple/Metal diagnostics remained host-gated only.
