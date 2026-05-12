# Frame Graph Pass Callback Execution v1 Implementation Plan (2026-05-08)

**Status:** Completed

**Goal:** Add a backend-neutral Frame Graph v1 execution helper that walks a successful `FrameGraphExecutionStep`
schedule and invokes caller-provided barrier/pass callbacks in deterministic order.

**Plan ID:** `frame-graph-pass-callback-execution-v1`

**Parent plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)

## Context

- Target unsupported gap: `frame-graph-v1`, currently `implemented-foundation-only`.
- Existing Frame Graph v1 already compiles pass/resource rows, emits deterministic barrier intent rows, builds
  `FrameGraphExecutionStep` schedules, and records texture barriers through a separate RHI adapter.
- The gap still lists production pass callbacks and general multi-pass scheduling as unsupported.

## Constraints

- Keep `mirakana/renderer/frame_graph.hpp` independent from RHI and native handles.
- Do not add transient allocation, aliasing, multi-queue scheduling, renderer-wide migration away from manual transitions,
  package streaming integration, or production render graph ownership.
- Use deterministic diagnostics instead of exceptions escaping from the helper.
- Preserve the existing schedule representation.

## Done When

- Unit tests prove callbacks run in schedule order and stop with diagnostics on missing or failing callbacks.
- `frame_graph.hpp/.cpp` expose and implement a minimal callback execution API.
- Manifest, roadmap/current capability docs, plan registry, and static checks describe the narrower ready surface without
  converting `frame-graph-v1` to ready.
- Focused renderer tests, boundary checks, static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete
  blocker.

## Implementation Steps

1. Add RED tests in `tests/unit/renderer_rhi_tests.cpp` for ordered barrier/pass callback dispatch and diagnostic stop.
2. Add `FrameGraphExecutionCallbacks`, `FrameGraphExecutionResult`, and
   `execute_frame_graph_v1_schedule` to `mirakana/renderer/frame_graph.hpp`.
3. Implement deterministic schedule walking in `engine/renderer/src/frame_graph.cpp`.
4. Update `engine/agent/manifest.json`, `docs/current-capabilities.md`, `docs/roadmap.md`, the plan registry, and static
   checks.
5. Run focused and full validation.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED `cmake --build --preset dev --target mirakana_renderer_tests` | PASS | Failed before implementation because `FrameGraphPassExecutionBinding`, `FrameGraphExecutionCallbacks`, and `execute_frame_graph_v1_schedule` did not exist. |
| `cmake --build --preset dev --target mirakana_renderer_tests` | PASS |  |
| `ctest --preset dev --output-on-failure -R "^mirakana_renderer_tests$"` | PASS |  |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public renderer header change. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/renderer/src/frame_graph.cpp,tests/unit/renderer_rhi_tests.cpp -MaxFiles 2` | PASS | Existing warning noise only; targeted check returned ok. |
| Subagent C++ review | PASS | Medium lifetime finding fixed by copying pass callbacks before dispatch; low missing-test finding fixed with binding-invalidation, returned failure, and thrown barrier callback coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS |  |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS |  |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Reports `unsupported_gaps=11`; this slice narrows `frame-graph-v1` but does not close the master plan. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Required one `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` pass before green. |
| `git diff --check` | PASS | CRLF conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | CTest 29/29 passed; Apple/Metal lanes remain host-gated/diagnostic-only on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Required before commit. |
