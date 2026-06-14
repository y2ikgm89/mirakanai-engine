# GameEngine Rendering Full Guidance

This file is detailed reference material for the `.claude/skills/gameengine-rendering/SKILL.md` skill. Load it only when the short
`SKILL.md` router says the current task needs detailed API names, detailed validation lanes, retained ids, or exact package/render/editor
counters.

# GameEngine Rendering

## Boundary Rules

- Keep API-independent renderer interfaces separate from backend implementation.
- Prefer Vulkan for Windows/Linux/Android backend work.
- Prefer Metal for Apple backend work.
- Do not expose graphics API handles through core game APIs unless an explicit interop design accepts it.

## Required Checks

1. Confirm which layer is changing: renderer interface, backend, shader pipeline, or sample.
2. Add the smallest tests or visual/golden checks that prove the layer's externally visible contract; avoid implementation-mirroring or
   duplicate coverage.
3. Document backend capability assumptions.
4. Use focused renderer/RHI/shader checks while iterating, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at
   the coherent slice-closing gate.

## Shader Tooling Checks

- Keep shader tools as executable-plus-argument vectors, never shell strings.
- Use `mirakana::discover_shader_tools` for deterministic tool descriptors.
- Use `mirakana::evaluate_shader_toolchain_readiness` before promoting shader-dependent backend work; Vulkan SPIR-V readiness requires DXC,
  proven DXC SPIR-V CodeGen support, and `spirv-val`.
- For local Vulkan shader lanes, prefer the official Vulkan SDK or an explicit `MK_SHADER_TOOLCHAIN_ROOT` over ambient PATH so Windows SDK
  DXIL-only `dxc.exe` does not shadow the Vulkan SDK DXC; verify with `pwsh -NoProfile -ExecutionPolicy Bypass -File
  tools/check-shader-toolchain.ps1`.
- Use `mirakana::make_spirv_shader_validation_command`, `mirakana::ShaderArtifactValidationProcessRunner`, and
  `mirakana::execute_shader_artifact_validation_action` for `spirv-val` backed artifact validation; do not shell out manually.
- `mirakana::ShaderToolExecutionPolicy::executable_override` may be a reviewed PATH tool name, project-relative tool path, or absolute
  installed SDK/tool path; shell executables must remain rejected by process validation.
- Use shader artifact provenance and input fingerprints before treating compiled shader outputs as cacheable.
- Use `mirakana::execute_shader_compile_action` and `mirakana::ShaderArtifactCacheIndex` for reviewed shader compile/cache workflows instead
  of ad hoc cache files.
- Use `mirakana::build_shader_pipeline_cache_plan` and `mirakana::reconcile_shader_pipeline_cache_index` for selected reviewed
  compile-request batches when repairing first-party cache-index metadata from current provenance; this planner/reconciler must not invoke
  compilers, write shader artifacts, expose native PSO/Vulkan/Metal cache blobs, or claim renderer/RHI residency.
- Generate Vulkan SPIR-V through DXC executable-plus-argument vectors with `-spirv` and `-fspv-target-env=vulkan1.3`; do not use shell
  strings or omit the target environment.
- For physical-sky Vulkan runtime proof, regenerate SPIR-V with `tools/compile-vulkan-physical-sky-test-spirv.ps1`, set
  `MK_VULKAN_TEST_PHYSICAL_SKY_VERTEX_SPV` and `MK_VULKAN_TEST_PHYSICAL_SKY_FRAGMENT_SPV`, and keep this as runtime readback evidence
  rather than Vulkan package readiness.
- Disable shader artifact marker writes for real compiler-backed runs so missing bytecode artifacts fail instead of being faked.

## Backend-Specific Checks

- For Debug Profiling Policy v1 work, keep `DebugProfilingPolicyDesc`, `DebugProfilingRequestDesc`, `DebugProfilingCaptureKind`,
  `DebugProfilingPolicyPlan`, and `plan_debug_profiling_policy` in the backend-neutral `MK_renderer` public contract. The planner may
  classify GPU timestamp/marker rows, CPU profile zone rows, PIX/Vulkan/Metal trace/capture handoff gates, package counter evidence, frame
  diagnostics, and fail-closed diagnostics for unsupported automatic capture, production flame graphs, or crash telemetry export. Frame
  renderers may insert `mirakana.presentation` GPU debug markers; `IRhiDevice::gpu_timestamp_ticks_per_second` and `RhiStats` `gpu_debug_*`
  counters supply evidence. `sample_desktop_runtime_game --require-debug-profiling-policy --require-d3d12-debug-profiling-evidence` may
  expose selected D3D12 counters through `evaluate_win32_desktop_presentation_debug_profiling_policy` and
  `evaluate_win32_desktop_presentation_d3d12_debug_profiling_execution`, including `debug_profiling_policy_status=ready`,
  `debug_profiling_policy_ready=1`, `debug_profiling_policy_diagnostics=0`, positive
  `debug_profiling_policy_gpu_timestamp_ticks_per_second`, positive `debug_profiling_policy_gpu_debug_markers_inserted`, positive
  `debug_profiling_policy_cpu_profile_zones`, positive `debug_profiling_policy_trace_capture_handoff_rows`, positive
  `debug_profiling_policy_cpu_profile_zone_requests`, positive `debug_profiling_policy_trace_capture_handoff_requests`, positive
  `debug_profiling_policy_package_counter_requests`, `debug_profiling_policy_cpu_profile_zone_evidence_ready=1`,
  `debug_profiling_policy_trace_capture_handoff_evidence_ready=1`, `debug_profiling_policy_package_counter_evidence_ready=1`,
  `debug_profiling_policy_backend_profiling_evidence_required=1`, `debug_profiling_policy_backend_profiling_evidence_ready=1`,
  `d3d12_debug_profiling_execution_status=ready`, `d3d12_debug_profiling_execution_ready=1`, `d3d12_debug_profiling_execution_selected=1`,
  `d3d12_debug_profiling_execution_gpu_timestamps_ok=1`, `d3d12_debug_profiling_execution_gpu_debug_markers_ok=1`, and
  `d3d12_debug_profiling_execution_frame_diagnostics_ok=1`, without automatic external capture execution. Backend Renderer Parity v1 may add
  `debug_profiling_policy_backend_evidence_ready`, `BackendRendererParityPolicyRequest`, `BackendRendererParityProofRow`,
  `BackendRendererParityPolicyPlan`, `plan_backend_renderer_parity_policy`, `backend_renderer_parity_proof_matches_selected_backend`, and
  `evaluate_win32_desktop_presentation_vulkan_debug_profiling_execution`; the separate Vulkan installed package smoke may require
  `--require-vulkan-debug-profiling-evidence` and report `vulkan_debug_profiling_execution_status=ready`,
  `vulkan_debug_profiling_execution_ready=1`, `vulkan_debug_profiling_execution_selected=1`,
  `vulkan_debug_profiling_execution_gpu_debug_markers_ok=1`, and `vulkan_debug_profiling_execution_frame_diagnostics_ok=1` without treating
  D3D12 proof as cross-backend evidence. Use `plan_backend_renderer_parity_policy` for value-only synchronization, shader-validation,
  memory-residency, profiling, and package-evidence proof rows; keep Metal Apple-host-gated and reject public native-handle or
  inferred-parity claims.
- For Backend Renderer Parity host recipe work, every Metal `BackendRendererParityProofRow` must set `host_validation_recipe_id` to a
  reviewed Apple validation lane: `shader-toolchain`, `mobile-packaging`, `renderer-metal-apple-host-evidence`, or `ios-simulator-smoke`.
  `plan_backend_renderer_parity_policy` must fail closed with `missing_host_validation_recipe` when Metal evidence omits the recipe id and
  `unreviewed_host_validation_recipe` when an arbitrary valid id is supplied, include the recipe id in replay hashes, keep `metal-apple`
  host-gated until Apple-host proof exists, and keep D3D12/Vulkan evidence backend-local.
- For Environment Rendering Readiness v1 Task 8 / Metal environment feature work, use
  `MetalEnvironmentFeatureHostEvidenceDesc`, `MetalEnvironmentFeatureEvidenceRequirement`, `MetalEnvironmentFeatureEvidenceRow`,
  `MetalEnvironmentFeatureHostEvidencePlan`, `default_environment_feature_evidence_requirements`, and
  `build_environment_feature_host_evidence_plan` in `MK_rhi_metal` for physical sky, height fog, cloud layer, precipitation,
  volumetric fog, volumetric cloud, and environment lighting/IBL host-gated rows; each default row requires synchronization evidence and
  can be summarized with `metal_environment_feature_host_evidence_status_line`. The reviewed `renderer-metal-apple-host-evidence` recipe
  must build generated `environment_feature_evidence.metallib`, run Apple-only private `create_native_environment_feature_host_evidence`,
  and emit selected `metal_environment_*` counters for runtime, command queue, metallib, render/compute pipeline, render pass, cube HDR,
  depth texture, particle buffer, synchronization/readback, seven ready rows, and zero native-handle access. D3D12/Vulkan proof and native
  handles must not promote Metal environment readiness, and this recipe does not claim broad backend parity or broad `environment_ready`.
  Commercial environment aggregate claims must use `renderer-metal-environment-aggregate-apple-host-evidence`, which delegates to
  `renderer-metal-apple-host-evidence` and may emit `environment_metal_host_aggregate_ready=1` only after Apple-host renderer Metal evidence
  succeeds; it must keep backend parity, all-platform readiness, broad optimization, commercial readiness, and broad `environment_ready`
  unclaimed.
