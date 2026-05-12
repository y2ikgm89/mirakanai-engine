# Depth Sampling Contract v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Allow first-party depth textures to be used as shader-readable sampled textures after an explicit depth-write to shader-read transition, proving the contract on NullRHI plus D3D12 and Vulkan backend-private implementations.

**Architecture:** Reuse the existing backend-neutral `TextureUsage::shader_resource`, `ResourceState::shader_read`, and `DescriptorType::sampled_texture` public contracts. Keep all D3D12 typeless resource/view format handling and Vulkan image usage/layout/view behavior inside backend implementations, with no native handle exposure to gameplay or runtime host public APIs. This slice does not implement visible shadows, PCF/comparison samplers, cascades, shadow atlases, postprocess depth effects, or Metal validation.

**Tech Stack:** C++23, `mirakana_rhi`, `mirakana_rhi_d3d12`, `mirakana_rhi_vulkan`, focused unit/host-gated tests, official Direct3D 12 and Khronos Vulkan synchronization guidance.

---

## Context

- Shadow Map Foundation v0 currently plans a `depth24_stencil8` depth target but cannot yet make that texture shader-readable.
- Official D3D12 guidance for shadow maps separates the depth write pass from a later pixel-shader read using `DEPTH_WRITE -> PIXEL_SHADER_RESOURCE`.
- Official Vulkan synchronization guidance uses depth attachment writes in early/late fragment tests, then a barrier to shader read with read-only image layout for later sampling.
- Current D3D12/Vulkan implementations intentionally reject `TextureUsage::depth_stencil | TextureUsage::shader_resource`.

## Constraints

- Keep public game API under `mirakana::` and do not expose SDL3, OS window handles, GPU handles, D3D12, Vulkan, Metal, Dear ImGui, or editor APIs.
- Do not add third-party dependencies.
- Prefer clean C++23 implementation over backward-compatibility shims.
- Treat Metal and Apple validation as host-gated.
- Public header/backend interop changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; renderer/RHI changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`; every completed slice ends with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Files

- Modify: `tests/unit/rhi_tests.cpp` for NullRHI sampled-depth descriptor/transition validation.
- Modify: `tests/unit/d3d12_rhi_tests.cpp` for D3D12 sampled-depth texture creation, descriptor write, state validation, and readback proof.
- Modify: `tests/unit/backend_scaffold_tests.cpp` for Vulkan sampled-depth create-plan behavior.
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp` for D3D12 depth+shader-resource validation, typeless resource format, DSV format, and depth SRV descriptor creation.
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp` for Vulkan depth+sampled usage planning.
- Modify: `docs/roadmap.md`, `docs/rhi.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, `.agents/skills/rendering-change/SKILL.md`, `.claude/skills/gameengine-rendering/SKILL.md`, `.codex/agents/rendering-auditor.toml`, `.claude/agents/rendering-auditor.md` for capability honesty.

## Tasks

### Task 1: RED Tests

- [x] Add NullRHI tests proving a `depth24_stencil8` texture with `depth_stencil | shader_resource` can be bound as a sampled texture, transitions to `shader_read`, is rejected as a depth attachment while in shader-read state, and is accepted again after transitioning back to `depth_write`.
- [x] Update D3D12 tests so `depth_stencil | shader_resource` creation and sampled descriptor update are expected to succeed while `depth_stencil | copy_source` remains rejected.
- [x] Add a D3D12 readback proof that writes depth in one pass, transitions it to `shader_read`, samples it in a later pass, and validates the sampled depth value through CPU readback.
- [x] Update Vulkan create-plan tests so `depth_stencil | shader_resource` is supported with both `depth_stencil_attachment` and `sampled` usage while depth copy remains rejected.
- [x] Run focused tests and record the expected RED failures:
  - `cmake --build --preset dev --target mirakana_rhi_tests mirakana_d3d12_rhi_tests mirakana_backend_scaffold_tests`
  - `ctest --preset dev --output-on-failure -R "mirakana_rhi_tests|mirakana_d3d12_rhi_tests|mirakana_backend_scaffold_tests"`

