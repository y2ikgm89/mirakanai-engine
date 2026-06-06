# MAVG GPU Visibility Buffer Layout v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a backend-neutral MAVG GPU visibility-buffer layout planner that turns validated visible cluster packet rows into deterministic slot, byte-range, and synchronization-intent rows for later GPU allocation/write work.

**Architecture:** Keep this as a value-only `MK_renderer` planning surface. The helper consumes a successful `MavgGpuVisibleClusterPacketPlan`, validates packet order/count/budget constraints, emits one logical visibility-buffer slot per packet, and records backend-neutral write-to-read synchronization intent without allocating an RHI buffer or writing GPU memory.

**Tech Stack:** C++23, `MK_renderer`, CMake/CTest, `tools/validate.ps1`, Context7 CMake docs, Microsoft Direct3D 12 resource-barrier/UAV documentation, Microsoft D3D12 indirect drawing documentation, Khronos Vulkan buffer-usage and synchronization documentation.

---

**Plan ID:** `mavg-gpu-visibility-buffer-layout-v1`

**Date:** 2026-06-06

**Base:** Stacked after MAVG GPU Visible Cluster Packet Rows v1 / draft PR #509 on branch `codex/mavg-gpu-visible-cluster-packets-v1`.

## Context

- MAVG GPU Visible Cluster Packet Rows v1 produces deterministic value-only visible packet rows from successful cluster format and culling plans.
- The MAVG master plan still lists no allocated/written GPU visibility buffer and no executed GPU cluster traversal.
- Microsoft D3D12 documentation separates resource binding, UAV/resource-barrier synchronization, and indirect-argument execution from application-level planning. This child records only layout and backend-neutral synchronization intent.
- Khronos Vulkan documentation requires storage/indirect usage flags and explicit synchronization when shader writes feed later reads. This child records only the backend-neutral need for a later write-to-read dependency row.

## Non-Claims

- No RHI buffer allocation.
- No GPU visibility-buffer write.
- No GPU command submission.
- No GPU traversal execution.
- No mesh shader execution.
- No indirect draw execution.
- No GPU decompression.
- No native handle exposure.
- No allocator/GPU budget enforcement.
- No Vulkan/Metal parity claim.
- No Nanite compatibility/equivalence/superiority or benchmark superiority claim.

## Files

- Create: `engine/renderer/include/mirakana/renderer/mavg_gpu_visibility_buffer_layout.hpp`
- Create: `engine/renderer/src/mavg_gpu_visibility_buffer_layout.cpp`
- Create: `tests/unit/mavg_gpu_visibility_buffer_layout_tests.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify after compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-136-mavg-gpu-visibility-buffer-layout.ps1`

## Task 1: RED Test

- [x] **Step 1: Add `tests/unit/mavg_gpu_visibility_buffer_layout_tests.cpp` with successful layout tests**

  Test expectations:
  - `plan_mavg_gpu_visibility_buffer_layout` succeeds for a successful `MavgGpuVisibleClusterPacketPlan`.
  - Slot count equals packet count.
  - Slot order follows packet order.
  - Each slot carries graph asset, slot index, packet index, indirect command index, cluster index, page index, LOD, material partition, fallback substitution, meshlet readiness, packet payload byte offset/size, slot byte offset, slot byte size, and a stable packed cluster key.
  - Result counters report source packet count, slot count, byte range count, layout row count, sync intent row count, meshlet-ready slot count, fallback slot count, slot record stride bytes, slot record alignment bytes, slot buffer byte size, and packet payload byte span.
  - Result flags remain false for `allocated_rhi_resources`, `wrote_gpu_visibility_buffer`, `submitted_gpu_work`, `executed_gpu_traversal`, `executed_mesh_shader`, `executed_indirect_draw`, `used_gpu_decompression`, `touched_native_handles`, `claimed_vulkan_parity`, `claimed_metal_parity`, and `claimed_nanite_compatibility`.

- [x] **Step 2: Add fail-closed tests**

  Test expectations:
  - Missing packet plan fails with `missing_visible_cluster_packet_plan`.
  - Invalid packet plan fails with `invalid_visible_cluster_packet_plan`.
  - `packet_count` that disagrees with `packets.size()` fails with `source_packet_count_mismatch`.
  - Duplicate packet index fails with `duplicate_packet_index`.
  - Packet index not matching its slot order fails with `non_dense_packet_index`.
  - Row stride smaller than `MavgGpuVisibilityBufferLayoutDesc::minimum_slot_record_stride_bytes` fails with `invalid_slot_stride`.
  - Non-power-of-two or zero alignment fails with `invalid_slot_alignment`.
  - `require_meshlet_ready_slots=true` fails with `meshlet_not_ready` when a packet is outside meshlet limits.
  - `max_slot_count` overflow fails with `max_slot_count_exceeded`.
  - Zero packets in a successful packet plan succeeds with zero slots and zero buffer size.

