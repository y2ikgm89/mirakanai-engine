# MAVG Automatic Eviction Policy v1 Implementation Plan

**Date:** 2026-06-07

**Plan ID:** `mavg-automatic-eviction-policy-v1`

**Status:** Completed candidate pending publication.

## Goal

Add a deterministic host-independent automatic eviction policy for MAVG page streaming so resident page mount rows can become a reviewed eviction candidate order without operator-supplied eviction ids.

## Context

MAVG Runtime LOD Milestone v1, MAVG Page Streaming Queue v1, and MAVG Page Streaming Eviction Review v1 already provide graph/page validation, selected visible cluster protection, fallback ancestor protection, caller-protected mount ids, reviewed eviction planning, and one-row safe-point drain through the existing resident package mount path. This slice adds only the missing candidate-order planner on top of that review surface.

## Official/Project Docs Review

This slice introduces no third-party dependency, SDK, platform API, native handle, renderer/RHI object, or build-system surface. The relevant guidance is the repository C++23/public API rules in `docs/cpp-style.md`, the production selection protocol in `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`, the MAVG architecture spec, and the existing `tools/*.ps1` validation entrypoints. Context7 was checked as the preferred route for library/SDK/toolchain docs, but this implementation has no external library/SDK API to query; prior Context7 access in this session also returned OAuth authorization failure.

## Constraints

- Keep the implementation pure planning until an existing safe-point commit path runs.
- Reuse selected/fallback page protection and resident mount validation semantics from `review_runtime_mavg_page_streaming_evictions`.
- Sort automatic candidates deterministically by descending page index, then ascending mount id.
- Do not infer runtime-inferred LRU/frequency behavior, GPU memory pressure, renderer/RHI residency, async-overlap performance, DirectStorage native execution, or Nanite compatibility/equivalence/superiority.
- Do not add third-party dependencies, native handles, compatibility shims, background workers, or package/file IO.

## Implementation

- [x] Add `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc`.
- [x] Add `plan_runtime_mavg_page_streaming_automatic_evictions`.
- [x] Share internal eviction review protection/validation before reviewed and automatic eviction planning.
- [x] Record `planned_automatic_eviction_policy`, `automatic_eviction_candidate_count`, and `protected_eviction_candidate_skip_count`.
- [x] Preserve side-effect flags proving no file IO, mount mutation, background streaming, renderer/RHI handle access, or inferred policy.
- [x] Sync current capabilities, roadmap, architecture spec, plan registry, manifest fragments, and static checks.
- [x] Compose `engine/agent/manifest.json`.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` failed because `plan_runtime_mavg_page_streaming_automatic_evictions` and `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc` were not defined.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed.
- GREEN focused CTest: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1.
- Static/API checks passed: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `git diff --check`, `tools/check-public-api-boundaries.ps1`, and `tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply`.
- Full validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed 107/107 tests and ended with `validate: ok`; Apple/Metal host checks remained diagnostic host-gated on Windows as expected.

## Done When

- Public runtime API exposes deterministic automatic MAVG page eviction planning.
- Tests prove protected visible/fallback pages are skipped, unprotected resident pages are ordered deterministically, duplicate resident page rows fail closed before eviction planning, and live mount state is not mutated during planning.
- Docs, plan registry, manifest fragments, composed manifest, and static checks describe the implemented scope and remaining non-claims.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No runtime-inferred LRU/frequency eviction policy.
- No GPU memory pressure integration or allocator enforcement.
- No DirectStorage native queue/file IO execution.
- No sustained async-overlap or performance benchmark claim.
- No renderer/RHI ownership, upload, residency, or native handle exposure.
- No background streaming worker, mesh shader, deformation, ray tracing, Metal parity, Nanite compatibility/equivalence/superiority, or benchmark-exceeds claim.
