---
name: gameengine-game-development
description: Scaffolds and maintains C++ games, game.agent.json, and desktop or mobile validation lanes. Use when editing games/, new-game scripts, or game manifests.
paths:
  - "games/**"
  - "tools/new-game.ps1"
  - "tools/new-game-helpers.ps1"
  - "tools/new-game-templates.ps1"
  - "docs/ai-game-development.md"
---

# GameEngine Game Development

## Scope

Use this skill for C++ games, `game.agent.json`, new-game scaffolding, desktop runtime packages, generated-game contracts, and mobile validation lanes.

## Load Strategy

- Start with targeted file reads, targeted manifest fragments, and `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard`; use `Full` only when the current game/API decision needs complete manifest-shaped output.
- Do not load `references/full-guidance.md` by default. Load only the sections needed for exact API names, package counters, retained ids, runtime package lanes, generated-game manifests, or backend/editor details.
- Keep slices small, clean-break, and evidence-backed. Do not add compatibility shims, stale aliases, broad ready claims, native handles, removed SDL3, Dear ImGui, renderer/RHI/backends, or middleware public contracts from game code.

## Required Workflow

- Use only public `mirakana::` headers in game code unless the task explicitly changes engine internals.
- Keep `game_name` and `new-game -Name` matching `^[a-z][a-z0-9_]*$`; source/runtime package path segments stay lowercase snake_case.
- Use `tools/new-game.ps1`, `tools/new-game-helpers.ps1`, and `tools/new-game-templates.ps1`; update the helper/template that owns the generated surface.
- For generated-game scaffolds, use `tools/create-game-recipe.ps1 -Mode DryRun|-Mode Apply -GameName <game_name> -DesignSpecPath <path>`. It reports `plannedFiles`/`changedFiles`, preserves `aiWorkflow.gameDesignSpec`, supports `DesktopRuntime2DPackage` and `DesktopRuntime3DPackage`, and does not execute arbitrary shell text.
- Runtime package payloads are byte-hashed. When adding a text cooked/runtime extension or `runtimePackageFiles` entry, update `runtime/.gitattributes` with `text eol=lf`, keep scaffold/static checks aligned, and run the narrowest package smoke.
- Use focused package/game validation first, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the coherent slice gate.

## Contract Index

Keep detailed implementation guidance in `references/full-guidance.md`; this SKILL.md keeps only the retained routing terms that make static guards and agents find the right contract family.

