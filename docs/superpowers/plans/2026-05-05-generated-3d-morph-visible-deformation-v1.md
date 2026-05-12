# Generated 3D Morph Visible Deformation v1 (2026-05-05)

**Plan ID:** `generated-3d-morph-visible-deformation-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Make generated `DesktopRuntime3DPackage` D3D12 scene GPU smoke execute renderer-visible POSITION-delta morph draws
through the host-owned RHI path, without adding scene schema morph semantics or broad backend parity claims.

## Context

- `gpu-morph-d3d12-proof-v1` proved a direct D3D12 `RhiFrameRenderer` POSITION-delta morph draw.
- `runtime-scene-rhi-morph-gpu-palette-v1` retained selected cooked `morph_mesh_cpu` rows as host-owned
  `SceneMorphGpuBindingPalette` entries.
- `generated-3d-morph-gpu-palette-smoke-v1` exposed those palette upload counters through SDL desktop presentation and
  generated `DesktopRuntime3DPackage` scene GPU smoke.
- Before this slice, generated 3D package smoke did not pair a scene mesh draw with the selected morph binding or use a
  morph scene shader/pipeline.

## Constraints

- Keep `IRhiDevice`, descriptor handles, native handles, and morph palettes host-owned.
- Use explicit generated mesh-to-morph mapping; do not infer relationships from all package rows or add scene schema
  morph components.
- Keep GPU skinning + GPU morphing together unsupported in this slice.
- Prove the visible path on the D3D12 primary lane. Vulkan shader artifact wiring may remain strict host/toolchain gated;
  do not claim broad Vulkan/Metal parity.
- Keep postprocess support narrow to the generated package scene path; do not extend directional shadow morph rendering
  unless a separate slice selects it.
- Add RED coverage before production behavior.

## Done When

- SDL desktop scene renderer descs can carry a first-party mesh-to-morph binding map and optional morph vertex shader
  bytecode.
- The scene GPU binding injector sets `MeshCommand::gpu_morphing` and resolves a `MorphMeshGpuBinding` for explicitly
  mapped mesh draws.
- `RhiPostprocessFrameRenderer` accepts a morph scene graphics pipeline and records morph descriptor binds/draw counters
  for morphed scene meshes.
- Runtime scene RHI material pipeline layout construction includes the morph descriptor set layout for morph-only scene
  paths, while preserving the existing skinned-layout precedence.
- Generated `DesktopRuntime3DPackage` emits a morph vertex shader, requests the mesh-to-morph binding, prints
  `scene_gpu_morph_mesh_resolved`, `renderer_gpu_morph_draws`, and `renderer_morph_descriptor_binds`, and makes
  `--require-morph-package --require-scene-gpu-bindings` require visible morph draw counters on the primary D3D12 path.
- Docs/manifest/static checks distinguish this from NORMAL/TANGENT/compute morph, skin+morph composition, directional
  shadow morph rendering, Vulkan/Metal parity, and broad generated 3D readiness.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete blocker.

## Tasks

- [x] Add RED runtime_scene_rhi coverage for morph-only material pipeline layouts including the morph descriptor set.
- [x] Add RED runtime_host_sdl3 coverage for mesh-to-morph desc fields and injector-resolved morph commands.
- [x] Add RED renderer coverage for `RhiPostprocessFrameRenderer` morph scene pipeline counters.
- [x] Add SDL scene morph binding descriptors, stats, and injector mapping.
- [x] Add postprocess morph pipeline support and preserve the existing skinned/morph mutual exclusion.
- [x] Wire D3D12/Vulkan scene renderer morph shader creation and pipeline creation from SDL presentation descs.
- [x] Extend generated 3D package shader/template/static checks and D3D12 smoke assertions.
- [x] Update docs, manifest, registry, and validation evidence.
- [x] Run focused tests, format/API/agent checks, desktop-runtime validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and record evidence.

## Validation Evidence

- RED, 2026-05-05: `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests` failed after adding host tests because `SdlDesktopPresentationSceneMorphMeshBinding`, `morph_vertex_shader`, `morph_mesh_bindings`, and `morph_mesh_bindings_resolved` were not implemented yet.
- RED, 2026-05-05: `cmake --build --preset desktop-runtime --target mirakana_renderer_tests` failed after adding renderer tests because `RhiPostprocessFrameRendererDesc::scene_morph_graphics_pipeline` did not exist yet.
- RED, 2026-05-05: `out\build\desktop-runtime\Debug\mirakana_runtime_scene_rhi_tests.exe` failed with `rhi descriptor set index is outside the pipeline layout`, proving the morph descriptor set layout was not included in morph-only scene material pipeline layouts.
- GREEN, 2026-05-05: `cmake --preset desktop-runtime` completed.
- GREEN, 2026-05-05: `tools/check-ai-integration.ps1` completed with `ai-integration-check: ok`.
- GREEN, 2026-05-05: `tools/check-json-contracts.ps1` completed with `json-contract-check: ok`.
- GREEN, 2026-05-05: `cmake --build --preset desktop-runtime --target sample_desktop_runtime_game mirakana_runtime_host_sdl3_tests mirakana_renderer_tests mirakana_runtime_scene_rhi_tests mirakana_runtime_host_sdl3_public_api_compile` completed through the sanitized `cmd.exe` Path workaround. The build emitted the existing Vulkan C4819 warning but exited successfully.
- GREEN, 2026-05-05: `out\build\desktop-runtime\Debug\mirakana_renderer_tests.exe`, `out\build\desktop-runtime\Debug\mirakana_runtime_scene_rhi_tests.exe`, `out\build\desktop-runtime\Debug\mirakana_runtime_host_sdl3_tests.exe`, and `out\build\desktop-runtime\Debug\mirakana_runtime_host_sdl3_public_api_compile.exe` all exited 0.
- GREEN, 2026-05-05: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` initially failed on `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` completed with `format: ok`, and rerun `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` completed with `format-check: ok`.
- GREEN, 2026-05-05: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` completed with `public-api-boundary-check: ok`.
- GREEN, 2026-05-05: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` completed with `ai-integration-check: ok`.
- GREEN, 2026-05-05: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` completed with `json-contract-check: ok`.
- DIAGNOSTIC, 2026-05-05: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` full 172-file clang-tidy run exceeded a 240-second local timeout after generating the dev compile database and surfacing existing broad warnings. The repository `validate` gate's configured tidy smoke later completed with `tidy-check: ok (1 files)`.
- DIAGNOSTIC, 2026-05-05: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` reported D3D12 DXIL, Vulkan SPIR-V, DXC SPIR-V CodeGen, `dxc`, and `spirv-val` ready; Metal `metal`/`metallib` were missing on this Windows host and reported as diagnostic-only blockers.
- GREEN, 2026-05-05: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` completed; desktop-runtime CTest reported 16/16 tests passed. The build emitted the existing Vulkan C4819 warning.
- GREEN, 2026-05-05: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed with `validate: ok`; dev CTest reported 29/29 tests passed, and diagnostic-only host gates remained Metal/Apple packaging on this Windows host.
