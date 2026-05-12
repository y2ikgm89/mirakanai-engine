# Runtime Scene RHI Morph GPU Palette v1 (2026-05-05)

**Plan ID:** `runtime-scene-rhi-morph-gpu-palette-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Bridge selected cooked `morph_mesh_cpu` package rows into retained RHI morph GPU bindings owned by
`mirakana_runtime_scene_rhi`. This gives host code a package-level `MorphMeshGpuBinding` palette to pair with the D3D12
`RhiFrameRenderer` GPU morph proof without making generated gameplay own RHI devices or native handles.

## Context

- `gpu-morph-d3d12-proof-v1` proved one D3D12 POSITION-delta draw path from `RuntimeMorphMeshCpuPayload` through
  `RhiFrameRenderer`.
- `mirakana_runtime_scene_rhi` already uploads static meshes, skinned meshes, textures, and materials from cooked packages
  into retained palettes for host-owned scene rendering.
- Scene v1/v2 does not yet define a first-party component relation from a base mesh to a morph asset, so this slice
  uses explicit selected morph asset ids in `RuntimeSceneGpuBindingOptions` rather than inventing schema semantics.

## Constraints

- Keep the API RHI-neutral and host-owned; no D3D12/Vulkan/Metal/native handles in gameplay contracts.
- Do not modify scene schema, generated `DesktopRuntime3DPackage` gameplay, shader artifacts, or package smoke args in
  this slice.
- Support POSITION delta uploads only through the existing `mirakana_runtime_rhi` morph helper. NORMAL/TANGENT/compute morph
  and backend parity remain follow-up work.
- Require explicit selected morph asset ids so package-wide scans do not silently upload unrelated rows.
- Add RED coverage before implementation.

## Done When

- `RuntimeSceneGpuBindingOptions` can name selected `morph_mesh_cpu` assets for upload.
- `RuntimeSceneGpuBindingResult` retains morph upload resources, a morph descriptor set layout, and a
  `SceneMorphGpuBindingPalette`.
- `make_runtime_scene_gpu_upload_execution_report` counts morph uploads and uploaded morph bytes.
- NullRHI safe-point teardown releases retained morph buffers/descriptors.
- Tests cover successful selected morph package upload and missing/wrong-kind failure behavior.
- Docs/manifest/static checks record the narrow bridge and preserve broader generated package/runtime claims as
  follow-up work.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete blocker.

## Tasks

- [x] Add RED runtime_scene_rhi tests for selected cooked morph package upload and reporting.
- [x] Add `SceneMorphGpuBindingPalette` and runtime scene RHI result/options/report fields.
- [x] Implement selected morph asset upload, descriptor attach, palette retention, and deterministic failures.
- [x] Extend NullRHI safe-point teardown for morph buffers, descriptor sets, and descriptor set layout.
- [x] Update docs, manifest, registry, and static agent checks.
- [x] Run focused tests, format/API/agent checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and record evidence.

## Validation Evidence

- RED: `mirakana_runtime_scene_rhi_tests.vcxproj` failed before implementation because `RuntimeSceneGpuBindingOptions`
  lacked `morph_mesh_assets`, `RuntimeSceneGpuBindingResult` lacked `morph_descriptor_set_layout`,
  `morph_palette`, and `morph_mesh_uploads`, and `RuntimeSceneGpuUploadExecutionReport` lacked
  `morph_mesh_uploads` / `uploaded_morph_bytes`.
- GREEN focused build: `mirakana_runtime_scene_rhi_tests.vcxproj` passed with 0 warnings and 0 errors.
- GREEN focused tests: `out\build\dev\Debug\mirakana_runtime_scene_rhi_tests.exe` passed, including selected cooked
  morph upload and wrong-kind `morph_mesh_cpu` diagnostics.
- Regression focused tests: `mirakana_scene_renderer_tests.vcxproj` passed, then
  `out\build\dev\Debug\mirakana_scene_renderer_tests.exe` passed.
- Formatting/static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied clang-format, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. CTest reported 29/29 tests passed. Metal and Apple packaging remained
  diagnostic-only host gates on this Windows host.
