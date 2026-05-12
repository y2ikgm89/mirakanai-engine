# RHI Compute Dispatch Foundation v1 (2026-05-05)

**Plan ID:** `rhi-compute-dispatch-foundation-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a backend-neutral compute pipeline and dispatch command foundation so the later compute GPU morph path can be built
on first-party RHI contracts instead of special-casing native D3D12 handles.

## Context

- `gpu-morph-d3d12-proof-v1`, `gpu-morph-normal-tangent-d3d12-proof-v1`, and generated 3D morph package slices prove
  POSITION/NORMAL/TANGENT morph streams through graphics-pipeline vertex shader reads.
- `IRhiDevice` already models `QueueKind::compute`, `ShaderStage::compute`, and compute-visible descriptor stages, but
  the public RHI contract does not yet expose compute pipeline creation, compute pipeline binding, or dispatch commands.
- A clean compute morph implementation needs a small dispatch foundation first, with NullRHI validation and a D3D12
  readback proof before morph-specific package wiring.

## Constraints

- Keep native D3D12/Vulkan/Metal handles private behind RHI backends.
- Add explicit value types, lifetime stats, and command validation consistent with existing graphics pipeline patterns.
- Do not add compute morph integration, generated-game package wiring, frame-graph scheduling, async compute overlap,
  Vulkan/Metal parity claims, skin+morph composition, or renderer quality claims in this slice.
- Prefer NullRHI and D3D12 primary-lane proof first; Vulkan/Metal compute can remain host/toolchain-gated follow-up
  unless the existing backend surface can be validated without broadening scope.
- Add RED tests before production implementation.

## Done When

- `mirakana_rhi` exposes a public `ComputePipelineHandle` / `ComputePipelineDesc` and `IRhiDevice::create_compute_pipeline`.
- `IRhiCommandList` can bind a compute pipeline, bind descriptor sets for compute, and dispatch workgroups with
  deterministic invalid-state diagnostics/stats in NullRHI.
- D3D12 has a focused compute readback proof that dispatches a first-party compute shader to write a buffer and reads
  deterministic bytes back without exposing native handles.
- Docs, manifest, registry, and AI guidance describe this as a compute-dispatch foundation for later compute GPU morph,
  not as compute morph itself.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED NullRHI tests for compute pipeline creation, command-list binding, dispatch stats, and invalid ordering.
- [x] Add RED D3D12 readback proof for a tiny compute shader writing deterministic buffer bytes.
- [x] Add public RHI compute pipeline descriptors, handles, stats, command-list methods, and device creation method.
- [x] Implement NullRHI validation/lifetime tracking and D3D12 compute PSO/dispatch recording.
- [x] Update docs, manifest, static checks, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_rhi_tests"` failed before implementation because `ComputePipelineDesc`, `IRhiDevice::create_compute_pipeline`, `IRhiCommandList::bind_compute_pipeline`, `IRhiCommandList::dispatch`, and compute dispatch stats did not exist.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_rhi_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_rhi_tests --output-on-failure"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_d3d12_rhi_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure"` passed.
- GREEN after formatting/docs/manifest sync: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- GREEN after public RHI/backend header changes: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- GREEN after shared C++ implementation changes: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed.
- GREEN after AI guidance/static check sync: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- GREEN final repository gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including build plus 29/29 CTest tests.