- For Environment Commercial Excellence v1 backend parity work, use `EnvironmentBackendParityRequest`, `EnvironmentBackendParityRow`,
  `EnvironmentBackendParityCounterExpectation`, `EnvironmentBackendParityPlan`, and `plan_environment_backend_parity` in `MK_renderer`.
  D3D12 primary, strict Vulkan aggregate, and Apple-host Metal rows must share the same profile revision, preset pack revision, package
  revision, normalized feature ids, quality tier/budget class, resource class, output tolerance class, package counter ids/semantics,
  unsupported-row list, and zero diagnostics before parity can be ready. Host-gated rows, fallback/native-handle evidence,
  cross-backend inference, feature-count-only comparisons, stale revisions, and nonzero diagnostics must not emit package-visible
  `environment_backend_parity_ready=1`. The package-visible `desktop-runtime-sample-game-environment-backend-parity` recipe is a
  host-gated review lane: it emits `environment_backend_parity_status=host_evidence_required`, `environment_backend_parity_ready=0`,
  14 D3D12/strict-Vulkan ready rows, seven Apple-host Metal host-gated rows, two host-validated backends, zero diagnostics,
  zero native-handle/GPU-command side effects, and a positive replay hash without promoting all-platform, commercial, broad optimization,
  or broad `environment_ready`.
- For GPU Memory Policy v1 work, keep `GpuMemoryPolicyDesc`, `GpuMemoryRequestDesc`, `GpuMemoryResidencyClass`, `GpuMemoryPolicyPlan`,
  `plan_gpu_memory_policy`, and `gpu_memory_policy_backend_evidence_ready` in the backend-neutral `MK_renderer` public contract. The planner
  may classify committed/placed/transient budget rows, declared memory budget evidence, residency pressure evidence, transient heap policy,
  upload-pressure rows, scene resource availability, package counter evidence, per-backend memory-evidence gates without cross-backend proof
  transfer, OS video-memory budget gates on D3D12, and fail-closed diagnostics for unsupported automatic eviction or background streaming
  before backend execution. `IRhiDevice::memory_diagnostics()` may expose `RhiDeviceMemoryDiagnostics` with DXGI `QueryVideoMemoryInfo` rows
  on D3D12 and committed-byte estimates on Vulkan; `RhiStats` may expose transient heap/placed counters and `upload_bytes_written`.
  `sample_desktop_runtime_game --require-gpu-memory-policy --require-memory-diagnostics --require-d3d12-gpu-memory-evidence` may expose
  selected D3D12 package counters through `evaluate_win32_desktop_presentation_gpu_memory_policy`, `summarize_memory_diagnostics`, and
  `evaluate_win32_desktop_presentation_d3d12_gpu_memory_execution`, and `installed-vulkan-scene-gpu-smoke` may require
  `--require-vulkan-gpu-memory-evidence` through `evaluate_win32_desktop_presentation_vulkan_gpu_memory_execution`, including
  `gpu_memory_policy_status=ready`, `gpu_memory_policy_ready=1`, `gpu_memory_policy_diagnostics=0`, positive `gpu_memory_policy_requests`,
  positive `gpu_memory_policy_committed_byte_estimate`, positive `gpu_memory_policy_upload_bytes_written`, positive
  `gpu_memory_policy_declared_budget_requests`, positive `gpu_memory_policy_residency_pressure_requests`, positive
  `gpu_memory_policy_package_counter_requests`, positive `gpu_memory_policy_residency_pressure_events`,
  `gpu_memory_policy_memory_budget_evidence_ready=1`, `gpu_memory_policy_residency_pressure_evidence_ready=1`,
  `gpu_memory_policy_package_counter_evidence_ready=1`, `gpu_memory_policy_backend_memory_evidence_required=1`,
  `gpu_memory_policy_backend_memory_evidence_ready=1`, `memory_diagnostics_status=ready`, `memory_diagnostics_ready=1`,
  `memory_diagnostics_diagnostics=0`, positive `memory_diagnostics_transient_gpu_aliasing_barriers`,
  `memory_diagnostics_transient_gpu_framegraph_aliasing_ready=1`, `d3d12_gpu_memory_execution_status=ready`,
  `d3d12_gpu_memory_execution_ready=1`, `d3d12_gpu_memory_execution_selected=1`, `d3d12_gpu_memory_execution_budget_ok=1`,
  `d3d12_gpu_memory_execution_transient_heap_ok=1`, `vulkan_gpu_memory_execution_status=ready`, `vulkan_gpu_memory_execution_ready=1`,
  `vulkan_gpu_memory_execution_selected=1`, `vulkan_gpu_memory_execution_budget_ok=1`, and
  `vulkan_gpu_memory_execution_transient_heap_ok=1`, without exposing native handles or claiming Metal memory parity.
- For Scene Scale Policy v1 work, keep `SceneScalePolicyDesc`, `SceneScaleDrawGroupDesc`, `SceneScaleBatchingMode`, `SceneScalePolicyPlan`,
  `SceneScaleDrawGroupRow`, `plan_scene_scale_policy`, and `scene_scale_policy_backend_instancing_evidence_ready` in the backend-neutral
  `MK_renderer` public contract. The planner may classify sprite/static/skinned/morph draw groups, instanced draw batching intent,
  CPU-frustum culling counts, LOD rows, draw-call and visible-instance budgets, scene resource availability, backend instancing-evidence
  gates, and missing performance measurement diagnostics before backend execution. `MeshCommand::instance_count` and
  `SceneRenderSubmitDesc::mesh_instance_count` may route backend-neutral instance counts into renderer-owned RHI draws; `RhiStats` may
  expose `instanced_draw_calls`, `instanced_indexed_draw_calls`, and `instanced_instances_submitted` counters. `sample_desktop_runtime_game
  --require-scene-scale-policy --require-d3d12-instanced-draw-evidence` may expose selected D3D12 package counters through
  `Win32DesktopPresentationD3d12InstancedDrawExecutionReport`, including `scene_scale_policy_status=ready`, `scene_scale_policy_ready=1`,
  `scene_scale_policy_diagnostics=0`, positive `scene_scale_policy_draw_groups`, positive `scene_scale_policy_requested_instances`, positive
  `scene_scale_policy_visible_instances`, positive `scene_scale_policy_draw_calls`, `d3d12_instanced_draw_execution_status=ready`,
  `d3d12_instanced_draw_execution_ready=1`, `d3d12_instanced_draw_execution_selected=1`, positive `d3d12_instanced_draw_calls`, positive
  `d3d12_instanced_indexed_draw_calls`, positive `d3d12_instanced_instances_submitted`, positive `rhi_instanced_draw_calls`, positive
  `rhi_instanced_indexed_draw_calls`, positive `rhi_instanced_instances_submitted`, `d3d12_instanced_draws_ok=1`,
  `d3d12_instanced_instances_ok=1`, `scene_scale_policy_backend_instancing_evidence_required=1`, and
  `scene_scale_policy_backend_instancing_evidence_ready=1`, and `installed-vulkan-scene-gpu-smoke` may require
  `--require-vulkan-instanced-draw-evidence` through `evaluate_win32_desktop_presentation_vulkan_instanced_draw_execution`, including
  `vulkan_instanced_draw_execution_status=ready`, `vulkan_instanced_draw_execution_ready=1`, `vulkan_instanced_draw_execution_selected=1`,
  positive `vulkan_instanced_draw_calls`, positive `vulkan_instanced_indexed_draw_calls`, positive `vulkan_instanced_instances_submitted`,
  `vulkan_instanced_draws_ok=1`, and `vulkan_instanced_instances_ok=1`. It must not expose `IRhiDevice`, native handles, package mutation,
  shader generation, GPU-driven indirect readiness, Metal instanced draw parity, or measured performance claims without separate
  backend/package evidence.
- For package-visible frame graph executor evidence, keep stdout and quality-gate fields backend-neutral: `framegraph_passes_executed`,
  `framegraph_render_passes_recorded`, `framegraph_barrier_steps_executed`, `renderer_quality_expected_framegraph_render_passes`,
  `renderer_quality_framegraph_render_passes_ok`, `renderer_quality_expected_framegraph_barrier_steps`, and
  `renderer_quality_framegraph_barrier_steps_ok` are allowed first-party counters. Do not expose D3D12/Vulkan barrier structs, image
  layouts, queue ownership, native handles, or frame-graph internals to gameplay.
- For Renderer General Quality Matrix v1 work, keep `RendererQualityMatrixRow`, `RendererQualityMatrixRowStatus`,
  `RendererQualityMatrixPlan`, `RendererQualityMatrixDiagnosticCode`, and `plan_renderer_quality_matrix` as backend-neutral `MK_renderer`
  value contracts. Generated 3D package smokes may require `--require-renderer-quality-matrix` and expose
  `renderer_quality_matrix_status=host_evidence_required`, `renderer_quality_matrix_reviewed=1`, `renderer_quality_matrix_ready=0`, 21 rows,
  14 ready rows, 7 Metal host-gated rows, `renderer_quality_matrix_dependency_gated_rows=0`, `renderer_quality_matrix_unsupported_rows=0`,
  D3D12/Vulkan readiness, `renderer_quality_matrix_general_renderer_quality_ready=0`, zero GPU command/native capture/crash upload flags,
  and clean diagnostics; do not infer Metal, execute captures, expose native handles, or claim broad production renderer quality.
- For Frame Graph production ownership boundary work, keep `FrameGraphProductionOwnershipCapability`,
  `FrameGraphProductionOwnershipBoundary`, `FrameGraphProductionOwnershipCandidate`, `FrameGraphProductionOwnershipPlan`, and
  `plan_frame_graph_production_ownership_boundary` in the backend-neutral `frame_graph` contract. The planner may classify reviewed executor
  capabilities, including Vulkan transient alias memory evidence, as `frame_graph_owned`, keep swapchain/present/readback/overlay work
  renderer-owned, keep package residency runtime-host-owned, keep Metal memory aliasing host-gated, and fail closed for production
  multi-queue adoption, broad/background package streaming, data preservation, async overlap/performance, and public native handles. Do not
  treat the planner as production graph migration evidence by itself.