- [x] **Step 3: Wire `MK_mavg_gpu_visibility_buffer_layout_tests` in root `CMakeLists.txt` and run RED**

  Command:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visibility_buffer_layout_tests
  ```

  Expected RED:

  ```text
  cannot open include file 'mirakana/renderer/mavg_gpu_visibility_buffer_layout.hpp'
  ```

## Task 2: GREEN Implementation

- [x] **Step 1: Add public API in `mavg_gpu_visibility_buffer_layout.hpp`**

  Public types:
  - `MavgGpuVisibilityBufferLayoutDiagnosticCode`
  - `MavgGpuVisibilityBufferLayoutDiagnostic`
  - `MavgGpuVisibilityBufferLayoutDesc`
  - `MavgGpuVisibilityBufferSlotRow`
  - `MavgGpuVisibilityBufferByteRangeRow`
  - `MavgGpuVisibilityBufferLayoutRow`
  - `MavgGpuVisibilityBufferSyncIntentRow`
  - `MavgGpuVisibilityBufferLayoutPlan`
  - `plan_mavg_gpu_visibility_buffer_layout`
  - `has_mavg_gpu_visibility_buffer_layout_diagnostic`

- [x] **Step 2: Implement validation and row layout in `mavg_gpu_visibility_buffer_layout.cpp`**

  Required behavior:
  - Require a non-null and successful packet plan.
  - Require `packet_plan.packet_count == packet_plan.packets.size()`.
  - Reject duplicate packet indexes.
  - Reject packet indexes that do not match their packet vector order.
  - Validate row stride and alignment.
  - Align every slot offset to `slot_record_alignment_bytes`.
  - Compute `slot_buffer_size_bytes` from the last slot end.
  - Reject packet counts above `max_slot_count` when non-zero.
  - Preserve packet order and copy packet identity fields.
  - Compute a deterministic 64-bit `cluster_key` from graph asset, cluster index, page index, LOD, material partition, and packet index.
  - Emit one backend-neutral `MavgGpuVisibilityBufferSyncIntentRow` for `visibility_buffer_write` to `visibility_resolve_read` when `emit_sync_intent_rows=true`.
  - Fail closed by clearing slots and zeroing counters on diagnostics.

- [x] **Step 3: Add source file to `engine/renderer/CMakeLists.txt`**

- [x] **Step 4: Run GREEN target build and CTest**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visibility_buffer_layout_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_gpu_visibility_buffer_layout_tests
  ```

## Task 3: Related Focused Validation

- [x] **Step 1: Build and run adjacent MAVG renderer tests**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visibility_buffer_layout_tests MK_mavg_gpu_visible_cluster_packets_tests MK_mavg_gpu_cluster_format_tests MK_mavg_gpu_culling_tests MK_mavg_lod_selection_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_gpu_visibility_buffer_layout_tests|MK_mavg_gpu_visible_cluster_packets_tests|MK_mavg_gpu_cluster_format_tests|MK_mavg_gpu_culling_tests|MK_mavg_lod_selection_tests"
  ```

## Task 4: Docs, Manifest, Static Guard

- [x] **Step 1: Update capability and roadmap docs**

  Record that MAVG GPU Visibility Buffer Layout v1 turns visible packet rows into deterministic logical visibility-buffer slot, byte-range, layout metadata, and sync-intent rows without allocating/writing GPU buffers, executing traversal, executing mesh shaders, executing indirect draws, GPU decompression, Vulkan/Metal parity, or Nanite claims.

- [x] **Step 2: Update MAVG master plan and plan registry**

  Add this child after MAVG GPU Visible Cluster Packet Rows v1 and keep `currentActivePlan` at the production master selection gate with `recommendedNextPlan.id = next-production-gap-selection`.

- [x] **Step 3: Update manifest fragments and compose**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
  ```

- [x] **Step 4: Add static guard**

  `tools/check-ai-integration-136-mavg-gpu-visibility-buffer-layout.ps1` must assert:
  - header/source/test file existence
  - public API names
  - diagnostics and fail-closed needles
  - backend-neutral sync intent needles
  - side-effect false claims
  - docs/manifest mention this child

- [x] **Step 5: Run agent-surface checks**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
  ```

## Task 5: Final Validation And Publication

- [x] **Step 1: Run full validation**

  Command:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
  ```

- [x] **Step 2: Run publication preflight**

  Command:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
  ```

- [x] **Step 3: Commit, push, and open stacked draft PR**

  Base branch:

  ```text
  codex/mavg-gpu-visible-cluster-packets-v1
  ```

  Head branch:

  ```text
  codex/mavg-gpu-visibility-buffer-layout-v1
  ```

  Commit message:

  ```text
  Add MAVG GPU visibility buffer layout rows
  ```

## Validation Evidence

| Check | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visibility_buffer_layout_tests` before implementation | RED: missing `mirakana/renderer/mavg_gpu_visibility_buffer_layout.hpp` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visibility_buffer_layout_tests` | Passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_gpu_visibility_buffer_layout_tests` | Passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visibility_buffer_layout_tests MK_mavg_gpu_visible_cluster_packets_tests MK_mavg_gpu_cluster_format_tests MK_mavg_gpu_culling_tests MK_mavg_lod_selection_tests` | Passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_gpu_visibility_buffer_layout_tests|MK_mavg_gpu_visible_cluster_packets_tests|MK_mavg_gpu_cluster_format_tests|MK_mavg_gpu_culling_tests|MK_mavg_lod_selection_tests"` | Passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` | Passed before staging and again clean before push |
| Stacked draft PR | Published as draft PR #510 with base `codex/mavg-gpu-visible-cluster-packets-v1` |

Published stacked draft PR #510.
