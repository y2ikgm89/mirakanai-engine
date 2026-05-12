# sample-generated-desktop-runtime-cooked-scene-package

## Goal

Describe the desktop runtime game goal here before expanding gameplay.

## Current Runtime

This game uses the optional desktop runtime package lane with a first-party cooked scene package:

- `mirakana::GameApp`
- `mirakana::SdlDesktopGameHost`
- `mirakana::RootedFileSystem`
- `mirakana::runtime::load_runtime_asset_package`
- `mirakana::instantiate_runtime_scene_render_data`
- `mirakana::submit_scene_render_packet`
- deterministic `NullRenderer` fallback unless host-owned shader artifacts are added later
- `game.agent.json.runtimePackageFiles` plus `PACKAGE_FILES_FROM_MANIFEST`

The generated package proves config and cooked scene loading only. It does not generate D3D12/Vulkan/Metal shader artifacts or claim scene GPU binding.

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_cooked_scene_package
```

The installed package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_cooked_scene_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_cooked_scene_package.config --require-scene-package runtime/sample_generated_desktop_runtime_cooked_scene_package.geindex
```
