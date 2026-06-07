# MAVG Resident Page Recency Eviction Order v1 Implementation Plan

**Date:** 2026-06-07

**Plan ID:** `mavg-resident-page-recency-eviction-order-v1`

**Status:** Completed candidate pending publication.

## Goal

Add a deterministic, caller-supplied recency ordering mode to MAVG automatic page eviction planning so a host can pass reviewed resident-page last-use evidence without making the runtime infer LRU or frequency behavior.

## Context

MAVG Automatic Eviction Policy v1 already produces a deterministic reviewed eviction candidate order from resident page mounts while protecting selected visible pages and fallback ancestors. That order is intentionally not a real LRU policy. This child adds a narrower policy mode: callers may supply one `RuntimeMavgPageStreamingRecencyRow` per unprotected eviction candidate, and the planner orders older `resident_page_last_used_generation` values first.

## Official/Project Docs Review

This slice introduces no third-party dependency, SDK, platform API, native handle, renderer/RHI object, or build-system surface. The relevant guidance is the repository C++23/public API rules in `docs/cpp-style.md`, the production selection protocol in `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`, the MAVG architecture spec, and the existing `tools/*.ps1` validation entrypoints. Context7 remains the preferred route for live library/SDK/toolchain docs, but this implementation has no external library/SDK API to query.

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
- [x] Sync current capabilities, roadmap, architecture spec, plan registry, manifest fragments, and static checks.
- [x] Compose `engine/agent/manifest.json`.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` failed after tests were added because `RuntimeMavgPageStreamingRecencyRow`, `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency`, recency descriptor fields, recency result counters, and recency diagnostics were not defined.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed.
- GREEN focused CTest: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1.
- Focused clang-tidy: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp" -ReuseExistingFileApiReply` passed.
- Agent/static checks: `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-ai-integration.ps1` passed after docs, manifest fragments, composed manifest, and static needles were synchronized.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; static checks, build, tidy smoke, and 107/107 CTest targets passed. Shader toolchain Metal and Apple host evidence remain expected diagnostic-only Windows host gates.

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
