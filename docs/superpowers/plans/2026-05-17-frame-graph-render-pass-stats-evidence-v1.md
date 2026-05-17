# Frame Graph Render Pass Stats Evidence v1 (2026-05-17)

**Plan ID:** `frame-graph-render-pass-stats-evidence-v1`
**Status:** Completed.

## Goal

Expose executor-owned render pass envelope evidence through `RendererStats` for completed high-level renderer frames. This makes raw primary, postprocess, and directional-shadow frame paths prove not only callback execution and barrier steps, but also the number of render pass envelopes recorded by `execute_frame_graph_rhi_texture_schedule`.

## Official practice check

- This slice does not introduce new graphics API behavior. It reports first-party executor evidence already produced by `FrameGraphRhiTextureExecutionResult::render_passes_recorded`.
- Prior render pass envelope slices already checked official Direct3D 12 and Vulkan render pass/dynamic-rendering documentation. This slice keeps those API scopes private behind `IRhiCommandList` and only adds backend-neutral stats propagation.

## Context

- `FrameGraphRhiRenderPassDesc` and `render_passes_recorded` already exist and are validated by focused renderer tests.
- `RendererStats::framegraph_passes_executed` and `RendererStats::framegraph_barrier_steps_executed` already expose completed-frame executor callback/barrier evidence.
- The active gap remains `frame-graph-v1`; this slice advances broader explicit pass ownership evidence without claiming production graph ownership or package-visible renderer migration.

## Constraints

- Do not expose native render pass objects, command buffers, descriptor handles, swapchain internals, backend queues, or `IRhiDevice` access to gameplay.
- Do not change pass/barrier budgets, package stdout, quality gates, or renderer behavior beyond stats evidence.
- Count only successful completed frames after command recording, submit, and optional wait follow the existing renderer success path.
- Keep the public API clean-break: add the field directly to `RendererStats`; do not add compatibility aliases.

## Tasks

- [x] Add RED tests requiring `RendererStats::framegraph_render_passes_recorded` for raw primary, postprocess, and directional-shadow completed frames.
- [x] Add `RendererStats::framegraph_render_passes_recorded`.
- [x] Propagate `FrameGraphRhiTextureExecutionResult::render_passes_recorded` from `RhiFrameRenderer`, `RhiPostprocessFrameRenderer`, and `RhiDirectionalShadowSmokeFrameRenderer`.
- [x] Update docs, manifest fragments/composed manifest, rendering skill references, and static needles.
- [x] Run focused renderer build/tests, agent/static checks, and full validation before publishing.

## Done When

- Completed raw `RhiFrameRenderer` frames report one executor-owned render pass envelope.
- Completed one-stage `RhiPostprocessFrameRenderer` frames report two executor-owned render pass envelopes.
- Completed `RhiDirectionalShadowSmokeFrameRenderer` frames report three executor-owned render pass envelopes.
- Failed/incomplete frames do not gain render-pass stats beyond existing completed-frame success paths.
- Agent surfaces describe the new counter without broadening unsupported renderer claims.

## Validation Evidence

| Command | Status | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | RED PASS | Expected compile failure before implementation because `mirakana::RendererStats` had no `framegraph_render_passes_recorded` member. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | PASS | `MK_renderer_tests.exe` rebuilt after stats propagation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_renderer_tests --output-on-failure` | PASS | `MK_renderer_tests` passed, including raw primary, postprocess, and directional-shadow render-pass stats assertions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Text and clang-format checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest compose output matched fragments; JSON contracts passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary checks passed after adding the renderer stats field. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent config/budget/parity checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Runtime rendering and manifest static needles passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full slice gate passed; 65/65 CTest targets passed. Existing Apple/Metal checks remained host-gated diagnostic-only on this Windows host. |
