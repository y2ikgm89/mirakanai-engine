# Runtime Package Discovery Resident Commit v1 (2026-05-15)

**Plan ID:** `runtime-package-discovery-resident-commit-v1`
**Status:** Completed. Child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a reviewed disk/VFS discovery-to-resident-commit orchestration API that discovers `.geindex` candidates, selects one exact candidate, and delegates to the existing explicit resident mount or replace commit path without adding background streaming, hot reload productization, renderer/RHI ownership, upload/staging, arbitrary eviction, allocator/GPU budget enforcement, inferred content roots, or native handles.

## Context

`runtime-resource-v2` now has reviewed package-index discovery, selected candidate loading, candidate resident mount/replace commit helpers, package-streaming resident mount/replace/unmount safe-point helpers, and descriptor-selected streaming candidate mount/replace. The remaining disk/VFS package orchestration claim still lacks one reviewed API boundary that starts from a discovery root, selects an intended candidate, validates the resident operation before package reads, and produces mount/replace evidence through the existing commit helpers.

## Official Practice Check

- CMake: add one target-scoped unit-test executable in the root test registration style and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit statuses/subresults, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep the orchestration in `MK_runtime` over `IFileSystem`; do not introduce editor, SDL3, renderer/RHI, upload/staging, native handles, background workers, or inferred filesystem roots.

## Constraints

- Reuse `discover_runtime_package_indexes_v2`, `commit_runtime_package_candidate_resident_mount_v2`, and `commit_runtime_package_candidate_resident_replace_v2` instead of duplicating package reads or catalog mutation.
- Validate operation mode, selected candidate path, and requested `RuntimeResidentPackageMountIdV2` against the live mount set before candidate package reads.
- Select exactly one discovered candidate by exact `package_index_path`; missing or duplicate discovered matches must fail before package reads and before mutation.
- Preserve the live mount set and cached catalog on discovery failure, invalid or duplicate mount ids, missing mount ids, candidate load/read failure, budget failure, or catalog refresh failure.
- Return discovery, selected candidate, delegated mount/replace result, catalog refresh evidence, byte counts, mount generation, package count, diagnostics, and `committed`/invocation flags.
- Do not claim broad/background package streaming, arbitrary/LRU eviction policy, renderer/RHI ownership, upload/staging package integration, allocator/GPU budget enforcement, hot reload productization, or 2D/3D vertical-slice readiness.

## Tasks

- [x] Add RED tests and a `MK_runtime_package_discovery_resident_commit_tests` target for successful discovery-selected mount, successful discovery-selected replace, invalid/duplicate/missing mount ids before reads, missing selected candidate before reads, discovery scan failure preservation, and delegated budget/candidate-load rollback.
- [x] Add public `RuntimePackageDiscoveryResidentCommit*V2` value contracts and `commit_runtime_package_discovery_resident_v2`.
- [x] Implement discovery, exact candidate selection, resident operation preflight, delegated mount/replace execution, diagnostic mapping, evidence propagation, and rollback-preserving failure paths.
- [x] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A caller can provide a reviewed discovery root plus exact `.geindex` path and commit either a new resident mount or an existing mount replacement through the existing copy-then-commit resident APIs.
- Invalid/duplicate/missing mount ids and missing selected candidates fail before package reads.
- Discovery failure, candidate load/read failure, budget failure, and catalog refresh failure leave live mount/cache state unchanged.
- Result evidence includes discovery, selected candidate, delegated mount/replace subresult, catalog refresh, estimated bytes, mount generation, resident package count, diagnostics, and invocation/commit flags.
- Current-truth docs and `engine/agent/manifest.json` describe this as reviewed discovery-selected resident commit orchestration only, with broad streaming/hot reload/RHI/upload/eviction/budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_discovery_resident_commit_tests'` | RED/PASS | Initial RED reproduced the missing discovery resident commit API; after implementation the focused target built. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_runtime_package_discovery_resident_commit_tests'` | PASS | Focused discovery-selected resident commit suite passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_discovery_resident_commit_tests MK_runtime_package_candidate_resident_mount_tests MK_runtime_package_candidate_resident_replace_tests MK_runtime_package_candidate_load_tests MK_runtime_package_index_discovery_tests MK_runtime_resource_resident_cache_tests MK_runtime_resource_resident_replace_tests MK_runtime_resource_resident_unmount_tests MK_runtime_package_streaming_resident_mount_tests MK_runtime_tests'` | PASS | Focused runtime package/resource regression targets built; MSBuild reported only existing intermediate-directory sharing warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_runtime_package_discovery_resident_commit_tests|MK_runtime_package_candidate_resident_mount_tests|MK_runtime_package_candidate_resident_replace_tests|MK_runtime_package_candidate_load_tests|MK_runtime_package_index_discovery_tests|MK_runtime_resource_resident_cache_tests|MK_runtime_resource_resident_replace_tests|MK_runtime_resource_resident_unmount_tests|MK_runtime_package_streaming_resident_mount_tests|MK_runtime_tests"'` | PASS | Ten focused runtime package/resource suites passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary guard passed for the new runtime header surface. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting guard passed after applying repository formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_discovery_resident_commit_tests.cpp` | PASS | Focused clang-tidy guard passed for changed C++ implementation/tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest JSON contract and composed manifest checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent-surface parity and text-format checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration guard passed with the updated runtime capability surface. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed; Metal/Apple lanes remain Windows host-gated or diagnostic-only as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Standalone commit-preflight build passed; MSBuild reported only existing intermediate-directory sharing warnings. |
