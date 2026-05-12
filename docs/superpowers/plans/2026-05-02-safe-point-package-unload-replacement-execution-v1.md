# Safe-Point Package Unload Replacement Execution v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Narrow runtime package unload/replacement into a host-owned safe-point execution path after package streaming and residency budget intent is explicit and validated.

**Architecture:** Reuse `mirakana_runtime` staged package replacement, runtime resource catalog generation checks, package validation diagnostics, and host-owned renderer/RHI teardown boundaries. Keep unload/replacement execution behind explicit safe points, keep gameplay on public package/resource handles, and do not expose backend or native handles.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_assets`, `mirakana_runtime_rhi`, desktop runtime package validation scripts, schemas, static checks, docs, and focused tests.

---

## Goal

Make package unload and replacement execution narrow, deterministic, and reviewable:

- execute selected package replacement only at host-owned safe points
- preserve generation-checked handle invalidation and diagnostics after replacement
- separate unload/replacement execution from broad async package streaming
- keep renderer/RHI teardown and native handles behind backend-owned adapters

## Constraints

- Do not implement broad async package streaming.
- Do not expose public native, renderer, or RHI handles to gameplay or manifests.
- Do not add third-party streaming, allocator, or hot-reload middleware.
- Do not claim Metal readiness or general renderer quality without Apple-host or focused renderer validation evidence.

## Done When

- A RED -> GREEN record exists in this plan.
- Tests or static checks distinguish safe-point replacement execution from broad async streaming and unsafe native-handle access.
- `engine/agent/manifest.json`, docs, static checks, and validation recipes remain honest about unsupported broad streaming, async eviction, and renderer quality claims.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Safe-Point Replacement Boundaries

- [x] Read runtime package staging, runtime resource catalog replacement, stale-handle diagnostics, hot-reload safe-point notes, and package smoke validation scripts.
- [x] Identify the smallest host-owned execution path that can unload/replace selected packages without broad streaming.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- `mirakana::runtime::RuntimeAssetPackageStore` already supports `seed`, `stage`, `stage_if_loaded`, `commit_safe_point`, `rollback_pending`, `active`, and `pending`; existing tests prove pending replacements do not affect the active package before `commit_safe_point`, and failed loads do not replace the active package.
- `mirakana::runtime::RuntimeResourceCatalogV2` already rebuilds generation-checked records from a `RuntimeAssetPackage`, increments catalog generation on successful rebuild, rejects duplicate package asset rows without replacing the existing catalog, and rejects stale handles after catalog replacement.
- `mirakana::AssetRuntimeReplacementState` already models source recook staging, rollback on failed recook, and sorted safe-point commits for per-asset hot reload state.
- Desktop package smoke paths already validate explicit `--require-scene-package` package loading and host-owned scene GPU upload, but they do not expose a reusable package safe-point replacement execution result.
- Renderer/RHI ownership remains host-side: `mirakana_runtime_scene_rhi::execute_runtime_scene_gpu_upload` takes a host-owned `IRhiDevice`, RHI resource lifetime uses deferred release/retire records, and gameplay/manifests do not receive native handles.
- The smallest host-owned execution path for this slice is a runtime helper that stages a preloaded package, rebuilds a runtime resource catalog from the pending package, commits both package and catalog only at a safe point, rejects invalid/duplicate replacement packages before commit, and reports deterministic diagnostics including no-pending, catalog-build failure, committed package record counts, generation changes, and stale-handle invalidation.
- Non-goals for this slice: background or async package streaming, eviction scheduling, texture streaming, allocator/GPU memory budget enforcement, renderer/RHI teardown execution, public `IRhiDevice` or native handles, manifest shell commands, package cooking, runtime source parsing, Metal readiness, and general renderer quality.

### Task 2: RED Checks

- [x] Add failing tests or static checks for explicit safe-point replacement execution and stale-handle diagnostics.
- [x] Add failing checks rejecting broad async streaming, public native handles, unsafe eviction, Metal readiness, and production renderer quality claims.
- [x] Record RED evidence.

### Task 3: Safe-Point Execution Implementation

- [x] Implement or tighten the selected safe-point replacement execution path.
- [x] Keep renderer/RHI teardown host-owned and separate from gameplay package handles.
- [x] Preserve deterministic diagnostics for replaced packages, stale handles, and invalid replacement requests.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed after adding safe-point replacement tests because `mirakana::runtime::commit_runtime_package_safe_point_replacement` and `mirakana::runtime::RuntimePackageSafePointReplacementStatus` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding `RuntimePackageSafePointReplacementStatus`, `RuntimePackageSafePointReplacementResult`, `RuntimeResourceCatalogV2::generation`, and `commit_runtime_package_safe_point_replacement`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after the safe-point helper implementation (`100% tests passed, 0 tests failed out of 28`).
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed after adding schema/static requirements because `engine/agent/manifest.json.aiOperableProductionLoop` did not yet contain `safePointPackageReplacementLoops`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed after adding static requirements because `engine/agent/manifest.json` did not yet contain `safePointPackageReplacementLoops`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after adding manifest `safePointPackageReplacementLoops`, docs, and registry updates (`json-contract-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after synchronizing docs/manifest/static checks (`ai-integration-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reports `currentActivePlan` as `docs/superpowers/plans/2026-05-02-host-gated-package-streaming-execution-v1.md` with `recommendedNextPlan.path` as `docs/superpowers/plans/2026-05-02-2d-atlas-tilemap-package-authoring-v1.md`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the public `mirakana_runtime` header change (`public-api-boundary-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed with `status: passed`, `exitCode: 0`, and `durationSeconds: 3.051`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after safe-point replacement implementation, docs/manifest/static checks, active-plan advancement, CMake configure/build, and CTest (`100% tests passed, 0 tests failed out of 28`). Diagnostic-only host gates remain Metal tools missing, Apple packaging blocked on macOS/Xcode tools, Android release signing/device smoke not fully configured, and clang-tidy strict analysis gated by compile database availability before configure.
