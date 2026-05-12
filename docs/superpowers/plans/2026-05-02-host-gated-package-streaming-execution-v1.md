# Host-Gated Package Streaming Execution v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After safe-point unload/replacement execution is validated, narrow host-owned package streaming execution for selected package streaming/residency descriptors without turning it into broad async streaming or public native-handle access.

**Architecture:** Reuse `packageStreamingResidencyTargets`, runtime scene validation, safe-point package replacement, runtime resource catalog generation checks, and host-owned renderer/RHI resource lifetime/upload boundaries. Keep execution explicit, deterministic, host-gated, and separate from gameplay code.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_assets`, `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, `mirakana_rhi`, desktop runtime package validation scripts, schemas, docs, and focused tests.

---

## Goal

Make a selected package streaming descriptor executable only through a narrow host-gated path:

- require prior runtime scene validation and safe-point replacement readiness
- keep package mount/replace/retire ordering deterministic
- keep renderer/RHI upload and teardown host-owned
- report first-party diagnostics without exposing native handles

## Constraints

- Do not implement broad background streaming, arbitrary eviction, or dependency-driven streaming middleware.
- Do not expose public native, renderer, or RHI handles to gameplay or manifests.
- Do not claim allocator/GPU memory budget enforcement beyond validated counters.
- Do not claim Metal readiness or general renderer quality without focused host validation evidence.

## Done When

- A RED -> GREEN record exists in this plan.
- Tests or static checks distinguish selected host-gated streaming execution from broad async streaming and unsafe native-handle access.
- `engine/agent/manifest.json`, docs, static checks, and validation recipes remain honest about unsupported broad streaming, allocator enforcement, Metal, and renderer quality claims.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Host-Gated Streaming Execution Boundaries

- [x] Read `packageStreamingResidencyTargets`, safe-point package unload/replacement execution, runtime resource catalog replacement, RHI lifetime retirement, and package smoke validation scripts.
- [x] Identify the smallest host-owned execution path that can consume descriptor rows without broad streaming.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- `game.agent.json.packageStreamingResidencyTargets` rows currently remain descriptor-only with `mode="planning-only"`, safe game-relative `.geindex` paths, `runtimeSceneValidationTargets` references, positive `residentBudgetBytes`, and forbidden async/native/allocator command fields.
- `mirakana::runtime::commit_runtime_package_safe_point_replacement` now provides the narrow package/catalog safe-point execution primitive for preloaded packages: candidate catalog build first, active package/catalog swap only on success, invalid pending package rollback, generation changes, and stale-handle invalidation counts.
- `validate-runtime-scene-package` remains the required preflight for manifest-selected packages; it loads explicit `.geindex` packages and instantiates runtime scenes without mutation, renderer/RHI residency, source parsing, or shell execution.
- Host-owned renderer/RHI upload execution already exists behind `mirakana_runtime_scene_rhi::execute_runtime_scene_gpu_upload` and desktop package smoke fields, while RHI lifetime retirement remains a separate host-owned registry concern.
- The smallest host-gated streaming execution path should consume one selected descriptor row, require runtime scene validation evidence, load/stage the selected replacement package, call the safe-point package/catalog helper, optionally compare resident budget intent against package/catalog byte metadata where available, and report deterministic first-party diagnostics. It should not run a background streaming thread or dependency-driven streamer.
- Non-goals for this slice: broad async/background package streaming, arbitrary eviction, dependency-driven streaming middleware, renderer/RHI teardown execution, allocator/GPU memory budget enforcement, package cooking, runtime source parsing, public native/RHI handles, Metal readiness, or general renderer quality.

### Task 2: RED Checks

- [x] Add failing tests or static checks for selected host-gated streaming execution and deterministic diagnostics.
- [x] Add failing checks rejecting broad background streaming, arbitrary eviction, public native handles, allocator enforcement claims, Metal readiness, and production renderer quality claims.
- [x] Record RED evidence.

### Task 3: Host-Gated Execution Implementation

- [x] Implement or tighten the selected host-owned streaming execution path.
- [x] Keep renderer/RHI upload, lifetime retirement, and native teardown behind host-owned adapters.
- [x] Preserve deterministic diagnostics for validation failures, stale handles, over-budget intent, and rejected unsafe execution requests.

Implementation notes:

- Added `mirakana::runtime::execute_selected_runtime_package_streaming_safe_point` in `mirakana_runtime` as an RHI-free helper over one selected descriptor. It rejects planning-only/unsafe descriptors, requires runtime scene validation preflight, consumes an already loaded `RuntimeAssetPackageLoadResult`, checks resident budget intent, stages the package, and commits through `commit_runtime_package_safe_point_replacement`.
- `RuntimePackageStreamingExecutionResult` reports selected target ids, estimated resident bytes, resident budget bytes, replacement status, stale-handle counts, and deterministic diagnostics. Over-budget intent is reported with `resident-budget-intent-exceeded` and does not claim allocator/GPU enforcement.
- Renderer/RHI upload, deferred lifetime retirement, and native teardown remain outside the helper and are guarded by static source scans that reject renderer/RHI/native symbols in the package streaming execution files.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected because `engine/agent/manifest.json` did not expose `host-gated-package-streaming-execution`.
- RED blocker: direct `cmake --build --preset dev --target mirakana_asset_identity_runtime_resource_tests` failed because `cmake` was not on the shell PATH; the project tool resolver found CMake through Visual Studio.
- RED blocker: first focused MSBuild attempt failed before compiling the RED tests because the process environment contained duplicate `Path`/`PATH` keys; rerunning with uppercase `Env:PATH` removed kept the build path usable.
- GREEN: focused MSBuild with normalized process `Path` passed for `mirakana_asset_identity_runtime_resource_tests`.
- GREEN: `out/build/dev/Debug/mirakana_asset_identity_runtime_resource_tests.exe` passed 13/13 tests, including selected streaming execution preflight, commit, unsafe descriptor rejection, and over-budget intent diagnostics.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed (`json-contract-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed (`ai-integration-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed (`public-api-boundary-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed after advancing `currentActivePlan` to `docs/superpowers/plans/2026-05-02-2d-atlas-tilemap-package-authoring-v1.md` and `recommendedNextPlan.path` to `docs/superpowers/plans/2026-05-02-3d-prefab-scene-package-authoring-v1.md`.
- GREEN: After advancing the plan registry, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed (`json-contract-check: ok`) and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed (`ai-integration-check: ok`).
- GREEN: After advancing the plan registry, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. CTest reported 28/28 tests passing. Diagnostic-only host gates remain: Metal tools missing, Apple packaging blocked by macOS/Xcode tools, Android release signing/device smoke not fully configured, and strict tidy compile database unavailable for the active Visual Studio generator.