- Frame Graph v1 is closed for the Engine 1.0 Windows-default ready surface after selected renderer/runtime upload execution,
  package-visible render-pass and multi-queue evidence, D3D12/Vulkan transient alias allocation evidence, and fail-closed production
  ownership boundary planning. Keep broad production render graph scheduling, broad/background package streaming, Metal memory alias
  allocation, async overlap/performance, actual content preservation, public native handles, and broad renderer quality as future or
  host-gated work; the next active foundation gap is `upload-staging-v1`.
- For package streaming texture/static/skinned/morph handoff work, keep
  `mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings` as a narrow bridge from a committed
  `RuntimePackageStreamingExecutionResult`, live `RuntimeResourceCatalogV2` texture rows, and successful owner-proven caller-owned
  `RuntimeTextureUploadResult` rows into imported `FrameGraphTextureBinding` values.
  `mirakana::runtime_rhi::upload_runtime_package_streaming_frame_graph_texture_bindings` may perform the host-owned committed-streaming +
  live-catalog + explicit `RuntimeTexturePayload` upload/binding transaction through `upload_runtime_texture`, returning upload rows,
  imported bindings, uploaded-byte totals, and Frame Graph command-list plus transition counters; when
  RuntimeTextureUploadOptions::upload_ring is set, it may reuse a caller-owned RhiUploadRing path while keeping ownership host-side.
  `mirakana::runtime_rhi::upload_runtime_package_streaming_mesh_gpu_bindings`,
  `mirakana::runtime_rhi::upload_runtime_package_streaming_skinned_mesh_gpu_bindings`, and
  `mirakana::runtime_rhi::upload_runtime_package_streaming_morph_mesh_gpu_bindings` may perform matching committed-streaming + live-catalog
  + explicit payload transactions for `RuntimeMeshPayload`, `RuntimeSkinnedMeshPayload`, and `RuntimeMorphMeshCpuPayload` rows, returning
  upload rows, renderer `MeshGpuBinding` / `SkinnedMeshGpuBinding` / `MorphMeshGpuBinding` rows, submitted fences, uploaded-byte totals,
  Frame Graph command-list counters, and `upload_queue_waits_recorded` when async copy-queue uploads are consumed by graphics bindings.
  `mirakana::runtime_rhi::wait_for_runtime_uploads_on_queue` may record those backend-neutral GPU-side waits through
  `IRhiDevice::wait_for_queue`; it must not perform CPU waits or expose native queues/fences. Package mesh/skinned/morph transactions may
  reuse caller-owned `RhiUploadRing` paths through `RuntimeMeshUploadOptions::upload_ring`, `RuntimeSkinnedMeshUploadOptions::upload_ring`,
  and `RuntimeMorphMeshUploadOptions::upload_ring` while keeping ownership host-side; those rings may use explicit `RhiStagingBufferLease`
  chunks through `RhiUploadRingDesc::buffer`. Reject non-committed results, stale or wrong-kind catalog rows, duplicate frame-graph resource
  names or mesh assets, failed uploads, null texture handles, ownerless uploads, and missing/mismatched payloads. Do not expose gameplay
  `IRhiDevice`, native handles, renderer-owned residency, runtime-wide upload ring ownership beyond reviewed texture/buffer uploads,
  background streaming, allocator/GPU budgets, runtime-wide staging-pool ownership beyond explicit lease-backed upload rings, production
  multi-queue graph adoption, async overlap/performance, or broad package streaming readiness through this bridge. Runtime mesh, skinned
  mesh, morph mesh, and texture upload options may carry caller-owned `RhiUploadRing` pointers; runtime-scene teardown must not release
  those caller-owned ring buffers.
- For low-level RHI upload staging work, prefer `mirakana::rhi::execute_upload_gpu_batch_async` when the caller already owns a
  `RhiUploadStagingPlan`, `RhiUploadRing`, and explicit buffer/texture targets and needs one no-wait backend-neutral queue submit. Use
  `RhiStagingBufferPool::try_acquire_lease`, `RhiStagingBufferLease`, and `RhiUploadRingDesc::buffer` only for explicit host-owned pool
  lease backing of a caller-owned upload ring. Runtime/package graphics-queue consumption should use
  `mirakana::runtime_rhi::wait_for_runtime_uploads_on_queue` at the runtime/host boundary;
  `mirakana::runtime_rhi::make_runtime_package_resource_update_readiness` should be used when committed package upload transactions need
  fail-closed async-ready resource update rows without mutating `MK_runtime` catalogs. Selected D3D12 generated 3D package proof uses
  `mirakana::runtime_rhi::execute_runtime_package_upload_staging_evidence` plus `--require-package-upload-staging` to verify package
  texture/static/skinned/morph uploads over pool-backed rings and resource update readiness counters. Keep runtime-wide staging-pool
  ownership, async overlap/performance, and native handles out of that helper.
- For Direct3D 12 work, keep COM objects and HWND/DXGI handles behind `engine/rhi/d3d12` PIMPL or first-party opaque handles.
- For D3D12 debug-layer work, require the official Windows Graphics Tools capability and verify `d3d12SDKLayers.dll`; use PIX on Windows /
  `pixtool` for D3D12 GPU capture or counter investigations, and Windows Performance Toolkit only for ETW/CPU/system traces.
- For D3D12 shared texture interop, create textures with `TextureUsage::shared`, export them only through first-party
  `mirakana::rhi::d3d12::D3d12SharedTextureHandle`, close exported handles with `mirakana::rhi::d3d12::close_shared_texture_handle`, and
  keep raw `HANDLE`/`ID3D12Resource` use in backend-private or editor-private bridge code.