- Generated-game manifests: `game.agent.json.aiWorkflow.gameDesignSpec`, validation recipe ids, same-manifest package targets.
- Generated-game mutation and assets: `game.agent.json.aiWorkflow.contentMutationLedger`, `register-runtime-package-files.ps1`, `games/CMakeLists.txt`, `game.agent.json.aiWorkflow.placeholderAssetPipeline`, first-party placeholder assets, no external asset downloads.
- Generated-game validation: `game.agent.json.aiWorkflow.generatedGamePlaytestLoop`, failure classification, no validation weakening, `game.agent.json.aiWorkflow.validationRemediationRecipes`, rerun selected validation recipes, `aiWorkflow.generatedGameQualityRubric`, subjective fun, broad production readiness.
- Generated-game tooling: `plan_placeholder_asset_bundle`, `PlaceholderAssetBundleRequest`, `PlaceholderAssetBundlePlan`, `plan_placeholder_asset_cook_package`, `PlaceholderAssetCookPackageRequest`, `plan_sprite_atlas_source_authoring`, `SpriteAtlasSourceAuthoringDesc`, `GameEngine.SourceAssetRegistry.v1`, `SpriteAtlasSourceAuthoringDesc`.
- Generated Game Studio v1: `Generated Game Studio v1`, `EditorAiGeneratedGameStudioV1Model`, `generated_game_studio`, no engine-internal edits, no renderer/RHI residency.
- Package and streaming smokes: quaternion package smoke is limited to cooked data; `sample_animation_local_pose_3d`; `--require-package-streaming-safe-point`, `execute_selected_runtime_package_streaming_safe_point`, `package_streaming_status`, broad async/background streaming; `--require-playable-3d-slice`, `playable_3d_status=ready`, broad generated 3D production readiness.
- Renderer-quality package evidence: `generated color-postprocess scaffold`, `postprocess_depth_input_ready`, `postprocess_depth_input_ready=1`, `renderer_quality_postprocess_depth_input_ready=1`.
- Renderer-quality smokes: `--require-renderer-quality-gates`, `evaluate_win32_desktop_presentation_quality_gate`, `renderer_quality_expected_framegraph_passes=2`, `renderer_quality_expected_framegraph_barrier_steps=4`, `renderer_quality_expected_framegraph_barrier_steps=9`, `framegraph_barrier_steps_executed`, depth-input postprocess.
- Renderer-quality render-pass counters: `framegraph_render_passes_recorded`, `renderer_quality_expected_framegraph_render_passes`, `renderer_quality_framegraph_render_passes`.
- Renderer matrix and package flags: `Renderer General Quality Matrix v1`, `--require-renderer-quality-matrix`, `renderer_quality_matrix_dependency_gated_rows=0`, `renderer_quality_matrix_unsupported_rows=0`, `renderer_quality_matrix_general_renderer_quality_ready=0`, `--require-shadow-morph-composition`, `--require-native-ui-overlay`, `--require-visible-3d-production-proof`, `--require-native-ui-text-glyph-atlas`.
- Directional shadow and light-space package evidence: `--require-directional-shadow`, `--require-directional-shadow-filtering`, `directional_shadow_ready`, `directional_shadow_filter_mode`, `Stable Directional Light-Space Policy v0`, `DirectionalShadowLightSpacePlan`.
- Compute morph/RHI skin ids: `make_runtime_compute_morph_skinned_mesh_gpu_binding`, `Runtime Scene RHI Compute Morph Skin Palette D3D12 v1`, `Generated 3D Compute Morph Skin Package Smoke D3D12 v1`, `Generated 3D Compute Morph Skin Package Smoke Vulkan v1`, `SceneSkinnedGpuBindingPalette`, `DesktopRuntime3DPackage`.
- Compute morph/RHI async ids: `Runtime RHI Compute Morph Async Telemetry D3D12 v1`, `Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1`, `Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1`, `Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1`, `Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1`, `scene_gpu_compute_morph_async_*`.
- Compute morph/RHI Vulkan ids: `Runtime RHI Compute Morph Vulkan Proof v1`, `Runtime RHI Compute Morph Renderer Consumption Vulkan v1`, `Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1`, `Generated 3D Compute Morph Package Smoke Vulkan v1`.
- Compute morph/RHI D3D12 timing ids: `RHI D3D12 Per-Queue Fence Synchronization v1`, `RHI D3D12 Queue Timestamp Measurement Foundation v1`, `RHI D3D12 Queue Clock Calibration Foundation v1`, `RHI D3D12 Calibrated Queue Timing Diagnostics v1`, `Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1`, `RHI D3D12 Submitted Command Calibrated Timing Scopes v1`, `Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1`.
- Runtime UI and input: `MonospaceTextLayoutPolicy`, `UiRendererGlyphAtlasPalette`, `UiRendererImagePalette`, `AccessibilityPublishPlan`, `ImeCompositionPublishPlan`, `PlatformTextInputSessionPlan`, `FontRasterizationRequestPlan`, `ImageDecodeRequestPlan`, `plan_image_decode_request`, `PngImageDecodingAdapter`, `decode_audited_png_rgba8`, glyph atlas generation, image decoding; debug overlay through `RuntimeGameplayDebugOverlayRowDesc`, `RuntimeGameplayDebugOverlayPlan`, and `plan_runtime_gameplay_debug_overlay`.
- Runtime input: `RuntimeInputStateView`, `RuntimeInputContextStack`, `bind_gamepad_button`, `GameEngine.RuntimeInputActions.v4`, `bind_gamepad_axis`, `bind_key_in_context`, `bind_pointer_in_context`, `bind_gamepad_button_in_context`, `bind_key_axis_in_context`, `axis_value`.
- Runtime input policy: UI focus integration, global input consumption, radial stick deadzones, per-device profiles, interactive runtime/game rebinding.
- Runtime input rebinding: `GameEngine.RuntimeInputRebindingProfile.v1`, `RuntimeInputRebindingProfile`, `apply_runtime_input_rebinding_profile`, `RuntimeInputRebindingCaptureRequest`, `capture_runtime_input_rebinding_action`, `RuntimeInputRebindingFocusCaptureRequest`, `RuntimeInputRebindingFocusCaptureResult`, `capture_runtime_input_rebinding_action_with_focus`, `RuntimeInputRebindingAxisCaptureRequest`, `capture_runtime_input_rebinding_axis`, `gameplay_input_consumed`, `RuntimeInputRebindingPresentationModel`, `make_runtime_input_rebinding_presentation`, symbolic glyph lookup keys.
- Audio: `AudioDeviceStreamRequest`, `AudioDeviceStreamPlan`, `plan_audio_device_stream`, `render_audio_device_stream_interleaved_float`, `AudioGameplayMixRequest`, `AudioGameplayMixPlan`, `plan_gameplay_audio_mix`, device hotplug/selection, mixer authoring.
- Navigation grid/navmesh: `validate_navigation_grid_path`, `replan_navigation_grid_path`, `calculate_navigation_local_avoidance`, `smooth_navigation_grid_path`, `NavigationGridAgentPathRequest`, `NavigationGridAgentPathPlan`, `plan_navigation_grid_agent_path`, `NavigationNavmeshPathRequest`, `plan_navigation_navmesh_path`, `NavigationCrowdPlanRequest`, `plan_navigation_navmesh_crowd`, navmesh, crowd, scene/physics, editor.
- Navigation world/streaming: `NavigationHierarchicalWorldPathRequest`, `plan_navigation_hierarchical_world_path`, `RuntimeWorldRegionNavigationRefReviewRequest`, `review_runtime_world_region_navigation_refs`, `RuntimeWorldRegionNavigationPathCacheReviewRequest`, `review_runtime_world_region_navigation_path_cache`, nav-data, native handles.
- Large scene readiness: `RuntimeWorldStreamingLargeSceneReadinessRequest`, `world_region_streaming_large_scene_readiness_status`, background streaming.
- Procedural and reusable gameplay: `plan_runtime_procedural_generation`, `gameplay_systems_procedural_generation_rows`, content quality; `plan_tile_chunk_renderer`, `--require-production-tile-renderer`, native texture ownership.
- Quest/dialogue: `RuntimeQuestDialogueDocument`, `validate_runtime_quest_dialogue_document`, `RuntimeQuestDialogueState`, `validate_runtime_quest_dialogue_state`, `advance_runtime_quest_dialogue_state`, game-owned.
- Item/crafting: `RuntimeItemCatalogDocument`, `RuntimeItemCatalogValidationContext`, `RuntimeItemCatalogValidationResult`, `validate_runtime_item_catalog_document`, `RuntimeInventoryState`, `validate_runtime_inventory_state`, `RuntimeCraftingRecipeDocument`, `RuntimeInventoryTransitionRequest`, `advance_runtime_inventory_state`.
- Construction placement: `RuntimeConstructionPlacementValidationContext`, `validate_runtime_construction_placement`, `RuntimeSceneConstructionPlacementIntentDesc`, `plan_runtime_scene_construction_placement_intents`, candidate row grid/world origins, same-batch occupied-cell, game-owned.
- Physics collision and character: `PhysicsCollisionQueryBatchStatus`, `PhysicsCollisionQueryBatchDiagnostic`, `PhysicsCollisionQueryRowStatus`, `PhysicsCollisionQueryRowDiagnostic`, `PhysicsWorld2D::raycast_batch`, `PhysicsWorld3D::shape_sweep_batch`, default-unbounded, `collision_query_batch_ready`, `move_physics_character_controller_3d`, `build_physics_world_3d_from_authored_collision_scene`.
- Physics 3D queries and policy: `PhysicsWorld3D::exact_shape_sweep`, `PhysicsShape3DDesc::aabb`, `PhysicsShape3DDesc::sphere`, `PhysicsShape3DDesc::capsule`, `PhysicsQueryFilter3D`, `PhysicsWorld3D::exact_sphere_cast`, `PhysicsWorld3D::contact_manifolds`, `PhysicsWorld3D::step_continuous`, `PhysicsCharacterDynamicPolicy3DDesc`, `evaluate_physics_character_dynamic_policy_3d`.
- Physics advanced movement: `PhysicsAdvancedController3DDesc`, `plan_physics_advanced_controller_3d`, `PhysicsJointSolve3DResult`, `solve_physics_joints_3d`, `PhysicsConstraintSolve3DResult`, `solve_physics_constraints_3d`, `max_rows`, `row_budget_exceeded`, rotational rigid-body constraints, `PhysicsKinematicMotion3DResult`, `plan_physics_kinematic_motion_3d`, `PhysicsSimpleVehicle3DDesc`, `plan_physics_simple_vehicle_3d`, simple vehicle, `PhysicsReplaySignature3D`, `evaluate_physics_determinism_gate_3d`, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, Jolt.
- Physics package counters: `gameplay_systems_advanced_controller_status=moved`, `gameplay_systems_advanced_controller_platform_applied=1`, `gameplay_systems_advanced_controller_constraint_rows=1`, `gameplay_systems_advanced_controller_replay_changed=1`, `gameplay_systems_physics_constraints_status=solved`, `gameplay_systems_physics_constraints_diagnostic=none`, `gameplay_systems_physics_constraints_rows=2`.
- Physics package continuation: `gameplay_systems_physics_constraints_fixed_rows=1`, `gameplay_systems_physics_constraints_linear_axis_rows=1`, `gameplay_systems_physics_constraints_axis_limit_clamped=1`, `gameplay_systems_kinematic_motion_status=constrained`, `gameplay_systems_kinematic_motion_rows=2`, `gameplay_systems_vehicle_status=grounded`, `gameplay_systems_vehicle_diagnostic=none`, `gameplay_systems_vehicle_wheel_rows=4`, `gameplay_systems_vehicle_grounded_wheels=4`, `gameplay_systems_vehicle_wheel_probe_hits=4`.
- AI: `BehaviorTreeBlackboard`, `BehaviorTreeEvaluationContext`, `AiPerceptionAgent2D`, `AiPerceptionTarget2D`, `build_ai_perception_snapshot_2d`, `write_ai_perception_blackboard`, `BehaviorAuthoringDocument`, `BehaviorAuthoringValidationContext`, `validate_behavior_authoring_document`, blackboard.
- Animation: `Animation CPU Skinning`, `skin_animation_vertices_cpu`, `solve_animation_two_bone_ik_3d_orientation`, `solve_animation_fabrik_ik_3d_chain`, `AnimationSkeleton3dDesc`, `build_animation_model_pose_3d`, `apply_animation_fabrik_ik_3d_solution_to_pose`, `sample_quat_keyframes`, `AnimationJointTrack3dDesc`, `sample_animation_local_pose_3d`, `import_gltf_node_transform_animation_tracks_3d`, `AnimationIkLocalRotationLimit3d`, `apply_animation_local_rotation_limits_3d`.

## Detailed Reference

- `references/full-guidance.md`: detailed procedures, API inventory, retained row ids, package/backend/editor lanes, and validation evidence. Load only the sections needed for the current task.
