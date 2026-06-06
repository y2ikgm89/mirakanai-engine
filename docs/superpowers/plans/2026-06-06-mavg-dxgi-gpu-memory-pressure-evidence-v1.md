# MAVG DXGI GPU Memory Pressure Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert D3D12/DXGI video memory budget diagnostics into MAVG caller-supplied GPU memory pressure rows without exposing native handles or enforcing residency.

**Architecture:** Keep `MK_runtime` independent from RHI by adding a `MK_runtime_rhi` value adapter that consumes `rhi::RhiDeviceMemoryDiagnostics`, resident MAVG page mount rows, and caller-reviewed per-page GPU byte estimates. The adapter emits `RuntimeMavgPageStreamingGpuMemoryPressureRow` rows for the already implemented `caller_supplied_gpu_memory_pressure` eviction policy and records fail-closed diagnostics when D3D12/DXGI evidence or page byte evidence is incomplete.

**Tech Stack:** C++23, `MK_runtime_rhi`, `MK_runtime`, `MK_rhi`, `RhiDeviceMemoryDiagnostics`, `RuntimeMavgPageStreamingGpuMemoryPressureRow`, `MK_runtime_rhi_tests`, `MK_runtime_mavg_page_streaming_tests`, Microsoft Learn DXGI/D3D12 memory management docs, Context7 `/microsoft/directx-graphics-samples`.

---

**Date:** 2026-06-06

**Plan ID:** `mavg-dxgi-gpu-memory-pressure-evidence-v1`

**Status:** Active candidate stacked on `mavg-gpu-memory-pressure-eviction-policy-v1`.

## Context

PR #486 added value-only MAVG GPU memory pressure ordering, but the rows are still caller supplied. The D3D12 RHI already captures OS video memory budget and process usage through `RhiDeviceMemoryDiagnostics`; Microsoft documents `IDXGIAdapter3::QueryVideoMemoryInfo` as the API that reports current process budget and usage, and Direct3D 12 memory guidance recommends classify, budget, and stream. Context7 DirectX samples also point to explicit D3D12 residency/budget management as the app's responsibility.

This child connects those existing diagnostic values to MAVG as reviewed value rows only. It must not call DXGI outside the D3D12 backend, expose `IDXGIAdapter3` / `ID3D12Device` / `IRhiDevice` handles through runtime APIs, mutate residency, reserve memory, execute DirectStorage, or claim performance improvement.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_memory_pressure.hpp`
- Create: `engine/runtime_rhi/src/mavg_gpu_memory_pressure.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `tests/unit/runtime_rhi_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generated: `engine/agent/manifest.json` via `tools/compose-agent-manifest.ps1 -Write`
- Create or modify: focused MAVG static guard under `tools/check-ai-integration-12x-*.ps1`
- Modify if needed: `tools/check-json-contracts-040-agent-surfaces.ps1`

## API Shape

Add `mirakana::runtime_rhi` types:

```cpp
enum class RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    unsupported_backend,
    missing_dxgi_video_memory_budget,
    invalid_dxgi_video_memory_budget,
    invalid_resident_page_mount,
    duplicate_resident_page_mount,
    invalid_estimate_row,
    duplicate_estimate_row,
    missing_estimate_row,
    estimated_byte_overflow,
};

struct RuntimeMavgResidentPageGpuMemoryEstimateRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    std::uint64_t estimated_gpu_resident_bytes{0};
};

struct RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc {
    AssetId graph_asset;
    rhi::BackendKind backend{rhi::BackendKind::null};
    rhi::RhiDeviceMemoryDiagnostics memory;
    std::span<const runtime::RuntimeMavgResidentPageMountRow> resident_page_mounts;
    std::span<const RuntimeMavgResidentPageGpuMemoryEstimateRow> estimated_pages;
};
```

Add result fields for output rows, diagnostics, budget/usage counters, estimated byte totals, row counts, and explicit side-effect flags:

- `used_dxgi_video_memory_budget_evidence`
- `produced_gpu_memory_pressure_rows`
- `invoked_file_io = false`
- `mutated_mount_set = false`
- `touched_renderer_or_rhi_handles = false`
- `enforced_gpu_residency = false`
- `reserved_video_memory = false`

Add function:

```cpp
[[nodiscard]] RuntimeMavgDxgiGpuMemoryPressureEvidenceResult
build_runtime_mavg_dxgi_gpu_memory_pressure_rows(const RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc& desc);
```

Pressure score rule:

- Require `backend == rhi::BackendKind::d3d12`.
- Require `memory.os_video_memory_budget_available`.
- Require `memory.local_video_memory_budget_bytes > 0`.
- Compute one global integer pressure score from local usage over local budget without multiplication overflow.
- Emit one pressure row per resident page mount using the matching estimate row.
- Keep the pressure score equal for all rows from the same diagnostic snapshot; existing MAVG eviction ordering then uses `estimated_gpu_resident_bytes` as the tie breaker.
- Sort output rows by page index then mount id for deterministic evidence.

## Implementation

- [x] **Step 1: Add RED tests for DXGI pressure row generation.**

Add tests to `tests/unit/runtime_rhi_tests.cpp`:

```cpp
MK_TEST("runtime rhi mavg dxgi gpu memory pressure builds rows from d3d12 budget evidence")
MK_TEST("runtime rhi mavg dxgi gpu memory pressure rows feed mavg eviction ordering")
```

Expected RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests
```

