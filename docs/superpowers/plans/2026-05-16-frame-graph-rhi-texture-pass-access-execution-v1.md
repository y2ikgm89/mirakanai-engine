# Frame Graph RHI Texture Pass Access Execution v1 (2026-05-16)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `frame-graph-rhi-texture-pass-access-execution-v1`
**Status:** Active. Local validation complete; publication pending.
**Goal:** Move `RhiPostprocessFrameRenderer` inter-pass texture barriers and pass callbacks through Frame Graph RHI schedule execution without exposing native handles.

**Architecture:** Keep the existing backend-neutral `MK_renderer` Frame Graph RHI execution API. The executor prevalidates binding and pass callback identity, then validates and records texture barriers in schedule order so pass callbacks can update caller-owned `FrameGraphTextureBinding::current_state` after writer pass work. `RhiPostprocessFrameRenderer` keeps host-owned command lists, swapchain frames, render-pass bodies, and final reusable-state resets; this slice does not implement transient heap allocation, aliasing, multi-queue scheduling, package streaming, shadow renderer migration, or broad production render-graph ownership.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, NullRHI unit tests, PowerShell repository validation.

---

## Official Practice Check

- Microsoft Direct3D 12 documentation for resource barriers states that applications record transition barriers before operations that require a different resource state. The relevant source is [Using Resource Barriers to Synchronize Resource States in Direct3D 12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12).
- Khronos Vulkan synchronization documentation shows image layout/access transitions between attachment writes and shader reads through command-buffer pipeline barriers. The relevant sources are [Vulkan Synchronization Examples](https://github.com/khronosgroup/vulkan-docs/wiki/Synchronization-Examples) and [Vulkan synchronization chapter](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/chapters/synchronization.adoc).
- Engine implication: keep native barrier details behind `IRhiCommandList::transition_texture`; the Frame Graph layer owns backend-neutral pass/resource/access ordering only.

## Context

- `frame-graph-v1` is the active Phase 1 foundation follow-up in the production completion master plan.
- Existing foundation includes `FrameGraphV1Desc`, deterministic `schedule_frame_graph_v1_execution`, generic callback execution, texture barrier recording, and `execute_frame_graph_rhi_texture_schedule`.
- Before this slice, `RhiPostprocessFrameRenderer` manually recorded scene-to-postprocess texture transitions even though its pass order and barrier intent were already represented in Frame Graph v1 schedules.

## Constraints

- Do not expose D3D12, Vulkan, Metal, command-list native handles, descriptor handles, query heaps, or queue/fence internals.
- Do not claim renderer-wide manual-transition removal, transient allocation, aliasing, multi-queue scheduling, package streaming, 2D/3D vertical-slice readiness, or Vulkan/Metal parity from this slice.
- Keep public contracts under `mirakana::` and use existing `IRhiCommandList::transition_texture`.
- Add or update tests before production code.
- Update docs, manifest fragments, composed manifest, skills/subagents/static checks only for durable behavior or agent-surface drift found in this slice.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`
- Modify: `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`
- Test: `tests/unit/renderer_rhi_tests.cpp`
- Docs/sync as needed: `docs/superpowers/plans/README.md`, `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, `engine/agent/manifest.json`, and scoped static guards.

## Tasks

### Task 1: RED Ordered Executor Test

- [x] Add a failing `MK_renderer_tests` case proving a pass callback may update a bound texture from `undefined` to `render_target`, after which the following scheduled barrier transitions it to `shader_read` before the reader pass callback.
- [x] Run the focused renderer test target and confirm the test fails because `execute_frame_graph_rhi_texture_schedule` currently prevalidates barriers before invoking writer pass callbacks.

### Task 2: Ordered Frame Graph RHI Execution

- [x] Prevalidate texture binding identity and pass callback identity before invoking any callback.
- [x] Validate and record planned texture barriers in schedule order, using current binding state at the moment the barrier step executes.
- [x] Keep existing deterministic diagnostics for missing bindings, invalid access, closed command lists, failing callbacks, and transition failures.
- [x] Run the focused renderer test target and confirm the new RED test turns green.

### Task 3: Postprocess Renderer Integration

- [x] In `RhiPostprocessFrameRenderer::end_frame`, replace scene-to-post and post-chain manual transitions with `execute_frame_graph_rhi_texture_schedule` over renderer-owned texture bindings and pass callbacks.
- [x] Keep first-use writer-state setup and final reusable-state resets outside the graph until a separate final-state policy exists.
- [x] Preserve existing renderer stats and NullRHI counts unless the new executor intentionally exposes a narrower count.

### Task 4: Agent Surface Drift And Validation

- [x] Update docs/manifest/static checks if this slice changes durable `frame-graph-v1` claims.
- [x] Run focused validation first: `MK_renderer_tests`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`.
- [x] Close the slice with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before commit.

## Validation Evidence

| Command | Status | Evidence |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_renderer_tests` | RED PASS | The new ordered executor test failed before implementation because prevalidation rejected the writer-updated texture state before the writer pass callback could run. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests` | PASS | Built focused renderer test target after executor and `RhiPostprocessFrameRenderer` changes; existing MSB8028 intermediate-directory warnings remain. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_renderer_tests` | PASS | `MK_renderer_tests` passed 1/1 after the ordered executor and postprocess renderer integration. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Composed manifest matched fragments and new postprocess executor/source guards passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent integration checks passed, including the `RhiPostprocessFrameRenderer` executor adoption needles. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting and text-format checks passed after applying `tools/format.ps1` to the changed C++ files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent config, skill parity, budgets, and tracked agent-surface checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed, including `check-production-readiness-audit.ps1` reporting `unsupported_gaps=8` and CTest 65/65 passing; Metal/Apple lanes remain diagnostic-only or host-gated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Standalone build passed after full validation; existing MSB8028 intermediate-directory warnings remain. |

## Done When

- `execute_frame_graph_rhi_texture_schedule` records scheduled texture barriers in order after writer pass callbacks can update binding state.
- `RhiPostprocessFrameRenderer` uses the Frame Graph RHI texture executor for scheduled inter-pass texture transitions.
- The manifest/docs still describe `frame-graph-v1` as an active foundation follow-up unless the unsupported gap row is explicitly narrowed with evidence.
- Focused renderer tests, agent/static checks, `validate.ps1`, and `build.ps1` pass or record concrete host blockers.
