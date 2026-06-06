# MAVG GPU Visible Cluster Packet Rows v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a backend-neutral visible-cluster packet planner that joins validated MAVG GPU cluster format rows with visible indirect command rows before later GPU traversal, visibility-buffer, mesh shader, or indirect clustered-geometry execution.

**Architecture:** Keep this as a value-only `MK_renderer` planning surface. The helper consumes a successful `MavgGpuClusterFormatPlan` plus a successful `MavgGpuCullingIndirectPlan`, preserves visible command order, joins each command to exactly one cluster-format row, emits deterministic packet records, and fails closed before RHI allocation or GPU work on stale/missing/duplicate row evidence.

**Tech Stack:** C++23, `MK_renderer`, `MK_assets`, CMake/CTest, `tools/validate.ps1`, Context7 CMake docs, Microsoft D3D12 `ExecuteIndirect` docs, DirectX Mesh Shader spec, and Khronos `VK_EXT_mesh_shader` documentation.

---

**Plan ID:** `mavg-gpu-visible-cluster-packets-v1`

**Date:** 2026-06-06

**Base:** Stacked after MAVG GPU Cluster Format Rows v1 / draft PR #508 on branch `codex/mavg-gpu-cluster-format-rows-v1`.

## Context

- MAVG GPU Cluster Format Rows v1 creates deterministic per-cluster GPU layout rows from validated graph/payload data.
- MAVG GPU Culling Indirect Planning v1 creates visible indexed indirect command rows and D3D12/Vulkan synchronization requirements without executing GPU work.
- The MAVG master plan still lists no GPU cluster traversal or visibility-buffer path. This child narrows that gap by creating a deterministic packet boundary that later GPU traversal, visibility-buffer upload/allocation, mesh shader, or indirect clustered geometry execution can consume.
- Official D3D12 `ExecuteIndirect` docs require indirect argument/count buffers to be in indirect-argument state when executed; this child only records packet metadata and does not execute indirect draws.
- DirectX and Vulkan mesh shader docs require backend feature/property limits before execution; this child carries meshlet readiness as metadata only.

## Non-Claims

- No RHI resource allocation.
- No GPU command submission.
- No GPU traversal execution.
- No GPU visibility-buffer write or allocation.
- No mesh shader execution.
- No indirect draw execution.
- No GPU decompression.
- No native handle exposure.
- No allocator/GPU budget enforcement.
- No Vulkan/Metal parity claim.
- No Nanite compatibility/equivalence/superiority or benchmark superiority claim.

## Files

- Create: `engine/renderer/include/mirakana/renderer/mavg_gpu_visible_cluster_packets.hpp`
- Create: `engine/renderer/src/mavg_gpu_visible_cluster_packets.cpp`
- Create: `tests/unit/mavg_gpu_visible_cluster_packets_tests.cpp`
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
- Create: `tools/check-ai-integration-135-mavg-gpu-visible-cluster-packets.ps1`

## Task 1: RED Test

- [x] **Step 1: Add `tests/unit/mavg_gpu_visible_cluster_packets_tests.cpp` with successful packet planning tests**

  Test expectations:
  - `plan_mavg_gpu_visible_cluster_packets` succeeds for a successful format plan plus successful culling plan.
  - Packet count equals visible indirect command count.
  - Packet order matches `MavgGpuCullingIndirectPlan::commands` order.
  - Each packet carries graph asset, cluster index, packet index, indirect command index, page index, local cluster index, LOD, material partition, parent/fallback rows, fallback substitution, first index, index count, vertex base, triangle count, vertex count, payload page byte offset/size, bounds center/radius, meshlet readiness, and command argument fields.
  - Result counters report source cluster row count, source visible command count, packet count, meshlet-ready packet count, fallback packet count, and payload byte span.
  - Result flags remain false for `allocated_rhi_resources`, `submitted_gpu_work`, `executed_gpu_traversal`, `wrote_gpu_visibility_buffer`, `executed_mesh_shader`, `executed_indirect_draw`, `used_gpu_decompression`, `touched_native_handles`, and `claimed_nanite_compatibility`.

- [x] **Step 2: Add fail-closed tests**

  Test expectations:
  - Missing format plan fails with `missing_cluster_format_plan`.
  - Invalid format plan fails with `invalid_cluster_format_plan`.
  - Missing culling plan fails with `missing_culling_plan`.
  - Invalid culling plan fails with `invalid_culling_plan`.
  - Duplicate format row fails with `duplicate_cluster_format_row`.
  - Visible command without a matching format row fails with `missing_cluster_format_row`.
  - Visible command whose draw range differs from the format row fails with `stale_visible_command`.
  - Count-buffer value that disagrees with visible command count fails with `visible_command_count_mismatch`.
  - Zero visible commands in an otherwise successful culling plan succeeds with zero packets.
  - `require_meshlet_ready_packets=true` fails with `meshlet_not_ready` when a visible command maps to a non-meshlet-ready format row.
  - Packet budget overflow fails with `max_packet_count_exceeded`.

