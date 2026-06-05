#requires -Version 7.0
#requires -PSEdition Core
# Chapter 2 for check-ai-integration.ps1 static contracts.
# Engine/RHI/runtime surface texts consumed by this chapter and later rendering packs.
$runtimeInputRebindingPresentationRowsPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$generatedGameValidationScenariosText = Get-AgentSurfaceText "docs/specs/generated-game-validation-scenarios.md"
$rhiPublicHeaderText = Get-AgentSurfaceText "engine/rhi/include/mirakana/rhi/rhi.hpp"
$rhiAsyncOverlapSourceText = Get-AgentSurfaceText "engine/rhi/src/async_overlap.cpp"
$nullRhiSourceText = Get-AgentSurfaceText "engine/rhi/src/null_rhi.cpp"
$d3d12RhiHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
$d3d12RhiSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_backend.cpp"
$vulkanRhiHeaderText = Get-AgentSurfaceText "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp"
$vulkanRhiSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
$runtimeRhiHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
$runtimeRhiSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/runtime_upload.cpp"
$runtimeSceneHeaderText = Get-AgentSurfaceText "engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp"
$runtimeSceneSourceText = Get-AgentSurfaceText "engine/runtime_scene/src/runtime_scene.cpp"
$runtimeSceneTestsText = Get-AgentSurfaceText "tests/unit/runtime_scene_tests.cpp"
$runtimeSceneRhiHeaderText = Get-AgentSurfaceText "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
$runtimeSceneRhiSourceText = Get-AgentSurfaceText "engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp"
$rendererHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/renderer.hpp"
$rendererSourceText = Get-AgentSurfaceText "engine/renderer/src/rhi_frame_renderer.cpp"
$runtimeHostWin32HeaderText = Get-AgentSurfaceText "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
$runtimeHostWin32SourceText = Get-AgentSurfaceText "engine/runtime_host/win32/src/win32_desktop_presentation.cpp"
$runtimeHostWin32SceneGpuInjectingRendererText = Get-AgentSurfaceText "engine/runtime_host/win32/src/scene_gpu_binding_injecting_renderer.hpp"
$runtimeHostWin32TestsText = Get-AgentSurfaceText "tests/unit/runtime_host_win32_tests.cpp"
$runtimeHostWin32PublicApiText = Get-AgentSurfaceText "tests/unit/runtime_host_win32_public_api_compile.cpp"
$rhiTestsText = Get-AgentSurfaceText "tests/unit/rhi_tests.cpp"
$runtimeRhiTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_tests.cpp"
$backendScaffoldTestsText = Get-AgentSurfaceText "tests/unit/backend_scaffold_tests.cpp"
$vulkanComputeMorphShaderText = Get-AgentSurfaceText "tests/shaders/vulkan_compute_morph_position.hlsl"
$vulkanComputeMorphRendererShaderText = Get-AgentSurfaceText "tests/shaders/vulkan_compute_morph_renderer_position.hlsl"
$vulkanComputeMorphTangentFrameShaderText = Get-AgentSurfaceText "tests/shaders/vulkan_compute_morph_tangent_frame.hlsl"

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
$currentCapabilitiesInputTimeline = [regex]::Match($currentCapabilitiesText, '(?m)^`2D Sprite Animation Package v1`.*$')
Assert-ContainsText $currentCapabilitiesInputTimeline.Value "RuntimeInputRebindingPresentationModel" "current capabilities input timeline summary"
Assert-ContainsText $currentCapabilitiesInputTimeline.Value "platform input glyph generation" "current capabilities input timeline summary"
$roadmapRuntimePresentationFoundationRow = [regex]::Match($roadmapText, '(?m)^- Runtime Input Rebinding Presentation Rows v1.*$')
Assert-ContainsText $roadmapRuntimePresentationFoundationRow.Value "RuntimeInputRebindingPresentationModel" "roadmap runtime foundation summary"
Assert-ContainsText $roadmapRuntimePresentationFoundationRow.Value "symbolic glyph lookup keys" "roadmap runtime foundation summary"
$masterPlanRuntimeUiLedgerNote = [regex]::Match($masterPlanText, '(?m)^Runtime UI and input ledger note:.*$')
Assert-ContainsText $masterPlanRuntimeUiLedgerNote.Value "RuntimeInputRebindingPresentationModel" "master plan runtime UI ledger note"
Assert-ContainsText $masterPlanRuntimeUiLedgerNote.Value "platform input glyph generation" "master plan runtime UI ledger note"
Assert-ContainsText $masterPlanText "Completed gap burn-down" "production master plan completed gap pointer"
Assert-ContainsText $masterPlanText "Renderer RHI Resource Foundation 1.0 Scope Closeout v1" "production master plan renderer-rhi closeout pointer"
Assert-ContainsText $masterPlanText "recommendedNextPlan.id = next-production-gap-selection" "production master plan selection gate pointer"
Assert-ContainsText $masterPlanText "Windows CPU Set Worker Placement v1" "production master plan Windows CPU Set closeout pointer"
Assert-ContainsText $masterPlanText "Job Execution Placement Policy v1" "production master plan placement policy closeout pointer"
Assert-ContainsText $masterPlanText "Native Win32 Editor Shell v1" "production master plan native editor shell closeout pointer"
Assert-ContainsText $masterPlanText "Renderer Backend Parity Metal Apple Evidence v1" "production master plan renderer Metal Apple closeout pointer"
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
Assert-ContainsText $runtimeHostWin32HeaderText "Win32DesktopPresentationVulkanSceneRendererDesc" "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostWin32HeaderText "compute_morph_vertex_shader" "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostWin32HeaderText "compute_morph_shader" "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostWin32HeaderText "compute_morph_mesh_bindings" "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostWin32HeaderText "compute_morph_skinned_shader" "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostWin32HeaderText "compute_morph_skinned_mesh_bindings" "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostWin32HeaderText "compute_morph_queue_waits" "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostWin32HeaderText "compute_morph_async_compute_queue_submits" "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostWin32HeaderText "compute_morph_async_last_graphics_submitted_fence_value" "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
Assert-ContainsText $runtimeHostWin32SourceText "Vulkan scene compute morph vertex SPIR-V validation failed" "engine/runtime_host/win32/src/win32_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostWin32SourceText "Vulkan scene compute morph compute SPIR-V validation failed" "engine/runtime_host/win32/src/win32_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostWin32SourceText "Vulkan scene compute mapping SPIR-V validation failed" "engine/runtime_host/win32/src/win32_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostWin32SourceText "build_scene_compute_morph_bindings(" "engine/runtime_host/win32/src/win32_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostWin32SourceText '"Vulkan"' "engine/runtime_host/win32/src/win32_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostWin32SourceText "device.wait_for_queue(rhi::QueueKind::graphics, fence)" "engine/runtime_host/win32/src/win32_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostWin32SourceText "++result.queue_waits" "engine/runtime_host/win32/src/win32_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostWin32SourceText "compute_morph_bindings.queue_waits" "engine/runtime_host/win32/src/win32_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostWin32SourceText "dispatch_scene_compute_morph_skinned_bindings" "engine/runtime_host/win32/src/win32_desktop_presentation.cpp"
Assert-ContainsText $runtimeHostWin32SceneGpuInjectingRendererText "compute_morph_async_last_graphics_submitted_fence_value" "engine/runtime_host/win32/src/scene_gpu_binding_injecting_renderer.hpp"
Assert-ContainsText $runtimeHostWin32SceneGpuInjectingRendererText "rhi_stats.last_graphics_submitted_fence_value" "engine/runtime_host/win32/src/scene_gpu_binding_injecting_renderer.hpp"
Assert-ContainsText $runtimeHostWin32TestsText "stats.compute_morph_queue_waits == 1" "tests/unit/runtime_host_win32_tests.cpp"
Assert-ContainsText $runtimeHostWin32TestsText "vulkan_desc.compute_morph_vertex_shader" "tests/unit/runtime_host_win32_tests.cpp"
Assert-ContainsText $runtimeHostWin32TestsText "vulkan_desc.compute_morph_shader" "tests/unit/runtime_host_win32_tests.cpp"
Assert-ContainsText $runtimeHostWin32TestsText "vulkan_desc.compute_morph_mesh_bindings" "tests/unit/runtime_host_win32_tests.cpp"
Assert-ContainsText $runtimeHostWin32TestsText "vulkan_desc.compute_morph_skinned_shader" "tests/unit/runtime_host_win32_tests.cpp"
Assert-ContainsText $runtimeHostWin32TestsText "vulkan_desc.compute_morph_skinned_mesh_bindings" "tests/unit/runtime_host_win32_tests.cpp"
Assert-ContainsText $runtimeHostWin32TestsText "stats.compute_morph_async_compute_queue_submits == 1" "tests/unit/runtime_host_win32_tests.cpp"
Assert-ContainsText $runtimeHostWin32TestsText "stats.compute_morph_async_last_graphics_submitted_fence_value == graphics_fence.value" "tests/unit/runtime_host_win32_tests.cpp"
Assert-ContainsText $runtimeHostWin32PublicApiText "stats.compute_morph_queue_waits == 1" "tests/unit/runtime_host_win32_public_api_compile.cpp"
Assert-ContainsText $runtimeHostWin32PublicApiText "stats.compute_morph_async_compute_queue_submits == 1" "tests/unit/runtime_host_win32_public_api_compile.cpp"
Assert-ContainsText $runtimeHostWin32PublicApiText "scene_renderer.compute_morph_vertex_shader.entry_point" "tests/unit/runtime_host_win32_public_api_compile.cpp"
Assert-ContainsText $runtimeHostWin32PublicApiText "scene_renderer.compute_morph_shader.entry_point" "tests/unit/runtime_host_win32_public_api_compile.cpp"
Assert-ContainsText $runtimeHostWin32PublicApiText "scene_renderer.compute_morph_mesh_bindings.push_back" "tests/unit/runtime_host_win32_public_api_compile.cpp"
Assert-ContainsText $runtimeHostWin32PublicApiText "scene_renderer.compute_morph_skinned_shader.entry_point" "tests/unit/runtime_host_win32_public_api_compile.cpp"
Assert-ContainsText $runtimeHostWin32PublicApiText "scene_renderer.compute_morph_skinned_mesh_bindings.push_back" "tests/unit/runtime_host_win32_public_api_compile.cpp"
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
    foreach ($module in @("MK_core", "MK_platform", "MK_platform_win32", "MK_runtime", "MK_runtime_scene", "MK_runtime_host", "MK_runtime_host_win32", "MK_runtime_host_win32_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_ai", "MK_navigation", "MK_physics", "MK_renderer")) {
        if (@($desktop2dRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json 2d-desktop-runtime-package recipe missing required module: $module"
        }
    }
    Assert-ContainsText ([string]$desktop2dRecipe[0].cookedRuntimeAssumptions) "--require-gameplay-systems" "2d-desktop-runtime-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop2dRecipe[0].cookedRuntimeAssumptions) "gameplay_systems_*" "2d-desktop-runtime-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop2dRecipe[0].cookedRuntimeAssumptions) "spriteAtlasSourceAuthoringTargets" "2d-desktop-runtime-package cooked runtime assumptions"
    foreach ($sourceFormat in @("GameEngine.TextureSource.v1", "GameEngine.SourceAssetRegistry.v1", "first-party-cooked-fixture")) {
        if (@($desktop2dRecipe[0].importerAssumptions.sourceFormats) -notcontains $sourceFormat) {
            Write-Error "engine/agent/manifest.json 2d-desktop-runtime-package recipe importerAssumptions.sourceFormats missing $sourceFormat"
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
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-2d-package-proof", "installed-2d-sandbox-package-budget-smoke", "installed-2d-performance-baseline-smoke", "installed-2d-long-run-readiness-smoke", "host-2d-long-run-readiness-soak", "desktop-runtime-2d-vulkan-window-package", "shader-toolchain")) {
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
    foreach ($module in @("MK_animation", "MK_core", "MK_math", "MK_platform", "MK_platform_win32", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_win32", "MK_runtime_host_win32_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
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
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_win32", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_win32", "MK_runtime_host_win32_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
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
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_win32", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_win32", "MK_runtime_host_win32_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
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
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_win32", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_win32", "MK_runtime_host_win32_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
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
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_win32", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_win32", "MK_runtime_host_win32_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
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
    "refresh-prefab-instance",
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
        @("create-game-recipe", "register-runtime-package-files", "update-ui-atlas-metadata-package", "create-material-instance", "create-material-from-graph", "update-scene-package", "migrate-scene-v2-runtime-package", "create-scene", "add-scene-node", "add-or-update-component", "create-prefab", "instantiate-prefab", "refresh-prefab-instance", "register-source-asset", "cook-registered-source-assets") -notcontains $commandSurface.id) {
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
    Assert-JsonProperty $commandSurface.undoToken @("status", "notes") "engine/agent/manifest.json aiOperableProductionLoop command surface undoToken"
    if ($commandSurface.undoToken.status -ne "placeholder-only") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' undoToken must remain placeholder-only in this slice"
    }
    if ($modeIds.ContainsKey("apply") -and $modeIds["apply"].status -eq "ready" -and @($commandSurface.validationRecipes).Count -lt 1) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop command surface '$($commandSurface.id)' must list validation recipes when apply is ready"
    }
}
$createGameRecipeCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "create-game-recipe" })
if ($createGameRecipeCommand.Count -ne 1 -or $createGameRecipeCommand[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ready create-game-recipe command surface"
} else {
    $createGameRecipeModes = @{}
    foreach ($mode in @($createGameRecipeCommand[0].requestModes)) {
        $createGameRecipeModes[$mode.id] = $mode
    }
    if (-not $createGameRecipeModes.ContainsKey("dry-run") -or $createGameRecipeModes["dry-run"].status -ne "ready" -or
        -not $createGameRecipeModes.ContainsKey("apply") -or $createGameRecipeModes["apply"].status -ne "ready") {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop create-game-recipe must keep dry-run and apply ready"
    }
    foreach ($requiredField in @("mode", "gameName", "designSpecPath")) {
        if (@($createGameRecipeCommand[0].requestShape.requiredFields) -notcontains $requiredField) {
            Write-Error "engine/agent/manifest.json create-game-recipe requestShape missing required field: $requiredField"
        }
    }
    foreach ($resultField in @("plannedFiles", "changedFiles")) {
        if (-not ((@($createGameRecipeCommand[0].resultShape.dryRunFields) -contains $resultField) -or
            (@($createGameRecipeCommand[0].resultShape.applyFields) -contains $resultField))) {
            Write-Error "engine/agent/manifest.json create-game-recipe resultShape missing field: $resultField"
        }
    }
    $createGameRecipeText = ($createGameRecipeCommand[0] | ConvertTo-Json -Depth 20)
    foreach ($needle in @("tools/create-game-recipe.ps1", "tools/new-game.ps1", "aiWorkflow.gameDesignSpec", "DesktopRuntime2DPackage", "DesktopRuntime3DPackage", "arbitrary shell", "external asset generation")) {
        if (-not (Test-AgentSurfaceContainsText -Text $createGameRecipeText -Needle $needle)) {
            Write-Error "engine/agent/manifest.json create-game-recipe command surface missing reviewed boundary needle: $needle"
        }
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
        if (-not (Test-AgentSurfaceContainsText -Text $materialGraphNotes -Needle $needle)) {
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
        if (-not (Test-AgentSurfaceContainsText -Text $sceneNotes -Needle $needle)) {
            Write-Error "engine/agent/manifest.json update-scene-package notes must keep helper names and unsupported scene/package limits explicit: $needle"
        }
    }
}
$scenePrefabAuthoringCommandIds = @(
    "create-scene",
    "add-scene-node",
    "add-or-update-component",
    "create-prefab",
    "instantiate-prefab",
    "refresh-prefab-instance"
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
            if (-not (Test-AgentSurfaceContainsText -Text $scenePrefabPolicyText -Needle $needle)) {
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
        if (-not (Test-AgentSurfaceContainsText -Text $sourceAssetPolicyText -Needle $needle)) {
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
        if (-not (Test-AgentSurfaceContainsText -Text $registeredCookPolicyText -Needle $needle)) {
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
        if (-not (Test-AgentSurfaceContainsText -Text $sceneMigrationPolicyText -Needle $needle)) {
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
        if (-not (Test-AgentSurfaceContainsText -Text $runtimeSceneValidationPolicyText -Needle $needle)) {
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
        if (-not (Test-AgentSurfaceContainsText -Text $runnerPolicyText -Needle $needle)) {
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
            "public-api-boundary", "shader-toolchain",
            "renderer-metal-apple-host-evidence",
            "desktop-game-runtime",
            "desktop-editor",
            "desktop-runtime-sample-game-scene-gpu-package",
            "desktop-runtime-sample-game-environment-fog-package",
            "desktop-runtime-sample-game-cloud-layer-package",
            "desktop-runtime-sample-game-environment-profile-package",
            "desktop-runtime-generated-material-shader-scaffold-package",
            "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict",
            "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package", "installed-2d-performance-baseline-smoke", "installed-2d-long-run-readiness-smoke", "host-2d-long-run-readiness-soak",
            "dev-windows-editor-game-module-driver-load-tests"
        )) {
        if (@($validationRunnerCommand[0].validationRecipes) -notcontains $recipe) {
            Write-Error "engine/agent/manifest.json run-validation-recipe validationRecipes missing allowlisted recipe: $recipe"
        }
    }
    if (@($validationRunnerCommand[0].validationRecipes).Count -ne 19) {
        Write-Error "engine/agent/manifest.json run-validation-recipe validationRecipes must be exactly the reviewed allowlist of 19 recipes"
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
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("ScenePrefabInstanceRefreshPlanV2") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("plan_scene_prefab_instance_refresh_v2") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("ScenePrefabInstanceRefreshResultV2") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("apply_scene_prefab_instance_refresh_v2") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("duplicate_prefab_source_identity") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("unsupported_nested_prefab_instance") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("unsupported_local_prefab_child") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("unsupported_local_prefab_component") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("source_node_id") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("source_component_id") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("nested prefab propagation/merge resolution UX") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("2D/3D vertical slices")) {
    Write-Error "engine/agent/manifest.json scene-component-prefab-schema-v2 authoring surface must keep contract-only follow-up limits explicit"
}
$gameplayBindingAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "runtime-scene-gameplay-binding-v1" })
if ($gameplayBindingAuthoringSurface.Count -ne 1 -or $gameplayBindingAuthoringSurface[0].status -ne "ready" -or
    $gameplayBindingAuthoringSurface[0].owner -ne "MK_runtime_scene") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface runtime-scene-gameplay-binding-v1 must be ready as an MK_runtime_scene surface"
}
if (-not ([string]$gameplayBindingAuthoringSurface[0].notes).Contains("RuntimeSceneGameplayBindingSourceRow") -or
    -not ([string]$gameplayBindingAuthoringSurface[0].notes).Contains("RuntimeSceneGameplayBindingComponentKind") -or
    -not ([string]$gameplayBindingAuthoringSurface[0].notes).Contains("resolve_runtime_scene_gameplay_bindings") -or
    -not ([string]$gameplayBindingAuthoringSurface[0].notes).Contains("duplicate binding ids") -or
    -not ([string]$gameplayBindingAuthoringSurface[0].notes).Contains("missing required components") -or
    -not ([string]$gameplayBindingAuthoringSurface[0].notes).Contains("gameplay_systems_scene_binding_ready") -or
    -not ([string]$gameplayBindingAuthoringSurface[0].notes).Contains("gameplay system scheduler") -or
    -not ([string]$gameplayBindingAuthoringSurface[0].notes).Contains("package format change")) {
    Write-Error "engine/agent/manifest.json runtime-scene-gameplay-binding-v1 authoring surface must keep binding contract and non-goals explicit"
}
foreach ($needle in @(
        "RuntimeSceneGameplayBindingSourceRow",
        "RuntimeSceneGameplayBindingComponentKind",
        "RuntimeSceneGameplayBindingResolution",
        "resolve_runtime_scene_gameplay_bindings"
    )) {
    Assert-ContainsText $runtimeSceneHeaderText $needle "engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp"
}
foreach ($needle in @(
        "RuntimeSceneGameplayBindingDiagnosticCode::duplicate_binding_id",
        "RuntimeSceneGameplayBindingDiagnosticCode::missing_required_component",
        "node_has_gameplay_binding_component"
    )) {
    Assert-ContainsText $runtimeSceneSourceText $needle "engine/runtime_scene/src/runtime_scene.cpp"
}
Assert-ContainsText $runtimeSceneTestsText "runtime scene resolves authored gameplay bindings to component-backed nodes" "tests/unit/runtime_scene_tests.cpp"
Assert-ContainsText $runtimeSceneTestsText "runtime scene gameplay bindings fail closed for invalid ambiguous and missing component rows" "tests/unit/runtime_scene_tests.cpp"
foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $generatedGameValidationScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "resolve_runtime_scene_gameplay_bindings" $docSurface.Label
    Assert-ContainsText $docSurface.Text "RuntimeSceneGameplayBindingSourceRow" $docSurface.Label
}
$gameplayInteractionAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "runtime-scene-gameplay-interaction-framework-v1" })
if ($gameplayInteractionAuthoringSurface.Count -ne 1 -or $gameplayInteractionAuthoringSurface[0].status -ne "ready" -or
    $gameplayInteractionAuthoringSurface[0].owner -ne "MK_runtime_scene") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface runtime-scene-gameplay-interaction-framework-v1 must be ready as an MK_runtime_scene surface"
}
if (-not ([string]$gameplayInteractionAuthoringSurface[0].notes).Contains("RuntimeSceneGameplayInteractionSourceRow") -or
    -not ([string]$gameplayInteractionAuthoringSurface[0].notes).Contains("RuntimeSceneGameplayInteractionPlanRequest") -or
    -not ([string]$gameplayInteractionAuthoringSurface[0].notes).Contains("plan_runtime_scene_gameplay_interactions") -or
    -not ([string]$gameplayInteractionAuthoringSurface[0].notes).Contains("RuntimeSceneGameplayBindingRow") -or
    -not ([string]$gameplayInteractionAuthoringSurface[0].notes).Contains("duplicate action ids") -or
    -not ([string]$gameplayInteractionAuthoringSurface[0].notes).Contains("missing source or target bindings") -or
    -not ([string]$gameplayInteractionAuthoringSurface[0].notes).Contains("rejected terminal-state transitions") -or
    -not ([string]$gameplayInteractionAuthoringSurface[0].notes).Contains("gameplay system scheduler") -or
    -not ([string]$gameplayInteractionAuthoringSurface[0].notes).Contains("physics/event dispatcher") -or
    -not ([string]$gameplayInteractionAuthoringSurface[0].notes).Contains("package format change")) {
    Write-Error "engine/agent/manifest.json runtime-scene-gameplay-interaction-framework-v1 authoring surface must keep interaction contract and non-goals explicit"
}
foreach ($needle in @(
        "RuntimeSceneGameplayInteractionSourceRow",
        "RuntimeSceneGameplayInteractionPlanRequest",
        "RuntimeSceneGameplayInteractionPlan",
        "plan_runtime_scene_gameplay_interactions"
    )) {
    Assert-ContainsText $runtimeSceneHeaderText $needle "engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp"
}
foreach ($needle in @(
        "RuntimeSceneGameplayInteractionDiagnosticCode::duplicate_action_id",
        "RuntimeSceneGameplayInteractionDiagnosticCode::missing_target_binding",
        "RuntimeSceneGameplayInteractionDiagnosticCode::rejected_transition",
        "gameplay_interaction_resulting_session_state"
    )) {
    Assert-ContainsText $runtimeSceneSourceText $needle "engine/runtime_scene/src/runtime_scene.cpp"
}
Assert-ContainsText $runtimeSceneTestsText "runtime scene gameplay interaction plan composes binding rows in authored order" "tests/unit/runtime_scene_tests.cpp"
Assert-ContainsText $runtimeSceneTestsText "runtime scene gameplay interaction plan rejects invalid targets duplicates and terminal transitions" "tests/unit/runtime_scene_tests.cpp"
foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $generatedGameValidationScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "plan_runtime_scene_gameplay_interactions" $docSurface.Label
    Assert-ContainsText $docSurface.Text "RuntimeSceneGameplayInteractionSourceRow" $docSurface.Label
}
foreach ($sampleSurface in @(
        "games/sample_2d_playable_foundation/main.cpp",
        "games/sample_gameplay_foundation/main.cpp"
    )) {
    $sampleSurfaceText = Get-AgentSurfaceText $sampleSurface
    Assert-ContainsText $sampleSurfaceText "plan_runtime_scene_gameplay_interactions" $sampleSurface
    Assert-ContainsText $sampleSurfaceText "RuntimeSceneGameplayInteractionSourceRow" $sampleSurface
    Assert-ContainsText $sampleSurfaceText "gameplay_interactions" $sampleSurface
}
$runtimeSessionProfilePathAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "runtime-session-profile-path-policy-v1" })
if ($runtimeSessionProfilePathAuthoringSurface.Count -ne 1 -or
    $runtimeSessionProfilePathAuthoringSurface[0].status -ne "ready" -or
    $runtimeSessionProfilePathAuthoringSurface[0].owner -ne "MK_runtime") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface runtime-session-profile-path-policy-v1 must be ready as an MK_runtime surface"
}
if (-not ([string]$runtimeSessionProfilePathAuthoringSurface[0].notes).Contains("RuntimeSessionProfilePathRequest") -or
    -not ([string]$runtimeSessionProfilePathAuthoringSurface[0].notes).Contains("RuntimeSessionProfilePathPlan") -or
    -not ([string]$runtimeSessionProfilePathAuthoringSurface[0].notes).Contains("plan_runtime_session_profile_paths") -or
    -not ([string]$runtimeSessionProfilePathAuthoringSurface[0].notes).Contains("save.gesave") -or
    -not ([string]$runtimeSessionProfilePathAuthoringSurface[0].notes).Contains("settings.settings") -or
    -not ([string]$runtimeSessionProfilePathAuthoringSurface[0].notes).Contains("input.geinputprofile") -or
    -not ([string]$runtimeSessionProfilePathAuthoringSurface[0].notes).Contains("absolute paths") -or
    -not ([string]$runtimeSessionProfilePathAuthoringSurface[0].notes).Contains("parent traversal") -or
    -not ([string]$runtimeSessionProfilePathAuthoringSurface[0].notes).Contains("platform user-directory resolver") -or
    -not ([string]$runtimeSessionProfilePathAuthoringSurface[0].notes).Contains("cloud save system")) {
    Write-Error "engine/agent/manifest.json runtime-session-profile-path-policy-v1 authoring surface must keep profile path contract and non-goals explicit"
}
$runtimeSessionServicesHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/session_services.hpp"
$runtimeSessionServicesSourceText = Get-AgentSurfaceText "engine/runtime/src/session_services.cpp"
$runtimeTestsText = Get-AgentSurfaceText "tests/unit/runtime_tests.cpp"
foreach ($needle in @(
        "RuntimeSessionProfilePathRequest",
        "RuntimeSessionProfilePathPlan",
        "RuntimeSessionProfilePathDiagnosticCode",
        "plan_runtime_session_profile_paths"
    )) {
    Assert-ContainsText $runtimeSessionServicesHeaderText $needle "engine/runtime/include/mirakana/runtime/session_services.hpp"
}
foreach ($needle in @(
        "RuntimeSessionProfilePathDiagnosticCode::invalid_game_id",
        "RuntimeSessionProfilePathDiagnosticCode::invalid_profile_id",
        "RuntimeSessionProfilePathDiagnosticCode::invalid_root_path",
        "is_valid_runtime_profile_root_path",
        "save.gesave",
        "settings.settings",
        "input.geinputprofile"
    )) {
    Assert-ContainsText $runtimeSessionServicesSourceText $needle "engine/runtime/src/session_services.cpp"
}
Assert-ContainsText $runtimeTestsText "runtime session profile path plan composes deterministic game local document paths" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime session profile path plan rejects unsafe ids and paths without partial paths" "tests/unit/runtime_tests.cpp"
foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $generatedGameValidationScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeSessionProfilePathRequest" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_session_profile_paths" $docSurface.Label
}
$runtimeSessionProfileDocumentBundleAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "runtime-session-profile-document-bundle-v1" })
if ($runtimeSessionProfileDocumentBundleAuthoringSurface.Count -ne 1 -or
    $runtimeSessionProfileDocumentBundleAuthoringSurface[0].status -ne "ready" -or
    $runtimeSessionProfileDocumentBundleAuthoringSurface[0].owner -ne "MK_runtime") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface runtime-session-profile-document-bundle-v1 must be ready as an MK_runtime surface"
}
if (-not ([string]$runtimeSessionProfileDocumentBundleAuthoringSurface[0].notes).Contains("RuntimeSessionProfileDocuments") -or
    -not ([string]$runtimeSessionProfileDocumentBundleAuthoringSurface[0].notes).Contains("RuntimeSessionProfileDocumentRow") -or
    -not ([string]$runtimeSessionProfileDocumentBundleAuthoringSurface[0].notes).Contains("load_runtime_session_profile_documents") -or
    -not ([string]$runtimeSessionProfileDocumentBundleAuthoringSurface[0].notes).Contains("write_runtime_session_profile_documents") -or
    -not ([string]$runtimeSessionProfileDocumentBundleAuthoringSurface[0].notes).Contains("defaulted_missing") -or
    -not ([string]$runtimeSessionProfileDocumentBundleAuthoringSurface[0].notes).Contains("failed_corrupt") -or
    -not ([string]$runtimeSessionProfileDocumentBundleAuthoringSurface[0].notes).Contains("failed_unsupported_version") -or
    -not ([string]$runtimeSessionProfileDocumentBundleAuthoringSurface[0].notes).Contains("unrelated files under the profile root are not deleted") -or
    -not ([string]$runtimeSessionProfileDocumentBundleAuthoringSurface[0].notes).Contains("game-specific save schema")) {
    Write-Error "engine/agent/manifest.json runtime-session-profile-document-bundle-v1 authoring surface must keep document bundle contract and non-goals explicit"
}
foreach ($needle in @(
        "RuntimeSessionProfileDocuments",
        "RuntimeSessionProfileDocumentRow",
        "RuntimeSessionProfileDocumentLoadRequest",
        "RuntimeSessionProfileDocumentWriteRequest",
        "load_runtime_session_profile_documents",
        "write_runtime_session_profile_documents"
    )) {
    Assert-ContainsText $runtimeSessionServicesHeaderText $needle "engine/runtime/include/mirakana/runtime/session_services.hpp"
}
foreach ($needle in @(
        "RuntimeSessionProfileDocumentStatus::defaulted_missing",
        "RuntimeSessionProfileDocumentStatus::failed_corrupt",
        "RuntimeSessionProfileDocumentStatus::failed_unsupported_version",
        "serialize_runtime_input_rebinding_profile",
        "runtime_session_profile_document_failure_status"
    )) {
    Assert-ContainsText $runtimeSessionServicesSourceText $needle "engine/runtime/src/session_services.cpp"
}
Assert-ContainsText $runtimeTestsText "runtime session profile document bundle loads documents and defaults missing optional rows" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime session profile document bundle separates corrupt and unsupported documents" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime session profile document bundle writes reviewed defaults without deleting unrelated files" "tests/unit/runtime_tests.cpp"
$sampleGameplayFoundationText = Get-AgentSurfaceText "games/sample_gameplay_foundation/main.cpp"
Assert-ContainsText $sampleGameplayFoundationText "load_runtime_session_profile_documents" "games/sample_gameplay_foundation/main.cpp"
Assert-ContainsText $sampleGameplayFoundationText "write_runtime_session_profile_documents" "games/sample_gameplay_foundation/main.cpp"
Assert-ContainsText $sampleGameplayFoundationText "runtime_profile_documents" "games/sample_gameplay_foundation/main.cpp"
foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $generatedGameValidationScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeSessionProfileDocuments" $docSurface.Label
    Assert-ContainsText $docSurface.Text "load_runtime_session_profile_documents" $docSurface.Label
    Assert-ContainsText $docSurface.Text "write_runtime_session_profile_documents" $docSurface.Label
}
$runtimeSessionProfileResumeAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "runtime-session-profile-resume-plan-v1" })
if ($runtimeSessionProfileResumeAuthoringSurface.Count -ne 1 -or
    $runtimeSessionProfileResumeAuthoringSurface[0].status -ne "ready" -or
    $runtimeSessionProfileResumeAuthoringSurface[0].owner -ne "MK_runtime") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface runtime-session-profile-resume-plan-v1 must be ready as an MK_runtime surface"
}
if (-not ([string]$runtimeSessionProfileResumeAuthoringSurface[0].notes).Contains("RuntimeSessionProfileResumeRequest") -or
    -not ([string]$runtimeSessionProfileResumeAuthoringSurface[0].notes).Contains("RuntimeSessionProfileResumePlan") -or
    -not ([string]$runtimeSessionProfileResumeAuthoringSurface[0].notes).Contains("RuntimeSessionProfileResumeDiagnostic") -or
    -not ([string]$runtimeSessionProfileResumeAuthoringSurface[0].notes).Contains("plan_runtime_session_profile_resume") -or
    -not ([string]$runtimeSessionProfileResumeAuthoringSurface[0].notes).Contains("save.slot") -or
    -not ([string]$runtimeSessionProfileResumeAuthoringSurface[0].notes).Contains("progression.checkpoint") -or
    -not ([string]$runtimeSessionProfileResumeAuthoringSurface[0].notes).Contains("package.id") -or
    -not ([string]$runtimeSessionProfileResumeAuthoringSurface[0].notes).Contains("runtime_profile_resume_ready") -or
    -not ([string]$runtimeSessionProfileResumeAuthoringSurface[0].notes).Contains("save migration executor") -or
    -not ([string]$runtimeSessionProfileResumeAuthoringSurface[0].notes).Contains("game-specific save schema")) {
    Write-Error "engine/agent/manifest.json runtime-session-profile-resume-plan-v1 authoring surface must keep resume evidence contract and non-goals explicit"
}
foreach ($needle in @(
        "RuntimeSessionProfileResumeRequest",
        "RuntimeSessionProfileResumePlan",
        "RuntimeSessionProfileResumeDiagnostic",
        "RuntimeSessionProfileResumeDiagnosticCode",
        "plan_runtime_session_profile_resume"
    )) {
    Assert-ContainsText $runtimeSessionServicesHeaderText $needle "engine/runtime/include/mirakana/runtime/session_services.hpp"
}
foreach ($needle in @(
        "RuntimeSessionProfileResumeStatus::ready",
        "RuntimeSessionProfileResumeDiagnosticCode::blocking_document_status",
        "RuntimeSessionProfileResumeDiagnosticCode::missing_progression_checkpoint",
        "RuntimeSessionProfileResumeDiagnosticCode::package_id_mismatch",
        "RuntimeSessionProfileResumeDiagnosticCode::profile_id_mismatch"
    )) {
    Assert-ContainsText $runtimeSessionServicesSourceText $needle "engine/runtime/src/session_services.cpp"
}
Assert-ContainsText $runtimeTestsText "runtime session profile resume plan verifies save slot progression and package evidence" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime session profile resume plan fails closed for missing and mismatched evidence" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime session profile resume plan blocks defaulted documents before package resume" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime session profile resume plan blocks unsupported migration rows before package resume" "tests/unit/runtime_tests.cpp"
$sample2DPackageText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample3DPackageText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/main.cpp"
$sample2DPackageManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample3DPackageManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
foreach ($sampleSurface in @(
        @{ Text = $sample2DPackageText; Label = "games/sample_2d_desktop_runtime_package/main.cpp" },
        @{ Text = $sample3DPackageText; Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp" }
    )) {
    Assert-ContainsText $sampleSurface.Text "--require-runtime-profile-resume" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "runtime_profile_resume_ready" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "runtime_profile_resume_loaded_documents" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "plan_runtime_session_profile_resume" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_runtime_profile_resume_loaded_documents() != 3U" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_runtime_profile_resume_defaulted_documents() != 0U" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_runtime_profile_resume_save_schema_version() != 3U" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_runtime_profile_resume_settings_schema_version() != 2U" $sampleSurface.Label
}
foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $generatedGameValidationScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeSessionProfileResumeRequest" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_session_profile_resume" $docSurface.Label
    Assert-ContainsText $docSurface.Text "--require-runtime-profile-resume" $docSurface.Label
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeProfileResume) "RuntimeSessionProfileResumePlan" "runtime profile resume guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeProfileResume) "runtime_profile_resume_ready" "runtime profile resume guidance"
$runtimeSimulationPersistenceAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "runtime-simulation-persistence-v1" })
if ($runtimeSimulationPersistenceAuthoringSurface.Count -ne 1 -or
    $runtimeSimulationPersistenceAuthoringSurface[0].status -ne "ready" -or
    $runtimeSimulationPersistenceAuthoringSurface[0].owner -ne "MK_runtime") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface runtime-simulation-persistence-v1 must be ready as an MK_runtime surface"
}
if (-not ([string]$runtimeSimulationPersistenceAuthoringSurface[0].notes).Contains("RuntimeSimulationPersistenceRequest") -or
    -not ([string]$runtimeSimulationPersistenceAuthoringSurface[0].notes).Contains("RuntimeSimulationPersistencePlan") -or
    -not ([string]$runtimeSimulationPersistenceAuthoringSurface[0].notes).Contains("RuntimeSimulationPersistenceDiagnostic") -or
    -not ([string]$runtimeSimulationPersistenceAuthoringSurface[0].notes).Contains("plan_runtime_simulation_persistence") -or
    -not ([string]$runtimeSimulationPersistenceAuthoringSurface[0].notes).Contains("entity.<id>.(type|region|state_hash)") -or
    -not ([string]$runtimeSimulationPersistenceAuthoringSurface[0].notes).Contains("RuntimeSimulationPersistenceRemediationAction") -or
    -not ([string]$runtimeSimulationPersistenceAuthoringSurface[0].notes).Contains("binary compatibility policy") -or
    -not ([string]$runtimeSimulationPersistenceAuthoringSurface[0].notes).Contains("migration execution")) {
    Write-Error "engine/agent/manifest.json runtime-simulation-persistence-v1 authoring surface must keep simulation persistence contract and non-goals explicit"
}
foreach ($needle in @(
        "RuntimeSimulationPersistenceRequest",
        "RuntimeSimulationPersistencePlan",
        "RuntimeSimulationPersistenceDiagnostic",
        "RuntimeSimulationPersistenceDiagnosticCode",
        "RuntimeSimulationPersistenceRemediationAction",
        "plan_runtime_simulation_persistence"
    )) {
    Assert-ContainsText $runtimeSessionServicesHeaderText $needle "engine/runtime/include/mirakana/runtime/session_services.hpp"
}
foreach ($needle in @(
        "RuntimeSimulationPersistenceStatus::ready",
        "RuntimeSimulationPersistenceStatus::migration_required",
        "RuntimeSimulationPersistenceStatus::remediation_required",
        "RuntimeSimulationPersistenceDiagnosticCode::corrupt_save_document",
        "RuntimeSimulationPersistenceDiagnosticCode::missing_migration_step",
        "RuntimeSimulationPersistenceRemediationAction::quarantine_corrupt_save"
    )) {
    Assert-ContainsText $runtimeSessionServicesSourceText $needle "engine/runtime/src/session_services.cpp"
}
Assert-ContainsText $runtimeTestsText "runtime simulation persistence plan accepts stable snapshot entity rows and save slot evidence" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime simulation persistence plan reports supported schema migration chain" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime simulation persistence plan chooses reachable migration path over dead end" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime simulation persistence plan blocks malformed entity rows and migration gaps" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime simulation persistence plan keeps valid save recovery when settings document is corrupt" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime simulation persistence plan recommends corrupt save remediation" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime simulation persistence plan recommends unsupported save reset" "tests/unit/runtime_tests.cpp"
foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $generatedGameValidationScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeSimulationPersistenceRequest" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_simulation_persistence" $docSurface.Label
    Assert-ContainsText $docSurface.Text "RuntimeSimulationPersistenceRemediationAction" $docSurface.Label
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeSimulationPersistence) "RuntimeSimulationPersistencePlan" "runtime simulation persistence guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeSimulationPersistence) "blocking save document status" "runtime simulation persistence guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeSimulationPersistence) "binary compatibility policy" "runtime simulation persistence guidance"
$runtimeMenuHudIntentAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "runtime-menu-hud-intent-model-v1" })
if ($runtimeMenuHudIntentAuthoringSurface.Count -ne 1 -or
    $runtimeMenuHudIntentAuthoringSurface[0].status -ne "ready" -or
    $runtimeMenuHudIntentAuthoringSurface[0].owner -ne "MK_ui") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface runtime-menu-hud-intent-model-v1 must be ready as an MK_ui surface"
}
if (-not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("RuntimeMenuHudRowDesc") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("RuntimeMenuHudCommandIntent") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("RuntimeMenuHudCommandTarget") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("dialogue_box") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("input_binding_prompt") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("runtime_menu_hud_ready") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("plan_runtime_menu_hud") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("RuntimeMenuHudDisplayRow") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("RuntimeMenuHudCommandRow") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("duplicate command ids") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("invalid command targets") -or
    -not ([string]$runtimeMenuHudIntentAuthoringSurface[0].notes).Contains("game-specific menu schema")) {
    Write-Error "engine/agent/manifest.json runtime-menu-hud-intent-model-v1 authoring surface must keep HUD/menu intent contract and non-goals explicit"
}
$uiHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/ui.hpp"
$uiSourceText = Get-AgentSurfaceText "engine/ui/src/ui.cpp"
$coreTestsText = Get-AgentSurfaceText "tests/unit/core_tests.cpp"
$sampleUiAudioAssetsText = Get-AgentSurfaceText "games/sample_ui_audio_assets/main.cpp"
foreach ($needle in @(
        "RuntimeMenuHudRowDesc",
        "RuntimeMenuHudRowKind",
        "RuntimeMenuHudCommandIntent",
        "RuntimeMenuHudCommandTarget",
        "RuntimeMenuHudDiagnosticCode",
        "RuntimeMenuHudPlan",
        "plan_runtime_menu_hud"
    )) {
    Assert-ContainsText $uiHeaderText $needle "engine/ui/include/mirakana/ui/ui.hpp"
}
foreach ($needle in @(
        "RuntimeMenuHudDiagnosticCode::duplicate_row_id",
        "RuntimeMenuHudDiagnosticCode::missing_command_id",
        "RuntimeMenuHudDiagnosticCode::duplicate_command_id",
        "RuntimeMenuHudDiagnosticCode::invalid_command_target",
        "is_valid_runtime_menu_hud_command_intent"
    )) {
    Assert-ContainsText $uiSourceText $needle "engine/ui/src/ui.cpp"
}
Assert-ContainsText $coreTestsText "runtime menu hud plan produces deterministic display and command rows" "tests/unit/core_tests.cpp"
Assert-ContainsText $coreTestsText "runtime menu hud plan supports dialogue and input binding prompt rows" "tests/unit/core_tests.cpp"
Assert-ContainsText $coreTestsText "runtime menu hud plan rejects duplicate row and command ids" "tests/unit/core_tests.cpp"
Assert-ContainsText $coreTestsText "runtime menu hud plan rejects missing command ids and invalid command targets" "tests/unit/core_tests.cpp"
Assert-ContainsText $sampleUiAudioAssetsText "plan_runtime_menu_hud" "games/sample_ui_audio_assets/main.cpp"
Assert-ContainsText $sampleUiAudioAssetsText "RuntimeMenuHudCommandIntent::restart_session" "games/sample_ui_audio_assets/main.cpp"
$sample2DPackageText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample3DPackageText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/main.cpp"
foreach ($sampleSurface in @(
        @{ Text = $sample2DPackageText; Label = "games/sample_2d_desktop_runtime_package/main.cpp" },
        @{ Text = $sample3DPackageText; Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp" }
    )) {
    Assert-ContainsText $sampleSurface.Text "--require-runtime-menu-hud" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "runtime_menu_hud_ready" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "runtime_menu_hud_display_rows" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "runtime_menu_hud_command_rows" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "runtime_menu_hud_dialogue_rows" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "runtime_menu_hud_input_binding_prompt_rows" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_runtime_menu_hud_display_rows() != 6U" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_runtime_menu_hud_command_rows() != 2U" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_runtime_menu_hud_dialogue_rows() != 1U" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_runtime_menu_hud_input_binding_prompt_rows() != 1U" $sampleSurface.Label
}
foreach ($sampleManifestSurface in @(
        @{ Text = $sample2DPackageManifestText; Label = "games/sample_2d_desktop_runtime_package/game.agent.json"; Recipe = "installed-2d-runtime-menu-hud-smoke" },
        @{ Text = $sample3DPackageManifestText; Label = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"; Recipe = "installed-3d-runtime-menu-hud-smoke" }
    )) {
    Assert-ContainsText $sampleManifestSurface.Text "runtime-menu-hud" $sampleManifestSurface.Label
    Assert-ContainsText $sampleManifestSurface.Text $sampleManifestSurface.Recipe $sampleManifestSurface.Label
    Assert-ContainsText $sampleManifestSurface.Text "--require-runtime-menu-hud" $sampleManifestSurface.Label
    Assert-ContainsText $sampleManifestSurface.Text "runtime_menu_hud_ready" $sampleManifestSurface.Label
}
foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $generatedGameValidationScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeMenuHudRowDesc" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_menu_hud" $docSurface.Label
    Assert-ContainsText $docSurface.Text "RuntimeMenuHudCommandRow" $docSurface.Label
    Assert-ContainsText $docSurface.Text "--require-runtime-menu-hud" $docSurface.Label
}
$runtimeGameplayDebugOverlayAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "runtime-gameplay-debug-overlay-v1" })
if ($runtimeGameplayDebugOverlayAuthoringSurface.Count -ne 1 -or
    $runtimeGameplayDebugOverlayAuthoringSurface[0].status -ne "ready" -or
    $runtimeGameplayDebugOverlayAuthoringSurface[0].owner -ne "MK_ui") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface runtime-gameplay-debug-overlay-v1 must be ready as an MK_ui surface"
}
foreach ($gameplayDebugOverlaySurfaceNeedle in @(
        "RuntimeGameplayDebugOverlayRowDesc",
        "RuntimeGameplayDebugOverlayCategory",
        "RuntimeGameplayDebugOverlayRowKind",
        "RuntimeGameplayDebugOverlayDiagnosticCode",
        "RuntimeGameplayDebugOverlayRow",
        "RuntimeGameplayDebugOverlayDiagnostic",
        "RuntimeGameplayDebugOverlayPlan",
        "plan_runtime_gameplay_debug_overlay",
        "missing or duplicate row ids",
        "unsupported categories",
        "unsupported row kinds",
        "game-specific debug schema"
    )) {
    Assert-ContainsText ([string]$runtimeGameplayDebugOverlayAuthoringSurface[0].notes) $gameplayDebugOverlaySurfaceNeedle "engine/agent/manifest.json runtime-gameplay-debug-overlay-v1 authoring surface"
}
foreach ($gameplayDebugOverlayHeaderNeedle in @(
        "RuntimeGameplayDebugOverlayCategory",
        "RuntimeGameplayDebugOverlayRowKind",
        "RuntimeGameplayDebugOverlayDiagnosticCode",
        "RuntimeGameplayDebugOverlayRowDesc",
        "RuntimeGameplayDebugOverlayRow",
        "RuntimeGameplayDebugOverlayDiagnostic",
        "RuntimeGameplayDebugOverlayPlan",
        "plan_runtime_gameplay_debug_overlay"
    )) {
    Assert-ContainsText $uiHeaderText $gameplayDebugOverlayHeaderNeedle "engine/ui/include/mirakana/ui/ui.hpp"
}
foreach ($gameplayDebugOverlaySourceNeedle in @(
        "is_valid_runtime_gameplay_debug_overlay_category",
        "is_valid_runtime_gameplay_debug_overlay_row_kind",
        "append_runtime_gameplay_debug_overlay_diagnostic",
        "RuntimeGameplayDebugOverlayDiagnosticCode::duplicate_row_id",
        "RuntimeGameplayDebugOverlayDiagnosticCode::missing_label",
        "RuntimeGameplayDebugOverlayDiagnosticCode::unsupported_category",
        "RuntimeGameplayDebugOverlayDiagnosticCode::unsupported_row_kind"
    )) {
    Assert-ContainsText $uiSourceText $gameplayDebugOverlaySourceNeedle "engine/ui/src/ui.cpp"
}
Assert-ContainsText $coreTestsText "runtime gameplay debug overlay plan produces deterministic display rows" "tests/unit/core_tests.cpp"
Assert-ContainsText $coreTestsText "runtime gameplay debug overlay plan rejects duplicate and invalid rows" "tests/unit/core_tests.cpp"
foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $generatedGameValidationScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeGameplayDebugOverlayPlan" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_gameplay_debug_overlay" $docSurface.Label
    Assert-ContainsText $docSurface.Text "debug overlay" $docSurface.Label
}
$runtimeInputContextStackPlannerAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "runtime-input-context-stack-planner-v1" })
if ($runtimeInputContextStackPlannerAuthoringSurface.Count -ne 1 -or
    $runtimeInputContextStackPlannerAuthoringSurface[0].status -ne "ready" -or
    $runtimeInputContextStackPlannerAuthoringSurface[0].owner -ne "MK_runtime") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface runtime-input-context-stack-planner-v1 must be ready as an MK_runtime surface"
}
if (-not ([string]$runtimeInputContextStackPlannerAuthoringSurface[0].notes).Contains("RuntimeInputContextStackRequest") -or
    -not ([string]$runtimeInputContextStackPlannerAuthoringSurface[0].notes).Contains("RuntimeInputContextLayerDesc") -or
    -not ([string]$runtimeInputContextStackPlannerAuthoringSurface[0].notes).Contains("RuntimeInputContextStackPlan") -or
    -not ([string]$runtimeInputContextStackPlannerAuthoringSurface[0].notes).Contains("plan_runtime_input_context_stack") -or
    -not ([string]$runtimeInputContextStackPlannerAuthoringSurface[0].notes).Contains("duplicate_context") -or
    -not ([string]$runtimeInputContextStackPlannerAuthoringSurface[0].notes).Contains("invalid_context") -or
    -not ([string]$runtimeInputContextStackPlannerAuthoringSurface[0].notes).Contains("full runtime rebinding panel") -or
    -not ([string]$runtimeInputContextStackPlannerAuthoringSurface[0].notes).Contains("game-specific menu schema")) {
    Write-Error "engine/agent/manifest.json runtime-input-context-stack-planner-v1 authoring surface must keep input context planner contract and non-goals explicit"
}
foreach ($needle in @(
        "RuntimeInputContextStackRequest",
        "RuntimeInputContextLayerDesc",
        "RuntimeInputContextLayerKind",
        "RuntimeInputContextStackDiagnostic",
        "RuntimeInputContextStackPlan",
        "plan_runtime_input_context_stack"
    )) {
    Assert-ContainsText $runtimeSessionServicesHeaderText $needle "engine/runtime/include/mirakana/runtime/session_services.hpp"
}
foreach ($needle in @(
        "RuntimeInputContextStackDiagnosticCode::invalid_context",
        "RuntimeInputContextStackDiagnosticCode::duplicate_context",
        "RuntimeInputContextStackDiagnosticCode::no_active_context",
        "is_capture_context_layer"
    )) {
    Assert-ContainsText $runtimeSessionServicesSourceText $needle "engine/runtime/src/session_services.cpp"
}
Assert-ContainsText $runtimeTestsText "runtime input context stack plan resolves modal menu before gameplay" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime input context stack plan keeps gameplay under passive overlay" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime input context stack plan uses default context when no layer is active" "tests/unit/runtime_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime input context stack plan rejects invalid layer descriptions" "tests/unit/runtime_tests.cpp"
$sample2DPlayableFoundationText = Get-AgentSurfaceText "games/sample_2d_playable_foundation/main.cpp"
Assert-ContainsText $sample2DPlayableFoundationText "plan_runtime_input_context_stack" "games/sample_2d_playable_foundation/main.cpp"
Assert-ContainsText $sample2DPlayableFoundationText "plan_gameplay_audio_mix" "games/sample_2d_playable_foundation/main.cpp"
Assert-ContainsText $sample2DPlayableFoundationText "plan_runtime_gameplay_debug_overlay" "games/sample_2d_playable_foundation/main.cpp"
Assert-ContainsText $sample2DPlayableFoundationText "RuntimeInputContextLayerKind::overlay" "games/sample_2d_playable_foundation/main.cpp"
Assert-ContainsText $sample2DPlayableFoundationText "input_contexts=" "games/sample_2d_playable_foundation/main.cpp"
Assert-ContainsText $sample2DPlayableFoundationText "debug_overlay_rows=" "games/sample_2d_playable_foundation/main.cpp"
foreach ($sampleSurface in @(
        @{ Text = $sample2DPackageText; Label = "games/sample_2d_desktop_runtime_package/main.cpp" },
        @{ Text = $sample3DPackageText; Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp" }
    )) {
    Assert-ContainsText $sampleSurface.Text "--require-audio-gameplay-mixer" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "plan_gameplay_audio_mix" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_ready" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_diagnostics" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_buses" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_cues" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_triggers" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_commands" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_paused_buses" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_faded_buses" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_looping_commands" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_spatial_commands" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_render_commands" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_render_frames" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_render_samples" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_sample_abs_sum" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "audio_gameplay_mixer_payload_diagnostics" $sampleSurface.Label
}
foreach ($sampleSurface in @(
        @{ Text = $sample2DPackageText; Label = "games/sample_2d_desktop_runtime_package/main.cpp" },
        @{ Text = $sample3DPackageText; Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp" }
    )) {
    Assert-ContainsText $sampleSurface.Text "runtime_audio_payload" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "make_audio_samples" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "packaged_audio_asset_id" $sampleSurface.Label
}
Assert-ContainsText $sample3DPackageText "--require-audio-gameplay-mixer requires --require-scene-package with packaged gameplay audio" "games/sample_generated_desktop_runtime_3d_package/main.cpp"
foreach ($sampleManifestSurface in @(
        @{ Text = $sample2DPackageManifestText; Label = "games/sample_2d_desktop_runtime_package/game.agent.json"; Recipe = "installed-2d-audio-gameplay-mixer-smoke" },
        @{ Text = $sample3DPackageManifestText; Label = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"; Recipe = "installed-3d-audio-gameplay-mixer-smoke" }
    )) {
    Assert-ContainsText $sampleManifestSurface.Text "audio-gameplay-mixer" $sampleManifestSurface.Label
    Assert-ContainsText $sampleManifestSurface.Text $sampleManifestSurface.Recipe $sampleManifestSurface.Label
    Assert-ContainsText $sampleManifestSurface.Text "--require-audio-gameplay-mixer" $sampleManifestSurface.Label
    Assert-ContainsText $sampleManifestSurface.Text "audio_gameplay_mixer_ready" $sampleManifestSurface.Label
}
Assert-ContainsText $sample3DPackageManifestText "runtime/assets/3d/gameplay_systems.audio.geasset" "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
Assert-ContainsText $sample3DPackageManifestText "cooked package audio payload" "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
if (-not [regex]::Match($sample3DPackageManifestText, '(?s)"residentResourceKinds"\s*:\s*\[[^\]]*"audio"').Success) {
    Write-Error "games/sample_generated_desktop_runtime_3d_package/game.agent.json residentResourceKinds must include audio for package streaming smokes"
}
Assert-ContainsText $sample3DPackageText "mirakana::AssetKind::audio" "games/sample_generated_desktop_runtime_3d_package/main.cpp package streaming resident kinds"
Assert-ContainsText (Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex") "runtime/assets/3d/gameplay_systems.audio.geasset" "games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex"
Assert-ContainsText (Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/gameplay_systems.audio.geasset") "GameEngine.CookedAudio.v1" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/gameplay_systems.audio.geasset"
foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $generatedGameValidationScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeInputContextStackRequest" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_input_context_stack" $docSurface.Label
    Assert-ContainsText $docSurface.Text "--require-audio-gameplay-mixer" $docSurface.Label
    Assert-ContainsText $docSurface.Text "audio_gameplay_mixer_ready" $docSurface.Label
}
Assert-ContainsText $manifestRaw "currentInputContextPlanning" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "RuntimeInputContextStackPlan" "engine/agent/manifest.json"
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
$assetPlaceholderAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "asset-placeholder-generation-v1" })
if ($assetPlaceholderAuthoringSurface.Count -ne 1 -or $assetPlaceholderAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface asset-placeholder-generation-v1 must be ready as an MK_tools surface"
}
if (-not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("plan_placeholder_asset_bundle") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("plan_placeholder_asset_cook_package") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("placeholder_asset_tool.hpp") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetBundleRequest") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetBundlePlan") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetCookPackageRequest") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetCookPackagePlan") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetChangedFile") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetProvenanceRow") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetDiagnostic") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("GameEngine.SourceAssetRegistry.v1") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("replacement recooks") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("placeholderAssetPipeline replacementWorkflow") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("package handoff counters") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("external asset downloader") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("renderer/RHI residency")) {
    Write-Error "engine/agent/manifest.json asset-placeholder-generation-v1 authoring surface must keep placeholder contract and non-goals explicit"
}
$spriteAtlasAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "sprite-atlas-authoring-v1" })
if ($spriteAtlasAuthoringSurface.Count -ne 1 -or $spriteAtlasAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop authoring surface sprite-atlas-authoring-v1 must be ready as an MK_tools surface"
}
if (-not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourceFrameDesc") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourcePagePolicyDesc") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourcePivot") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourceSliceBorder") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourceAuthoringDesc") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourceAuthoringPlan") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("plan_sprite_atlas_source_authoring") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("sprite_atlas_tool.hpp") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("GameEngine.TextureSource.v1") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("GameEngine.SourceAssetRegistry.v1") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("single-page-tight-rgba8-texture-source") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("slice-border") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("renderer/RHI residency") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("package streaming") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("animation semantics")) {
    Write-Error "engine/agent/manifest.json sprite-atlas-authoring-v1 authoring surface must keep source atlas contract and non-goals explicit"
}

