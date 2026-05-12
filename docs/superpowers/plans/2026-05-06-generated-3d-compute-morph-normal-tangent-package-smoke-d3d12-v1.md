# Generated 3D Compute Morph Normal Tangent Package Smoke D3D12 v1 (2026-05-06)

**Plan ID:** `generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Extend the generated `DesktopRuntime3DPackage` D3D12 compute morph smoke from POSITION-only output into host-owned
POSITION/NORMAL/TANGENT compute output buffers and lit tangent-frame package rendering evidence.

## Context

- `generated-3d-compute-morph-package-smoke-d3d12-v1` proved generated package code can request a host-owned D3D12
  compute morph dispatch, render the compute-written POSITION output, and report `scene_gpu_compute_morph_*` counters.
- `runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1` added opt-in NORMAL/TANGENT output buffers to
  `RuntimeMorphMeshComputeBinding` and proved deterministic D3D12 normalized tangent-frame readback rows.
- Generated package graphics morph already proves POSITION/NORMAL/TANGENT deltas in a vertex-shader path; compute morph
  package output is still limited to POSITION unless this slice wires the new runtime RHI output buffers into the SDL3
  D3D12 package host path.

## Constraints

- Keep compute dispatch, descriptor sets, output buffers, pipeline layouts, command lists, and native D3D12 details
  host-owned inside `mirakana_runtime_host_sdl3_presentation` and `mirakana_runtime_rhi`.
- Do not expose `IRhiDevice`, `SceneGpuBindingPalette`, swapchain frames, command lists, descriptor handles, or native
  graphics handles to generated gameplay code.
- Keep the generated package switch explicit and D3D12-only. Vulkan/Metal compute parity, async compute overlap,
  skin+morph composition, directional-shadow morph rendering, scene-schema compute-morph authoring, and broad renderer
  quality remain follow-up work.
- Preserve existing POSITION-only package smoke behavior for callers that do not request tangent-frame compute output.
- Add focused RED tests/static expectations before production implementation.

## Done When

- A focused test/static expectation fails first for generated package NORMAL/TANGENT compute morph readiness.
- `SdlDesktopPresentationD3d12SceneRendererDesc` can request tangent-frame compute output without leaking backend-native
  handles, and the host path passes `output_normal_usage` / `output_tangent_usage` into
  `create_runtime_morph_mesh_compute_binding`.
- The compute-morph scene draw path can bind compute-written POSITION/NORMAL/TANGENT vertex buffers with deterministic
  lit tangent-frame vertex input metadata.
- Generated `DesktopRuntime3DPackage` shader/scaffold/package smoke fields distinguish POSITION-only compute morph from
  tangent-frame compute morph and report deterministic package counters.
- Docs, manifest, plan registry, static checks, skills, and subagents describe the D3D12-only package smoke boundary.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect existing generated compute morph package smoke, SDL3 D3D12 scene renderer descriptors, vertex input helper
      functions, and package shader template.
- [x] Add RED coverage for a tangent-frame compute morph package request, including descriptor/request fields, generated
      shader/scaffold expectations, and package smoke counter names.
- [x] Extend runtime-host descriptor/request structs and scene compute morph binding creation so NORMAL/TANGENT output is
      explicit and opt-in.
- [x] Add compute-morph tangent-frame vertex buffer/attribute binding support while preserving the POSITION-only path.
- [x] Extend generated D3D12 package shader/scaffold metadata so `--require-compute-morph-normal-tangent` requests
      POSITION/NORMAL/TANGENT compute output and emits deterministic smoke fields.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"` failed because `mirakana::runtime_rhi::make_runtime_compute_morph_tangent_frame_output_mesh_gpu_binding` did not exist.
- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile"` failed because `SdlDesktopPresentationD3d12SceneRendererDesc::enable_compute_morph_tangent_frame_output` did not exist.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed because the generated `DesktopRuntime3DPackage` scaffold did not emit `COMPUTE_MORPH_TANGENT_ENTRY cs_compute_morph_tangent_frame`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_runtime_rhi_tests --output-on-failure"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset desktop-runtime -R mirakana_runtime_host_sdl3_public_api_compile --output-on-failure"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset desktop-runtime -R mirakana_runtime_host_sdl3_tests --output-on-failure"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_material_shader_package"` passed after the CMake helper accepted existing generated shader-artifact callers.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_renderer_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_renderer_tests --output-on-failure"` passed.
- Static/format/API/tidy: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed before the final documentation/pointer update.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after selecting `runtime-rhi-compute-morph-queue-synchronization-d3d12-v1` as the next active plan and updating docs/manifest/static checks.
