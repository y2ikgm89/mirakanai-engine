# GPU Morph Normal Tangent D3D12 Proof v1 (2026-05-05)

**Plan ID:** `gpu-morph-normal-tangent-d3d12-proof-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Extend the narrow D3D12 GPU morph proof beyond POSITION-only deltas so a lit tangent-space mesh can upload optional
NORMAL and TANGENT delta streams from `RuntimeMorphMeshCpuPayload` and prove a vertex-shader NORMAL morph changes
renderer-visible lighting through `RhiFrameRenderer`.

## Context

- `gpu-morph-d3d12-proof-v1` proved D3D12 POSITION-delta morphing through `mirakana_runtime_rhi`,
  `MorphMeshGpuBinding`, `MeshCommand::gpu_morphing`, and `RhiFrameRenderer`.
- `generated-3d-morph-visible-deformation-v1` used the same path for generated package counters, but it still kept the
  shader/data contract POSITION-delta only.
- The first-party `morph_mesh_cpu` source/runtime payload already preserves optional NORMAL and TANGENT delta byte
  streams, and `mirakana_animation` has CPU NORMAL/TANGENT morph evaluation.

## Constraints

- Keep `engine/core` independent of renderer/RHI, OS APIs, GPU APIs, and asset formats.
- Keep public renderer/runtime-RHI contracts backend-neutral. Do not expose D3D12, DXGI, COM, native descriptor handles,
  or `IRhiDevice` to game code.
- Preserve existing POSITION-only morph uploads and descriptor layout behavior.
- Scope this proof to D3D12 primary-lane graphics-pipeline morphing. Compute morph, skin+morph composition,
  scene-schema morph authoring, generated-package NORMAL/TANGENT smoke, directional-shadow morph rendering,
  Vulkan/Metal visible parity, and broad production renderer quality remain follow-up work.
- Add RED tests before production implementation.

## Done When

- `mirakana_runtime_rhi::upload_runtime_morph_mesh_cpu` uploads valid optional NORMAL and TANGENT delta streams when present
  alongside required POSITION deltas, records byte counts, and keeps POSITION-only payloads compatible.
- `MorphMeshGpuBinding` carries optional normal/tangent delta buffers and byte counts without native handles.
- `attach_morph_mesh_descriptor_set` binds NORMAL and TANGENT delta buffers at stable vertex-stage storage-buffer
  bindings when those streams exist, while keeping the existing POSITION/weight bindings unchanged.
- A D3D12 visible test renders the same lit triangle twice and proves the morphed-normal shader path changes the
  center-pixel lighting through uploaded NORMAL deltas.
- Docs, manifest, registry, and static checks describe this as a D3D12 primary-lane POSITION+NORMAL/TANGENT proof and
  keep compute morph, generated-package NORMAL/TANGENT smoke, scene schema authoring, skin+morph, and backend parity
  unsupported.
- Validation evidence is recorded below, including `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` or a concrete blocker.

## Tasks

- [x] Add RED `mirakana_runtime_rhi_tests` coverage for NORMAL/TANGENT morph uploads and descriptor bindings.
- [x] Add RED D3D12 readback coverage proving uploaded NORMAL deltas change lit output through `RhiFrameRenderer`.
- [x] Extend RHI-neutral morph upload/binding structs and descriptor attach behavior.
- [x] Implement optional NORMAL/TANGENT upload packing, validation, and binding.
- [x] Update docs, manifest, registry, and static agent checks for the narrow capability.
- [x] Run focused tests, format/API/agent checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and record evidence.

## Validation Evidence

- RED build: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests mirakana_d3d12_rhi_tests'` failed before implementation because the planned NORMAL/TANGENT upload and binding fields did not exist.
- GREEN focused build: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests mirakana_d3d12_rhi_tests mirakana_renderer_tests mirakana_scene_renderer_tests mirakana_runtime_scene_rhi_tests'` passed after implementation.
- Focused tests passed: `out\build\dev\Debug\mirakana_runtime_rhi_tests.exe`, `out\build\dev\Debug\mirakana_d3d12_rhi_tests.exe`, `out\build\dev\Debug\mirakana_renderer_tests.exe`, `out\build\dev\Debug\mirakana_scene_renderer_tests.exe`, and `out\build\dev\Debug\mirakana_runtime_scene_rhi_tests.exe`.
- D3D12 tangent-read strengthening passed: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_d3d12_rhi_tests'` and `out\build\dev\Debug\mirakana_d3d12_rhi_tests.exe`.
- Static checks passed before the final gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after the completion record update; CTest reported 29/29 tests passed and `validate: ok`.
