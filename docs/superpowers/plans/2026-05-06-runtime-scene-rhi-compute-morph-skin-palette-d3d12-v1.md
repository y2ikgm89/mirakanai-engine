# Runtime Scene RHI Compute Morph Skin Palette D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Carry the completed D3D12 runtime/renderer skin+compute-morph proof into the host-owned `runtime_scene_rhi` scene
palette bridge so selected cooked scene assets can resolve a compute-morphed skinned mesh binding without exposing RHI
or native handles to gameplay.

## Architecture

Keep the bridge host-owned: `mirakana_runtime` and `mirakana_scene` remain RHI-free, gameplay still sees only first-party asset and
scene contracts, and `runtime_scene_rhi` owns the conversion from package rows plus `SceneRenderPacket` meshes into
retained renderer bindings. Preserve distinct material, joint-palette, and compute-morph descriptor contracts instead
of reusing set index 1 for two meanings in one draw.

## Tech Stack

C++23, `mirakana_runtime_scene_rhi`, `mirakana_runtime_rhi`, `mirakana_scene_renderer`, `mirakana_renderer`, D3D12/NullRHI focused tests, static
docs checks.

---

## Context

- Runtime RHI Compute Morph Skin Composition D3D12 v1 added `make_runtime_compute_morph_skinned_mesh_gpu_binding`,
  `SkinnedMeshGpuBinding::skin_attribute_vertex_buffer`, and a D3D12 readback proof where compute-written POSITION
  output is skinned through a distinct joint-palette descriptor set.
- `runtime_scene_rhi` can already build retained static mesh, material, morph, and skinned palettes for selected scene
  assets, but it still treats morph and skin composition as unsupported because graphics morph and joint palette
  descriptors both use set index 1.
- This slice should bridge only the compute-morph output path with skinned bindings. Graphics-pipeline morph+skin,
  generated package smoke, scene-schema authoring, and backend parity remain separate work.

## Constraints

- Do not expose `IRhiDevice`, command lists, descriptor handles, native handles, backend stats, or retained palette
  internals to gameplay.
- Do not claim generated package skin+compute readiness, graphics morph+skin composition, async compute overlap,
  Vulkan/Metal parity, directional-shadow morph rendering, scene-schema compute-morph authoring, or broad skeletal
  animation readiness.
- Preserve existing static mesh, material, morph-only, skinned-only, compute-morph, and renderer proof tests.
- Keep any new scene palette diagnostics first-party and deterministic.

## Done When

- A RED `runtime_scene_rhi` test fails first because selected scene assets cannot resolve a compute-morphed skinned mesh
  into a retained `SceneSkinnedGpuBindingPalette` entry with separate skin attribute and joint-palette contracts.
- The bridge creates only first-party renderer/runtime-RHI value types and keeps RHI/native details host-owned.
- Focused NullRHI or D3D12-backed tests prove the composed scene palette path without promoting generated package smoke.
- Docs, manifest, plan registry, static checks, and rendering skills describe this as runtime scene RHI skin+compute
  palette composition only.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect current `RuntimeSceneGpuBindingOptions`, `SceneSkinnedGpuBindingPalette`, compute-morph setup, and
      descriptor-set assumptions in `runtime_scene_rhi`.
- [x] Add a RED scene palette test for selected assets that require compute-morph output plus skinned joint attributes.
- [x] Implement the minimal host-owned bridge using the existing `make_runtime_compute_morph_skinned_mesh_gpu_binding`
      contract.
- [x] Add focused proof counters/diagnostics without exposing RHI/native handles to gameplay.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_scene_rhi_tests"`
  failed because `RuntimeSceneGpuBindingOptions::compute_morph_skinned_mesh_bindings`,
  `RuntimeSceneComputeMorphSkinnedMeshBinding`, and compute-morph skinned report fields did not exist.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_scene_rhi_tests"`
  and `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_runtime_scene_rhi_tests --output-on-failure"`
  passed after adding the host-owned `runtime_scene_rhi` bridge, retained compute resources, and
  `compute_morph_skinned_mesh_bindings` / `compute_morph_output_position_bytes` report counters.
- Focused regression: `mirakana_runtime_rhi_tests` and `mirakana_d3d12_rhi_tests` build/CTest passed after the bridge, preserving
  the direct runtime/renderer D3D12 skin+compute proof.
- Static/docs validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after manifest/docs/skills/static-check synchronization.
- Format/API/tidy validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after rerunning outside the sandbox because the first
  sandboxed run could not access the sandbox cwd; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed; `pwsh -NoProfile
  -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Diagnostic-only host gates remain unchanged: Metal shader tooling is
  missing `metal` / `metallib` on Windows, Apple packaging requires macOS/Xcode tools, Android release signing is not
  configured, and no Android device-smoke target is connected.
