# MAVG Resident Page Recency Eviction Order v1 Implementation Plan

**Date:** 2026-06-06

**Plan ID:** `mavg-resident-page-recency-eviction-order-v1`

**Status:** Completed candidate pending publication.

## Goal

Add a deterministic, caller-supplied recency ordering mode to MAVG automatic page eviction planning so a host can pass reviewed resident-page last-use evidence without making the runtime infer LRU or frequency behavior.

## Context

MAVG Automatic Eviction Policy v1 already produces a deterministic reviewed eviction candidate order from resident page mounts while protecting selected visible pages and fallback ancestors. That order is intentionally not a real LRU policy. This child adds a narrower policy mode: callers may supply one `RuntimeMavgPageStreamingRecencyRow` per unprotected eviction candidate, and the planner orders older `resident_page_last_used_generation` values first.

## Constraints

- Keep this as pure planning until existing safe-point execution runs.
- Require caller-supplied recency rows to match the same graph asset, page index, and resident mount id as existing resident page mount rows.
- Fail closed for invalid, duplicate, mismatched, or missing recency rows before invoking eviction planning.
- Use deterministic tie-breaks after recency: descending page index, then ascending mount id.
- Do not add a runtime-inferred LRU/frequency or GPU memory pressure policy.
- Do not execute DirectStorage native queues/file IO, mutate live mounts, touch renderer/RHI handles, or claim async-overlap/performance.
- Do not claim Nanite compatibility/equivalence/superiority or benchmark superiority.

## Implementation

- [x] Add `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency`.
- [x] Add `RuntimeMavgPageStreamingRecencyRow` with `resident_page_last_used_generation`.
- [x] Extend `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc` with `policy_kind` and `recency_rows`.
- [x] Extend `RuntimeMavgPageStreamingEvictionReviewResult` with `applied_caller_supplied_recency_policy`, `recency_eviction_candidate_count`, `duplicate_recency_row_count`, and `missing_recency_row_count`.
- [x] Validate caller-supplied recency rows before eviction planning.
- [x] Order recency candidates by older `resident_page_last_used_generation` first with deterministic page/mount tie-breaks.
- [x] Preserve existing deterministic page-index ordering as the default policy.
- [x] Preserve side-effect flags: no file IO, no live mount mutation, and no renderer/RHI handle access.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` failed before implementation because caller-supplied recency ordering was not applied and duplicate recency rows did not fail closed.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed after implementation.
- GREEN focused CTest: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1 after implementation.
- Static/API checks passed: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply`.
- Full validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` ended with `validate: ok` and CTest `109/109` passing.

## Done When

- Public runtime API exposes a caller-supplied recency ordering mode for automatic MAVG page eviction planning.
- Tests prove older resident-page generations are ordered first, duplicate recency rows fail closed, missing recency rows fail closed, and no live state mutates during planning.
- Docs, plan registry, manifest fragments, composed manifest, and static checks describe the implemented scope and remaining non-claims.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No runtime-inferred LRU/frequency eviction policy.
- No GPU memory pressure integration or allocator enforcement.
- No DirectStorage native queue/file IO execution.
- No sustained async-overlap or performance benchmark claim.
- No renderer/RHI ownership or native handle exposure.
- No mesh shader, deformation, ray tracing, Metal parity, Nanite compatibility/equivalence/superiority, or benchmark-exceeds claim.
