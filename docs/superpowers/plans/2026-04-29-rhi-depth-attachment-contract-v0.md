# RHI Depth Attachment Contract v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first backend-neutral depth attachment/depth-test contract to `mirakana_rhi` and opt-in renderer ownership so future D3D12/Vulkan native depth and shadow-map slices have a validated public RHI shape.

**Architecture:** Keep this slice backend-neutral and host-feasible: add first-party depth attachment value types to `RenderPassDesc`, validate depth texture usage/format/clear values in `NullRhiDevice`, validate pipeline depth-format compatibility when binding graphics pipelines, and let `RhiFrameRenderer` optionally pass a host-owned depth texture into the frame render pass. This slice does not claim native D3D12/Vulkan depth clears, Vulkan layout transitions, visible depth-tested package proof, shadow maps, cascades, or renderer light shadow integration.

**Tech Stack:** C++23, `mirakana_rhi`, `mirakana_renderer`, existing unit tests, no new third-party dependencies.

---

## Context

- `Format::depth24_stencil8`, `TextureUsage::depth_stencil`, and `GraphicsPipelineDesc::depth_format` already exist.
- `RenderPassDesc` currently has only one color attachment, so pipelines cannot express a matching render-pass depth attachment at the backend-neutral layer.
- D3D12/Vulkan native depth implementation remains larger and should be built after the public RHI contract and NullRHI validation are deterministic.

## Constraints

- Do not expose native D3D12/Vulkan/Metal handles, swapchain frames, image views, descriptor handles, DSV/DSV heap internals, or depth texture ownership through game APIs.
- Do not implement shadows, shadow maps, GPU markers, full render graph, D3D12/Vulkan native depth readback proof, or packaged visible depth smoke in this slice.
- Keep `mirakana_scene`, `mirakana_runtime`, and gameplay code RHI-free.
- Public RHI/renderer header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- RHI/renderer changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` before completion.

## Done When

- [x] `mirakana_rhi` exposes a first-party optional render-pass depth attachment with deterministic clear depth data and minimal depth-test/write/compare pipeline state.
- [x] `NullRhiDevice` validates depth attachment texture ownership, usage, format, finite clear depth range, and color/depth pipeline compatibility.
- [x] `RhiFrameRenderer` can accept an optional host-owned depth texture, include it in render passes, and replace it only between frames after host-owned target resize/recreation without changing public game APIs or presentation reports.
- [x] Tests cover valid depth render passes, invalid depth attachment usage/format/clear values, unknown depth handles, depth extent mismatch, invalid depth pipeline state, depth pipeline mismatch, no-depth pipeline mismatch, renderer optional depth target forwarding/replacement, and existing color-only paths.
- [x] Docs, roadmap, gap analysis, manifest, rendering skills, and Codex/Claude guidance describe this as backend-neutral RHI Depth Attachment Contract v0, not native visible D3D12/Vulkan depth, shadows, or production shadow maps.
- [x] D3D12 and Vulkan `IRhiDevice` bridges fail fast on public depth attachments and public depth pipeline state until native depth support is implemented.
- [x] D3D12 and Vulkan command lists validate active render-pass color/depth formats against bound graphics pipeline formats.
- [x] Focused RHI/renderer tests, API boundary, shader toolchain diagnostics, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Depth Attachment Tests

**Files:**
- Modify: `tests/unit/rhi_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Add NullRHI tests for a valid color+depth render pass with a depth-enabled graphics pipeline.
- [x] Add NullRHI tests that reject wrong depth usage, wrong depth format, invalid clear depth, binding a no-depth pipeline in a depth render pass, and binding a depth pipeline in a color-only render pass.
- [x] Add renderer tests proving `RhiFrameRendererDesc::depth_texture` forwards an optional depth attachment and leaves the color-only path intact.
- [x] Run focused build and verify failure because the depth attachment API and renderer desc field are missing.

### Task 2: Backend-Neutral RHI Contract

