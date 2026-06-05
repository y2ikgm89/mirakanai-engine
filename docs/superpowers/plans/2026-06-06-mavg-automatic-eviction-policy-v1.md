# MAVG Automatic Eviction Policy v1 Implementation Plan

**Date:** 2026-06-06

**Plan ID:** `mavg-automatic-eviction-policy-v1`

**Status:** Completed candidate pending publication.

## Goal

Add a deterministic host-independent automatic eviction policy for MAVG page streaming so resident page mount rows can become a reviewed eviction candidate order without operator-supplied eviction ids.

## Context

MAVG Runtime LOD Milestone v1 already has reviewed page request planning, selected visible/fallback page protection, safe-point dispatch planning, and the joined engine-owned page streaming worker. Before this candidate, eviction planning required a caller-reviewed candidate order. This child keeps the same safe-point and resident-budget boundaries while adding only the missing deterministic candidate-order planner.

## Constraints

- Keep the implementation pure planning until the existing safe-point commit path runs.
- Reuse `review_runtime_mavg_page_streaming_evictions` validation semantics for graph, resident page rows, selected clusters, fallback ancestors, and caller-protected mounts.
- Do not infer LRU, recency, frequency, GPU memory pressure, renderer/RHI residency, async-overlap performance, DirectStorage native execution, or Nanite compatibility/equivalence/superiority.
- Do not add third-party dependencies or compatibility shims.

## Implementation

- [x] Add `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc`.
- [x] Add `plan_runtime_mavg_page_streaming_automatic_evictions`.
- [x] Reuse a shared internal protection/validation helper for both reviewed and automatic eviction planning.
- [x] Record `planned_automatic_eviction_policy`, `automatic_eviction_candidate_count`, and `protected_eviction_candidate_skip_count`.
- [x] Sort unprotected resident page candidates deterministically by descending page index, then ascending mount id.
- [x] Preserve side-effect flags: no file IO, no live mount mutation, and no renderer/RHI handle access.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` failed because `plan_runtime_mavg_page_streaming_automatic_evictions` and `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc` were not yet defined.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed.
- GREEN focused CTest: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1.
- Static/API checks passed: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply`.
- Full validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` ended with `validate: ok`.

## Done When

- Public runtime API exposes deterministic automatic MAVG page eviction planning.
- Tests prove protected visible/fallback pages are skipped, unprotected candidates are ordered deterministically, duplicate resident page rows fail closed before eviction planning, and no live state mutates during planning.
- Docs, plan registry, manifest fragments, composed manifest, and static checks describe the implemented scope and remaining non-claims.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No LRU, recency, or frequency eviction policy.
- No GPU memory pressure integration or allocator enforcement.
- No DirectStorage native queue/file IO execution.
- No sustained async-overlap or performance benchmark claim.
- No renderer/RHI ownership or native handle exposure.
- No mesh shader, deformation, ray tracing, Metal parity, Nanite compatibility/equivalence/superiority, or benchmark-exceeds claim.
