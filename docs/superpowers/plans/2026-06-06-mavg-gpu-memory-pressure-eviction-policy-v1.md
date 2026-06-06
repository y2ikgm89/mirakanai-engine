# MAVG GPU Memory Pressure Eviction Policy v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add value-only GPU memory pressure eviction planning for MAVG resident pages.

**Architecture:** The runtime accepts caller-reviewed GPU memory pressure rows, applies the existing selected visible/fallback ancestor protection first, then orders only unprotected resident page eviction candidates by pressure score and estimated GPU bytes. This child does not inspect renderer/RHI objects, mutate GPU residency, execute DirectStorage, or claim allocator enforcement.

**Tech Stack:** C++23, `MK_runtime`, `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc`, `MK_runtime_mavg_page_streaming_tests`, PowerShell validation tools, Context7 `/websites/cppreference`, Microsoft Direct3D 12 memory-management guidance.

---

**Date:** 2026-06-06

**Plan ID:** `mavg-gpu-memory-pressure-eviction-policy-v1`

**Status:** Published as stacked draft PR #486.

## Context

PR #484 adds `mavg-runtime-inferred-frequency-eviction-policy-v1` as the previous stacked child. This child advances the remaining GPU memory pressure gap without broadening into real GPU residency management. Microsoft Direct3D 12 memory management guidance recommends a classify/budget/stream strategy and `IDXGIAdapter3::QueryVideoMemoryInfo` / `DXGI_QUERY_VIDEO_MEMORY_INFO` expose current usage and budget as pressure evidence; this child consumes equivalent reviewed value rows only. Context7 `/websites/cppreference` confirms the existing C++23 shape remains appropriate: `std::span` for caller-owned input rows, `std::vector` for owned result evidence, and `std::ranges::sort` with explicit tie-breakers for deterministic ordering.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- Modify: `engine/runtime/src/mavg_page_streaming.cpp`
- Modify: `tests/unit/runtime_mavg_page_streaming_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generated: `engine/agent/manifest.json` via `tools/compose-agent-manifest.ps1 -Write`
- Modify: `tools/check-ai-integration-104-mavg-runtime-lod.ps1`
- Create: `tools/check-ai-integration-124-mavg-gpu-memory-pressure-eviction-policy.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

## Implementation

- [x] **Step 1: Add RED tests for caller-supplied GPU memory pressure ordering.**

Add tests named:

```cpp
MK_TEST("runtime mavg page streaming caller supplied gpu memory pressure orders high pressure unprotected pages first")
MK_TEST("runtime mavg page streaming caller supplied gpu memory pressure uses estimated bytes tie breaker")
```

Expected RED before implementation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests
```

Expected failure: `RuntimeMavgPageStreamingGpuMemoryPressureRow`, `caller_supplied_gpu_memory_pressure`, and pressure counters are undefined.

- [x] **Step 2: Add RED tests for fail-closed pressure evidence.**

Add tests named:

```cpp
MK_TEST("runtime mavg page streaming caller supplied gpu memory pressure rejects missing candidate row")
MK_TEST("runtime mavg page streaming caller supplied gpu memory pressure rejects duplicate rows")
MK_TEST("runtime mavg page streaming caller supplied gpu memory pressure rejects estimated byte overflow")
```

Expected RED command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests
```

- [x] **Step 3: Add public value-only pressure API.**

Add diagnostic codes:

```cpp
gpu_memory_pressure_graph_mismatch,
invalid_gpu_memory_pressure_row,
duplicate_gpu_memory_pressure_row,
missing_gpu_memory_pressure_row,
gpu_memory_pressure_counter_overflow,
```

Add policy kind:

```cpp
caller_supplied_gpu_memory_pressure,
```

Add row:

```cpp
struct RuntimeMavgPageStreamingGpuMemoryPressureRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    RuntimeResidentPackageMountIdV2 mount_id;
    std::uint64_t eviction_pressure_score{0};
    std::uint64_t estimated_gpu_resident_bytes{0};
};
```

