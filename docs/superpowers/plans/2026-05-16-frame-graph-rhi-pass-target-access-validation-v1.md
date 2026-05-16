# Frame Graph RHI Pass Target Access Validation v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make frame graph RHI pass target-state rows fail closed unless they match a declared writer access for the same pass/resource pair.

**Architecture:** Keep the executor backend-neutral and continue using first-party `FrameGraphAccess` plus `rhi::ResourceState` values rather than native D3D12 or Vulkan barrier structs. Add a small pass target access contract derived from `FrameGraphV1Desc::passes[*].writes`, pass it into `FrameGraphRhiTextureExecutionDesc`, and validate every `FrameGraphTexturePassTargetState` against that contract before command recording.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi` public value contracts, `MK_renderer_tests`, PowerShell validation wrappers.

---

## Status

**Plan ID:** `frame-graph-rhi-pass-target-access-validation-v1`
**Status:** Completed.

## Official Practice Check

- Microsoft Direct3D 12 documentation models render target, depth write, shader resource, copy, and present usage as explicit resource states transitioned through command-list barriers before the operation that needs the state. This slice keeps that state/access relationship explicit in the backend-neutral frame graph contract.
- Khronos Vulkan synchronization examples model color/depth attachment writes and shader reads as explicit access/layout transitions between passes. This slice validates that a pass target-state row corresponds to a declared writer access before native backends ever see barriers.

Sources:

- https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
- https://learn.microsoft.com/en-us/windows/win32/direct3d12/cd3dx12-resource-barrier
- https://github.com/khronosgroup/vulkan-docs/wiki/Synchronization-Examples
- https://github.com/khronosgroup/vulkan-docs/wiki/Synchronization-Examples-(Legacy-synchronization-APIs)

## Context

Frame Graph RHI Pass Target State Execution v1 added `FrameGraphTexturePassTargetState` and executor-owned target-state transitions before pass callbacks. The first implementation validates scheduled pass names, texture bindings, duplicate rows, and concrete states, but it intentionally does not prove that the row is backed by the pass' declared writer access. This slice closes that contract gap without claiming native render pass inference, renderer-wide pass migration, native transient heap allocation, native alias execution, or multi-queue scheduling.

## Constraints

- Do not expose native handles, backend barrier structs, queue ownership, image layouts, heaps, or platform APIs.
- Keep pass bodies renderer-owned.
- Do not infer target rows from callback names.
- Validate all pass target access rows and all pass target state rows before recording commands.
- Keep `frame-graph-v1` `status` as `implemented-foundation-only`.
- No compatibility shims or duplicate public APIs.

## Done When

- `frame_graph_rhi.hpp` exposes a small pass target access value contract and a helper that derives writer target accesses from `FrameGraphV1Desc`.
- `execute_frame_graph_rhi_texture_schedule` rejects pass target states that are missing a declared writer access, duplicate target access rows, or disagree with `frame_graph_texture_state_for_access`.
- `RhiPostprocessFrameRenderer` and `RhiDirectionalShadowSmokeFrameRenderer` pass derived writer target access rows into the executor.
- `MK_renderer_tests` cover helper output, read-only mismatch rejection, access/state mismatch rejection, duplicate access rejection, and existing postprocess/directional shadow execution.
- Docs, plan registry, manifest fragments/composed manifest, agent guidance, and static guards describe the access validation boundary and remaining non-goals.
- Focused tests, agent/static checks, full `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED tests for pass target access validation

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [ ] Add a helper test that expects a new public helper to produce writer target access rows from a `FrameGraphV1Desc` containing color, depth, and shader-read rows.
- [ ] Add executor tests that call the intended API and fail before implementation:
  - pass target state on a pass/resource pair without a declared writer access is rejected before recording
  - pass target state whose `rhi::ResourceState` disagrees with the declared `FrameGraphAccess` is rejected before recording
  - duplicate pass/resource target access rows are rejected before recording
- [ ] Run the focused renderer build and record the expected compile/test failure.

### Task 2: Implement pass target access contracts

**Files:**

- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`

- [ ] Add `FrameGraphTexturePassTargetAccess` with `pass_name`, `resource`, and `FrameGraphAccess access`.
- [ ] Add `std::span<const FrameGraphTexturePassTargetAccess> pass_target_accesses` to `FrameGraphRhiTextureExecutionDesc`.
- [ ] Add `build_frame_graph_texture_pass_target_accesses(const FrameGraphV1Desc& desc)` that returns one row for each pass write whose `FrameGraphAccess` maps to a concrete `rhi::ResourceState`.
- [ ] Validate duplicate target access rows and reject `unknown` or unmapped access rows before recording commands.
- [ ] Require every `FrameGraphTexturePassTargetState` to match one target access row and require `state == frame_graph_texture_state_for_access(access)`.

### Task 3: Wire renderers through declared writer access rows

**Files:**

- Modify: `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`
- Modify: `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`

- [ ] Derive `pass_target_accesses` from the same `FrameGraphV1Desc` used to compile each renderer's frame graph.
- [ ] Pass `.pass_target_accesses = pass_target_accesses` into `execute_frame_graph_rhi_texture_schedule`.
- [ ] Keep first-use scene writer setup, swapchain acquire/present, native UI overlay preparation, mesh draw bodies, and final reusable states outside the new scope.

### Task 4: GREEN focused tests

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [ ] Run the focused renderer build.
- [ ] Run `ctest --preset dev --output-on-failure -R MK_renderer_tests`.
- [ ] Refactor only if tests are green.

### Task 5: Agent surface and documentation sync

**Files:**

- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/rhi.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.json` via compose only
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1` if rendering needles need the new API/evidence text
- Modify: agent skills/rules/subagents only if durable guidance changed

- [ ] Keep `frame-graph-v1` open and foundation-only.
- [ ] State that this is pass target access validation only, not native render-pass inference, first-use scene writer setup ownership, native transient heap allocation, native alias execution, multi-queue scheduling, package streaming, Metal readiness, or renderer-wide manual-transition removal.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 6: Slice validation and publication

**Files:**

- All touched files.

- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED build:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests'`
  - Failed as expected because `FrameGraphTexturePassTargetAccess`, `build_frame_graph_texture_pass_target_accesses`, and `.pass_target_accesses` were not implemented yet.
- Focused renderer build:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests'`
  - Passed.
- Focused renderer/runtime tests:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_(renderer|runtime_host_sdl3)_tests"'`
  - Passed 2/2.
- Package desktop runtime smoke:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`
  - Passed, including `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_barrier_steps_executed=14`, and `renderer_quality_expected_framegraph_barrier_steps=14`.
- Agent/static checks:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - Passed.
- Full validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  - Passed with 65/65 CTest tests. Diagnostic-only host gates remained Apple/macOS/Xcode and Metal toolchain availability on this Windows host.
- Standalone build:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`
  - Passed.
