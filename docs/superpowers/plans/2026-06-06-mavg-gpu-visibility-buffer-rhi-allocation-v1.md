# MAVG GPU Visibility Buffer RHI Allocation v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Allocate a first-party RHI buffer for a successful MAVG GPU visibility-buffer layout without writing, binding, or traversing it.

**Architecture:** Keep the existing `MK_renderer` visibility-buffer layout as the value-only source of truth and add a narrow `MK_runtime_rhi` execution adapter. The adapter validates a successful `MavgGpuVisibilityBufferLayoutPlan`, validates requested buffer usage and byte budgets, calls the existing `rhi::IRhiDevice::create_buffer`, and records allocation/readback-readiness rows while preserving explicit non-claims for GPU writes, descriptors, barriers, traversal, mesh shaders, indirect draws, native handles, and performance.

**Tech Stack:** C++23, `MK_runtime_rhi`, `MK_renderer`, `MK_rhi`, Null RHI, CMake/CTest, Context7 CMake and Vulkan docs, Microsoft Direct3D 12 resource-barrier/UAV documentation, Khronos Vulkan buffer-usage and synchronization documentation.

---

**Plan ID:** `mavg-gpu-visibility-buffer-rhi-allocation-v1`

**Date:** 2026-06-06

**Base:** Stacked after MAVG GPU Visibility Buffer Layout v1 / draft PR #510 on branch `codex/mavg-gpu-visibility-buffer-layout-v1`.

## Context

- MAVG GPU Visibility Buffer Layout v1 emits deterministic logical slot, byte-range, layout metadata, and backend-neutral write-to-read sync-intent rows.
- The next smallest implementation boundary is RHI allocation only: a storage/readback-capable `rhi::BufferHandle` can be created through the already-reviewed `IRhiDevice::create_buffer` surface.
- Microsoft D3D12 resource-barrier and UAV documentation keeps resource synchronization/write ordering separate from resource allocation. This child therefore does not record UAV barriers or dispatches.
- Khronos Vulkan buffer usage and synchronization docs require storage-buffer usage and explicit barriers when shader writes feed later reads. This child only allocates a storage/copy-source-capable buffer for later proof.

## Non-Claims

- No GPU visibility-buffer write.
- No descriptor set, UAV/SRV, or storage-buffer descriptor creation.
- No shader, compute pipeline, dispatch, or command submission.
- No UAV barrier, resource transition, or Vulkan pipeline barrier execution.
- No GPU traversal, mesh shader execution, or indirect draw execution.
- No DirectStorage, D3D12, Vulkan, Metal, COM, or native handle exposure.
- No allocator/GPU budget enforcement.
- No Vulkan/Metal parity claim.
- No Nanite compatibility/equivalence/superiority or benchmark superiority claim.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_visibility_buffer_resource.hpp`
- Create: `engine/runtime_rhi/src/mavg_gpu_visibility_buffer_resource.cpp`
- Create: `tests/unit/runtime_rhi_mavg_gpu_visibility_buffer_resource_tests.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify after compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-137-mavg-gpu-visibility-buffer-rhi-allocation.ps1`

## Task 1: RED Tests

- [x] **Step 1: Add `tests/unit/runtime_rhi_mavg_gpu_visibility_buffer_resource_tests.cpp`**

  Test expectations:
  - A successful layout plan with two slots allocates one Null RHI buffer through `create_runtime_mavg_gpu_visibility_buffer_resource`.
  - Result exposes one `RuntimeMavgGpuVisibilityBufferResourceRow` with non-zero `rhi::BufferHandle`, `rhi::BufferDesc::size_bytes == layout.slot_buffer_size_bytes`, `BufferUsage::storage`, and `BufferUsage::copy_source`.
  - Result counters report `layout_slot_count`, `allocated_buffer_count`, `allocated_buffer_size_bytes`, and `device_buffers_created_after - before == 1`.
  - Result flags report `allocated_rhi_resources=true`, `ready_for_readback_proof=true`, and all non-claim flags false.

- [x] **Step 2: Add fail-closed and no-op tests**

  Test expectations:
  - Missing layout plan fails with `missing_layout_plan`.
  - Invalid layout plan fails with `invalid_layout_plan`.
  - Zero-slot layout fails with `zero_slot_buffer_disallowed` by default and succeeds without allocation when `allow_zero_slot_buffer=true`.
  - Zero `slot_buffer_size_bytes` with positive slots fails with `invalid_slot_buffer_size`.
  - `max_buffer_size_bytes` smaller than the layout buffer size fails with `max_buffer_size_exceeded`.
  - Usage without `BufferUsage::storage` fails with `missing_storage_usage`.
  - Usage without `BufferUsage::copy_source` fails with `missing_copy_source_usage_for_readback` when `require_copy_source_for_readback=true`.
  - A device that returns an empty handle fails with `invalid_allocated_buffer_handle`.

- [x] **Step 3: Wire the test target and run RED**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests
  ```

  Expected RED:

  ```text
  cannot open include file 'mirakana/runtime_rhi/mavg_gpu_visibility_buffer_resource.hpp'
  ```

## Task 2: GREEN Implementation

