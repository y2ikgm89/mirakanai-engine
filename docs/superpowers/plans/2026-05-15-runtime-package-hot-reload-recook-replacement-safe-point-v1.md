# Runtime Package Hot Reload Recook Replacement Safe Point v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed `MK_runtime` safe-point composition API that takes reviewed recook apply rows through candidate review, replacement intent review, and the existing reviewed resident replacement commit without duplicating the resident replacement primitive.

**Architecture:** Reuse `plan_runtime_package_hot_reload_recook_change_review_v2`, `plan_runtime_package_hot_reload_replacement_intent_review_v2`, and `commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2` in sequence. The new API only coordinates those reviewed contracts, selects one reviewed candidate by exact package-index path, propagates diagnostics/subresults, and commits resident state only after every review stage succeeds.

**Tech Stack:** C++23, `MK_runtime`, `MK_assets` public hot-reload rows, CMake/CTest, PowerShell repository validation wrappers.

---

## Context

`runtime-resource-v2` already has the building blocks for host-driven hot reload: recook apply-result review, candidate review, replacement intent review, and reviewed replacement safe-point commit. The missing narrow boundary is the safe-point composition from recook apply rows to one reviewed resident replacement commit.

This slice intentionally does not implement file watching, recook/importer execution, editor UI wiring, background streaming, automatic/LRU eviction, renderer/RHI ownership, upload/staging integration, allocator/GPU enforcement, inferred content roots, native handles, or 2D/3D vertical-slice readiness.

## Constraints

- Do not add a duplicate resident replacement primitive or compatibility alias.
- Keep `commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2` as the sole live resident replacement executor.
- Accept already-reviewed `AssetHotReloadApplyResult` rows and already-reviewed discovery candidate rows; do not run recook or discover packages until the final reviewed commit stage.
- Select exactly one hot-reload candidate by `selected_package_index_path` from `recook_change_review.candidate_review.rows`.
- Run replacement intent review before package discovery/package reads.
- Preserve live mount/cache state on recook review, candidate selection, intent review, discovery, package load, eviction, budget, resident commit, or catalog refresh failure.
- Return subresults, deterministic diagnostics, counters, and explicit no-file-watch/no-recook flags.
- Update docs, registry, master plan, manifest fragments, composed manifest, and static guards when the durable API surface changes.

## Done When

- `RuntimePackageHotReloadRecookReplacementDescV2`, result/status/diagnostic types, and `commit_runtime_package_hot_reload_recook_replacement_v2` exist in `MK_runtime`.
- Focused tests prove the valid path commits a reviewed replacement, recook review failures block before package reads/commit, candidate selection failures block before intent review, intent review failures block before discovery/package reads/commit, and commit-stage failures preserve live state.
- Docs, registry, roadmap/current-capabilities/testing, master plan pointer, manifest fragments/composed output, and static guards describe the new API honestly.
- Focused target build/CTest and relevant static checks pass, followed by full `tools/validate.ps1` and `tools/build.ps1` before commit.

## Tasks

- [x] Add focused RED tests in `tests/unit/runtime_package_hot_reload_recook_replacement_tests.cpp`.
- [x] Register `MK_runtime_package_hot_reload_recook_replacement_tests` in `CMakeLists.txt`.
- [x] Run the focused target build/test and record the expected missing-symbol RED failure.
- [x] Implement the public API and safe-point composition in `engine/runtime/include/mirakana/runtime/resource_runtime.hpp` and `engine/runtime/src/resource_runtime.cpp`.
- [x] Run the focused GREEN build/test.
- [x] Update docs, registry, master plan pointer, manifest fragments, compose output, and static guards.
- [x] Run focused static checks, full `validate.ps1`, and standalone `build.ps1`.
- [ ] Commit, push, open PR, and merge/auto-merge after PR preflight.

## Validation Evidence

| Check | Command | Result |
| --- | --- | --- |
| RED focused build/test | `pwsh -NoProfile -ExecutionPolicy Bypass -Command ". (Join-Path (Get-Location) 'tools/common.ps1'); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_hot_reload_recook_replacement_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_runtime_package_hot_reload_recook_replacement_tests"` | PASS: target build failed before implementation on missing `RuntimePackageHotReloadRecookReplacementDescV2`, status/diagnostic phase types, and `commit_runtime_package_hot_reload_recook_replacement_v2`. |
| GREEN focused build/test | Same focused command | PASS: `MK_runtime_package_hot_reload_recook_replacement_tests` built and `ctest -R MK_runtime_package_hot_reload_recook_replacement_tests` passed. After code-review fixes, the same focused command passed with content-path recook input, delegated candidate-load diagnostics, and resident-budget failure preservation coverage. |
| Public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS: `public-api-boundary-check: ok`. |
| Format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS: `text-format-check: ok`; `format-check: ok`. |
| Focused tidy | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_hot_reload_recook_replacement_tests.cpp` | PASS: `tidy-check: ok (2 files)`. |
| JSON contracts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS: `agent-manifest-compose: ok`; `json-contract-check: ok`. |
| Agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS: `ai-integration-check: ok`. |
| Agent config | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS: `agent-config-check: ok`. |
| PowerShell analyzer | `pwsh -NoProfile -ExecutionPolicy Bypass -Command 'if (Get-Module -ListAvailable -Name PSScriptAnalyzer) { Invoke-ScriptAnalyzer -Path tools/check-ai-integration-020-engine-manifest.ps1,tools/check-ai-integration-030-runtime-rendering.ps1,tools/check-json-contracts-010-engine-manifest.ps1,tools/check-json-contracts-030-tooling-contracts.ps1,tools/check-json-contracts-040-agent-surfaces.ps1 -Severity Error } else { "PSScriptAnalyzer not installed" }'` | WARN: PSScriptAnalyzer is not installed on this host. |
| Full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS: `validate: ok`; CTest reported 64/64 passing. Metal/Apple lanes remained diagnostic-only/host-gated on this Windows host. |
| Standalone build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS: standalone build completed. MSBuild emitted existing shared intermediate-directory warnings. |
