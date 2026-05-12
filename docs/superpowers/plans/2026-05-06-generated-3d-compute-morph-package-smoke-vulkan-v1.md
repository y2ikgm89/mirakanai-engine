# Generated 3D Compute Morph Package Smoke Vulkan v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `generated-3d-compute-morph-package-smoke-vulkan-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Promote the completed Vulkan Runtime RHI compute morph POSITION proof into the generated `DesktopRuntime3DPackage`
package-smoke path by letting generated packages request a Vulkan backend compute-morph smoke that dispatches selected
POSITION deltas, renders the compute-written POSITION output, and reports first-party `scene_gpu_compute_morph_*`
counters through `--require-compute-morph` without exposing native Vulkan objects.

## Architecture

Build on RHI Vulkan Compute Dispatch Foundation v1, Runtime RHI Compute Morph Vulkan Proof v1, Runtime RHI Compute Morph
Renderer Consumption Vulkan v1, and Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1. Keep the generated
package surface parallel to the D3D12 POSITION package smoke: package metadata selects SPIR-V shader artifacts,
`mirakana_runtime_host_sdl3_presentation` remains the host-owned bridge, and gameplay sees only package files, smoke
requirements, selected mesh-to-morph rows, and first-party counters. Add Vulkan scene renderer compute-morph request
fields only behind backend-neutral SDL3 host descriptors such as `SdlDesktopPresentationVulkanSceneRendererDesc`; keep
`Vk*`, descriptor sets, command buffers, queues, shader modules, and pipelines inside `mirakana_rhi_vulkan` / host adapters.

## Tech Stack

C++23, `mirakana_runtime_host_sdl3_presentation`, `mirakana_runtime_scene_rhi`, `mirakana_runtime_rhi`, `mirakana_renderer`, `mirakana_rhi_vulkan`,
generated `DesktopRuntime3DPackage`, PowerShell package scaffolding, Vulkan SPIR-V artifacts, CTest package smoke, and
`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` when the package lane is selected.

---

## Official References

- Vulkan spec `vkCmdDispatch` for direct compute dispatch.
- Vulkan synchronization2 references for compute shader writes becoming visible before graphics vertex input reads.
- Project shader policy: generated Vulkan shader artifacts are explicit package files and are validated from SPIR-V
  bytecode; CMake configure must not compile or download shader artifacts.

## Context

- D3D12 generated compute morph package smoke already reports `scene_gpu_compute_morph_mesh_bindings`,
  `scene_gpu_compute_morph_dispatches`, `scene_gpu_compute_morph_queue_waits`,
  `scene_gpu_compute_morph_mesh_resolved`, `scene_gpu_compute_morph_draws`, and
  `scene_gpu_compute_morph_output_position_bytes`.
- Vulkan backend tests now prove compute dispatch, Runtime RHI POSITION readback, renderer consumption of the
  compute-written POSITION buffer, and optional NORMAL/TANGENT output readback through public RHI/runtime RHI APIs.
- Generated packages already have Vulkan scene shader artifact loading for the base scene path; compute-morph-specific
  Vulkan artifact metadata and host wiring remain separate work.

## Constraints

- Do not expose `IRhiDevice`, command lists, descriptor handles, `Vk*` handles, command buffers, queues, shader modules,
  pipeline layouts, or pipelines to generated gameplay.
- Do not claim Vulkan NORMAL/TANGENT package smoke, skin+morph composition, async overlap/performance, Metal parity,
  directional-shadow morph rendering, scene-schema compute-morph authoring, frame graph scheduling, or broad renderer
  quality.
- Do not compile shader artifacts in CMake configure or backend code. Generated package workflows may reference
  committed or generated SPIR-V package files explicitly.
- Keep the first package promotion focused on POSITION compute morph smoke.

## File Map

- `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`: add Vulkan scene renderer
  compute-morph request fields if needed.
- `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`: validate and execute the host-owned Vulkan compute-morph
  scene path.
- `tools/new-game.ps1` and sample package metadata/shaders: emit/load Vulkan compute morph SPIR-V artifacts and package
  files for generated `DesktopRuntime3DPackage` smoke.
- `tests/unit/runtime_host_sdl3_tests.cpp`, `tests/unit/runtime_host_sdl3_public_api_compile.cpp`, and package/runtime
  smoke tests: prove request validation and package-visible counters.
- Docs, manifest, plan registry, static checks, skills, and subagents: describe this as the active narrow Vulkan package
  promotion without broad backend parity claims.

## Done When

- A RED package/host test fails first because Vulkan generated compute morph package smoke cannot request or validate
  the compute-morph SPIR-V artifacts/counters.
- Generated `DesktopRuntime3DPackage` Vulkan smoke can require POSITION compute morph, dispatch selected morph deltas,
  render the compute-written POSITION output, and report first-party `scene_gpu_compute_morph_*` counters.
- Native Vulkan handles and backend stats remain private to backend/host adapters.
- D3D12 generated package behavior remains compatible within the current greenfield API surface.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused package/runtime tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or
  concrete host/tool blockers are recorded.

## Tasks

- [x] Add RED generated-package/host test for Vulkan POSITION compute morph package smoke request/counters.
- [x] Add Vulkan compute morph package shader artifact metadata and generated package loading.
- [x] Wire host-owned Vulkan scene renderer compute-morph dispatch/render consumption.
- [x] Update docs, manifest, static checks, skills/subagents, and validation evidence.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `cmd /c "set PATH=& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests"` | RED FAIL (expected) | Before implementation, `SdlDesktopPresentationVulkanSceneRendererDesc` had no `compute_morph_vertex_shader`, `compute_morph_shader`, or `compute_morph_mesh_bindings` members. |
| `cmd /c "set PATH=& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests"` | PASS | Host-owned SDL3 presentation tests built after adding Vulkan compute morph request fields and validation. |
| `cmd /c "set PATH=& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile"` | PASS | Public SDL3 host API compile target accepted the Vulkan compute morph descriptor fields. |
| `ctest --preset desktop-runtime -R "mirakana_runtime_host_sdl3_(tests\|public_api_compile)" --output-on-failure` | PASS | 2/2 focused runtime host tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok` after static checks required Vulkan compute morph SPIR-V artifacts, generated loader wiring, and POSITION-only package-scope guard. |
| `cmd /c "set PATH=& cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_material_shader_package"` | PASS | Generated package shader target built with the updated scene shader artifact helper. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok` after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok` for the public SDL3 host descriptor changes. |
| `git diff --check` | PASS | No whitespace errors. Git reported only existing CRLF normalization warnings on this Windows checkout. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed, including agent/json/dependency policy checks, toolchain/tidy diagnostics, build, and 29/29 `dev` CTest tests. Metal and Apple packaging remained diagnostic-only host blockers on this Windows machine. |

## Next-Step Decision

Registry, master plan, and manifest were re-read after focused validation. This slice completes only the generated
`DesktopRuntime3DPackage` Vulkan POSITION compute morph package smoke through `SdlDesktopPresentationVulkanSceneRendererDesc`,
`--require-compute-morph`, explicit SPIR-V package artifacts, and first-party `scene_gpu_compute_morph_*` counters. It
does not promote Vulkan NORMAL/TANGENT package smoke, Vulkan skin+compute package smoke, measured async overlap,
Metal parity, directional-shadow morph rendering, scene-schema compute-morph authoring, public native handles, or broad
renderer quality. The next active slice is `2d-native-sprite-batching-execution-v1`, which moves the remaining 2D
production gap from telemetry-only sprite batch planning toward a narrow renderer-owned native sprite batching
execution proof.
