# Runtime Package Discovery Resident Mount Reviewed Evictions v1 (2026-05-15)

**Plan ID:** `runtime-package-discovery-resident-mount-reviewed-evictions-v1`
**Status:** Completed. Child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a reviewed discovery-selected resident mount helper that discovers `.geindex` candidates, selects one exact candidate path, mounts it into a projected resident view, applies only caller-reviewed eviction candidates when needed, and commits the final mount/cache view atomically.

## Context

`runtime-resource-v2` now has reviewed `.geindex` discovery, selected candidate loading, explicit candidate resident mounting, reviewed resident eviction planning, and eviction-assisted candidate resident mounting. The next narrow boundary is a discovery-selected helper that composes those pieces so callers can choose a package by reviewed discovery path and free resident bytes without adding broad disk/VFS orchestration, automatic eviction policy, background streaming, or renderer/RHI ownership.

## Official Practice Check

- CMake: add one target-scoped unit-test executable in the root test registration style and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit statuses/subresults, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep orchestration in `MK_runtime` over `IFileSystem`, `RuntimeResidentPackageMountSetV2`, and `RuntimeResidentCatalogCacheV2`; do not introduce editor, SDL3, renderer/RHI, upload/staging, native handles, background workers, inferred content roots, or automatic eviction policy.

## Constraints

- Reuse `discover_runtime_package_indexes_v2` and `commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2` instead of duplicating package discovery, parsing, resident mount, or eviction planning behavior.
- Validate the selected `.geindex` path and requested `RuntimeResidentPackageMountIdV2` before discovery scans or package reads.
- Select only an exact discovered `RuntimePackageIndexDiscoveryCandidateV2::package_index_path`.
- Preserve live mount/cache state on invalid descriptors, invalid/duplicate mount ids, discovery failures, missing selected candidates, candidate load/read failure, invalid/duplicate/missing/protected eviction candidates, insufficient reviewed eviction candidates, budget failure, or catalog refresh failure.
- Return discovery, selected candidate, delegated eviction-assisted mount result, eviction plan, catalog refresh, byte counts, evicted mount count, mount generation, package count, diagnostics, and `committed`/invocation flags.
- Do not claim replace-mode eviction assistance, arbitrary/LRU eviction policy, allocator/GPU budget enforcement, broad/background package streaming, renderer/RHI ownership, upload/staging package integration, hot reload productization, or 2D/3D vertical-slice readiness.

## Tasks

- [x] Add RED tests and a `MK_runtime_package_discovery_resident_mount_reviewed_evictions_tests` target for successful discovery-selected eviction-assisted mount, no-eviction mount, invalid selected path/mount id before scans or reads, missing selected candidate before package reads, discovery scan failure rollback, delegated candidate load rollback, delegated invalid/duplicate/missing/protected eviction candidate rollback, and insufficient reviewed eviction rollback.
- [x] Add public `RuntimePackageDiscoveryResidentMountReviewedEvictions*V2` value contracts and `commit_runtime_package_discovery_resident_mount_with_reviewed_evictions_v2`.
- [x] Implement discovery, exact candidate selection, delegated reviewed eviction-assisted candidate mount, diagnostic/status mapping, evidence propagation, and rollback-preserving commit.
- [x] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A caller can discover reviewed `.geindex` candidates, select one exact path, provide an explicit new mount id and reviewed eviction candidate order, and commit the selected package after unmounting only the delegated planned ids needed for the target budget.
- Invalid selected package paths and invalid/duplicate mount ids fail before discovery scans, package reads, or delegated resident commits.
- Missing selected candidate, discovery failure, candidate load/read failure, eviction-plan failure, budget failure, and catalog refresh failure leave live mount/cache state unchanged.
- Result evidence includes discovery, selected candidate, delegated eviction-assisted mount, eviction plan, catalog refresh, estimated bytes, evicted mount count, mount generation, resident package count, diagnostics, and invocation/commit flags.
- Current-truth docs and `engine/agent/manifest.json` describe this as reviewed discovery-selected eviction-assisted resident mount only, with broad streaming/hot reload/RHI/upload/arbitrary-eviction/GPU-budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_discovery_resident_mount_reviewed_evictions_tests'` | Passed | RED failed before implementation on the missing public contracts; GREEN passed after implementation and again after reviewer test tightening. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_runtime_package_discovery_resident_mount_reviewed_evictions_tests'` | Passed | Focused discovery-selected reviewed eviction-assisted resident mount suite. |
| Focused runtime package/resource regression build and CTest set | Passed | Existing resident package discovery, candidate load/mount/replace, streaming candidate, resident unmount/replace, and runtime resource tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public runtime API surface changed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_discovery_resident_mount_reviewed_evictions_tests.cpp` | Passed | Focused clang-tidy guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest JSON contract and composed manifest checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent-surface parity and text-format checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full repository validation; Apple/Metal host-gated diagnostics remain expected on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Standalone commit-preflight build. |
