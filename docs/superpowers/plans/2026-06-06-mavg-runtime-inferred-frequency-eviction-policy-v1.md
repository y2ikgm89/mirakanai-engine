# MAVG Runtime-Inferred Frequency Eviction Policy v1 Implementation Plan

**Date:** 2026-06-06

**Plan ID:** `mavg-runtime-inferred-frequency-eviction-policy-v1`

**Status:** Validated; publication pending.

## Goal

Add a value-only runtime-inferred frequency eviction policy that derives resident-page selection counts from current selected resident clusters and orders unprotected eviction candidates by least-selected resident pages first.

## Context

MAVG Runtime-Inferred LRU Eviction Policy v1 added an explicit `runtime_inferred_lru` policy over inferred last-use generation evidence. This child adds the adjacent LFU-style frequency policy while keeping it in the same reviewed planning boundary: caller-owned previous frequency rows in, deterministic value rows out, reviewed automatic eviction plan out.

Context7 `/websites/cppreference` was checked for the C++23 API shape: `std::span` remains the borrowed input view for caller-owned selected, resident, and previous-frequency rows, `std::vector` remains the owning result row storage, and `std::ranges::sort` remains the deterministic in-place ordering mechanism with explicit tie-breakers.

## Constraints

- Keep the policy value-only: no file IO, live mount mutation, renderer/RHI handle access, DirectStorage execution, GPU memory pressure integration, or background worker ownership.
- Reuse existing selected visible/fallback ancestor protection before ordering eviction candidates.
- Add one frequency sample per selected resident page, not per selected cluster, so duplicated selected clusters cannot inflate page frequency.
- Carry previous matching frequencies for unselected resident pages, initialize new unselected resident pages to `0`, and initialize new selected resident pages to `1`.
- Sort runtime-inferred frequency candidates by lower `resident_page_selection_count` first, then existing deterministic page-index and mount-id tie-breakers.
- Fail closed before eviction planning if selected rows, resident mount rows, previous frequency rows, or counter overflow evidence are invalid.
- Claim only runtime-inferred frequency eviction candidate ordering. Do not claim GPU memory pressure integration, DirectStorage native queue/file IO, async-overlap/performance, benchmark superiority, Vulkan/Metal parity, mesh shaders, deformation, ray tracing, or Nanite compatibility/equivalence/superiority.

## Implementation

- [x] Add `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency`.
- [x] Add `RuntimeMavgPageStreamingFrequencyRow`.
- [x] Add `RuntimeMavgResidentPageFrequencyDesc` and `RuntimeMavgResidentPageFrequencyResult`.
- [x] Extend `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc` with `previous_frequency_rows`.
- [x] Extend `RuntimeMavgPageStreamingEvictionReviewResult` with `inferred_frequency_eviction_policy`, `runtime_inferred_frequency_eviction_candidate_count`, `inferred_resident_page_frequency`, `frequency_counter_overflow`, and frequency evidence counters.
- [x] Add `infer_runtime_mavg_resident_page_frequencies`.
- [x] Route `runtime_inferred_frequency` through the frequency inference helper.
- [x] Add focused unit tests for least-selected ordering, cold-page ordering, and counter-overflow fail-closed behavior.
- [x] Update docs, plan registry, manifest fragments, composed manifest, and static checks.
- [x] Run focused and full validation.
- [ ] Publish as a stacked draft PR over `codex/mavg-runtime-inferred-lru-eviction-policy-v1`.

## Validation Evidence

- Baseline: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed before this candidate's test edits.
- Baseline: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1 before this candidate's test edits.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` failed after adding frequency tests because the new frequency public API, policy kind, previous-frequency rows, and counters did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed after implementing `runtime_inferred_frequency`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1 after implementing `runtime_inferred_frequency`.
- Static/API: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Static/API: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- Static/API: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply` passed.
- Static/API: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Static/API: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed.
- Format: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- Format: `git diff --check` passed.
- Slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with CTest 109/109; Metal/Apple checks remain diagnostic host-gated on this Windows host.

## Done When

- Public runtime API exposes `runtime_inferred_frequency` and the required previous-frequency input rows.
- Tests prove runtime-inferred frequency orders lower-count unprotected resident pages first, initializes cold resident pages correctly, and rejects counter overflow before eviction planning.
- Docs, plan registry, manifest fragments, composed manifest, and static checks describe the implemented frequency scope and remaining non-claims.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No GPU memory pressure integration or allocator enforcement.
- No DirectStorage native queue/status/fence/file IO execution.
- No sustained async-overlap or performance benchmark claim.
- No background streaming service readiness beyond existing joined worker evidence.
- No renderer/RHI ownership or native handle exposure.
- No mesh shader, deformation, ray tracing, Metal/Vulkan parity, Nanite compatibility/equivalence/superiority, or benchmark-exceeds claim.
