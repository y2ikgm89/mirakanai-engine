# RHI Vulkan Compute Dispatch Foundation v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `rhi-vulkan-compute-dispatch-foundation-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Implement the first native Vulkan `IRhiDevice` compute dispatch foundation by creating compute pipelines, binding them on
compute command lists, binding compatible descriptor sets for compute, recording `vkCmdDispatch`, and proving buffer
readback through an environment-gated Vulkan compute test.

## Architecture

Keep Vulkan handles and command pointers private to `MK_rhi_vulkan` runtime owners. Extend the existing Vulkan runtime
device command-resolution plan to require `vkCreateComputePipelines`, `vkCmdBindPipeline`, `vkCmdBindDescriptorSets`,
and `vkCmdDispatch` before compute is exposed through `create_rhi_device`. Reuse the existing RHI compute contracts
(`ComputePipelineDesc`, `ComputePipelineHandle`, descriptor sets, compute command lists, and `IRhiCommandList::dispatch`)
without adding generated package, runtime-host, gameplay, or backend-neutral native-handle surfaces.

## Tech Stack

C++23, `MK_rhi`, `MK_rhi_vulkan`, Vulkan 1.3 dynamic loader/device command resolution, first-party Vulkan PIMPL owners,
existing SPIR-V artifact validation, existing descriptor-set/pipeline-layout owners, and CTest env-gated Vulkan runtime
tests.

---

## Official References

- Vulkan spec `vkCreateComputePipelines`: creates compute pipeline objects from `VkComputePipelineCreateInfo`; the
  selected device must expose a queue family with `VK_QUEUE_COMPUTE_BIT`.
- Vulkan spec `vkCmdBindPipeline`: binds a pipeline to `VK_PIPELINE_BIND_POINT_COMPUTE` before dispatch.
- Vulkan spec `vkCmdBindDescriptorSets`: binds descriptor sets using the pipeline layout for the selected bind point.
- Vulkan spec `vkCmdDispatch`: records direct compute dispatch; group counts must be non-zero and within device limits.

## Context

- `rhi-compute-dispatch-foundation-v1` already added backend-neutral compute contracts, NullRHI validation, and a D3D12
  compute shader readback proof.
- D3D12 compute morph, queue synchronization, telemetry, submitted timing, and submitted overlap helper slices are
  complete.
- Vulkan RHI currently exposes graphics, descriptor, buffer, readback, shader module, and graphics pipeline foundations,
  but `VulkanRhiDevice::create_compute_pipeline`, `VulkanRhiCommandList::bind_compute_pipeline`, and
  `VulkanRhiCommandList::dispatch` still throw `std::logic_error`.
- Vulkan/Metal compute morph parity and package-visible compute morph claims remain unsupported until separate slices
  build on this backend foundation.

## Constraints

- Do not expose `VkDevice`, `VkPipeline`, `VkPipelineLayout`, `VkShaderModule`, `VkDescriptorSet`, `VkCommandBuffer`,
  `VkQueue`, command function pointers, or native handles through public RHI, renderer, runtime-host, gameplay, package,
  editor, manifest, or generated-game APIs.
- Do not claim Vulkan compute morph parity, generated-package Vulkan compute morph readiness, async compute overlap,
  performance improvement, frame graph scheduling, Metal parity, skin+morph composition, directional-shadow morph
  rendering, scene-schema compute-morph authoring, or broad renderer quality.
- Do not compile shaders in CMake configure or in Vulkan backend code. Tests may consume precompiled SPIR-V artifacts
  through the existing environment-gated artifact loading helpers.
- Do not make Vulkan `IRhiDevice` compute-ready unless the runtime command-resolution plan proves the required native
  compute commands and the selected queue family supports compute.
- Keep descriptor compatibility validation at the command-list/RHI layer by matching descriptor set layouts with the
  currently bound compute pipeline layout.

## File Map

- `engine/rhi/vulkan/src/vulkan_backend.cpp`: Add native compute pipeline owner support, command resolution for
  `vkCreateComputePipelines` and `vkCmdDispatch`, compute pipeline creation, compute pipeline binding, compute descriptor
  binding compatibility, and `vkCmdDispatch` recording.
- `tests/unit/backend_scaffold_tests.cpp`: Add RED Vulkan mapping/command-plan coverage plus an environment-gated compute
  buffer readback proof using precompiled SPIR-V.
