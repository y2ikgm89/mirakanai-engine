# MAVG Vulkan Mesh Shader Indirect Dispatch v1 Implementation Plan (2026-06-23)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the selected Vulkan MAVG mesh shader LOD evidence from direct `vkCmdDrawMeshTasksEXT` execution to source-backed `vkCmdDrawMeshTasksIndirectEXT`, optional `vkCmdDrawMeshTasksIndirectCountEXT`, and synchronization2 task/mesh consumption evidence without promoting package-visible broad MAVG readiness.

**Architecture:** Keep the work inside `MK_rhi_vulkan` backend-private evidence APIs. `execute_vulkan_mavg_mesh_shader_lod` remains the public test entrypoint for this backend-private slice, direct dispatch remains unchanged, indirect dispatch gets explicit argument/count buffer creation and command recording, and count-buffer readiness stays host-gated unless the device enables `drawIndirectCount` and resolves the count command. New result fields and validators publish narrow Vulkan indirect evidence only.

**Tech Stack:** C++23, Vulkan 1.3, `VK_EXT_mesh_shader`, `VkPhysicalDeviceMeshShaderFeaturesEXT`, `VkPhysicalDeviceMeshShaderPropertiesEXT`, `vkCmdDrawMeshTasksIndirectEXT`, `vkCmdDrawMeshTasksIndirectCountEXT`, `VkDrawMeshTasksIndirectCommandEXT`, synchronization2, `MK_rhi_vulkan`, HLSL-to-SPIR-V test shader artifacts, CMake/CTest, PowerShell validation, Context7 `/khronosgroup/vulkan-docs`, and current Khronos Vulkan reference pages.

---

**Plan ID:** `mavg-vulkan-mesh-shader-indirect-dispatch-v1`

**Status:** Implemented locally on `codex/mavg-vulkan-mesh-indirect-impl`; publication and hosted PR evidence pending. The slice proves selected Vulkan task+mesh direct, indirect, and indirect-count dispatch evidence on a ready host while keeping package-visible broad `mavg_mesh_shader_lod_ready` and all Nanite/Metal/broad backend claims false.

**Date:** 2026-06-23

## Context

`MAVG Advanced Backend Evidence Closeout v1` Task 4 is complete for selected Vulkan direct-dispatch evidence: feature/property probing, logical-device mesh/task feature enablement, backend-private mesh graphics pipeline creation, direct `vkCmdDrawMeshTasksEXT`, dynamic rendering, no vertex input layout, no index buffer, texture readback, and nonzero readback hash evidence. Its remaining unchecked items are not completed subclaims:

- `vkCmdDrawMeshTasksIndirectEXT` execution.
- `vkCmdDrawMeshTasksIndirectCountEXT` execution or precise host-gated evidence.
- synchronization2 barriers for compute/page upload to task/mesh shader consumption.

This plan owns only those three subclaims. It does not reopen the completed direct-dispatch slice, and it must not claim Nanite compatibility/equivalence/superiority, Metal readiness, deformation integration, ray tracing integration, broad CPU/GPU/memory optimization, or broad package-visible MAVG backend readiness.

## Official Source Gate

- Context7 resolved Vulkan to the official Khronos source `/khronosgroup/vulkan-docs` on 2026-06-23. The query for `VK_EXT_mesh_shader`, indirect mesh task draws, indirect-count draws, `drawIndirectCount`, and synchronization2 task/mesh stage bits confirmed the source surface used by this plan.
- Khronos `vkCmdDrawMeshTasksIndirectEXT` reference requires the indirect argument buffer to be created with `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`, `offset` to be 4-byte aligned, `drawCount <= VkPhysicalDeviceLimits::maxDrawIndirectCount`, `stride` alignment and size constraints for multi-draw, range bounds over `offset + stride * (drawCount - 1) + sizeof(VkDrawMeshTasksIndirectCommandEXT)`, and a current graphics pipeline containing a `MeshEXT` execution model.
  Source: https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdDrawMeshTasksIndirectEXT.html
- Khronos `vkCmdDrawMeshTasksIndirectCountEXT` reference additionally requires both `buffer` and `countBuffer` to use `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`, `countBufferOffset` to be 4-byte aligned, the count stored in `countBuffer` to be `<= VkPhysicalDeviceLimits::maxDrawIndirectCount`, `drawIndirectCount` to be enabled, `stride >= sizeof(VkDrawMeshTasksIndirectCommandEXT)`, range bounds over `maxDrawCount`, and a current graphics pipeline containing a `MeshEXT` execution model.
  Source: https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdDrawMeshTasksIndirectCountEXT.html
