# 2026-06-10 MAVG D3D12 Compute-Generated Indirect Consumption v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-d3d12-compute-generated-indirect-consumption-v1`

**Status:** Completed.

**Execution State:** Completed through PR #560 over activation PR #559 and plan draft PR #558 after MAVG GPU Culling Dispatch v1 closeout PR #557 and completed dispatch PR #556. D3D12 `draw_indexed_indirect` now accepts compute-generated `BufferUsage::indirect | BufferUsage::storage` argument and count buffers through `is_compute_generated_indexed_indirect_buffer`, records backend-private UAV-to-`D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT` transitions before `ExecuteIndirect`, preserves the CPU-upload `copy_source` path, adds `native_buffer_resource` and `leave_indirect_argument_state_for_consumption` on `dispatch_mavg_gpu_culling_indirect`, and proves dispatch-plus-draw visible/culled execution through WARP-backed `MK_mavg_d3d12_compute_generated_indirect_consumption_tests`. Planned Vulkan dispatch follow-up child `mavg-vulkan-gpu-culling-dispatch-v1` completed through plan draft PR #561 and implementation PR #563, and sibling MAVG Vulkan Compute-Generated Indirect Consumption v1 (`mavg-vulkan-compute-generated-indirect-consumption-v1`) completed through PR #567 / merge commit `9c6b681f` with `is_compute_generated_indexed_indirect_buffer`, `leave_indirect_argument_state_for_consumption`, and `MK_mavg_vulkan_compute_generated_indirect_consumption_tests` while mesh shaders, Metal readiness, Nanite equivalence/superiority, and broad optimization remain unclaimed.

**Goal:** Close the D3D12 Phase 4 compute-to-draw gap by letting `IRhiCommandList::draw_indexed_indirect` consume GPU-written MAVG packed indexed indirect argument and count buffers produced by `dispatch_mavg_gpu_culling_indirect`, recording the required compute-write-to-indirect-read synchronization and proving visible execution through WARP-backed end-to-end tests without claiming Vulkan compute dispatch, Vulkan compute-generated indirect consumption, mesh shaders, Nanite equivalence/superiority, or broad optimization.

**Context:** `mavg-gpu-culling-dispatch-v1` already executes D3D12 compute dispatch that writes packed argument/count bytes into reviewed default buffers with `BufferUsage::indirect` plus backend-private UAV usage, and records compute-write synchronization inside the dispatch module. The completed D3D12/Vulkan indirect execution stack through PR #537-#539, PR #541, PR #547, PR #549, PR #552, and PR #553 still requires `BufferUsage::copy_source` upload usage in `draw_indexed_indirect` v1 so CPU map/readback can populate `RhiStats`. That gate blocks the natural follow-up: dispatch on GPU, then consume the same buffers through the existing RHI indirect draw path. This child is the next stacked MAVG GPU step because Microsoft Learn requires explicit UAV-to-indirect-argument transitions before `ExecuteIndirect`, and the repository already records those requirement rows in `MavgGpuCullingSyncRequirement` without wiring consumption.

**Constraints:**

- Keep `MK_renderer` backend-neutral; keep D3D12 resource states, UAV barriers, and `ExecuteIndirect` details backend-private.
- Start with D3D12-only compute-generated consumption in v1; keep Vulkan compute dispatch, Vulkan compute-generated indirect consumption, Metal compute, and public native handles out of scope.
- Reuse existing `MavgGpuCullingIndirectCommandLayout`, packed argument layout, count-buffer alignment rows, and `dispatch_mavg_gpu_culling_indirect`; do not invent a second indirect command format.
- Preserve the completed CPU-upload `copy_source` indirect draw path unchanged for existing `MK_d3d12_rhi_tests` and Vulkan scaffold coverage.
- Introduce a reviewed compute-generated consumption contract for default indirect buffers without `copy_source`, gated fail-closed when buffers are not dispatch-owned or lack required usage.
- Insert the synchronization required by completed `MavgGpuCullingSyncRequirement` D3D12 rows before `ExecuteIndirect` consumes compute-written buffers.
- Do not claim full Phase 4 completion on Vulkan, mesh shaders, Work Graphs, package-visible MAVG backend readiness, benchmark superiority, or Nanite compatibility/equivalence/superiority.

**Done When:**

- D3D12 `draw_indexed_indirect` accepts compute-generated argument and count buffers that satisfy the reviewed indirect-buffer contract and executes `ExecuteIndirect` with the expected `min(count_buffer_value, max_draw_count)` semantics.
- D3D12 records UAV-to-indirect-argument transitions for compute-written buffers without exposing public buffer-state tracking.
- WARP-backed end-to-end tests prove `dispatch_mavg_gpu_culling_indirect` followed by `draw_indexed_indirect` renders visible geometry for non-zero visible clusters and draws nothing for zero-count culled cases.
- CPU-upload indirect draw tests remain green and unchanged in claim scope.
- Docs, manifest fragments, composed manifest, and static guards describe the exact D3D12 compute-generated-consumption-only claim after activation.
- Focused validation and full `tools/validate.ps1` pass before publication.

---