Expected failure: `mirakana/runtime_rhi/mavg_gpu_memory_pressure.hpp` and `build_runtime_mavg_dxgi_gpu_memory_pressure_rows` do not exist.

- [x] **Step 2: Add RED tests for fail-closed evidence.**

Add tests:

```cpp
MK_TEST("runtime rhi mavg dxgi gpu memory pressure requires d3d12 dxgi budget evidence")
MK_TEST("runtime rhi mavg dxgi gpu memory pressure rejects missing estimate rows")
MK_TEST("runtime rhi mavg dxgi gpu memory pressure rejects duplicate estimate rows")
MK_TEST("runtime rhi mavg dxgi gpu memory pressure rejects estimated byte overflow")
```

- [x] **Step 3: Implement `mavg_gpu_memory_pressure` value API.**

Create header/source in `engine/runtime_rhi`, add the source to `engine/runtime_rhi/CMakeLists.txt`, and include only public `MK_runtime`, `MK_rhi`, and standard-library headers. Do not include `dxgi*.h`, `d3d12*.h`, or backend-private headers.

- [x] **Step 4: Strengthen D3D12 diagnostics test without host flakiness.**

Update `tests/unit/d3d12_rhi_tests.cpp` so the existing memory diagnostics test asserts:

```cpp
if (mem.os_video_memory_budget_available) {
    MK_REQUIRE(mem.local_video_memory_budget_bytes > 0);
}
```

Do not require DXGI budget availability on hosts where the OS/API path does not report it.

- [x] **Step 5: Run focused GREEN.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_d3d12_rhi_tests
```

- [x] **Step 6: Update docs, manifest fragments, composed manifest, and static checks.**

Claim only selected D3D12/DXGI value evidence:

- `RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc`
- `RuntimeMavgResidentPageGpuMemoryEstimateRow`
- `build_runtime_mavg_dxgi_gpu_memory_pressure_rows`
- `used_dxgi_video_memory_budget_evidence`
- output `RuntimeMavgPageStreamingGpuMemoryPressureRow` integration
- no native handle exposure
- no residency or allocator enforcement
- no DirectStorage queue/file IO
- no async-overlap/performance or Nanite claim

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] **Step 7: Run focused/static/full validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_rhi/src/mavg_gpu_memory_pressure.cpp,tests/unit/runtime_rhi_tests.cpp,tests/unit/d3d12_rhi_tests.cpp -ReuseExistingFileApiReply
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 8: Publish stacked PR.**

Run publication preflight, commit the candidate, push `codex/mavg-dxgi-gpu-memory-pressure-evidence-v1`, and create a draft PR over `codex/mavg-gpu-memory-pressure-eviction-policy-v1`.

## Validation Evidence

- RED confirmed before implementation:
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests`
  failed because `mirakana/runtime_rhi/mavg_gpu_memory_pressure.hpp` did not exist.
- Focused GREEN:
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` passed.
- Focused CTest:
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` passed: 1/1 test passed.
- D3D12 diagnostics build:
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests` passed.
- D3D12 diagnostics CTest:
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_d3d12_rhi_tests` passed: 1/1 test passed.
- Manifest composition:
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` passed and rewrote `engine/agent/manifest.json`.
- Static/public/API checks:
  `tools/check-public-api-boundaries.ps1`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, targeted `tools/check-tidy.ps1 -Files engine/runtime_rhi/src/mavg_gpu_memory_pressure.cpp,tests/unit/runtime_rhi_tests.cpp,tests/unit/d3d12_rhi_tests.cpp -ReuseExistingFileApiReply`, and `git diff --check` passed.
- Full validation:
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Static checks passed; build passed; `ctest` passed 109/109 tests, including `MK_runtime_rhi_tests` and `MK_d3d12_rhi_tests`.
- Publication:
  Created commit `f9fff3ba` (`Add MAVG DXGI GPU memory pressure evidence`), pushed `codex/mavg-dxgi-gpu-memory-pressure-evidence-v1`, and opened draft PR `https://github.com/y2ikgm89/mirakanai-engine/pull/488` over `codex/mavg-gpu-memory-pressure-eviction-policy-v1`.

## Done When

- `MK_runtime_rhi` exposes a value-only adapter from D3D12/DXGI memory diagnostics to MAVG GPU pressure rows.
- Tests prove row generation, integration with existing MAVG pressure ordering, missing evidence rejection, duplicate rejection, overflow rejection, and host-safe D3D12 diagnostics.
- Docs, plan registry, manifest fragments, composed manifest, and static checks describe the selected D3D12/DXGI value-evidence scope and remaining non-claims.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No `IDXGIAdapter3`, `ID3D12Device`, `IRhiDevice`, RHI handle, or backend-native object exposure through runtime/game APIs.
- No `SetVideoMemoryReservation`, residency mutation, real GPU residency enforcement, allocator enforcement, GPU budget enforcement, or live mount mutation.
- No DirectStorage native queue/status/fence/file IO execution.
- No sustained async-overlap or performance benchmark claim.
- No Vulkan/Metal parity, mesh shader readiness, Nanite compatibility/equivalence/superiority, or benchmark-exceeds claim.
