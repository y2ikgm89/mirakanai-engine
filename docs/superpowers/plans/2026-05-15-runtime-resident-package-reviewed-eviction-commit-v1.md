# Runtime Resident Package Reviewed Eviction Commit v1 (2026-05-15)

**Plan ID:** `runtime-resident-package-reviewed-eviction-commit-v1`
**Status:** Completed. Current child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a host-independent safe-point helper that atomically applies a caller-reviewed resident eviction candidate order to `RuntimeResidentPackageMountSetV2` and `RuntimeResidentCatalogCacheV2`.

## Context

`runtime-resource-v2` now has reviewed eviction planning and several candidate/discovery/package-streaming paths that internally execute caller-reviewed evictions while mounting or replacing a package. The next smallest clean-break boundary is a standalone resident eviction commit helper so hosts can review and apply an eviction plan without also loading, mounting, replacing, or discovering packages.

## Official Practice Check

- CMake: keep focused runtime resident unmount/eviction unit tests and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit statuses/subresults, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep orchestration in `MK_runtime` over `RuntimeResidentPackageMountSetV2`, `RuntimeResidentCatalogCacheV2`, `RuntimeResourceResidencyBudgetV2`, and reviewed `RuntimeResidentPackageMountIdV2` rows; do not introduce filesystem discovery/load, editor, SDL3, renderer/RHI, upload/staging, native handles, background workers, inferred content roots, or automatic eviction policy.

## Constraints

- Reuse `plan_runtime_resident_package_evictions_v2` and existing resident unmount/cache-refresh behavior.
- Apply only caller-reviewed eviction candidates; do not invent LRU, scoring, or automatic policy.
- Preserve live mount/cache state on invalid/duplicate/missing/protected candidates, insufficient reviewed candidates, budget failure, or catalog refresh failure.
- Support no-op success when the current resident view already satisfies the target budget.
- Return result evidence with eviction plan, catalog refresh, applied eviction count, byte/count/generation counters, diagnostics, and committed state.
- Do not claim broad/background package streaming, hot reload productization, renderer/RHI ownership, upload/staging package integration, allocator/GPU budget enforcement, inferred content roots, native handles, or 2D/3D vertical-slice readiness.

## Tasks

- [x] Add RED tests for standalone reviewed eviction commit success, no-op success, invalid/duplicate/missing/protected candidate rollback, insufficient reviewed candidates rollback, and catalog/budget rollback.
- [x] Add the public reviewed resident eviction commit overload/result evidence needed by the tests.
- [x] Implement reviewed planning, projected unmount execution, projected catalog refresh, diagnostic/status mapping, evidence propagation, and rollback preservation.
- [x] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A host can apply a reviewed resident eviction candidate order atomically to satisfy a resident budget.
- No-op, invalid candidate, protected candidate, insufficient candidate, budget, and catalog-refresh outcomes are typed and leave live mount/cache state unchanged unless committed.
- Result evidence includes eviction plan, catalog refresh, estimated bytes, resident package count, mount generation, diagnostics, and committed state.
- Current-truth docs and `engine/agent/manifest.json` describe this as standalone reviewed resident eviction execution only, with broad/background streaming, hot reload/RHI/upload/arbitrary-eviction/GPU-budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_resource_resident_unmount_tests'` | PASS | RED failed before implementation on missing reviewed-eviction commit API; GREEN passed after implementation and docs sync. |
| `ctest --preset dev --output-on-failure -R MK_runtime_resource_resident_unmount_tests` | PASS | Focused resident unmount/eviction suite passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public runtime API boundary passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_resource_resident_unmount_tests.cpp` | PASS | Focused clang-tidy guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest JSON contract and composed manifest checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent-surface parity and text-format checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration guard passed after manifest reason/history and reviewed-eviction needles were synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed; CTest reported 60/60 tests passed, with existing Apple/Metal host gates remaining diagnostic-only on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Standalone commit-preflight build passed; MSBuild repeated existing shared-intermediate-directory warnings. |
