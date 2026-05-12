# Frame Graph v1 Duplicate Declaration Contract v1 (2026-05-09)

**Plan ID:** `frame-graph-v1-duplicate-declaration-contract-v1`  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md)

## Goal

Lock `compile_frame_graph_v1` diagnostics when pass names or resource names are declared more than once (`invalid_pass` / `invalid_resource`), matching existing compiler validation.

## Context

`engine/renderer/src/frame_graph.cpp` rejects duplicate pass indices and duplicate resource rows before scheduling; no renderer-wide migration or SDL execution.

## Constraints

- Compile-only `mirakana_renderer_tests`; no gap-row promotion for `frame-graph-v1`.

## Done when

- Unit tests assert `invalid_pass` for duplicate pass names and `invalid_resource` for duplicate resource declarations.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Configure/build | `cmake --preset dev` → `cmake --build --preset dev --target mirakana_renderer_tests` | OK |
| Tests | `ctest --preset dev -R mirakana_renderer_tests --output-on-failure` | OK |
| Slice close | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
