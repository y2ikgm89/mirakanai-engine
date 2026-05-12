# sample-generated-desktop-runtime-3d-package

## Goal

Describe the packaged 3D game goal here before expanding gameplay.

## Current Runtime

This `DesktopRuntime3DPackage` game uses the optional desktop runtime package lane with a first-party cooked 3D scene package:

- `mirakana::GameApp`
- `mirakana::SdlDesktopGameHost`
- `mirakana::runtime::load_runtime_asset_package`
- `mirakana::instantiate_runtime_scene_render_data`
- `mirakana::sample_and_apply_runtime_scene_render_animation_float_clip`
- `mirakana::sample_runtime_morph_mesh_cpu_animation_float_clip`
- `mirakana::runtime::runtime_animation_quaternion_clip_payload`
- `mirakana::sample_animation_local_pose_3d`
- deterministic public gameplay systems composition through `mirakana_physics`, `mirakana_navigation`, `mirakana_ai`, `mirakana_audio`, and `mirakana_animation`
- `mirakana::submit_scene_render_packet`
- primary camera/controller movement over cooked scene data
- cooked scalar transform animation applied to the packaged mesh through first-party binding rows
- cooked morph mesh CPU payload consumed with scalar morph-weight animation smoke counters
- cooked quaternion local-pose animation consumed through first-party `mirakana_animation` sampling counters
- selected host-gated package streaming safe-point smoke over the packaged scene validation target
- selected generated 3D renderer quality smoke over scene GPU, depth-aware postprocess, and framegraph=2 counters
- selected generated 3D postprocess depth-input smoke with `postprocess_depth_input_ready=1` and `renderer_quality_postprocess_depth_input_ready=1`
- selected generated 3D directional shadow package smoke with `directional_shadow_ready=1` and fixed PCF 3x3 filtering counters
- selected D3D12 generated 3D graphics morph + directional shadow receiver package smoke with `--require-shadow-morph-composition`, `renderer_gpu_morph_draws`, `renderer_morph_descriptor_binds`, `directional_shadow_ready=1`, and `framegraph_passes=3`
- selected generated 3D gameplay systems package smoke with `gameplay_systems_status=ready`, `gameplay_systems_ready=1`, physics authored collision/controller counters, navigation, AI perception/behavior, audio stream, and animation/lifecycle counters
- selected generated 3D scene collision package smoke with `--require-scene-collision-package`, `collision_package_status=ready`, `collision_package_bodies=3`, `collision_package_trigger_overlaps=1`, and `gameplay_systems_collision_package_ready=1`
- selected D3D12 visible generated 3D production-style package proof with `--require-visible-3d-production-proof`, `visible_3d_status=ready`, `visible_3d_presented_frames=2`, D3D12 selection, scene GPU, postprocess, renderer quality, playable aggregate, and native UI overlay readiness counters
- selected D3D12 generated 3D native UI overlay HUD box package smoke with `--require-native-ui-overlay`, `hud_boxes=2`, `ui_overlay_ready=1`, `ui_overlay_sprites_submitted=2`, and `ui_overlay_draws=2`
- selected D3D12 generated 3D cooked UI atlas image sprite package smoke with `--require-native-ui-textured-sprite-atlas`, `hud_images=2`, `ui_atlas_metadata_status=ready`, `ui_texture_overlay_atlas_ready=1`, `ui_texture_overlay_sprites_submitted=2`, `ui_texture_overlay_texture_binds=2`, and `ui_texture_overlay_draws=2`
- selected D3D12 generated 3D cooked UI atlas text glyph package smoke with `--require-native-ui-text-glyph-atlas`, `hud_text_glyphs=2`, `text_glyphs_resolved=2`, `text_glyphs_missing=0`, `ui_atlas_metadata_glyphs=1`, `ui_texture_overlay_sprites_submitted=2`, `ui_texture_overlay_texture_binds=2`, and `ui_texture_overlay_draws=2`
- static mesh, material, animation, morph, quaternion animation, and directional light package payloads
- host-built D3D12 scene/postprocess/shadow/native UI overlay shader artifacts when the selected package target is validated
- Vulkan SPIR-V artifacts only when DXC SPIR-V CodeGen and `spirv-val` are available and requested
- deterministic `NullRenderer` fallback
- `game.agent.json.runtimePackageFiles`
- `game.agent.json.runtimeSceneValidationTargets`
- `game.agent.json.packageStreamingResidencyTargets` as host-gated safe-point package streaming intent
- `PACKAGE_FILES_FROM_MANIFEST`

