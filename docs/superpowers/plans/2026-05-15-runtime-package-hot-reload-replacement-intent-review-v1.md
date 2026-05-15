# Runtime Package Hot Reload Replacement Intent Review v1 (2026-05-15)

**Plan ID:** `runtime-package-hot-reload-replacement-intent-review-v1`
**Status:** Completed. Returned `currentActivePlan` to `production-completion-master-plan-v1` with `recommendedNextPlan.id=next-production-gap-selection`.
**Goal:** Add a host-independent reviewed hot-reload replacement intent planner that validates one reviewed candidate-review row, explicit existing mount id, and optional reviewed eviction ids into a safe-point replacement descriptor without file watching, recook execution, package reads, or resident mutation.

## Context

`runtime-resource-v2` now has reviewed package discovery, hot-reload change-candidate review planning, candidate load, resident mount/replacement commits, optional reviewed evictions, standalone reviewed eviction commits, and a reviewed replacement safe point through `commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2`. The remaining gap between candidate review and execution is a deterministic handoff row: hosts should be able to validate the operator-selected candidate row and replacement intent before calling the safe-point commit helper.

## Official Practice Check

- CMake: add focused runtime hot-reload replacement intent review tests and run focused CMake/CTest loops before the full validation gate.
- C++/API: follow `docs/cpp-style.md`, C++23 value-result contracts, explicit statuses/diagnostics, no compatibility aliases, and no broad ready claims.
- Runtime boundary: keep orchestration in `MK_runtime` over reviewed candidate rows, mount ids, budgets, overlay policy, and reviewed eviction ids; do not add file watchers, recook execution, package reads, background workers, editor code, SDL3, renderer/RHI, upload/staging, native handles, inferred content roots, or automatic eviction policy.

## Constraints

- Reuse `RuntimePackageHotReloadCandidateReviewRowV2` and `RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDescV2`; do not duplicate replacement commit APIs.
- Validate that the selected candidate review row has at least one matched change and a valid `.geindex` candidate.
- Validate that the selected candidate package index stays under the reviewed discovery root and that normalized candidate/discovery content roots match.
- Validate explicit existing `RuntimeResidentPackageMountIdV2` before any later commit helper can read packages.
- Reject unknown overlay enum values instead of allowing downstream merge behavior to reinterpret them.
- Preserve caller-reviewed eviction order and protected mount ids exactly; do not infer LRU or automatic eviction policy.
- Return typed review diagnostics and no-commit evidence only.

## Tasks

- [x] Add RED tests for valid replacement-intent descriptor planning, invalid selected candidate rows, invalid/missing mount ids, reviewed eviction passthrough, and no package-read/no-mutation evidence.
- [x] Add the public hot-reload replacement intent review descriptor/result/status needed by the tests.
- [x] Implement deterministic reviewed intent to safe-point descriptor mapping with diagnostics.
- [x] Update runtime docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A host can validate one reviewed hot-reload candidate row plus explicit replacement intent before calling the existing reviewed replacement safe point.
- Invalid selected candidates, candidate/discovery mismatches, invalid overlay modes, invalid mount ids, and unsafe descriptor inputs are typed diagnostics only and never trigger package reads, resident mutation, file watcher ownership, recook execution, or background work.
- Current-truth docs and `engine/agent/manifest.json` describe this as intent review only, with file watching/recook execution, RHI/upload/arbitrary-eviction/GPU-budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. (Join-Path (Get-Location) "tools/common.ps1"); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_package_hot_reload_replacement_intent_review_tests'` | RED failed as expected before implementation; GREEN passed after implementation. | RED rejected the missing public status/descriptor/result/planner symbols. |
| `ctest --preset dev --output-on-failure -R MK_runtime_package_hot_reload_replacement_intent_review_tests` | Passed. | Focused hot-reload replacement intent review suite reported 1/1 passing in the GREEN loop. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. | Public runtime API boundary guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. | Formatting guard after `tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_hot_reload_replacement_intent_review_tests.cpp` | Passed. | Focused clang-tidy guard over 2 files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. | Manifest JSON contract and composed manifest checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. | Agent-surface parity and text-format checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. | AI integration guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed. | Full repository validation; diagnostic-only host gates still report Metal/Apple blockers on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed. | Standalone commit-preflight build. |
