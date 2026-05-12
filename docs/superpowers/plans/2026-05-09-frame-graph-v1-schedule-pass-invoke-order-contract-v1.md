# Frame Graph v1 Schedule Pass Invoke Order Contract v1 (2026-05-09)

**Plan ID:** `frame-graph-v1-schedule-pass-invoke-order-contract-v1`  
**Parent wave:** [2026-05-09-phase-4-5-continuation-wave-1-v1.md](2026-05-09-phase-4-5-continuation-wave-1-v1.md)  
**Status:** Completed.

## Goal

Lock **compile-only** proof that `schedule_frame_graph_v1_execution` emits `pass_invoke` steps in the same order as `FrameGraphV1BuildResult::ordered_passes`, so effect stacks that rely on topological order remain stable when barriers are present.

## Context

- [`schedule_frame_graph_v1_execution`](../../../engine/renderer/include/mirakana/renderer/frame_graph.hpp) prepends sorted barriers before each pass invoke.

## Constraints

- Does not migrate renderer passes or claim `frame-graph-v1` gap closure beyond foundation diagnostics.

## Done when

- `mirakana_renderer_tests` asserts extracted `pass_invoke` sequence matches `ordered_passes` on a non-trivial graph with inter-pass barriers.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Tests | `ctest --preset dev -R mirakana_renderer_tests --output-on-failure` | Pass |
| Repo | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
