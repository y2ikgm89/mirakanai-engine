# Production Completion v1 - Historical Verdict Archive

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter only for historical audit or static-check maintenance; it is not the active execution index and it must not override the composed manifest, current capabilities, roadmap, or plan registry.

## Status

Historical retained-evidence ledger. This file replaces the former embedded verdict snapshot and raw PowerShell fragments with a compact list of literals that current static checks still need to find while the real implementation evidence remains in the referenced plan files, docs, source, tests, and Git history.

## Maintenance Rules

- Do not add active plans, current verdict prose, raw validator source, or long changelog tables here.
- If a static check no longer needs a literal below, delete the literal instead of expanding the archive.
- If current guidance changes, update the owning live surface first; this archive is evidence only.
- The production-completion split corpus is exactly `01-one-dot-zero-readiness-ledger.md`, `04-developer-owned-engine-capability-backlog.md`, `05-projections-and-scenarios.md`, and this archive.

## Retained Evidence Summary

- Generated 3D Visible Production Game Proof v1 retained evidence: `visible_3d_status=ready`, `visible_3d_ready=1`, `visible_3d_d3d12_selected=1`, `visible_3d_scene_gpu_ready=1`, `visible_3d_postprocess_ready=1`, `visible_3d_renderer_quality_ready=1`, `visible_3d_playable_ready=1`, and `visible_3d_ui_overlay_ready=1`; broad generated 3D production readiness, source import execution, broad package streaming, Vulkan/Metal parity, public native/RHI handles, and general renderer quality stay outside this retained evidence.
- Runtime package, renderer, editor, UI, input, profiler, physics, and generated-game historical closeouts remain discoverable through their dated plan files and Git history. This archive keeps only short literals for validators that span the split master-plan corpus.

## Line-Based Retained Notes

Runtime UI and input ledger note: RuntimeInputRebindingPresentationModel; platform input glyph generation.
workspace `ai_commands` panel state

## Static-Check Retained Literals