### Task 2: Backend Implementation

- [x] In D3D12, allow exactly `depth_stencil` or `depth_stencil | shader_resource` for `Format::depth24_stencil8` textures while keeping copy/storage/render-target combinations rejected.
- [x] In D3D12, create sampled depth textures with `DXGI_FORMAT_R24G8_TYPELESS`, create the DSV with `DXGI_FORMAT_D24_UNORM_S8_UINT`, and create sampled SRVs with `DXGI_FORMAT_R24_UNORM_X8_TYPELESS`.
- [x] In Vulkan, allow depth textures with `depth_stencil | shader_resource` in the runtime create plan and map that to sampled plus depth-stencil attachment image usage.
- [x] Keep native formats, image views, descriptors, barriers, and backend state private to RHI backends.

### Task 3: Focused Validation

- [x] Run `cmake --build --preset dev --target mirakana_rhi_tests mirakana_d3d12_rhi_tests mirakana_backend_scaffold_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R "mirakana_rhi_tests|mirakana_d3d12_rhi_tests|mirakana_backend_scaffold_tests"`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.

### Task 4: Docs, Manifest, and Agent Guidance

- [x] Update roadmap/RHI/gap-analysis docs to state that sampled depth texture contracts are implemented for NullRHI, D3D12, and Vulkan planning, while visible shadows and postprocess depth remain follow-up work.
- [x] Update `engine/agent/manifest.json` without marking visible shadows as implemented.
- [x] Update Codex and Claude rendering guidance so future agents use sampled depth textures only through backend-neutral RHI contracts.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.

### Task 5: Slice Closure

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation results, any host-gated blockers, and next production slice candidate.
- [x] Move this plan from Active slice to Completed in `docs/superpowers/plans/README.md`.

## Validation Results

- Focused build: `Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_rhi_tests mirakana_d3d12_rhi_tests mirakana_backend_scaffold_tests mirakana_renderer_tests mirakana_scene_renderer_tests` PASS.
- Focused CTest: `ctest --test-dir out\build\dev -C Debug --output-on-failure -R "mirakana_rhi_tests|mirakana_d3d12_rhi_tests|mirakana_backend_scaffold_tests|mirakana_renderer_tests|mirakana_scene_renderer_tests"` PASS, 5/5 tests.
- Reviewer follow-up: `cpp-reviewer` found two gaps; follow-up fixes added NullRHI depth usage creation rejection and Vulkan sampled-depth descriptor update coverage. Reviewer re-check returned no findings.
- Follow-up focused CTest: `ctest --test-dir out\build\dev -C Debug --output-on-failure -R "mirakana_rhi_tests|mirakana_backend_scaffold_tests"` PASS, 2/2 tests.
- Static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` PASS.
- Shader toolchain: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` reports D3D12 DXIL ready, Vulkan SPIR-V ready, DXC SPIR-V CodeGen ready, and diagnostic-only Metal blockers because `metal` / `metallib` are missing on this Windows host.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with existing diagnostic-only host gates for Metal library tools, Apple packaging/Xcode, Android release signing/device smoke, and strict clang-tidy compile database generation.

## Next Candidate

- Prefer a host-feasible renderer slice that consumes the sampled-depth contract without overclaiming production shadows: a backend-neutral visible shadow receiver planning/pass slice or a Vulkan visible depth-sampling readback proof, depending on subagent risk review.

## Done When

- `TextureUsage::depth_stencil | TextureUsage::shader_resource` is accepted consistently by NullRHI and D3D12, and planned as sampled depth attachment usage by Vulkan.
- Sampled depth descriptor writes are covered by focused tests.
- Render pass depth attachment use still requires `ResourceState::depth_write`.
- Depth copy/storage/render-target combinations remain rejected.
- Docs, manifest, skills, and subagent guidance distinguish sampled depth support from visible/production shadows.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` have passed or concrete host/tool blockers are recorded.
