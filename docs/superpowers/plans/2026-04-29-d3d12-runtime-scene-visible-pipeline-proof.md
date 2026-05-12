# D3D12 Runtime Scene Visible Pipeline Proof Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove that cooked runtime scene packages can reach a real D3D12 `RhiFrameRenderer` draw with uploaded mesh buffers, material descriptors, a material-compatible pipeline layout, indexed draw submission, and texture readback evidence.

**Architecture:** Keep `mirakana_runtime`, `mirakana_scene`, and gameplay code RHI-free. Use the completed `mirakana_runtime_scene_rhi` bridge to build a retained `SceneGpuBindingPalette` from cooked package payloads and a renderer-neutral `SceneRenderPacket`. Create the D3D12 graphics pipeline from the exact material pipeline layout returned by that palette so `RhiFrameRenderer` descriptor binding uses the currently bound pipeline layout handle. This slice is a focused D3D12/RHI/renderer proof; packaged SDL3 game-window scene GPU injection remains follow-up until the host-owned ownership model is designed without exposing `IRhiDevice` or native handles to gameplay.

**Tech Stack:** C++23, `mirakana_rhi_d3d12`, `mirakana_runtime_scene_rhi`, `mirakana_runtime_rhi`, `mirakana_scene_renderer`, `RhiFrameRenderer`, focused `mirakana_d3d12_rhi_tests`.

---

## Context

- `mirakana_runtime_scene_rhi` now uploads unique scene mesh, texture, and material payloads and returns a retained `SceneGpuBindingPalette`.
- `RhiFrameRenderer` binds a single graphics pipeline at frame start and requires material descriptor bindings to use the same `PipelineLayoutHandle` as the bound pipeline.
- D3D12 already has separate visible proofs for runtime material texture sampling and uploaded runtime mesh/index draw, but not the combined cooked-scene package path.
- `sample_desktop_runtime_game` still validates package load and scene submit through `NullRenderer` or CPU material palette fallback. Host-owned D3D12 scene GPU binding is intentionally left for a later slice.

## Constraints

- Do not expose D3D12, native window, SDL3, or `IRhiDevice` handles through game public APIs.
- Do not add third-party dependencies.
- Do not change `mirakana_runtime` or `mirakana_scene` dependency direction.
- Use the exact `MaterialGpuBinding.pipeline_layout` produced by the runtime scene GPU palette when creating the D3D12 graphics pipeline.
- Keep the shader contract aligned with runtime material metadata: material factors at `b0`, base-color texture at `t1`, and base-color sampler at `s16`.
- Keep Vulkan/Metal presentation and packaged SDL3 host injection as follow-up unless validated directly.

## Done When

- [x] A focused D3D12 test constructs a cooked runtime scene package, instantiates a `SceneRenderPacket`, builds a `SceneGpuBindingPalette` through `mirakana_runtime_scene_rhi`, and creates a D3D12 graphics pipeline from the palette material pipeline layout.
- [x] The focused test renders through `RhiFrameRenderer`, resolves mesh and material GPU bindings from the palette, performs an indexed draw, copies the render target to a readback buffer, and verifies non-clear material-textured color bytes.
- [x] CMake links the D3D12 test target directly to `mirakana_runtime_scene_rhi`, and `mirakana_runtime_scene_rhi` declares direct `mirakana_rhi` public dependency because its public header exposes RHI abstraction handles.
- [x] Plan registry, roadmap/gap analysis, manifest, rendering skills, and Codex/Claude guidance distinguish this visible D3D12 proof from the still-planned packaged SDL3 game-window scene GPU injection.
- [x] Focused D3D12 validation, shader/toolchain check, API boundary check, desktop runtime validation, selected package validation, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact blockers are recorded.

---

### Task 1: RED D3D12 Cooked Scene Visible Test

**Files:**
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add a failing D3D12 scene GPU palette readback test**

Create a cooked package with:

- one texture payload with RGBA8 byte data
- one mesh payload with float32x3 position vertices and uint32 indices
- one material payload referencing the texture
- one scene render packet referencing the mesh/material pair

Expected behavior:

- `build_runtime_scene_gpu_binding_palette` succeeds
- the resulting palette contains one mesh and one material GPU binding
- the D3D12 graphics pipeline is created from the material binding pipeline layout
- `submit_scene_render_packet` resolves mesh/material GPU bindings
- `RhiFrameRenderer` records an indexed draw and the render target readback contains the sampled material color

- [x] **Step 2: Verify RED**

Run:

```powershell
cmake --build --preset dev --target mirakana_d3d12_rhi_tests
```

Expected before implementation: compile/link failure for missing `mirakana_runtime_scene_rhi` linkage or missing test helper implementation.

### Task 2: Implement Focused D3D12 Proof

**Files:**
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add package/scene helper payloads**

Use deterministic first-party cooked texture/material/mesh payload text. Encode mesh bytes from explicit float32 triangle positions and uint32 indices instead of using opaque dummy bytes.

- [x] **Step 2: Add D3D12 scene material shaders**

Compile inline test shaders that match runtime material metadata: position-only vertex input with fixed UVs for the current sample mesh payload, and a pixel shader sampling `b0/t1/s16`.

- [x] **Step 3: Render and read back**

Create the render target/readback buffer, pre-transition the target for `RhiFrameRenderer`, create the graphics pipeline from the palette material layout, submit the scene packet with the GPU palette, transition/copy to readback, and assert material-tinted color bytes plus D3D12 stats.

### Task 3: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`

- [x] **Step 1: Keep capability claims honest**

Describe the D3D12 visible cooked-scene proof as implemented only after the test passes. Keep packaged SDL3 game-window scene GPU injection, Vulkan SDL3 presentation, and Metal presentation as follow-up.

- [x] **Step 2: Register this plan and close the prior slice**

Move Runtime Scene GPU Binding Palette to completed implementation slices and set this plan as the current active slice.

### Task 4: Verification

- [x] Run `cmake --build --preset dev --target mirakana_d3d12_rhi_tests mirakana_runtime_scene_rhi_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R "mirakana_d3d12_rhi_tests|mirakana_runtime_scene_rhi_tests"`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Validation Evidence

- RED check: `cmake --build --preset dev --target mirakana_d3d12_rhi_tests` failed as expected before helper implementation because `compile_runtime_scene_material_vertex_shader`, `make_d3d12_runtime_scene_package`, and `make_d3d12_runtime_scene` were missing.
- `cmake --build --preset dev --target mirakana_d3d12_rhi_tests mirakana_runtime_scene_rhi_tests`: PASS.
- `ctest --preset dev --output-on-failure -R "mirakana_d3d12_rhi_tests|mirakana_runtime_scene_rhi_tests"`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: PASS as diagnostic-only; D3D12 DXIL and Vulkan SPIR-V are ready, while Metal shader/library packaging remains host-gated because `metal` and `metallib` are missing.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS after rerunning outside the sandbox for the known vcpkg 7zip `CreateFileW stdin failed with 5` sandbox issue.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: PASS after rerunning outside the sandbox for the same vcpkg 7zip sandbox issue.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Known diagnostic-only blockers remain: Metal shader/library tools missing on this Windows host, Apple packaging requires macOS/Xcode tools, Android release signing is not configured, Android device smoke is not connected, and strict clang-tidy remains diagnostic-only without the active generator compile database.