**Files:**
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`

- [x] Add `ClearDepthValue` and `RenderPassDepthAttachment`.
- [x] Add minimal `DepthStencilStateDesc` and compare operation state to `GraphicsPipelineDesc`.
- [x] Extend `RenderPassDesc` with optional depth attachment state while preserving existing aggregate color-only call sites.
- [x] Store pipeline color/depth formats in `NullRhiDevice`.
- [x] Validate render-pass depth attachment texture ownership, usage, `depth24_stencil8` format, and clear depth in `[0, 1]`.
- [x] Validate bound graphics pipeline color/depth formats against the active render pass.
- [x] Reject public depth attachments and public depth pipeline state in D3D12/Vulkan `IRhiDevice` paths until native support exists.
- [x] Preserve render-pass state when NullRHI depth validation rejects a swapchain-backed begin-frame attempt.
- [x] Add D3D12/Vulkan bind-time active render-pass vs graphics-pipeline format validation.
- [x] Keep D3D12 abandoned swapchain-frame cleanup from double-counting stats after host-level failure cleanup.

### Task 3: Renderer Optional Depth Target

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/rhi_frame_renderer.hpp`
- Modify: `engine/renderer/src/rhi_frame_renderer.cpp`

- [x] Add `RhiFrameRendererDesc::depth_texture`.
- [x] Preserve exactly-one-color-target validation.
- [x] Forward the optional depth texture to `begin_render_pass`.
- [x] Add frame-between `replace_depth_texture` for host-owned resized/recreated depth targets.
- [x] Do not expose depth target state through `IRenderer`, presentation reports, gameplay APIs, or native backend handles.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/rhi.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md` if present, otherwise relevant Claude rendering/auditor guidance.
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`

- [x] Mark only backend-neutral RHI depth attachment and renderer optional depth target as implemented.
- [x] Keep native D3D12/Vulkan/Metal depth, visible package proof, shadow maps, and production shadowing as follow-up work.

### Task 5: Verification

- [x] Run focused RHI and renderer tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_rhi_tests; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_renderer_tests` failed as expected because `GraphicsPipelineDesc::depth_state`, `DepthStencilStateDesc`, `CompareOp`, `RenderPassDepthAttachment`, `ClearDepthValue`, and the extended `RenderPassDesc` / `RhiFrameRendererDesc` API did not exist yet.
- Reviewer follow-up RED: focused build including `mirakana_rhi_tests`, `mirakana_renderer_tests`, `mirakana_d3d12_rhi_tests`, and `mirakana_backend_scaffold_tests` failed as expected because tests referenced the missing `RhiFrameRenderer::replace_depth_texture` API before production code was added.
- Focused RHI/renderer: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_rhi_tests; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_renderer_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_rhi_tests|mirakana_renderer_tests"` passed.
- Reviewer follow-up focused validation: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_rhi_tests; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_renderer_tests; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_d3d12_rhi_tests; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_backend_scaffold_tests` passed, and `Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_rhi_tests|mirakana_renderer_tests|mirakana_d3d12_rhi_tests|mirakana_backend_scaffold_tests"` passed 4/4 tests.
- Reviewer second follow-up RED: focused D3D12/backend test run failed on `d3d12 rhi frame renderer releases failed swapchain begin once` because D3D12 command-list abandonment double-counted an already manually released frame. The Vulkan mismatch test is present for hosts where the runtime can create the public bridge pipeline; on this host that existing test lane can skip after native pipeline creation diagnostics.
- Reviewer second follow-up focused validation after fixes: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_d3d12_rhi_tests; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_backend_scaffold_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_d3d12_rhi_tests|mirakana_backend_scaffold_tests"` passed 2/2 tests.
- Static/sync: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- Full build/test: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev` passed, and `Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure` passed all 22 tests.
- Shader tooling: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` reported D3D12 DXIL ready, Vulkan SPIR-V ready, DXC SPIR-V CodeGen ready, and Metal library diagnostic-only blockers because `metal` and `metallib` are missing on this host.
- Tidy: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` reported config ok with diagnostic-only compile database blocker for the Visual Studio generator path `out/build/dev/compile_commands.json`.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after reviewer follow-up fixes with existing diagnostic-only host gates for Metal tools, Apple packaging/macOS/Xcode tools, Android release signing/device smoke, and strict tidy compile database availability.