The generated package proves cooked texture/mesh/skinned-mesh/material/animation/morph/quaternion-animation/scene/physics-collision loading, runtime scene validation target descriptors, camera/controller package smoke validation, transform animation binding smoke validation, morph package consumption smoke validation, quaternion local-pose sampling smoke validation, selected host-gated safe-point package streaming counters through `--require-package-streaming-safe-point`, selected generated gameplay systems counters through `--require-gameplay-systems`, selected package collision counters through `--require-scene-collision-package`, generated 3D renderer quality counters through `--require-renderer-quality-gates` for scene GPU + depth-aware postprocess + framegraph=2 only, generated 3D postprocess depth-input counters through `--require-postprocess-depth-input`, selected generated 3D directional shadow counters through `--require-directional-shadow --require-directional-shadow-filtering`, selected D3D12 generated 3D graphics morph + directional shadow receiver counters through `--require-shadow-morph-composition`, selected D3D12 visible generated 3D production-style package counters through `--require-visible-3d-production-proof`, selected D3D12 generated 3D native UI overlay HUD box counters through `--require-native-ui-overlay`, selected D3D12 generated 3D cooked UI atlas image sprite counters through `--require-native-ui-textured-sprite-atlas`, selected D3D12 generated 3D cooked UI atlas text glyph counters through `--require-native-ui-text-glyph-atlas`, selected generated 3D playable package counters through `--require-playable-3d-slice`, D3D12 compute morph dispatch into renderer-consumed POSITION/NORMAL/TANGENT buffers, generated D3D12 skin+compute package smoke counters, Vulkan POSITION/NORMAL/TANGENT compute morph package smoke through explicit SPIR-V artifacts, Vulkan skin+compute package smoke counters through explicit SPIR-V artifacts, and selected-target shader artifact metadata. It does not claim runtime source parsing, broad dependency cooking, broad async/background package streaming, scene/physics perception integration, navmesh/crowd, middleware, production physics middleware/native backend readiness, CCD, joints, production text shaping, font rasterization, glyph atlas generation, runtime source image decoding, source image atlas packing, authored animation graph workflows, broad skeletal renderer deformation, broad directional shadow production quality, morph-deformed shadow-caster silhouettes, compute morph + shadow composition, Vulkan/Metal parity for the visible proof, Metal compute morph deformation, async compute overlap/performance, broad frame graph scheduling, graphics morph+skin composition beyond the host-owned skin+compute package smoke, material/shader graphs, live shader generation, editor productization, native/RHI handle exposure, Metal readiness, general renderer quality, or broad generated 3D production readiness.

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package
```

The installed D3D12 package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice
```

The selected D3D12 directional shadow package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-directional-shadow --require-directional-shadow-filtering --require-renderer-quality-gates
```

The selected D3D12 graphics morph + directional shadow receiver package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-shadow-morph-composition
```

The selected D3D12 native UI overlay HUD box package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay
```

The selected D3D12 visible generated 3D production-style package proof uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay --require-visible-3d-production-proof
```

The selected scene collision package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-scene-collision-package
```

The selected D3D12 native UI textured sprite atlas package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay --require-native-ui-textured-sprite-atlas
```

The selected D3D12 native UI text glyph atlas package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay --require-native-ui-text-glyph-atlas
```

The Vulkan package lane is toolchain-gated:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/sample_generated_desktop_runtime_3d_package.config', '--require-scene-package', 'runtime/sample_generated_desktop_runtime_3d_package.geindex', '--require-primary-camera-controller', '--require-transform-animation', '--require-morph-package', '--require-compute-morph', '--require-compute-morph-skin', '--require-quaternion-animation', '--require-package-streaming-safe-point', '--require-gameplay-systems', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-renderer-quality-gates', '--require-playable-3d-slice')
```

The selected Vulkan directional shadow package lane is toolchain-gated:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/sample_generated_desktop_runtime_3d_package.config', '--require-scene-package', 'runtime/sample_generated_desktop_runtime_3d_package.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-directional-shadow', '--require-directional-shadow-filtering', '--require-renderer-quality-gates')
```