- Khronos `VkDrawMeshTasksIndirectCommandEXT` defines exactly three `uint32_t` fields: `groupCountX`, `groupCountY`, and `groupCountZ`. Those group counts must satisfy task workgroup limits when a task shader is in the pipeline, or mesh workgroup limits when no task shader is in the pipeline.
  Source: https://docs.vulkan.org/refpages/latest/refpages/source/VkDrawMeshTasksIndirectCommandEXT.html
- Khronos `VkPhysicalDeviceMeshShaderFeaturesEXT` requires enabled `taskShader` before task shader stage/pipeline-stage enum use and enabled `meshShader` before mesh shader stage/pipeline-stage enum use.
  Source: https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceMeshShaderFeaturesEXT.html
- Khronos `VkPhysicalDeviceVulkan12Features` contains `drawIndirectCount`; when it is not enabled, count-indirect draw functions must not be used.
  Source: https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceVulkan12Features.html
- Khronos `VkPipelineStageFlagBits2` defines `VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT`, `VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT`, and `VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT`; the task/mesh bits are the required synchronization2 stage names for task/mesh shader consumption evidence.
  Source: https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineStageFlagBits2.html

## Non-Overlap Contract

This plan must not duplicate or broaden these completed plans:

- `MAVG Mesh Shader Capability Gate v1`: already proves value-level D3D12/Vulkan mesh shader capability rows and explicitly leaves execution unclaimed.
- `MAVG Advanced Backend Evidence Closeout v1` Task 4: already proves Vulkan direct `vkCmdDrawMeshTasksEXT` readback.
- `MAVG Vulkan Count-Buffer Indirect Execution v1`: already proves indexed indirect count-buffer execution with `vkCmdDrawIndexedIndirectCount`; this plan may reuse count decode/effective count patterns, but it owns mesh task indirect only.
- `MAVG Vulkan GPU Culling Dispatch v1`: already proves compute-generated indexed-indirect argument/count buffers; this plan may reuse synchronization2 buffer-barrier patterns, but it owns task/mesh shader consumption evidence.

This plan must keep these non-claims explicit in code, tests, validators, docs, and manifest fragments:

- `mavg_mesh_shader_lod_ready=0` unless a later package-visible aggregate plan intentionally promotes it.
- `mavg_mesh_shader_lod_metal_ready=0`, `mavg_nanite_compatible=0`, `mavg_nanite_equivalent=0`, and `mavg_nanite_superior=0`.
- No public native Vulkan handle exposure.
- No Metal, D3D12, deformation, ray tracing, async-overlap performance, broad optimization, or broad backend readiness promotion.

## Implementation Evidence

- `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_mesh_shader_lod.hpp` now has `VulkanMavgMeshShaderLodIndirectCommand`, direct/indirect/indirect-count mode fields, result fields for indirect/count readiness, draw-indirect/task/mesh barrier counters, payload consumption, count-buffer value, executed draw count, and `mavg_mesh_shader_lod_ready_promoted=false`.
- `engine/rhi/vulkan/src/vulkan_mavg_mesh_shader_lod.cpp` validates indirect/count buffer usage, 4-byte offset alignment, stride alignment and size, range bounds, `maxDrawIndirectCount`, workgroup limits, and count support before recording. It creates real indirect/count upload buffers, records synchronization2 draw-indirect barriers, records `vkCmdDrawMeshTasksIndirectEXT` or `vkCmdDrawMeshTasksIndirectCountEXT` only for the selected execution mode, and requires deterministic readback evidence before readiness.
- `engine/rhi/vulkan/src/vulkan_backend.cpp` now queries/enables `VkPhysicalDeviceVulkan12Features::drawIndirectCount`, resolves `vkCmdDrawMeshTasksIndirectCountEXT` through `vkGetDeviceProcAddr` only when enabled, and keeps command resolution tied to mesh/task feature enablement.
- `tests/shaders/vulkan_mavg_mesh_shader_lod.task.hlsl` and `.mesh.hlsl` now read a descriptor-backed payload buffer so the test can prove task/mesh shader consumption. The local host compiled all three shader artifacts with DXC `-spirv -fspv-target-env=vulkan1.3 -fspv-extension=SPV_EXT_mesh_shader`, and `spirv-val --target-env vulkan1.3` passed for task, mesh, and fragment SPIR-V.
- `tests/unit/vulkan_mavg_mesh_shader_lod_tests.cpp` now covers direct non-promotion, invalid indirect usage/alignment/stride/range, invalid count usage/offset/range/count support, indirect execution readiness, indirect-count execution readiness, payload consumption, and validator counters. Mesh-only execution is intentionally outside this slice because the selected first-party evidence path uses a task+mesh pipeline; no mesh-only readiness claim is emitted.
- `tools/validate-mavg-vulkan-mesh-shader-indirect-dispatch.ps1` now configures/builds/runs `MK_mavg_vulkan_mesh_shader_lod_tests`, prints the validation counters below, supports `-RequireReady`, and fails if broad `mavg_mesh_shader_lod_ready`, Nanite, or native-handle counters are promoted.

