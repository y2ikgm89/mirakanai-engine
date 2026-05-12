# Generated 3D Compute Morph Skin Package Smoke D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `generated-3d-compute-morph-skin-package-smoke-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Promote the completed runtime-scene RHI compute-morph skinned palette bridge into the generated
`DesktopRuntime3DPackage` D3D12 smoke path so generated packages can request and verify a package-visible
skin+compute-morph scene GPU path without exposing RHI or native handles to gameplay.

## Architecture

Keep package smoke ownership in the SDL3 desktop runtime host: generated gameplay may declare first-party package files,
shader artifact metadata, selected mesh-to-morph rows, and smoke requirements, but command lists, descriptor sets,
queue/fence handles, retained palettes, and native D3D12 objects stay inside engine/host adapters.

## Tech Stack

C++23, `mirakana_runtime_host_sdl3_presentation`, `mirakana_runtime_scene_rhi`, `mirakana_runtime_rhi`, `mirakana_renderer`, generated
`DesktopRuntime3DPackage` scaffolding, D3D12 focused smoke/tests, static docs checks.

---

## Context

- Runtime RHI Compute Morph Skin Composition D3D12 v1 proved a D3D12 renderer draw can consume compute-written POSITION
  output with GPU skinning attributes through a distinct joint-palette descriptor set.
- Runtime Scene RHI Compute Morph Skin Palette D3D12 v1 carried that proof into `mirakana_runtime_scene_rhi` as a host-owned
  selected-asset bridge using `RuntimeSceneComputeMorphSkinnedMeshBinding` and retained
  `SceneSkinnedGpuBindingPalette` entries.
- Generated `DesktopRuntime3DPackage` D3D12 compute morph smoke already proves POSITION/NORMAL/TANGENT compute output,
  queue-wait evidence, and package-visible counters for non-skinned meshes.

## Constraints

- Do not expose `IRhiDevice`, command lists, descriptor handles, queue/fence handles, native D3D12 handles, retained
  palette internals, or backend stats to generated gameplay.
- Do not claim graphics morph+skin composition, async compute overlap/performance, Vulkan/Metal parity,
  directional-shadow morph rendering, scene-schema compute-morph authoring, or broad skeletal animation readiness.
- Preserve existing generated POSITION/NORMAL/TANGENT compute morph package smokes and runtime-scene bridge tests.
- Keep package smoke diagnostics first-party, deterministic, and manifest-visible.

## Done When

- A RED generated/package or runtime-host test fails first because `DesktopRuntime3DPackage` cannot request a
  compute-morph skinned mesh package smoke path.
- Generated D3D12 package smoke can report and require first-party skin+compute counters without gameplay-native handle
  exposure.
- Focused D3D12/package tests prove the host-owned path, while Vulkan/Metal and async overlap remain gated.
- Docs, manifest, plan registry, static checks, and skills describe this as generated D3D12 package skin+compute smoke
  only.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect generated `DesktopRuntime3DPackage` compute morph shader/artifact, scene binding, and smoke counter flow.
- [x] Add a RED package/runtime-host test for requested D3D12 compute-morph skinned smoke counters.
- [x] Implement the minimal generated package and SDL3 host wiring using the runtime-scene RHI bridge.
- [x] Add package-visible counters/diagnostics without exposing RHI/native handles.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed because generated `DesktopRuntime3DPackage` did not create
  `runtime/assets/3d/skinned_triangle.skinned_mesh`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after generated package scaffolding emitted the skinned mesh row,
  `--require-compute-morph-skin`, D3D12 skinned compute shader artifact metadata, host-owned scene binding wiring, and
  first-party package counters.
- Focused runtime-host checks passed:
  - `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile"`
  - `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_host_tests"`
  - `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_runtime_host_tests --output-on-failure"`
- Static/quality checks passed so far:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`
- Final validation passed:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
