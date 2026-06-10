# 2026-06-10 MAVG Vulkan Count-Buffer Indirect Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-vulkan-count-buffer-indirect-execution-v1`

**Status:** Completed.

**Execution State:** Completed through PR #552 (merge commit `a436fc7a`) and post-land closeout PR #553. The closeout returned `currentActivePlan` to the production-completion master plan and `recommendedNextPlan.id` to `next-production-gap-selection` while preserving no broad MAVG/Nanite/backend readiness claims. Follow-up actual GPU culling dispatch is the active child plan `mavg-gpu-culling-dispatch-v1` after a separate activation PR lands over merged `origin/main`; implementation remains fail-closed until the independent implementation PR completes the sibling slice.

**Goal:** Extend the completed Vulkan no-count `vkCmdDrawIndexedIndirect` path so `IRhiCommandList::draw_indexed_indirect` executes when the caller supplies a CPU-generated upload count buffer plus argument buffer, records `min(count, max_draw_count)` semantics consistent with Null RHI through `vkCmdDrawIndexedIndirectCount`, and proves visible indexed indirect rendering through SPIR-V environment-gated `MK_backend_scaffold_tests` without claiming compute-generated count buffers, GPU culling dispatch, D3D12 changes, Metal, mesh shaders, Nanite equivalence/superiority, or broad optimization.

