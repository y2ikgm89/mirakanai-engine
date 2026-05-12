# D3D12 Production Renderer Hardening v1 Implementation Plan (2026-05-03)

> **For agentic workers:** Phase 3 master child **`d3d12-production-renderer-hardening-v1`**. Applies **Microsoft-recommended** `ID3D12Object::SetName` wide-string labels across the primary D3D12 `DeviceContext` ownership path so PIX, GPU-based validation, and post-mortem tooling can list **device, queues, fence, committed buffers/textures, swapchain RTVs and back buffers, descriptor heaps, root signatures, graphics PSOs, command allocators/lists**, plus **bootstrap proof** committed resources—without changing public RHI handles, fence semantics, or barrier contracts.

**Goal:** Align the Windows D3D12 backend with official debugging/diagnostics practice: every long-lived `ID3D12DeviceChild` created on the production `DeviceContext` path receives a stable `GameEngine.RHI.D3D12.*` name keyed by handle ordinal or swapchain id.

**Architecture:** Helpers `set_d3d12_object_name` / `set_d3d12_object_name_fmt` live in the existing anonymous namespace in `d3d12_backend.cpp` (UTF-16 format strings only; no new public headers). `SwapchainRecord` stores `native_handle_value` (matches `NativeSwapchainHandle::value`) so back-buffer and RTV heap names stay stable across resize.

**Tech Stack:** `engine/rhi/d3d12/src/d3d12_backend.cpp`, `tests/unit/d3d12_rhi_tests.cpp` (implicit regression via existing suite), `engine/agent/manifest.json` `currentD3d12`, master plan + plan registry.

---

## Phases

### Phase v1（本スライス・完了）

- [x] `SetName` on device bootstrap (`DeviceContext::create`), lazy compute/copy queues, committed buffers/textures, swapchain buffers + RTV heap, descriptor heaps, root signatures, graphics PSOs, command lists + allocators, `bootstrap_resource_ownership` proof resources.
- [x] `SwapchainRecord::native_handle_value` for deterministic swapchain-scoped names.
- [x] Manifest `currentD3d12` sync; master plan Phase 3 checkbox; `recommendedNextPlan` → Vulkan promotion child.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` evidence.

### Follow-ups

- Optional `SetName` on per-texture RTV/DSV heaps and DXGI swapchain COM diagnostics where APIs allow; align `bootstrap_device` standalone path if it gains ComPtr ownership for tests.

## Validation Evidence

| Command | Result | Date |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok` | 2026-05-03 |
| `ctest -C Debug` (preset `dev`, 28 tests) | 100% passed | 2026-05-03 |

## Done When

- [x] D3D12 `DeviceContext` creates named `ID3D12Object` children on the paths exercised by `mirakana_d3d12_rhi_tests` and desktop package smoke.
- [x] Swapchain resize refreshes buffers that are still named with the same swapchain id.

## Non-Goals

- Vulkan/Metal `SetDebugUtilsObjectNameEXT` parity (separate host-gated children).
- Public API for reading names from gameplay code.

---

*Completed: 2026-05-03.*