Focused validation evidence on 2026-06-23:

```text
validation_recipe=mavg-vulkan-mesh-shader-indirect-dispatch
mavg_vulkan_mesh_shader_spirv_artifacts_configured=1
mavg_vulkan_mesh_shader_indirect_dispatch_status=ready
mavg_vulkan_mesh_shader_indirect_dispatch_ready=1
mavg_vulkan_mesh_shader_indirect_count_status=ready
mavg_vulkan_mesh_shader_indirect_count_ready=1
mavg_vulkan_mesh_shader_indirect_count_host_gated=0
mavg_vulkan_mesh_shader_indirect_argument_buffer_usage_ready=1
mavg_vulkan_mesh_shader_indirect_count_buffer_usage_ready=1
mavg_vulkan_mesh_shader_indirect_stride_ready=1
mavg_vulkan_mesh_shader_indirect_range_ready=1
mavg_vulkan_mesh_shader_draw_indirect_stage_barriers=2
mavg_vulkan_mesh_shader_task_stage_barriers=1
mavg_vulkan_mesh_shader_mesh_stage_barriers=1
mavg_vulkan_mesh_shader_payload_consumption_ready=1
mavg_vulkan_mesh_shader_indirect_count_value=1
mavg_vulkan_mesh_shader_indirect_executed_draw_count=1
mavg_mesh_shader_lod_vulkan_ready=1
mavg_mesh_shader_lod_ready=0
mavg_nanite_compatible=0
mavg_nanite_equivalent=0
mavg_nanite_superior=0
mavg_vulkan_native_handles_exposed=0
```

## Implementation Decisions

1. Use real `VulkanRuntimeBuffer` instances for indirect argument and count buffers. Value-only fake buffers are not sufficient for readiness.
2. Argument and count buffers must be created with `BufferUsage::indirect`; compute-generated variants additionally require `BufferUsage::storage`, and readback evidence may add `BufferUsage::copy_source`.
3. Use a backend-private `VulkanMavgMeshShaderLodIndirectCommand` value that exactly mirrors `VkDrawMeshTasksIndirectCommandEXT`: three `std::uint32_t` fields in X/Y/Z order and a size check equal to `vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes`.
4. Record direct, indirect, and indirect-count as distinct execution modes. A single call to `execute_vulkan_mavg_mesh_shader_lod` must not record all modes by default; it records one selected mode from the descriptor.
5. Count-indirect readiness requires `drawIndirectCount` support and enablement plus resolved `vkCmdDrawMeshTasksIndirectCountEXT`. If any of those are unavailable, return `host_gated=true`, `mavg_mesh_shader_lod_vulkan_indirect_count_host_gated=true`, and keep `draw_mesh_tasks_indirect_count_calls=0`.
6. For indirect-count evidence, read or decode the count-buffer value and publish `last_indirect_count_buffer_value` and `last_indirect_executed_draw_count`, using `min(count_buffer_value, max_indirect_count_draws)` semantics in the result evidence.
7. Use synchronization2 buffer barriers with official stage/access semantics:
   - upload or compute writes to indirect argument/count buffers -> `VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT` with indirect-command-read access before mesh task indirect recording.
   - upload or compute writes to a selected task/mesh payload buffer -> `VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT` or `VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT` with shader-read access before shader consumption.
   - render target -> transfer readback remains covered by existing texture barriers.