Extend plan desc/result with `gpu_memory_pressure_rows`, `planned_gpu_memory_pressure_eviction_policy`, `applied_caller_supplied_gpu_memory_pressure_policy`, `gpu_memory_pressure_eviction_candidate_count`, `missing_gpu_memory_pressure_row_count`, `duplicate_gpu_memory_pressure_row_count`, `gpu_memory_pressure_candidate_estimated_bytes`, `gpu_memory_pressure_protected_estimated_bytes`, and `gpu_memory_pressure_counter_overflow`.

- [x] **Step 4: Implement pressure validation and ordering.**

Rules:

- Validate graph asset, mounted page row match, non-zero mount id, unique page/mount rows.
- Missing pressure row for an unprotected eviction candidate fails before `invoked_eviction_plan`.
- Duplicate pressure rows fail before `invoked_eviction_plan`.
- Aggregated estimated byte overflow fails before `invoked_eviction_plan`.
- Sort by higher `eviction_pressure_score`, then higher `estimated_gpu_resident_bytes`, then higher `page_index`, then lower mount id.

- [x] **Step 5: Run focused GREEN.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests
```

- [x] **Step 6: Update docs, manifest fragments, composed manifest, and static checks.**

Claim only:

- `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_gpu_memory_pressure`
- `RuntimeMavgPageStreamingGpuMemoryPressureRow`
- `planned_gpu_memory_pressure_eviction_policy`
- `applied_caller_supplied_gpu_memory_pressure_policy`
- `gpu_memory_pressure_eviction_candidate_count`
- `gpu_memory_pressure_counter_overflow`
- pressure candidate ordering and fail-closed diagnostics
- no renderer/RHI handles or handle access
- no real GPU residency enforcement
- no allocator enforcement
- no DirectStorage execution
- no Nanite equivalence/superiority

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] **Step 7: Run focused/static/full validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 8: Publish stacked PR.**

Run publication preflight, commit the candidate, push `codex/mavg-gpu-memory-pressure-eviction-policy-v1`, and create a draft PR over `codex/mavg-runtime-inferred-frequency-eviction-policy-v1`.

## Validation Evidence

- Baseline: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed before this candidate's test edits.
- Baseline: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1 before this candidate's test edits.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` failed because `RuntimeMavgPageStreamingGpuMemoryPressureRow`, `caller_supplied_gpu_memory_pressure`, pressure desc fields, pressure counters, and pressure diagnostic codes were undefined.
- Focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed after the value-only pressure API and ordering implementation.
- Focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed 1/1 after the value-only pressure API and ordering implementation.
- Docs/static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after docs/manifest/static guard updates.
- Focused static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `git diff --check` passed.
- Full slice validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; CTest reported 109/109 tests passing. Metal/Apple and mobile Apple diagnostics remained host-gated on Windows as expected.
- Publication: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` passed with `publication-preflight: ok`; commit `4641e2df` was pushed to `origin/codex/mavg-gpu-memory-pressure-eviction-policy-v1`; stacked draft PR #486 was created over `codex/mavg-runtime-inferred-frequency-eviction-policy-v1`.

## Done When

- Runtime exposes caller-supplied GPU memory pressure rows and deterministic pressure-aware eviction ordering.
- Tests prove ordering, tie-breaking, missing row rejection, duplicate row rejection, and byte aggregation overflow rejection.
- Docs, plan registry, manifest fragments, composed manifest, and static checks describe the implemented value-only scope and remaining non-claims.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No GPU residency enforcement, allocator enforcement, or RHI memory mutation.
- No renderer/RHI/native handle access.
- No DirectStorage native queue/status/fence/file IO execution.
- No sustained async-overlap or performance benchmark claim.
- No Vulkan/Metal parity, mesh shader readiness, Nanite compatibility/equivalence/superiority, or benchmark-exceeds claim.
