# MAVG GPU Visibility Buffer RHI Write Readback v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove that successful MAVG visibility-buffer layout/resource rows can be encoded into deterministic bytes, copied through `MK_rhi`, and read back without claiming shader-generated visibility, descriptors, traversal, or Nanite parity.

**Architecture:** Keep `MK_renderer` layout rows as the value contract and keep the existing `MK_runtime_rhi` allocation row as the visibility-buffer ownership contract. Add a narrow `MK_runtime_rhi` adapter that creates an upload buffer, copies deterministic 64-byte records into the allocated visibility buffer, copies the visibility buffer into a readback buffer, waits for the submitted copy work, and compares readback bytes against the encoded record bytes. The adapter requires `BufferUsage::storage | BufferUsage::copy_source | BufferUsage::copy_destination` for the visibility buffer resource row, uses only first-party `IRhiDevice` / `IRhiCommandList` APIs, and preserves explicit non-claims for descriptor bindings, compute pipelines, GPU traversal, mesh shaders, indirect draws, GPU decompression, allocator budgets, backend parity, native handles, benchmark superiority, and async-overlap/performance.

**Tech Stack:** C++23, `MK_runtime_rhi`, `MK_renderer`, `MK_rhi`, Null RHI, CMake/CTest, Context7 `/khronosgroup/vulkan-docs`, Context7 `/microsoft/directx-graphics-samples`, Microsoft Learn D3D12 resource barriers/readback heap documentation.

---

**Plan ID:** `mavg-gpu-visibility-buffer-rhi-write-readback-v1`

**Date:** 2026-06-06

**Base:** Stacked after MAVG GPU Visibility Buffer RHI Allocation v1 / draft PR #512 on branch `codex/mavg-gpu-visibility-buffer-rhi-allocation-v1`.

## Context

- MAVG GPU Visibility Buffer Layout v1 emits deterministic slot rows, byte ranges, and write-to-read sync intent rows, but it does not allocate or write GPU resources.
- MAVG GPU Visibility Buffer RHI Allocation v1 allocates exactly one first-party `rhi::BufferHandle` for successful non-empty layouts, but it intentionally leaves `wrote_gpu_visibility_buffer=false`.
- This child narrows only the next handoff: deterministic records are staged, copied into the allocated visibility buffer, copied out to a readback buffer, and compared.
- Context7 Vulkan synchronization examples confirm transfer writes must be made visible before shader/storage-buffer reads, and compute/storage writes require explicit barriers before later reads. This child does not consume the buffer from shaders, so it records copy/readback proof but not a shader-read barrier claim.
- Microsoft Learn D3D12 readback guidance requires readback heaps/buffers and GPU completion synchronization before CPU mapping. This child uses the repository `IRhiDevice::submit`/`wait` and `read_buffer` contract for the proof instead of exposing D3D12 fences or native resources.

## Non-Claims

