# Runtime RHI Compute Morph Normal Tangent Output D3D12 v1 (2026-05-06)

**Plan ID:** `runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Extend the existing D3D12 primary compute morph proof from POSITION-only output into optional NORMAL and TANGENT
compute output buffers so tangent-frame morph data can be validated without moving broad renderer/package claims forward.

## Context

- `runtime-rhi-compute-morph-d3d12-proof-v1` added `RuntimeMorphMeshComputeBinding`,
  `create_runtime_morph_mesh_compute_binding`, and a D3D12 readback proof for morphed POSITION bytes.
- `runtime-rhi-compute-morph-renderer-consumption-d3d12-v1` added
  `make_runtime_compute_morph_output_mesh_gpu_binding` so a D3D12 `RhiFrameRenderer` can consume the compute-written
  POSITION output as vertex input.
- Generated package smoke currently remains explicitly POSITION-only for compute morph. Existing GPU graphics morph
  paths already cover optional NORMAL/TANGENT buffers; compute output parity is still a production gap.

## Constraints

- Keep the slice inside `mirakana_runtime_rhi` and D3D12 tests. Do not update generated package smoke yet unless a focused
  follow-up plan selects that packaging surface.
- Preserve the current POSITION-only path and make NORMAL/TANGENT output opt-in based on uploaded morph streams and
  output usage options.
- Keep public gameplay APIs free of `IRhiDevice`, D3D12, native handles, SDL3, Dear ImGui, and editor APIs.
- Do not claim Vulkan/Metal parity, async compute overlap, skin+morph composition, directional-shadow morph rendering,
  or broad renderer quality.
- Add RED coverage before production implementation.

## Done When

- A focused test fails first for expected optional NORMAL/TANGENT compute output fields on
  `RuntimeMorphMeshComputeBinding`.
- `create_runtime_morph_mesh_compute_binding` can allocate/bind optional output NORMAL and TANGENT buffers when the
  uploaded morph payload contains those streams and the caller requests suitable usage.
- A D3D12 readback test proves deterministic normalized morphed NORMAL and TANGENT bytes from compute output buffers.
- Docs, manifest, and static AI integration checks distinguish this D3D12 compute output proof from generated package
  smoke and broad renderer claims.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect current compute morph binding descriptors, D3D12 compute shader helpers, and morph upload stream metadata.
- [x] Add RED runtime_rhi test coverage for optional normal/tangent output buffer handles, byte counts, and descriptor
      binding expectations.
- [x] Add RED D3D12 readback coverage that dispatches a compute shader writing morphed NORMAL/TANGENT output rows.
- [x] Implement minimal `RuntimeMorphMeshComputeBindingOptions` and `RuntimeMorphMeshComputeBinding` fields for optional
      normal/tangent outputs while preserving POSITION-only callers.
- [x] Extend compute descriptor layout/update logic for optional normal/tangent delta inputs and output buffers with
      deterministic diagnostics when requested streams are unavailable.
- [x] Update docs, manifest, plan registry, static checks, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"` failed before implementation because `RuntimeMorphMeshComputeBindingOptions::output_normal_usage`, `output_tangent_usage`, and the output NORMAL/TANGENT binding fields did not exist.
- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_d3d12_rhi_tests"` failed before implementation on the same missing public compute binding fields while the new D3D12 readback test referenced them.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"` passed after adding opt-in NORMAL/TANGENT compute output fields and descriptor binding logic.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_runtime_rhi_tests --output-on-failure"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_d3d12_rhi_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure"` passed and covered `d3d12 rhi compute morph writes morphed runtime normals and tangents`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` after applying clang-format to `engine/runtime_rhi/src/runtime_upload.cpp`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (29/29 CTest tests passed; shader-toolchain Metal and Apple packaging entries remained diagnostic-only host gates on Windows).
