# Frame Graph Minimal Three-Pass Stack Proof v1 (2026-05-09)

**Plan ID:** `frame-graph-minimal-three-pass-stack-proof-v1`  
**Status:** Completed  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md) (Phase 4 — narrow multi-pass scheduling evidence)

## Goal

Add a **compile-only** regression test that proves Frame Graph v1 orders **directional shadow depth → lit scene → postprocess** for a minimal resource dependency chain resembling the desktop SDL presentation stack, **without** claiming production render-graph ownership, renderer-wide pass migration, host SDL/RHI execution, or broader Phase 4 readiness.

## Context

- `compile_frame_graph_v1`, `schedule_frame_graph_v1_execution`, and `execute_frame_graph_v1_schedule` already ship as foundation-only APIs (`frame-graph-v1` gap remains foundation-only in `engine/agent/manifest.json`).
- Runtime hosts (`RhiPostprocessFrameRenderer`, `RhiDirectionalShadowSmokeFrameRenderer`) already record multi-pass behavior with separate validation; this slice adds a **pure planner/scheduler** contract test so CI can lock pass order independently of GPU backends.

## Constraints

- No new dependencies; no manifest ready-claim changes.
- English-only plan text under `docs/superpowers/plans/` per `AGENTS.md`.

## Done when

- Unit test asserts topological order `directional_shadow` → `scene` → `postprocess` and the derived execution schedule shape (barriers before dependent passes).
- Default dev preset build passes; `mirakana_renderer_tests` includes the new test.

## Validation evidence

- Build: `cmake --build --preset dev --target mirakana_renderer_tests`
- Run: `out/build/dev/Debug/mirakana_renderer_tests.exe` (filter matches test name substring); observed `[PASS] frame graph v1 orders directional shadow scene and postprocess for a minimal desktop-style stack`

## Out of scope

- Wiring this abstract graph into SDL desktop hosts beyond existing renderer code paths.
- Closing `frame-graph-v1` unsupported gap beyond **foundation scheduling proof**.