- [x] **Step 1: Add public API**

  Public types and functions:
  - `RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode`
  - `RuntimeMavgGpuVisibilityBufferResourceDiagnostic`
  - `RuntimeMavgGpuVisibilityBufferResourceDesc`
  - `RuntimeMavgGpuVisibilityBufferResourceRow`
  - `RuntimeMavgGpuVisibilityBufferResourceResult`
  - `create_runtime_mavg_gpu_visibility_buffer_resource`
  - `has_runtime_mavg_gpu_visibility_buffer_resource_diagnostic`

- [x] **Step 2: Implement allocation-only behavior**

  Required behavior:
  - Require a non-null, successful `MavgGpuVisibilityBufferLayoutPlan`.
  - Require `slot_count == slots.size()` and `slot_buffer_size_bytes > 0` when slots are present.
  - Allow zero-slot no-op only when `allow_zero_slot_buffer=true`.
  - Validate `max_buffer_size_bytes` when non-zero.
  - Validate `BufferUsage::storage`.
  - Validate `BufferUsage::copy_source` when `require_copy_source_for_readback=true`.
  - Call `IRhiDevice::create_buffer` exactly once for a non-empty layout.
  - Reject a zero returned handle.
  - Preserve all write/traversal/descriptor/native/performance non-claim flags.

- [x] **Step 3: Add source and CMake wiring**

  - Add source to `engine/runtime_rhi/CMakeLists.txt`.
  - Add `MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests` to root `CMakeLists.txt`.

- [x] **Step 4: Run GREEN target build and CTest**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests
  ```

## Task 3: Adjacent Focused Validation

- [x] **Step 1: Build and test adjacent MAVG handoff targets**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests MK_mavg_gpu_visibility_buffer_layout_tests MK_mavg_gpu_visible_cluster_packets_tests MK_mavg_gpu_cluster_format_tests MK_mavg_gpu_culling_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests|MK_mavg_gpu_visibility_buffer_layout_tests|MK_mavg_gpu_visible_cluster_packets_tests|MK_mavg_gpu_cluster_format_tests|MK_mavg_gpu_culling_tests"
  ```

## Task 4: Docs, Manifest, Static Guard

- [x] **Step 1: Update current capability docs and roadmap**

  Record that MAVG GPU Visibility Buffer RHI Allocation v1 turns a successful logical layout into a caller-visible first-party `rhi::BufferHandle` allocation through `IRhiDevice::create_buffer`, with storage/copy-source usage and readback-proof readiness, but without write/descriptor/barrier/traversal execution.

- [x] **Step 2: Update MAVG spec, master plan, and plan registry**

  Add this child after MAVG GPU Visibility Buffer Layout v1. Keep `currentActivePlan` at the production master selection gate and keep `unsupportedProductionGaps = []`.

- [x] **Step 3: Update manifest fragments and compose**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
  ```

- [x] **Step 4: Add static guard**

  `tools/check-ai-integration-137-mavg-gpu-visibility-buffer-rhi-allocation.ps1` must assert:
  - header/source/test/plan file existence
  - public API names
  - diagnostic names
  - `IRhiDevice::create_buffer`, `BufferUsage::storage`, and `BufferUsage::copy_source` allocation evidence
  - non-claim flags for write, descriptors, barriers, traversal, mesh shaders, indirect draws, GPU decompression, native handles, Vulkan/Metal parity, Nanite, and benchmark superiority
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

- [ ] **Step 3: Commit, push, and open stacked draft PR**

  Base branch:

  ```text
  codex/mavg-gpu-visibility-buffer-layout-v1
  ```

  Head branch:

  ```text
  codex/mavg-gpu-visibility-buffer-rhi-allocation-v1
  ```

  Commit message:

  ```text
  Add MAVG visibility buffer RHI allocation
  ```

## Validation Evidence

| Check | Result |
| --- | --- |
| RED build for `MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests` before public API | Failed as expected on missing `mirakana/runtime_rhi/mavg_gpu_visibility_buffer_resource.hpp`. |
| `tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests` | Passed. |
| `tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests` | Passed. |
| Adjacent MAVG target build for `MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests`, `MK_mavg_gpu_visibility_buffer_layout_tests`, `MK_mavg_gpu_visible_cluster_packets_tests`, `MK_mavg_gpu_cluster_format_tests`, `MK_mavg_gpu_culling_tests` | Passed. |
| Adjacent MAVG CTest regex for the same five targets | Passed: 5/5 tests. |
| `tools/compose-agent-manifest.ps1 -Write` | Passed. |
| `tools/check-json-contracts.ps1` | Passed. |
| `tools/check-ai-integration.ps1` | Passed. |
| `tools/check-public-api-boundaries.ps1` | Passed. |
| `tools/check-format.ps1` after `tools/format.ps1` | Passed. |
| `tools/validate.ps1` | Passed. Diagnostic-only host gates remain for Apple/Metal tooling on this Windows host. |
| `tools/check-publication-preflight.ps1` | Passed for `codex/mavg-gpu-visibility-buffer-rhi-allocation-v1`. |

## Done When

- The focused target and adjacent MAVG tests pass.
- Docs, plan registry, master plan/spec, manifest fragments, composed manifest, and static guard agree on the exact claim.
- Full `tools/validate.ps1` passes or any host-gated blocker is recorded.
- A validated commit is pushed and a stacked draft PR is opened.
