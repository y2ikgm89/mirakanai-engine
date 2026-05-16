# Frame Graph Postprocess Scene Pass Ownership v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move `RhiPostprocessFrameRenderer` scene pass command recording under `execute_frame_graph_rhi_texture_schedule` without broad renderer-wide migration.

**Architecture:** Keep the pass body renderer-owned, but make the frame graph executor own the scene pass callback timing and target-state preparation for `scene_color` and optional `scene_depth`. `draw_mesh()` should validate and queue mesh commands while `end_frame()` records the scene render pass from the executor's `scene_color` callback.

**Tech Stack:** C++23, `MK_renderer`, `MK_runtime_host_sdl3`, `MK_renderer_tests`, desktop runtime package smoke, PowerShell validation wrappers.

---

## Status

**Plan ID:** `frame-graph-postprocess-scene-pass-ownership-v1`
**Status:** Completed.

## Official Practice Check

- Direct3D 12 keeps resource states explicit and requires target resources to be transitioned before render-pass work. This slice keeps the transition and pass callback order explicit in the backend-neutral frame graph executor.
- Vulkan synchronization examples model color/depth attachment writes and shader reads as explicit pass-to-pass synchronization. This slice moves one postprocess renderer pass body into that explicit schedule without exposing native layouts or barriers.

Sources:

- https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
- https://github.com/khronosgroup/vulkan-docs/wiki/Synchronization-Examples

## Context

`FrameGraphTexturePassTargetState` and `FrameGraphTexturePassTargetAccess` now let the executor prepare writer target states only when they match declared `FrameGraphV1Desc::passes[*].writes` rows. `RhiPostprocessFrameRenderer` still records its scene render pass during `begin_frame()` / `draw_mesh()` before the executor dispatches the `scene_color` pass callback. This slice moves only the postprocess renderer's scene pass body into the existing executor callback sequence.

## Constraints

- Do not migrate `RhiDirectionalShadowSmokeFrameRenderer` in this slice.
- Do not add native render-pass inference, native transient heap allocation, alias execution, multi-queue scheduling, package streaming, Metal readiness, public native handles, or renderer-wide manual-transition removal.
- Do not change public gameplay/runtime-host APIs except existing package quality counter expectations.
- Preserve `FrameGraphV1Desc` as the source of pass/resource/write access truth.
- Keep `frame-graph-v1` status as `implemented-foundation-only`.

## Done When

- `RhiPostprocessFrameRenderer::begin_frame()` no longer begins the scene render pass or performs first-use scene-color/depth writer transitions directly.
- `draw_mesh()` validates commands and appends them to a pending scene command queue.
- `end_frame()` registers a `scene_color` callback that begins the scene render pass, records queued mesh draws, ends the scene render pass, and then lets downstream postprocess callbacks run.
- Postprocess no-depth, two-stage, and depth-input barrier counters prove scene pass target-state preparation is executor-owned.
- End-frame failure tests prove pre-present swapchain frames are released when executor-owned scene pass target-state preparation fails.
- Docs, plan registry, manifest fragments/composed manifest, skills/subagents if needed, and static guards reflect the new ownership boundary and remaining non-goals.
- Focused renderer/runtime tests, package smoke, agent/static checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED tests for postprocess scene pass ownership

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`

- [x] In `rhi postprocess frame renderer records scene color and postprocess passes`, update the expected one-frame no-depth `framegraph_barrier_steps_executed` from `1` to `2`.
- [x] In `rhi postprocess frame renderer two-stage chain uses three frame graph passes and two postprocess draws`, update the one-frame two-stage expected barrier steps from `3` to `4`.
- [x] In `rhi postprocess frame renderer can bind scene depth as a postprocess input`, update the depth expected barrier formula to `1 + (frames * 4)` for positive frame counts and use a three-frame `13` barrier regression.
- [x] Add or update a failure test so a transition exception during executor-owned scene target preparation is reported from `end_frame()`, leaves `frames_finished == 0`, and releases the acquired swapchain frame.
- [x] Update package quality expectations for postprocess depth from `frames * 3` to `1 + (frames * 4)` and no-depth postprocess from `frames` to `frames * 2` for positive frame counts, then update package sample expected counters that depend on those helpers.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests'` and record the expected failure before implementation.

