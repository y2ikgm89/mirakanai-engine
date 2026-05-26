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
- selected D3D12 package upload staging smoke with `--require-package-upload-staging`, `package_upload_staging_ready=1`, `package_upload_staging_package_transactions=4`, `package_upload_staging_ring_backed_uploads=4`, `package_upload_staging_graphics_waited_for_copy=1`, and `package_upload_staging_resource_updates_ready=1`
- selected generated 3D renderer quality smoke over scene GPU, depth-aware postprocess, framegraph_passes=2, framegraph_passes_executed=4, framegraph_render_passes_recorded=4, framegraph_barrier_steps_executed=9, renderer_quality_expected_framegraph_render_passes=4, and renderer_quality_expected_framegraph_barrier_steps=9
- selected generated 3D postprocess depth-input smoke with `postprocess_depth_input_ready=1` and `renderer_quality_postprocess_depth_input_ready=1`
- selected generated 3D directional shadow package smoke with `directional_shadow_ready=1`, fixed PCF 3x3 filtering counters, `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_render_passes_recorded=6`, and `framegraph_barrier_steps_executed=15`
- selected D3D12 generated 3D graphics morph + directional shadow receiver package smoke with `--require-shadow-morph-composition`, `renderer_gpu_morph_draws`, `renderer_morph_descriptor_binds`, `directional_shadow_ready=1`, `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_render_passes_recorded=6`, and `framegraph_barrier_steps_executed=15`
- selected generated 3D gameplay systems package smoke with `gameplay_systems_status=ready`, `gameplay_systems_ready=1`, physics authored collision/controller counters, navmesh dynamic-obstacle counters, deterministic navmesh/crowd counters, local avoidance counters, character/dynamic physics policy counters, navigation, AI perception/behavior, audio stream, `mirakana::runtime::plan_runtime_gameplay_interactions` interaction counters including `gameplay_systems_interaction_ready`, `gameplay_systems_interaction_rows`, `gameplay_systems_interaction_feedback_rows`, `gameplay_systems_interaction_final_session_state`, `mirakana::runtime::plan_runtime_session_profile_resume` counters including `runtime_profile_resume_ready`, `runtime_profile_resume_loaded_documents`, and schema versions, `mirakana::ui::plan_runtime_menu_hud` counters including `runtime_menu_hud_ready`, `runtime_menu_hud_display_rows`, `runtime_menu_hud_command_rows`, `runtime_menu_hud_dialogue_rows`, and `runtime_menu_hud_input_binding_prompt_rows`, and animation/lifecycle counters
- selected generated 3D RPG systems package smoke with `mirakana::runtime::plan_runtime_rpg_systems` under `--require-gameplay-systems`, requiring `rpg_systems_status=ready`, `rpg_systems_ready=1`, two party members, one enemy member, eight stat rows, two progression rows, two skill rows with one blocked row, two equipment rows with one blocked row, six combat turn rows across two rounds, two reward rows, two save-validation rows with one repairable row, a positive `rpg_systems_replay_hash`, zero combat/reward/save side-effect counters, and `rpg_systems_diagnostics=0`
- selected generated 3D sandbox world package smoke with `mirakana::runtime::plan_runtime_sandbox_world_mutation` under `--require-gameplay-systems`, requiring `sandbox_world_status=ready`, `sandbox_world_ready=1`, two chunks, two resident chunks, two existing cells, three placement intents with one accepted row, two destruction intents with one accepted row, two construction cost rows, five mutation rows, two persistence rows with one repairable row, three rejected unsafe mutation rows, a positive `sandbox_world_replay_hash`, zero world/persistence/package side-effect counters, and `sandbox_world_diagnostics=0`
- selected generated 3D simulation management package smoke with `mirakana::runtime::plan_runtime_simulation_management` under `--require-gameplay-systems`, requiring `simulation_management_status=ready`, `simulation_management_ready=1`, `simulation_management_tick_count=240`, four resource balance rows, two job rows with one assigned row, two logistics links with one scheduled transfer, one economy summary, two population need rows with one deficit, two schedules, two save-review rows with one repairable row, seven dashboard rows, a positive `simulation_management_replay_hash`, zero economy/save/runtime-UI/package side-effect counters, and `simulation_management_diagnostics=0`
- selected generated 3D network replication package smoke with `mirakana::runtime::plan_runtime_network_replication` under `--require-gameplay-systems`, requiring `network_replication_status=host_evidence_required`, `network_replication_reviewed=1`, `network_replication_ready=0`, two object rows, two input rows, two snapshot rows, one rollback row, a positive `network_replication_replay_hash`, `network_replication_requires_transport_host_evidence=1`, `network_replication_transport_host_evidence=0`, zero network/rollback/world side-effect counters, and `network_replication_diagnostics=0`
- selected generated 3D rendering VFX profiling package smoke with `mirakana::plan_renderer_production_vfx_profiling` under `--require-rendering-vfx-profiling`, requiring `rendering_vfx_profiling_status=host_evidence_required`, `rendering_vfx_profiling_reviewed=1`, `rendering_vfx_profiling_ready=0`, three rows each for feature, GPU particle budget, postprocess, backend timing, backend evidence, and crash telemetry handoff, D3D12 and strict Vulkan host evidence ready, Metal host evidence absent, zero GPU command/native capture/crash upload side-effect counters, a positive `rendering_vfx_profiling_replay_hash`, and `rendering_vfx_profiling_diagnostics=0`
- selected generated 3D audio gameplay mixer package smoke with `--require-audio-gameplay-mixer`, a cooked package audio payload, `mirakana::plan_gameplay_audio_mix`, and deterministic `audio_gameplay_mixer_*` counters for bus, cue, trigger, command, pause, fade, loop, spatial, render, and payload diagnostics without native device handles or middleware contracts
- selected generated 3D scene collision package smoke with `--require-scene-collision-package`, `collision_package_status=ready`, `collision_package_bodies=3`, `collision_package_trigger_overlaps=1`, and `gameplay_systems_collision_package_ready=1`
- selected generated 3D entity scale/culling package smoke with `--require-entity-scale-culling`, `mirakana::runtime::plan_runtime_entity_scale_culling`, and `entity_scale_culling_*` counters including `entity_scale_culling_status=planned`, `entity_scale_culling_rows=4`, `entity_scale_culling_visible_rows=2`, `entity_scale_culling_culled_rows=2`, `entity_scale_culling_lod_rows=2`, `entity_scale_culling_budget_protected_rows=1`, `entity_scale_culling_diagnostics=0`, and `entity_scale_culling_budget_diagnostics=2`
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

