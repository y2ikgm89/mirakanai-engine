# SDL3 D3D12 Packaged Scene GPU Injection Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let `sample_desktop_runtime_game` promote its packaged cooked scene from CPU material-palette submission to host-owned D3D12 scene GPU bindings when a real SDL3/Win32 D3D12 presentation is available, while preserving deterministic `NullRenderer` fallback and without exposing native handles or an `IRhiDevice` getter to gameplay.

**Architecture:** Keep D3D12 device/swapchain ownership inside `mirakana_runtime_host_sdl3_presentation`. Add a host-owned scene GPU binding path that can use the internal RHI device to call `mirakana_runtime_scene_rhi`, create a scene graphics pipeline from the exact palette material pipeline layout, replace the frame renderer pipeline between frames, and hand the game only first-party renderer/scene submission state. Package target-specific scene shader artifacts beside `sample_desktop_runtime_game` so installed smoke can require the artifact presence without runtime shader compilation.

**Tech Stack:** C++23, `mirakana_runtime_host_sdl3_presentation`, `mirakana_runtime_scene_rhi`, `mirakana_runtime_host` shader bytecode loading, `sample_desktop_runtime_game`, `games/CMakeLists.txt`, focused SDL3 runtime-host tests, desktop-runtime/package validation.

---

## Context

- `mirakana_runtime_scene_rhi` builds retained scene GPU palettes from cooked packages.
- `mirakana_d3d12_rhi_tests` now proves a cooked runtime scene GPU palette can drive D3D12 `RhiFrameRenderer` indexed draw/readback when the graphics pipeline is created from the palette material layout.
- `SdlDesktopPresentation` owns the D3D12 `IRhiDevice`, swapchain, and `RhiFrameRenderer`, but currently creates its initial D3D12 renderer with an empty pipeline layout.
- `sample_desktop_runtime_game` loads packaged config and cooked scene package, then submits the scene with CPU material palette fallback. It does not yet load target-specific scene shader artifacts or request host-owned scene GPU bindings.

## Constraints

- Do not expose native Win32, SDL3, DXGI, D3D12, Vulkan, Metal, or Dear ImGui handles to gameplay.
- Do not add an `IRhiDevice` getter to `SdlDesktopGameHost`, `SdlDesktopPresentation`, `GameApp`, or game samples.
- Keep `mirakana_runtime`, `mirakana_scene`, and game code free of concrete RHI backend dependencies.
- Preserve `NullRenderer` fallback for dummy/headless smoke paths.
- Runtime shader compilation remains disallowed; D3D12 scene shaders are build/package artifacts.
- The D3D12 scene graphics pipeline must be created from the exact `MaterialGpuBinding::pipeline_layout` returned by the host-built scene GPU palette.
- Vulkan/Metal presentation remains follow-up.

## Done When

- [x] Runtime-host tests cover the host-owned scene GPU binding request shape, null-fallback diagnostics, and no-device behavior without exposing `IRhiDevice`.
- [x] `SdlDesktopPresentation`/`SdlDesktopGameHost` can build scene GPU bindings using the private D3D12 RHI device and create the renderer pipeline from the exact palette material pipeline layout.
- [x] `sample_desktop_runtime_game` loads packaged scene shader DXIL artifacts, requests scene GPU binding promotion when D3D12 is active, reports deterministic scene GPU counters/diagnostics, and keeps CPU/Null fallback behavior.
- [x] `games/CMakeLists.txt` builds and installs target-specific `sample_desktop_runtime_game` scene shader artifacts when DXC is available and the target is selected for desktop runtime packaging.
- [x] Docs, roadmap, gap analysis, manifest, rendering skills, gameplay-development skills, and Codex/Claude subagent guidance describe packaged SDL3 D3D12 scene GPU injection as implemented only after validation.
- [x] Focused runtime-host/sample validations, shader/toolchain check, desktop runtime validation, selected package validation, API boundary check, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact blockers are recorded.

---

### Task 1: RED Host-Owned Scene GPU Binding Tests

