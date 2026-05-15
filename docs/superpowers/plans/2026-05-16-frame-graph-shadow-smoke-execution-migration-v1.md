# Frame Graph Shadow Smoke Execution Migration v1 (2026-05-16)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `frame-graph-shadow-smoke-execution-migration-v1`
**Status:** Completed. Returned `currentActivePlan` to the master plan with `recommendedNextPlan.id=next-production-gap-selection` after slice validation on 2026-05-16.
**Goal:** Route `RhiDirectionalShadowSmokeFrameRenderer` scheduled shadow, scene, and postprocess pass transitions through Frame Graph RHI texture schedule execution without exposing native handles.

**Architecture:** Reuse the existing backend-neutral `execute_frame_graph_rhi_texture_schedule` contract. Keep swapchain acquire/present, first-use writer setup, render-pass bodies, native UI overlay preparation, and final reusable-state resets renderer-owned; use the Frame Graph executor for the declared shadow-depth, scene-color, and scene-depth inter-pass barriers and pass callback ordering. This slice does not implement final-state policy, transient allocation, aliasing, multi-queue scheduling, package streaming, Metal parity, or a production render graph.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, NullRHI unit tests, PowerShell repository validation.

---

## Official Practice Check

- Microsoft Direct3D 12 documents explicit transition barriers before operations that require a different resource state: https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12.
- Khronos Vulkan synchronization guidance documents image layout/access transitions between attachment writes and shader reads: https://docs.vulkan.org/guide/latest/synchronization_examples.html and https://github.khronos.org/Vulkan-Site/spec/latest/chapters/synchronization.html.
- Engine implication: Frame Graph owns backend-neutral pass/resource/access ordering only; native barrier recording stays behind `IRhiCommandList::transition_texture`.

## Context

- `frame-graph-v1` remains the active Phase 1 foundation follow-up after [Frame Graph RHI Texture Pass Access Execution v1](2026-05-16-frame-graph-rhi-texture-pass-access-execution-v1.md).
- The prior child slice made `execute_frame_graph_rhi_texture_schedule` record ordered texture barriers after writer pass callbacks can update caller-owned binding states, and migrated `RhiPostprocessFrameRenderer`.
- `RhiDirectionalShadowSmokeFrameRenderer` already declares a three-pass Frame Graph v1 schedule but still records inter-pass barriers manually and increments framegraph counters from scheduled rows rather than the executor result.

## Constraints

- Do not expose D3D12, Vulkan, Metal, command-list native handles, descriptor handles, query heaps, queues, or fences.
- Do not claim renderer-wide manual-transition removal, final-state policy, transient allocation, aliasing, multi-queue scheduling, package streaming, 2D/3D vertical-slice readiness, or Vulkan/Metal parity from this slice.
- Keep first-use writer transitions and final reusable-state resets outside the graph until a separate final-state policy exists.
- Add or update tests before production code.
- Update docs, manifest fragments, composed manifest, skills/subagents/static checks only for durable behavior or agent-surface drift found in this slice.

## Files

- Modify: `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/rhi_directional_shadow_smoke_frame_renderer.hpp` only if helper declarations become necessary.
- Test: `tests/unit/renderer_rhi_tests.cpp`
- Docs/sync as needed: `docs/superpowers/plans/README.md`, `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/plans/2026-05-11-phase1-foundation-gaps-coherent-slice-order-v1.md`, `docs/rhi.md`, `docs/current-capabilities.md`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, `engine/agent/manifest.json`, and scoped static guards.

## Tasks

### Task 1: RED Shadow Executor Evidence

- [x] Add or update a focused `MK_renderer_tests` case proving an inter-pass shadow renderer transition failure is surfaced through the Frame Graph RHI texture executor's deterministic renderer error, not as a raw manual `IRhiCommandList::transition_texture` exception.
- [x] Run the focused renderer test target and confirm the new expectation fails on the current manual-transition implementation.

### Task 2: Shadow Smoke Executor Migration

- [x] Include `mirakana/renderer/frame_graph_rhi.hpp` in `rhi_directional_shadow_smoke_frame_renderer.cpp`.
- [x] Build `FrameGraphTextureBinding` rows for `shadow_depth`, `scene_color`, and `scene_depth` with the states owned by the renderer at each execution point.
- [x] Build pass callbacks for `shadow.directional.depth`, `scene.shadow_receiver`, and `postprocess`.
- [x] Move the shadow cascade draw/end-pass body, scene receiver render pass body, and postprocess render pass body behind those callbacks while keeping first-use transitions and final resets outside the graph.
- [x] Call `execute_frame_graph_rhi_texture_schedule` with `shadow_smoke_frame_graph_execution_` and throw a deterministic renderer error if execution diagnostics are returned.
- [x] Update renderer-owned texture states from the executor bindings and increment framegraph stats from `barriers_recorded` and `pass_callbacks_invoked`.

### Task 3: Focused Validation

- [x] Run `MK_renderer_tests` focused build/test and confirm the RED expectation turns green.
- [x] Run formatting/static checks that match changed files.

### Task 4: Agent Surface Drift And Slice Close

- [x] Update docs/manifest/static guards if this slice changes durable `frame-graph-v1` claims.
- [x] Re-run `tools/compose-agent-manifest.ps1 -Write` after manifest fragment edits.
- [x] Run `tools/validate.ps1` and `tools/build.ps1` before commit.

## Validation Evidence

| Command | Status | Evidence |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_renderer_tests` | RED PASS | The new shadow executor failure test failed before implementation because the manual transition path surfaced the raw transition exception instead of the deterministic executor error. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests` | PASS | Built focused renderer test target after the shadow renderer executor migration; existing MSB8028 intermediate-directory warnings remain. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_renderer_tests` | PASS | `MK_renderer_tests` passed 1/1 after the shadow renderer executor migration. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting/text-format checks passed after `tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest compose and JSON contract checks passed after static guard updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration checks passed after manifest, docs, skills, and subagent drift updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent file budgets and cross-tool skill/subagent consistency passed after shortening auditor guidance. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed, including agent/static checks, format, focused tidy smoke, dev build, and 65/65 CTest entries. Diagnostic-only host gates remain for Metal shader/library tools and Apple packaging on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Standalone dev build passed after full validation; existing MSB8028 intermediate-directory warnings remain. |

## Done When

- `RhiDirectionalShadowSmokeFrameRenderer` uses `execute_frame_graph_rhi_texture_schedule` for scheduled shadow-depth, scene-color, and scene-depth inter-pass texture transitions.
- Framegraph stats for this renderer come from executor results.
- Docs/manifest/static checks describe this as a second renderer migration while keeping final-state policy, production graph ownership, transient allocation, aliasing, multi-queue scheduling, package streaming, Metal parity, and broad renderer readiness unsupported.
- Focused renderer tests, agent/static checks, `validate.ps1`, and `build.ps1` pass or record concrete host blockers.
