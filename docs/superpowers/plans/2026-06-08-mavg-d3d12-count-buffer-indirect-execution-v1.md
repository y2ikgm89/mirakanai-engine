# 2026-06-08 MAVG D3D12 Count-Buffer Indirect Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-d3d12-count-buffer-indirect-execution-v1`

**Status:** Completed.

**Execution State:** Completed through PR #547 (merge commit `731c8231`) and post-land closeout PR #549. Follow-up Vulkan count-buffer execution is the active child plan `mavg-vulkan-count-buffer-indirect-execution-v1` after a separate activation PR lands over merged `origin/main`; implementation remains fail-closed until the independent implementation PR completes the sibling slice.

**Goal:** Extend the completed D3D12 no-count `ExecuteIndirect` path so `IRhiCommandList::draw_indexed_indirect` executes when the caller supplies a CPU-generated upload count buffer plus argument buffer, records `min(count, max_draw_count)` semantics consistent with Null RHI, and proves visible indexed indirect rendering through WARP-backed D3D12 RHI tests without claiming compute-generated count buffers, GPU culling dispatch, count-buffer Vulkan execution, Metal, mesh shaders, Nanite equivalence/superiority, or broad optimization.

**Context:** `mavg-rhi-indirect-draw-v1` already defines `IndexedIndirectDrawDesc::count_buffer`, `indexed_indirect_draw_count_buffer_size_bytes`, Null RHI deterministic count-buffer execution (`std::min(count_buffer_value, desc.max_draw_count)`), and `indexed_indirect_count_buffer_reads` / `last_indexed_indirect_count_buffer_value` stats. `mavg-d3d12-indexed-indirect-draw-execution-v1` completed native D3D12 `ExecuteIndirect` for CPU-generated upload argument buffers with count-buffer fail-closed evidence. `mavg-vulkan-indexed-indirect-draw-execution-v1` (PR #541) completed the Vulkan no-count sibling and keeps count-buffer Vulkan execution fail-closed. This child completed D3D12 count-buffer `ExecuteIndirect` execution per Microsoft Learn count-buffer semantics with WARP-backed visible readback tests converted from the prior count-buffer rejection coverage.

**Constraints:**

- Keep the public RHI clean-break contract unchanged; do not add compatibility aliases.
- Keep `ID3D12CommandSignature`, resources, native command lists, and resource states backend-private.
- Support only CPU-generated upload count buffers in v1: require `BufferUsage::indirect | BufferUsage::copy_source` on the count buffer so bytes can be read back for deterministic stats, matching the existing no-count argument-buffer rule.
- Preserve the completed no-count path when `count_buffer.value == 0`.
- When a count buffer is supplied, execute `min(count_buffer_value, max_draw_count)` commands through `ID3D12GraphicsCommandList::ExecuteIndirect` with a non-null count resource and 4-byte-aligned `count_buffer_offset`.
- Do not add a public buffer-state model or public `ResourceState::indirect_argument` in this child.
- Do not claim compute-generated count/argument buffers, UAV-to-indirect synchronization, Vulkan `vkCmdDrawIndexedIndirectCount`, Metal ICB, actual GPU culling dispatch, mesh shaders, Work Graphs, benchmark superiority, or Nanite compatibility/equivalence/superiority.

**Done When:**

- D3D12 `IRhiCommandList::draw_indexed_indirect` validates count-buffer usage, alignment, range, and pairs it with the existing argument-buffer validation when `has_indexed_indirect_count_buffer(desc)` is true.
- D3D12 backend calls `ExecuteIndirect` with the existing draw-indexed command signature, a count resource, and `CountBufferOffset`, executing `min(count, max_draw_count)` draws per Microsoft Learn semantics.
- D3D12 RHI stats increment `indexed_indirect_count_buffer_reads`, set `last_indexed_indirect_count_buffer_value` from the decoded upload count, and set `last_indexed_indirect_executed_draw_count` to the effective executed command count while preserving existing direct/indexed draw stat rows.
- WARP-backed `MK_d3d12_rhi_tests` proves visible indexed indirect draw execution with a count buffer limiting draws below `max_draw_count`, proves zero-count execution submits no visible draw, and retains focused invalid-input rejection coverage.
- Docs, manifest fragments, composed manifest, and static guards describe the exact D3D12 count-buffer-only claim after activation.
- Focused validation and full `tools/validate.ps1` pass before publication.

---

## Official Source Audit

Checked on 2026-06-08:

- Microsoft Learn `ID3D12GraphicsCommandList::ExecuteIndirect` (`https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-executeindirect`): when `pCountBuffer` is non-null, the number of commands executed is `min(*pCountBuffer, MaxCommandCount)`; argument and count offsets are 4-byte aligned; both buffers must be buffer resources with in-bounds ranges; the debug layer expects indirect-argument resource state for argument and count buffers.
- Microsoft Learn Direct3D 12 Indirect Drawing (`https://learn.microsoft.com/en-us/windows/win32/direct3d12/indirect-drawing`): command signatures define argument layout and stride; count-buffer execution is the standard path for GPU-driven draw counts once buffers are synchronized.
- Completed sibling audit in `docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-draw-execution-v1.md`: no-count `ExecuteIndirect` uses `nullptr` count buffer and exact `MaxCommandCount`; this child must not regress that path.
- Context7 `/khronosgroup/vulkan-docs`: `vkCmdDrawIndexedIndirectCount` remains a separate Vulkan-only future child and must stay fail-closed on Vulkan until explicitly planned.

## Scope

In scope:

- D3D12-only count-buffer `ExecuteIndirect` for indexed indirect draws with CPU-generated upload argument and count buffers.
- Backend-private reuse of the existing draw-indexed command signature cache keyed by command stride.
- Deterministic upload count-buffer decode for stats and effective command decode bounded by `min(count, max_draw_count)`.
- Visible WARP-backed D3D12 texture readback proof for count-limited execution and zero-count execution.
- Conversion of the existing count-buffer rejection test into GREEN execution coverage plus targeted invalid-input tests.

Out of scope:

- Compute-generated count or argument buffers and UAV-to-indirect synchronization.
- Public buffer resource-state tracking.
- Vulkan `vkCmdDrawIndexedIndirectCount`.
- Metal indirect command buffers.
- Actual GPU culling compute dispatch writing count buffers.
- Mesh shaders, Work Graphs, deformation, ray tracing, Metal readiness, benchmark superiority, or Nanite compatibility/equivalence/superiority.

## Sequencing

1. Wait for PR #541 merge to `main`.
2. Publish the post-merge Vulkan closeout PR (#540 pattern): mark `mavg-vulkan-indexed-indirect-draw-execution-v1` completed, return `currentActivePlan` to the production-completion master plan, set `recommendedNextPlan.id` to `next-production-gap-selection`, and extend closeout static guards.
3. Activate this plan in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` with `recommendedNextPlan.id = mavg-d3d12-count-buffer-indirect-execution-v1`.
4. Implement in a fresh linked worktree from merged `origin/main`, validate, commit, push, and open an independent implementation PR.

## Files

- Modify: `engine/rhi/include/mirakana/rhi/indirect_draw.hpp`
- Modify: `engine/rhi/src/indirect_draw.cpp`
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-d3d12-indexed-indirect-draw-execution-v1.md`
- Modify: `docs/superpowers/plans/2026-06-08-mavg-vulkan-indexed-indirect-draw-execution-v1.md` (after #541 merge; completed sibling pointer only)
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-106-mavg-rhi-indirect-draw.ps1`
- Modify: `tools/check-ai-integration-107-mavg-d3d12-indexed-indirect-draw-execution.ps1`
- Create: `tools/check-ai-integration-109-mavg-d3d12-count-buffer-indirect-execution.ps1`

## Tasks

- [x] Confirm D3D12 count-buffer official constraints through Microsoft Learn and retain completed no-count sibling audit references.
- [x] Run a read-only rendering subagent audit and split D3D12 count-buffer execution from compute-generated/Vulkan count-buffer work.
- [x] Add shared RHI helper(s) for effective indexed indirect draw count and bounded command decode when a count buffer is present.
- [x] Convert RED `MK_d3d12_rhi_tests` count-buffer rejection coverage into GREEN visible execution, zero-count, and invalid-input tests.
- [x] Implement backend-private D3D12 count-buffer validation and `ExecuteIndirect` recording with non-null count resource.
- [x] Update D3D12 RHI stats for count-buffer reads and effective executed command counts.
- [x] After PR #541 merge and Vulkan closeout, synchronize docs, plan registry, architecture spec, manifest fragments, composed manifest, and static checks.
- [x] Run focused validation plus full `tools/validate.ps1`.
- [x] Publish a validated PR over `codex/mavg-d3d12-count-buffer-indirect-execution-v1` (PR #547).
- [x] Publish post-land closeout PR returning `currentActivePlan` to the production-completion master plan.

## Validation Evidence

| Command | Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | Passed 2026-06-08 before linked-worktree configure. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Passed 2026-06-08 in the linked worktree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed 2026-06-09 after D3D12 count-buffer static chapter 109 and prerequisite retention checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed 2026-06-09 after LF normalization. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed 2026-06-09 after manifest fragment sync. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed 2026-06-09 after formatting changed C++ and text surfaces. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed 2026-06-08 for the D3D12 backend/private handle boundary. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/rhi/d3d12/src/d3d12_backend.cpp,engine/rhi/src/indirect_draw.cpp,tests/unit/d3d12_rhi_tests.cpp` | Passed 2026-06-08 for changed implementation and tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests` | Passed 2026-06-08 after D3D12 count-buffer implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_d3d12_rhi_tests$"` | GREEN 2026-06-08: count-limited, zero-count, and invalid-input coverage passed on WARP. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests MK_d3d12_rhi_tests` | Passed 2026-06-08 before adjacent CTest. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_rhi_tests\|MK_d3d12_rhi_tests"` | Passed 2026-06-08 after building adjacent test executables. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed 2026-06-09 before PR #547 publication; closeout re-run pending post-land sync. |

## Non-Claims

This plan does not claim compute-generated count or argument buffers, GPU culling dispatch, public buffer state tracking, count-buffer Vulkan execution, Metal indirect command buffers, mesh shaders, Work Graphs, deformation, ray tracing, package-visible MAVG backend readiness, native handle exposure, benchmark superiority, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