- `**Status:** Completed`
- `++result.queue_waits`
- `--match-head-commit`
- `--match-head-commit <headRefOid>`
- `--require-d3d12-postprocess-evidence`
- `--require-directional-shadow`
- `--require-directional-shadow-filtering`
- `--require-entity-scale-culling`
- `--require-framegraph-multiqueue-evidence`
- `--require-gameplay-systems`
- `--require-native-ui-overlay`
- `--require-native-ui-text-glyph-atlas`
- `--require-native-ui-textured-sprite-atlas`
- `--require-package-streaming-safe-point`
- `--require-playable-3d-slice`
- `--require-postprocess-depth-input`
- `--require-renderer-quality-gates`
- `--require-scene-collision-package`
- `--require-shadow-morph-composition`
- `--require-visible-3d-production-proof`
- `--require-vulkan-framegraph-multiqueue-evidence`
- `--require-vulkan-postprocess-evidence`
- `--warnings-as-errors=*`
- `-DMK_SAMPLE_SKINNED_SCENE_SHADOW_RECEIVER_PS=1`
- `-Jobs 0`
- `.claude/settings.json`
- `.claude/settings.local.json`
- `.claude/worktrees/`
- `.codex/rules`
- `.lastbuildstate`
- `.mcp.json`
- `.worktrees/`
- `/MP2`
- `/Zf`
- `2026-05-18-upload-staging-v1-package-static-mesh-upload-binding-transaction-v1.md`
- `2026-05-18-upload-staging-v1-runtime-buffer-ring-backed-uploads-v1.md`
- `2026-05-18-upload-staging-v1-runtime-ring-backed-texture-upload-v1.md`
- `2026-05-18-upload-staging-v1-runtime-upload-queue-wait-v1.md`
- `2026-05-18-upload-staging-v1-staging-pool-lease-adoption-v1.md`
- `2d-playable-vertical-slice`
- `3d-playable-vertical-slice`
- `AGENTS.override.md`
- `AI Behavior Authoring Foundation v1`
- `AI Perception Services v1`
- `AccessibilityPublishPlan::ready`
- `AccessibilityPublishResult::succeeded`
- `Add-WindowsCapability`
- `AiPerceptionAgent2D`
- `AiPerceptionTarget2D`
- `Animation CPU Skinning`
- `AnimationIkLocalRotationLimit3d`
- `AnimationJointTrack3dDesc`
- `AnimationSkeleton3dDesc`
- `AudioDeviceStreamPlan`
- `AudioDeviceStreamRequest`
- `AudioGameplayMixRequest`
- `Behavior Tree Blackboard Conditions v0`
- `BehaviorAuthoringDocument`
- `BehaviorAuthoringValidationContext`
- `BehaviorTreeBlackboard`
- `BehaviorTreeEvaluationContext`
- `C1041`
- `CI check selection changes`
- `CMake File API`
- `CMakeCache.txt`
- `COMPILE_PDB_OUTPUT_DIRECTORY`
- `CPU QPC sample`
- `CapturingAccessibilityAdapter`
- `CapturingFontRasterizerAdapter`
- `CapturingImageDecodingAdapter`
- `CapturingImeAdapter`
- `CapturingTextShapingAdapter`
- `Codex app Worktree/Handoff`
- `Commits pushed after a PR merged need a new PR`
- `ComputePipelineHandle create_compute_pipeline`
- `Context7`
- `Crash Telemetry Trace Ops v1`
- `CreateHeap`
- `CreatePlacedResource`
- `Cursor global instructions`
- `D3D12-only helper`
- `D3D12_COMPUTE_PIPELINE_STATE_DESC`
- `D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES`
- `D3D12_QUERY_HEAP_TYPE_TIMESTAMP`
- `D3D12_RESOURCE_BARRIER_TYPE_ALIASING`
- `Debugging Tools for Windows`
- `DesktopRuntime2DPackage`
- `DesktopRuntime3DPackage`
- `DiagnosticsOpsArtifactStatus`
- `Direct default-branch pushes are forbidden`
- `DirectionalShadowLightSpacePlan`
- `Do not claim Vulkan NORMAL/TANGENT package smoke`
- `Do not claim Vulkan compute morph parity`
- `Do not claim generated-package Vulkan compute morph readiness`
- `Do not copy stale API lists`
- `Do not edit files`
- `Do not embed stale capability snapshots`
- `Do not expose native fences`
- `Do not update generated`
- `Do not update generated package validation to require overlap`
- `Do not write plan files`
- `Docs/agent/rules/subagent-only changes run formatting plus agent/static guards`
- `EditorInputRebindingProfileReviewModel`
- `FILE_SET CXX_MODULES`
- `FenceValue`
- `FontRasterizationRequestPlan::ready`
- `FontRasterizationResult::succeeded`
- `Frame Graph Automatic Aliasing Barrier Insertion v1`
- `Frame Graph RHI queue dependency and multi-queue pass-command work`
- `Frame Graph v1`
- `FrameGraphRhiMultiQueueExecutionDesc::texture_bindings`
- `FrameGraphRhiMultiQueueExecutionResult::barriers_recorded`
- `FrameGraphRhiRenderPassDesc```` envelope around the ````primary_color```` callback`
- `FrameGraphTextureAliasingBarrier`
- `FrameGraphTransientTextureLeaseBindingResult`
- `GITHUB_TOKEN`
- `GameEngine.RuntimeInputActions.v4`
- `GameEngine.Tilemap.v1`
- `GameEngine.UiAtlas.v1`
- `Generated 3D Committed Package Sample v1`
- `Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1`
- `Generated 3D Compute Morph NORMAL/TANGENT Package Smoke D3D12/Vulkan work`
- `Generated 3D Compute Morph NORMAL/TANGENT Package Smoke Vulkan v1`
- `Generated 3D Compute Morph Package Smoke Vulkan v1`
- `Generated 3D Compute Morph Queue Sync Package Smoke D3D12 v1`
- `Generated 3D Compute Morph Skin Package Smoke D3D12 v1`
- `Generated 3D Compute Morph Skin Package Smoke D3D12/Vulkan work`
- `Generated 3D Compute Morph Skin Package Smoke Vulkan v1`
- `Generated 3D Directional Shadow Package Smoke v1`
- `Generated 3D Native UI Overlay Package Smoke v1`
- `Generated 3D Native UI Text Glyph Atlas Package Smoke v1`
- `Generated 3D Playable Package Smoke v1`
- `Generated 3D Postprocess Depth Package Smoke v1`
- `Generated 3D Renderer Quality Package Smoke v1`
- `Generated 3D Shadow Morph Composition Package Smoke v1`
- `Generated 3D Visible Production-Style Package Proof v1`
- `Get-ClangFormatCommand`
- `Get-ValidationRecipeCommandPlan`
- `GetClockCalibration`
- `GetResourceAllocationInfo`
- `GetTimestampFrequency`
- `Git main worktree porcelain records`
- `Git worktree porcelain records`
- `Git/GitHub publishing workflow changes`
- `GitHub Flow`
- `HeaderFilterRegex`
- `Hosted PR failure hardening`
- `ImageDecodeDispatchResult::succeeded`
- `ImageDecodePixelFormat`
- `ImageDecodePixelFormat::rgba8_unorm`
- `ImageDecodeRequestPlan::ready`
- `ImeCompositionPublishPlan::ready`
- `ImeCompositionPublishResult::succeeded`
- `Input Rebinding Profile UX v1`
- `InvalidFontRasterizerAdapter`
- `Invoke-MobilePackagingProbe`
- `Invoke-RestMethod`
- `Invoke-ValidationRecipeCommandPlan`
- `Invoke-WebRequest`
- `JSON manifest IDs`
- `LinkIncremental=false`
- `MCP connection state`
- `MK_MSVC_CXX23_STANDARD_OPTION`
- `MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV`
- `MK_VULKAN_TEST_COMPUTE_SPV`
- `MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV`
- `MK_VULKAN_TEST_DEPTH_SAMPLE_FRAGMENT_SPV`
- `MK_VULKAN_TEST_DEPTH_SAMPLE_VERTEX_SPV`
- `MK_VULKAN_TEST_DEPTH_VERTEX_SPV`
- `MK_VULKAN_TEST_POSTPROCESS_DEPTH_FRAGMENT_SPV`
- `MK_VULKAN_TEST_POSTPROCESS_DEPTH_VERTEX_SPV`
- `MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV`
- `MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV`
- `MK_runtime_tests`
- `MK_tools_tests`
- `MK_ui_renderer_tests`
- `MSB8028`
- `Microsoft Learn: Timing`
- `NN warnings generated.`
- `NativeComputePipelineDesc`
- `NavigationCrowdPlanRequest`
- `NavigationGridAgentPathPlan`
- `NavigationGridAgentPathRequest`
- `NavigationNavmeshPathRequest`
- `NullRhiDevice::acquire_transient_texture_alias_group`
- `NullRhiDevice::wait_for_queue`
- `OpenAI developer documentation MCP`
- `OutputWaitMilliseconds`
- `PACKAGE_FILES_FROM_MANIFEST`
- `PATH/Path variants`
- `PIX on Windows`
- `PR validation cost proportional to risk`
- `Package Static Mesh Upload Binding Transaction v1`
- `Package Streaming Frame Graph Texture Binding Handoff v1`
- `PackedUiAtlasPackageUpdateDesc`
- `PackedUiGlyphAtlasPackageUpdateDesc`
- `Phase 1: Overlay Row Contract`
- `PhysicsAdvancedController3DDesc`
- `PhysicsCharacterDynamicPolicy3DDesc`
- `PhysicsCollisionQueryBatchDiagnostic`
- `PhysicsCollisionQueryBatchStatus`
- `PhysicsCollisionQueryRowDiagnostic`
- `PhysicsCollisionQueryRowStatus`
- `PhysicsJointSolve3DResult`
- `PhysicsQueryFilter3D`
- `PhysicsReplaySignature3D`
- `PhysicsShape3DDesc::aabb`
- `PhysicsShape3DDesc::capsule`
- `PhysicsShape3DDesc::sphere`
- `PhysicsWorld2D::raycast_batch`
- `PhysicsWorld3D::contact_manifolds`
- `PhysicsWorld3D::exact_shape_sweep`
- `PhysicsWorld3D::exact_sphere_cast`
- `PhysicsWorld3D::shape_sweep_batch`
- `PhysicsWorld3D::step_continuous`
- `PlaceholderAssetBundleRequest`
- `PlaceholderAssetCookPackageRequest`
- `PlatformTextInputEndPlan`
- `PlatformTextInputEndPlan::ready`
- `PlatformTextInputEndResult`
- `PlatformTextInputEndResult::succeeded`
- `PlatformTextInputSessionPlan::ready`
- `PlatformTextInputSessionResult::succeeded`
- `PngImageDecodingAdapter::decode_image`
- `Postprocess Depth Input Readback Foundation v0`
- `PowerShell parse checks`
- `Presets must normalize raw`
- `QueryPerformanceCounter`
- `QueueCalibratedOverlapDiagnostics`
- `QueueCalibratedOverlapStatus`
- `QueueCalibratedTiming`
- `QueueCalibratedTimingStatus`
- `QueueClockCalibration`
- `QueueClockCalibrationStatus`
- `QueueKind queue{QueueKind::graphics}`
- `QueueTimestampInterval`
- `QueueTimestampMeasurementStatus`
- `QueueTimestampMeasurementSupport`
- `RED tests describe overlay row planning`
- `RHI D3D12 Calibrated Queue Timing Diagnostics v1`
- `RHI D3D12 Per-Queue Fence Synchronization v1`
- `RHI D3D12 Queue Clock Calibration Foundation v1`
- `RHI D3D12 Queue Timestamp Measurement Foundation v1`
- `RHI D3D12 Submitted Command Calibrated Timing Scopes v1`
- `RHI Depth Attachment Contract v0`
- `RHI Native Async Upload Execution v1`
- `RHI Vulkan Compute Dispatch Foundation v1`
- `RWByteAddressBuffer base_positions`
- `RWByteAddressBuffer output_normals`
- `RWByteAddressBuffer output_positions`
- `RWByteAddressBuffer output_tangents`
- `Remove-Item`
- `ResolveQueryData`
- `RhiAsyncOverlapReadinessStatus`
- `RhiFrameRenderer`
- `RhiPipelinedComputeGraphicsScheduleEvidence`
- `RhiStagingBufferLease`
- `RhiUploadGpuBatchExecutionResult`
- `RhiUploadRingDesc::buffer`
- `Rules/permissions stay narrow command gates`
- `Runtime Buffer Ring-Backed Uploads v1`
- `Runtime Package Streaming RHI Upload Binding Transaction v1`
- `Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1`
- `Runtime RHI Compute Morph Async Telemetry D3D12 v1`
- `Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1`
- `Runtime RHI Compute Morph NORMAL/TANGENT Output D3D12 v1`
- `Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1`
- `Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1`
- `Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1`
- `Runtime RHI Compute Morph Queue Synchronization D3D12 v1`
- `Runtime RHI Compute Morph Renderer Consumption Vulkan v1`
- `Runtime RHI Compute Morph Skin Composition D3D12 v1`
- `Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1`
- `Runtime RHI Compute Morph Vulkan Proof v1`
- `Runtime Ring-Backed Texture Upload v1`
- `Runtime Scene RHI Compute Morph Skin Palette D3D12 v1`
- `Runtime UI Font Image Adapter v1`
- `Runtime Upload Queue Wait v1`
- `RuntimeConstructionPlacementValidationContext`
- `RuntimeCraftingRecipeDocument`
- `RuntimeGameplayDebugOverlayPlan`
- `RuntimeGameplayDebugOverlayRowDesc`
- `RuntimeInputContextLayerKind::overlay`
- `RuntimeInputContextStack`
- `RuntimeInputRebindingAxisCaptureRequest`
- `RuntimeInputRebindingCaptureResult`
- `RuntimeInputRebindingFocusCaptureResult`
- `RuntimeInputRebindingPresentationRow`
- `RuntimeInputStateView`
- `RuntimeInventoryState`
- `RuntimeInventoryTransitionRequest`
- `RuntimeItemCatalogDocument`
- `RuntimeItemCatalogValidationContext`
- `RuntimeItemCatalogValidationResult`
- `RuntimeMenuHudCommandIntent::restart_session`
- `RuntimeMeshUploadOptions::upload_ring`
- `RuntimeMorphMeshComputeBindingOptions::output_normal_usage`
- `RuntimeMorphMeshComputeOutputSlot`
- `RuntimeMorphMeshUploadOptions::upload_ring`
- `RuntimePackageStreamingMeshUploadBindingResult`
- `RuntimePackageStreamingMeshUploadSource`
- `RuntimeQuestDialogueDocument`
- `RuntimeQuestDialogueState`
- `RuntimeSceneComputeMorphSkinnedMeshBinding`
- `RuntimeSceneConstructionPlacementIntentDesc`
- `RuntimeSceneGameplayInteractionSourceRow`
- `RuntimeSkinnedMeshUploadOptions::upload_ring`
- `RuntimeTextureUploadOptions::upload_ring`
- `RuntimeUiAtlasGlyph`
- `SceneSkinnedGpuBindingPalette`
- `SdlDesktopPresentationVulkanSceneRendererDesc`
- `SdlDesktopPresentationVulkanSceneRendererDesc::compute_morph_skinned_shader`
- `SetComputeRootDescriptorTable`
- `SpriteAtlasSourceAuthoringDesc`
- `Stable Directional Light-Space Policy v0`
- `Staging Pool Lease Adoption v1`
- `Static-analysis drift includes`
- `SubmittedCommandCalibratedTiming`
- `SubmittedCommandCalibratedTimingStatus`
- `TextAdapterGlyphPlaceholder`
- `TextShapingRequestPlan`
- `TextShapingRequestPlan::ready`
- `TextShapingResult::succeeded`
- `TransientTextureAliasGroup`
- `UI focus integration`
- `UiAtlasMetadataGlyph`
- `UiRendererGlyphAtlasBinding`
- `Use lightweight static validation for docs/agent/rules/subagent-only PRs`
- `VCPKG_MANIFEST_FEATURES`
- `VCPKG_MANIFEST_INSTALL=OFF`
- `Vulkan POSITION/NORMAL/TANGENT compute morph package smoke through explicit SPIR-V artifacts`
- `Vulkan compute morph package smoke does not support async telemetry requirements; use`
- `Vulkan scene compute mapping SPIR-V validation failed`
- `Vulkan scene compute morph compute SPIR-V validation failed`
- `Vulkan scene compute morph vertex SPIR-V validation failed`
- `Vulkan skin+compute package smoke counters through explicit SPIR-V artifacts`
- `VulkanRhiDevice::wait_for_queue`
- `VulkanRuntimeComputePipeline`
- `VulkanRuntimeTexture::Impl::MemoryAllocation`
- `Windows Graphics Tools`
- `Windows Performance Toolkit`
- `Windows host diagnostics guidance`
- `Windows long-path fallback`
- `Windows long-path fallback inside the guarded script`
- `[[vk::binding(7, 0)]]`
- `[[vk::location(0)]]`
- `[numthreads(1, 1, 1)]`
- `_NT_SYMBOL_PATH`
- `acquire_transient_texture_alias_group(const TextureDesc& desc`
- `activate_placed_texture`
- `actual submitted D3D12 graphics/compute command lists`
- `actual submitted compute/graphics fences`
- `adapter.begin_text_input`
- `adapter.decode_image`
- `adapter.end_text_input`
- `adapter.publish_nodes`
- `adapter.rasterize_glyph`
- `adapter.shape_text`
- `adapter.update_composition`
- `add_compute_morph_skinned_mesh_binding`
- `advance_runtime_inventory_state`
- `advance_runtime_quest_dialogue_state`
- `after ````ExecuteCommandLists```` submits work rather than after fence completion`
- `after producer waits and before the callback`
- `agent-context.ps1 -ContextProfile Minimal|Standard`
- `agent-surface drift check`
- `aliasing_barriers_recorded`
- `always-running aggregate gate`
- `apple-host-evidence-check: host-gated`
- `apple-host-helpers.ps1`
- `apply_animation_fabrik_ik_3d_solution_to_pose`
- `apply_animation_local_rotation_limits_3d`
- `apply_packed_ui_atlas_package_update`
- `apply_packed_ui_glyph_atlas_package_update`
- `apply_runtime_input_rebinding_profile`
- `approval-capable session`
- `auto-merge registration`
- `automatic aliasing barrier before the first pass`
- `axis_value`
- `backend-private`
- `begin_platform_text_input`
- `billing or account limits`
- `bind_compute_pipeline`
- `bind_gamepad_axis`
- `bind_gamepad_button`
- `bind_gamepad_button_in_context`
- `bind_key_axis_in_context`
- `bind_key_in_context`
- `bind_pointer_in_context`
- `bind_skinned_mesh_vertex_buffers`
- `blackboard`
- `broad async/background streaming`
- `budget gates`
- `build_ai_perception_snapshot_2d`
- `build_animation_model_pose_3d`
- `build_diagnostics_ops_plan`
- `build_physics_world_3d_from_authored_collision_scene`
- `build_scene_compute_morph_bindings(`
- `calculate_navigation_local_avoidance`
- `calibrate_queue_clock`
- `calibrated interval rows`
- `calibrated overlap diagnostic`
- `candidate row grid/world origins`
- `capability or milestone plan boundaries`
- `catch (...)`
- `cdb -version`
- `collision_query_batch_ready`
- `compile_runtime_morph_position_output_slot_compute_shader`
- `compute_binding.output_slots`
- `compute_dispatch_mapped`
- `compute_morph_async_compute_queue_submits`
- `compute_morph_async_last_graphics_submitted_fence_value`
- `compute_morph_bindings.queue_waits`
- `compute_morph_mesh_bindings`
- `compute_morph_output_position_bytes`
- `compute_morph_output_position_bytes == 36`
- `compute_morph_shader`
- `compute_morph_skinned_mesh_bindings`
- `compute_morph_skinned_shader`
- `compute_morph_vertex_shader`
- `compute_options.output_normal_usage`
- `compute_options.output_tangent_usage`
- `compute_queue_submits`
- `conflicting initial shared-handle states`
- `context->stats().last_copy_queue_wait_fence_value == graphics_fence.value`
- `context_->queue_wait_for_fence(queue, fence)`
- `convert_gpu_timestamp_to_qpc`
- `count-based`
- `create_compute_pipeline`
- `create_placed_texture`
- `create_placed_texture_alias_group`
- `create_placed_texture_alias_group(desc, texture_count)`
- `create_runtime_compute_pipeline`
- `create_transient_texture_alias_images`
- `credential-manager-core`
- `crowd`
- `currentActivePlan`
- `d3d12 calibrated overlap diagnostic measured`
- `d3d12 calibrated queue timing measured`
- `d3d12 device context applies public null placed texture aliasing state updates`
- `d3d12 device context creates placed transient texture resources`
- `d3d12 device context keeps unrelated placed texture aliasing barriers conservative`
- `d3d12 device context records backend private calibrated queue timing diagnostics`
- `d3d12 device context records backend private queue timestamp intervals`
- `d3d12 device context records non null placed resource aliasing barriers`
- `d3d12 device context reports graphics and compute queue clock calibration`
- `d3d12 device context reports graphics and compute timestamp measurement support`
- `d3d12 queue clock calibration ready`
- `d3d12 queue timestamp interval measured`
- `d3d12 rhi compute morph output ring writes a selected position slot`
- `d3d12 rhi compute morph writes morphed runtime normals and tangents`
- `d3d12 rhi compute morph writes morphed runtime positions`
- `d3d12 rhi device records public wildcard texture aliasing barrier commands`
- `d3d12 rhi device records texture aliasing barrier commands`
- `d3d12 rhi device reports compute graphics async overlap as serial dependency when graphics waits`
- `d3d12 rhi device reports invalid queue wait attempts`
- `d3d12 rhi device reports pipelined compute graphics output slot scheduling as a timing candidate`
- `d3d12 rhi device synchronizes graphics queue with compute fence`
- `d3d12 rhi device transient texture alias group returns distinct placed handles`
- `d3d12 rhi device uses per queue fence identity for colliding fence values`
- `d3d12 rhi frame renderer composes compute morphed positions with skinned joint palette`
- `d3d12 rhi frame renderer consumes compute morph output positions`
- `d3d12 submitted command calibrated timing measured`
- `d3d12SDKLayers.dll`
- `data inheritance/content preservation`
- `debug overlay`
- `debug_overlay_rows=`
- `declared shadow-color/shadow-depth/scene-color/scene-depth writer-access-backed target-state preparation`
- `decode_image`
- `decode_image_request`
- `decoded image must be RGBA8`
- `default-unbounded`
- `depth-input postprocess`
- `desktopRuntime3dDirectionalShadowPackageSmoke`
- `desktopRuntime3dEntityScaleCullingPackageSmoke`
- `desktopRuntime3dNativeUiOverlayPackageSmoke`
- `desktopRuntime3dNativeUiTextGlyphAtlasPackageSmoke`
- `desktopRuntime3dNativeUiTexturedSpriteAtlasPackageSmoke`
- `desktopRuntime3dPackageStreamingSafePointSmoke`
- `desktopRuntime3dPlayablePackageSmoke`
- `desktopRuntime3dPostprocessDepthPackageSmoke`
- `desktopRuntime3dRendererQualityPackageSmoke`
- `desktopRuntime3dShadowMorphCompositionPackageSmoke`
- `desktopRuntime3dVisibleProductionPackageProof`
- `deterministic frame sampling`
- `deterministic-glyph-atlas-rgba8-max-side`
- `device hotplug/selection`
- `device->stats().last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute`
- `device->stats().last_graphics_queue_wait_fence_value == compute_fence.value`
- `device->stats().queue_waits == 1`
- `device->wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence)`
- `device.wait_for_queue(rhi::QueueKind::graphics, fence)`
- `diagnose_calibrated_compute_graphics_overlap`
- `diagnose_compute_graphics_async_overlap_readiness`
- `diagnose_pipelined_compute_graphics_async_overlap_readiness`
- `diagnose_rhi_device_submitted_command_compute_graphics_overlap`
- `diagnostics ops plan reports trace summary and unsupported upload boundaries`
- `different current slot`
- `direct render pass begin/end APIs`
- `direct-clang-format-status`
- `directional_shadow_filter_mode`
- `directional_shadow_filter_radius_texels`
- `directional_shadow_filter_taps`
- `directional_shadow_ready`
- `directional_shadow_status`
- `directional_shadow_status=ready`
- `dispatch(`
- `dispatch_scene_compute_morph_skinned_bindings`
- `distinct joint-palette descriptor`
- `distinct texture handles in group resource order`
- `docs, skills, rules, settings, subagents`
- `docs/README.md`
- `docs/agent-only PRs use lightweight static validation`
- `docs/agent/rules/subagent-only PRs should use lightweight static validation`
- `docs/roadmap.md`
- `docs/superpowers/plans/README.md`
- `empty_image_decode_bytes`
- `end_platform_text_input`
- `ensure_fence(QueueKind queue)`
- `evaluate_physics_character_dynamic_policy_3d`
- `evaluate_physics_determinism_gate_3d`
- `evaluate_sdl_desktop_presentation_quality_gate`
- `execute_frame_graph_rhi_multi_queue_schedule`
- `execute_selected_runtime_package_streaming_safe_point`
- `execute_upload_gpu_batch_async`
- `explicit 1.0 exclusion`
- `final completion report must not stop after local validation`
- `focus_retained`
- `framegraph_barrier_steps_executed`
- `framegraph_barrier_steps_executed=15`
- `framegraph_barrier_steps_executed=9`
- `framegraph_multiqueue_aliasing_barriers_recorded=1`
- `framegraph_multiqueue_barriers_recorded=4`
- `framegraph_multiqueue_command_lists_submitted=4`
- `framegraph_multiqueue_queue_waits_recorded=3`
- `framegraph_multiqueue_submitted_pass_fences=4`
- `framegraph_passes_executed=4`
- `framegraph_passes_executed=6`
- `framegraph_render_passes_recorded`
- `framegraph_render_passes_recorded=6`
- `function Format-CppSourceText`
- `function New-DesktopRuntime3DMainCpp`
- `function New-DesktopRuntime3DPackageFiles`
- `game-owned`
- `gameengine-agent-integration`
- `gameengine-game-development`
- `gameengine-rendering`
- `gameplay_interactions`
- `gameplay_systems_construction_placement_intent_accepted_rows`
- `gameplay_systems_construction_placement_intent_occupied_cells`
- `gameplay_systems_construction_placement_intent_rows`
- `gameplay_systems_construction_placement_validation_rows`
- `gameplay_systems_inventory_items_final_workbench_quantity`
- `gameplay_systems_inventory_items_transition_rows`
- `generated color-postprocess scaffold`
- `gh pr create`
- `gh pr merge`
- `gh pr merge --auto --merge --delete-branch`
- `gh pr merge --merge --delete-branch`
- `gh pr view`
- `gh pr view <pr> --json headRefOid,statusCheckRollup,url`
- `git checkout`
- `git commit`
- `git push`
- `git push --force`
- `git push --force origin main`
- `git push -u origin main`
- `git push origin HEAD:main`
- `git push origin main`
- `git restore`
- `git worktree prune`
- `git worktree remove`
- `global input consumption`
- `glyph_lookup_key`
- `graphics_queue_submits`
- `graphics_queue_waited_for_previous_compute`
- `guarded/non-following`
- `hosted PR check failure`
- `import_gltf_node_transform_animation_tracks_3d`
- `input_contexts=`
- `installed-d3d12-3d-entity-scale-culling-smoke`
- `interactive runtime/game rebinding`
- `invalid_accessibility_bounds`
- `invalid_font_allocation`
- `invalid_font_family`
- `invalid_font_glyph`
- `invalid_font_pixel_size`
- `invalid_image_decode_result`
- `invalid_image_decode_uri`
- `invalid_ime_cursor`
- `invalid_ime_target`
- `invalid_platform_text_input_bounds`
- `invalid_platform_text_input_target`
- `invalid_text_shaping_font_family`
- `invalid_text_shaping_max_width`
- `invalid_text_shaping_result`
- `invalid_text_shaping_text`
- `isolation: worktree`
- `kRuntimeSceneVulkanComputeMorphShaderPath`
- `kRuntimeSceneVulkanComputeMorphSkinnedShaderPath`
- `kRuntimeSceneVulkanComputeMorphSkinnedVertexShaderPath`
- `kRuntimeSceneVulkanComputeMorphVertexShaderPath`
- `kebab-case`
- `keeps Windows long-path fallback inside that guarded script`
- `kind=animation_quaternion_clip`
- `kind=ui_atlas`
- `kind=ui_atlas_texture`
- `last_compute_submitted_fence_value`
- `last_copy_queue_wait_fence_value`
- `last_graphics_queue_wait_fence_queue`
- `last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute`
- `last_graphics_queue_wait_fence_value`
- `last_graphics_queue_wait_fence_value == compute_fence.value`
- `last_graphics_queue_wait_sequence`
- `last_graphics_submitted_fence_value`
- `last_graphics_submitted_fence_value == graphics_fence.value`
- `last_signaled_by_queue_`
- `lcov --ignore-errors unused`
- `lightweight static validation`
- `load_packaged_vulkan_scene_compute_morph_shaders`
- `load_packaged_vulkan_scene_compute_morph_skinned_shaders`
- `load_runtime_session_profile_documents`
- `local checkout fast-forward`
- `lowercase snake_case`
- `machine-readable status claims`
- `make_runtime_compute_morph_skinned_mesh_gpu_binding`
- `make_runtime_package_streaming_frame_graph_texture_bindings`
- `make_skinned_position_payload`
- `make_ui_text_glyph_sprite_command`
- `manifest.fragments`
- `match =`
- `measure_calibrated_queue_timing`
- `measure_queue_timestamp_interval`
- `measure_rhi_device_calibrated_queue_timing`
- `merge/delete-branch`
- `mergeStateStatus`
- `metallib`
- `missing_pipelined_slot_evidence`
- `mixer authoring`
- `move_physics_character_controller_3d`
- `msiexec`
- `multi-slot compute morph output-ring`
- `narrow validation commands`
- `native handle`
- `native handles`
- `native text-input object/session ownership`
- `navmesh`
- `new-game-helpers.ps1`
- `new-game-templates.ps1`
- `no durable guidance changed`
- `normalized-build-environment`
- `normalized-configure-environment`
- `not troubleshooting playbooks`
- `not_match =`
- `null rhi records queue waits for submitted fences`
- `null rhi records texture aliasing barriers without changing texture state`
- `null rhi records wildcard texture aliasing barriers`
- `null rhi reports invalid queue wait attempts`
- `null rhi submitted fences carry queue identity and per queue values`
- `null rhi transient texture alias group returns distinct handles under one lease`
- `null-resource aliasing barrier`
- `null_resource_aliasing_barriers`
- `official Anthropic Claude Code docs`
- `official Anthropic docs`
- `official Anthropic documentation`
- `official GitHub Flow`
- `official Microsoft D3D12`
- `official OpenAI Codex docs`
- `one backend-neutral ````IRhiDevice::acquire_transient_texture_alias_group```` lease per alias group`
- `one roadmap, one active gap-cluster burn-down or milestone`
- `output_normal_buffer`
- `output_normal_usage`
- `output_position_resources`
- `output_slot1`
- `output_slot_count`
- `output_slots`
- `output_tangent_buffer`
- `output_tangent_usage`
- `pResourceAfter = after_resource`
- `pResourceAfter = texture_resource`
- `pResourceBefore = before_resource`
- `pack_sprite_atlas_rgba8_max_side`
- `package-visible compute morph queue-wait smoke evidence`
- `package-visible tilemap runtime/editor counters`
- `package_streaming_status`
- `packed runtime UI atlas apply leaves existing files unchanged when validation fails`
- `packed runtime UI atlas authoring maps decoded images into texture page and metadata`
- `packed runtime UI atlas package update writes texture page metadata and package index`
- `packed runtime UI atlas rejects invalid decoded images and package path collisions`
- `packed runtime UI glyph atlas apply leaves existing files unchanged when validation fails`
- `packed runtime UI glyph atlas authoring maps rasterized glyphs into texture page and metadata`
- `packed runtime UI glyph atlas package update writes texture page metadata and package index`
- `packed runtime UI glyph atlas rejects invalid glyph pixels and package path collisions`
- `path-filtered required checks`
- `per-device profiles`
- `per-queue fences`
- `persistent blackboard`
- `phase-gated milestone plan`
- `placed_resource_activation_barriers`
- `placed_resource_aliasing_barriers`
- `placed_resource_state_updates`
- `placed_resources_alive`
- `placed_texture_alias_groups_created`
- `placed_texture_heaps_created`
- `placed_textures_created`
- `plan_accessibility_publish`
- `plan_audio_device_stream`
- `plan_font_rasterization_request`
- `plan_frame_graph_rhi_queue_waits`
- `plan_gameplay_audio_mix`
- `plan_ime_composition_update`
- `plan_navigation_grid_agent_path`
- `plan_navigation_navmesh_crowd`
- `plan_navigation_navmesh_path`
- `plan_packed_ui_glyph_atlas_package_update`
- `plan_physics_advanced_controller_3d`
- `plan_placeholder_asset_bundle`
- `plan_placeholder_asset_cook_package`
- `plan_runtime_entity_scale_culling`
- `plan_runtime_gameplay_debug_overlay`
- `plan_runtime_input_context_stack`
- `plan_runtime_menu_hud`
- `plan_runtime_scene_construction_placement_intents`
- `plan_runtime_scene_gameplay_interactions`
- `plan_sprite_atlas_source_authoring`
- `plan_sprite_batches`
- `plan_text_shaping_request`
- `playable_3d_status`
- `playable_3d_status=ready`
- `policy reload`
- `post-merge remote-tracking cleanup`
- `post-merge worktree cleanup`
- `postprocess_depth_input_ready`
- `postprocess_depth_input_ready=1`
- `prefix_rule`
- `prepare-worktree.ps1`
- `present_runtime_input_action_trigger`
- `present_runtime_input_axis_source`
- `previously completed output slot`
- `probe-timeout`
- `production-readiness-audit-check: ok`
- `ps_main`
- `publish_accessibility_payload`
- `publish_ime_composition`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game sample_headless -Configuration Debug`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -Game sample_headless -UseLocalValidationKey`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ready-task-pr.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game sample_headless -Configuration Release -SkipBuild -StartEmulator -AvdName Mirakanai_API36`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
- `quaternion package smoke is limited to cooked`
- `query_performance_counter_frequency`
- `queue_fences`
- `queue_timestamp_measurement_support`
- `queue_wait_failures`
- `radial stick deadzones`
- `rasterize_font_glyph`
- `rasterized glyph must be RGBA8`
- `rasterized-glyph-adapter`
- `read_rhi_device_submitted_command_calibrated_timing`
- `ready_for_backend_private_timing`
- `recommendedNextPlan`
- `record_frame_graph_texture_aliasing_barriers`
- `record_queue_submit(stats_, d3d12_commands->queue_kind(), fence)`
- `record_queue_submit(stats_, queue, last_signaled_)`
- `record_queue_submit(stats_, vulkan_commands->queue_kind(), fence)`
- `record_queue_wait(impl_->stats, queue, fence)`
- `record_queue_wait(stats_, queue, fence)`
- `record_runtime_compute_dispatch`
- `record_runtime_compute_pipeline_binding`
- `record_runtime_texture_aliasing_barrier`
- `record_submitted_command_timing_begin`
- `render_audio_device_stream_interleaved_float`
- `renderer_morph_descriptor_binds`
- `renderer_quality_expected_framegraph_barrier_steps`
- `renderer_quality_expected_framegraph_barrier_steps=4`
- `renderer_quality_expected_framegraph_barrier_steps=9`
- `renderer_quality_expected_framegraph_passes=2`
- `renderer_quality_expected_framegraph_render_passes`
- `renderer_quality_framegraph_barrier_steps_ok`
- `renderer_quality_framegraph_execution_budget_ok`
- `renderer_quality_framegraph_render_passes`
- `renderer_quality_framegraph_render_passes_ok`
- `renderer_quality_postprocess_depth_input_ready=1`
- `renderer_quality_status`
- `rendering`
- `replan_navigation_grid_path`
- `resolve_ui_text_glyph_binding`
- `resource_alias_group_ids`
- `resources_share_placed_alias_group`
- `reviewer, explorer, architect, planning, and auditor subagents read-only`
- `rhi async overlap readiness diagnostics classify pipelined output slot scheduling`
- `rhi async overlap readiness diagnostics classify serial graphics waits`
- `rhi_stats.last_graphics_submitted_fence_value`
- `runtime UI PNG image decoding adapter fails closed when importers are disabled`
- `runtime UI PNG image decoding adapter returns rgba8 image when importers are enabled`
- `runtime gameplay debug overlay plan produces deterministic display rows`
- `runtime gameplay debug overlay plan rejects duplicate and invalid rows`
- `runtime input context stack plan keeps gameplay under passive overlay`
- `runtime input context stack plan rejects invalid layer descriptions`
- `runtime input context stack plan resolves modal menu before gameplay`
- `runtime input context stack plan uses default context when no layer is active`
- `runtime menu hud plan produces deterministic display and command rows`
- `runtime menu hud plan rejects duplicate row and command ids`
- `runtime menu hud plan rejects missing command ids and invalid command targets`
- `runtime morph compute normal output requires morph normal delta buffer`
- `runtime morph compute output ring currently supports POSITION output slots only`
- `runtime morph compute output ring requires at least one slot`
- `runtime morph compute tangent output requires morph tangent delta buffer`
- `runtime rhi composes compute morph output with skinned mesh attributes`
- `runtime rhi creates compute morph binding for normal and tangent outputs`
- `runtime rhi creates compute morph binding for position output`
- `runtime rhi creates compute morph output ring with distinct position slots`
- `runtime rhi exposes compute morph output as mesh binding`
- `runtime scene gameplay bindings fail closed for invalid ambiguous and missing component rows`
- `runtime scene gameplay interaction plan composes binding rows in authored order`
- `runtime scene gameplay interaction plan rejects invalid targets duplicates and terminal transitions`
- `runtime scene resolves authored gameplay bindings to component-backed nodes`
- `runtime scene rhi builds compute morph skinned gpu palette from selected cooked assets`
- `runtime session profile document bundle loads documents and defaults missing optional rows`
- `runtime session profile document bundle separates corrupt and unsupported documents`
- `runtime session profile document bundle writes reviewed defaults without deleting unrelated files`
- `runtime session profile path plan composes deterministic game local document paths`
- `runtime session profile path plan rejects unsafe ids and paths without partial paths`
- `runtime/.gitattributes`
- `runtime/RHI-only D3D12 evidence`
- `runtime/assets/desktop_runtime/hud.uiatlas`
- `runtime_morph_compute_output_slot`
- `runtime_profile_documents`
- `same alias-group placed pairs`
- `same-batch occupied-cell`
- `same-offset placed textures`
- `same_frame_graphics_wait_serializes_compute`
- `sample_2d_desktop_runtime_package`
- `sample_animation_local_pose_3d`
- `sample_gameplay_foundation`
- `sample_generated_desktop_runtime_3d_package`
- `sample_quat_keyframes`
- `sampled-depth readback`
- `scene-component-prefab-schema-v2`
- `scene/physics`
- `scene_compute_morph.cs.spv`
- `scene_compute_morph.vs.spv`
- `scene_compute_morph_skinned.cs.spv`
- `scene_compute_morph_skinned.vs.spv`
- `scene_gpu_compute_morph_async_*`
- `scene_gpu_compute_morph_async_compute_queue_submits`
- `scene_gpu_stats.compute_morph_async_compute_queue_submits < 1`
- `scene_gpu_stats.compute_morph_queue_waits < 1`
- `scene_renderer.compute_morph_mesh_bindings.push_back`
- `scene_renderer.compute_morph_shader.entry_point`
- `scene_renderer.compute_morph_skinned_mesh_bindings.push_back`
- `scene_renderer.compute_morph_skinned_shader.entry_point`
- `scene_renderer.compute_morph_vertex_shader.entry_point`
- `selected D3D12 generated 3D graphics morph + directional shadow receiver smoke through --require-shadow-morph-composition with renderer_gpu_morph_draws, renderer_morph_descriptor_binds, directional_shadow_ready=1, framegraph_passes=3, framegraph_passes_executed=6, framegraph_render_passes_recorded=6`
- `selected hosted checks to complete`
- `shadow receiver readback`
- `shape_text_run`
- `shared-handle state-handoff aware`
- `single-quoted PowerShell strings`
- `skin_animation_vertices_cpu`
- `skin_attribute_vertex_buffer`
- `skin_attribute_vertex_stride`
- `smooth_navigation_grid_path`
- `solve_animation_fabrik_ik_3d_chain`
- `solve_animation_two_bone_ik_3d_orientation`
- `solve_physics_joints_3d`
- `span.glyph`
- `specific, concise, verifiable`
- `sprite_animation_frames_sampled=3`
- `stats.compute_morph_async_compute_queue_submits == 1`
- `stats.compute_morph_async_last_graphics_submitted_fence_value == graphics_fence.value`
- `stats.compute_morph_queue_waits == 1`
- `stats.last_graphics_queue_wait_fence_queue == QueueKind::compute`
- `stats.last_graphics_submit_sequence > stats.last_graphics_queue_wait_sequence`
- `std::uint32_t glyph`
- `std::vector<TextureHandle> textures`
- `struct ComputePipelineDesc`
- `struct RuntimeMorphMeshComputeBinding`
- `target_compile_features`
- `targeted drift checks`
- `text eol=lf`
- `text_glyph_sprites_submitted`
- `text_glyphs_missing`
- `text_glyphs_resolved=2`
- `texture_aliasing_barrier(TextureHandle before, TextureHandle after)`
- `texture_aliasing_barriers`
- `thin static-contract ledger entrypoints`
- `tilemap_cells_sampled=3`
- `timestamp query heap recording`
- `tools/check-coverage-thresholds.ps1`
- `tools/check-format.ps1`
- `tools/check-text-format-contract.ps1`
- `tools/check-text-format.ps1`
- `tools/check-tidy.ps1`
- `tools/check-toolchain.ps1`
- `tools/check-toolchain.ps1 -RequireDirectCMake`
- `tools/cmake.ps1`
- `tools/ctest.ps1`
- `tools/new-game-templates.ps1`
- `tools/prepare-worktree.ps1`
- `tools/ready-task-pr.ps1`
- `tools/remove-merged-worktree.ps1`
- `tools/static-contract-ledger.ps1`
- `transient_texture_heap_allocations`
- `transient_texture_placed_allocations`
- `transient_texture_placed_resources_alive`
- `ui accessibility publish plan blocks invalid nodes before adapter`
- `ui accessibility publish plan dispatches validated nodes to adapter`
- `ui font rasterization request plan blocks invalid request before adapter`
- `ui font rasterization request plan dispatches valid request to adapter`
- `ui font rasterization result reports invalid adapter allocation`
- `ui image decode request plan blocks invalid request before adapter`
- `ui image decode request plan dispatches valid request to adapter`
- `ui image decode result reports missing or invalid adapter output`
- `ui ime composition publish plan blocks invalid composition before adapter`
- `ui ime composition publish plan dispatches valid composition to adapter`
- `ui renderer reports missing glyph atlas bindings without fake sprites`
- `ui renderer submits monospace text glyphs through glyph atlas palette`
- `ui text shaping request plan blocks invalid request before adapter`
- `ui text shaping request plan dispatches valid request to adapter`
- `ui text shaping result reports invalid adapter runs`
- `ui::IImageDecodingAdapter`
- `ui_texture_overlay_atlas_ready=1`
- `unsupportedProductionGaps`
- `unsupported_missing_timestamp_support`
- `upload_queue_waits_recorded`
- `upload_runtime_package_streaming_frame_graph_texture_bindings`
- `upload_runtime_package_streaming_mesh_gpu_bindings`
- `utf8_scalar_glyph`
- `validate_behavior_authoring_document`
- `validate_navigation_grid_path`
- `validate_runtime_construction_placement`
- `validate_runtime_inventory_state`
- `validate_runtime_item_catalog_document`
- `validate_runtime_quest_dialogue_document`
- `validate_runtime_quest_dialogue_state`
- `validation-recipe-core.ps1`
- `viewport color-state executor slices`
- `viewport.clear```` render pass envelopes`
- `vkCmdDispatch`
- `vkCreateComputePipelines`
- `void bind_compute_pipeline(ComputePipelineHandle pipeline)`
- `void dispatch(std::uint32_t group_count_x`
- `void texture_aliasing_barrier(TextureHandle before, TextureHandle after) override`
- `vs_main`
- `vulkan rhi device bridge proves compute dispatch readback with configured SPIR-V artifact`
- `vulkan rhi device bridge proves runtime compute morph normal tangent readback with configured SPIR-V artifact`
- `vulkan rhi device bridge proves runtime compute morph position readback with configured SPIR-V artifact`
- `vulkan rhi device bridge records texture aliasing barrier`
- `vulkan rhi device transient texture alias group shares one memory allocation`
- `vulkan rhi frame renderer consumes runtime compute morph output positions when configured`
- `vulkan_compute_morph_shader_diagnostic`
- `vulkan_compute_morph_skinned_shader_diagnostic`
- `vulkan_compute_morph_tangent_frame.cs.spv`
- `vulkan_desc.compute_morph_mesh_bindings`
- `vulkan_desc.compute_morph_shader`
- `vulkan_desc.compute_morph_skinned_mesh_bindings`
- `vulkan_desc.compute_morph_skinned_shader`
- `vulkan_desc.compute_morph_vertex_shader`
- `vulkan_image_create_alias_bit`
- `wait_for_queue(QueueKind queue, FenceValue fence)`
- `wait_for_runtime_uploads_on_queue`
- `wildcard/null barrier support`
- `without adding native dump writing`
- `without changing sprite order`
- `without claiming Play upload`
- `without claiming font loading/rasterization`
- `without claiming performance overlap`
- `without exposing RHI or native backend details`
- `without exposing RHI or native handles`
- `without exposing Vulkan/native handles`
- `without exposing queue/fence handles`
- `without following reparse points`
- `without font loading/rasterization implementations`
- `without image decoding implementations`
- `without shaping implementations`
- `workspace override`
- `worktree-local vcpkg reparse points`
- `write_ai_perception_blackboard`
- `write_runtime_session_profile_documents`
- `xcodebuild`
- `xcrun`
- `AccessibilityPublishPlan`
- `FontRasterizationRequestPlan`
- `ImageDecodeRequestPlan`
- `ImeCompositionPublishPlan`
- `MonospaceTextLayoutPolicy`
- `PlatformTextInputSessionPlan`
- `UiRendererImagePalette`
- `glyph atlas generation`
- `image decoding`
- `plan_image_decode_request`
- `plan_packed_ui_atlas_package_update`
- `Completed gap burn-down`
- `Renderer RHI Resource Foundation 1.0 Scope Closeout v1`
- `upload-staging-v1`
- `--require-compute-morph-async-telemetry`
- `2D Native Sprite Batching Execution v1`
- `2D Sprite Animation Package v1`
- `2D Tilemap Editor Runtime UX v1`
- `ComputePipelineDesc`
- `GameEngine.RuntimeInputRebindingProfile.v1`
- `Generated 3D Compute Morph Async Telemetry Package Smoke D3D12`
- `Generated 3D Compute Morph NORMAL/TANGENT Package Smoke D3D12`
- `Generated 3D Compute Morph Package Smoke Vulkan`
- `Generated 3D Compute Morph Skin Package Smoke D3D12`
- `Generated 3D Compute Morph Skin Package Smoke Vulkan`
- `IRhiCommandList::dispatch`
- `IRhiDevice::wait_for_queue`
- `MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV`
- `MK_VULKAN_TEST_COMPUTE_MORPH_SPV`
- `MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV`
- `RHI D3D12 Calibrated Queue Timing Diagnostics`
- `RHI D3D12 Per-Queue Fence Synchronization`
- `RHI D3D12 Queue Clock Calibration Foundation`
- `RHI D3D12 Queue Timestamp Measurement Foundation`
- `RHI D3D12 Submitted Command Calibrated Timing Scopes`
- `RHI Upload Stale Generation Diagnostics v1`
- `RHI Vulkan Compute Dispatch Foundation`
- `RhiAsyncOverlapReadinessDiagnostics`
- `Runtime RHI Compute Morph Async Overlap Evidence D3D12`
- `Runtime RHI Compute Morph Async Telemetry D3D12`
- `Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12`
- `Runtime RHI Compute Morph NORMAL/TANGENT Output D3D12`
- `Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan`
- `Runtime RHI Compute Morph Pipelined Output Ring D3D12`
- `Runtime RHI Compute Morph Pipelined Scheduling D3D12`
- `Runtime RHI Compute Morph Queue Synchronization D3D12`
- `Runtime RHI Compute Morph Renderer Consumption Vulkan`
- `Runtime RHI Compute Morph Skin Composition D3D12`
- `Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12`
- `Runtime RHI Compute Morph Vulkan Proof`
- `Runtime Scene RHI Compute Morph Skin Palette D3D12`
- `RuntimeMorphMeshComputeBinding`
- `create_runtime_morph_mesh_compute_binding`
- `make_runtime_compute_morph_output_mesh_gpu_binding`
- `not_proven_serial_dependency`
- `package-visible`
- `scene_gpu_compute_morph_async_`
- `scene_gpu_compute_morph_queue_waits`
- `stale_generation`
- `std::locale::classic()`
- `symbolic glyph lookup keys`
- `Android Release Device Matrix v1`
- `Apple Metal iOS Host Evidence v1`
- `Production 1.0 Readiness Audit v1`
- `sample_headless`
- `DiagnosticsOpsPlan`
- `Execute this master plan by burning down one`
- `Gap Burn-down Execution Strategy`
- `Mirakanai_API36`
- `UiRendererGlyphAtlasPalette`
- `apple-host-evidence-check`
- `production-readiness-audit-check`
- `2026-05-02-2d-packaged-playable-generation-loop-v1.md`
- `2026-05-02-editor-playtest-package-review-loop-v1.md`
- `2026-05-02-renderer-resource-residency-upload-execution-v1.md`
- `2026-05-05-animation-float-transform-application-v1.md`
- `2026-05-05-animation-transform-binding-source-v1.md`
- `2026-05-05-cooked-animation-quaternion-clip-v1.md`
- `2026-05-05-generated-3d-morph-gpu-palette-smoke-v1.md`
- `2026-05-05-generated-3d-morph-normal-tangent-package-smoke-v1.md`
- `2026-05-05-generated-3d-morph-package-consumption-v1.md`
- `2026-05-05-generated-3d-transform-animation-scaffold-v1.md`
- `2026-05-05-gltf-node-transform-animation-binding-source-bridge-v1.md`
- `2026-05-05-gltf-node-transform-animation-float-clip-bridge-v1.md`
- `2026-05-05-gltf-node-transform-animation-import-v1.md`
- `2026-05-05-gpu-morph-d3d12-proof-v1.md`
- `2026-05-05-gpu-morph-normal-tangent-d3d12-proof-v1.md`
- `2026-05-05-rhi-compute-dispatch-foundation-v1.md`
- `2026-05-05-runtime-rhi-compute-morph-d3d12-proof-v1.md`
- `2026-05-05-runtime-scene-animation-transform-binding-v1.md`
- `2026-05-05-runtime-scene-rhi-morph-gpu-palette-v1.md`
- `2026-05-06-2d-native-sprite-batching-execution-v1.md`
- `2026-05-06-2d-sprite-animation-package-v1.md`
- `2026-05-06-2d-tilemap-editor-runtime-ux-v1.md`
- `2026-05-06-android-release-device-matrix-v1.md`
- `2026-05-06-apple-metal-ios-host-evidence-v1.md`
- `2026-05-06-crash-telemetry-trace-ops-v1.md`
- `2026-05-06-desktop-release-package-evidence-v1.md`
- `2026-05-06-generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1.md`
- `2026-05-06-generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1.md`
- `2026-05-06-generated-3d-compute-morph-package-smoke-d3d12-v1.md`
- `2026-05-06-generated-3d-compute-morph-package-smoke-vulkan-v1.md`
- `2026-05-06-generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1.md`
- `2026-05-06-generated-3d-compute-morph-skin-package-smoke-d3d12-v1.md`
- `2026-05-06-installed-sdk-release-metadata-validation-v1.md`
- `2026-05-06-production-1-0-readiness-audit-v1.md`
- `2026-05-06-rhi-d3d12-calibrated-queue-timing-diagnostics-v1.md`
- `2026-05-06-rhi-d3d12-per-queue-fence-synchronization-v1.md`
- `2026-05-06-rhi-d3d12-queue-clock-calibration-foundation-v1.md`
- `2026-05-06-rhi-d3d12-queue-timestamp-measurement-foundation-v1.md`
- `2026-05-06-rhi-d3d12-submitted-command-calibrated-timing-scopes-v1.md`
- `2026-05-06-rhi-vulkan-compute-dispatch-foundation-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-async-telemetry-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-queue-synchronization-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-renderer-consumption-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-renderer-consumption-vulkan-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-skin-composition-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-vulkan-proof-v1.md`
- `2026-05-06-runtime-scene-quaternion-animation-transform-binding-v1.md`
- `2026-05-06-runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1.md`
- `2026-05-06-runtime-ui-font-image-adapter-v1.md`
- `2026-05-07-generated-3d-compute-morph-skin-package-smoke-vulkan-v1.md`
- `Active gap burn-down`
- `physics-1-0-collision-system-closeout-v1`
- `physics-benchmark-determinism-gates-v1`
- `physics-jolt-adapter-gate-v1`
- `-HostGateAcknowledgements`
- `2026-05-07-ci-matrix-contract-check-v1.md`
- `2026-05-07-editor-ai-command-diagnostics-panel-v1.md`
- `2026-05-07-editor-ai-evidence-import-review-v1.md`
- `2026-05-07-editor-ai-host-gated-validation-execution-ack-v1.md`
- `2026-05-07-editor-ai-reviewed-validation-batch-execution-v1.md`
- `2026-05-07-editor-ai-reviewed-validation-execution-v1.md`
- `2026-05-07-editor-content-browser-import-codec-adapter-review-v1.md`
- `2026-05-07-editor-content-browser-import-diagnostics-v1.md`
- `2026-05-07-editor-content-browser-import-external-copy-review-v1.md`
- `2026-05-07-editor-content-browser-import-native-dialog-v1.md`
- `2026-05-07-editor-input-rebinding-profile-panel-v1.md`
- `2026-05-07-editor-material-asset-preview-diagnostics-v1.md`
- `2026-05-07-editor-material-gpu-preview-execution-evidence-v1.md`
- `2026-05-07-editor-prefab-variant-batch-resolution-review-v1.md`
- `2026-05-07-editor-prefab-variant-conflict-review-v1.md`
- `2026-05-07-editor-prefab-variant-missing-node-cleanup-v1.md`
- `2026-05-07-editor-prefab-variant-native-dialog-v1.md`
- `2026-05-07-editor-prefab-variant-node-retarget-review-v1.md`
- `2026-05-07-editor-prefab-variant-reviewed-resolution-v1.md`
- `2026-05-07-editor-prefab-variant-source-mismatch-accept-current-review-v1.md`
- `2026-05-07-editor-prefab-variant-source-mismatch-retarget-review-v1.md`
- `2026-05-07-editor-profiler-native-trace-open-dialog-v1.md`
- `2026-05-07-editor-profiler-native-trace-save-dialog-v1.md`
- `2026-05-07-editor-profiler-telemetry-handoff-v1.md`
- `2026-05-07-editor-profiler-trace-export-v1.md`
- `2026-05-07-editor-profiler-trace-file-import-review-v1.md`
- `2026-05-07-editor-profiler-trace-file-save-v1.md`
- `2026-05-07-editor-profiler-trace-import-reconstruction-v1.md`
- `2026-05-07-editor-profiler-trace-import-review-v1.md`
- `2026-05-07-editor-project-native-dialog-v1.md`
- `2026-05-07-editor-resource-capture-execution-evidence-v1.md`
- `2026-05-07-editor-resource-capture-request-v1.md`
- `2026-05-07-editor-runtime-host-playtest-launch-v1.md`
- `2026-05-07-editor-scene-native-dialog-v1.md`
- `2026-05-07-runtime-ui-accessibility-publish-plan-v1.md`
- `2026-05-07-runtime-ui-font-rasterization-request-plan-v1.md`
- `2026-05-07-runtime-ui-ime-composition-publish-plan-v1.md`
- `2026-05-08-cpp23-release-package-artifact-ci-evidence-v1.md`
- `2026-05-08-editor-input-rebinding-action-capture-panel-v1.md`
- `2026-05-08-runtime-input-rebinding-capture-contract-v1.md`
- `2026-05-08-runtime-input-rebinding-focus-consumption-v1.md`
- `2026-05-08-runtime-input-rebinding-presentation-rows-v1.md`
- `2026-05-08-runtime-ui-decoded-image-atlas-package-bridge-v1.md`
- `2026-05-08-runtime-ui-glyph-atlas-package-bridge-v1.md`
- `2026-05-08-runtime-ui-image-decode-request-plan-v1.md`
- `2026-05-08-runtime-ui-platform-text-input-session-plan-v1.md`
- `2026-05-08-runtime-ui-png-image-decoding-adapter-v1.md`
- `2026-05-08-runtime-ui-text-shaping-request-plan-v1.md`
- `2026-05-09-editor-nested-prefab-refresh-resolution-v1.md`
- `2026-05-09-editor-prefab-instance-local-child-refresh-resolution-v1.md`
- `2026-05-09-editor-prefab-instance-stale-node-refresh-resolution-v1.md`
- `2026-05-09-editor-prefab-variant-base-refresh-merge-review-v1.md`
- `2026-05-09-editor-scene-prefab-instance-refresh-review-v1.md`
- `2026-05-10-editor-input-rebinding-axis-capture-gamepad-v1.md`
- `2026-05-12-editor-content-browser-source-registry-population-v1.md`
- `2026-05-12-editor-source-registry-visible-content-browser-v1.md`
- `2D Sprite Batch Package Telemetry v1`
- `2D Sprite Batch Planning Contract v1`
- `3D Scene Mesh Package Telemetry v1`
- `Accept current node`
- `AccessibilityPublishResult`
- `Active slice`
- `Apply All Reviewed`
- `Assert-ReleasePackageArtifacts`
- `Browse Import Sources`
- `Browse Load Variant`
- `Browse Open Scene`
- `Browse Save Scene As`
- `Browse Save Trace JSON`
- `Browse Save Variant`
- `Browse Trace JSON`
- `C++23 Release Package Artifact CI Evidence v1`
- `CI Matrix Contract Check v1`
- `Copy External Sources`
- `Copy Trace JSON`
- `DiagnosticsTraceImportResult`
- `DiagnosticsTraceImportReview`
- `Editor AI Command Diagnostics Panel v1`
- `Editor AI Command Diagnostics Panel v1 retained evidence: unacknowledged or automatic host-gated AI command execution; raw manifest command evaluation.`
- `Editor AI Evidence Import Review v1`
- `Editor AI Reviewed Validation Batch Execution v1`
- `Editor AI Reviewed Validation Execution v1`
- `Editor Content Browser Import Codec Adapter Review v1`
- `Editor Content Browser Import Diagnostics v1`
- `Editor Content Browser Import External Copy Review v1`
- `Editor Content Browser Import Native Dialog v1`
- `Editor Input Rebinding Action Capture Panel v1`
- `Editor Input Rebinding Axis Capture Gamepad v1`
- `Editor Input Rebinding Profile Panel v1`
- `Editor Material Asset Preview Diagnostics v1`
- `Editor Material GPU Preview Execution Evidence v1`
- `Editor Nested Prefab Refresh Resolution v1`
- `Editor Prefab Instance Local Child Refresh Resolution v1`
- `Editor Prefab Instance Stale Node Refresh Resolution v1`
- `Editor Prefab Variant Base Refresh Merge Review v1`
- `Editor Prefab Variant Batch Resolution Review v1`
- `Editor Prefab Variant Conflict Review v1`
- `Editor Prefab Variant Missing Node Cleanup v1`
- `Editor Prefab Variant Native Dialog v1`
- `Editor Prefab Variant Node Retarget Review v1`
- `Editor Prefab Variant Reviewed Resolution v1`
- `Editor Prefab Variant Source Mismatch Accept Current Review v1`
- `Editor Prefab Variant Source Mismatch Retarget Review v1`
- `Editor Profiler Native Trace Open Dialog v1`
- `Editor Profiler Native Trace Save Dialog v1`
- `Editor Profiler Telemetry Handoff v1`
- `Editor Profiler Trace Export v1`
- `Editor Profiler Trace File Import Review v1`
- `Editor Profiler Trace File Save v1`
- `Editor Profiler Trace Import Reconstruction v1`
- `Editor Profiler Trace Import Review v1`
- `Editor Project Native Dialog v1`
- `Editor Resource Capture Execution Evidence v1`
- `Editor Resource Capture Request v1`
- `Editor Resource Capture Request v1 retained evidence: reviewed Resources capture request handoff rows; resource management/capture execution beyond host-owned evidence rows.`
- `Editor Runtime Host Playtest Launch v1`
- `Editor Scene Native Dialog v1`
- `Editor Scene Prefab Instance Refresh Review v1`
- `Editor profiler trace export contract retained evidence: Profiler trace JSON copy export; project-relative Profiler trace JSON file save; native Profiler Trace JSON save dialog; read-only Profiler telemetry handoff rows; pasted Profiler Trace JSON review; project-relative Profiler Trace JSON file import review/reconstruction; native Profiler Trace JSON open dialog; broader editor native save/open dialogs outside Profiler; arbitrary trace conversion beyond first-party exported Trace Event JSON reconstruction; telemetry SDK/upload/backend follow-ups.`
- `Editor resource capture execution evidence contract retained evidence: host-owned Resources capture execution evidence rows.`
- `EditorAiCommandPanelModel`
- `EditorAiPlaytestEvidenceImportModel`
- `EditorAiReviewedValidationExecutionBatchModel`
- `EditorAiReviewedValidationExecutionModel`
- `EditorContentBrowserImportOpenDialogModel`
- `EditorContentBrowserImportPanelModel`
- `EditorInputRebindingCaptureModel`
- `EditorInputRebindingProfilePanelModel`
- `EditorMaterialAssetPreviewPanelModel`
- `EditorMaterialGpuPreviewExecutionSnapshot`
- `EditorPrefabVariantFileDialogModel`
- `EditorProfilerTelemetryHandoffModel`
- `EditorProfilerTraceExportModel`
- `EditorProfilerTraceFileImportRequest`
- `EditorProfilerTraceFileSaveRequest`
- `EditorProfilerTraceOpenDialogModel`
- `EditorProfilerTraceSaveDialogModel`
- `EditorProjectFileDialogMode`
- `EditorProjectFileDialogModel`
- `EditorRuntimeHostPlaytestLaunchModel`
- `EditorSceneFileDialogModel`
- `Execute Ready`
- `ExternalAssetImportAdapters`
- `FontRasterizationResult`
- `GameEngine.CookedTexture.v1`
- `Git history for deleted evidence`
- `Host-Gated Validation Execution Ack v1`
- `IAccessibilityAdapter`
- `IFontRasterizerAdapter`
- `IImageDecodingAdapter`
- `IImeAdapter`
- `IPlatformIntegrationAdapter`
- `ITextShapingAdapter`
- `ImageDecodeDispatchResult`
- `ImeCompositionPublishResult`
- `Import Trace JSON`
- `Open Project...`
- `Open Scene...`
- `PackedUiAtlasAuthoringDesc`
- `PackedUiGlyphAtlasAuthoringDesc`
- `PlatformTextInputSessionResult`
- `PngImageDecodingAdapter`
- `PrefabNodeOverride::source_node_name`
- `PrefabVariantBaseRefreshPlan`
- `PrefabVariantConflictBatchResolutionPlan`
- `PrefabVariantConflictReviewModel`
- `Profiler Telemetry Handoff`
- `Profiler trace JSON copy export`
- `Refresh Prefab Instance`
- `Reload Source Registry`
- `Retarget override to node`
- `Review Trace JSON`
- `Runtime Input Rebinding Capture Contract v1`
- `Runtime Input Rebinding Focus Consumption v1`
- `Runtime Input Rebinding Presentation Rows v1`
- `Runtime Resource v2 1.0 Scope Closeout v1`
- `Runtime UI Accessibility Publish Plan v1`
- `Runtime UI Decoded Image Atlas Package Bridge v1`
- `Runtime UI Font Rasterization Request Plan v1`
- `Runtime UI Glyph Atlas Package Bridge v1`
- `Runtime UI IME Composition Publish Plan v1`
- `Runtime UI Image Decode Request Plan v1`
- `Runtime UI PNG Image Decoding Adapter v1`
- `Runtime UI Platform Text Input Session Plan v1`
- `Runtime UI Text Shaping Request Plan v1`
- `Runtime UI and input ledger note: RuntimeInputRebindingPresentationModel; platform input glyph generation.`
- `RuntimeInputRebindingCaptureRequest`
- `RuntimeInputRebindingFocusCaptureRequest`
- `RuntimeInputRebindingPresentationModel`
- `RuntimeInputRebindingPresentationToken`
- `Save Project As...`
- `Save Scene As...`
- `Save Trace JSON`
- `SceneAuthoringDocument::set_scene_path`
- `TextShapingResult`
- `Vulkan display parity`
- `Vulkan/Metal material-preview display parity`
- `Win32ProcessRunner`
- `accept-current`
- `accept_current_node`
- `ai_commands.execution`
- `ai_commands.execution.batch`
- `ai_evidence_import`
- `apply_prefab_variant_base_refresh`
- `arbitrary trace conversion beyond first-party exported Trace Event JSON reconstruction`
- `asset-importers`
- `author_packed_ui_atlas_from_decoded_images`
- `author_packed_ui_glyph_atlas_from_rasterized_glyphs`
- `automatic host-gated AI command execution`
- `automatic merge/rebase/resolution UX`
- `broad importer readiness`
- `broader editor native save/open dialogs outside Profiler`
- `build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas`
- `capture_runtime_input_rebinding_action`
- `capture_runtime_input_rebinding_action_with_focus`
- `capture_runtime_input_rebinding_axis`
- `content_browser_import.external_copy`
- `content_browser_import.open_dialog`
- `decode_audited_png_rgba8`
- `deserialize_prefab_variant_definition_for_review`
- `dynamic game-module loading`
- `editor-ai-host-gated-validation-execution-ack-v1`
- `editor-ai-reviewed-validation-execution-v1`
- `editor-content-browser-and-import-diagnostics-v1`
- `editor-content-browser-import-codec-adapter-review-v1`
- `editor-input-rebinding-action-capture-panel-v1`
- `editor-input-rebinding-profile-panel-v1`
- `editor-material-asset-preview-diagnostics-v1`
- `editor-material-gpu-preview-execution-evidence-v1`
- `embedded-base refresh`
- `external importer claims`
- `full cross-platform package matrix readiness`
- `gameplay_input_consumed`
- `host-gate-free reviewed`
- `host-owned Resources capture execution evidence rows`
- `hot-reload summaries`
- `import_diagnostics_trace_json`
- `import_editor_profiler_trace_json`
- `in-memory profile`
- `input_rebinding`
- `keep_local_child`
- `keep_nested_prefab_instance`
- `keep_stale_source_node_as_local`
- `load_project_bundle`
- `make_content_browser_import_open_dialog_ui_model`
- `make_editor_ai_reviewed_validation_execution_batch`
- `make_editor_profiler_trace_open_dialog_model`
- `make_editor_profiler_trace_open_dialog_request`
- `make_editor_profiler_trace_save_dialog_model`
- `make_editor_profiler_trace_save_dialog_request`
- `make_prefab_variant_base_refresh_ui_model`
- `make_prefab_variant_conflict_batch_resolution_action`
- `make_prefab_variant_conflict_resolution_action`
- `make_prefab_variant_open_dialog_request`
- `make_prefab_variant_save_dialog_model`
- `make_prefab_variant_save_dialog_request`
- `make_project_file_dialog_ui_model`
- `make_project_open_dialog_model`
- `make_project_open_dialog_request`
- `make_project_save_dialog_model`
- `make_project_save_dialog_request`
- `make_runtime_input_rebinding_presentation`
- `make_scene_file_dialog_ui_model`
- `make_scene_open_dialog_model`
- `make_scene_open_dialog_request`
- `make_scene_save_dialog_model`
- `make_scene_save_dialog_request`
- `native IME/text-input sessions`
- `native Profiler Trace JSON open dialog`
- `native Profiler Trace JSON save dialog`
- `native accessibility objects`
- `next-production-gap-selection`
- `outside Profiler, Scene, Prefab Variant, and Project`
- `outside Profiler, Scene, and Prefab Variant`
- `pasted Profiler Trace JSON review`
- `plan_prefab_variant_base_refresh`
- `platform input glyph generation`
- `play_in_editor.runtime_host`
- `prefab_variant_base_refresh`
- `prefab_variant_conflicts`
- `prefab_variant_conflicts.batch_resolution`
- `prefab_variant_file_dialog.open`
- `prefab_variant_file_dialog.save`
- `production sprite batching`
- `profiler.telemetry`
- `profiler.trace_export`
- `profiler.trace_file_import`
- `profiler.trace_file_save`
- `profiler.trace_import`
- `profiler.trace_import.reconstructed_*`
- `profiler.trace_open_dialog`
- `profiler.trace_save_dialog`
- `project-relative Profiler Trace JSON file import review/reconstruction`
- `project-relative Profiler trace JSON file save`
- `project_file_dialog.open`
- `project_file_dialog.save`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1`
- `raw manifest command evaluation`
- `read-only Profiler telemetry handoff rows`
- `registered asset watch-tick orchestration`
- `remaining plan evidence that is still referenced`
- `renderer texture upload`
- `renderer-rhi-resource-foundation`
- `resolve_prefab_variant_conflict`
- `resolve_prefab_variant_conflicts`
- `resource management/capture execution beyond host-owned evidence rows`
- `resources.capture_execution`
- `resources.capture_requests`
- `reviewed Content Browser import selections`
- `reviewed Resources capture request handoff rows`
- `reviewed editor action capture lane`
- `save_project_bundle`
- `scene_file_dialog.open`
- `scene_file_dialog.save`
- `scene_prefab_instance_refresh`
- `selected linked prefab root`
- `source codecs`
- `source image codecs`
- `source-node hint uniquely matches`
- `source-node-mismatch rows`
- `source_node_mismatch`
- `telemetry SDK/upload/backend follow-ups`
- `third-party adapters unsupported`
- `tools/check-ci-matrix.ps1`
- `unacknowledged or automatic host-gated AI command execution`
- `virtual keyboard behavior`
- `workspace ``ai_commands`` panel state`

## Retired dated plan static evidence (2026-05-22 cleanup)

The static guards use this section as the single historical sink for retired dated implementation plans. These literals preserve the evidence that used to require keeping individual completed plan files in `docs/superpowers/plans/`; the detailed prose remains available through Git history.

### 2026-05-02-ai-operable-production-loop-clean-uplift-v1.md
- 2026-05-02-ai-operable-production-loop-clean-uplift-v1.md
- AI-Operable Production Loop Clean Uplift v1 Implementation Plan (2026-05-02)
- Phase 2 decision: retire the separate remediation evidence-review and closeout-report candidates
- Phase 3 consolidation: package diagnostics -> validation recipe preflight -> readiness -> operator handoff -> evidence summary -> remediation queue -> remediation handoff
- closeout through existing evidence summary

### 2026-05-02-editor-ai-playtest-remediation-closeout-report-v1.md
- 2026-05-02-editor-ai-playtest-remediation-closeout-report-v1.md
- Editor AI Playtest Remediation Closeout Report v1 Implementation Plan (2026-05-02)
- Status note (2026-05-02): Retired by
- no separate remediation closeout-report row model

### 2026-05-02-editor-ai-playtest-remediation-evidence-review-v1.md
- 2026-05-02-editor-ai-playtest-remediation-evidence-review-v1.md
- Editor AI Playtest Remediation Evidence Review v1 Implementation Plan (2026-05-02)
- Status note (2026-05-02): Retired by
- no separate remediation evidence-review row model

### 2026-05-06-android-release-device-matrix-v1.md
- **Status:** Completed on 2026-05-06
- 2026-05-06-android-release-device-matrix-v1.md
- Android Release Device Matrix v1 Implementation Plan (2026-05-06)
- Phase 1: Overlay Row Contract
- RED tests describe overlay row planning
- RuntimeGameplayDebugOverlayPlan
- plan_runtime_gameplay_debug_overlay

### 2026-05-06-apple-metal-ios-host-evidence-v1.md
- **Status:** Completed
- **Status:** Completed on 2026-05-06
- 2026-05-06-apple-metal-ios-host-evidence-v1.md
- Apple Metal iOS Host Evidence v1 Implementation Plan (2026-05-06)
- Crash Telemetry Trace Ops v1
- build_diagnostics_ops_plan
- without adding native dump writing

### 2026-05-06-crash-telemetry-trace-ops-v1.md
- **Status:** Completed
- **Status:** Completed on 2026-05-06
- 2026-05-06-crash-telemetry-trace-ops-v1.md
- Crash Telemetry Trace Ops v1 Implementation Plan (2026-05-06)
- IImageDecodingAdapter
- ImageDecodePixelFormat::rgba8_unorm
- MK_tools_tests
- PngImageDecodingAdapter
- Runtime UI PNG Image Decoding Adapter v1
- decode_audited_png_rgba8
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1

### 2026-05-06-desktop-release-package-evidence-v1.md
- **Status:** Completed on 2026-05-06
- 2026-05-06-desktop-release-package-evidence-v1.md
- Desktop Release Package Evidence v1 Implementation Plan (2026-05-06)

### 2026-05-06-generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1.md
- **Plan ID:** `generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1`
- **Status:** Completed
- 2026-05-06-generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1.md
- Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1 (2026-05-06)
- Runtime RHI Compute Morph Async Telemetry D3D12 v1
- last_graphics_queue_wait_fence_value
- without claiming performance overlap

### 2026-05-06-generated-3d-compute-morph-package-smoke-vulkan-v1.md
- **Plan ID:** `generated-3d-compute-morph-package-smoke-vulkan-v1`
- **Status:** Completed
- 2026-05-06-generated-3d-compute-morph-package-smoke-vulkan-v1.md
- Do not claim generated-package Vulkan compute morph readiness
- Generated 3D Compute Morph Package Smoke Vulkan v1 Implementation Plan (2026-05-06)
- MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV
- Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1
- RuntimeMorphMeshComputeBindingOptions::output_normal_usage
- descriptor bindings `4..7`
- output_tangent_usage
- vulkan_compute_morph_tangent_frame.cs.spv

### 2026-05-06-generated-3d-compute-morph-skin-package-smoke-d3d12-v1.md
- **Plan ID:** `generated-3d-compute-morph-skin-package-smoke-d3d12-v1`
- **Status:** Completed
- 2026-05-06-generated-3d-compute-morph-skin-package-smoke-d3d12-v1.md
- Generated 3D Compute Morph Skin Package Smoke D3D12 v1 (2026-05-06)
- Runtime Scene RHI Compute Morph Skin Palette D3D12 v1
- RuntimeSceneComputeMorphSkinnedMeshBinding
- SceneSkinnedGpuBindingPalette
- compute_morph_output_position_bytes
- compute_morph_skinned_mesh_bindings

### 2026-05-06-installed-sdk-release-metadata-validation-v1.md
- **Status:** Completed on 2026-05-06
- 2026-05-06-installed-sdk-release-metadata-validation-v1.md
- Installed SDK Release Metadata Validation v1 Implementation Plan (2026-05-06)

### 2026-05-06-production-1-0-readiness-audit-v1.md
- **Status:** Completed on 2026-05-06
- 2026-05-06-production-1-0-readiness-audit-v1.md
- Invoke-MobilePackagingProbe
- Kill($true)
- OutputWaitMilliseconds
- Production 1.0 Readiness Audit v1 Implementation Plan (2026-05-06)
- apple-host-helpers.ps1
- probe-timeout

### 2026-05-06-rhi-d3d12-calibrated-queue-timing-diagnostics-v1.md
- **Plan ID:** `rhi-d3d12-calibrated-queue-timing-diagnostics-v1`
- **Status:** Completed
- 2026-05-06-rhi-d3d12-calibrated-queue-timing-diagnostics-v1.md
- CPU QPC sample
- GetClockCalibration
- QueueClockCalibration
- RHI D3D12 Calibrated Queue Timing Diagnostics v1 (2026-05-06)
- RHI D3D12 Queue Clock Calibration Foundation v1

### 2026-05-06-rhi-d3d12-per-queue-fence-synchronization-v1.md
- **Plan ID:** `rhi-d3d12-per-queue-fence-synchronization-v1`
- **Status:** Completed
- 2026-05-06-rhi-d3d12-per-queue-fence-synchronization-v1.md
- Do not update generated
- RHI D3D12 Per-Queue Fence Synchronization v1 (2026-05-06)
- RhiPipelinedComputeGraphicsScheduleEvidence
- Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1
- different current slot
- previously completed output slot

### 2026-05-06-rhi-d3d12-queue-clock-calibration-foundation-v1.md
- **Plan ID:** `rhi-d3d12-queue-clock-calibration-foundation-v1`
- **Status:** Completed
- 2026-05-06-rhi-d3d12-queue-clock-calibration-foundation-v1.md
- Do not expose `ID3D12QueryHeap`
- Microsoft Learn: Timing
- RHI D3D12 Queue Clock Calibration Foundation v1 (2026-05-06)
- RHI D3D12 Queue Timestamp Measurement Foundation v1
- ResolveQueryData
- timestamp query heap recording

### 2026-05-06-rhi-d3d12-queue-timestamp-measurement-foundation-v1.md
- **Plan ID:** `rhi-d3d12-queue-timestamp-measurement-foundation-v1`
- **Status:** Completed
- 2026-05-06-rhi-d3d12-queue-timestamp-measurement-foundation-v1.md
- Do not expose native fences
- FenceValue
- RHI D3D12 Per-Queue Fence Synchronization v1
- RHI D3D12 Queue Timestamp Measurement Foundation v1 (2026-05-06)
- per-queue fences

### 2026-05-06-rhi-d3d12-submitted-command-calibrated-timing-scopes-v1.md
- **Plan ID:** `rhi-d3d12-submitted-command-calibrated-timing-scopes-v1`
- **Status:** Completed
- 2026-05-06-rhi-d3d12-submitted-command-calibrated-timing-scopes-v1.md
- QueryPerformanceCounter
- RHI D3D12 Submitted Command Calibrated Timing Scopes v1 (2026-05-06)
- RhiPipelinedComputeGraphicsScheduleEvidence
- Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1
- calibrated overlap diagnostic

### 2026-05-06-runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1.md
- **Plan ID:** `runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1`
- **Status:** Completed
- --require-compute-morph-async-telemetry
- 2026-05-06-runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1.md
- Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1
- Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1 (2026-05-06)
- without exposing queue/fence handles

### 2026-05-06-runtime-rhi-compute-morph-async-telemetry-d3d12-v1.md
- **Plan ID:** `runtime-rhi-compute-morph-async-telemetry-d3d12-v1`
- **Status:** Completed
- 2026-05-06-runtime-rhi-compute-morph-async-telemetry-d3d12-v1.md
- DesktopRuntime3DPackage
- Generated 3D Compute Morph Skin Package Smoke D3D12 v1
- Runtime RHI Compute Morph Async Telemetry D3D12 v1 (2026-05-06)
- without exposing RHI or native handles

### 2026-05-06-runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1.md
- **Plan ID:** `runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1`
- **Status:** Completed
- 2026-05-06-runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1.md
- QueueCalibratedTiming
- RHI D3D12 Calibrated Queue Timing Diagnostics v1
- Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1 (2026-05-06)
- calibrated interval rows

### 2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1.md
- **Plan ID:** `runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1`
- **Status:** Completed
- 2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1.md
- Do not claim generated-package Vulkan compute morph readiness
- MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV
- MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV
- RhiFrameRenderer
- Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1 Implementation Plan (2026-05-06)
- Runtime RHI Compute Morph Renderer Consumption Vulkan v1
- make_runtime_compute_morph_output_mesh_gpu_binding

### 2026-05-06-runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1.md
- **Plan ID:** `runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1`
- **Status:** Completed
- 2026-05-06-runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1.md
- Do not update generated package validation to require overlap
- RhiAsyncOverlapReadinessDiagnostics
- Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1
- Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1 (2026-05-06)
- not_proven_serial_dependency
- official Microsoft D3D12
- runtime/RHI-only D3D12 evidence

### 2026-05-06-runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1.md
- **Plan ID:** `runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1`
- **Status:** Completed
- 2026-05-06-runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1.md
- DesktopRuntime3DPackage
- Do not update generated
- Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1
- Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1 (2026-05-06)
- RuntimeMorphMeshComputeOutputSlot
- multi-slot compute morph output-ring
- not_proven_serial_dependency
- output_slot_count

### 2026-05-06-runtime-rhi-compute-morph-renderer-consumption-vulkan-v1.md
- **Plan ID:** `runtime-rhi-compute-morph-renderer-consumption-vulkan-v1`
- **Status:** Completed
- 2026-05-06-runtime-rhi-compute-morph-renderer-consumption-vulkan-v1.md
- Do not claim generated-package Vulkan compute morph readiness
- MK_VULKAN_TEST_COMPUTE_MORPH_SPV
- Runtime RHI Compute Morph Renderer Consumption Vulkan v1 Implementation Plan (2026-05-06)
- Runtime RHI Compute Morph Vulkan Proof v1
- RuntimeMorphMeshComputeBinding
- create_runtime_morph_mesh_compute_binding

### 2026-05-06-runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1.md
- **Plan ID:** `runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1`
- **Status:** Completed
- 2026-05-06-runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1.md
- RHI D3D12 Submitted Command Calibrated Timing Scopes v1
- ResolveQueryData
- Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1 (2026-05-06)
- SubmittedCommandCalibratedTiming
- actual submitted D3D12 graphics/compute command lists

### 2026-05-06-runtime-rhi-compute-morph-vulkan-proof-v1.md
- **Plan ID:** `runtime-rhi-compute-morph-vulkan-proof-v1`
- **Status:** Completed
- 2026-05-06-runtime-rhi-compute-morph-vulkan-proof-v1.md
- Do not claim Vulkan compute morph parity
- MK_VULKAN_TEST_COMPUTE_SPV
- RHI Vulkan Compute Dispatch Foundation v1
- Runtime RHI Compute Morph Vulkan Proof v1 Implementation Plan (2026-05-06)
- vkCmdDispatch
- vkCreateComputePipelines

### 2026-05-06-runtime-scene-quaternion-animation-transform-binding-v1.md
- **Plan ID:** `runtime-scene-quaternion-animation-transform-binding-v1`
- **Status:** Completed
- 2026-05-06-runtime-scene-quaternion-animation-transform-binding-v1.md
- Runtime Scene Quaternion Animation Transform Binding v1 (2026-05-06)

### 2026-05-06-runtime-ui-font-image-adapter-v1.md
- **Status:** Completed
- 2026-05-06-runtime-ui-font-image-adapter-v1.md
- Runtime UI Font Image Adapter v1 Implementation Plan (2026-05-06)

### 2026-05-07-ci-matrix-contract-check-v1.md
- 2026-05-07-ci-matrix-contract-check-v1.md
- CI Matrix Contract Check v1 Implementation Plan
- CI Matrix Contract Check v1 Implementation Plan (2026-05-07)
- Windows/Linux/sanitizer/macOS/iOS
- full package/build matrix readiness
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1

### 2026-05-07-generated-3d-compute-morph-skin-package-smoke-vulkan-v1.md
- **Status:** Completed
- **Status:** Completed.
- --require-compute-morph
- 2026-05-07-generated-3d-compute-morph-skin-package-smoke-vulkan-v1.md
- DesktopRuntime3DPackage
- Do not claim Vulkan NORMAL/TANGENT package smoke
- Generated 3D Compute Morph Package Smoke Vulkan v1
- Generated 3D Compute Morph Skin Package Smoke Vulkan v1 Implementation Plan (2026-05-07)
- SdlDesktopPresentationVulkanSceneRendererDesc

### 2026-05-07-runtime-ui-accessibility-publish-plan-v1.md
- **Status:** Completed
- 2026-05-07-runtime-ui-accessibility-publish-plan-v1.md
- AccessibilityPublishPlan
- AccessibilityPublishResult
- IAccessibilityAdapter
- MK_ui_renderer_tests
- OS accessibility adapter
- Runtime UI Accessibility Publish Plan v1
- Runtime UI Accessibility Publish Plan v1 Implementation Plan (2026-05-07)
- plan_accessibility_publish
- publish_accessibility_payload

### 2026-05-07-runtime-ui-font-rasterization-request-plan-v1.md
- **Status:** Completed
- 2026-05-07-runtime-ui-font-rasterization-request-plan-v1.md
- FontRasterizationRequestPlan
- FontRasterizationResult
- IFontRasterizerAdapter
- MK_ui_renderer_tests
- Runtime UI Font Image Adapter v1
- Runtime UI Font Rasterization Request Plan v1
- Runtime UI Font Rasterization Request Plan v1 Implementation Plan (2026-05-07)
- UiRendererGlyphAtlasPalette
- invalid_font_allocation
- make_ui_text_glyph_sprite_command
- plan_font_rasterization_request
- rasterize_font_glyph
- text_glyph_sprites_submitted
- without claiming font loading/rasterization

### 2026-05-07-runtime-ui-ime-composition-publish-plan-v1.md
- **Status:** Completed
- 2026-05-07-runtime-ui-ime-composition-publish-plan-v1.md
- IImeAdapter
- ImeCompositionPublishPlan
- ImeCompositionPublishResult
- MK_ui_renderer_tests
- Runtime UI IME Composition Publish Plan v1
- Runtime UI IME Composition Publish Plan v1 Implementation Plan (2026-05-07)
- Win32/TSF
- plan_ime_composition_update
- publish_ime_composition

### 2026-05-08-cpp23-release-package-artifact-ci-evidence-v1.md
- **Status:** Completed on 2026-05-08.
- .zip.sha256
- 2026-05-08-cpp23-release-package-artifact-ci-evidence-v1.md
- Assert-ReleasePackageArtifacts
- C++23 Release Package Artifact CI Evidence v1 Implementation Plan
- C++23 Release Package Artifact CI Evidence v1 Implementation Plan (2026-05-08)
- full cross-platform package matrix readiness
- tools/evaluate-cpp23.ps1 -Release

### 2026-05-08-frame-graph-rhi-texture-schedule-execution-v1.md
- **Plan ID:** `frame-graph-rhi-texture-schedule-execution-v1`
- **Status:** Completed
- **Status:** Completed.
- 2026-05-08-frame-graph-rhi-texture-schedule-execution-v1.md
- Frame Graph RHI Texture Schedule Execution v1 Implementation Plan (2026-05-08)
- Runtime RHI Upload Submission Fence Rows v1
- last_submitted_upload_fence.value != 0
- native async upload execution
- runtime scene rhi upload execution preserves submitted fences in submit order across queues
- submitted_upload_fence_count
- submitted_upload_fence_count == 3
- submitted_upload_fence_count == 4
- submitted_upload_fences
- submitted_upload_fences[0].queue == mirakana::rhi::QueueKind::compute
- submitted_upload_fences[1].value == compute_resource.base_position_upload.submitted_fence.value

### 2026-05-08-full-repository-static-analysis-ci-contract-v1.md
- **Plan ID:** `full-repository-static-analysis-ci-contract-v1`
- **Status:** Completed.
- 2026-05-08-full-repository-static-analysis-ci-contract-v1.md
- Full Repository Static Analysis CI Contract v1 Implementation Plan
- Full Repository Static Analysis CI Contract v1 Implementation Plan (2026-05-08)
- tools/check-tidy.ps1 -Strict
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
- static-analysis
- static-analysis-tidy-logs

### 2026-05-08-rhi-upload-stale-generation-diagnostics-v1.md
- **Plan ID:** `rhi-upload-stale-generation-diagnostics-v1`
- **Status:** Completed.
- 2026-05-08-rhi-upload-stale-generation-diagnostics-v1.md
- FrameGraphRhiTextureExecutionDesc
- FrameGraphRhiTextureExecutionResult
- RHI Upload Stale Generation Diagnostics v1 Implementation Plan (2026-05-08)
- execute_frame_graph_rhi_texture_schedule

### 2026-05-08-runtime-rhi-upload-submission-fence-rows-v1.md
- **Status:** Completed
- 2026-05-08-runtime-rhi-upload-submission-fence-rows-v1.md
- Runtime RHI Upload Submission Fence Rows v1 Implementation Plan (2026-05-08)
- base_upload.submitted_fence
- binding.frame_graph_barriers_recorded == 0
- binding.frame_graph_command_lists_submitted == 1
- binding.frame_graph_pass_callbacks_invoked == 1
- binding.frame_graph_queue_waits_recorded == 0
- binding.submitted_fence
- binding.submitted_fence.value != 0
- bindings.submitted_upload_fences
- device.stats().fence_waits == 0
- device.stats().last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::copy
- evidence.graphics_waited_for_copy
- evidence.package_transactions == 4
- evidence.resource_updates_ready
- evidence.ring_backed_uploads == 4
- record_submitted_upload_fence
- result.submitted_fence.value != 0
- result.submitted_upload_fences.push_back
- runtime package resource update readiness publishes rows after upload fences are graphics ready
- runtime package streaming mesh upload transaction waits graphics queue for async copy upload
- runtime package upload staging evidence uses pooled async ring for selected package transactions
- runtime rhi upload reports submitted fence without forcing wait
- transaction.upload_queue_waits_recorded == 1
- upload.submitted_fence
- upload.submitted_fence.value != 0

### 2026-05-08-runtime-ui-decoded-image-atlas-package-bridge-v1.md
- **Status:** Completed
- **Status:** Completed.
- 2026-05-08-runtime-ui-decoded-image-atlas-package-bridge-v1.md
- GameEngine.CookedTexture.v1
- GameEngine.UiAtlas.v1
- MK_tools_tests
- PackedUiAtlasAuthoringDesc
- Runtime UI Decoded Image Atlas Package Bridge v1
- Runtime UI Decoded Image Atlas Package Bridge v1 Implementation Plan (2026-05-08)
- apply_packed_ui_atlas_package_update
- author_packed_ui_atlas_from_decoded_images
- pack_sprite_atlas_rgba8_max_side
- plan_packed_ui_atlas_package_update

### 2026-05-08-runtime-ui-glyph-atlas-package-bridge-v1.md
- **Status:** Completed
- 2026-05-08-runtime-ui-glyph-atlas-package-bridge-v1.md
- GameEngine.CookedTexture.v1
- GameEngine.UiAtlas.v1
- MK_tools_tests
- PackedUiGlyphAtlasAuthoringDesc
- Runtime UI Glyph Atlas Package Bridge v1
- Runtime UI Glyph Atlas Package Bridge v1 Implementation Plan (2026-05-08)
- RuntimeUiAtlasGlyph
- UiAtlasMetadataGlyph
- apply_packed_ui_glyph_atlas_package_update
- author_packed_ui_glyph_atlas_from_rasterized_glyphs
- build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas
- plan_packed_ui_glyph_atlas_package_update

### 2026-05-08-runtime-ui-image-decode-request-plan-v1.md
- **Status:** Completed
- 2026-05-08-runtime-ui-image-decode-request-plan-v1.md
- IImageDecodingAdapter
- IPlatformIntegrationAdapter
- ImageDecodeDispatchResult
- ImageDecodePixelFormat
- ImageDecodeRequestPlan
- MK_ui_renderer_tests
- PlatformTextInputEndPlan
- PlatformTextInputEndResult
- PlatformTextInputSessionPlan
- PlatformTextInputSessionResult
- Runtime UI Image Decode Request Plan v1
- Runtime UI Image Decode Request Plan v1 Implementation Plan (2026-05-08)
- Runtime UI Platform Text Input Session Plan v1
- begin_platform_text_input
- decode_image_request
- end_platform_text_input
- invalid_image_decode_result
- native text-input object/session ownership
- plan_image_decode_request

### 2026-05-08-runtime-ui-platform-text-input-session-plan-v1.md
- **Status:** Completed
- 2026-05-08-runtime-ui-platform-text-input-session-plan-v1.md
- FontRasterizationRequestPlan
- FontRasterizationResult
- IFontRasterizerAdapter
- IPlatformIntegrationAdapter
- MK_ui_renderer_tests
- PlatformTextInputEndPlan
- PlatformTextInputEndResult
- PlatformTextInputSessionPlan
- PlatformTextInputSessionResult
- Runtime UI Font Rasterization Request Plan v1
- Runtime UI Platform Text Input Session Plan v1
- Runtime UI Platform Text Input Session Plan v1 Implementation Plan (2026-05-08)
- begin_platform_text_input
- end_platform_text_input
- invalid_font_allocation
- native text-input object/session ownership
- plan_font_rasterization_request
- rasterize_font_glyph
- without font loading/rasterization implementations

### 2026-05-08-runtime-ui-png-image-decoding-adapter-v1.md
- **Status:** Completed
- 2026-05-08-runtime-ui-png-image-decoding-adapter-v1.md
- IImageDecodingAdapter
- ImageDecodeDispatchResult
- ImageDecodePixelFormat
- ImageDecodePixelFormat::rgba8_unorm
- ImageDecodeRequestPlan
- MK_tools_tests
- MK_ui_renderer_tests
- PngImageDecodingAdapter
- Runtime UI Image Decode Request Plan v1
- Runtime UI PNG Image Decoding Adapter v1
- Runtime UI PNG Image Decoding Adapter v1 Implementation Plan (2026-05-08)
- decode_audited_png_rgba8
- decode_image_request
- invalid_image_decode_result
- plan_image_decode_request
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
- without image decoding implementations

### 2026-05-08-runtime-ui-text-shaping-request-plan-v1.md
- **Plan ID:** `runtime-ui-text-shaping-request-plan-v1`
- **Status:** Completed
- 2026-05-08-runtime-ui-text-shaping-request-plan-v1.md
- ITextShapingAdapter
- MK_ui_renderer_tests
- Runtime UI Text Shaping Request Plan v1
- Runtime UI Text Shaping Request Plan v1 Implementation Plan (2026-05-08)
- TextShapingRequestPlan
- TextShapingResult
- invalid_text_shaping_result
- plan_text_shaping_request
- shape_text_run

### 2026-05-09-editor-dynamic-game-module-driver-load-v1.md
- **Plan ID:** `editor-dynamic-game-module-driver-load-v1`
- **Status:** Completed.
- 2026-05-09-editor-dynamic-game-module-driver-load-v1.md
- Editor Dynamic Game Module Driver Load Implementation Plan
- Editor Dynamic Game Module Driver Load Implementation Plan (2026-05-09)
- LoadLibraryExW
- play_in_editor.game_module_driver
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1

### 2026-05-09-editor-game-module-driver-contract-metadata-review-v1.md
- **Plan ID:** `editor-game-module-driver-contract-metadata-review-v1`
- **Status:** Completed.
- 2026-05-09-editor-game-module-driver-contract-metadata-review-v1.md
- Editor Game Module Driver Contract Metadata Review Implementation Plan
- Editor Game Module Driver Contract Metadata Review Implementation Plan (2026-05-09)
- EditorGameModuleDriverContractMetadataModel
- make_editor_game_module_driver_contract_metadata_model
- play_in_editor.game_module_driver.contract
- same-engine-build
- stable third-party ABI

### 2026-05-09-editor-game-module-driver-dynamic-probe-v1.md
- **Plan ID:** `editor-game-module-driver-dynamic-probe-v1`
- **Status:** Completed.
- 2026-05-09-editor-game-module-driver-dynamic-probe-v1.md
- Editor Game Module Driver Dynamic Probe Implementation Plan
- Editor Game Module Driver Dynamic Probe Implementation Plan (2026-05-09)
- MK_editor_game_module_driver_load_tests
- MK_editor_game_module_driver_probe
- mirakana_create_editor_game_module_driver_v1
- stable third-party ABI

### 2026-05-09-editor-game-module-driver-safe-reload-review-v1.md
- **Plan ID:** `editor-game-module-driver-safe-reload-review-v1`
- **Status:** Completed.
- 2026-05-09-editor-game-module-driver-safe-reload-review-v1.md
- Editor Game Module Driver Safe Reload Review Implementation Plan
- Editor Game Module Driver Safe Reload Review Implementation Plan (2026-05-09)
- EditorGameModuleDriverReloadModel
- Reload Game Module Driver
- active-session hot reload
- make_editor_game_module_driver_reload_model
- play_in_editor.game_module_driver.reload

### 2026-05-09-editor-in-process-runtime-host-review-v1.md
- **Plan ID:** `editor-in-process-runtime-host-review-v1`
- **Status:** Completed.
- 2026-05-09-editor-in-process-runtime-host-review-v1.md
- Editor In-Process Runtime Host Review Implementation Plan
- Editor In-Process Runtime Host Review Implementation Plan (2026-05-09)
- EditorInProcessRuntimeHostModel
- play_in_editor.in_process_runtime_host
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1

### 2026-05-09-editor-runtime-scene-package-validation-execution-v1.md
- **Plan ID:** `editor-runtime-scene-package-validation-execution-v1`
- **Status:** Completed.
- 2026-05-09-editor-runtime-scene-package-validation-execution-v1.md
- Editor Runtime Scene Package Validation Execution Implementation Plan
- Editor Runtime Scene Package Validation Execution Implementation Plan (2026-05-09)
- EditorRuntimeScenePackageValidationExecutionModel
- playtest_package_review.runtime_scene_validation
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1

### 2026-05-09-physics-1-0-collision-system-closeout-v1.md
- **Gap:** `physics-1-0-collision-system` Phase P4.
- **Plan ID:** `physics-1-0-collision-system-closeout-v1`
- **Status:** Completed.
- 2026-05-09-physics-1-0-collision-system-closeout-v1.md
- Gap:** `physics-1-0-collision-system` Phase P2
- Physics 1.0 Collision System Closeout Implementation Plan (2026-05-09)
- PhysicsReplaySignature3D
- Plan ID:** `physics-benchmark-determinism-gates-v1`
- budget gates
- count-based
- evaluate_physics_determinism_gate_3d

### 2026-05-09-physics-benchmark-determinism-gates-v1.md
- **Gap:** `physics-1-0-collision-system` Phase P2.
- **Plan ID:** `physics-benchmark-determinism-gates-v1`
- **Status:** Completed.
- 2026-05-09-physics-benchmark-determinism-gates-v1.md
- DiagnosticsOpsPlan
- Execute this master plan by burning down one
- Gap Burn-down Execution Strategy
- IFontRasterizerAdapter
- Mirakanai_API36
- Physics Benchmark Determinism Gates Implementation Plan (2026-05-09)
- PngImageDecodingAdapter
- Runtime UI Font Rasterization Request Plan v1
- Runtime UI PNG Image Decoding Adapter v1
- UiRendererGlyphAtlasPalette
- android-release-device-matrix-v1
- apple-host-evidence-check
- apple-metal-ios-host-evidence-v1
- crash-telemetry-trace-ops-v1
- generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1
- generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1
- generated-3d-compute-morph-package-smoke-d3d12-v1
- generated-3d-compute-morph-package-smoke-vulkan-v1
- generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1
- generated-3d-compute-morph-skin-package-smoke-d3d12-v1
- physics-1-0-collision-system-closeout-v1
- production-1-0-readiness-audit-v1
- production-readiness-audit-check
- rhi-compute-dispatch-foundation-v1
- rhi-d3d12-calibrated-queue-timing-diagnostics-v1
- rhi-d3d12-per-queue-fence-synchronization-v1
- rhi-d3d12-queue-clock-calibration-foundation-v1
- rhi-d3d12-queue-timestamp-measurement-foundation-v1
- rhi-d3d12-submitted-command-calibrated-timing-scopes-v1
- rhi-vulkan-compute-dispatch-foundation-v1
- runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1
- runtime-rhi-compute-morph-async-telemetry-d3d12-v1
- runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1
- runtime-rhi-compute-morph-d3d12-proof-v1
- runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1
- runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1
- runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1
- runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1
- runtime-rhi-compute-morph-queue-synchronization-d3d12-v1
- runtime-rhi-compute-morph-renderer-consumption-d3d12-v1
- runtime-rhi-compute-morph-renderer-consumption-vulkan-v1
- runtime-rhi-compute-morph-skin-composition-d3d12-v1
- runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1
- runtime-rhi-compute-morph-vulkan-proof-v1
- runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1
- runtime-ui-font-image-adapter-v1

### 2026-05-09-physics-joints-foundation-v1.md
- **Gap:** `physics-1-0-collision-system` Phase P1.
- **Plan ID:** `physics-joints-foundation-v1`
- **Status:** Completed.
- 2026-05-09-physics-joints-foundation-v1.md
- Active gap burn-down
- Gap:** `physics-1-0-collision-system` Phase P3
- Jolt
- Physics Joints Foundation Implementation Plan (2026-05-09)
- Plan ID:** `physics-jolt-adapter-gate-v1`
- explicit 1.0 exclusion

### 2026-05-09-physics-jolt-adapter-gate-v1.md
- **Gap:** `physics-1-0-collision-system` Phase P3.
- **Plan ID:** `physics-jolt-adapter-gate-v1`
- **Status:** Completed.
- 2026-05-09-physics-jolt-adapter-gate-v1.md
- Gap:** `physics-1-0-collision-system` Phase P1
- Physics Jolt Adapter Gate Implementation Plan (2026-05-09)

### 2026-05-18-editor-productization-1-0-host-gated-exclusion-closeout-v1.md
- **Gap:** `editor-productization`
- **Plan ID:** `editor-productization-1-0-host-gated-exclusion-closeout-v1`
- **Status:** Completed
- 2026-05-18-editor-productization-1-0-host-gated-exclusion-closeout-v1.md
- Editor Productization 1.0 Host-Gated Exclusion Closeout
- Editor Productization 1.0 Host-Gated Exclusion Closeout (2026-05-18)
- EditorAiReviewedValidationExecutionModel
- EditorMaterialGpuPreviewExecutionSnapshot
- EditorPlaySession
- PrefabVariantBaseRefreshPlan
- ScenePrefabInstanceRefreshPlan
- Vulkan/Metal material-preview display parity
- explicit 1.0 exclusion
- explicit 1.0 host-gated exclusion
- full-repository-quality-gate
- production-ui-importer-platform-adapters
- reviewed editor authoring/playtest/AI command/resource/input/prefab/material-preview evidence

### 2026-05-18-full-repository-quality-gate-1-0-closeout-v1.md
- **Plan ID:** `full-repository-quality-gate-1-0-closeout-v1`
- **Status:** Completed
- 2026-05-18-full-repository-quality-gate-1-0-closeout-v1.md
- CI Matrix Contract Check v1
- Full Repository Quality Gate 1.0 Closeout
- Full Repository Quality Gate 1.0 Closeout v1 Implementation Plan
- Full Repository Static Analysis CI Contract v1
- Linux coverage threshold policy
- Windows release package artifact evidence
- broader analyzer profile expansion
- full cross-platform package execution evidence
- local full validate
- notarization
- release distribution
- sanitizer lane documentation
- signing
- unsupported_gaps=0

### 2026-05-18-upload-staging-v1-selected-package-upload-evidence-v1.md
- **Status:** Completed.
- 2026-05-18-upload-staging-v1-selected-package-upload-evidence-v1.md
- Runtime Upload Queue Wait v1
- Upload Staging v1 Selected Package Upload Evidence v1 - 2026-05-18
- execute_runtime_package_upload_staging_evidence
- --require-package-upload-staging
- upload_queue_waits_recorded
- wait_for_runtime_uploads_on_queue

## Retired dated plan static evidence (2026-05-22 roadmap/current-capabilities cleanup)

This section is the historical sink for dated plans retired after roadmap/current-capabilities stopped rooting completed slice files. Direct static-guard needles are retained here; detailed prose remains available through Git history.

### 2026-05-01-desktop-runtime-productization.md
- 2026-05-01-desktop-runtime-productization.md
- Desktop Runtime Productization Implementation Plan (2026-05-01)
- **Goal:** Productize the first practical Windows desktop vertical slice so a game can be created, launched from the source tree, packaged, installed, and validated through one documented desktop runtime path.

### 2026-05-01-scene-package-apply-tooling-v1.md
- 2026-05-01-scene-package-apply-tooling-v1.md
- Scene Package Apply Tooling v1 Implementation Plan (2026-05-01)
- **Goal:** Add a reviewed dry-run/apply tooling surface for first-party scene package authoring that can create or update explicit `.scene` content and matching cooked `.geindex` `AssetKind::scene` rows with scene mesh/material/sprite dependency edges, without claiming broad editor productization, package streaming, renderer/RHI residency, material graphs, shader graphs, live shader generation, or native handle access.

### 2026-05-01-scene-prefab-authoring-command-tooling-v1.md
- 2026-05-01-scene-prefab-authoring-command-tooling-v1.md
- Scene Prefab Authoring Command Tooling v1 Implementation Plan (2026-05-01)
- **Status:** Completed on 2026-05-01.
- **Goal:** Add a reviewed AI-safe dry-run/apply command surface for stable-id Scene/Component/Prefab Schema v2 authoring operations, so agents can create scenes, add nodes/components, create prefabs, and instantiate prefabs through typed engine tooling instead of ad hoc JSON/text edits.

### 2026-05-03-metal-visible-presentation-apple-host-v1.md
- 2026-05-03-metal-visible-presentation-apple-host-v1.md
- Metal Visible Presentation Apple Host v1 Implementation Plan (2026-05-03)
- **Goal:** Align Metal runtime owners with the same “visible in GPU capture / Xcode Metal debugger object list” standard as D3D12 `SetName` and Vulkan `vkSetDebugUtilsObjectNameEXT`, scoped to code that already compiles on macOS with Metal+QuartzCore linked.

### 2026-05-03-phase-2-manifest-schema-docs-skills-validation-recipes.md
- 2026-05-03-phase-2-manifest-schema-docs-skills-validation-recipes.md
- Phase 2 manifest, schema, docs, skills, and validation recipe evidence (2026-05-03)
- **Goal:** `ai-cook-package-command-surface-v1` 完了後に残っていた「マニフェスト／スキーマ／ドキュメント／Cursor スキル／`run-validation-recipe` 証跡」の一括整合を記録し、マスター Phase 2 の該当チェックボックスを緑の根拠で更新する。

### 2026-05-03-renderer-rhi-safe-point-resource-teardown-v1.md
- 2026-05-03-renderer-rhi-safe-point-resource-teardown-v1.md
- Renderer RHI Safe Point Resource Teardown v1 Implementation Plan (2026-05-03)
- **Goal:** `RuntimeSceneGpuBindingResult` に保持された RHI ハンドルを、**フレーム境界などホストが選ぶ safe-point** で破棄できるよう、バックエンド中立のティアダウン入口と診断を提供する。実 GPU バックエンドでは `IRhiDevice` に destroy が無いため、**`NullRhiDevice` では決定的にハンドルを無効化**し、それ以外では **ホストがネイティブ側で破棄する必要がある**ことを明示する。

### 2026-05-03-runtime-resource-residency-budget-execution-v1.md
- 2026-05-03-runtime-resource-residency-budget-execution-v1.md
- Runtime Resource Residency Budget Execution v1 Implementation Plan (2026-05-03)
- **Goal:** `packageStreamingResidencyTargets` 等で表現される **常駐バジェット意図**と同一の定義（`RuntimeAssetRecord::content` の合計バイト）を、`merge_runtime_asset_packages_overlay` 後の **単一マージ済みビュー**に対しても機械的に検証できるようにする。上限超過時は **カタログを変更せず**、診断コードで失敗を返す。

### 2026-05-03-sanitizer-and-ci-matrix-hardening-v1.md
- 2026-05-03-sanitizer-and-ci-matrix-hardening-v1.md
- Sanitizer And CI Matrix Hardening v1 Implementation Plan (2026-05-03)
- **Goal:** AddressSanitizer / UndefinedBehaviorSanitizer（およびプロジェクトが既に採用している他サニタイザ）付きビルドで **CTest が緑**であり、その構成が **CI マトリクス上で明示**される。Phase 1 の「Sanitizer CTest lane remains green」と「複数レーンの現行証跡」を満たす。

### 2026-05-04-animation-root-motion-foundation-v1.md
- 2026-05-04-animation-root-motion-foundation-v1.md
- Animation Root Motion Foundation v1 Implementation Plan (2026-05-04)
- **Plan ID:** `animation-root-motion-foundation-v1`
- **Status:** Completed on 2026-05-04. This slice is limited to deterministic `mirakana_animation` root-joint translation delta sampling.

### 2026-05-04-cascaded-and-atlased-shadow-maps-v1.md
- 2026-05-04-cascaded-and-atlased-shadow-maps-v1.md
- Cascaded and atlased shadow maps v1 (2026-05-04)

### 2026-05-05-animation-3d-local-rotation-limits-twist-controls-v1.md
- 2026-05-05-animation-3d-local-rotation-limits-twist-controls-v1.md
- Animation 3D Local Rotation Limits Twist Controls v1 (2026-05-05)
- **Plan ID:** `animation-3d-local-rotation-limits-twist-controls-v1`
- **Status:** Completed

### 2026-05-05-animation-fabrik-3d-chain-ik-v1.md
- 2026-05-05-animation-fabrik-3d-chain-ik-v1.md
- Animation FABRIK 3D Chain IK v1 (2026-05-05)
- **Plan ID:** `animation-fabrik-3d-chain-ik-v1`
- **Status:** Completed

### 2026-05-05-animation-fabrik-3d-pose-application-v1.md
- 2026-05-05-animation-fabrik-3d-pose-application-v1.md
- Animation FABRIK 3D Pose Application v1 (2026-05-05)
- **Plan ID:** `animation-fabrik-3d-pose-application-v1`
- **Status:** Completed

### 2026-05-05-animation-fabrik-integrated-rotation-constraints-v1.md
- 2026-05-05-animation-fabrik-integrated-rotation-constraints-v1.md
- Animation FABRIK Integrated Rotation Constraints v1 (2026-05-05)
- **Plan ID:** `animation-fabrik-integrated-rotation-constraints-v1`
- **Status:** Completed

### 2026-05-05-animation-fabrik-pole-vector-xy-v1.md
- 2026-05-05-animation-fabrik-pole-vector-xy-v1.md
- Animation FABRIK Pole Vector XY v1 (2026-05-05)
- **Plan ID:** `animation-fabrik-pole-vector-xy-v1`
- **Status:** Completed

### 2026-05-05-animation-float-transform-application-v1.md
- 2026-05-05-animation-float-transform-application-v1.md
- Animation Float Transform Application v1 Implementation Plan (2026-05-05)
- **Goal:** Apply sampled scalar float animation curves to caller-owned `Transform3D` rows through explicit, deterministic bindings.
- **Plan ID:** `animation-float-transform-application-v1`
- **Status:** Completed

### 2026-05-05-animation-gltf-quaternion-import-v1.md
- 2026-05-05-animation-gltf-quaternion-import-v1.md
- Animation glTF Quaternion Import v1 (2026-05-05)
- **Plan ID:** `animation-gltf-quaternion-import-v1`
- **Status:** Completed

### 2026-05-05-animation-ik-angle-wrap-normalization-v1.md
- 2026-05-05-animation-ik-angle-wrap-normalization-v1.md
- Animation IK Angle Wrap Normalization v1 (2026-05-05)
- **Plan ID:** `animation-ik-angle-wrap-normalization-v1`
- **Status:** Completed

### 2026-05-05-animation-quaternion-clip-sampling-application-v1.md
- 2026-05-05-animation-quaternion-clip-sampling-application-v1.md
- Animation Quaternion Clip Sampling Application v1 (2026-05-05)
- **Plan ID:** `animation-quaternion-clip-sampling-application-v1`
- **Status:** Completed

### 2026-05-05-animation-quaternion-local-pose-foundation-v1.md
- 2026-05-05-animation-quaternion-local-pose-foundation-v1.md
- Animation Quaternion Local Pose Foundation v1 (2026-05-05)
- **Plan ID:** `animation-quaternion-local-pose-foundation-v1`
- **Status:** Completed

### 2026-05-05-animation-root-motion-loop-accumulation-foundation-v1.md
- 2026-05-05-animation-root-motion-loop-accumulation-foundation-v1.md
- Animation Root Motion Loop Accumulation Foundation v1 Implementation Plan (2026-05-05)
- **Plan ID:** `animation-root-motion-loop-accumulation-foundation-v1`
- **Status:** Completed on 2026-05-05. This slice is limited to deterministic `mirakana_animation` root-motion delta accumulation over an explicit positive clip duration.

### 2026-05-05-animation-root-rotation-delta-foundation-v1.md
- 2026-05-05-animation-root-rotation-delta-foundation-v1.md
- Animation Root Rotation Delta Foundation v1 Implementation Plan (2026-05-05)
- **Plan ID:** `animation-root-rotation-delta-foundation-v1`
- **Status:** Completed on 2026-05-05. This slice is limited to deterministic `mirakana_animation` root-joint Z-rotation delta sampling over the existing root-motion contract.

### 2026-05-05-animation-transform-binding-source-v1.md
- 2026-05-05-animation-transform-binding-source-v1.md
- Animation Transform Binding Source v1 Implementation Plan (2026-05-05)
- **Goal:** Add a first-party source document that maps sampled scalar animation curve targets to named transform components.
- **Plan ID:** `animation-transform-binding-source-v1`
- **Status:** Completed

### 2026-05-05-animation-two-bone-ik-3d-orientation-v1.md
- 2026-05-05-animation-two-bone-ik-3d-orientation-v1.md
- Animation Two Bone IK 3D Orientation v1 (2026-05-05)
- **Plan ID:** `animation-two-bone-ik-3d-orientation-v1`
- **Status:** Completed

### 2026-05-05-animation-two-bone-ik-pole-vector-xy-v1.md
- 2026-05-05-animation-two-bone-ik-pole-vector-xy-v1.md
- Animation Two-Bone IK Pole Vector XY v1 (2026-05-05)
- **Plan ID:** `animation-two-bone-ik-pole-vector-xy-v1`
- **Status:** Completed

### 2026-05-05-cooked-animation-quaternion-clip-v1.md
- 2026-05-05-cooked-animation-quaternion-clip-v1.md
- Cooked Animation Quaternion Clip v1 (2026-05-05)
- **Plan ID:** `cooked-animation-quaternion-clip-v1`
- **Status:** Completed

### 2026-05-05-generated-3d-morph-gpu-palette-smoke-v1.md
- 2026-05-05-generated-3d-morph-gpu-palette-smoke-v1.md
- Generated 3D Morph GPU Palette Smoke v1 (2026-05-05)
- **Plan ID:** `generated-3d-morph-gpu-palette-smoke-v1`
- **Status:** Completed

### 2026-05-05-generated-3d-morph-normal-tangent-package-smoke-v1.md
- 2026-05-05-generated-3d-morph-normal-tangent-package-smoke-v1.md
- Generated 3D Morph Normal Tangent Package Smoke v1 (2026-05-05)
- **Plan ID:** `generated-3d-morph-normal-tangent-package-smoke-v1`
- **Status:** Completed

### 2026-05-05-generated-3d-morph-package-consumption-v1.md
- 2026-05-05-generated-3d-morph-package-consumption-v1.md
- Generated 3D Morph Package Consumption v1 Implementation Plan (2026-05-05)
- **Goal:** Extend generated `DesktopRuntime3DPackage` games so their package smoke consumes a cooked `morph_mesh_cpu` payload plus a cooked morph-weight `animation_float_clip` and reports deterministic CPU morph evaluation counters.
- **Plan ID:** `generated-3d-morph-package-consumption-v1`
- **Status:** Completed

### 2026-05-05-generated-3d-morph-visible-deformation-v1.md
- 2026-05-05-generated-3d-morph-visible-deformation-v1.md
- Generated 3D Morph Visible Deformation v1 (2026-05-05)
- **Plan ID:** `generated-3d-morph-visible-deformation-v1`
- **Status:** Completed

### 2026-05-05-generated-3d-quaternion-animation-package-smoke-v1.md
- 2026-05-05-generated-3d-quaternion-animation-package-smoke-v1.md
- Generated 3D Quaternion Animation Package Smoke v1 (2026-05-05)
- **Plan ID:** `generated-3d-quaternion-animation-package-smoke-v1`
- **Status:** Completed

### 2026-05-05-generated-3d-transform-animation-scaffold-v1.md
- 2026-05-05-generated-3d-transform-animation-scaffold-v1.md
- Generated 3D Transform Animation Scaffold v1 Implementation Plan (2026-05-05)
- **Goal:** Extend generated `DesktopRuntime3DPackage` games so the packaged 3D smoke applies a cooked scalar animation float clip to a scene node transform through first-party transform binding rows.
- **Plan ID:** `generated-3d-transform-animation-scaffold-v1`
- **Status:** Completed

### 2026-05-05-gltf-animation-float-clip-bridge-v1.md
- 2026-05-05-gltf-animation-float-clip-bridge-v1.md
- glTF Animation Float Clip Bridge v1 (2026-05-05)
- **Plan ID:** `gltf-animation-float-clip-bridge-v1`
- **Status:** Completed on 2026-05-05.

### 2026-05-05-gltf-node-transform-animation-binding-source-bridge-v1.md
- 2026-05-05-gltf-node-transform-animation-binding-source-bridge-v1.md
- glTF Node Transform Animation Binding Source Bridge v1 Implementation Plan (2026-05-05)
- **Goal:** Import authored transform-binding source rows for glTF node TRS animation targets.
- **Plan ID:** `gltf-node-transform-animation-binding-source-bridge-v1`
- **Status:** Completed

### 2026-05-05-gltf-node-transform-animation-float-clip-bridge-v1.md
- 2026-05-05-gltf-node-transform-animation-float-clip-bridge-v1.md
- glTF Node Transform Animation Float Clip Bridge v1 (2026-05-05)
- **Plan ID:** `gltf-node-transform-animation-float-clip-bridge-v1`
- **Status:** Completed

### 2026-05-05-gltf-node-transform-animation-import-v1.md
- 2026-05-05-gltf-node-transform-animation-import-v1.md
- glTF Node Transform Animation Import v1 (2026-05-05)
- **Plan ID:** `gltf-node-transform-animation-import-v1`
- **Status:** Completed

### 2026-05-05-gpu-morph-d3d12-proof-v1.md
- 2026-05-05-gpu-morph-d3d12-proof-v1.md
- GPU Morph D3D12 Proof v1 (2026-05-05)
- **Plan ID:** `gpu-morph-d3d12-proof-v1`
- **Status:** Completed

### 2026-05-05-gpu-morph-normal-tangent-d3d12-proof-v1.md
- 2026-05-05-gpu-morph-normal-tangent-d3d12-proof-v1.md
- GPU Morph Normal Tangent D3D12 Proof v1 (2026-05-05)
- **Plan ID:** `gpu-morph-normal-tangent-d3d12-proof-v1`
- **Status:** Completed

### 2026-05-05-rhi-compute-dispatch-foundation-v1.md
- 2026-05-05-rhi-compute-dispatch-foundation-v1.md
- RHI Compute Dispatch Foundation v1 (2026-05-05)
- **Plan ID:** `rhi-compute-dispatch-foundation-v1`
- **Status:** Completed

### 2026-05-05-runtime-rhi-compute-morph-d3d12-proof-v1.md
- 2026-05-05-runtime-rhi-compute-morph-d3d12-proof-v1.md
- Runtime RHI Compute Morph D3D12 Proof v1 (2026-05-05)
- **Plan ID:** `runtime-rhi-compute-morph-d3d12-proof-v1`
- **Status:** Completed

### 2026-05-05-runtime-scene-animation-transform-binding-v1.md
- 2026-05-05-runtime-scene-animation-transform-binding-v1.md
- Runtime Scene Animation Transform Binding v1 Implementation Plan (2026-05-05)
- **Goal:** Resolve authored animation transform binding source rows against a runtime scene instance and optionally apply sampled scalar curves to scene node transforms.
- **Plan ID:** `runtime-scene-animation-transform-binding-v1`
- **Status:** Completed

### 2026-05-05-runtime-scene-rhi-morph-gpu-palette-v1.md
- 2026-05-05-runtime-scene-rhi-morph-gpu-palette-v1.md
- Runtime Scene RHI Morph GPU Palette v1 (2026-05-05)
- **Plan ID:** `runtime-scene-rhi-morph-gpu-palette-v1`
- **Status:** Completed

### 2026-05-06-2d-native-sprite-batching-execution-v1.md
- 2026-05-06-2d-native-sprite-batching-execution-v1.md
- 2D Native Sprite Batching Execution v1 Implementation Plan (2026-05-06)
- **Plan ID:** `2d-native-sprite-batching-execution-v1`
- **Status:** Completed
- Static guard literals:
  - `plan_sprite_batches`
  - `sample_2d_desktop_runtime_package`

### 2026-05-06-2d-sprite-animation-package-v1.md
- 2026-05-06-2d-sprite-animation-package-v1.md
- 2D Sprite Animation Package v1 Implementation Plan (2026-05-06)
- **Plan ID:** `2d-sprite-animation-package-v1`
- **Status:** Completed
- Static guard literals:
  - `DesktopRuntime2DPackage`
  - `sprite_animation_frames_sampled=3`

### 2026-05-06-2d-tilemap-editor-runtime-ux-v1.md
- 2026-05-06-2d-tilemap-editor-runtime-ux-v1.md
- 2D Tilemap Editor Runtime UX v1 Implementation Plan (2026-05-06)
- **Plan ID:** `2d-tilemap-editor-runtime-ux-v1`
- **Status:** Completed
- Static guard literals:
  - `GameEngine.Tilemap.v1`
  - `tilemap_cells_sampled=3`

### 2026-05-06-generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1.md
- 2026-05-06-generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1.md
- Generated 3D Compute Morph Normal Tangent Package Smoke D3D12 v1 (2026-05-06)
- **Plan ID:** `generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1`
- **Status:** Completed

### 2026-05-06-generated-3d-compute-morph-package-smoke-d3d12-v1.md
- 2026-05-06-generated-3d-compute-morph-package-smoke-d3d12-v1.md
- Generated 3D Compute Morph Package Smoke D3D12 v1 (2026-05-06)
- **Plan ID:** `generated-3d-compute-morph-package-smoke-d3d12-v1`
- **Status:** Completed

### 2026-05-06-generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1.md
- 2026-05-06-generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1.md
- Generated 3D Compute Morph Queue Sync Package Smoke D3D12 v1 (2026-05-06)
- **Plan ID:** `generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1`
- **Status:** Completed
- Static guard literals:
  - `compute_morph_queue_waits`

### 2026-05-06-input-rebinding-profile-ux-v1.md
- 2026-05-06-input-rebinding-profile-ux-v1.md
- Input Rebinding Profile UX v1 Implementation Plan (2026-05-06)
- **Goal:** Add a host-independent, persisted runtime input rebinding profile contract plus editor-core review diagnostics so generated games can expose controller/key rebinding without SDL3, editor-private APIs, native handles, or middleware.
- **Status:** Completed
- Static guard literals:
  - `EditorInputRebindingProfileReviewModel`
  - `GameEngine.RuntimeInputRebindingProfile.v1`
  - `apply_runtime_input_rebinding_profile`

### 2026-05-06-material-graph-package-binding-v1.md
- 2026-05-06-material-graph-package-binding-v1.md
- Material Graph Package Binding v1 Implementation Plan (2026-05-06)
- **Goal:** Add a reviewed, host-independent package update surface that lowers explicit `GameEngine.MaterialGraph.v1` source text into the existing runtime-loadable `GameEngine.Material.v1` material package row.
- **Status:** Completed

### 2026-05-06-play-in-editor-gameplay-driver-v1.md
- 2026-05-06-play-in-editor-gameplay-driver-v1.md
- Play-In-Editor Gameplay Driver v1 Implementation Plan (2026-05-06)
- **Goal:** Extend the Play-In-Editor editor-core contract so a reviewed gameplay simulation driver can run against the isolated simulation scene without mutating the authored source scene.

### 2026-05-06-play-in-editor-session-isolation-v1.md
- 2026-05-06-play-in-editor-session-isolation-v1.md
- Play-In-Editor Session Isolation v1 Implementation Plan (2026-05-06)
- **Goal:** Add a GUI-independent Play-In-Editor session isolation model so editor play mode runs against a mutable simulation scene copy and stopping play discards simulation edits while preserving the authored source scene.

### 2026-05-06-play-in-editor-visible-viewport-wiring-v1.md
- 2026-05-06-play-in-editor-visible-viewport-wiring-v1.md
- Play-In-Editor Visible Viewport Wiring v1 Implementation Plan (2026-05-06)
- **Goal:** Wire the visible `mirakana_editor` Run/Viewport controls to `EditorPlaySession` so the editor shell displays and advances the isolated simulation scene while source-scene mutations stay blocked during play.

### 2026-05-06-rhi-vulkan-compute-dispatch-foundation-v1.md
- 2026-05-06-rhi-vulkan-compute-dispatch-foundation-v1.md
- RHI Vulkan Compute Dispatch Foundation v1 Implementation Plan (2026-05-06)
- **Plan ID:** `rhi-vulkan-compute-dispatch-foundation-v1`
- **Status:** Completed
- Static guard literals:
  - `MK_VULKAN_TEST_COMPUTE_SPV`
  - `vkCmdDispatch`
  - `vkCreateComputePipelines`

### 2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1.md
- 2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1.md
- Runtime RHI Compute Morph Normal Tangent Output D3D12 v1 (2026-05-06)
- **Plan ID:** `runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1`
- **Status:** Completed

### 2026-05-06-runtime-rhi-compute-morph-queue-synchronization-d3d12-v1.md
- 2026-05-06-runtime-rhi-compute-morph-queue-synchronization-d3d12-v1.md
- Runtime RHI Compute Morph Queue Synchronization D3D12 v1 (2026-05-06)
- **Plan ID:** `runtime-rhi-compute-morph-queue-synchronization-d3d12-v1`
- **Status:** Completed
- without host-side `IRhiDevice::wait`

### 2026-05-06-runtime-rhi-compute-morph-renderer-consumption-d3d12-v1.md
- 2026-05-06-runtime-rhi-compute-morph-renderer-consumption-d3d12-v1.md
- Runtime RHI Compute Morph Renderer Consumption D3D12 v1 (2026-05-06)
- **Plan ID:** `runtime-rhi-compute-morph-renderer-consumption-d3d12-v1`
- **Status:** Completed

### 2026-05-06-runtime-rhi-compute-morph-skin-composition-d3d12-v1.md
- 2026-05-06-runtime-rhi-compute-morph-skin-composition-d3d12-v1.md
- Runtime RHI Compute Morph Skin Composition D3D12 v1 (2026-05-06)
- **Plan ID:** `runtime-rhi-compute-morph-skin-composition-d3d12-v1`
- **Status:** Completed
- Static guard literals:
  - `skin_attribute_vertex_buffer`

### 2026-05-06-runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1.md
- 2026-05-06-runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1.md
- Runtime Scene RHI Compute Morph Skin Palette D3D12 v1 (2026-05-06)
- **Plan ID:** `runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1`
- **Status:** Completed
- Static guard literals:
  - `RuntimeSceneComputeMorphSkinnedMeshBinding`
  - `SceneSkinnedGpuBindingPalette`
  - `compute_morph_output_position_bytes`
  - `compute_morph_skinned_mesh_bindings`

### 2026-05-07-editor-ai-command-diagnostics-panel-v1.md
- 2026-05-07-editor-ai-command-diagnostics-panel-v1.md
- Editor AI Command Diagnostics Panel v1 Implementation Plan (2026-05-07)
- **Goal:** Add a visible editor AI Commands panel backed by a GUI-independent diagnostics model over existing AI package, validation recipe, operator handoff, and evidence summary contracts.

### 2026-05-07-editor-ai-evidence-import-review-v1.md
- 2026-05-07-editor-ai-evidence-import-review-v1.md
- Editor AI Evidence Import Review v1 Implementation Plan (2026-05-07)
- **Goal:** Add a deterministic editor-core evidence import review surface so externally supplied AI validation evidence can be pasted, reviewed, and fed into the existing AI Commands panel without editor-core execution or file mutation.

### 2026-05-07-editor-ai-host-gated-validation-execution-ack-v1.md
- 2026-05-07-editor-ai-host-gated-validation-execution-ack-v1.md
- Editor AI Host-Gated Validation Execution Ack v1 Implementation Plan (2026-05-07)
- **Goal:** Let the visible `mirakana_editor` AI Commands panel execute reviewed host-gated `run-validation-recipe` rows only after an explicit per-row host-gate acknowledgement.
- Static guard literals:
  - `-HostGateAcknowledgements`
  - `Host-Gated Validation Execution Ack v1`
  - `acknowledge_host_gates`
  - `host_gate_acknowledgement_required`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### 2026-05-07-editor-ai-reviewed-validation-batch-execution-v1.md
- 2026-05-07-editor-ai-reviewed-validation-batch-execution-v1.md
- Editor AI Reviewed Validation Batch Execution v1 Implementation Plan (2026-05-07)
- **Goal:** Add a reviewed batch execution model and visible `MK_editor` control for all currently executable AI validation recipe rows, including host-gated rows only after explicit acknowledgement.
- Static guard literals:
  - `Editor AI Reviewed Validation Batch Execution v1 Implementation Plan`
  - `EditorAiReviewedValidationExecutionBatchModel`
  - `Execute Ready`
  - `ai_commands.execution.batch`
  - `make_editor_ai_reviewed_validation_execution_batch`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### 2026-05-07-editor-ai-reviewed-validation-execution-v1.md
- 2026-05-07-editor-ai-reviewed-validation-execution-v1.md
- Editor AI Reviewed Validation Execution v1 Implementation Plan (2026-05-07)
- **Goal:** Let the visible `mirakana_editor` AI Commands panel execute host-gate-free reviewed `run-validation-recipe` rows and feed the result into the existing evidence summary without moving process execution into `editor/core`.

### 2026-05-07-editor-content-browser-import-codec-adapter-review-v1.md
- 2026-05-07-editor-content-browser-import-codec-adapter-review-v1.md
- Editor Content Browser Import Codec Adapter Review v1 Implementation Plan (2026-05-07)
- **Goal:** Let the Content Browser reviewed import-source dialog build explicit import plans for supported PNG, glTF, and common-audio codec sources through the existing optional `MK_tools` importer adapters.
- Static guard literals:
  - `.flac`
  - `.glb`
  - `.gltf`
  - `.mp3`
  - `.png`
  - `.wav`
  - `Editor Content Browser Import Codec Adapter Review v1 Implementation Plan`
  - `ExternalAssetImportAdapters`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1`

### 2026-05-07-editor-content-browser-import-diagnostics-v1.md
- 2026-05-07-editor-content-browser-import-diagnostics-v1.md
- Editor Content Browser Import Diagnostics v1 Implementation Plan (2026-05-07)
- **Goal:** Add a GUI-independent editor content/import diagnostics panel model that unifies Content Browser rows, import progress, diagnostics, dependencies, thumbnail requests, and material preview rows for the visible Assets panel and AI/editor tooling.

### 2026-05-07-editor-content-browser-import-external-copy-review-v1.md
- 2026-05-07-editor-content-browser-import-external-copy-review-v1.md
- Editor Content Browser Import External Copy Review v1 Implementation Plan (2026-05-07)
- **Goal:** Let the Content Browser reviewed import-source dialog stage external first-party source documents into the project before rebuilding the import plan.
- Static guard literals:
  - `Copy External Sources`
  - `Editor Content Browser Import External Copy Review v1 Implementation Plan`
  - `EditorContentBrowserImportExternalSourceCopyModel`
  - `content_browser_import.external_copy`
  - `make_content_browser_import_external_source_copy_model`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### 2026-05-07-editor-content-browser-import-native-dialog-v1.md
- 2026-05-07-editor-content-browser-import-native-dialog-v1.md
- Editor Content Browser Import Native Dialog v1 Implementation Plan (2026-05-07)
- **Goal:** Add a reviewed native open-file dialog path for Content Browser asset import source selection without moving file dialogs, filesystem path conversion, or import execution into `editor/core`.
- Static guard literals:
  - `Editor Content Browser Import Native Dialog v1 Implementation Plan`
  - `EditorContentBrowserImportOpenDialogModel`
  - `content_browser_import.open_dialog`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### 2026-05-07-editor-input-rebinding-profile-panel-v1.md
- 2026-05-07-editor-input-rebinding-profile-panel-v1.md
- Editor Input Rebinding Profile Panel v1 Implementation Plan (2026-05-07)
- **Goal:** Promote the existing input rebinding profile review contract into a visible read-only editor panel that `mirakana_editor` and AI/editor tooling can share.
- Static guard literals:
  - `Editor Input Rebinding Profile Panel v1`
  - `EditorInputRebindingProfilePanelModel`
  - `PanelId::input_rebinding`
  - `interactive runtime/game rebinding panels`

### 2026-05-07-editor-material-asset-preview-diagnostics-v1.md
- 2026-05-07-editor-material-asset-preview-diagnostics-v1.md
- Editor Material Asset Preview Diagnostics v1 Implementation Plan (2026-05-07)
- **Goal:** Add a GUI-independent read-only material asset preview diagnostics model that visible `mirakana_editor` and AI/editor tooling can share.

### 2026-05-07-editor-material-gpu-preview-execution-evidence-v1.md
- 2026-05-07-editor-material-gpu-preview-execution-evidence-v1.md
- Editor Material GPU Preview Execution Evidence v1 Implementation Plan (2026-05-07)
- **Goal:** Add retained editor-core evidence rows for host-owned selected-material GPU preview execution without moving RHI, SDL3, Dear ImGui, shader compiler, or native handle work into `editor/core`.
- Static guard literals:
  - `EditorMaterialGpuPreviewExecutionSnapshot`
  - `Vulkan display parity`
  - `apply_editor_material_gpu_preview_execution_snapshot`
  - `editor-material-gpu-preview-execution-evidence-v1`
  - `material_asset_preview.gpu.execution`

### 2026-05-07-editor-prefab-variant-batch-resolution-review-v1.md
- 2026-05-07-editor-prefab-variant-batch-resolution-review-v1.md
- Editor Prefab Variant Batch Resolution Review v1 Implementation Plan (2026-05-07)
- **Goal:** Add a reviewed batch-resolution path for prefab variant conflicts so the editor can apply all currently safe cleanup/retarget rows as one undoable action.
- Static guard literals:
  - `Apply All Reviewed`
  - `Editor Prefab Variant Batch Resolution Review v1`
  - `PrefabVariantConflictBatchResolutionPlan`
  - `automatic merge/rebase`
  - `make_prefab_variant_conflict_batch_resolution_action`
  - `prefab_variant_conflicts.batch_resolution`
  - `resolve_prefab_variant_conflicts`

### 2026-05-07-editor-prefab-variant-conflict-review-v1.md
- 2026-05-07-editor-prefab-variant-conflict-review-v1.md
- Editor Prefab Variant Conflict Review v1 Implementation Plan (2026-05-07)
- **Goal:** Add a GUI-independent, read-only prefab variant conflict review model so editor/AI tooling can surface unsafe or surprising prefab override rows before save, instantiation, or future nested propagation work.
- Static guard literals:
  - `PrefabVariantConflictReviewModel`
  - `automatic merge/rebase/resolution UX`
  - `nested-prefab-conflict-ux-v1`
  - `prefab_variant_conflicts`

### 2026-05-07-editor-prefab-variant-missing-node-cleanup-v1.md
- 2026-05-07-editor-prefab-variant-missing-node-cleanup-v1.md
- Editor Prefab Variant Missing Node Cleanup v1 Implementation Plan (2026-05-07)
- **Goal:** Add an explicit reviewed cleanup path for prefab variant overrides that target nodes no longer present in the base prefab.
- Static guard literals:
  - `Remove missing-node override`
  - `automatic merge/rebase/resolution UX`
  - `deserialize_prefab_variant_definition_for_review`
  - `editor-prefab-variant-missing-node-cleanup-v1`
  - `node remapping`

### 2026-05-07-editor-prefab-variant-native-dialog-v1.md
- 2026-05-07-editor-prefab-variant-native-dialog-v1.md
- Editor Prefab Variant Native Dialog v1 Implementation Plan (2026-05-07)
- **Plan ID:** `editor-prefab-variant-native-dialog-v1`
- **Status:** Completed
- **Goal:** Add a narrow native open/save dialog review path for visible Prefab Variant authoring so `mirakana_editor` can browse `.prefabvariant` files outside the Profiler-only dialog path.
- Static guard literals:
  - `Browse Load Variant`
  - `Browse Save Variant`
  - `Editor Prefab Variant Native Dialog v1 Implementation Plan`
  - `EditorPrefabVariantFileDialogModel`
  - `SdlFileDialogService`
  - `broader editor native save/open dialogs`
  - `make_prefab_variant_file_dialog_ui_model`
  - `make_prefab_variant_open_dialog_model`
  - `make_prefab_variant_open_dialog_request`
  - `make_prefab_variant_save_dialog_model`
  - `make_prefab_variant_save_dialog_request`

### 2026-05-07-editor-prefab-variant-node-retarget-review-v1.md
- 2026-05-07-editor-prefab-variant-node-retarget-review-v1.md
- Editor Prefab Variant Node Retarget Review v1 Implementation Plan (2026-05-07)
- **Goal:** Implement `editor-prefab-variant-node-retarget-review-v1`: add a reviewed, undoable prefab-variant missing-node retarget path when a stale override carries a stable source-node name that uniquely resolves to a current base-prefab node.
- Static guard literals:
  - `PrefabNodeOverride::source_node_name`
  - `Retarget override to node`
  - `automatic merge/rebase/resolution UX`
  - `editor-prefab-variant-node-retarget-review-v1`
  - `override.N.source_node_name`

### 2026-05-07-editor-prefab-variant-reviewed-resolution-v1.md
- 2026-05-07-editor-prefab-variant-reviewed-resolution-v1.md
- Editor Prefab Variant Reviewed Resolution v1 Implementation Plan (2026-05-07)
- **Goal:** Add an explicit reviewed cleanup path for safe prefab variant conflict rows, starting with redundant overrides and later duplicate override rows.
- Static guard literals:
  - `automatic merge/rebase/resolution UX`
  - `editor-prefab-variant-reviewed-resolution-v1`
  - `make_prefab_variant_conflict_resolution_action`
  - `missing-node resolution`
  - `prefab_variant_conflicts`
  - `resolve_prefab_variant_conflict`

### 2026-05-07-editor-prefab-variant-source-mismatch-accept-current-review-v1.md
- 2026-05-07-editor-prefab-variant-source-mismatch-accept-current-review-v1.md
- Editor Prefab Variant Source Mismatch Accept Current Review v1 Implementation Plan (2026-05-07)
- **Goal:** Add a reviewed accept-current-node path for existing-node prefab variant source mismatches when the old source-node name no longer maps uniquely to another base node.
- Static guard literals:
  - `Accept current node N`
  - `Editor Prefab Variant Source Mismatch Accept Current Review v1`
  - `accept_current.node.1.transform`
  - `accept_current_node`
  - `automatic merge/rebase`
  - `resolve_prefab_variant_conflict`
  - `resolve_prefab_variant_conflicts`
  - `source_node_mismatch`
  - strict `MK_scene` prefab variant validation and composition index-based
  - `updates only`

### 2026-05-07-editor-prefab-variant-source-mismatch-retarget-review-v1.md
- 2026-05-07-editor-prefab-variant-source-mismatch-retarget-review-v1.md
- Editor Prefab Variant Source Mismatch Retarget Review v1 Implementation Plan (2026-05-07)
- **Goal:** Add a reviewed retarget path when a prefab variant override still targets an existing node index but its recorded `source_node_name` now points to a different current base-prefab node.
- Static guard literals:
  - `Editor Prefab Variant Source Mismatch Retarget Review v1`
  - `Retarget override to node N`
  - `automatic merge/rebase`
  - `composition index-based`
  - `prefab_variant_conflicts.rows.node.1.transform.resolution_kind`
  - `resolve_prefab_variant_conflict`
  - `resolve_prefab_variant_conflicts`
  - `source_node_mismatch`

### 2026-05-07-editor-profiler-native-trace-open-dialog-v1.md
- 2026-05-07-editor-profiler-native-trace-open-dialog-v1.md
- Editor Profiler Native Trace Open Dialog v1 Implementation Plan (2026-05-07)
- **Plan ID:** `editor-profiler-native-trace-open-dialog-v1`
- **Goal:** Add a narrow native file-open dialog path for selecting Profiler Trace Event JSON files and feeding safe project-relative selections into the existing trace import review workflow.
- Static guard literals:
  - `Browse Trace JSON`
  - `Editor Profiler Native Trace Open Dialog v1 Implementation Plan`
  - `EditorProfilerTraceOpenDialogModel`
  - `SdlFileDialogService`
  - `make_editor_profiler_trace_open_dialog_model`
  - `make_editor_profiler_trace_open_dialog_request`
  - `profiler.trace_open_dialog`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### 2026-05-07-editor-profiler-native-trace-save-dialog-v1.md
- 2026-05-07-editor-profiler-native-trace-save-dialog-v1.md
- Editor Profiler Native Trace Save Dialog v1 Implementation Plan (2026-05-07)
- **Plan ID:** `editor-profiler-native-trace-save-dialog-v1`
- **Goal:** Add a narrow native file-save dialog path for choosing the Profiler Trace Event JSON output file while preserving the existing safe project-relative trace save contract.
- Static guard literals:
  - `Browse Save Trace JSON`
  - `Editor Profiler Native Trace Save Dialog v1 Implementation Plan`
  - `EditorProfilerTraceSaveDialogModel`
  - `SdlFileDialogService`
  - `make_editor_profiler_trace_save_dialog_model`
  - `make_editor_profiler_trace_save_dialog_request`
  - `profiler.trace_save_dialog`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### 2026-05-07-editor-profiler-telemetry-handoff-v1.md
- 2026-05-07-editor-profiler-telemetry-handoff-v1.md
- Editor Profiler Telemetry Handoff v1 Implementation Plan (2026-05-07)
- **Plan ID:** `editor-profiler-telemetry-handoff-v1`
- **Goal:** Expose the existing dependency-free `DiagnosticsOpsPlan` telemetry upload handoff state in the editor Profiler panel without uploading telemetry or adding a backend dependency.
- Static guard literals:
  - `Editor Profiler Telemetry Handoff v1 Implementation Plan`
  - `EditorProfilerTelemetryHandoffModel`
  - `Profiler Telemetry Handoff`
  - `make_editor_profiler_telemetry_handoff_model`
  - `profiler.telemetry`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### 2026-05-07-editor-profiler-trace-export-v1.md
- 2026-05-07-editor-profiler-trace-export-v1.md
- Editor Profiler Trace Export v1 Implementation Plan (2026-05-07)
- **Goal:** Add a deterministic Profiler trace-export model and visible `MK_editor` copy action for the current diagnostics capture.
- Static guard literals:
  - `Copy Trace JSON`
  - `Editor Profiler Trace Export v1 Implementation Plan`
  - `EditorProfilerTraceExportModel`
  - `make_editor_profiler_trace_export_model`
  - `profiler.trace_export`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### 2026-05-07-editor-profiler-trace-file-import-review-v1.md
- 2026-05-07-editor-profiler-trace-file-import-review-v1.md
- Editor Profiler Trace File Import Review v1 Implementation Plan (2026-05-07)
- **Plan ID:** `editor-profiler-trace-file-import-review-v1`
- **Goal:** Add a safe project-relative trace JSON file import review path so the editor Profiler can read a saved trace from the project `ITextStore` and review it through the existing structured Trace Event JSON reviewer.
- Static guard literals:
  - `Editor Profiler Trace File Import Review v1 Implementation Plan`
  - `EditorProfilerTraceFileImportRequest`
  - `EditorProfilerTraceFileImportResult`
  - `Import Trace JSON`
  - `Trace Import Path`
  - `import_editor_profiler_trace_json`
  - `profiler.trace_file_import`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### 2026-05-07-editor-profiler-trace-file-save-v1.md
- 2026-05-07-editor-profiler-trace-file-save-v1.md
- Editor Profiler Trace File Save v1 Implementation Plan (2026-05-07)
- **Plan ID:** `editor-profiler-trace-file-save-v1`
- **Goal:** Add an explicit editor Profiler action that saves the current diagnostics Chrome Trace Event JSON to a safe project-relative `.json` file.
- Static guard literals:
  - `Editor Profiler Trace File Save v1 Implementation Plan`
  - `EditorProfilerTraceFileSaveRequest`
  - `EditorProfilerTraceFileSaveResult`
  - `Save Trace JSON`
  - `profiler.trace_file_save`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`
  - `save_editor_profiler_trace_json`

### 2026-05-07-editor-profiler-trace-import-reconstruction-v1.md
- 2026-05-07-editor-profiler-trace-import-reconstruction-v1.md
- Editor Profiler Trace Import Reconstruction v1 Implementation Plan (2026-05-07)
- **Plan ID:** `editor-profiler-trace-import-reconstruction-v1`
- **Status:** Completed
- **Goal:** Reconstruct a narrow `mirakana::DiagnosticCapture` from GameEngine-exported Chrome Trace Event JSON so the editor Profiler import path can hand callers reviewed capture rows instead of review counts only.
- Static guard literals:
  - `DiagnosticsTraceImportResult`
  - `Editor Profiler Trace Import Reconstruction v1 Implementation Plan`
  - `EditorProfilerTraceFileImportResult`
  - `EditorProfilerTraceImportReviewModel`
  - `first-party exported`
  - `import_diagnostics_trace_json`
  - `profiler.trace_import.reconstructed_*`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

### 2026-05-07-editor-profiler-trace-import-review-v1.md
- 2026-05-07-editor-profiler-trace-import-review-v1.md
- Editor Profiler Trace Import Review v1 Implementation Plan (2026-05-07)
- **Plan ID:** `editor-profiler-trace-import-review-v1`
- **Goal:** Add a safe structured review path for pasted Chrome Trace Event JSON so the editor Profiler can inspect trace files exported by `mirakana::export_diagnostics_trace_json` without adding file import, flame graphs, or third-party telemetry tooling.
- Static guard literals:
  - `DiagnosticsTraceImportReview`
  - `Editor Profiler Trace Import Review v1 Implementation Plan`
  - `EditorProfilerTraceImportReviewModel`
  - `Review Trace JSON`
  - `make_editor_profiler_trace_import_review_model`
  - `profiler.trace_import`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`
  - `review_diagnostics_trace_json`

### 2026-05-07-editor-project-native-dialog-v1.md
- 2026-05-07-editor-project-native-dialog-v1.md
- Editor Project Native Dialog v1 Implementation Plan (2026-05-07)
- **Goal:** Add reviewed native `.geproject` open/save-as dialog models and visible `MK_editor` wiring for project bundles without moving SDL3, native handles, or filesystem path conversion into `editor/core`.
- Static guard literals:
  - `Editor Project Native Dialog v1 Implementation Plan`
  - `EditorProjectFileDialogModel`
  - `SdlFileDialogService`
  - `make_project_file_dialog_ui_model`
  - `make_project_open_dialog_model`
  - `make_project_open_dialog_request`
  - `make_project_save_dialog_model`
  - `make_project_save_dialog_request`
  - `project_file_dialog.open`
  - `project_file_dialog.save`

### 2026-05-07-editor-runtime-host-playtest-launch-v1.md
- 2026-05-07-editor-runtime-host-playtest-launch-v1.md
- Editor Runtime Host Playtest Launch v1 Implementation Plan (2026-05-07)
- **Goal:** Add a reviewed Play-In-Editor runtime-host launch path so the visible editor can execute a selected desktop runtime host as an external process and record evidence without loading dynamic game modules into editor core.
- Static guard literals:
  - `Editor Runtime Host Playtest Launch v1 Implementation Plan`
  - `EditorRuntimeHostPlaytestLaunchModel`
  - `play_in_editor.runtime_host`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### 2026-05-07-editor-scene-native-dialog-v1.md
- 2026-05-07-editor-scene-native-dialog-v1.md
- Editor Scene Native Dialog v1 Implementation Plan (2026-05-07)
- **Plan ID:** `editor-scene-native-dialog-v1`
- **Status:** Completed
- **Goal:** Add a narrow native open/save dialog review path for visible Scene authoring so `mirakana_editor` can browse `.scene` files through reviewed first-party contracts.
- Static guard literals:
  - `Editor Scene Native Dialog v1 Implementation Plan`
  - `EditorSceneFileDialogModel`
  - `SdlFileDialogService`
  - `broader editor native save/open`
  - `make_scene_file_dialog_ui_model`
  - `make_scene_open_dialog_model`
  - `make_scene_open_dialog_request`
  - `make_scene_save_dialog_model`
  - `make_scene_save_dialog_request`

### 2026-05-08-editor-input-rebinding-action-capture-panel-v1.md
- 2026-05-08-editor-input-rebinding-action-capture-panel-v1.md
- Editor Input Rebinding Action Capture Panel v1 Implementation Plan (2026-05-08)
- **Goal:** Promote the visible editor Input Rebinding panel from read-only diagnostics to reviewed in-memory digital action capture using the existing first-party runtime rebinding capture contract. Validated in `MK_editor_core_tests`.
- **Status:** Completed.
- EditorInputRebindingCaptureModel
- make_editor_input_rebinding_capture_action_model
- make_input_rebinding_capture_action_ui_model
- input_rebinding.capture
- MK_editor_core_tests
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1

### 2026-05-08-runtime-input-rebinding-capture-contract-v1.md
- 2026-05-08-runtime-input-rebinding-capture-contract-v1.md
- Runtime Input Rebinding Capture Contract v1 Implementation Plan (2026-05-08)
- **Goal:** Add a host-independent runtime input rebinding capture contract that turns the next first-party key, pointer, or gamepad-button press into a validated `RuntimeInputRebindingProfile` action override candidate.
- **Status:** Completed.
- Static guard literals:
  - `MK_runtime_tests`
  - `RuntimeInputRebindingCaptureRequest`
  - `RuntimeInputRebindingCaptureResult`
  - `capture_runtime_input_rebinding_action`

### 2026-05-08-runtime-input-rebinding-focus-consumption-v1.md
- 2026-05-08-runtime-input-rebinding-focus-consumption-v1.md
- Runtime Input Rebinding Focus Consumption v1 Implementation Plan (2026-05-08)
- **Status:** Completed.
- **Goal:** Add a host-independent runtime input rebinding capture guard that keeps a UI/menu capture focused and consumes gameplay input while a digital action rebinding capture is waiting or captured.
- Static guard literals:
  - `RuntimeInputRebindingFocusCaptureRequest`

### 2026-05-08-runtime-input-rebinding-presentation-rows-v1.md
- 2026-05-08-runtime-input-rebinding-presentation-rows-v1.md
- Runtime Input Rebinding Presentation Rows v1 Implementation Plan (2026-05-08)
- **Plan ID:** `runtime-input-rebinding-presentation-rows-v1`
- **Status:** Completed
- **Goal:** Add host-independent `mirakana_runtime` input rebinding presentation rows so runtime/game rebinding UI can display reviewed base/profile bindings and symbolic glyph lookup keys without depending on editor code, SDL3, native handles, or platform glyph generation.
- RuntimeInputRebindingPresentationToken
- RuntimeInputRebindingPresentationRow
- RuntimeInputRebindingPresentationModel
- present_runtime_input_action_trigger
- present_runtime_input_axis_source
- make_runtime_input_rebinding_presentation
- glyph_lookup_key
- model.diagnostics[0].path == row->id
- MK_runtime_tests

### 2026-05-08-runtime-resource-safe-point-unload-v1.md
- 2026-05-08-runtime-resource-safe-point-unload-v1.md
- Runtime Resource Safe-Point Unload v1 Implementation Plan (2026-05-08)
- **Goal:** Add an explicit host-independent safe-point unload primitive for the active runtime package and `RuntimeResourceCatalogV2`.
- **Status:** Completed.

### 2026-05-09-editor-nested-prefab-refresh-resolution-v1.md
- 2026-05-09-editor-nested-prefab-refresh-resolution-v1.md
- Editor Nested Prefab Refresh Resolution Implementation Plan (2026-05-09)
- **Goal:** Extend reviewed scene prefab instance refresh so nested linked prefab instances can be explicitly preserved under refreshed parent prefab anchors.
- **Plan ID:** `editor-nested-prefab-refresh-resolution-v1`
- **Status:** Completed.

### 2026-05-09-editor-prefab-instance-stale-node-refresh-resolution-v1.md
- 2026-05-09-editor-prefab-instance-stale-node-refresh-resolution-v1.md
- Editor Prefab Instance Stale Node Refresh Resolution Implementation Plan (2026-05-09)
- **Goal:** Extend reviewed scene prefab instance refresh so an author can explicitly keep stale source-node subtrees as local author-owned scene nodes.
- **Plan ID:** `editor-prefab-instance-stale-node-refresh-resolution-v1`
- **Status:** Completed.

### 2026-05-09-editor-prefab-variant-base-refresh-merge-review-v1.md
- 2026-05-09-editor-prefab-variant-base-refresh-merge-review-v1.md
- Editor Prefab Variant Base Refresh Merge Review Implementation Plan (2026-05-09)
- **Goal:** Add a GUI-independent reviewed base-prefab refresh surface for prefab variants so editor authors can retarget existing overrides by stable `source_node_name` hints before accepting a refreshed embedded base prefab.
- **Plan ID:** `editor-prefab-variant-base-refresh-merge-review-v1`
- **Status:** Completed.
- **Gap:** `editor-productization` focused child slice.
- Static guard literals:
  - `Editor Prefab Variant Base Refresh Merge Review Implementation Plan`
  - `PrefabVariantBaseRefreshPlan`
  - `apply_prefab_variant_base_refresh`
  - `duplicate target`
  - `full nested prefab propagation`
  - `make_prefab_variant_base_refresh_ui_model`
  - `missing hints`
  - `plan_prefab_variant_base_refresh`
  - `prefab_variant_base_refresh`

### 2026-05-10-editor-input-rebinding-axis-capture-keyboard-key-pair-v1.md
- 2026-05-10-editor-input-rebinding-axis-capture-keyboard-key-pair-v1.md
- Editor Input Rebinding Axis Capture Keyboard Key Pair v1 (2026-05-10)
- **Plan ID:** `editor-input-rebinding-axis-capture-keyboard-key-pair-v1`
- **Status:** Completed.

### 2026-05-12-asset-identity-v2-command-apply-surface-evidence-v1.md
- 2026-05-12-asset-identity-v2-command-apply-surface-evidence-v1.md
- Asset Identity v2 Command Apply Surface Evidence v1 (2026-05-12)
- **Plan ID:** `asset-identity-v2-command-apply-surface-evidence-v1`
- **Status:** Completed
- **Gap:** `asset-identity-v2`

### 2026-05-12-asset-identity-v2-reference-cleanup-milestone-v1.md
- 2026-05-12-asset-identity-v2-reference-cleanup-milestone-v1.md
- Asset Identity v2 Reference Cleanup Milestone Implementation Plan (2026-05-12)
- **Goal:** Close the `asset-identity-v2` `scene/render/UI/gameplay reference cleanup` blocker by adding testable Asset Identity v2 provenance at runtime-scene boundaries and by requiring generated/game source package references to derive from `AssetKeyV2`.
- **Plan ID:** `asset-identity-v2-reference-cleanup-milestone-v1`
- **Status:** Completed.
- **Gap:** `asset-identity-v2`

### 2026-05-12-asset-identity-v2-scene-runtime-reference-placement-evidence-v1.md
- 2026-05-12-asset-identity-v2-scene-runtime-reference-placement-evidence-v1.md
- Asset Identity v2 Scene Runtime Reference Placement Evidence v1 Implementation Plan (2026-05-12)
- **Plan ID:** `asset-identity-v2-scene-runtime-reference-placement-evidence-v1`
- **Status:** Completed
- **Goal:** Add deterministic Asset Identity v2 placement evidence to the reviewed Scene v2 runtime package migration surface so scene-side mesh/material/sprite references prove which stable `AssetKeyV2` rows produced the runtime `AssetId` values.

### 2026-05-12-editor-content-browser-source-registry-population-v1.md
- 2026-05-12-editor-content-browser-source-registry-population-v1.md
- Editor Content Browser Source Registry Population v1 Implementation Plan (2026-05-12)
- **Plan ID:** `editor-content-browser-source-registry-population-v1`
- **Status:** Completed
- **Goal:** Let the GUI-independent editor Content Browser populate rows directly from `GameEngine.SourceAssetRegistry.v1` while preserving existing `AssetId` selection and retained Assets panel behavior.

### 2026-05-12-editor-productization-1-0-scope-closeout-v1.md
- 2026-05-12-editor-productization-1-0-scope-closeout-v1.md
- Editor Productization 1.0 Scope Closeout v1 (2026-05-12)
- **Plan ID:** `editor-productization-1-0-scope-closeout-v1`
- **Status:** Completed
- **Gap:** `editor-productization`

### 2026-05-12-editor-source-registry-visible-content-browser-v1.md
- 2026-05-12-editor-source-registry-visible-content-browser-v1.md
- Editor Source Registry Visible Content Browser v1 Implementation Plan
- **Goal:** Make the visible `MK_editor` Assets panel load the project-owned `GameEngine.SourceAssetRegistry.v1` file and show source-registry-backed Content Browser rows without falling back to hard-coded cooked registry rows when a reviewed registry is available.

### 2026-05-12-runtime-resource-v2-resident-package-replacement-commit-v1.md
- 2026-05-12-runtime-resource-v2-resident-package-replacement-commit-v1.md
- Runtime Resource v2 Resident Package Replacement Commit v1 (2026-05-12)
- **Status:** Completed
- **Gap:** `runtime-resource-v2` foundation follow-up

### 2026-05-12-runtime-resource-v2-resident-unmount-cache-refresh-v1.md
- 2026-05-12-runtime-resource-v2-resident-unmount-cache-refresh-v1.md
- Runtime Resource v2 Resident Unmount Cache Refresh v1 (2026-05-12)
- **Plan ID:** `runtime-resource-v2-resident-unmount-cache-refresh-v1`
- **Status:** Completed

### 2026-05-12-runtime-resource-v2-streaming-resident-mount-commit-v1.md
- 2026-05-12-runtime-resource-v2-streaming-resident-mount-commit-v1.md
- Runtime Resource v2 Streaming Resident Mount Commit v1 (2026-05-12)
- **Plan ID:** `runtime-resource-v2-streaming-resident-mount-commit-v1`
- **Status:** Completed

### 2026-05-15-runtime-package-streaming-resident-replace-v1.md
- 2026-05-15-runtime-package-streaming-resident-replace-v1.md
- Runtime Package Streaming Resident Replace v1 (2026-05-15)
- **Plan ID:** `runtime-package-streaming-resident-replace-v1`

### 2026-05-16-frame-graph-postprocess-scene-pass-ownership-v1.md
- 2026-05-16-frame-graph-postprocess-scene-pass-ownership-v1.md
- Frame Graph Postprocess Scene Pass Ownership v1 Implementation Plan
- **Goal:** Move `RhiPostprocessFrameRenderer` scene pass command recording under `execute_frame_graph_rhi_texture_schedule` without broad renderer-wide migration.
- **Plan ID:** `frame-graph-postprocess-scene-pass-ownership-v1`
- **Status:** Completed.

### 2026-05-16-frame-graph-rhi-pass-target-access-validation-v1.md
- 2026-05-16-frame-graph-rhi-pass-target-access-validation-v1.md
- Frame Graph RHI Pass Target Access Validation v1 Implementation Plan
- **Goal:** Make frame graph RHI pass target-state rows fail closed unless they match a declared writer access for the same pass/resource pair.
- **Plan ID:** `frame-graph-rhi-pass-target-access-validation-v1`
- **Status:** Completed.

### 2026-05-16-frame-graph-rhi-pass-target-state-execution-v1.md
- 2026-05-16-frame-graph-rhi-pass-target-state-execution-v1.md
- Frame Graph RHI Pass Target State Execution v1 Implementation Plan
- **Goal:** Move Frame Graph RHI texture execution from inter-pass/final-state barriers only to also owning declared per-pass writer texture preparation before pass callbacks run.
- **Plan ID:** `frame-graph-rhi-pass-target-state-execution-v1`
- **Status:** Completed.

### 2026-05-16-frame-graph-rhi-primary-pass-ownership-v1.md
- 2026-05-16-frame-graph-rhi-primary-pass-ownership-v1.md
- Frame Graph RHI Primary Pass Ownership v1 Implementation Plan
- **Goal:** Move `RhiFrameRenderer` primary color pass timing into the Frame Graph RHI executor so the simple renderer path participates in production pass ownership without native handle exposure.
- **Status:** Completed.

### 2026-05-16-frame-graph-shadow-scratch-color-target-state-ownership-v1.md
- 2026-05-16-frame-graph-shadow-scratch-color-target-state-ownership-v1.md
- Frame Graph Shadow Scratch Color Target-State Ownership v1 Implementation Plan
- **Goal:** Move the `RhiDirectionalShadowSmokeFrameRenderer` `shadow_color` scratch render-target first-use state preparation under `execute_frame_graph_rhi_texture_schedule`.
- **Plan ID:** `frame-graph-shadow-scratch-color-target-state-ownership-v1`
- **Status:** Completed.

### 2026-05-16-frame-graph-shared-texture-state-handoff-v1.md
- 2026-05-16-frame-graph-shared-texture-state-handoff-v1.md
- Frame Graph Shared Texture State Handoff v1 Implementation Plan
- **Goal:** Make Frame Graph RHI texture barrier/pass-target/final-state execution track state by shared `TextureHandle` when multiple frame-graph resource binding rows intentionally refer to the same backend-neutral transient texture lease.
- **Status:** Completed.

### 2026-05-16-frame-graph-transient-texture-alias-plan-v1.md
- 2026-05-16-frame-graph-transient-texture-alias-plan-v1.md
- Frame Graph Transient Texture Alias Planning v1 Implementation Plan
- **Goal:** Add a backend-neutral Frame Graph v1 transient texture lifetime and alias planning contract that can prove conservative alias groups before native heap aliasing execution exists.

### 2026-05-16-frame-graph-transient-texture-lease-binding-v1.md
- 2026-05-16-frame-graph-transient-texture-lease-binding-v1.md
- Frame Graph Transient Texture Lease Binding v1 Implementation Plan
- **Goal:** Bind the existing Frame Graph transient texture alias plan to backend-neutral `IRhiDevice` transient texture leases without claiming native heap aliasing or backend aliasing barriers.
- **Status:** Completed.

### 2026-05-16-frame-graph-viewport-surface-color-state-executor-v1.md
- 2026-05-16-frame-graph-viewport-surface-color-state-executor-v1.md
- Frame Graph Viewport Surface Color State Executor v1 Implementation Plan
- **Goal:** Route `RhiViewportSurface` color-target state changes through the Frame Graph RHI texture executor so high-level renderer sources no longer call `IRhiCommandList::transition_texture` directly outside `frame_graph_rhi.cpp`.
- **Status:** Completed.

### 2026-05-16-renderer-rhi-resource-foundation-1-0-scope-closeout-v1.md
- 2026-05-16-renderer-rhi-resource-foundation-1-0-scope-closeout-v1.md
- Renderer RHI Resource Foundation 1.0 Scope Closeout v1 Implementation Plan
- **Goal:** Close `renderer-rhi-resource-foundation` for the Engine 1.0 Windows-default renderer/RHI foundation surface without broadening frame graph, upload/staging, Metal, allocator-enforcement, native-handle, or renderer-quality claims.
- Static guard literals:
  - `$($check.Path) renderer-rhi-resource-foundation closeout evidence`
  - `D3D12/Vulkan deferred native teardown`
  - `Metal IRhiDevice parity`
  - `Renderer RHI Resource Foundation 1.0 Scope Closeout`
  - `RhiDeviceMemoryDiagnostics`
  - `engine manifest aiOperableProductionLoop frame-graph-v1 gap must leave unsupportedProductionGaps after 1.0 scope closeout`
  - `engine/agent/manifest.json aiOperableProductionLoop frame-graph-v1 gap must leave unsupportedProductionGaps after 1.0 scope closeout`
  - `frame-graph-v1`
  - `upload-staging-v1`

### 2026-05-16-runtime-resource-v2-1-0-scope-closeout-v1.md
- 2026-05-16-runtime-resource-v2-1-0-scope-closeout-v1.md
- Runtime Resource v2 1.0 Scope Closeout Implementation Plan
- **Goal:** Close the `runtime-resource-v2` 1.0 foundation follow-up by recording the implemented reviewed safe-point, residency, streaming, and hot-reload surfaces without broadening claims beyond evidence.
- **Plan ID:** `runtime-resource-v2-1-0-scope-closeout-v1`
- **Status:** Completed.
- **Gap:** `runtime-resource-v2`
- Static guard literals:
  - `$($check.Path) runtime-resource-v2 closeout evidence`
  - `Runtime Resource v2 1.0 Scope Closeout`
  - `engine/agent/manifest.json aiOperableProductionLoop renderer-rhi-resource-foundation gap must leave unsupportedProductionGaps after 1.0 scope closeout`
  - `native file watcher ownership`
  - `renderer-rhi-resource-foundation`

### 2026-05-17-frame-graph-automatic-aliasing-barrier-insertion-v1.md
- 2026-05-17-frame-graph-automatic-aliasing-barrier-insertion-v1.md
- Frame Graph Automatic Aliasing Barrier Insertion v1 Implementation Plan

### 2026-05-17-frame-graph-d3d12-overlapping-placed-texture-alias-execution-v1.md
- 2026-05-17-frame-graph-d3d12-overlapping-placed-texture-alias-execution-v1.md
- Frame Graph D3D12 Overlapping Placed Texture Alias Execution v1 Implementation Plan
- **Goal:** Prove D3D12 backend-private overlapping placed texture alias barrier recording and submit-time state bookkeeping with distinct resource handles and non-null aliasing barriers.
- **Status:** Completed.

### 2026-05-17-frame-graph-d3d12-placed-transient-texture-lease-v1.md
- 2026-05-17-frame-graph-d3d12-placed-transient-texture-lease-v1.md
- Frame Graph D3D12 Placed Transient Texture Lease v1 Implementation Plan
- **Goal:** Back D3D12 `IRhiDevice::acquire_transient_texture` leases with backend-private native heaps and placed resources.
- **Status:** Completed.

### 2026-05-17-frame-graph-directional-shadow-render-pass-envelope-v1.md
- 2026-05-17-frame-graph-directional-shadow-render-pass-envelope-v1.md
- Frame Graph Directional Shadow Render Pass Envelope v1 (2026-05-17)
- **Plan ID:** `frame-graph-directional-shadow-render-pass-envelope-v1`
- **Status:** Completed slice for `frame-graph-v1`.

### 2026-05-17-frame-graph-multiqueue-package-evidence-v1.md
- 2026-05-17-frame-graph-multiqueue-package-evidence-v1.md
- Frame Graph Multi-Queue Package Evidence v1 (2026-05-17)
- **Plan ID:** `frame-graph-multiqueue-package-evidence-v1`
- **Status:** Completed slice under `frame-graph-v1`.

### 2026-05-17-frame-graph-package-streaming-texture-binding-handoff-v1.md
- 2026-05-17-frame-graph-package-streaming-texture-binding-handoff-v1.md
- Frame Graph Package Streaming Texture Binding Handoff v1 Implementation Plan (2026-05-17)
- **Plan ID:** `frame-graph-package-streaming-texture-binding-handoff-v1`
- **Status:** Completed.

### 2026-05-17-frame-graph-primary-pass-target-state-evidence-v1.md
- 2026-05-17-frame-graph-primary-pass-target-state-evidence-v1.md
- Frame Graph Primary Pass Target-State Evidence v1 - 2026-05-17

### 2026-05-17-frame-graph-public-null-aliasing-barriers-v1.md
- 2026-05-17-frame-graph-public-null-aliasing-barriers-v1.md
- Frame Graph Public Null Aliasing Barriers v1
- **Goal:** Promote public wildcard/null texture aliasing barriers from unsupported to a backend-neutral Frame Graph/RHI contract.

### 2026-05-17-frame-graph-remaining-render-pass-envelopes-v1.md
- 2026-05-17-frame-graph-remaining-render-pass-envelopes-v1.md
- Frame Graph Remaining Render Pass Envelopes v1 (2026-05-17)
- **Plan ID:** `frame-graph-remaining-render-pass-envelopes-v1`
- **Status:** Completed slice for `frame-graph-v1`.

### 2026-05-17-frame-graph-render-pass-envelope-v1.md
- 2026-05-17-frame-graph-render-pass-envelope-v1.md
- Frame Graph Render Pass Envelope v1 (2026-05-17)
- **Plan ID:** `frame-graph-render-pass-envelope-v1`
- **Status:** Completed.

### 2026-05-17-frame-graph-render-pass-package-evidence-v1.md
- 2026-05-17-frame-graph-render-pass-package-evidence-v1.md
- 2026-05-17 Frame Graph Render Pass Package Evidence v1

### 2026-05-17-frame-graph-render-pass-stats-evidence-v1.md
- 2026-05-17-frame-graph-render-pass-stats-evidence-v1.md
- Frame Graph Render Pass Stats Evidence v1 (2026-05-17)
- **Plan ID:** `frame-graph-render-pass-stats-evidence-v1`
- **Status:** Completed.

### 2026-05-17-frame-graph-rhi-multiqueue-executor-v1.md
- 2026-05-17-frame-graph-rhi-multiqueue-executor-v1.md
- Frame Graph RHI Multi-Queue Executor v1
- **Goal:** Add a backend-neutral Frame Graph RHI executor that opens native RHI command lists on declared pass queues, submits each pass, and records cross-queue waits only after producer pass fences exist.

### 2026-05-17-frame-graph-rhi-multiqueue-texture-barrier-execution-v1.md
- 2026-05-17-frame-graph-rhi-multiqueue-texture-barrier-execution-v1.md
- Frame Graph RHI Multi-Queue Texture Barrier Execution v1
- **Goal:** Let the backend-neutral Frame Graph RHI multi-queue executor optionally record scheduled texture barriers on the consumer pass command list after producer queue waits and before the consumer callback.

### 2026-05-17-frame-graph-rhi-queue-dependency-plan-v1.md
- 2026-05-17-frame-graph-rhi-queue-dependency-plan-v1.md
- Frame Graph RHI Queue Dependency Plan v1 - 2026-05-17

### 2026-05-17-runtime-mesh-frame-graph-command-evidence-v1.md
- 2026-05-17-runtime-mesh-frame-graph-command-evidence-v1.md
- Runtime Mesh Frame Graph Command Evidence v1 (2026-05-17)
- **Status:** Completed
- **Gap:** `frame-graph-v1`

### 2026-05-17-runtime-morph-mesh-frame-graph-command-evidence-v1.md
- 2026-05-17-runtime-morph-mesh-frame-graph-command-evidence-v1.md
- Runtime Morph Mesh Frame Graph Command Evidence v1 (2026-05-17)
- **Status:** Completed
- **Gap:** `frame-graph-v1`

### 2026-05-17-runtime-skinned-mesh-frame-graph-command-evidence-v1.md
- 2026-05-17-runtime-skinned-mesh-frame-graph-command-evidence-v1.md
- Runtime Skinned Mesh Frame Graph Command Evidence v1 (2026-05-17)
- **Status:** Completed
- **Gap:** `frame-graph-v1`

### 2026-05-17-runtime-upload-frame-graph-transition-evidence-v1.md
- 2026-05-17-runtime-upload-frame-graph-transition-evidence-v1.md
- Runtime Upload Frame Graph Transition Evidence v1 (2026-05-17)
- **Plan ID:** `runtime-upload-frame-graph-transition-evidence-v1`
- **Status:** Completed slice under `frame-graph-v1`.

### 2026-05-18-2d-playable-vertical-slice-1-0-closeout-v1.md
- 2026-05-18-2d-playable-vertical-slice-1-0-closeout-v1.md
- 2D Playable Vertical Slice 1.0 Closeout v1 (2026-05-18)
- **Gap:** `2d-playable-vertical-slice`
- **Status:** Completed

### 2026-05-18-3d-playable-vertical-slice-1-0-closeout-v1.md
- 2026-05-18-3d-playable-vertical-slice-1-0-closeout-v1.md
- 3D Playable Vertical Slice 1.0 Closeout v1 (2026-05-18)
- **Gap:** `3d-playable-vertical-slice`
- **Status:** Completed

### 2026-05-18-frame-graph-multiqueue-render-pass-execution-v1.md
- 2026-05-18-frame-graph-multiqueue-render-pass-execution-v1.md
- 2026-05-18 Frame Graph Multi-Queue Render Pass Execution v1
- **Plan ID:** `frame-graph-multiqueue-render-pass-execution-v1`
- **Status:** Completed.

### 2026-05-18-frame-graph-transient-alias-content-initialization-v1.md
- 2026-05-18-frame-graph-transient-alias-content-initialization-v1.md
- 2026-05-18 Frame Graph Transient Alias Content Initialization v1

### 2026-05-18-frame-graph-v1-production-ownership-milestone-v1.md
- 2026-05-18-frame-graph-v1-production-ownership-milestone-v1.md
- 2026-05-18 Frame Graph v1 Production Ownership Milestone v1
- **Status:** Completed.

### 2026-05-18-production-ui-importer-platform-adapters-1-0-closeout-v1.md
- 2026-05-18-production-ui-importer-platform-adapters-1-0-closeout-v1.md
- Production UI Importer Platform Adapters 1.0 Closeout v1 Implementation Plan
- **Goal:** Close `production-ui-importer-platform-adapters` for the Engine 1.0 Windows-default ready surface without claiming broad low-level UI, codec, or platform-service parity.
- **Plan ID:** `production-ui-importer-platform-adapters-1-0-closeout-v1`
- **Status:** Completed
- reviewed adapter-boundary and package evidence
- AccessibilityPublishPlan
- ImeCompositionPublishPlan
- PlatformTextInputSessionPlan
- TextShapingRequestPlan
- FontRasterizationRequestPlan
- ImageDecodeRequestPlan
- PngImageDecodingAdapter
- author_packed_ui_atlas_from_decoded_images
- author_packed_ui_glyph_atlas_from_rasterized_glyphs
- selected SDL3 platform bridges
- package-visible native UI overlay/atlas smokes
- production text shaping implementation
- real font loading/rasterization
- OS accessibility publication
- broad native IME/text services
- broader source codecs
- SVG/vector parsing
- renderer texture-upload APIs
- arbitrary importer adapters
- UI middleware
- full-repository-quality-gate

### 2026-05-18-runtime-package-streaming-rhi-upload-binding-transaction-v1.md
- 2026-05-18-runtime-package-streaming-rhi-upload-binding-transaction-v1.md
- Runtime Package Streaming RHI Upload Binding Transaction v1 - 2026-05-18

### 2026-05-18-scene-v2-command-authored-runtime-workflow-validation-v1.md
- 2026-05-18-scene-v2-command-authored-runtime-workflow-validation-v1.md
- Scene v2 Command-Authored Runtime Workflow Validation v1
- **Status:** Completed.
- **Goal:** Narrow `scene-component-prefab-schema-v2` by proving reviewed Scene/Prefab v2 authoring commands can feed source asset registration, registered cook, Scene v2 runtime migration, and non-mutating runtime scene package validation in one host-independent workflow.

### 2026-05-18-scene-v2-prefab-refresh-apply-v1.md
- 2026-05-18-scene-v2-prefab-refresh-apply-v1.md
- Scene v2 Prefab Refresh Apply v1 (2026-05-18)
- **Plan ID:** `scene-v2-prefab-refresh-apply-v1`
- **Status:** Completed

### 2026-05-18-scene-v2-prefab-refresh-command-surface-v1.md
- 2026-05-18-scene-v2-prefab-refresh-command-surface-v1.md
- Scene v2 Prefab Refresh Command Surface v1 - 2026-05-18

### 2026-05-18-scene-v2-prefab-refresh-integrity-v1.md
- 2026-05-18-scene-v2-prefab-refresh-integrity-v1.md
- Scene v2 Prefab Refresh Integrity v1 (2026-05-18)
- **Plan ID:** `scene-v2-prefab-refresh-integrity-v1`
- **Status:** Completed
- **Gap:** `scene-component-prefab-schema-v2`

### 2026-05-18-scene-v2-prefab-refresh-local-ownership-guard-v1.md
- 2026-05-18-scene-v2-prefab-refresh-local-ownership-guard-v1.md
- Scene v2 Prefab Refresh Local Ownership Guard v1 - 2026-05-18
- **Status:** Completed

### 2026-05-18-scene-v2-prefab-refresh-nested-guard-v1.md
- 2026-05-18-scene-v2-prefab-refresh-nested-guard-v1.md
- Scene v2 Prefab Refresh Nested Guard v1 (2026-05-18)
- **Plan ID:** `scene-v2-prefab-refresh-nested-guard-v1`
- **Status:** Completed
- **Gap:** `scene-component-prefab-schema-v2`

### 2026-05-18-scene-v2-prefab-source-provenance-v1.md
- 2026-05-18-scene-v2-prefab-source-provenance-v1.md
- Scene v2 Prefab Source Provenance v1 (2026-05-18)
- **Plan ID:** `scene-v2-prefab-source-provenance-v1`
- **Status:** Completed
- **Gap:** `scene-component-prefab-schema-v2`

### 2026-05-18-scene-v2-stable-id-prefab-refresh-plan-v1.md
- 2026-05-18-scene-v2-stable-id-prefab-refresh-plan-v1.md
- Scene v2 Stable-Id Prefab Refresh Plan v1 (2026-05-18)
- **Plan ID:** `scene-v2-stable-id-prefab-refresh-plan-v1`
- **Status:** Completed
- **Gap:** `scene-component-prefab-schema-v2`

### 2026-05-18-upload-staging-v1-async-ready-resource-updates-v1.md
- 2026-05-18-upload-staging-v1-async-ready-resource-updates-v1.md
- Upload Staging v1 Async-Ready Resource Updates Implementation Plan
- **Status:** Completed.
- **Goal:** Close the remaining `upload-staging-v1` ready-claim blocker by proving selected package resource updates are ready only after submitted upload fences are available and graphics-queue consumption waits are recorded.
- Static guard literals:
  - ` `
  - `$($check.Path) upload-staging-v1 closeout evidence`
  - `2026-05-18-upload-staging-v1-package-static-mesh-upload-binding-transaction-v1.md`
  - `2026-05-18-upload-staging-v1-runtime-buffer-ring-backed-uploads-v1.md`
  - `2026-05-18-upload-staging-v1-runtime-ring-backed-texture-upload-v1.md`
  - `2026-05-18-upload-staging-v1-runtime-upload-queue-wait-v1.md`
  - `2026-05-18-upload-staging-v1-staging-pool-lease-adoption-v1.md`
  - `2d-playable-vertical-slice`
  - `3d-playable-vertical-slice`
  - `6 pass callbacks/15 barrier steps`
  - `Frame Graph Automatic Aliasing Barrier Insertion v1`
  - `Frame Graph Backend-Neutral Distinct Alias-Group Lease Binding v1`
  - `Frame Graph D3D12 Texture Aliasing Barrier Evidence v1`
  - `Frame Graph RHI Multi-Queue Executor v1`
  - `Frame Graph RHI Multi-Queue Texture Barrier Execution v1`
  - `Frame Graph RHI Pass Target Access Validation v1`
  - `Frame Graph RHI Queue Dependency Plan v1`
  - `Frame Graph Remaining Render Pass Envelopes v1`
  - `Frame Graph Render Pass Envelope v1`
  - `Frame Graph Shadow Scratch Color Target-State Ownership v1`
  - `Frame Graph Shared Texture State Handoff v1`
  - `Frame Graph Texture Aliasing Barrier Command v1`
  - `Frame Graph Transient Texture Alias Planning v1`
  - `Frame Graph Viewport Surface Color State Executor v1`
  - `Frame Graph v1 1.0 Scope Closeout v1 closes frame-graph-v1`
  - `FrameGraphRhiMultiQueueExecutionResult::barriers_recorded`
  - `FrameGraphTexturePassTargetAccess`
  - `FrameGraphTransientTextureAliasPlan`
  - `FrameGraphTransientTextureLeaseBindingResult`
  - `IRhiDevice::acquire_transient_texture_alias_group`
  - `IRhiDevice::wait_for_queue`
  - `NavigationNavmeshPathRequest`
  - `Package Static Mesh Upload Binding Transaction v1`
  - `Package Streaming Frame Graph Texture Binding Handoff v1`
  - `RhiPostprocessFrameRenderer scene pass command recording`
  - `RhiStagingBufferLease`
  - `RhiUploadRingDesc::buffer`
  - `RhiViewportSurface`
  - `Runtime Buffer Ring-Backed Uploads v1`
  - `Runtime Package Streaming RHI Upload Binding Transaction v1`
  - `Runtime Ring-Backed Texture Upload v1`
  - `Runtime Upload Queue Wait v1`
  - `RuntimeMeshUploadOptions::upload_ring`
  - `RuntimeMorphMeshUploadOptions::upload_ring`
  - `RuntimePackageResourceUpdateReadinessResult`
  - `RuntimePackageStreamingMeshUploadBindingResult`
  - `RuntimePackageStreamingMeshUploadSource`
  - `RuntimeSkinnedMeshUploadOptions::upload_ring`
  - `RuntimeTextureUploadOptions::upload_ring`
  - `Staging Pool Lease Adoption v1`
  - `Upload Staging v1 Async-Ready Resource Updates`
  - `acquire_frame_graph_transient_texture_lease_bindings`
  - `broad/background streaming`
  - `build_frame_graph_texture_pass_target_accesses`
  - `conflicting initial shared-handle states`
  - `editor-productization`
  - `engine manifest aiOperableProductionLoop 3d-playable-vertical-slice gap must leave unsupportedProductionGaps after 1.0 closeout`
  - `engine manifest aiOperableProductionLoop physics-1-0-collision-system gap must leave unsupportedProductionGaps after Physics 1.0 closeout`
  - `engine manifest aiOperableProductionLoop recommendedNextPlan 3d closeout`
  - `engine manifest aiOperableProductionLoop recommendedNextPlan must describe frame-graph closeout and upload-staging next gap: $needle`
  - `engine/agent/manifest.json aiOperableProductionLoop $closedGameplayGapId gap must leave unsupportedProductionGaps after gameplay physics/navigation closeout`
  - `engine/agent/manifest.json aiOperableProductionLoop physics-1-0-collision-system gap must leave unsupportedProductionGaps after Physics 1.0 closeout`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan 3d closeout`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan automatic aliasing barrier`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan gameplay closeout evidence`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package static mesh upload transaction`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package streaming handoff`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package streaming upload transaction`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime buffer ring-backed uploads`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime ring-backed texture upload`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime upload queue wait`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan staging pool lease adoption`
  - `engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext`
  - `evaluate_physics_character_dynamic_policy_3d`
  - `execute_frame_graph_rhi_multi_queue_schedule`
  - `frame-graph-shadow-scratch-color-target-state-ownership-v1`
  - `gameplay-2d-3d-package-evidence`
  - `gameplay-physics-navigation-ai-foundation-v1`
  - `generated desktop 3D package proof`
  - `host-gated D3D12/Vulkan package smokes`
  - `make_runtime_package_resource_update_readiness`
  - `make_runtime_package_streaming_frame_graph_texture_bindings`
  - `native UI overlay/atlas package counters`
  - `navigation-navmesh-and-dynamic-obstacle-follow-up`
  - `null_resource_aliasing_barriers`
  - `package_upload_staging_resource_updates_ready`
  - `physics-1-0-collision-system`
  - `physics-advanced-dynamics-follow-up`
  - `plan_frame_graph_rhi_queue_waits`
  - `plan_frame_graph_transient_texture_aliases`
  - `plan_navigation_navmesh_path`
  - `record_frame_graph_texture_aliasing_barriers`
  - `refresh-prefab-instance`
  - `render_passes_recorded`
  - `scene-component-prefab-schema-v2`
  - `selected generated 2D and 3D package gameplay systems composition smokes`
  - `shadow_color`
  - `upload-staging-v1`
  - `upload_queue_waits_recorded`
  - `upload_runtime_package_streaming_frame_graph_texture_bindings`
  - `upload_runtime_package_streaming_mesh_gpu_bindings`
  - `viewport_color`
  - `visible 3D aggregate counters`
  - `wait_for_runtime_uploads_on_queue`

### 2026-05-18-upload-staging-v1-package-static-mesh-upload-binding-transaction-v1.md
- 2026-05-18-upload-staging-v1-package-static-mesh-upload-binding-transaction-v1.md
- Upload Staging v1 Package Static Mesh Upload Binding Transaction v1 Implementation Plan
- **Goal:** Add a host-owned package streaming transaction for resident static mesh payload uploads and renderer mesh bindings.

### 2026-05-18-upload-staging-v1-runtime-buffer-ring-backed-uploads-v1.md
- 2026-05-18-upload-staging-v1-runtime-buffer-ring-backed-uploads-v1.md
- Upload Staging v1 Runtime Buffer Ring-Backed Uploads v1 Implementation Plan
- **Goal:** Add opt-in caller-owned upload-ring staging for runtime mesh, skinned mesh, and morph mesh buffer uploads.

### 2026-05-18-upload-staging-v1-runtime-ring-backed-texture-upload-v1.md
- 2026-05-18-upload-staging-v1-runtime-ring-backed-texture-upload-v1.md
- Upload Staging v1 Runtime Ring-Backed Texture Upload v1 Implementation Plan
- **Goal:** Add an opt-in runtime texture upload path that uses a caller-owned `RhiUploadRing` and `RhiUploadStagingPlan` rows instead of allocating one staging buffer per texture upload.

### 2026-05-18-upload-staging-v1-runtime-upload-queue-wait-v1.md
- 2026-05-18-upload-staging-v1-runtime-upload-queue-wait-v1.md
- Upload Staging v1 Runtime Upload Queue Wait v1 - 2026-05-18
- **Status:** Completed.
- **Goal:** Add runtime/package upload queue-consumption evidence so async copy-queue uploads can be made visible to graphics consumers through backend-neutral GPU-side queue waits.

### 2026-05-18-upload-staging-v1-staging-pool-lease-adoption-v1.md
- 2026-05-18-upload-staging-v1-staging-pool-lease-adoption-v1.md
- Upload Staging v1 Staging Pool Lease Adoption v1 - 2026-05-18
- **Status:** Completed.
- **Goal:** Let host-owned staging-pool chunks back `RhiUploadRing` uploads so selected package upload transactions can reuse pooled staging buffers without creating per-transaction upload buffers.

### 2026-05-19-engine-save-settings-profile-v1.md
- 2026-05-19-engine-save-settings-profile-v1.md
- Engine Save Settings Profile v1 (2026-05-19)
- **Plan ID:** `engine-save-settings-profile-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.

### 2026-05-19-engine-ui-game-menu-hud-v1.md
- 2026-05-19-engine-ui-game-menu-hud-v1.md
- Engine UI Game Menu HUD v1 (2026-05-19)
- **Plan ID:** `engine-ui-game-menu-hud-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.

### 2026-05-19-gameplay-authoring-foundation-v1.md
- 2026-05-19-gameplay-authoring-foundation-v1.md
- Gameplay Authoring Foundation v1 (2026-05-19)
- **Plan ID:** `gameplay-authoring-foundation-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Not selected.

### 2026-05-20-engine-asset-placeholder-generation-v1.md
- 2026-05-20-engine-asset-placeholder-generation-v1.md
- Engine Asset Placeholder Generation v1 (2026-05-20)
- **Plan ID:** `engine-asset-placeholder-generation-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.

### 2026-05-20-engine-gameplay-debug-overlay-v1.md
- 2026-05-20-engine-gameplay-debug-overlay-v1.md
- Engine Gameplay Debug Overlay v1 (2026-05-20)
- **Plan ID:** `engine-gameplay-debug-overlay-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- Static guard literals:
  - `RuntimeGameplayDebugOverlayPlan`
  - `plan_runtime_gameplay_debug_overlay`

### 2026-05-20-engine-inventory-items-crafting-v1.md
- 2026-05-20-engine-inventory-items-crafting-v1.md
- Engine Inventory Items Crafting v1 (2026-05-20)
- **Plan ID:** `engine-inventory-items-crafting-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.

### 2026-05-20-engine-quest-dialogue-state-v1.md
- 2026-05-20-engine-quest-dialogue-state-v1.md
- Engine Quest Dialogue State v1 (2026-05-20)
- **Plan ID:** `engine-quest-dialogue-state-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.

### 2026-05-20-sprite-animation-flipbook-v1.md
- 2026-05-20-sprite-animation-flipbook-v1.md
- Sprite Animation Flipbook v1 (2026-05-20)
- **Plan ID:** `sprite-animation-flipbook-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.

### 2026-05-20-sprite-batching-renderer-v1.md
- 2026-05-20-sprite-batching-renderer-v1.md
- Sprite Batching Renderer v1 (2026-05-20)
- **Plan ID:** `sprite-batching-renderer-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.

### 2026-05-21-engine-construction-placement-v1.md
- 2026-05-21-engine-construction-placement-v1.md
- Engine Construction Placement v1 (2026-05-21)
- **Plan ID:** `engine-construction-placement-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.

### 2026-05-21-engine-procedural-generation-v1.md
- 2026-05-21-engine-procedural-generation-v1.md
- Engine Procedural Generation v1 (2026-05-21)
- **Plan ID:** `engine-procedural-generation-v1`
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
- **Status:** Completed.
