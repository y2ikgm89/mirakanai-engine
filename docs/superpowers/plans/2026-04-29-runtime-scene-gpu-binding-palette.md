# Runtime Scene GPU Binding Palette Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn cooked runtime scene packages into renderer-ready mesh and material GPU binding palettes through a backend-neutral RHI adapter, so desktop runtime games can progress from package-only scene submission toward real uploaded scene draws.

**Architecture:** Add a small `mirakana_runtime_scene_rhi` bridge that depends on `mirakana_runtime`, `mirakana_runtime_rhi`, and `mirakana_scene_renderer`. Keep `mirakana_runtime` and `mirakana_scene` free of RHI dependencies. The bridge reads asset ids from a `SceneRenderPacket`, loads typed mesh/material/texture payloads from a `RuntimeAssetPackage`, uploads them through `mirakana_runtime_rhi`, creates material descriptor bindings, and returns a `SceneGpuBindingPalette` plus retained upload results and failures. The API must expose only first-party `mirakana::` value types and RHI abstraction handles; it must not expose SDL3, native OS handles, D3D12/Vulkan/Metal backend handles, Dear ImGui, or editor APIs to gameplay.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_runtime_rhi`, `mirakana_scene_renderer`, `mirakana_rhi`, `SceneRenderPacket`, `SceneGpuBindingPalette`, focused `mirakana_runtime_scene_rhi_tests`, desktop-runtime validation.

---

## Context

- `sample_desktop_runtime_game` already packages cooked texture, mesh, material, scene, and package-index files and submits the cooked scene through `IRenderer`.
- `mirakana_runtime_rhi` can upload typed runtime texture and mesh payloads and create material descriptor sets from `MaterialPipelineBindingMetadata`.
- `mirakana_scene_renderer` can resolve `SceneGpuBindingPalette` entries into `MeshCommand` values and `RhiFrameRenderer` can bind descriptors, vertex buffers, index buffers, and draw indexed geometry.
- The missing production step is a reusable package-to-GPU-palette adapter so game hosts and samples do not hand-roll per-scene RHI upload plumbing.

## Constraints

- Keep public APIs under `mirakana::` and avoid backend-native handles in game-facing contracts.
- Do not add third-party dependencies.
- Do not make `mirakana_runtime`, `mirakana_scene`, or game code depend on concrete RHI backends.
- Preserve deterministic failure reporting; missing or malformed assets must produce non-throwing failures where practical.
- Keep material pipeline-layout ownership honest: use one scene-shared material pipeline layout for the current single-pipeline `RhiFrameRenderer` path, allow a host-supplied descriptor set layout only when the adapter can create the matching pipeline layout itself, and reject incompatible default material descriptor layouts instead of returning a palette that fails later during descriptor binding.
- Keep broader D3D12 shader/pipeline integration, Vulkan presentation, and Metal presentation as follow-up unless this slice validates them directly.

## Done When

- [x] `mirakana_runtime_scene_rhi` exposes a backend-neutral function that builds a `SceneGpuBindingPalette` from a `RuntimeAssetPackage` and `SceneRenderPacket`.
- [x] The result retains uploaded mesh, texture, and material binding resources and reports deterministic per-asset failures.
- [x] Tests prove mesh upload registration, material texture upload/descriptor registration, duplicate asset de-duplication, compatible multi-material scene-shared layouts, incompatible layout failures, missing payload failures, and successful `RhiFrameRenderer` submission through the generated palette.
- [x] CMake target, install/export metadata, docs, roadmap, gap analysis, manifest, skills, subagents, and Claude guidance are synchronized.
- [x] Focused runtime-scene RHI tests, desktop runtime validation, shader/toolchain/API boundary checks where relevant, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact blockers are recorded.

---

### Task 1: RED Runtime Scene RHI Tests

**Files:**
- Add: `tests/unit/runtime_scene_rhi_tests.cpp`
- Add: `engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add failing tests for package-to-GPU palette creation**

Create a cooked package with:

- one texture payload with RGBA8 byte data
- one mesh payload with float32x3 positions and uint32 indices
- one material payload that references the texture
- one scene render packet that references the mesh/material pair twice

Expected behavior:

