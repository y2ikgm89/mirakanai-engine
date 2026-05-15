#requires -Version 7.0
#requires -PSEdition Core

# Chapter 2 for check-ai-integration.ps1 static contracts.

Assert-ContainsText $runtimeInputRebindingFocusConsumptionPlanText "RuntimeInputRebindingFocusCaptureResult" "runtime input rebinding focus consumption plan"
Assert-ContainsText $runtimeInputRebindingFocusConsumptionPlanText "capture_runtime_input_rebinding_action_with_focus" "runtime input rebinding focus consumption plan"
Assert-ContainsText $runtimeInputRebindingFocusConsumptionPlanText "focus_retained" "runtime input rebinding focus consumption plan"
Assert-ContainsText $runtimeInputRebindingFocusConsumptionPlanText "gameplay_input_consumed" "runtime input rebinding focus consumption plan"
Assert-ContainsText $runtimeInputRebindingFocusConsumptionPlanText "MK_runtime_tests" "runtime input rebinding focus consumption plan"
Assert-ContainsText $runtimeInputRebindingPresentationRowsPlanText "Runtime Input Rebinding Presentation Rows v1" "runtime input rebinding presentation rows plan"
Assert-ContainsText $runtimeInputRebindingPresentationRowsPlanText "**Status:** Completed" "runtime input rebinding presentation rows plan"
Assert-ContainsText $runtimeInputRebindingPresentationRowsPlanText "RuntimeInputRebindingPresentationToken" "runtime input rebinding presentation rows plan"
Assert-ContainsText $runtimeInputRebindingPresentationRowsPlanText "RuntimeInputRebindingPresentationRow" "runtime input rebinding presentation rows plan"
Assert-ContainsText $runtimeInputRebindingPresentationRowsPlanText "RuntimeInputRebindingPresentationModel" "runtime input rebinding presentation rows plan"
Assert-ContainsText $runtimeInputRebindingPresentationRowsPlanText "present_runtime_input_action_trigger" "runtime input rebinding presentation rows plan"
Assert-ContainsText $runtimeInputRebindingPresentationRowsPlanText "present_runtime_input_axis_source" "runtime input rebinding presentation rows plan"
Assert-ContainsText $runtimeInputRebindingPresentationRowsPlanText "make_runtime_input_rebinding_presentation" "runtime input rebinding presentation rows plan"
Assert-ContainsText $runtimeInputRebindingPresentationRowsPlanText "glyph_lookup_key" "runtime input rebinding presentation rows plan"
Assert-ContainsText $runtimeInputRebindingPresentationRowsPlanText "MK_runtime_tests" "runtime input rebinding presentation rows plan"
foreach ($docText in @($currentCapabilitiesText, $aiGameDevelopmentText, $roadmapText, $masterPlanText)) {
    Assert-ContainsText $docText "ComputePipelineDesc" "RHI compute dispatch foundation docs"
    Assert-ContainsText $docText "IRhiCommandList::dispatch" "RHI compute dispatch foundation docs"
    Assert-ContainsText $docText "RuntimeMorphMeshComputeBinding" "Runtime RHI compute morph proof docs"
    Assert-ContainsText $docText "create_runtime_morph_mesh_compute_binding" "Runtime RHI compute morph proof docs"
    Assert-ContainsText $docText "make_runtime_compute_morph_output_mesh_gpu_binding" "Runtime RHI compute morph renderer consumption docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph NORMAL/TANGENT Output D3D12" "Runtime RHI compute morph normal/tangent output docs"
    Assert-ContainsText $docText "Generated 3D Compute Morph NORMAL/TANGENT Package Smoke D3D12" "Generated 3D compute morph normal/tangent package smoke docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph Queue Synchronization D3D12" "Runtime RHI compute morph queue synchronization docs"
    Assert-ContainsText $docText "IRhiDevice::wait_for_queue" "Runtime RHI compute morph queue synchronization docs"
    Assert-ContainsText $docText "scene_gpu_compute_morph_queue_waits" "Generated 3D compute morph queue sync package smoke docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph Skin Composition D3D12" "Runtime RHI compute morph skin composition docs"
    Assert-ContainsText $docText "Runtime Scene RHI Compute Morph Skin Palette D3D12" "Runtime scene RHI compute morph skin palette docs"
    Assert-ContainsText $docText "Generated 3D Compute Morph Skin Package Smoke D3D12" "Generated 3D compute morph skin package smoke docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph Async Telemetry D3D12" "Runtime RHI compute morph async telemetry docs"
    Assert-ContainsText $docText "Generated 3D Compute Morph Async Telemetry Package Smoke D3D12" "Generated 3D compute morph async telemetry package smoke docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph Async Overlap Evidence D3D12" "Runtime RHI compute morph async overlap evidence docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph Pipelined Output Ring D3D12" "Runtime RHI compute morph pipelined output ring docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph Pipelined Scheduling D3D12" "Runtime RHI compute morph pipelined scheduling docs"
    Assert-ContainsText $docText "RHI D3D12 Per-Queue Fence Synchronization" "RHI D3D12 per-queue fence synchronization docs"
    Assert-ContainsText $docText "RHI D3D12 Queue Timestamp Measurement Foundation" "RHI D3D12 queue timestamp measurement foundation docs"
    Assert-ContainsText $docText "RHI D3D12 Queue Clock Calibration Foundation" "RHI D3D12 queue clock calibration foundation docs"
    Assert-ContainsText $docText "RHI D3D12 Calibrated Queue Timing Diagnostics" "RHI D3D12 calibrated queue timing diagnostics docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12" "Runtime RHI compute morph calibrated overlap diagnostics docs"
    Assert-ContainsText $docText "RHI D3D12 Submitted Command Calibrated Timing Scopes" "RHI D3D12 submitted command calibrated timing scopes docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12" "Runtime RHI compute morph submitted overlap diagnostics docs"
    Assert-ContainsText $docText "RHI Vulkan Compute Dispatch Foundation" "RHI Vulkan compute dispatch foundation docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph Vulkan Proof" "Runtime RHI compute morph Vulkan proof docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph Renderer Consumption Vulkan" "Runtime RHI compute morph renderer consumption Vulkan docs"
    Assert-ContainsText $docText "Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan" "Runtime RHI compute morph normal/tangent output Vulkan docs"
    Assert-ContainsText $docText "Generated 3D Compute Morph Package Smoke Vulkan" "Generated 3D compute morph package smoke Vulkan docs"
    Assert-ContainsText $docText "Generated 3D Compute Morph Skin Package Smoke Vulkan" "Generated 3D compute morph skin package smoke Vulkan docs"
    Assert-ContainsText $docText "2D Native Sprite Batching Execution v1" "2D native sprite batching execution docs"
    Assert-ContainsText $docText "2D Sprite Animation Package v1" "2D sprite animation package docs"
    Assert-ContainsText $docText "2D Tilemap Editor Runtime UX v1" "2D tilemap editor runtime UX docs"
    Assert-ContainsText $docText "GameEngine.RuntimeInputRebindingProfile.v1" "input rebinding profile UX docs"
    Assert-ContainsText $docText "Runtime Input Rebinding Capture Contract v1" "runtime input rebinding capture docs"
    Assert-ContainsText $docText "capture_runtime_input_rebinding_action" "runtime input rebinding capture docs"
    Assert-ContainsText $docText "Runtime Input Rebinding Focus Consumption v1" "runtime input rebinding focus consumption docs"
    Assert-ContainsText $docText "RuntimeInputRebindingFocusCaptureRequest" "runtime input rebinding focus consumption docs"
    Assert-ContainsText $docText "capture_runtime_input_rebinding_action_with_focus" "runtime input rebinding focus consumption docs"
    Assert-ContainsText $docText "gameplay_input_consumed" "runtime input rebinding focus consumption docs"
    Assert-ContainsText $docText "Runtime Input Rebinding Presentation Rows v1" "runtime input rebinding presentation rows docs"
    Assert-ContainsText $docText "RuntimeInputRebindingPresentationModel" "runtime input rebinding presentation rows docs"
    Assert-ContainsText $docText "make_runtime_input_rebinding_presentation" "runtime input rebinding presentation rows docs"
    Assert-ContainsText $docText "symbolic glyph lookup keys" "runtime input rebinding presentation rows docs"
    Assert-ContainsText $docText "platform input glyph generation" "runtime input rebinding presentation rows docs"
    Assert-ContainsText $docText "MK_VULKAN_TEST_COMPUTE_MORPH_SPV" "Runtime RHI compute morph Vulkan proof docs"
    Assert-ContainsText $docText "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV" "Runtime RHI compute morph renderer consumption Vulkan docs"
    Assert-ContainsText $docText "MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV" "Runtime RHI compute morph normal/tangent output Vulkan docs"
    Assert-ContainsText $docText "RhiAsyncOverlapReadinessDiagnostics" "Runtime RHI compute morph async overlap evidence docs"
    Assert-ContainsText $docText "not_proven_serial_dependency" "Runtime RHI compute morph async overlap evidence docs"
    Assert-ContainsText $docText "--require-compute-morph-async-telemetry" "Generated 3D compute morph async telemetry package smoke docs"
    Assert-ContainsText $docText "scene_gpu_compute_morph_async_" "Generated 3D compute morph async telemetry package smoke docs"
    Assert-ContainsText $docText "package-visible" "Generated 3D compute morph queue sync package smoke docs"
}
$currentCapabilitiesInputTimeline = [regex]::Match($currentCapabilitiesText, '(?m)^`docs/superpowers/plans/2026-05-06-2d-sprite-animation-package-v1\.md`.*$')
Assert-ContainsText $currentCapabilitiesInputTimeline.Value "RuntimeInputRebindingPresentationModel" "current capabilities input timeline summary"
Assert-ContainsText $currentCapabilitiesInputTimeline.Value "platform input glyph generation" "current capabilities input timeline summary"
$roadmapRuntimePresentationFoundationRow = [regex]::Match($roadmapText, '(?m)^- Runtime Input Rebinding Presentation Rows v1.*$')
Assert-ContainsText $roadmapRuntimePresentationFoundationRow.Value "RuntimeInputRebindingPresentationModel" "roadmap runtime foundation summary"
Assert-ContainsText $roadmapRuntimePresentationFoundationRow.Value "symbolic glyph lookup keys" "roadmap runtime foundation summary"
$masterPlanRuntimeUiLedgerNote = [regex]::Match($masterPlanText, '(?m)^Runtime UI and input ledger note:.*$')
Assert-ContainsText $masterPlanRuntimeUiLedgerNote.Value "RuntimeInputRebindingPresentationModel" "master plan runtime UI ledger note"
Assert-ContainsText $masterPlanRuntimeUiLedgerNote.Value "platform input glyph generation" "master plan runtime UI ledger note"
Assert-ContainsText $rhiPublicHeaderText "struct ComputePipelineDesc" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "create_compute_pipeline" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "bind_compute_pipeline" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "dispatch(" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "queue_waits" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "queue_wait_failures" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "compute_queue_submits" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "graphics_queue_submits" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "last_compute_submitted_fence_value" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "last_graphics_queue_wait_fence_value" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "QueueKind queue{QueueKind::graphics}" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "last_graphics_queue_wait_fence_queue" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "last_graphics_queue_wait_sequence" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "last_graphics_submitted_fence_value" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "wait_for_queue(QueueKind queue, FenceValue fence)" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "RhiAsyncOverlapReadinessStatus" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "RhiAsyncOverlapReadinessDiagnostics" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "diagnose_compute_graphics_async_overlap_readiness" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "RhiPipelinedComputeGraphicsScheduleEvidence" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "diagnose_pipelined_compute_graphics_async_overlap_readiness" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiPublicHeaderText "graphics_queue_waited_for_previous_compute" "engine/rhi/include/mirakana/rhi/rhi.hpp"
Assert-ContainsText $rhiAsyncOverlapSourceText "not_proven_serial_dependency" "engine/rhi/src/async_overlap.cpp"
Assert-ContainsText $rhiAsyncOverlapSourceText "ready_for_backend_private_timing" "engine/rhi/src/async_overlap.cpp"
Assert-ContainsText $rhiAsyncOverlapSourceText "same_frame_graphics_wait_serializes_compute" "engine/rhi/src/async_overlap.cpp"
Assert-ContainsText $rhiAsyncOverlapSourceText "missing_pipelined_slot_evidence" "engine/rhi/src/async_overlap.cpp"
Assert-ContainsText $rhiAsyncOverlapSourceText "graphics_queue_waited_for_previous_compute" "engine/rhi/src/async_overlap.cpp"
Assert-ContainsText $rhiAsyncOverlapSourceText "stats.last_graphics_queue_wait_fence_queue == QueueKind::compute" "engine/rhi/src/async_overlap.cpp"
Assert-ContainsText $rhiAsyncOverlapSourceText "stats.last_graphics_submit_sequence > stats.last_graphics_queue_wait_sequence" "engine/rhi/src/async_overlap.cpp"
Assert-ContainsText $nullRhiSourceText "NullRhiDevice::wait_for_queue" "engine/rhi/src/null_rhi.cpp"
Assert-ContainsText $nullRhiSourceText "last_signaled_by_queue_" "engine/rhi/src/null_rhi.cpp"
Assert-ContainsText $nullRhiSourceText "record_queue_submit(stats_, queue, last_signaled_)" "engine/rhi/src/null_rhi.cpp"
Assert-ContainsText $nullRhiSourceText "record_queue_wait(stats_, queue, fence)" "engine/rhi/src/null_rhi.cpp"
Assert-ContainsText $nullRhiSourceText "queue_wait_failures" "engine/rhi/src/null_rhi.cpp"
Assert-ContainsText $d3d12RhiHeaderText "NativeComputePipelineDesc" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "last_copy_queue_wait_fence_value" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "last_graphics_queue_wait_fence_queue" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiSourceText "D3D12_COMPUTE_PIPELINE_STATE_DESC" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "SetComputeRootDescriptorTable" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "context_->queue_wait_for_fence(queue, fence)" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "record_queue_submit(stats_, d3d12_commands->queue_kind(), fence)" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "record_queue_wait(impl_->stats, queue, fence)" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "queue_fences" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "ensure_fence(QueueKind queue)" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $vulkanRhiSourceText "VulkanRhiDevice::wait_for_queue" "engine/rhi/vulkan/src/vulkan_backend.cpp"
Assert-ContainsText $vulkanRhiSourceText "record_queue_submit(stats_, vulkan_commands->queue_kind(), fence)" "engine/rhi/vulkan/src/vulkan_backend.cpp"
Assert-ContainsText $vulkanRhiHeaderText "VulkanRuntimeComputePipeline" "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp"
Assert-ContainsText $vulkanRhiHeaderText "create_runtime_compute_pipeline" "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp"
Assert-ContainsText $vulkanRhiHeaderText "record_runtime_compute_pipeline_binding" "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp"
Assert-ContainsText $vulkanRhiHeaderText "record_runtime_compute_dispatch" "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp"
Assert-ContainsText $vulkanRhiHeaderText "compute_dispatch_mapped" "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp"
Assert-ContainsText $vulkanRhiSourceText "vkCreateComputePipelines" "engine/rhi/vulkan/src/vulkan_backend.cpp"
Assert-ContainsText $vulkanRhiSourceText "vkCmdDispatch" "engine/rhi/vulkan/src/vulkan_backend.cpp"
Assert-ContainsText $vulkanRhiSourceText "ComputePipelineHandle create_compute_pipeline" "engine/rhi/vulkan/src/vulkan_backend.cpp"
Assert-ContainsText $vulkanRhiSourceText "void bind_compute_pipeline(ComputePipelineHandle pipeline)" "engine/rhi/vulkan/src/vulkan_backend.cpp"
Assert-ContainsText $vulkanRhiSourceText "void dispatch(std::uint32_t group_count_x" "engine/rhi/vulkan/src/vulkan_backend.cpp"
Assert-ContainsText $backendScaffoldTestsText "MK_VULKAN_TEST_COMPUTE_SPV" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $backendScaffoldTestsText "vulkan rhi device bridge proves compute dispatch readback with configured SPIR-V artifact" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $backendScaffoldTestsText "MK_VULKAN_TEST_COMPUTE_MORPH_SPV" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $backendScaffoldTestsText "vulkan rhi device bridge proves runtime compute morph position readback with configured SPIR-V artifact" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $backendScaffoldTestsText "vulkan rhi frame renderer consumes runtime compute morph output positions when configured" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $backendScaffoldTestsText "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $backendScaffoldTestsText "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $backendScaffoldTestsText "vulkan rhi device bridge proves runtime compute morph normal tangent readback with configured SPIR-V artifact" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $backendScaffoldTestsText "MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $backendScaffoldTestsText "compute_options.output_normal_usage" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $backendScaffoldTestsText "compute_options.output_tangent_usage" "tests/unit/backend_scaffold_tests.cpp"
Assert-ContainsText $vulkanComputeMorphShaderText "RWByteAddressBuffer base_positions" "tests/shaders/vulkan_compute_morph_position.hlsl"
Assert-ContainsText $vulkanComputeMorphShaderText "RWByteAddressBuffer output_positions" "tests/shaders/vulkan_compute_morph_position.hlsl"
Assert-ContainsText $vulkanComputeMorphShaderText "[numthreads(1, 1, 1)]" "tests/shaders/vulkan_compute_morph_position.hlsl"
Assert-ContainsText $vulkanComputeMorphRendererShaderText "vs_main" "tests/shaders/vulkan_compute_morph_renderer_position.hlsl"
Assert-ContainsText $vulkanComputeMorphRendererShaderText "ps_main" "tests/shaders/vulkan_compute_morph_renderer_position.hlsl"
Assert-ContainsText $vulkanComputeMorphRendererShaderText "[[vk::location(0)]]" "tests/shaders/vulkan_compute_morph_renderer_position.hlsl"
Assert-ContainsText $vulkanComputeMorphTangentFrameShaderText "RWByteAddressBuffer output_normals" "tests/shaders/vulkan_compute_morph_tangent_frame.hlsl"
Assert-ContainsText $vulkanComputeMorphTangentFrameShaderText "RWByteAddressBuffer output_tangents" "tests/shaders/vulkan_compute_morph_tangent_frame.hlsl"
Assert-ContainsText $vulkanComputeMorphTangentFrameShaderText "[[vk::binding(7, 0)]]" "tests/shaders/vulkan_compute_morph_tangent_frame.hlsl"
Assert-ContainsText $runtimeHostSdl3HeaderText "SdlDesktopPresentationVulkanSceneRendererDesc" "engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostSdl3HeaderText "compute_morph_vertex_shader" "engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostSdl3HeaderText "compute_morph_shader" "engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostSdl3HeaderText "compute_morph_mesh_bindings" "engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostSdl3HeaderText "compute_morph_skinned_shader" "engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostSdl3HeaderText "compute_morph_skinned_mesh_bindings" "engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostSdl3HeaderText "compute_morph_queue_waits" "engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostSdl3HeaderText "compute_morph_async_compute_queue_submits" "engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostSdl3HeaderText "compute_morph_async_last_graphics_submitted_fence_value" "engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostSdl3SourceText "Vulkan scene compute morph vertex SPIR-V validation failed" "engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostSdl3SourceText "Vulkan scene compute morph compute SPIR-V validation failed" "engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostSdl3SourceText "Vulkan scene compute mapping SPIR-V validation failed" "engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostSdl3SourceText "build_scene_compute_morph_bindings(" "engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostSdl3SourceText '"Vulkan"' "engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostSdl3SourceText "device.wait_for_queue(rhi::QueueKind::graphics, fence)" "engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostSdl3SourceText "++result.queue_waits" "engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostSdl3SourceText "compute_morph_bindings.queue_waits" "engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostSdl3SourceText "dispatch_scene_compute_morph_skinned_bindings" "engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostSdl3SceneGpuInjectingRendererText "compute_morph_async_last_graphics_submitted_fence_value" "engine/runtime_host/sdl3/src/scene_gpu_binding_injecting_renderer.hpp"
Assert-ContainsText $runtimeHostSdl3SceneGpuInjectingRendererText "rhi_stats.last_graphics_submitted_fence_value" "engine/runtime_host/sdl3/src/scene_gpu_binding_injecting_renderer.hpp"
Assert-ContainsText $runtimeHostSdl3TestsText "stats.compute_morph_queue_waits == 1" "tests/unit/runtime_host_sdl3_tests.cpp"
Assert-ContainsText $runtimeHostSdl3TestsText "vulkan_desc.compute_morph_vertex_shader" "tests/unit/runtime_host_sdl3_tests.cpp"
Assert-ContainsText $runtimeHostSdl3TestsText "vulkan_desc.compute_morph_shader" "tests/unit/runtime_host_sdl3_tests.cpp"
Assert-ContainsText $runtimeHostSdl3TestsText "vulkan_desc.compute_morph_mesh_bindings" "tests/unit/runtime_host_sdl3_tests.cpp"
Assert-ContainsText $runtimeHostSdl3TestsText "vulkan_desc.compute_morph_skinned_shader" "tests/unit/runtime_host_sdl3_tests.cpp"
Assert-ContainsText $runtimeHostSdl3TestsText "vulkan_desc.compute_morph_skinned_mesh_bindings" "tests/unit/runtime_host_sdl3_tests.cpp"
Assert-ContainsText $runtimeHostSdl3TestsText "stats.compute_morph_async_compute_queue_submits == 1" "tests/unit/runtime_host_sdl3_tests.cpp"
Assert-ContainsText $runtimeHostSdl3TestsText "stats.compute_morph_async_last_graphics_submitted_fence_value == graphics_fence.value" "tests/unit/runtime_host_sdl3_tests.cpp"
Assert-ContainsText $runtimeHostSdl3PublicApiText "stats.compute_morph_queue_waits == 1" "tests/unit/runtime_host_sdl3_public_api_compile.cpp"
Assert-ContainsText $runtimeHostSdl3PublicApiText "stats.compute_morph_async_compute_queue_submits == 1" "tests/unit/runtime_host_sdl3_public_api_compile.cpp"
Assert-ContainsText $runtimeHostSdl3PublicApiText "scene_renderer.compute_morph_vertex_shader.entry_point" "tests/unit/runtime_host_sdl3_public_api_compile.cpp"
Assert-ContainsText $runtimeHostSdl3PublicApiText "scene_renderer.compute_morph_shader.entry_point" "tests/unit/runtime_host_sdl3_public_api_compile.cpp"
Assert-ContainsText $runtimeHostSdl3PublicApiText "scene_renderer.compute_morph_mesh_bindings.push_back" "tests/unit/runtime_host_sdl3_public_api_compile.cpp"
Assert-ContainsText $runtimeHostSdl3PublicApiText "scene_renderer.compute_morph_skinned_shader.entry_point" "tests/unit/runtime_host_sdl3_public_api_compile.cpp"
Assert-ContainsText $runtimeHostSdl3PublicApiText "scene_renderer.compute_morph_skinned_mesh_bindings.push_back" "tests/unit/runtime_host_sdl3_public_api_compile.cpp"
Assert-ContainsText $newGameToolText "scene_gpu_compute_morph_queue_waits" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "kRuntimeSceneVulkanComputeMorphVertexShaderPath" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "kRuntimeSceneVulkanComputeMorphShaderPath" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "load_packaged_vulkan_scene_compute_morph_shaders" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "vulkan_compute_morph_shader_diagnostic" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "Vulkan POSITION/NORMAL/TANGENT compute morph package smoke through explicit SPIR-V artifacts" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "kRuntimeSceneVulkanComputeMorphSkinnedVertexShaderPath" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "kRuntimeSceneVulkanComputeMorphSkinnedShaderPath" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "load_packaged_vulkan_scene_compute_morph_skinned_shaders" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "vulkan_compute_morph_skinned_shader_diagnostic" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "Vulkan skin+compute package smoke counters through explicit SPIR-V artifacts" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "Vulkan compute morph package smoke does not support async telemetry requirements; use" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "scene_compute_morph.vs.spv" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "scene_compute_morph.cs.spv" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "scene_compute_morph_skinned.vs.spv" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "scene_compute_morph_skinned.cs.spv" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "scene_gpu_stats.compute_morph_queue_waits < 1" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "--require-compute-morph-async-telemetry" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "scene_gpu_compute_morph_async_compute_queue_submits" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "scene_gpu_stats.compute_morph_async_compute_queue_submits < 1" "tools/new-game.ps1"
Assert-ContainsText $runtimeRhiHeaderText "struct RuntimeMorphMeshComputeBinding" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $runtimeRhiHeaderText "create_runtime_morph_mesh_compute_binding" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $runtimeRhiHeaderText "make_runtime_compute_morph_output_mesh_gpu_binding" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $runtimeRhiHeaderText "make_runtime_compute_morph_skinned_mesh_gpu_binding" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $runtimeRhiHeaderText "output_normal_usage" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $runtimeRhiHeaderText "output_tangent_usage" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $runtimeRhiHeaderText "output_normal_buffer" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $runtimeRhiHeaderText "output_tangent_buffer" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $runtimeRhiHeaderText "RuntimeMorphMeshComputeOutputSlot" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $runtimeRhiHeaderText "output_slot_count" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $runtimeRhiHeaderText "output_slots" "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
Assert-ContainsText $rendererHeaderText "skin_attribute_vertex_buffer" "engine/renderer/include/mirakana/renderer/renderer.hpp"
Assert-ContainsText $rendererSourceText "bind_skinned_mesh_vertex_buffers" "engine/renderer/src/rhi_frame_renderer.cpp"
Assert-ContainsText $rendererSourceText "skin_attribute_vertex_stride" "engine/renderer/src/rhi_frame_renderer.cpp"
Assert-ContainsText $runtimeRhiSourceText "RuntimeMorphMeshComputeBinding" "engine/runtime_rhi/src/runtime_upload.cpp"
Assert-ContainsText $runtimeRhiSourceText "create_runtime_morph_mesh_compute_binding" "engine/runtime_rhi/src/runtime_upload.cpp"
Assert-ContainsText $runtimeRhiSourceText "make_runtime_compute_morph_output_mesh_gpu_binding" "engine/runtime_rhi/src/runtime_upload.cpp"
Assert-ContainsText $runtimeRhiSourceText "make_runtime_compute_morph_skinned_mesh_gpu_binding" "engine/runtime_rhi/src/runtime_upload.cpp"
Assert-ContainsText $runtimeRhiSourceText "runtime morph compute normal output requires morph normal delta buffer" "engine/runtime_rhi/src/runtime_upload.cpp"
Assert-ContainsText $runtimeRhiSourceText "runtime morph compute tangent output requires morph tangent delta buffer" "engine/runtime_rhi/src/runtime_upload.cpp"
Assert-ContainsText $runtimeRhiSourceText "runtime morph compute output ring requires at least one slot" "engine/runtime_rhi/src/runtime_upload.cpp"
Assert-ContainsText $runtimeRhiSourceText "runtime morph compute output ring currently supports POSITION output slots only" "engine/runtime_rhi/src/runtime_upload.cpp"
Assert-ContainsText $runtimeRhiSourceText "output_position_resources" "engine/runtime_rhi/src/runtime_upload.cpp"
Assert-ContainsText $runtimeRhiSourceText "runtime_morph_compute_output_slot" "engine/runtime_rhi/src/runtime_upload.cpp"
Assert-ContainsText $runtimeSceneRhiHeaderText "RuntimeSceneComputeMorphSkinnedMeshBinding" "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
Assert-ContainsText $runtimeSceneRhiHeaderText "compute_morph_skinned_mesh_bindings" "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
Assert-ContainsText $runtimeSceneRhiHeaderText "compute_morph_output_position_bytes" "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
Assert-ContainsText $runtimeSceneRhiSourceText "make_runtime_compute_morph_skinned_mesh_gpu_binding" "engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp"
Assert-ContainsText $runtimeSceneRhiSourceText "make_skinned_position_payload" "engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp"
Assert-ContainsText $runtimeSceneRhiSourceText "add_compute_morph_skinned_mesh_binding" "engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp"
Assert-ContainsText $runtimeSceneRhiSourceText "compute_binding.output_slots" "engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp"
Assert-ContainsText $runtimeRhiTestsText "runtime rhi creates compute morph binding for position output" "tests/unit/runtime_rhi_tests.cpp"
Assert-ContainsText $runtimeRhiTestsText "runtime rhi creates compute morph binding for normal and tangent outputs" "tests/unit/runtime_rhi_tests.cpp"
Assert-ContainsText $runtimeRhiTestsText "runtime rhi creates compute morph output ring with distinct position slots" "tests/unit/runtime_rhi_tests.cpp"
Assert-ContainsText $runtimeRhiTestsText "runtime rhi exposes compute morph output as mesh binding" "tests/unit/runtime_rhi_tests.cpp"
Assert-ContainsText $runtimeRhiTestsText "runtime rhi composes compute morph output with skinned mesh attributes" "tests/unit/runtime_rhi_tests.cpp"
$runtimeSceneRhiTestsText = Get-AgentSurfaceText "tests/unit/runtime_scene_rhi_tests.cpp"
Assert-ContainsText $runtimeSceneRhiTestsText "runtime scene rhi builds compute morph skinned gpu palette from selected cooked assets" "tests/unit/runtime_scene_rhi_tests.cpp"
Assert-ContainsText $runtimeSceneRhiTestsText "compute_morph_output_position_bytes == 36" "tests/unit/runtime_scene_rhi_tests.cpp"
Assert-ContainsText $rhiTestsText "null rhi records queue waits for submitted fences" "tests/unit/rhi_tests.cpp"
Assert-ContainsText $rhiTestsText "last_graphics_queue_wait_fence_value == compute_fence.value" "tests/unit/rhi_tests.cpp"
Assert-ContainsText $rhiTestsText "last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute" "tests/unit/rhi_tests.cpp"
Assert-ContainsText $rhiTestsText "null rhi submitted fences carry queue identity and per queue values" "tests/unit/rhi_tests.cpp"
Assert-ContainsText $rhiTestsText "last_graphics_submitted_fence_value == graphics_fence.value" "tests/unit/rhi_tests.cpp"
Assert-ContainsText $rhiTestsText "null rhi reports invalid queue wait attempts" "tests/unit/rhi_tests.cpp"
Assert-ContainsText $rhiTestsText "rhi async overlap readiness diagnostics classify serial graphics waits" "tests/unit/rhi_tests.cpp"
Assert-ContainsText $rhiTestsText "rhi async overlap readiness diagnostics classify pipelined output slot scheduling" "tests/unit/rhi_tests.cpp"
Assert-ContainsText $rhiTestsText "unsupported_missing_timestamp_support" "tests/unit/rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi compute morph writes morphed runtime positions" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi compute morph output ring writes a selected position slot" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "compile_runtime_morph_position_output_slot_compute_shader" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "output_slot1" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi compute morph writes morphed runtime normals and tangents" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi frame renderer consumes compute morph output positions" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi frame renderer composes compute morphed positions with skinned joint palette" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device synchronizes graphics queue with compute fence" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device uses per queue fence identity for colliding fence values" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device reports invalid queue wait attempts" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "device->wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence)" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "device->stats().queue_waits == 1" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "context->stats().last_copy_queue_wait_fence_value == graphics_fence.value" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "device->stats().last_graphics_queue_wait_fence_value == compute_fence.value" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "device->stats().last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device reports compute graphics async overlap as serial dependency when graphics waits" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device reports pipelined compute graphics output slot scheduling as a timing candidate" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "diagnose_compute_graphics_async_overlap_readiness" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "diagnose_pipelined_compute_graphics_async_overlap_readiness" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiHeaderText "QueueTimestampMeasurementStatus" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "QueueTimestampMeasurementSupport" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "QueueTimestampInterval" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "queue_timestamp_measurement_support" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "measure_queue_timestamp_interval" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiSourceText "GetTimestampFrequency" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "D3D12_QUERY_HEAP_TYPE_TIMESTAMP" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "ResolveQueryData" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "d3d12 queue timestamp interval measured" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiHeaderText "QueueClockCalibrationStatus" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "QueueClockCalibration" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "calibrate_queue_clock" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiSourceText "GetClockCalibration" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "d3d12 queue clock calibration ready" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiHeaderText "QueueCalibratedTimingStatus" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "QueueCalibratedTiming" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "measure_calibrated_queue_timing" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "QueueCalibratedOverlapStatus" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "QueueCalibratedOverlapDiagnostics" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "measure_rhi_device_calibrated_queue_timing" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "diagnose_calibrated_compute_graphics_overlap" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "SubmittedCommandCalibratedTimingStatus" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "SubmittedCommandCalibratedTiming" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "read_rhi_device_submitted_command_calibrated_timing" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "diagnose_rhi_device_submitted_command_compute_graphics_overlap" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiSourceText "query_performance_counter_frequency" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "convert_gpu_timestamp_to_qpc" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "d3d12 calibrated queue timing measured" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "d3d12 calibrated overlap diagnostic measured" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "record_submitted_command_timing_begin" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "d3d12 submitted command calibrated timing measured" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiSourceText "diagnose_rhi_device_submitted_command_compute_graphics_overlap" "engine/rhi/d3d12/src/d3d12_backend.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context reports graphics and compute timestamp measurement support" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context records backend private queue timestamp intervals" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context reports graphics and compute queue clock calibration" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context records backend private calibrated queue timing diagnostics" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "diagnose_rhi_device_submitted_command_compute_graphics_overlap" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "create_runtime_morph_mesh_compute_binding" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "make_runtime_compute_morph_output_mesh_gpu_binding" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $renderingSkillText "IRhiCommandList::dispatch" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "RuntimeMorphMeshComputeBinding" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "make_runtime_compute_morph_output_mesh_gpu_binding" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph NORMAL/TANGENT Output D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Generated 3D Compute Morph NORMAL/TANGENT Package Smoke D3D12/Vulkan work" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Generated 3D Compute Morph NORMAL/TANGENT Package Smoke Vulkan v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph Queue Synchronization D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Generated 3D Compute Morph Queue Sync Package Smoke D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph Skin Composition D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime Scene RHI Compute Morph Skin Palette D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Generated 3D Compute Morph Skin Package Smoke D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Generated 3D Compute Morph Skin Package Smoke D3D12/Vulkan work" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Generated 3D Compute Morph Skin Package Smoke Vulkan v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph Async Telemetry D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "RHI D3D12 Per-Queue Fence Synchronization v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "RHI D3D12 Queue Timestamp Measurement Foundation v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "RHI D3D12 Queue Clock Calibration Foundation v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "RHI D3D12 Calibrated Queue Timing Diagnostics v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "RHI D3D12 Submitted Command Calibrated Timing Scopes v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph Vulkan Proof v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph Renderer Consumption Vulkan v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "Generated 3D Compute Morph Package Smoke Vulkan v1" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "MK_VULKAN_TEST_COMPUTE_MORPH_SPV" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "scene_gpu_compute_morph_async_*" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "DesktopRuntime3DPackage" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "SceneSkinnedGpuBindingPalette" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $renderingSkillText "output_normal_usage" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph NORMAL/TANGENT Output D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Generated 3D Compute Morph NORMAL/TANGENT Package Smoke D3D12/Vulkan work" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Generated 3D Compute Morph NORMAL/TANGENT Package Smoke Vulkan v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph Queue Synchronization D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Generated 3D Compute Morph Queue Sync Package Smoke D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph Skin Composition D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime Scene RHI Compute Morph Skin Palette D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Generated 3D Compute Morph Skin Package Smoke D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Generated 3D Compute Morph Skin Package Smoke D3D12/Vulkan work" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Generated 3D Compute Morph Skin Package Smoke Vulkan v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph Async Telemetry D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "RHI D3D12 Per-Queue Fence Synchronization v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "RHI D3D12 Queue Timestamp Measurement Foundation v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "RHI D3D12 Queue Clock Calibration Foundation v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "RHI D3D12 Calibrated Queue Timing Diagnostics v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "RHI D3D12 Submitted Command Calibrated Timing Scopes v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph Vulkan Proof v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph Renderer Consumption Vulkan v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Generated 3D Compute Morph Package Smoke Vulkan v1" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "MK_VULKAN_TEST_COMPUTE_MORPH_SPV" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "scene_gpu_compute_morph_async_*" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "DesktopRuntime3DPackage" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "SceneSkinnedGpuBindingPalette" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "output_tangent_usage" ".claude/skills/gameengine-rendering/SKILL.md"
foreach ($gameSkillText in @($gameDevelopmentSkillText, $claudeGameDevelopmentSkillText)) {
    Assert-ContainsText $gameSkillText "make_runtime_compute_morph_skinned_mesh_gpu_binding" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Runtime Scene RHI Compute Morph Skin Palette D3D12 v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Generated 3D Compute Morph Skin Package Smoke D3D12 v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Generated 3D Compute Morph Skin Package Smoke Vulkan v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Runtime RHI Compute Morph Async Telemetry D3D12 v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Runtime RHI Compute Morph Vulkan Proof v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Runtime RHI Compute Morph Renderer Consumption Vulkan v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Generated 3D Compute Morph Package Smoke Vulkan v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "RHI D3D12 Per-Queue Fence Synchronization v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "RHI D3D12 Queue Timestamp Measurement Foundation v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "RHI D3D12 Queue Clock Calibration Foundation v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "RHI D3D12 Calibrated Queue Timing Diagnostics v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "RHI D3D12 Submitted Command Calibrated Timing Scopes v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "scene_gpu_compute_morph_async_*" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "DesktopRuntime3DPackage" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "SceneSkinnedGpuBindingPalette" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "runtime/.gitattributes" "gameengine game-development skill"
    Assert-ContainsText $gameSkillText "text eol=lf" "gameengine game-development skill"
}

