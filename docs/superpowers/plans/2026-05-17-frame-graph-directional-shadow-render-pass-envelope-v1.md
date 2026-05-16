# Frame Graph Directional Shadow Render Pass Envelope v1 (2026-05-17)

**Plan ID:** `frame-graph-directional-shadow-render-pass-envelope-v1`
**Status:** Completed slice for `frame-graph-v1`.

## Goal

Move `RhiDirectionalShadowSmokeFrameRenderer` render pass begin/end scopes into the Frame Graph RHI executor using the existing `FrameGraphRhiRenderPassDesc` envelope contract. Keep shadow-depth, scene receiver, and postprocess pass bodies as callbacks, while attachment binding and begin/end ordering become executor-owned for this selected directional-shadow smoke path.

## Official practice check

- Direct3D 12 render passes declare render target/depth-stencil bindings at `BeginRenderPass`, record commands inside the pass, and close with `EndRenderPass`; at least one output binding is required. Source: Microsoft Direct3D 12 render pass documentation via Context7 (`/websites/learn_microsoft_en-us_windows_win32_direct3d12`, queried 2026-05-17).
- Vulkan dynamic rendering uses paired `vkCmdBeginRendering` / `vkCmdEndRendering` calls with attachments declared in rendering info; these calls do not provide synchronization, so barriers remain separate. Source: Khronos Vulkan docs via Context7 (`/khronosgroup/vulkan-docs`, queried 2026-05-17).
- This slice follows that model at the first-party RHI level: render pass envelopes are explicit executor work, while synchronization stays in the existing texture barrier, pass target-state, and final-state machinery.

## Context

- `frame-graph-v1` remains the active foundation follow-up gap.
- `FrameGraphRhiRenderPassDesc` and `render_passes_recorded` already exist from the postprocess render-pass envelope slice.
- `RhiDirectionalShadowSmokeFrameRenderer` already routes pass callbacks, pass target-state preparation, inter-pass transitions, and final depth-state restoration through `execute_frame_graph_rhi_texture_schedule`, but its render pass begin/end scopes are still renderer-owned inside callbacks.

## Constraints

- Do not expose native handles, backend command buffers, swapchain internals, descriptor handles, or RHI devices to gameplay.
- Do not claim production graph ownership, broad/background package streaming integration, multi-queue scheduling, renderer-wide pass migration, Vulkan/Metal memory alias allocation, data inheritance/content preservation, or broad renderer readiness.
- Keep pass counts and barrier budgets unchanged.
- Keep overlay preparation outside executor-owned render pass scopes so callbacks record only pass-body commands.

## Tasks

- [x] Add RED static guard requiring directional-shadow `FrameGraphRhiRenderPassDesc` usage and no direct renderer-owned render pass begin/end.
- [x] Declare shadow-depth, scene receiver, and postprocess render pass envelopes in `RhiDirectionalShadowSmokeFrameRenderer`.
- [x] Remove direct render pass begin/end calls from directional-shadow callbacks while preserving draw ordering and overlay recording.
- [x] Update docs, manifest fragments/composed manifest, skills/subagents, and validation needles for the new narrow ready surface.
- [x] Run focused renderer tests and agent/static checks, then full `tools/validate.ps1` and `tools/build.ps1` before publishing.

## Done When

- `RhiDirectionalShadowSmokeFrameRenderer` passes `.render_passes` to `execute_frame_graph_rhi_texture_schedule`.
- Directional-shadow callbacks record only pass-body work; render pass begin/end scopes are executor-owned.
- Existing directional-shadow frame graph pass counts, barrier-step budgets, and renderer behavior remain unchanged.
- Agent surfaces accurately describe directional-shadow render pass envelope ownership without broadening unsupported claims.

## Validation Evidence

| Command | Status | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireVcpkgToolchain` | PASS | Linked worktree ready; external/vcpkg and vcpkg_installed linked; normalized environment ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | PASS | Configure completed using normalized preset environment in the linked worktree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | RED | Static guard failed before implementation because `RhiDirectionalShadowSmokeFrameRenderer` did not yet contain `FrameGraphRhiRenderPassDesc`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Static guard passes after directional-shadow render pass envelopes moved into the executor. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | C++ and tracked text formatting pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent surface parity and instruction-size checks pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest composition and JSON contracts pass after fragment updates. |
| `cmake --build --preset dev --target MK_renderer_tests` | PASS | `MK_renderer_tests.exe` rebuilt after the renderer change. |
| `ctest --preset dev -R MK_renderer_tests --output-on-failure` | PASS | `MK_renderer_tests` passed; directional-shadow pass count/barrier budget assertions remain green. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed; CTest reported 65/65 passed, with Apple/Metal host gates diagnostic-only on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Full dev preset build completed after validation. |
