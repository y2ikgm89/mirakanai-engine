# Allocator And Residency Diagnostics v1 Implementation Plan (2026-05-03)

> **For agentic workers:** Phase 3 master child **`allocator-and-residency-diagnostics-v1`**. Surfaces **read-only** OS and engine-side memory signals through `IRhiDevice` without exposing DXGI/Vulkan handles or implying allocator enforcement.

**Goal:** Add `IRhiDevice::memory_diagnostics()` returning `RhiDeviceMemoryDiagnostics` with optional **DXGI `QueryVideoMemoryInfo`** local/non-local budget and usage (D3D12), plus a **best-effort committed byte estimate** from `ID3D12Device::GetResourceAllocationInfo` over committed resources (D3D12) or summed buffer/texture descriptor footprints for active resources (null/Vulkan). No eviction, no budgets as contracts, and no `VK_EXT_memory_budget` wiring in this slice.

**Architecture:** `mirakana/rhi/memory_diagnostics.hpp` holds the POD snapshot. `DeviceContext::memory_diagnostics` resolves the DXGI adapter via `IDXGIDevice::GetAdapter` → `IDXGIAdapter3`. Null and Vulkan share descriptor-sum logic aligned with existing `bytes_per_texel` rules.

**Tech Stack:** `engine/rhi/include/mirakana/rhi/rhi.hpp`, `engine/rhi/include/mirakana/rhi/memory_diagnostics.hpp`, `engine/rhi/src/null_rhi.cpp`, `engine/rhi/d3d12/*`, `engine/rhi/vulkan/src/vulkan_backend.cpp`, `tests/unit/rhi_tests.cpp`, `tests/unit/d3d12_rhi_tests.cpp`, test doubles implementing `IRhiDevice`.

---

## Phases

### Phase v1（本スライス・完了）

- [x] `RhiDeviceMemoryDiagnostics` + `IRhiDevice::memory_diagnostics()`.
- [x] Null / Vulkan descriptor-byte estimates for active buffers/textures.
- [x] D3D12 DXGI video memory + allocation info sum.
- [x] `rhi_tests` / `d3d12_rhi_tests` + `IRhiDevice` mock updates.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` evidence.

### Follow-ups

- Vulkan: `VK_EXT_memory_budget` / heap budgeting when extension is enabled on the logical device.
- D3D12: separate committed default vs upload vs readback heaps in diagnostics rows.

## Validation Evidence

| Command | Result | Date |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok`、CTest 28/28 Passed | 2026-05-03 |
| `ctest -C Debug -R mirakana_rhi_tests` | Passed | 2026-05-03 |
| `ctest -C Debug -R mirakana_d3d12_rhi_tests` | Passed | 2026-05-03 |

## Done When

- [x] All `IRhiDevice` implementations return a consistent snapshot shape.
- [x] Null tests assert deterministic descriptor sums and `os_video_memory_budget_available == false`.

## Non-Goals

- GPU heap allocators, D3D12 residency management APIs, or runtime enforcement of memory caps.

---

*Completed: 2026-05-03.*