### Task 2: Move postprocess scene pass recording into the executor callback

**Files:**

- Modify: `engine/renderer/include/mirakana/renderer/rhi_postprocess_frame_renderer.hpp`
- Modify: `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`

- [x] Add a private `std::vector<MeshCommand> pending_meshes_` member and clear it at frame boundaries.
- [x] Change `begin_frame()` so it acquires the swapchain frame, begins the command list, and prepares descriptors/resources, but does not begin the scene render pass and does not transition `scene_color` or `scene_depth` to writer states directly.
- [x] Change `draw_mesh()` so it calls the existing validation path, increments submission stats, and stores the command for later recording instead of emitting draw commands immediately.
- [x] Add a `scene_color` `FrameGraphPassExecutionBinding` callback in `end_frame()` that begins the scene render pass, records each queued mesh draw through existing mesh recording helpers, and ends the scene render pass.
- [x] Add `FrameGraphTexturePassTargetState` rows for `scene_color -> render_target` and, when depth input is enabled, `scene_depth -> depth_write`; keep the rows backed by `postprocess_frame_graph_target_accesses_`.
- [x] Keep swapchain acquire/present, postprocess pass bodies, native UI overlay preparation, and reusable final states outside the new scope.

### Task 3: GREEN focused validation

**Files:**

- Modified files from Tasks 1 and 2.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests'`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_(renderer|runtime_host_sdl3)_tests"'`.
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

- [x] State that postprocess scene pass ownership is executor-owned, while directional shadow scene pass ownership migration, native render-pass inference, native aliasing, multi-queue scheduling, package streaming, Metal readiness, and broad renderer quality remain unsupported.
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
- [ ] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED: `MK_renderer_tests` built successfully after the expectation-only test edits, then `ctest --preset dev --output-on-failure -R "MK_renderer_tests"` failed as expected on the updated postprocess barrier counters and scene target-preparation failure hook before implementation.
- GREEN focused renderer/runtime loop: `MK_renderer_tests` built and `ctest --preset dev --output-on-failure -R "MK_(renderer|runtime_host_sdl3)_tests"` passed after moving postprocess scene pass recording into the executor callback and updating runtime quality budgets.
- Package smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed with `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_barrier_steps_executed=14`, `renderer_quality_expected_framegraph_barrier_steps=14`, and `renderer_quality_framegraph_barrier_steps_ok=1` on the selected directional-shadow package lane.
- Post-review fix loop: Goodall's final review found stale generated no-depth postprocess budgets, runtime-host zero-frame quality fallbacks, rendering skill guidance, and the untracked next plan. The follow-up patch updated material/cooked scene package budgets, generator template budgets, runtime-host minimum postprocess target-prep budgets, rendering skills, and a committed material-shader tangent-space static guard.
- Focused post-review runtime loop: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_host_sdl3_tests sample_generated_desktop_runtime_cooked_scene_package sample_generated_desktop_runtime_material_shader_package; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_runtime_host_sdl3_tests|sample_generated_desktop_runtime_(cooked_scene|material_shader)_package_smoke"'` passed.
- Material shader package validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package` first exposed the committed sample's stale non-tangent vertex layout, then passed after aligning it to the cooked tangent-space mesh payload. Final smoke reported `renderer=d3d12`, `scene_gpu_status=ready`, `postprocess_status=ready`, `framegraph_passes_executed=4`, and `framegraph_barrier_steps_executed=4`.
- Cooked scene package validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_cooked_scene_package` passed after the no-depth budget cleanup.
- Agent/static validation: `tools/format.ps1`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed after the final agent-surface drift check and static guard updates.
- Final full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `65/65` tests; Metal/Apple diagnostics remain host-gated on this Windows host as expected.
- Final build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed.
