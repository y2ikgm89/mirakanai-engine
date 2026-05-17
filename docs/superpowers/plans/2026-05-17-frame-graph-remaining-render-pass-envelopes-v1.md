# Frame Graph Remaining Render Pass Envelopes v1 (2026-05-17)

**Plan ID:** `frame-graph-remaining-render-pass-envelopes-v1`
**Status:** Completed slice for `frame-graph-v1`.

## Goal

Move the remaining high-level renderer-owned render pass begin/end scopes in `RhiFrameRenderer` and `RhiViewportSurface` into the Frame Graph RHI executor using `FrameGraphRhiRenderPassDesc`. Keep pass bodies, swapchain acquire/present, command-list submit/wait, and viewport display/readback ownership in the renderer layer.

## Official practice check

- Direct3D 12 render passes declare render target/depth-stencil bindings at `BeginRenderPass`, record commands inside the pass, and close with `EndRenderPass`; the application remains responsible for resource barriers and state tracking. Source: Microsoft Direct3D 12 render pass documentation via Context7 (`/websites/learn_microsoft_en-us_windows_win32_direct3d12`, queried 2026-05-17).
- Vulkan dynamic rendering uses paired `vkCmdBeginRendering` / `vkCmdEndRendering` calls with attachment load/store state in rendering attachment info; the commands do not provide synchronization, so barriers remain separate. Source: Khronos Vulkan docs via Context7 (`/khronosgroup/vulkan-docs`, queried 2026-05-17).
- This slice follows that model at the first-party RHI level: render pass envelopes are executor work, while synchronization remains in texture barrier, pass target-state, and final-state rows.

## Context

- `frame-graph-v1` remains the active foundation follow-up gap.
- `FrameGraphRhiRenderPassDesc`, `render_passes_recorded`, and executor-owned envelope handling already exist from the postprocess and directional-shadow slices.
- Current `origin/main` still had direct high-level begin/end scopes in `RhiFrameRenderer::end_frame()` and `RhiViewportSurface::render_clear_frame()`.

## Constraints

- Do not expose native handles, backend command buffers, swapchain internals, descriptor handles, or RHI devices to gameplay.
- Do not claim production graph ownership, broad/background package streaming integration, multi-queue scheduling, Vulkan/Metal memory alias allocation, data inheritance/content preservation, public wildcard/null aliasing barriers, or broad renderer readiness.
- Preserve existing pass counts, draw ordering, viewport state recovery behavior, and raw primary frame stats.
- Keep `RhiViewportSurface` display/readback state transitions in the existing Frame Graph RHI texture schedule path.

## Tasks

- [x] Add RED static guard requiring `RhiFrameRenderer` and `RhiViewportSurface` render pass envelope usage and no high-level direct render pass begin/end calls.
- [x] Move `RhiFrameRenderer` `primary_color` render pass begin/end into `.render_passes` while keeping queued primary/native-overlay work in the callback.
- [x] Move `RhiViewportSurface::render_clear_frame()` clear pass begin/end into `.render_passes` with writer-access-backed target-state and final copy-source restoration.
- [x] Update docs, manifest fragments/composed manifest, skills/subagents, and validation needles for the new narrow ready surface.
- [x] Preserve the Windows MSVC dev build gate by disabling incremental linking for MSVC linkable targets after two long reviewed-evictions runtime-package test executables reproduced `LNK1104` in this linked worktree.
- [x] Run focused renderer tests and agent/static checks, then full `tools/validate.ps1` and `tools/build.ps1` before publishing.

## Done When

- `RhiFrameRenderer` passes `FrameGraphRhiRenderPassDesc` rows to `execute_frame_graph_rhi_texture_schedule` and no longer calls `commands_->begin_render_pass` / `commands_->end_render_pass`.
- `RhiViewportSurface::render_clear_frame()` uses a scheduled no-op pass callback plus `FrameGraphRhiRenderPassDesc`, pass target-state, and final-state rows for its clear pass.
- `engine/renderer/src/frame_graph_rhi.cpp` is the only high-level renderer source in the selected set that directly records RHI render pass begin/end calls.
- Agent surfaces accurately describe primary and viewport render pass envelope ownership without broadening unsupported claims.

## Validation Evidence

| Command | Status | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | RED | Static guard failed before implementation because `RhiFrameRenderer` did not yet contain `FrameGraphRhiRenderPassDesc`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Static guard passes after primary and viewport render pass envelopes moved into the executor. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake` | PASS | Linked worktree ready; external/vcpkg and vcpkg_installed available; normalized environment and direct CMake ready. |
| `cmake --preset dev` | PASS | Configure completed in the linked worktree with the dev preset. |
| `cmake --build --preset dev --target MK_runtime_package_candidate_resident_replace_reviewed_evictions_tests` | PASS | Direct CMake/MSBuild target build succeeds with checked-in `/INCREMENTAL:NO` policy. |
| `cmake --build --preset dev --target MK_runtime_package_discovery_resident_replace_reviewed_evictions_tests` | PASS | Direct CMake/MSBuild target build succeeds with checked-in `/INCREMENTAL:NO` policy. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | PASS | `MK_renderer_tests.exe` rebuilt after the renderer change. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` | PASS | `MK_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | C++ and tracked text format checks pass after formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent instruction/skill/subagent surface checks pass after rendering guidance updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest fragments, composed manifest, and agent JSON contracts pass after production-loop updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passes; this slice does not broaden gameplay-facing API exposure. |
| `rg -n "begin_render_pass|end_render_pass" engine/renderer/src/rhi_frame_renderer.cpp engine/renderer/src/rhi_viewport_surface.cpp engine/renderer/src/rhi_postprocess_frame_renderer.cpp engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp engine/renderer/src/frame_graph_rhi.cpp` | PASS | Only `engine/renderer/src/frame_graph_rhi.cpp` directly records render pass begin/end calls in the selected high-level renderer set. |
| `cmake --build --preset dev --target MK_runtime_package_candidate_resident_replace_reviewed_evictions_tests -- /m:1` | FAIL | Reproduced local MSVC `LNK1104` opening the long test exe with Debug incremental linking enabled in this linked worktree. |
| `cmake --build --preset dev --target MK_runtime_package_candidate_resident_replace_reviewed_evictions_tests -- /p:LinkIncremental=false /m:1` and `cmake --build --preset dev --target MK_runtime_package_discovery_resident_replace_reviewed_evictions_tests -- /p:LinkIncremental=false /m:1` | PASS | Confirmed the blocker was target Debug incremental linking; `MK_apply_common_target_options` now applies `/INCREMENTAL:NO` to MSVC linkable targets. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Full Windows MSVC dev build passes after the targeted CMake link-option fix. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passes, including static checks, build, and CTest `65/65` on this Windows host. Metal/Apple lanes remain documented diagnostic-only host gates. |
