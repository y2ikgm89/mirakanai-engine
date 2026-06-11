# 2026-06-11 MAVG Cluster Streaming Safe Point Adoption v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow caller-owned safe-point adoption helper that commits PR #575 MAVG background-loaded page rows into resident mount/catalog state with reviewed eviction planning.

**Architecture:** Keep the adoption helper in `MK_runtime_rhi` because it consumes the runtime-RHI closeout result and existing GPU-memory-residency eviction evidence, while using only public `MK_runtime` `RuntimeResidentPackageMountSetV2` / `RuntimeResidentCatalogCacheV2` primitives. The helper must be deterministic and fail-closed: it may mutate the caller-owned mount set and refresh the caller-owned catalog cache only after projection succeeds through `plan_runtime_resident_package_evictions_v2`, and it must not execute DirectStorage, GPU upload, backend rendering, async-overlap proof, or autonomous streaming.

**Tech Stack:** C++23, `MK_runtime`, `MK_runtime_rhi`, CMake/CTest, PowerShell validation tools.

---

**Plan ID:** `mavg-cluster-streaming-safe-point-adoption-v1`

**Status:** Published as draft PR #576 after PR #575 merged and this branch rebased onto `origin/main`.

## Context

Prerequisites:

- `mavg-gpu-memory-pressure-residency-v1` completed through PR #574 / merge commit `c6153740`.
- `mavg-cluster-streaming-residency-closeout-v1` is published as draft PR #575 and provides `RuntimeMavgClusterStreamingResidencyCloseoutResult`.
- Existing `MK_runtime` public APIs provide resident mount mutation, reviewed eviction planning, and resident catalog cache refresh.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_cluster_streaming_safe_point_adoption.hpp`
- Create: `engine/runtime_rhi/src/mavg_cluster_streaming_safe_point_adoption.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Create: `tests/unit/runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests.cpp`
- Modify docs, plan registry, master plan, roadmap/current capabilities, manifest fragments, composed manifest, and static guard needles after behavior is green.

## Task 1 - RED Adoption Coverage

- [x] Add a failing test proving a successful closeout background-loaded row is adopted at a caller-owned safe point without reloading the package candidate, with the new page mounted, reviewed eviction applied, catalog cache refreshed, and selected/fallback protection preserved.
- [x] Add a failing test proving adoption fails closed on invalid/duplicate mount ids or failed closeout evidence, leaving mount set generation and catalog cache unchanged.

## Task 2 - Runtime-RHI Safe Point Adoption Helper

- [x] Add `RuntimeMavgClusterStreamingSafePointAdoptionDesc`, diagnostics, counters, `RuntimeMavgClusterStreamingSafePointAdoptionResult`, and `execute_runtime_mavg_cluster_streaming_safe_point_adoption`.
- [x] Consume `RuntimeMavgClusterStreamingResidencyCloseoutResult::background_load.loaded_rows` and `gpu_memory_residency.eviction_review` / budget evidence without invoking package candidate load again.
- [x] Project mount + reviewed eviction + catalog refresh before committing live state, then commit only on success.
- [x] Preserve explicit non-claims for DirectStorage, GPU upload, backend execution, renderer/RHI handle access, autonomous streaming, async-overlap/performance proof, mesh shaders, Nanite readiness, and broad optimization.

## Task 3 - Docs, Manifest, And Validation

- [x] Record the exact ready claim: caller-owned MAVG safe-point adoption can commit reviewed background-loaded page packages into resident mount/catalog state after closeout evidence succeeds.
- [x] Keep non-claims explicit: no persistent/autonomous streaming service, DirectStorage, GPU upload/backend execution, async-overlap/performance proof, mesh shaders, Metal readiness, ray tracing, deformation, Nanite equivalence/superiority, or broad optimization.
- [x] Update manifest fragments and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.
- [x] Run focused tests, static guards, public API checks, full `tools/validate.ps1`, publication preflight, and `git diff --check`.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests --output-on-failure` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 117/117 CTest tests passing.

## Non-Claims

This plan does not claim persistent/autonomous background MAVG streaming services, async overlap, performance proof, DirectStorage execution, GPU upload, backend execution, package-visible MAVG backend readiness, mesh shaders, Metal readiness, deformation, ray tracing, benchmark results, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