8. Add a minimal shader payload read for task/mesh consumption evidence if the current task/mesh shader variants do not consume a descriptor-backed payload buffer. The descriptor-backed variant must remain under `tests/shaders/` and be validated through the same DXC SPIR-V lane as the existing direct-dispatch shaders.

## Files

- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_mesh_shader_lod.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_mavg_mesh_shader_lod.cpp`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/vulkan_mavg_mesh_shader_lod_tests.cpp`
- Modify or add shader variants under `tests/shaders/` only if descriptor-backed task/mesh payload consumption evidence is needed.
- Create: `tools/validate-mavg-vulkan-mesh-shader-indirect-dispatch.ps1`
- Create or modify: `tools/check-ai-integration-13x-mavg-vulkan-mesh-shader-indirect-dispatch.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Regenerate: `engine/agent/manifest.json`

## Result Contract

Add or verify these result fields in `VulkanMavgMeshShaderLodDispatchResult`:

- `draw_mesh_tasks_direct_calls`
- `draw_mesh_tasks_indirect_calls`
- `draw_mesh_tasks_indirect_count_calls`
- `draw_mesh_tasks_indirect_eligible`
- `draw_mesh_tasks_indirect_count_eligible`
- `mavg_mesh_shader_lod_vulkan_indirect_ready`
- `mavg_mesh_shader_lod_vulkan_indirect_count_ready`
- `mavg_mesh_shader_lod_vulkan_indirect_count_host_gated`
- `indirect_argument_buffer_usage_ready`
- `indirect_count_buffer_usage_ready`
- `indirect_argument_offset_aligned`
- `indirect_count_offset_aligned`
- `indirect_stride_valid`
- `indirect_argument_range_valid`
- `indirect_count_range_valid`
- `last_indirect_count_buffer_value`
- `last_indirect_executed_draw_count`
- `draw_indirect_stage_barriers_recorded`
- `task_shader_stage_barriers_recorded`
- `mesh_shader_stage_barriers_recorded`
- `shader_payload_consumed_by_task_or_mesh`
- `mavg_mesh_shader_lod_ready_promoted`

`mavg_mesh_shader_lod_ready_promoted` must remain false in this plan.

## Validation Surface

The focused validator must emit these exact counters:

```text
mavg_vulkan_mesh_shader_indirect_dispatch_status=ready|host_gated|blocked
mavg_vulkan_mesh_shader_indirect_dispatch_ready=0|1
mavg_vulkan_mesh_shader_indirect_count_status=ready|host_gated|blocked
mavg_vulkan_mesh_shader_indirect_count_ready=0|1
mavg_vulkan_mesh_shader_indirect_count_host_gated=0|1
mavg_vulkan_mesh_shader_indirect_argument_buffer_usage_ready=0|1
mavg_vulkan_mesh_shader_indirect_count_buffer_usage_ready=0|1
mavg_vulkan_mesh_shader_indirect_stride_ready=0|1
mavg_vulkan_mesh_shader_indirect_range_ready=0|1
mavg_vulkan_mesh_shader_draw_indirect_stage_barriers=0|N
mavg_vulkan_mesh_shader_task_stage_barriers=0|N
mavg_vulkan_mesh_shader_mesh_stage_barriers=0|N
mavg_vulkan_mesh_shader_payload_consumption_ready=0|1
mavg_mesh_shader_lod_vulkan_ready=0|1
mavg_mesh_shader_lod_ready=0
mavg_nanite_compatible=0
mavg_nanite_equivalent=0
mavg_nanite_superior=0
mavg_vulkan_native_handles_exposed=0
```

## Done When

- Direct dispatch behavior and tests remain green without changing the existing readiness meaning of `mavg_mesh_shader_lod_vulkan_ready`.
- `vkCmdDrawMeshTasksIndirectEXT` is recorded only with a real indirect argument buffer, valid 4-byte aligned offset, valid stride, range-safe command records, valid workgroup limits, a mesh graphics pipeline, and readback evidence.
- `vkCmdDrawMeshTasksIndirectCountEXT` is recorded only when `drawIndirectCount` is supported and enabled, the command resolves through `vkGetDeviceProcAddr`, the argument and count buffers have indirect usage, offsets are 4-byte aligned, count/range limits are valid, and actual count evidence is captured.
- Host-gated systems report explicit diagnostics rather than silently passing readiness.
- synchronization2 barriers include draw-indirect consumption and task/mesh shader consumption evidence when those resources are consumed.
- Focused tests, focused validator, static checks, agent-surface drift checks, and full `tools/validate.ps1` pass before publication.
- Docs and manifest fragments state this is selected Vulkan mesh shader indirect evidence only and preserve all broad non-claims.

## Tasks

### Task 1: RED Tests For Source-Backed Validation

- [x] Add tests that reject an indirect argument buffer descriptor without `BufferUsage::indirect`.
- [x] Add tests that reject non-4-byte `indirect_buffer_offset_bytes` and non-4-byte `indirect_stride_bytes`.
- [x] Add tests that reject `indirect_stride_bytes < sizeof(VkDrawMeshTasksIndirectCommandEXT)`.
- [x] Add tests that reject single-draw and multi-draw range overflow using the exact Khronos formulas.
- [x] Add tests that reject group counts exceeding task workgroup per-axis and total limits when task shader execution is selected.
- [x] Keep mesh-only execution outside this slice; selected readiness uses a task+mesh pipeline and emits no mesh-only readiness claim.
- [x] Add tests that reject count-buffer execution when `drawIndirectCount` is unsupported or not enabled.
- [x] Add tests that reject count buffers without `BufferUsage::indirect`, unaligned `count_buffer_offset_bytes`, invalid `max_indirect_count_draws`, and count-buffer range overflow.
- [x] Add tests that prove direct dispatch still records one direct call and zero indirect/count calls.

Expected focused command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_vulkan_mesh_shader_lod_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_mavg_vulkan_mesh_shader_lod_tests --output-on-failure
```

