# 2026-06-11 MAVG Vulkan Compute-Generated Indirect Consumption v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the Vulkan RHI consume MAVG GPU-culling generated indexed indirect argument and count buffers through the existing `IRhiCommandList::draw_indexed_indirect` path.

**Architecture:** Keep the public RHI contract clean-break and backend-neutral by reusing `BufferUsage::indirect | BufferUsage::storage` as the reviewed compute-generated buffer contract already used by the D3D12 sibling. Keep Vulkan buffer ownership, synchronization2 barriers, command recording, and readback proof backend-private. Preserve CPU-upload Vulkan indirect draw behavior unchanged.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, `MK_rhi_vulkan`, Vulkan synchronization2, SPIR-V environment-gated tests, CMake/CTest, PowerShell validation tools.

---

**Plan ID:** `mavg-vulkan-compute-generated-indirect-consumption-v1`

**Status:** Completed locally; publication PR pending. Not selected as `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Context

Completed prerequisites:

- `mavg-gpu-culling-indirect-v1` provides `MavgGpuCullingIndirectPlan`, packed five-field indexed indirect command rows, count-buffer rows, and D3D12/Vulkan compute-write-to-indirect-read synchronization requirement rows.
- `mavg-rhi-indirect-draw-v1` provides `indirect_draw.hpp`, `IndexedIndirectDrawDesc`, `BufferUsage::indirect`, `IRhiCommandList::draw_indexed_indirect`, and Null RHI deterministic execution.
- `mavg-vulkan-indexed-indirect-draw-execution-v1` and `mavg-vulkan-count-buffer-indirect-execution-v1` execute CPU-generated upload argument/count buffers through `vkCmdDrawIndexedIndirect` and `vkCmdDrawIndexedIndirectCount`.
- `mavg-vulkan-gpu-culling-dispatch-v1` writes MAVG packed argument/count buffers through Vulkan compute dispatch and records `VK_KHR_synchronization2` buffer-memory barrier evidence, but it proves bytes through readback rather than consuming those buffers through the RHI draw path.
- `mavg-d3d12-compute-generated-indirect-consumption-v1` already proved the symmetric D3D12 path: GPU-written MAVG buffers can be consumed by `draw_indexed_indirect` when the backend records private UAV-to-indirect transitions.

This plan closes the Vulkan half of that compute-to-draw gap. It must not broaden the claim to mesh shaders, package-visible backend readiness, async overlap, GPU memory pressure, Metal, ray tracing, deformation, benchmark superiority, or Nanite compatibility/equivalence/superiority.

## Official Source Audit

Checked during plan authoring on 2026-06-11:

- Khronos Vulkan `vkCmdDrawIndexedIndirectCount`: `countBuffer` supplies an unsigned 32-bit draw count during command execution, and execution behaves like indexed indirect draw with a device-read count.
- Khronos Vulkan `VK_KHR_draw_indirect_count`: the extension exists to let applications do GPU culling in compute and then execute the generated draw count without host intervention.
- Khronos Vulkan synchronization2 guidance: compute shader writes that become indirect command reads need an availability/visibility dependency from `VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT` / `VK_ACCESS_2_SHADER_WRITE_BIT` to `VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT` / `VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT`.
- Microsoft vcpkg CMake integration documentation: CMake integration uses `<vcpkg root>/scripts/buildsystems/vcpkg.cmake`. This repository keeps manifest install disabled during configure and routes dependency installation through `tools/bootstrap-deps.ps1`; a missing `external/vcpkg` clone is a host/worktree setup blocker, not a reason to move dependency restore into CMake.

Context7 note: Context7 lookup was attempted for current toolchain docs, but the connector returned an expired OAuth token. Use Context7 again during implementation if the connector is reauthenticated; otherwise use official Khronos and Microsoft documentation.

## Files

- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_gpu_culling_dispatch.hpp`
- Create: `tests/unit/mavg_vulkan_compute_generated_indirect_consumption_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md`
- Modify: `docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-114-mavg-vulkan-compute-generated-indirect-consumption.ps1`
- Modify: `tools/static-contract-ledger.ps1` only if the numeric chapter discovery requires explicit registration for chapter 114.

