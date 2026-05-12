# GPU Marker And Timestamp Adapter v1 Implementation Plan (2026-05-03)

> **For agentic workers:** Phase 3 master child `**gpu-marker-and-timestamp-adapter-v1`**. Exposes **backend-neutral** GPU debug labels and timestamp frequency without leaking native handles through public game APIs.

**Goal:** Add `IRhiCommandList` hooks for nested GPU debug regions and one-shot markers, centralize **printable ASCII** label validation in `mirakana/rhi/gpu_debug.hpp`, extend `RhiStats` with counters, expose `IRhiDevice::gpu_timestamp_ticks_per_second()` (D3D12 primary graphics queue via `ID3D12CommandQueue::GetTimestampFrequency`, `0` on null and Vulkan until timestamp queries are wired), and keep **Vulkan** on counter-only markers until `VK_EXT_debug_utils` is chartered.

**Architecture:** `NullRhiDevice` tracks scope depth for balanced `close()` validation. `mirakana::rhi::d3d12::DeviceContext` records per-command-list `gpu_debug_scope_depth` alongside `ID3D12GraphicsCommandList` and unwinds `EndEvent` when a `D3d12RhiCommandList` is destroyed without `close()`. D3D12 uses `D3D12_EVENT_METADATA_STRING` with UTF-8–compatible ASCII payloads per Microsoft guidance for `BeginEvent` / `SetMarker`.

**Tech Stack:** `engine/rhi/include/mirakana/rhi/rhi.hpp`, `engine/rhi/include/mirakana/rhi/gpu_debug.hpp`, `engine/rhi/src/null_rhi.cpp`, `engine/rhi/d3d12/`*, `engine/rhi/vulkan/src/vulkan_backend.cpp`, `tests/unit/rhi_tests.cpp`, renderer/runtime test doubles that implement `IRhiDevice` / `IRhiCommandList`.

---

## Phases

### Phase v1（本スライス・完了）

- `gpu_debug.hpp` with `max_rhi_debug_label_bytes`, `is_valid_rhi_debug_label`, `validate_rhi_debug_label`.
- `IRhiCommandList` pure virtual `begin_gpu_debug_scope` / `end_gpu_debug_scope` / `insert_gpu_debug_marker`.
- `IRhiDevice::gpu_timestamp_ticks_per_second` pure virtual + `NullRhiDevice` / `D3d12RhiDevice` / `VulkanRhiDevice` implementations.
- `RhiStats` fields `gpu_debug_scopes_begun`, `gpu_debug_scopes_ended`, `gpu_debug_markers_inserted`.
- D3D12 `DeviceContext` helpers + `CommandListRecord::gpu_debug_scope_depth` + reset clearing + destructor unwind.
- `rhi_tests` and mock `IRhiDevice` / `IRhiCommandList` updates.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` evidence.

### Follow-ups（別子計画）

- Vulkan: optional `VK_EXT_debug_utils` command-buffer labels when extension + functions are available.
- Timestamp queries: expose resolution per queue family and wire Vulkan query pools.

## Validation Evidence


| Command                          | Result                            | Date       |
| -------------------------------- | --------------------------------- | ---------- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`               | `validate: ok`、CTest 28/28 Passed | 2026-05-03 |
| `ctest -C Debug -R mirakana_rhi_tests` | Passed                            | 2026-05-03 |


## Done When

- All `IRhiCommandList` / `IRhiDevice` implementations compile with the new API surface.
- Null backend enforces label rules, balanced scopes on `close`, and deterministic stats.
- D3D12 records PIX-friendly markers for graphics command lists created through `DeviceContext`.

## Non-Goals

- Public exposure of `ID3D12GraphicsCommandList`, `VkCommandBuffer`, or Metal encoders.
- Allocator / residency diagnostics (separate master child).

---

*Completed: 2026-05-03.*
