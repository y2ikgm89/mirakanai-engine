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
- `2d Packaged Playable Generation Loop v1`
- `Editor Playtest Package Review Loop v1`
- `Renderer Resource Residency Upload Execution v1`
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
- `Editor Resource Capture Execution Evidence v1`
- `Editor Resource Capture Request v1`
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
- `Editor Prefab Instance Local Child Refresh Resolution v1`
- `2026-05-09-editor-prefab-instance-stale-node-refresh-resolution-v1.md`
- `2026-05-09-editor-prefab-variant-base-refresh-merge-review-v1.md`
- `Editor Scene Prefab Instance Refresh Review v1`
- `Editor Input Rebinding Axis Capture Gamepad v1`
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

## Retired retained plan corpus (2026-05-22 plan volume compression)

This section preserves minimal current static-check evidence for retained dated plans removed from `docs/superpowers/plans/` during the plan-volume cleanup. Detailed execution prose remains available through Git history.

### Engine Excellence Roadmap
- Retired file: `Engine Excellence Roadmap`
- Engine Excellence Roadmap Implementation Plan (2026-04-26)
- ## Plan Status
- **Goal:** Turn the current C++23 core-first MVP into a production-grade desktop/mobile game engine that supports real 2D/3D rendering, editor workflows, asset import/cooking, gameplay systems, mobile packaging, release packaging, and AI-driven game creation through Codex and Claude Code.

### 2d Desktop Runtime Package Proof v1
- Retired file: `2d Desktop Runtime Package Proof v1`
- 2D Desktop Runtime Package Proof v1 Implementation Plan (2026-05-01)
- - a host-gated 2D desktop runtime package recipe or package proof status
- **Goal:** Promote the validated source-tree 2D playable foundation into an honest host-gated desktop runtime/package proof, while keeping gameplay on first-party `mirakana::` APIs and native presentation behind engine host adapters.

### 2d Playable Vertical Slice Foundation v1
- Retired file: `2d Playable Vertical Slice Foundation v1`
- 2D Playable Vertical Slice Foundation v1 Implementation Plan (2026-05-01)
- **Goal:** Promote the planned 2D generated-game path toward a validated, AI-operable playable vertical slice using first-party C++23 engine contracts, without exposing SDL3, Dear ImGui, native OS handles, graphics API handles, or RHI backend handles to gameplay code.

### 3d Playable Vertical Slice Foundation v1
- Retired file: `3d Playable Vertical Slice Foundation v1`
- 3D Playable Vertical Slice Foundation v1 Implementation Plan (2026-05-01)
- Selected boundary: use the existing `sample_desktop_runtime_game` as the first 3D proof and expose it through the new host-gated recipe id `3d-playable-desktop-package`. The proof is not a broad generated-game production renderer claim. It is a desktop package foundation with cooked config, `.geindex`, texture, mesh, material, scene, target-specific D3D12 scene/postprocess/shadow DXIL artifacts, strict host/toolchain/runtime-gated Vulkan SPIR-V artifacts, static mesh scene submission, material instance intent, camera/controller status, light/shadow/postprocess metadata, HUD diagnostics, and deterministic smoke output. `future-3d-playable-vertical-slice` stays planned for the broader production 3D path.
- **Goal:** Promote the planned 3D generated-game path into an honest first 3D playable recipe using cooked packages, first-party scene/material/runtime contracts, and host-gated desktop runtime validation without exposing native or RHI handles to gameplay code.

### AI Command Surface Foundation v1
- Retired file: `AI Command Surface Foundation v1`
- AI Command Surface Foundation v1 Implementation Plan (2026-05-01)
- **Goal:** Add the first backend-neutral AI command surface contract so agents can dry-run and report safe authoring/package/game mutations without scraping prose or bypassing engine capability gates.

### Asset Identity Runtime Resource v2
- Retired file: `Asset Identity Runtime Resource v2`
- Asset Identity Runtime Resource v2 Implementation Plan (2026-05-01)
- - `MK_assets.status` to an Asset Identity v2 foundation status and include `engine/assets/include/mirakana/assets/asset_identity.hpp`.
- **Goal:** Add a small asset identity layer plus generation-checked runtime resource handles so Scene/Component/Prefab Schema v2 can reference assets without depending on importer, package, renderer, or editor internals.

### Frame Graph Upload Staging Foundation v1
- Retired file: `Frame Graph Upload Staging Foundation v1`
- Frame Graph and Upload/Staging Foundation v1 Implementation Plan (2026-05-01)
- **Goal:** Add the next backend-neutral planning layer for resource-declared frame graph execution and upload/staging retirement without exposing native GPU handles or claiming production renderer readiness.

### Game Manifest Runtime Scene Validation Targets v1
- Retired file: `Game Manifest Runtime Scene Validation Targets v1`
- Game Manifest Runtime Scene Validation Targets v1 Implementation Plan (2026-05-01)
- - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS (`status: passed`, `exitCode: 0`)
- **Goal:** Add an AI-readable `game.agent.json` descriptor for explicit runtime scene package validation targets so agents can choose `validate-runtime-scene-package` inputs from a reviewed manifest contract, then hand off directly to packaged 2D playable game generation.

### Material Instance Apply Tooling v1
- Retired file: `Material Instance Apply Tooling v1`
- Material Instance Apply Tooling v1 Implementation Plan (2026-05-01)
- - GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed for the selected D3D12 package with `ui_atlas_metadata_status=ready`, `ui_texture_overlay_status=ready`, and `ui_texture_overlay_draws=2`.
- **Goal:** Add a reviewed dry-run/apply tooling surface for first-party material instance authoring that can create or update explicit `.material` content and matching cooked `.geindex` `AssetKind::material` rows with texture dependency edges, without claiming material graphs, shader graphs, live shader generation, renderer residency, or package streaming.

### Native GPU UI Overlay Foundation v1
- Retired file: `Native GPU UI Overlay Foundation v1`
- Native GPU UI Overlay Foundation v1 Implementation Plan (2026-05-01)
- - deterministic status fields proving overlay requests, submitted box count, native overlay readiness, and native overlay draw count
- **Goal:** Turn first-party runtime UI box/image placeholder submissions from diagnostics-only counters into a narrow native GPU overlay path for desktop runtime scene packages, while keeping gameplay on `mirakana_ui` / `mirakana_ui_renderer` / `mirakana::IRenderer` and keeping SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, and RHI handles private.

### Native UI Atlas Package Metadata Foundation v1
- Retired file: `Native UI Atlas Package Metadata Foundation v1`
- Native UI Atlas Package Metadata Foundation v1 Implementation Plan (2026-05-01)
- - [x] Identify the minimum sample status fields for metadata load/use diagnostics.
- **Goal:** Move the validated native UI textured sprite proof from sample-hardcoded atlas page/UV binding to a first-party cooked package metadata contract that an AI agent can inspect, update, package, and validate without adding runtime source image decoding, production atlas packing, text/font shaping, accessibility bridges, or public native/RHI handles.

### Native UI Textured Sprite Atlas Foundation v1
- Retired file: `Native UI Textured Sprite Atlas Foundation v1`
- Native UI Textured Sprite Atlas Foundation v1 Implementation Plan (2026-05-01)
- - deterministic status fields proving texture overlay requests, atlas page readiness, sprites submitted, texture binds, and draw counts
- **Goal:** Extend the renderer-owned native UI overlay proof from colored box/image-placeholder sprites to a narrow cooked-texture UI image sprite path, using first-party package assets and backend-neutral atlas metadata while keeping source image decoding, production atlas packing, font glyph atlases, text shaping, IME, OS accessibility bridges, and public native/RHI handles out of gameplay-facing APIs.

### Registered Source Asset Cook Package Command Tooling v1
- Retired file: `Registered Source Asset Cook Package Command Tooling v1`
- Registered Source Asset Cook Package Command Tooling v1 Implementation Plan (2026-05-01)
- **Goal:** Add a reviewed AI-safe dry-run/apply command surface that takes explicit `GameEngine.SourceAssetRegistry.v1` rows and updates deterministic cooked artifacts plus a runtime-loadable `.geindex` package through existing import/package helpers.

### Renderer RHI Resource Foundation v1
- Retired file: `Renderer RHI Resource Foundation v1`
- Renderer RHI Resource Foundation v1 Implementation Plan (2026-05-01)
- - `MK_rhi.status` to a Renderer/RHI Resource Foundation v1 status.
- **Goal:** Add a backend-neutral renderer/RHI resource lifetime foundation with explicit resource ids, debug names, deferred release, and deterministic diagnostics without exposing native GPU handles.

### Runtime Scene Package Validation Command Tooling v1
- Retired file: `Runtime Scene Package Validation Command Tooling v1`
- Runtime Scene Package Validation Command Tooling v1 Implementation Plan (2026-05-01)
- **Goal:** Add a reviewed host-independent validation command surface that loads an explicit runtime `.geindex` package and instantiates an explicit scene asset through `mirakana_runtime` and `mirakana_runtime_scene`.

### Scene Component Prefab Schema v2
- Retired file: `Scene Component Prefab Schema v2`
- Scene Component Prefab Schema v2 Implementation Plan (2026-05-01)
- - Modify `engine/agent/manifest.json`: update `MK_scene` purpose/status and `aiOperableProductionLoop.unsupportedProductionGaps`.
- **Goal:** Add the first stable-id, schema-driven scene/component/prefab authoring contract that future AI commands, 2D/3D vertical slices, and editor-core productization can share.

### Scene v2 Registered Asset Runtime Workflow Validation v1
- Retired file: `Scene v2 Registered Asset Runtime Workflow Validation v1`
- Scene v2 Registered Asset Runtime Workflow Validation v1 Implementation Plan (2026-05-01)
- **Goal:** Prove the AI-operable authored-to-runtime workflow by chaining reviewed source asset registration, explicit registered source asset cook/package updates, Scene v2 runtime package migration, and runtime package loading/scene instantiation in focused tests and docs.

### Scene v2 Runtime Package Migration v1
- Retired file: `Scene v2 Runtime Package Migration v1`
- Scene v2 Runtime Package Migration v1 Implementation Plan (2026-05-01)
- **Goal:** Add a reviewed AI-safe dry-run/apply bridge from authored `GameEngine.Scene.v2` documents plus `GameEngine.SourceAssetRegistry.v1` source asset rows into the existing runtime-loadable `GameEngine.Scene.v1` scene package update surface.

### Source Asset Registration Command Tooling v1
- Retired file: `Source Asset Registration Command Tooling v1`
- Source Asset Registration Command Tooling v1 Implementation Plan (2026-05-01)
- - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed with `status: passed` and both `agent-check` and `schema-check` exit code 0.
- **Goal:** Add a reviewed AI-safe dry-run/apply command surface for registering first-party source assets and their deterministic import intent, so agents can connect authored Scene/Prefab v2 data to known texture, mesh, audio, material, and scene sources without broad package cooking, arbitrary filesystem edits, or renderer/RHI residency claims.

### UI Atlas Metadata Apply Tooling v1
- Retired file: `UI Atlas Metadata Apply Tooling v1`
- UI Atlas Metadata Apply Tooling v1 Implementation Plan (2026-05-01)
- - `sample_desktop_runtime_game` still validates D3D12 and strict Vulkan selected package lanes with `ui_atlas_metadata_status=ready` and existing textured overlay fields.
- **Goal:** Add a reviewed dry-run/apply tooling surface that updates cooked `.uiatlas` metadata and matching `.geindex` package rows together, using the validated `GameEngine.UiAtlas.v1` author/verify contracts without source image decoding or production atlas packing.

### UI Atlas Metadata Authoring Tooling v1
- Retired file: `UI Atlas Metadata Authoring Tooling v1`
- UI Atlas Metadata Authoring Tooling v1 Implementation Plan (2026-05-01)
- - `sample_desktop_runtime_game` still validates D3D12 and strict Vulkan selected package lanes with `ui_atlas_metadata_status=ready`, positive metadata page/binding counts, and existing textured overlay fields.
- **Goal:** Make first-party cooked UI atlas metadata authorable and verifiable by AI/tooling without adding runtime source image decoding, production atlas packing, text/font shaping, accessibility bridges, public native/RHI handles, or broad renderer quality claims.

### Validation Recipe Runner Tooling v1
- Retired file: `Validation Recipe Runner Tooling v1`
- Validation Recipe Runner Tooling v1 Implementation Plan (2026-05-01)
- - [x] Define dry-run result fields: `recipe`, `status`, `command`, `argv`, `hostGates`, `diagnostics`, and `blockedBy`.
- **Goal:** Add a reviewed validation-recipe runner surface that lets AI agents dry-run and execute only manifest-declared validation recipes with host-gate diagnostics and no free-form shell input.

### 2d Atlas Tilemap Package Authoring v1
- Retired file: `2d Atlas Tilemap Package Authoring v1`
- 2D Atlas Tilemap Package Authoring v1 Implementation Plan (2026-05-02)
- - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS (`status: passed`, `agent-check` and `schema-check` both exit 0).
- **Goal:** Add a narrow generated 2D atlas/tilemap package authoring loop after host-gated package streaming execution is validated.

### 2d Native RHI Sprite Package Proof v1
- Retired file: `2d Native RHI Sprite Package Proof v1`
- 2D Native RHI Sprite Package Proof v1 (2026-05-02)
- - [x] The host-owned RHI renderer consumes packaged 2D scene sprite texture/material identity through existing scene/runtime package contracts and records native 2D sprite/HUD diagnostics in the package smoke status line.

### 2d Packaged Playable Generation Loop v1
- Retired file: `2d Packaged Playable Generation Loop v1`
- 2D Packaged Playable Generation Loop v1 Implementation Plan (2026-05-02)
- - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS (`status: passed`, `exitCode: 0`)
- **Goal:** Make AI-generated packaged 2D games use reviewed source, scene, cook, package, manifest, runtime validation, and desktop runtime package validation surfaces instead of isolated sample fixtures.

Static guard source snapshot:

```markdown
# 2D Packaged Playable Generation Loop v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make AI-generated packaged 2D games use reviewed source, scene, cook, package, manifest, runtime validation, and desktop runtime package validation surfaces instead of isolated sample fixtures.

**Architecture:** Reuse existing command surfaces first: `register-source-asset`, `cook-registered-source-assets`, `migrate-scene-v2-runtime-package`, `validate-runtime-scene-package`, `register-runtime-package-files`, and `run-validation-recipe`. Keep the first production uplift narrow: one deterministic generated 2D package with sprite, camera, input, HUD intent, audio cue intent, cooked package files, manifest runtime scene validation targets, and package smoke validation. Do not implement broad dependency cooking, package streaming, renderer/RHI residency, production atlas packing, tilemap editor UX, material/shader graphs, live shader generation, editor productization, native handles, Metal readiness, or general renderer quality.

**Tech Stack:** C++23 generated game scaffolding, `tools/new-game.ps1`, `game.agent.json`, `mirakana_scene`, `mirakana_assets`, `mirakana_tools`, `mirakana_runtime`, `mirakana_runtime_scene`, existing desktop runtime package scripts, static checks, docs, and validation recipes.

---

## Goal

Raise practical game-engine completeness by proving that an agent can create a packaged 2D playable game through reviewed engine contracts:

- create or scaffold a 2D packaged game manifest with runtime package files and runtime scene validation targets
- register first-party source rows for sprite texture, material, audio cue metadata, and Scene v2 authoring inputs where applicable
- cook explicitly selected registered source assets into deterministic cooked artifacts and `.geindex` rows
- migrate supported Scene v2 rows to the current runtime-loadable Scene v1 package surface
- validate the explicit runtime `.geindex` plus scene asset through `validate-runtime-scene-package`
- package and smoke the selected desktop runtime target through reviewed validation recipes

## Context

The engine has a ready `2d-playable-source-tree` recipe, a host-gated `2d-desktop-runtime-package` proof, and a strong AI command surface set. What is missing for higher completion is a normal generated-game loop that composes those pieces into one validated packaged 2D game workflow. This plan should convert the current foundations into a repeatable agent-facing path before moving to generated 3D production work.

## Constraints

- Do not add third-party dependencies or assets.
- Do not parse source assets at runtime; gameplay consumes cooked packages only.
- Do not make broad dependency cooking ready. Dependencies must be selected explicitly until a later focused plan implements dependency traversal.
- Do not claim production atlas packing, tilemap editor UX, native GPU sprite output, renderer/RHI residency, package streaming, editor productization, public native/RHI handles, Metal readiness, or general renderer quality.
- Keep generated game code on public `mirakana::` APIs.
- Keep desktop runtime proofs host-gated where SDL3/vcpkg or renderer backends are required.

## Done When

- A focused RED -> GREEN record exists in this plan.
- A generated or sample packaged 2D game has a manifest, runtime package files, and `runtimeSceneValidationTargets`.
- Focused tests or static checks prove the generated game includes deterministic package files, explicit validation target rows, and no source authoring files in `runtimePackageFiles`.
- The workflow validates through `validate-runtime-scene-package` before package smoke.
- `engine/agent/manifest.json`, docs, static checks, generated-game guidance, and `agent-context` expose the workflow honestly.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused tests/checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory The Existing 2D And Package Paths

**Files:**
- Read: `tools/new-game.ps1`
- Read: `games/sample_2d_playable_foundation/game.agent.json`
- Read: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Read: `games/sample-generated-desktop-runtime-cooked-scene-package/game.agent.json`
- Read: `tests/unit/tools_tests.cpp`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `docs/specs/generated-game-validation-scenarios.md`
- Read: `docs/ai-game-development.md`

- [x] Define the generated 2D packaged workflow boundary and list reused command surfaces.
  - Reuse `register-source-asset`, `cook-registered-source-assets`, `migrate-scene-v2-runtime-package`, `validate-runtime-scene-package`, `register-runtime-package-files`, and `run-validation-recipe` as the reviewed workflow surfaces.
  - The first GREEN implementation is a deterministic generated desktop-runtime 2D package scaffold, not broad dependency cooking or a runtime source parser.
- [x] Decide whether to extend `DesktopRuntimeCookedScenePackage` or add a separate 2D packaged template.
  - Add a separate `DesktopRuntime2DPackage` template. The existing `DesktopRuntimeCookedScenePackage` remains a generic cooked mesh/scene scaffold; forcing 2D sprite/HUD/audio semantics into it would make the recipe contract ambiguous.
- [x] Define package files, scene asset key, validation target id, and validation recipe names.
  - Generated package files: `runtime/<game>.config`, `runtime/<game>.geindex`, `runtime/assets/2d/player.texture.geasset`, `runtime/assets/2d/player.material`, `runtime/assets/2d/jump.audio.geasset`, and `runtime/assets/2d/playable.scene`.
  - Runtime scene validation target id: `packaged-2d-scene`.
  - Scene asset key: `<game>/scenes/packaged-2d-scene`.
  - Validation recipes: `desktop-game-runtime`, `desktop-runtime-release-target`, and `installed-2d-package-smoke`.
- [x] Record non-goals before RED checks are added.
  - No broad dependency cooking, runtime source parsing, package streaming, renderer/RHI residency, production atlas/tilemap/native GPU sprite output, editor productization, material/shader graph, live shader generation, native handles, Metal readiness, or general renderer quality claim.

### Task 2: RED Checks For Generated 2D Package Workflow

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: focused tests if a C++ helper is introduced
- Modify: this plan

- [x] Add failing static checks requiring the generated 2D packaged game manifest shape.
- [x] Add failing checks requiring `runtimeSceneValidationTargets` to match package files and scene asset key.
- [x] Add failing checks rejecting source files in `runtimePackageFiles`, broad dependency cooking, renderer/RHI residency, package streaming, material/shader graph, live shader generation, editor productization, native handle, and Metal claims.
- [x] Record RED evidence.

### Task 3: Generate Or Scaffold The Packaged 2D Game Path

**Files:**
- Modify: `tools/new-game.ps1`
- Modify or create: `games/<selected-2d-package-game>/game.agent.json`
- Modify or create: `games/<selected-2d-package-game>/main.cpp`
- Modify or create: selected runtime package fixture files under `games/<selected-2d-package-game>/runtime/`
- Modify: `games/CMakeLists.txt` if a new sample target is added

- [x] Emit deterministic runtime config, `.geindex`, texture/material/audio/scene package files, and validation target rows.
- [x] Keep source authoring inputs out of `runtimePackageFiles`.
- [x] Keep gameplay code on public `mirakana::` APIs and cooked package/runtime scene access.
- [x] Ensure package smoke args consume the generated package path and scene package path.

### Task 4: Validation Command Composition

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `engine/agent/manifest.json`

- [x] Document the ordered workflow: register source rows, cook selected rows, migrate Scene v2, validate runtime scene package, package/smoke.
- [x] Keep `validate-runtime-scene-package` as the reviewed non-mutating package/scene check before desktop smoke.
- [x] Keep package smoke host-gated and separate from package/scene validation.
- [x] Update static checks so stale generated-game guidance fails.

### Task 5: Plan Completion And Next Slice

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.json`
- Modify: this plan

- [x] Mark this 2D generated package workflow ready only inside its validated scope.
- [x] Keep production 2D atlas/tilemap/native GPU output planned or host-gated.
- [x] Create the next focused plan for generated packaged 3D gameplay or editor playtest package review, based on validation evidence.
- [x] Run the full required validation set and record evidence.

## Validation Evidence

- 2026-05-02 RED:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: FAIL as expected with `engine/agent/manifest.json 2d-desktop-runtime-package recipe must allow DesktopRuntime2DPackage`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: FAIL as expected with `engine manifest commands.newGame must expose DesktopRuntime2DPackage`.
- 2026-05-02 GREEN focused checks:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS; generated `DesktopRuntime2DPackage` scaffold emitted deterministic config, `.geindex`, cooked player texture/material, cooked jump audio, orthographic sprite scene, `runtimeSceneValidationTargets`, public `mirakana::` input/UI/audio/package code, and `PACKAGE_FILES_FROM_MANIFEST`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS (`json-contract-check: ok`)
- 2026-05-02 final validation after docs/manifest/registry sync:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS; `currentActivePlan` points at `3d Packaged Playable Generation Loop v1` and `recommendedNextPlan.path` points at `Editor Playtest Package Review Loop v1`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS (`json-contract-check: ok`)
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS (`ai-integration-check: ok`)
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS (`status: passed`, `exitCode: 0`)
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS (`validate: ok`, CTest `28/28` passed)
  - Diagnostic-only host gates remain honest: Metal tools are missing on this Windows host, Apple packaging requires macOS/Xcode, Android release signing/device smoke is not fully configured, and strict tidy analysis still depends on compile database availability.
```

### 2d Sprite Batch Package Telemetry v1
- Retired file: `2d Sprite Batch Package Telemetry v1`
- 2D Sprite Batch Package Telemetry v1 Implementation Plan (2026-05-02)
- - `sample_2d_desktop_runtime_package` already emits package smoke counters for scene sprite submission and native 2D overlay status.
- **Goal:** Connect the host-independent sprite batch planner to the packaged 2D sample telemetry so package smokes can verify deterministic batch-plan counts without claiming native GPU batch execution.

### 2d Sprite Batch Planning Contract v1
- Retired file: `2d Sprite Batch Planning Contract v1`
- 2D Sprite Batch Planning Contract v1 (2026-05-02)
- **Goal:** Add a host-independent, renderer-neutral 2D sprite batch planning contract that lets engine/AI workflows reason about stable sprite draw batches without claiming production sprite batching or exposing native handles.

### 3d Packaged Playable Generation Loop v1
- Retired file: `3d Packaged Playable Generation Loop v1`
- 3D Packaged Playable Generation Loop v1 Implementation Plan (2026-05-02)
- - GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed (`status: passed`).
- **Goal:** Make AI-generated packaged 3D games use reviewed source, scene, cook, package, manifest, runtime validation, and host-gated desktop package validation surfaces instead of staying limited to the fixed `sample_desktop_runtime_game` package proof.

### 3d Prefab Scene Package Authoring v1
- Retired file: `3d Prefab Scene Package Authoring v1`
- 3D Prefab Scene Package Authoring v1 Implementation Plan (2026-05-02)
- **Goal:** After the 2D atlas/tilemap package authoring slice, add a narrow generated 3D prefab/scene package authoring loop without claiming broad importer execution, skeletal animation production, material/shader graphs, live shader generation, Metal readiness, public native/RHI handles, or general renderer quality.

### 3d Scene Mesh Package Telemetry v1
- Retired file: `3d Scene Mesh Package Telemetry v1`
- 3D Scene Mesh Package Telemetry v1 Implementation Plan (2026-05-02)
- **Architecture:** Keep `mirakana_scene` renderer-neutral and keep all native/RHI ownership in host adapters. Add a small `mirakana_scene_renderer` planning helper over `SceneRenderPacket::meshes`, then emit the resulting counters from the existing 3D desktop package smoke status line and installed validation scripts.
- **Goal:** Add package-visible, execution-free 3D scene mesh planning telemetry for `sample_desktop_runtime_game` so Windows package smokes can prove cooked scene mesh/material intent without claiming broader 3D production readiness.

### Animation CPU Skinned Bitangent Handedness v1
- Retired file: `Animation CPU Skinned Bitangent Handedness v1`
- Animation CPU Skinned Bitangent Handedness v1 Implementation Plan (2026-05-02)
- **Goal:** Extend the existing `mirakana_animation` CPU skinning contract so it can deterministically reconstruct optional skinned bitangents from skinned normals, skinned tangents, and per-vertex tangent handedness signs without adding renderer/RHI execution, glTF import, GPU skinning, morph deformation, or broad skeletal-animation readiness.

### Animation CPU Skinned Normal Stream v1
- Retired file: `Animation CPU Skinned Normal Stream v1`
- Animation CPU Skinned Normal Stream v1 Implementation Plan (2026-05-02)
- **Goal:** Extend the existing `mirakana_animation` CPU skinning contract so it can deterministically skin an optional bind-normal stream alongside bind positions without adding renderer/RHI execution or broad skeletal-animation readiness.

### Animation CPU Skinned Tangent Stream v1
- Retired file: `Animation CPU Skinned Tangent Stream v1`
- Animation CPU Skinned Tangent Stream v1 Implementation Plan (2026-05-02)
- **Goal:** Extend the existing `mirakana_animation` CPU skinning contract so it can deterministically skin an optional bind-tangent stream alongside bind positions and optional bind normals without adding renderer/RHI execution or broad skeletal-animation readiness.

### Desktop Runtime Shippable RHI Window v1
- Retired file: `Desktop Runtime Shippable RHI Window v1`
- Desktop Runtime Shippable RHI Window v1 Implementation Plan (2026-05-02)
- ## Status
- **Goal:** Move the desktop visible game runtime shell closer to a shippable native RHI-backed game-window presentation by making the committed packaged 2D sample use the same host-supplied D3D12/Vulkan shader-bytecode presentation path already used by the shell and 3D sample.

### Editor AI Package Authoring Diagnostics v1
- Retired file: `Editor AI Package Authoring Diagnostics v1`
- Editor AI Package Authoring Diagnostics v1 Implementation Plan (2026-05-02)
- **Goal:** After the 3D prefab/scene package authoring slice, add a narrow editor/AI diagnostics loop that reviews authored package candidates, manifest descriptors, package payload diagnostics, and validation recipe status without turning the editor into a broad package cooker or play-in-editor product.
- **Goal:** After the 3D prefab/scene package authoring slice, add a narrow editor/AI diagnostics loop that reviews authored package candidates, manifest descriptors, package payload diagnostics, and validation recipe status without turning the editor into a broad package cooker or play-in-editor product.

### Editor AI Playtest Evidence Summary v1
- Retired file: `Editor AI Playtest Evidence Summary v1`
- Editor AI Playtest Evidence Summary v1 Implementation Plan (2026-05-02)
- - `tools/run-validation-recipe.ps1 -Mode Execute` returns structured external execution evidence with `status`, `exitCode`, `stdoutSummary`, `stderrSummary`, `commandResults`, `hostGates`, and diagnostics. This slice treats that shape as caller/operator supplied data only.
- **Goal:** After the operator handoff slice, summarize externally collected playtest validation evidence into deterministic AI/editor rows without running validation commands from editor core.

### Editor AI Playtest Operator Handoff v1
- Retired file: `Editor AI Playtest Operator Handoff v1`
- Editor AI Playtest Operator Handoff v1 Implementation Plan (2026-05-02)
- - GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed (`status: passed`, `exitCode: 0`, `durationSeconds: 3.881`).
- **Goal:** After the read-only playtest readiness report, create a deterministic operator handoff that lists the reviewed external validation commands an operator may run, without executing those commands from editor core.

### Editor AI Playtest Operator Workflow Ux v1
- Retired file: `Editor AI Playtest Operator Workflow Ux v1`
- Editor AI Playtest Operator Workflow UX v1 Implementation Plan (2026-05-02)
- > **Status note (2026-05-02):** This was the focused next production gap selected by the AI-operable production loop clean-uplift milestone. The detailed clean-uplift plan has been retired; retained literals live in [99-historical-verdict-archive.md](99-historical-verdict-archive.md) and full prose remains in Git history.
- **Goal:** Turn the consolidated read-only Editor AI playtest operator workflow into a practical editor/operator UX surface without adding another remediation row model.

### Editor AI Playtest Readiness Report v1
- Retired file: `Editor AI Playtest Readiness Report v1`
- Editor AI Playtest Readiness Report v1 Implementation Plan (2026-05-02)
- **Architecture:** Reuse `EditorAiPackageAuthoringDiagnosticsModel`, the validation recipe preflight model, manifest descriptor rows, package payload diagnostics, and host-gate status. Keep mutation and execution on existing reviewed surfaces outside the report model.
- **Goal:** After validation recipe preflight, aggregate package diagnostics and recipe preflight rows into a deterministic read-only playtest readiness report for AI/editor workflows without executing package scripts or claiming play-in-editor productization.

### Editor AI Playtest Remediation Handoff v1
- Retired file: `Editor AI Playtest Remediation Handoff v1`
- Editor AI Playtest Remediation Handoff v1 Implementation Plan (2026-05-02)
- - preserve recipe id, evidence status, host gate, blocker, and readiness dependency references
- **Goal:** After the remediation-queue slice, turn read-only remediation rows into deterministic external handoff rows without executing commands or mutating game/editor data.

### Editor AI Playtest Remediation Queue v1
- Retired file: `Editor AI Playtest Remediation Queue v1`
- Editor AI Playtest Remediation Queue v1 Implementation Plan (2026-05-02)
- - Rows preserve recipe id, evidence status, exit code/summary, host gates, blockers, readiness dependency, remediation category, and deterministic next-action text.
- **Goal:** After the evidence-summary slice, turn externally supplied playtest evidence failures and blockers into deterministic read-only remediation rows without executing commands or mutating game/editor data.

### Editor AI Validation Recipe Preflight v1
- Retired file: `Editor AI Validation Recipe Preflight v1`
- Editor AI Validation Recipe Preflight v1 Implementation Plan (2026-05-02)
- - GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` PASS (`status: passed`, `agent-check` and `schema-check` both exit 0).
- **Goal:** After editor/AI package authoring diagnostics, add a narrow validation-recipe preflight loop that lets editor/AI surfaces explain selected manifest validation recipes and host gates without executing arbitrary commands or turning the editor into play-in-editor productization.

### Editor Playtest Package Review Loop v1
- Retired file: `Editor Playtest Package Review Loop v1`
- Editor Playtest Package Review Loop v1 Implementation Plan (2026-05-02)
- - Gap found during inventory: editor-core did not have a single review/status model that tied package registration state, manifest validation target selection, `validate-runtime-scene-package`, and host-gated desktop smoke into an ordered pre-playtest loop.
- **Goal:** Connect editor-facing package candidates, manifest runtime package files, runtime scene validation targets, diagnostics, and desktop run/package review into one AI-operable playtest loop without productizing the full editor.

Static guard source snapshot:

```markdown
# Editor Playtest Package Review Loop v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Connect editor-facing package candidates, manifest runtime package files, runtime scene validation targets, diagnostics, and desktop run/package review into one AI-operable playtest loop without productizing the full editor.

**Architecture:** Reuse existing editor-core scene/prefab package candidate rows, reviewed package registration apply paths, `runtimeSceneValidationTargets`, `validate-runtime-scene-package`, and manifest-declared validation recipes. Keep this as a review/run workflow over existing models; do not add broad package cooking, runtime source parsing, renderer/RHI residency, package streaming, native handles, Metal readiness, or a Unity/UE-like editor productization claim.

**Tech Stack:** `mirakana_editor_core`, `mirakana_editor`, `game.agent.json`, `tools/register-runtime-package-files.ps1`, `validate-runtime-scene-package`, `run-validation-recipe`, docs, static checks, and validation recipes.

---

## Goal

Make a generated or sample packaged game easier for an agent or editor operator to review before playtest:

- surface reviewed package candidates and manifest registration state
- select runtime scene validation inputs from `runtimeSceneValidationTargets`
- run the non-mutating runtime scene package validation before desktop package smoke
- keep diagnostics tied to manifest/package/scene rows
- preserve host-gated desktop package execution as a separate validation step

## Constraints

- Do not implement broad/dependent package cooking.
- Do not parse source assets at runtime.
- Do not expose renderer, RHI, SDL3, OS, or native handles to gameplay code.
- Do not claim package streaming, editor productization, Metal readiness, or general renderer quality.
- Keep Dear ImGui as the optional developer shell only.

## Done When

- A focused RED -> GREEN record exists in this plan.
- Editor/package review docs and checks describe the ordered review loop.
- Existing package registration apply paths and `runtimeSceneValidationTargets` are used rather than new free-form shell commands.
- Runtime scene package validation is documented and checked as the pre-smoke gate.
- `engine/agent/manifest.json`, roadmap, current capabilities, and plan registry are synchronized.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Editor And Package Review Surfaces

- [x] Read editor-core package candidate and registration apply models.
- [x] Read visible editor package registration UI wiring.
- [x] Read runtime scene validation target docs and static checks.
- [x] Define the review loop boundary and non-goals.

Inventory notes:

- `SceneAuthoringDocument` already exposes `ScenePackageCandidateRow`, `ScenePackageRegistrationDraftRow`, `ScenePackageRegistrationApplyPlan`, `ScenePackageRegistrationApplyResult`, and `apply_scene_package_registration_to_manifest` for narrow `runtimePackageFiles` updates.
- `mirakana_editor` already renders `Scene Package Candidates`, `Package Registration Draft`, and `Package Registration Apply`, and calls the editor-core apply helper from the `Apply Package Registration` button.
- `runtimeSceneValidationTargets` schema/static checks already require safe game-relative `.geindex` paths, non-empty safe scene asset keys, `packageIndexPath` membership in `runtimePackageFiles`, and no command/native/renderer/source-file fields.
- Gap found during inventory: editor-core did not have a single review/status model that tied package registration state, manifest validation target selection, `validate-runtime-scene-package`, and host-gated desktop smoke into an ordered pre-playtest loop.
- Boundary: add a GUI-independent review/status model and machine-readable manifest review-loop descriptor. Do not run CMake, package scripts, shader tools, desktop runtime, or arbitrary shell from editor core; keep actual validation execution in `validate-runtime-scene-package`, `run-validation-recipe`, and package scripts.

### Task 2: RED Review Loop Checks

- [x] Add failing checks for stale editor/package review guidance.
- [x] Add failing checks that runtime scene package validation is selected from manifest descriptors before desktop smoke.
- [x] Add failing checks rejecting broad package cooking, source runtime parsing, package streaming, renderer/RHI residency, editor productization, native handles, Metal readiness, and general renderer quality claims.
- [x] Record RED evidence.

### Task 3: Review Loop Implementation

- [x] Update editor/package docs and manifest guidance.
- [x] Add or adjust narrow diagnostics/status rows if existing models lack enough review state.
- [x] Keep package registration apply and validation execution separate.
- [x] Keep host-gated desktop package smoke separate from package/scene validation.

Implementation notes:

- Added `mirakana::editor::EditorPlaytestPackageReviewModel` in `editor/core/include/mirakana/editor/playtest_package_review.hpp` and `editor/core/src/playtest_package_review.cpp`.
- The model orders `review-editor-package-candidates -> apply-reviewed-runtime-package-files -> select-runtime-scene-validation-target -> validate-runtime-scene-package -> run-host-gated-desktop-smoke`.
- The model blocks runtime scene validation until reviewed package files have been applied and the selected `runtimeSceneValidationTargets` row points at a registered package index.
- Desktop smoke remains a host-gated step and is not executed from editor core.
- `engine/agent/manifest.json.aiOperableProductionLoop.reviewLoops` now exposes the same ordered loop and unsupported-claim boundary.

### Task 4: Completion And Next Slice

- [x] Mark the editor playtest package review loop ready only inside its validated scope.
- [x] Keep full editor productization, play-in-editor isolation, package streaming, renderer/RHI residency, native handles, Metal, and general renderer quality planned or host-gated.
- [x] Create the next focused plan for renderer resource residency/upload execution or material/shader authoring based on validation evidence.
- [x] Run the full required validation set and record evidence.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected because `engine/agent/manifest.json aiOperableProductionLoop` was missing required property `reviewLoops`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected because `docs/current-capabilities.md` did not contain `Editor Playtest Package Review Loop v1`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected after adding editor-core review model tests because `mirakana/editor/playtest_package_review.hpp` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after adding `EditorPlaytestPackageReviewModel` (`CTest 28/28 passed`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed (`json-contract-check: ok`) after manifest review-loop sync.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after docs/static/editor-core sync.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed after manifest sync. Active plan advanced to `Renderer Resource Residency Upload Execution v1`; recommended next plan advanced to `Material Shader Authoring Loop v1`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed (`public-api-boundary-check: ok`) after adding the editor-core review header.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed (`schema-check`, `agent-check`, `api-boundary-check`, `shader-toolchain-check`, `mobile-check`, `validation-recipe-runner-check`, and `agent-context`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` passed with the editor package review model included (`desktop-gui` CTest `41/41` passed).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed (`validate: ok`, CTest `28/28` passed).
- DIAGNOSTIC: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` still reports Metal shader tooling as missing (`metal`, `metallib`) and keeps Metal library packaging diagnostic-only.
- DIAGNOSTIC: Apple packaging remains host-blocked on this Windows host because macOS/Xcode tools are unavailable.
- DIAGNOSTIC: Android packaging reports ready, while release signing is not configured and device smoke is not connected.
- DIAGNOSTIC: `tidy-check` reports the compile database availability gate before configure and then `config ok`; strict tidy analysis remains CI/toolchain dependent.
```

### Generated 2d Native RHI Sprite Package Scaffold v1
- Retired file: `Generated 2d Native RHI Sprite Package Scaffold v1`
- Generated 2D Native RHI Sprite Package Scaffold v1 (2026-05-02)

### Generated 3d Mesh Telemetry Scaffold v1
- Retired file: `Generated 3d Mesh Telemetry Scaffold v1`
- Generated 3D Mesh Telemetry Scaffold v1 Implementation Plan (2026-05-02)
- **Goal:** Extend generated `DesktopRuntime3DPackage` games so they emit the existing `scene_mesh_plan_*` counters during package smoke validation, matching the committed 3D package proof without claiming broad generated 3D production readiness.

### Host Gated Package Streaming Execution v1
- Retired file: `Host Gated Package Streaming Execution v1`
- Host-Gated Package Streaming Execution v1 Implementation Plan (2026-05-02)
- - `RuntimePackageStreamingExecutionResult` reports selected target ids, estimated resident bytes, resident budget bytes, replacement status, stale-handle counts, and deterministic diagnostics. Over-budget intent is reported with `resident-budget-intent-exceeded` and does not claim allocator/GPU enforcement.
- **Goal:** After safe-point unload/replacement execution is validated, narrow host-owned package streaming execution for selected package streaming/residency descriptors without turning it into broad async streaming or public native-handle access.

### Material Shader Authoring Loop v1
- Retired file: `Material Shader Authoring Loop v1`
- Material Shader Authoring Loop v1 Implementation Plan (2026-05-02)
- - GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe desktop-runtime-generated-material-shader-scaffold-package -HostGateAcknowledgements d3d12-windows-primary` passed (`status: passed`, `durationSeconds: 50.979`) for the selected generated material/shader package lane.
- **Goal:** Move material/shader authoring from narrow first-party material instance and generated shader-artifact scaffolds toward an AI-operable authored material/shader review loop without claiming shader graphs, material graphs, live shader generation, or broad renderer quality.

### Package Streaming Residency Budget Contract v1
- Retired file: `Package Streaming Residency Budget Contract v1`
- Package Streaming Residency Budget Contract v1 Implementation Plan (2026-05-02)
- - [x] Add failing checks for package streaming/residency budget descriptor fields and blocked execution status.
- **Goal:** Define the first AI-operable package streaming and residency budget contract without claiming broad streaming, async eviction, native handle access, or production renderer quality.

### Renderer Resource Residency Upload Execution v1
- Retired file: `Renderer Resource Residency Upload Execution v1`
- Renderer Resource Residency Upload Execution v1 Implementation Plan (2026-05-02)
- - Existing package smokes already distinguish cooked package/scene validation from host-gated scene GPU execution through `--require-scene-package`, `--require-scene-gpu-bindings`, backend selection, and `scene_gpu_status`, but they did not report texture upload counts or uploaded byte totals.
- **Goal:** Move the current renderer/RHI resource and upload/staging foundations from planning contracts toward a narrow validated execution path for explicitly selected cooked runtime texture, mesh, and material payloads without exposing native handles to gameplay.

Static guard source snapshot:

```markdown
# Renderer Resource Residency Upload Execution v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the current renderer/RHI resource and upload/staging foundations from planning contracts toward a narrow validated execution path for explicitly selected cooked runtime texture, mesh, and material payloads without exposing native handles to gameplay.

**Architecture:** Keep gameplay on cooked package loading plus `mirakana_scene_renderer` submission. Host/backend adapters may own `IRhiDevice`, upload execution, residency tracking, and `SceneGpuBindingPalette` construction behind first-party contracts. Reuse `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, `RhiResourceLifetimeRegistry`, `RhiUploadStagingPlan`, desktop runtime package metadata, and existing D3D12/Vulkan host gates. Do not turn this into package streaming, broad renderer quality, public native handle exposure, editor productization, material/shader graph authoring, live shader generation, or Metal readiness.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, `mirakana_rhi`, `mirakana_renderer`, D3D12/Vulkan private backends, desktop runtime package validation scripts, static checks, docs, and focused tests.

---

## Goal

Prove one narrow resource execution loop:

- explicitly selected cooked texture, mesh, and material payloads load from a runtime package
- host-owned upload execution produces first-party GPU binding descriptors or clear blocked diagnostics
- residency/lifetime rows stay internal to renderer/RHI ownership
- generated/sample package validation can distinguish package/scene validation from host-gated residency/upload execution

## Context

Renderer/RHI resource foundation, upload/staging planning, runtime RHI upload helpers, and sample D3D12/Vulkan scene GPU package proofs already exist. The next uplift should make the boundary more operational for AI-generated packages without broadening gameplay APIs or claiming production renderer quality.

## Constraints

- Do not expose native API handles or `IRhiDevice` ownership to gameplay code.
- Do not implement package streaming, broad residency budgets, async streaming, or hot-reload safe-point unload in this slice.
- Do not add third-party dependencies.
- Do not claim Metal readiness from Windows validation.
- Keep Vulkan strict and toolchain-gated.
- Keep material/shader graph authoring and live shader generation out of scope.

## Done When

- A RED -> GREEN record exists in this plan.
- Focused tests or static checks distinguish package validation, scene instantiation, and host-owned upload/residency execution.
- One selected package path can report uploaded texture/mesh/material binding readiness or precise host-gated diagnostics through existing first-party report rows.
- `engine/agent/manifest.json`, docs, static checks, and validation recipes remain honest about host gates and unsupported claims.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Resource And Upload Boundaries

- [x] Read `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, `mirakana_rhi`, `mirakana_renderer`, desktop runtime package metadata, and current D3D12/Vulkan package smoke checks.
- [x] Identify the smallest host-owned upload execution path that can be validated without gameplay native handles.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- `mirakana_runtime_rhi` already uploads cooked texture bytes, mesh vertex/index bytes, and material factor uniform bytes through an `IRhiDevice` while preserving owner-device provenance.
- `mirakana_runtime_scene_rhi::build_runtime_scene_gpu_binding_palette` is the narrow bridge from `RuntimeAssetPackage` plus `SceneRenderPacket` into retained mesh uploads, texture uploads, material descriptor bindings, and scene-shared material pipeline layouts.
- `mirakana_runtime_host_sdl3_presentation` is the host-owned execution point: it validates requested vertex input against cooked mesh payload layout, privately creates the D3D12/Vulkan `IRhiDevice`, builds scene GPU bindings, creates the graphics pipeline from the palette material pipeline layout, and wraps the renderer in `SceneGpuBindingInjectingRenderer`.
- Existing package smokes already distinguish cooked package/scene validation from host-gated scene GPU execution through `--require-scene-package`, `--require-scene-gpu-bindings`, backend selection, and `scene_gpu_status`, but they did not report texture upload counts or uploaded byte totals.
- Non-goals for this slice: public native handles, public `IRhiDevice` access, package streaming, broad residency budgets, async upload rings, material/shader graphs, live shader generation, Metal readiness, editor resource panels, or general production renderer quality.

### Task 2: RED Checks

- [x] Add failing tests/static checks for explicit package/scene/upload boundary reporting.
- [x] Add failing checks rejecting public native handles, package streaming, broad residency budgets, material/shader graph, live shader generation, Metal/general renderer quality claims.
- [x] Record RED evidence.

### Task 3: Host-Owned Upload/Residency Execution

- [x] Implement or tighten the selected execution path.
- [x] Keep resource lifetime/residency rows behind renderer/RHI ownership.
- [x] Keep gameplay code on package loading and scene submission contracts.

Implementation notes:

