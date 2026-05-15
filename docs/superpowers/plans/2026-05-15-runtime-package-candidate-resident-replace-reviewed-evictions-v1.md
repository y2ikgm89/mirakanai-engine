# Runtime Package Candidate Resident Replace Reviewed Evictions v1 (2026-05-15)

**Plan ID:** `runtime-package-candidate-resident-replace-reviewed-evictions-v1`
**Status:** Completed. Child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a reviewed candidate resident replacement helper that loads one selected `.geindex` candidate, replaces an existing resident mount in a projected view, applies only caller-reviewed eviction candidates when the replacement exceeds the target resident budget, and commits the final mount/cache view atomically.

## Context

`runtime-resource-v2` now has reviewed candidate loading, explicit candidate resident replacement, reviewed eviction planning, and eviction-assisted candidate resident mounting. The next smallest clean-break boundary is candidate replacement with reviewed evictions, so future discovery-selected or package-streaming replacement surfaces can compose it without inventing eviction policy or mutating live state before all preflight succeeds.

## Official Practice Check

- CMake: add one target-scoped unit-test executable in the root test registration style and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit statuses/subresults, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep orchestration in `MK_runtime` over `IFileSystem`, `RuntimeResidentPackageMountSetV2`, and `RuntimeResidentCatalogCacheV2`; do not introduce editor, SDL3, renderer/RHI, upload/staging, native handles, background workers, inferred content roots, or automatic eviction policy.

## Constraints

- Reuse `load_runtime_package_candidate_v2`, the resident replacement projection path, `plan_runtime_resident_package_evictions_v2`, and `RuntimeResidentCatalogCacheV2` refresh semantics instead of duplicating package parsing, catalog building, or eviction validation.
- Validate the existing `RuntimeResidentPackageMountIdV2` before filesystem reads.
- Preserve the replaced mount slot/order/label when the replacement succeeds.
- Automatically protect the replacement mount id from the reviewed eviction plan.
- Preserve live mount/cache state on invalid/missing mount ids, candidate load/read failure, invalid/duplicate/missing/protected eviction candidates, insufficient reviewed eviction candidates, budget failure, or catalog refresh failure.
- Return candidate load, resident replacement, eviction plan, catalog refresh, byte counts, evicted mount count, mount generation/count, diagnostics, and `committed`/invocation flags.
- Do not claim discovery orchestration, package-streaming integration, arbitrary/LRU eviction policy, allocator/GPU budget enforcement, broad/background package streaming, renderer/RHI ownership, upload/staging package integration, hot reload productization, or 2D/3D vertical-slice readiness.

## Tasks

- [x] Add RED tests and a `MK_runtime_package_candidate_resident_replace_reviewed_evictions_tests` target for successful replacement with reviewed eviction, no-eviction replacement, invalid/missing mount id before reads, candidate load rollback, invalid/duplicate/missing/protected eviction candidate rollback, replacement mount id protection, and insufficient reviewed eviction rollback.
- [x] Add public `RuntimePackageCandidateResidentReplaceReviewedEvictions*V2` value contracts and `commit_runtime_package_candidate_resident_replace_with_reviewed_evictions_v2`.
- [x] Implement selected candidate load, projected slot-preserving replacement, reviewed eviction planning over the projected view, final catalog refresh, diagnostic/status mapping, evidence propagation, and rollback-preserving commit.
- [x] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A caller can load one reviewed selected `.geindex` candidate, replace an existing resident mount, provide reviewed eviction candidates, and commit the selected replacement after unmounting only the planned ids needed for the target budget.
- Invalid or missing replacement mount ids fail before filesystem reads or delegated resident commits.
- The replacement mount id is protected from eviction, and caller-protected mount ids stay honored.
- Candidate load/read failure, eviction-plan failure, budget failure, and catalog refresh failure leave live mount/cache state unchanged.
- Result evidence includes candidate load, resident replacement, eviction plan, catalog refresh, estimated bytes, evicted mount count, mount generation, resident package count, diagnostics, and invocation/commit flags.
- Current-truth docs and `engine/agent/manifest.json` describe this as reviewed candidate replacement with reviewed evictions only, with discovery/package-streaming/hot reload/RHI/upload/arbitrary-eviction/GPU-budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | PASS | Local CMake/CTest/clang-format/MSVC/vcpkg preflight ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_candidate_resident_replace_reviewed_evictions_tests'` | RED/PASS | Direct `cmake --build` first hit the parent `Path`/`PATH` MSBuild issue; the repository wrapper reproduced the intended RED on missing `RuntimePackageCandidateResidentReplaceReviewedEvictions*V2` API, then passed after implementation. |
| `ctest --preset dev --output-on-failure -R MK_runtime_package_candidate_resident_replace_reviewed_evictions_tests` | PASS | Focused reviewed eviction-assisted candidate resident replacement suite passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_candidate_resident_replace_tests MK_runtime_package_candidate_resident_mount_reviewed_evictions_tests MK_runtime_package_discovery_resident_mount_reviewed_evictions_tests'` | PASS | Adjacent candidate replace, reviewed-eviction candidate mount, and discovery-reviewed mount targets built. |
| `ctest --preset dev --output-on-failure -R "MK_runtime_package_(candidate_resident_replace_tests\|candidate_resident_replace_reviewed_evictions_tests\|candidate_resident_mount_reviewed_evictions_tests\|discovery_resident_mount_reviewed_evictions_tests)"` | PASS | Four adjacent runtime package suites passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public runtime API surface changed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting guard passed after targeted clang-format. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_candidate_resident_replace_reviewed_evictions_tests.cpp` | PASS | Focused clang-tidy guard passed for runtime implementation and new test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest JSON contract and composed manifest checks passed after keeping the next child plan `Planned` until the active slice closes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent-surface parity and text-format checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed; Windows host-gated Apple/Metal diagnostics remained diagnostic-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Standalone commit-preflight build passed after full validation. |
