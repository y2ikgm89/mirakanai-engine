# Runtime RHI Compute Morph Vulkan Proof v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-vulkan-proof-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Prove the existing public Runtime RHI compute morph helper on the Vulkan backend by dispatching a Vulkan compute shader
through `mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding`, reading the POSITION output back through public RHI
buffer readback, and keeping all Vulkan objects private.

## Architecture

Build on the completed RHI Vulkan Compute Dispatch Foundation v1. Use the existing backend-neutral
`RuntimeMorphMeshComputeBinding`, public `IRhiDevice` compute contracts, descriptor sets, `IRhiCommandList::dispatch`,
and public buffer readback. Add only Vulkan-focused test coverage and narrow docs/agent evidence; do not add generated
package hooks or gameplay-visible RHI/native surfaces.

## Tech Stack

C++23, `MK_runtime_rhi`, `MK_rhi`, `MK_rhi_vulkan`, Vulkan 1.3 compute queues, existing SPIR-V artifact validation,
CTest env-gated Vulkan runtime tests, and DXC/SPIR-V shader artifacts produced outside CMake configure.

---

## Official References

- Vulkan spec `vkCmdDispatch`: records direct compute dispatch and requires non-zero workgroup counts.
- Vulkan synchronization2 references for compute shader writes becoming visible to transfer readback.
- Existing project shader tooling policy: generate Vulkan SPIR-V through DXC executable-plus-argument vectors with
  `-spirv` and `-fspv-target-env=vulkan1.3`, then validate with `spirv-val`.

## Context

- `rhi-compute-dispatch-foundation-v1` added backend-neutral compute pipeline, descriptor binding, and dispatch
  contracts, with NullRHI and D3D12 proof.
- `runtime-rhi-compute-morph-d3d12-proof-v1` added `RuntimeMorphMeshComputeBinding` and D3D12 POSITION output readback.
- `rhi-vulkan-compute-dispatch-foundation-v1` completed native Vulkan compute pipeline creation, descriptor binding,
  `vkCmdDispatch`, and env-gated storage-buffer readback proof.
- Generated D3D12 compute morph package smokes, queue waits, skin+compute, and async telemetry are already separate
  D3D12 evidence boundaries.

## Constraints

- Do not expose `Vk*` handles, command pointers, descriptor sets, queues, command buffers, shader modules, pipeline
  layouts, or pipelines outside `MK_rhi_vulkan`.
- Do not claim generated-package Vulkan compute morph readiness, Vulkan renderer consumption of compute morph output,
  Vulkan NORMAL/TANGENT compute morph output, skin+morph composition on Vulkan, async overlap/performance, frame graph
  scheduling, Metal parity, directional-shadow morph rendering, scene-schema compute morph authoring, or broad renderer
  quality.
- Do not compile shader artifacts in CMake configure or backend code. Tests may consume precompiled SPIR-V through
  environment variables.
- Keep the proof limited to POSITION output readback through public `IRhiDevice` APIs.

## File Map

- `tests/unit/runtime_rhi_tests.cpp` or `tests/unit/backend_scaffold_tests.cpp`: add the env-gated Vulkan runtime morph
  compute readback proof.
- `tests/shaders/`: add a Vulkan-friendly compute morph POSITION shader fixture when an existing shader cannot be
  reused cleanly.
- Docs, manifest, plan registry, static checks, skills/subagents, and `tools/check-ai-integration.ps1`: mark this
  Vulkan compute morph POSITION readback proof complete and move the active slice to Vulkan renderer consumption of the
  compute morph output buffer.

## Done When

- A RED test fails first because the Vulkan runtime morph compute proof is missing, skipped incorrectly, or cannot bind
  the Runtime RHI compute morph descriptors on Vulkan.
- A ready Vulkan host with configured SPIR-V dispatches the compute morph shader through
  `RuntimeMorphMeshComputeBinding` and reads deterministic POSITION output bytes back through public RHI APIs.
- The proof uses existing public RHI/runtime RHI contracts and keeps Vulkan handles private.
- Docs and agent surfaces describe Vulkan compute dispatch as complete and this morph proof as the active, narrow
  follow-up.
- Focused Vulkan/runtime RHI tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`, `ctest --preset dev --output-on-failure`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete host/tool blockers are
  recorded.

## Tasks

- [x] Add RED env-gated Vulkan Runtime RHI compute morph POSITION readback proof.
- [x] Add or reuse a Vulkan SPIR-V compute morph shader fixture with deterministic POSITION output.
- [x] Wire the test through `create_runtime_morph_mesh_compute_binding` and public Vulkan `IRhiDevice` compute dispatch.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd /c "set PATH=& set MK_VULKAN_TEST_COMPUTE_MORPH_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_position.cs.spv& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"` failed before the artifact existed with `requirement failed: compute_artifact.diagnostic == "loaded"`.
- Toolchain: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` reported `vulkan_spirv=ready`, `dxc_spirv_codegen=ready`, `dxc=found via known-location at C:\VulkanSDK\1.4.341.1\Bin\dxc.exe`, and `spirv-val=found via known-location at C:\VulkanSDK\1.4.341.1\Bin\spirv-val.exe`.
- Fixture compile: `C:\VulkanSDK\1.4.341.1\Bin\dxc.exe -spirv -T cs_6_0 -E main -fspv-target-env=vulkan1.3 -Fo out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_position.cs.spv tests\shaders\vulkan_compute_morph_position.hlsl` passed.
- Fixture validation: `C:\VulkanSDK\1.4.341.1\Bin\spirv-val.exe --target-env vulkan1.3 out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_position.cs.spv` passed.
- GREEN: `cmd /c "set PATH=& set MK_VULKAN_TEST_COMPUTE_MORPH_SPV=G:\workspace\development\GameEngine\out\vulkan-compute-morph-test-artifacts\vulkan_compute_morph_position.cs.spv& ctest --preset dev -R MK_backend_scaffold_tests --output-on-failure"` passed.
- Focused build: `cmd /c "set PATH=& cmake --build --preset dev --target MK_backend_scaffold_tests"` passed.
- Static sync: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after moving `currentActivePlan` to Runtime RHI Compute Morph Renderer Consumption Vulkan v1.
- Formatting and API boundary: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. The run included license/config/json/recipe/agent/dependency/toolchain/shader-toolchain/mobile diagnostic checks, CMake configure/build, tidy-check, and `ctest --preset dev` with 29/29 tests passing.
