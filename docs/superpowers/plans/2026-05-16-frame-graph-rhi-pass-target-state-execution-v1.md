# Frame Graph RHI Pass Target State Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move Frame Graph RHI texture execution from inter-pass/final-state barriers only to also owning declared per-pass writer texture preparation before pass callbacks run.

**Architecture:** Keep pass bodies renderer-owned and native handles backend-private. Extend `MK_renderer` frame graph RHI execution with a small value contract that maps a pass/resource pair to the RHI state required before that pass callback. The executor validates the pass exists, the resource has a texture binding, the desired state is concrete, and the state is consistent with simulated inter-pass barriers, then records the needed transition immediately before invoking the pass callback.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi` public value contracts, `MK_renderer_tests`, PowerShell validation wrappers.

---

## Status

**Plan ID:** `frame-graph-rhi-pass-target-state-execution-v1`
**Status:** Completed.

## Official Practice Check

- Microsoft Direct3D 12 documentation states that applications manage per-resource states explicitly with `ID3D12GraphicsCommandList::ResourceBarrier`, including transitions such as present to render target and render target to shader resource. This slice keeps that explicit ownership in the backend-neutral frame graph executor and does not add native D3D12 structs or handles to renderer public APIs.
- Khronos Vulkan synchronization examples show image memory barriers for color/depth attachment writes becoming shader-readable layouts between passes. This slice keeps the same explicit pass-to-pass state model at the `MK_rhi::ResourceState` level; Vulkan image layouts, queue-family ownership, and native synchronization details remain backend-private.

Sources:

- https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
- https://learn.microsoft.com/en-us/windows/win32/direct3d12/creating-a-basic-direct3d-12-component
- https://github.com/khronosgroup/vulkan-docs/wiki/Synchronization-Examples
- https://github.com/khronosgroup/vulkan-docs/wiki/Synchronization-Examples-(Legacy-synchronization-APIs)

## Context

`frame-graph-v1` already has deterministic v1 compile/schedule, RHI texture barrier execution, pass callbacks, final-state restoration, package-visible executor evidence, and conservative transient texture alias planning. The current renderer migrations still perform some renderer-owned texture preparation around frame boundaries; this slice moves the scheduled postprocess work texture and directional shadow/receiver target-state preparation that belongs immediately before frame graph pass callbacks.

This slice attacks the next smallest production pass-ownership boundary: executor-owned pre-pass texture state preparation for declared writer resources. It does not allocate resources, execute native aliasing, schedule queues, infer native render passes, or move mesh/UI draw bodies out of the current renderers.

## Constraints

- Do not expose native handles, heaps, command queues, backend barrier structs, or platform APIs.
- Do not allocate textures, alias memory, execute native aliasing barriers, or change package streaming/upload ownership.
- Pass target state rows must be explicit; no implicit state inference from pass callback names.
- The executor must validate all pass target rows before recording any new command.
- Keep `frame-graph-v1` `status` as `implemented-foundation-only`.
- No compatibility shims or duplicate public APIs.

## Done When

- `frame_graph_rhi.hpp` exposes a small pass target state value contract.
- `execute_frame_graph_rhi_texture_schedule` records pass target state transitions immediately before pass callbacks and updates caller-owned texture binding states.
- `RhiPostprocessFrameRenderer` and `RhiDirectionalShadowSmokeFrameRenderer` remove the matching in-callback writer-preparation texture transitions for scheduled frame graph resources while keeping first-use scene writer setup and pass bodies renderer-owned.
- `MK_renderer_tests` cover executor pass target state success, duplicate/missing/invalid rows, and renderer transition-count evidence.
- Docs, plan registry, master plan pointer text, manifest fragment/composed manifest, and static guards describe the narrow evidence and remaining non-goals.
- Focused tests, agent/static checks, full `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED tests for pass target state execution

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Add tests that call the intended pass target state API:
  - target state transition is recorded before the matching pass callback and updates the binding state
  - duplicate pass/resource target rows are rejected before command recording
  - target rows for missing pass/resource or `undefined` state are rejected before command recording
- [x] Add renderer evidence tests that expect the postprocess chain, directional shadow-depth, and directional receiver writer-preparation transitions to be executor-owned.
- [x] Run the focused renderer build.

Expected: compile fails because the new types/fields do not exist yet, or renderer transition-count expectations fail before migration.

### Task 2: Implement executor-owned pass target states

**Files:**

- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`

- [x] Add `FrameGraphTexturePassTargetState` with `pass_name`, `resource`, and concrete `rhi::ResourceState`.
- [x] Add the pass target state span to `FrameGraphRhiTextureExecutionDesc`.
- [x] Validate duplicate rows, empty names, missing scheduled pass, missing texture binding, and `undefined` target states before recording commands.
- [x] Simulate inter-pass barriers and pass target transitions so state mismatches are diagnosed before recording partial work.
- [x] Record required target transitions immediately before pass callback invocation and update `FrameGraphTextureBinding::current_state`.

### Task 3: Migrate renderer writer target preparation

**Files:**

- Modify: `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`
- Modify: `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`

- [x] Provide pass target state rows for scheduled postprocess/shadow intermediate writer resources and receiver attachments.
- [x] Remove matching manual `transition_texture` calls from the migrated pass callbacks.
- [x] Keep first-use scene writer setup, swapchain acquire/present, native UI overlay preparation, mesh draw bodies, and final reusable states outside the new scope.

### Task 4: GREEN focused renderer tests

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Run the focused renderer build.
- [x] Run `ctest --preset dev --output-on-failure -R MK_renderer_tests`.
- [x] Refactor only if tests are green.

### Task 5: Agent surface and documentation sync

**Files:**

- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/rhi.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.json` via compose only
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1` if rendering needles need the new API/evidence text
- Modify: agent skills/rules/subagents only if durable guidance changed

- [x] Keep `frame-graph-v1` open and foundation-only.
- [x] State that this is executor-owned pass target preparation for scheduled frame graph resources, not native render-pass inference, first-use scene writer setup ownership, native transient heap allocation, native alias execution, multi-queue scheduling, package streaming, Metal readiness, or renderer-wide manual-transition removal.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 6: Slice validation and publication

**Files:**

- All touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests'` failed before implementation because `FrameGraphTexturePassTargetState` / `.pass_target_states` did not exist.
- PASS: focused renderer build `MK_renderer_tests`.
- PASS: `ctest --preset dev --output-on-failure -R "MK_(renderer|runtime_host_sdl3)_tests"`: 2/2 tests passed.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`; installed D3D12 smoke proved `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_barrier_steps_executed=14`, `renderer_quality_expected_framegraph_barrier_steps=14`, and `renderer_quality_framegraph_barrier_steps_ok=1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`; full validation passed with 65/65 CTest tests.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- REVIEW: read-only rendering-auditor subagent found no blockers. Non-blocking feedback to add source-level static guards and clarify the active plan scope was applied before final validation.