- No compute-shader generated visibility buffer.
- No descriptor set, UAV/SRV, or storage-buffer descriptor creation.
- No compute pipeline creation or dispatch.
- No GPU traversal.
- No mesh shader execution.
- No indirect draw execution.
- No GPU decompression.
- No allocator/GPU budget enforcement.
- No DirectStorage/file IO execution.
- No async-overlap/performance claim.
- No D3D12/Vulkan/Metal native handle exposure.
- No Vulkan/Metal parity claim.
- No Nanite compatibility/equivalence/superiority or benchmark superiority claim.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_visibility_buffer_write_readback.hpp`
- Create: `engine/runtime_rhi/src/mavg_gpu_visibility_buffer_write_readback.cpp`
- Create: `tests/unit/runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify after compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-138-mavg-gpu-visibility-buffer-write-readback.ps1`

## Task 1: RED Tests

- [x] **Step 1: Add `tests/unit/runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests.cpp`**

  Test expectations:
  - A successful two-slot layout and allocation with `BufferUsage::storage | BufferUsage::copy_source | BufferUsage::copy_destination` writes deterministic records through `execute_runtime_mavg_gpu_visibility_buffer_write_readback`.
  - Result exposes one `RuntimeMavgGpuVisibilityBufferWriteReadbackRow` with non-zero upload, visibility, and readback buffers.
  - Result counters report `encoded_record_count=2`, `encoded_record_stride_bytes=64`, `encoded_byte_count=128`, `uploaded_byte_count=128`, `readback_byte_count=128`, `copy_command_count=2`, and `submitted_copy_work=true`.
  - Record bytes are little-endian and place `slot_index`, `packet_index`, `cluster_key`, `payload_page_byte_offset`, `payload_page_byte_size`, and flags at the offsets declared by `MavgGpuVisibilityBufferLayoutRow`.
  - `readback_bytes_match_encoded_records=true`, `wrote_gpu_visibility_buffer=true`, and all descriptor/compute/traversal/native/backend/performance non-claim flags remain false.

- [x] **Step 2: Add fail-closed tests**

  Test expectations:
  - Missing device fails with `missing_rhi_device`.
  - Missing layout fails with `missing_layout_plan`.
  - Invalid layout shape fails with `invalid_layout_plan`.
  - Missing resource result fails with `missing_resource_result`.
  - Invalid resource result fails with `invalid_resource_result`.
  - Missing allocated resource row fails with `missing_resource_row`.
  - Resource row with zero buffer handle fails with `invalid_visibility_buffer_handle`.
  - Visibility buffer usage without `copy_source` fails with `missing_copy_source_usage_for_readback_copy`.
  - Visibility buffer usage without `copy_destination` fails with `missing_copy_destination_usage_for_staged_write`.
  - Slot byte ranges outside the buffer fail with `slot_byte_range_overflow`.
  - A caller `max_write_size_bytes` smaller than the encoded byte count fails with `max_write_size_exceeded`.

- [x] **Step 3: Wire the test target and run RED**

  Command:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests
  ```

  Expected RED:

  ```text
  cannot open include file 'mirakana/runtime_rhi/mavg_gpu_visibility_buffer_write_readback.hpp'
  ```

## Task 2: GREEN Implementation

- [x] **Step 1: Add public API**

  Public types and functions:
  - `RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode`
  - `RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnostic`
  - `RuntimeMavgGpuVisibilityBufferWriteReadbackDesc`
  - `RuntimeMavgGpuVisibilityBufferWriteReadbackRow`
  - `RuntimeMavgGpuVisibilityBufferWriteReadbackResult`
  - `execute_runtime_mavg_gpu_visibility_buffer_write_readback`
  - `has_runtime_mavg_gpu_visibility_buffer_write_readback_diagnostic`

- [x] **Step 2: Implement deterministic record packing**

  Required behavior:
  - Pack each `MavgGpuVisibilityBufferSlotRow` into `slot_record_stride_bytes` bytes.
  - Use little-endian writes for 32-bit and 64-bit fields.
  - Use `MavgGpuVisibilityBufferLayoutRow` field offsets for `slot_index`, `packet_index`, `cluster_key`, `payload_page_byte_offset`, `payload_page_byte_size`, and `flags`.
  - Flags bit 0 records `fallback_substitution`; bit 1 records `meshlet_ready`.
  - Reject field writes that exceed the slot stride or slot byte range.

- [x] **Step 3: Implement RHI staged copy/readback proof**

  Required behavior:
  - Require a successful layout plan with matching row counts.
  - Require a successful resource result with one valid resource row.
  - Require visibility-buffer usage `storage | copy_source | copy_destination`.
  - Create upload buffer with `BufferUsage::copy_source`.
  - Create readback buffer with `BufferUsage::copy_destination`.
  - Write encoded bytes into the upload buffer with `IRhiDevice::write_buffer`.
  - Record two copy commands: upload buffer to visibility buffer, then visibility buffer to readback buffer.
  - Submit the command list and wait for the returned fence.
  - Read bytes from the readback buffer and compare them to encoded bytes.
  - Preserve non-claim flags for descriptors, compute, traversal, mesh shaders, indirect draws, GPU decompression, native handles, backend parity, Nanite, benchmark superiority, and broad optimization.

