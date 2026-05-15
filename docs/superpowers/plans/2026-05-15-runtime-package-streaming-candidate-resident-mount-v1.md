# Runtime Package Streaming Candidate Resident Mount v1 (2026-05-15)

**Plan ID:** `runtime-package-streaming-candidate-resident-mount-v1`
**Status:** Completed. Child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Connect reviewed disk/VFS `.geindex` candidate loading to the selected host-gated package-streaming resident mount safe-point surface without adding background streaming, hot reload productization, renderer/RHI ownership, upload/staging, arbitrary eviction, allocator/GPU budget enforcement, inferred content roots, or native handles.

## Context

`runtime-resource-v2` now has reviewed `.geindex` discovery, selected candidate loading, candidate resident mount/replace helpers, already-loaded package-streaming resident mount/replace/unmount safe-point overloads, and descriptor-selected candidate resident replacement. The paired narrow productization step is a streaming descriptor overload that loads the descriptor-selected candidate through `IFileSystem`, validates the same residency hints before mutation, and commits one explicit resident mount plus resident catalog cache refresh only after projected state succeeds.

## Official Practice Check

- CMake: keep the existing target-scoped unit-test executable and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit statuses/subresults, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep `MK_runtime` independent from editor, SDL3, renderer/RHI, upload/staging, native handles, and background workers.

## Constraints

- Build the candidate from `RuntimePackageStreamingExecutionDesc` (`package_index_path`, `content_root`, `target_id`) and use `load_runtime_package_candidate_v2` for selected candidate validation and package-read diagnostics.
- Validate the requested `RuntimeResidentPackageMountIdV2` before any filesystem reads so invalid or duplicate ids remain cheap, deterministic preflight failures.
- Run runtime-scene validation preflight and residency hints before live mutation.
- Commit through projected mount/catalog state so candidate load, residency hints, projected budget, and resident catalog refresh can fail without mutating the live mount set or cached catalog.
- Preserve the live mount set and cached catalog on invalid descriptor, missing runtime-scene validation, invalid/duplicate mount id, candidate load/read failure, residency hint failure, projected budget failure, or cache-refresh failure.
- Do not claim broad/background package streaming, broad disk/VFS package mount orchestration beyond reviewed candidate resident mount/replacement, arbitrary/LRU eviction policy, renderer/RHI ownership, upload/staging package integration, allocator/GPU budget enforcement, hot reload productization, or 2D/3D vertical-slice readiness.

## Tasks

- [x] Add RED tests in `tests/unit/runtime_package_streaming_resident_mount_tests.cpp` for successful selected candidate mount, invalid/duplicate mount ids before reads, candidate load failure rollback, residency hint rollback, and projected budget rollback.
- [x] Add public `execute_selected_runtime_package_streaming_candidate_resident_mount_safe_point` and return `candidate_load` evidence through `RuntimePackageStreamingExecutionResult`.
- [x] Implement descriptor-selected candidate construction, candidate-load diagnostic mapping reuse, mount-id read-preflight, residency hint checks, projected resident budget/catalog preflight, and copy-then-commit resident mount/cache refresh.
- [x] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A selected host-gated package-streaming descriptor can load its `.geindex` candidate from disk/VFS and mount one new explicit resident package while refreshing the resident catalog cache.
- Invalid or duplicate mount ids fail before any filesystem reads.
- Candidate load/read failures, residency hint failures, projected budget failures, and cache-refresh failures leave the live mount set and cached catalog unchanged.
- `RuntimePackageStreamingExecutionResult` reports `candidate_load`, `resident_mount`, `resident_catalog_refresh`, estimated bytes, resident mount generation, package count, and diagnostics.
- Current-truth docs and `engine/agent/manifest.json` describe this as selected candidate streaming resident mount only, with broader streaming/hot reload/RHI/upload/eviction/budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests'` | RED/PASS | Initial RED reproduced the missing `execute_selected_runtime_package_streaming_candidate_resident_mount_safe_point`; after implementation the focused target built. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_runtime_package_streaming_resident_mount_tests'` | PASS | Focused package-streaming resident mount/replace/unmount suite passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests MK_runtime_package_candidate_resident_mount_tests MK_runtime_package_candidate_resident_replace_tests MK_runtime_package_candidate_load_tests MK_runtime_resource_resident_cache_tests MK_runtime_resource_resident_replace_tests MK_runtime_tests'` | PASS | Focused runtime package/resource regression targets built; MSBuild reported only existing intermediate-directory sharing warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_runtime_package_streaming_resident_mount_tests|MK_runtime_package_candidate_resident_mount_tests|MK_runtime_package_candidate_resident_replace_tests|MK_runtime_package_candidate_load_tests|MK_runtime_resource_resident_cache_tests|MK_runtime_resource_resident_replace_tests|MK_runtime_tests"'` | PASS | Seven focused runtime package/resource suites passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary guard passed for the new runtime header surface. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting guard passed after applying repository formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/package_streaming.cpp,tests/unit/runtime_package_streaming_resident_mount_tests.cpp` | PASS | Focused clang-tidy guard passed for changed C++ implementation/tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest JSON contract and composed manifest checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent-surface parity and text-format checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration guard passed with the updated runtime capability surface. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed; Metal/Apple lanes remain Windows host-gated or diagnostic-only as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Standalone commit-preflight build passed; MSBuild reported only existing intermediate-directory sharing warnings. |
