# Frame Graph v1 Execution v1 Implementation Plan (2026-05-03)

> **For agentic workers:** Phase 3 master child **`frame-graph-v1-execution-v1`**. Turns compiled Frame Graph v1 plans into a **deterministic linear execution schedule** (barrier steps then pass steps) and wires **RHI postprocess** and **directional shadow smoke** renderers plus **shadow map planning** to v1 declarations—no Frame Graph v0 validation on those paths.

**Goal:** After `compile_frame_graph_v1` succeeds, `schedule_frame_graph_v1_execution` emits a backend-neutral `std::vector<FrameGraphExecutionStep>` ordered like real submission: for each pass in `ordered_passes`, all incoming barriers (sorted by resource, then `from_pass`) then one `pass_invoke` step. Hosts record barrier-step counts against `RendererStats::framegraph_barrier_steps_executed` when a frame completes so package smokes can observe v1 barrier scheduling without native handles.

**Architecture:** Value types live in `mirakana/renderer/frame_graph.hpp`. Scheduling is pure C++ over `FrameGraphV1BuildResult::barriers` and `ordered_passes`. RHI renderers validate v1 at construction, cache the schedule, and bump stats once per finished frame by counting barrier steps in the cached schedule.

**Tech Stack:** `engine/renderer/include/mirakana/renderer/frame_graph.hpp`, `engine/renderer/src/frame_graph.cpp`, `rhi_postprocess_frame_renderer.*`, `rhi_directional_shadow_smoke_frame_renderer.*`, `shadow_map.*`, `engine/renderer/include/mirakana/renderer/renderer.hpp`, `tests/unit/renderer_rhi_tests.cpp`.

---

## Phases

### Phase v1（本スライス・完了）

- [x] `FrameGraphExecutionStep` + `schedule_frame_graph_v1_execution`.
- [x] `RendererStats::framegraph_barrier_steps_executed` + per-frame increments on v1 renderers.
- [x] `RhiPostprocessFrameRenderer` / `RhiDirectionalShadowSmokeFrameRenderer`: v1 desc + compile + schedule validation.
- [x] `ShadowMapPlan`: `frame_graph_plan` (`FrameGraphV1BuildResult`) + `frame_graph_execution`; removed v0 desc/result fields.
- [x] Unit tests for schedule shape + shadow map plan fields.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` evidence.

### Follow-ups

- Drive `IRhiCommandList::transition_texture` from `FrameGraphBarrier` rows via a resource-handle map (requires graph to carry abstract resource ids beyond strings).
- Retire Frame Graph v0 from remaining tests once all callers migrate.

## Validation Evidence

| Command | Result | Date |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok` | 2026-05-03 |
| `ctest -C Debug -R mirakana_renderer_tests` | Passed | 2026-05-03 |

## Done When

- [x] Failed `FrameGraphV1BuildResult` yields an empty schedule.
- [x] Postprocess and shadow-smoke constructors throw on invalid v1 plans matching prior v0 guarantees.

## Non-Goals

- Automatic RHI state translation tables, queue scheduling, or resource aliasing heaps.

---

*Completed: 2026-05-03.*
