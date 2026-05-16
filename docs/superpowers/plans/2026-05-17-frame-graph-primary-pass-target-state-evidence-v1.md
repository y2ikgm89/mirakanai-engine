# Frame Graph Primary Pass Target-State Evidence v1 - 2026-05-17

## Goal

Move `RhiFrameRenderer` primary texture targets into the Frame Graph RHI texture binding and pass target-state path so the raw primary pass has backend-neutral evidence for target-state preparation before its executor-owned render pass envelope.

## Context

- `frame-graph-v1` already owns RHI texture schedule execution, pass target-state rows, final-state rows, render pass envelopes, postprocess and directional-shadow pass ownership, and viewport color-state execution.
- The raw `RhiFrameRenderer` primary pass now uses an executor-owned `FrameGraphRhiRenderPassDesc` envelope but still passes no texture bindings or target-state rows for `primary_color`.
- Official D3D12 and Vulkan guidance keeps render pass declarations separate from resource-state synchronization; this slice keeps synchronization in explicit Frame Graph target-state rows rather than hiding it inside render pass envelope setup.

## Constraints

- Clean break; do not add compatibility shims or deprecated raw-render-pass state paths.
- Keep swapchain acquire/present, command-list submission, and renderer-owned resource lifetime outside this slice.
- Do not claim production graph ownership, multi-queue scheduling, Vulkan/Metal memory alias allocation, public native handles, broad package streaming, or renderer-wide manual-transition removal.
- Preserve caller-owned texture state honesty; imported texture bindings must start from the state the renderer actually knows.

## Plan

- [x] Add failing renderer tests proving the primary texture target and optional depth target record Frame Graph pass target-state barriers.
- [x] Add explicit primary pass texture binding, writer access, and target-state rows in `RhiFrameRenderer`.
- [x] Track imported primary/depth target states across completed frames and replacement paths.
- [x] Update docs, plan registry, manifest fragments/composed manifest, and agent-surface checks if durable guidance or AI-operable claims changed.
- [x] Run focused renderer validation, then full slice validation before commit/PR.

## Done When

- `RhiFrameRenderer` texture-target frames use `execute_frame_graph_rhi_texture_schedule` with `primary_color` texture binding, `color_attachment_write` access, and `render_target` target-state rows.
- Optional primary depth attachments use a declared depth writer access and `depth_write` target-state row.
- Renderer stats expose the new target-state barrier evidence through `framegraph_barrier_steps_executed`.
- Relevant docs/manifest/plan surfaces describe the narrowed capability without broad readiness claims.
- Focused renderer tests and the slice-closing validation pass or an exact host blocker is recorded.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` failed on the new `framegraph_barrier_steps_executed` expectations before implementation.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
