# Runtime RHI Compute Morph Skin Composition D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-skin-composition-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a narrow D3D12 proof that compute-morphed runtime mesh output can be composed with the existing first-party GPU
skinning contract without exposing native RHI handles or claiming broad skeletal animation production readiness.

## Architecture

Keep the composition at renderer/runtime-RHI boundaries: gameplay and generated packages must still see only first-party
mesh, skin, morph, and smoke counters. Resolve the current set-index conflict between skinned joint palettes and morph
or compute-morph resources explicitly instead of relying on implicit descriptor layout overlap.

## Tech Stack

C++23, `mirakana_renderer`, `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, `mirakana_rhi_d3d12`, D3D12 focused tests, static docs checks.

---

## Context

- Runtime RHI compute morph output now supports POSITION/NORMAL/TANGENT output buffers, renderer consumption, public
  queue-to-queue synchronization, and generated package queue-wait smoke counters.
- GPU skinning already has a D3D12 primary proof lane with `SkinnedMeshGpuBinding`, joint descriptor sets, and
  `RendererStats::gpu_skinning_draws`.
- `runtime_scene_rhi` currently documents that skinning and morphing together are intentionally unsupported because
  skinned joint palettes and morph-only resources both occupy descriptor set index 1.
- This plan should remove only the next smallest blocker: a deterministic D3D12 proof that a skinned draw can consume a
  compute-morphed mesh binding while preserving a distinct joint-palette descriptor contract.

## Constraints

- Do not expose `IRhiDevice`, command lists, descriptor handles, native D3D12 handles, or backend stats to gameplay.
- Do not claim generated package support, scene-schema compute-morph authoring, async compute overlap/performance,
  Vulkan/Metal parity, directional-shadow morph rendering, or broad skeletal animation readiness.
- Preserve existing graphics morph, compute morph, and skinned-only proofs.
- Keep descriptor-set layout changes explicit and covered by NullRHI/D3D12 tests.

## Done When

- A focused RED test fails first because skinned draws cannot consume a compute-morphed mesh binding with a distinct
  joint-palette descriptor contract.
- Runtime/renderer contracts expose only first-party value types for the composition path.
- D3D12 proves the composed path with deterministic readback or renderer stats while native handles stay private.
- Docs, manifest, plan registry, static checks, and rendering skills describe this as D3D12 skin+compute-morph
  composition only, not generated package readiness or async overlap.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect current `MeshCommand`, `SkinnedMeshGpuBinding`, compute-morph output bindings, and D3D12 pipeline layout
      set-index assumptions.
- [x] Add a RED renderer/runtime-RHI test for a skinned draw that consumes a compute-morphed mesh output through a
      distinct joint-palette descriptor set.
- [x] Implement the minimal first-party composition contract without exposing native handles.
- [x] Add D3D12 readback or stats proof for the composed path.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"`
  failed because `mirakana::runtime_rhi::make_runtime_compute_morph_skinned_mesh_gpu_binding` did not exist.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"`
  and `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_runtime_rhi_tests --output-on-failure"`
  passed after adding the first-party composition helper and `SkinnedMeshGpuBinding::skin_attribute_vertex_buffer`.
- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure"`
  failed at the composed center-pixel readback before `RhiFrameRenderer` bound the separate skin attribute vertex stream.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_d3d12_rhi_tests"`
  and `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure"`
  passed after binding the skin attribute stream at vertex slot 3 for skinned draws.
- Static/format/API/tidy/final validation passed with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`,
  focused `mirakana_runtime_rhi_tests`, `mirakana_d3d12_rhi_tests`, `mirakana_renderer_tests`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`. The final
  validate run reports Metal shader tools and Apple packaging/signing as diagnostic-only host blockers on this Windows
  host.
