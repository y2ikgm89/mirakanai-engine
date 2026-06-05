# MAVG Runtime-Inferred LRU Eviction Policy v1 Implementation Plan

**Date:** 2026-06-06

**Plan ID:** `mavg-runtime-inferred-lru-eviction-policy-v1`

**Status:** Completed candidate published as stacked draft PR #481.

## Goal

Add a value-only runtime-inferred LRU eviction policy that derives resident-page last-use generations from current selected resident clusters and orders unprotected eviction candidates through the existing reviewed automatic eviction planner.

## Context

MAVG Runtime-Inferred Page Use Generation v1 added `infer_runtime_mavg_resident_page_use_generations`, which produces reviewed `RuntimeMavgPageStreamingRecencyRow` evidence from selected resident pages. This child makes that evidence directly selectable inside `plan_runtime_mavg_page_streaming_automatic_evictions` through an explicit `runtime_inferred_lru` policy while preserving selected/fallback ancestor protection, fail-closed diagnostics, and value-only side-effect boundaries.

Context7 `/websites/cppreference` was checked for the C++23 API shape: `std::span` remains the borrowed input view for caller-owned selected, resident, and previous-recency rows, while `std::vector` remains the owning result row storage.

## Constraints

- Keep the policy value-only: no file IO, live mount mutation, renderer/RHI handle access, DirectStorage execution, or background worker ownership.
- Reuse existing selected visible/fallback ancestor protection before ordering eviction candidates.
- Use `infer_runtime_mavg_resident_page_use_generations` as the only runtime inference source.
- Touch only selected resident page generations; fallback/protected ancestors remain protected but are not refreshed unless selected.
- Sort runtime-inferred LRU candidates by older `resident_page_last_used_generation` first, then existing deterministic page-index and mount-id tie-breakers.
- Fail closed before eviction planning if selected rows, resident mount rows, previous recency rows, or monotonic generation evidence are invalid.
- Claim only runtime-inferred LRU eviction candidate ordering. Do not claim frequency policy, GPU memory pressure integration, DirectStorage native queue/file IO, async-overlap/performance, benchmark superiority, Vulkan/Metal parity, mesh shaders, deformation, ray tracing, or Nanite compatibility/equivalence/superiority.

## Implementation

- [x] Add `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru`.
- [x] Extend `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc` with `previous_recency_rows` and `current_use_generation`.
- [x] Extend `RuntimeMavgPageStreamingEvictionReviewResult` with `inferred_eviction_policy`, `inferred_lru_eviction_policy`, `runtime_inferred_lru_eviction_candidate_count`, and use-generation evidence counters.
- [x] Route `runtime_inferred_lru` through `infer_runtime_mavg_resident_page_use_generations`.
- [x] Reuse existing recency ordering for unprotected candidates only.
- [x] Preserve no-file-IO, no-live-mount-mutation, and no-renderer/RHI-handle evidence flags.
- [x] Add focused unit tests for oldest-first ordering, cold-page ordering, and non-monotonic fail-closed behavior.
- [x] Update docs, plan registry, manifest fragments, composed manifest, and static checks.
- [x] Run focused and full validation.
- [x] Publish as stacked draft PR #481 over `codex/mavg-runtime-inferred-page-use-generation-v1`.

## Validation Evidence

- Baseline: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed before this candidate's test edits.
- Baseline: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1 before this candidate's test edits.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` failed before implementation because `runtime_inferred_lru`, `previous_recency_rows`, `current_use_generation`, and the LRU evidence result fields were not declared.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed after implementation.
- GREEN focused CTest: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1 after implementation.
- GREEN targeted tidy/API/static checks: `tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply`, `tools/check-public-api-boundaries.ps1`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, and `git diff --check` passed.
- GREEN full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` ended with `validate: ok` and CTest `109/109` passing on this Windows worktree.
- Apple/Metal lanes remain host-gated in the existing diagnostic checks because this host is Windows and lacks Xcode/Metal tools.

## Done When

- Public runtime API exposes `runtime_inferred_lru` and the required previous-recency/current-generation inputs.
- Tests prove runtime-inferred LRU orders older unprotected resident pages first, initializes cold resident pages as oldest, and rejects non-monotonic retained generations before eviction planning.
- Docs, plan registry, manifest fragments, composed manifest, and static checks describe the implemented LRU scope and remaining non-claims.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No runtime-inferred frequency eviction policy.
- No GPU memory pressure integration or allocator enforcement.
- No DirectStorage native queue/status/fence/file IO execution.
- No sustained async-overlap or performance benchmark claim.
- No background streaming service readiness beyond existing joined worker evidence.
- No renderer/RHI ownership or native handle exposure.
- No mesh shader, deformation, ray tracing, Metal/Vulkan parity, Nanite compatibility/equivalence/superiority, or benchmark-exceeds claim.