Expected RED result before implementation: at least one new test fails for missing validation, missing indirect execution, missing count execution, or missing synchronization evidence.

### Task 2: Indirect Descriptor And Validation Model

- [x] Add a backend-private indirect command value mirroring `VkDrawMeshTasksIndirectCommandEXT` with `group_count_x`, `group_count_y`, and `group_count_z`.
- [x] Add descriptor fields that distinguish direct, indirect, and indirect-count execution mode without changing default direct-dispatch behavior.
- [x] Add validation helpers for offset alignment, stride alignment, command size, single-draw bounds, multi-draw bounds, count-buffer bounds, workgroup limits, and `maxDrawIndirectCount` limits.
- [x] Keep validation side-effect-free until all preconditions pass.
- [x] Add result diagnostics for each validation failure with stable diagnostic strings.

Expected GREEN result: validation-only unit tests pass, host-supported execution tests still behave as direct-dispatch only.

### Task 3: Backend Command Resolution And Feature Enablement

- [x] Extend Vulkan logical-device feature query/create planning so `drawIndirectCount` support can be queried and enabled when `request_indirect_count_draw` is selected.
- [x] Resolve `vkCmdDrawMeshTasksIndirectCountEXT` through `vkGetDeviceProcAddr` only when count-indirect support is available for the selected device.
- [x] Add `draw_mesh_tasks_indirect_count_command_available` population in `probe_vulkan_mavg_mesh_shader_lod_capability`.
- [x] Host-gate count-indirect execution when the feature, extension/core support, or command resolution is missing.
- [x] Keep `vkCmdDrawMeshTasksIndirectEXT` command resolution tied to mesh shader enablement.

