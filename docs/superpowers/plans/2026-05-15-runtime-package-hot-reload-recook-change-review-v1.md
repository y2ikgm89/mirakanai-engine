# Runtime Package Hot Reload Recook Change Review v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a pure `MK_runtime` planner that turns reviewed asset recook apply results into runtime package hot-reload candidate review rows without running recook, package load, resident commits, file watching, background streaming, renderer/RHI, upload/staging, allocator/GPU enforcement, or native handles.

**Architecture:** Reuse the existing `AssetHotReloadApplyResult` and `plan_runtime_package_hot_reload_candidate_review_v2` contracts. The new planner validates successful `staged`/`applied` recook output rows, blocks failed or invalid recook rows, delegates exact package-index/content matching to the existing candidate review planner, and returns explicit no-side-effect evidence.

**Tech Stack:** C++23, `MK_runtime`, `MK_assets` public hot-reload rows, CMake/CTest, PowerShell repository validation wrappers.

---

## Context

`runtime-resource-v2` already has reviewed `.geindex` discovery, candidate load, resident mount/replace, reviewed eviction planning/commit, hot-reload candidate review, and hot-reload replacement intent review. The next narrow boundary is between existing asset recook execution evidence and runtime package hot-reload candidate review.

This slice intentionally does not implement automatic file watching, recook/build execution, background streaming, automatic/LRU eviction, renderer/RHI resource ownership, upload/staging integration, allocator/GPU memory enforcement, inferred content roots, or native handles.

## Constraints

- Keep the planner pure: no `IFileSystem`, no package load/read, no resident mount/cache mutation, no recook execution.
- Accept only existing reviewed `AssetHotReloadApplyResult` rows.
- Treat `AssetHotReloadApplyResult::path` as a caller-reviewed runtime package/index/content relative VFS path, not as a source-asset path.
- Treat `staged` and `applied` rows as successful changed package paths.
- Treat `failed_rolled_back`, `unknown`, empty/unsafe paths, zero asset ids, and invalid applied active revisions as blocking diagnostics before candidate review delegation.
- Preserve deterministic sorted/de-duplicated behavior by delegating path matching to `plan_runtime_package_hot_reload_candidate_review_v2`.
- Update docs, manifest fragments, composed manifest, and static guards when the durable API surface changes.

## Done When

- `RuntimePackageHotReloadRecookChangeReviewDescV2`, result/status/diagnostic types, and `plan_runtime_package_hot_reload_recook_change_review_v2` exist in `MK_runtime`.
- Focused tests prove valid staged/applied rows produce candidate review rows, failed/invalid rows block before candidate review, no successful recook rows returns no changes, candidate-review failures are surfaced, and no-side-effect flags remain false.
- Docs, registry, master plan pointer, manifest fragments/composed output, and static guards describe the new API honestly.
- Focused target build/CTest and relevant static checks pass, followed by full `tools/validate.ps1` and `tools/build.ps1` before commit.

## Tasks

- [x] Add focused RED tests in `tests/unit/runtime_package_hot_reload_recook_change_review_tests.cpp`.
- [x] Register `MK_runtime_package_hot_reload_recook_change_review_tests` in `CMakeLists.txt`.
- [x] Run the focused target build/test and record the expected missing-symbol RED failure.
- [x] Implement the public API and pure planner in `engine/runtime/include/mirakana/runtime/resource_runtime.hpp` and `engine/runtime/src/resource_runtime.cpp`.
- [x] Run the focused GREEN build/test.
- [x] Update docs, registry, master plan pointer, manifest fragments, compose output, and static guards.
- [x] Run focused static checks, full `validate.ps1`, and standalone `build.ps1`.
- [ ] Commit, push, open PR, and merge/auto-merge after PR preflight.

## Validation Evidence

| Check | Command | Result |
| --- | --- | --- |
| RED focused build/test | `pwsh -NoProfile -ExecutionPolicy Bypass -Command ". (Join-Path (Get-Location) 'tools/common.ps1'); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_hot_reload_recook_change_review_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_runtime_package_hot_reload_recook_change_review_tests"` | PASS: target build failed before production implementation on missing `plan_runtime_package_hot_reload_recook_change_review_v2`, `RuntimePackageHotReloadRecookChangeReviewDescV2`, status, and diagnostic phase symbols. |
| GREEN focused build/test | Same focused command | PASS: `MK_runtime_package_hot_reload_recook_change_review_tests` built and `ctest -R MK_runtime_package_hot_reload_recook_change_review_tests` passed. |
| Review regression RED | Same focused command after adding the out-of-range `AssetHotReloadApplyResultKind` test | PASS: failed on `!out_of_range_kind.succeeded()` before the whitelist fix. |
| Review regression GREEN | Same focused command after the whitelist fix | PASS: `MK_runtime_package_hot_reload_recook_change_review_tests` passed. |
| Public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS: `public-api-boundary-check: ok`. |
| Format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS: `format-check: ok`. |
| Focused tidy | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_hot_reload_recook_change_review_tests.cpp` | PASS: `tidy-check: ok (2 files)`. |
| JSON contracts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS: `json-contract-check: ok`. |
| Agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS: `ai-integration-check: ok`. |
| Full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS: `validate: ok`; CTest reported 63/63 passing. Metal/Apple lanes remained diagnostic host-gated on Windows. |
| Standalone build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS: build completed with exit code 0. |