$validationRecipeNames = @{}
foreach ($recipe in $manifest.validationRecipes) {
    $validationRecipeNames[$recipe.name] = $true
}
$moduleNames = @{}
foreach ($module in $manifest.modules) {
    $moduleNames[$module.name] = $true
}
$packagingTargetNames = @{}
foreach ($target in $manifest.packagingTargets) {
    $packagingTargetNames[$target.name] = $true
}
$allowedProductionStatuses = @("ready", "host-gated", "planned", "blocked")
$expectedProductionRecipeIds = @(
    "headless-gameplay",
    "desktop-runtime-config-package",
    "desktop-runtime-cooked-scene-package",
    "desktop-runtime-material-shader-package",
    "ai-navigation-headless",
    "runtime-ui-headless",
    "2d-playable-source-tree",
    "2d-desktop-runtime-package",
    "3d-playable-desktop-package",
    "native-gpu-runtime-ui-overlay",
    "native-ui-textured-sprite-atlas",
    "native-ui-atlas-package-metadata",
    "future-3d-playable-vertical-slice"
)
$productionRecipeIds = @{}
foreach ($recipe in $productionLoop.recipes) {
    Assert-JsonProperty $recipe @("id", "status", "requiredModules", "allowedTemplates", "allowedPackagingTargets", "validationRecipes", "unsupportedClaims", "followUpCapability") "engine/agent/manifest.json aiOperableProductionLoop recipe"
    if ($productionRecipeIds.ContainsKey($recipe.id)) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop recipe id is duplicated: $($recipe.id)"
    }
    $productionRecipeIds[$recipe.id] = $true
    if ($allowedProductionStatuses -notcontains $recipe.status) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop recipe '$($recipe.id)' has invalid status: $($recipe.status)"
    }
    foreach ($module in @($recipe.requiredModules)) {
        if (-not $moduleNames.ContainsKey($module)) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop recipe '$($recipe.id)' references unknown module: $module"
        }
    }
    foreach ($target in @($recipe.allowedPackagingTargets)) {
        if (-not $packagingTargetNames.ContainsKey($target)) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop recipe '$($recipe.id)' references unknown packaging target: $target"
        }
    }
    foreach ($validationRecipe in @($recipe.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop recipe '$($recipe.id)' references unknown validation recipe: $validationRecipe"
        }
    }
    if (@("ready", "host-gated") -contains $recipe.status -and @($recipe.validationRecipes).Count -lt 1) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop recipe '$($recipe.id)' must declare validation recipes"
    }
}
foreach ($recipeId in $expectedProductionRecipeIds) {
    if (-not $productionRecipeIds.ContainsKey($recipeId)) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing recipe id: $recipeId"
    }
}
foreach ($recipeId in @("future-3d-playable-vertical-slice")) {
    $futureRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq $recipeId })
    if ($futureRecipe.Count -ne 1 -or $futureRecipe[0].status -ne "planned") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop recipe '$recipeId' must remain planned"
    }
}
foreach ($recipeId in @("desktop-runtime-config-package", "desktop-runtime-cooked-scene-package", "desktop-runtime-material-shader-package", "2d-desktop-runtime-package", "3d-playable-desktop-package", "native-gpu-runtime-ui-overlay", "native-ui-textured-sprite-atlas", "native-ui-text-glyph-atlas", "native-ui-atlas-package-metadata")) {
    $desktopRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq $recipeId })
    if ($desktopRecipe.Count -ne 1 -or $desktopRecipe[0].status -ne "host-gated") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop recipe '$recipeId' must remain host-gated"
    }
}
foreach ($recipeId in @("headless-gameplay", "ai-navigation-headless", "runtime-ui-headless", "2d-playable-source-tree")) {
    $readyRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq $recipeId })
    if ($readyRecipe.Count -ne 1 -or $readyRecipe[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop recipe '$recipeId' must remain ready"
    }
}
$sourceTree2dRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "2d-playable-source-tree" })
if ($sourceTree2dRecipe.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose exactly one 2d-playable-source-tree recipe"
} else {
    foreach ($module in @("MK_core", "MK_runtime", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($sourceTree2dRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json 2d-playable-source-tree recipe missing required module: $module"
        }
    }
    if (@($sourceTree2dRecipe[0].allowedTemplates) -notcontains "Headless") {
        Write-Error "engine/agent/manifest.json 2d-playable-source-tree recipe must allow the Headless template"
    }
    if (@($sourceTree2dRecipe[0].allowedPackagingTargets) -notcontains "source-tree-default") {
        Write-Error "engine/agent/manifest.json 2d-playable-source-tree recipe must allow source-tree-default"
    }
    if (@($sourceTree2dRecipe[0].validationRecipes) -notcontains "default") {
        Write-Error "engine/agent/manifest.json 2d-playable-source-tree recipe must include default validation"
    }
    foreach ($claim in @("visible desktop package proof", "texture atlas cook", "tilemap editor UX", "runtime image decoding", "production sprite batching", "native GPU output", "public native or RHI handle access")) {
        if (-not ((@($sourceTree2dRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json 2d-playable-source-tree recipe must keep unsupported claim explicit: $claim"
        }
    }
}
if (@($productionLoop.recipes | Where-Object { $_.id -eq "future-2d-playable-vertical-slice" }).Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must not keep stale future-2d-playable-vertical-slice after the source-tree 2D recipe is implemented"
}
$desktop2dRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "2d-desktop-runtime-package" })
if ($desktop2dRecipe.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose exactly one 2d-desktop-runtime-package recipe"
} else {
    foreach ($module in @("MK_core", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_scene", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($desktop2dRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json 2d-desktop-runtime-package recipe missing required module: $module"
        }
    }
    if (@($desktop2dRecipe[0].allowedTemplates) -notcontains "DesktopRuntime2DPackage") {
        Write-Error "engine/agent/manifest.json 2d-desktop-runtime-package recipe must allow DesktopRuntime2DPackage"
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($desktop2dRecipe[0].allowedPackagingTargets) -notcontains $target) {
            Write-Error "engine/agent/manifest.json 2d-desktop-runtime-package recipe must allow $target"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-2d-package-proof", "desktop-runtime-2d-vulkan-window-package", "shader-toolchain")) {
        if (@($desktop2dRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine/agent/manifest.json 2d-desktop-runtime-package recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("texture atlas cook", "tilemap editor UX", "runtime image decoding", "production sprite batching", "package streaming", "3D playable vertical slice", "editor productization", "public native or RHI handle access", "Metal readiness", "general production renderer quality")) {
        if (-not ((@($desktop2dRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json 2d-desktop-runtime-package recipe must keep unsupported claim explicit: $claim"
        }
    }
}
$desktop3dRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "3d-playable-desktop-package" })
if ($desktop3dRecipe.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose exactly one 3d-playable-desktop-package recipe"
} else {
    foreach ($module in @("MK_animation", "MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($desktop3dRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json 3d-playable-desktop-package recipe missing required module: $module"
        }
    }
    if (@($desktop3dRecipe[0].importerAssumptions.sourceFormats) -notcontains "GameEngine.AnimationFloatClipSource.v1") {
        Write-Error "engine/agent/manifest.json 3d-playable-desktop-package recipe must include animation float clip source format"
    }
    if (@($desktop3dRecipe[0].importerAssumptions.sourceFormats) -notcontains "GameEngine.MorphMeshCpuSource.v1") {
        Write-Error "engine/agent/manifest.json 3d-playable-desktop-package recipe must include morph mesh CPU source format"
    }
    Assert-ContainsText ([string]$desktop3dRecipe[0].cookedRuntimeAssumptions) "sample_and_apply_runtime_scene_render_animation_float_clip" "3d-playable-desktop-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].cookedRuntimeAssumptions) "runtime_morph_mesh_cpu_payload" "3d-playable-desktop-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].cookedRuntimeAssumptions) "sample_runtime_morph_mesh_cpu_animation_float_clip" "3d-playable-desktop-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].summary) "--require-shadow-morph-composition" "3d-playable-desktop-package summary"
    Assert-ContainsText ([string]$desktop3dRecipe[0].cookedRuntimeAssumptions) "--require-shadow-morph-composition" "3d-playable-desktop-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].cookedRuntimeAssumptions) "renderer_morph_descriptor_binds" "3d-playable-desktop-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].rendererBackendAssumptions.d3d12) "selected generated graphics morph + directional shadow receiver" "3d-playable-desktop-package d3d12 backend assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].rendererBackendAssumptions.vulkan) "no Vulkan shadow-morph validation recipe is ready" "3d-playable-desktop-package vulkan backend assumptions"
    if (@($desktop3dRecipe[0].allowedTemplates) -notcontains "DesktopRuntime3DPackage") {
        Write-Error "engine/agent/manifest.json 3d-playable-desktop-package recipe must allow DesktopRuntime3DPackage"
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($desktop3dRecipe[0].allowedPackagingTargets) -notcontains $target) {
            Write-Error "engine/agent/manifest.json 3d-playable-desktop-package recipe must allow $target"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-scene-gpu-package", "desktop-runtime-sample-game-vulkan-scene-gpu-package", "installed-d3d12-3d-shadow-morph-composition-smoke", "shader-toolchain")) {
        if (@($desktop3dRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine/agent/manifest.json 3d-playable-desktop-package recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("runtime source asset parsing", "material graph", "shader graph", "skeletal animation production path", "GPU skinning", "package streaming", "broad shadow+morph composition beyond the selected receiver smoke", "compute morph + shadow composition", "morph-deformed shadow-caster silhouettes", "Metal ready", "public native or RHI handle access", "general production renderer quality")) {
        if (-not ((@($desktop3dRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json 3d-playable-desktop-package recipe must keep unsupported claim explicit: $claim"
        }
    }
    if (((@($desktop3dRecipe[0].unsupportedClaims) -join " ").Contains("native GPU HUD or sprite overlay output"))) {
        Write-Error "engine/agent/manifest.json 3d-playable-desktop-package recipe must not keep stale native GPU HUD or sprite overlay unsupported claim after the focused overlay recipe is added"
    }
}
$nativeOverlayRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "native-gpu-runtime-ui-overlay" })
if ($nativeOverlayRecipe.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose exactly one native-gpu-runtime-ui-overlay recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($nativeOverlayRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json native-gpu-runtime-ui-overlay recipe missing required module: $module"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-native-ui-overlay-package", "desktop-runtime-sample-game-vulkan-native-ui-overlay-package", "shader-toolchain")) {
        if (@($nativeOverlayRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine/agent/manifest.json native-gpu-runtime-ui-overlay recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("production text shaping", "font rasterization", "glyph atlas", "image decoding", "real texture atlas", "IME", "OS accessibility bridge", "Metal ready", "public native or RHI handle access", "general production renderer quality")) {
        if (-not ((@($nativeOverlayRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json native-gpu-runtime-ui-overlay recipe must keep unsupported claim explicit: $claim"
        }
    }
}
$texturedUiRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "native-ui-textured-sprite-atlas" })
if ($texturedUiRecipe.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose exactly one native-ui-textured-sprite-atlas recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($texturedUiRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json native-ui-textured-sprite-atlas recipe missing required module: $module"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-textured-ui-atlas-package", "desktop-runtime-sample-game-vulkan-textured-ui-atlas-package", "shader-toolchain")) {
        if (@($texturedUiRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine/agent/manifest.json native-ui-textured-sprite-atlas recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("source image decoding", "production atlas packing", "production text shaping", "font rasterization", "glyph atlas", "IME", "OS accessibility bridge", "Metal ready", "public native or RHI handle access", "general production UI renderer quality")) {
        if (-not ((@($texturedUiRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json native-ui-textured-sprite-atlas recipe must keep unsupported claim explicit: $claim"
        }
    }
}
$textGlyphUiRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "native-ui-text-glyph-atlas" })
if ($textGlyphUiRecipe.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose exactly one native-ui-text-glyph-atlas recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($textGlyphUiRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json native-ui-text-glyph-atlas recipe missing required module: $module"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "installed-d3d12-3d-native-ui-text-glyph-atlas-smoke", "shader-toolchain")) {
        if (@($textGlyphUiRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine/agent/manifest.json native-ui-text-glyph-atlas recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("production text shaping", "font rasterization", "glyph atlas generation", "runtime source image decoding", "source image atlas packing", "IME", "OS accessibility bridge", "Metal ready", "public native or RHI handle access", "general production UI renderer quality")) {
        if (-not ((@($textGlyphUiRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json native-ui-text-glyph-atlas recipe must keep unsupported claim explicit: $claim"
        }
    }
}
$uiAtlasMetadataRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "native-ui-atlas-package-metadata" })
if ($uiAtlasMetadataRecipe.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose exactly one native-ui-atlas-package-metadata recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($uiAtlasMetadataRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json native-ui-atlas-package-metadata recipe missing required module: $module"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-ui-atlas-metadata-package", "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package", "shader-toolchain")) {
        if (@($uiAtlasMetadataRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine/agent/manifest.json native-ui-atlas-package-metadata recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("runtime source PNG/JPEG image decoding", "production atlas packing", "production text shaping", "font rasterization", "glyph atlas", "IME", "OS accessibility bridge", "Metal ready", "public native or RHI handle access", "general production UI renderer quality")) {
        if (-not ((@($uiAtlasMetadataRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json native-ui-atlas-package-metadata recipe must keep unsupported claim explicit: $claim"
        }
    }
}

$expectedCommandSurfaceIds = @(
    "create-game-recipe",
    "create-scene",
    "update-scene-package",
    "migrate-scene-v2-runtime-package",
    "validate-runtime-scene-package",
    "add-scene-node",
    "add-or-update-component",
    "create-prefab",
    "instantiate-prefab",
    "create-material-instance",
    "create-material-from-graph",
    "register-source-asset",
    "cook-registered-source-assets",
    "cook-runtime-package",
    "register-runtime-package-files",
    "update-ui-atlas-metadata-package",
    "update-game-agent-manifest",
    "run-validation-recipe"
)
$knownAuthoringSurfaceIds = @{}
foreach ($authoringSurface in $productionLoop.authoringSurfaces) {
    $knownAuthoringSurfaceIds[$authoringSurface.id] = $true
}
$knownPackageSurfaceIds = @{}
foreach ($packageSurface in $productionLoop.packageSurfaces) {
    $knownPackageSurfaceIds[$packageSurface.id] = $true
}
$knownUnsupportedGapIds = @{}
foreach ($gap in $productionLoop.unsupportedProductionGaps) {
    $knownUnsupportedGapIds[$gap.id] = $true
}
$knownHostGateIds = @{}
foreach ($hostGate in $productionLoop.hostGates) {
    $knownHostGateIds[$hostGate.id] = $true
}
$commandSurfaceIds = @{}
foreach ($commandSurface in $productionLoop.commandSurfaces) {
    Assert-JsonProperty $commandSurface @("id", "schemaVersion", "status", "owner", "summary", "requestModes", "requestShape", "resultShape", "requiredModules", "capabilityGates", "hostGates", "validationRecipes", "unsupportedGapIds", "undoToken", "notes") "engine/agent/manifest.json aiOperableProductionLoop command surface"
    Assert-ManifestCommandSurfaceHasNoLegacyTopLevelFields -CommandSurface $commandSurface -MessagePrefix "engine/agent/manifest.json aiOperableProductionLoop command surface"
    if ($commandSurface.schemaVersion -ne 1) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' schemaVersion must be 1"
    }
    if ($commandSurfaceIds.ContainsKey($commandSurface.id)) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface id is duplicated: $($commandSurface.id)"
    }
    $commandSurfaceIds[$commandSurface.id] = $true
    $modeIds = @{}
    foreach ($mode in @($commandSurface.requestModes)) {
        Assert-JsonProperty $mode @("id", "status", "mutates", "requiresDryRun", "notes") "engine/agent/manifest.json aiOperableProductionLoop command surface requestModes"
        $modeIds[$mode.id] = $mode
        if (@("dry-run", "apply", "execute") -notcontains $mode.id) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' has unknown request mode: $($mode.id)"
        }
        if ($allowedProductionStatuses -notcontains $mode.status) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' mode '$($mode.id)' has invalid status: $($mode.status)"
        }
    }
    if (-not $modeIds.ContainsKey("dry-run")) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' must expose a dry-run request mode"
    }
    if ($modeIds.ContainsKey("apply") -and $modeIds["apply"].status -eq "ready" -and $modeIds["dry-run"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make apply ready before dry-run is ready"
    }
    if ($modeIds.ContainsKey("execute") -and $modeIds["execute"].status -eq "ready" -and $modeIds["dry-run"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make execute ready before dry-run is ready"
    }
    if ($modeIds.ContainsKey("apply") -and $modeIds["apply"].status -eq "ready" -and
        @("register-runtime-package-files", "update-ui-atlas-metadata-package", "create-material-instance", "create-material-from-graph", "update-scene-package", "migrate-scene-v2-runtime-package", "create-scene", "add-scene-node", "add-or-update-component", "create-prefab", "instantiate-prefab", "register-source-asset", "cook-registered-source-assets") -notcontains $commandSurface.id) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make apply ready without a focused apply tooling slice"
    }
    if ($modeIds.ContainsKey("execute") -and $modeIds["execute"].status -eq "ready" -and
        @("run-validation-recipe", "validate-runtime-scene-package") -notcontains $commandSurface.id) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make execute ready without a focused execution tooling slice"
    }
    Assert-JsonProperty $commandSurface.requestShape @("schema", "requiredFields", "optionalFields", "pathPolicy", "nativeHandlePolicy") "engine/agent/manifest.json aiOperableProductionLoop command surface requestShape"
    if ($commandSurface.requestShape.nativeHandlePolicy -ne "forbidden") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' requestShape must forbid native handles"
    }
    Assert-JsonProperty $commandSurface.resultShape @("schema", "requiredFields", "diagnosticFields", "dryRunFields") "engine/agent/manifest.json aiOperableProductionLoop command surface resultShape"
    if (@("run-validation-recipe", "validate-runtime-scene-package") -contains $commandSurface.id) {
        Assert-JsonProperty $commandSurface.resultShape @("executeFields") "engine/agent/manifest.json aiOperableProductionLoop run-validation-recipe resultShape"
    } else {
        Assert-JsonProperty $commandSurface.resultShape @("applyFields") "engine/agent/manifest.json aiOperableProductionLoop command surface resultShape"
    }
    foreach ($requiredResultField in @("commandId", "mode", "status", "diagnostics", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($commandSurface.resultShape.requiredFields) -notcontains $requiredResultField) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' resultShape missing required result field: $requiredResultField"
        }
    }
    foreach ($diagnosticField in @("severity", "code", "message", "unsupportedGapId", "validationRecipe")) {
        if (@($commandSurface.resultShape.diagnosticFields) -notcontains $diagnosticField) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' resultShape missing diagnostic field: $diagnosticField"
        }
    }
    foreach ($module in @($commandSurface.requiredModules)) {
        if (-not $moduleNames.ContainsKey($module)) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown module: $module"
        }
    }
    foreach ($gate in @($commandSurface.capabilityGates)) {
        Assert-JsonProperty $gate @("id", "source", "requiredStatus", "notes") "engine/agent/manifest.json aiOperableProductionLoop command surface capabilityGates"
        switch ($gate.source) {
            "module" { if (-not $moduleNames.ContainsKey($gate.id)) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown module capability gate: $($gate.id)" } }
            "recipe" { if (-not $productionRecipeIds.ContainsKey($gate.id)) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown recipe capability gate: $($gate.id)" } }
            "authoring-surface" { if (-not $knownAuthoringSurfaceIds.ContainsKey($gate.id)) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown authoring surface capability gate: $($gate.id)" } }
            "package-surface" { if (-not $knownPackageSurfaceIds.ContainsKey($gate.id)) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown package surface capability gate: $($gate.id)" } }
            "unsupported-gap" { if (-not $knownUnsupportedGapIds.ContainsKey($gate.id)) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown unsupported gap capability gate: $($gate.id)" } }
            "host-gate" { if (-not $knownHostGateIds.ContainsKey($gate.id)) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown host gate capability gate: $($gate.id)" } }
            "validation-recipe" { if (-not $validationRecipeNames.ContainsKey($gate.id)) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown validation recipe capability gate: $($gate.id)" } }
            default { Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' has unknown capability gate source: $($gate.source)" }
        }
    }
    foreach ($hostGate in @($commandSurface.hostGates)) {
        if (-not $knownHostGateIds.ContainsKey($hostGate)) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown host gate: $hostGate"
        }
    }
    foreach ($validationRecipe in @($commandSurface.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown validation recipe: $validationRecipe"
        }
    }
    foreach ($gapId in @($commandSurface.unsupportedGapIds)) {
        if (-not $knownUnsupportedGapIds.ContainsKey($gapId)) {
            Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown unsupported gap: $gapId"
        }
    }
    if (@($commandSurface.unsupportedGapIds).Count -lt 1) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' must list unsupportedGapIds for diagnostics"
    }
    Assert-JsonProperty $commandSurface.undoToken @("status", "notes") "engine/agent/manifest.json aiOperableProductionLoop command surface undoToken"
    if ($commandSurface.undoToken.status -ne "placeholder-only") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' undoToken must remain placeholder-only in this slice"
    }
    if ($modeIds.ContainsKey("apply") -and $modeIds["apply"].status -eq "ready" -and @($commandSurface.validationRecipes).Count -lt 1) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' must list validation recipes when apply is ready"
    }
}
$runtimePackageCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "register-runtime-package-files" })
if ($runtimePackageCommand.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose exactly one register-runtime-package-files command surface"
} else {
    $runtimeModes = @{}
    foreach ($mode in @($runtimePackageCommand[0].requestModes)) {
        $runtimeModes[$mode.id] = $mode
    }
    if (-not $runtimeModes.ContainsKey("dry-run") -or $runtimeModes["dry-run"].status -ne "ready" -or
        -not $runtimeModes.ContainsKey("apply") -or $runtimeModes["apply"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop register-runtime-package-files must keep dry-run and apply ready"
    }
    if (-not ([string]$runtimeModes["dry-run"].notes).Contains("-DryRun")) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop register-runtime-package-files dry-run notes must reference the actual -DryRun switch"
    }
}
$uiAtlasPackageCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "update-ui-atlas-metadata-package" })
if ($uiAtlasPackageCommand.Count -ne 1 -or $uiAtlasPackageCommand[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready update-ui-atlas-metadata-package command surface"
} else {
    $uiAtlasModes = @{}
    foreach ($mode in @($uiAtlasPackageCommand[0].requestModes)) {
        $uiAtlasModes[$mode.id] = $mode
    }
    if (-not $uiAtlasModes.ContainsKey("dry-run") -or $uiAtlasModes["dry-run"].status -ne "ready" -or
        -not $uiAtlasModes.ContainsKey("apply") -or $uiAtlasModes["apply"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json update-ui-atlas-metadata-package must keep dry-run and apply ready"
    }
    $uiAtlasNotes = [string]$uiAtlasPackageCommand[0].notes
    if (-not $uiAtlasNotes.Contains("plan_cooked_ui_atlas_package_update") -or
        -not $uiAtlasNotes.Contains("apply_cooked_ui_atlas_package_update") -or
        -not $uiAtlasNotes.Contains("author_packed_ui_atlas_from_decoded_images") -or
        -not $uiAtlasNotes.Contains("GameEngine.CookedTexture.v1") -or
        -not $uiAtlasNotes.Contains("renderer texture upload")) {
        Write-Error "engine/agent/manifest.json update-ui-atlas-metadata-package notes must keep cooked helper names, decoded atlas bridge, and renderer-upload limits explicit"
    }
}
$materialInstanceCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "create-material-instance" })
if ($materialInstanceCommand.Count -ne 1 -or $materialInstanceCommand[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready create-material-instance command surface"
} else {
    $materialModes = @{}
    foreach ($mode in @($materialInstanceCommand[0].requestModes)) {
        $materialModes[$mode.id] = $mode
    }
    if (-not $materialModes.ContainsKey("dry-run") -or $materialModes["dry-run"].status -ne "ready" -or
        -not $materialModes.ContainsKey("apply") -or $materialModes["apply"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json create-material-instance must keep dry-run and apply ready"
    }
    $materialNotes = [string]$materialInstanceCommand[0].notes
    if (-not $materialNotes.Contains("plan_material_instance_package_update") -or
        -not $materialNotes.Contains("apply_material_instance_package_update") -or
        -not $materialNotes.Contains("material graph") -or
        -not $materialNotes.Contains("shader graph") -or
        -not $materialNotes.Contains("live shader generation")) {
        Write-Error "engine/agent/manifest.json create-material-instance notes must keep helper names and unsupported graph/shader limits explicit"
    }
}
$materialGraphCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "create-material-from-graph" })
if ($materialGraphCommand.Count -ne 1 -or $materialGraphCommand[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready create-material-from-graph command surface"
} else {
    $materialGraphModes = @{}
    foreach ($mode in @($materialGraphCommand[0].requestModes)) {
        $materialGraphModes[$mode.id] = $mode
    }
    if (-not $materialGraphModes.ContainsKey("dry-run") -or $materialGraphModes["dry-run"].status -ne "ready" -or
        -not $materialGraphModes.ContainsKey("apply") -or $materialGraphModes["apply"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json create-material-from-graph must keep dry-run and apply ready"
    }
    $materialGraphNotes = [string]$materialGraphCommand[0].notes
    foreach ($needle in @(
            "plan_material_graph_package_update",
            "apply_material_graph_package_update",
            "GameEngine.MaterialGraph.v1",
            "GameEngine.Material.v1",
            "material_texture",
            "shader graph",
            "shader compiler execution",
            "live shader generation",
            "renderer/RHI residency",
            "package streaming"
        )) {
        if (-not $materialGraphNotes.Contains($needle)) {
            Write-Error "engine/agent/manifest.json create-material-from-graph notes must keep helper names and unsupported graph/package limits explicit: $needle"
        }
    }
}
$scenePackageCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "update-scene-package" })
if ($scenePackageCommand.Count -ne 1 -or $scenePackageCommand[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready update-scene-package command surface"
} else {
    $sceneModes = @{}
    foreach ($mode in @($scenePackageCommand[0].requestModes)) {
        $sceneModes[$mode.id] = $mode
    }
    if (-not $sceneModes.ContainsKey("dry-run") -or $sceneModes["dry-run"].status -ne "ready" -or
        -not $sceneModes.ContainsKey("apply") -or $sceneModes["apply"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json update-scene-package must keep dry-run and apply ready"
    }
    $sceneNotes = [string]$scenePackageCommand[0].notes
    foreach ($needle in @(
            "plan_scene_package_update",
            "apply_scene_package_update",
            "scene_mesh",
            "scene_material",
            "scene_sprite",
            "editor productization",
            "prefab mutation",
            "runtime source import",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation"
        )) {
        if (-not $sceneNotes.Contains($needle)) {
            Write-Error "engine/agent/manifest.json update-scene-package notes must keep helper names and unsupported scene/package limits explicit: $needle"
        }
    }
}
$scenePrefabAuthoringCommandIds = @(
    "create-scene",
    "add-scene-node",
    "add-or-update-component",
    "create-prefab",
    "instantiate-prefab"
)
foreach ($commandId in $scenePrefabAuthoringCommandIds) {
    $scenePrefabCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq $commandId })
    if ($scenePrefabCommand.Count -ne 1 -or $scenePrefabCommand[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready Scene/Prefab v2 authoring command surface: $commandId"
    } else {
        $scenePrefabModes = @{}
        foreach ($mode in @($scenePrefabCommand[0].requestModes)) {
            $scenePrefabModes[$mode.id] = $mode
        }
        if (-not $scenePrefabModes.ContainsKey("dry-run") -or $scenePrefabModes["dry-run"].status -ne "ready" -or
            -not $scenePrefabModes.ContainsKey("apply") -or $scenePrefabModes["apply"].status -ne "ready") {
            Write-Error "engine/agent/manifest.json Scene/Prefab v2 authoring command '$commandId' must keep dry-run and apply ready"
        }
        foreach ($module in @("MK_scene", "MK_tools")) {
            if (@($scenePrefabCommand[0].requiredModules) -notcontains $module) {
                Write-Error "engine/agent/manifest.json Scene/Prefab v2 authoring command '$commandId' missing required module: $module"
            }
        }
        foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
            if (@($scenePrefabCommand[0].resultShape.dryRunFields) -notcontains $field) {
                Write-Error "engine/agent/manifest.json Scene/Prefab v2 authoring command '$commandId' dryRunFields missing: $field"
            }
        }
        foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
            if (@($scenePrefabCommand[0].resultShape.applyFields) -notcontains $field) {
                Write-Error "engine/agent/manifest.json Scene/Prefab v2 authoring command '$commandId' applyFields missing: $field"
            }
        }
        $scenePrefabPolicyText = "$($scenePrefabCommand[0].summary) $($scenePrefabCommand[0].requestShape.pathPolicy) $($scenePrefabCommand[0].notes)"
        foreach ($needle in @(
                "GameEngine.Scene.v2",
                "GameEngine.Prefab.v2",
                "plan_scene_prefab_authoring",
                "apply_scene_prefab_authoring",
                "safe repository-relative",
                "does not evaluate arbitrary shell",
                "free-form edits are not supported",
                "Scene v2 runtime package migration",
                "editor productization",
                "nested prefab merge/resolution UX"
            )) {
            if (-not $scenePrefabPolicyText.Contains($needle)) {
                Write-Error "engine/agent/manifest.json Scene/Prefab v2 authoring command '$commandId' must document reviewed helper/policy text: $needle"
            }
        }
    }
}
$sourceAssetCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "register-source-asset" })
if ($sourceAssetCommand.Count -ne 1 -or $sourceAssetCommand[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready register-source-asset command surface"
} else {
    $sourceAssetModes = @{}
    foreach ($mode in @($sourceAssetCommand[0].requestModes)) {
        $sourceAssetModes[$mode.id] = $mode
    }
    if (-not $sourceAssetModes.ContainsKey("dry-run") -or $sourceAssetModes["dry-run"].status -ne "ready" -or
        -not $sourceAssetModes.ContainsKey("apply") -or $sourceAssetModes["apply"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json register-source-asset must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_assets", "MK_tools")) {
        if (@($sourceAssetCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json register-source-asset missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "importMetadata", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($sourceAssetCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json register-source-asset dryRunFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("cookedOutputHint", "packageIndexPath", "backend", "shaderArtifactRequirements", "renderer", "rhi", "metal", "descriptor", "pipeline", "nativeHandle")) {
        if (@($sourceAssetCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($sourceAssetCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine/agent/manifest.json register-source-asset must not expose package/renderer/native field: $forbiddenField"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($sourceAssetCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json register-source-asset applyFields missing: $field"
        }
    }
    $sourceAssetPolicyText = "$($sourceAssetCommand[0].summary) $($sourceAssetCommand[0].requestShape.pathPolicy) $($sourceAssetCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.AssetIdentity.v2",
            "GameEngine.SourceAssetRegistry.v1",
            "plan_source_asset_registration",
            "apply_source_asset_registration",
            "safe repository-relative",
            "does not evaluate arbitrary shell",
            "free-form edits are not supported",
            "external importer execution is not supported",
            "does not write cooked artifacts",
            "does not update .geindex",
            "package cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles"
        )) {
        if (-not $sourceAssetPolicyText.Contains($needle)) {
            Write-Error "engine/agent/manifest.json register-source-asset must document reviewed helper/policy text: $needle"
        }
    }
    $sourceAssetHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/source_asset_registration_tool.hpp"
    $sourceAssetSourcePath = Join-Path $root "engine/tools/asset/source_asset_registration_tool.cpp"
    foreach ($requiredPath in @($sourceAssetHeaderPath, $sourceAssetSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "register-source-asset reviewed helper file is missing: $requiredPath"
        }
    }
    foreach ($helperPath in @($sourceAssetHeaderPath, $sourceAssetSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/tools/asset_import_tool.hpp",
                "mirakana/tools/asset_package_tool.hpp",
                "mirakana/assets/asset_package.hpp",
                "mirakana/renderer/",
                "mirakana/rhi/",
                "execute_asset_import_plan",
                "assemble_asset_cooked_package",
                "write_asset_cooked_package_index"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use package/import execution or renderer/RHI surface: $forbiddenText"
                }
            }
        }
    }
}
$registeredCookCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "cook-registered-source-assets" })
if ($registeredCookCommand.Count -ne 1 -or $registeredCookCommand[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready cook-registered-source-assets command surface"
} else {
    $registeredCookModes = @{}
    foreach ($mode in @($registeredCookCommand[0].requestModes)) {
        $registeredCookModes[$mode.id] = $mode
    }
    if (-not $registeredCookModes.ContainsKey("dry-run") -or $registeredCookModes["dry-run"].status -ne "ready" -or
        -not $registeredCookModes.ContainsKey("apply") -or $registeredCookModes["apply"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json cook-registered-source-assets must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_assets", "MK_tools")) {
        if (@($registeredCookCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json cook-registered-source-assets missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($registeredCookCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json cook-registered-source-assets dryRunFields missing: $field"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($registeredCookCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json cook-registered-source-assets applyFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice")) {
        if (@($registeredCookCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($registeredCookCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine/agent/manifest.json cook-registered-source-assets must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $registeredCookPolicyText = "$($registeredCookCommand[0].summary) $($registeredCookCommand[0].requestShape.pathPolicy) $($registeredCookCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.SourceAssetRegistry.v1",
            "explicitly selected",
            "plan_registered_source_asset_cook_package",
            "apply_registered_source_asset_cook_package",
            "build_asset_import_plan",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package",
            "safe repository-relative",
            "package-relative",
            "external importer execution is not supported",
            "broad dependency cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $registeredCookPolicyText.Contains($needle)) {
            Write-Error "engine/agent/manifest.json cook-registered-source-assets must document reviewed helper/policy text: $needle"
        }
    }
    $registeredCookHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/registered_source_asset_cook_package_tool.hpp"
    $registeredCookSourcePath = Join-Path $root "engine/tools/asset/registered_source_asset_cook_package_tool.cpp"
    foreach ($requiredPath in @($registeredCookHeaderPath, $registeredCookSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "cook-registered-source-assets reviewed helper file is missing: $requiredPath"
        }
    }
    if (Test-Path -LiteralPath $registeredCookSourcePath -PathType Leaf) {
        $helperText = Get-Content -LiteralPath $registeredCookSourcePath -Raw
        foreach ($requiredText in @(
            "mirakana/tools/asset_import_tool.hpp",
            "mirakana/tools/asset_package_tool.hpp",
            "build_asset_import_plan",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package"
        )) {
            if (-not $helperText.Contains($requiredText)) {
                Write-Error "$registeredCookSourcePath must reuse selected source asset import/package helper: $requiredText"
            }
        }
    }
    foreach ($helperPath in @($registeredCookHeaderPath, $registeredCookSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/renderer/",
                "mirakana/rhi/",
                "IRhiDevice",
                "ID3D12",
                "VkDevice",
                "MTLDevice"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use renderer/RHI/native surfaces: $forbiddenText"
                }
            }
        }
    }
}
$sceneMigrationCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "migrate-scene-v2-runtime-package" })
if ($sceneMigrationCommand.Count -ne 1 -or $sceneMigrationCommand[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready migrate-scene-v2-runtime-package command surface"
} else {
    $sceneMigrationModes = @{}
    foreach ($mode in @($sceneMigrationCommand[0].requestModes)) {
        $sceneMigrationModes[$mode.id] = $mode
    }
    if (-not $sceneMigrationModes.ContainsKey("dry-run") -or $sceneMigrationModes["dry-run"].status -ne "ready" -or
        -not $sceneMigrationModes.ContainsKey("apply") -or $sceneMigrationModes["apply"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json migrate-scene-v2-runtime-package must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_scene", "MK_assets", "MK_tools")) {
        if (@($sceneMigrationCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json migrate-scene-v2-runtime-package missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($sceneMigrationCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json migrate-scene-v2-runtime-package dryRunFields missing: $field"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($sceneMigrationCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json migrate-scene-v2-runtime-package applyFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice")) {
        if (@($sceneMigrationCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($sceneMigrationCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine/agent/manifest.json migrate-scene-v2-runtime-package must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $sceneMigrationPolicyText = "$($sceneMigrationCommand[0].summary) $($sceneMigrationCommand[0].requestShape.pathPolicy) $($sceneMigrationCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.Scene.v2",
            "GameEngine.SourceAssetRegistry.v1",
            "GameEngine.Scene.v1",
            "plan_scene_v2_runtime_package_migration",
            "apply_scene_v2_runtime_package_migration",
            "plan_scene_package_update",
            "apply_scene_package_update",
            "safe repository-relative",
            "package-relative",
            "external importer execution is not supported",
            "package cooking",
            "dependent asset cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $sceneMigrationPolicyText.Contains($needle)) {
            Write-Error "engine/agent/manifest.json migrate-scene-v2-runtime-package must document reviewed helper/policy text: $needle"
        }
    }
    $sceneMigrationHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/scene_v2_runtime_package_migration_tool.hpp"
    $sceneMigrationSourcePath = Join-Path $root "engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp"
    foreach ($requiredPath in @($sceneMigrationHeaderPath, $sceneMigrationSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "migrate-scene-v2-runtime-package reviewed helper file is missing: $requiredPath"
        }
    }
    foreach ($helperPath in @($sceneMigrationHeaderPath, $sceneMigrationSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/tools/asset_import_tool.hpp",
                "mirakana/tools/asset_package_tool.hpp",
                "mirakana/renderer/",
                "mirakana/rhi/",
                "execute_asset_import_plan",
                "assemble_asset_cooked_package",
                "write_asset_cooked_package_index",
                "IRhiDevice",
                "ID3D12",
                "VkDevice",
                "MTLDevice"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use importer/package execution or renderer/RHI/native surfaces: $forbiddenText"
                }
            }
        }
    }
    if (Test-Path -LiteralPath $sceneMigrationSourcePath -PathType Leaf) {
        $sceneMigrationSourceText = Get-Content -LiteralPath $sceneMigrationSourcePath -Raw
        foreach ($requiredCall in @(
                "plan_scene_package_update(",
                "apply_scene_package_update("
            )) {
            if (-not $sceneMigrationSourceText.Contains($requiredCall)) {
                Write-Error "migrate-scene-v2-runtime-package source must reuse existing scene package helper call: $requiredCall"
            }
        }
    }
}
$runtimeSceneValidationCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "validate-runtime-scene-package" })
if ($runtimeSceneValidationCommand.Count -ne 1 -or $runtimeSceneValidationCommand[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready validate-runtime-scene-package command surface"
} else {
    $runtimeSceneValidationModes = @{}
    foreach ($mode in @($runtimeSceneValidationCommand[0].requestModes)) {
        $runtimeSceneValidationModes[$mode.id] = $mode
    }
    if (-not $runtimeSceneValidationModes.ContainsKey("dry-run") -or
        $runtimeSceneValidationModes["dry-run"].status -ne "ready" -or
        -not $runtimeSceneValidationModes.ContainsKey("execute") -or
        $runtimeSceneValidationModes["execute"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json validate-runtime-scene-package must keep dry-run and execute ready"
    }
    if ($runtimeSceneValidationModes.ContainsKey("apply") -and $runtimeSceneValidationModes["apply"].status -eq "ready") {
        Write-Error "engine/agent/manifest.json validate-runtime-scene-package must remain non-mutating and must not expose ready apply"
    }
    foreach ($module in @("MK_runtime", "MK_runtime_scene", "MK_tools")) {
        if (@($runtimeSceneValidationCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json validate-runtime-scene-package missing required module: $module"
        }
    }
    foreach ($field in @("packageIndexPath", "sceneAssetKey")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.requiredFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json validate-runtime-scene-package requestShape requiredFields missing: $field"
        }
    }
    foreach ($field in @("contentRoot", "validateAssetReferences", "requireUniqueNodeNames")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.optionalFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json validate-runtime-scene-package requestShape optionalFields missing: $field"
        }
    }
    foreach ($field in @("packageSummary", "sceneAsset", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($runtimeSceneValidationCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json validate-runtime-scene-package dryRunFields missing: $field"
        }
    }
    foreach ($field in @("packageSummary", "sceneSummary", "references", "packageRecordCount", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($runtimeSceneValidationCommand[0].resultShape.executeFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json validate-runtime-scene-package executeFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice", "shaderArtifactRequirements")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($runtimeSceneValidationCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine/agent/manifest.json validate-runtime-scene-package must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $runtimeSceneValidationPolicyText = "$($runtimeSceneValidationCommand[0].summary) $($runtimeSceneValidationCommand[0].requestShape.pathPolicy) $($runtimeSceneValidationCommand[0].notes)"
    foreach ($needle in @(
            "plan_runtime_scene_package_validation",
            "execute_runtime_scene_package_validation",
            "mirakana::runtime::load_runtime_asset_package",
            "mirakana::runtime_scene::instantiate_runtime_scene",
            "safe package-relative",
            "does not mutate",
            "runtime source parsing is not supported",
            "package cooking is not supported",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $runtimeSceneValidationPolicyText.Contains($needle)) {
            Write-Error "engine/agent/manifest.json validate-runtime-scene-package must document reviewed helper/policy text: $needle"
        }
    }
    $runtimeSceneValidationHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/runtime_scene_package_validation_tool.hpp"
    $runtimeSceneValidationSourcePath = Join-Path $root "engine/tools/scene/runtime_scene_package_validation_tool.cpp"
    foreach ($requiredPath in @($runtimeSceneValidationHeaderPath, $runtimeSceneValidationSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "validate-runtime-scene-package reviewed helper file is missing: $requiredPath"
        }
    }
    if (Test-Path -LiteralPath $runtimeSceneValidationSourcePath -PathType Leaf) {
        $helperText = Get-Content -LiteralPath $runtimeSceneValidationSourcePath -Raw
        foreach ($requiredText in @(
            "mirakana/runtime/asset_runtime.hpp",
            "mirakana/runtime_scene/runtime_scene.hpp",
            "load_runtime_asset_package",
            "instantiate_runtime_scene"
        )) {
            if (-not $helperText.Contains($requiredText)) {
                Write-Error "$runtimeSceneValidationSourcePath must reuse runtime package/scene validation helper: $requiredText"
            }
        }
        foreach ($forbiddenText in @(
            "mirakana/tools/asset_import_tool.hpp",
            "mirakana/tools/asset_package_tool.hpp",
            "mirakana/renderer/",
            "mirakana/rhi/",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package",
            "write_text(",
            "IRhiDevice",
            "ID3D12",
            "VkDevice",
            "MTLDevice"
        )) {
            if ($helperText.Contains($forbiddenText)) {
                Write-Error "$runtimeSceneValidationSourcePath must not mutate, import, package, or use renderer/RHI/native surfaces: $forbiddenText"
            }
        }
    }
}
$validationRunnerCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "run-validation-recipe" })
if ($validationRunnerCommand.Count -ne 1 -or $validationRunnerCommand[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready run-validation-recipe command surface"
} else {
    $runnerModes = @{}
    foreach ($mode in @($validationRunnerCommand[0].requestModes)) {
        $runnerModes[$mode.id] = $mode
    }
    if (-not $runnerModes.ContainsKey("dry-run") -or $runnerModes["dry-run"].status -ne "ready" -or
        -not $runnerModes.ContainsKey("execute") -or $runnerModes["execute"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json run-validation-recipe must keep dry-run and execute ready"
    }
    foreach ($requiredField in @("mode", "validationRecipe")) {
        if (@($validationRunnerCommand[0].requestShape.requiredFields) -notcontains $requiredField) {
            Write-Error "engine/agent/manifest.json run-validation-recipe requestShape missing required field: $requiredField"
        }
    }
    foreach ($optionalField in @("gameTarget", "strictBackend", "hostGateAcknowledgements", "timeoutSeconds")) {
        if (@($validationRunnerCommand[0].requestShape.optionalFields) -notcontains $optionalField) {
            Write-Error "engine/agent/manifest.json run-validation-recipe requestShape missing optional field: $optionalField"
        }
    }
    $runnerPolicyText = "$($validationRunnerCommand[0].requestShape.pathPolicy) $($validationRunnerCommand[0].notes)"
    foreach ($needle in @(
            "tools/run-validation-recipe.ps1",
            "Get-ValidationRecipeCommandPlan",
            "Invoke-ValidationRecipeCommandPlan",
            "does not evaluate raw manifest command strings",
            "free-form arguments are rejected",
            "not arbitrary shell"
        )) {
        if (-not $runnerPolicyText.Contains($needle)) {
            Write-Error "engine/agent/manifest.json run-validation-recipe must document runner path/helper/policy text: $needle"
        }
    }
    foreach ($field in @("recipe", "status", "command", "argv", "hostGates", "diagnostics", "blockedBy")) {
        if (@($validationRunnerCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json run-validation-recipe dryRunFields missing: $field"
        }
    }
    foreach ($field in @("recipe", "status", "exitCode", "durationSeconds", "stdoutSummary", "stderrSummary", "hostGates", "diagnostics")) {
        if (@($validationRunnerCommand[0].resultShape.executeFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json run-validation-recipe executeFields missing: $field"
        }
    }
    foreach ($recipe in @(
            "agent-contract",
            "default",
            "public-api-boundary",
            "shader-toolchain",
            "desktop-game-runtime",
            "desktop-runtime-sample-game-scene-gpu-package",
            "desktop-runtime-generated-material-shader-scaffold-package",
            "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict",
            "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package",
            "dev-windows-editor-game-module-driver-load-tests"
        )) {
        if (@($validationRunnerCommand[0].validationRecipes) -notcontains $recipe) {
            Write-Error "engine/agent/manifest.json run-validation-recipe validationRecipes missing allowlisted recipe: $recipe"
        }
    }
    if (@($validationRunnerCommand[0].validationRecipes).Count -ne 10) {
        Write-Error "engine/agent/manifest.json run-validation-recipe validationRecipes must be exactly the reviewed allowlist"
    }
    if (@($validationRunnerCommand[0].requestModes | Where-Object { $_.id -eq "apply" -and $_.status -eq "ready" }).Count -gt 0) {
        Write-Error "engine/agent/manifest.json run-validation-recipe must not expose a ready apply mode"
    }
    $runnerScript = Resolve-RequiredAgentPath "tools/run-validation-recipe.ps1"
    $runnerText = Get-Content -LiteralPath $runnerScript -Raw
    Assert-ContainsText $runnerText "validation-recipe-core.ps1" "tools/run-validation-recipe.ps1"
    Assert-ContainsText $runnerText "Get-ValidationRecipeCommandPlan" "tools/run-validation-recipe.ps1"
    Assert-ContainsText $runnerText "Invoke-ValidationRecipeCommandPlan" "tools/run-validation-recipe.ps1"
    $runnerCoreScript = Resolve-RequiredAgentPath "tools/validation-recipe-core.ps1"
    $runnerCoreText = Get-Content -LiteralPath $runnerCoreScript -Raw
    foreach ($needle in @("Test-ValidationRecipeRequest", "New-ValidationRecipeDryRunResult", "New-ValidationRecipeRejectedResult")) {
        Assert-ContainsText $runnerCoreText $needle "tools/validation-recipe-core.ps1"
    }
    foreach ($forbiddenRunnerPattern in @(
        "\bInvoke-Expression\b",
        "\biex\b",
        "\[scriptblock\]::Create",
        "\bcmd\s*/c\b",
        "\bbash\s+-c\b",
        "\bpwsh\b[^\r\n]*-Command\b",
        "\bpowershell\b[^\r\n]*-Command\b"
    )) {
        foreach ($runnerFile in @(
            @{ Label = "tools/run-validation-recipe.ps1"; Text = $runnerText },
            @{ Label = "tools/validation-recipe-core.ps1"; Text = $runnerCoreText }
        )) {
            if ([System.Text.RegularExpressions.Regex]::IsMatch($runnerFile.Text, $forbiddenRunnerPattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
                Write-Error "$($runnerFile.Label) must not contain shell-eval pattern: $forbiddenRunnerPattern"
            }
        }
    }
}
foreach ($commandId in $expectedCommandSurfaceIds) {
    if (-not $commandSurfaceIds.ContainsKey($commandId)) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing command surface id: $commandId"
    }
}

$authoringSurfaceIds = @{}
foreach ($authoringSurface in $productionLoop.authoringSurfaces) {
    Assert-JsonProperty $authoringSurface @("id", "status", "owner", "notes") "engine/agent/manifest.json aiOperableProductionLoop authoringSurfaces"
    if ($authoringSurfaceIds.ContainsKey($authoringSurface.id)) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface id is duplicated: $($authoringSurface.id)"
    }
    $authoringSurfaceIds[$authoringSurface.id] = $true
    if ($allowedProductionStatuses -notcontains $authoringSurface.status) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface '$($authoringSurface.id)' has invalid status: $($authoringSurface.status)"
    }
}
$sceneAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "scene-component-prefab-schema-v2" })
if ($sceneAuthoringSurface.Count -ne 1 -or $sceneAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface scene-component-prefab-schema-v2 must be ready as a contract-only MK_scene surface"
}
if (-not ([string]$sceneAuthoringSurface[0].notes).Contains("Contract-only") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("nested prefab propagation/merge resolution UX") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("2D/3D vertical slices")) {
    Write-Error "engine/agent/manifest.json scene-component-prefab-schema-v2 authoring surface must keep contract-only follow-up limits explicit"
}
$assetIdentityAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "asset-identity-v2" })
if ($assetIdentityAuthoringSurface.Count -ne 1 -or $assetIdentityAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface asset-identity-v2 must be ready as a foundation-only MK_assets surface"
}
if (-not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Foundation-only") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("GameEngine.AssetIdentity.v2") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("plan_asset_identity_placements_v2") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Reviewed command-owned apply surfaces") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("placement_rows") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("ContentBrowserState") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("SourceAssetRegistryDocumentV1") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("ContentBrowserState::refresh_from") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("content_browser_import.assets") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("GameEngine.Project.v4 project.source_registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("refresh_content_browser_from_project_source_registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Reload Source Registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("audit_runtime_scene_asset_identity") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("AssetKeyV2 key-first") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("tools/new-game.ps1") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("runtime source registry parsing") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("2D/3D vertical slices")) {
    Write-Error "engine/agent/manifest.json asset-identity-v2 authoring surface must keep foundation-only follow-up limits explicit"
}
$uiAtlasAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "ui-atlas-metadata-authoring-tooling-v1" })
if ($uiAtlasAuthoringSurface.Count -ne 1 -or $uiAtlasAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface ui-atlas-metadata-authoring-tooling-v1 must be ready as a cooked-metadata-only MK_assets/MK_tools surface"
}
if (-not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("GameEngine.UiAtlas.v1") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_cooked_ui_atlas_metadata") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("verify_cooked_ui_atlas_package_metadata") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_packed_ui_atlas_from_decoded_images") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("GameEngine.CookedTexture.v1") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("renderer texture upload")) {
    Write-Error "engine/agent/manifest.json ui-atlas-metadata-authoring-tooling-v1 authoring surface must keep cooked metadata tooling, decoded atlas bridge, and renderer-upload limits explicit"
}

$requiredProductionGapIds = @(
    "scene-component-prefab-schema-v2",
    "runtime-resource-v2",
    "renderer-rhi-resource-foundation",
    "frame-graph-v1",
    "upload-staging-v1",
    "2d-playable-vertical-slice",
    "3d-playable-vertical-slice",
    "editor-productization",
    "production-ui-importer-platform-adapters",
    "full-repository-quality-gate"
)
$productionGapIds = @{}
foreach ($gap in $productionLoop.unsupportedProductionGaps) {
    Assert-JsonProperty $gap @("id", "oneDotZeroCloseoutTier", "status", "requiredBeforeReadyClaim", "notes") "engine/agent/manifest.json aiOperableProductionLoop unsupportedProductionGaps"
    $productionGapIds[$gap.id] = $true
    if ($gap.status -eq "ready") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop unsupported gap '$($gap.id)' must not be ready"
    }
}
foreach ($gapId in $requiredProductionGapIds) {
    if (-not $productionGapIds.ContainsKey($gapId)) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing unsupported gap id: $gapId"
    }
}
$sceneSchemaGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "scene-component-prefab-schema-v2" })
if ($sceneSchemaGap.Count -ne 1 -or $sceneSchemaGap[0].status -ne "implemented-contract-only") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop scene-component-prefab-schema-v2 gap must be implemented-contract-only"
}
if (-not ([string]$sceneSchemaGap[0].notes).Contains("contract-only") -or
    -not ([string]$sceneSchemaGap[0].notes).Contains("broad/dependent package cooking") -or
    -not ([string]$sceneSchemaGap[0].notes).Contains("nested prefab propagation/merge resolution UX")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop scene-component-prefab-schema-v2 gap must keep remaining unsupported claims explicit"
}
$assetIdentityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "asset-identity-v2" })
if ($assetIdentityGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop asset-identity-v2 gap must leave unsupportedProductionGaps after reference cleanup closeout"
}
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Asset Identity v2 Reference Cleanup Milestone v1 completes" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "audit_runtime_scene_asset_identity" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Runtime Package Streaming Resident Replace v1 completes" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "execute_selected_runtime_package_streaming_resident_replace_safe_point" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Runtime Package Streaming Resident Unmount v1 completes" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "execute_selected_runtime_package_streaming_resident_unmount_safe_point" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Runtime Resident Package Eviction Plan v1 completes" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "plan_runtime_resident_package_evictions_v2" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Runtime Resident Package Reviewed Eviction Commit v1 completes" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "commit_runtime_resident_package_reviewed_evictions_v2" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Runtime Package Hot Reload Reviewed Replacement v1 completes" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Runtime Package Hot Reload Change Candidate Review v1 completes" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "plan_runtime_package_hot_reload_candidate_review_v2" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Runtime Package Hot Reload Recook Change Review v1 completes" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "plan_runtime_package_hot_reload_recook_change_review_v2" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Runtime Package Hot Reload Replacement Intent Review v1 completes" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "plan_runtime_package_hot_reload_replacement_intent_review_v2" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "runtime-resource-v2 next" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.reason"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "Runtime Package Hot Reload Recook Change Review v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.reason"
foreach ($check in @(
    @{
        Path = "engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp"
        Needles = @(
            "RuntimeSceneAssetIdentityAudit",
            "RuntimeSceneAssetIdentityReferenceRow",
            "audit_runtime_scene_asset_identity"
        )
    },
    @{
        Path = "engine/runtime_scene/src/runtime_scene.cpp"
        Needles = @(
            "make_identity_lookup",
            "append_asset_identity_audit",
            "asset_id_from_key_v2"
        )
    },
    @{
        Path = "tests/unit/runtime_scene_tests.cpp"
        Needles = @(
            "runtime scene audits asset identity keys for component references",
            "runtime scene asset identity audit reports missing and wrong-kind rows"
        )
    },
    @{
        Path = "tools/new-game-templates.ps1"
        Needles = @(
            "asset_id_from_game_asset_key",
            "asset_id_from_key_v2"
        )
    }
)) {
    $fileText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) asset identity reference cleanup evidence"
    }
}
$runtimeResourceGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "runtime-resource-v2" })
if ($runtimeResourceGap.Count -ne 1 -or $runtimeResourceGap[0].status -ne "implemented-foundation-only") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop runtime-resource-v2 gap must be implemented-foundation-only"
}
if (-not ([string]$runtimeResourceGap[0].notes).Contains("foundation-only") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("generation-checked handles") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimeResidentPackageMountSetV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimeResidentCatalogCacheV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimeResidentPackageMountIdV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("commit_runtime_resident_package_replace_v2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("execute_selected_runtime_package_streaming_resident_replace_safe_point") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("resident_replace result rows") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("execute_selected_runtime_package_streaming_resident_unmount_safe_point") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("resident_unmount result rows") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("projected remaining preload/kind/count") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("plan_runtime_resident_package_evictions_v2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimeResidentPackageEvictionPlanStatusV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("reviewed explicit candidate unmount order") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("budget_unreachable") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("commit_runtime_resident_package_reviewed_evictions_v2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimeResidentPackageReviewedEvictionCommitStatusV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("standalone reviewed resident eviction execution") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("Runtime Package Hot Reload Reviewed Replacement v1") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("host-driven reviewed hot-reload replacement safe point") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("Runtime Package Hot Reload Candidate Review v1") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("plan_runtime_package_hot_reload_candidate_review_v2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimePackageHotReloadCandidateReviewResultV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("Runtime Package Hot Reload Replacement Intent Review v1") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("plan_runtime_package_hot_reload_replacement_intent_review_v2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimePackageHotReloadReplacementIntentReviewResultV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("Runtime Package Hot Reload Recook Change Review v1") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("plan_runtime_package_hot_reload_recook_change_review_v2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimePackageHotReloadRecookChangeReviewResultV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("candidate/discovery root coherence") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("defined overlay") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("file watching/recook execution") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("slot-preserving") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("projected resident budget") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("raw loaded-package catalog") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("mount-set generations") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("package streaming") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("required preload asset") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("resident resource kind") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("renderer/RHI resource ownership")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop runtime-resource-v2 gap must keep remaining unsupported claims explicit"
}
$runtimeResourceRequiredClaims = @($runtimeResourceGap[0].requiredBeforeReadyClaim)
if ($runtimeResourceRequiredClaims -contains "production package mounts") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop runtime-resource-v2 production package mounts claim must be closed by resident package mount set"
}
foreach ($requiredClaim in @("resource residency", "hot reload", "renderer/RHI resource ownership", "package streaming")) {
    if ($runtimeResourceRequiredClaims -notcontains $requiredClaim) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop runtime-resource-v2 remaining claim missing: $requiredClaim"
    }
}
foreach ($check in @(
    @{
        Path = "engine/runtime/include/mirakana/runtime/resource_runtime.hpp"
        Needles = @(
            "RuntimeResidentPackageMountSetV2",
            "RuntimeResidentPackageMountIdV2",
            "RuntimeResidentPackageMountStatusV2",
            "RuntimeResidentPackageMountCatalogBuildResultV2",
            "build_runtime_resource_catalog_v2_from_resident_mount_set",
            "RuntimeResidentCatalogCacheV2",
            "RuntimeResidentCatalogCacheStatusV2",
            "RuntimeResidentPackageReplaceCommitStatusV2",
            "RuntimeResidentPackageReplaceCommitResultV2",
            "commit_runtime_resident_package_replace_v2",
            "RuntimeResidentPackageUnmountCommitStatusV2",
            "RuntimeResidentPackageUnmountCommitResultV2",
            "commit_runtime_resident_package_unmount_v2",
            "RuntimeResidentPackageEvictionPlanStatusV2",
            "RuntimeResidentPackageEvictionPlanDescV2",
            "RuntimeResidentPackageEvictionPlanResultV2",
            "plan_runtime_resident_package_evictions_v2",
            "RuntimeResidentPackageReviewedEvictionCommitStatusV2",
            "RuntimeResidentPackageReviewedEvictionCommitDescV2",
            "RuntimeResidentPackageReviewedEvictionCommitResultV2",
            "commit_runtime_resident_package_reviewed_evictions_v2",
            "RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2",
            "RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDescV2",
            "RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2",
            "commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2",
            "RuntimePackageHotReloadCandidateReviewStatusV2",
            "RuntimePackageHotReloadCandidateReviewDescV2",
            "RuntimePackageHotReloadCandidateReviewResultV2",
            "plan_runtime_package_hot_reload_candidate_review_v2",
            "RuntimePackageHotReloadRecookChangeReviewStatusV2",
            "RuntimePackageHotReloadRecookChangeReviewDescV2",
            "RuntimePackageHotReloadRecookChangeReviewResultV2",
            "plan_runtime_package_hot_reload_recook_change_review_v2",
            "RuntimePackageHotReloadReplacementIntentReviewStatusV2",
            "invalid_overlay",
            "RuntimePackageHotReloadReplacementIntentReviewDescV2",
            "RuntimePackageHotReloadReplacementIntentReviewResultV2",
            "plan_runtime_package_hot_reload_replacement_intent_review_v2"
        )
    },
    @{
        Path = "engine/runtime/src/resource_runtime.cpp"
        Needles = @(
            "duplicate-mount-id",
            "missing-mount-id",
            "mount_set.generation()",
            "build_runtime_resource_catalog_v2_from_resident_mount_set",
            "RuntimeResidentCatalogCacheV2::refresh",
            "RuntimeResidentCatalogCacheStatusV2::cache_hit",
            "RuntimeResidentPackageReplaceCommitResultV2::succeeded",
            "RuntimeResidentPackageMountSetReplaceAccessV2",
            "invoked_candidate_catalog_build",
            "RuntimeResidentPackageUnmountCommitResultV2::succeeded",
            "RuntimeResidentPackageEvictionPlanResultV2::succeeded",
            "RuntimeResidentPackageReviewedEvictionCommitResultV2::succeeded",
            "RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2::succeeded",
            "RuntimePackageHotReloadCandidateReviewResultV2::succeeded",
            "RuntimePackageHotReloadRecookChangeReviewResultV2::succeeded",
            "RuntimePackageHotReloadReplacementIntentReviewResultV2::succeeded",
            "plan_runtime_package_hot_reload_candidate_review_v2",
            "plan_runtime_package_hot_reload_recook_change_review_v2",
            "plan_runtime_package_hot_reload_replacement_intent_review_v2",
            "invalid-recook-apply-result-asset",
            "invalid-recook-apply-result-revision",
            "commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2",
            "invalid-selected-candidate",
            "candidate-outside-discovery-root",
            "candidate-content-root-mismatch",
            "invalid-overlay",
            "protected-eviction-candidate-mount-id",
            "map_reviewed_evictions_replace_diagnostic_phase",
            "add_reviewed_evictions_replace_commit_diagnostics",
            "map_reviewed_eviction_commit_plan_status",
            "contains_mount_id",
            "protected-candidate-mount-id",
            "budget_unreachable",
            "projected_mount_set.unmount",
            "RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set",
            "RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache",
            "mount_set = std::move(projected_mount_set)",
            "catalog_cache = std::move(projected_catalog_cache)"
        )
    },
    @{
        Path = "tests/unit/asset_identity_runtime_resource_tests.cpp"
        Needles = @(
            "runtime resident package mount set rebuilds catalog from explicit mount order",
            "runtime resident package mount set rejects duplicate invalid and missing ids",
            "runtime resident package mount set unmounts package and invalidates catalog handles on rebuild"
        )
    },
    @{
        Path = "tests/unit/runtime_resource_resident_cache_tests.cpp"
        Needles = @(
            "runtime resident catalog cache reuses catalog for unchanged mount generation and budget",
            "runtime resident catalog cache rebuilds when mount set generation changes",
            "runtime resident catalog cache rejects budget changes without replacing cached catalog"
        )
    },
    @{
        Path = "engine/runtime/include/mirakana/runtime/package_streaming.hpp"
        Needles = @(
            "resident_mount_failed",
            "resident_replace_failed",
            "resident_unmount_failed",
            "resident_catalog_refresh_failed",
            "resident_eviction_plan_failed",
            "RuntimeResidentCatalogCacheV2& catalog_cache",
            "RuntimeResidentPackageMountIdV2 mount_id",
            "RuntimeResidentPackageReplaceCommitResultV2 resident_replace",
            "RuntimeResidentPackageUnmountCommitResultV2 resident_unmount",
            "execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point",
            "execute_selected_runtime_package_streaming_resident_replace_safe_point",
            "execute_selected_runtime_package_streaming_resident_unmount_safe_point",
            "RuntimeResidentPackageEvictionPlanResultV2 eviction_plan",
            "resident_catalog_refresh"
        )
    },
    @{
        Path = "engine/runtime/src/package_streaming.cpp"
        Needles = @(
            "project_resident_packages",
            "evaluate_projected_resident_budget",
            "validate_loaded_package_catalog_before_mount",
            "validate_resident_replace_mount_id",
            "validate_projected_resident_catalog_hints",
            "commit_runtime_resident_package_replace_v2",
            "commit_runtime_resident_package_unmount_v2",
            "add_candidate_catalog_build_diagnostics",
            "map_reviewed_evictions_mount_status",
            "map_reviewed_evictions_replace_status",
            "commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2",
            "commit_runtime_package_candidate_resident_replace_with_reviewed_evictions_v2",
            "execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point",
            "resident_replace_failed",
            "resident_unmount_failed",
            "resident_catalog_refresh_failed",
            "resident_eviction_plan_failed",
            "mount_set.unmount(mount_id)"
        )
    },
    @{
        Path = "tests/unit/runtime_package_streaming_resident_mount_tests.cpp"
        Needles = @(
            "runtime package streaming resident mount commit mounts package and refreshes resident catalog",
            "runtime package streaming resident mount commit rejects duplicate mount id before mutation",
            "runtime package streaming resident mount commit rejects duplicate records before mutation",
            "runtime package streaming resident mount commit preserves catalog on projected budget failure",
            "runtime package streaming resident replace commit replaces mounted package and refreshes resident catalog",
            "runtime package streaming resident replace commit rejects invalid and missing ids before mutation",
            "runtime package streaming resident replace commit preserves state on candidate catalog failure",
            "runtime package streaming resident replace commit preserves state on projected budget failure",
            "runtime package streaming candidate resident mount with reviewed evictions commits after eviction",
            "runtime package streaming candidate resident mount with reviewed evictions rejects reviewed eviction ",
            "runtime package streaming candidate resident replace with reviewed evictions commits after eviction",
            "runtime package streaming candidate resident replace with reviewed evictions rejects reviewed eviction ",
            "runtime package streaming candidate resident replace with reviewed evictions preserves state when candidates ",
            "runtime package streaming resident unmount commit removes mounted package and refreshes resident catalog",
            "runtime package streaming resident unmount commit rejects invalid and missing ids before mutation",
            "runtime package streaming resident unmount commit preserves state on projected residency hint failure"
        )
    },
    @{
        Path = "tests/unit/runtime_resource_resident_unmount_tests.cpp"
        Needles = @(
            "runtime resident package unmount commit refreshes catalog cache",
            "runtime resident package unmount commit rejects missing id before mutation",
            "runtime resident package unmount commit preserves state on projected remaining budget failure",
            "runtime resident package eviction plan is no op when current view is within budget",
            "runtime resident package eviction plan returns reviewed candidate order until budget passes",
            "runtime resident package eviction plan rejects protected candidates before partial planning",
            "runtime resident package eviction plan rejects duplicate and missing candidates before partial planning",
            "runtime resident package eviction plan reports unreachable budget without mutating mounts",
            "runtime resident package reviewed eviction commit applies reviewed candidates atomically",
            "runtime resident package reviewed eviction commit succeeds as no op when current view fits",
            "runtime resident package reviewed eviction commit rejects reviewed candidates before mutation",
            "runtime resident package reviewed eviction commit preserves state when candidates are insufficient",
            "result.catalog_refresh.mounted_package_count == 1"
        )
    },
    @{
        Path = "tests/unit/runtime_resource_resident_replace_tests.cpp"
        Needles = @(
            "runtime resident package replacement commit preserves mount slot and refreshes catalog cache",
            "runtime resident package replacement commit rejects invalid and missing ids before mutation",
            "runtime resident package replacement commit rejects duplicate candidate records before mutation",
            "runtime resident package replacement commit preserves state on projected budget failure"
        )
    },
    @{
        Path = "tests/unit/runtime_package_discovery_resident_replace_reviewed_evictions_tests.cpp"
        Needles = @(
            "runtime package discovery resident replace with reviewed evictions commits selected candidate after eviction",
            "runtime package discovery resident replace with reviewed evictions skips eviction when projected view fits",
            "runtime package discovery resident replace with reviewed evictions rejects descriptors before scans",
            "runtime package discovery resident replace with reviewed evictions reports missing candidate before package ",
            "runtime package discovery resident replace with reviewed evictions preserves state on delegated load failure",
            "runtime package discovery resident replace with reviewed evictions rejects duplicate-asset indexes before ",
            "runtime package discovery resident replace with reviewed evictions maps reviewed eviction failures",
            "runtime package discovery resident replace with reviewed evictions preserves state when candidates are "
        )
    },
    @{
        Path = "tests/unit/runtime_package_hot_reload_candidate_review_tests.cpp"
        Needles = @(
            "runtime package hot reload candidate review maps package index paths in stable order",
            "runtime package hot reload candidate review maps changed payload paths under candidate content roots",
            "runtime package hot reload candidate review rejects invalid and reports unmatched changed paths",
            "runtime package hot reload candidate review deduplicates repeated matches for one candidate",
            "runtime package hot reload candidate review ignores invalid candidate rows before review",
            "runtime package hot reload candidate review returns typed no-match statuses without package reads"
        )
    },
    @{
        Path = "tests/unit/runtime_package_hot_reload_replacement_intent_review_tests.cpp"
        Needles = @(
            "runtime package hot reload replacement intent review builds safe point descriptor",
            "runtime package hot reload replacement intent review rejects invalid candidate rows",
            "runtime package hot reload replacement intent review rejects invalid and missing mount ids",
            "runtime package hot reload replacement intent review rejects unsafe discovery descriptors",
            "runtime package hot reload replacement intent review rejects invalid overlay intent",
            "runtime package hot reload replacement intent review validates reviewed eviction candidates"
        )
    },
    @{
        Path = "tests/unit/runtime_package_hot_reload_recook_change_review_tests.cpp"
        Needles = @(
            "runtime package hot reload recook change review maps staged and applied recook outputs",
            "runtime package hot reload recook change review blocks failed recook rows before candidate review",
            "runtime package hot reload recook change review rejects invalid recook rows before candidate review",
            "static_cast<mirakana::AssetHotReloadApplyResultKind>(255)",
            "runtime package hot reload recook change review reports no recook rows without reading packages",
            "runtime package hot reload recook change review surfaces candidate review failures"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Resident package mount set",
            "RuntimeResidentPackageMountSetV2",
            "Resident catalog cache",
            "RuntimeResidentCatalogCacheV2",
            "Resident package streaming mount commit",
            "Resident package streaming replace commit",
            "execute_selected_runtime_package_streaming_resident_replace_safe_point",
            "Runtime Package Streaming Resident Unmount v1",
            "execute_selected_runtime_package_streaming_resident_unmount_safe_point",
            "Resident package eviction plan",
            "plan_runtime_resident_package_evictions_v2",
            "Resident package reviewed eviction commit",
            "commit_runtime_resident_package_reviewed_evictions_v2",
            "Runtime Package Hot Reload Reviewed Replacement v1",
            "host-driven reviewed hot-reload replacement safe point",
            "Runtime Package Hot Reload Candidate Review v1",
            "plan_runtime_package_hot_reload_candidate_review_v2",
            "Runtime Package Hot Reload Recook Change Review v1",
            "plan_runtime_package_hot_reload_recook_change_review_v2",
            "Runtime Package Hot Reload Replacement Intent Review v1",
            "plan_runtime_package_hot_reload_replacement_intent_review_v2",
            "RuntimePackageHotReloadReplacementIntentReviewResultV2",
            "invalid overlay modes",
            "Resident package replacement commit",
            "commit_runtime_resident_package_replace_v2",
            "commit_runtime_resident_package_unmount_v2",
            "disk/VFS mount discovery"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Runtime Resource Resident Package Mount Set v1 coverage",
            "Runtime Resource Resident Catalog Cache v1 coverage",
            "Runtime Resource Streaming Resident Mount Commit v1 coverage",
            "Runtime Resource Resident Unmount Cache Refresh v1 coverage",
            "Runtime Resource Resident Package Replacement Commit v1 coverage",
            "Runtime Package Streaming Resident Replace v1 coverage",
            "execute_selected_runtime_package_streaming_resident_replace_safe_point",
            "Runtime Package Streaming Resident Unmount v1 coverage",
            "execute_selected_runtime_package_streaming_resident_unmount_safe_point",
            "Runtime Resident Package Eviction Plan v1 coverage",
            "plan_runtime_resident_package_evictions_v2",
            "Runtime Resident Package Reviewed Eviction Commit v1 coverage",
            "commit_runtime_resident_package_reviewed_evictions_v2",
            "Runtime Package Hot Reload Reviewed Replacement v1 coverage",
            "commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2",
            "Runtime Package Hot Reload Candidate Review v1 coverage",
            "MK_runtime_package_hot_reload_candidate_review_tests",
            "plan_runtime_package_hot_reload_candidate_review_v2",
            "Runtime Package Hot Reload Recook Change Review v1 coverage",
            "MK_runtime_package_hot_reload_recook_change_review_tests",
            "plan_runtime_package_hot_reload_recook_change_review_v2",
            "Runtime Package Hot Reload Replacement Intent Review v1 coverage",
            "MK_runtime_package_hot_reload_replacement_intent_review_tests",
            "plan_runtime_package_hot_reload_replacement_intent_review_v2",
            "candidate/discovery root mismatches",
            "invalid overlay modes",
            "rejects zero/duplicate/missing mount ids"
        )
    }
)) {
    $fileText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) runtime resource resident package mount set evidence"
    }
}
foreach ($check in @(
    @{
        Path = "engine/runtime/include/mirakana/runtime/package_streaming.hpp"
        Needles = @(
            "residency_hint_failed",
            "required_preload_assets",
            "resident_resource_kinds",
            "max_resident_packages",
            "required_preload_asset_count",
            "resident_resource_kind_count",
            "resident_package_count"
        )
    },
    @{
        Path = "engine/runtime/src/package_streaming.cpp"
        Needles = @(
            "validate_residency_hints",
            "preload-asset-missing",
            "resident-resource-kind-disallowed",
            "max-resident-packages-exceeded"
        )
    },
    @{
        Path = "tests/unit/asset_identity_runtime_resource_tests.cpp"
        Needles = @(
            "missing required preload asset hints",
            "disallowed resident resource kinds",
            "commits when residency hints match loaded package"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "selected safe-point package streaming with required preload asset",
            "broad/background package streaming"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "rejects missing required preload assets",
            "disallowed resident resource kinds"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Runtime Resource Residency Hints Execution v1 coverage",
            "preserves the active package/catalog"
        )
    }
)) {
    $fileText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) runtime resource residency hints execution evidence"
    }
}
$rendererRhiGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "renderer-rhi-resource-foundation" })
if ($rendererRhiGap.Count -ne 1 -or $rendererRhiGap[0].status -ne "implemented-foundation-only") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop renderer-rhi-resource-foundation gap must be implemented-foundation-only"
}
if (-not ([string]$rendererRhiGap[0].notes).Contains("foundation-only") -or
    -not ([string]$rendererRhiGap[0].notes).Contains("RhiResourceLifetimeRegistry") -or
    -not ([string]$rendererRhiGap[0].notes).Contains("GPU allocator") -or
    -not ([string]$rendererRhiGap[0].notes).Contains("upload/staging") -or
    -not ([string]$rendererRhiGap[0].notes).Contains("2D/3D playable vertical slices")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop renderer-rhi-resource-foundation gap must keep foundation-only follow-up limits explicit"
}
$frameGraphGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "frame-graph-v1" })
if ($frameGraphGap.Count -ne 1 -or $frameGraphGap[0].status -ne "implemented-foundation-only") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop frame-graph-v1 gap must be implemented-foundation-only"
}
if (-not ([string]$frameGraphGap[0].notes).Contains("foundation-only") -or
    -not ([string]$frameGraphGap[0].notes).Contains("FrameGraphV1Desc") -or
    -not ([string]$frameGraphGap[0].notes).Contains("barrier intent") -or
    -not ([string]$frameGraphGap[0].notes).Contains("execute_frame_graph_v1_schedule") -or
    -not ([string]$frameGraphGap[0].notes).Contains("execute_frame_graph_rhi_texture_schedule") -or
    -not ([string]$frameGraphGap[0].notes).Contains("production render graph") -or
    -not ([string]$frameGraphGap[0].notes).Contains("2D/3D playable vertical slices")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop frame-graph-v1 gap must keep foundation-only follow-up limits explicit"
}
$uploadStagingGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "upload-staging-v1" })
if ($uploadStagingGap.Count -ne 1 -or $uploadStagingGap[0].status -ne "implemented-foundation-only") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop upload-staging-v1 gap must be implemented-foundation-only"
}
if (-not ([string]$uploadStagingGap[0].notes).Contains("foundation-only") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("RhiUploadStagingPlan") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("FenceValue") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("Runtime RHI Upload Submission Fence Rows v1") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("submitted_upload_fences") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("submitted_upload_fence_count") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("RHI Upload Stale Generation Diagnostics v1") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("stale_generation") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("native GPU upload") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("2D/3D playable vertical slices")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop upload-staging-v1 gap must keep foundation-only follow-up limits explicit"
}
$desktop3dGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "3d-playable-vertical-slice" })
if ($desktop3dGap.Count -ne 1 -or $desktop3dGap[0].status -ne "implemented-generated-desktop-3d-package-proof") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop 3d-playable-vertical-slice gap must keep the generated desktop 3D package proof status"
}
Assert-ContainsText ([string]$desktop3dGap[0].notes) "selected generated directional shadow package smoke" "engine/agent/manifest.json aiOperableProductionLoop 3d-playable-vertical-slice gap"
Assert-ContainsText ([string]$desktop3dGap[0].notes) "directional_shadow_status=ready" "engine/agent/manifest.json aiOperableProductionLoop 3d-playable-vertical-slice gap"
Assert-ContainsText ([string]$desktop3dGap[0].notes) "selected generated graphics morph + directional shadow receiver package smoke" "engine/agent/manifest.json aiOperableProductionLoop 3d-playable-vertical-slice gap"
Assert-ContainsText ([string]$desktop3dGap[0].notes) "renderer_morph_descriptor_binds" "engine/agent/manifest.json aiOperableProductionLoop 3d-playable-vertical-slice gap"
Assert-ContainsText ([string]$desktop3dGap[0].notes) "production directional shadow quality" "engine/agent/manifest.json aiOperableProductionLoop 3d-playable-vertical-slice gap"
Assert-ContainsText ([string]$desktop3dGap[0].notes) "broad shadow+morph composition beyond the selected receiver smoke" "engine/agent/manifest.json aiOperableProductionLoop 3d-playable-vertical-slice gap"
Assert-DoesNotContainText ([string]$desktop3dGap[0].notes) "directional shadows and shadow filtering for generated packages" "engine/agent/manifest.json aiOperableProductionLoop 3d-playable-vertical-slice gap"
$physicsCollisionGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "physics-1-0-collision-system" })
if ($physicsCollisionGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop physics-1-0-collision-system gap must leave unsupportedProductionGaps after Physics 1.0 closeout"
}
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Physics 1.0 Collision System Closeout v1 is complete" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "first-party MK_physics 1.0 ready surface" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, oriented boxes, mesh/convex casts" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "runtime-resource-v2 next" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.reason"
$editorProductizationGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "editor-productization" })
if ($editorProductizationGap.Count -ne 1 -or $editorProductizationGap[0].status -ne "partly-ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop editor-productization gap must be partly-ready after Play-In-Editor Visible Viewport Wiring v1"
}