- Docs, manifest, plan registry, static checks, skills/subagents, and `tools/check-ai-integration.ps1`: Mark the D3D12
  submitted overlap helper slice complete and set this Vulkan compute dispatch slice as active.

## Done When

- A RED test fails first because Vulkan compute pipeline creation/binding/dispatch is not implemented or because required
  Vulkan compute commands are not part of the command-resolution/mapping plan.
- Vulkan runtime device command resolution includes `vkCreateComputePipelines`, `vkCmdBindPipeline`,
  `vkCmdBindDescriptorSets`, and `vkCmdDispatch` for compute-capable device mapping.
- `VulkanRhiDevice::create_compute_pipeline` creates a backend-private compute pipeline from a validated compute SPIR-V
  shader module and caller-owned pipeline layout.
- `VulkanRhiCommandList::bind_compute_pipeline` binds the compute pipeline to `VK_PIPELINE_BIND_POINT_COMPUTE`.
- `VulkanRhiCommandList::bind_descriptor_set` accepts descriptor sets compatible with the currently bound compute pipeline
  layout and rejects incompatible or graphics-only binding states.
- `VulkanRhiCommandList::dispatch` rejects zero group counts and records `vkCmdDispatch` for valid compute command lists.
- An environment-gated Vulkan test dispatches a compute shader that writes deterministic bytes to a storage buffer and
  reads the bytes back through public RHI buffer readback.
- Focused Vulkan tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`, `ctest --preset dev -R backend_scaffold_tests --output-on-failure`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete
  host/tool blockers are recorded.

## Tasks

- [x] Add RED command-plan and environment-gated Vulkan compute dispatch/readback tests.
- [x] Add Vulkan native function pointer resolution for `vkCreateComputePipelines` and `vkCmdDispatch`.
- [x] Implement backend-private Vulkan compute pipeline owner creation/destruction.
- [x] Wire `VulkanRhiDevice::create_compute_pipeline` through existing SPIR-V shader module and pipeline layout owners.
- [x] Wire `VulkanRhiCommandList::bind_compute_pipeline`, compute descriptor-set compatibility, and `dispatch`.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED command-resolution evidence: `cmd /c "set PATH=& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"`
  failed on `has_compute_pipeline` before `vkCreateComputePipelines` / `vkCmdDispatch` were added to the Vulkan
  device command requests.
- RED mapping evidence: `cmd /c "set PATH=& cmake --build --preset dev --target MK_backend_scaffold_tests"` failed
  with missing `compute_shader`, `compute_dispatch_ready`, and `compute_dispatch_mapped` fields before the Vulkan RHI
  mapping gate was extended.
- GREEN focused evidence: `cmd /c "set PATH=& cmake --build --preset dev --target MK_backend_scaffold_tests"` passed,
  then `cmd /c "set PATH=& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"` passed.
- Shader-tool evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` reported `dxc_spirv_codegen=ready`, Vulkan SDK `dxc`, and
  `spirv-val` readiness on this Windows host.
- Env-gated Vulkan compute readback evidence: `tests/shaders/vulkan_compute_dispatch.hlsl` was compiled to
  `out/vulkan-compute-dispatch-test-artifacts/vulkan_compute_dispatch.cs.spv` with DXC `-spirv` and
  `-fspv-target-env=vulkan1.3`, validated with `spirv-val --target-env vulkan1.3`, and
  `cmd /c "set PATH=& set MK_VULKAN_TEST_COMPUTE_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-dispatch-test-artifacts\vulkan_compute_dispatch.cs.spv& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"`
  passed with the configured compute readback proof active.
- Final focused evidence after formatting/docs updates:
  `cmd /c "set PATH=& cmake --build --preset dev --target MK_backend_scaffold_tests"` passed, and
  `cmd /c "set PATH=& set MK_VULKAN_TEST_COMPUTE_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-dispatch-test-artifacts\vulkan_compute_dispatch.cs.spv& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"`
  passed.
- Final repository evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`,
  `cmd /c "set PATH=& cmake --build --preset dev"`, `cmd /c "set PATH=& ctest --preset dev --output-on-failure"`,
  and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` reported Vulkan SPIR-V readiness and Metal toolchain
  blockers as diagnostic-only on this Windows host.
