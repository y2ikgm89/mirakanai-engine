# Generated 3D Morph GPU Palette Smoke v1 (2026-05-05)

**Plan ID:** `generated-3d-morph-gpu-palette-smoke-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Expose the runtime-scene RHI morph GPU palette bridge through the SDL desktop scene presentation and generated
`DesktopRuntime3DPackage` smoke counters, without claiming renderer-visible morph deformation.

## Context

- `gpu-morph-d3d12-proof-v1` proved a D3D12 primary-lane POSITION-delta draw path.
- `runtime-scene-rhi-morph-gpu-palette-v1` retained explicitly selected cooked `morph_mesh_cpu` package rows as
  host-owned `SceneMorphGpuBindingPalette` resources.
- Generated `DesktopRuntime3DPackage` scaffolds already ship a cooked morph payload and CPU morph package smoke
  counters, but their scene GPU path does not yet request the morph GPU palette or report morph upload counters.

## Constraints

- Keep `IRhiDevice`, descriptor handles, native handles, and `SceneMorphGpuBindingPalette` host-owned.
- Do not add scene schema morph-component semantics or infer morph relations from all package rows.
- Do not claim generated-package renderer-visible morph deformation, NORMAL/TANGENT GPU morph, compute morph, or
  Vulkan/Metal visible morph parity.
- Use explicit selected morph ids from the generated package scaffold.
- Add RED coverage before production behavior.

## Done When

- `SdlDesktopPresentationD3d12SceneRendererDesc` and `SdlDesktopPresentationVulkanSceneRendererDesc` can carry selected
  morph mesh asset ids into `RuntimeSceneGpuBindingOptions::morph_mesh_assets`.
- `SdlDesktopPresentationSceneGpuBindingStats` reports morph binding count, morph upload count, and uploaded morph bytes.
- `SceneGpuBindingInjectingRenderer` mirrors retained morph palette/upload stats without exposing the palette.
- Generated `DesktopRuntime3DPackage` source emits selected morph ids into scene GPU renderer descs and validates positive
  morph GPU palette counters when `--require-morph-package` and `--require-scene-gpu-bindings` are both requested.
- Docs/manifest/static checks distinguish this GPU palette smoke from renderer-visible morph deformation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete blocker.

## Tasks

- [x] Add RED runtime_host_sdl3 tests for morph stats fields and desc-selected morph asset ids.
- [x] Add SDL desktop presentation desc fields and scene GPU stats counters.
- [x] Pass selected morph assets into D3D12/Vulkan `RuntimeSceneGpuBindingOptions`.
- [x] Extend `SceneGpuBindingInjectingRenderer` stats from `SceneMorphGpuBindingPalette` and upload reports.
- [x] Update generated 3D package template smoke output/checks and static integration assertions.
- [x] Update docs, manifest, registry, and validation evidence.
- [x] Run focused tests, format/API/agent checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and record evidence.

## Validation Evidence

- RED: `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests` failed while the tests referenced missing `SdlDesktopPresentationSceneGpuBindingStats::morph_mesh_bindings`, `morph_mesh_uploads`, `uploaded_morph_bytes`, and missing D3D12/Vulkan scene renderer desc `morph_mesh_assets`.
- RED: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed while the generated `DesktopRuntime3DPackage` main did not contain `.morph_mesh_assets = {packaged_morph_mesh_asset_id()}`.
- GREEN: `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests` passed.
- GREEN: `out/build/desktop-runtime/Debug/mirakana_runtime_host_sdl3_tests.exe` passed, including `sdl scene gpu binding renderer reports retained morph palette upload counters` and `sdl desktop scene renderer descs carry selected morph mesh assets`.
- GREEN: `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile` passed.
- GREEN: `out/build/desktop-runtime/Debug/mirakana_runtime_host_sdl3_public_api_compile.exe` passed.
- GREEN: `cmake --build --preset desktop-runtime --target sample_desktop_runtime_game` passed after adding the committed sample's morph scene GPU report fields.
- GREEN: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- GREEN: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- GREEN: `git diff --check` exited 0 with only existing CRLF conversion warnings.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed, including 16/16 desktop-runtime CTest rows.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including `ctest --preset dev` 29/29 passed; Metal and Apple checks remained diagnostic-only host blockers as expected on Windows.
