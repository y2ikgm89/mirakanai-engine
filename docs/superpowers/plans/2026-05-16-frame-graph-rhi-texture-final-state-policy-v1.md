# Frame Graph RHI Texture Final State Policy v1 (2026-05-16)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `frame-graph-rhi-texture-final-state-policy-v1`
**Status:** Completed.
**Goal:** Let `execute_frame_graph_rhi_texture_schedule` own declared reusable final texture states after scheduled pass execution, then remove renderer-owned final depth reset transitions from the postprocess and directional shadow smoke renderers.

**Architecture:** Extend the backend-neutral Frame Graph RHI texture executor with explicit caller-owned final-state rows keyed by resource name. The executor records final transitions after all scheduled barrier/pass callback steps, updates the same `FrameGraphTextureBinding` state rows, and reports deterministic diagnostics for missing, duplicate, invalid, or RHI-failed final-state transitions. Keep first-use writer setup, swapchain acquire/present, render-pass bodies, native UI overlay preparation, transient allocation, aliasing, multi-queue scheduling, and production graph ownership out of scope.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, NullRHI unit tests, PowerShell repository validation.

---

## Official Practice Check

- Microsoft Direct3D 12 resource barriers keep per-resource state management in the application; transition barriers are inserted before later operations require another state: https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12.
- Khronos Vulkan synchronization examples use image barriers between attachment writes and fragment shader reads, and final layouts remain explicit renderer/application policy: https://docs.vulkan.org/guide/latest/synchronization_examples.html and https://docs.vulkan.org/spec/latest/chapters/synchronization.html.
- Engine implication: the Frame Graph may own backend-neutral final desired states, but native barrier recording stays behind public `IRhiCommandList::transition_texture`.

## Context

- [Frame Graph RHI Texture Pass Access Execution v1](2026-05-16-frame-graph-rhi-texture-pass-access-execution-v1.md) made `execute_frame_graph_rhi_texture_schedule` interleave scheduled barriers with pass callbacks.
- [Frame Graph Shadow Smoke Execution Migration v1](2026-05-16-frame-graph-shadow-smoke-execution-migration-v1.md) migrated the directional shadow smoke renderer after the postprocess renderer migration.
- Both migrated renderers still perform reusable depth final resets outside the executor after schedule execution. This keeps final-state policy stale and leaves part of renderer-wide manual-transition removal outside the graph-owned state contract.

## Constraints

- Do not expose D3D12, Vulkan, Metal, command-list native handles, descriptor handles, queue/fence handles, or backend layout details.
- Do not claim transient allocation, aliasing, multi-queue scheduling, package streaming, Metal parity, production render graph ownership, or renderer-wide pass ownership.
- Do not move first-use writer setup into the graph in this slice.
- Add/update tests before production code and update every designated `FrameGraphRhiTextureExecutionDesc` initializer when the aggregate grows.
- Update docs, manifest fragments, composed manifest, skills/subagents/static checks only for durable behavior or agent-surface drift found in this slice.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`
- Modify: `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`
- Modify: `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`
- Test: `tests/unit/renderer_rhi_tests.cpp`
- Docs/sync as needed: `docs/rhi.md`, `docs/current-capabilities.md`, `docs/architecture.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/plans/2026-05-11-phase1-foundation-gaps-coherent-slice-order-v1.md`, `engine/agent/manifest.fragments/004-modules.json`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, `engine/agent/manifest.json`, scoped static guards, and rendering/auditor agent surfaces if durable guidance changes.

## Tasks

### Task 1: RED Final-State Executor Contract

- [x] Add `FrameGraphTextureFinalState` and a `final_states` field to `FrameGraphRhiTextureExecutionDesc` in the header, then update existing designated initializers with `.final_states = {}`.
- [x] Add a focused `MK_renderer_tests` case proving `execute_frame_graph_rhi_texture_schedule` records a requested final state after all pass callbacks, updates the binding state, increments `barriers_recorded`, and exposes a dedicated final-state counter.
- [x] Add a focused diagnostics case proving missing or duplicate final-state resource rows fail before any pass callback is invoked.
- [x] Run the focused renderer test target and confirm the new final-state expectations fail before implementation.

### Task 2: Final-State Executor Implementation

- [x] Validate final-state rows before execution: non-empty resource, existing texture binding, no duplicate resource rows, and target state not `undefined`.
- [x] After the scheduled steps finish, record final-state transitions in caller-provided order, skip rows already in the requested state, update `FrameGraphTextureBinding::current_state`, increment `barriers_recorded`, and increment the final-state counter.
- [x] Convert final-state RHI transition failures to deterministic `FrameGraphDiagnostic` rows without throwing raw RHI exceptions.
- [x] Run the focused `MK_renderer_tests` target and confirm the final-state contract tests pass.

### Task 3: Renderer Migration

- [x] Pass final-state rows from `RhiPostprocessFrameRenderer` for depth input reusable `scene_depth -> depth_write`, and remove the manual post-executor depth reset.
- [x] Pass final-state rows from `RhiDirectionalShadowSmokeFrameRenderer` for reusable `scene_depth -> depth_write` and `shadow_depth -> depth_write`, and remove the manual post-executor depth resets.
- [x] Update focused renderer stats expectations so executor-owned final-state barriers are counted as framegraph barrier steps while total RHI transition counts remain honest.
- [x] Add or update a renderer-level failure test proving a final-state transition failure is surfaced as the deterministic renderer executor error and releases the acquired swapchain frame.

### Task 4: Agent Surface Drift And Slice Close

- [x] Update docs/manifest/static guards if this slice changes durable `frame-graph-v1` claims.
- [x] Re-run `tools/compose-agent-manifest.ps1 -Write` after manifest fragment edits.
- [x] Run `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1`.
- [x] Run `tools/validate.ps1` and `tools/build.ps1` before commit.

## Validation Evidence

| Command | Status | Evidence |
| --- | --- | --- |
| RED `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_renderer_tests` | PASS | Failed as expected before implementation: the three new final-state executor tests failed because final states were ignored. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests` | PASS | Focused renderer test target built after final-state implementation and renderer migration. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_renderer_tests` | PASS | `MK_renderer_tests` passed after final-state executor implementation, renderer final reset migration, and stats updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Text and C++ formatting checks passed after C++/docs/static updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | JSON contracts and composed manifest checks passed after manifest/static guard edits. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration checks passed after docs/manifest/agent-surface edits. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent surface budget/parity checks passed after rendering skill/subagent edits. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed: 65/65 CTest passed; Metal/Apple diagnostics remained host-gated on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Standalone build passed after full validation; existing MSB8028 shared-intermediate warnings were emitted. |

## Done When

- `execute_frame_graph_rhi_texture_schedule` records declared final texture states after scheduled pass execution with deterministic diagnostics and binding-state updates.
- `RhiPostprocessFrameRenderer` and `RhiDirectionalShadowSmokeFrameRenderer` no longer record their reusable depth final resets outside the executor.
- Framegraph barrier stats count executor-owned final-state transitions.
- Docs/manifest/static checks describe this as final-state policy support while keeping first-use writer setup, transient allocation, aliasing, multi-queue scheduling, package streaming, production graph ownership, and broad renderer readiness unsupported.
- Focused renderer tests, agent/static checks, `validate.ps1`, and `build.ps1` pass or record concrete host blockers.
