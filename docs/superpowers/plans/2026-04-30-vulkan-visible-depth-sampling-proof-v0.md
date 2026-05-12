# Vulkan Visible Depth Sampling Readback Proof v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove that the Vulkan RHI bridge can write a first-party depth texture, transition it from `depth_write` to `shader_read`, sample it in a later fragment pass, and verify the sampled value through CPU readback.

**Architecture:** Keep the proof behind `MK_rhi_vulkan` private runtime owners and the backend-neutral `IRhiDevice` contract. Use `Format::depth24_stencil8`, `TextureUsage::depth_stencil | TextureUsage::shader_resource`, `ResourceState::depth_write`, `ResourceState::shader_read`, sampled texture descriptors, and samplers without exposing `VkImage`, `VkImageView`, layouts, descriptor objects, SDL3, OS handles, or backend handles to game/runtime-host public APIs.

**Tech Stack:** C++23, `MK_rhi_vulkan`, env-gated Vulkan RHI tests, first-party HLSL shader test sources compiled to SPIR-V by the host Vulkan SDK DXC, official Khronos Vulkan synchronization guidance.

---

## Context

- Depth Sampling Contract v0 completed NullRHI and D3D12 sampled-depth proof, and Vulkan create/barrier/descriptor coverage.
- Vulkan currently proves depth-ordered rendering through readback, but not shader sampling of the depth image after the `depth_write` to `shader_read` transition.
- Khronos synchronization guidance for depth attachment write followed by fragment shader sampling uses early/late fragment test source stages, depth-stencil attachment write access, fragment shader destination stage, shader-read access, and an attachment-to-read-only image layout transition.
- Shadow Map Foundation v0 intentionally stops at backend-neutral pass planning; this slice provides the Vulkan sampling proof required before visible shadows or postprocess-depth rendering can be claimed.

## Constraints

- Keep public game API under `mirakana::` and do not expose Vulkan, SDL3, OS window handles, GPU handles, Dear ImGui, or editor APIs.
- Do not add third-party dependencies. Test shader source is first-party repo content; compiled SPIR-V remains host-generated validation input.
- Prefer clean C++23 implementation over backward-compatibility shims.
- Treat missing Vulkan runtime or missing SPIR-V env vars as host/config gates; partial env configuration must fail clearly.
- Public header/backend interop changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; renderer/RHI/shader proof changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`; every completed slice ends with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Files

- Add: `tests/shaders/vulkan_depth_sampling.hlsl` for macro-selected `main` entry point depth-write and depth-sample proof shader variants.
- Modify: `tests/unit/backend_scaffold_tests.cpp` for the env-gated Vulkan sampled-depth readback test.
- Modify as needed: `engine/rhi/vulkan/src/vulkan_backend.cpp` if the proof exposes a backend gap.
- Modify: `docs/testing.md`, `docs/rhi.md`, `docs/architecture.md`, `docs/roadmap.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, `tools/check-ai-integration.ps1`, `.agents/skills/rendering-change/SKILL.md`, `.claude/skills/gameengine-rendering/SKILL.md`, `.codex/agents/rendering-auditor.toml`, and `.claude/agents/rendering-auditor.md` for capability honesty and guidance sync.

## Tasks

### Task 1: RED Test and Shader Inputs

- [x] Add first-party HLSL test shaders with macro-selected `main` entry point variants for depth-write vertex/fragment and depth-sample vertex/fragment artifacts.
- [x] Add an env-gated Vulkan RHI test requiring `MK_VULKAN_TEST_DEPTH_VERTEX_SPV`, `MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV`, `MK_VULKAN_TEST_DEPTH_SAMPLE_VERTEX_SPV`, and `MK_VULKAN_TEST_DEPTH_SAMPLE_FRAGMENT_SPV` when any one is configured.
- [x] In the test, write depth into a `depth24_stencil8` texture with `depth_stencil | shader_resource`, transition `depth_write` to `shader_read`, sample it in a second pass, copy the color target to a readback buffer, and validate the center pixel depth value.
- [x] Run focused `MK_backend_scaffold_tests` without env vars to verify the default skip path remains green.
- [x] Compile the first-party shaders to SPIR-V with the Vulkan SDK DXC and run the focused test with all env vars configured to capture the first failure.

### Task 2: Backend Fixes if Required

