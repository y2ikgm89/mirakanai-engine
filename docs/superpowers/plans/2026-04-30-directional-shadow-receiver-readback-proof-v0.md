# Directional Shadow Receiver Readback Proof v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove a minimal directional shadow receiver path by describing a backend-neutral receiver policy and validating that a shader-readable shadow depth texture can darken a receiver pass through D3D12 and Vulkan RHI readback.

**Architecture:** Extend `MK_renderer` shadow planning with first-party receiver descriptor/bias policy and descriptor layout planning while keeping native resources, D3D12/Vulkan descriptors, image views, layouts, and samplers behind RHI backends. Use existing sampled-depth contracts and host-gated shader artifacts for visible backend proof; do not expose native handles or claim package-visible production shadows.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, `MK_rhi_d3d12`, `MK_rhi_vulkan`, first-party HLSL shader test sources, Vulkan SDK DXC/SPIR-V validation when available.

---

## Context

- Shadow Map Foundation v0 can plan one directional shadow-casting light and a shader-readable `depth24_stencil8` shadow texture, but it does not describe receiver shader inputs or prove visible occlusion.
- Depth Sampling Contract v0 plus D3D12 and Vulkan sampled-depth readback proofs show that both backends can sample a previously written depth value.
- This slice bridges those pieces by adding the receiver-side policy and proving a receiver pass darkens pixels when receiver depth is behind sampled shadow depth.

## Constraints

