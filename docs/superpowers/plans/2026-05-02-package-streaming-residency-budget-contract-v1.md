# Package Streaming Residency Budget Contract v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Define the first AI-operable package streaming and residency budget contract without claiming broad streaming, async eviction, native handle access, or production renderer quality.

**Architecture:** Reuse `mirakana_runtime` package/resource handles, runtime replacement staging, hot-reload safe-point concepts, `mirakana_rhi` resource lifetime rows, upload staging plans, package metadata, validation recipes, and host-owned renderer/RHI adapters. Keep gameplay on public package, scene, renderer, and validation descriptors. Do not expose native handles, execute arbitrary shell, add dependency-driven streaming middleware, or claim host-owned unload/eviction execution before focused validation exists.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_assets`, `mirakana_rhi`, `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, desktop runtime package metadata, schemas, static checks, docs, and focused tests.

---

## Goal

Make package streaming and residency-budget intent explicit enough for AI agents to plan safe follow-up work:

- describe selected package mount/replacement candidates and residency budget intent through reviewed descriptors
- keep unload/eviction and async streaming execution host-owned and blocked until validated
- distinguish package validation, resource upload readiness, budget intent, and actual residency execution in manifest/docs
- prevent agents from treating foundation rows as broad streaming readiness

## Constraints

- Do not implement broad package streaming in this slice unless a focused RED -> GREEN task narrows the execution path.
- Do not expose native renderer/RHI handles or `IRhiDevice` to gameplay.
- Do not add third-party streaming, allocator, or asset middleware.
- Do not claim async eviction, safe-point unload, texture streaming, memory budgeting, Metal readiness, or production renderer quality without explicit validation evidence.

## Done When

- A RED -> GREEN record exists in this plan.
- Static checks or tests distinguish descriptor/planning readiness from host-owned streaming/unload execution.
- `engine/agent/manifest.json`, docs, static checks, and validation recipes honestly classify broad package streaming and residency budgets as planned or host-gated.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Package And Residency Boundaries

- [x] Read runtime package handles, package replacement staging, hot reload recook state, RHI lifetime registry, upload staging, and desktop package validation scripts.
- [x] Identify the smallest descriptor/planning surface that can be AI-operable without claiming execution.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- `mirakana_runtime::RuntimeAssetPackageStore` already stages a loaded `RuntimeAssetPackage` and swaps it into active state only through `commit_safe_point`; failed loads are rejected by `stage_if_loaded`.
- `mirakana_runtime::RuntimeResourceCatalogV2` already rebuilds generation-checked resource handles from a cooked package, rejects duplicate package asset rows, and makes stale handles fail lookup after catalog replacement.
- `mirakana_assets::AssetRuntimeReplacementState` already models hot-reload recook staging, rollback, and safe-point commits for recooked asset files.
- `mirakana_rhi::RhiResourceLifetimeRegistry` and `mirakana_rhi::RhiUploadStagingPlan` already expose foundation-only lifetime, deferred-release, upload allocation, submission, retirement, and stale-generation diagnostics.
- The smallest safe contract for this slice is descriptor/planning-only `packageStreamingResidencyTargets`: rows select a manifest-declared runtime scene validation target, a package `.geindex`, positive resident budget bytes, safe-point intent, and optional asset/resource-kind hints.
- Non-goals for this slice: broad async package streaming, eviction execution, safe-point unload/replacement execution, runtime source parsing, public native/RHI handles, allocator middleware, Metal readiness, or production renderer quality.

### Task 2: RED Checks

- [x] Add failing checks for package streaming/residency budget descriptor fields and blocked execution status.
- [x] Add failing checks rejecting broad package streaming, async eviction, public native handles, Metal, and production renderer quality claims.
- [x] Record RED evidence.

### Task 3: Contract Implementation

- [x] Implement or tighten the selected descriptor/planning surface.
- [x] Keep runtime package mutation and renderer/RHI residency execution behind reviewed existing surfaces.
- [x] Keep gameplay code on public package/scene/renderer contracts.

Implementation notes:

- Added `game.agent.json.packageStreamingResidencyTargets` schema and static checks for safe game-relative `.geindex` paths, `runtimeSceneValidationTargets` references, `mode="planning-only"`, positive resident budget bytes, `safePointRequired=true`, optional safe asset-key/resource-kind/preflight hints, and forbidden shell/native/RHI/async execution fields.
- Added manifest `aiOperableProductionLoop.packageStreamingResidencyLoops` as a planned descriptor/planning loop that sequences target selection, runtime scene validation, budget-intent review, host-owned resource upload gate review, and blocked streaming/unload execution.
- Updated committed cooked-scene, material/shader, 2D package, and 3D package manifests plus generated templates to emit planning-only package streaming/residency budget descriptor rows.
- Kept runtime package mutation on existing package surfaces and kept renderer/RHI execution behind host-owned validation/resource execution loops; this slice does not add async streaming, eviction, unload/replacement execution, allocator budget enforcement, or native handle exposure.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after adding static checks because `schemas/game-agent.schema.json` did not define `packageStreamingResidencyTargets`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected because `schemas/game-agent.schema.json` did not contain `"packageStreamingResidencyTargets"` and `engine/agent/manifest.json` did not yet expose `packageStreamingResidencyLoops`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after adding `packageStreamingResidencyTargets`, committed sample descriptor rows, generated template rows, and `packageStreamingResidencyLoops` checks (`json-contract-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after synchronizing scaffold/static docs checks (`ai-integration-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reports `currentActivePlan` as this package streaming/residency budget plan with safe-point package unload/replacement execution as the recommended next plan.
- GREEN: After plan registry advancement, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reports `currentActivePlan` as `docs/superpowers/plans/2026-05-02-safe-point-package-unload-replacement-execution-v1.md` with `recommendedNextPlan.path` as `docs/superpowers/plans/2026-05-02-host-gated-package-streaming-execution-v1.md`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after the active-plan advancement and manifest guidance synchronization (`json-contract-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after the active-plan advancement and generated scaffold descriptor checks (`ai-integration-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed after the active-plan advancement with `status: passed`, `exitCode: 0`, and `durationSeconds: 3.03`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after the active-plan advancement, manifest guidance synchronization, docs updates, schema/static checks, CMake configure/build, and CTest (`100% tests passed, 0 tests failed out of 28`). Diagnostic-only host gates remain Metal tools missing, Apple packaging blocked on macOS/Xcode tools, Android release signing/device smoke not fully configured, and clang-tidy strict analysis gated by compile database availability before configure.
