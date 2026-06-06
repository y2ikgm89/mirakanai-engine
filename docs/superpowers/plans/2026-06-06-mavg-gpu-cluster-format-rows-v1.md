# MAVG GPU Cluster Format Rows v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a backend-neutral, value-only MAVG GPU cluster format row planner that converts validated cluster graph and payload documents into deterministic per-cluster GPU layout rows for later GPU traversal, mesh shader, and indirect clustered geometry work.

**Architecture:** Keep this as a `MK_renderer` planning surface, not RHI execution. The helper consumes `MavgClusterGraphDocument` plus `MavgClusterPayloadDocument`, validates both existing asset contracts first, derives per-cluster bounds center/radius, draw range, page byte range, vertex/index layout, and meshlet-readiness counters, and fails closed before GPU/native work on any mismatch.

**Tech Stack:** C++23, `MK_assets`, `MK_renderer`, CMake/CTest, `tools/validate.ps1`, Context7 CMake docs, public Epic Nanite 5.7 documentation as feature taxonomy only, Microsoft DirectX Mesh Shader spec, and Khronos `VK_EXT_mesh_shader` documentation.

---

**Plan ID:** `mavg-gpu-cluster-format-rows-v1`

**Date:** 2026-06-06

**Base:** Stacked after MAVG Page GPU Resource Residency Execution v1 / draft PR #506 on branch `codex/mavg-page-gpu-resource-residency-execution-v1`.

## Context

- Official-source refresh for this child checked Epic's current Nanite overview, Microsoft DirectX Mesh Shader, Khronos `VK_EXT_mesh_shader`, and Context7 CMake docs.
- Epic's public Nanite docs are used only to frame feature classes: hierarchical clusters, compressed internal format, fine-grained streaming, and automatic LOD. No Unreal source, shaders, private tools, cooked data, or product/API naming are used.
- Existing project state already has `MavgClusterGraphDocument`, `MavgClusterPayloadDocument`, `plan_mavg_cluster_graph_cook_package`, CPU LOD selection, GPU culling indirect planning, page streaming, DirectStorage/RHI residency handoffs, and conventional upload evidence.
- Remaining MAVG master-plan gap for this slice is the GPU-facing cluster/meshlet format descriptor needed before real GPU traversal or mesh shader execution can be validated.

## Non-Claims

- No GPU command submission.
- No mesh shader execution.
- No GPU cluster traversal or visibility buffer.
- No RHI resource allocation or native handle exposure.
- No allocator/GPU budget enforcement.
- No GPU decompression.
- No Vulkan/Metal parity claim.
- No async-overlap/performance claim.
- No Nanite compatibility/equivalence/superiority or benchmark superiority claim.

## Files

- Create: `engine/renderer/include/mirakana/renderer/mavg_gpu_cluster_format.hpp`
- Create: `engine/renderer/src/mavg_gpu_cluster_format.cpp`
- Create: `tests/unit/mavg_gpu_cluster_format_tests.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify after compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-134-mavg-gpu-cluster-format-rows.ps1`

## Task 1: RED Test

- [x] **Step 1: Add `tests/unit/mavg_gpu_cluster_format_tests.cpp` with tests for a valid graph/payload pair**

  Test expectations:
  - `plan_mavg_gpu_cluster_format_rows` succeeds for valid graph/payload rows.
  - Result contains one row per graph cluster in deterministic cluster index order.
  - Row 0 exposes graph asset, cluster index, page index, local cluster index, lod level, material partition, parent/fallback rows, first index, index count, vertex base, triangle count, vertex count, page byte offset/size, vertex stride 32, index format `uint32`, bounds center/radius, and meshlet readiness for cluster sizes within limits.
  - Result flags remain false for `allocated_rhi_resources`, `submitted_gpu_work`, `executed_mesh_shader`, `executed_gpu_traversal`, `used_gpu_decompression`, `touched_native_handles`, and `claimed_nanite_compatibility`.

- [x] **Step 2: Add invalid tests**

  Test expectations:
  - Invalid graph fails closed with `invalid_graph`.
  - Invalid payload fails closed with `invalid_payload`.
  - Unsupported vertex stride fails closed with `unsupported_vertex_stride`.
  - Unsupported index format fails closed with `unsupported_index_format`.
  - Draw range overflow against payload index count fails closed with `cluster_draw_range_out_of_payload`.
  - Cluster triangle or vertex count above meshlet limits succeeds but reports `meshlet_ready_cluster_count` lower than `cluster_row_count`.

