# Frame Graph And Postprocess v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a minimal production renderer frame-graph/postprocess foundation that proves an ordered scene-color pass followed by a fullscreen postprocess pass before swapchain presentation, without adding depth/shadows or exposing backend handles to gameplay.

**Architecture:** Keep frame orchestration in `mirakana_renderer` and host-owned presentation adapters. Use existing `mirakana_rhi` color texture, render-target, shader-resource, transition, descriptor, and pipeline contracts. The first slice should model pass declarations, resource read/write intent, ordering, diagnostics, and a postprocess pass path that can be tested on `NullRhiDevice` and D3D12, then wired into SDL3 D3D12/Vulkan presentation only through `mirakana_runtime_host_sdl3_presentation`.

**Tech Stack:** C++23, `mirakana_renderer`, `mirakana_rhi`, `mirakana_runtime_host_sdl3_presentation`, `mirakana_runtime_scene_rhi`, D3D12/Vulkan shader artifact lanes, `sample_desktop_runtime_game`.

---

## Context

- Lit Material v0 now proves position+normal+uv cooked meshes, base-color texture/material factors, one deterministic directional light, and D3D12/Vulkan selected package smokes.
- `RhiFrameRenderer` is currently a single render-pass renderer path.
- Depth/shadow-map support is not ready enough for a focused shadow slice because depth render-target and depth sampling contracts still need separate RHI design.
- The current RHI already has color render targets, shader resources, texture transitions, descriptor sets, graphics pipelines, and readback tests that can support a small color-only postprocess proof.

## Constraints

- Do not expose `IRhiDevice`, SDL3, D3D12, Vulkan, Metal, swapchain frames, native handles, or scene GPU palettes to game public APIs.
- Do not add new third-party dependencies.
- Do not add shadows, depth, SSAO, bloom chains, HDR tonemapping, temporal AA, particles, GPU timestamps, or editor UI in this slice.
- Keep `NullRenderer` and headless samples deterministic.
- Keep Vulkan/Metal readiness honest and host/toolchain-gated.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; renderer/RHI/shader changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.

## Done When

- [x] A focused renderer frame-graph v0 model declares ordered passes, resource read/write intent, pass diagnostics, and rejects cycles or invalid read/write combinations in tests.
- [x] A postprocess v0 path can render scene color into an offscreen color target, transition it for shader-read, run a fullscreen postprocess pass, and present/copy the result without public native handles.
- [x] `sample_desktop_runtime_game` can report backend-neutral postprocess/frame-graph status fields when the selected D3D12 or Vulkan scene presentation path is used.
- [x] D3D12 readback or strict package smoke proves the postprocess path is non-empty and does not regress Lit Material v0 scene GPU binding counters.
- [x] Docs, roadmap, gap analysis, manifest, skills, and Codex/Claude guidance describe this as Frame Graph/Postprocess v0, not a full render graph, HDR pipeline, or effects stack.
- [x] Focused renderer/RHI/runtime-host validation, shader toolchain diagnostics, desktop runtime validation, selected package validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Frame-Graph Model Tests

**Files:**
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tests/unit/rhi_tests.cpp` if RHI stats or transition coverage needs tightening.

- [x] Add tests for pass ordering, named resources, read-after-write dependencies, and deterministic diagnostics.
- [x] Add tests rejecting cycles, missing producers, missing consumers where required, and write/write hazards in the v0 model.
- [x] Keep tests backend-neutral with `NullRhiDevice` before adding D3D12 coverage.

### Task 2: Renderer Frame-Graph v0 Implementation

**Files:**
- Modify/Add under `engine/renderer/include/mirakana/renderer/`
- Modify/Add under `engine/renderer/src/`
- Modify: `engine/renderer/CMakeLists.txt`

- [x] Add first-party value types for frame-graph v0 pass/resource declarations and execution diagnostics.
- [x] Keep the API renderer/RHI-facing only; do not make generated gameplay depend on frame-graph internals.
- [x] Record backend-neutral counters/status values that host adapters can report.

### Task 3: Postprocess Pass Proof

**Files:**
- Modify: `engine/renderer/src/rhi_frame_renderer.cpp` or add a dedicated adjacent renderer path.
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `games/sample_desktop_runtime_game/shaders/`

- [x] Add a color-only scene-color target plus fullscreen postprocess shader path using existing RHI descriptors and transitions.
- [x] Add a D3D12 visible/readback or selected package proof that the postprocess pass runs after scene color.
- [x] Keep Vulkan support host/toolchain-gated and use precompiled SPIR-V artifacts only.

### Task 4: SDL3 Presentation And Sample Wiring

**Files:**
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/CMakeLists.txt`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`

- [x] Wire postprocess/frame-graph v0 only inside host-owned presentation setup.
- [x] Emit stable backend-neutral smoke status fields such as `framegraph_passes` and `postprocess_status`.
- [x] Preserve Null fallback and Lit Material v0 counters.

### Task 5: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/rhi.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`

- [x] Mark Frame Graph/Postprocess v0 implemented only after validation proves it.
- [x] Keep shadows, depth, HDR, bloom, TAA, GPU markers, and full render-graph scheduling as follow-up work.

### Task 6: Verification

- [x] Run focused renderer/RHI/runtime-host tests through the active CMake presets.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run selected package validation for `sample_desktop_runtime_game`.
- [x] Run strict Vulkan selected package validation when Vulkan runtime/toolchain is ready.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- Focused renderer tests: `cmake --build --preset dev --target mirakana_renderer_tests` plus `ctest --preset dev --output-on-failure -R "mirakana_renderer_tests"` passed.
- Focused runtime-host tests: `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests` plus `ctest --preset desktop-runtime --output-on-failure -R "mirakana_runtime_host_sdl3_tests"` passed after the known sandbox vcpkg `CreateFileW stdin failed with 5` extraction failure was rerun with elevated permissions.
- Focused sample/runtime smoke: `cmake --build --preset desktop-runtime --target sample_desktop_runtime_game mirakana_runtime_host_sdl3_tests` plus `ctest --preset desktop-runtime --output-on-failure -R "mirakana_runtime_host_sdl3_tests|sample_desktop_runtime_game_smoke"` passed after the same sandbox rerun.
- Selected D3D12 package: `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed after the same sandbox rerun; installed smoke reported `renderer=d3d12`, `scene_gpu_status=ready`, `postprocess_status=ready`, and `framegraph_passes=2`.
- Selected Vulkan package: `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireVulkanShaders -SmokeArgs @('--smoke','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-vulkan-scene-shaders','--video-driver','windows','--require-vulkan-renderer','--require-scene-gpu-bindings','--require-postprocess')` passed after the same sandbox rerun; installed smoke reported `renderer=vulkan`, `scene_gpu_status=ready`, `postprocess_status=ready`, and `framegraph_passes=2`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: D3D12 DXIL ready and Vulkan SPIR-V ready; Metal `metal`/`metallib` remain missing as diagnostic-only host/toolchain blockers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: passed after the same sandbox rerun with 9/9 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed during the final slice validation.