- After public RHI/backend header changes, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` so
  native D3D12/DXGI/Win32 symbols stay out of public APIs.
- For public sampler work, use `mirakana::rhi::SamplerDesc`, `SamplerHandle`, `DescriptorType::sampler`, and `DescriptorResource::sampler`;
  keep native sampler/image-view objects backend-private. New backend/material sampling claims must include native descriptor writes plus
  visible shader-read proof for the touched backend.
- For RHI Depth Attachment Contract v0 work, use `mirakana::rhi::RenderPassDepthAttachment`, `ClearDepthValue`, and `DepthStencilStateDesc`
  only as backend-neutral first-party contracts. `NullRhiDevice` can validate ownership, usage, format, clear range, extent, pipeline
  compatibility, sampled-depth descriptor binding, and depth texture state transitions; `RhiFrameRenderer` may forward and frame-between
  replace a host-owned depth texture without exposing ownership through `IRenderer`. D3D12 has native private-DSV/depth-state/readback proof
  plus sampled depth through private typeless DSV/SRV views, sampled-depth readback proof, depth-aware postprocess readback proof, and
  directional shadow receiver readback proof. Vulkan has native private depth image-view/layout/barrier/dynamic-rendering/depth-state proof
  plus required synchronization2 feature query/device enablement, D24S8 depth|stencil barrier aspect masks, depth+sampled create planning,
  sampled-depth descriptor update coverage, depth_write-to-shader_read barrier planning, environment-gated visible sampled-depth readback
  proof when `MK_VULKAN_TEST_DEPTH_VERTEX_SPV`, `MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV`, `MK_VULKAN_TEST_DEPTH_SAMPLE_VERTEX_SPV`, and
  `MK_VULKAN_TEST_DEPTH_SAMPLE_FRAGMENT_SPV` SPIR-V artifacts are configured, environment-gated postprocess-depth readback proof when those
  depth-write artifacts plus `MK_VULKAN_TEST_POSTPROCESS_DEPTH_VERTEX_SPV` and `MK_VULKAN_TEST_POSTPROCESS_DEPTH_FRAGMENT_SPV` are
  configured, environment-gated physical-sky runtime readback proof when `MK_VULKAN_TEST_PHYSICAL_SKY_VERTEX_SPV` and
  `MK_VULKAN_TEST_PHYSICAL_SKY_FRAGMENT_SPV` are configured with `VK_LAYER_KHRONOS_validation`, and environment-gated directional shadow receiver readback proof when those depth-write artifacts plus
  `MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV` and `MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV` are configured. Both must keep native
  descriptors, resource states, COM/Vk objects, layouts, and backend details private. Metal native depth recording/sampling, Metal
  postprocess depth, broader postprocess effects, and production shadow quality remain separate backend slices until proven.
- For Frame Graph transient texture alias planning, lease binding, shared-handle state handoff, and texture aliasing barrier command
  recording, keep `FrameGraphTransientTextureDesc`, `FrameGraphTransientTextureLifetime`, `FrameGraphTransientTextureAliasPlan`,
  `plan_frame_graph_transient_texture_aliases`, `FrameGraphTransientTextureLeaseBindingResult`,
  `acquire_frame_graph_transient_texture_lease_bindings`, `release_frame_graph_transient_texture_lease_bindings`,
  `FrameGraphTextureAliasingBarrier`, and `record_frame_graph_texture_aliasing_barriers` in `frame_graph_rhi` because they depend on
  `mirakana::rhi::TextureDesc`, `IRhiDevice` transient leases, and public `IRhiCommandList::texture_aliasing_barrier`. The planner may
  validate used transient texture descriptors, compute first/last pass lifetimes, group only non-overlapping exact-desc matches, and report
  byte estimates; the lease binder must acquire one backend-neutral `IRhiDevice::acquire_transient_texture_alias_group` lease per alias
  group, retain the returned `rhi::TransientTextureAliasGroup`, emit resource-name `FrameGraphTextureBinding` rows with distinct texture
  handles in group resource order, accept empty plans without allocation, reject malformed alias groups before acquisition, reject
  zero/duplicate/wrong-count backend returns, and release already acquired leases on later acquisition failure. The executor is
  shared-handle state-handoff aware for existing backend-neutral `TextureHandle` rows: barrier recording, pass-target preparation, and
  final-state restoration must update every `FrameGraphTextureBinding` row sharing a handle, and conflicting initial shared-handle states
  must be rejected before recording. When callers provide transient texture lifetimes, `execute_frame_graph_rhi_texture_schedule` and
  `execute_frame_graph_rhi_multi_queue_schedule` must prevalidate them, then record one automatic aliasing barrier before the first pass
  that uses each later resource in a non-overlapping alias group and report `aliasing_barriers_recorded`
  (`FrameGraphRhiMultiQueueExecutionResult::aliasing_barriers_recorded` for multi-queue); malformed lifetime ranges, missing bindings,
  duplicate lifetime rows, overlapping alias lifetimes, same-handle automatic aliases, and resource-backed transient first render-pass
  `LoadAction::load` rows must fail before command recording or pass callbacks. The aliasing barrier helper may record explicit dependencies
  for two distinct bound texture handles or wildcard/null endpoints without changing tracked states; empty
  `FrameGraphTextureAliasingBarrier` resource names map to public `TextureHandle{}` wildcard endpoints. D3D12 records a conservative
  null-resource `D3D12_RESOURCE_BARRIER_TYPE_ALIASING` after public non-zero handle validation when the handles are not a proven
  placed-resource alias pair, maps explicit public wildcard/null endpoints to `nullptr` native aliasing endpoints, and may record
  backend-private non-null aliasing barriers only for proven same alias-group placed pairs; Vulkan records a conservative synchronization2
  memory dependency, and transient alias-group leases may use backend-private `VK_IMAGE_CREATE_ALIAS_BIT` images bound to one shared
  `VkDeviceMemory` owner with first-party `transient_texture_heap_allocations` / `transient_texture_placed_allocations` /
  `transient_texture_placed_resources_alive` evidence. D3D12 transient texture leases may use backend-private one-heap-per-lease
  `CreateHeap` plus `CreatePlacedResource` allocation evidence through `transient_texture_heap_allocations` /
  `transient_texture_placed_allocations` / `transient_texture_placed_resources_alive`, and D3D12 alias-group leases may prove multiple
  same-offset placed textures on one heap with non-null barrier/state counters, and Vulkan alias-group leases may prove one shared
  backend-private memory allocation behind distinct first-party handles, but this remains foundation-only. First-use placed-resource
  activation must stay pending per command list until submission, reset-before-submit must re-record activation, alias before/after state
  must update after `ExecuteCommandLists` submits work rather than after fence completion, and transient release must retire placed heap
  ownership after the lifetime fence. It must not expose native handles, claim data inheritance/content preservation, or claim production
  render graph ownership from wildcard/null barrier support.
- For Frame Graph RHI queue dependency and multi-queue pass-command work, keep `FrameGraphRhiPassQueueBinding`, `FrameGraphRhiQueueWait`,
  `plan_frame_graph_rhi_queue_waits`, `FrameGraphRhiSubmittedPassFence`, `record_frame_graph_rhi_queue_waits`,
  `FrameGraphRhiPassCommandBinding`, `FrameGraphRhiMultiQueueExecutionDesc`, `FrameGraphRhiMultiQueueExecutionDesc::texture_bindings`,
  `FrameGraphRhiMultiQueueExecutionDesc::final_states`, `FrameGraphRhiMultiQueueExecutionDesc::transient_texture_lifetimes`,
  `FrameGraphRhiMultiQueueExecutionResult::barriers_recorded`, `FrameGraphRhiMultiQueueExecutionResult::final_state_barriers_recorded`,
  `FrameGraphRhiMultiQueueExecutionResult::aliasing_barriers_recorded`, and `execute_frame_graph_rhi_multi_queue_schedule` in the
  backend-neutral `frame_graph_rhi` contract. Derive wait rows from scheduled barrier edges, explicit pass queue bindings, and alias-induced
  dependencies from the previous transient alias lifetime last-use pass to the later alias lifetime first-use pass, default unbound passes
  to graphics, dedupe by consumer/producer pass pair, and validate duplicate/unscheduled bindings before recording. The multi-queue executor
  may begin one command list per scheduled pass on its declared queue, pass it to the callback, require callbacks to leave command lists
  open, close/submit it, retain the submitted pass fence, and record waits before consumer passes only after the producer fence exists. When
  callers opt in with transient texture lifetimes, automatic aliasing barriers must prevalidate before command recording, reject transient
  first render-pass `LoadAction::load`, add cross-queue waits when the alias predecessor and successor run on different queues, and record
  after producer waits and before scheduled texture barriers, target states, render pass envelopes, or callbacks. When callers opt in with
  texture bindings, scheduled texture barriers must prevalidate bindings before command recording and record on the consumer pass command
  list after producer waits and before the callback. When callers opt in with final states, final texture transitions must record on the
  last scheduled resource pass command list and report `final_state_barriers_recorded`.
  `execute_frame_graph_rhi_multi_queue_package_evidence` must keep the selected `--require-framegraph-multiqueue-evidence` D3D12
  package-visible smoke and the separate `--require-vulkan-framegraph-multiqueue-evidence` Vulkan installed smoke on transient alias-group
  lease bindings with `framegraph_multiqueue_command_lists_submitted=4`, `framegraph_multiqueue_queue_waits_recorded=3`,
  `framegraph_multiqueue_barriers_recorded=4`, `framegraph_multiqueue_aliasing_barriers_recorded=1`,
  `framegraph_multiqueue_submitted_pass_fences=4`, `framegraph_multiqueue_graphics_waited_for_copy=1`,
  `vulkan_framegraph_multiqueue_evidence_ready=1`, and `vulkan_framegraph_multiqueue_evidence_selected=1` without treating D3D12 proof as
  cross-backend evidence. It must not expose native queues/fences/semaphores, claim async overlap/performance, mark Vulkan/Metal production
  multi-queue ready, or migrate package-visible renderers without separate evidence.
- For Frame Graph RHI pass target-state work, keep `FrameGraphTexturePassTargetAccess`, `build_frame_graph_texture_pass_target_accesses`,
  and `FrameGraphTexturePassTargetState` in the backend-neutral `frame_graph_rhi` contract. Every pass target-state row must be backed by
  the same pass/resource writer access from `FrameGraphV1Desc::passes[*].writes` and must match
  `frame_graph_texture_state_for_access(access)` before command recording; do not infer target state from callback names, native render-pass
  attachments, D3D12/Vulkan layouts, or backend handles.
- For `RhiFrameRenderer` primary pass ownership work, keep `begin_frame()` to swapchain acquire plus graphics command-list begin, queue raw
  sprite/mesh work during draw calls after the same ownership validation as before, and execute a scheduled `primary_color` callback through
  `execute_frame_graph_rhi_texture_schedule` from `end_frame()`. Texture-backed color targets must declare a `primary_color`
  `FrameGraphTextureBinding` with the renderer's known state, a `color_attachment_write` row in `FrameGraphV1Desc`, and a
  `FrameGraphTexturePassTargetState` to `render_target`; optional depth targets must declare `primary_depth`, `depth_attachment_write`, and
  `depth_write`. Swapchain-backed frames keep the color attachment on `swapchain_frame` without a color texture binding. The executor must
  begin/end the declared `FrameGraphRhiRenderPassDesc` envelope around the `primary_color` callback; the callback records queued
  primary/native-overlay work in order, while swapchain acquire/present, command-list close/submit/wait, and native resource ownership
  remain renderer-owned. Completed texture-target frames may report `framegraph_barrier_steps_executed` when imported target states differ
  from attachment states; completed raw frames still report one `framegraph_passes_executed` callback and one
  `framegraph_render_passes_recorded` executor-owned render pass envelope. Do not claim native transient allocation, native alias execution,
  multi-queue scheduling, package streaming, Metal parity, renderer-wide manual-transition removal, or production render graph ownership.
- For Postprocess Depth Input Readback Foundation v0 and Package-Visible Postprocess Depth Effect v0 work, keep
  `RhiPostprocessFrameRenderer` depth input renderer-owned and opt-in, validate `depth24_stencil8`, keep scene color bindings at `0/1` and
  scene depth bindings at `2/3`, queue scene mesh commands during `draw_mesh()`, let `execute_frame_graph_rhi_texture_schedule` prepare
  declared writer-access-backed `scene_color` and optional `scene_depth` target states, let the executor begin/end declared render pass
  envelopes around the scene and one/two-stage postprocess callbacks, keep callbacks to pass-body recording, route post-chain inter-pass
  transitions through the executor, prove depth returns from `shader_read` to `depth_write` for reuse, and treat the package smoke as
  limited to selected package readiness via `postprocess_depth_input_ready=1`. Positive frame-count package budgets are `frames * 2`
  barriers for no-depth postprocess and `1 + (frames * 4)` barriers for depth-aware postprocess. Do not claim SSAO, depth of field, fog,
  temporal history, broader postprocess material/effect stacks, Metal postprocess depth, broader production graph pass ownership beyond the
  completed raw primary and postprocess render pass envelopes, overlapping native alias execution, multi-queue scheduling, or production
  render graph ownership.
- For Renderer Postprocess Chain Policy v1 work, keep `PostprocessChainPolicyDesc` / `PostprocessChainPolicyPlan` value-only and
  backend-neutral. Use it to classify tone mapping, exposure, bloom, color grading, fog, FXAA, scene-color/depth requirements, postprocess
  pass counts, framegraph barrier budgets, and backend shader-evidence gates before backend shader execution.
  `Win32DesktopPresentationPostprocessPolicyReport` / `evaluate_win32_desktop_presentation_postprocess_policy` may expose only
  package-visible `postprocess_policy_*` readiness, diagnostic, effect, pass, resource, and shader-evidence counters for the selected
  existing package postprocess path. Do not use it to claim new shader execution beyond that selected color-grading package lane, temporal
  AA, vendor upscalers, Vulkan/Metal parity, native/RHI handle exposure, or broad production postprocess quality.
- For Renderer Postprocess Tone Mapping Evidence v1 work, keep `PostprocessToneMappingEvidenceRequest` /
  `PostprocessToneMappingEvidencePlan` and `plan_postprocess_tone_mapping_evidence` value-only and backend-neutral. Validate explicit HDR
  input, transfer function, luminance range, color-space evidence, resource-synchronization evidence, shader-validation evidence,
  backend-validation evidence, and host-validation evidence per backend. D3D12/Vulkan proof must not promote Metal; Metal stays host-gated
  without Apple-host evidence; reject native handles, SDL3, cross-backend proof transfer, subjective visual-quality claims, GPU command
  execution, native capture, and crash upload side effects.
- For Shadow Map Foundation v0 / Stable Directional Light-Space Policy v0 / Directional Shadow Receiver Readback Proof v0 / Package-Visible
  Directional Shadow Filtering v0 work, keep the implementation in
  `mirakana_renderer`/`mirakana_scene_renderer`/`mirakana_runtime_host_win32_presentation` as backend-neutral planning and narrow
  package-visible sample proof: `ShadowMapDesc`, `ShadowMapPlan`, `DirectionalShadowLightSpaceDesc`, `DirectionalShadowLightSpacePlan`,
  `ShadowReceiverDesc`, `ShadowReceiverPlan`, deterministic diagnostics, `Format::depth24_stencil8`, `TextureUsage::depth_stencil |
  TextureUsage::shader_resource`, sampled-depth plus sampler descriptor layout policy, fixed sampled-depth 3x3 PCF policy metadata, depth
  bias and lit/shadow intensity validation, caster/receiver counts, first directional shadow-casting scene light selection, stable
  shadow-depth-before-receiver pass declaration, deterministic texel-snapped orthographic light-space matrix planning, D3D12/Vulkan RHI
  readback proof that sampled shadow depth can darken a receiver pass, D3D12 fixed-PCF boundary readback proof,
  `RhiDirectionalShadowSmokeFrameRenderer` scheduled shadow-depth/scene-color/scene-depth inter-pass transitions, declared
  shadow-color/shadow-depth/scene-color/scene-depth writer-access-backed target-state preparation, executor-owned render pass envelopes for
  shadow-depth, scene-receiver, and postprocess callbacks, overlay preparation outside executor-owned render pass scopes, and reusable depth
  final-state restoration through `execute_frame_graph_rhi_texture_schedule`, and selected `sample_desktop_runtime_game` package smoke
  readiness through `--require-directional-shadow`, `--require-directional-shadow-filtering`, `--require-d3d12-shadow-cascade-policy,
  --require-vulkan-shadow-cascade-policy, vulkan_shadow_cascade_policy_ready=1, vulkan_shadow_cascade_policy_selected=1`,
  `directional_shadow_status=ready`, `directional_shadow_ready=1`, `directional_shadow_filter_mode=fixed_pcf_3x3`,
  `directional_shadow_filter_taps=9`, `directional_shadow_filter_radius_texels=1`, `directional_shadow_cascade_count=4`,
  `directional_shadow_cascade_tile_width=225`, `directional_shadow_atlas_width=900`, `directional_shadow_atlas_height=225`,
  `directional_shadow_light_space_cascades=4`, `directional_shadow_cascade_splits=5`, `framegraph_passes=3`, `framegraph_passes_executed=6`,
  `framegraph_render_passes_recorded=6`, and `framegraph_barrier_steps_executed=15`. Do not claim overlapping native alias execution,
  multi-queue scheduling, hardware comparison samplers, production shadow quality, shader-side cascade selection expansion, Metal shadow
  presentation, editor shadow authoring, or broad backend parity until separately implemented and validated.
- For Lighting Shadow Policy v1 package evidence, keep the public contract value-only through `LightingShadowPolicyDesc` /
  `LightingShadowPolicyPlan` and `plan_scene_lighting_shadow_policy`. `sample_desktop_runtime_game --require-lighting-shadow-policy` may
  report `lighting_shadow_policy_status=ready`, `lighting_shadow_policy_ready=1`, `lighting_shadow_policy_diagnostics=0`, one directional
  shadowed light, `lighting_shadow_policy_light_rows=1`, a 1024x1024 policy atlas, and `lighting_shadow_policy_ready_frames=<expected smoke
  frames>`; it must not imply additional rendered lights, native/RHI handles, Vulkan/Metal visual parity, or production shadow quality.
- For runtime material GPU bindings, preserve `owner_device` provenance for textures, mesh buffers, and material descriptor sets, reject
  cross-device use, reject unsupported descriptor sets before allocation, and keep the material factor payload contract at 64 bytes even
  when backend uniform allocation uses 256-byte backing. Material-factor uniform copies must route command recording and submission through
  `execute_frame_graph_rhi_multi_queue_schedule`; `RuntimeMaterialGpuBinding` reports command-list, queue-wait, barrier, and callback
  counters, without implying buffer barriers, ring-backed material upload staging beyond texture, native async upload execution, or public
  native handles.
- For runtime mesh layout changes, keep layout derivation in `mirakana_runtime_rhi` from first-party cooked `RuntimeMeshPayload` metadata.
  Position-only meshes use stride 12; Lit Material v0 meshes use interleaved position+normal+uv stride 32. Reject partial normal/UV layouts
  before creating buffers even when callers disable automatic derivation, and update `sample_desktop_runtime_game` D3D12/Vulkan package
  smokes when the lit sample shader or vertex input changes.
- For runtime scene GPU binding palettes, use `mirakana_runtime_scene_rhi` to bridge `RuntimeAssetPackage` plus `SceneRenderPacket` into
  retained `SceneGpuBindingPalette` resources. Keep `mirakana_runtime` and `mirakana_scene` RHI-free, keep `IRhiDevice` inside
  renderer/RHI/host adapter work, and remember the current bridge uses one scene-shared material pipeline layout; D3D12/Vulkan visible
  proofs must create the graphics pipeline from the exact `MaterialGpuBinding::pipeline_layout` returned by the palette so descriptor
  binding matches the currently bound pipeline.
- For Win32 desktop runtime packaged scene GPU paths, keep the palette host-owned inside `mirakana_runtime_host_win32_presentation`:
  validate the request vertex input against every referenced cooked mesh layout before reporting scene GPU readiness, build the palette with
  the private D3D12 or Vulkan device, wrap `RhiFrameRenderer` with an internal `IRenderer` injector that fills mesh/material GPU bindings
  from asset ids, create the scene graphics pipeline from the exact palette material pipeline layout, expose only first-party scene GPU
  status/counters/diagnostics, and validate target-specific DXIL/SPIR-V shader artifact metadata plus any Vulkan mapping compute proof
  artifact through the selected package lane. Do not hand `IRhiDevice`, native handles, or `SceneGpuBindingPalette` to gameplay.
- For Win32 desktop runtime Frame Graph/Postprocess v0 and Directional Shadow Filtering v0 paths, keep the offscreen scene-color texture,
  optional renderer-owned scene-depth texture, shadow color/depth textures, descriptor set layouts/sets, samplers, postprocess pipeline
  layout/pipeline, shadow pipeline, and frame sequencing host-owned inside `mirakana_runtime_host_win32_presentation` and
  `mirakana_renderer::RhiPostprocessFrameRenderer` / `mirakana_renderer::RhiDirectionalShadowSmokeFrameRenderer`. Scene color descriptor
  bindings are `0/1`; opt-in scene depth descriptor bindings are `2/3`; postprocess queues scene mesh commands, prepares declared
  scene-color and optional scene-depth target states through `execute_frame_graph_rhi_texture_schedule`, lets the executor own render pass
  begin/end envelopes for the scene and postprocess chain, and keeps scene/postprocess callbacks to pass-body recording; directional-shadow
  scheduled shadow-depth/scene-color/scene-depth inter-pass transitions, declared shadow-color/shadow-depth/scene-color/scene-depth
  writer-access-backed target-state preparation, executor-owned shadow-depth/scene-receiver/postprocess render pass envelopes, pass-body
  callbacks, overlay preparation outside render pass scopes, and reusable depth final-state restoration run through the same executor;
  shadow receiver descriptors live in caller-owned set 1 for the sample scene pipeline. Expose only first-party `postprocess_status`,
  `postprocess_depth_input_requested`, `postprocess_depth_input_ready`, package-visible `postprocess_policy_*` summary counters, selected
  D3D12 `postprocess_d3d12_execution_*` counters, `directional_shadow_status`, `directional_shadow_requested`, `directional_shadow_ready`,
  `directional_shadow_filter_mode`, `directional_shadow_filter_tap_count`, `directional_shadow_filter_radius_texels`, selected
  cascade/atlas/light-space counters, diagnostics, `framegraph_passes`, and `IRenderer::stats()` counters; validate target-specific scene,
  shadow receiver, shadow, and postprocess DXIL/SPIR-V artifacts plus installed smoke fields such as `postprocess_status=ready`,
  `postprocess_depth_input_ready=1`, `postprocess_policy_status=ready`, `postprocess_policy_ready=1`, `postprocess_policy_diagnostics=0`,
  `postprocess_policy_effects=1`, `postprocess_policy_framegraph_passes=2`, `postprocess_policy_scene_depth_required=1`,
  `postprocess_d3d12_execution_status=ready`, `postprocess_d3d12_execution_ready=1`, `postprocess_d3d12_execution_selected=1`,
  `postprocess_d3d12_execution_shader_evidence_ready=1`, `postprocess_d3d12_execution_passes_ok=1,
  vulkan_postprocess_execution_status=ready, vulkan_postprocess_execution_ready=1, vulkan_postprocess_execution_selected=1,
  vulkan_postprocess_execution_shader_evidence_ready=1, vulkan_postprocess_execution_passes_ok=1`, `directional_shadow_status=ready`,
  `directional_shadow_ready=1`, `directional_shadow_filter_mode=fixed_pcf_3x3`, `directional_shadow_filter_taps=9`,
  `directional_shadow_filter_radius_texels=1`, `directional_shadow_cascade_count=4`, `directional_shadow_atlas_width=900`,
  `directional_shadow_light_space_cascades=4`, `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_render_passes_recorded=6`,
  and `framegraph_barrier_steps_executed=15` when directional shadow filtering is requested. Do not expose `IRhiDevice`, swapchain frames,
  native texture/image views, descriptor handles, or frame-graph internals to gameplay, and do not generalize the sample package smoke into
  SSAO, DoF, fog, temporal history, shader-side cascade selection expansion, hardware comparison samplers, production shadow/postprocess
  stack support, broader production graph pass ownership beyond the completed raw primary, postprocess, directional-shadow, and
  viewport-clear render pass envelopes, renderer-wide manual-transition removal beyond the completed final-state, writer-access-backed
  pass-target-state policy, shadow-color target-state ownership, and viewport color-state executor slices, overlapping native alias
  execution, multi-queue scheduling, or production render graph ownership.
- For Renderer Package Quality Gates v1 work, keep `evaluate_win32_desktop_presentation_quality_gate` as a summary over existing
  `Win32DesktopPresentationReport` and `IRenderer::stats()` fields only. Package smokes may require `--require-renderer-quality-gates` and
  validate `renderer_quality_status=ready`, `renderer_quality_ready=1`, `renderer_quality_diagnostics=0`,
  `renderer_quality_expected_framegraph_passes=3`, `renderer_quality_expected_framegraph_render_passes=6`,
  `renderer_quality_framegraph_render_passes_ok=1`, `renderer_quality_expected_framegraph_barrier_steps=15`,
  `renderer_quality_framegraph_barrier_steps_ok=1`, and `renderer_quality_framegraph_execution_budget_ok=1`; do not add GPU timestamps,
  backend-native stats, native handle accessors, cross-backend performance parity, Metal readiness, or a general production renderer quality
  claim.
- For Win32/Vulkan game-window presentation, keep surface probing, Vulkan device/swapchain ownership, shader module creation, graphics
  pipeline creation, and `RhiFrameRenderer` ownership inside `mirakana_runtime_host_win32_presentation` and `mirakana_rhi_vulkan`. Use
  host-supplied precompiled SPIR-V bytecode only, validate it before shader module creation, query actual surface
  capabilities/formats/present modes before swapchain creation, and validate the strict smoke command `sample_desktop_runtime_shell --smoke
  --video-driver windows --require-vulkan-shaders --require-vulkan-renderer` on a ready host before calling the path visible.
- For Win32 presentation diagnostics, expose only first-party `Win32DesktopPresentationReport`, `Win32DesktopPresentationBackendReport`, and
  `Win32DesktopPresentationQualityGateReport` values for requested/selected backend, fallback status/reason, diagnostic counts, renderer
  stats, scene GPU status/stats, postprocess status, postprocess depth-input requested/ready status, directional shadow
  requested/ready/status/filter diagnostics, renderer quality summary fields, framegraph pass count, framegraph render-pass count, and
  framegraph barrier count. Do not add native handle getters, `IRhiDevice` getters, backend stats, swapchain frame handles, GPU timestamps,
  frame-graph internals, or scene GPU palette access to game/runtime-host public APIs.
- For runtime-host observability, bridge renderer information only from backend-neutral `IRenderer::stats()` into `mirakana_core`
  `CounterSample` values. `mirakana_core` trace export may serialize only first-party `DiagnosticCapture` CPU diagnostics, counters, and
  profile samples; do not expose `IRhiDevice`, backend stats, swapchain frame handles, native surfaces, or raw GPU timestamp data through
  `mirakana_runtime_host`. Backend-neutral RHI GPU debug markers, timestamp-frequency rows, read-only memory diagnostics, editor Resources
  diagnostics, and reviewed capture handoff evidence exist; production trace import/export, telemetry upload, crash reporting, allocator
  enforcement, broader backend timing capture, and package-visible performance budgets remain separate adapters.
- For RHI submit/wait changes, update `RhiStats` coverage for fence waits, wait failures, last submitted fence values, and last completed
  fence values on `NullRhiDevice` and any touched native backend.
- For RHI compute dispatch foundation work, use `mirakana::rhi::ComputePipelineDesc`, `ComputePipelineHandle`,
  `IRhiDevice::create_compute_pipeline`, `IRhiCommandList::bind_compute_pipeline`, descriptor-set binding, and `IRhiCommandList::dispatch`
  as backend-neutral contracts. NullRHI should validate compute queue state, pipeline/layout compatibility, non-zero workgroups, and compute
  stats; D3D12 proof work should stay backend-private through compute PSOs and readback tests. Do not claim generated-package compute morph
  outside the generated D3D12 POSITION package smoke evidence, async compute overlap, Vulkan/Metal compute parity, skin+morph composition,
  or broad renderer quality until separate slices prove them.
- For Runtime RHI Compute Morph D3D12 Proof v1 work, treat `mirakana::runtime_rhi::RuntimeMorphMeshComputeBinding` and
  `mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding` as the narrow helper that creates compute-visible base POSITION, morph
  POSITION-delta, weight, and output-position descriptor bindings plus an output storage/copy-source buffer. The proof is D3D12
  output-position readback only after `IRhiCommandList::dispatch`; renderer consumption, optional NORMAL/TANGENT compute output, and
  generated package smoke are separate slices, and async compute overlap, Vulkan/Metal compute parity, skin+morph composition,
  generated-package NORMAL/TANGENT compute output, or broad renderer quality remain unclaimed without separate evidence.
- For Runtime RHI Compute Morph Renderer Consumption D3D12 v1 work, use
  `mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding` only after the compute output buffer has
  `BufferUsage::vertex`; the helper maps that output POSITION buffer plus the original index buffer into `MeshGpuBinding` for a narrow
  `RhiFrameRenderer` draw/readback proof. Generated package smoke and optional NORMAL/TANGENT compute output are separate, and async compute
  overlap, Vulkan/Metal parity, generated-package NORMAL/TANGENT compute output, skin+morph composition, directional-shadow morph rendering,
  or broad renderer quality remain unclaimed.
- For Runtime RHI Compute Morph NORMAL/TANGENT Output D3D12 v1 work, keep `RuntimeMorphMeshComputeBindingOptions::output_normal_usage` and
  `output_tangent_usage` opt-in, require uploaded morph normal/tangent streams before creating output buffers, keep base
  POSITION/POSITION-delta/weights/output-POSITION descriptor bindings at `0..3`, bind optional normal/tangent delta inputs at `4/5`, bind
  optional output normal/tangent buffers at `6/7`, and prove the D3D12 path with normalized readback rows. Do not claim generated-package
  NORMAL/TANGENT compute smoke, async compute overlap, Vulkan/Metal parity, skin+morph composition, directional-shadow morph rendering,
  scene-schema compute-morph authoring, or broad renderer quality.
- For Generated 3D Compute Morph Package Smoke D3D12 v1 work, keep compute dispatch and output binding host-owned inside
  `mirakana_runtime_host_win32_presentation`; generated `DesktopRuntime3DPackage` may emit `vs_compute_morph`/`cs_compute_morph_position`,
  pass `compute_morph_*` shader bytecode and explicit mesh-to-morph rows, and require `scene_gpu_compute_morph_mesh_bindings`,
  `scene_gpu_compute_morph_dispatches`, `scene_gpu_compute_morph_mesh_resolved`, and `scene_gpu_compute_morph_draws`. Do not claim async
  compute overlap, Vulkan/Metal parity, generated-package NORMAL/TANGENT compute output, skin+morph composition, directional-shadow morph
  rendering, scene-schema compute-morph authoring, public native handles, or broad renderer quality.
- For Generated 3D Compute Morph NORMAL/TANGENT Package Smoke D3D12/Vulkan work, keep the NORMAL/TANGENT compute output request explicit on
  the Win32 scene renderer descriptor, pass `RuntimeMorphMeshComputeBindingOptions::output_normal_usage` and `output_tangent_usage` only
  when generated package smoke requests tangent-frame compute output, keep the POSITION-only and skinned compute package smokes compatible,
  and expose only first-party smoke counters/diagnostics. Do not claim Metal parity, Vulkan/D3D12 async compute overlap, graphics morph+skin
  composition, directional-shadow morph rendering, scene-schema compute-morph authoring, public native handles, or broad renderer quality.
- For Runtime RHI Compute Morph Queue Synchronization D3D12 v1 work, expose only backend-neutral queue-to-queue wait semantics such as
  `IRhiDevice::wait_for_queue(QueueKind queue, FenceValue fence)` and `RhiStats` queue-wait counters; route D3D12 through private
  queue/fence primitives, keep native queue/fence handles hidden, and keep host-side `IRhiDevice::wait` available as completion waiting
  rather than GPU queue ordering. Do not claim async compute overlap/performance, Vulkan/Metal parity, broad production frame graph
  scheduling beyond selected executor evidence, skin+morph composition, generated scene-schema authoring, directional-shadow morph
  rendering, public native handles, or broad renderer quality.
- For Generated 3D Compute Morph Queue Sync Package Smoke D3D12 v1 work, expose only first-party package smoke counters/diagnostics proving
  host-owned compute morph queue waits. Do not expose `IRhiDevice`, queue/fence handles, command lists, descriptor handles, swapchain
  frames, native backend stats, or native D3D12 handles to generated gameplay, and do not claim async compute overlap/performance,
  Vulkan/Metal parity, broad production frame graph scheduling beyond selected executor evidence, skin+morph composition, generated
  scene-schema authoring, directional-shadow morph rendering, or broad renderer quality.
- For Runtime RHI Compute Morph Skin Composition D3D12 v1 work, keep the proof inside `mirakana_runtime_rhi`, `mirakana_renderer`, and
  D3D12-focused tests using first-party mesh, morph, and skinning contracts. Prove compute-written morph vertex streams can compose with the
  GPU skinning path through a distinct joint-palette descriptor without exposing `IRhiDevice`, native D3D12 handles, descriptor handles, or
  generated gameplay access. Do not claim generated package skin+compute readiness, async compute overlap/performance, Vulkan/Metal parity,
  broad production frame graph scheduling beyond selected executor evidence, directional-shadow morph rendering, scene-schema compute-morph
  authoring, broad skeletal animation readiness, or broad renderer quality.
- For Runtime Scene RHI Compute Morph Skin Palette D3D12 v1 work, keep composition host-owned inside `mirakana_runtime_scene_rhi` by
  retaining selected compute-morphed skinned scene bindings in a first-party `SceneSkinnedGpuBindingPalette`. Use package assets, scene
  render packets, and `make_runtime_compute_morph_skinned_mesh_gpu_binding` without exposing `IRhiDevice`, descriptor handles, queue/fence
  handles, or native backend handles to gameplay. Do not claim generated package skin+compute readiness, graphics morph+skin composition,
  async compute overlap/performance, Vulkan/Metal parity, directional-shadow morph rendering, scene-schema compute-morph authoring, broad
  skeletal animation readiness, or broad renderer quality.
- For Generated 3D Compute Morph Skin Package Smoke D3D12/Vulkan work, including Generated 3D Compute Morph Skin Package Smoke D3D12 v1 and
  Generated 3D Compute Morph Skin Package Smoke Vulkan v1, keep the package smoke host-owned inside
  `mirakana_runtime_host_win32_presentation` and generated `DesktopRuntime3DPackage` package metadata. Generated gameplay may declare
  first-party package files, D3D12 DXIL or Vulkan SPIR-V artifact metadata, selected mesh-to-morph rows, and smoke requirements; expose only
  first-party package counters/diagnostics through `scene_gpu_compute_morph_skinned_*`. Use
  `Win32DesktopPresentationVulkanSceneRendererDesc::compute_morph_skinned_shader` and `compute_morph_skinned_mesh_bindings` for the Vulkan
  lane. Do not hand `IRhiDevice`, `SceneSkinnedGpuBindingPalette`, descriptor handles, queue/fence handles, swapchain frames, or native
  backend handles to gameplay, and do not claim graphics morph+skin composition, async compute overlap/performance, Metal parity,
  directional-shadow morph rendering, scene-schema compute-morph authoring, broad skeletal animation readiness, or broad renderer quality.
- For Runtime RHI Compute Morph Async Telemetry D3D12 v1 work, expose only first-party queue/fence sequencing telemetry and backend-neutral
  counters/diagnostics. Keep D3D12 queue, fence, command-list, descriptor, timestamp, and swapchain details backend-private, and do not
  claim performance overlap, async speedup, Vulkan/Metal parity, broad production frame graph scheduling beyond selected executor evidence,
  directional-shadow morph rendering, scene-schema compute-morph authoring, public native handles, or broad renderer quality.
- For Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1 work, keep sequencing evidence package-visible only through
  `--require-compute-morph-async-telemetry` and first-party `scene_gpu_compute_morph_async_*` counters/diagnostics owned by
  `mirakana_runtime_host_win32_presentation` and mapped from `IRhiDevice::stats()`. Do not expose `IRhiDevice`, command lists, descriptor
  handles, queue/fence handles, swapchain frames, native backend stats, GPU timestamps, or D3D12 objects to generated gameplay, and do not
  claim performance overlap, async speedup, Vulkan/Metal parity, broad production frame graph scheduling beyond selected executor evidence,
  graphics morph+skin composition, directional-shadow morph rendering, scene-schema compute-morph authoring, or broad renderer quality.
- For Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1 work, keep overlap evidence runtime/RHI-only and backend-private.
  `RhiAsyncOverlapReadinessDiagnostics` may report readiness or `not_proven_serial_dependency` from first-party counters, but must not
  expose native queues/fences/query heaps/timestamp resources or promote generated package performance claims, Vulkan/Metal parity, broad
  production frame graph scheduling beyond selected executor evidence, directional-shadow morph rendering, scene-schema compute-morph
  authoring, graphics morph+skin composition, or broad renderer quality.
- For Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1 work, keep output-ring readiness runtime/RHI-only. Build first-party
  multi-slot compute morph output buffers/descriptors so future pipelined scheduling can avoid same-frame graphics waits; do not claim
  measured overlap/performance, do not update generated `DesktopRuntime3DPackage` validation, and do not expose native
  queues/fences/descriptor handles/query heaps/timestamp resources.
- Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1 is completed runtime/RHI-only scheduling evidence. Do not promote it into
  generated package validation or measured performance claims.
- RHI D3D12 Per-Queue Fence Synchronization v1, RHI D3D12 Queue Timestamp Measurement Foundation v1, RHI D3D12 Queue Clock Calibration
  Foundation v1, RHI D3D12 Calibrated Queue Timing Diagnostics v1, Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1, RHI
  D3D12 Submitted Command Calibrated Timing Scopes v1, Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1, and RHI Vulkan
  Compute Dispatch Foundation v1 are complete: keep native fences, queues, events, D3D12 query heaps, readback buffers, raw GPU timestamp
  values, `GetClockCalibration` samples, raw GPU/CPU timing values, calibration math, calibrated timing rows, submitted-command timing rows,
  calibrated overlap diagnostics, `Vk*` handles, command pointers, shader modules, pipelines, descriptor sets, and command buffers
  backend-private. Runtime RHI Compute Morph Vulkan Proof v1 is complete for Vulkan POSITION-output readback through existing public
  RHI/runtime RHI contracts and `MK_VULKAN_TEST_COMPUTE_MORPH_SPV`; Runtime RHI Compute Morph Renderer Consumption Vulkan v1 is complete for
  renderer-consumption proof work through `make_runtime_compute_morph_output_mesh_gpu_binding`, `RhiFrameRenderer`, and
  `MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV` / `MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV`; Runtime RHI Compute Morph
  NORMAL/TANGENT Output Vulkan v1 is complete NORMAL/TANGENT output readback proof work through
  `RuntimeMorphMeshComputeBindingOptions::output_normal_usage`, `output_tangent_usage`, descriptor bindings `4..7`, and
  `MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV`; Generated 3D Compute Morph Package Smoke Vulkan v1 is complete POSITION package-smoke
  evidence, Generated 3D Compute Morph NORMAL/TANGENT Package Smoke Vulkan v1 is complete tangent-frame package-smoke evidence through
  `DesktopRuntime3DPackage`, `Win32DesktopPresentationVulkanSceneRendererDesc::enable_compute_morph_tangent_frame_output`,
  `--require-compute-morph-normal-tangent`, and explicit tangent-frame SPIR-V artifacts, and Generated 3D Compute Morph Skin Package Smoke
  Vulkan v1 is complete skin+compute package-smoke evidence through
  `Win32DesktopPresentationVulkanSceneRendererDesc::compute_morph_skinned_shader`, `compute_morph_skinned_mesh_bindings`,
  `--require-compute-morph-skin`, and skinned SPIR-V artifacts; do not claim async overlap/performance, Metal parity, graphics morph+skin
  composition, or broad renderer quality.
- For backend availability, shader artifact format, host support, or bootstrap sequencing, update `mirakana/rhi/backend_capabilities.hpp`
  and its tests before wiring native Vulkan/Metal/D3D12 code.
- For Vulkan runtime availability, use `mirakana::rhi::vulkan::probe_runtime_loader` and keep `vkGetInstanceProcAddr`/loader handles private
  to `engine/rhi/vulkan`.
- For Vulkan runtime global-command availability, use `mirakana::rhi::vulkan::probe_runtime_global_commands` before attempting native
  `vkCreateInstance` wiring.
- For Vulkan runtime instance API/extension availability, use `mirakana::rhi::vulkan::probe_runtime_instance_capabilities` before native
  `vkCreateInstance` wiring.
- For Vulkan transient instance command readiness, use `mirakana::rhi::vulkan::probe_runtime_instance_commands` and keep `VkInstance`
  handles private to `engine/rhi/vulkan`.
- For Vulkan persistent instance ownership, use `mirakana::rhi::vulkan::create_runtime_instance`; keep `VkInstance`, runtime library
  handles, and loaded function pointers behind `VulkanRuntimeInstance` PIMPL.
- For Vulkan physical-device availability, start with `mirakana::rhi::vulkan::probe_runtime_physical_device_count`, then use
  `mirakana::rhi::vulkan::probe_runtime_physical_device_snapshots` for properties, queue-family, device-extension, and dynamic-rendering
  snapshots before selecting or creating logical devices.
- Use `probe_runtime_physical_device_snapshots` plus `make_physical_device_candidate` before feeding runtime data into
  `select_physical_device`; do not infer API version, device type, swapchain, dynamic rendering, or queues from command-name availability
  alone.
- Use `mirakana::rhi::vulkan::probe_runtime_physical_device_selection` when editor/AI availability code needs the complete runtime
  snapshot-to-candidate-to-selection diagnostic path.
- Treat `probe_runtime_physical_device_snapshots` present support as unknown unless a surface-specific
  `mirakana::rhi::vulkan::probe_runtime_surface_support` call has populated it for the target surface.
- For Vulkan surface present support, use `mirakana::rhi::vulkan::vulkan_surface_instance_extensions` and
  `mirakana::rhi::vulkan::probe_runtime_surface_support`; keep `VkSurfaceKHR` private and destroy it before returning.
- For Vulkan command loading, validate loader/global/instance/device command readiness through
  `mirakana::rhi::vulkan::build_command_resolution_plan` before exposing a native backend as usable.
- For Vulkan instance creation, validate API version and required/optional instance extensions through
  `mirakana::rhi::vulkan::build_instance_create_plan` before native `vkCreateInstance` wiring.
- For Vulkan logical device creation, validate selected queues, `VK_KHR_swapchain`, optional device extensions, and dynamic rendering
  through `mirakana::rhi::vulkan::build_logical_device_create_plan` before using or extending native `vkCreateDevice` wiring.
- For Vulkan runtime logical device ownership, use `mirakana::rhi::vulkan::create_runtime_device` and
  `mirakana::rhi::vulkan::vulkan_device_command_requests`; keep `VkDevice`, `VkQueue`, `VkPhysicalDevice`, and `vkGetDeviceProcAddr` results
  behind `VulkanRuntimeDevice` PIMPL.
- For Vulkan command pool work, use `mirakana::rhi::vulkan::create_runtime_command_pool`; keep `VkCommandPool` and `VkCommandBuffer` behind
  `VulkanRuntimeCommandPool` PIMPL.
- For Vulkan swapchain work, feed surface data through `build_swapchain_create_plan` and `build_swapchain_resize_plan`, then use
  `mirakana::rhi::vulkan::create_runtime_swapchain`; keep `VkSurfaceKHR`, `VkSwapchainKHR`, swapchain images, and `VkImageView` handles
  private to `VulkanRuntimeSwapchain` PIMPL.
- For Vulkan frame sync/acquire/present work, use `mirakana::rhi::vulkan::create_runtime_frame_sync`,
  `mirakana::rhi::vulkan::acquire_next_runtime_swapchain_image`, and `mirakana::rhi::vulkan::present_runtime_swapchain_image`; keep
  `VkSemaphore`, `VkFence`, `VkPresentInfoKHR`, acquire/present `VkResult`, and image indices behind first-party result types and runtime
  PIMPL owners.
- For Vulkan dynamic rendering work, validate extent, color/depth attachment formats, and `vkCmdBeginRendering`/`vkCmdEndRendering` command
  readiness through `mirakana::rhi::vulkan::build_dynamic_rendering_plan`, then use
  `mirakana::rhi::vulkan::record_runtime_dynamic_rendering_clear` for shader-free clear recording or
  `mirakana::rhi::vulkan::record_runtime_dynamic_rendering_draw` / `record_runtime_texture_rendering_draw` for native begin-rendering,
  viewport/scissor, pipeline bind, optional vertex/index buffer binding, direct/indexed draw, and end-rendering recording while keeping
  `VkRenderingInfo`, `VkRenderingAttachmentInfo`, `VkCommandBuffer`, `VkPipeline`, image views, buffers, and function pointers private.
- For Vulkan synchronization2 work, validate `vkCmdPipelineBarrier2`/`vkQueueSubmit2` readiness and acquire-to-render-to-readback-to-present
  ordering through `mirakana::rhi::vulkan::build_frame_synchronization_plan`, then use
  `mirakana::rhi::vulkan::record_runtime_swapchain_frame_barrier` and `mirakana::rhi::vulkan::submit_runtime_command_buffer` for native
  barrier and queue submission work while keeping `VkDependencyInfo`, `VkImageMemoryBarrier2`, `VkSubmitInfo2`, queue handles, semaphores,
  fences, and command buffers private.
- For Vulkan readback work, use `mirakana::rhi::vulkan::create_runtime_readback_buffer`,
  `mirakana::rhi::vulkan::record_runtime_swapchain_image_readback`, and `mirakana::rhi::vulkan::read_runtime_readback_buffer`; keep
  `VkBuffer`, `VkDeviceMemory`, `VkMemoryRequirements`, `VkPhysicalDeviceMemoryProperties`, `VkBufferImageCopy`, mapped pointers, and native
  memory commands private to `engine/rhi/vulkan`.
- For Vulkan descriptor work, use `mirakana::rhi::vulkan::create_runtime_descriptor_set_layout`,
  `mirakana::rhi::vulkan::create_runtime_descriptor_set`, descriptor-aware `mirakana::rhi::vulkan::create_runtime_pipeline_layout`, and
  `mirakana::rhi::vulkan::record_runtime_descriptor_set_binding`; keep `VkDescriptorSetLayout`, `VkDescriptorPool`, `VkDescriptorSet`, and
  native descriptor bind commands private to `engine/rhi/vulkan`. At the `IRhiCommandList` layer, descriptor set binding must validate
  compatibility with the currently bound graphics pipeline layout, not just the descriptor set layout.
- For Vulkan sampled texture/sampler descriptor updates, keep `VkImageView`, `VkSampler`, `VkDescriptorImageInfo`, and sampled-image/sampler
  write details private; require explicit native ownership, update tests, and visible shader-read proof before promoting beyond public
  validation.
- For Vulkan `IRhiDevice` promotion, validate command-pool readiness, swapchain planning, dynamic rendering planning, synchronization2
  planning, SPIR-V shader validation, descriptor binding readiness, visible clear/readback proof, visible draw/readback proof, visible
  texture-sampling/readback proof, and visible depth/readback proof through `mirakana::rhi::vulkan::build_rhi_device_mapping_plan`, then
  pass the supported mapping plan into `mirakana::rhi::vulkan::create_rhi_device`; do not expose a Vulkan device as backend-neutral RHI from
  `owns_device()` alone.
- For Vulkan shader module work, use `mirakana::rhi::vulkan::create_runtime_shader_module`; keep `VkShaderModule`, shader module creation
  info, and destroy calls private to `VulkanRuntimeShaderModule` PIMPL.
- Validate Vulkan shader bytecode with `mirakana::rhi::vulkan::validate_spirv_shader_artifact` before native shader module creation.
- For Vulkan pipeline work, use `mirakana::rhi::vulkan::create_runtime_pipeline_layout` and
  `mirakana::rhi::vulkan::create_runtime_graphics_pipeline`; keep `VkPipelineLayout`, `VkPipeline`, `VkPipelineVertexInputStateCreateInfo`,
  `VkGraphicsPipelineCreateInfo`, dynamic-rendering pipeline create info, and native vertex input descriptions private to runtime PIMPL
  owners.
- For swapchain, presentation, RTV, resource barrier, or clear changes, cover creation, explicit
  `IRhiDevice::acquire_swapchain_frame`/`release_swapchain_frame`, invalid handles, resize/recreation, command recording, throttling, and
  synchronization in tests.
- For CPU readback or viewport display changes, route CPU copies through `IRhiDevice::read_buffer` and
  `mirakana::RhiViewportSurface::readback_color_frame`; route native display preparation through
  `mirakana::RhiViewportSurface::prepare_display_frame`; keep GUI texture objects out of renderer/RHI APIs.
- For `RhiViewportSurface` color-state and clear-frame changes, route `render_target`, `copy_source`, `shader_read`, and `viewport.clear`
  render pass envelopes through `execute_frame_graph_rhi_texture_schedule`; `engine/renderer/src/rhi_viewport_surface.cpp` must not call
  `transition_texture(` or direct render pass begin/end APIs.
- For editor viewport scene submission, route scene packets through `mirakana_scene_renderer` and
  `mirakana::RhiViewportSurface::render_frame`; keep GUI-local display textures in `mirakana_editor`.
- For editor viewport backend selection, persist only `mirakana::editor::EditorRenderBackend` preference in the project and resolve it
  through `mirakana::editor::choose_editor_render_backend`; do not instantiate real D3D12/Vulkan/Metal viewport paths until availability is
  backed by a verified visible backend.
- For the Windows editor D3D12 viewport bridge, require `mirakana::editor::ViewportShaderArtifactState` to verify non-empty VS/PS DXIL
  artifacts before marking D3D12 available, create the backend through `mirakana::rhi::d3d12::create_rhi_device`, and keep fallback to
  `NullRhiDevice` if native device or pipeline creation fails.
- For editor material previews, use `mirakana::editor::make_material_preview_shader_compile_requests` and shader-id-specific
  `ViewportShaderArtifactState` queries; do not reuse the default viewport shader when validating material descriptor sampling.
- For future editor Vulkan viewport promotion, require `mirakana::editor::ViewportShaderArtifactState::ready_for_vulkan()` to verify
  non-empty vertex/fragment SPIR-V artifacts before marking Vulkan available.
- For descriptor heap, root signature, descriptor table, vertex input, or PSO changes, cover creation, invalid handles/descriptions,
  command-list binding, shader visibility/register-space assumptions, vertex layout/attribute compatibility, and backend-neutral ownership
  boundaries in tests or docs.
- For scene-to-renderer handoff, use `mirakana::build_scene_render_packet` and keep the packet renderer-neutral; do not make
  `mirakana_scene` depend on renderer, RHI, or backend handles.
- Use `mirakana_scene_renderer` for packet-to-renderer command submission instead of adding a direct `mirakana_renderer` dependency to
  `mirakana_scene`.
- Keep scene renderer transform extraction deterministic and covered by tests when changing position, scale, or rotation behavior.
- Route scene sprites through `SpriteRendererComponent`, `SceneRenderPacket::sprites`, and `make_scene_sprite_command`; do not make
  `mirakana_scene` depend on renderer command types.
- Route scene cameras and lights through `SceneRenderPacket::cameras`/`lights`, `make_scene_camera_matrices`, and
  `make_scene_light_command`; do not make `mirakana_scene` depend on renderer command types.
- Resolve material base colors and material instance overrides through `mirakana::SceneMaterialPalette` until a fuller shader/material
  binding model exists.
- Before calling a backend "visible", verify a real window path with render-target transition, clear/draw, execute, fence wait, and present.

## Do Not

- Put rendering code in `engine/core`.
- Mix platform window lifecycle with renderer resource ownership.
- Commit shader compiler binaries or add shader cross-compilation dependencies without license records.