$requiredProductionGapIds = @()
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
if ($sceneSchemaGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop scene-component-prefab-schema-v2 gap must leave unsupportedProductionGaps after foundation closeout"
}
$playable2dGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "2d-playable-vertical-slice" })
if ($playable2dGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop 2d-playable-vertical-slice gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$playable3dGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "3d-playable-vertical-slice" })
if ($playable3dGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop 3d-playable-vertical-slice gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$editorProductizationGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "editor-productization" })
if ($editorProductizationGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop editor-productization gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$productionUiImporterPlatformGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "production-ui-importer-platform-adapters" })
if ($productionUiImporterPlatformGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$fullRepoQualityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "full-repository-quality-gate" })
if ($fullRepoQualityGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop full-repository-quality-gate gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$assetIdentityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "asset-identity-v2" })
if ($assetIdentityGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop asset-identity-v2 gap must leave unsupportedProductionGaps after reference cleanup closeout"
}
$runtimeResourceGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "runtime-resource-v2" })
if ($runtimeResourceGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop runtime-resource-v2 gap must leave unsupportedProductionGaps after 1.0 scope closeout"
}
$recommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
$recommendedPlanId = [string]$productionLoop.recommendedNextPlan.id
Assert-JsonProperty $productionLoop.cpuProfilingMatrix @("id", "status", "hostClasses", "requiredCpuFields", "traceRecipes", "classification", "beforeAfterTracePair", "regressionBudget", "nonGoals", "officialReferences", "validationRecipes") "engine/agent/manifest.json aiOperableProductionLoop.cpuProfilingMatrix"
if ($productionLoop.cpuProfilingMatrix.id -ne "long-running-performance-readiness-v1-phase-2") { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.cpuProfilingMatrix.id must be long-running-performance-readiness-v1-phase-2" }
if ($productionLoop.cpuProfilingMatrix.status -ne "host-gated") { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.cpuProfilingMatrix.status must remain host-gated" }
foreach ($needle in @("intel-mainstream-desktop-laptop", "intel-hybrid-pcore-ecore", "amd-ryzen", "amd-threadripper", "amd-epyc-nps", "linux-ci-host", "windows-ci-host")) { if (-not ((@($productionLoop.cpuProfilingMatrix.hostClasses | ForEach-Object { $_.id }) -join " ").Contains($needle))) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.cpuProfilingMatrix hostClasses missing: $needle" } }
foreach ($needle in @("exact_cpu_model", "processor_groups", "numa_node_count", "nps_state", "selected_simd_lane", "profiler_name_version")) { if (@($productionLoop.cpuProfilingMatrix.requiredCpuFields) -notcontains $needle) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.cpuProfilingMatrix requiredCpuFields missing: $needle" } }
foreach ($needle in @("cpu-frame-time", "worker-utilization", "queue-waits", "cache-behavior", "branch-misses", "memory-bandwidth", "false-sharing", "numa-locality")) { if (-not ((@($productionLoop.cpuProfilingMatrix.traceRecipes | ForEach-Object { $_.id }) -join " ").Contains($needle))) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.cpuProfilingMatrix traceRecipes missing: $needle" } }
if ($productionLoop.cpuProfilingMatrix.beforeAfterTracePair.required -ne $true -or $productionLoop.cpuProfilingMatrix.regressionBudget.required -ne $true) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.cpuProfilingMatrix must require before/after traces and regression budgets" }
if (@($productionLoop.cpuProfilingMatrix.validationRecipes) -notcontains "host-cpu-profiling-matrix") { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.cpuProfilingMatrix validationRecipes missing host-cpu-profiling-matrix" }
$cpuProfilingHostGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "cpu-profiling-matrix-host" })
if ($cpuProfilingHostGate.Count -ne 1 -or $cpuProfilingHostGate[0].status -ne "host-gated") { Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one host-gated cpu-profiling-matrix-host gate" }
$cpuProfilingRecipe = @($manifest.validationRecipes | Where-Object { $_.name -eq "host-cpu-profiling-matrix" })
if ($cpuProfilingRecipe.Count -ne 1) { Write-Error "engine/agent/manifest.json validationRecipes must include host-cpu-profiling-matrix" }
foreach ($needle in @("Intel/AMD CPU Profiling Matrix v1", "host-gated evidence selection", "before/after trace pairs")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentCpuProfilingMatrix) $needle "engine/agent/manifest.json gameCodeGuidance.currentCpuProfilingMatrix" }
Assert-JsonProperty $productionLoop.optionalGpuComputeReview @("id", "status", "classifications", "requiredEvidenceFields", "candidateRows", "fallbackRequirements", "nonGoals", "officialReferences", "validationRecipes") "engine/agent/manifest.json aiOperableProductionLoop.optionalGpuComputeReview"
if ($productionLoop.optionalGpuComputeReview.id -ne "long-running-performance-readiness-v1-phase-7" -or $productionLoop.optionalGpuComputeReview.status -ne "review-only") { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.optionalGpuComputeReview must be Phase 7 review-only" }
foreach ($needle in @("rhi_compute", "offline_tool_acceleration", "cuda_hip_private_adapter_candidate", "sycl_private_adapter_candidate", "non_goal")) { if (@($productionLoop.optionalGpuComputeReview.classifications) -notcontains $needle) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.optionalGpuComputeReview classifications missing: $needle" } }
foreach ($needle in @("data transfer cost", "memory residency", "synchronization", "stream/event usage", "queue/profiler visibility", "dependency burden", "scalar or RHI fallback")) { if (@($productionLoop.optionalGpuComputeReview.requiredEvidenceFields) -notcontains $needle) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.optionalGpuComputeReview requiredEvidenceFields missing: $needle" } }
foreach ($needle in @("CUDA/HIP/SYCL runtime dependency", "vcpkg.json feature", "CMake linkage", "default validation dependency", "broad GPU compute", "async overlap", "cross-vendor parity", "cross-backend parity", "broad CPU/GPU/memory optimization")) { if (@($productionLoop.optionalGpuComputeReview.nonGoals) -notcontains $needle) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.optionalGpuComputeReview nonGoals missing: $needle" } }
foreach ($needle in @("nvidia-cuda-best-practices", "amd-hip-asynchronous", "khronos-sycl-2020", "vulkan-queues", "d3d12-multi-engine-synchronization")) { if (@($productionLoop.optionalGpuComputeReview.officialReferences | ForEach-Object { $_.id }) -notcontains $needle) { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.optionalGpuComputeReview officialReferences missing: $needle" } }
if (@($productionLoop.optionalGpuComputeReview.validationRecipes) -notcontains "host-optional-gpu-compute-review") { Write-Error "engine/agent/manifest.json aiOperableProductionLoop.optionalGpuComputeReview validationRecipes missing host-optional-gpu-compute-review" }
$optionalGpuComputeHostGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "optional-gpu-compute-review-host" }); if ($optionalGpuComputeHostGate.Count -ne 1 -or $optionalGpuComputeHostGate[0].status -ne "host-gated") { Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one host-gated optional-gpu-compute-review-host gate" }
$optionalGpuComputeRecipe = @($manifest.validationRecipes | Where-Object { $_.name -eq "host-optional-gpu-compute-review" }); if ($optionalGpuComputeRecipe.Count -ne 1) { Write-Error "engine/agent/manifest.json validationRecipes must include host-optional-gpu-compute-review" }
foreach ($needle in @("Optional GPU Compute Review v1", "review-only evidence selection", "CMake linkage")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentOptionalGpuComputeReview) $needle "engine/agent/manifest.json gameCodeGuidance.currentOptionalGpuComputeReview" }
$recommendedPlanUsesLegacyCloseoutContext = $recommendedPlanId -notin @("general-purpose-game-production-v1", "generated-game-studio-v1", "engine-1-0-gap-matrix-v1", "next-production-gap-selection", "native-win32-editor-shell-v1", "first-party-editor-shell-v1", "first-party-ui-editor-production-stack-v1", "physics-navigation-commercial-coverage-v1", "renderer-backend-parity-metal-apple-evidence-v1", "renderer-postprocess-tone-mapping-evidence-v1", "sandbox-world-network-modding-gate-v1", "sandbox-world-package-validation-performance-budgets-v1", "ai-operable-performance-budget-and-evidence-v1", "performance-baseline-v1", "long-running-performance-readiness-v1-phase-1", "long-running-performance-readiness-v1-phase-2", "long-running-performance-readiness-v1-phase-7", "memory-lifetime-taxonomy-v1", "memory-diagnostics-v1", "frame-thread-scratch-v1", "job-scheduling-evidence-v1", "job-execution-worker-pool-v1", "job-execution-topology-policy-v1", "job-execution-work-stealing-v1", "job-execution-placement-policy-v1", "windows-cpu-set-worker-placement-v1", "windows-cpu-set-smt-worker-placement-v1", "simd-dispatch-policy-and-evidence-v1", "avx2-reviewed-target-execution-v1", "environment-system-v1", "mavg-asset-graph-v1", "mavg-runtime-lod-milestone-v1", "mavg-package-streaming-residency-dispatch-v1", "mavg-page-addressable-payload-schema-v1", "mavg-payload-byte-range-file-io-v1", "mavg-directstorage-request-plan-v1", "mavg-native-directstorage-win32-async-io-dispatch-status-v1", "mavg-win32-async-file-io-adapter-v1", "mavg-win32-iocp-file-io-worker-v1")
if ($productionLoop.currentActivePlan -eq "docs/superpowers/plans/2026-05-23-candidate-backlog-burn-down-v1.md") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop.currentActivePlan must not point at completed Candidate Backlog Burn-down v1"
}
if ($productionLoop.recommendedNextPlan.id -eq "simulation-persistence-v1") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop.recommendedNextPlan.id must not point at merged simulation-persistence-v1"
}
if ($recommendedPlanUsesLegacyCloseoutContext) {
    Assert-ContainsText $recommendedText "Candidate Backlog Burn-down v1 completed all seven canonical post-1.0 candidate rows" "engine/agent/manifest.json candidate backlog closeout evidence"
    Assert-ContainsText $recommendedText "PR #204" "engine/agent/manifest.json simulation persistence PR closeout evidence"
    Assert-ContainsText $recommendedText "971cee3f6c5b42965721c06974bc506f1b35508c" "engine/agent/manifest.json simulation persistence merge evidence"
    Assert-ContainsText $recommendedText "plan_runtime_simulation_persistence" "engine/agent/manifest.json simulation persistence closeout evidence"
}
$candidateBacklogPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-23-candidate-backlog-burn-down-v1.md"
Assert-ContainsText $candidateBacklogPlanText "**Status:** Completed." "Candidate Backlog Burn-down v1 status"
Assert-ContainsText $candidateBacklogPlanText '| simulation-persistence-v1 | #204 | `971cee3f6c5b42965721c06974bc506f1b35508c` | pass |' "Candidate Backlog Burn-down v1 simulation persistence closeout row"
if ($recommendedPlanUsesLegacyCloseoutContext) {
    foreach ($needle in @(
    "Frame Graph Transient Texture Alias Planning v1",
    "FrameGraphTransientTextureAliasPlan",
    "plan_frame_graph_transient_texture_aliases",
    "Frame Graph Shadow Scratch Color Target-State Ownership v1",
    "shadow_color",
    "6 pass callbacks/15 barrier steps",
    "Frame Graph Viewport Surface Color State Executor v1",
    "RhiViewportSurface",
    "viewport_color",
    "native heap allocation",
    "Frame Graph Texture Aliasing Barrier Command v1",
    "record_frame_graph_texture_aliasing_barriers",
    "Frame Graph Automatic Aliasing Barrier Insertion v1",
    "Package Streaming Frame Graph Texture Binding Handoff v1",
    "make_runtime_package_streaming_frame_graph_texture_bindings",
    "Runtime Package Streaming RHI Upload Binding Transaction v1",
    "upload_runtime_package_streaming_frame_graph_texture_bindings",
    "Frame Graph Production Ownership Boundary Selection v1",
    "FrameGraphProductionOwnershipPlan",
    "plan_frame_graph_production_ownership_boundary",
    "Frame Graph Render Pass Envelope v1",
    "render_passes_recorded",
    "Frame Graph Render Pass Stats Evidence v1",
    "RendererStats::framegraph_render_passes_recorded",
    "Runtime Material Factor Frame Graph Command Evidence v1",
    "RuntimeMaterialGpuBinding",
    "create_runtime_material_gpu_binding",
    "Frame Graph Multi-Queue Automatic Aliasing Barrier Execution v1",
    "FrameGraphRhiMultiQueueExecutionDesc::transient_texture_lifetimes",
    "FrameGraphRhiMultiQueueExecutionResult::aliasing_barriers_recorded",
    "FrameGraphRhiMultiQueuePackageEvidence",
    "framegraph_multiqueue_aliasing_barriers_recorded",
    "alias-induced cross-queue waits",
    "frame-graph-v1"
    )) {
        Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan frame-graph closeout evidence"
    }
}
foreach ($check in @(
    @{
        Path = "engine/runtime/include/mirakana/runtime/resource_runtime.hpp"
        Needles = @(
            "RuntimeResidentPackageMountSetV2",
            "RuntimeResidentCatalogCacheV2",
            "commit_runtime_resident_package_reviewed_evictions_v2",
            "commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2",
            "commit_runtime_package_hot_reload_recook_replacement_v2"
        )
    },
    @{
        Path = "engine/runtime/include/mirakana/runtime/package_streaming.hpp"
        Needles = @(
            "execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point",
            "execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point",
            "execute_selected_runtime_package_streaming_resident_unmount_safe_point"
        )
    },
    @{
        Path = "engine/tools/include/mirakana/tools/asset_runtime_package_hot_reload_tool.hpp"
        Needles = @(
            "execute_asset_runtime_package_hot_reload_replacement_safe_point",
            "execute_asset_runtime_package_hot_reload_registered_asset_watch_tick_safe_point",
            "AssetRuntimePackageHotReloadRegisteredAssetWatchTickState"
        )
    },
    @{
        Path = "tests/unit/tools_runtime_hot_reload_package_tests.cpp"
        Needles = @(
            "asset runtime package registered watch tick",
            "ready_recook_requests.size() == 2"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Runtime Resource v2 1.0 Scope Closeout",
            "renderer-rhi-resource-foundation",
            "native file watcher ownership"
        )
    }
)) {
    $fileText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) runtime-resource-v2 closeout evidence"
    }
}
$rendererRhiGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "renderer-rhi-resource-foundation" })
if ($rendererRhiGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop renderer-rhi-resource-foundation gap must leave unsupportedProductionGaps after 1.0 scope closeout"
}
foreach ($check in @(
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Renderer RHI Resource Foundation 1.0 Scope Closeout",
            "D3D12/Vulkan deferred native teardown",
            "RhiDeviceMemoryDiagnostics",
            "frame-graph-v1",
            "upload-staging-v1",
            "Metal IRhiDevice parity"
        )
    }
)) {
    $fileText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) renderer-rhi-resource-foundation closeout evidence"
    }
}
$frameGraphGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "frame-graph-v1" })
if ($frameGraphGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop frame-graph-v1 gap must leave unsupportedProductionGaps after 1.0 scope closeout"
}
foreach ($check in @(
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Frame Graph v1 1.0 Scope Closeout",
            "upload-staging-v1",
            "FrameGraphRhiMultiQueuePackageEvidence",
            "broad production render graph scheduling",
            "Metal memory alias allocation"
        )
    }
)) {
    $fileText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) frame-graph-v1 closeout evidence"
    }
}
$uploadStagingGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "upload-staging-v1" })
if ($uploadStagingGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop upload-staging-v1 gap must leave unsupportedProductionGaps after async-ready resource update closeout"
}
foreach ($check in @(
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Upload Staging v1 Async-Ready Resource Updates",
            "make_runtime_package_resource_update_readiness",
            "RuntimePackageResourceUpdateReadinessResult",
            "package_upload_staging_resource_updates_ready",
            "broad/background streaming"
        )
    }
)) {
    $fileText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) upload-staging-v1 closeout evidence"
    }
}
if ($recommendedPlanUsesLegacyCloseoutContext) {
    foreach ($needle in @(
            "3d-playable-vertical-slice",
            "generated desktop 3D package proof",
            "host-gated D3D12/Vulkan package smokes",
            "visible 3D aggregate counters",
            "native UI overlay/atlas package counters"
        )) {
        Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan 3d closeout"
    }
}
$physicsCollisionGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "physics-1-0-collision-system" })
if ($physicsCollisionGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop physics-1-0-collision-system gap must leave unsupportedProductionGaps after Physics 1.0 closeout"
}
foreach ($closedGameplayGapId in @(
    "navigation-navmesh-and-dynamic-obstacle-follow-up",
    "physics-advanced-dynamics-follow-up",
    "gameplay-2d-3d-package-evidence"
)) {
    $closedGameplayGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq $closedGameplayGapId })
    if ($closedGameplayGap.Count -ne 0) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop $closedGameplayGapId gap must leave unsupportedProductionGaps after gameplay physics/navigation closeout"
    }
}
if ($recommendedPlanUsesLegacyCloseoutContext) {
    foreach ($needle in @(
            "gameplay-physics-navigation-ai-foundation-v1",
            "NavigationNavmeshPathRequest",
            "plan_navigation_navmesh_path",
            "evaluate_physics_character_dynamic_policy_3d",
            "selected generated 2D and 3D package gameplay systems composition smokes",
            "navigation-navmesh-and-dynamic-obstacle-follow-up",
            "physics-advanced-dynamics-follow-up",
            "gameplay-2d-3d-package-evidence"
        )) {
        Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan gameplay closeout evidence"
    }
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Transient Texture Alias Planning v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "FrameGraphTransientTextureAliasPlan" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Shadow Scratch Color Target-State Ownership v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "6 pass callbacks/15 barrier steps" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Viewport Surface Color State Executor v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "RhiViewportSurface" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "viewport_color" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Texture Aliasing Barrier Command v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "record_frame_graph_texture_aliasing_barriers" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText $recommendedText "Package Streaming Frame Graph Texture Binding Handoff v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package streaming handoff"
    Assert-ContainsText $recommendedText "make_runtime_package_streaming_frame_graph_texture_bindings" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package streaming handoff"
    Assert-ContainsText $recommendedText "Runtime Package Streaming RHI Upload Binding Transaction v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package streaming upload transaction"
    Assert-ContainsText $recommendedText "upload_runtime_package_streaming_frame_graph_texture_bindings" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package streaming upload transaction"
    Assert-ContainsText $recommendedText "Runtime Ring-Backed Texture Upload v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime ring-backed texture upload"
    Assert-ContainsText $recommendedText "RuntimeTextureUploadOptions::upload_ring" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime ring-backed texture upload"
    Assert-ContainsText $recommendedText "Upload Staging v1 Runtime Ring Backed Texture Upload v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime ring-backed texture upload"
    Assert-ContainsText $recommendedText "Runtime Buffer Ring-Backed Uploads v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime buffer ring-backed uploads"
    Assert-ContainsText $recommendedText "RuntimeMeshUploadOptions::upload_ring" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime buffer ring-backed uploads"
    Assert-ContainsText $recommendedText "RuntimeSkinnedMeshUploadOptions::upload_ring" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime buffer ring-backed uploads"
    Assert-ContainsText $recommendedText "RuntimeMorphMeshUploadOptions::upload_ring" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime buffer ring-backed uploads"
    Assert-ContainsText $recommendedText "Upload Staging v1 Runtime Buffer Ring Backed Uploads v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime buffer ring-backed uploads"
    Assert-ContainsText $recommendedText "Package Static Mesh Upload Binding Transaction v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package static mesh upload transaction"
    Assert-ContainsText $recommendedText "RuntimePackageStreamingMeshUploadSource" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package static mesh upload transaction"
    Assert-ContainsText $recommendedText "RuntimePackageStreamingMeshUploadBindingResult" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package static mesh upload transaction"
    Assert-ContainsText $recommendedText "upload_runtime_package_streaming_mesh_gpu_bindings" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package static mesh upload transaction"
    Assert-ContainsText $recommendedText "Upload Staging v1 Package Static Mesh Upload Binding Transaction v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan package static mesh upload transaction"
    Assert-ContainsText $recommendedText "Runtime Upload Queue Wait v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime upload queue wait"
    Assert-ContainsText $recommendedText "wait_for_runtime_uploads_on_queue" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime upload queue wait"
    Assert-ContainsText $recommendedText "upload_queue_waits_recorded" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime upload queue wait"
    Assert-ContainsText $recommendedText "Upload Staging v1 Runtime Upload Queue Wait v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan runtime upload queue wait"
    Assert-ContainsText $recommendedText "Staging Pool Lease Adoption v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan staging pool lease adoption"
    Assert-ContainsText $recommendedText "RhiStagingBufferLease" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan staging pool lease adoption"
    Assert-ContainsText $recommendedText "RhiUploadRingDesc::buffer" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan staging pool lease adoption"
    Assert-ContainsText $recommendedText "Upload Staging v1 Staging Pool Lease Adoption v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan staging pool lease adoption"
    Assert-ContainsText $recommendedText "Frame Graph Automatic Aliasing Barrier Insertion v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan automatic aliasing barrier"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Render Pass Envelope v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "render_passes_recorded" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph RHI Queue Dependency Plan v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "plan_frame_graph_rhi_queue_waits" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "IRhiDevice::wait_for_queue" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph RHI Multi-Queue Executor v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph RHI Multi-Queue Texture Barrier Execution v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "FrameGraphRhiMultiQueueExecutionResult::barriers_recorded" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Multi-Queue Automatic Aliasing Barrier Execution v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "FrameGraphRhiMultiQueueExecutionDesc::transient_texture_lifetimes" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "FrameGraphRhiMultiQueueExecutionResult::aliasing_barriers_recorded" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "FrameGraphRhiMultiQueuePackageEvidence" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "framegraph_multiqueue" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "alias-induced cross-queue waits" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Public Null Aliasing Barriers v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Render Pass Stats Evidence v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "RendererStats::framegraph_render_passes_recorded" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "execute_frame_graph_rhi_multi_queue_schedule" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Remaining Render Pass Envelopes v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "RhiFrameRenderer primary_color" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "RhiViewportSurface viewport.clear" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph v1 1.0 Scope Closeout v1 closes frame-graph-v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "broad production render graph scheduling" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Metal memory alias allocation" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan.completedContext"
}
if ($recommendedPlanId -eq "general-purpose-game-production-v1") {
    foreach ($needle in @(
            "General Purpose Game Production v1",
            "gameplay-runtime-scheduler-production-v1",
            "world-entity-model-production-v1",
            "addressable-content-streaming-production-v1",
            "production-authoring-workflows-v1",
            "production-runtime-ui-workbench-v1",
            "unsupportedProductionGaps empty"
        )) {
        Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan production milestone"
    }
} elseif ($recommendedPlanId -eq "generated-game-studio-v1") {
    Assert-ContainsText $recommendedText "Generated Game Studio v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "EditorAiGeneratedGameStudioV1Model" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "EditorAiCommandPanelModel" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "ai-generated-game-playtest-loop-v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "ai-validation-remediation-recipes-v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "unsupportedProductionGaps empty" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
} elseif ($recommendedPlanId -eq "engine-1-0-gap-matrix-v1") {
    Assert-ContainsText $recommendedText "Engine 1.0 Gap Matrix v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "Generated Game Studio v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "implemented-1x-foundation" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "renderer-backend-parity-v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "strict Vulkan evidence" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "Metal remains Apple-host-gated" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "unsupportedProductionGaps empty" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "broad commercial-engine" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
} elseif ($recommendedPlanId -eq "next-production-gap-selection") {
    foreach ($needle in @("First-Party Desktop Platform And SDL3 Removal v1", "MK_platform_win32", "MK_runtime_host_win32_presentation", "MK_audio_wasapi", "Job Execution Placement Policy v1", "job_execution_placement_policy_status=ready", "host-independent CPU placement policy evidence", "Windows CPU Set Worker Placement v1", "windows_cpu_set_worker_placement_status=ready", "windows_cpu_set_worker_placement_native_thread_handles_exposed=0", "unsupportedProductionGaps = []", "selection gate")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan selection gate" }
} elseif ($recommendedPlanId -eq "physics-navigation-commercial-coverage-v1") {
    Assert-ContainsText $recommendedText "Physics Navigation Commercial Coverage v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan physics/navigation selection"
    Assert-ContainsText $recommendedText "Jolt/Recast/Detour-class" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan physics/navigation selection"
    Assert-ContainsText $recommendedText "adapter_boundary_id" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan physics/navigation selection"
    Assert-ContainsText $recommendedText "host_validation_recipe_id" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan physics/navigation selection"
    Assert-ContainsText $recommendedText "adapter_lifecycle_reviewed" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan physics/navigation selection"
    Assert-ContainsText $recommendedText "unsupportedProductionGaps = []" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan physics/navigation selection"
    Assert-ContainsText $recommendedText "native handles hidden" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan physics/navigation selection"
    Assert-ContainsText $recommendedText "broad middleware parity fail-closed" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan physics/navigation selection"
} elseif ($recommendedPlanId -eq "renderer-backend-parity-metal-apple-evidence-v1") {
    foreach ($needle in @("Renderer Backend Parity Metal Apple Evidence v1", "renderer-backend-parity-v1", "metal-apple remains host-gated", "shader-toolchain", "mobile-packaging", "ios-simulator-smoke", "Apple/Metal host evidence", "Windows/Vulkan proof must not promote Metal readiness", "no SDL3", "native handles remain hidden", "unsupportedProductionGaps = []")) {
        Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan renderer Metal Apple selection"
    }
} elseif ($recommendedPlanId -eq "renderer-postprocess-tone-mapping-evidence-v1") { foreach ($needle in @("Renderer Postprocess Tone Mapping Evidence v1", "renderer-postprocess-v1", "PostprocessToneMappingEvidencePlan", "plan_postprocess_tone_mapping_evidence", "D3D12/Vulkan", "Metal host-gated", "no SDL3", "native handles", "subjective visual quality", "unsupportedProductionGaps = []")) {
        Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan renderer postprocess tone mapping selection"
    }
} elseif ($recommendedPlanId -eq "job-execution-work-stealing-v1") { foreach ($needle in @("Job Execution Work Stealing v1", "opt-in JobExecutionPool", "work_stealing_enabled", "JobExecutionTopologyPolicyDesc.enable_work_stealing", "steal attempt/success/wait counters", "--require-job-execution-work-stealing", "job_execution_work_stealing_status=ready", "job_execution_work_stealing_applied=1", "deterministic publish order", "unsupportedProductionGaps = []", "affinity", "NUMA", "SMT/hybrid", "SIMD", "GPU async overlap", "CUDA/HIP/SYCL", "broad all-core CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan job execution work stealing selection" }
} elseif ($recommendedPlanId -eq "job-execution-placement-policy-v1") { foreach ($needle in @("Job Execution Placement Policy v1", "host-independent CPU placement policy evidence", "topology", "worker-pool", "work-stealing", "affinity", "NUMA", "SMT", "hybrid-core", "Windows CPU Set", "Linux affinity", "SIMD", "GPU async", "CUDA/HIP/SYCL", "broad all-core CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan job execution placement policy selection" }
} elseif ($recommendedPlanId -eq "windows-cpu-set-worker-placement-v1") { foreach ($needle in @("Windows CPU Set Worker Placement v1", "Windows CPU Sets", "worker-start placement", "value-only MK_core callback", "MK_platform_win32 adapter", "sample_desktop_runtime_game", "unsupportedProductionGaps = []", "native handles", "Linux affinity", "NUMA allocation execution", "hybrid P-core/E-core", "SMT scheduling", "SIMD dispatch", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan Windows CPU Set worker placement selection" }
} elseif ($recommendedPlanId -eq "windows-cpu-set-smt-worker-placement-v1") { foreach ($needle in @("Windows CPU Set SMT Worker Placement v1", "avoid_smt_siblings", "CoreIndex", "distinct cores", "SMT sibling", "--require-windows-cpu-set-smt-worker-placement", "windows_cpu_set_smt_worker_placement_status=ready", "smt_policy_applied=1", "unsupportedProductionGaps = []", "native handles", "hybrid P-core/E-core", "Linux affinity", "NUMA allocation execution", "SIMD dispatch", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan Windows CPU Set SMT worker placement selection" }
} elseif ($recommendedPlanId -eq "avx2-reviewed-target-execution-v1") { foreach ($needle in @("AVX2 Reviewed Target Execution v1", "target-local AVX2 OBJECT", "mirakana_core_avx2", "simd_dispatch_avx2.cpp", "CpuSimdFeatureSet", "avx2_compile_supported", "avx2_runtime_supported", "auto_select", "sample_desktop_runtime_game --require-simd-dispatch-policy", "unsupportedProductionGaps = []", "global /arch:AVX2", "NUMA allocation execution", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan AVX2 reviewed target execution selection" }
} elseif ($recommendedPlanId -eq "simd-dispatch-policy-and-evidence-v1") { foreach ($needle in @("SIMD Dispatch Policy And Evidence v1", "scalar/SSE2", "CPU SIMD dispatch", "simd_dispatch_policy_*", "Intel and AMD x86/x64", "AVX2 behind compile/runtime gates", "span-based inputs", "raw pointers non-owning", "unsupportedProductionGaps = []", "ARM NEON", "NUMA allocation execution", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan SIMD dispatch policy selection" }
} elseif ($recommendedPlanId -eq "mavg-asset-graph-v1") { foreach ($needle in @("MAVG Asset Graph v1", "MAVG Phase 0 completed", "deterministic MK_assets MAVG cluster graph validation", "mavg_cluster_graph.hpp", "MavgClusterGraphDocument", "GameEngine.MavgClusterGraph.v1", "AssetKind::mavg_cluster_graph", "mavg_source_mesh", "mavg_material", "MK_tools cook/package planning", "mavg_cluster_cook.hpp", "MavgClusterCookRequest", "plan_mavg_cluster_graph_cook_package", "apply_mavg_cluster_graph_cook_package", "no SDL3/Dear ImGui", "no public native handles", "no Nanite/UE compatibility", "CPU selection", "renderer execution", "streaming/residency", "unsupportedProductionGaps = []")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan MAVG asset graph selection" }
} elseif ($recommendedPlanId -eq "mavg-runtime-lod-milestone-v1" -or $recommendedPlanId -eq "mavg-package-streaming-residency-dispatch-v1" -or $recommendedPlanId -eq "mavg-page-addressable-payload-schema-v1" -or $recommendedPlanId -eq "mavg-payload-byte-range-file-io-v1" -or $recommendedPlanId -eq "mavg-directstorage-request-plan-v1" -or $recommendedPlanId -eq "mavg-native-directstorage-win32-async-io-dispatch-status-v1" -or $recommendedPlanId -eq "mavg-win32-async-file-io-adapter-v1" -or $recommendedPlanId -eq "mavg-win32-iocp-file-io-worker-v1") { foreach ($needle in @("MAVG Runtime LOD Milestone v1", "MAVG Asset Graph v1", "MAVG Phase 0 completed", "hierarchy/error/fallback", "MavgClusterGraphCluster", "resident_fallback_cluster_index", "geometric_error", "first_index", "index_count", "vertex_base", "GameEngine.MavgClusterGraph.v1", "MavgClusterCookVertex", "GameEngine.MavgClusterPayload.v1", "vertex.data_hex", "index.data_hex", "page.data_hex", "per-material root/leaf", "mavg_cluster_payload.hpp", "MavgClusterPayloadDocument", "MavgClusterPayloadPage", "validate_mavg_cluster_payload", "mavg_payload_pages.hpp", "RuntimeMavgPayloadPageSliceResult", "extract_runtime_mavg_payload_page_slices", "mavg_lod_selection.hpp", "MavgLodViewDesc", "MavgLodResidentPageSet", "MavgLodSelectionResult", "CPU LOD selection", "select_mavg_lod_clusters", "MK_mavg_lod_selection_tests", "mavg_lod_residency.hpp", "RuntimeMavgLodResidencyDesc", "RuntimeMavgLodResidencyResult", "build_runtime_mavg_lod_residency", "MK_runtime_mavg_lod_residency_tests", "range-aware conventional indexed draws", "mavg_scene_lod.hpp", "MavgSceneLodSubmitDesc", "MavgSceneLodSubmitResult", "plan_mavg_scene_lod_mesh_commands", "MK_scene_renderer_mavg_lod_tests", "RuntimeMavgPageStreamingDispatchPlan", "plan_runtime_mavg_page_streaming_dispatches", "RuntimeMavgPageStreamingDrainDesc", "GPU culling", "indirect draw execution", "mesh shaders", "Nanite compatibility/equivalence/superiority", "unsupportedProductionGaps = []")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan MAVG runtime LOD selection" }
} elseif ($recommendedPlanId -eq "sandbox-world-network-modding-gate-v1") { foreach ($needle in @("Selected focused child plan", "sandbox-world-specific mutation replication", "reviewed modding policy gates", "unsupportedProductionGaps = []", "Broad online multiplayer", "SDL3", "native handle exposure")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan sandbox world network/modding selection" } } elseif ($recommendedPlanId -eq "sandbox-world-package-validation-performance-budgets-v1") { foreach ($needle in @("Selected focused child plan", "sample package smoke flags", "installed validation", "package-visible counters", "--require-sandbox-package-budgets", "sandbox_package_budget_*", "unsupportedProductionGaps = []", "broad renderer quality", "package mutation", "SDL3", "native handle exposure")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan sandbox world package validation and performance budget selection" } } elseif ($recommendedPlanId -eq "ai-operable-performance-budget-and-evidence-v1") { foreach ($needle in @("performanceBudgets", "budgetRows", "evidenceRows", "validation recipe", "unsupported broad optimization claims", "CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan performance budget evidence selection" } } elseif ($recommendedPlanId -eq "performance-baseline-v1") { foreach ($needle in @("reproducible benchmark scenes/packages", "trace export recipes", "subsystem counters", "p95/p99 frame budget reporting", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan performance baseline selection" } } elseif ($recommendedPlanId -eq "long-running-performance-readiness-v1-phase-1") { foreach ($needle in @("Long-Running Performance Readiness v1", "sample_2d_desktop_runtime_package", "--require-long-run-performance-readiness", "long_run_readiness_status=ready", "host-2d-long-run-readiness-soak", "Linux affinity", "NUMA", "broad SIMD", "GPU async overlap", "CUDA", "HIP", "SYCL", "unsupportedProductionGaps = []")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan long-running performance readiness selection" } } elseif ($recommendedPlanId -eq "long-running-performance-readiness-v1-phase-2") { foreach ($needle in @("Intel/AMD CPU Profiling Matrix v1", "cpuProfilingMatrix", "host-cpu-profiling-matrix", "representative Intel/AMD host classes", "before/after trace", "regression budgets", "Linux affinity", "NUMA", "broader SIMD", "PGO/LTO", "data-layout", "host-gated", "unsupportedProductionGaps = []")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan CPU profiling matrix selection" } } elseif ($recommendedPlanId -eq "long-running-performance-readiness-v1-phase-7") { foreach ($needle in @("Optional GPU Compute Review v1", "optionalGpuComputeReview", "host-optional-gpu-compute-review", "rhi_compute", "offline_tool_acceleration", "cuda_hip_private_adapter_candidate", "sycl_private_adapter_candidate", "non_goal", "data transfer cost", "memory residency", "synchronization", "stream/event usage", "queue/profiler visibility", "dependency burden", "scalar or RHI fallback", "CUDA/HIP/SYCL runtime dependency", "vcpkg.json", "CMake", "default validation", "broad GPU compute", "async overlap", "cross-vendor", "cross-backend", "broad CPU/GPU/memory optimization", "unsupportedProductionGaps = []")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan optional GPU compute review selection" } } elseif ($recommendedPlanId -eq "memory-lifetime-taxonomy-v1") { foreach ($needle in @("memory lifetime taxonomy", "ownership semantics", "raw pointers are non-owning", "std::unique_ptr", "std::span", "allocator/job/NUMA/GPU memory", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan memory lifetime taxonomy selection" } } elseif ($recommendedPlanId -eq "memory-diagnostics-v1") { foreach ($needle in @("memory diagnostics", "memory class counters", "high-water marks", "budget pressure", "stale-generation", "use-after-safe-point", "allocator replacement", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan memory diagnostics selection" } } elseif ($recommendedPlanId -eq "frame-thread-scratch-v1") { foreach ($needle in @("frame temporary", "worker scratch", "first-party frame arenas", "per-worker scratch arenas", "explicit ownership APIs", "high-water marks", "false-sharing diagnostics", "allocator replacement", "all-core CPU scheduling", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan frame/thread scratch selection" } } elseif ($recommendedPlanId -eq "job-scheduling-evidence-v1") { foreach ($needle in @("job scheduling", "worker topology", "bounded job queues", "deterministic job/scratch evidence", "queue/steal/wait/merge diagnostics", "work stealing", "processor groups", "package-visible job_scheduling_evidence_* counters", "all-core CPU scheduling", "affinity pinning", "NUMA placement", "SIMD dispatch", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan job scheduling evidence selection" } } elseif ($recommendedPlanId -eq "job-execution-worker-pool-v1") { foreach ($needle in @("Job Execution Worker Pool v1", "persistent worker-thread pool", "explicit worker_count", "bounded worker queues", "std::thread", "JobExecutionStopToken", "execute(batch)", "worker-local ScratchArena", "deterministic publish order", "JobSchedulingExecutionEvidence", "--require-job-execution-foundation", "job_execution_foundation_status=ready", "job_execution_foundation_worker_threads_started=2", "unsupportedProductionGaps = []", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan job execution worker pool selection" } } elseif ($recommendedPlanId -eq "job-execution-topology-policy-v1") { foreach ($needle in @("Job Execution Topology Policy v1", "portable MK_core worker-count selection", "JobExecutionTopologyPolicyDesc", "select_job_execution_topology_policy", "observe_job_execution_logical_processor_count", "derived JobExecutionPoolDesc", "fallback/cap/reserve rules", "processor-group and NUMA host-evidence diagnostics", "--require-job-execution-topology-policy", "job_execution_topology_policy_status=ready", "job_execution_topology_policy_ready=1", "selected worker count = 2", "unsupportedProductionGaps = []", "broad all-core CPU/GPU/memory optimization")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan job execution topology policy selection" } } elseif ($recommendedPlanId -eq "first-party-ui-editor-production-stack-v1") { foreach ($needle in @("First-Party UI Editor Production Stack v1", "MK_editor", "MK_editor_core", "desktop-editor", "mirakana::ui", "MK_ui_renderer", "dock graph", "rich text", "DirectWrite", "selected text atlas handoff evidence", "Text Services Framework", "UI Automation", "D3D12 viewport/material texture display", "AI-operable", "editor.cross_platform.adapter.macos.core_text", "editor.cross_platform.adapter.dependency.harfbuzz", "compatibility shims", "unsupportedProductionGaps = []", "SDL3", "native handles", "Dear ImGui")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan first-party UI editor production selection" } } elseif ($recommendedPlanId -eq "first-party-editor-shell-v1") { foreach ($needle in @("First-Party Editor Shell v1", "MK_editor", "MK_editor_core", "first-party retained", "desktop-editor", "mirakana::ui", "MK_ui_renderer", "EditorAiOperationSnapshot", "EditorAiCommandCatalog", "EditorAiCommandRequest", "EditorAiCommandDryRunResult", "EditorAiCommandApplyResult", "dock graph", "rich-text", "adapter-boundary diagnostics", "DirectWrite", "Text Services Framework", "UI Automation", "unsupportedProductionGaps = []", "SDL3", "native handles", "Dear ImGui")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan first-party editor shell selection" } } elseif ($recommendedPlanId -eq "native-win32-editor-shell-v1") { foreach ($needle in @("Native Win32 Editor Shell v1", "MK_editor", "Dear ImGui", "Direct3D 12", "desktop-gui", "PR #316", "unsupportedProductionGaps = []", "SDL3", "native handles", "editor/developer-shell")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan native editor shell selection" } } elseif ($recommendedPlanId -eq "environment-system-v1") { foreach ($needle in @("Environment System v1", "MK_environment", "EnvironmentProfileDesc", "validate_environment_profile", "official docs/Context7", "sky", "sun/moon", "fog", "clouds", "rain/snow/storm", "time-of-day", "quality tiers", "D3D12", "strict Vulkan", "Metal host-gated", "unsupportedProductionGaps = []", "broad environment_ready", "native handles", "Dear ImGui", "SDL3", "OpenEXR/KTX")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan environment system selection" } } elseif ($recommendedPlanId -eq "mavg-research-legal-benchmark-baseline-v1") { foreach ($needle in @("MAVG Phase 0", "research/specification", "official-source checks", "clean-room/legal guardrails", "benchmark methodology", "stale-doc cleanup", "MAVG not implemented", "no SDL3/Dear ImGui", "no public native handles", "no Nanite/UE compatibility", "unsupportedProductionGaps = []")) { Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan MAVG Phase 0 selection" } } else {
    Assert-ContainsText $recommendedText "Frame Graph v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "upload-staging-v1" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "scene-component-prefab-schema-v2" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "2d-playable-vertical-slice" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
    Assert-ContainsText $recommendedText "3d-playable-vertical-slice" "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan"
}
if ($recommendedPlanId -eq "first-party-ui-editor-production-stack-v1") {
    foreach ($needle in @(
            "EditorAiOperationSnapshot.status_rows",
            "editor.ai.dock.selected_panel",
            "editor.ai.material_preview.display",
            "<rich_text_document_id>.copy_selection_plain_text",
            "validation-recipe execution",
            "screen coordinates",
            "editor.cross_platform.adapter.macos.core_text",
            "editor.cross_platform.adapter.linux.at_spi",
            "editor.cross_platform.adapter.android.input_method_service",
            "editor.cross_platform.adapter.ios.uitextinput",
            "editor.cross_platform.adapter.dependency.harfbuzz",
            "license-audit", "THIRD_PARTY_NOTICES.md"
        )) {
        Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan first-party UI editor AI UX operation rows"
    }
}
if ($recommendedPlanUsesLegacyCloseoutContext) {
    foreach ($needle in @(
            "editor-productization",
            "reviewed editor authoring/playtest/AI command/resource/input/prefab/material-preview evidence",
            "explicit host-gated exclusion of Vulkan/Metal material-preview display parity",
            "production-ui-importer-platform-adapters",
            "reviewed runtime UI adapter contracts",
            "selected first-party platform text-input/event/clipboard bridge evidence",
            "reviewed PNG/UI atlas/glyph atlas package bridges",
            "explicit future/dependency-gated exclusions for broad low-level UI, codec, importer, platform SDK",
            "full-repository-quality-gate"
        )) {
        Assert-ContainsText $recommendedText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan closeout wedges"
    }
}
