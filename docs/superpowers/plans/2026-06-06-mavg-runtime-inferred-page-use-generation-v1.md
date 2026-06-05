# MAVG Runtime-Inferred Page Use Generation v1 Implementation Plan

**Date:** 2026-06-06

**Plan ID:** `mavg-runtime-inferred-page-use-generation-v1`

**Status:** Completed candidate pending publication.

## Goal

Add a value-only runtime helper that infers resident MAVG page last-use generations from reviewed selected-cluster evidence and returns `RuntimeMavgPageStreamingRecencyRow` rows that can feed the existing caller-supplied recency eviction policy.

## Context

MAVG Resident Page Recency Eviction Order v1 lets a host provide `RuntimeMavgPageStreamingRecencyRow` evidence to order unprotected eviction candidates by older `resident_page_last_used_generation` first. This child adds the next narrow step: runtime can derive those rows from current selected resident pages without introducing a hidden clock, background residency service, GPU memory pressure integration, or a broad LRU/frequency eviction policy.

## Constraints

- Keep inference value-only and side-effect-free: no file IO, mount-set mutation, renderer/RHI handle access, or background worker ownership.
- Touch only pages selected by current selected-cluster rows; selected fallback/protection logic remains owned by existing eviction review.
- Emit exactly one recency row per current resident page mount, sorted by ascending page index and ascending mount id.
- Carry previous matching generations for unselected resident pages, initialize new resident pages to cold generation `0`, and drop nonresident previous rows with a counter instead of failing.
- Fail closed for invalid graph inputs, invalid or duplicate resident page mounts, invalid selected clusters, invalid or duplicate previous recency rows, and non-monotonic retained generations.
- Preserve the existing caller-supplied recency eviction ordering and do not set `inferred_eviction_policy`.
- Do not claim runtime-inferred LRU/frequency eviction policy, GPU memory pressure integration, DirectStorage execution, async overlap/performance, Nanite compatibility/equivalence/superiority, or benchmark superiority.

## Implementation

- [x] Add `RuntimeMavgResidentPageUseGenerationDesc`.
- [x] Add `RuntimeMavgResidentPageUseGenerationResult` with `recency_rows`, `inferred_resident_page_use_generation`, diagnostics, side-effect flags, and counters for selected, carried, new, dropped, duplicate, and missing evidence.
- [x] Add `RuntimeMavgPageStreamingDiagnosticCode::non_monotonic_use_generation`.
- [x] Add `infer_runtime_mavg_resident_page_use_generations`.
- [x] Validate selected cluster rows against the requested graph and current resident page mounts.
- [x] Validate previous recency rows before emitting output rows.
- [x] Feed generated rows through the existing `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency` path in tests.
- [x] Preserve no-file-IO, no-live-mount-mutation, and no-renderer/RHI-handle evidence flags.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` failed before implementation because `infer_runtime_mavg_resident_page_use_generations`, `RuntimeMavgResidentPageUseGenerationDesc`, and `non_monotonic_use_generation` were not declared.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed after implementation.
- GREEN focused CTest: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1 after implementation.
- Slice-closing static/API/full validation is pending in this branch.

## Done When

- Public runtime API exposes the value-only page-use generation inference helper.
- Tests prove selected pages get the current generation, unselected rows carry forward, new resident pages initialize cold, nonresident old rows are dropped and counted, duplicate old rows fail closed, non-monotonic retained generations fail closed, and generated rows feed the existing caller-supplied recency eviction order.
- Docs, plan registry, manifest fragments, composed manifest, and static checks describe the implemented scope and remaining non-claims.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No runtime-inferred LRU/frequency eviction policy.
- No GPU memory pressure integration or allocator enforcement.
- No DirectStorage native queue/file IO execution.
- No sustained async-overlap or performance benchmark claim.
- No background streaming service readiness beyond existing joined worker evidence.
- No renderer/RHI ownership or native handle exposure.
- No mesh shader, deformation, ray tracing, Metal parity, Nanite compatibility/equivalence/superiority, or benchmark-exceeds claim.
