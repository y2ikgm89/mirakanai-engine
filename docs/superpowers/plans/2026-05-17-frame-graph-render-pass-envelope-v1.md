# Frame Graph Render Pass Envelope v1 (2026-05-17)

**Plan ID:** `frame-graph-render-pass-envelope-v1`
**Status:** Completed.

## Goal

Move one more renderer-owned pass boundary into the Frame Graph RHI executor by letting `execute_frame_graph_rhi_texture_schedule` own render pass begin/end envelopes for declared passes. Keep pass body commands caller-supplied callbacks, but make attachment binding, begin/end ordering, and validation executor-owned for the selected postprocess path.

## Official practice check

- Direct3D 12 render passes declare render target/depth-stencil bindings at `BeginRenderPass`, record commands inside the pass, and close with `EndRenderPass`; at least one output binding is required. Source: Microsoft Direct3D 12 render pass documentation via Context7 (`/websites/learn_microsoft_en-us_windows_win32_direct3d12`, queried 2026-05-17).
- Vulkan dynamic rendering uses `vkCmdBeginRendering` / `vkCmdEndRendering` as a paired command-buffer scope; synchronization remains separate and must be handled by explicit barriers. Source: Khronos Vulkan docs via Context7 (`/khronosgroup/vulkan-docs`, queried 2026-05-17).
- This slice follows that model at the first-party RHI level: render pass attachment declarations and begin/end scopes are explicit executor work, while texture synchronization remains in the existing Frame Graph barrier and target-state machinery.

## Context

- The master plan points back to `production-completion-master-plan-v1` with `recommendedNextPlan.id=next-production-gap-selection`.
- The active gap remains `frame-graph-v1`; completed slices already cover pass callback scheduling, texture target-state preparation, final-state transitions, alias planning/leases/barriers, automatic aliasing-barrier insertion, selected primary/postprocess/shadow/viewport executor routing, and package-visible pass/barrier counters.
- Remaining `frame-graph-v1` work includes broader production graph pass ownership, package streaming integration, multi-queue scheduling, and broader renderer migration. This slice advances only pass ownership.

## Constraints

- Do not expose native handles, backend command buffers, swapchain internals, descriptor handles, or RHI devices to gameplay.
- Do not claim production graph ownership, package streaming integration, multi-queue scheduling, Vulkan/Metal memory alias allocation, data inheritance/content preservation, or broad renderer readiness.
- Keep render pass envelopes backend-neutral and based on existing public RHI `RenderPassDesc`.
- Keep texture state transitions and aliasing barriers in the existing Frame Graph executor path; render pass begin/end is not a synchronization substitute.
- Keep tests focused on externally meaningful guarantees: prevalidated envelope descriptors, executor-owned begin/end ordering, and postprocess renderer use without changing package budgets.

## Tasks

- [x] Add `FrameGraphRhiRenderPassDesc` attachment envelope descriptors and `render_passes_recorded` result evidence.
- [x] Validate duplicate/unknown/invalid render pass envelopes before command recording or callbacks.
- [x] Wrap selected pass callbacks in executor-owned `begin_render_pass` / `end_render_pass`.
- [x] Move `RhiPostprocessFrameRenderer` scene/postprocess render pass begin/end into executor envelopes while preserving existing pass and barrier budgets.
- [x] Update docs, manifest fragments/composed manifest, skills/subagents, and static needles for the new narrow ready surface.
- [x] Run focused renderer build/tests and agent/static checks, then full `tools/validate.ps1` and `tools/build.ps1` before publishing.

## Done When

- `execute_frame_graph_rhi_texture_schedule` records render pass envelopes for declared passes before invoking the pass callback and closes them after successful callback recording.
- Invalid render pass envelopes fail before callbacks and before RHI render pass recording.
- `RhiPostprocessFrameRenderer` uses executor-owned render pass envelopes for the scene pass and postprocess pass chain while keeping swapchain acquire/present, command-list close/submit/wait, resource ownership, descriptor updates, and native UI overlay preparation renderer-owned.
- Existing postprocess frame graph pass counts, barrier-step budgets, and renderer behavior remain unchanged except for executor-owned render pass envelope evidence.
- Agent surfaces accurately describe render pass envelope ownership without broadening unsupported claims.

## Validation Evidence

| Command | Status | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | PASS | Linked worktree ready; CMake/CTest/CPack/clang-format/direct CMake ready; normalized environment ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | PASS | Configure completed using normalized preset environment in the linked worktree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | PASS | `MK_renderer_tests.exe` rebuilt after render pass envelope changes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` | PASS | `MK_renderer_tests` passed, including render pass envelope success and invalid-envelope failure coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `text-format-check: ok`; `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `agent-manifest-compose: ok`; `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public headers keep native backend symbols out of public APIs. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent surface size/parity/config checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent integration contract and runtime rendering needles passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full slice gate passed; 65/65 CTest targets passed. Diagnostic-only host gates remain Metal/Apple host availability. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Standalone build evidence passed after full validation. |
