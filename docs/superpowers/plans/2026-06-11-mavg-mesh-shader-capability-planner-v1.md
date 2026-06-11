# MAVG Mesh Shader Capability Planner v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a renderer-neutral, value-only MAVG mesh shader capability planner that records D3D12/Vulkan support, limits, statistics availability, and compute/indirect fallback diagnostics without executing mesh shaders.

**Architecture:** The planner lives in `MK_renderer` beside the existing MAVG GPU culling planner. It consumes caller-reviewed backend capability rows and selected MAVG cluster output-shape requirements, then returns deterministic path rows and fail-closed diagnostics. Backend execution, shader files, native handles, Work Graphs, Metal proof, and benchmark claims remain out of scope.

**Tech Stack:** C++23, `MK_renderer`, CMake/CTest, PowerShell validation tools, official DirectX Mesh Shader and Vulkan `VK_EXT_mesh_shader` constraints.

---

## Scope

Files:

- Create: `engine/renderer/include/mirakana/renderer/mavg_mesh_shader_policy.hpp`
- Create: `engine/renderer/src/mavg_mesh_shader_policy.cpp`
- Create: `tests/unit/mavg_mesh_shader_policy_tests.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json` via `tools/compose-agent-manifest.ps1 -Write`

Non-claims:

- No D3D12 `DispatchMesh`, Vulkan `vkCmdDrawMeshTasksEXT`, shader compilation, PSO creation, native handle exposure, Work Graphs, Metal readiness, async overlap, benchmark superiority, or broad MAVG backend readiness.
- Missing or insufficient mesh shader support must select the compute/indirect fallback path with diagnostics.

## Task 1: Planner API And RED Test

- [x] **Step 1: Write the failing planner test**

Add `tests/unit/mavg_mesh_shader_policy_tests.cpp` with tests that include `mirakana/renderer/mavg_mesh_shader_policy.hpp` and assert:

- `plan_mavg_mesh_shader_capability` selects `MavgMeshShaderPathKind::mesh_shader` when D3D12 and Vulkan rows both report mesh shader support, D3D12 pipeline statistics support, Vulkan task/mesh shader features, and limits at least as high as the requested cluster output shape.
- It records two backend rows, no diagnostics, `selected_mesh_shader_path=true`, `selected_compute_indirect_fallback=false`, `executed_mesh_shader=false`, and `touched_native_handles=false`.

Run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_mesh_shader_policy_tests`

Expected: fail because the target/header does not exist.

- [x] **Step 2: Add the minimal API surface**

Create `mavg_mesh_shader_policy.hpp` with:

- `MavgMeshShaderBackend`
- `MavgMeshShaderPathKind`
- `MavgMeshShaderDiagnosticCode`
- `MavgMeshShaderBackendCapabilityRow`
- `MavgMeshShaderOutputShape`
- `MavgMeshShaderCapabilityDesc`
- `MavgMeshShaderBackendPlanRow`
- `MavgMeshShaderDiagnostic`
- `MavgMeshShaderCapabilityPlan`
- `plan_mavg_mesh_shader_capability`
- `has_mavg_mesh_shader_diagnostic`

- [x] **Step 3: Add minimal implementation and CMake wiring**

Create `mavg_mesh_shader_policy.cpp`, add it to `engine/renderer/CMakeLists.txt`, and add `MK_mavg_mesh_shader_policy_tests` beside `MK_mavg_gpu_culling_tests` in root `CMakeLists.txt`.

- [x] **Step 4: Verify GREEN**

Run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_mesh_shader_policy_tests`

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_mesh_shader_policy_tests`

Expected: configure/build/test pass.

## Task 2: Fallback And Limit Diagnostics

- [x] **Step 1: Add failing tests for fallback behavior**

Extend `mavg_mesh_shader_policy_tests.cpp` to assert:

- Missing D3D12 mesh shader support returns `MavgMeshShaderPathKind::compute_indirect_fallback`, reports `d3d12_mesh_shader_unsupported`, and keeps `selected_mesh_shader_path=false`.
- Missing Vulkan `VK_EXT_mesh_shader`/feature support returns fallback and reports `vulkan_mesh_shader_unsupported`.
- Output vertex/primitive/threadgroup/payload limits below the requested output shape report limit diagnostics and return fallback.
- D3D12 missing mesh shader pipeline statistics support reports `d3d12_pipeline_statistics_unavailable` but does not make the plan fail if mesh shader support and limits are otherwise valid.

Run the same CTest command and verify the new tests fail.

- [x] **Step 2: Implement deterministic validation**

Update `mavg_mesh_shader_policy.cpp` so diagnostics are deterministic by backend order (`d3d12`, then `vulkan`) and fail closed to fallback for unsupported backends or insufficient limits. Pipeline-statistics absence must stay a diagnostic because official DirectX guidance treats it as separately queryable.

- [x] **Step 3: Verify GREEN**

Run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_mesh_shader_policy_tests`

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_mesh_shader_policy_tests`

Expected: pass.

## Task 3: Docs, Manifest, And Slice Validation

- [x] **Step 1: Update docs and manifest fragment**

Update the MAVG architecture spec and master plan to record the new value-only capability planner, while preserving non-claims for backend execution, shader code, Work Graphs, Metal, and broad MAVG readiness.

Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and compose:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`

- [x] **Step 2: Run focused static checks**

Run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`

- [x] **Step 3: Run full validation and publish**

Run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1`

Then stage only task-owned files, commit, push `codex/mavg-mesh-shader-capability-planner-v1`, and create a draft PR.
