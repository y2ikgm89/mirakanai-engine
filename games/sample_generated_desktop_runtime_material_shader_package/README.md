# sample-generated-desktop-runtime-material-shader-package

## Goal

Describe the desktop runtime game goal here before expanding gameplay.

## Current Runtime

This game uses the optional desktop runtime package lane with first-party material/shader authoring inputs and a cooked scene package:

- `mirakana::GameApp`
- `mirakana::Win32DesktopGameHost`
- `mirakana::RootedFileSystem`
- `mirakana::runtime::load_runtime_asset_package`
- `mirakana::instantiate_runtime_scene_render_data`
- `mirakana::submit_scene_render_packet`
- `source/materials/lit.material` as the authoring material mirror for the cooked runtime material
- `source/materials/lit.materialgraph`, `source/materials/lit.shader_export`, and `shaders/material_graph_lit.hlsl` as reviewed material graph production-authoring inputs
- `shaders/runtime_scene.hlsl` and `shaders/runtime_postprocess.hlsl` as host-built shader inputs; the current Win32 package smoke uses the scene shader path while postprocess execution remains unsupported on this host path
- D3D12 DXIL artifacts installed by the selected desktop runtime package target
- Vulkan SPIR-V artifacts only when DXC SPIR-V CodeGen and `spirv-val` are available and requested; they are shader-artifact evidence only until non-SDL Vulkan presentation is designed
- deterministic `NullRenderer` fallback when native presentation gates are unavailable
- `game.agent.json.runtimePackageFiles` plus `PACKAGE_FILES_FROM_MANIFEST`
- `game.agent.json.materialShaderAuthoringTargets` for the source material, material graph, shader export descriptor, reviewed HLSL input, cooked runtime material, fixed HLSL inputs, selected compile-request targets, and selected shader artifact paths
- `game.agent.json.packageStreamingResidencyTargets` as host-gated safe-point package streaming intent

The generated game does not runtime-compile shaders, execute arbitrary shader graphs, expose native handles to gameplay, generate Metal libraries, execute package streaming, or ship source material/graph/HLSL files as runtime package payloads.

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package
```

The installed D3D12 package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_material_shader_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_material_shader_package.config --require-scene-package runtime/sample_generated_desktop_runtime_material_shader_package.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-material-graph-authoring
```

The Vulkan package lane is toolchain-gated:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/sample_generated_desktop_runtime_material_shader_package.config', '--require-scene-package', 'runtime/sample_generated_desktop_runtime_material_shader_package.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-material-graph-authoring')
```
