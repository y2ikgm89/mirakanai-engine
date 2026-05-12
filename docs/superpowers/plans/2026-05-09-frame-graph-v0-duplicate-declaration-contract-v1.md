# Frame Graph v0 Duplicate Declaration Contract v1 (2026-05-09)

**Plan ID:** `frame-graph-v0-duplicate-declaration-contract-v1`  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md)

## Goal

Lock `compile_frame_graph_v0` diagnostics when pass names or string resource names are declared more than once (`invalid_pass` / `invalid_resource`).

## Context

Mirrors the Frame Graph v1 duplicate-declaration slice for the legacy v0 string-edge graph API used by smaller tests and migration comparisons.

## Constraints

- Compile-only `mirakana_renderer_tests`; no `frame-graph-v1` gap closure or SDL execution.

## Done when

- Unit tests assert `invalid_pass` for duplicate pass names and `invalid_resource` for duplicate resource rows.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Configure/build | `cmake --preset dev` → `cmake --build --preset dev --target mirakana_renderer_tests` | OK |
| Tests | `ctest --preset dev -R mirakana_renderer_tests --output-on-failure` | OK |
| Slice close | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
