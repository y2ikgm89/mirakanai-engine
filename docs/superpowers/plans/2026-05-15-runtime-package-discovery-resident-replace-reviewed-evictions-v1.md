# Runtime Package Discovery Resident Replace Reviewed Evictions v1 (2026-05-15)

**Plan ID:** `runtime-package-discovery-resident-replace-reviewed-evictions-v1`
**Status:** Active. Child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a reviewed discovery-selected resident replacement helper that discovers `.geindex` candidates, selects one exact reviewed package index path, delegates to candidate resident replacement with caller-reviewed evictions, and commits only after all projected state passes.

## Context

`runtime-resource-v2` now has reviewed `.geindex` discovery, selected candidate loading, discovery-selected resident mount/replace commits, reviewed eviction planning, eviction-assisted candidate resident mounting, discovery-selected eviction-assisted resident mounting, and eviction-assisted candidate resident replacement. The next smallest clean-break boundary is the discovery-selected replacement variant so hosts can review a discovery root plus exact selected `.geindex` and still use caller-reviewed eviction candidates without inventing automatic eviction policy or mutating live state early.

## Official Practice Check

- CMake: add one target-scoped unit-test executable in the root test registration style and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit statuses/subresults, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep orchestration in `MK_runtime` over `IFileSystem`, `RuntimeResidentPackageMountSetV2`, and `RuntimeResidentCatalogCacheV2`; do not introduce editor, SDL3, renderer/RHI, upload/staging, native handles, background workers, inferred content roots, or automatic eviction policy.

## Constraints

- Reuse `discover_runtime_package_indexes_v2` for discovery and exact selected path matching.
- Delegate resident replacement and reviewed evictions to `commit_runtime_package_candidate_resident_replace_with_reviewed_evictions_v2`; do not duplicate package loading, replacement, eviction planning, or cache-refresh logic.
- Validate `selected_package_index_path` and the existing `RuntimeResidentPackageMountIdV2` before discovery or package reads.
- Preserve live mount/cache state on invalid descriptors, invalid/missing mount ids, discovery failure, missing selected candidate, candidate load/read failure, invalid/duplicate/missing/protected eviction candidates, insufficient reviewed eviction candidates, budget failure, or catalog refresh failure.
- Return discovery, selected candidate, delegated replacement, eviction plan, catalog refresh, byte counts, evicted mount count, mount generation/count, diagnostics, and `committed`/invocation flags.
- Do not claim package-streaming integration, arbitrary/LRU eviction policy, allocator/GPU budget enforcement, broad/background package streaming, renderer/RHI ownership, upload/staging package integration, hot reload productization, inferred content roots, native handles, or 2D/3D vertical-slice readiness.

## Tasks

- [ ] Add RED tests and a `MK_runtime_package_discovery_resident_replace_reviewed_evictions_tests` target for successful discovery-selected replacement with reviewed eviction, no-eviction replacement, invalid selected path before reads, invalid/missing mount id before reads, discovery failure rollback, selected candidate not found, delegated candidate load rollback, delegated protected/invalid/duplicate/missing eviction rollback, and insufficient reviewed eviction rollback.
- [ ] Add public `RuntimePackageDiscoveryResidentReplaceReviewedEvictions*V2` value contracts and `commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2`.
- [ ] Implement descriptor validation, existing mount-id preflight, discovery, exact candidate selection, delegated `commit_runtime_package_candidate_resident_replace_with_reviewed_evictions_v2` execution, diagnostic/status mapping, evidence propagation, and rollback-preserving commit.
- [ ] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [ ] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A caller can provide a reviewed discovery root, exact selected `.geindex` path, existing resident mount id, target budget, and reviewed eviction candidates, then commit the selected replacement after unmounting only the planned ids needed for the target budget.
- Invalid selected package paths and invalid/missing replacement mount ids fail before discovery or package reads.
- Discovery failure, missing selected candidate, candidate load/read failure, eviction-plan failure, budget failure, and catalog refresh failure leave live mount/cache state unchanged.
- Result evidence includes discovery, selected candidate, delegated candidate resident replacement, eviction plan, catalog refresh, estimated bytes, evicted mount count, mount generation, resident package count, diagnostics, and invocation/commit flags.
- Current-truth docs and `engine/agent/manifest.json` describe this as reviewed discovery-selected replacement with reviewed evictions only, with package-streaming/hot reload/RHI/upload/arbitrary-eviction/GPU-budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_discovery_resident_replace_reviewed_evictions_tests'` | Pending | RED first, then GREEN after implementation. |
| `ctest --preset dev --output-on-failure -R MK_runtime_package_discovery_resident_replace_reviewed_evictions_tests` | Pending | Focused discovery-selected eviction-assisted resident replacement suite. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pending | Public runtime API surface will change. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pending | Formatting guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_discovery_resident_replace_reviewed_evictions_tests.cpp` | Pending | Focused clang-tidy guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pending | Manifest JSON contract and composed manifest checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pending | Agent-surface parity and text-format checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pending | AI integration guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pending | Full repository validation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pending | Standalone commit-preflight build. |
