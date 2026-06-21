# MAVG Mesh Shader Capability Gate v1 Implementation Plan (2026-06-21)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a fail-closed MAVG mesh shader capability gate that records selected D3D12 and Vulkan mesh shader feature-query rows while keeping compute/indirect fallback mandatory and mesh shader execution unclaimed.

**Architecture:** Keep v1 value-only in `MK_runtime_rhi`: callers provide D3D12/Vulkan capability rows derived from official backend feature queries, and the gate validates query kind, supported feature bits, selected output/payload/shared-memory limits, fallback readiness, and explicit non-claims. This slice does not add mesh/task/amplification shader stages to `MK_rhi`, does not create mesh shader pipelines, does not call D3D12 mesh dispatch or `vkCmdDrawMeshTasks*EXT`, and does not claim broad MAVG backend readiness.

**Tech Stack:** C++23, `MK_runtime_rhi`, existing `MK_rhi` backend ids, CMake/CTest, PowerShell validation, `sample_desktop_runtime_game` package smoke counters, Context7 `/khronosgroup/vulkan-docs`, Microsoft DirectX-Specs, Microsoft Learn D3D12 headers/API reference, and Khronos Vulkan documentation.

---

**Plan ID:** `mavg-mesh-shader-capability-gate-v1`

**Status:** Implemented; PR publication in progress.

**Date:** 2026-06-21

## Context

The production-completion selection gate is active with `unsupportedProductionGaps = []`. The MAVG master roadmap lists Phase 6 as Mesh Shader Backend v1, but current completed evidence stops before mesh shader execution. The latest retained MAVG proof closes the selected async-overlap performance proof and still leaves GPU DirectStorage destinations, GDeflate/GPU decompression, autonomous streaming-service performance beyond selected samples, mesh shader execution, Metal MAVG readiness, Nanite equivalence/superiority, broad MAVG backend readiness, and broad optimization unclaimed.

This plan selects the first Phase 6 prerequisite: a capability gate and package-visible counter surface. It makes later backend execution safer by requiring D3D12 and Vulkan capability evidence before any future mesh shader LOD implementation can promote readiness.

## Official Source Gate

- Source status on 2026-06-21: Context7 resolved the official Vulkan documentation as `/khronosgroup/vulkan-docs`; D3D12 evidence is from Microsoft DirectX-Specs and Microsoft Learn. Apple/Metal is intentionally not queried for readiness because this plan must not promote Metal MAVG readiness.
- Context7 `/khronosgroup/vulkan-docs` query for `VK_EXT_mesh_shader` confirms `VkPhysicalDeviceMeshShaderFeaturesEXT`, `VkPhysicalDeviceMeshShaderPropertiesEXT`, `taskShader`, `meshShader`, `meshShaderQueries`, mesh/task workgroup/output/payload limits, `vkCmdDrawMeshTasksEXT`, `vkCmdDrawMeshTasksIndirectEXT`, `vkCmdDrawMeshTasksIndirectCountEXT`, `VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT`, and `VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT`.
- Microsoft DirectX-Specs Mesh Shader page states D3D12 support is detected with `ID3D12Device::CheckFeatureSupport`, `D3D12_FEATURE_D3D12_OPTIONS7`, `D3D12_FEATURE_DATA_D3D12_OPTIONS7`, `MeshShaderTier`, and `D3D12_MESH_SHADER_TIER`, and that `DispatchMesh` / mesh pipeline state creation are separate execution steps.
- Microsoft Learn `D3D12_FEATURE_DATA_D3D12_OPTIONS7` API reference states the struct carries `D3D12_MESH_SHADER_TIER MeshShaderTier` and requires header `d3d12.h`.
- Khronos Vulkan `VK_EXT_mesh_shader` reference states the extension adds `VkPhysicalDeviceMeshShaderFeaturesEXT`, `VkPhysicalDeviceMeshShaderPropertiesEXT`, `vkCmdDrawMeshTasksEXT`, indirect mesh task commands, mesh/task shader stage bits, and mesh/task pipeline statistic query bits.
- This plan uses those official names only as capability-row evidence. It does not execute mesh shaders or expose native Direct3D/Vulkan handles.

## Source-Backed Acceptance Matrix

