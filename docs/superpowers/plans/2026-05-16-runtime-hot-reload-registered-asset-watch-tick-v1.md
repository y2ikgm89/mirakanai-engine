# Runtime Hot Reload Registered Asset Watch Tick v1 (2026-05-16)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-hot-reload-registered-asset-watch-tick-v1`
**Status:** Completed. Landed in PR #50 and merged as `66017f7`.
**Gap:** `runtime-resource-v2` foundation follow-up.

**Goal:** Add a host-independent `MK_tools` tick helper that scans registered asset files, advances hot-reload tracker/scheduler state, and executes the existing reviewed recook-to-runtime-package replacement safe point only when debounced recook requests are ready.

**Architecture:** Keep native watcher ownership out of `MK_tools`: callers may use polling/native watchers elsewhere, but this helper uses a caller-owned `IFileSystem`, `AssetRegistry`, `AssetDependencyGraph`, and persistent tick state. Reuse `scan_asset_files_for_hot_reload`, `AssetHotReloadTracker`, `AssetHotReloadRecookScheduler`, and `execute_asset_runtime_package_hot_reload_replacement_safe_point`; do not infer runtime package targets, choose evictions, run package scripts, spawn background work, or touch renderer/RHI/upload/native handles.

**Tech Stack:** C++23, `MK_tools`, `MK_assets`, `MK_runtime`, CTest, PowerShell repository validation wrappers.

---

## Context

`runtime-resource-v2` now has reviewed runtime package discovery/load/resident mount/replace/unmount, reviewed eviction planning/execution, hot-reload candidate and replacement intent review, recook change review, recook replacement safe-point composition, and `MK_tools` recook-to-resident-replacement execution. The remaining hot-reload edge is connecting the existing registered-file scan/tracker/scheduler contracts into that reviewed execution path without claiming automatic native watching or target inference.

## Constraints

- Do not add native watcher ownership, threads, background workers, package scripts, automatic target inference, automatic eviction policy, renderer/RHI ownership, upload/staging integration, allocator/GPU enforcement, or native handles.
- Keep the helper in `MK_tools`; `MK_runtime` must not depend on tools/importer execution.
- Require callers to provide the reviewed runtime replacement descriptor and import plan/options.
- Default the first tick to baseline priming, not recooking every registered asset on startup.
- Preserve scheduler pending rows when the debounce window has not elapsed.
- Preserve live resident mounts/cache and unrelated pending asset replacement rows by delegating to the existing replacement helper.

## Official Practice Check

- Context7 `/kitware/cmake` docs confirm the target-based test pattern this repository already uses when a new test target is needed: `add_executable`, `target_link_libraries` for usage requirements, and `add_test(NAME ... COMMAND ...)`. This slice is expected to extend the existing `MK_tools_runtime_hot_reload_package_tests` target unless the test file becomes too broad.

## Done When

- A public `MK_tools` watch-tick state/descriptor/result API exists for registered asset scan -> tracker -> scheduler -> reviewed replacement execution.
- Focused tests prove initial baseline priming, debounce-pending behavior, successful debounced modified-asset replacement, dependency-invalidated recook request forwarding, ready-request retry preservation on downstream failure, no-ready-change no-op behavior, and scan/recook failure diagnostics without runtime package reads.
- Docs, registry, roadmap/current capabilities/testing/editor notes, manifest fragments/composed output, skills, and static guards describe the new API honestly.
- Focused build/CTest and relevant static checks pass, followed by full `tools/validate.ps1` and `tools/build.ps1` before commit.

## Tasks

- [x] Add focused RED tests to `tests/unit/tools_runtime_hot_reload_package_tests.cpp`.
- [x] Add public watch-tick types and `execute_asset_runtime_package_hot_reload_registered_asset_watch_tick_safe_point` to `engine/tools/include/mirakana/tools/asset_runtime_package_hot_reload_tool.hpp`.
- [x] Implement the helper in `engine/tools/asset/asset_runtime_package_hot_reload_tool.cpp`.
- [x] Run focused GREEN build/CTest for `MK_tools_runtime_hot_reload_package_tests`.
- [x] Update docs, plan registry/master plan pointer, manifest fragments/composed output, skills, and static guards.
- [x] Run focused static checks, full `validate.ps1`, and standalone `build.ps1`.
- [x] Commit, push, open PR, and merge/auto-merge after PR preflight.

## Validation Evidence

| Check | Command | Result |
| --- | --- | --- |
| RED focused build/test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS: failed after adding tests and before implementation with missing `AssetRuntimePackageHotReloadRegisteredAssetWatchTick*` symbols. |
| GREEN focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -Command '& { . (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_tools_runtime_hot_reload_package_tests }'` | PASS |
| GREEN focused CTest | `ctest --preset dev --output-on-failure -R MK_tools_runtime_hot_reload_package_tests` | PASS: 1/1 test passed. |
| Public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS |
| Format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS |
| Focused tidy | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/tools/asset/asset_runtime_package_hot_reload_tool.cpp,tests/unit/tools_runtime_hot_reload_package_tests.cpp` | PASS: 2 files checked. |
| JSON contracts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS |
| Agent config | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS |
| Agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS |
| Full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS: 65/65 CTest tests passed; Metal/Apple diagnostics remain host-gated as expected. |
| Standalone build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS |
| PR preflight and merge | `gh pr view 50 --json state,isDraft,baseRefName,headRefName,headRefOid,mergeable,mergeStateStatus,reviewDecision,statusCheckRollup,autoMergeRequest,url`; `gh pr merge 50 --auto --merge --delete-branch --match-head-commit 9296aa037b31442af2e9cec855d213973b884627` | PASS: PR #50 merged into `main` as `66017f7`; branch deletion requested and stale remote tracking was pruned. |
