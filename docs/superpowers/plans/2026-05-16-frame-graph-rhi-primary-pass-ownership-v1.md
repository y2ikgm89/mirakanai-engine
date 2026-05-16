# Frame Graph RHI Primary Pass Ownership v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move `RhiFrameRenderer` primary color pass timing into the Frame Graph RHI executor so the simple renderer path participates in production pass ownership without native handle exposure.

**Architecture:** Keep swapchain acquisition/presentation and command-list submission renderer-owned, but change `RhiFrameRenderer::begin_frame()` to prepare frame state only. `draw_sprite()` and `draw_mesh()` validate and queue ordered draw work. `end_frame()` builds a one-pass `FrameGraphV1Desc` and executes the primary pass through `execute_frame_graph_rhi_texture_schedule`, whose callback begins the existing render pass, records queued draw and native overlay work in order, and ends the render pass. This does not implement native transient heap allocation, native aliasing barriers, multi-queue scheduling, package streaming, Metal parity, or broad renderer quality.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, Frame Graph v1 RHI executor, PowerShell 7 validation tools.

---

## Status

**Status:** Completed.

## Official Practice Check

- Context7 Microsoft Direct3D 12 docs (`/websites/learn_microsoft_en-us_windows_win32_direct3d12`) show command lists recording resource barriers before render target use and then recording render commands before returning resources to presentation or later use.
- Direct3D 12 command-list examples keep render work ordered inside the command list and rely on fences for submitted work completion. This slice keeps those constraints backend-neutral by preserving one graphics command list, one primary render pass, and the existing optional wait.
- No new SDK or backend API is introduced; native D3D12/Vulkan/Metal handles stay behind `MK_rhi_*`.

## Context

`RhiPostprocessFrameRenderer`, `RhiDirectionalShadowSmokeFrameRenderer`, and `RhiViewportSurface` already route their completed frame-graph texture scheduling slices through `execute_frame_graph_rhi_texture_schedule`. `RhiFrameRenderer` is now the smallest remaining primary renderer path whose pass still starts directly in `begin_frame()`. Moving it to an executor pass callback is a focused production pass ownership step and avoids the broader native transient allocation or multi-queue design space.

## Constraints

- Preserve the public `IRenderer` / `RhiFrameRenderer` API.
- Preserve validation timing for `draw_mesh()` GPU binding ownership and incompatible gpu skinning/morphing requests.
- Preserve visible draw order for sprite, mesh, skinned mesh, morph mesh, and native UI overlay submissions.
- Preserve failure cleanup so a failed submission leaves `frames_finished == 0`, clears active frame state, and allows a later frame to begin and finish with the same swapchain.
- Do not expose native handles, command lists, or frame graph internals to game/editor callers.
- Do not add compatibility paths that keep the primary pass recording directly in `begin_frame()`.
- Do not claim native transient allocation, alias execution, multi-queue scheduling, renderer-wide graph ownership, package streaming, or Metal readiness.
- Before completion, run an agent-surface drift check and update docs, manifest fragments, static guards, skills, or subagents if durable guidance changed.

## Done When

- `RhiFrameRenderer::begin_frame()` no longer begins a render pass or binds the primary pipeline directly.
- `RhiFrameRenderer::end_frame()` invokes `execute_frame_graph_rhi_texture_schedule` for a scheduled `primary_color` pass and increments `RendererStats::framegraph_passes_executed` from the executor result.
- Existing raw `RhiFrameRenderer` sprite/mesh/native-overlay tests still report the same RHI render-pass and draw stats, with `framegraph_passes_executed == 1` for completed frames.
- Failure-path tests prove submit failure still clears active frame state, does not finish the frame, and does not report a completed frame graph pass before a later retry succeeds.
- Docs, manifest fragments, registry pointers, and agent/static guards distinguish this narrow primary pass ownership slice from native transient allocation, alias execution, and multi-queue scheduling.
- Focused tests, static checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete host blocker.

## Tasks