## Official Source Audit

Checked on 2026-06-10:

- Microsoft Learn `ID3D12GraphicsCommandList::Dispatch`: compute writes to UAV-backed buffers require explicit resource state and UAV barriers before indirect consumption.
- Microsoft Learn `ID3D12GraphicsCommandList::ExecuteIndirect`: indirect argument and count buffers must be in `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT` at consumption time; count-buffer offsets remain 4-byte aligned; `CommandCount = min(*pCountBuffer, MaxCommandCount)` when a count buffer is supplied.
- Microsoft Learn resource states: transitions from `D3D12_RESOURCE_STATE_UNORDERED_ACCESS` to `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT` are required between compute output and indirect draw on the same command list unless split-queue ordering is proven elsewhere; this child stays same-list WARP evidence in v1.
- Completed sibling audit in `docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md`: dispatch buffer layout, sync requirement rows, and readback proof remain authoritative; this child must not regress dispatch-only claims or CPU-reference planner evidence.

## Scope

In scope:

- D3D12-only compute-generated indirect consumption through existing `IRhiCommandList::draw_indexed_indirect`.
- Backend-private UAV-to-indirect-argument synchronization before `ExecuteIndirect`.
- Reviewed buffer usage contract distinguishing CPU-upload `copy_source` indirect buffers from compute-generated default indirect buffers.
- WARP-backed end-to-end dispatch-plus-draw visible/culled proof in focused tests.
- Targeted invalid-input fail-closed tests for compute-generated buffers missing required usage or alignment.

Out of scope:

- Vulkan compute dispatch or Vulkan compute-generated indirect consumption.
- Changing completed Vulkan CPU-upload indirect draw behavior.
- Replacing or removing CPU-upload D3D12 indirect draw coverage.
- Public buffer resource-state tracking APIs.
- Mesh shaders, Work Graphs, deformation, ray tracing, Metal readiness, benchmark superiority, or Nanite compatibility/equivalence/superiority.

## Sequencing

1. Wait for MAVG GPU Culling Dispatch v1 closeout PR #557 merge to `main`.
2. Publish this draft plan-only PR without manifest activation.
3. Publish a separate activation PR: point `currentActivePlan` at this plan or return through the production-completion master plan with `recommendedNextPlan.id = mavg-d3d12-compute-generated-indirect-consumption-v1`, and extend activation static guards without landing implementation in the same PR.
4. Implement in a fresh linked worktree from merged `origin/main`, validate, commit, push, and open an independent implementation PR over `codex/mavg-d3d12-compute-generated-indirect-consumption-v1`.

## Files

- Modify: `engine/rhi/include/mirakana/rhi/indirect_draw.hpp`
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp` (only if a reviewed public usage/desc field is required)
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp` (only if shared barrier helpers must be reused)
- Create: `tests/unit/mavg_d3d12_compute_generated_indirect_consumption_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp` (invalid-input coverage only; preserve CPU-upload GREEN cases)
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md` (completed sibling pointer only)
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-111-mavg-gpu-culling-dispatch.ps1` (completed sibling pointer only)
- Create: `tools/check-ai-integration-112-mavg-d3d12-compute-generated-indirect-consumption.ps1`

## Tasks

- [x] Confirm D3D12 UAV-to-indirect-argument synchronization constraints through Microsoft Learn and retain completed GPU-culling dispatch audit references.
- [x] Run a read-only rendering subagent audit and split D3D12 compute-generated consumption from Vulkan symmetric follow-up work.
- [x] Add RED WARP-backed end-to-end tests: dispatch writes buffers, `draw_indexed_indirect` consumes them, visible clusters render and culled clusters draw nothing.
- [x] Implement reviewed compute-generated buffer contract and backend-private D3D12 barriers before `ExecuteIndirect`.
- [x] Preserve CPU-upload `copy_source` indirect path and existing stats behavior without broadening claims.
- [x] After activation PR, synchronize docs, plan registry, architecture spec, manifest fragments, composed manifest, and static checks.
- [x] Run focused validation plus full `tools/validate.ps1`.
- [x] Publish a validated PR over `codex/mavg-d3d12-compute-generated-indirect-consumption-v1`.

## Validation Evidence

| Command | Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_mavg_gpu_culling_dispatch_tests --output-on-failure` | Required baseline; dispatch-only proof must remain green. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_mavg_d3d12_compute_generated_indirect_consumption_tests --output-on-failure` | Required new end-to-end visible/culled proof. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_d3d12_rhi_tests --output-on-failure` | Required regression guard for CPU-upload indirect draw coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Required before publication. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Required before publication. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Required before publication. |

## Non-Claims

This plan does not claim Vulkan compute dispatch, Vulkan compute-generated indirect consumption, Metal compute, D3D12 compute dispatch changes beyond shared barrier helper reuse, public buffer state tracking, mesh shaders, Work Graphs, deformation, ray tracing, package-visible MAVG backend readiness, native handle exposure, benchmark superiority, Nanite compatibility, Nanite equivalence, Nanite superiority, full Phase 4 completion on Vulkan, or broad CPU/GPU/memory optimization.
