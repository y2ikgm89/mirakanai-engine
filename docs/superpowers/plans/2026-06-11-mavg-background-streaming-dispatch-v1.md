# 2026-06-11 MAVG Background Streaming Dispatch v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow first-party worker-pool dispatch path for reviewed MAVG page package candidate loads.

**Architecture:** Extend `MK_runtime` MAVG page streaming with a helper that dispatches already-planned `RuntimeMavgPageStreamingPlanRow` package candidate loads onto caller-owned `JobExecutionPool` workers. The helper returns loaded package rows for a later caller-owned safe point; resident mount mutation, catalog refresh, DirectStorage, GPU memory pressure policy, renderer/RHI handles, and async-overlap/performance proof stay out of scope.

**Tech Stack:** C++23, `MK_core`, `MK_runtime`, CMake/CTest, PowerShell validation tools.

---

**Plan ID:** `mavg-background-streaming-dispatch-v1`

**Status:** Active local child candidate. Not selected as `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Context

Completed prerequisites:

- `mavg-page-streaming-queue-v1` plans reviewed page requests into package candidates and drains one selected row at a caller-owned safe point.
- `mavg-automatic-eviction-policy-v1`, `mavg-runtime-inferred-page-use-generation-v1`, and `mavg-resident-page-recency-eviction-order-v1` cover reviewed resident page protection and deterministic eviction candidate ordering.
- `mavg-payload-filesystem-byte-range-io-v1` adds synchronous filesystem byte-range MAVG payload extraction, but not background package streaming workers.
- `JobExecutionPool` provides first-party worker execution evidence with deterministic scheduling rows.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- Modify: `engine/runtime/src/mavg_page_streaming.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Modify: `tests/unit/runtime_mavg_page_streaming_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`

## Task 1 - Plan Activation And RED Coverage

- [x] Close `mavg-payload-filesystem-byte-range-io-v1` as completed through PR #570 / merge commit `ab6b8b3`.
- [x] Add this plan to the plan registry as the only active local MAVG child slice.
- [x] Add RED tests proving reviewed MAVG page rows dispatch through `JobExecutionPool` workers, load package candidates, and keep resident mounts/catalogs unchanged.
- [x] Add RED failure coverage for package load failures returning diagnostics without safe-point mutation.

## Task 2 - Runtime Background Load Dispatch

- [x] Add `RuntimeMavgPageStreamingBackgroundLoadDesc`, `RuntimeMavgPageStreamingBackgroundLoadedRow`, and `RuntimeMavgPageStreamingBackgroundLoadResult`.
- [x] Add `dispatch_runtime_mavg_page_streaming_background_loads`.
- [x] Dispatch one worker task per reviewed queued row with deterministic scheduling evidence and worker-local output rows.
- [x] Load package candidates on workers while keeping filesystem access serialized inside the helper instead of assuming broad `IFileSystem` thread safety.
- [x] Return loaded package rows and counters while keeping resident mount mutation, catalog refresh, renderer/RHI handles, DirectStorage, GPU memory pressure policy, and async-overlap/performance proof flags false.

## Task 3 - Docs, Manifest, And Validation

- [x] Record the exact ready claim: reviewed MAVG package candidate loads can execute on first-party background job workers and return loaded rows for a later safe point.
- [x] Keep non-claims explicit: no persistent/autonomous background streaming service, async-overlap/performance proof, DirectStorage, GPU memory pressure integration, resident mount mutation, renderer/RHI upload, package-visible MAVG backend readiness, mesh shaders, Metal readiness, ray tracing, deformation, benchmark results, Nanite equivalence/superiority, or broad optimization.
- [x] Update manifest fragments and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.
- [x] Run focused tests, static guards, public API checks, full `tools/validate.ps1`, publication preflight, and `git diff --check`.

## Non-Claims

This plan does not claim persistent/autonomous background MAVG streaming services, async overlap, performance proof, DirectStorage execution, GPU memory pressure integration, resident mount mutation, catalog refresh, renderer/RHI upload, package-visible MAVG backend readiness, mesh shaders, Metal readiness, deformation, ray tracing, benchmark results, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
