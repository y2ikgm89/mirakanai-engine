# sample-generated-desktop-runtime-material-shader-package

## Goal

Describe the desktop runtime game goal here before expanding gameplay.

## Current Runtime

This game uses the optional desktop runtime package lane with first-party material/shader authoring inputs and a cooked scene package:

- `mirakana::GameApp`
- `mirakana::SdlDesktopGameHost`
- `mirakana::RootedFileSystem`
- `mirakana::runtime::load_runtime_asset_package`
- `mirakana::instantiate_runtime_scene_render_data`
- `mirakana::submit_scene_render_packet`
- `source/materials/lit.material` as the authoring material mirror for the cooked runtime material
- `shaders/runtime_scene.hlsl` and `shaders/runtime_postprocess.hlsl` as host-built shader inputs
- D3D12 DXIL artifacts installed by the selected desktop runtime package target
- Vulkan SPIR-V artifacts only when DXC SPIR-V CodeGen and `spirv-val` are available and requested
- deterministic `NullRenderer` fallback when native presentation gates are unavailable
- `game.agent.json.runtimePackageFiles` plus `PACKAGE_FILES_FROM_MANIFEST`
- `game.agent.json.materialShaderAuthoringTargets` for the source material, cooked runtime material, fixed HLSL inputs, and selected shader artifact paths

The generated game does not runtime-compile shaders, expose native handles to gameplay, create a shader graph, generate Metal libraries, or ship source material/HLSL files as runtime package payloads.

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package
```

The installed D3D12 package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_material_shader_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_material_shader_package.config --require-scene-package runtime/sample_generated_desktop_runtime_material_shader_package.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess
```

The Vulkan package lane is toolchain-gated:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/sample_generated_desktop_runtime_material_shader_package.config', '--require-scene-package', 'runtime/sample_generated_desktop_runtime_material_shader_package.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess')
```