- [x] **Step 4: Add source and CMake wiring**

  - Add source to `engine/runtime_rhi/CMakeLists.txt`.
  - Add `MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests` to root `CMakeLists.txt`.

- [x] **Step 5: Run GREEN target build and CTest**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests
  ```

## Task 3: Adjacent Focused Validation

- [x] **Step 1: Build and test adjacent MAVG visibility-buffer targets**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests MK_mavg_gpu_visibility_buffer_layout_tests MK_mavg_gpu_visible_cluster_packets_tests MK_mavg_gpu_cluster_format_tests MK_mavg_gpu_culling_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests|MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests|MK_mavg_gpu_visibility_buffer_layout_tests|MK_mavg_gpu_visible_cluster_packets_tests|MK_mavg_gpu_cluster_format_tests|MK_mavg_gpu_culling_tests"
  ```

## Task 4: Docs, Manifest, Static Guard

- [x] **Step 1: Update current capability docs, roadmap, MAVG spec, master plans, and plan registry**

  Record that this child writes deterministic visibility-buffer records through staged RHI copies and validates readback bytes, but does not create descriptor bindings, run compute, traverse clusters, execute mesh shaders, execute indirect draws, use GPU decompression, expose native handles, enforce allocator/GPU budgets, or claim backend/Nanite/performance superiority.

- [x] **Step 2: Update manifest fragments and compose**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
  ```

- [x] **Step 3: Add static guard**

  `tools/check-ai-integration-138-mavg-gpu-visibility-buffer-write-readback.ps1` must assert:
  - header/source/test/plan file existence
  - public API names
  - diagnostic names
  - `BufferUsage::storage`, `BufferUsage::copy_source`, and `BufferUsage::copy_destination` visibility-buffer validation
  - upload buffer `BufferUsage::copy_source`
  - readback buffer `BufferUsage::copy_destination`
  - `write_buffer`, `copy_buffer`, `submit`, `wait`, and `read_buffer` proof evidence
  - deterministic record packing evidence for layout offsets and flags
  - non-claim flags for descriptors, compute pipelines, dispatch, traversal, mesh shaders, indirect draws, GPU decompression, native handles, Vulkan/Metal parity, Nanite, benchmark superiority, and broad optimization
  - docs/manifest mention this child

- [x] **Step 4: Run agent-surface checks**

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

- [ ] **Step 2: Run publication preflight**

  Command:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
  ```

- [ ] **Step 3: Commit, push, and open stacked draft PR**

  Base branch:

  ```text
  codex/mavg-gpu-visibility-buffer-rhi-allocation-v1
  ```

  Head branch:

  ```text
  codex/mavg-gpu-visibility-buffer-rhi-write-readback-v1
  ```

  Commit message:

  ```text
  Add MAVG visibility buffer write readback proof
  ```

## Validation Evidence

- RED confirmed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests` failed before implementation because `mirakana/runtime_rhi/mavg_gpu_visibility_buffer_write_readback.hpp` did not exist.
- GREEN/focused build passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests MK_mavg_gpu_visibility_buffer_layout_tests MK_mavg_gpu_visible_cluster_packets_tests MK_mavg_gpu_cluster_format_tests MK_mavg_gpu_culling_tests`.
- Focused CTest passed 6/6: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_rhi_mavg_gpu_visibility_buffer_write_readback_tests|MK_runtime_rhi_mavg_gpu_visibility_buffer_resource_tests|MK_mavg_gpu_visibility_buffer_layout_tests|MK_mavg_gpu_visible_cluster_packets_tests|MK_mavg_gpu_cluster_format_tests|MK_mavg_gpu_culling_tests"`.
- Agent-surface checks passed: `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-format.ps1`.
- Full slice validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`; all 118 CTest tests passed, with Apple/Metal lanes reported as host-gated diagnostic-only on this Windows host.