| Topic | Official fact used by this plan | Implementation decision |
| --- | --- | --- |
| D3D12 capability detection | `D3D12_FEATURE_DATA_D3D12_OPTIONS7::MeshShaderTier` reports mesh/amplification shader support after `ID3D12Device::CheckFeatureSupport`. | A D3D12 row is ready only when it represents the selected `D3D12_FEATURE_D3D12_OPTIONS7` / `MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED` query evidence. |
| D3D12 execution boundary | `DispatchMesh`, mesh/amplification shader bytecode, and mesh pipeline state creation are separate execution work. | This slice must keep `executed_mesh_shader=0`, `executed_backend=0`, and no `ShaderStage::mesh` / PSO / dispatch API changes. |
| D3D12 output/memory limits | The DirectX mesh shader spec defines output-size and groupshared-memory constraints for actual shader execution. | This capability gate records selected non-zero output/payload/shared-memory limits only; detailed shader-output byte-budget validation belongs to the future execution plan. |
| Vulkan feature query | `VkPhysicalDeviceMeshShaderFeaturesEXT` contains `taskShader`, `meshShader`, and `meshShaderQueries`; `meshShader` must be enabled before using mesh shader stage/pipeline bits. | A Vulkan row is ready only when it represents `VK_EXT_mesh_shader` feature evidence with `meshShader=true`, selected task/amplification support, and query-statistics support. |
| Vulkan properties query | `VkPhysicalDeviceMeshShaderPropertiesEXT` exposes independent task/mesh workgroup, payload, shared-memory, output-vertex, and output-primitive limits. | Do not invent a Vulkan rule that `maxMeshOutputPrimitives <= maxMeshOutputVertices`; the v1 gate requires non-zero selected limits and at least one triangle worth of output capacity. |
| Vulkan execution boundary | `vkCmdDrawMeshTasksEXT`, `vkCmdDrawMeshTasksIndirectEXT`, and `vkCmdDrawMeshTasksIndirectCountEXT` record mesh task draws. | This slice must not call those commands, must not add SPIR-V mesh shader artifacts, and must not claim Vulkan mesh shader LOD readiness. |
| Pipeline statistics | `meshShaderQueries` covers mesh primitive/generated and task/mesh invocation query support. | Package counters must expose `mavg_mesh_shader_capability_gate_pipeline_statistics_rows=2`; missing query-statistics evidence fails closed. |
| Fallback policy | Microsoft's mesh shader guidance explicitly recognizes conventional index-buffer fallback for meshlet-style assets; existing MAVG already has compute/indirect and conventional indexed draw evidence. | `compute_indirect_fallback_ready=true` and `fallback_to_conventional_indexed_draws=1` are mandatory before the capability gate can be ready. |

## Ambiguity Lock

- `mavg_mesh_shader_capability_gate_ready=1` means only that selected D3D12 and Vulkan capability rows are present and the fallback path remains ready.
- It does not mean `mavg_mesh_shader_lod_ready=1`, D3D12/Vulkan mesh shader execution, backend PSO support, shader artifact support, GPU readback proof, Metal readiness, Nanite compatibility/equivalence/superiority, autonomous streaming, deformation, ray tracing, or broad CPU/GPU/memory optimization.
- The `task_or_amplification_shader_supported` field is a MAVG v1 policy requirement for the future task/amplification-assisted cluster path. It is not a generic statement that Vulkan mesh shaders always require `taskShader`.
- The selected output-limit check is intentionally minimal for a pre-execution gate. Future shader execution must add backend-local validation for actual shader workgroup sizes, output counts, payload/shared memory, shader compilation, pipeline creation, dispatch, synchronization, and readback.
- This plan is a child of the existing MAVG master Phase 6 prerequisite. It must update the master/registry/manifest instead of creating a second MAVG master plan or reopening completed LOD/streaming/async-overlap plans.

## Scope

In scope:

- `MK_runtime_rhi` value API `RuntimeMavgMeshShaderCapabilityGateDesc`, `RuntimeMavgMeshShaderCapabilityBackendRow`, `RuntimeMavgMeshShaderCapabilityGateResult`, `RuntimeMavgMeshShaderCapabilityDiagnosticCode`, and `evaluate_runtime_mavg_mesh_shader_capability_gate`.
- D3D12 row kind for `D3D12_FEATURE_D3D12_OPTIONS7` / `MeshShaderTier` evidence.
- Vulkan row kind for `VK_EXT_mesh_shader` `VkPhysicalDeviceMeshShaderFeaturesEXT` / `VkPhysicalDeviceMeshShaderPropertiesEXT` evidence.
- Fail-closed diagnostics for missing rows, duplicate rows, unsupported backend rows, missing feature query, unsupported mesh/task features, missing or invalid selected limits, missing compute/indirect fallback, native handle access, mesh shader execution, backend execution claim, Metal readiness claim, Nanite equivalence claim, and broad MAVG backend readiness claim.
- Unit tests for ready and fail-closed behavior.
- `sample_desktop_runtime_game --require-mavg-mesh-shader-capability-gate` counters.
- `tools/validate-mavg-mesh-shader-capability-gate.ps1`.
- Docs, manifest fragment, composed manifest, plan registry, and static AI integration guard synchronization.

