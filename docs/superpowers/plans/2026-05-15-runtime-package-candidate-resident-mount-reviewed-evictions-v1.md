# Runtime Package Candidate Resident Mount Reviewed Evictions v1 (2026-05-15)

**Plan ID:** `runtime-package-candidate-resident-mount-reviewed-evictions-v1`
**Status:** Completed. Child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a reviewed candidate resident mount helper that loads one selected `.geindex` candidate, mounts it into a projected resident view, applies only caller-reviewed resident package eviction candidates when the projected view exceeds the target budget, and commits the final mount/cache view atomically.

## Context

`runtime-resource-v2` already has reviewed resident package eviction planning, selected candidate loading, explicit candidate resident mount, and discovery-selected resident commit orchestration. The next useful boundary is a clean helper that composes candidate mount with reviewed eviction planning so callers can free resident bytes without introducing arbitrary/LRU policy or background streaming.

## Official Practice Check

- CMake: add one target-scoped unit-test executable in the root test registration style and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit statuses/subresults, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep the orchestration in `MK_runtime` over `IFileSystem`, `RuntimeResidentPackageMountSetV2`, and `RuntimeResidentCatalogCacheV2`; do not introduce editor, SDL3, renderer/RHI, upload/staging, native handles, background workers, inferred filesystem roots, or automatic eviction policy.

## Constraints

- Reuse `load_runtime_package_candidate_v2`, `plan_runtime_resident_package_evictions_v2`, `RuntimeResidentPackageMountSetV2`, and `RuntimeResidentCatalogCacheV2` instead of duplicating package parsing or eviction selection policy.
- Validate the requested `RuntimeResidentPackageMountIdV2` before candidate package reads or eviction planning.
- Protect the newly mounted candidate id from eviction, and only unmount ids explicitly listed by the caller.
- Preserve the live mount set and cached catalog on invalid or duplicate mount ids, candidate load/read failure, invalid/duplicate/missing/protected eviction candidates, insufficient reviewed eviction candidates, budget failure, or catalog refresh failure.
- Return candidate load, eviction plan, resident mount, catalog refresh, byte counts, mount generation, package count, diagnostics, and `committed`/invocation flags.
- Do not claim arbitrary/LRU eviction policy, allocator/GPU budget enforcement, broad/background package streaming, renderer/RHI ownership, upload/staging package integration, hot reload productization, or 2D/3D vertical-slice readiness.

## Tasks

- [x] Add RED tests and a `MK_runtime_package_candidate_resident_mount_reviewed_evictions_tests` target for successful reviewed eviction-assisted mount, no-eviction mount, invalid/duplicate mount ids before reads, candidate load rollback, invalid/duplicate/protected/missing eviction candidate rollback, and insufficient reviewed eviction rollback.
- [x] Add public `RuntimePackageCandidateResidentMountReviewedEvictions*V2` value contracts and `commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2`.
- [x] Implement candidate load, projected mount, reviewed eviction planning over the projected mount set, final projected catalog refresh, diagnostic mapping, evidence propagation, and rollback-preserving commit.
- [x] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A caller can provide a reviewed candidate plus explicit mount id and reviewed eviction candidate order, and the helper commits the new mount after unmounting only the planned ids needed for the target budget.
- Invalid/duplicate mount ids fail before package reads and before eviction planning.
- Candidate load/read failure and eviction-plan failure leave live mount/cache state unchanged.
- Insufficient reviewed eviction candidates fail without mutation and expose eviction-plan evidence.
- Result evidence includes candidate load, eviction plan, resident mount, catalog refresh, estimated bytes, mount generation, resident package count, diagnostics, and invocation/commit flags.
- Current-truth docs and `engine/agent/manifest.json` describe this as reviewed eviction-assisted candidate resident mount only, with broad streaming/hot reload/RHI/upload/arbitrary-eviction/GPU-budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_candidate_resident_mount_reviewed_evictions_tests'` | Passed | RED failed on missing API before implementation; GREEN passed after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_runtime_package_candidate_resident_mount_reviewed_evictions_tests'` | Passed | Focused reviewed eviction-assisted candidate resident mount suite passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_candidate_resident_mount_reviewed_evictions_tests MK_runtime_package_candidate_resident_mount_tests MK_runtime_package_candidate_resident_replace_tests MK_runtime_package_candidate_load_tests MK_runtime_package_discovery_resident_commit_tests MK_runtime_resource_resident_cache_tests MK_runtime_resource_resident_unmount_tests MK_runtime_resource_resident_replace_tests MK_runtime_package_streaming_resident_mount_tests MK_runtime_tests'` | Passed | Focused runtime package/resource regression build passed; MSBuild reported existing shared-intermediate-directory warnings for some test projects. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_runtime_package_candidate_resident_mount_reviewed_evictions_tests|MK_runtime_package_candidate_resident_mount_tests|MK_runtime_package_candidate_resident_replace_tests|MK_runtime_package_candidate_load_tests|MK_runtime_package_discovery_resident_commit_tests|MK_runtime_resource_resident_cache_tests|MK_runtime_resource_resident_unmount_tests|MK_runtime_resource_resident_replace_tests|MK_runtime_package_streaming_resident_mount_tests|MK_runtime_tests"'` | Passed | 10 focused runtime package/resource tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public runtime API surface changed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting guard passed before final evidence update. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_candidate_resident_mount_reviewed_evictions_tests.cpp` | Passed | Focused clang-tidy guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest JSON contract and composed manifest checks passed after active-plan closeout. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent-surface parity and text-format checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full repository validation passed after the invalid/duplicate eviction-candidate wrapper tests were added. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Standalone commit-preflight build passed. |
