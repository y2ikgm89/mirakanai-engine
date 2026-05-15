# Runtime Package Hot Reload Change Candidate Review v1 (2026-05-15)

**Plan ID:** `runtime-package-hot-reload-change-candidate-review-v1`
**Status:** Completed. Current child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a host-independent reviewed hot-reload change-candidate planner that turns caller-reviewed changed package/index/content paths into exact `.geindex` replacement candidate review rows without file watching, recook execution, background workers, or live mutation.

## Context

`runtime-resource-v2` now has reviewed package discovery, candidate load, resident mount/replacement commits, optional reviewed evictions, standalone reviewed eviction commits, and a host-driven reviewed hot-reload replacement safe point through `commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2`. The next smallest boundary is choosing which discovered candidate should be reviewed after a host observes a package/index/content path change. This should remain a deterministic planner: hosts still own file watcher events, recook/build steps, UI review, and the later safe-point commit.

## Official Practice Check

- CMake: add focused runtime hot-reload change-candidate review tests and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit statuses/diagnostics, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep orchestration in `MK_runtime` over reviewed path strings and discovered `RuntimePackageIndexDiscoveryCandidateV2` rows; do not add file watchers, recook execution, background workers, editor code, SDL3, renderer/RHI, upload/staging, native handles, inferred content roots, or automatic eviction policy.

## Constraints

- Reuse `discover_runtime_package_indexes_v2` candidate rows; do not read package payloads or mutate resident state.
- Accept only caller-reviewed changed paths and exact discovered candidate rows.
- Treat changed `.geindex` paths and payload paths under a candidate content root as review signals; reject unsafe relative paths deterministically.
- Return deterministic candidate-review rows, diagnostics, matched counts, and no-commit evidence.
- Do not claim automatic file watching, live recook, broad/background package streaming, renderer/RHI ownership, upload/staging package integration, allocator/GPU budget enforcement, inferred content roots, native handles, or 2D/3D vertical-slice readiness.

## Tasks

- [x] Add RED tests for changed `.geindex` candidate matching, changed payload path candidate matching, invalid path diagnostics, unmatched paths, duplicate candidate de-duplication, and no filesystem/package reads.
- [x] Add the public hot-reload change-candidate review descriptor/result/status needed by the tests.
- [x] Implement deterministic reviewed changed-path to discovered-candidate matching with diagnostics and stable ordering.
- [x] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A host can turn reviewed package/index/content path changes into exact discovered candidate rows for later hot-reload replacement review.
- Invalid or unmatched paths are typed diagnostics only and never trigger package reads, resident mutation, file watcher ownership, recook execution, or background work.
- Current-truth docs and `engine/agent/manifest.json` describe this as review planning only, with file watching/recook execution, RHI/upload/arbitrary-eviction/GPU-budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --preset dev; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_hot_reload_candidate_review_tests'` | PASS (RED expected failure before implementation) | The first focused build failed on missing public hot-reload candidate review API/types, proving the new tests were red before implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_hot_reload_candidate_review_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_runtime_package_hot_reload_candidate_review_tests'` | PASS | Focused hot-reload change-candidate review target built and the CTest filter passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public runtime API boundary check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_hot_reload_candidate_review_tests.cpp` | PASS | Focused clang-tidy guard passed for the runtime implementation and focused test file. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest compose and JSON/static contract checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent-surface parity and text-format checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed; CTest reported 61/61 tests passed, with Metal/Apple checks remaining diagnostic/host-gated as expected on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Standalone commit-preflight build passed. |