- [x] **Step 3: Wire `MK_mavg_gpu_cluster_format_tests` in root `CMakeLists.txt` and run RED**

  Command:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_cluster_format_tests
  ```

  Expected RED:

  ```text
  cannot open include file 'mirakana/renderer/mavg_gpu_cluster_format.hpp'
  ```

## Task 2: GREEN Implementation

- [x] **Step 1: Add public API in `mavg_gpu_cluster_format.hpp`**

  Public types:
  - `MavgGpuClusterFormatDiagnosticCode`
  - `MavgGpuClusterFormatDiagnostic`
  - `MavgGpuClusterFormatDesc`
  - `MavgGpuClusterFormatRow`
  - `MavgGpuClusterFormatPlan`
  - `plan_mavg_gpu_cluster_format_rows`
  - `has_mavg_gpu_cluster_format_diagnostic`

- [x] **Step 2: Implement validation and row derivation in `mavg_gpu_cluster_format.cpp`**

  Required behavior:
  - Call existing `validate_mavg_cluster_graph` and `validate_mavg_cluster_payload` before deriving rows.
  - Accept only `vertex_stride_bytes == 32`.
  - Accept only `index_format == "uint32"`.
  - Reject cluster draw ranges whose `first_index + index_count` exceeds `payload.index_count`.
  - Derive center as midpoint of graph bounds.
  - Derive radius as half diagonal length.
  - Mark meshlet-ready rows when `triangle_count <= max_meshlet_triangles` and `vertex_count <= max_meshlet_vertices`.
  - Populate D3D12/Vulkan mesh shader limit fields as data only.
  - Keep all side-effect/non-claim flags false.

- [x] **Step 3: Add source file to `engine/renderer/CMakeLists.txt`**

- [x] **Step 4: Run GREEN target build and CTest**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_cluster_format_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_gpu_cluster_format_tests
  ```

## Task 3: Related Focused Validation

- [x] **Step 1: Build and run adjacent MAVG tests**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_cluster_format_tests MK_mavg_gpu_culling_tests MK_mavg_lod_selection_tests MK_mavg_cluster_graph_tests MK_mavg_cluster_payload_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_gpu_cluster_format_tests|MK_mavg_gpu_culling_tests|MK_mavg_lod_selection_tests|MK_mavg_cluster_graph_tests|MK_mavg_cluster_payload_tests"
  ```

## Task 4: Docs, Manifest, Static Guard

- [x] **Step 1: Update current capability and roadmap docs**

  Record that MAVG GPU Cluster Format Rows v1 is value-only and narrows the GPU-cluster-format gap without GPU traversal, mesh shader execution, native handles, GPU decompression, Vulkan/Metal parity, or Nanite claims.

- [x] **Step 2: Update MAVG master plan and plan registry**

  Add this child after MAVG Page GPU Resource Residency Execution v1 and keep `currentActivePlan` at the production master selection gate with `recommendedNextPlan.id = next-production-gap-selection`.

- [x] **Step 3: Update manifest fragments and compose**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
  ```

- [x] **Step 4: Add static guard**

  `tools/check-ai-integration-134-mavg-gpu-cluster-format-rows.ps1` must assert:
  - header/source/test file existence
  - public API names
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

- [ ] **Step 2: Run publication preflight**

  Command:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
  ```

- [ ] **Step 3: Commit, push, and open stacked draft PR**

  Base branch:

  ```text
  codex/mavg-page-gpu-resource-residency-execution-v1
  ```

  Head branch:

  ```text
  codex/mavg-gpu-cluster-format-rows-v1
  ```

  Commit message:

  ```text
  Add MAVG GPU cluster format rows
  ```

## Validation Evidence

2026-06-06 evidence:

- RED build reached the intended missing-header failure:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_cluster_format_tests
  ```

  Result: failed before implementation with missing `mirakana/renderer/mavg_gpu_cluster_format.hpp`.

- GREEN target build and adjacent MAVG CTest passed:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_cluster_format_tests MK_mavg_gpu_culling_tests MK_mavg_lod_selection_tests MK_mavg_cluster_graph_tests MK_mavg_cluster_payload_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_gpu_cluster_format_tests|MK_mavg_gpu_culling_tests|MK_mavg_lod_selection_tests|MK_mavg_cluster_graph_tests|MK_mavg_cluster_payload_tests"
  ```

  Result: 5/5 focused tests passed.

- Agent-surface and public API checks passed:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
  ```

  Result: all passed.

- Full repository validation passed:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
  ```

  Result: `validate: ok`; CTest passed 114/114. Metal/iOS checks remained host-gated diagnostics on this Windows host.