Expected focused command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R "MK_backend_scaffold_tests|MK_mavg_vulkan_mesh_shader_lod_tests" --output-on-failure
```

Expected result: direct-dispatch tests pass, count-indirect unsupported hosts report host-gated diagnostics.

### Task 4: Indirect Argument Buffer Execution

- [x] Create a `VulkanRuntimeBuffer` for `VkDrawMeshTasksIndirectCommandEXT` records with `BufferUsage::indirect | BufferUsage::copy_source`; compute-generated storage mode remains outside this slice.
- [x] Write CPU-generated indirect command records for the selected first-party task rows.
- [x] Record synchronization from upload writes to `VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT` before command consumption.
- [x] Extend `record_runtime_texture_rendering_mesh_tasks_draw` to record `vkCmdDrawMeshTasksIndirectEXT` without exposing native handles.
- [x] Require deterministic readback hash evidence for indirect dispatch readiness.
- [x] Set `draw_mesh_tasks_indirect_calls=1`, `draw_mesh_tasks_direct_calls=0`, and `mavg_mesh_shader_lod_vulkan_indirect_ready=true` only after execution and readback succeed.

Expected result: indirect mode can be ready on a host that supports the same mesh/task shader direct-dispatch evidence.

### Task 5: Indirect-Count Execution And Actual Count Evidence

- [x] Create a count buffer with `BufferUsage::indirect | BufferUsage::copy_source`; compute-generated count mode remains outside this slice.
- [x] Write or generate a 32-bit count value and clamp executed draws with `min(count_buffer_value, max_indirect_count_draws)` in result evidence.
- [x] Record synchronization from upload writes to `VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT` before count and argument buffer consumption.
- [x] Record `vkCmdDrawMeshTasksIndirectCountEXT` only after feature, command, usage, offset, count, and range gates pass.
- [x] Read back or otherwise decode the actual count value into `last_indirect_count_buffer_value`.
- [x] Publish `last_indirect_executed_draw_count` and set `draw_mesh_tasks_indirect_count_calls=1` only when the command is recorded.

Expected result: supported hosts can produce count-indirect readiness; unsupported hosts produce explicit `host_gated` evidence without readiness.

### Task 6: Task/Mesh Shader Payload Synchronization Evidence

- [x] Add a minimal descriptor-backed task/mesh payload read if existing shaders do not consume an external buffer.
- [x] Update shader sources under `tests/shaders/` and validate them with DXC SPIR-V CodeGen using `-spirv -fspv-target-env=vulkan1.3 -fspv-extension=SPV_EXT_mesh_shader`.
- [x] Record upload-write barriers to `VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT` when task shader consumes payload data.
- [x] Record upload-write barriers to `VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT` when mesh shader consumes payload data.
- [x] Do not use task stage bits unless `taskShader` is enabled; do not use mesh stage bits unless `meshShader` is enabled.
- [x] Add tests and static needles for `task_shader_stage_barriers_recorded`, `mesh_shader_stage_barriers_recorded`, and `shader_payload_consumed_by_task_or_mesh`.

Expected result: synchronization evidence distinguishes indirect-command consumption from task/mesh shader data consumption.

### Task 7: Focused Validator And Static Guards

- [x] Add `tools/validate-mavg-vulkan-mesh-shader-indirect-dispatch.ps1`.
- [x] The validator must run `MK_mavg_vulkan_mesh_shader_lod_tests`, parse/status-print the counters in the Validation Surface section, and support `-RequireReady` only for a host with required SPIR-V artifacts and Vulkan feature support.
- [x] Extend chapter 116 rather than creating a duplicate chapter, keeping the static-ledger owner unchanged.
- [x] Static needles must include official API names, feature names, buffer-usage rules, range formulas, sync2 stage names, counter names, and non-claim rows.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.

Expected result: static checks fail if future edits remove official constraints or accidentally promote broad readiness.

### Task 8: Docs, Manifest, And Plan Registry

- [x] Update `docs/current-capabilities.md` and `docs/roadmap.md` with the narrow indirect evidence claim and non-claims.
- [x] Update `docs/superpowers/plans/README.md` with this implementation slice after the direct-dispatch Task 4 row.
- [x] Update `docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md` so the three unchecked Task 4 bullets point to this plan rather than looking like ambiguous abandoned work.
- [x] Update `engine/agent/manifest.fragments/004-modules.json` with the new backend-private evidence summary.
- [x] Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` only after implementation evidence exists; do not set this plan active merely because the plan file exists.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

Expected result: manifest and docs describe selected Vulkan indirect evidence only, while `currentActivePlan` remains honest for the selected production-completion state.

### Task 9: Slice Validation And Publication

- [x] Run focused build and CTest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_vulkan_mesh_shader_lod_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_mavg_vulkan_mesh_shader_lod_tests --output-on-failure
```

- [x] Run the focused validator without `-RequireReady` on unsupported hosts and with `-RequireReady` only on a host that has mesh/task shader, count-indirect, and SPIR-V artifact evidence:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-vulkan-mesh-shader-indirect-dispatch.ps1
```

- [x] Run agent-surface and static checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

- [x] Run full validation after code, tests, docs, manifest, and static guards settle:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Run publication preflight, commit the validated slice, push the task branch, open a draft PR, wait for hosted checks, mark ready through `tools/ready-task-pr.ps1`, and register auto-merge only after the required PR gate succeeds.

Expected final state: a validated PR that proves selected Vulkan mesh shader indirect dispatch evidence, count-indirect readiness or precise host-gated evidence, task/mesh synchronization evidence, and no broad MAVG readiness promotion.