- Added `mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_upload` and `RuntimeSceneGpuUploadExecutionReport` as a backend-neutral host-owned report over the existing cooked texture/mesh/material upload path.
- `SdlDesktopPresentationSceneGpuBindingStats` now carries first-party upload counts and uploaded byte totals only; it does not expose `IRhiDevice`, native handles, descriptor handles, swapchain frames, GPU timestamps, or `SceneGpuBindingPalette`.
- `SceneGpuBindingInjectingRenderer` derives upload counters from the retained scene GPU binding result while continuing to inject mesh/material GPU bindings internally.
- Selected desktop runtime game status lines now print `scene_gpu_mesh_uploads`, `scene_gpu_texture_uploads`, `scene_gpu_material_uploads`, `scene_gpu_uploaded_texture_bytes`, `scene_gpu_uploaded_mesh_bytes`, and `scene_gpu_uploaded_material_factor_bytes`.
- Sample game sources no longer include `mirakana/runtime_rhi/runtime_upload.hpp` just to obtain the lit mesh stride; game code keeps the vertex-input stride as local package/shader contract data.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused material/shader authoring slice if this slice completes.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected after adding runtime scene RHI upload execution tests because `mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc`, `RuntimeSceneGpuUploadExecutionStatus`, and `execute_runtime_scene_gpu_upload` did not exist.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected because `engine/agent/manifest.json.aiOperableProductionLoop` was missing required `resourceExecutionLoops`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected because `docs/current-capabilities.md` did not contain `Renderer Resource Residency Upload Execution v1`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after adding the host-owned upload execution API and report (`CTest 28/28 passed`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after adding `aiOperableProductionLoop.resourceExecutionLoops`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after syncing manifest/docs/static checks.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after public report/header changes (`public-api-boundary-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe desktop-runtime-sample-game-scene-gpu-package -HostGateAcknowledgements d3d12-windows-primary` passed, validating the selected `sample_desktop_runtime_game` package lane with the new backend-neutral scene GPU upload counters.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed again after sample/CMake boundary cleanup (`CTest 28/28 passed`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe desktop-runtime-sample-game-scene-gpu-package -HostGateAcknowledgements d3d12-windows-primary` passed after cleanup (`status: passed`, `durationSeconds: 73.229`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after advancing `currentActivePlan` to Material Shader Authoring Loop v1 and creating the next package streaming/residency budget contract plan.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed after docs/manifest synchronization.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed (`validate: ok`, `CTest 28/28 passed`). Diagnostic-only gates remain honest: Metal tools missing on this Windows host, Apple packaging requires macOS/Xcode, Android release signing/device smoke not fully configured, and strict tidy analysis requires a compile database.
```

### Safe Point Package Unload Replacement Execution v1
- Retired file: `Safe Point Package Unload Replacement Execution v1`
- Safe-Point Package Unload Replacement Execution v1 Implementation Plan (2026-05-02)
- - RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed after adding safe-point replacement tests because `mirakana::runtime::commit_runtime_package_safe_point_replacement` and `mirakana::runtime::RuntimePackageSafePointReplacementStatus` did not exist.
- **Goal:** Narrow runtime package unload/replacement into a host-owned safe-point execution path after package streaming and residency budget intent is explicit and validated.

### Strict Clang Tidy Compile Database Enforcement v1
- Retired file: `Strict Clang Tidy Compile Database Enforcement v1`
- Strict Clang-Tidy Compile Database Enforcement v1 Implementation Plan (2026-05-02)
- **Goal:** Make `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict` enforce compile database availability on the default Windows `dev` preset by generating a clang-tidy-compatible compile database from CMake File API codemodel data when the active generator does not emit `compile_commands.json`.

### AI Cook Package Command Surface v1
- Retired file: `AI Cook Package Command Surface v1`
- AI Cook Package Command Surface v1 Implementation Plan (2026-05-03)
- **Goal:** 3D デスクトップパッケージ等で、エージェントが `GameEngine.SourceAssetRegistry.v1` のどのキーをどの `dependencyExpansion` / `dependencyCooking` でクックするかを **`game.agent.json.registeredSourceAssetCookTargets`** として宣言し、既存の `prefabScenePackageAuthoringTargets` 行と **同一の `sourceRegistryPath` / `packageIndexPath`** であることを静的検証で保証する。

### Broad Dependency Cook Plan v1
- Retired file: `Broad Dependency Cook Plan v1`
- Broad Dependency Cook Plan v1 Implementation Plan (2026-05-03)
- **Goal:** `cook-registered-source-assets` / `plan_registered_source_asset_cook_package` に、**同一 `GameEngine.SourceAssetRegistry.v1` 文書内**で辿れる依存キーのみを対象とした **登録レジストリ依存閉包（registry closure）** を追加する。既定は従来どおり **明示的キー選択** のままとし、後方互換の shim は置かない。

### Cmake Install Export And CXX Modules Audit v1
- Retired file: `Cmake Install Export And CXX Modules Audit v1`
- CMake Install Export And C++ Modules Audit v1 Implementation Plan (2026-05-03)
- **Goal:** `cmake --install` / `()` / `install(EXPORT …)`（またはプロジェクトが採用する同等の消費者向け CMake パッケージ）と **C++20/23 モジュール**（`MK_ENABLE_CXX_MODULE_SCANNING`、`MK_ENABLE_IMPORT_STD` 等）の設定を監査し、**公式 CMake / コンパイラドキュメント**に沿って一貫した推奨状態にする。Phase 1 の「後続 Phase がビルド上で迷子にならない」下地を整える。

### Coverage Threshold Policy v1
- Retired file: `Coverage Threshold Policy v1`
- Coverage Threshold Policy v1 Implementation Plan (2026-05-03)
- **Goal:** Linux GCC/Clang CI でのカバレッジ収集（`tools/check-coverage.ps1`、CTest `-T Coverage`）に対し、**閾値・レポート形式・失敗条件**をリポジトリ方針として固定し、Phase 1 の「Linux coverage lane reports stable threshold evidence」を満たす。

### Full Clang Tidy Warning Cleanup v1
- Retired file: `Full Clang Tidy Warning Cleanup v1`
- Full Clang-Tidy Warning Cleanup v1 Implementation Plan (2026-05-03)
- **Goal:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict` を既定の `dev` preset（Debug）でリポジトリ内の全対象 C++ translation unit に対して **clang-tidy が非エラー終了（終了コード 0）**する状態にする。残余 **warning のゼロ化**は Step 3 の継続バッチで追う（LLVM 流儀に合わせ、まずゲートを exit code で固定する）。既存の `.clang-tidy` 方針を維持し、広範なチェック無効化で「完了」を偽装しない。

### Native RHI Resource Lifetime Migration v1
- Retired file: `Native RHI Resource Lifetime Migration v1`
- Native RHI Resource Lifetime Migration v1 Implementation Plan (2026-05-03)
- **Goal:** `RhiResourceLifetimeRegistry` を **`NullRhiDevice` のリソース寿命**（バッファ／テクスチャに加え Phase E からサンプラー・シェーダー・ディスクリプタ／パイプライン。作成、`null_mark_*_released`、`release_transient`、デバイス破棄時の未解放フラッシュ）に結線し、フェンス完了値に基づく `retire_released_resources` を **`submit` / `wait`** から呼び出して遅延解放を確実に消化する。

### Package Mount And Resident Cache v1
- Retired file: `Package Mount And Resident Cache v1`
- Package Mount And Resident Cache v1 Implementation Plan (2026-05-03)
- **Goal:** 複数の調理済み `RuntimeAssetPackage` を **明示的なマウント順**でオーバーレイ結合し、結果を **単一の `RuntimeResourceCatalogV2` 再構築**に渡す。生成世代（`RuntimeResourceHandleV2::generation`）は既存の `build_runtime_resource_catalog_v2` 契約のまま **置換時に無効化**される。

### Material Graph Authoring v1
- Retired file: `Material Graph Authoring v1`
- Material Graph Authoring v1 Implementation Plan (2026-05-04)
- **Goal:** Introduce **first-party `GameEngine.MaterialGraph.v1`** as the canonical authoring document for material graphs (format **A** from the Phase 4 plan), with deterministic text IO, validation, **lowering to existing runtime `MaterialDefinition`**, and a **GUI-independent** `MaterialGraphAuthoringDocument` in `mirakana_editor_core` mirroring `MaterialAuthoringDocument` patterns.

### Shader Graph And Generation Pipeline v1
- Retired file: `Shader Graph And Generation Pipeline v1`
- Shader Graph and Generation Pipeline v1 Implementation Plan (2026-05-04)
- **Goal:** Close the Phase-4 gap between **`GameEngine.MaterialGraph.v1`** and reviewed **DXC** compilation by introducing **`GameEngine.MaterialGraphShaderExport.v0`** (bridge document), deterministic **HLSL stub emission** from validated graphs, and **`plan_material_graph_shader_pipeline`** in `mirakana_tools` that materializes `ShaderCompileExecutionRequest` rows for **D3D12 DXIL** and **Vulkan SPIR-V** (vertex + fragment) without raw shell, new third-party dependencies, or renderer binding changes.

### Generated Static 3d Production Game Recipe v1
- Retired file: `Generated Static 3d Production Game Recipe v1`
- 生成静的 3D デスクトップゲーム運用レシピ v1 実装計画 (2026-05-05)

### Editor Resource Capture Execution Evidence v1
- Retired file: `Editor Resource Capture Execution Evidence v1`
- Editor Resource Capture Execution Evidence v1 Implementation Plan (2026-05-07)
- - `make_resource_panel_ui_model` emits retained `resources.capture_execution.<id>` rows with status, artifact, and diagnostic labels.
- **Goal:** Add reviewed Resources panel evidence rows for host-owned resource capture execution without letting editor core launch tools, toggle graphics APIs, or expose native handles.

Static guard source snapshot:

```markdown
# Editor Resource Capture Execution Evidence v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add reviewed Resources panel evidence rows for host-owned resource capture execution without letting editor core launch tools, toggle graphics APIs, or expose native handles.

**Architecture:** Extend the GUI-independent `MK_editor_core` resource panel model with caller-supplied capture execution snapshots. The model only normalizes deterministic evidence rows and retained `MK_ui` ids; the optional `MK_editor` shell may surface acknowledged request waiting states, but actual PIX/debug-layer/ETW capture execution remains an external host workflow.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, retained `MK_ui`, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the next narrow resource-management/capture productization gap:

- Keep `Editor Resource Capture Request v1` handoff rows intact.
- Add host-owned capture execution evidence rows under retained `resources.capture_execution` ids.
- Allow evidence to represent requested, host-gated, running, captured, failed, or blocked states.
- Reject unsupported claims that editor core executed capture tooling or exposed native handles.
- Keep actual PIX launch, D3D12 debug-layer toggles, ETW/performance capture, process execution, residency/allocator policy, package streaming, GPU capture execution, and native handles out of editor core.

## Context

- `EditorResourcePanelInput` already accepts plain diagnostics and capture request inputs.
- The visible `MK_editor` Resources panel tracks transient acknowledgement ids for capture request handoff only.
- The master plan still lists resource management/capture execution beyond reviewed handoff rows as follow-up editor productization work.
- This slice records evidence about a host-owned workflow; it does not perform that workflow.

## Constraints

- Do not add dependencies on PIX, D3D12 SDK Layers, ETW, SDL3, Dear ImGui, concrete RHI backends, or native handles to `editor/core`.
- Do not run process commands from `editor/core`.
- Do not claim capture execution is ready merely because a request is acknowledged.
- Keep evidence rows deterministic, sanitized, sorted, and safe for retained UI.
- Update Codex and Claude editor guidance together.

## Done When

- RED `MK_editor_core_tests` proves the new capture execution evidence surface is missing.
- `EditorResourcePanelModel` exposes `capture_execution_rows`.
- `make_resource_panel_ui_model` emits retained `resources.capture_execution.<id>` rows with status, artifact, and diagnostic labels.
- The visible `MK_editor` Resources panel shows capture execution evidence/waiting rows derived from acknowledged capture requests without executing capture tools.
- Docs, master plan, registry, manifest, skills, and static checks record the boundary truthfully.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor resource capture execution evidence reports host owned snapshots")`.
- [x] Build an `EditorResourcePanelInput` with `device_available=true`, `backend_id=d3d12`, and three `capture_execution_snapshots`:
  - `pix_gpu_capture`: captured externally with artifact `captures/pix/frame_8.wpix`,
  - `d3d12_debug_validation`: host-gated and waiting on Windows Graphics Tools,
  - `unsafe_native_capture`: claims editor-core execution and native handle exposure.
- [x] Assert `make_editor_resource_panel_model` exposes sorted `capture_execution_rows` with:
  - `d3d12_debug_validation.status_label == "Host-gated"` and `artifact_path == "-"`,
  - `pix_gpu_capture.status_label == "Captured"` and `artifact_path == "captures/pix/frame_8.wpix"`,
  - `unsafe_native_capture.status_label == "Blocked"` and a diagnostic mentioning unsupported editor-core execution or native handle exposure.
- [x] Assert retained UI ids exist:
  - `resources.capture_execution.pix_gpu_capture.status`,
  - `resources.capture_execution.pix_gpu_capture.artifact`,
  - `resources.capture_execution.unsafe_native_capture.diagnostic`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`; confirm the build fails before implementation because `capture_execution_snapshots` / `capture_execution_rows` are not declared.

### Task 2: Editor-Core Evidence Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/resource_panel.hpp`
- Modify: `editor/core/src/resource_panel.cpp`

- [x] Add `EditorResourceCaptureExecutionInput` with ids, labels, state booleans, artifact path, diagnostic, and unsupported-claim booleans.
- [x] Add `EditorResourceCaptureExecutionRow` to the public resource panel model.
- [x] Add `capture_execution_snapshots` to `EditorResourcePanelInput` and `capture_execution_rows` to `EditorResourcePanelModel`.
- [x] In `make_editor_resource_panel_model`, sanitize and sort snapshots by id/tool/label.
- [x] Derive status labels:
  - unsupported editor-core execution or native handles: `Blocked`,
  - `capture_failed`: `Failed`,
  - `capture_completed`: `Captured`,
  - `capture_started`: `Running`,
  - `host_gated`: `Host-gated`,
  - `requested`: `Requested`,
  - otherwise `Not requested`.
- [x] Emit safe diagnostics for blocked rows and keep artifact path as `-` when absent.
- [x] In `make_resource_panel_ui_model`, add retained `resources.capture_execution` rows with `label`, `tool`, `status`, `artifact`, and `diagnostic`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Visible Resources Panel Adapter

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add `append_resource_capture_execution_snapshots(EditorResourcePanelInput&)`.
- [x] For acknowledged `pix_gpu_capture` and `d3d12_debug_layer_gpu_validation` requests, add host-gated waiting evidence rows with `requested=true`, `host_gated=true`, no artifact path, and diagnostics that say the editor is waiting for external host capture evidence.
- [x] Add `draw_resource_capture_execution_rows_table` and render it below Capture Requests.
- [x] Keep the table read-only: no process execution, no PIX launch, no debug-layer API toggles, and no native handle display.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Guidance, Static Checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

- [x] Record `Editor Resource Capture Execution Evidence v1`, `EditorResourceCaptureExecutionInput`, `EditorResourceCaptureExecutionRow`, and retained `resources.capture_execution` ids.
- [x] Keep PIX launch, D3D12 debug-layer toggles, ETW/performance capture, process execution, residency/allocator policy, package streaming, GPU capture execution, native handles, and broad renderer/resource management readiness unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS (expected fail) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation on missing `EditorResourceCaptureExecutionRow`, `EditorResourceCaptureExecutionInput`, `EditorResourcePanelInput::capture_execution_snapshots`, and `EditorResourcePanelModel::capture_execution_rows`. |
| Focused `MK_editor_core_tests` | PASS | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after adding the editor-core evidence model and retained `resources.capture_execution` rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Desktop GUI lane built `MK_editor` with Capture Execution Evidence rows and passed 46/46 CTest entries. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok` after adding resource capture execution evidence API, UI, docs, manifest, and skill checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Unsupported gap audit accepted the updated editor-productization and renderer/resource notes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok` after applying repository clang-format. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check accepted the editor-core resource capture execution evidence additions. |
| `git diff --check` | PASS | No whitespace errors reported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; dev CTest passed 29/29. Diagnostic-only host gates remain for Metal/Apple as expected on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev build completed through `tools/build.ps1`. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the Editor Resource Capture Execution Evidence v1 files; leave unrelated pre-existing guidance changes unstaged. |
```

### Editor Resource Capture Request v1
- Retired file: `Editor Resource Capture Request v1`
- Editor Resource Capture Request v1 Implementation Plan (2026-05-07)
- **Goal:** Extend the visible Resources panel with deterministic, reviewed resource/GPU capture request rows without executing capture tooling or exposing renderer/RHI/native handles from `editor/core`.

Static guard source snapshot:

```markdown
# Editor Resource Capture Request v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the visible Resources panel with deterministic, reviewed resource/GPU capture request rows without executing capture tooling or exposing renderer/RHI/native handles from `editor/core`.

**Architecture:** Keep `editor/core` as a GUI-independent request/handoff model over plain rows. The optional `mirakana_editor` Dear ImGui shell may display request buttons and transient acknowledgement state, but all actual PIX, debug-layer, GPU validation, ETW, or backend capture execution remains an external host/operator workflow.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_ui`, `mirakana_editor` Dear ImGui adapter, existing Resources panel diagnostics, `mirakana_editor_core_tests`, AI/static validation checks.

---

## Goal

Implement `editor-resource-capture-request-v1` as a narrow follow-up to Resources diagnostics:

- add reviewed capture request input rows for host-owned diagnostics workflows such as PIX GPU capture handoff and D3D12 debug-layer/GPU-validation capture prep;
- expose deterministic request rows with host gates, availability, acknowledgement requirement, reviewed action labels, and blocking diagnostics;
- keep editor-core output as retained `mirakana_ui` labels under `resources.capture_requests`;
- let visible `mirakana_editor` display request rows and record transient reviewed request acknowledgement only after a user action;
- keep all capture execution, process launch/attach, native handles, RHI handles, and platform capture APIs out of `editor/core`.

## Context

- Editor Resource Panel Diagnostics v1 already shows active viewport RHI stats, memory diagnostics, and lifetime summaries.
- The master plan still lists resource management/capture panels beyond read-only diagnostics as a production gap.
- Microsoft PIX guidance treats GPU captures as host/tool-owned workflows where the operator launches or attaches PIX, or uses a dedicated programmatic capture path. It also recommends validating invalid D3D12 usage with the D3D12 debug layer and GPU-based validation when captures fail. This slice turns that into reviewable editor data only; it does not integrate WinPixEventRuntime, D3D12 debug-layer APIs, ETW, PIX launch, or native handles.

## Constraints

- Keep `editor/core` independent from RHI, renderer, SDL3, Dear ImGui, OS APIs, PIX APIs, debug-layer APIs, ETW, and native handles.
- Do not execute capture tools, start processes, attach debuggers/profilers, mutate project files, or persist acknowledgement state.
- Do not expose RHI/backend/native handles or command strings as an editor-core execution contract.
- Host-gated request rows must require explicit acknowledgement before visible editor state reports a request as acknowledged.
- No residency enforcement, allocator policy, eviction, package streaming, backend destruction migration, or broad renderer quality claims.

## Done When

- Unit tests prove resource capture request rows are deterministic, sanitize labels/diagnostics, require acknowledgement when host-gated, and expose retained `resources.capture_requests` UI labels.
- Unit tests prove no-device/resource-unavailable rows are blocked and cannot be acknowledged.
- `mirakana_editor` renders a Capture Requests section with an explicit `Request` button only for available rows that have not yet been acknowledged in the current session.
- Docs, registry, master plan, manifest, skills, and `tools/check-ai-integration.ps1` describe Editor Resource Capture Request v1 without claiming capture execution, native handles, PIX integration, ETW integration, residency management, allocator enforcement, package streaming, or general renderer quality.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Files

- Modify: `editor/core/include/mirakana/editor/resource_panel.hpp`
- Modify: `editor/core/src/resource_panel.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

---

### Task 1: RED Tests For Capture Request Rows

- [x] Add `editor resource capture request model exposes reviewed host gated requests`.
- [x] Build a ready D3D12 resource panel input with PIX and debug-layer capture request rows.
- [x] Assert request rows are sorted deterministically, sanitize text, expose host gates, require acknowledgement, and show unavailable diagnostics only when blocked.
- [x] Assert retained `make_resource_panel_ui_model` output contains `resources.capture_requests.<id>.action`, `.host_gates`, `.acknowledgement`, and `.diagnostic`.
- [x] Add `editor resource capture request model blocks unavailable device requests`.
- [x] Assert no-device requests are blocked, cannot be acknowledged, and expose a diagnostic that capture execution stays host-gated/external.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and confirm failure because capture request fields do not exist yet.

### Task 2: Editor-Core Capture Request Model

- [x] Add `EditorResourceCaptureRequestInput` and `EditorResourceCaptureRequestRow` plain structs.
- [x] Extend `EditorResourcePanelInput` with capture request inputs and `EditorResourcePanelModel` with capture request rows.
- [x] Implement deterministic row ordering, text sanitization, host-gate display labels, acknowledgement labels, and unavailable diagnostics.
- [x] Extend `make_resource_panel_ui_model` with retained `resources.capture_requests` rows.
- [x] Keep the API free of RHI, renderer, OS, PIX, debug-layer, ETW, and native-handle includes.

### Task 3: Visible Editor Wiring

- [x] Add transient `mirakana_editor` session state for acknowledged capture request ids.
- [x] Populate resource capture request inputs from the active viewport resource context using plain backend id/device availability.
- [x] Render a Capture Requests table in the Resources panel.
- [x] Show a `Request` button only when `request_available` is true and the row is not already acknowledged.
- [x] Do not execute any capture/process/API path from the button; record an editor log row and transient acknowledgement only.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Skills, Static Checks

- [x] Document Editor Resource Capture Request v1 as reviewed host/operator handoff data, not capture execution.
- [x] Keep non-ready claims explicit: no PIX integration, debug-layer API toggles, ETW capture, native handles, residency/allocator enforcement, package streaming, backend destruction migration, or broad renderer quality.
- [x] Add static checks for new APIs, tests, retained ids, visible editor wiring, docs, manifest, and Codex/Claude skill guidance.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS (expected fail) | `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation on missing `EditorResourceCaptureRequestRow`, `EditorResourceCaptureRequestInput`, `EditorResourcePanelInput::capture_requests`, and `EditorResourcePanelModel::capture_request_rows`. |
| Focused `mirakana_editor_core_tests` | PASS | `cmake --build --preset dev --target mirakana_editor_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed after implementing the editor-core request model. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial check found clang-format issues in `resource_panel.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` fixed them and rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check accepted the editor-core resource capture request additions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok` after adding resource capture request API, UI, docs, manifest, and skill checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; unsupported gap count remains 11 with editor-productization still partly-ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Desktop GUI lane built `mirakana_editor` with the Resources Capture Requests table and passed 46/46 CTest entries. |
| `git diff --check` | PASS | No whitespace errors; Git reported line-ending conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; Metal/Apple lanes remained diagnostic/host-gated on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default dev build completed after validation. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the Editor Resource Capture Request v1 files; leave unrelated pre-existing guidance changes unstaged. |
```

### Editor Resource Panel Diagnostics v1
- Retired file: `Editor Resource Panel Diagnostics v1`
- Editor Resource Panel Diagnostics v1 Implementation Plan (2026-05-07)
- - The model reports device availability, backend label/status, selected RHI counter rows, memory diagnostic rows with unavailable values shown clearly, and lifetime summary rows from caller-supplied plain counts.
- **Goal:** Add a visible editor Resources panel backed by a GUI-independent `mirakana_editor_core` diagnostics model for RHI/resource counters, memory diagnostics, and lifetime-ledger summaries.

### Editor Prefab Instance Local Child Refresh Resolution v1
- Retired file: `Editor Prefab Instance Local Child Refresh Resolution v1`
- Editor Prefab Instance Local Child Refresh Resolution Implementation Plan (2026-05-09)
- **Status:** Completed.
- **Goal:** Extend reviewed scene prefab instance refresh so an author can explicitly keep local child subtrees while refreshing a selected linked prefab root.

Static guard source snapshot:

```markdown
# Editor Prefab Instance Local Child Refresh Resolution Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend reviewed scene prefab instance refresh so an author can explicitly keep local child subtrees while refreshing a selected linked prefab root.

**Architecture:** Keep the merge decision in `mirakana_editor_core` as a reviewed, deterministic refresh policy and row model. The optional `mirakana_editor` Dear ImGui shell only exposes an explicit keep-local toggle and still routes all mutation through undoable `SceneAuthoringDocument` actions.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_editor`, first-party `mirakana_ui` retained models, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Plan ID:** `editor-prefab-instance-local-child-refresh-resolution-v1`

**Status:** Completed.

---

## Context

- Active master plan: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- Selected gap: `editor-productization`.
- Previous slice: `Editor Scene Prefab Instance Refresh Review v1`.
- Current refresh blocks unlinked or differently linked children because applying the refreshed source subtree would delete them without a reviewed merge decision.
- This slice adds an explicit local-child preservation policy. It does not add fuzzy matching, automatic merge/rebase, stale-node keep-as-local resolution, nested prefab propagation, package script execution, validation recipe execution, renderer/RHI uploads, native handles, or runtime prefab semantics.

## Files

- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
  - Add `ScenePrefabInstanceRefreshPolicy`.
  - Add `keep_local_child` row kind.
  - Add plan/result counters for kept local child subtrees.
  - Thread the policy through plan/apply/action APIs.
- Modify: `editor/core/src/scene_authoring.cpp`
  - Detect local child subtree roots deterministically.
  - Keep local subtrees only when the reviewed policy enables it and their refreshed parent anchor remains present.
  - Rebuild preserved local subtrees with original names, transforms, components, source-link metadata, and parenting.
  - Preserve selected-node remapping for kept local nodes.
  - Surface retained `scene_prefab_instance_refresh` UI rows and summary labels for kept local children.
- Modify: `tests/unit/editor_core_tests.cpp`
  - Add failing coverage for reviewed local child preservation.
  - Keep existing blocker coverage for the default policy.
- Modify: `editor/src/main.cpp`
  - Add a visible `Keep Local Children` control for selected prefab refresh.
  - Pass `ScenePrefabInstanceRefreshPolicy` into the plan and undoable action.
- Modify: `docs/editor.md`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`
  - Document the narrowed capability and explicit unsupported boundaries.
- Modify: `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`
  - Keep Codex and Claude editor guidance aligned.
- Modify: `engine/agent/manifest.json`
  - Update current editor/source-link capability text and active plan pointer.
- Modify: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`
  - Add sentinel checks for the new reviewed local-child refresh policy.
- Modify: `docs/superpowers/plans/README.md`
  - Track this plan as the active slice while work is in progress, then as latest completed evidence at closeout.
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update the current verdict and selected-gap evidence after validation.

## Done When

- Default refresh still blocks unsupported local children.
- With `ScenePrefabInstanceRefreshPolicy::keep_local_children == true`, local child subtrees under retained refreshed source nodes are preserved and remain undoable.
- The plan/result/UI model exposes kept-local counts and `keep_local_child` rows.
- The visible editor has an explicit keep-local control and still does not execute package scripts, validation recipes, or renderer/RHI work from editor core.
- Focused `mirakana_editor_core_tests` pass.
- GUI build passes because `editor/src/main.cpp` changes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `git diff --check -- <touched files>`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Tasks

### Task 1: Add failing editor-core coverage

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add a test named `editor scene prefab instance refresh review can keep local child subtrees`.
- [x] Build `mirakana_editor_core_tests` and verify the test fails because `ScenePrefabInstanceRefreshPolicy`, `keep_local_child`, and kept-local counters do not exist yet.

Run:

```powershell
cmake --build --preset dev --target mirakana_editor_core_tests
```

Expected before implementation: compile failure for the new refresh policy and row kind.

### Task 2: Implement the reviewed refresh policy

**Files:**
- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Modify: `editor/core/src/scene_authoring.cpp`

- [x] Add `ScenePrefabInstanceRefreshPolicy` with `bool keep_local_children{false}`.
- [x] Add `ScenePrefabInstanceRefreshRowKind::keep_local_child`.
- [x] Add `keep_local_child_count` to `ScenePrefabInstanceRefreshPlan`.
- [x] Add `kept_local_child_count` to `ScenePrefabInstanceRefreshResult`.
- [x] Thread the policy through `plan_scene_prefab_instance_refresh`, `apply_scene_prefab_instance_refresh`, and `make_scene_prefab_instance_refresh_action`.
- [x] Keep default behavior blocking local children.
- [x] When policy is enabled, preserve local child subtrees under refreshed source parents and expose warning rows instead of blockers.

### Task 3: Wire the visible editor shell

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add a `Keep Local Children` checkbox near `Refresh Prefab Instance`.
- [x] Pass `ScenePrefabInstanceRefreshPolicy{.keep_local_children = scene_prefab_refresh_keep_local_children_}` to the refresh plan and undoable action.
- [x] Keep refresh disabled unless the selected node is a linked prefab root with a safe prefab path.
- [x] Stage warning-capable retained refresh rows and require `Apply Reviewed Prefab Refresh` before visible mutation.

### Task 4: Synchronize docs, skills, manifest, and static checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/roadmap.md`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`

- [x] Document local-child preservation as explicit reviewed refresh policy.
- [x] Keep unsupported claims explicit: no automatic nested propagation, no fuzzy merge/rebase, no package/validation execution, no renderer/RHI/native handles.
- [x] Add sentinels for `ScenePrefabInstanceRefreshPolicy`, `keep_local_child`, retained UI labels, visible shell checkbox, docs, skills, and manifest text.

### Task 5: Validate and close the slice

**Files:**
- Modify: this plan file.

- [x] Run focused editor-core build and tests.
- [x] Run GUI build.
- [x] Run relevant static checks.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact validation evidence in this plan.
- [x] Set this plan status to `Completed`, move `currentActivePlan` back to the master plan, and update the plan registry latest completed slice.

## Validation Evidence

- RED: `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation on missing `mirakana::editor::ScenePrefabInstanceRefreshPolicy`, `keep_local_child`, and kept-local counters.
- Focused editor-core build: `cmake --build --preset dev --target mirakana_editor_core_tests` passed.
- Focused editor-core tests: `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed.
- GUI build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` passed, including 46/46 desktop-gui CTest tests.
- Static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Slice-closing validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. The production readiness audit still reports 11 unsupported gaps; `editor-productization` remains `partly-ready`.
- Host gates observed during validation: Metal shader tools are missing on this Windows host, and Apple host evidence remains macOS/Xcode gated; both are diagnostic-only for this slice.

## Status

Completed.
```

### Editor Scene Prefab Instance Refresh Review v1
- Retired file: `Editor Scene Prefab Instance Refresh Review v1`
- Editor Scene Prefab Instance Refresh Review Implementation Plan (2026-05-09)
- ## Status
- **Goal:** Add a reviewed, undoable editor refresh path for scene prefab instances that already carry `ScenePrefabSourceLink` metadata.

Static guard source snapshot:

```markdown
# Editor Scene Prefab Instance Refresh Review Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed, undoable editor refresh path for scene prefab instances that already carry `ScenePrefabSourceLink` metadata.

**Architecture:** Keep the behavior in `mirakana_editor_core` as a GUI-independent review/apply model over `SceneAuthoringDocument`. The optional `mirakana_editor` shell may call the model from the Scene panel, but editor core must not execute package scripts, validation recipes, renderer/RHI work, native handles, or arbitrary file operations. The first refresh slice is intentionally conservative: it maps instance nodes by unique `source_node_name`, preserves existing scene node state for retained nodes, adds new source nodes from the refreshed prefab, removes reviewed stale source nodes, and blocks ambiguous or mixed local-child cases.

**Tech Stack:** C++23, `mirakana_scene`, `mirakana_editor_core`, retained `mirakana_ui` model rows, `mirakana_editor` Dear ImGui adapter, CMake/CTest, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Status

Completed.

## Context

The selected production gap is `editor-productization`. The previous slices added reviewed prefab variant base refresh/merge rows and scene prefab source-link diagnostics. This slice turns source-link diagnostics into an explicit, host-independent refresh review/apply operation for scene prefab instances. It does not claim full nested prefab propagation, fuzzy merge/rebase UX, runtime prefab instance semantics, package scripts, validation recipe execution, renderer/RHI uploads, native handles, or package streaming.

## Constraints

- Use TDD: add failing `mirakana_editor_core_tests` coverage before production code.
- Keep `editor/core` GUI-independent.
- Preserve existing scene node state for retained source nodes; do not infer local override intent from unavailable historical source data.
- Block refresh when the selected root is not a valid linked prefab root, when source names are ambiguous, or when the subtree contains unlinked/different-link local children that would be deleted.
- Apply only through an explicit undoable action.
- Update docs, manifest, skills, and static checks after focused tests are green.

## Files

- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Modify: `editor/core/src/scene_authoring.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`

## Tasks

### Task 1: Red Tests

- [x] Add `editor scene prefab instance refresh review preserves linked nodes and applies with undo` to `tests/unit/editor_core_tests.cpp`.
- [x] The test must instantiate a prefab with `source_path`, edit one retained node in the scene, refresh against a prefab with one added node and one removed node, verify retained state is preserved, added nodes use refreshed prefab data, removed source nodes are gone, source links are updated, retained `scene_prefab_instance_refresh` UI rows exist, and undo restores the original scene.
- [x] Add `editor scene prefab instance refresh review blocks unsafe mappings` to `tests/unit/editor_core_tests.cpp`.
- [x] The test must cover a non-root linked node, duplicate refreshed source names, and an unlinked local child under the instance root.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests`; expected RED is missing `ScenePrefabInstanceRefresh*`, `plan_scene_prefab_instance_refresh`, `apply_scene_prefab_instance_refresh`, `make_scene_prefab_instance_refresh_action`, or `make_scene_prefab_instance_refresh_ui_model`.

### Task 2: Editor-Core Contract

- [x] Add refresh status, row-kind, row, plan, and result structs to `editor/core/include/mirakana/editor/scene_authoring.hpp`.
- [x] Add declarations for `scene_prefab_instance_refresh_status_label`, `scene_prefab_instance_refresh_row_kind_label`, `plan_scene_prefab_instance_refresh`, `apply_scene_prefab_instance_refresh`, `make_scene_prefab_instance_refresh_action`, and `make_scene_prefab_instance_refresh_ui_model`.
- [x] Implement the conservative source-name review in `editor/core/src/scene_authoring.cpp`.
- [x] Implement scene rebuild/apply by replacing the linked root subtree, preserving outside nodes, preserving retained linked node name/transform/components, adding new refreshed nodes, removing reviewed stale nodes, and selecting the refreshed counterpart or refreshed root.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests`; expected GREEN.

### Task 3: Visible Editor Adapter

- [x] Wire the Scene panel to show a `Refresh Prefab Instance` control only for selected linked root nodes with a safe prefab path.
- [x] Load the refreshed prefab through the existing project text store and call `make_scene_prefab_instance_refresh_action`; log a warning on blocked review or load failure.
- [x] Keep file path conversion, loading, and ImGui state in `mirakana_editor`; do not move I/O or Dear ImGui into `editor/core`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, and Static Checks

- [x] Update editor docs, current capabilities, roadmap, testing docs, master plan, registry, manifest, and Codex/Claude editor skills with the narrow ready boundary.
- [x] Add static check sentinels for the new API names, retained `scene_prefab_instance_refresh` rows, tests, docs, manifest guidance, and unsupported boundary text.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.

### Task 5: Closeout Validation

- [x] Run `git diff --check` on touched files.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence in this plan.
- [x] Update this plan status to `Completed`, point `currentActivePlan` back to the master plan, update the plan registry latest completed slice, and keep `recommendedNextPlan.id=next-production-gap-selection`.

## Done When

- Focused editor-core tests prove RED then GREEN for review/apply and blockers.
- Visible `mirakana_editor` can explicitly refresh a selected linked prefab root from its reviewed source path.
- Docs, skills, manifest, and static checks agree that this is a narrow reviewed scene instance refresh surface, not full nested prefab propagation or broad merge/rebase UX.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes, or a concrete host/tool blocker is recorded here.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_editor_core_tests` | Passed | RED before implementation failed on the missing `ScenePrefabInstanceRefresh*` API surface; the focused target then built successfully after implementation. |
| `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` | Passed | Focused editor-core test run passed, including refresh apply/undo and unsafe-mapping blockers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed | Visible `mirakana_editor` adapter and desktop GUI lane built; desktop-gui CTest completed 46/46. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public editor-core header additions accepted by the API boundary check. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest/static JSON contracts passed after docs/manifest synchronization. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration sentinels passed after docs, manifest, skills, plan registry, and static-check updates. |
| `git diff --check -- .agents/skills/editor-change/SKILL.md .claude/skills/gameengine-editor/SKILL.md docs/editor.md docs/current-capabilities.md docs/roadmap.md docs/testing.md docs/superpowers/plans/README.md docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md Editor Scene Prefab Instance Refresh Review v1 editor/core/include/mirakana/editor/scene_authoring.hpp editor/core/src/scene_authoring.cpp editor/src/main.cpp engine/agent/manifest.json tests/unit/editor_core_tests.cpp tools/check-ai-integration.ps1 tools/check-json-contracts.ps1` | Passed | Whitespace check passed; Git reported line-ending normalization warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed. `production-readiness-audit-check` still reports 11 non-ready unsupported gaps, including `editor-productization` as `partly-ready`; Metal/Apple diagnostics remain host-gated. |
```

### Phase 4 5 Closure Milestone v1
- Retired file: `Phase 4 5 Closure Milestone v1`
- Phase 4–5 Production Closure Milestone v1 (2026-05-09)
- **Status:** Completed (closure evidence: Phase 4 5 Milestone Closure Evidence Index v1). Does **not** replace `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` unless maintainers explicitly pivot the active slice to this milestone.

### Phase 4 5 Milestone Closure Evidence Index v1
- Retired file: `Phase 4 5 Milestone Closure Evidence Index v1`
- Phase 4–5 Production Closure Milestone — Evidence Index v1 (2026-05-09)
- **Status:** Completed.

### Editor Input Rebinding Axis Capture Gamepad v1
- Retired file: `Editor Input Rebinding Axis Capture Gamepad v1`
- Editor Input Rebinding Axis Capture (Gamepad) v1 (2026-05-10)
- **Status:** Completed

Static guard source snapshot:

```markdown
# Editor Input Rebinding Axis Capture (Gamepad) v1 (2026-05-10)

**Plan ID:** `editor-input-rebinding-axis-capture-gamepad-v1`
**Gap:** `editor-productization`
**Parent:** [../master-plans/2026-05-03-production-completion-master-plan-v1.md](../2026-05-03-production-completion-master-plan-v1.md)
**Status:** Completed

## Goal

Add `capture_runtime_input_rebinding_axis` for deterministic gamepad axis detection and wire the editor Input Rebinding panel so axis bindings get a Capture workflow (gamepad stick/trigger only; keyboard key-pair capture stays out of scope).

## Context

Action rebinding capture already exists end-to-end; axis presentation and profile overrides exist, but there was no runtime capture path or editor controls for axis rows.

## Constraints

- MVP captures **gamepad axes** only; keyboard `key_pair` axis capture remains a separate slice.
- Keep SDL3 out of capture; use `RuntimeInputStateView` only.
- Do not broaden `editor-productization` ready claims beyond this narrow rebinding UX.

## Done when

- Public runtime API `capture_runtime_input_rebinding_axis` with unit tests (blocked / waiting / captured).
- Editor panel exposes Axis Capture controls and applies captured axis overrides to the in-memory profile when capture succeeds.
- `docs/editor.md` notes gamepad axis capture; `engine/agent/manifest.json` reflects new surfaces where required by validation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
```

### Editor Game Module Driver Active Session Hot Reload Fail Closed Order v1
- Retired file: `Editor Game Module Driver Active Session Hot Reload Fail Closed Order v1`
- Editor Game Module Driver Active Session Hot Reload Fail-Closed Order v1 (2026-05-11)

### Editor Game Module Driver Hot Reload Session State Machine Spec v1
- Retired file: `Editor Game Module Driver Hot Reload Session State Machine Spec v1`
- Editor Game Module Driver Hot Reload Session State Machine Spec v1 (2026-05-11)
- **Status:** Completed (spec + API + MK_ui + tests + manifest needles)

### Editor Productization Hot Reload Stable ABI Stream v1
- Retired file: `Editor Productization Hot Reload Stable ABI Stream v1`
- Editor Productization Hot Reload And Stable ABI Stream v1 (2026-05-11)
- **Status:** Completed (hot reload track exit; stable ABI exclusion path complete; mid-play DLL replacement remains unsupported)

### Asset Identity v2 Placement Resolution v1
- Retired file: `Asset Identity v2 Placement Resolution v1`
- Asset Identity v2 Placement Resolution v1 (2026-05-12)
- **Status:** Completed

### Editor Content Browser Asset Identity v2 Rows v1
- Retired file: `Editor Content Browser Asset Identity v2 Rows v1`
- Editor Content Browser Asset Identity v2 Rows v1 Implementation Plan (2026-05-12)
- **Status:** Completed
- **Goal:** Add read-only Asset Identity v2 key/source provenance rows to the GUI-independent Content Browser and retained Assets panel models.

### Runtime Resource v2 Resident Catalog Cache v1
- Retired file: `Runtime Resource v2 Resident Catalog Cache v1`
- Runtime Resource v2 Resident Catalog Cache v1 (2026-05-12)
- **Status:** Completed

### Runtime Resource v2 Resident Package Mount Set v1
- Retired file: `Runtime Resource v2 Resident Package Mount Set v1`
- Runtime Resource v2 Resident Package Mount Set v1 (2026-05-12)
- **Status:** Completed.

### Frame Graph D3D12 Texture Aliasing Barrier Evidence v1
- Retired file: `Frame Graph D3D12 Texture Aliasing Barrier Evidence v1`
- Frame Graph D3D12 Texture Aliasing Barrier Evidence v1 Implementation Plan
- ## Status
- **Goal:** Make the D3D12 texture aliasing barrier implementation and durable agent guidance agree by recording a conservative backend-private null-resource D3D12 aliasing barrier after public handle validation.

### Frame Graph Texture Aliasing Barrier Command v1
- Retired file: `Frame Graph Texture Aliasing Barrier Command v1`
- Frame Graph Texture Aliasing Barrier Command v1 Implementation Plan
- ## Status
- **Goal:** Add a backend-neutral texture aliasing barrier command contract that Frame Graph helpers can record before future native transient heap alias allocation is implemented.

### Frame Graph Backend Neutral Distinct Alias Group Lease Binding v1
- Retired file: `Frame Graph Backend Neutral Distinct Alias Group Lease Binding v1`
- 2026-05-17 Frame Graph Backend-Neutral Distinct Alias-Group Lease Binding v1 Implementation Plan
- ## Status
- **Goal:** Bind Frame Graph transient texture alias groups to backend-neutral RHI alias-group leases that return distinct `TextureHandle` rows per resource.

### Frame Graph v1 1 0 Scope Closeout v1
- Retired file: `Frame Graph v1 1 0 Scope Closeout v1`
- Frame Graph v1 1.0 Scope Closeout v1 Implementation Plan
- **Status:** Completed.
- **Goal:** Close `frame-graph-v1` for the Engine 1.0 Windows-default ready surface without claiming a broad production render graph, async overlap/performance, Metal memory alias allocation, public native handles, or actual content preservation.

Static guard source snapshot:

```markdown
# Frame Graph v1 1.0 Scope Closeout v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close `frame-graph-v1` for the Engine 1.0 Windows-default ready surface without claiming a broad production render graph, async overlap/performance, Metal memory alias allocation, public native handles, or actual content preservation.

**Architecture:** `MK_renderer` owns the Frame Graph v1 planning and RHI execution foundation for selected renderer/runtime upload paths. D3D12 and Vulkan now have backend-private transient alias allocation evidence, selected desktop package smokes expose render-pass and multi-queue counters, and the production ownership planner fails closed for broader graph migration claims that remain future or host-gated.

**Tech Stack:** C++23 renderer/RHI/runtime contracts, `engine/agent/manifest.fragments`, `tools/check-ai-integration*.ps1`, `tools/check-json-contracts*.ps1`, package validation scripts, and docs/roadmap/AI game-development guidance.

---

**Plan ID:** `frame-graph-v1-1-0-scope-closeout-v1`

**Status:** Completed.

**Parent:** [../master-plans/2026-05-03-production-completion-master-plan-v1.md](../2026-05-03-production-completion-master-plan-v1.md)

**Gap:** `frame-graph-v1`

## Context

- The `frame-graph-v1` burn-down now has selected execution evidence for Frame Graph v1 planning, callback dispatch, texture transitions, pass target-state preparation, final-state restoration, render-pass envelopes, queue dependency waits, multi-queue command submission, automatic aliasing barriers, package-visible render-pass and multi-queue counters, runtime texture/mesh/skinned/morph/material-factor upload command evidence, package-streaming texture binding handoff/transaction bridges, D3D12 placed alias resources, Vulkan alias memory, and Frame Graph Production Ownership Boundary Selection v1 (`FrameGraphProductionOwnershipPlan` / `plan_frame_graph_production_ownership_boundary`) fail-closed production ownership boundary planning.
- The latest `FrameGraphRhiMultiQueuePackageEvidence` package smoke validates `sample_desktop_runtime_game --require-framegraph-multiqueue-evidence` with four submitted command lists, three queue waits, four texture barriers, one aliasing barrier, and four submitted pass fences over transient alias-group lease bindings.
- Remaining broad claims are no longer required blockers for the Engine 1.0 Windows-default ready surface: broad production render graph scheduling, broad/background package streaming, Metal memory alias allocation, async overlap/performance, actual content preservation across aliases, public native handles, and general renderer quality stay explicit future or host-gated work.

## Constraints

- Do not change C++ behavior in this closeout slice.
- Do not claim broad production graph ownership beyond the selected renderer/runtime upload and package evidence already validated.
- Do not claim Metal readiness, data inheritance/content preservation, async overlap/performance, public native/RHI handles, or broad renderer quality.
- Keep `upload-staging-v1` as the next Phase 1 foundation gap and keep package streaming/upload staging limits explicit.
- Keep `engine/agent/manifest.json` composed output only; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.

## Done When

- `frame-graph-v1` is removed from `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json.unsupportedProductionGaps`, and the composed manifest is regenerated.
- `recommendedNextPlan` records this closeout and points the next Phase 1 foundation work at `upload-staging-v1`.
- Plan registry, master plan, roadmap, current capabilities, architecture, AI game-development guidance, rendering skills, and static guards agree that Frame Graph v1 is closed for Engine 1.0 Windows-default scope.
- Static validation proves the manifest is valid and the production readiness audit lists the remaining unsupported rows.

## Tasks

- [x] **Task 1: Close the machine-readable gap row.**
  Remove `frame-graph-v1` from `unsupportedProductionGaps`, update `recommendedNextPlan` to select `upload-staging-v1`, and regenerate the composed manifest.

- [x] **Task 2: Synchronize current-truth docs and skills.**
  Update this registry, the master plan, roadmap, current capabilities, architecture, AI game-development guidance, and Codex/Claude rendering guidance so the closeout and future/host-gated exclusions are explicit.

- [x] **Task 3: Update static guards.**
  Change manifest/static guard checks so stale `frame-graph-v1` unsupported rows fail validation and `upload-staging-v1` becomes the next selected Phase 1 gap.

- [x] **Task 4: Validate and publish.**
  Run focused manifest/agent/static checks, commit the validated closeout, push the existing PR branch, and re-check PR status.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass | Regenerated `engine/agent/manifest.json` from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `agent-manifest-compose: ok`; `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass | `agent-config-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `unsupported_gaps=7`; no `frame-graph-v1` row. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | `text-format-check: ok`; `format-check: ok`. |

## Agent Surface Drift Review

- Checked. Durable closeout behavior changed the machine-readable production gap contract, so docs, plan registry, roadmap, current capabilities, AI game-development guidance, Codex/Claude rendering guidance, manifest fragments, composed manifest, and static guards were updated in the same slice.

## Next Candidate After Validation

- Continue the Phase 1 foundation order with `upload-staging-v1`.
```

### Gameplay Physics Navigation AI Foundation v1
- Retired file: `Gameplay Physics Navigation AI Foundation v1`
- Gameplay Physics / AI Navigation / 2D-3D Essential Systems Expansion Master v1 (2026-05-19)
- **Status:** Completed.

### Engine Entity Scale And Culling v1
- Retired file: `Engine Entity Scale And Culling v1`
- Engine Entity Scale and Culling v1 (2026-05-21)
- **Status:** Completed.

### Engine Scripting Sandbox v1
- Retired file: `Engine Scripting Sandbox v1`
- Engine Scripting Sandbox v1 (2026-05-21)
- **Status:** Completed.

### Engine World Region Streaming v1
- Retired file: `Engine World Region Streaming v1`
- Engine World Region Streaming v1 (2026-05-21)
- **Status:** Completed.

### Generated 3d Entity Scale Culling Package Evidence v1
- Retired file: `Generated 3d Entity Scale Culling Package Evidence v1`
- Generated 3D Entity Scale Culling Package Evidence v1 (2026-05-21)
- **Status:** Completed.

### Renderer Backend Parity v1
- Retired file: `Renderer Backend Parity v1`
- Renderer Backend Parity v1 (2026-05-21)
- **Status:** Completed.

### Renderer Debug Profiling v1
- Retired file: `Renderer Debug Profiling v1`
- Renderer Debug Profiling v1 (2026-05-21)
- **Status:** Completed.

### Renderer GPU Memory v1
- Retired file: `Renderer GPU Memory v1`
- Renderer GPU Memory v1 (2026-05-21)
- **Status:** Completed.

### AI Gameplay Authoring Tools v1
- Retired file: `AI Gameplay Authoring Tools v1`
- AI Gameplay Authoring Tools v1 (2026-05-22)
- **Status:** Completed.

### Engine Networking Foundation v1
- Retired file: `Engine Networking Foundation v1`
- Engine Networking Foundation v1 (2026-05-22)
- **Status:** Completed.

### Gameplay Simulation Orchestration v1
- Retired file: `Gameplay Simulation Orchestration v1`
- Gameplay Simulation Orchestration v1 (2026-05-22)
- **Status:** Completed.

## Retired governance and bridge plan corpus (2026-05-22 plan volume compression)

These formerly retained dated plan files were folded into current docs, specs, ADRs, legal/workflow guidance, or this archive. They are no longer active implementation-plan files under `docs/superpowers/plans/`.

### Directory layout tools split v1 (2026-05-11)
- Retired file: `docs/superpowers/plans/2026-05-11-directory-layout-tools-split-v1.md`
- Full snapshot preserved during 2026-05-22 plan volume compression.

```markdown
# Directory layout tools split v1 (2026-05-11)

## Goal

Split `engine/tools` implementation sources into named subdirectories with CMake `OBJECT` libraries aggregated by `MK_tools`, per [ADR 0003](../../../adr/0003-directory-layout-clean-break.md) and [directory layout target v1](../../../specs/2026-05-11-directory-layout-target-v1.md).

## Context

- Public headers remain `engine/tools/include/mirakana/tools/*.hpp`.
- `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` reference specific `.cpp` paths for contract needles; those paths move with the split.
- Follow-up: each `MK_tools_*` `OBJECT` target uses minimal `target_link_libraries` for compile; `MK_tools` retains the full `PUBLIC` link set for consumers (see spec invariant 4).

## Done when

- [x] Subdirs `engine/tools/shader`, `gltf`, `asset`, `scene` exist with sources and `CMakeLists.txt` fragments.
- [x] `MK_tools` is a `STATIC` library built from `$<TARGET_OBJECTS:...>` of the four object targets.
- [x] Optional importer defines and `miniaudio` / `fastgltf` / `spng` wiring behave as before.
- [x] Validation scripts updated for new `.cpp` paths.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` green on a capable host.

## Validation

| Check | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass (Windows dev preset; host-gated Apple/Metal diagnostics only). |

## Integration

- Plan registry and master plan **Context** treat this slice as **completed foundation**, not a gap burn-down. Navigation and clean-break doc policy: `2026-05-11-production-documentation-stack-v1.md`.
```

### Editor Game Module Driver Stable Third-Party ABI 1.0 Exclusion v1 (2026-05-11)
- Retired file: `docs/superpowers/plans/2026-05-11-editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1.md`
- Full snapshot preserved during 2026-05-22 plan volume compression.

```markdown
# Editor Game Module Driver Stable Third-Party ABI 1.0 Exclusion v1 (2026-05-11)

**Plan ID:** `editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1`
**Gap:** `editor-productization` (hot reload + stable ABI stream — stable ABI track exit via exclusion)
**Parent:** [../master-plans/2026-05-03-production-completion-master-plan-v1.md](../2026-05-03-production-completion-master-plan-v1.md)
**Stream:** Editor Productization Hot Reload Stable ABI Stream v1

## Goal

Record an explicit **1.0 product decision**: the engine does **not** ship or support a **stable third-party binary ABI** for out-of-tree editor game module DLLs. Same-repository, **same-engine-build** dynamic loading of `GameEngine.EditorGameModuleDriver.v1` remains the reviewed interchange surface.

## Context

Editor Productization Hot Reload Stable ABI Stream v1 requires the stable ABI track to close with either an external SDK contract **or** a **manifest-backed exclusion with rationale**. Implementation slices already document same-build metadata and stopped-state reload transactions; this slice closes the **exclusion** path for 1.0.

## Constraints

- Do not imply third-party SDK stability, LTS C exports, or cross-minor binary compatibility for editor DLLs.
- Do not remove honest `requiredBeforeReadyClaim` wording for **hot reload** or broader editor productization from the manifest gap row; this slice addresses only the **stable third-party ABI** branch of the stream exit criteria.
- ASCII for code paths and public identifiers in tracked docs per repository policy.

## Done when

- [`docs/legal-and-licensing.md`](../../../legal-and-licensing.md) contains a dedicated subsection stating the 1.0 exclusion and engineering rationale for editor game module DLLs.
- [`docs/dependencies.md`](../../../dependencies.md) cross-references that boundary (no new vcpkg entries required for this policy-only slice).
- Stream ledger Editor Productization Hot Reload Stable ABI Stream v1 records this slice as completing the **stable ABI track** exclusion path.
- The retired production-completion gap stream index listed this plan under the hot reload + stable ABI stream; that index is now Git-history evidence.
- [`docs/superpowers/plans/README.md`](../../plans/README.md) active-work notes mention the completed exclusion slice where hot-reload stream progress is summarized.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` `recommendedNextPlan` / gap narrative reflects the exclusion (no change to `unsupportedProductionGaps[].id` list required for a documentation-only 1.0 boundary).
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass.

## Validation evidence

| Step | Command / artifact | Result |
| --- | --- | --- |
| Compose manifest | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass |
| Repo gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass |
```

### Editor Resource Capture PIX Host Handoff Evidence v1 (2026-05-11)
- Retired file: `docs/superpowers/plans/2026-05-11-editor-resource-capture-pix-host-handoff-evidence-v1.md`
- Full snapshot preserved during 2026-05-22 plan volume compression.

```markdown
# Editor Resource Capture PIX Host Handoff Evidence v1 (2026-05-11)

**Plan ID:** `editor-resource-capture-pix-host-handoff-evidence-v1`
**Gap:** `editor-productization` (shared diagnostics with `renderer-rhi-resource-foundation`)
**Parent:** [../master-plans/2026-05-03-production-completion-master-plan-v1.md](../2026-05-03-production-completion-master-plan-v1.md)
**Stream:** editor-productization resource-management execution stream (stream item 3; retired stream ledger retained through Git history)

## Goal

After the operator acknowledges the retained `pix_gpu_capture` capture request, `MK_editor` on Windows may run a **single reviewed** `pwsh` invocation of `tools/launch-pix-host-helper.ps1` with `-SkipLaunch`, capture stdout/stderr through `Win32ProcessRunner`, and surface **sanitized** session scratch path plus process output summary in existing `resources.capture_execution` / `EditorResourceCaptureExecutionInput` rows. This closes the stream ledger gap between “helper script exists” and “editor-visible verified host-helper execution evidence” without claiming GPU capture completion, automatic AI command execution, or native RHI handle exposure.

## Context

- `2026-05-11-editor-resource-capture-pix-launch-helper-v1.md` added `tools/launch-pix-host-helper.ps1` (no editor integration).
- [editor/core/src/resource_panel.cpp](../../../../editor/core/src/resource_panel.cpp) already materializes `resources.capture_execution.*` MK_ui rows with bounded artifact/diagnostic fields.
- `mirakana::run_process_command` and `Win32ProcessRunner` historically allowed only non-shell executables; reviewed `pwsh` allowlists must extend the shared gate alongside `is_safe_reviewed_validation_recipe_invocation`.

## Constraints

- Microsoft host diagnostics policy: do not bundle PIX; do not auto-run host tools from AI command surfaces without reviewed operator acknowledgement rows.
- Default editor action uses **`-SkipLaunch`** so CI and hosts without PIX can still observe deterministic process exit behavior when PIX is discoverable; full PIX UI launch remains an operator choice outside the default button.
- No new public gameplay API for native handles; artifact text stays host file paths under `%LocalAppData%` scratch roots only.
- Unacknowledged automatic execution of host-gated validation recipes remains out of scope.

## Cross-link: `renderer-rhi-resource-foundation`

GPU capture and RHI lifetime evidence must stay aligned with native backend teardown and registry diagnostics tracked under gap `renderer-rhi-resource-foundation` (see `engine/agent/manifest.json` for the current gap state). This slice records **host-helper** evidence only; it does not close RHI teardown migration.

## Done when

- Windows `MK_editor` Resources panel exposes an operator control on the PIX capture execution row that runs the reviewed helper with `-SkipLaunch` when the repo `tools/launch-pix-host-helper.ps1` is discoverable from the process current working directory walk, and refreshes retained execution diagnostics/artifact fields from process output (bounded).
- `mirakana::is_allowed_process_command` (or equivalent) permits `Win32ProcessRunner` + `run_process_command` to execute both `tools/run-validation-recipe.ps1` and `tools/launch-pix-host-helper.ps1` reviewed argv shapes.
- `MK_editor_core_tests` / resource panel contract coverage updated for any new MK_ui element ids.
- `engine/agent/manifest.fragments` + `tools/compose-agent-manifest.ps1 -Write`, plan registry stream item 3 pointer, and `docs/editor.md` operator notes updated.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the implementation host.

## Validation evidence

| Check | Result |
| --- | --- |
| `MK_editor_core_tests` | `resources.capture_execution.pix_gpu_capture.host_helper_hint` contract assertion |
| `MK_core_tests` | `process contract accepts reviewed pwsh pix host helper invocations` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Run on the implementation host at slice close |

## Non-goals

- Launching PIX UI by default from the editor button (operators may run the script manually with launch enabled).
- Clearing Vulkan/Metal material-preview `requiredBeforeReadyClaim` rows.
- Marking `editor-productization` gap ready or removing `resource management/capture execution` follow-ups beyond this reviewed helper evidence slice.
```

### Editor Resource Capture PIX Host Helper v1 (2026-05-11)
- Retired file: `docs/superpowers/plans/2026-05-11-editor-resource-capture-pix-launch-helper-v1.md`
- Full snapshot preserved during 2026-05-22 plan volume compression.

```markdown
# Editor Resource Capture PIX Host Helper v1 (2026-05-11)

**Plan ID:** `editor-resource-capture-pix-launch-helper-v1`
**Gap:** `editor-productization`
**Parent:** editor-productization resource-management execution stream (retired stream ledger retained through Git history)
**Status:** Completed (host-operator helper only; no editor-core mutation)

## Goal

Provide an **optional** repository `tools/` entrypoint that helps Windows operators launch **Microsoft PIX** from a **non-repository scratch directory** under `%LocalAppData%`, matching the resource execution stream item 2 intent without bundling PIX, auto-launching from `MK_editor`, or recording native handles in the engine.

## Context

- Editor Resource Capture Execution Evidence v1 keeps PIX launch out of `editor/core`; execution evidence remains caller-supplied.
- The resource management execution stream listed an optional PIX wrapper as stream item 2; the stream ledger is now Git-history evidence.

## Constraints

- PowerShell 7 Core only; follow `tools/*.ps1` `#requires` contract and UTF-8 without BOM (`tools/check-agents.ps1`).
- No writes under the repository root except this script and docs; scratch data lives under `%LocalAppData%\MirakanaiEngine\pix-host-helper\...`.
- Do not claim `requiredBeforeReadyClaim` clearance for resource capture execution; this is operator ergonomics only.

## Done when

- [x] `tools/launch-pix-host-helper.ps1` resolves `PIX.exe` via `MIRAKANA_PIX_EXE`, standard `Program Files` paths, or `PATH`, creates a session scratch directory, writes a short `README.txt` there, and starts PIX with that working directory when found.
- [x] Script fails fast with a clear message when PIX is not installed (exit code non-zero).
- [x] Stream ledger item 2 marked complete with link to this plan.
- [x] Plan registry lists this slice as latest completed evidence where appropriate.
- [x] `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` `editor-productization` notes reference this helper; composed manifest regenerated.

## Validation

| Check | Result |
| --- | --- |
| `tools/check-agents.ps1` (via `validate.ps1`) accepts new `tools/*.ps1` | PASS |
| `tools/launch-pix-host-helper.ps1 -SkipLaunch` on host without PIX | PASS (expected `Write-Error`, exit code 1) |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |

## Operator usage

```text
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/launch-pix-host-helper.ps1
```

Optional: set `MIRAKANA_PIX_EXE` to the full path of `WinPix.exe` or legacy `PIX.exe` when Microsoft PIX is installed outside default locations.
```

### Editor Resources Capture Operator Validated Launch Workflow v1 (2026-05-11)
- Retired file: `docs/superpowers/plans/2026-05-11-editor-resources-capture-operator-validated-launch-workflow-v1.md`
- Full snapshot preserved during 2026-05-22 plan volume compression.

```markdown
# Editor Resources Capture Operator Validated Launch Workflow v1 (2026-05-11)

**Plan ID:** `editor-resources-capture-operator-validated-launch-workflow-v1`
**Gap:** `editor-productization` (resource management execution stream)
**Parent:** [../master-plans/2026-05-03-production-completion-master-plan-v1.md](../2026-05-03-production-completion-master-plan-v1.md)
**Stream:** editor-productization resource-management execution stream (retired stream ledger retained through Git history)
**Gap index:** retired production-completion gap stream index (Git-history evidence)

## Goal

Close the resource execution stream toward **operator-validated external tool invocation**: after acknowledging the PIX capture request, Windows `MK_editor` offers a **second reviewed control** that runs `tools/launch-pix-host-helper.ps1` **without** `-SkipLaunch` only behind an explicit confirmation modal, records stdout/stderr and session scratch paths in existing `resources.capture_execution` evidence rows, and exposes a retained contract label for agent drift checks—without automatic launch, AI-driven execution, or native RHI handle exposure.

## Context

- Items 1–4 of the resource management execution stream are complete (bounded rows, PIX helper script, `-SkipLaunch` handoff, contract label).
- `mirakana::is_safe_reviewed_pix_host_helper_invocation` already allows exactly five `pwsh` arguments (no `-SkipLaunch`) alongside the six-argument `-SkipLaunch` shape (`engine/platform/src/process.cpp`).
- Stream exit still requires a documented operator loop to **verified** external invocation; the default path remains `-SkipLaunch` for CI and hosts without PIX.

## Constraints

- Microsoft host diagnostics policy: PIX is not bundled; no default automatic PIX start from the first button.
- Confirmation modal text must state that the Microsoft PIX UI process may start; operator must confirm.
- `Win32ProcessRunner` / `run_process_command` only; argv must remain allowlisted.
- No new gameplay or RHI public handle surfaces.

## Done when

- Retained `resources.capture_execution.operator_validated_launch_workflow_contract_label` with `ge.editor.resources_capture_operator_validated_launch_workflow.v1` appears in `make_resource_panel_ui_model` when capture execution rows exist, before per-row items (alongside the existing capture execution contract label).
- Windows Resources **Capture Execution Evidence** table exposes **Run helper (launch PIX)** (or equivalent label) with modal confirm; on confirm runs reviewed `pwsh` with five-arg `launch-pix-host-helper.ps1` (no `-SkipLaunch`).
- Execution diagnostics in the PIX row distinguish skip-launch vs launch-enabled helper runs when `pix_host_helper_last_run_valid_` is true.
- `MK_core_tests` asserts the five-arg PIX helper command is allowed.
- `MK_editor_core_tests` asserts the new retained element id and contract string.
- `tools/check-ai-integration.ps1` needles updated for new symbols and ids.
- `docs/editor.md` Resources section documents both controls and the launch confirmation boundary.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` `editor-productization` notes reference this slice; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` run after fragment edits.

## Validation evidence

| Step | Command / artifact | Expected |
| --- | --- | --- |
| Core process contract | `ctest --preset dev -R MK_core_tests --output-on-failure` | Pass (`process contract accepts reviewed pwsh pix host helper invocations` covers 5-arg) |
| Editor core | `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | Pass |
| Compose manifest | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass on implementation host |

## Checklist

- [x] Plan + gap index + stream ledger pointers
- [x] Retained MK_ui contract label row + `resource_panel.hpp` constexpr
- [x] `MK_editor` modal + `execute_pix_host_helper_reviewed(bool skip_launch)`
- [x] Tests + `check-ai-integration` needles
- [x] `docs/editor.md` + manifest fragment notes + compose

## Next concrete follow-up (optional)

- Extend operator workflow docs with screenshots or a short **Recommended workflow** subsection tying PIX session dir → attach capture for AI analysis (`docs/ai-integration.md` cross-link only if needed).
- Stream exit: reconcile `requiredBeforeReadyClaim` wording after broader operator evidence is recorded (separate manifest slice if narrowing claims).
```

### Production documentation stack v1 (2026-05-11)
- Retired file: `docs/superpowers/plans/2026-05-11-production-documentation-stack-v1.md`
- Full snapshot preserved during 2026-05-22 plan volume compression.

```markdown
# Production documentation stack v1 (2026-05-11)

## Goal

Define a **clean-break, no-backward-compatibility** documentation authority stack so production execution does not require bulk-rewriting the master plan ledger whenever repository layout or agent surfaces change.

## Context

- The master plan file is intentionally a **long-form ledger** (verdict snapshots, completed slice references). Treat it as historical evidence plus high-level gap strategy, not as the only place to record physical tree rules.
- Recent `engine/tools` layout work is a **foundation cross-cut**: it constrains CMake, validation needles, and agent skills, but it is not a new `unsupportedProductionGaps` row.
- AGENTS.md plan-volume policy discourages huge pointer churn inside completed child plans and discourages creating micro-plans for pure docs sync; this file is the **single governance slice** for “how plans relate to specs/ADRs.”

## Authority stack (read order)

1. **`engine/agent/manifest.json`** — `aiOperableProductionLoop.currentActivePlan`, `recommendedNextPlan`, `unsupportedProductionGaps`, readiness recipes.
2. **`docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`** — production completion definition, gap burn-down strategy, current verdict pointers (must stay consistent with manifest for *active gap* and *ready claims*).
3. **`docs/superpowers/plans/README.md`** — registry: what is active vs completed; links to latest slices.
4. **Dated child plans** (`docs/superpowers/plans/2026-*-*.md`) — scoped Goal / Done when / validation tables; **do not rewrite** completed files to “fix” old path strings; add a short note in the registry or this stack doc instead.
5. **Specs and ADRs** — durable layout and invariant truth (example: [directory layout target v1](../../../specs/2026-05-11-directory-layout-target-v1.md), [ADR 0003](../../../adr/0003-directory-layout-clean-break.md)).
6. **`docs/architecture.md`**, **`docs/workflows.md`**, **`docs/current-capabilities.md`**, **`docs/roadmap.md`** — human-facing summaries; must not contradict (1)–(5).

## Clean-break rules

- **No compatibility shims** for removed physical layouts (for example do not reintroduce a flat `engine/tools/src/` tree “for old docs or scripts”).
- When CMake paths, needles in `tools/check-json-contracts.ps1` / `tools/check-ai-integration.ps1`, or agent skills change, update **spec/ADR + skills + scripts in one task**, per `directory layout tools split v1`.
- **Do not** expand the master plan’s “Previous Verdict Snapshot” or historical paragraphs to enumerate every path migration; link **spec invariant 4** for `MK_tools` link policy instead.

## Done when

- [x] Master plan **Context** references this stack and the directory layout spec/ADR as foundation authority.
- [x] Plan registry lists the directory layout slice as completed foundation evidence and points readers here for stack rules.
- [x] `Directory layout tools split v1` links back to this document under integration.

## Validation

| Check | Result |
| --- | --- |
| Manifest `currentActivePlan` unchanged unless a deliberate execution switch | N/A (docs-only governance); manifest still points at master plan for gap burn-down. |
| No contradiction between spec invariant 4 and CMake skills | Verified at authoring time. |
```

### Post-1.0 Capability Program v1 (2026-05-22)
- Retired file: `docs/superpowers/plans/2026-05-22-post-1-0-capability-program-v1.md`
- Full snapshot preserved during 2026-05-22 plan volume compression.

```markdown
# Post-1.0 Capability Program v1 (2026-05-22)

**Plan ID:** `post-1-0-capability-program-v1`
**Status:** Completed; closed at Phase 2.
**Milestone type:** Post-1.0 / 1.x phase-gated capability program.
**Closed phase:** Phase 2 - `physics-vehicles-and-kinematics-v1`.

## Goal

Select and execute the next coherent post-1.0 / 1.x engine capability program after the 1.0 closeout backlog reached `unsupportedProductionGaps = []`.

This plan kept the production-completion pointer on one active dated milestone through the Phase 1 and Phase 2 physics checkpoints. It is now closed at that coherent boundary; later navigation, large-world, persistence, and AI game-creation candidate streams return to the master backlog / selection pool.

## Context

- The 1.0 Windows-default closeout surface remains zero-gap: `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps = []`.
- The previous selection-gate state intentionally pointed `currentActivePlan` back to the production-completion master plan until a concrete roadmap decision selected the next developer-owned capability.
- The selected decision kept the post-1.0 / 1.x candidate backlog active as a single phase-gated milestone through `physics-constraints-and-joints-v1` and `physics-vehicles-and-kinematics-v1`; closeout returns later candidates to backlog selection.
- Existing completed foundations remain evidence, not active work: `physics-collision-query-v1`, `physics-joints-foundation-v1`, `engine-advanced-physics-controller-v1`, `engine-navmesh-crowd-v1`, `ai-perception-services-v1`, `gameplay-simulation-orchestration-v1`, `engine-world-region-streaming-v1`, `engine-entity-scale-and-culling-v1`, and `ai-gameplay-authoring-tools-v1`.

## Constraints

- Do not re-open 1.0 readiness rows. New work in this plan is post-1.0 / 1.x unless a later plan explicitly promotes a phase into a release-ready definition.
- Keep `unsupportedProductionGaps = []`; selected post-1.0 work must not be represented as an unsupported 1.0 blocker.
- Use first-party value contracts in engine modules. Middleware such as Jolt, PhysX, Recast/Detour, Havok, or scripting runtimes requires a separate optional-adapter plan with dependency, legal, manifest, schema, and validation updates before adoption.
- Use tests first for production behavior/API changes when the local environment can run them.
- Keep public APIs free of backend, platform, editor, renderer, SDL3, and middleware-native handles.
- Update agent-operable surfaces only when durable behavior, validation recipes, manifest claims, or command contracts change.

## Candidate Map

| Phase | Capability row | Plan action | Done boundary |
| --- | --- | --- | --- |
| 0 | `post-1-0-capability-program-v1` | Select this milestone, update registry/master-plan/matrix/advanced-track/current-capabilities/manifest pointers. | Focused docs/manifest checks passed while this plan was active. |
| 1 | `physics-constraints-and-joints-v1` | Extend first-party `MK_physics` joint/constraint evidence beyond the completed distance-joint foundation. | Deterministic constraint rows, invalid-input diagnostics, package-visible counters, and focused/full validation evidence. |
| 2 | `physics-vehicles-and-kinematics-v1` | Add kinematic body and simple vehicle policy foundations for small games. | Fixed-step movement policy, interaction diagnostics, package-visible evidence, and explicit non-goals for AAA vehicle physics. |
| 3 | `navigation-hierarchical-world-v1` | Returned to backlog after Phase 2 closeout. | Select through a new dated plan before implementation. |
| 4 | `world-streaming-and-large-scenes-v1` plus `simulation-persistence-v1` | Returned to backlog after Phase 2 closeout. | Select through a new dated plan before implementation. |
| 5 | AI game-creation follow-ons | Returned to backlog after Phase 2 closeout. | Select through a new dated plan before implementation. |

## Phase 0 - Plan Surface Sync

### Work

- [x] Create this active milestone plan.
- [x] Update the plan registry current active work row.
- [x] Update the production-completion master-plan verdict.
- [x] Update the 2D/3D matrix so completed foundations are not described as active 1.0 gaps.
- [x] Update the gameplay physics/navigation/AI advanced track with the selected post-1.0 stream and completed lower-level foundations.
- [x] Update current-capabilities active-work text.
- [x] Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, compose `engine/agent/manifest.json`, and keep `unsupportedProductionGaps = []`.

### Done When

- `currentActivePlan` pointed to this plan during the implementation phase.
- `recommendedNextPlan.id` pointed to `physics-vehicles-and-kinematics-v1` during the implementation phase.
- Related planning surfaces agreed that Phase 2 was active during implementation; closeout returns `currentActivePlan` to the production-completion master plan with `recommendedNextPlan.id = next-production-gap-selection`.
- Focused docs/manifest validation passes.

## Phase 1 - Physics Constraints and Joints v1

### Goal

Advance the existing distance-joint foundation into a broader first-party constraints and joints surface suitable for post-1.0 gameplay systems.

### Planned Scope

- Preserve existing `physics-joints-foundation-v1` behavior as compatibility within the same greenfield task, without adding deprecated aliases.
- Add deterministic value rows for at least fixed and linear-axis-style 3D constraints, while retaining distance-joint evidence.
- Add explicit diagnostics for invalid bodies, invalid axes/anchors/limits, impossible limits, exhausted constraint budgets, and unsupported constraint kinds.
- Add stable solve ordering and bounded iteration policy evidence.
- Add package-visible counters for selected generated 3D gameplay-system evidence.
- Document non-goals: ragdoll authoring, vehicle suspension, soft bodies, cloth, destructible physics, and middleware-native handles.

### First Implementation Slice

- [x] Add RED tests for fixed/linear-axis constraint rows and invalid linear-axis axis/limit mutation safety.
- [x] Add `PhysicsConstraint3DStatus`, `PhysicsConstraint3DDiagnostic`, `PhysicsConstraint3DKind`, `PhysicsFixedConstraint3DDesc`, `PhysicsLinearAxisConstraint3DDesc`, `PhysicsConstraintSolve3DConfig`, `PhysicsConstraintSolve3DDesc`, `PhysicsConstraintSolve3DRow`, `PhysicsConstraintSolve3DResult`, and `solve_physics_constraints_3d`.
- [x] Add deterministic row order across distance, fixed, and linear-axis vectors; invalid-request validation happens before mutation.
- [x] Add explicit `PhysicsConstraintSolve3DConfig::max_rows` row-count budget with `row_budget_exceeded` diagnostics, default-unbounded behavior, and no world mutation on budget failure.
- [x] Add package-visible selected constraint counters to `sample_generated_desktop_runtime_3d_package`.
- [x] Update docs, skills, manifest fragments/composed manifest, and static AI-contract guards.
- [x] Decide whether Phase 1 needs an explicit row-count budget API before moving to Phase 2; Phase 1 now has explicit `max_rows` evidence, while unsupported constraint kinds remain a non-goal until an extensible constraint input surface exists.

### Done When

- A RED test demonstrates the missing constraint behavior.
- Focused `MK_physics` build/test passes after implementation.
- Public/API docs and agent surfaces are updated only where durable contracts changed.
- Full `tools/validate.ps1` passes before publication of the C++/runtime slice, or a concrete host/toolchain blocker is recorded.

## Phase 2 - Physics Vehicles and Kinematics v1

### Goal

Build simple kinematic and vehicle foundations on top of the Phase 1 constraint surface and existing advanced-controller policy.

### Implementation Slices

- [x] Add RED tests for deterministic kinematic fixed-step movement and package-visible simple vehicle evidence.
- [x] Add first-party `MK_physics` value contracts for value-only kinematic motion planning without middleware/native handles.
- [x] Add package-visible selected generated 3D counters for kinematic motion and public simple vehicle policy evidence.
- [x] Add RED tests for public `PhysicsSimpleVehicle3D*` value contracts and invalid wheel/filter diagnostics.
- [x] Add first-party `plan_physics_simple_vehicle_3d` policy rows that compose value-only kinematic motion with deterministic wheel probes without persistent vehicle bodies or drivetrain simulation.
- [x] Update generated 3D gameplay-systems package evidence to report `gameplay_systems_vehicle_status`, `gameplay_systems_vehicle_diagnostic`, `gameplay_systems_vehicle_wheel_rows`, grounded-wheel count, wheel-probe hit count, and final x.
- [x] Update docs, manifest fragments/composed manifest, templates, and static guard needles where durable contracts change.

### Done When

- Kinematic movement policy is deterministic under fixed timestep.
- Simple vehicle policy remains first-party and value-only; broad vehicle dynamics, persistent vehicle simulation, tire models, suspension tuning suites, and middleware vehicle modules are not claimed.
- Generated package evidence exposes selected counters.

## Deferred Backlog References

The following phases were not implemented in this plan. They remain post-1.0 / 1.x developer-owned backlog rows and require a new dated plan before work starts. Canonical backlog status lives in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md).

| Deferred area | Canonical backlog row ids |
| --- | --- |
| Navigation hierarchical world | `navigation-hierarchical-world-v1` |
| Large world and persistence | `world-streaming-and-large-scenes-v1`, `simulation-persistence-v1` |
| AI game-creation follow-ons | `ai-game-design-spec-v1`, `ai-game-generation-orchestrator-v1`, `ai-placeholder-asset-pipeline-v1`, `ai-generated-game-playtest-loop-v1`, `ai-validation-remediation-recipes-v1`, `ai-generated-game-quality-rubric-v1`, `ai-engine-capability-handoff-v1` |
## Validation Evidence

| Gate | Command | Evidence |
| --- | --- | --- |
| Phase 0 docs/manifest sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS on 2026-05-22 in this branch; `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| Phase 0 agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS on 2026-05-22 in this branch; `ai-integration-check: ok`. |
| Phase 0 agent surfaces | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS on 2026-05-22 in this branch; `agent-config-check: ok`. |
| Phase 1 RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` before implementation | Expected failure on 2026-05-22: missing `solve_physics_constraints_3d` and `PhysicsConstraint*` public types. |
| Phase 1 focused physics build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` | PASS on 2026-05-22 after implementation. |
| Phase 1 focused physics test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests"` | PASS on 2026-05-22; `MK_core_tests` passed. |
| Phase 1 row-budget RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` before row-budget implementation | Expected failure on 2026-05-22: missing `PhysicsConstraintSolve3DConfig::max_rows` and `PhysicsConstraint3DDiagnostic::row_budget_exceeded`. |
| Phase 1 row-budget focused physics test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests"` | PASS on 2026-05-22 after row-budget implementation and review follow-up; `MK_core_tests` passed, including `requested_rows == max_rows` acceptance and explicit aggregate `max_rows` call sites. |
| Phase 1 package build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_generated_desktop_runtime_3d_package` | PASS on 2026-05-22. |
| Phase 1 package smoke | `out\build\dev\games\Debug\sample_generated_desktop_runtime_3d_package\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-gameplay-systems` | PASS on 2026-05-22; reports `gameplay_systems_physics_constraints_status=solved`, `gameplay_systems_physics_constraints_rows=2`, fixed rows `1`, linear-axis rows `1`, and axis-limit-clamped `1`. |
| Phase 2 RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` before implementation | Expected failure on 2026-05-22: missing `PhysicsKinematicMotion3D*` public types and `plan_physics_kinematic_motion_3d`. |
| Phase 2 focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests sample_generated_desktop_runtime_3d_package` | PASS on 2026-05-22 after implementation. |
| Phase 2 focused test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` | PASS on 2026-05-22; kinematic motion tests cover deterministic slide rows, trigger overlap rows, invalid requests, initial-overlap diagnostics, and no caller-world mutation. |
| Phase 2 package smoke | `out\build\dev\games\Debug\sample_generated_desktop_runtime_3d_package\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-gameplay-systems` | PASS on 2026-05-22; reports `gameplay_systems_kinematic_motion_status=constrained`, `gameplay_systems_kinematic_motion_rows=2`, `gameplay_systems_vehicle_status=grounded`, `gameplay_systems_vehicle_diagnostic=none`, `gameplay_systems_vehicle_wheel_rows=4`, `gameplay_systems_vehicle_grounded_wheels=4`, and `gameplay_systems_vehicle_wheel_probe_hits=4`. |
| Phase 2 constraint-review RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` before constraint diagnostic hardening | Expected failure on 2026-05-22: exact linear-axis boundary was flagged as clamped and static-pair constraint rows did not retain kind-specific deltas. |
| Phase 2 constraint-review focused test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` | PASS on 2026-05-22 after constraint diagnostic hardening. |
| Phase 2 simple vehicle RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` before implementation | Expected failure on 2026-05-22: missing `PhysicsSimpleVehicle3D*` public types and `plan_physics_simple_vehicle_3d`. |
| Phase 2 simple vehicle focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests sample_generated_desktop_runtime_3d_package` | PASS on 2026-05-22 after implementation. |
| Phase 2 simple vehicle focused test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` | PASS on 2026-05-22; simple vehicle tests cover deterministic kinematic composition, wheel rows, invalid wheel/filter diagnostics, and no caller-world mutation. |
| Phase 2 simple vehicle package smoke | `out\build\dev\games\Debug\sample_generated_desktop_runtime_3d_package\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-gameplay-systems` | PASS on 2026-05-22; reports `gameplay_systems_vehicle_status=grounded`, `gameplay_systems_vehicle_diagnostic=none`, `gameplay_systems_vehicle_wheel_rows=4`, grounded wheels `4`, wheel-probe hits `4`, and final x `1`. |
| Phase 2 public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS on 2026-05-22. |
| Phase 2 format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `git diff --check` | PASS on 2026-05-22 after `tools/format.ps1`. |
| Phase 2 closeout pointer sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS on 2026-05-22; `currentActivePlan` returns to the production-completion master plan, `recommendedNextPlan.id = next-production-gap-selection`, `unsupportedProductionGaps = []`, `agent-manifest-compose: ok`, and `json-contract-check: ok`. |
| Phase 2 agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS on 2026-05-22 after simple vehicle and closeout pointer sync; `ai-integration-check: ok`. |
| Phase 2 agent surfaces | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS on 2026-05-22; `agent-config-check: ok`. |
| Phase 2 full gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticJobs 1` | PASS on 2026-05-22; all 73 CTest tests passed. Diagnostic-only host gates still report missing Apple/macOS and Metal host tooling on this Windows host. |
| Phase 1 public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS on 2026-05-22. |
| Phase 1 format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` plus `git diff --check` | PASS on 2026-05-22 after `tools/format.ps1` normalized C++ formatting. |
| Phase 1 docs/manifest sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS on 2026-05-22; `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| Phase 1 agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS on 2026-05-22; `ai-integration-check: ok`. |
| Phase 1 agent surfaces | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS on 2026-05-22; `agent-config-check: ok`. |
| Phase 1 full gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS on 2026-05-22; all 73 CTest tests passed. Diagnostic-only host gates still report missing Apple/macOS and Metal host tooling on this Windows host. |
```

## Retired plan filename index (2026-05-22 plan volume compression)

This compact index preserves retired dated plan filename literals for static guards while detailed prose lives in earlier archive sections or Git history.

- `2026-04-26-engine-excellence-roadmap.md`
- `2026-05-01-2d-desktop-runtime-package-proof-v1.md`
- `2026-05-01-2d-playable-vertical-slice-foundation-v1.md`
- `2026-05-01-3d-playable-vertical-slice-foundation-v1.md`
- `2026-05-01-ai-command-surface-foundation-v1.md`
- `2026-05-01-asset-identity-runtime-resource-v2.md`
- `2026-05-01-frame-graph-upload-staging-foundation-v1.md`
- `2026-05-01-game-manifest-runtime-scene-validation-targets-v1.md`
- `2026-05-01-material-instance-apply-tooling-v1.md`
- `2026-05-01-native-gpu-ui-overlay-foundation-v1.md`
- `2026-05-01-native-ui-atlas-package-metadata-foundation-v1.md`
- `2026-05-01-native-ui-textured-sprite-atlas-foundation-v1.md`
- `2026-05-01-registered-source-asset-cook-package-command-tooling-v1.md`
- `2026-05-01-renderer-rhi-resource-foundation-v1.md`
- `2026-05-01-runtime-scene-package-validation-command-tooling-v1.md`
- `2026-05-01-scene-component-prefab-schema-v2.md`
- `2026-05-01-scene-v2-registered-asset-runtime-workflow-validation-v1.md`
- `2026-05-01-scene-v2-runtime-package-migration-v1.md`
- `2026-05-01-source-asset-registration-command-tooling-v1.md`
- `2026-05-01-ui-atlas-metadata-apply-tooling-v1.md`
- `2026-05-01-ui-atlas-metadata-authoring-tooling-v1.md`
- `2026-05-01-validation-recipe-runner-tooling-v1.md`
- `2026-05-02-2d-atlas-tilemap-package-authoring-v1.md`
- `2026-05-02-2d-native-rhi-sprite-package-proof-v1.md`
- `2026-05-02-2d-packaged-playable-generation-loop-v1.md`
- `2026-05-02-2d-sprite-batch-package-telemetry-v1.md`
- `2026-05-02-2d-sprite-batch-planning-contract-v1.md`
- `2026-05-02-3d-packaged-playable-generation-loop-v1.md`
- `2026-05-02-3d-prefab-scene-package-authoring-v1.md`
- `2026-05-02-3d-scene-mesh-package-telemetry-v1.md`
- `2026-05-02-animation-cpu-skinned-bitangent-handedness-v1.md`
- `2026-05-02-animation-cpu-skinned-normal-stream-v1.md`
- `2026-05-02-animation-cpu-skinned-tangent-stream-v1.md`
- `2026-05-02-desktop-runtime-shippable-rhi-window-v1.md`
- `2026-05-02-editor-ai-package-authoring-diagnostics-v1.md`
- `2026-05-02-editor-ai-playtest-evidence-summary-v1.md`
- `2026-05-02-editor-ai-playtest-operator-handoff-v1.md`
- `2026-05-02-editor-ai-playtest-operator-workflow-ux-v1.md`
- `2026-05-02-editor-ai-playtest-readiness-report-v1.md`
- `2026-05-02-editor-ai-playtest-remediation-handoff-v1.md`
- `2026-05-02-editor-ai-playtest-remediation-queue-v1.md`
- `2026-05-02-editor-ai-validation-recipe-preflight-v1.md`
- `2026-05-02-editor-playtest-package-review-loop-v1.md`
- `2026-05-02-generated-2d-native-rhi-sprite-package-scaffold-v1.md`
- `2026-05-02-generated-3d-mesh-telemetry-scaffold-v1.md`
- `2026-05-02-host-gated-package-streaming-execution-v1.md`
- `2026-05-02-material-shader-authoring-loop-v1.md`
- `2026-05-02-package-streaming-residency-budget-contract-v1.md`
- `2026-05-02-renderer-resource-residency-upload-execution-v1.md`
- `2026-05-02-safe-point-package-unload-replacement-execution-v1.md`
- `2026-05-02-strict-clang-tidy-compile-database-enforcement-v1.md`
- `2026-05-03-ai-cook-package-command-surface-v1.md`
- `2026-05-03-broad-dependency-cook-plan-v1.md`
- `2026-05-03-cmake-install-export-and-cxx-modules-audit-v1.md`
- `2026-05-03-coverage-threshold-policy-v1.md`
- `2026-05-03-full-clang-tidy-warning-cleanup-v1.md`
- `2026-05-03-native-rhi-resource-lifetime-migration-v1.md`
- `2026-05-03-package-mount-and-resident-cache-v1.md`
- `2026-05-04-material-graph-authoring-v1.md`
- `2026-05-04-shader-graph-and-generation-pipeline-v1.md`
- `2026-05-05-generated-static-3d-production-game-recipe-v1.md`
- `2026-05-07-editor-resource-capture-execution-evidence-v1.md`
- `2026-05-07-editor-resource-capture-request-v1.md`
- `2026-05-07-editor-resource-panel-diagnostics-v1.md`
- `2026-05-09-editor-prefab-instance-local-child-refresh-resolution-v1.md`
- `2026-05-09-editor-scene-prefab-instance-refresh-review-v1.md`
- `2026-05-09-phase-4-5-closure-milestone-v1.md`
- `2026-05-09-phase-4-5-milestone-closure-evidence-index-v1.md`
- `2026-05-10-editor-input-rebinding-axis-capture-gamepad-v1.md`
- `2026-05-11-editor-game-module-driver-active-session-hot-reload-fail-closed-order-v1.md`
- `2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-spec-v1.md`
- `2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md`
- `2026-05-12-asset-identity-v2-placement-resolution-v1.md`
- `2026-05-12-editor-content-browser-asset-identity-v2-rows-v1.md`
- `2026-05-12-runtime-resource-v2-resident-catalog-cache-v1.md`
- `2026-05-12-runtime-resource-v2-resident-package-mount-set-v1.md`
- `2026-05-16-frame-graph-d3d12-texture-aliasing-barrier-evidence-v1.md`
- `2026-05-16-frame-graph-texture-aliasing-barrier-command-v1.md`
- `2026-05-17-frame-graph-backend-neutral-distinct-alias-group-lease-binding-v1.md`
- `2026-05-18-frame-graph-v1-1-0-scope-closeout-v1.md`
- `2026-05-19-gameplay-physics-navigation-ai-foundation-v1.md`
- `2026-05-21-engine-entity-scale-and-culling-v1.md`
- `2026-05-21-engine-scripting-sandbox-v1.md`
- `2026-05-21-engine-world-region-streaming-v1.md`
- `2026-05-21-generated-3d-entity-scale-culling-package-evidence-v1.md`
- `2026-05-21-renderer-backend-parity-v1.md`
- `2026-05-21-renderer-debug-profiling-v1.md`
- `2026-05-21-renderer-gpu-memory-v1.md`
- `2026-05-22-ai-gameplay-authoring-tools-v1.md`
- `2026-05-22-engine-networking-foundation-v1.md`
- `2026-05-22-gameplay-simulation-orchestration-v1.md`
- `2026-05-11-directory-layout-tools-split-v1.md`
- `2026-05-11-editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1.md`
- `2026-05-11-editor-resource-capture-pix-host-handoff-evidence-v1.md`
- `2026-05-11-editor-resource-capture-pix-launch-helper-v1.md`
- `2026-05-11-editor-resources-capture-operator-validated-launch-workflow-v1.md`
- `2026-05-11-production-documentation-stack-v1.md`
- `2026-05-22-post-1-0-capability-program-v1.md`
