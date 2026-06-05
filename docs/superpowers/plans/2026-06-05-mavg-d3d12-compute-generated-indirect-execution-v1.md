# 2026-06-05 MAVG D3D12 Compute-Generated Indirect Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-d3d12-compute-generated-indirect-execution-v1`

**Status:** Completed.

**Execution State:** Stacked on `mavg-d3d12-indexed-indirect-count-buffer-execution-v1`. This child promotes only the D3D12 compute-written argument/count buffer lane for indexed indirect draws from unsupported to WARP-backed execution evidence.

**Goal:** Execute a D3D12 indexed indirect draw whose argument buffer and count buffer are written by a compute shader, with backend-private D3D12 UAV/transition synchronization proof and visible texture readback.

**Architecture:** Keep the public RHI contract unchanged. Extend only the D3D12 backend internals so storage buffers used by compute descriptors are tracked as `D3D12_RESOURCE_STATE_UNORDERED_ACCESS`, then transitioned to `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT` before `ExecuteIndirect`; CPU-generated upload buffers keep the existing decode/stat path, while compute-generated default-heap buffers skip CPU decoding and report narrow GPU-generated indirect evidence with no CPU decode/stat overclaim.

**Tech Stack:** C++23, D3D12 WARP, Microsoft Learn D3D12 `ExecuteIndirect`, Microsoft Learn D3D12 resource barriers, repository PowerShell 7 validation scripts.

---

## Official Source Audit

Checked on 2026-06-05:

- Microsoft Learn Direct3D 12 Indirect Drawing and GPU Culling sample: processed command buffers are default-heap buffers with `D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS`; compute writes them through UAV descriptors; rendering transitions the processed command buffer from `D3D12_RESOURCE_STATE_UNORDERED_ACCESS` to `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT` before `ExecuteIndirect`, and can pass the same buffer as command/count resource at different offsets.
- Microsoft Learn `ID3D12GraphicsCommandList::ExecuteIndirect`: argument and count buffers must be buffer resources, offsets are 4-byte aligned, and count-buffer execution is `min(count_buffer_uint32, MaxCommandCount)`.
- Microsoft Learn D3D12 resource barrier guidance: D3D12 applications own per-resource state management through `ID3D12GraphicsCommandList::ResourceBarrier`; UAV barriers and transition barriers are the reviewed synchronization tools for unordered access writes and later reads.
- Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12`: confirmed the Direct3D 12 GPU-culling sample records compute UAV writes, then records rendering barriers to `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT` before `ExecuteIndirect`.

## Scope

In scope:

- D3D12-only compute-generated indexed indirect argument/count buffers.
- Default-heap buffers with `BufferUsage::storage | BufferUsage::indirect`.
- Backend-private D3D12 buffer state tracking for storage-buffer UAV writes and indirect-argument reads.
- Backend-private UAV barrier and transition evidence for storage buffers consumed by `ExecuteIndirect`.
- Same-buffer argument/count layout using argument offset `0` and count offset `indexed_indirect_draw_command_stride_bytes`.
- Visible WARP-backed texture readback proving the compute-written indirect command draws a triangle.
- fail-closed validation for compute-generated indirect buffers without `BufferUsage::storage | BufferUsage::indirect`.

Out of scope:

- Public `ResourceState::unordered_access`, public `ResourceState::indirect_argument`, or public buffer transition APIs.
- Vulkan `vkCmdDrawIndexedIndirectCount`, Metal indirect command buffers, mesh shaders, Work Graphs, or Nanite equivalence/superiority.
- General GPU culling algorithms, append/consume UAV counters, multi-command compaction, async-overlap/performance claims, or broad GPU optimization.

## Files

- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-count-buffer-execution-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-ai-integration-108-mavg-d3d12-indexed-indirect-count-buffer-execution.ps1`
- Create: `tools/check-ai-integration-109-mavg-d3d12-compute-generated-indirect-execution.ps1`

## Tasks

- [x] Confirm D3D12 compute-generated indirect synchronization through Microsoft Learn and Context7.
- [x] Create stacked branch `codex/mavg-d3d12-compute-generated-indirect-execution-v1` from PR #449 head.
- [x] Add RED D3D12 RHI test `d3d12 rhi device executes compute generated indexed indirect count buffer draw into texture readback bytes`.
- [x] Add RED D3D12 RHI invalid-desc test for compute-generated indirect buffers missing `storage` or `indirect` usage.
- [x] Add backend-private D3D12 buffer state tracking for committed buffers.
- [x] Record storage-buffer UAV state for compute-bound buffers that are both `BufferUsage::storage` and `BufferUsage::indirect`.
- [x] Before D3D12 `ExecuteIndirect`, record a UAV barrier and transition storage indirect argument/count buffers to `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT`.
- [x] Split CPU-generated upload decode/stats from GPU-generated default-buffer execution so compute-generated buffers do not require CPU mapping.
- [x] Update docs, plan registry, architecture spec, manifest fragments, composed manifest, and static guards.
- [x] Run focused D3D12 build/test/static validation.
- [x] Run full `tools/validate.ps1`.
- [x] Publish a validated stacked draft PR over `codex/mavg-d3d12-indexed-indirect-count-buffer-execution-v1`.

## Validation Plan

| Command | Expected Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests` | RED test build passed after the compute-generated indirect tests compiled. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_d3d12_rhi_tests$"` | RED failed as expected before implementation with `d3d12 rhi indexed indirect draw argument buffer requires copy_source upload usage in v1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests` | GREEN build passed after D3D12 storage/indirect synchronization implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_d3d12_rhi_tests$"` | GREEN passed; `MK_d3d12_rhi_tests` reported 1/1 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed; static chapter 109 validates code/docs/manifest evidence and retained non-claims. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed; manifest compose and JSON contracts are clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed; agent surfaces and line-ending contracts are clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed; no unsupported public native/RHI surface drift. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed; C++ and tracked text formatting are clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/rhi/d3d12/src/d3d12_backend.cpp,tests/unit/d3d12_rhi_tests.cpp` | Passed for both files after internal-linkage cleanup. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests MK_backend_scaffold_tests MK_d3d12_rhi_tests` | Passed; focused RHI/D3D12 targets built. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_rhi_tests|MK_backend_scaffold_tests|MK_d3d12_rhi_tests)$"` | Passed 3/3 focused tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed; full slice gate ended with `validate: ok`, 107/107 CTest tests passed, Apple/Metal diagnostics remained expected host-gated rows on Windows. |
| `gh pr create --draft --base codex/mavg-d3d12-indexed-indirect-count-buffer-execution-v1 --head codex/mavg-d3d12-compute-generated-indirect-execution-v1` | Published stacked draft PR #451: `https://github.com/y2ikgm89/mirakanai-engine/pull/451`. |

## Non-Claims

This plan does not claim Vulkan indirect draw execution, Vulkan indirect count execution, Metal indirect command buffers, public buffer transition APIs, generic GPU-generated count-buffer systems beyond this D3D12 storage-indirect lane, generic storage-buffer state management, append/consume GPU culling counters, broad GPU culling dispatch algorithms, mesh shaders, Work Graphs, ray tracing, deformation, Nanite compatibility, Nanite equivalence, Nanite superiority, async-overlap/performance, or broad CPU/GPU/memory optimization.