**Context:** `mavg-rhi-indirect-draw-v1` already defines `IndexedIndirectDrawDesc::count_buffer`, `indexed_indirect_draw_count_buffer_size_bytes`, Null RHI deterministic count-buffer execution (`std::min(count_buffer_value, desc.max_draw_count)`), and `indexed_indirect_count_buffer_reads` / `last_indexed_indirect_count_buffer_value` stats. Shared helpers `decode_indexed_indirect_count_buffer_value` and `effective_indexed_indirect_draw_count` already live in `indirect_draw.hpp` and are used by Null RHI and the completed D3D12 count-buffer path. `mavg-vulkan-indexed-indirect-draw-execution-v1` (PR #541) completed native Vulkan `vkCmdDrawIndexedIndirect` for CPU-generated upload argument buffers with count-buffer fail-closed evidence in `vulkan_backend.cpp` and `MK_backend_scaffold_tests`. Vulkan count-buffer execution is the next backend follow-up because Khronos documents the exact `vkCmdDrawIndexedIndirectCount` semantics and this repository already has SPIR-V environment-gated visible readback tests plus a RED count-buffer rejection test to convert.

**Constraints:**

- Keep the public RHI clean-break contract unchanged; do not add compatibility aliases.
- Keep Vulkan command buffers, resources, barriers, and native handles backend-private.
- Support only CPU-generated upload count buffers in v1: require `BufferUsage::indirect | BufferUsage::copy_source` on the count buffer so bytes can be read back for deterministic stats, matching the existing no-count argument-buffer rule and the completed D3D12 count-buffer rule.
- Preserve the completed no-count path when `count_buffer.value == 0`.
- When a count buffer is supplied, execute `min(count_buffer_value, max_draw_count)` draws through `vkCmdDrawIndexedIndirectCount` with 4-byte-aligned `count_buffer_offset` and `argument_buffer_offset`.
- Reuse existing shared helpers `decode_indexed_indirect_count_buffer_value` and `effective_indexed_indirect_draw_count` from `indirect_draw.hpp`; do not duplicate count decode or clamp logic in the Vulkan backend.
- Require Vulkan 1.2 core or `VK_KHR_draw_indirect_count` support before recording the count-buffer path; fail closed when the device lacks the feature.
- Do not add a public buffer-state model or public `ResourceState::indirect_argument` in this child.
- Do not claim compute-generated count/argument buffers, UAV-to-indirect synchronization, D3D12 changes, Metal ICB, actual GPU culling dispatch, mesh shaders, Work Graphs, benchmark superiority, or Nanite compatibility/equivalence/superiority.

**Done When:**

- Vulkan `IRhiCommandList::draw_indexed_indirect` validates count-buffer usage, alignment, range, and pairs it with the existing argument-buffer validation when `has_indexed_indirect_count_buffer(desc)` is true.
- Vulkan backend calls `vkCmdDrawIndexedIndirectCount` with reviewed stride/range validation over CPU-generated upload argument and count buffers, executing `min(count, max_draw_count)` draws per Khronos semantics.
- Vulkan RHI stats increment `indexed_indirect_count_buffer_reads`, set `last_indexed_indirect_count_buffer_value` from the decoded upload count via shared helpers, and set `last_indexed_indirect_executed_draw_count` to the effective executed command count while preserving existing direct/indexed draw stat rows.
- SPIR-V environment-gated `MK_backend_scaffold_tests` proves visible indexed indirect draw execution with a count buffer limiting draws below `max_draw_count`, proves zero-count execution submits no visible draw, and retains focused invalid-input rejection coverage.
- Docs, manifest fragments, composed manifest, and static guards describe the exact Vulkan count-buffer-only claim after activation.
- Focused validation and full `tools/validate.ps1` pass before publication.

---

## Official Source Audit

Checked on 2026-06-10:

- Khronos Vulkan Specification `vkCmdDrawIndexedIndirectCount` (`https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdDrawIndexedIndirectCount.html`): the number of draws executed is `min(countBuffer contents, maxDrawCount)`; `countBufferOffset` and `byteOffset` must be multiples of 4; the argument buffer must have been created with `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`; the count buffer must have been created with `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`; requires Vulkan 1.2 or `VK_KHR_draw_indirect_count`.
- Context7 `/khronosgroup/vulkan-docs`: `vkCmdDrawIndexedIndirectCount` reads a 32-bit count from a device buffer and executes indexed indirect draws bounded by `maxDrawCount`; stride must be a multiple of 4 and at least `sizeof(VkDrawIndexedIndirectCommand)`; compute writes feeding indirect draws require synchronization from compute shader write access to draw-indirect/indirect-command-read access.
- Completed sibling audit in `docs/superpowers/plans/2026-06-08-mavg-vulkan-indexed-indirect-draw-execution-v1.md`: no-count `vkCmdDrawIndexedIndirect` uses exact `drawCount`; this child must not regress that path.
- Completed D3D12 sibling audit in `docs/superpowers/plans/2026-06-08-mavg-d3d12-count-buffer-indirect-execution-v1.md`: D3D12 count-buffer `ExecuteIndirect` uses shared helpers and `min(count, max_draw_count)` semantics; this child must not modify D3D12 code.

## Scope

In scope:

- Vulkan-only count-buffer `vkCmdDrawIndexedIndirectCount` for indexed indirect draws with CPU-generated upload argument and count buffers.
- Backend-private reuse of existing shared count decode helpers in `indirect_draw.hpp`.
- Deterministic upload count-buffer decode for stats and effective command decode bounded by `min(count, max_draw_count)`.
- Visible SPIR-V environment-gated texture readback proof in `tests/unit/backend_scaffold_tests.cpp` for count-limited execution and zero-count execution.
- Conversion of the existing count-buffer rejection test into GREEN execution coverage plus targeted invalid-input tests.

Out of scope:

- Compute-generated count or argument buffers and UAV-to-indirect synchronization.
- Public buffer resource-state tracking.
- D3D12 `ExecuteIndirect` changes.
- Metal indirect command buffers.
- Actual GPU culling compute dispatch writing count buffers.
- Mesh shaders, Work Graphs, deformation, ray tracing, Metal readiness, benchmark superiority, or Nanite compatibility/equivalence/superiority.

## Sequencing

1. Wait for D3D12 count-buffer closeout PR #549 merge to `main`.
2. Publish a separate activation PR: point `currentActivePlan` at this plan or return through the production-completion master plan with `recommendedNextPlan.id = mavg-vulkan-count-buffer-indirect-execution-v1`, and extend activation static guards without landing implementation in the same PR.
3. Implement in a fresh linked worktree from merged `origin/main`, validate, commit, push, and open an independent implementation PR over `codex/mavg-vulkan-count-buffer-indirect-execution-v1`.

## Files

- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-08-mavg-vulkan-indexed-indirect-draw-execution-v1.md` (completed sibling pointer only)
- Modify: `docs/superpowers/plans/2026-06-08-mavg-d3d12-count-buffer-indirect-execution-v1.md` (completed sibling pointer only)
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-106-mavg-rhi-indirect-draw.ps1`
- Modify: `tools/check-ai-integration-108-mavg-vulkan-indexed-indirect-draw-execution.ps1`
- Create: `tools/check-ai-integration-110-mavg-vulkan-count-buffer-indirect-execution.ps1`

## Tasks

- [x] Confirm Vulkan count-buffer official constraints through Khronos spec and retain completed no-count Vulkan and D3D12 count-buffer sibling audit references.
- [x] Run a read-only rendering subagent audit and split Vulkan count-buffer execution from compute-generated/D3D12 work.
- [x] Convert RED SPIR-V environment-gated `MK_backend_scaffold_tests` count-buffer rejection coverage into GREEN visible execution, zero-count, and invalid-input tests.
- [x] Implement backend-private Vulkan count-buffer validation and `vkCmdDrawIndexedIndirectCount` recording with shared helper reuse.
- [x] Update Vulkan RHI stats for count-buffer reads and effective executed command counts.
- [x] After PR #549 merge and activation PR, synchronize docs, plan registry, architecture spec, manifest fragments, composed manifest, and static checks.
- [ ] Run focused validation plus full `tools/validate.ps1`.
- [ ] Publish a validated PR over `codex/mavg-vulkan-count-buffer-indirect-execution-v1`.

## Validation Evidence

| Command | Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | Pending before configure. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Pending for the linked worktree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pending after adding the Vulkan count-buffer static chapter and prerequisite retention checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pending after LF normalization. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pending after manifest fragment sync. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pending after formatting changed C++ and text surfaces. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pending for the Vulkan backend/private handle boundary. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/rhi/vulkan/src/vulkan_backend.cpp,tests/unit/backend_scaffold_tests.cpp` | Pending for changed implementation and tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests` | Pending after Vulkan count-buffer implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_backend_scaffold_tests$"` | RED expected before implementation; GREEN expected after count-buffer path lands. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests MK_backend_scaffold_tests MK_d3d12_rhi_tests` | Pending before adjacent CTest. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_rhi_tests\|MK_backend_scaffold_tests\|MK_d3d12_rhi_tests"` | Pending after building adjacent test executables. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pending before publication. |

## Non-Claims

This plan does not claim compute-generated count or argument buffers, GPU culling dispatch, public buffer state tracking, D3D12 changes, Metal indirect command buffers, mesh shaders, Work Graphs, deformation, ray tracing, package-visible MAVG backend readiness, native handle exposure, benchmark superiority, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
