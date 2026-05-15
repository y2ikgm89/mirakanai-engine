# Runtime Hot Reload Recook Package Replacement Execution v1 (2026-05-16)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `MK_tools` orchestration helper that runs reviewed asset recook execution, stages runtime replacement rows, and commits one reviewed runtime package hot-reload resident replacement at a safe point.

**Architecture:** Keep `MK_runtime` free of tool/importer dependencies by placing the composition in `MK_tools`. Reuse `execute_asset_runtime_recook`, `AssetRuntimeReplacementState`, and `commit_runtime_package_hot_reload_recook_replacement_v2`; commit only selected package matched asset replacement rows after the runtime resident package replacement succeeds.

**Tech Stack:** C++23, `MK_tools`, `MK_assets`, `MK_runtime`, CMake/CTest, PowerShell repository validation wrappers.

---

## Context

`runtime-resource-v2` already has reviewed recook apply rows, hot-reload candidate review, replacement-intent review, and resident package replacement safe-point commit. `MK_tools` already executes asset recook plans through `execute_asset_runtime_recook`, but callers still have to manually connect staged recook rows to the runtime package replacement safe point and decide when the staged asset replacement state becomes active.

This slice adds the smallest bridge for that host-owned step. It does not add automatic file watching, background workers, inferred mount selection, automatic/LRU eviction, renderer/RHI ownership, upload/staging integration, allocator/GPU enforcement, native handles, package scripts, or broad hot reload productization.

## Constraints

- Do not add a duplicate resident package replacement primitive in `MK_runtime`.
- Do not make `MK_runtime` depend on `MK_tools` or importer execution.
- Keep the new API in `MK_tools` and require callers to provide the reviewed runtime replacement descriptor fields.
- Run recook execution before runtime package discovery/load, and block runtime resident replacement when recook fails.
- Preserve live `RuntimeResidentPackageMountSetV2`, `RuntimeResidentCatalogCacheV2`, and `AssetRuntimeReplacementState` active revisions on recook or runtime commit failure.
- Commit only selected package matched `AssetRuntimeReplacementState` pending rows after `commit_runtime_package_hot_reload_recook_replacement_v2` succeeds; do not flush unrelated pending replacements.
- Return recook execution, runtime replacement, committed asset apply rows, status, diagnostics, and no-file-watch evidence.
- Update docs, registry, master plan, manifest fragments, composed manifest, and static guards when the durable API surface changes.

## Official Practice Check

- CMake wiring follows the Context7 `/kitware/cmake` guidance for target-based test setup: add the focused executable with `add_executable`, link only required usage requirements through `target_link_libraries`, and register the CTest entry with `add_test(NAME ... COMMAND ...)`.

## Done When

- A public `MK_tools` helper, descriptor, result/status/diagnostic types exist for recook-to-runtime-package-replacement execution.
- Focused tests prove success, selected package commit isolation, recook failure preempts package reads/runtime commit, runtime commit failure preserves asset replacement state and resident state, and asset safe-point commit happens only after runtime success.
- `MK_tools_asset` links only the necessary engine modules and keeps dependency direction clean.
- Docs, registry, roadmap/current-capabilities/testing, master plan pointer, manifest fragments/composed output, and static guards describe the new API honestly.
- Focused target build/CTest and relevant static checks pass, followed by full `tools/validate.ps1` and `tools/build.ps1` before commit.

## Tasks

- [x] Add focused RED tests in `tests/unit/tools_runtime_hot_reload_package_tests.cpp`.
- [x] Register `MK_tools_runtime_hot_reload_package_tests` in `CMakeLists.txt`.
- [x] Run the focused target build/test and record the expected missing-symbol RED failure.
- [x] Add `engine/tools/include/mirakana/tools/asset_runtime_package_hot_reload_tool.hpp`.
- [x] Add `engine/tools/asset/asset_runtime_package_hot_reload_tool.cpp` and wire `MK_tools_asset` to `MK_runtime`.
- [x] Run the focused GREEN build/test.
- [x] Update docs, registry, master plan pointer, manifest fragments, compose output, and static guards.
- [x] Run focused static checks, full `validate.ps1`, and standalone `build.ps1`.
- [ ] Commit, push, open PR, and merge/auto-merge after PR preflight.

## Validation Evidence

| Check | Command | Result |
| --- | --- | --- |
| RED focused build/test | `pwsh -NoProfile -ExecutionPolicy Bypass -Command ". (Join-Path (Get-Location) 'tools/common.ps1'); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_tools_runtime_hot_reload_package_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_tools_runtime_hot_reload_package_tests"` | Failed as expected before implementation: `fatal error C1083: Cannot open include file: 'mirakana/tools/asset_runtime_package_hot_reload_tool.hpp'`. |
| GREEN focused build/test | Same focused command | Passed after implementation: `MK_tools_runtime_hot_reload_package_tests` built and CTest reported `100% tests passed, 0 tests failed out of 1`. |
| Selective commit focused build/test | `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_tools_runtime_hot_reload_package_tests MK_core_tests'` then matching CTest command | Passed after review hardening: `MK_core_tests` and `MK_tools_runtime_hot_reload_package_tests` built, then CTest reported `100% tests passed, 0 tests failed out of 2`. |
| Public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS: `public-api-boundary-check: ok`. |
| Format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS: `format-check: ok`. |
| Focused tidy | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/tools/asset/asset_runtime_package_hot_reload_tool.cpp,engine/tools/asset/asset_import_tool.cpp,engine/assets/src/asset_hot_reload.cpp,tests/unit/tools_runtime_hot_reload_package_tests.cpp,tests/unit/core_tests.cpp` | PASS: `tidy-check: ok (5 files)`. |
| JSON contracts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS: `json-contract-check: ok`. |
| Agent config | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS: `agent-config-check: ok`. |
| Agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS: `ai-integration-check: ok`. |
| Full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS: `validate: ok`; CTest reported `100% tests passed, 0 tests failed out of 65`; Metal/Apple checks remain diagnostic/host-gated on this Windows host. |
| Standalone build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS: exit 0; MSBuild emitted existing shared intermediate directory warnings (`MSB8028`). |
