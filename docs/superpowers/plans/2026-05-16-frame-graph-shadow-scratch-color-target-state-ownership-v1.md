# Frame Graph Shadow Scratch Color Target-State Ownership v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the `RhiDirectionalShadowSmokeFrameRenderer` `shadow_color` scratch render-target first-use state preparation under `execute_frame_graph_rhi_texture_schedule`.

**Architecture:** Keep directional-shadow render-pass bodies renderer-owned, but declare `shadow_color` as a frame graph writer and let the RHI texture executor prepare its `render_target` state before the `shadow_depth` callback records the shadow map pass. This keeps the clean-break rule that scheduled pass target states are executor-owned while avoiding native render-pass inference or transient heap execution.

**Tech Stack:** C++23, `MK_renderer`, `MK_runtime_host_sdl3`, `MK_renderer_tests`, desktop runtime package smoke, PowerShell validation wrappers.

---

## Status

**Plan ID:** `frame-graph-shadow-scratch-color-target-state-ownership-v1`
**Status:** Completed.

## Official Practice Check

- Direct3D 12 keeps per-resource state management in the application and requires render-target resources to be in the correct state before render-target use. This slice moves the remaining scheduled scratch color target state preparation into the backend-neutral executor instead of relying on renderer-side first-use setup.
- Vulkan synchronization examples model attachment writes and later reads through explicit access/layout synchronization. This slice only changes where the engine records the target-state transition; it does not expose Vulkan layouts or native barriers to gameplay.

Sources:

- https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
- https://github.com/khronosgroup/vulkan-docs/wiki/Synchronization-Examples

## Context

`frame-graph-postprocess-scene-pass-ownership-v1` moved postprocess scene pass command recording into the executor `scene_color` callback and proved postprocess-depth barrier budgets after the scene target preparation became executor-owned. The directional-shadow smoke path still prepares `shadow_color` directly in `begin_frame()` before the frame graph schedule records the `shadow_depth`, `scene_color`, and `postprocess` callbacks.

## Constraints

- Do not migrate directional-shadow render-pass bodies into a new renderer-wide graph abstraction.
- Do not infer native render passes, allocate native transient heaps, execute native aliasing barriers, add multi-queue scheduling, add package streaming, expose native handles, claim Metal readiness, or broaden renderer quality claims.
- Do not change public gameplay/runtime-host APIs except existing package quality counter expectations.
- Preserve `FrameGraphV1Desc` as the source of pass/resource/write access truth.
- Keep `frame-graph-v1` status as `implemented-foundation-only`.

## Done When

- `RhiDirectionalShadowSmokeFrameRenderer::begin_frame()` no longer transitions `shadow_color` to `render_target`.
- The shadow smoke `FrameGraphV1Desc` declares `shadow_color` as a transient or imported scratch resource written by the `shadow_depth` pass.
- `end_frame()` supplies a writer-access-backed `FrameGraphTexturePassTargetState` for `shadow_color -> render_target`.
- Directional-shadow barrier counters prove `shadow_color` target preparation is executor-owned without changing visible shadow output expectations.
- Failure tests prove executor-owned `shadow_color` target preparation failures are reported from `end_frame()` and release acquired swapchain frames.
- Docs, plan registry, manifest fragments/composed manifest, skills/subagents if needed, and static guards reflect the new ownership boundary and remaining non-goals.
- Focused renderer/runtime tests, package smoke, agent/static checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED tests for shadow scratch color target-state ownership

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`

- [x] Update directional-shadow tests so two frames expect one additional executor-recorded barrier for `shadow_color` target preparation.
- [x] Add or update a failure test so a `shadow_color` target preparation transition exception is reported from `end_frame()`, leaves `frames_finished == 0`, and releases the acquired swapchain frame.
- [x] Update package quality expectations for directional shadow from the previous two-frame `14` barrier budget to the new executor-owned scratch color target-state budget.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_(renderer|runtime_host_sdl3)_tests"'` and record the expected failure before implementation.

### Task 2: Move shadow scratch color target preparation into the executor

**Files:**

- Modify: `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`

- [x] Add the `shadow_color` scratch texture to the directional-shadow frame graph resource declarations with writer access owned by the `shadow_depth` pass.
- [x] Remove the direct `begin_frame()` transition from `shadow_color_state_` to `render_target`.
- [x] Add a `FrameGraphTexturePassTargetState` row for `shadow_color -> render_target`, backed by the generated pass target access rows.
- [x] Keep shadow-depth, scene receiver, postprocess, native UI overlay, swapchain acquire/present, reusable final states, and native resource ownership inside their existing scope.

### Task 3: GREEN focused validation

**Files:**

- Modified files from Tasks 1 and 2.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_(renderer|runtime_host_sdl3)_tests"'`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`.
- [x] Refactor only after the focused tests and package smoke are green.

### Task 4: Agent surface and documentation sync

**Files:**

- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/rhi.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.json` via compose only
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1` if manifest needles change
- Modify: `.agents/skills/rendering-change/references/full-guidance.md`, `.claude/skills/gameengine-rendering/references/full-guidance.md`, `.codex/agents/rendering-auditor.toml`, and `.claude/agents/rendering-auditor.md` only if the durable guidance changes

- [x] State that directional shadow `shadow_color` target-state preparation is executor-owned, while render-pass body migration, native render-pass inference, native aliasing, multi-queue scheduling, package streaming, Metal readiness, and broad renderer quality remain unsupported.
- [x] Keep `frame-graph-v1` in `unsupportedProductionGaps` with `status = implemented-foundation-only`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 5: Slice validation and publication

**Files:**

- All touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests MK_runtime_host_sdl3_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_(renderer|runtime_host_sdl3)_tests"'` built, then failed as expected before implementation: directional-shadow renderer counters still reported `8` instead of `9`, the executor-failure test failed because `begin_frame()` still owned the `shadow_color` transition, and runtime-host quality gates still expected the old `14` budget.
- GREEN: the same focused build/ctest command passed after moving `shadow_color` target preparation into `execute_frame_graph_rhi_texture_schedule` and updating package quality formulas.
- Review RED: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_renderer_tests"'` failed on the new submit-failure regression before recovery was complete, first exposing the abandoned-present cleanup mismatch and then the second-frame stale texture-state mismatch.
- Review GREEN: the same focused renderer command passed after submit-success-only state adoption plus lazy internal texture/descriptor recreation for unsubmitted failed recordings.
- Focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests MK_runtime_host_sdl3_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_(renderer|runtime_host_sdl3)_tests"'` passed after the review fix.
- Package smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed with installed D3D12 output including `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_barrier_steps_executed=15`, `renderer_quality_expected_framegraph_barrier_steps=15`, and `renderer_quality_framegraph_barrier_steps_ok=1`.
- Static/agent: `tools/format.ps1`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed after updating the rendering skill/subagent surfaces and strengthening the shadow-color writer-access/target-state static guard.
- Slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed 65/65 tests; Metal and Apple evidence remained host-gated diagnostic-only on this Windows host.
- Build gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after `validate.ps1`.
- Publication: committed and pushed `codex/frame-graph-shadow-scratch-color-target-state-ownership`, then opened PR [#64](https://github.com/y2ikgm89/mirakanai-engine/pull/64).
