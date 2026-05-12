# D3D12 Native Depth Proof v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote RHI Depth Attachment Contract v0 into the first native D3D12 depth-stencil view and depth-test proof so future shadow-map and production depth slices have a validated Windows backend path.

**Architecture:** Keep D3D12 COM objects, descriptor heaps, DSV handles, and resource state details private to `engine/rhi/d3d12`. The public API remains `mirakana::rhi::RenderPassDepthAttachment`, `DepthStencilStateDesc`, `Format::depth24_stencil8`, and first-party handles only. Prove native behavior through D3D12 backend tests that observe color output after depth-tested draws; do not expose native handles, add public game API, implement shadows, or claim Vulkan/Metal parity in this slice.

**Tech Stack:** C++23, Windows D3D12 backend, existing HLSL bytecode helpers, D3D12 readback tests, no new third-party dependencies.

---

## Context

- The backend-neutral depth contract is implemented and validated in `mirakana_rhi`/`NullRhiDevice`.
- Before this slice, D3D12 rejected public depth attachments and public depth pipeline state until native DSV support existed.
- Existing D3D12 tests already prove render-target clears, triangle draws, readback, pipeline validation, and texture render passes.

## Constraints

- Keep native D3D12/DXGI/Win32/COM details behind `engine/rhi/d3d12`.
- Do not expose DSV descriptors, descriptor heaps, raw resources, or native handles through public RHI/game APIs.
- Do not implement Vulkan/Metal depth, postprocess depth, shadow maps, depth sampling, or packaged visible depth smokes in this slice.
- Public header/backend interop changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- RHI/shader/backend changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.

## Done When

- [x] D3D12 can create `Format::depth24_stencil8` textures with `TextureUsage::depth_stencil` as private native depth resources.
- [x] D3D12 creates private DSV descriptors for depth-stencil textures without exposing descriptor handles.
- [x] D3D12 render passes with `RenderPassDepthAttachment` bind color RTV plus DSV, clear depth when requested, and preserve the existing color-only path.
- [x] D3D12 graphics pipeline creation supports the public `DepthStencilStateDesc` for native depth-enabled pipelines while still rejecting invalid depth state.
- [x] D3D12 command-list binding validates active color/depth formats against the bound pipeline.
- [x] Tests prove depth-tested color output through readback and cover invalid depth texture usage/state/format/pipeline mismatches.
- [x] Docs, roadmap, gap analysis, manifest, rendering skills, and Codex/Claude guidance distinguish D3D12 native depth proof from Vulkan/Metal depth, depth sampling, postprocess depth, and shadows.
- [x] Focused D3D12/RHI/renderer validation, API boundary, shader toolchain diagnostics, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED D3D12 Native Depth Tests

**Files:**
- Modify: `tests/unit/d3d12_rhi_tests.cpp`

- [x] Add a D3D12 texture render pass test that draws overlapping geometry with different depth values and verifies readback color is depth-tested, not draw-order-only.
- [x] Add tests for D3D12 depth attachment rejection on wrong usage, wrong format, wrong state, and color/depth extent mismatch where native ownership can be created.
- [x] Add tests proving depth pipeline cannot bind into color-only passes and no-depth pipelines cannot bind into depth passes.
- [x] Run focused D3D12 build/test and verify failure before native DSV/depth-state implementation.

### Task 2: D3D12 Depth Resource And DSV Ownership

**Files:**
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp` only if private backend declarations need extension; keep public game/RHI headers native-free.

- [x] Map `Format::depth24_stencil8` to the appropriate D3D12 depth format for resource creation and DSV views.
- [x] Create a private DSV descriptor heap/range for committed depth-stencil textures.
- [x] Track whether a texture has a DSV and keep ownership lifetime tied to the backend resource record.
- [x] Keep render-target RTV and depth DSV allocation paths independent.

### Task 3: D3D12 Render Pass And Pipeline Depth State

**Files:**
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`

- [x] Translate `DepthStencilStateDesc` into D3D12 PSO depth-stencil state.
- [x] Allow public depth pipeline state only when `depth_format == Format::depth24_stencil8` and depth state is valid.
- [x] Bind RTV+DSV for render passes with depth attachments.
- [x] Clear DSV depth when `LoadAction::clear` and validate clear depth range.
- [x] Validate active render pass color/depth formats at pipeline bind.
- [x] Preserve existing color-only D3D12 behavior and tests.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/rhi.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`

- [x] Mark D3D12 native depth proof as implemented only after tests pass.
- [x] Keep Vulkan/Metal native depth, shadow maps, depth sampling, and package-visible shadow claims as follow-up work.

### Task 5: Verification

- [x] Run focused D3D12/RHI/renderer tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Execution Evidence

- RED: focused D3D12 build/CTest failed before implementation because D3D12 rejected `Format::depth24_stencil8` texture creation for the new native depth tests.
- Implementation: `mirakana_rhi_d3d12` now keeps DSV records private, creates `depth24_stencil8` depth-stencil textures only with `TextureUsage::depth_stencil`, binds RTV+DSV render passes, clears DSV depth, maps `DepthStencilStateDesc` into D3D12 PSO state, and validates active color/depth formats before pipeline binding.
- Focused validation: `. (Join-Path (Get-Location) 'tools/common.ps1'); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_d3d12_rhi_tests mirakana_rhi_tests mirakana_renderer_tests` passed.
- Focused validation: `. (Join-Path (Get-Location) 'tools/common.ps1'); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R 'mirakana_d3d12_rhi_tests|mirakana_rhi_tests|mirakana_renderer_tests'` passed, including `mirakana_d3d12_rhi_tests`, `mirakana_rhi_tests`, and `mirakana_renderer_tests`.
- Review: read-only rendering auditor found no implementation blockers and flagged only plan-registry completion drift to resolve after full validation. Read-only cpp-reviewer found no depth-proof/API-boundary blocker, flagged `format-check`, and requested explicit rejection tests for `depth_stencil | shader_resource` and `depth_stencil | copy_source`; both were fixed.
- Static validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed. Shader toolchain remains diagnostic-only for Metal because `metal` and `metallib` are missing on this Windows host.
- Default validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Diagnostic-only host gates remain: Metal `metal`/`metallib` missing, Apple packaging requires macOS/Xcode with `xcodebuild`/`xcrun`, Android release signing is not configured, Android device smoke is not connected, and strict tidy is gated by the Visual Studio generator compile database.