- [x] Fix only backend-private Vulkan behavior exposed by the proof, such as sampled-depth image layout, depth-aspect descriptor view use, state transition recording, or descriptor binding compatibility.
- [x] Keep native `Vk*` handles, layouts, descriptor writes, image views, and synchronization structures private to `MK_rhi_vulkan`.
- [x] Re-run the strict focused Vulkan proof until it passes.

### Task 3: Docs, Manifest, and Agent Guidance

- [x] Update docs to state that Vulkan visible depth-sampling readback is implemented as an env-gated RHI proof, while visible shadows, comparison filtering, postprocess depth, cascades, atlases, package-visible shadows, and Metal sampling remain follow-up work.
- [x] Update `engine/agent/manifest.json` validation recipes and capability text with the new `MK_VULKAN_TEST_DEPTH_SAMPLE_*` env vars without marking planned features as implemented.
- [x] Update Codex and Claude rendering skills/subagents so future agents distinguish Vulkan sampled-depth proof from visible shadow support.
- [x] Strengthen `tools/check-ai-integration.ps1` if needed so the new Vulkan depth-sampling guidance does not drift.

### Task 4: Validation and Review

- [x] Run focused build for `MK_backend_scaffold_tests`.
- [x] Run focused CTest without env vars.
- [x] Run focused CTest with locally compiled SPIR-V env vars.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers or backend interop surfaces changed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Request `cpp-reviewer` read-only review and fix any accepted findings.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 5: Slice Closure

- [x] Record validation results, host-gated blockers, and remaining follow-up features.
- [x] Move this plan from Active slice to Completed in `docs/superpowers/plans/README.md`.
- [x] Select the next host-feasible production slice without waiting for user confirmation.

## Validation Results

- First focused build: `Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_backend_scaffold_tests` PASS.
- Default focused CTest without SPIR-V env vars: `ctest --test-dir out\build\dev -C Debug --output-on-failure -R "MK_backend_scaffold_tests"` PASS.
- Strict SPIR-V proof: compiled `tests/shaders/vulkan_depth_sampling.hlsl` into four Vulkan 1.3 SPIR-V artifacts with Vulkan SDK DXC using macro-selected `main` variants and ran `MK_backend_scaffold_tests` with `MK_VULKAN_TEST_DEPTH_VERTEX_SPV`, `MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV`, `MK_VULKAN_TEST_DEPTH_SAMPLE_VERTEX_SPV`, and `MK_VULKAN_TEST_DEPTH_SAMPLE_FRAGMENT_SPV` configured; PASS.
- Reviewer follow-up: `cpp-reviewer` found an entry-point contract mismatch, unchecked plan evidence, and incomplete AI integration drift checks. Fixes changed the first-party shader to macro-selected `main` variants, kept both existing depth-ordered and new sampled-depth tests on the same `MK_VULKAN_TEST_DEPTH_*` contract, and strengthened `tools/check-ai-integration.ps1` to require all four env vars.
- Static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` PASS.
- Shader toolchain: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` reports D3D12 DXIL ready, Vulkan SPIR-V ready, DXC SPIR-V CodeGen ready, and diagnostic-only Metal blockers because `metal` / `metallib` are missing on this Windows host.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with existing diagnostic-only host gates for Metal library tools, Apple packaging/Xcode, Android release signing/device smoke, and strict clang-tidy compile database generation.

## Remaining Follow-Up

- Visible/package shadow rendering, comparison sampling/filtering, cascades, atlases, and postprocess-depth rendering remain separate renderer/package slices.
- Metal native depth recording/sampling remains Apple-host-gated.

## Next Slice

- Selected next host-feasible production slice: Directional Shadow Receiver Readback Proof v0. The first slice will keep package-visible shadow rendering as follow-up while proving a backend-neutral shadow receiver descriptor/bias policy plus D3D12/Vulkan readback evidence that a shader-readable shadow depth texture can darken a receiver pass without exposing native handles.

## Done When

- Vulkan has a real env-gated `IRhiDevice` proof that writes depth, transitions to shader-read, samples the depth texture, and validates a readback pixel.
- The default test lane remains skip-safe when SPIR-V artifacts are not configured and fails clearly on partial artifact configuration.
- Docs, manifest, skills, and subagent guidance no longer list Vulkan visible depth-sampling readback as a missing proof, and still do not claim production visible shadows.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` have passed or concrete host/tool blockers are recorded.
