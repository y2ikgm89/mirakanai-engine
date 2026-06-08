# 2026-06-05 MAVG D3D12 Indexed Indirect Draw Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-d3d12-indexed-indirect-draw-execution-v1`

**Status:** Completed.

**Execution State:** Completed through PR #537 conventional upload re-land, PR #538 GPU culling plus RHI indirect contract re-land, and PR #539 D3D12 ExecuteIndirect execution re-land. The closeout returns `currentActivePlan` to the production-completion master plan and `recommendedNextPlan.id` to `next-production-gap-selection` while preserving no broad MAVG/Nanite/backend readiness claims.

**Goal:** Implement a clean D3D12 `ExecuteIndirect` execution path for `IRhiCommandList::draw_indexed_indirect` when the caller supplies a CPU-generated upload argument buffer and no count buffer, then prove visible indexed indirect rendering through WARP-backed D3D12 RHI tests without claiming GPU culling dispatch, count-buffer execution, Vulkan, Metal, mesh shaders, Nanite equivalence/superiority, or broad optimization.

**Context:** `mavg-gpu-culling-indirect-v1` produces deterministic value-only packed command rows. `mavg-rhi-indirect-draw-v1` adds `indirect_draw.hpp`, `IndexedIndirectDrawDesc`, `BufferUsage::indirect`, Null RHI deterministic argument/count execution, and Vulkan usage-bit planning. Native backend execution is still missing; D3D12 is the first backend because Microsoft Learn documents the exact command-signature and `ExecuteIndirect` requirements and this repository already has WARP-backed D3D12 visible readback tests.

**Constraints:**

- Keep the public RHI clean-break contract unchanged; do not add compatibility aliases.
- Keep `ID3D12CommandSignature`, resources, native command lists, and resource states backend-private.
- Support only `count_buffer.value == 0` in this child; count-buffer execution must fail closed.
- Require `BufferUsage::indirect | BufferUsage::copy_source` for this first D3D12 path so argument bytes are CPU-generated upload data and can be decoded for deterministic stats.
- Do not add a public buffer-state model or public `ResourceState::indirect_argument` in this child.
- Do not claim compute-generated indirect buffers, UAV-to-indirect synchronization, Vulkan execution, Metal ICB, mesh shaders, Work Graphs, benchmark superiority, or Nanite compatibility/equivalence/superiority.

**Done When:**

- D3D12 `IRhiCommandList::draw_indexed_indirect` validates render-pass/pipeline/VB/IB state, argument usage, alignment, stride, range, and fail-closed count-buffer support.
- D3D12 backend creates a private `D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED` command signature and calls `ID3D12GraphicsCommandList::ExecuteIndirect`.
- D3D12 RHI and backend-private stats update the existing direct/indexed draw rows plus indirect-specific rows from decoded CPU-generated argument bytes.
- WARP-backed `MK_d3d12_rhi_tests` proves a visible indexed indirect draw through texture readback and proves count-buffer rejection.
- Docs, manifest fragments, composed manifest, and static guards describe the exact D3D12-only claim.
- Focused validation and full `tools/validate.ps1` pass before publication.

---

## Official Source Audit

Checked on 2026-06-05:

- Microsoft Learn `ID3D12GraphicsCommandList::ExecuteIndirect` (`https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-executeindirect`): `MaxCommandCount` is exact when no count buffer is supplied; with a count buffer execution is `min(count, MaxCommandCount)`. Argument/count offsets are 4-byte aligned, buffers must be buffer resources, argument/count ranges must remain in bounds, and the debug layer requires argument/count buffers in `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT`.
- Microsoft Learn `ID3D12Device::CreateCommandSignature` (`https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createcommandsignature`): root signature is required only when the command signature updates bindings; a draw-only command signature may use `NULL` root signature.
- Microsoft Learn Direct3D 12 Indirect Drawing (`https://learn.microsoft.com/en-us/windows/win32/direct3d12/indirect-drawing`): command signatures define the argument buffer format, command type, changed bindings, and command byte stride.
- Context7 `/khronosgroup/vulkan-docs`: `vkCmdDrawIndexedIndirect` has stride/range requirements and `vkCmdDrawIndexedIndirectCount` is a separate feature/extension-gated path. Vulkan execution remains a separate future child.

## Scope

In scope:

- D3D12-only `ExecuteIndirect` for indexed indirect draws without a count buffer.
- Backend-private command-signature cache keyed by command stride.
- CPU-generated upload indirect argument buffer validation and deterministic command-byte decode for stats.
- Visible WARP-backed D3D12 texture readback proof.
- Explicit count-buffer fail-closed test.

Out of scope:

- Count-buffer execution.
- Compute-generated argument buffers and UAV-to-indirect synchronization.
- Public buffer resource-state tracking.
- Vulkan `vkCmdDrawIndexedIndirect` / `vkCmdDrawIndexedIndirectCount`.
- Metal indirect command buffers.
- actual GPU culling dispatch.
- Mesh shaders, Work Graphs, deformation, ray tracing, Metal readiness, benchmark superiority, or Nanite compatibility/equivalence/superiority.

## Files

- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-rhi-indirect-draw-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-106-mavg-rhi-indirect-draw.ps1`
- Create: `tools/check-ai-integration-107-mavg-d3d12-indexed-indirect-draw-execution.ps1`

## Tasks

- [x] Confirm D3D12/Vulkan official constraints through Microsoft Learn and Context7.
- [x] Run a read-only rendering subagent audit and split D3D12 from Vulkan/count-buffer work.
- [x] Add RED D3D12 RHI tests for visible indexed indirect draw execution and count-buffer rejection.
- [x] Implement backend-private D3D12 command signature cache and `ExecuteIndirect` path.
- [x] Update D3D12 RHI stats from decoded CPU-generated argument bytes.
- [x] Synchronize docs, plan registry, architecture spec, manifest fragments, composed manifest, and static checks.
- [x] Run focused validation plus full `tools/validate.ps1`.
- [ ] Publish a validated stacked PR over `codex/mavg-rhi-indirect-draw-v1`.

## Validation Evidence

| Command | Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | Passed before configure. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Passed for the new linked worktree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed after adding the D3D12-specific static chapter and prerequisite retention checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed after LF normalization. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed after manifest fragment sync and static chapter line-budget repair. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed after `tools/format.ps1` normalized C++ and text formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed for the D3D12 backend/private handle boundary. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/rhi/d3d12/src/d3d12_backend.cpp,tests/unit/d3d12_rhi_tests.cpp` | Passed for the changed D3D12 implementation and tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests` | Passed after D3D12 implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_d3d12_rhi_tests$"` | RED failed before implementation on unsupported `draw_indexed_indirect`; passed after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests MK_backend_scaffold_tests MK_d3d12_rhi_tests` | Passed before adjacent CTest. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_rhi_tests\|MK_backend_scaffold_tests\|MK_d3d12_rhi_tests"` | Passed after building adjacent test executables. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed: 19 static checks, build, tidy smoke, and 107/107 CTest tests. |

## Non-Claims

This plan does not claim count-buffer execution, compute-generated indirect buffers, GPU culling dispatch, public buffer state tracking, Vulkan indirect draw execution, Metal indirect command buffers, mesh shaders, Work Graphs, deformation, ray tracing, package-visible MAVG backend readiness, native handle exposure, benchmark superiority, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