**Files:**
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp`

- [x] **Step 1: Add failing tests for null/no-device scene GPU binding requests**

Expected behavior:

- dummy SDL presentation remains `NullRenderer`
- a scene GPU binding request returns a deterministic unavailable diagnostic
- no public API exposes `IRhiDevice` or native handles

- [x] **Step 2: Add a strict D3D12 host integration proof through selected package validation**

The real-window integration proof is the selected installed `sample_desktop_runtime_game` package lane. It loads target-specific scene DXIL artifacts, creates the host-owned SDL3/Win32/D3D12 renderer, builds scene GPU bindings through the private presentation device, creates the pipeline from the palette material layout, submits the scene for two frames, and verifies backend-neutral scene GPU binding counters/diagnostics without exposing `IRhiDevice`.

### Task 2: Implement Host-Owned Binding And Pipeline Replacement

**Files:**
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_game_host.cpp`
- Modify: `engine/runtime_host/sdl3/CMakeLists.txt`

- [x] **Step 1: Add first-party request/result value types**

Expose package/packet input, D3D12 shader bytecode, vertex layout metadata, success state, backend-neutral diagnostics, and a retained scene GPU binding owner without exposing native handles or `IRhiDevice`.

- [x] **Step 2: Use private presentation device**

Inside `SdlDesktopPresentation`, call `mirakana_runtime_scene_rhi::build_runtime_scene_gpu_binding_palette` only when the backend is D3D12 and the private device/RHI renderer are available.

- [x] **Step 3: Create the scene graphics pipeline from the palette material layout**

Create D3D12 shaders and the graphics pipeline from the exact palette material pipeline layout. Reject invalid shader/package/packet/layout requests deterministically.

### Task 3: Sample Game Shader Artifacts And Promotion

**Files:**
- Add: `games/sample_desktop_runtime_game/shaders/runtime_scene.hlsl`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_desktop_runtime_game/README.md`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `games/CMakeLists.txt`

- [x] **Step 1: Add target-specific scene shader artifact path**

Build `sample_desktop_runtime_game_scene.vs.dxil` and `sample_desktop_runtime_game_scene.ps.dxil` from `runtime_scene.hlsl` with DXC when available. Install them only when `sample_desktop_runtime_game` is the selected desktop-runtime package target.

- [x] **Step 2: Load artifacts and request GPU scene promotion**

Add `--require-d3d12-scene-shaders`, `--require-d3d12-renderer`, and `--require-scene-gpu-bindings` style flags. Default smoke remains dummy/Null-compatible; strict local Windows smoke can require real D3D12.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Keep status honest**

Mark the packaged SDL3 D3D12 scene GPU injection implemented only after host/sample/package validations pass. Keep Vulkan/Metal presentation and broader material pipeline authoring as follow-up.

### Task 5: Verification

- [x] Run focused runtime-host SDL3 tests.
- [x] Run focused `sample_desktop_runtime_game` source-tree smoke paths.
- [x] Run optional real-window D3D12 strict smoke if the local host supports it.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Validation Evidence

- `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests mirakana_runtime_host_sdl3_public_api_compile sample_desktop_runtime_game`: PASS after the known sandbox vcpkg `CreateFileW stdin failed with 5` configure failure was rerun outside the sandbox.
- `ctest --preset desktop-runtime --output-on-failure -R "mirakana_runtime_host_sdl3_tests|mirakana_runtime_host_sdl3_public_api_compile|sample_desktop_runtime_game_smoke"`: PASS.
- `cmake --build --preset desktop-runtime --target sample_desktop_runtime_game_shaders`: PASS; DXC compiled `sample_desktop_runtime_game_scene.vs.dxil` and `.ps.dxil`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: PASS after the known sandbox vcpkg extraction failure was rerun outside the sandbox. Installed smoke output included `renderer=d3d12`, `scene_gpu_status=ready`, `scene_gpu_mesh_bindings=1`, `scene_gpu_material_bindings=1`, `scene_gpu_mesh_resolved=2`, and `scene_gpu_material_resolved=2`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: diagnostic-only PASS; D3D12 DXIL and Vulkan SPIR-V are ready, Metal `metal` and `metallib` are missing on this Windows host.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS after the known sandbox vcpkg extraction failure was rerun outside the sandbox; this now builds and runs `mirakana_runtime_host_sdl3_public_api_compile`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Known diagnostic-only blockers remain Metal/Apple host tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database unavailable for the active Visual Studio generator.
