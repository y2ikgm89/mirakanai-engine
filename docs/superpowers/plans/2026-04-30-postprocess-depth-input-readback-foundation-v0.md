# Postprocess Depth Input Readback Foundation v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a minimal renderer-owned postprocess depth input foundation so the existing color-only `RhiPostprocessFrameRenderer` can bind a sampled depth texture in the postprocess pass and prove deterministic depth-aware output through focused RHI readback.

**Architecture:** Keep the depth input entirely behind `mirakana_renderer` and `mirakana_rhi` first-party contracts. Reuse `TextureUsage::depth_stencil | TextureUsage::shader_resource`, explicit `depth_write` to `shader_read` transitions, and backend-private descriptor writes. Do not expose `IRhiDevice`, native texture/image views, descriptor handles, swapchain frames, SDL3, D3D12, Vulkan, Metal, Dear ImGui, or editor API to gameplay.

**Tech Stack:** C++23, `mirakana_renderer`, `mirakana_rhi`, `mirakana_rhi_d3d12`, `mirakana_rhi_vulkan`, first-party HLSL shader test sources, Vulkan SDK DXC/SPIR-V validation when available.

---

## Context

- Frame Graph/Postprocess v0 currently proves a two-pass color-only path: scene color render target, sampled scene color descriptor, fullscreen postprocess pass, and backend-neutral postprocess/framegraph counters.
- Depth Attachment, Depth Sampling, Vulkan visible sampled-depth, and Directional Shadow Receiver Readback slices now prove backend-private depth sampling and `depth_write` to `shader_read` transitions.
- This slice should connect those existing contracts to postprocess without claiming SSAO, depth of field, fog, package-visible shadows, or a general render graph.

## Constraints

