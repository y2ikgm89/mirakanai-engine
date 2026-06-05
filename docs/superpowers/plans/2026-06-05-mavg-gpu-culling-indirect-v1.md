# 2026-06-05 MAVG GPU Culling Indirect v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-gpu-culling-indirect-v1`

**Status:** Active.

**Execution State:** Stacked on `mavg-runtime-lod-milestone-v1` after the conventional static MAVG LoD path. This child is the first GPU-driven step and must stay reviewable without claiming backend execution.

**Goal:** Add a clean-break backend-neutral MAVG GPU culling/indirect command planning contract that converts selected MAVG LoD clusters plus reviewed culling bounds into packed indexed indirect command rows and D3D12/Vulkan synchronization requirement rows, without executing GPU culling, D3D12 `ExecuteIndirect`, Vulkan indirect draws, mesh shaders, Metal paths, or native handle interop.

**Context:** The parent milestone already provides graph hierarchy/error/fallback/draw-range rows, static payload cook rows, CPU reference selection, runtime residency/page-streaming review evidence, range-aware conventional indexed draws, scene submission planning, and conventional runtime RHI upload evidence. This child starts from those value rows and prepares the indirect command buffer contract needed by later backend candidates.

**Constraints:**

- Keep `MK_renderer` backend-neutral and native-handle-free.
- Keep D3D12/Vulkan/Metal execution out of this child.
- Use official-source constraints for indirect command layout, argument/count buffer alignment, and compute-write to indirect-read synchronization.
- Preserve clean-room policy and no Nanite compatibility/equivalence/superiority claim.

**Done when:**

- `engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp` exposes value-only selected-cluster rows through `MavgGpuCullingClusterBoundsRow`, `MavgGpuCullingIndirectCommand`, `MavgGpuCullingIndirectCommandLayout`, `MavgGpuCullingSyncRequirement`, `MavgGpuCullingIndirectDesc`, `MavgGpuCullingIndirectPlan`, diagnostics, and planner rows.
- `plan_mavg_gpu_culling_indirect_commands` emits deterministic packed indexed command rows from successful `MavgLodSelectionResult` rows and reviewed bounds.
- Invalid selection, missing/duplicate/invalid bounds, invalid instance count, invalid draw ranges, zero max command count, command budget overflow, and buffer-size overflow fail closed before command publication.
- Compute-produced command buffers return D3D12 and Vulkan sync requirement rows with 4-byte argument/count buffer alignment.
- Tests, docs, manifest fragments, composed manifest, and static checks describe exactly this scope.
- Focused validation passes, then full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Official Source Audit

Checked on 2026-06-05:

- Context7 `/kitware/cmake`: target-based `add_executable`, `target_link_libraries`, `target_sources` / explicit sources, and `add_test` are the expected CMake/CTest pattern for this repository's focused unit target.
- Context7 `/khronosgroup/vulkan-docs`: compute shader writes to an indirect command buffer require a memory barrier before draw indirect consumption; draw indirect count buffers need indirect-buffer usage, 4-byte count offsets, and feature-gated draw-indirect-count support when count-buffer variants are used.
- Khronos Vulkan `VkDrawIndexedIndirectCommand` docs define the five-field indexed indirect layout: `indexCount`, `instanceCount`, `firstIndex`, `vertexOffset`, and `firstInstance`.
- Microsoft Learn Direct3D 12 Indirect Drawing documents GPU/CPU generated indirect argument buffers, command signatures, tightly packed command arguments, and `ExecuteIndirect` interpretation.
- Microsoft Learn `ID3D12GraphicsCommandList::ExecuteIndirect` documents `MaxCommandCount`, optional count buffer semantics, 4-byte argument/count offsets, buffer-resource requirements, and `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT` validation.

## Scope

In scope:

- `MK_renderer` value-only GPU culling/indirect planning rows.
- Packed indexed indirect argument layout compatible with the current conventional `draw_indexed` row order.
- CPU-reference visibility filtering over reviewed culling bounds as deterministic test evidence.
- Compute-produced D3D12/Vulkan synchronization requirement rows for later backend work.
- Fail-closed diagnostics and explicit non-execution flags.

Out of scope:

- Actual GPU culling dispatch.
- D3D12 command signature creation or `ExecuteIndirect`.
- Vulkan `vkCmdDrawIndexedIndirect` / `vkCmdDrawIndexedIndirectCount`.
- RHI buffer allocation/state transitions.
- Mesh/task/amplification shaders.
- Metal indirect command buffers.
- Performance, async overlap, or benchmark superiority.

## Files

- Create: `engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp`
- Create: `engine/renderer/src/mavg_gpu_culling.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Create: `tests/unit/mavg_gpu_culling_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-105-mavg-gpu-culling-indirect.ps1`

## Tasks

### Task 1: Add Failing GPU Culling Indirect Tests

- [x] Add `MK_mavg_gpu_culling_tests`.
- [x] Prove RED on missing `mirakana/renderer/mavg_gpu_culling.hpp`.
- [x] Cover packed indexed command rows, invisible-cluster filtering, invalid bounds fail-closed behavior, max command count fail-closed behavior, and D3D12/Vulkan sync requirement rows.

### Task 2: Implement Backend-Neutral Planner

- [x] Add `mavg_gpu_culling.hpp` public rows.
- [x] Add deterministic planner implementation.
- [x] Keep execution flags false for GPU culling, indirect draw, mesh shader, and native handles.
- [x] Keep sync rows as requirements only; do not call backend APIs.

### Task 3: Sync Docs, Manifest, And Static Contracts

- [x] Update current capabilities, roadmap, architecture spec, plan registry, manifest fragments, composed manifest, and static guard.
- [x] Keep non-claims explicit for actual GPU dispatch, indirect draw execution, mesh shaders, Metal readiness, Nanite equivalence/superiority, and broad optimization.

### Task 4: Validate And Publish

- [x] Run focused build and CTest for `MK_mavg_gpu_culling_tests`.
- [x] Run adjacent renderer/RHI focused tests as needed.
- [x] Run `tools/check-public-api-boundaries.ps1`, targeted tidy, docs/manifest/static checks, formatting, `git diff --check`, and full `tools/validate.ps1`.
- [x] Commit the candidate and open draft PR #445 stacked on `codex/mavg-page-streaming-eviction-review-v1`.

## Validation Commands

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_culling_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "mavg_gpu_culling"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/renderer/src/mavg_gpu_culling.cpp,tests/unit/mavg_gpu_culling_tests.cpp -ReuseExistingFileApiReply
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

## Validation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_gpu_culling_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R mavg_gpu_culling` | Passed: 1/1 test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_lod_selection_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_gpu_culling_tests\|MK_mavg_lod_selection_tests\|MK_renderer_tests\|MK_rhi_tests"` | Passed: 4/4 tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/renderer/src/mavg_gpu_culling.cpp,tests/unit/mavg_gpu_culling_tests.cpp -ReuseExistingFileApiReply` | Passed: 2 files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. |
| `git diff --check` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed: 19 static checks, full build, tidy smoke, and 107/107 CTest tests. |

## Non-Claims

- No executed GPU culling dispatch.
- No D3D12 `ExecuteIndirect` execution.
- No Vulkan indirect draw execution.
- No mesh/task/amplification shader readiness.
- No RHI buffer allocation, native state transition, command signature, descriptor, or native handle exposure.
- No Metal readiness from Windows validation.
- No autonomous/background streaming, automatic eviction policy, partial `.mavgpayload` byte-range loader/schema, GPU memory pressure integration, deformation, ray tracing, benchmark result, Nanite compatibility, Nanite equivalence, or Nanite superiority.
