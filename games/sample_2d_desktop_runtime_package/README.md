# Sample 2D Desktop Runtime Package

This sample is the host-gated desktop/package proof for the `2d-desktop-runtime-package` recipe.

- Runtime host: `mirakana::SdlDesktopGameHost`
- Gameplay contract: `mirakana::GameApp`, `mirakana_scene`, `mirakana_ui`, `mirakana_audio`, and public `mirakana::IRenderer`
- Package files: manifest-derived `runtimePackageFiles`, including deterministic data-only tilemap metadata
- Native presentation: owned by runtime host adapters, not game code
- D3D12 package window smoke: packaged `sample_2d_desktop_runtime_package_sprite.vs.dxil` and `sample_2d_desktop_runtime_package_sprite.ps.dxil` loaded through `mirakana::load_desktop_shader_bytecode_pair`, then required with `--require-d3d12-shaders --video-driver windows --require-d3d12-renderer`
- Native 2D sprite package proof: packaged `sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil` and `sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil` let the host-owned D3D12 RHI path upload the cooked sprite texture and draw scene/HUD sprites with `--require-native-2d-sprites`, including renderer-owned native sprite batch execution counters such as `native_2d_sprite_batches_executed`
- Sprite animation package proof: cooked `runtime/assets/2d/player.sprite_animation` is sampled through `mirakana::sample_and_apply_runtime_scene_render_sprite_animation`, then required with `--require-sprite-animation` and status counters such as `sprite_animation_frames_sampled`
- Tilemap runtime UX proof: cooked `runtime/assets/2d/level.tilemap` is sampled through `mirakana::runtime::sample_runtime_tilemap_visible_cells`, then required with `--require-tilemap-runtime-ux` and status counters such as `tilemap_cells_sampled`
- Vulkan package window smoke: toolchain/host-gated packaged `sample_2d_desktop_runtime_package_sprite.vs.spv` and `sample_2d_desktop_runtime_package_sprite.ps.spv`, required with `--require-vulkan-shaders --video-driver windows --require-vulkan-renderer`

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package
out\install\desktop-runtime-release\bin\sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-d3d12-shaders --video-driver windows --require-d3d12-renderer
out\install\desktop-runtime-release\bin\sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-sprite-animation
out\install\desktop-runtime-release\bin\sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-tilemap-runtime-ux
out\install\desktop-runtime-release\bin\sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-d3d12-shaders --video-driver windows --require-d3d12-renderer --require-native-2d-sprites --require-sprite-animation --require-tilemap-runtime-ux
pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -RequireVulkanShaders -SmokeArgs @('--smoke','--require-config','runtime/sample_2d_desktop_runtime_package.config','--require-scene-package','runtime/sample_2d_desktop_runtime_package.geindex','--require-vulkan-shaders','--video-driver','windows','--require-vulkan-renderer')"
```

This sample intentionally does not claim production atlas packing, full tilemap editor UX, runtime image decoding, package streaming, 3D readiness, editor productization, Metal readiness, or broad renderer quality. public native or RHI handle access remains unsupported, broad production sprite batching readiness remains unsupported, and general production renderer quality remains unsupported.


