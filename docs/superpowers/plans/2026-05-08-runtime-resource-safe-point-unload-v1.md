# Runtime Resource Safe-Point Unload v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an explicit host-independent safe-point unload primitive for the active runtime package and `RuntimeResourceCatalogV2`.

**Architecture:** Extend the existing `RuntimeAssetPackageStore` / `RuntimeResourceCatalogV2` safe-point replacement path with a separate unload operation. The unload operation clears the active package only at a safe point, discards any pending replacement, rebuilds the catalog to an empty generation so old handles become stale, and keeps async streaming, eviction, renderer/RHI teardown, allocator enforcement, native handles, and platform behavior out of scope.

**Tech Stack:** C++23, `MK_runtime`, existing asset/runtime resource unit tests, PowerShell validation wrappers.

---

## Goal

Narrow the `runtime-resource-v2` gap by implementing the missing explicit safe-point unload half of the existing safe-point package replacement loop.

## Context

- `RuntimeAssetPackageStore` already supports active/pending package replacement through `stage`, `stage_if_loaded`, `commit_safe_point`, and `rollback_pending`.
- `commit_runtime_package_safe_point_replacement` already rebuilds a replacement catalog before swapping active packages.
- Manifest/docs already describe a `safe-point-package-unload-replacement-execution` loop, but the public runtime API only exposes replacement, not explicit active-package unload.

## Constraints

- Keep the operation host-independent and deterministic.
- Do not implement async/background streaming, arbitrary eviction policy, renderer/RHI teardown, allocator/GPU budget enforcement, native handles, or platform-specific behavior.
- Do not change existing replacement semantics.
- Do not touch pre-existing unrelated dirty guidance files.

## Done When

- `RuntimeAssetPackageStore` exposes an explicit safe-point unload primitive.
- `mirakana::runtime::commit_runtime_package_safe_point_unload` clears the active package/catalog together, invalidates previous handles through a generation bump, reports previous/committed generation and stale-handle counts, and reports whether a pending replacement was discarded.
- Calling unload without an active package reports `no_active_package` and does not mutate a staged pending package.
- Static checks require the manifest safe-point loop and docs to mention `RuntimePackageSafePointUnloadResult`.
- Focused tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before commit.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
- Modify: `engine/runtime/src/asset_runtime.cpp`
- Modify: `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`
- Modify: `engine/runtime/src/resource_runtime.cpp`
- Modify: `tests/unit/asset_identity_runtime_resource_tests.cpp`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

## Tasks

### Task 1: RED Runtime Tests

- [x] Add `MK_TEST("runtime package safe point unload clears active package and invalidates handles")`:
  - Seed an active package containing one texture.
  - Build `RuntimeResourceCatalogV2` and capture a live handle.
  - Stage a pending replacement package.
  - Call `commit_runtime_package_safe_point_unload`.
  - Expect `status == unloaded`, `discarded_pending_package == true`, active and pending packages null, empty catalog, changed generation, and the old handle stale.
- [x] Add `MK_TEST("runtime package safe point unload without active package preserves pending package")`:
  - Stage a package without seeding an active package.
  - Call `commit_runtime_package_safe_point_unload`.
  - Expect `status == no_active_package`, pending package still present, catalog generation unchanged, and no stale handles.
- [x] Run `cmake --build --preset dev --target MK_asset_identity_runtime_resource_tests` and confirm the expected compile failure on missing unload API/types.

### Task 2: Runtime API And Implementation

- [x] Add `RuntimeAssetPackageStore::unload_safe_point() noexcept -> bool`.
- [x] Add `RuntimePackageSafePointUnloadStatus`, `RuntimePackageSafePointUnloadResult`, and `commit_runtime_package_safe_point_unload` to `resource_runtime.hpp`.
- [x] Implement unload by:
  - returning `no_active_package` without mutation when no active package exists;
  - recording previous record count/generation;
  - clearing active and pending package state;
  - rebuilding the catalog from an empty `RuntimeAssetPackage`;
  - reporting stale handles when the previous generation was non-zero and changed.
- [x] Run focused build and CTest for `MK_asset_identity_runtime_resource_tests`.

### Task 3: Manifest, Static Checks, And Docs

- [x] Update `engine/agent/manifest.json` safe-point loop summary, ordered steps, result fields, and `runtime-resource-v2` notes with `RuntimePackageSafePointUnloadResult`.
- [x] Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` so the manifest must expose unload-specific steps/fields.
- [x] Update docs/current capabilities, roadmap, master plan, and plan registry.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `feat: add runtime resource safe point unload`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_asset_identity_runtime_resource_tests` | PASS (RED expected failure) | First direct attempt exposed host `Path`/`PATH` duplicate MSBuild environment; rerun with normalized process environment failed on missing `commit_runtime_package_safe_point_unload` / `RuntimePackageSafePointUnloadStatus` as expected. |
| `cmake --build --preset dev --target MK_asset_identity_runtime_resource_tests` | PASS | Focused target built after implementation. |
| `ctest --preset dev --output-on-failure -R MK_asset_identity_runtime_resource_tests` | PASS | Focused runtime resource tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration/static doc contract passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | JSON contract checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `runtime-resource-v2` remains `implemented-foundation-only` with narrowed notes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public runtime header boundary check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial failure was corrected by `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`; rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,engine/runtime/src/asset_runtime.cpp,tests/unit/asset_identity_runtime_resource_tests.cpp -MaxFiles 3` | PASS | Targeted tidy passed; new const-correctness warning in `resource_runtime.cpp` was fixed and rerun for that file passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed; Apple/Metal host lanes remain diagnostic/host-gated on Windows as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Commit-gate build passed. |

## Status

**Status:** Completed.
