# Runtime RHI Compute Morph Renderer Consumption D3D12 v1 (2026-05-06)

**Plan ID:** `runtime-rhi-compute-morph-renderer-consumption-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Consume the D3D12 primary-lane runtime RHI compute morph output as renderer vertex input in a narrow draw/readback proof,
without promoting generated-package compute morph readiness.

## Context

- `rhi-compute-dispatch-foundation-v1` added the first-party compute pipeline and dispatch contracts:
  `ComputePipelineDesc`, `ComputePipelineHandle`, `IRhiDevice::create_compute_pipeline`,
  `IRhiCommandList::bind_compute_pipeline`, descriptor-set binding, and `IRhiCommandList::dispatch`.
- `runtime-rhi-compute-morph-d3d12-proof-v1` added `RuntimeMorphMeshComputeBinding`,
  `create_runtime_morph_mesh_compute_binding`, and a D3D12 readback proof that writes deterministic morphed POSITION
  bytes into a storage/copy-source output buffer.
- The remaining gap before generated package wiring is proving that the compute-written output can be consumed by a
  renderer/RHI draw path without native D3D12 handles escaping and without broadening the supported production claim.

## Constraints

- Keep `engine/core`, `engine/assets`, `engine/runtime`, `mirakana_scene`, and gameplay code independent from RHI/native
  handles.
- Use first-party `mirakana_rhi`, `mirakana_runtime_rhi`, and existing renderer-facing binding contracts; do not expose
  `ID3D12Resource`, descriptor heap handles, command queues, fences, or swapchain internals.
- Keep the proof D3D12 primary-lane and deterministic. Vulkan, Metal, async compute overlap, frame-graph scheduling,
  generated-package compute morph, skin+morph composition, normal/tangent compute output, directional-shadow morph
  rendering, and broad renderer quality stay out of scope.
- Add or update focused RED tests before production implementation.

## Done When

- A focused test fails first for drawing from a compute-morphed POSITION output buffer through public RHI/renderer
  contracts.
- The implementation proves a compute dispatch can write morphed POSITION bytes and a subsequent D3D12 draw/readback
  can consume those bytes as vertex input without native handle exposure.
- Ownership, usage flags, synchronization assumptions, and diagnostics remain deterministic and first-party.
- Docs, manifest, registry, static checks, and AI guidance describe this as renderer consumption of a D3D12
  compute-morph position output only.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Inspect `RhiFrameRenderer`, `RuntimeMeshUploadResult`, and D3D12 copy/compute queue synchronization coverage to
  choose the narrowest public renderer-consumption boundary.
- [x] Add a RED D3D12 test that dispatches compute morph output and then draws from the output position buffer.
- [x] Implement the minimal helper or renderer path needed for vertex-input consumption of the compute output.
- [x] Update docs, manifest, static checks, AI guidance, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"` failed before implementation with `C2039: 'make_runtime_compute_morph_output_mesh_gpu_binding'` and the matching missing identifier diagnostic.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"` built after adding `make_runtime_compute_morph_output_mesh_gpu_binding`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_runtime_rhi_tests --output-on-failure"` passed the NullRHI helper contract test.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_d3d12_rhi_tests"` built the renderer-consumption proof.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure"` passed `d3d12 rhi frame renderer consumes compute morph output positions`, proving compute-written POSITION bytes can be consumed by `RhiFrameRenderer` as vertex input on D3D12.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` reformatted the new helper declaration/definition, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed afterward.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after adding the public runtime RHI helper.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed the validate-scoped tidy lane with `tidy-check: ok (1 files)`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after syncing docs, manifest, plan registry, static checks, and AI guidance.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok` and 29/29 CTest tests passing. The Metal shader tools and Apple packaging diagnostics remained host-lane blockers only, as expected on this Windows host.