- mesh and material palette counts are de-duplicated to one each
- texture and mesh uploads are retained in the result
- material descriptor set, owner device, and pipeline layout are valid
- submitting the packet through `RhiFrameRenderer` resolves mesh/material GPU bindings and records an indexed draw

- [x] **Step 2: Add failure-path tests**

Cover at least one missing mesh payload and one missing material texture payload. Expected behavior: result is unsuccessful, failures identify the missing asset id, and the palette does not register the incomplete binding.

- [x] **Step 3: Verify RED**

Run:

```powershell
cmake --build --preset dev --target mirakana_runtime_scene_rhi_tests
```

Expected before implementation: compile failure for missing target/header/API.

### Task 2: Implement `mirakana_runtime_scene_rhi`

**Files:**
- Add: `engine/runtime_scene_rhi/CMakeLists.txt`
- Add: `engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp`
- Add: `engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add result and option types**

Define value types for:

- `RuntimeSceneGpuBindingFailure`
- `RuntimeSceneGpuBindingOptions`
- `RuntimeSceneGpuBindingResult`

Retain mesh uploads, texture uploads, material GPU bindings, generated scene-shared material pipeline layouts, and the `SceneGpuBindingPalette`.

- [x] **Step 2: Upload unique referenced meshes**

For every mesh referenced by `SceneRenderPacket::meshes`, find a package mesh record, parse `RuntimeMeshPayload`, call `upload_runtime_mesh`, convert with `make_runtime_mesh_gpu_binding`, and register the binding once.

- [x] **Step 3: Upload unique referenced material textures and material bindings**

For every referenced material, parse `RuntimeMaterialPayload`, upload its referenced texture payloads once, create `RuntimeMaterialTextureResource` values, call `create_runtime_material_gpu_binding`, then register a `MaterialGpuBinding` using the scene-shared pipeline layout created from the material descriptor set layout. Reject incompatible default material descriptor layouts because the current `RhiFrameRenderer` binds one graphics pipeline per active frame.

- [x] **Step 4: Keep failures non-throwing and deterministic**

Record failures by asset id with concise diagnostics. Skip duplicate failures for the same asset where doing so avoids noisy output, but keep enough context to diagnose missing mesh/material/texture data.

### Task 3: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/workflows.md` if validation flow changes
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.codex/agents/engine-architect.toml`
- Modify: `.claude/agents/engine-architect.md`
- Modify: rendering/gameplay builder guidance if the new adapter changes recommended game workflow

- [x] **Step 1: Synchronize capability claims**

Describe the new adapter as a backend-neutral package-to-scene-GPU-palette bridge. Keep visible packaged D3D12 scene drawing, Vulkan SDL3 presentation, Metal presentation, shadowing, and broader render graph work as follow-up unless validated here.

- [x] **Step 2: Keep manifest status honest**

Mark the bridge implemented only after tests pass. Keep backend-specific presentation statuses separated as implemented, host-gated, toolchain-gated, diagnostic-only, planned, or not implemented.

### Task 4: Verification

- [x] Run focused `mirakana_runtime_scene_rhi_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` because renderer/RHI path behavior is affected.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run selected package validation for `sample_desktop_runtime_game`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Validation Evidence

- `cmake --build --preset dev --target mirakana_runtime_rhi_tests mirakana_runtime_scene_rhi_tests`: PASS.
- `ctest --preset dev --output-on-failure -R "mirakana_runtime_rhi_tests|mirakana_runtime_scene_rhi_tests"`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: PASS as diagnostic-only; D3D12 DXIL and Vulkan SPIR-V are ready, while Metal shader/library packaging remains host-gated because `metal` and `metallib` are missing.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS after rerunning outside the sandbox for the known vcpkg 7zip `CreateFileW stdin failed with 5` sandbox issue.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: PASS after rerunning outside the sandbox for the same vcpkg 7zip sandbox issue; the installed SDK/package includes `mirakana_runtime_scene_rhi` and the sample still reports deterministic scene counters.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Known diagnostic-only blockers remain: Metal shader/library tools missing on this Windows host, Apple packaging requires macOS/Xcode tools, Android release signing is not configured, Android device smoke is not connected, and strict clang-tidy remains diagnostic-only without the active generator compile database.
