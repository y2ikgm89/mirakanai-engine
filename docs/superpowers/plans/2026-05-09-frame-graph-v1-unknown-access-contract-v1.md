# Frame Graph v1 Unknown Access Contract v1 (2026-05-09)

**Plan ID:** `frame-graph-v1-unknown-access-contract-v1`  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md)

## Goal

Lock `compile_frame_graph_v1` rejection of `FrameGraphAccess::unknown` on pass reads/writes (`invalid_resource`, explicit-access contract).

## Constraints

- Compile-only `mirakana_renderer_tests`; no renderer-wide migration.

## Done when

- Unit test asserts failure with `invalid_resource` for unknown access on a declared transient resource.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Build/test | `cmake --build --preset dev --target mirakana_renderer_tests` → `ctest --preset dev -R mirakana_renderer_tests` | OK |
| Slice close | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
