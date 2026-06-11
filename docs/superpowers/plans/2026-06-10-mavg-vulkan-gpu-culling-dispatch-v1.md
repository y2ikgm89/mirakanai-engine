# 2026-06-10 MAVG Vulkan GPU Culling Dispatch v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-vulkan-gpu-culling-dispatch-v1`

**Status:** Completed.

**Execution State:** Completed through PR #563 (merge commit `5dce6857`). The post-land closeout returns `currentActivePlan` to the production-completion master plan and `recommendedNextPlan.id` to `next-production-gap-selection` while preserving no broad MAVG/Nanite/backend readiness claims. `dispatch_mavg_gpu_culling_indirect` in `engine/rhi/vulkan/src/vulkan_mavg_gpu_culling_dispatch.cpp` executes the first Vulkan compute dispatch that writes MAVG packed indexed indirect argument and count buffers from reviewed cluster rows, records compute-write-to-indirect-read synchronization through `VK_KHR_synchronization2` buffer_memory_barrier2 rows, and proves visible/culled cases through SPIR-V environment-gated `MK_mavg_vulkan_gpu_culling_dispatch_tests` (`MK_VULKAN_TEST_MAVG_GPU_CULLING_DISPATCH_SPV`) while `MK_mavg_gpu_culling_tests` retain `executed_gpu_culling=false` value-only planner evidence. Vulkan compute-generated indirect consumption, mesh shaders, Metal readiness, Nanite equivalence/superiority, and broad optimization remain unclaimed. MAVG D3D12 Compute-Generated Indirect Consumption v1 (`mavg-d3d12-compute-generated-indirect-consumption-v1`) completed through PR #560. Post-land closeout returns `recommendedNextPlan.id` to `next-production-gap-selection`.

**Goal:** Extend the completed value-only `mavg-gpu-culling-indirect-v1` contract with the first actual GPU compute dispatch that writes MAVG packed indexed indirect argument and count buffers on Vulkan, records the required compute-write-to-indirect-read synchronization through backend-private `VK_KHR_synchronization2` barriers, and proves deterministic buffer contents through SPIR-V environment-gated readback without claiming Vulkan compute-generated indirect consumption through existing RHI paths, D3D12 changes, Metal compute, mesh shaders, Nanite equivalence/superiority, or broad optimization.

**Context:** `mavg-gpu-culling-indirect-v1` already exposes `MavgGpuCullingIndirectPlan`, `plan_mavg_gpu_culling_indirect_commands`, packed 20-byte indexed indirect argument rows, 4-byte count rows, fail-closed diagnostics, and D3D12/Vulkan synchronization requirement rows while keeping actual dispatch out of scope except on the completed D3D12 path. Completed D3D12 dispatch (`mavg-gpu-culling-dispatch-v1`) reuses backend-neutral encode helpers in `mavg_gpu_culling.hpp` and proves visible/culled buffer bytes through WARP-backed `MK_mavg_gpu_culling_dispatch_tests`. Completed Vulkan indirect execution through PR #541, PR #552, and closeout PR #553 executes CPU-generated upload argument and count buffers only. This child is the next stacked MAVG Vulkan GPU step because Khronos requires explicit memory barriers from compute shader write access to indirect-command-read before draw-indirect consumption, and the repository already records those requirement rows without executing Vulkan dispatch.

**Constraints:**

- Keep `MK_renderer` backend-neutral; keep Vulkan command buffers, descriptor sets, SPIR-V modules, storage/indirect buffers, and `VK_KHR_synchronization2` barriers backend-private.
- Start with Vulkan-only dispatch in v1; keep Vulkan compute-generated indirect consumption, Metal compute, D3D12 dispatch changes, and public native handles out of scope.
- Reuse existing `MavgGpuCullingIndirectCommandLayout`, `MavgGpuCullingDispatchClusterRow`, `build_mavg_gpu_culling_dispatch_cluster_rows`, and packed encode helpers from `mavg_gpu_culling.hpp`; do not invent a second indirect command format.
- Preserve the completed CPU-reference planning path and completed D3D12 dispatch path when Vulkan dispatch is not requested.
- Require reviewed default argument/count buffers with `BufferUsage::indirect` plus backend-private `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT`; prove writes through copy-source readback or host-visible staging consistent with existing Vulkan scaffold readback patterns.
- Insert the synchronization required by completed Vulkan `MavgGpuCullingSyncRequirement` rows before any indirect draw consumes compute-written buffers in follow-up work; this child proves dispatch bytes only.
- Do not claim compute-generated indirect consumption through existing RHI paths, count-buffer execution changes, mesh shaders, Work Graphs, package-visible MAVG backend readiness, benchmark superiority, or Nanite compatibility/equivalence/superiority.

**Done When:**

- A reviewed Vulkan compute dispatch path executes from `MavgGpuCullingDispatchClusterRow` inputs and writes deterministic packed argument/count buffer bytes for visible selected clusters.
- Vulkan dispatch records the `VK_KHR_synchronization2` barrier rows implied by the existing Vulkan sync requirement without exposing public buffer-state tracking.
- SPIR-V environment-gated tests prove non-zero visible clusters produce expected indirect bytes and culled clusters reduce the effective count relative to the CPU-reference planner baseline.
- `MK_mavg_gpu_culling_tests` retain `executed_gpu_culling=false` value-only planner evidence unchanged.
- Docs, manifest fragments, composed manifest, and static guards describe the exact Vulkan compute-dispatch-only claim after activation.
- Focused validation and full `tools/validate.ps1` pass before publication.

