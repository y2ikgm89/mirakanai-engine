# Generated 3D Morph Normal Tangent Package Smoke v1 (2026-05-05)

**Plan ID:** `generated-3d-morph-normal-tangent-package-smoke-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Extend generated `DesktopRuntime3DPackage` D3D12 morph smoke from POSITION-delta-only shader/data scaffolding to a
host-owned package path that ships optional NORMAL and TANGENT morph delta streams and uses them in the generated morph
vertex shader.

## Context

- `gpu-morph-normal-tangent-d3d12-proof-v1` proved direct D3D12 `RhiFrameRenderer` reads for POSITION plus optional
  NORMAL/TANGENT morph delta buffers.
- `generated-3d-morph-visible-deformation-v1` wired generated `DesktopRuntime3DPackage` mesh-to-morph bindings and
  renderer morph counters, but the generated package shader/payload contract stayed POSITION-delta-only.
- `RuntimeMorphMeshCpuPayload`, `mirakana_runtime_rhi`, and `mirakana_runtime_scene_rhi` can now upload optional NORMAL/TANGENT
  streams when the cooked `morph_mesh_cpu` payload contains them.

## Constraints

- Keep `IRhiDevice`, native handles, descriptor sets, and morph palettes host-owned inside runtime-host/RHI adapters.
- Do not add scene schema morph components, authoring graph semantics, skin+morph composition, compute morph, or
  directional-shadow morph rendering.
- Keep this proof D3D12-primary and package-smoke-oriented. Vulkan shader artifact generation may stay toolchain-gated
  and must not become a broad backend parity claim.
- Align generated scene vertex input with the cooked tangent-frame mesh layout instead of weakening runtime layout
  validation.
- Add RED static/integration coverage before production changes.

## Done When

- Generated 3D cooked and source `morph_mesh_cpu` files include bind NORMAL/TANGENT streams plus target NORMAL/TANGENT
  delta streams.
- Generated `runtime_scene.hlsl` declares tangent-space vertex input and reads morph POSITION, NORMAL, and TANGENT
  delta buffers using the stable morph descriptor bindings `0`, `2`, and `3` with weights at binding `1`.
- Generated scene renderer vertex input matches the cooked tangent-frame mesh layout used by `mirakana_runtime_rhi`.
- Static integration checks reject a generated 3D package scaffold that lacks NORMAL/TANGENT morph payload data or
  shader reads.
- Docs, manifest, and registry describe this as generated-package D3D12 NORMAL/TANGENT morph smoke, while compute morph,
  scene-schema morph authoring, skin+morph, directional-shadow morph rendering, Vulkan/Metal parity, and broad generated
  3D readiness remain follow-up work.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED static integration checks for generated 3D NORMAL/TANGENT morph payloads, shader reads, and tangent-frame
  vertex input.
- [x] Update generated 3D cooked/source morph payload fixtures with NORMAL/TANGENT bind and delta streams.
- [x] Update generated scene vertex input and HLSL morph shader to use POSITION/NORMAL/TANGENT morph buffers.
- [x] Update docs, manifest, registry, and static checks for the narrow generated package smoke capability.
- [x] Run focused static checks, desktop-runtime validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and record evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before implementation because `DesktopRuntime3DPackage` generated morph payloads
  lacked `morph.bind_normals_hex=`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after generated payload, shader, vertex-input, docs, manifest, and registry
  updates.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed after the generated scaffold changes; desktop-runtime CTest reported
  `16/16` tests passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after final evidence updates; CTest reported `29/29` tests passed and
  `validate: ok`.
