# MAVG Streamed Cluster Backend Draw Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Connect committed streamed MAVG page GPU bindings to backend-neutral `MeshCommand` draw submission evidence.

**Plan ID:** `mavg-streamed-cluster-backend-draw-execution-v1`

**Architecture:** Keep the bridge in `engine/runtime_scene_rhi`, where runtime GPU upload results can be combined with scene-renderer command planning. Preserve the existing `scene_renderer` dependency boundary by not making it depend on `runtime_rhi`.

**Non-claims:** This slice does not add candidate reload, streaming-state mutation, DirectStorage, autonomous streaming services, mesh shader execution, native handle exposure, async-overlap/performance proof, Nanite readiness, or broad optimization.

**Tech Stack:** C++23, CMake, `MK_runtime_scene_rhi`, `MK_scene_renderer`, `MK_runtime_rhi`, Null RHI renderer tests.

---

### Task 1: Streamed Page Binding Draw Planning

**Files:**
- Create: `tests/unit/runtime_scene_rhi_mavg_streamed_backend_draw_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `engine/scene_renderer/include/mirakana/scene_renderer/mavg_scene_lod.hpp`
- Modify: `engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp`
- Modify: `engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp`

- [x] **Step 1: Write the failing test**

Add a focused test target that asks `runtime_scene_rhi` to convert a succeeded `RuntimeMavgStreamedClusterGpuUploadResult` into page-asset `MeshCommand` rows, fail closed when a selected page binding is missing, submit through `NullRenderer`, and record range-aware indexed RHI draws through `RhiFrameRenderer`.

- [x] **Step 2: Run RED**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests
```

Expected: compile failure for the new `RuntimeMavgStreamedSceneLodSubmitDesc`, `plan_runtime_mavg_streamed_scene_lod_mesh_commands`, and streamed-page diagnostic enum values.

- [x] **Step 3: Implement the bridge**

Add the runtime-scene-RHI submit desc and planner. It must validate graph/selection/upload readiness, reject missing page bindings, keep material bindings optional with existing fallback diagnostics, and emit `MeshCommand` rows whose `mesh` and `mesh_binding` come from the resident streamed page binding.

- [x] **Step 4: Verify focused behavior**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests|MK_scene_renderer_mavg_lod_tests|MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests"
```

- [x] **Step 5: Synchronize durable agent/docs surfaces**

Update roadmap/current capabilities/master plan/spec/plan registry and manifest fragments, then compose `engine/agent/manifest.json`.

- [ ] **Step 6: Close validation and publication**

Run focused static/document checks, full `tools/validate.ps1`, publication preflight, commit, push, and open a draft PR with validation evidence.

**Local evidence so far:**
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests"`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests|MK_scene_renderer_mavg_lod_tests|MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests"`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp,tests/unit/runtime_scene_rhi_mavg_streamed_backend_draw_tests.cpp"`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