## Task 1 - Plan Activation And RED Coverage

**Files:**

- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `docs/superpowers/plans/README.md`
- Create: `tools/check-ai-integration-114-mavg-vulkan-compute-generated-indirect-consumption.ps1`
- Create: `tests/unit/mavg_vulkan_compute_generated_indirect_consumption_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add this plan to the plan registry after PR #565/#566 merged and keep it as a completed local child slice rather than changing `currentActivePlan`.
- [x] Keep `unsupportedProductionGaps = []`, return `recommendedNextPlan.id` to `next-production-gap-selection`, and compose `engine/agent/manifest.json`.
- [x] Add static guard chapter 114 requiring the plan id, new test target name `MK_mavg_vulkan_compute_generated_indirect_consumption_tests`, `is_compute_generated_indexed_indirect_buffer`, `dispatch_mavg_gpu_culling_indirect`, `vkCmdDrawIndexedIndirectCount`, `record_runtime_buffer_memory_barrier2`, and explicit non-claims for mesh shaders, Metal, Nanite, ray tracing, deformation, and broad optimization.
- [x] Add RED test skeleton `MK_TEST("mavg vulkan dispatch plus draw consumes compute generated indirect buffers")` that creates MAVG dispatch rows, dispatches into Vulkan-owned argument/count buffers, then calls `IRhiCommandList::draw_indexed_indirect` on those same buffers.
- [x] Add RED test skeleton `MK_TEST("mavg vulkan dispatch plus draw respects culled cluster count")` that proves a culled cluster dispatch produces reduced-count draw evidence when consumed through `vkCmdDrawIndexedIndirectCount`.
- [x] Register `MK_mavg_vulkan_compute_generated_indirect_consumption_tests` under the existing Vulkan test gate near `MK_mavg_vulkan_gpu_culling_dispatch_tests`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_vulkan_compute_generated_indirect_consumption_tests
```

Expected before implementation: configure may be blocked if `external/vcpkg/scripts/buildsystems/vcpkg.cmake` is missing; otherwise build fails because the new compute-generated Vulkan consumption helpers do not exist yet.

## Task 2 - Vulkan Compute-Generated Buffer Contract

**Files:**

- Modify: `engine/rhi/include/mirakana/rhi/indirect_draw.hpp`
- Modify: `engine/rhi/src/indirect_draw.cpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`

- [x] Reuse the existing `is_compute_generated_indexed_indirect_buffer(BufferUsage usage)` helper. If it is already sufficient, do not add a new public API.
- [x] In Vulkan `draw_indexed_indirect`, accept argument and count buffers when `is_compute_generated_indexed_indirect_buffer(desc.argument_buffer_usage)` and `is_compute_generated_indexed_indirect_buffer(desc.count_buffer_usage)` are true.
- [x] Preserve the existing CPU-upload `copy_source` path and its decoded stats behavior for `BufferUsage::indirect | BufferUsage::copy_source`.
- [x] Fail closed when argument/count buffers are neither CPU-upload nor compute-generated, when count-buffer offset alignment is invalid, or when buffer ranges are too small.
- [x] Keep native Vulkan handles private to `MK_rhi_vulkan`; do not expose public state tracking.

## Task 3 - Backend-Private Synchronization And Draw Consumption

**Files:**

- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_gpu_culling_dispatch.hpp`

- [x] Add `BufferHandle external_argument_buffer`, `BufferHandle external_count_buffer`, and `bool leave_indirect_argument_state_for_consumption` fields to the Vulkan MAVG dispatch descriptor, plus an `IRhiDevice&` overload that writes into RHI-owned buffers without exposing `VkBuffer` or `VkDevice`.
- [x] Keep the new RHI-owned-buffer path backend-private in `vulkan_backend.cpp`; the completed standalone readback helper remains unchanged in `vulkan_mavg_gpu_culling_dispatch.cpp`.
- [x] Ensure compute-written argument and count buffers receive synchronization2 buffer barriers from `VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT` / `VK_ACCESS_2_SHADER_WRITE_BIT` to `VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT` / `VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT` before `vkCmdDrawIndexedIndirectCount` records.
- [x] Record stats/evidence rows for the barrier count and conservative draw-call evidence without reading GPU-written command bytes on the CPU path.
- [x] Preserve the completed standalone Vulkan dispatch readback helper behavior used by `MK_mavg_vulkan_gpu_culling_dispatch_tests`.

## Task 4 - GREEN Tests And Regression Checks

**Files:**

- Modify: `tests/unit/mavg_vulkan_compute_generated_indirect_consumption_tests.cpp`
- Modify: `tests/unit/mavg_vulkan_gpu_culling_dispatch_tests.cpp` only if shared test helpers are extracted.
- Modify: `tests/unit/backend_scaffold_tests.cpp` only if invalid-input coverage belongs in the existing Vulkan RHI suite.

- [x] Make `mavg vulkan dispatch plus draw consumes compute generated indirect buffers` pass with visible readback proof on hosts with `MK_VULKAN_TEST_MAVG_GPU_CULLING_DISPATCH_SPV`.
- [x] Make `mavg vulkan dispatch plus draw respects culled cluster count` pass with reduced-count evidence.
- [x] Add host-gated fail-closed coverage for compute-generated buffers missing `BufferUsage::storage` or `BufferUsage::indirect`.
- [x] Add fail-closed coverage for wrong-device handles. Mixed-mode and alignment/range rejection remain covered by existing Vulkan indirect/count-buffer validation paths.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_vulkan_compute_generated_indirect_consumption_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_vulkan_compute_generated_indirect_consumption_tests|MK_mavg_vulkan_gpu_culling_dispatch_tests|MK_backend_scaffold_tests"
```

Expected after implementation: all selected tests pass, with SPIR-V dependent cases skipped/fail-closed only when the documented environment variables or Vulkan toolchain evidence are absent.

## Task 5 - Docs, Manifest, Static Guards, And Publication

**Files:**

- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-06-10-mavg-vulkan-gpu-culling-dispatch-v1.md`
- Modify: `docs/superpowers/plans/2026-06-10-mavg-d3d12-compute-generated-indirect-consumption-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-114-mavg-vulkan-compute-generated-indirect-consumption.ps1`

- [x] Record the exact ready claim: Vulkan `draw_indexed_indirect` can consume MAVG compute-generated argument/count buffers from the completed Vulkan dispatch path with backend-private synchronization2 barriers.
- [x] Keep non-claims explicit: mesh shaders, Metal, ray tracing, deformation, benchmark superiority, Nanite compatibility/equivalence/superiority, GPU memory pressure, background streaming, and broad optimization.
- [x] Return `currentActivePlan` to the production-completion master plan and `recommendedNextPlan.id` to `next-production-gap-selection` in the closeout commit.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
git diff --check
```

Expected at closeout: docs/static checks pass, focused CTest passes, full validation passes or records only host-gated diagnostics already accepted by the repository, and publication preflight is `ok`.

## Worktree And Dependency Evidence

This plan was drafted in `G:\workspace\development\GameEngine\.worktrees\mavg-vulkan-compute-indirect-consumption-v1` on branch `codex/mavg-vulkan-compute-indirect-consumption-v1`. Implementation moved to `G:\workspace\development\GameEngine\.worktrees\mavg-vulkan-compute-generated-indirect-consumption-impl-v1` on branch `codex/mavg-vulkan-compute-generated-indirect-consumption-impl-v1`. The initial C++ configure blocker was missing `external/vcpkg/scripts/buildsystems/vcpkg.cmake`; it was resolved by cloning official Microsoft vcpkg into the shared worktree dependency root, running `tools/prepare-worktree.ps1`, then passing `tools/check-toolchain.ps1` and `tools/cmake.ps1 --preset dev`.

## Non-Claims

This plan does not claim Vulkan mesh shaders, Vulkan meshlet rendering, Metal readiness, D3D12 changes, public native handles, background streaming, partial `.mavgpayload` byte-range loading, GPU memory pressure integration, deformation, ray tracing, benchmark results, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