- Keep public API under `mirakana::`; no native handles or backend SDK types in game public APIs.
- Do not add third-party dependencies.
- Keep package-visible shadows, SSAO, depth of field, volumetrics/fog, temporal history, GPU markers, full render graph scheduling, and Metal depth sampling out of this slice.
- Public renderer/RHI headers changed here require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; renderer/RHI/shader changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- End the slice with focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/rhi_postprocess_frame_renderer.hpp` for optional depth input descriptors/state.
- Modify: `engine/renderer/src/rhi_postprocess_frame_renderer.cpp` for descriptor layout, depth transition sequencing, resize/recreate behavior, and framegraph declaration.
- Modify: `tests/unit/renderer_rhi_tests.cpp` for RED/green postprocess depth input contract tests on `NullRhiDevice`.
- Modify: `tests/unit/d3d12_rhi_tests.cpp` for D3D12 depth-aware postprocess readback proof.
- Add or modify: `tests/shaders/*postprocess_depth*.hlsl` for first-party depth-aware test shaders if existing inline shaders are insufficient.
- Modify: `tests/unit/backend_scaffold_tests.cpp` for optional env-gated Vulkan postprocess-depth readback proof if current Vulkan artifacts and runtime can validate it in this slice.
- Modify: docs, `engine/agent/manifest.json`, `tools/check-ai-integration.ps1`, `.agents/skills/rendering-change/SKILL.md`, `.claude/skills/gameengine-rendering/SKILL.md`, `.codex/agents/rendering-auditor.toml`, and `.claude/agents/rendering-auditor.md` for capability honesty.

## Tasks

### Task 1: RED Renderer Postprocess Depth Contract Tests

- [x] Add tests proving the default postprocess renderer remains color-only and creates the existing two descriptor writes.
- [x] Add tests for an opt-in depth input that creates sampled-depth descriptor bindings at stable bindings `2/3`, records a depth read in the frame graph, transitions depth from `depth_write` to `shader_read` for the postprocess pass, and restores a reusable state before the next scene pass.
- [x] Add invalid and lifecycle tests for unsupported depth format, renderer-owned depth texture creation, resize/recreate behavior, and two-frame reuse. The final implementation intentionally does not accept external/foreign depth handles in this slice, so zero/foreign handle tests were superseded by renderer-owned validation.
- [x] Run focused renderer tests and verify the new tests fail before production implementation.

### Task 2: Renderer Implementation

- [x] Add optional depth input fields to `RhiPostprocessFrameRendererDesc` without changing the game public API or exposing native handles.
- [x] Update descriptor set layout/write policy to bind scene color, scene sampler, optional depth texture, and optional depth sampler deterministically.
- [x] Update frame sequencing so depth state transitions are explicit and compatible with `NullRhiDevice`, D3D12, and Vulkan contracts.
- [x] Re-run focused renderer tests until they pass.

### Task 3: D3D12 Depth-Aware Postprocess Readback Proof

- [x] Add or reuse D3D12 shader helpers that sample depth in the postprocess pass and produce a deterministic output difference.
- [x] Add a D3D12 RHI test that writes depth, runs a depth-aware postprocess pass, copies the output to CPU readback, and asserts the expected depth-derived pixel value.
- [x] Run focused D3D12 RHI tests.

### Task 4: Vulkan Env-Gated Depth-Aware Postprocess Proof

- [x] Decide whether current Vulkan shader artifacts can cover postprocess-depth proof in this slice without broad package integration.
- [x] If host-feasible, add first-party macro-selected Vulkan postprocess-depth HLSL variants and env-gated `mirakana_backend_scaffold_tests` coverage.
- [x] Compile local Vulkan 1.3 SPIR-V artifacts, validate with `spirv-val --target-env vulkan1.3`, and run strict focused `mirakana_backend_scaffold_tests` with all required env vars.
- [x] If Vulkan strict proof is not host-feasible, record the exact blocker and keep the implementation D3D12/NullRHI-proven only. Not applicable: strict Vulkan proof is host-feasible and passing on this Windows/Vulkan SDK host.

### Task 5: Docs, Manifest, Guidance, and Validation

- [x] Update docs/manifest/skills/subagents/Claude guidance to state postprocess depth input foundation status honestly.
- [x] Strengthen `tools/check-ai-integration.ps1` for postprocess depth wording and any new Vulkan env vars.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, focused build/tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `cpp-reviewer`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Move this plan from Active slice to Completed in `docs/superpowers/plans/README.md`.
- [ ] Select the next host-feasible production slice without waiting for user confirmation.

## Validation Results

- RED: initial renderer tests failed before `RhiPostprocessFrameRendererDesc::enable_depth_input` and depth binding helpers existed.
- PASS: focused `mirakana_renderer_tests` after renderer implementation.
- PASS: focused `mirakana_d3d12_rhi_tests` with depth-aware postprocess CPU readback.
- PASS: local `dxc -spirv -fspv-target-env=vulkan1.3` compile for `tests/shaders/vulkan_postprocess_depth.hlsl` vertex/fragment variants.
- PASS: `spirv-val --target-env vulkan1.3` for the local postprocess-depth SPIR-V artifacts.
- PASS: strict env-gated `mirakana_backend_scaffold_tests` with depth-write, sampled-depth, shadow receiver, and postprocess-depth SPIR-V artifacts configured.
- RED/PASS: `cpp-reviewer` found a pre-present swapchain-frame cleanup bug in `RhiPostprocessFrameRenderer::end_frame`; added a failing depth-restore exception test and fixed cleanup so present-before-failure is distinguished from swapchain-pass-begun-before-present, with manual release before command-list abandonment.
- PASS: reviewer-fix focused `mirakana_renderer_tests`.
- PASS: focused build and CTest for `mirakana_renderer_tests`, `mirakana_d3d12_rhi_tests`, and `mirakana_backend_scaffold_tests`.
- PASS: strict env-gated `mirakana_backend_scaffold_tests` with depth-write, sampled-depth, shadow receiver, and postprocess-depth SPIR-V artifacts configured after reviewer fix.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` for D3D12 DXIL and Vulkan SPIR-V readiness; Metal `metal` / `metallib` remain missing and diagnostic-only.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`; known diagnostic-only host gates remain Metal shader/library tools, Apple packaging/Xcode, Android release signing/device smoke, and strict clang-tidy compile database for the Visual Studio generator.

## Remaining Follow-Up

- Package-visible desktop runtime depth-aware postprocess effects and installed smoke proof.
- SSAO, depth of field, volumetrics/fog, temporal history, GPU markers, render-graph scheduling, postprocess editor controls, and GPU profiler integration.
- Metal native sampled-depth postprocess validation behind macOS/Xcode host gates.

## Done When

- `RhiPostprocessFrameRenderer` can opt into a first-party sampled depth input without native handle leakage.
- Focused tests prove descriptor layout, state transitions, resize behavior, and color-only compatibility.
- D3D12 proves a depth-aware postprocess output through CPU readback.
- Vulkan proof is either strict env-gated and passing on this host or recorded as host/toolchain-gated with exact requirements.
- Docs, manifest, skills, subagents, and Claude guidance distinguish this foundation from package-visible production effects.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or concrete host blockers are recorded.