---

## Official Source Audit

Checked on 2026-06-10:

- Khronos Vulkan Specification `vkCmdDispatch`: compute dispatch executes on a compute queue/command buffer and requires explicit pipeline barriers before indirect argument consumption.
- Khronos Vulkan Specification `vkCmdPipelineBarrier2` / `VK_KHR_synchronization2`: memory barriers must transition access from `VK_ACCESS_2_SHADER_WRITE_BIT` at `VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT` to `VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT` at `VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT` before indirect draw consumption.
- Khronos `VkDrawIndexedIndirectCommand` and `vkCmdDrawIndexedIndirectCount`: indirect argument buffers require `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`; count buffers require `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`; offsets remain 4-byte aligned.
- Completed sibling audit in `docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md`: D3D12 dispatch buffer layout, sync requirement rows, cluster-row stride, and readback proof remain authoritative; this child must mirror semantics on Vulkan without regressing D3D12 dispatch-only claims or CPU-reference planner evidence.
- Completed sibling audit in `docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md`: CPU-upload Vulkan indirect draw and count-buffer execution remain authoritative; this child must not modify those paths.

## Scope

In scope:

- Vulkan-only MAVG visibility-culling compute dispatch writing packed indexed indirect argument and count buffers.
- Backend-private reuse of existing `mavg_gpu_culling.hpp` layout, cluster rows, and planner encode helpers.
- Deterministic SPIR-V environment-gated buffer readback proof for visible-cluster and culled-cluster cases.
- Conversion of existing fail-closed Vulkan dispatch evidence into GREEN execution coverage plus targeted invalid-input tests.

Out of scope:

- D3D12 dispatch changes or D3D12 compute-generated indirect consumption.
- Changing completed Vulkan `IRhiCommandList::draw_indexed_indirect` CPU-upload execution paths.
- Vulkan compute-generated indirect consumption without a separate follow-up plan.
- Public buffer resource-state tracking.
- Mesh shaders, Work Graphs, deformation, ray tracing, Metal readiness, benchmark superiority, or Nanite compatibility/equivalence/superiority.

## Sequencing

1. Wait for MAVG D3D12 Compute-Generated Indirect Consumption v1 implementation PR #560 merge to `main`.
2. Publish this draft plan-only PR without manifest activation.
3. Publish a separate activation PR: point `currentActivePlan` at this plan or return through the production-completion master plan with `recommendedNextPlan.id = mavg-vulkan-gpu-culling-dispatch-v1`, and extend activation static guards without landing implementation in the same PR.
4. Implement in a fresh linked worktree from merged `origin/main`, validate, commit, push, and open an independent implementation PR over `codex/mavg-vulkan-gpu-culling-dispatch-v1`.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp` (only if a reviewed backend-neutral dispatch descriptor is required)
- Modify: `engine/renderer/src/mavg_gpu_culling.cpp` (only if shared Vulkan/D3D12 dispatch helpers must be factored)
- Create: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_gpu_culling_dispatch.hpp`
- Create: `engine/rhi/vulkan/src/vulkan_mavg_gpu_culling_dispatch.cpp`
- Modify: `engine/rhi/vulkan/CMakeLists.txt`
- Create: `tests/unit/mavg_vulkan_gpu_culling_dispatch_tests.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp` (invalid-input coverage only if needed)
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-gpu-culling-indirect-v1.md` (completed sibling pointer only)
- Modify: `docs/superpowers/plans/2026-06-11-mavg-gpu-culling-dispatch-v1.md` (completed sibling pointer only)
- Modify: `docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md` (completed sibling pointer only after #560 lands)
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-105-mavg-gpu-culling-indirect.ps1`
- Create: `tools/check-ai-integration-113-mavg-vulkan-gpu-culling-dispatch.ps1`

## Tasks

- [x] Confirm Vulkan compute-to-indirect synchronization constraints through Khronos spec and retain completed GPU-culling planner plus D3D12 dispatch audit references.
- [x] Run a read-only rendering subagent audit and split Vulkan compute dispatch from Vulkan compute-generated indirect consumption work.
- [x] Add SPIR-V environment-gated dispatch/readback tests for visible and culled MAVG cluster cases.
- [x] Implement backend-private Vulkan compute dispatch writing packed argument/count buffers with required `VK_KHR_synchronization2` barriers.
- [x] Synchronize docs, plan registry, architecture spec, `004-modules.json` evidence, and static checks (`check-ai-integration-113`); keep `010-aiOperableProductionLoop.json` activation/closeout for a separate PR.
- [x] Run focused validation plus full `tools/validate.ps1`.
- [x] Publish a validated PR over `codex/mavg-vulkan-gpu-culling-dispatch-v1`.

## Validation Evidence

| Command | Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_mavg_gpu_culling_tests --output-on-failure` | PASS (value-only planner baseline retained). |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_mavg_vulkan_gpu_culling_dispatch_tests --output-on-failure` | PASS (SPIR-V environment-gated visible/culled readback vs CPU planner). |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Required before publication. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Required before publication. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Required before publication. |

## Non-Claims

This plan does not claim Vulkan compute-generated indirect consumption through existing RHI paths, D3D12 dispatch or compute-generated consumption changes, Vulkan indirect draw contract changes, public buffer state tracking, mesh shaders, Work Graphs, deformation, ray tracing, package-visible MAVG backend readiness, native handle exposure, benchmark superiority, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
