# 2026-06-11 MAVG Cluster Streaming Residency Closeout v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the narrow Phase 5 MAVG cluster streaming/residency evidence lane by composing reviewed page request planning, filesystem byte-range payload reads, background package candidate loads, and GPU memory pressure residency planning into one value-level closeout result.

**Architecture:** Keep the closeout in `MK_runtime_rhi` because it consumes renderer `GpuMemoryPolicyPlan` evidence while delegating to existing `MK_runtime` MAVG page streaming, payload byte-range loading, background dispatch, and selected/fallback-protected eviction planning. The result is orchestration evidence only: it does not perform safe-point resident mount mutation, catalog refresh, DirectStorage, GPU upload, backend execution, or async-overlap/performance proof.

**Tech Stack:** C++23, `MK_runtime`, `MK_runtime_rhi`, `MK_renderer`, CMake/CTest, PowerShell validation tools.

---

**Plan ID:** `mavg-cluster-streaming-residency-closeout-v1`

**Status:** Published draft PR #575 after rebasing onto `origin/main` after PR #574 merged.

## Context

Completed prerequisites:

- `mavg-payload-filesystem-byte-range-io-v1` completed through PR #570.
- `mavg-background-streaming-dispatch-v1` completed through PR #573 / merge commit `933fe6dc`.
- `mavg-gpu-memory-pressure-residency-v1` completed through PR #574 / merge commit `c6153740`.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_cluster_streaming_residency_closeout.hpp`
- Create: `engine/runtime_rhi/src/mavg_cluster_streaming_residency_closeout.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Create: `tests/unit/runtime_rhi_mavg_cluster_streaming_residency_closeout_tests.cpp`
- Modify docs, plan registry, master plan, manifest fragments, composed manifest, and static guard needles after behavior is green.

## Task 1 - RED Closeout Coverage

- [x] Add tests proving selected large MAVG page requests load payload byte ranges, dispatch reviewed package candidate loads, and apply GPU memory pressure eviction planning while preserving selected/fallback protection.
- [x] Add fail-closed coverage for payload byte-range failure or background load failure returning deterministic degradation diagnostics without mount mutation, catalog refresh, DirectStorage, GPU upload, or backend execution.

## Task 2 - Runtime-RHI Closeout Orchestrator

- [x] Add `RuntimeMavgClusterStreamingResidencyCloseoutDesc`, diagnostics, counters, `RuntimeMavgClusterStreamingResidencyCloseoutResult`, and `plan_runtime_mavg_cluster_streaming_residency_closeout`.
- [x] Call existing `plan_runtime_mavg_page_streaming_requests`, `load_runtime_mavg_payload_pages_from_filesystem`, `dispatch_runtime_mavg_page_streaming_background_loads`, and `plan_runtime_mavg_gpu_memory_pressure_residency`.
- [x] Preserve no-mutation and non-claim flags for safe-point mount mutation, catalog refresh, DirectStorage, GPU upload, backend execution, renderer/RHI handle access, and async-overlap/performance proof.

## Task 3 - Docs, Manifest, And Validation

- [x] Record the exact ready claim: Phase 5 now has one value-level closeout result tying reviewed page streaming, byte-range payload reads, background candidate loads, and GPU memory pressure resident byte-budget planning together.
- [x] Keep non-claims explicit: no persistent/autonomous streaming service, live mount mutation, catalog refresh, DirectStorage, GPU upload/backend execution, package-visible backend readiness beyond existing D3D12/Vulkan compute-generated indirect evidence, mesh shaders, Metal readiness, ray tracing, deformation, benchmark results, Nanite equivalence/superiority, or broad optimization.
- [x] Update manifest fragments and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.
- [x] Run focused tests, static guards, public API checks, full `tools/validate.ps1`, and `git diff --check`.
- [x] After PR #574 merges, rebase onto `origin/main` and rerun drift-sensitive checks.
- [x] Run publication preflight and publish this closeout branch as draft PR #575.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_cluster_streaming_residency_closeout_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_rhi_mavg_cluster_streaming_residency_closeout_tests --output-on-failure`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `git diff --check`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (exit 0; 116/116 tests passed)

## Non-Claims

This plan does not claim persistent/autonomous background MAVG streaming services, async overlap, performance proof, DirectStorage execution, live resident mount mutation, catalog refresh, GPU upload, package-visible MAVG backend readiness beyond existing selected D3D12/Vulkan compute-generated indirect evidence, mesh shaders, Metal readiness, deformation, ray tracing, benchmark results, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
