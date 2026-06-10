# 2026-06-11 MAVG GPU Culling Dispatch v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-gpu-culling-dispatch-v1`

**Status:** Completed.

**Execution State:** Completed through PR #556 (merge commit `de14f385`). The post-land closeout returns `currentActivePlan` to the production-completion master plan and `recommendedNextPlan.id` to `next-production-gap-selection` while preserving no broad MAVG/Nanite/backend readiness claims. `dispatch_mavg_gpu_culling_indirect` in `engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp` executes the first D3D12-only compute dispatch that writes MAVG packed indexed indirect argument and count buffers from reviewed cluster rows, records compute-write-to-indirect-read synchronization, and proves visible/culled cases through WARP-backed `MK_mavg_gpu_culling_dispatch_tests` while `MK_mavg_gpu_culling_tests` retain `executed_gpu_culling=false` value-only planner evidence. Vulkan compute dispatch and compute-generated indirect consumption remain unclaimed.

**Goal:** Extend the completed value-only `mavg-gpu-culling-indirect-v1` contract with the first actual GPU compute dispatch that writes MAVG packed indexed indirect argument and count buffers on D3D12, records the required compute-write-to-indirect-read synchronization, and proves deterministic buffer contents through WARP-backed readback without claiming compute-generated indirect consumption on Vulkan, Metal compute, mesh shaders, Nanite equivalence/superiority, or broad optimization.

**Context:** `mavg-gpu-culling-indirect-v1` already exposes `MavgGpuCullingIndirectPlan`, `plan_mavg_gpu_culling_indirect_commands`, packed 20-byte indexed indirect argument rows, 4-byte count rows, fail-closed diagnostics, and D3D12/Vulkan synchronization requirement rows while keeping actual dispatch out of scope. The completed indirect execution stack through PR #537-#539, PR #541, PR #547, PR #552, and closeout PR #553 now executes CPU-generated upload argument and count buffers on D3D12 and Vulkan. This child is the next stacked MAVG GPU step because Microsoft Learn and Khronos both require explicit synchronization between compute shader writes and indirect draw consumption, and the repository already records those requirement rows without executing dispatch.

**Constraints:**

- Keep `MK_renderer` backend-neutral; keep D3D12 command lists, root signatures, PSOs, UAV/indirect resources, and barriers backend-private.
- Start with D3D12-only dispatch in v1; keep Vulkan compute dispatch, Metal compute, and public native handles out of scope.
- Reuse existing `MavgGpuCullingIndirectCommandLayout`, packed argument layout, and count-buffer alignment rows from `mavg_gpu_culling.hpp`; do not invent a second indirect command format.
- Preserve the completed CPU-reference planning path when dispatch is not requested.
- Require reviewed upload or default argument/count buffers with `BufferUsage::indirect` plus backend-private UAV usage where needed; prove writes through readback or copy-source usage consistent with existing CPU-upload indirect stats rules.
- Insert the synchronization required by completed `MavgGpuCullingSyncRequirement` rows before any indirect draw consumes compute-written buffers.
- Do not claim compute-generated indirect consumption through existing RHI paths, count-buffer execution changes, mesh shaders, Work Graphs, package-visible MAVG backend readiness, benchmark superiority, or Nanite compatibility/equivalence/superiority.

**Done When:**

- A reviewed D3D12 compute dispatch path executes from `MavgGpuCullingIndirectPlan` inputs and writes deterministic packed argument/count buffer bytes for visible selected clusters.
- D3D12 dispatch records the barrier or state transition rows implied by the existing D3D12 sync requirement without exposing public buffer-state tracking.
- WARP-backed tests prove non-zero visible clusters produce expected indirect bytes and culled clusters reduce the effective count relative to the CPU-reference planner baseline.
- Docs, manifest fragments, composed manifest, and static guards describe the exact D3D12 compute-dispatch-only claim after activation.
- Focused validation and full `tools/validate.ps1` pass before publication.

---

## Official Source Audit

Checked on 2026-06-11:

