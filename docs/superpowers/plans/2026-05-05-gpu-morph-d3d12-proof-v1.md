# GPU Morph D3D12 Proof v1 (2026-05-05)

**Plan ID:** `gpu-morph-d3d12-proof-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Prove the first renderer-visible GPU morph deformation path on the D3D12 primary lane. A runtime
`morph_mesh_cpu` payload should be uploadable as GPU-side POSITION delta data plus weight constants, then drawn
through `RhiFrameRenderer` as a visibly morphed indexed mesh using RHI-neutral public contracts.

## Context

- `generated-3d-morph-package-consumption-v1` completed CPU package smoke counters through
  `sample_runtime_morph_mesh_cpu_animation_float_clip`, but it intentionally did not render morphed geometry.
- `gpu-skinning-upload-and-rendering-v1` already established the D3D12 pattern: material set 0, a per-mesh vertex
  descriptor set at set 1, and an optional `RhiFrameRenderer` specialized pipeline.
- Microsoft Direct3D 12 guidance keeps shader resources behind root signatures/descriptors, binds vertex/index buffers
  directly on command lists, and recommends keeping root signatures as small as necessary. This slice follows that
  shape by using one additional descriptor table for morph resources rather than exposing D3D12 handles or bulk root
  data.

## Constraints

- Keep public contracts RHI-neutral. Do not expose D3D12, DXGI, HWND, COM, or native descriptor handles.
- Keep `engine/core` independent of renderer/RHI, OS APIs, GPU APIs, and asset formats.
- Support POSITION deltas only for this proof. NORMAL/TANGENT GPU morph, compute morph, animation graph integration,
  generated package smoke, Vulkan/Metal parity, and broad renderer-visible morph readiness remain follow-up work.
- Reuse the existing runtime morph payload shape and the existing D3D12 GPU skinning descriptor/pipeline pattern.
- Add RED tests first where feasible before implementation changes.

## Done When

- `mirakana_runtime_rhi` can upload a valid `RuntimeMorphMeshCpuPayload` into a storage buffer for POSITION deltas and a
  256-byte-backed uniform buffer for target weights.
- A per-mesh morph descriptor set can bind the delta storage buffer and weight uniform buffer at set 1.
- `RhiFrameRenderer` can draw a `MeshCommand` with GPU morph enabled through an optional morph graphics pipeline,
  bind material set 0 and morph set 1, and report dedicated renderer counters.
- A D3D12 visible test renders a base triangle whose center pixel only becomes covered after the GPU morph shader
  applies the uploaded delta/weight data.
- Docs, manifest, and agent static checks state the narrow D3D12 proof and keep broad/backend/generated-package claims
  unsupported.
- Validation evidence is recorded below, including `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` or a concrete blocker.

## Tasks

- [x] Add RED coverage for runtime morph GPU upload/descriptor binding.
- [x] Add RED D3D12 readback coverage for visible morph deformation through `RhiFrameRenderer`.
- [x] Add RHI-neutral morph GPU binding and upload helpers in `mirakana_runtime_rhi`.
- [x] Add optional morph draw path, validation, and counters in renderer contracts and `RhiFrameRenderer`.
- [x] Update docs, manifest, registry, and static agent checks for the narrow capability.
- [x] Run focused tests, format/API/agent checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and record evidence.

## Validation Evidence

- RED focused build: `mirakana_runtime_rhi_tests.vcxproj` failed before implementation with missing
  `upload_runtime_morph_mesh_cpu`, `make_runtime_morph_mesh_gpu_binding`, and
  `attach_morph_mesh_descriptor_set`.
- Focused build: `mirakana_runtime_rhi_tests.vcxproj` passed with 0 warnings / 0 errors.
- Focused test: `out\build\dev\Debug\mirakana_runtime_rhi_tests.exe` passed, including
  `runtime rhi morph mesh upload binds position delta storage and weight descriptor set`.
- Focused build: `mirakana_d3d12_rhi_tests.vcxproj` passed with 0 warnings / 0 errors after a non-code parallel-PDB
  retry.
- Focused test: `out\build\dev\Debug\mirakana_d3d12_rhi_tests.exe` passed, including
  `d3d12 rhi frame renderer visibly draws uploaded runtime morph mesh vertices`.
- Focused build/test: `mirakana_renderer_tests.vcxproj` and `out\build\dev\Debug\mirakana_renderer_tests.exe` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed; 29/29 CTest tests passed. Metal and Apple checks remained diagnostic-only host gates
  on this Windows host.
