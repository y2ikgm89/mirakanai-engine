# Frame Graph RHI Texture Schedule Execution v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Execute a Frame Graph v1 schedule against caller-owned RHI texture bindings and pass callbacks in the same deterministic order.

**Plan ID:** `frame-graph-rhi-texture-schedule-execution-v1`

**Status:** Completed.

**Architecture:** Keep `mirakana/renderer/frame_graph.hpp` RHI-free and extend only the `mirakana/renderer/frame_graph_rhi.hpp` adapter. The new helper prevalidates texture barrier bindings and pass callbacks, then interleaves `IRhiCommandList::transition_texture` calls with caller-owned pass callbacks according to `FrameGraphExecutionStep` order. This does not migrate renderer-wide pass ownership, add transient allocation, aliasing, multi-queue scheduling, package streaming, or native backend handles.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, `NullRhiDevice`, `tests/unit/renderer_rhi_tests.cpp`.

---

## Context

- Target unsupported gap: `frame-graph-v1`, currently `implemented-foundation-only`.
- Existing `record_frame_graph_texture_barriers` records barrier steps through public RHI command lists, but it records all barriers as a batch.
- Existing `execute_frame_graph_v1_schedule` dispatches barrier/pass callbacks in schedule order, but its barrier callback is caller-owned and not wired to the texture barrier recorder.
- The next narrow production step is to provide one backend-neutral RHI adapter that preserves schedule order for texture transitions and pass callbacks without making the renderer own the graph.

## Constraints

- Do not put RHI types in `engine/core` or the pure `frame_graph.hpp` planner.
- Do not expose native D3D12, Vulkan, Metal, swapchain, command-list, descriptor, queue, or fence handles.
- Do not remove manual transitions from existing renderers in this slice.
- Do not claim production render graph ownership, transient heap allocation, aliasing, multi-queue graph scheduling, renderer-wide migration away from manual transitions, package streaming, or broad renderer quality.
- Add RED tests before production code.

## Done When

- Unit tests prove schedule execution observes pass callbacks before and after the appropriate texture transition.
- Unit tests prove missing texture bindings are diagnosed before any pass callback runs.
- Public adapter API lives in `frame_graph_rhi.hpp` and implementation lives in `frame_graph_rhi.cpp`.
- Docs, manifest notes, plan registry, master plan, and static checks mention Frame Graph RHI Texture Schedule Execution v1 while `frame-graph-v1` remains `implemented-foundation-only`.
- Focused renderer tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, targeted tidy, schema/agent checks, production-readiness audit, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact blockers.

## File Plan

- Modify `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`: add `FrameGraphRhiTextureExecutionDesc`, `FrameGraphRhiTextureExecutionResult`, and `execute_frame_graph_rhi_texture_schedule`.
- Modify `engine/renderer/src/frame_graph_rhi.cpp`: prevalidate bindings/callbacks, preplan texture barriers, then execute transitions and pass callbacks in schedule order.
- Modify `tests/unit/renderer_rhi_tests.cpp`: add RED coverage for ordered execution and prevalidation failure.
- Modify `engine/agent/manifest.json`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/rhi.md`, `docs/superpowers/plans/README.md`, this master plan, and static checks to prevent drift.

## Tasks

### Task 1: RED Tests

- [x] Add `MK_TEST("frame graph rhi texture schedule execution interleaves barriers and pass callbacks")` in `tests/unit/renderer_rhi_tests.cpp`.
- [x] In the test, build a two-pass schedule where `scene` writes `scene-color` as `color_attachment_write` and `postprocess` reads it as `shader_read`.
- [x] Create a `NullRhiDevice`, a texture binding whose initial state is `render_target`, and pass callbacks that capture `bindings[0].current_state`.
- [x] Assert the scene callback observes `render_target`, the postprocess callback observes `shader_read`, `barriers_recorded == 1`, `pass_callbacks_invoked == 2`, and the command list records the expected transition count.
- [x] Add `MK_TEST("frame graph rhi texture schedule execution validates barriers before pass callbacks")`.
- [x] In that test, execute the same schedule without texture bindings and assert failure diagnostics mention `scene-color`, `pass_callbacks_invoked == 0`, and no callback side effect occurs.
- [x] Run `cmake --build --preset dev --target MK_renderer_tests` and record the expected compile failure because `FrameGraphRhiTextureExecutionDesc` and `execute_frame_graph_rhi_texture_schedule` do not exist.

### Task 2: Adapter API And Implementation

- [x] Add this public API to `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`:

```cpp
struct FrameGraphRhiTextureExecutionDesc {
    rhi::IRhiCommandList* commands{nullptr};
    std::span<const FrameGraphExecutionStep> schedule;
    std::span<FrameGraphTextureBinding> texture_bindings;
    std::span<const FrameGraphPassExecutionBinding> pass_callbacks;
};

struct FrameGraphRhiTextureExecutionResult {
    std::size_t barriers_recorded{0};
    std::size_t pass_callbacks_invoked{0};
    std::vector<FrameGraphDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] FrameGraphRhiTextureExecutionResult
execute_frame_graph_rhi_texture_schedule(const FrameGraphRhiTextureExecutionDesc& desc);
```

- [x] In `frame_graph_rhi.cpp`, validate `desc.commands != nullptr`, `!desc.commands->closed()`, non-empty/unique pass callback names, and non-empty callbacks before executing any schedule step.
- [x] Reuse the existing texture binding validation and simulated-state planning so missing/stale texture bindings are diagnosed before pass callbacks run.
- [x] Execute each schedule step in order: record the planned texture transition for barrier steps, and call the matching pass callback for pass steps.
- [x] Convert pass callback failures and exceptions into deterministic `FrameGraphDiagnostic` rows and stop execution.
- [x] Run `cmake --build --preset dev --target MK_renderer_tests` and `ctest --preset dev --output-on-failure -R "^MK_renderer_tests$"`.

### Task 3: Docs And Static Contract

- [x] Update current-truth docs and manifest notes for the new ordered RHI texture schedule execution helper.
- [x] Keep `frame-graph-v1` `implemented-foundation-only` and explicitly keep production render graph ownership, renderer-wide pass migration, transient allocation, aliasing, multi-queue scheduling, package streaming, and renderer-wide manual-transition removal unsupported.
- [x] Add static checks for the new public API, implementation strings, tests, docs, and completed plan status.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit the coherent slice after validation and build pass.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED `cmake --build --preset dev --target MK_renderer_tests` | PASS | Failed as expected before implementation because `FrameGraphRhiTextureExecutionDesc` and `execute_frame_graph_rhi_texture_schedule` were missing. |
| `cmake --build --preset dev --target MK_renderer_tests` | PASS | Re-run after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`; built `MK_renderer_tests.exe`. |
| `ctest --preset dev --output-on-failure -R "^MK_renderer_tests$"` | PASS | `MK_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check accepted the `frame_graph_rhi.hpp` addition. |
| Targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | PASS | `-Files engine/renderer/src/frame_graph_rhi.cpp,tests/unit/renderer_rhi_tests.cpp -MaxFiles 2`; existing warnings are reported, wrapper result is ok. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `unsupported_gaps=11`; `frame-graph-v1` remains `implemented-foundation-only`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Repository format check passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `git diff --check` | PASS | Whitespace check passed; Git reported existing LF-to-CRLF working-copy warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest passed 29/29. Metal/Apple diagnostics remain host-gated on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | `tools/build.ps1` completed the `dev` preset build. |
