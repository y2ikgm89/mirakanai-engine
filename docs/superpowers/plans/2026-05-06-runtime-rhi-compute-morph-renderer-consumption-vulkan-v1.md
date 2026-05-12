# Runtime RHI Compute Morph Renderer Consumption Vulkan v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-renderer-consumption-vulkan-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Prove that Vulkan can render from the POSITION buffer written by Runtime RHI compute morph dispatch by converting the
compute output through `mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding`, drawing with
`RhiFrameRenderer`, reading back deterministic visible output through public RHI/renderer contracts, and keeping all
Vulkan objects private.

## Architecture

Build on the completed Runtime RHI Compute Morph Vulkan Proof v1 and the existing D3D12 renderer-consumption proof.
Use the public `RuntimeMorphMeshComputeBinding`, `make_runtime_compute_morph_output_mesh_gpu_binding`,
`IRhiCommandList::dispatch`, queue/fence ordering, and `RhiFrameRenderer` contracts. The Vulkan backend remains an
implementation detail behind `MK_rhi_vulkan`; tests consume configured SPIR-V artifacts and do not add CMake configure
shader compilation.

## Tech Stack

C++23, `MK_runtime_rhi`, `MK_renderer`, `MK_rhi`, `MK_rhi_vulkan`, Vulkan 1.3 compute and graphics queues,
`RhiFrameRenderer`, CTest env-gated Vulkan runtime tests, and DXC/SPIR-V shader artifacts produced outside CMake
configure.

---

## Official References

- Vulkan spec `vkCmdDispatch`: records direct compute dispatch with non-zero workgroup counts.
- Vulkan synchronization2 references for compute shader writes becoming visible before graphics vertex input reads.
- Vulkan dynamic rendering and vertex input rules as already encapsulated by `MK_rhi_vulkan` draw recording.
- Existing project shader tooling policy: generate Vulkan SPIR-V through DXC executable-plus-argument vectors with
  `-spirv` and `-fspv-target-env=vulkan1.3`, then validate with `spirv-val`.

## Context

- Runtime RHI Compute Morph Vulkan Proof v1 proved `RuntimeMorphMeshComputeBinding` POSITION output readback on Vulkan
  through public RHI compute dispatch and buffer readback.
- Runtime RHI Compute Morph Renderer Consumption D3D12 v1 already proves
  `make_runtime_compute_morph_output_mesh_gpu_binding` plus `RhiFrameRenderer` draw/readback on the D3D12 primary lane.
- RHI Vulkan Compute Dispatch Foundation v1 and the Vulkan visible draw/readback proofs provide the backend-private
  compute, descriptor, graphics pipeline, and readback foundation.

## Constraints

- Do not expose `Vk*` handles, command pointers, descriptor sets, queues, command buffers, shader modules, pipeline
  layouts, pipelines, image views, barriers, or synchronization objects outside `MK_rhi_vulkan`.
- Do not claim generated-package Vulkan compute morph readiness, Vulkan NORMAL/TANGENT compute morph output,
  skin+morph composition on Vulkan, async overlap/performance, frame graph scheduling, Metal parity,
  directional-shadow morph rendering, scene-schema compute morph authoring, or broad renderer quality.
- Do not compile shader artifacts in CMake configure or backend code. Tests may consume precompiled SPIR-V through
  environment variables.
- Keep the proof limited to POSITION output rendering through public Runtime RHI, RHI, and renderer APIs.

## File Map

- `tests/unit/backend_scaffold_tests.cpp`: add the env-gated Vulkan compute-morph renderer-consumption proof.
- `tests/shaders/`: add or reuse Vulkan-friendly compute, vertex, and fragment shader fixtures.
- Docs, manifest, plan registry, static checks, skills/subagents, and `tools/check-ai-integration.ps1`: describe this
  Vulkan renderer-consumption slice as complete and move the active slice to Vulkan NORMAL/TANGENT output parity.

## Done When

- A RED test fails first because Vulkan cannot render from the Runtime RHI compute morph output buffer or the configured
  SPIR-V artifacts are missing.
- A ready Vulkan host dispatches the compute morph shader, converts the compute-written POSITION output through
  `make_runtime_compute_morph_output_mesh_gpu_binding`, draws it through `RhiFrameRenderer`, and reads deterministic
  visible output through public RHI/renderer APIs.
- The proof uses existing public Runtime RHI/RHI/renderer contracts and keeps Vulkan handles private.
- Docs and agent surfaces describe Runtime RHI Compute Morph Vulkan Proof v1 as complete and this renderer-consumption
  proof as the active, narrow follow-up.
- Focused Vulkan/runtime RHI/renderer tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Add RED env-gated Vulkan renderer-consumption proof for compute-written POSITION output.
- [x] Add or reuse Vulkan SPIR-V compute, vertex, and fragment fixtures with deterministic visible output.
- [x] Dispatch compute morph, produce a vertex-usage output buffer, draw through `RhiFrameRenderer`, and read back the
  result.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd /c "set PATH=& set MK_VULKAN_TEST_COMPUTE_MORPH_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_position.cs.spv& set MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_renderer_position.vs.spv& set MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_renderer_position.ps.spv& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"` failed before the render artifacts existed with `requirement failed: vertex_artifact.diagnostic == "loaded"`.
- Fixture compile: `C:\VulkanSDK\1.4.341.1\Bin\dxc.exe -spirv -T vs_6_0 -E vs_main -fspv-target-env=vulkan1.3 -Fo out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_renderer_position.vs.spv tests\shaders\vulkan_compute_morph_renderer_position.hlsl` passed.
- Fixture compile: `C:\VulkanSDK\1.4.341.1\Bin\dxc.exe -spirv -T ps_6_0 -E ps_main -fspv-target-env=vulkan1.3 -Fo out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_renderer_position.ps.spv tests\shaders\vulkan_compute_morph_renderer_position.hlsl` passed.
- Fixture validation: `C:\VulkanSDK\1.4.341.1\Bin\spirv-val.exe --target-env vulkan1.3 out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_renderer_position.vs.spv` passed.
- Fixture validation: `C:\VulkanSDK\1.4.341.1\Bin\spirv-val.exe --target-env vulkan1.3 out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_renderer_position.ps.spv` passed.
- GREEN: `cmd /c "set PATH=& set MK_VULKAN_TEST_COMPUTE_MORPH_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_position.cs.spv& set MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_renderer_position.vs.spv& set MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_renderer_position.ps.spv& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"` passed.
