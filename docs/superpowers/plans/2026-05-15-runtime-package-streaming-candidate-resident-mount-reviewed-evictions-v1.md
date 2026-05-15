# Runtime Package Streaming Candidate Resident Mount Reviewed Evictions v1 (2026-05-15)

**Plan ID:** `runtime-package-streaming-candidate-resident-mount-reviewed-evictions-v1`
**Status:** Active. Current child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a selected package-streaming descriptor bridge that loads the descriptor-selected `.geindex` candidate, validates selected residency hints, delegates to reviewed candidate resident mounting with caller-reviewed evictions, and commits only after all projected state passes.

## Context

`runtime-resource-v2` now has reviewed candidate and discovery-selected resident mount/replacement with caller-reviewed evictions, plus selected package-streaming candidate resident mount/replacement without eviction assistance. The next smallest clean-break boundary is the package-streaming candidate resident mount variant with reviewed evictions so host-gated package-streaming descriptors can reuse the reviewed eviction planner without inventing automatic eviction policy or broad background streaming.

## Official Practice Check

- CMake: add or update the focused package-streaming unit-test target and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit statuses/subresults, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep orchestration in `MK_runtime` over `IFileSystem`, `RuntimePackageStreamingExecutionDesc`, `RuntimeResidentPackageMountSetV2`, and `RuntimeResidentCatalogCacheV2`; do not introduce editor, SDL3, renderer/RHI, upload/staging, native handles, background workers, inferred content roots, or automatic eviction policy.

## Constraints

- Reuse `make_package_streaming_candidate`, selected descriptor validation, runtime-scene validation preflight, residency hint validation, and `commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2`.
- Validate `RuntimePackageStreamingExecutionDesc`, runtime scene validation preflight, new mount id, and selected residency hints before mutation.
- Apply only caller-reviewed eviction candidates; automatically protect the newly mounted package through the delegated helper.
- Preserve live mount/cache state on invalid descriptors, missing validation preflight, invalid/duplicate mount ids, candidate load/read failure, residency hint failure, invalid/duplicate/missing/protected eviction candidates, insufficient reviewed eviction candidates, budget failure, or catalog refresh failure.
- Return streaming result evidence with candidate load, resident mount, eviction plan, resident catalog refresh, resident byte/count/generation counters, diagnostics, and committed state without exposing renderer/RHI/native handles.
- Do not claim discovery orchestration, replacement mode, arbitrary/LRU eviction policy, allocator/GPU budget enforcement, broad/background package streaming, renderer/RHI ownership, upload/staging package integration, hot reload productization, inferred content roots, native handles, or 2D/3D vertical-slice readiness.

## Tasks

- [ ] Add RED tests for selected package-streaming candidate resident mount with reviewed eviction, no-eviction mount, invalid descriptor/preflight/mount-id rollback, candidate load rollback, residency hint rollback, invalid/duplicate/missing/protected eviction candidate rollback, and insufficient reviewed eviction rollback.
- [ ] Add the public selected package-streaming reviewed-eviction candidate resident mount overload/result evidence needed by the tests.
- [ ] Implement descriptor validation, selected candidate load, residency hint preflight, delegated `commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2` execution, diagnostic/status mapping, evidence propagation, and rollback preservation.
- [ ] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [ ] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A host-gated package-streaming descriptor can mount its selected candidate and satisfy the resident budget by unmounting only caller-reviewed candidate ids.
- Invalid descriptors, missing validation preflight, invalid/duplicate mount ids, candidate load/read failure, residency hint failure, eviction-plan failure, budget failure, and catalog refresh failure leave live mount/cache state unchanged.
- Result evidence includes candidate load, delegated reviewed-eviction resident mount, eviction plan, catalog refresh, estimated bytes, resident package count, mount generation, diagnostics, and committed state.
- Current-truth docs and `engine/agent/manifest.json` describe this as selected package-streaming candidate resident mounting with reviewed evictions only, with replacement-mode streaming eviction assistance, broad/background streaming, hot reload/RHI/upload/arbitrary-eviction/GPU-budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests'` | Pending | RED first, then GREEN after implementation. |
| `ctest --preset dev --output-on-failure -R MK_runtime_package_streaming_resident_mount_tests` | Pending | Focused selected package-streaming resident mount suite. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pending | Public runtime API surface may change. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pending | Formatting guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/package_streaming.cpp,tests/unit/runtime_package_streaming_resident_mount_tests.cpp` | Pending | Focused clang-tidy guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pending | Manifest JSON contract and composed manifest checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pending | Agent-surface parity and text-format checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pending | AI integration guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pending | Full repository validation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pending | Standalone commit-preflight build. |
