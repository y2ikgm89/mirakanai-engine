# 2026-06-05 MAVG D3D12 GPU Culling Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-d3d12-gpu-culling-execution-v1`

**Status:** Completed.

**Execution State:** Stacked on `mavg-d3d12-compute-generated-indirect-execution-v1`. This child promotes only the selected D3D12 WARP-backed MAVG GPU-culling dispatch lane from non-claim to execution evidence and is published as stacked draft PR #452.

**Goal:** Prove that a D3D12 compute shader can consume selected MAVG cluster command rows plus culling visibility rows, compact only visible clusters into a GPU-generated indexed indirect argument/count buffer, and execute that compacted buffer with `ExecuteIndirect`.

**Architecture:** Keep public RHI, renderer, and MAVG value contracts unchanged. Use `MavgGpuCullingIndirectPlan` as the CPU reference expectation and synchronization contract, then exercise the existing D3D12 RHI compute and indexed indirect execution path from PR #451 with default-heap `BufferUsage::storage | BufferUsage::indirect` output. This is a D3D12 proof lane, not a generic GPU-culling framework.

**Tech Stack:** C++23, `MK_renderer` MAVG value rows, `MK_rhi_d3d12`, Microsoft Learn Direct3D 12 GPU culling / `ExecuteIndirect`, Context7 Direct3D 12 docs, WARP-backed CTest, PowerShell 7 validation scripts.

---

## Official Source Audit

Checked on 2026-06-05:

- Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12`: the Microsoft GPU-culling sample writes processed command buffers through compute UAVs, transitions processed command buffers to `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT`, passes the same processed buffer as argument and count resource at different offsets, and synchronizes compute and graphics queues through fences.
- Microsoft Learn Direct3D 12 Indirect Drawing and GPU Culling sample: processed command buffers are default-heap UAV buffers, the UAV counter is reset before compute, compute dispatch emits visible commands, rendering transitions the processed command buffer before `ExecuteIndirect`, and graphics waits for the compute queue when queues are split.
- Microsoft Learn `ID3D12GraphicsCommandList::ExecuteIndirect`: argument and count buffers must be buffer resources, offsets are 4-byte aligned, and count-buffer execution is clamped by `MaxCommandCount`.

## Scope

In scope:

- D3D12-only WARP-backed actual GPU culling dispatch proof.
- Selected MAVG rows from `MavgLodSelectionResult` and `MavgGpuCullingClusterBoundsRow`.
- GPU compaction from two selected candidate clusters to one visible indexed indirect command.
- Same-buffer argument/count layout with `count_buffer_offset = max_command_count * indexed_indirect_draw_command_stride_bytes`.
- Visible readback proof: the visible center triangle is drawn and the culled top-left triangle remains absent.
- Narrow stats evidence for compute dispatch, graphics queue wait, GPU-generated indirect execution, UAV barrier, and indirect argument transition.

Out of scope:

- Public RHI buffer-state APIs or native handle exposure.
- Generic append/consume counters, multi-command GPU culling framework, generic storage-buffer state tracking, or generic GPU-generated count-buffer systems.
- Vulkan `vkCmdDrawIndexedIndirectCount`, Metal indirect command buffers, mesh/task/amplification shaders, Work Graphs, async overlap/performance, broad GPU optimization, Nanite compatibility, Nanite equivalence, or Nanite superiority.

## Files

- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-d3d12-compute-generated-indirect-execution-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-109-mavg-d3d12-compute-generated-indirect-execution.ps1`
- Create: `tools/check-ai-integration-110-mavg-d3d12-gpu-culling-execution.ps1`

## Tasks

- [x] Add RED D3D12 RHI test `d3d12 rhi device executes mavg gpu culling dispatch into indexed indirect draw`.
- [x] Use `MavgGpuCullingIndirectPlan` as the expected compacted command/count reference while the GPU compute shader receives the un-compacted selected-cluster command rows and visibility rows.
- [x] Implement the D3D12 test scene: source command upload/storage copy, visibility upload/storage copy, default storage/indirect compacted output buffer, compute pipeline, graphics pipeline, queue wait, and indexed indirect draw.
- [x] Verify visible center pixel is orange and the culled top-left probe pixel remains black.
- [x] Verify generic and D3D12-specific stats for compute dispatch, queue wait, GPU-generated indirect execution, UAV barrier, and indirect argument transition without claiming CPU-decoded executed command counts.
- [x] Update docs, plan registry, architecture spec, manifest fragments, composed manifest, and static guards.
- [x] Run focused D3D12/MAVG validation.
- [x] Run full `tools/validate.ps1`.
- [x] Publish a validated stacked draft PR over `codex/mavg-d3d12-compute-generated-indirect-execution-v1`.

## Validation Plan

| Command | Expected Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests` | RED failed after the test was added and before helpers existed: missing `create_mavg_gpu_culling_execution_scene`, `submit_mavg_gpu_culling_indirect_draw`, and `require_black_probe_pixel`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_culling_tests MK_d3d12_rhi_tests` | Passed on 2026-06-05 after implementation; focused MAVG planner and D3D12 proof targets built. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_mavg_gpu_culling_tests|MK_d3d12_rhi_tests)$"` | Passed on 2026-06-05; 2/2 focused tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed on 2026-06-05; no public native/RHI handle or backend state API drift. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files tests/unit/d3d12_rhi_tests.cpp` | Passed on 2026-06-05; D3D12 test changes pass targeted tidy. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed on 2026-06-05; static guard validates code/docs/manifest evidence and retained non-claims. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed on 2026-06-05; manifest compose and JSON contracts are clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed on 2026-06-05; agent surfaces and skill parity checks are clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed on 2026-06-05; C++ and tracked text formatting are clean. |
| `git diff --check` | Passed on 2026-06-05; whitespace is clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed on 2026-06-05; static checks, build, generated C++23 check, tidy smoke, and 107/107 tests passed. Apple/Metal lanes remain diagnostic host-gated on Windows. |
| `gh pr create --draft --base codex/mavg-d3d12-compute-generated-indirect-execution-v1 --head codex/mavg-d3d12-gpu-culling-execution-v1` | Published stacked draft PR #452: `https://github.com/y2ikgm89/mirakanai-engine/pull/452`. |

## Non-Claims

This plan does not claim Vulkan indirect draw execution, Vulkan indirect count execution, Metal indirect command buffers, public buffer transition APIs, generic GPU-generated count-buffer systems beyond the D3D12 selected proof lane, generic storage-buffer state management, append/consume GPU culling counters, broad GPU culling algorithms, mesh shaders, Work Graphs, ray tracing, deformation, Nanite compatibility, Nanite equivalence, Nanite superiority, async-overlap/performance, or broad CPU/GPU/memory optimization.
