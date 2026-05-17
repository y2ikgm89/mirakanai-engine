# 2026-05-17 Frame Graph Render Pass Package Evidence v1

## Goal

Make executor-owned render pass envelope counts package-visible for the already selected SDL desktop runtime package smokes, using existing first-party `RendererStats::framegraph_render_passes_recorded` evidence.

## Context

- Frame Graph Render Pass Stats Evidence v1 already added `RendererStats::framegraph_render_passes_recorded` for completed raw primary, postprocess, and directional-shadow renderer frames.
- Package stdout and installed validation already expose `framegraph_passes_executed` and `framegraph_barrier_steps_executed`.
- `evaluate_sdl_desktop_presentation_quality_gate` already validates framegraph pass and barrier budgets for selected package lanes.

## Constraints

- Do not expose native render pass objects, RHI handles, backend stats, GPU timestamps, or queue internals.
- Do not change renderer execution order or migrate additional renderer paths.
- Keep exact package evidence scoped to selected postprocess/depth and directional-shadow package smokes.
- Keep `frame-graph-v1` as foundation-only; do not claim production graph ownership or broad renderer quality.

## Plan

- [x] Add RED runtime-host tests requiring quality-gate expected/current render-pass envelope fields.
- [x] Add `SdlDesktopPresentationQualityGateReport::expected_framegraph_render_passes` and `framegraph_render_passes_current`.
- [x] Validate render-pass envelope counts inside `evaluate_sdl_desktop_presentation_quality_gate`.
- [x] Print and validate `framegraph_render_passes_recorded` in selected sample/generated desktop package stdout and installed validation.
- [x] Update generated-game templates, docs, manifests, static guards, and agent-surface guidance.

## Done When

- Selected package status lines can expose `framegraph_render_passes_recorded`.
- Renderer quality gates expose `renderer_quality_expected_framegraph_render_passes` and `renderer_quality_framegraph_render_passes_ok`.
- Installed desktop runtime validation can require exact render-pass envelope counts for selected postprocess/depth and directional-shadow smokes.
- Focused runtime-host tests and package sample builds pass.
- Agent-surface drift is reconciled.

## Validation Evidence

| Check | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_sdl3_tests` | RED PASS: failed before implementation because `SdlDesktopPresentationQualityGateReport` had no `expected_framegraph_render_passes` / `framegraph_render_passes_current` members. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_sdl3_tests` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_host_sdl3_tests --output-on-failure` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_sdl3_tests sample_desktop_runtime_game sample_generated_desktop_runtime_cooked_scene_package sample_generated_desktop_runtime_material_shader_package sample_generated_desktop_runtime_3d_package` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` | PASS: installed D3D12 smoke reported `framegraph_passes_executed=6`, `framegraph_render_passes_recorded=6`, `framegraph_barrier_steps_executed=15`, `renderer_quality_expected_framegraph_render_passes=6`, and `renderer_quality_framegraph_render_passes_ok=1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
