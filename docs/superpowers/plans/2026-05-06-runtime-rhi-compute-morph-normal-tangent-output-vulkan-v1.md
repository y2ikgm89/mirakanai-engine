# Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Prove the existing Runtime RHI opt-in NORMAL/TANGENT compute morph output contract on Vulkan by dispatching a Vulkan
compute shader through `RuntimeMorphMeshComputeBindingOptions::output_normal_usage` and `output_tangent_usage`, reading
the NORMAL/TANGENT output buffers back through public RHI APIs, and keeping Vulkan objects private.

## Architecture

Build on the completed Runtime RHI Compute Morph Vulkan Proof v1 and Runtime RHI Compute Morph Renderer Consumption
Vulkan v1. Use the already public `RuntimeMorphMeshComputeBinding`, optional output NORMAL/TANGENT buffer usage fields,
descriptor bindings `4..7`, `IRhiCommandList::dispatch`, public RHI buffer copies/readback, and configured SPIR-V
artifacts. Keep all native Vulkan shader modules, descriptor sets, command buffers, and pipeline objects inside
`MK_rhi_vulkan`.

## Tech Stack

C++23, `MK_runtime_rhi`, `MK_rhi`, `MK_rhi_vulkan`, Vulkan 1.3 compute queues, CTest env-gated Vulkan runtime tests,
and DXC/SPIR-V shader artifacts produced outside CMake configure.

---

## Official References

- Vulkan spec `vkCmdDispatch`: records direct compute dispatch with non-zero workgroup counts.
- Vulkan synchronization2 references for compute shader writes becoming visible to transfer readback.
- Existing project shader tooling policy: generate Vulkan SPIR-V through DXC executable-plus-argument vectors with
  `-spirv` and `-fspv-target-env=vulkan1.3`, then validate with `spirv-val`.

## Context

- Runtime RHI Compute Morph NORMAL/TANGENT Output D3D12 v1 proved opt-in NORMAL/TANGENT output buffers and normalized
  tangent-frame readback on D3D12.
- Runtime RHI Compute Morph Vulkan Proof v1 proved POSITION output readback on Vulkan.
- Runtime RHI Compute Morph Renderer Consumption Vulkan v1 proved POSITION output rendering through `RhiFrameRenderer`
  on Vulkan.

## Constraints

- Do not expose `Vk*` handles, command pointers, descriptor sets, queues, command buffers, shader modules, pipeline
  layouts, or pipelines outside `MK_rhi_vulkan`.
- Do not claim generated-package Vulkan compute morph readiness, Vulkan tangent-frame renderer/package consumption,
  skin+morph composition on Vulkan, async overlap/performance, frame graph scheduling, Metal parity,
  directional-shadow morph rendering, scene-schema compute morph authoring, or broad renderer quality.
- Do not compile shader artifacts in CMake configure or backend code. Tests may consume precompiled SPIR-V through
  environment variables.
- Keep the proof limited to NORMAL/TANGENT output readback through public Runtime RHI/RHI APIs.

## File Map

- `tests/unit/backend_scaffold_tests.cpp`: add the env-gated Vulkan NORMAL/TANGENT compute morph readback proof.
- `tests/shaders/`: add or reuse a Vulkan-friendly compute morph tangent-frame shader fixture.
- Docs, manifest, plan registry, static checks, skills/subagents, and `tools/check-ai-integration.ps1`: describe this
  Vulkan NORMAL/TANGENT output slice as active without widening generated-package claims.

## Done When

- A RED test fails first because the Vulkan NORMAL/TANGENT output proof is missing or its configured SPIR-V artifact is
  not available.
- A ready Vulkan host dispatches the compute shader through `RuntimeMorphMeshComputeBinding` with opt-in
  NORMAL/TANGENT outputs and reads deterministic normalized tangent-frame bytes back through public RHI APIs.
- The proof uses existing public Runtime RHI/RHI contracts and keeps Vulkan handles private.
- Docs and agent surfaces describe the Vulkan POSITION readback and renderer-consumption slices as complete and this
  NORMAL/TANGENT output proof as the active, narrow follow-up.
- Focused Vulkan/runtime RHI tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Add RED env-gated Vulkan Runtime RHI compute morph NORMAL/TANGENT output readback proof.
- [x] Add or reuse a Vulkan SPIR-V compute morph tangent-frame shader fixture.
- [x] Wire the proof through `RuntimeMorphMeshComputeBindingOptions::output_normal_usage` and `output_tangent_usage`.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd /c "set PATH=& set MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\missing_tangent_frame.cs.spv& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"` failed as expected on the missing configured SPIR-V artifact.
- Shader compile: `C:\VulkanSDK\1.4.341.1\Bin\dxc.exe -spirv -T cs_6_0 -E main "-fspv-target-env=vulkan1.3" -Fo out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_tangent_frame.cs.spv tests\shaders\vulkan_compute_morph_tangent_frame.hlsl` passed.
- Shader validation: `C:\VulkanSDK\1.4.341.1\Bin\spirv-val.exe --target-env vulkan1.3 out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_tangent_frame.cs.spv` passed.
- Build: `cmd /c "set PATH=& cmake --build --preset dev --target MK_backend_scaffold_tests"` passed.
- GREEN: `cmd /c "set PATH=& set MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_tangent_frame.cs.spv& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"` passed.
- Format/static gates: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after syncing the next active slice to Generated 3D Compute Morph Package Smoke Vulkan v1.
- Focused Vulkan compute-morph gate: `cmd /c "set PATH=& set MK_VULKAN_TEST_COMPUTE_MORPH_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_position.cs.spv& set MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_renderer_position.vs.spv& set MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_renderer_position.ps.spv& set MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_tangent_frame.cs.spv& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"` passed.
- Boundary/whitespace gates: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed; `git diff --check` exited 0 with LF/CRLF warnings only.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including `agent-check`, `shader-toolchain-check` diagnostic-only Vulkan SPIR-V readiness, configure/build, and 29/29 CTest tests.