- [x] **Step 3: Wire `MK_mavg_gpu_visible_cluster_packets_tests` in root `CMakeLists.txt` and run RED**

  Command:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visible_cluster_packets_tests
  ```

  Expected RED:

  ```text
  cannot open include file 'mirakana/renderer/mavg_gpu_visible_cluster_packets.hpp'
  ```

## Task 2: GREEN Implementation

- [x] **Step 1: Add public API in `mavg_gpu_visible_cluster_packets.hpp`**

  Public types:
  - `MavgGpuVisibleClusterPacketDiagnosticCode`
  - `MavgGpuVisibleClusterPacketDiagnostic`
  - `MavgGpuVisibleClusterPacketDesc`
  - `MavgGpuVisibleClusterPacketRow`
  - `MavgGpuVisibleClusterPacketPlan`
  - `plan_mavg_gpu_visible_cluster_packets`
  - `has_mavg_gpu_visible_cluster_packet_diagnostic`

- [x] **Step 2: Implement validation and row joining in `mavg_gpu_visible_cluster_packets.cpp`**

  Required behavior:
  - Require non-null and successful format/culling plans.
  - Reject duplicate format rows by `(graph_asset, cluster_index)`.
  - Require each visible indirect command to match exactly one format row.
  - Reject stale commands when page, LOD, material partition, first index, index count, or vertex base disagree with the format row.
  - Reject non-zero `count_buffer_value` mismatches against the visible command count.
  - Treat zero visible commands as a successful zero-row plan when the source culling plan itself succeeded.
  - Preserve visible command order.
  - Compute `payload_byte_span` as the min/max span over packet page byte ranges.
  - Enforce `max_packet_count` when non-zero.
  - Fail closed by clearing packets and zeroing counters on diagnostics.
  - Keep all side-effect/non-claim flags false.

- [x] **Step 3: Add source file to `engine/renderer/CMakeLists.txt`**

- [x] **Step 4: Run GREEN target build and CTest**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visible_cluster_packets_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_gpu_visible_cluster_packets_tests
  ```

## Task 3: Related Focused Validation

- [x] **Step 1: Build and run adjacent MAVG renderer tests**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visible_cluster_packets_tests MK_mavg_gpu_cluster_format_tests MK_mavg_gpu_culling_tests MK_mavg_lod_selection_tests
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_gpu_visible_cluster_packets_tests|MK_mavg_gpu_cluster_format_tests|MK_mavg_gpu_culling_tests|MK_mavg_lod_selection_tests"
  ```

## Task 4: Docs, Manifest, Static Guard

- [x] **Step 1: Update capability and roadmap docs**

  Record that MAVG GPU Visible Cluster Packet Rows v1 joins format rows with visible indirect command rows without GPU traversal, visibility-buffer allocation/write, mesh shader execution, RHI allocation, GPU decompression, Vulkan/Metal parity, or Nanite claims.

- [x] **Step 2: Update MAVG master plan and plan registry**

  Add this child after MAVG GPU Cluster Format Rows v1 and keep `currentActivePlan` at the production master selection gate with `recommendedNextPlan.id = next-production-gap-selection`.

- [x] **Step 3: Update manifest fragments and compose**

  Commands:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
  ```

- [x] **Step 4: Add static guard**

  `tools/check-ai-integration-135-mavg-gpu-visible-cluster-packets.ps1` must assert:
  - header/source/test file existence
  - public API names
  - diagnostics and stale-row fail-closed needles
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

- [ ] **Step 3: Commit, push, and open stacked draft PR**

  Base branch:

  ```text
  codex/mavg-gpu-cluster-format-rows-v1
  ```

  Head branch:

  ```text
  codex/mavg-gpu-visible-cluster-packets-v1
  ```

  Commit message:

  ```text
  Add MAVG GPU visible cluster packet rows
  ```

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visible_cluster_packets_tests` failed before implementation with missing `mirakana/renderer/mavg_gpu_visible_cluster_packets.hpp`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visible_cluster_packets_tests` passed.
- GREEN CTest: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_gpu_visible_cluster_packets_tests` passed.
- Focused adjacent build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_visible_cluster_packets_tests MK_mavg_gpu_cluster_format_tests MK_mavg_gpu_culling_tests MK_mavg_lod_selection_tests` passed after final formatting.
- Focused adjacent CTest: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_gpu_visible_cluster_packets_tests|MK_mavg_gpu_cluster_format_tests|MK_mavg_gpu_culling_tests|MK_mavg_lod_selection_tests"` passed 4/4.
- Agent-surface checks: `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-format.ps1` passed after adding `tools/check-ai-integration-135-mavg-gpu-visible-cluster-packets.ps1`.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 19 static checks, build, tidy, and 115/115 CTests. Metal and Apple evidence remained diagnostic-only / host-gated on this Windows host.
- Publication preflight: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` passed on branch `codex/mavg-gpu-visible-cluster-packets-v1`.
