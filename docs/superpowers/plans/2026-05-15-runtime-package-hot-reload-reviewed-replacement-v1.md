# Runtime Package Hot Reload Reviewed Replacement v1 (2026-05-15)

**Plan ID:** `runtime-package-hot-reload-reviewed-replacement-v1`
**Status:** Completed. Child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a host-independent reviewed hot-reload replacement safe point that composes selected package discovery/candidate load, an explicit existing resident mount id, optional caller-reviewed evictions, and atomic resident mount/cache replacement evidence.

## Context

`runtime-resource-v2` now has reviewed package discovery, selected candidate load, resident mount/replace commits, eviction planning, and standalone reviewed eviction commit execution. The next smallest hot-reload productization boundary is not file watching or background recook execution; it is a typed safe-point contract that lets a host present one discovered replacement candidate and one existing resident mount for review, then commit the replacement atomically with the same resident budget/cache rollback guarantees.

Implementation review showed that the existing `commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2` API already owns this exact safe-point contract: reviewed discovery, exact selected `.geindex` matching, explicit existing mount-id preflight, delegated candidate load/replacement, optional caller-reviewed evictions, projected resident cache refresh, typed evidence, and rollback. This slice therefore closes by formalizing that existing API as the host-driven reviewed hot-reload replacement safe point rather than adding a duplicate `HotReload` alias.

## Official Practice Check

- CMake: reuse the focused discovery resident replacement reviewed-evictions tests as the hot-reload replacement safe-point evidence and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit phase subresults, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep orchestration in `MK_runtime` over reviewed `RuntimePackageIndexDiscoveryDescV2`, `RuntimeResidentPackageMountIdV2`, `RuntimeResidentPackageMountSetV2`, `RuntimeResidentCatalogCacheV2`, `RuntimeResourceResidencyBudgetV2`, and optional caller-reviewed eviction candidates; do not add file watchers, recook execution, background workers, editor code, SDL3, renderer/RHI, upload/staging, native handles, inferred content roots, or automatic eviction policy.

## Constraints

- Reuse existing discovery, candidate-load, resident replacement, reviewed-eviction planning/commit, and catalog-cache behavior; do not add duplicate public aliases for an already-covered contract.
- Validate the selected package index path and existing resident mount id before package reads when possible.
- Apply only caller-reviewed eviction candidates; do not invent LRU, scoring, or automatic policy.
- Preserve live mount/cache state on descriptor, discovery, selection, candidate-load, replacement, reviewed-eviction, budget, or catalog-refresh failure.
- Return result evidence with discovery, selected candidate, candidate load, resident replacement, eviction/commit, catalog refresh, byte/count/generation counters, diagnostics, and committed state.
- Do not claim file watching, live recook, broad/background package streaming, renderer/RHI ownership, upload/staging package integration, allocator/GPU budget enforcement, inferred content roots, native handles, or 2D/3D vertical-slice readiness.

## Tasks

- [x] Confirm focused coverage for reviewed hot-reload replacement success, invalid descriptor/id rollback, discovery/selection failure rollback, candidate load failure rollback, reviewed eviction rollback, and projected budget/cache rollback.
- [x] Reuse the existing reviewed discovery resident replacement descriptor/result/status API without adding duplicate compatibility or alias surfaces.
- [x] Confirm existing discovery/selection/load/replacement/reviewed-eviction composition with diagnostic phase mapping and copy-then-commit preservation.
- [x] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A host can review and commit one selected package replacement for one resident mount at a safe point without file watcher or editor dependencies.
- Optional caller-reviewed resident evictions can be applied in the same projected transaction when the replacement would otherwise exceed the target resident budget.
- Result evidence exposes discovery, load, replacement/eviction, budget/catalog, generation/count/byte, diagnostics, and committed state.
- Current-truth docs and `engine/agent/manifest.json` describe this as a reviewed hot-reload replacement safe point only, with broad/background streaming, file watching/recook execution, RHI/upload/arbitrary-eviction/GPU-budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_discovery_resident_replace_reviewed_evictions_tests'` | PASS | Existing focused suite is the reviewed hot-reload replacement safe-point evidence; it covers selected discovery replacement, optional reviewed evictions, rollback, and evidence. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_runtime_package_discovery_resident_replace_reviewed_evictions_tests'` | PASS | Focused discovery resident replacement reviewed-evictions suite passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary guard passed; this slice formalizes the existing runtime API without adding duplicate aliases. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting guard passed after `tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_discovery_resident_replace_reviewed_evictions_tests.cpp` | PASS | Focused clang-tidy guard passed for the existing runtime implementation and focused evidence suite. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest JSON contract and composed manifest checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent-surface parity and text-format checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed; Apple/Metal host blockers remain diagnostic-only on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Standalone commit-preflight build passed. |
