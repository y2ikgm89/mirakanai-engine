# 2026-06-11 MAVG GPU Memory Pressure Residency v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow runtime-RHI bridge that applies reviewed renderer GPU memory pressure evidence to MAVG resident page eviction planning.

**Architecture:** Keep `MK_runtime` independent from renderer policy types. Add the MAVG GPU memory residency bridge in `MK_runtime_rhi`, where runtime MAVG page residency rows and renderer `GpuMemoryPolicyPlan` can meet without changing live resident mounts. The helper converts a validated GPU memory policy counted-byte target into `RuntimeResourceResidencyBudgetV2::max_resident_content_bytes`, delegates to the existing MAVG selected/fallback-protected automatic eviction planner, and returns value-only evidence. DirectStorage, persistent/autonomous streaming services, async-overlap/performance proof, live mount mutation, catalog refresh, GPU upload, backend execution, and native handles stay out of scope.

**Tech Stack:** C++23, `MK_renderer`, `MK_runtime`, `MK_runtime_rhi`, CMake/CTest, PowerShell validation tools.

---

**Plan ID:** `mavg-gpu-memory-pressure-residency-v1`

**Status:** Active local child candidate. Not selected as `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Context

Completed prerequisites:

- `mavg-background-streaming-dispatch-v1` completed through PR #573 / merge commit `933fe6dc` with first-party `JobExecutionPool` package candidate load dispatch evidence.
- `gpu_memory_policy.hpp` already provides renderer-side memory budget, residency pressure, package counter, and fail-closed diagnostics.
- `mavg-resident-page-recency-eviction-order-v1` and `plan_runtime_mavg_page_streaming_automatic_evictions` already protect selected visible pages plus resident fallback ancestors before calling reviewed resident package eviction planning.
- `RuntimeResourceResidencyBudgetV2` is the existing runtime byte budget contract for resident package catalogs.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_memory_residency.hpp`
- Create: `engine/runtime_rhi/src/mavg_gpu_memory_residency.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Create: `tests/unit/runtime_rhi_mavg_gpu_memory_residency_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`

## Task 1 - Plan Activation And RED Coverage

- [x] Close `mavg-background-streaming-dispatch-v1` as completed through PR #573 / merge commit `933fe6dc`.
- [x] Add this plan to the plan registry as the only active local MAVG child slice.
- [x] Add RED tests proving GPU memory policy counted bytes become an explicit resident content byte budget for selected/fallback-protected MAVG eviction planning.
- [x] Add RED failure coverage for missing GPU residency pressure evidence returning diagnostics without eviction planning or live mount mutation.

## Task 2 - Runtime-RHI GPU Memory Residency Bridge

- [x] Add `RuntimeMavgGpuMemoryResidencyDesc`, `RuntimeMavgGpuMemoryResidencyResult`, diagnostics, and `plan_runtime_mavg_gpu_memory_pressure_residency`.
- [x] Require successful renderer `GpuMemoryPolicyPlan` evidence, declared memory budget evidence, residency pressure evidence, package counter evidence, and a non-zero counted byte target.
- [x] Convert the counted byte target into `RuntimeResourceResidencyBudgetV2::max_resident_content_bytes`.
- [x] Delegate to `plan_runtime_mavg_page_streaming_automatic_evictions` so selected visible pages and fallback ancestors remain protected and caller-supplied recency can order candidates.
- [x] Return counters and fail-closed diagnostics while keeping live mount mutation, catalog refresh, file IO, DirectStorage, async-overlap/performance proof, renderer/RHI handle access, and GPU upload flags false.

## Task 3 - Docs, Manifest, And Validation

- [x] Record the exact ready claim: MAVG resident page eviction planning can consume reviewed renderer GPU memory pressure evidence as a resident content byte budget.
- [x] Keep non-claims explicit: no persistent/autonomous background streaming service, async-overlap/performance proof, DirectStorage execution, live mount mutation, catalog refresh, GPU upload, package-visible MAVG backend readiness, mesh shaders, Metal readiness, ray tracing, deformation, benchmark results, Nanite equivalence/superiority, or broad optimization.
- [x] Update manifest fragments and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.
- [x] Run focused tests, static guards, public API checks, full `tools/validate.ps1`, publication preflight, and `git diff --check`.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_gpu_memory_residency_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_rhi_mavg_gpu_memory_residency_tests --output-on-failure`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `git diff --check`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1`

The first full validation pass reproduced a transient `MK_d3d12_rhi_tests` SegFault. A targeted rerun of `MK_d3d12_rhi_tests` passed, no D3D12/RHI backend files were changed by this slice, and the subsequent full `tools/validate.ps1` pass completed successfully.

## Non-Claims

This plan does not claim persistent/autonomous background MAVG streaming services, async overlap, performance proof, DirectStorage execution, live resident mount mutation, catalog refresh, GPU upload, package-visible MAVG backend readiness, mesh shaders, Metal readiness, deformation, ray tracing, benchmark results, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