The generated package proves cooked texture/mesh/skinned-mesh/material/animation/morph/quaternion-animation/scene/physics-collision loading, runtime scene validation target descriptors, camera/controller package smoke validation, transform animation binding smoke validation, morph package consumption smoke validation, quaternion local-pose sampling smoke validation, selected host-gated safe-point package streaming counters through `--require-package-streaming-safe-point`, selected D3D12 package upload staging counters through `--require-package-upload-staging`, selected generated gameplay systems counters through `--require-gameplay-systems`, including `gameplay_systems_navigation_navmesh_dynamic_obstacles=1`, `gameplay_systems_navigation_crowd_source_order_ready=1`, `gameplay_systems_navigation_crowd_applied_neighbors=2`, `gameplay_systems_local_avoidance_applied_neighbors`, and `gameplay_systems_physics_policy_dynamic_pushes=1`, `gameplay_systems_advanced_controller_status=moved`, `gameplay_systems_advanced_controller_platform_applied=1`, `gameplay_systems_advanced_controller_constraint_rows=1`, `gameplay_systems_advanced_controller_replay_changed=1`, `gameplay_systems_physics_constraints_status=solved`, `gameplay_systems_physics_constraints_diagnostic=none`, `gameplay_systems_physics_constraints_rows=2`, `gameplay_systems_physics_constraints_fixed_rows=1`, `gameplay_systems_physics_constraints_linear_axis_rows=1`, and `gameplay_systems_physics_constraints_axis_limit_clamped=1`, selected package collision counters through `--require-scene-collision-package`, selected generated 3D rendering VFX profiling counters through `--require-rendering-vfx-profiling` with `rendering_vfx_profiling_reviewed=1`, `rendering_vfx_profiling_ready=0`, D3D12 and strict Vulkan host evidence ready, Metal host evidence absent, zero side-effect counters, and positive replay hash, generated 3D renderer quality counters through `--require-renderer-quality-gates` for scene GPU + depth-aware postprocess with framegraph_passes=2, framegraph_passes_executed=4, framegraph_render_passes_recorded=4, framegraph_barrier_steps_executed=9, renderer_quality_expected_framegraph_render_passes=4, and renderer_quality_expected_framegraph_barrier_steps=9 only, generated 3D postprocess depth-input counters through `--require-postprocess-depth-input`, selected generated 3D directional shadow counters through `--require-directional-shadow --require-directional-shadow-filtering`, selected D3D12 generated 3D graphics morph + directional shadow receiver counters through `--require-shadow-morph-composition` with framegraph_render_passes_recorded=6, selected D3D12 visible generated 3D production-style package counters through `--require-visible-3d-production-proof`, selected D3D12 generated 3D native UI overlay HUD box counters through `--require-native-ui-overlay`, selected D3D12 generated 3D cooked UI atlas image sprite counters through `--require-native-ui-textured-sprite-atlas`, selected D3D12 generated 3D cooked UI atlas text glyph counters through `--require-native-ui-text-glyph-atlas`, selected generated 3D playable package counters through `--require-playable-3d-slice`, D3D12 compute morph dispatch into renderer-consumed POSITION/NORMAL/TANGENT buffers, generated D3D12 skin+compute package smoke counters, Vulkan POSITION/NORMAL/TANGENT compute morph package smoke through explicit SPIR-V artifacts, Vulkan skin+compute package smoke counters through explicit SPIR-V artifacts, and selected-target shader artifact metadata. It does not claim runtime source parsing, broad dependency cooking, broad async/background package streaming, scene/physics perception integration, navmesh asset import, persistent/full crowd simulation beyond value-only batch planning, middleware, production physics middleware/native backend readiness, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, persistent joint assets, production text shaping, font rasterization, glyph atlas generation, runtime source image decoding, source image atlas packing, authored animation graph workflows, broad skeletal renderer deformation, broad directional shadow production quality, morph-deformed shadow-caster silhouettes, compute morph + shadow composition, Vulkan/Metal parity for the visible proof, Metal compute morph deformation, async compute overlap/performance, broad frame graph scheduling, graphics morph+skin composition beyond the host-owned skin+compute package smoke, material/shader graphs, live shader generation, editor productization, native/RHI handle exposure, Metal readiness, general renderer quality, or broad generated 3D production readiness.

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package
```

The installed D3D12 package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-rendering-vfx-profiling --require-playable-3d-slice --require-native-ui-overlay --require-visible-3d-production-proof --require-scene-collision-package --require-native-ui-textured-sprite-atlas --require-package-upload-staging
```

The selected runtime menu HUD package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-runtime-menu-hud
```

The selected audio gameplay mixer package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-audio-gameplay-mixer
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
