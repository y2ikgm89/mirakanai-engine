# Frame Graph v1 Tie-Break and Cycle Contract v1 (2026-05-09)

**Plan ID:** `frame-graph-v1-tie-break-and-cycle-contract-v1`  
**Status:** Completed  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md) (P1 — narrow effect-order and scheduling diagnostics; does not close `frame-graph-v1` gap)

## Goal

Add **compile-only** `mirakana_renderer_tests` coverage that locks:

1. **Tie-break policy:** when two passes are both ready (indegree zero), `compile_frame_graph_v1` orders them by **ascending pass declaration index** (first match in the pass array wins).  
2. **Cycle policy:** a two-pass **ping-pong** resource pattern (each pass reads the other’s output) produces `FrameGraphDiagnosticCode::cycle` and an empty `ordered_passes`, matching the existing v0 cycle model.

No SDL, RHI, or renderer-wide pass migration. No `frame-graph-v1` unsupported gap promotion.

## Context

- `compile_frame_graph_v1` scans passes in index order when picking the next zero-indegree node (`frame_graph.cpp`).
- Milestone queue **P1** asks for additional narrow effect-order binding on Frame Graph v1 without claiming production graph ownership.

## Constraints

- English-only plan text under `docs/superpowers/plans/` per `AGENTS.md`.
- No manifest or `docs/current-capabilities.md` ready-language inflation.

## Done when

- Tests live in `tests/unit/renderer_rhi_tests.cpp` and pass under `ctest --preset dev -R mirakana_renderer_tests`.

## Validation evidence

- Build: `cmake --build --preset dev --target mirakana_renderer_tests`
- Run: `ctest --preset dev -R mirakana_renderer_tests --output-on-failure`
- Repo: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (includes `tools/check-ai-integration.ps1` needles aligned with committed `game.agent.json` prose where touched)

## Out of scope

- SDL desktop wiring, transient resource pooling, multi-queue scheduling, or closing the manifest `frame-graph-v1` row beyond existing foundation-only stance.