Out of scope:

- `ShaderStage::mesh`, task/amplification shader stage APIs, mesh shader PSO/pipeline creation, `DispatchMesh`, `vkCmdDrawMeshTasksEXT`, `vkCmdDrawMeshTasksIndirectEXT`, `vkCmdDrawMeshTasksIndirectCountEXT`, shader artifacts, backend live query adapters, GPU readback proof, Metal MAVG readiness, mesh shader LOD execution, Nanite compatibility/equivalence/superiority, broad MAVG backend readiness, or broad optimization.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_mesh_shader_capability_gate.hpp`
- Create: `engine/runtime_rhi/src/mavg_mesh_shader_capability_gate.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Create: `tests/unit/runtime_rhi_mavg_mesh_shader_capability_gate_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Create: `tools/validate-mavg-mesh-shader-capability-gate.ps1`
- Create: `tools/check-ai-integration-123-mavg-mesh-shader-capability-gate.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Regenerate: `engine/agent/manifest.json`

## Done When

- `evaluate_runtime_mavg_mesh_shader_capability_gate` returns `mavg_mesh_shader_capability_gate_ready=1` only when selected D3D12 and Vulkan rows use the official query kinds, feature-query evidence exists, mesh/task support and selected limits are present, pipeline statistics rows are present, compute/indirect fallback remains ready, and diagnostics are empty.
- Package smoke emits `mavg_mesh_shader_capability_gate_ready=1`, two backend rows, two ready backend rows, D3D12/Vulkan ready rows, two feature-query rows, two pipeline-statistics rows, fallback ready, fallback-to-conventional-indexed-draws ready, and zero counters for mesh shader LOD ready, D3D12/Vulkan mesh shader LOD ready, native handles, mesh shader execution, backend execution, Metal readiness, Nanite equivalence, and broad MAVG backend readiness.
- Docs and manifest state that mesh shader execution remains 0 and broad MAVG backend readiness remains unclaimed.
- Static guards reject missing official API names, missing counters, missing non-claims, stale active-plan selection, or broad readiness promotion.
- Focused validator, static checks, and full validation pass before publication.

## Tasks

### Task 1: Active Plan Selection

- [x] Create this dated child plan under `docs/superpowers/plans/`.
- [x] Point `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json.aiOperableProductionLoop.currentActivePlan` at this plan.
- [x] Set `recommendedNextPlan.id` to `mavg-mesh-shader-capability-gate-v1`.
- [x] Update `docs/superpowers/plans/README.md`, the production master plan, and the MAVG master plan.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 2: Runtime-RHI API And Tests

- [x] Add `mavg_mesh_shader_capability_gate.hpp`.
- [x] Add `mavg_mesh_shader_capability_gate.cpp`.
- [x] Add `MK_runtime_rhi_mavg_mesh_shader_capability_gate_tests`.
- [x] Register runtime source and test target in CMake.
- [x] Run the focused test target and fix compile/test failures.

### Task 3: Package Counter And Validator

- [x] Add `--require-mavg-mesh-shader-capability-gate` to `sample_desktop_runtime_game`.
- [x] Emit package-visible counters for ready status, backend rows, D3D12/Vulkan rows, feature-query rows, pipeline-statistics rows, fallback rows, and explicit zero non-claims.
- [x] Add `tools/validate-mavg-mesh-shader-capability-gate.ps1`.
- [x] Run the validator with `-RequireReady`.

### Task 4: Docs, Manifest, Static Guards

- [x] Update current capabilities, roadmap, MAVG master plan, production master plan, plan registry, manifest fragments, and composed manifest.
- [x] Fix stale MAVG master wording that still treats selected async-overlap performance proof as unclaimed.
- [x] Add `tools/check-ai-integration-123-mavg-mesh-shader-capability-gate.ps1`.
- [x] Run `check-ai-integration`, `check-json-contracts`, `check-agents`, `check-text-format`, `check-format`, and `git diff --check`.

### Task 5: Slice Validation And Publication

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-mesh-shader-capability-gate.ps1 -RequireReady`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [ ] Run publication preflight, commit the validated candidate, push branch `codex/mavg-mesh-shader-capability-gate`, open a draft PR, wait for hosted checks, mark ready with `tools/ready-task-pr.ps1`, and register auto-merge only after PR Gate succeeds.
