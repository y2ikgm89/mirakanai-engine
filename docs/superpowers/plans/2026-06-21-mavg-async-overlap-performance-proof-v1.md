# MAVG Async Overlap Performance Proof v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a fail-closed, selected measured-sample async-overlap performance proof that caller-supplied MAVG background-load and GPU-upload timing samples demonstrate async overlap performance for one reviewed package lane.

**Architecture:** Keep the selected measured-sample async-overlap performance proof value-only in `MK_runtime_rhi`: it consumes existing `RuntimeMavgStreamingUploadOverlapEvidenceResult` rows plus reviewed measurement samples, computes deterministic p95 serial/overlapped ticks, and promotes only a narrow `proved_async_overlap_performance` row. It does not measure live hardware in unit tests, expose native handles, claim GPU DirectStorage destinations, GDeflate, mesh shaders, Metal readiness, Nanite equivalence, or broad optimization.

**Tech Stack:** C++23, `MK_runtime_rhi`, CMake/CTest, PowerShell validation, existing package smoke counters, Microsoft DirectStorage SDK 1.3.0 retained evidence, Microsoft Windows Performance Toolkit/GPUView guidance for future host-attached traces, and Context7 `/kitware/cmake` guidance for target/test integration.

---

**Plan ID:** `mavg-async-overlap-performance-proof-v1`

**Status:** Active.

**Date:** 2026-06-21

## Context

MAVG Streaming Upload Overlap Evidence v1 already records deterministic temporal overlap from caller-owned timing windows, but it intentionally keeps `claimed_speedup=false` and `proved_async_overlap_performance=false`. MAVG Win32 DirectStorage SDK Adapter v1 added the first-party DirectStorage system-memory adapter and still preserves zero async-overlap/performance claims. This slice closes the next narrow roadmap gap by validating selected measured samples after overlap evidence exists.

## Official Source Gate

- Microsoft DirectStorage landing/API guidance: `https://devblogs.microsoft.com/directx/directstorage-api-downloads/`.
- Microsoft DirectStorage 1.3 release guidance: `https://devblogs.microsoft.com/directx/directstorage-1-3-is-now-available/`.
- Microsoft Windows Performance Toolkit: `https://learn.microsoft.com/en-us/windows-hardware/test/wpt/`.
- Microsoft Windows Performance Analyzer: `https://learn.microsoft.com/en-us/windows-hardware/test/wpt/windows-performance-analyzer`.
- Microsoft GPUView: `https://learn.microsoft.com/en-us/windows-hardware/drivers/display/using-gpuview`.
- Context7 CMake docs: `/kitware/cmake`, queried for `target_sources`, `target_link_libraries`, CMake presets, and CTest integration.

## Scope

In scope:

- New `MK_runtime_rhi` value API for selected measured-sample async-overlap performance proof rows.
- Deterministic p95 and speedup-basis-point calculation over caller-supplied measurement rows.
- Fail-closed diagnostics for missing overlap evidence, mismatched workload ids, insufficient samples, non-overlapped samples, invalid tick data, missing artifact ids, and insufficient speedup.
- Unit tests proving ready and fail-closed behavior.
- `sample_desktop_runtime_game` package counters for a selected reviewed sample lane.
- A focused validator script and static AI integration guard.
- Docs, manifest fragment, composed manifest, and plan registry synchronization.

Out of scope:

- Live timing collection inside unit tests.
- Broad CPU/GPU/memory optimization readiness.
- GPU DirectStorage destinations, GDeflate/GPU decompression, DirectStorage 1.4 preview APIs, native handle exposure, mesh shaders, Metal MAVG readiness, ray tracing, deformation, or Nanite equivalence/superiority.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_async_overlap_performance_proof.hpp`
- Create: `engine/runtime_rhi/src/mavg_async_overlap_performance_proof.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Create: `tests/unit/runtime_rhi_mavg_async_overlap_performance_proof_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Create: `tools/validate-mavg-async-overlap-performance-proof.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Create: `tools/check-ai-integration-122-mavg-async-overlap-performance-proof.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Regenerate: `engine/agent/manifest.json`

## Done When

- `RuntimeMavgAsyncOverlapPerformanceProofResult::proved_async_overlap_performance` is true only for selected samples with valid overlap evidence, matching workload/artifact identity, enough samples, positive p95 speedup, and no diagnostics.
- Package smoke emits `mavg_async_overlap_performance_proof_ready=1` with speedup 2500 basis points for the selected lane and keeps all broad/non-selected claims at zero.
- Static guards reject broad promotion or missing non-claims.
- Focused validator and full validation pass.

## Tasks

### Task 1: Active Plan Selection

- [x] Create this plan with official source gate, exact scope, files, done criteria, and explicit non-claims.
- [x] Point `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json.aiOperableProductionLoop.currentActivePlan` at this plan.
- [x] Set `recommendedNextPlan.id` to `mavg-async-overlap-performance-proof-v1`.
- [x] Update `docs/superpowers/plans/README.md` active slice row.
- [x] Update `tools/check-ai-integration-020-engine-manifest.ps1` selection needles.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 2: API Tests

- [x] Add tests that build a ready overlap evidence result, five serial samples, five overlapped samples, and one artifact id.
- [x] Assert p95 serial ticks, p95 overlapped ticks, positive `speedup_basis_points`, `claimed_speedup=true`, and `proved_async_overlap_performance=true`.
- [x] Add fail-closed tests for missing overlap evidence, insufficient samples, mismatched workload ids, zero artifact id, no temporal overlap, non-positive ticks, and insufficient speedup.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_async_overlap_performance_proof_tests` and confirm the target initially fails before implementation.

### Task 3: Runtime-RHI Implementation

- [x] Add value types and diagnostic enum in `mavg_async_overlap_performance_proof.hpp`.
- [x] Implement deterministic p95 calculation, workload/artifact validation, sample validation, and basis-point speedup calculation.
- [x] Keep all inputs caller-owned and expose no native handles, backend objects, or timing collection APIs.
- [x] Register the source in `engine/runtime_rhi/CMakeLists.txt`.
- [x] Run the focused test target until it passes.

### Task 4: Package Smoke And Validator

- [x] Add `--require-mavg-async-overlap-performance-proof` to `sample_desktop_runtime_game`.
- [x] Emit package counters for status, ready flag, sample count, serial p95, overlapped p95, speedup basis points, proof flag, and explicit zero non-claim counters.
- [x] Add `tools/validate-mavg-async-overlap-performance-proof.ps1` to build/run the focused target and package smoke.
- [x] Run the validator and require `mavg_async_overlap_performance_proof_ready=1`.

### Task 5: Docs, Manifest, Static Guards

- [x] Update current capabilities, roadmap, MAVG master plan, production master plan, plan registry, manifest fragment, and composed manifest.
- [x] Add static guard `tools/check-ai-integration-122-mavg-async-overlap-performance-proof.ps1`.
- [x] Ensure docs say this is selected measured-sample proof, not broad optimization or hardware-wide performance readiness.
- [x] Run `check-ai-integration`, `check-json-contracts`, `check-text-format`, `check-format`, and `git diff --check`.

### Task 6: Slice Validation And Publication

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-async-overlap-performance-proof.ps1 -RequireReady`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [ ] Commit the validated candidate.
- [ ] Push branch `codex/mavg-async-overlap-performance-proof`.
- [ ] Open a draft PR, wait for hosted checks, mark ready with `tools/ready-task-pr.ps1`, and auto-merge only after PR Gate succeeds.