- Keep public API under `mirakana::`; no SDL3, OS, GPU, D3D12, Vulkan, Metal, Dear ImGui, or editor API exposure.
- Do not add third-party dependencies.
- Keep comparison filtering, cascades, atlases, PCF, normal bias from mesh normals, package-visible shadows, and Metal shadow sampling out of this slice.
- Public renderer/RHI headers changed here require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; renderer/RHI/shader changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- End the slice with focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/shadow_map.hpp` for receiver plan types.
- Modify: `engine/renderer/src/shadow_map.cpp` for deterministic receiver planning.
- Modify: `tests/unit/renderer_rhi_tests.cpp` for RED/green receiver plan tests.
- Modify: `tests/unit/d3d12_rhi_tests.cpp` for D3D12 directional shadow receiver readback proof.
- Add: `tests/shaders/vulkan_shadow_receiver.hlsl` for macro-selected Vulkan shadow receiver SPIR-V variants.
- Modify: `tests/unit/backend_scaffold_tests.cpp` for env-gated Vulkan directional shadow receiver readback proof.
- Modify: docs, `engine/agent/manifest.json`, `tools/check-ai-integration.ps1`, `.agents/skills/rendering-change/SKILL.md`, `.claude/skills/gameengine-rendering/SKILL.md`, `.codex/agents/rendering-auditor.toml`, and `.claude/agents/rendering-auditor.md` for capability honesty.

## Tasks

### Task 1: RED Receiver Policy Tests

- [x] Add tests for a directional shadow receiver plan that consumes a successful `ShadowMapPlan`, emits a sampled-depth plus sampler descriptor layout at stable bindings, preserves receiver count, and records default bias/intensity values.
- [x] Add invalid-plan tests for failed shadow plans, non-finite/out-of-range bias, invalid lit/shadow intensities, and shadow intensity greater than lit intensity.
- [x] Run focused renderer tests and verify the new tests fail before production implementation.

### Task 2: Receiver Policy Implementation

- [x] Add receiver policy types and diagnostics to `mirakana/renderer/shadow_map.hpp`.
- [x] Implement deterministic `build_shadow_receiver_plan` and `has_shadow_receiver_diagnostic` in `shadow_map.cpp`.
- [x] Re-run focused renderer tests until they pass.

### Task 3: D3D12 Visible Receiver Proof

- [x] Add a D3D12 RHI test shader helper that samples `Texture2D<float>` shadow depth and applies a fixed receiver-depth comparison with a small bias.
- [x] Add a D3D12 RHI test that writes a shadow depth texture, transitions it to `shader_read`, samples it in a receiver pass, copies the receiver target to CPU readback, and asserts the center pixel is darkened.
- [x] Run focused D3D12 RHI tests.

### Task 4: Vulkan Env-Gated Receiver Proof

- [x] Add first-party macro-selected `main` HLSL shader variants for Vulkan shadow receiver vertex/fragment artifacts.
- [x] Add an env-gated Vulkan RHI test requiring `MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV` and `MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV` plus the existing depth-write artifacts when any one is configured.
- [x] Compile local Vulkan 1.3 SPIR-V artifacts and run strict focused `MK_backend_scaffold_tests` with all required env vars.

### Task 5: Docs, Manifest, Guidance, and Validation

- [x] Update docs/manifest/skills/subagents/Claude guidance to state D3D12 and Vulkan RHI directional shadow receiver readback proofs exist, while package-visible shadows, comparison filtering, cascades, atlases, PCF, postprocess depth, and Metal shadows remain follow-up.
- [x] Strengthen `tools/check-ai-integration.ps1` for the new shadow receiver env vars and honesty wording.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, focused build/tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `cpp-reviewer`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Move this plan from Active slice to Completed in `docs/superpowers/plans/README.md`.
- [x] Select the next host-feasible production slice without waiting for user confirmation.

## Validation Results

- RED receiver policy build failed as expected before production API implementation.
- Focused `MK_renderer_tests` build passed after adding `ShadowReceiverDesc`, `ShadowReceiverPlan`, descriptor policy, bias/intensity diagnostics, and receiver plan implementation.
- Focused D3D12 receiver readback proof passed through `MK_d3d12_rhi_tests`: the test writes shadow depth, transitions it to `shader_read`, samples it in a receiver pass, and verifies the receiver target is darkened through CPU readback.
- Vulkan hardening was added before the receiver proof: physical-device snapshots and logical-device planning now require `synchronization2`, device command requests gate `vkCmdPipelineBarrier2`/`vkQueueSubmit2` on the enabled feature, and D24S8 image barriers use depth|stencil aspects while sampled image views remain depth-only.
- Focused `MK_backend_scaffold_tests` build and default no-env CTest passed.
- Local Vulkan SDK DXC compiled Vulkan 1.3 SPIR-V artifacts for depth-write and receiver shaders; `spirv-val --target-env vulkan1.3` passed for all required artifacts.
- Strict env-gated `MK_backend_scaffold_tests` passed with `MK_VULKAN_TEST_DEPTH_VERTEX_SPV`, `MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV`, `MK_VULKAN_TEST_DEPTH_SAMPLE_VERTEX_SPV`, `MK_VULKAN_TEST_DEPTH_SAMPLE_FRAGMENT_SPV`, `MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV`, and `MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV` configured.
- `cpp-reviewer` found a false-pass risk in receiver proof, a forged `ShadowMapPlan` acceptance path, and incomplete optional Vulkan synchronization2 command gating. Follow-up fixes added lit/control pixel assertions with fullscreen receiver coverage and non-white clear targets, made `build_shadow_receiver_plan` validate the source plan's depth texture, receiver/caster counts, and frame graph evidence, and made `create_runtime_device` require/store `vkCmdPipelineBarrier2`/`vkQueueSubmit2` only when `synchronization2` is enabled.
- Focused build for `MK_renderer_tests`, `MK_d3d12_rhi_tests`, and `MK_backend_scaffold_tests` passed after reviewer fixes.
- Focused CTest for `MK_renderer_tests|MK_d3d12_rhi_tests|MK_backend_scaffold_tests` passed after reviewer fixes.
- Strict env-gated Vulkan `MK_backend_scaffold_tests` passed after reviewer fixes with depth-write, sampled-depth, and shadow receiver SPIR-V artifacts configured.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed; `shader-toolchain-check` remains diagnostic-only for missing Apple `metal`/`metallib`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with known diagnostic-only host gates for Metal/Apple packaging, Android release signing/device smoke, and strict clang-tidy compile database availability.

## Remaining Follow-Up

- Package-visible desktop runtime shadow rendering and installed smoke proof.
- Shadow comparison sampler/filtering, PCF, cascades, atlases, spot/point shadows, per-object receiver/caster masks, GPU markers, and editor shadow debugging.
- Metal native sampled shadow depth behind macOS/Xcode host gates.

## Done When

- `MK_renderer` exposes a backend-neutral receiver policy for one directional shadow depth texture without native handle leakage.
- D3D12 proves a shadow depth texture can darken a receiver pass through CPU readback.
- Vulkan proves the same behavior when real SPIR-V artifacts and Vulkan runtime are configured, while default test lanes remain skip-safe.
- Docs, manifest, skills, subagents, and Claude guidance distinguish this proof from package-visible production shadows.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or concrete host blockers are recorded.