- Microsoft Learn `ID3D12GraphicsCommandList::Dispatch`: compute dispatch executes on the compute queue/list and requires explicit resource state and UAV barriers before indirect argument consumption.
- Microsoft Learn `ID3D12GraphicsCommandList::ExecuteIndirect`: indirect argument and count buffers must be in `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT` at consumption time; count-buffer offsets remain 4-byte aligned.
- Context7 `/khronosgroup/vulkan-docs`: compute shader writes to indirect buffers require a memory barrier from shader write to indirect-command-read before draw-indirect consumption; this child records the D3D12 analogue only in v1.
- Completed sibling audit in `docs/superpowers/plans/2026-06-05-mavg-gpu-culling-indirect-v1.md`: packed argument layout, count rows, and sync requirement rows remain authoritative; this child must not regress the value-only planner.

## Scope

In scope:

- D3D12-only MAVG visibility-culling compute dispatch writing packed indexed indirect argument and count buffers.
- Backend-private reuse of existing `mavg_gpu_culling.hpp` layout and planner rows.
- Deterministic WARP-backed buffer readback proof for visible-cluster and culled-cluster cases.
- Conversion of existing fail-closed dispatch evidence into GREEN execution coverage plus targeted invalid-input tests.

Out of scope:

- Vulkan or Metal compute dispatch.
- Changing completed D3D12/Vulkan `IRhiCommandList::draw_indexed_indirect` CPU-upload execution paths.
- Compute-generated indirect consumption without a separate follow-up plan.
- Public buffer resource-state tracking.
- Mesh shaders, Work Graphs, deformation, ray tracing, Metal readiness, benchmark superiority, or Nanite compatibility/equivalence/superiority.

## Sequencing

1. Wait for MAVG Vulkan count-buffer closeout PR #553 merge to `main`.
2. Publish this draft plan-only PR without manifest activation.
3. Publish a separate activation PR: point `currentActivePlan` at this plan or return through the production-completion master plan with `recommendedNextPlan.id = mavg-gpu-culling-dispatch-v1`, and extend activation static guards without landing implementation in the same PR.
4. Implement in a fresh linked worktree from merged `origin/main`, validate, commit, push, and open an independent implementation PR over `codex/mavg-gpu-culling-dispatch-v1`.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp`
- Modify: `engine/renderer/src/mavg_gpu_culling.cpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp` (or backend-private D3D12 compute dispatch module)
- Create: `tests/unit/mavg_gpu_culling_dispatch_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp` or focused backend scaffold coverage as needed
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-gpu-culling-indirect-v1.md` (completed sibling pointer only)
- Modify: `docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md` (completed sibling pointer only)
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-105-mavg-gpu-culling-indirect.ps1`
- Create: `tools/check-ai-integration-111-mavg-gpu-culling-dispatch.ps1`

## Tasks

- [ ] Confirm D3D12 compute-to-indirect synchronization constraints through Microsoft Learn and retain completed GPU-culling planner audit references.
- [ ] Run a read-only rendering subagent audit and split D3D12 compute dispatch from Vulkan/compute-generated indirect consumption work.
- [ ] Add RED WARP-backed dispatch/readback tests for visible and culled MAVG cluster cases.
- [ ] Implement backend-private D3D12 compute dispatch writing packed argument/count buffers with required synchronization.
- [ ] After activation PR, synchronize docs, plan registry, architecture spec, manifest fragments, composed manifest, and static checks.
- [ ] Run focused validation plus full `tools/validate.ps1`.
- [ ] Publish a validated PR over `codex/mavg-gpu-culling-dispatch-v1`.

## Validation Evidence

| Command | Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_mavg_gpu_culling_tests --output-on-failure` | PASS (value-only planner baseline retained). |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_mavg_gpu_culling_dispatch_tests --output-on-failure` | PASS (3/3 WARP-backed visible/culled readback vs CPU planner). |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Required before publication. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Required before publication. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Required before publication. |

## Non-Claims

This plan does not claim Vulkan or Metal compute dispatch, compute-generated indirect consumption through existing RHI paths, D3D12/Vulkan indirect draw contract changes, public buffer state tracking, mesh shaders, Work Graphs, deformation, ray tracing, package-visible MAVG backend readiness, native handle exposure, benchmark superiority, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
