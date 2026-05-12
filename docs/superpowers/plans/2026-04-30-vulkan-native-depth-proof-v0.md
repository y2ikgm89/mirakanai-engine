# Vulkan Native Depth Proof v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote RHI Depth Attachment Contract v0 into the first native Vulkan depth image-view/layout/dynamic-rendering proof so D3D12 and Vulkan share a validated depth foundation before shadow-map work.

**Architecture:** Keep `VkImage`, `VkImageView`, `VkImageMemoryBarrier2`, `VkRenderingAttachmentInfo`, `VkPipelineDepthStencilStateCreateInfo`, `VkPipelineRenderingCreateInfo`, layout transitions, and function pointers private to `engine/rhi/vulkan`. The public API remains `mirakana::rhi::RenderPassDepthAttachment`, `DepthStencilStateDesc`, `Format::depth24_stencil8`, and first-party handles only. Do not expose Vulkan handles, implement Metal depth, add depth sampling, implement shadows, or change runtime/game public APIs in this slice.

**Tech Stack:** C++23, existing SDK-independent Vulkan runtime owners, Vulkan 1.3 dynamic rendering and synchronization2 wrappers, existing SPIR-V shader artifact validation/test environment, no new third-party dependencies.

---

## Context

- RHI Depth Attachment Contract v0 is implemented and validated in `mirakana_rhi`/`NullRhiDevice`.
- D3D12 Native Depth Proof v0 is implemented with private DSVs and readback-observable depth-tested output.
- Before this slice, Vulkan had dynamic rendering and texture/render-target draw/readback paths, but the backend-neutral `IRhiDevice` path still failed fast on public depth attachments and public depth pipeline state.
- Existing Vulkan tests are environment-gated when real SPIR-V artifacts or local runtime support are required; host-feasible non-runtime planning tests should still run deterministically.

## Constraints

- Keep native Vulkan handles, layouts, barriers, image views, and command function pointers behind `engine/rhi/vulkan`.
- Preserve Vulkan runtime planning before native recording: validate image aspect, layout transition, dynamic rendering attachment, and pipeline readiness before promoting public depth use.
- Do not expose native handles, add public game API, implement Vulkan depth sampling, postprocess depth, shadow maps, Metal parity, or packaged visible shadow smokes in this slice.
- Public header/backend interop changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- RHI/shader/backend changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.

## Done When

- [x] Vulkan can create `Format::depth24_stencil8` textures with `TextureUsage::depth_stencil` as private native depth resources or reports explicit runtime/toolchain blockers.
- [x] Vulkan creates private depth image views for depth-stencil textures without exposing handles.
- [x] Vulkan command recording can transition depth textures into the correct depth-attachment layout/state through first-party resource-state terms.
- [x] Vulkan dynamic rendering render passes with `RenderPassDepthAttachment` bind color image view plus depth attachment info, clear depth when requested, and preserve existing color-only behavior.
- [x] Vulkan graphics pipeline creation maps public `DepthStencilStateDesc` into native depth-stencil pipeline state and validates active render-pass color/depth formats.
- [x] Tests cover native-planning validation and, when the host/runtime/SPIR-V lane is available, depth-tested readback proof.
- [x] Docs, roadmap, gap analysis, manifest, rendering skills, and Codex/Claude guidance distinguish Vulkan native depth proof from Metal depth, depth sampling, postprocess depth, and shadows.
- [ ] Focused Vulkan/RHI/renderer validation, API boundary, shader toolchain diagnostics, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Vulkan Native Depth Tests

**Files:**
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `tests/unit/vulkan_rhi_tests.cpp` if present/appropriate

- [x] Add deterministic planning tests for Vulkan depth texture/image-view/dynamic-rendering readiness.
- [x] Add tests for Vulkan rejection on wrong depth usage, wrong depth format, wrong state/layout, clear-depth range, and color/depth extent mismatch.
- [x] Add tests proving depth pipelines cannot bind into color-only passes and no-depth pipelines cannot bind into depth passes at the `IRhiDevice` layer.
- [x] Add environment-gated readback proof using overlapping depth-ordered geometry when local Vulkan runtime and SPIR-V artifacts are available.
- [x] Run focused Vulkan/RHI build/test and verify failure before native depth implementation or record exact host gate.

### Task 2: Vulkan Depth Resource And View Ownership

**Files:**
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: Vulkan backend private helpers only; keep public game/RHI headers native-free unless an existing first-party contract needs a bug fix.

- [x] Map `Format::depth24_stencil8` to the Vulkan depth/stencil format and depth aspect mask through first-party helpers.
- [x] Create private depth image views for depth-stencil textures.
- [x] Track whether a texture has a depth attachment view and keep lifetime tied to the runtime texture/resource record.
- [x] Keep color image-view and depth image-view allocation paths independent.

### Task 3: Vulkan Render Pass And Pipeline Depth State

**Files:**
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`

- [x] Translate `DepthStencilStateDesc` into native Vulkan depth-stencil pipeline state.
- [x] Allow public depth pipeline state only when `depth_format == Format::depth24_stencil8` and depth state is valid.
- [x] Transition depth attachments to the appropriate depth attachment layout/state before dynamic rendering.
- [x] Bind color plus depth attachments in dynamic rendering info.
- [x] Clear depth when requested and validate clear depth in `[0, 1]`.
- [x] Validate active render pass color/depth formats at pipeline bind.
- [x] Preserve existing color-only Vulkan behavior and tests.

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

- [x] Mark Vulkan native depth proof as implemented only after tests pass or host-gated if runtime/SPIR-V proof cannot run.
- [x] Keep Metal native depth, depth sampling, postprocess depth, shadow maps, and package-visible shadow claims as follow-up work.

### Task 5: Verification

- [x] Run focused Vulkan/RHI/renderer tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Evidence

- `cmake --build --preset dev --target mirakana_backend_scaffold_tests mirakana_rhi_tests mirakana_renderer_tests`: PASS.
- `ctest --preset dev --output-on-failure -R 'mirakana_backend_scaffold_tests|mirakana_rhi_tests|mirakana_renderer_tests'`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: D3D12 DXIL ready, Vulkan SPIR-V ready, Metal `metal` / `metallib` missing as diagnostic-only host gate.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS after the known sandbox `CreateFileW stdin failed with 5` vcpkg/7zip blocker was reproduced and the same command was rerun with elevated permissions.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`: PASS after the known sandbox `CreateFileW stdin failed with 5` vcpkg/7zip blocker was reproduced and the same command was rerun with elevated permissions.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS with existing diagnostic-only Metal/Apple/signing/device/tidy host gates.

## Review Follow-Ups

- `cpp-reviewer` found that the new visible depth/readback gate initially broke SDL3/Vulkan presentation mapping. Fixed by probing depth dynamic-rendering/barrier readiness inside the presentation mapping helper before `create_rhi_device`.
- `cpp-reviewer` found that `create_rhi_device` trusted a hand-built `supported=true` mapping plan. Fixed by requiring every mapping readiness bit, including visible depth/readback, at the factory boundary.
- `cpp-reviewer` requested lower-level texture barrier coverage. Added a native runtime test for incompatible direct `record_runtime_texture_barrier` usage/format transitions.
