# Frame Graph Viewport Surface Color State Executor v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Route `RhiViewportSurface` color-target state changes through the Frame Graph RHI texture executor so high-level renderer sources no longer call `IRhiCommandList::transition_texture` directly outside `frame_graph_rhi.cpp`.

**Architecture:** Keep `RhiViewportSurface` as a renderer-owned offscreen viewport target, but make its color-state transitions use `execute_frame_graph_rhi_texture_schedule` with a single imported `viewport_color` binding and a declared final state. This is a narrow manual-transition-removal slice only; it does not claim production render graph ownership, native transient heap allocation, alias execution, multi-queue scheduling, or native handle exposure.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, Frame Graph v1 RHI executor, PowerShell 7 validation tools.

---

## Status

**Status:** Completed.

## Context

`RhiPostprocessFrameRenderer` and `RhiDirectionalShadowSmokeFrameRenderer` already route scheduled inter-pass, pass target-state, and final-state texture transitions through `execute_frame_graph_rhi_texture_schedule`. A repository scan after `Frame Graph Shadow Scratch Color Target-State Ownership v1` shows the remaining high-level renderer direct `transition_texture(` calls live in `engine/renderer/src/rhi_viewport_surface.cpp`; `engine/renderer/src/frame_graph_rhi.cpp` remains the allowed low-level executor implementation.

## Constraints

- Preserve the existing `RhiViewportSurface` public API and renderer-owned native resource boundary.
- Do not expose `IRhiDevice`, native textures, command lists, or frame graph internals to game/editor callers.
- Do not add a compatibility path that keeps direct viewport-surface `transition_texture` calls.
- Keep this focused on color target state transitions. Native transient allocation, aliasing barriers, multi-queue scheduling, renderer-wide graph ownership, package streaming, and Metal readiness remain unsupported.
- Before completion, run an agent-surface drift check and update docs, manifest fragments, static guards, skills, or subagents if durable guidance changed.

## Done When

- `RhiViewportSurface` uses the Frame Graph RHI executor for color-state transitions to `render_target`, `copy_source`, and `shader_read`.
- `rg -n "transition_texture\(" engine/renderer/src -g "*.cpp"` only reports executor-owned calls in `engine/renderer/src/frame_graph_rhi.cpp`.
- Unit tests cover an executor-reported viewport color transition failure and the existing viewport render/readback/display paths still pass.
- Static checks require the viewport surface to use the executor and reject direct high-level renderer transition calls.
- Docs, manifest fragments, registry pointers, and agent surfaces reflect the narrowed manual-transition-removal slice.
- Focused tests, package/static checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete host blocker.

## Tasks

### Task 1: RED tests and static guard

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`

- [x] Add a failing unit test named `rhi viewport surface reports frame graph color transition failure` using `ThrowingTransitionRhiDevice` with `throw_on_submit = false` and `throw_on_transition = 1`. The test should construct `RhiViewportSurface`, call `render_clear_frame()`, and require a `std::runtime_error` message of `rhi viewport surface frame graph color state execution failed` plus `frames_rendered() == 0`.
- [x] Add a static guard that asserts `engine/renderer/src/rhi_viewport_surface.cpp` contains `execute_frame_graph_rhi_texture_schedule` and does not contain `transition_texture(`.
- [x] Run:

`pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_renderer_tests"'`

Expected: fail only on the new viewport surface executor error-message expectation before implementation.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.

Expected: fail on the new viewport surface direct-transition guard before implementation.

### Task 2: Route viewport color transitions through the executor

**Files:**

- Modify: `engine/renderer/src/rhi_viewport_surface.cpp`

- [x] Include `mirakana/renderer/frame_graph_rhi.hpp`.
- [x] Add a small file-local helper that builds one `FrameGraphTextureBinding{.resource = "viewport_color", .texture = color_texture, .current_state = current_state}` and one `FrameGraphTextureFinalState{.resource = "viewport_color", .state = target_state}`, calls `execute_frame_graph_rhi_texture_schedule`, and returns the updated binding state or throws `std::runtime_error("rhi viewport surface frame graph color state execution failed")` when execution diagnostics are present.
- [x] Replace the direct transition in `render_clear_frame()` before the render pass with the helper and adopt the updated `color_state_` only after executor success.
- [x] Replace the direct render-target-to-copy-source transition in `render_clear_frame()` with the helper.
- [x] Replace `transition_color_state()` internals with the helper while preserving the early return when `color_state_ == state`, command submission, optional wait, and state adoption after executor success.
- [x] Run the focused renderer command from Task 1.

Expected: `MK_renderer_tests` passes.

### Task 3: Docs, manifest, and agent-surface drift

**Files:**

- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.json` via compose only
- Modify: `.agents/skills/rendering-change/references/full-guidance.md` and `.claude/skills/gameengine-rendering/references/full-guidance.md` only if the durable rendering guidance needs a viewport-surface note
- Modify: `.codex/agents/rendering-auditor.toml` and `.claude/agents/rendering-auditor.md` only if reviewer expectations need a viewport-surface note

- [x] State that `RhiViewportSurface` color transitions are executor-owned while viewport render/readback/display ownership remains renderer-owned.
- [x] Keep `frame-graph-v1` in `unsupportedProductionGaps` with `status = implemented-foundation-only`.
- [x] Point `currentActivePlan` at this plan during execution and return it to the master plan on completion.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 4: Validation and publication

**Files:**

- All touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED focused renderer test: `rhi viewport surface reports frame graph color transition failure` failed before implementation.
- RED regression tests after review: final-transition failure recovery and submit-failure state sync failed before adopting recorded states immediately.
- GREEN focused renderer test command passed:
  `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_renderer_tests"'`
- Static drift guard passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- Source scan passed: `rg -n "transition_texture\(" engine/renderer/src -g "*.cpp"` reports only `engine/renderer/src/frame_graph_rhi.cpp`.
- Slice gates passed: `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1`.
- Full gate passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`; Apple/Metal diagnostics remain host-gated as expected on this Windows host.
- Standalone build passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
