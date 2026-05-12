# Runtime RHI Compute Morph D3D12 Proof v1 (2026-05-05)

**Plan ID:** `runtime-rhi-compute-morph-d3d12-proof-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Use the first-party RHI compute pipeline/dispatch foundation to prove a narrow D3D12 primary-lane GPU morph compute
path that writes deterministic morphed vertex data through public `mirakana_rhi` contracts, without exposing native D3D12
handles or promoting generated-package compute morph readiness.

## Context

- `rhi-compute-dispatch-foundation-v1` added `ComputePipelineHandle`, `ComputePipelineDesc`,
  `IRhiDevice::create_compute_pipeline`, `IRhiCommandList::bind_compute_pipeline`, descriptor binding for compute, and
  `IRhiCommandList::dispatch`, with NullRHI validation and a D3D12 buffer readback proof.
- Existing graphics-pipeline morph slices upload POSITION/NORMAL/TANGENT morph streams and prove vertex-shader reads
  through D3D12 package smokes, but those paths still deform during graphics draws.
- The next production step should use the RHI compute contract for a small deterministic morph write/readback proof
  before attempting renderer scheduling, generated package wiring, or backend parity.

## Constraints

- Keep `engine/core`, `engine/assets`, `engine/runtime`, and gameplay code independent from RHI/native handles.
- Prefer `mirakana_runtime_rhi`/`mirakana_renderer` integration only where it removes duplicate test scaffolding; keep the proof
  narrow enough to validate through existing Windows `dev` D3D12 tests.
- Do not claim generated-game compute morph, async compute overlap, frame-graph scheduling, skin+morph composition,
  Vulkan/Metal parity, directional-shadow morph rendering, or broad renderer quality.
- Add RED tests before production implementation.

## Done When

- A focused test expresses the desired compute morph contract and fails before implementation.
- The implementation dispatches a D3D12 compute shader through `ComputePipelineDesc`, descriptor sets, and
  `IRhiCommandList::dispatch` to write deterministic morphed vertex bytes that are read back through public RHI APIs.
- Any reusable helper stays behind first-party RHI/runtime-renderer boundaries and records deterministic diagnostics.
- Docs, manifest, registry, and AI guidance describe this as a narrow D3D12 compute morph proof, not generated-package
  compute morph or backend parity.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Inspect the existing `mirakana_runtime_rhi` morph upload and `mirakana_renderer` morph binding paths to decide the narrowest
  reusable compute proof boundary.
- [x] Add a RED D3D12 test for compute-dispatched morph output readback using public RHI handles and descriptor sets.
- [x] Implement the compute morph proof helper/path without exposing native handles or broadening generated package
  claims.
- [x] Update docs, manifest, static checks, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"` failed before implementation with `C2039: 'create_runtime_morph_mesh_compute_binding'` and the matching missing identifier diagnostic.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"` built after adding `RuntimeMorphMeshComputeBinding` and `create_runtime_morph_mesh_compute_binding`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_runtime_rhi_tests --output-on-failure"` passed the NullRHI binding contract test.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_d3d12_rhi_tests"` built sequentially after a parallel MSVC PDB conflict on a shared static library.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure"` passed `d3d12 rhi compute morph writes morphed runtime positions`, proving the compute shader wrote `base + weighted deltas` into the output position buffer through public RHI dispatch and readback.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied clang-format to the new runtime RHI declarations/implementation.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed for the public runtime RHI header changes.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed with a synthesized dev compile database.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after docs, manifest, registry, skills, subagents, and static checks were synchronized.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 29/29 CTest tests; Metal and Apple packaging checks remained diagnostic-only host blockers.