### Task 1: RED tests and static guard

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`

- [x] Update raw `RhiFrameRenderer` texture-frame and native-overlay tests to require `renderer.stats().framegraph_passes_executed == 1` after one completed frame.
- [x] Add or update a failure-path assertion proving a submit failure still leaves `frames_finished == 0`, clears active frame state, and does not report a completed frame graph pass before a later retry succeeds.
- [x] Add a static guard that requires `rhi_frame_renderer.cpp` to contain `execute_frame_graph_rhi_texture_schedule`, `primary_color`, and `framegraph_passes_executed`, and rejects `begin_render_pass` in `RhiFrameRenderer::begin_frame`.
- [x] Run:

`pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_renderer_tests"'`

Expected: fail on the new `framegraph_passes_executed == 1` expectations before implementation.

### Task 2: Queue primary pass work and execute through the frame graph

**Files:**

- Modify: `engine/renderer/include/mirakana/renderer/rhi_frame_renderer.hpp`
- Modify: `engine/renderer/src/rhi_frame_renderer.cpp`

- [x] Add a small private queued-command representation for raw sprite draws and mesh draws, keeping full `MeshCommand` rows where needed for validated GPU binding data.
- [x] Move primary render-pass construction into a private helper used from `end_frame()`.
- [x] Change `begin_frame()` to acquire the swapchain frame, begin the graphics command list, reset queued work, and mark the frame active without beginning a render pass or binding the pipeline.
- [x] Change non-overlay `draw_sprite()` to queue a draw command and keep existing stats increments.
- [x] Change `draw_mesh()` to validate exactly as it does today, then queue the draw command and keep existing stats increments, GPU skinning/morph counters, and descriptor-bind counters aligned with actual callback recording.
- [x] In `end_frame()`, prepare native overlay data before the executor callback as today, build a one-pass schedule for `primary_color`, and invoke `execute_frame_graph_rhi_texture_schedule` with one callback that begins the render pass, binds the primary pipeline, records queued work in submission order, records overlay draw, and ends the render pass.
- [x] After executor success, add `frame_graph_execution.pass_callbacks_invoked` to `stats_.framegraph_passes_executed`.
- [x] Preserve present/close/submit/wait cleanup and failure cleanup semantics.
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
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json` only if generated-game guidance needs the new primary-pass note
- Modify: `engine/agent/manifest.json` via compose only
- Modify: `.agents/skills/rendering-change/references/full-guidance.md` and `.claude/skills/gameengine-rendering/references/full-guidance.md` if durable rendering guidance needs a primary-pass note
- Modify: `.codex/agents/rendering-auditor.toml` and `.claude/agents/rendering-auditor.md` if reviewer expectations need a primary-pass note

- [x] State that `RhiFrameRenderer` primary pass timing is executor-owned, while swapchain acquire/present, command-list submission, pass body draw work, native transient allocation, alias execution, and multi-queue scheduling remain out of scope.
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

- RED: focused `MK_renderer_tests` failed before implementation on the new raw `RhiFrameRenderer` `framegraph_passes_executed == 1` assertions.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before implementation because `rhi_frame_renderer.cpp` did not yet route through `execute_frame_graph_rhi_texture_schedule`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_renderer_tests"'` passed after the queue/callback implementation.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after the primary-pass static guard and implementation landed.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` regenerated `engine/agent/manifest.json` after manifest fragment updates.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` completed with `format: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` completed with `format-check: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` completed with `json-contract-check: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` completed with `agent-config-check: ok` after keeping Codex/Claude rendering-auditor subagents under the initial-load budget.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` completed with `ai-integration-check: ok` after the `RhiFrameRenderer::begin_frame` guard was tightened to catch any `begin_render_pass` call in the function body.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_d3d12_rhi_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_d3d12_rhi_tests"'` passed after updating D3D12 failure-path expectations for executor-owned primary pass timing.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed with `validate: ok` and 65/65 CTest tests passing. Metal and Apple host lanes remained diagnostic/host-gated on Windows as expected.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` completed successfully after full validation.
