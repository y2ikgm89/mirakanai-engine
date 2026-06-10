# 2026-06-08 MAVG Vulkan Indexed Indirect Draw Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-vulkan-indexed-indirect-draw-execution-v1`

**Status:** Completed.

**Execution State:** Completed through PR #541. The closeout returned `currentActivePlan` to the production-completion master plan and `recommendedNextPlan.id` to `next-production-gap-selection` while preserving no broad MAVG/Nanite/backend readiness claims. Native D3D12 count-buffer `ExecuteIndirect` execution completed through the follow-up D3D12 count-buffer execution plan `mavg-d3d12-count-buffer-indirect-execution-v1` (PR #547).

**Goal:** Implement a clean Vulkan `vkCmdDrawIndexedIndirect` execution path for `IRhiCommandList::draw_indexed_indirect` when the caller supplies a CPU-generated upload argument buffer and no count buffer, then prove visible indexed indirect rendering through SPIR-V environment-gated `MK_backend_scaffold_tests` without claiming GPU culling dispatch, count-buffer execution, D3D12 changes, mesh shaders, Nanite equivalence/superiority, or broad optimization.

**Context:** `mavg-gpu-culling-indirect-v1` produces deterministic value-only packed command rows. `mavg-rhi-indirect-draw-v1` adds `indirect_draw.hpp`, `IndexedIndirectDrawDesc`, `BufferUsage::indirect`, Null RHI deterministic argument/count execution, and Vulkan usage-bit planning. `mavg-d3d12-indexed-indirect-draw-execution-v1` completed native D3D12 `ExecuteIndirect` execution with WARP-backed visible readback proof and count-buffer fail-closed evidence. Native Vulkan backend execution is still missing; Vulkan is the next backend because Khronos documents the exact `vkCmdDrawIndexedIndirect` stride/range requirements and this repository already has SPIR-V environment-gated visible readback tests in `backend_scaffold_tests.cpp`.

**Constraints:**

- Keep the public RHI clean-break contract unchanged; do not add compatibility aliases.
- Keep Vulkan command buffers, resources, barriers, and native handles backend-private.
- Support only `count_buffer.value == 0` in this child; count-buffer execution must fail closed.
- Require `BufferUsage::indirect | BufferUsage::copy_source` for this first Vulkan path so argument bytes are CPU-generated upload data and can be decoded for deterministic stats.
- Do not add a public buffer-state model or public `ResourceState::indirect_argument` in this child.
- Do not claim compute-generated indirect buffers, UAV/compute-to-indirect synchronization, D3D12 changes, Metal ICB, mesh shaders, Work Graphs, benchmark superiority, or Nanite compatibility/equivalence/superiority.

**Done When:**

- Vulkan `IRhiCommandList::draw_indexed_indirect` validates render-pass/pipeline/VB/IB state, argument usage, alignment, stride, range, and fail-closed count-buffer support.
- Vulkan backend records `vkCmdDrawIndexedIndirect` with reviewed stride/range validation over CPU-generated upload argument buffers.
- Vulkan RHI and backend-private stats update the existing direct/indexed draw rows plus indirect-specific rows from decoded CPU-generated argument bytes.
- SPIR-V environment-gated `MK_backend_scaffold_tests` proves a visible indexed indirect draw through texture readback and proves count-buffer rejection.
- Docs, manifest fragments, composed manifest, and static guards describe the exact Vulkan-only claim.
- Focused validation and full `tools/validate.ps1` pass before publication.

---

## Official Source Audit

Checked on 2026-06-08:

- Context7 `/khronosgroup/vulkan-docs`: `vkCmdDrawIndexedIndirect` reads indexed draw parameters from a buffer; `drawCount` is exact when no count-buffer path is used; `stride` must be a multiple of 4 and at least `sizeof(VkDrawIndexedIndirectCommand)`; argument offsets and ranges must remain in bounds; the buffer must have been created with `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`.
- Context7 `/khronosgroup/vulkan-docs`: `vkCmdDrawIndexedIndirectCount` is a separate feature/extension-gated path and must remain fail-closed in this child.
- Context7 `/khronosgroup/vulkan-docs`: compute writes feeding indirect draws require synchronization from compute shader write access to draw-indirect/indirect-command-read access.
- Microsoft Learn `ID3D12GraphicsCommandList::ExecuteIndirect` remains the completed D3D12 sibling; this child must not regress or broaden D3D12 claims.

## Scope

In scope:

- Vulkan-only `vkCmdDrawIndexedIndirect` for indexed indirect draws without a count buffer.
- CPU-generated upload indirect argument buffer validation and deterministic command-byte decode for stats.
- Visible SPIR-V environment-gated texture readback proof in `tests/unit/backend_scaffold_tests.cpp`.
- Explicit count-buffer fail-closed test.

Out of scope:

- Count-buffer execution (`vkCmdDrawIndexedIndirectCount`).
- Compute-generated argument buffers and compute-to-indirect synchronization.
- Public buffer resource-state tracking.
- D3D12 `ExecuteIndirect` changes.
- Metal indirect command buffers.
- Actual GPU culling compute dispatch.
- Mesh shaders, Work Graphs, deformation, ray tracing, Metal readiness, benchmark superiority, or Nanite compatibility/equivalence/superiority.

## Files

- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-draw-execution-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-106-mavg-rhi-indirect-draw.ps1`
- Create: `tools/check-ai-integration-108-mavg-vulkan-indexed-indirect-draw-execution.ps1`

## Tasks

- [ ] Confirm Vulkan official constraints through Context7 and retain completed D3D12 sibling audit references.
- [ ] Run a read-only rendering subagent audit and split Vulkan from count-buffer/compute-generated work.
- [ ] Add RED SPIR-V environment-gated `MK_backend_scaffold_tests` for visible indexed indirect draw execution and count-buffer rejection.
- [ ] Implement backend-private Vulkan `draw_indexed_indirect` validation and `vkCmdDrawIndexedIndirect` recording.
- [ ] Update Vulkan RHI stats from decoded CPU-generated argument bytes.
- [ ] Synchronize docs, plan registry, architecture spec, manifest fragments, composed manifest, and static checks.
- [ ] Run focused validation plus full `tools/validate.ps1`.
- [ ] Publish a validated stacked PR over `codex/mavg-vulkan-indexed-indirect-draw-v1`.

## Validation Evidence

| Command | Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | Pending before configure. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Pending for the linked worktree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pending after adding the Vulkan-specific static chapter and prerequisite retention checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pending after LF normalization. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pending after manifest fragment sync and static chapter line-budget repair. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pending after formatting changed C++ and text surfaces. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pending for the Vulkan backend/private handle boundary. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/rhi/vulkan/src/vulkan_backend.cpp,tests/unit/backend_scaffold_tests.cpp` | Pending for the changed Vulkan implementation and tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests` | Pending after Vulkan implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_backend_scaffold_tests$"` | RED expected before implementation on unsupported `draw_indexed_indirect`; GREEN expected after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests MK_backend_scaffold_tests MK_d3d12_rhi_tests` | Pending before adjacent CTest. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_rhi_tests\|MK_backend_scaffold_tests\|MK_d3d12_rhi_tests"` | Pending after building adjacent test executables. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pending before publication. |

## Non-Claims

This plan does not claim count-buffer execution, count-buffer Vulkan execution, compute-generated indirect buffers, actual GPU culling dispatch, public buffer state tracking, D3D12 changes, Metal indirect command buffers, mesh shaders, Work Graphs, deformation, ray tracing, package-visible MAVG backend readiness, native handle exposure, benchmark superiority, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
