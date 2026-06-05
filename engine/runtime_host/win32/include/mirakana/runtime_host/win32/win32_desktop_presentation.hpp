// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/win32/win32_window.hpp"
#include "mirakana/renderer/cloud_layer_policy.hpp"
#include "mirakana/renderer/environment_fog_policy.hpp"
#include "mirakana/renderer/physical_sky_policy.hpp"
#include "mirakana/renderer/precipitation_policy.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/renderer/volumetric_fog_policy.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

namespace runtime {
class RuntimeAssetPackage;
}

struct SceneRenderPacket;

enum class Win32DesktopPresentationBackend : std::uint8_t {
    null_renderer = 0,
    d3d12,
    vulkan,
};

enum class Win32DesktopPresentationFallbackReason : std::uint8_t {
    none = 0,
    native_window_unavailable,
    native_backend_unavailable,
    runtime_pipeline_unavailable,
};

enum class Win32DesktopPresentationBackendReportStatus : std::uint8_t {
    not_requested = 0,
    missing_request,
    native_window_unavailable,
    native_backend_unavailable,
    runtime_pipeline_unavailable,
    ready,
};

enum class Win32DesktopPresentationPresentStatus : std::uint8_t {
    not_requested = 0,
    ready,
    failed,
};

enum class Win32DesktopPresentationResizeStatus : std::uint8_t {
    not_requested = 0,
    ready,
    failed,
    recreate_required,
};

struct Win32DesktopPresentationDiagnostic {
    Win32DesktopPresentationFallbackReason reason{Win32DesktopPresentationFallbackReason::none};
    std::string message;
};

struct Win32DesktopPresentationBackendReport {
    Win32DesktopPresentationBackend backend{Win32DesktopPresentationBackend::null_renderer};
    Win32DesktopPresentationBackendReportStatus status{Win32DesktopPresentationBackendReportStatus::not_requested};
    Win32DesktopPresentationFallbackReason fallback_reason{Win32DesktopPresentationFallbackReason::none};
    std::string diagnostic;
};

enum class Win32DesktopPresentationSceneGpuBindingStatus : std::uint8_t {
    not_requested = 0,
    unavailable,
    invalid_request,
    failed,
    ready,
};

struct Win32DesktopPresentationSceneGpuBindingDiagnostic {
    Win32DesktopPresentationSceneGpuBindingStatus status{Win32DesktopPresentationSceneGpuBindingStatus::not_requested};
    std::string message;
};

struct Win32DesktopPresentationSceneGpuBindingStats {
    std::size_t mesh_bindings{0};
    std::size_t skinned_mesh_bindings{0};
    std::size_t morph_mesh_bindings{0};
    std::size_t compute_morph_mesh_bindings{0};
    std::size_t compute_morph_mesh_dispatches{0};
    std::size_t compute_morph_queue_waits{0};
    std::size_t compute_morph_mesh_draws{0};
    std::uint64_t compute_morph_async_compute_queue_submits{0};
    std::uint64_t compute_morph_async_graphics_queue_submits{0};
    std::uint64_t compute_morph_async_graphics_queue_waits{0};
    std::uint64_t compute_morph_async_last_compute_submitted_fence_value{0};
    std::uint64_t compute_morph_async_last_graphics_queue_wait_fence_value{0};
    std::uint64_t compute_morph_async_last_graphics_submitted_fence_value{0};
    std::size_t compute_morph_skinned_mesh_bindings{0};
    std::size_t compute_morph_skinned_mesh_dispatches{0};
    std::size_t compute_morph_skinned_queue_waits{0};
    std::size_t compute_morph_skinned_mesh_draws{0};
    std::size_t material_bindings{0};
    std::size_t mesh_uploads{0};
    std::size_t skinned_mesh_uploads{0};
    std::size_t morph_mesh_uploads{0};
    std::size_t texture_uploads{0};
    std::size_t material_uploads{0};
    std::size_t material_pipeline_layouts{0};
    std::uint64_t uploaded_texture_bytes{0};
    std::uint64_t uploaded_mesh_bytes{0};
    std::uint64_t uploaded_morph_bytes{0};
    std::uint64_t uploaded_material_factor_bytes{0};
    std::size_t mesh_bindings_resolved{0};
    std::size_t skinned_mesh_bindings_resolved{0};
    std::size_t morph_mesh_bindings_resolved{0};
    std::size_t compute_morph_mesh_bindings_resolved{0};
    std::size_t compute_morph_skinned_mesh_bindings_resolved{0};
    std::size_t material_bindings_resolved{0};
    std::uint64_t compute_morph_output_position_bytes{0};
};

enum class Win32DesktopPresentationPostprocessStatus : std::uint8_t {
    not_requested = 0,
    unavailable,
    invalid_request,
    failed,
    ready,
};

struct Win32DesktopPresentationPostprocessDiagnostic {
    Win32DesktopPresentationPostprocessStatus status{Win32DesktopPresentationPostprocessStatus::not_requested};
    std::string message;
};

enum class Win32DesktopPresentationDirectionalShadowStatus : std::uint8_t {
    not_requested = 0,
    unavailable,
    invalid_request,
    failed,
    ready,
};

enum class Win32DesktopPresentationDirectionalShadowFilterMode : std::uint8_t {
    none = 0,
    fixed_pcf_3x3,
};

struct Win32DesktopPresentationDirectionalShadowDiagnostic {
    Win32DesktopPresentationDirectionalShadowStatus status{
        Win32DesktopPresentationDirectionalShadowStatus::not_requested};
    std::string message;
};

enum class Win32DesktopPresentationNativeUiOverlayStatus : std::uint8_t {
    not_requested = 0,
    unavailable,
    invalid_request,
    failed,
    ready,
};

struct Win32DesktopPresentationNativeUiOverlayDiagnostic {
    Win32DesktopPresentationNativeUiOverlayStatus status{Win32DesktopPresentationNativeUiOverlayStatus::not_requested};
    std::string message;
};

enum class Win32DesktopPresentationNativeUiTextureOverlayStatus : std::uint8_t {
    not_requested = 0,
    unavailable,
    invalid_request,
    failed,
    ready,
};

struct Win32DesktopPresentationNativeUiTextureOverlayDiagnostic {
    Win32DesktopPresentationNativeUiTextureOverlayStatus status{
        Win32DesktopPresentationNativeUiTextureOverlayStatus::not_requested};
    std::string message;
};

struct Win32D3d12SwapChainPlanDesc {
    Extent2D extent;
    rhi::Format format{rhi::Format::bgra8_unorm};
    std::uint32_t buffer_count{2};
    bool vsync{true};
    bool request_tearing{false};
    bool tearing_supported{false};
};

struct Win32D3d12SwapChainPlan {
    Extent2D extent;
    rhi::Format format{rhi::Format::bgra8_unorm};
    std::uint32_t buffer_count{2};
    bool uses_create_swap_chain_for_hwnd{false};
    bool uses_direct_command_queue{false};
    bool flip_discard_swap_effect{false};
    bool render_target_output{false};
    bool resize_buffers_supported{false};
    bool requires_present_state_before_present{false};
    bool allow_tearing_flag{false};
    std::uint32_t present_sync_interval{1};
    bool public_native_handles_exposed{false};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct Win32DesktopPresentationReport {
    Win32DesktopPresentationBackend requested_backend{Win32DesktopPresentationBackend::null_renderer};
    Win32DesktopPresentationBackend selected_backend{Win32DesktopPresentationBackend::null_renderer};
    Win32DesktopPresentationFallbackReason fallback_reason{Win32DesktopPresentationFallbackReason::none};
    bool used_null_fallback{false};
    bool allow_null_fallback{true};
    Win32D3d12SwapChainPlan swapchain_plan;
    bool d3d12_tearing_requested{false};
    bool d3d12_tearing_supported{false};
    bool d3d12_tearing_active{false};
    Win32DesktopPresentationPresentStatus present_status{Win32DesktopPresentationPresentStatus::not_requested};
    Win32DesktopPresentationResizeStatus resize_status{Win32DesktopPresentationResizeStatus::not_requested};
    Win32DesktopPresentationSceneGpuBindingStatus scene_gpu_status{
        Win32DesktopPresentationSceneGpuBindingStatus::not_requested};
    Win32DesktopPresentationSceneGpuBindingStats scene_gpu_stats;
    Win32DesktopPresentationPostprocessStatus postprocess_status{
        Win32DesktopPresentationPostprocessStatus::not_requested};
    bool postprocess_depth_input_requested{false};
    bool postprocess_depth_input_ready{false};
    bool environment_fog_requested{false};
    bool environment_fog_constant_buffer_ready{false};
    std::uint64_t environment_fog_constant_buffer_bytes{0};
    bool environment_fog_vulkan_package_requested{false};
    bool environment_fog_vulkan_package_shader_contract_evidence_ready{false};
    bool environment_fog_vulkan_package_evidence_ready{false};
    bool environment_fog_vulkan_package_constant_buffer_ready{false};
    std::uint64_t environment_fog_vulkan_package_constant_buffer_bytes{0};
    bool physical_sky_requested{false};
    bool physical_sky_shader_contract_evidence_ready{false};
    bool physical_sky_package_evidence_ready{false};
    bool physical_sky_execution_evidence_ready{false};
    bool physical_sky_constant_buffer_ready{false};
    std::uint64_t physical_sky_constant_buffer_bytes{0};
    bool physical_sky_allocates_lut_textures{false};
    bool physical_sky_invokes_backend{false};
    bool physical_sky_exposes_native_handles{false};
    std::uint32_t physical_sky_constant_layout_rows{0};
    std::uint32_t physical_sky_lut_intent_rows{0};
    std::uint32_t physical_sky_policy_diagnostics_count{0};
    bool cloud_layer_requested{false};
    bool cloud_layer_shader_contract_evidence_ready{false};
    bool cloud_layer_package_evidence_ready{false};
    bool cloud_layer_execution_evidence_ready{false};
    bool cloud_layer_uploads_textures{false};
    bool cloud_layer_invokes_backend{false};
    bool cloud_layer_exposes_native_handles{false};
    bool cloud_layer_uses_volumetric_clouds{false};
    bool cloud_layer_uses_latlong_projection{false};
    bool cloud_layer_uses_flow_map{false};
    std::uint32_t cloud_layer_texture_rows{0};
    std::uint32_t cloud_layer_visual_rows{0};
    std::uint32_t cloud_layer_ibl_rows{0};
    std::uint32_t cloud_layer_shader_contract_rows{0};
    std::uint32_t cloud_layer_quality_rows{0};
    std::uint32_t cloud_layer_policy_diagnostics_count{0};
    std::uint64_t cloud_layer_renderer_draws{0};
    bool environment_precipitation_requested{false};
    EnvironmentWeatherKind environment_precipitation_weather{EnvironmentWeatherKind::clear};
    EnvironmentPrecipitationKind environment_precipitation_kind{EnvironmentPrecipitationKind::none};
    bool environment_precipitation_shader_contract_evidence_ready{false};
    bool environment_precipitation_package_evidence_ready{false};
    bool environment_precipitation_execution_evidence_ready{false};
    bool environment_precipitation_uploads_particle_buffers{false};
    bool environment_precipitation_invokes_backend{false};
    bool environment_precipitation_exposes_native_handles{false};
    bool environment_precipitation_mutates_materials{false};
    bool environment_precipitation_plays_audio{false};
    bool environment_precipitation_uses_camera_near_particles{false};
    bool environment_precipitation_uses_scene_depth_occlusion{false};
    std::uint32_t environment_precipitation_weather_rows{0};
    std::uint32_t environment_precipitation_particle_rows{0};
    std::uint32_t environment_precipitation_occlusion_rows{0};
    std::uint32_t environment_precipitation_wetness_rows{0};
    std::uint32_t environment_precipitation_audio_handoff_rows{0};
    std::uint32_t environment_precipitation_shader_rows{0};
    std::uint32_t environment_precipitation_quality_rows{0};
    std::uint32_t environment_precipitation_policy_diagnostics_count{0};
    bool environment_volumetric_fog_requested{false};
    bool environment_volumetric_fog_shader_contract_evidence_ready{false};
    bool environment_volumetric_fog_package_evidence_ready{false};
    bool environment_volumetric_fog_execution_evidence_ready{false};
    bool environment_volumetric_fog_froxel_output_ready{false};
    bool environment_volumetric_fog_scene_depth_ready{false};
    std::uint64_t environment_volumetric_fog_compute_dispatches{0};
    bool environment_volumetric_fog_exposes_native_handles{false};
    std::uint32_t environment_volumetric_fog_policy_diagnostics_count{0};
    Win32DesktopPresentationDirectionalShadowStatus directional_shadow_status{
        Win32DesktopPresentationDirectionalShadowStatus::not_requested};
    bool directional_shadow_requested{false};
    bool directional_shadow_ready{false};
    Win32DesktopPresentationDirectionalShadowFilterMode directional_shadow_filter_mode{
        Win32DesktopPresentationDirectionalShadowFilterMode::none};
    std::uint32_t directional_shadow_filter_tap_count{0};
    float directional_shadow_filter_radius_texels{0.0F};
    std::uint32_t directional_shadow_cascade_count{0};
    std::uint32_t directional_shadow_cascade_tile_width{0};
    std::uint32_t directional_shadow_atlas_width{0};
    std::uint32_t directional_shadow_atlas_height{0};
    std::uint32_t directional_shadow_light_space_cascades{0};
    std::uint32_t directional_shadow_cascade_splits{0};
    Win32DesktopPresentationNativeUiOverlayStatus native_ui_overlay_status{
        Win32DesktopPresentationNativeUiOverlayStatus::not_requested};
    bool native_ui_overlay_requested{false};
    bool native_ui_overlay_ready{false};
    std::uint64_t native_ui_overlay_sprites_submitted{0};
    std::uint64_t native_ui_overlay_draws{0};
    Win32DesktopPresentationNativeUiTextureOverlayStatus native_ui_texture_overlay_status{
        Win32DesktopPresentationNativeUiTextureOverlayStatus::not_requested};
    bool native_ui_texture_overlay_requested{false};
    bool native_ui_texture_overlay_atlas_ready{false};
    std::uint64_t native_ui_texture_overlay_sprites_submitted{0};
    std::uint64_t native_ui_texture_overlay_texture_binds{0};
    std::uint64_t native_ui_texture_overlay_draws{0};
    std::uint64_t rhi_instanced_draw_calls{0};
    std::uint64_t rhi_instanced_indexed_draw_calls{0};
    std::uint64_t rhi_instanced_instances_submitted{0};
    rhi::RhiDeviceMemoryDiagnostics rhi_memory_diagnostics;
    std::uint64_t rhi_transient_heap_allocations{0};
    std::uint64_t rhi_transient_placed_allocations{0};
    std::uint64_t rhi_transient_placed_resources_alive{0};
    std::uint64_t rhi_bytes_written{0};
    std::uint64_t rhi_gpu_timestamp_ticks_per_second{0};
    std::uint64_t rhi_gpu_debug_scopes_begun{0};
    std::uint64_t rhi_gpu_debug_scopes_ended{0};
    std::uint64_t rhi_gpu_debug_markers_inserted{0};
    std::size_t framegraph_passes{0};
    RendererStats renderer_stats;
    rhi::RhiStats rhi_stats;
    Extent2D backbuffer_extent;
    std::size_t diagnostics_count{0};
    std::size_t backend_reports_count{0};
    std::size_t scene_gpu_diagnostics_count{0};
    std::size_t postprocess_diagnostics_count{0};
    std::size_t directional_shadow_diagnostics_count{0};
    std::size_t native_ui_overlay_diagnostics_count{0};
    std::size_t native_ui_texture_overlay_diagnostics_count{0};
};

enum class Win32DesktopPresentationQualityGateStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationPostprocessPolicyStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationSceneScalePolicyStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationD3d12PostprocessExecutionStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationEnvironmentFogStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationVulkanEnvironmentFogPackageStatus : std::uint8_t {
    not_requested = 0,
    host_evidence_required,
    blocked,
    ready,
};

enum class Win32DesktopPresentationPhysicalSkyStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationCloudLayerStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationEnvironmentPrecipitationStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationEnvironmentVolumetricFogStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationD3d12InstancedDrawExecutionStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

struct Win32DesktopPresentationPostprocessPolicyReport {
    Win32DesktopPresentationPostprocessPolicyStatus status{
        Win32DesktopPresentationPostprocessPolicyStatus::not_requested};
    bool ready{false};
    std::uint32_t diagnostics_count{0};
    std::uint32_t effect_count{0};
    std::uint32_t postprocess_pass_count{0};
    std::uint32_t framegraph_pass_count{0};
    std::uint32_t framegraph_barrier_step_budget{0};
    bool scene_color_required{false};
    bool scene_depth_required{false};
    bool bloom_work_texture_required{false};
    bool tone_mapping_effect{false};
    bool exposure_effect{false};
    bool bloom_effect{false};
    bool color_grading_effect{false};
    bool fog_effect{false};
    bool anti_aliasing_effect{false};
    bool backend_shader_evidence_ready{false};
};

struct Win32DesktopPresentationSceneScalePolicyDesc {
    bool require_scene_gpu_bindings{false};
    bool require_backend_instancing_evidence{false};
    bool backend_instancing_evidence_ready{false};
    bool require_performance_measurement{false};
    bool performance_measurement_ready{false};
    std::uint64_t expected_frames{0};
    std::uint32_t max_draw_group_count{128};
    std::uint32_t max_visible_instance_count{16'384};
    std::uint32_t max_draw_call_count{4'096};
};

struct Win32DesktopPresentationSceneScalePolicyReport {
    Win32DesktopPresentationSceneScalePolicyStatus status{
        Win32DesktopPresentationSceneScalePolicyStatus::not_requested};
    bool ready{false};
    bool scene_resources_ready{false};
    bool frames_current{false};
    bool backend_instancing_evidence_required{false};
    bool backend_instancing_evidence_ready{false};
    bool performance_measurement_required{false};
    bool performance_measurement_ready{false};
    std::uint64_t expected_frames{0};
    std::uint64_t frames_finished{0};
    std::uint32_t diagnostics_count{0};
    std::uint32_t draw_group_count{0};
    std::uint32_t static_mesh_draw_groups{0};
    std::uint32_t skinned_mesh_draw_groups{0};
    std::uint32_t morph_mesh_draw_groups{0};
    std::uint32_t sprite_draw_groups{0};
    std::uint64_t requested_instance_count{0};
    std::uint64_t visible_instance_count{0};
    std::uint64_t culled_instance_count{0};
    std::uint32_t draw_call_count{0};
    std::uint32_t instanced_draw_call_count{0};
    std::uint64_t instanced_visible_instance_count{0};
    std::uint32_t lod_group_count{0};
    std::uint32_t cpu_culling_group_count{0};
};

struct Win32DesktopPresentationD3d12PostprocessExecutionReport {
    Win32DesktopPresentationD3d12PostprocessExecutionStatus status{
        Win32DesktopPresentationD3d12PostprocessExecutionStatus::not_requested};
    bool ready{false};
    bool d3d12_backend_selected{false};
    bool postprocess_ready{false};
    bool backend_shader_evidence_ready{false};
    std::uint64_t expected_postprocess_passes{0};
    std::uint64_t postprocess_passes_executed{0};
    std::uint64_t framegraph_passes_executed{0};
    std::uint64_t framegraph_render_passes_recorded{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    bool postprocess_passes_current{false};
};

struct Win32DesktopPresentationEnvironmentFogReport {
    Win32DesktopPresentationEnvironmentFogStatus status{Win32DesktopPresentationEnvironmentFogStatus::not_requested};
    bool ready{false};
    bool requested{false};
    bool d3d12_backend_selected{false};
    bool postprocess_ready{false};
    bool postprocess_depth_input_ready{false};
    bool d3d12_postprocess_execution_ready{false};
    bool constant_buffer_ready{false};
    std::uint32_t constants_binding{0};
    std::uint64_t constant_buffer_bytes{0};
    std::uint64_t expected_postprocess_passes{0};
    std::uint64_t postprocess_passes_executed{0};
    bool postprocess_passes_current{false};
    std::uint32_t diagnostics_count{0};
};

struct Win32DesktopPresentationVulkanEnvironmentFogPackageReport {
    Win32DesktopPresentationVulkanEnvironmentFogPackageStatus status{
        Win32DesktopPresentationVulkanEnvironmentFogPackageStatus::not_requested};
    bool ready{false};
    bool requested{false};
    bool vulkan_backend_selected{false};
    bool postprocess_ready{false};
    bool postprocess_depth_input_ready{false};
    bool vulkan_postprocess_execution_ready{false};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool constant_buffer_ready{false};
    std::uint32_t constants_binding{0};
    std::uint64_t constant_buffer_bytes{0};
    std::uint64_t expected_postprocess_passes{0};
    std::uint64_t postprocess_passes_executed{0};
    bool postprocess_passes_current{false};
    bool exposes_native_handles{false};
    std::uint32_t diagnostics_count{0};
};

struct Win32DesktopPresentationPhysicalSkyReport {
    Win32DesktopPresentationPhysicalSkyStatus status{Win32DesktopPresentationPhysicalSkyStatus::not_requested};
    bool ready{false};
    bool requested{false};
    bool d3d12_backend_selected{false};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool constant_buffer_ready{false};
    std::uint32_t constants_binding{0};
    std::uint64_t constant_buffer_bytes{0};
    std::uint32_t constant_layout_rows{0};
    std::uint32_t lut_intent_rows{0};
    bool allocates_lut_textures{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    std::uint32_t diagnostics_count{0};
};

struct Win32DesktopPresentationCloudLayerReport {
    Win32DesktopPresentationCloudLayerStatus status{Win32DesktopPresentationCloudLayerStatus::not_requested};
    bool ready{false};
    bool requested{false};
    bool d3d12_backend_selected{false};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool execution_evidence_ready{false};
    std::uint32_t cloud_map_binding{0};
    std::uint32_t flow_map_binding{0};
    std::uint32_t sampler_binding{0};
    std::uint32_t constants_binding{0};
    bool uses_latlong_projection{false};
    bool uses_flow_map{false};
    std::uint32_t texture_rows{0};
    std::uint32_t visual_rows{0};
    std::uint32_t ibl_rows{0};
    std::uint32_t shader_contract_rows{0};
    std::uint32_t quality_rows{0};
    bool uploads_textures{false};
    bool invokes_backend{false};
    std::uint64_t renderer_draws{0};
    bool exposes_native_handles{false};
    bool uses_volumetric_clouds{false};
    std::uint32_t diagnostics_count{0};
};

struct Win32DesktopPresentationEnvironmentPrecipitationReport {
    Win32DesktopPresentationEnvironmentPrecipitationStatus status{
        Win32DesktopPresentationEnvironmentPrecipitationStatus::not_requested};
    bool ready{false};
    bool requested{false};
    bool d3d12_backend_selected{false};
    EnvironmentWeatherKind weather{EnvironmentWeatherKind::clear};
    EnvironmentPrecipitationKind kind{EnvironmentPrecipitationKind::none};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool execution_evidence_ready{false};
    std::uint32_t particle_texture_binding{0};
    std::uint32_t scene_depth_texture_binding{0};
    std::uint32_t sampler_binding{0};
    std::uint32_t constants_binding{0};
    bool uses_camera_near_particles{false};
    bool uses_scene_depth_occlusion{false};
    std::uint32_t weather_rows{0};
    std::uint32_t particle_rows{0};
    std::uint32_t occlusion_rows{0};
    std::uint32_t wetness_rows{0};
    std::uint32_t audio_handoff_rows{0};
    std::uint32_t shader_rows{0};
    std::uint32_t quality_rows{0};
    bool uploads_particle_buffers{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool mutates_materials{false};
    bool plays_audio{false};
    std::uint32_t diagnostics_count{0};
};

struct Win32DesktopPresentationEnvironmentPrecipitationExpectation {
    EnvironmentWeatherKind weather{EnvironmentWeatherKind::storm};
    EnvironmentPrecipitationKind kind{EnvironmentPrecipitationKind::rain};
    std::uint32_t wetness_rows{1};
    std::uint32_t minimum_audio_handoff_rows{1};
};

struct Win32DesktopPresentationEnvironmentVolumetricFogReport {
    Win32DesktopPresentationEnvironmentVolumetricFogStatus status{
        Win32DesktopPresentationEnvironmentVolumetricFogStatus::not_requested};
    bool ready{false};
    bool requested{false};
    bool d3d12_backend_selected{false};
    bool scene_depth_ready{false};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool froxel_output_ready{false};
    std::uint64_t compute_dispatches{0};
    std::uint32_t constants_binding{0};
    std::uint64_t constant_buffer_bytes{0};
    std::uint32_t froxel_output_buffer_binding{0};
    bool exposes_native_handles{false};
    std::uint32_t diagnostics_count{0};
};

enum class Win32DesktopPresentationVulkanPostprocessExecutionStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

struct Win32DesktopPresentationVulkanPostprocessExecutionReport {
    Win32DesktopPresentationVulkanPostprocessExecutionStatus status{
        Win32DesktopPresentationVulkanPostprocessExecutionStatus::not_requested};
    bool ready{false};
    bool vulkan_backend_selected{false};
    bool postprocess_ready{false};
    bool backend_shader_evidence_ready{false};
    std::uint64_t expected_postprocess_passes{0};
    std::uint64_t postprocess_passes_executed{0};
    std::uint64_t framegraph_passes_executed{0};
    std::uint64_t framegraph_render_passes_recorded{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    bool postprocess_passes_current{false};
};

struct Win32DesktopPresentationD3d12InstancedDrawExecutionReport {
    Win32DesktopPresentationD3d12InstancedDrawExecutionStatus status{
        Win32DesktopPresentationD3d12InstancedDrawExecutionStatus::not_requested};
    bool ready{false};
    bool d3d12_backend_selected{false};
    std::uint64_t expected_instances_submitted{0};
    std::uint64_t instanced_draw_calls{0};
    std::uint64_t instanced_indexed_draw_calls{0};
    std::uint64_t instanced_instances_submitted{0};
    bool instanced_draws_current{false};
    bool instanced_instances_current{false};
};

enum class Win32DesktopPresentationVulkanInstancedDrawExecutionStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

struct Win32DesktopPresentationVulkanInstancedDrawExecutionReport {
    Win32DesktopPresentationVulkanInstancedDrawExecutionStatus status{
        Win32DesktopPresentationVulkanInstancedDrawExecutionStatus::not_requested};
    bool ready{false};
    bool vulkan_backend_selected{false};
    std::uint64_t expected_instances_submitted{0};
    std::uint64_t instanced_draw_calls{0};
    std::uint64_t instanced_indexed_draw_calls{0};
    std::uint64_t instanced_instances_submitted{0};
    bool instanced_draws_current{false};
    bool instanced_instances_current{false};
};

enum class Win32DesktopPresentationGpuMemoryPolicyStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationD3d12GpuMemoryExecutionStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationVulkanGpuMemoryExecutionStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

struct Win32DesktopPresentationGpuMemoryPolicyDesc {
    bool require_scene_gpu_bindings{false};
    bool require_backend_memory_evidence{false};
    bool backend_memory_evidence_ready{false};
    bool require_os_video_memory_budget{false};
    bool require_declared_budget_evidence{false};
    bool require_residency_pressure_evidence{false};
    bool require_package_counter_evidence{false};
    std::uint64_t expected_frames{0};
    std::uint64_t declared_local_budget_bytes{0};
    std::uint64_t residency_pressure_event_count{0};
    bool package_counter_evidence_ready{false};
};

struct Win32DesktopPresentationGpuMemoryPolicyReport {
    Win32DesktopPresentationGpuMemoryPolicyStatus status{Win32DesktopPresentationGpuMemoryPolicyStatus::not_requested};
    bool ready{false};
    bool scene_resources_ready{false};
    bool frames_current{false};
    bool backend_memory_evidence_required{false};
    bool backend_memory_evidence_ready{false};
    bool os_video_memory_budget_required{false};
    bool os_video_memory_budget_available{false};
    bool memory_budget_evidence_ready{false};
    bool residency_pressure_evidence_ready{false};
    bool package_counter_evidence_ready{false};
    std::uint64_t expected_frames{0};
    std::uint64_t frames_finished{0};
    std::uint32_t diagnostics_count{0};
    std::uint32_t request_count{0};
    std::uint64_t total_requested_bytes{0};
    std::uint64_t total_counted_bytes{0};
    std::uint64_t os_local_budget_bytes{0};
    std::uint64_t os_local_usage_bytes{0};
    std::uint64_t committed_byte_estimate{0};
    std::uint64_t transient_heap_allocations{0};
    std::uint64_t transient_placed_allocations{0};
    std::uint64_t transient_placed_resources_alive{0};
    std::uint64_t upload_bytes_written{0};
    std::uint64_t residency_pressure_event_count{0};
    std::uint32_t transient_heap_request_count{0};
    std::uint32_t upload_pressure_request_count{0};
    std::uint32_t declared_budget_request_count{0};
    std::uint32_t residency_pressure_request_count{0};
    std::uint32_t package_counter_request_count{0};
};

struct Win32DesktopPresentationD3d12GpuMemoryExecutionReport {
    Win32DesktopPresentationD3d12GpuMemoryExecutionStatus status{
        Win32DesktopPresentationD3d12GpuMemoryExecutionStatus::not_requested};
    bool ready{false};
    bool d3d12_backend_selected{false};
    bool os_video_memory_budget_available{false};
    bool committed_byte_estimate_available{false};
    std::uint64_t local_video_memory_budget_bytes{0};
    std::uint64_t local_video_memory_usage_bytes{0};
    std::uint64_t committed_resources_byte_estimate{0};
    std::uint64_t transient_heap_allocations{0};
    std::uint64_t transient_placed_allocations{0};
    std::uint64_t transient_placed_resources_alive{0};
    std::uint64_t upload_bytes_written{0};
    bool memory_budget_current{false};
    bool transient_heap_current{false};
};

struct Win32DesktopPresentationVulkanGpuMemoryExecutionReport {
    Win32DesktopPresentationVulkanGpuMemoryExecutionStatus status{
        Win32DesktopPresentationVulkanGpuMemoryExecutionStatus::not_requested};
    bool ready{false};
    bool vulkan_backend_selected{false};
    bool committed_byte_estimate_available{false};
    std::uint64_t committed_resources_byte_estimate{0};
    std::uint64_t transient_heap_allocations{0};
    std::uint64_t transient_placed_allocations{0};
    std::uint64_t transient_placed_resources_alive{0};
    std::uint64_t upload_bytes_written{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    bool memory_budget_current{false};
    bool transient_heap_current{false};
};

enum class Win32DesktopPresentationDebugProfilingPolicyStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationD3d12DebugProfilingExecutionStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

enum class Win32DesktopPresentationVulkanDebugProfilingExecutionStatus : std::uint8_t {
    not_requested = 0,
    blocked,
    ready,
};

struct Win32DesktopPresentationDebugProfilingPolicyDesc {
    bool require_scene_gpu_bindings{false};
    bool require_backend_profiling_evidence{false};
    bool backend_profiling_evidence_ready{false};
    bool require_cpu_profile_zone_evidence{false};
    bool require_trace_capture_handoff_evidence{false};
    bool require_package_counter_evidence{false};
    std::uint64_t expected_frames{0};
    std::uint64_t cpu_profile_zone_count{0};
    std::uint64_t trace_capture_handoff_row_count{0};
    bool package_counter_evidence_ready{false};
};

struct Win32DesktopPresentationDebugProfilingPolicyReport {
    Win32DesktopPresentationDebugProfilingPolicyStatus status{
        Win32DesktopPresentationDebugProfilingPolicyStatus::not_requested};
    bool ready{false};
    bool scene_resources_ready{false};
    bool frames_current{false};
    bool backend_profiling_evidence_required{false};
    bool backend_profiling_evidence_ready{false};
    std::uint64_t expected_frames{0};
    std::uint64_t frames_finished{0};
    std::uint32_t diagnostics_count{0};
    std::uint32_t request_count{0};
    std::uint64_t gpu_timestamp_ticks_per_second{0};
    std::uint64_t gpu_debug_scopes_begun{0};
    std::uint64_t gpu_debug_scopes_ended{0};
    std::uint64_t gpu_debug_markers_inserted{0};
    std::uint64_t cpu_profile_zone_count{0};
    std::uint64_t trace_capture_handoff_row_count{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    std::uint64_t framegraph_render_passes_recorded{0};
    std::uint32_t gpu_timestamp_request_count{0};
    std::uint32_t gpu_debug_marker_request_count{0};
    std::uint32_t capture_handoff_request_count{0};
    std::uint32_t cpu_profile_zone_request_count{0};
    std::uint32_t trace_capture_handoff_request_count{0};
    std::uint32_t package_counter_request_count{0};
    bool cpu_profile_zone_evidence_ready{false};
    bool trace_capture_handoff_evidence_ready{false};
    bool package_counter_evidence_ready{false};
};

struct Win32DesktopPresentationD3d12DebugProfilingExecutionReport {
    Win32DesktopPresentationD3d12DebugProfilingExecutionStatus status{
        Win32DesktopPresentationD3d12DebugProfilingExecutionStatus::not_requested};
    bool ready{false};
    bool d3d12_backend_selected{false};
    std::uint64_t gpu_timestamp_ticks_per_second{0};
    std::uint64_t gpu_debug_scopes_begun{0};
    std::uint64_t gpu_debug_scopes_ended{0};
    std::uint64_t gpu_debug_markers_inserted{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    std::uint64_t framegraph_render_passes_recorded{0};
    bool gpu_timestamps_current{false};
    bool gpu_debug_markers_current{false};
    bool frame_diagnostics_current{false};
};

struct Win32DesktopPresentationVulkanDebugProfilingExecutionReport {
    Win32DesktopPresentationVulkanDebugProfilingExecutionStatus status{
        Win32DesktopPresentationVulkanDebugProfilingExecutionStatus::not_requested};
    bool ready{false};
    bool vulkan_backend_selected{false};
    std::uint64_t gpu_timestamp_ticks_per_second{0};
    std::uint64_t gpu_debug_scopes_begun{0};
    std::uint64_t gpu_debug_scopes_ended{0};
    std::uint64_t gpu_debug_markers_inserted{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    std::uint64_t framegraph_render_passes_recorded{0};
    bool gpu_timestamps_current{false};
    bool gpu_debug_markers_current{false};
    bool frame_diagnostics_current{false};
};

struct Win32DesktopPresentationQualityGateDesc {
    bool require_scene_gpu_bindings{false};
    bool require_postprocess{false};
    bool require_postprocess_depth_input{false};
    bool require_directional_shadow{false};
    bool require_directional_shadow_filtering{false};
    std::uint64_t expected_frames{0};
};

struct Win32DesktopPresentationQualityGateReport {
    Win32DesktopPresentationQualityGateStatus status{Win32DesktopPresentationQualityGateStatus::not_requested};
    bool ready{false};
    bool scene_gpu_ready{false};
    bool postprocess_ready{false};
    bool postprocess_depth_input_ready{false};
    bool directional_shadow_ready{false};
    bool directional_shadow_filter_ready{false};
    std::uint32_t expected_framegraph_passes{0};
    std::uint64_t expected_framegraph_render_passes{0};
    std::uint64_t expected_framegraph_barrier_steps{0};
    bool framegraph_passes_current{false};
    bool framegraph_render_passes_current{false};
    bool framegraph_barrier_steps_current{false};
    bool framegraph_execution_budget_current{false};
    std::uint32_t diagnostics_count{0};
};

struct Win32DesktopPresentationShaderBytecode {
    std::string_view entry_point;
    std::span<const std::uint8_t> bytecode;
};

struct Win32DesktopPresentationSceneMorphMeshBinding {
    AssetId mesh;
    AssetId morph_mesh;
};

struct Win32DesktopPresentationD3d12RendererDesc {
    Win32DesktopPresentationShaderBytecode vertex_shader;
    Win32DesktopPresentationShaderBytecode fragment_shader;
    Win32DesktopPresentationShaderBytecode native_sprite_overlay_vertex_shader;
    Win32DesktopPresentationShaderBytecode native_sprite_overlay_fragment_shader;
    const runtime::RuntimeAssetPackage* native_sprite_overlay_package{nullptr};
    AssetId native_sprite_overlay_atlas_asset;
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
    bool enable_native_sprite_overlay{false};
    bool enable_native_sprite_overlay_textures{false};
};

struct Win32DesktopPresentationVulkanRendererDesc {
    Win32DesktopPresentationShaderBytecode vertex_shader;
    Win32DesktopPresentationShaderBytecode fragment_shader;
    Win32DesktopPresentationShaderBytecode native_sprite_overlay_vertex_shader;
    Win32DesktopPresentationShaderBytecode native_sprite_overlay_fragment_shader;
    const runtime::RuntimeAssetPackage* native_sprite_overlay_package{nullptr};
    AssetId native_sprite_overlay_atlas_asset;
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
    bool enable_native_sprite_overlay{false};
    bool enable_native_sprite_overlay_textures{false};
};

struct Win32DesktopPresentationD3d12SceneRendererDesc {
    Win32DesktopPresentationShaderBytecode vertex_shader;
    Win32DesktopPresentationShaderBytecode fragment_shader;
    /// Optional skinned mesh scene vertex shader bytecode (`vs_skinned` in `runtime_scene.hlsl`).
    Win32DesktopPresentationShaderBytecode skinned_vertex_shader;
    /// Optional morph mesh scene vertex shader bytecode (`vs_morph` in generated package shaders).
    Win32DesktopPresentationShaderBytecode morph_vertex_shader;
    /// Optional compute morph POSITION-only scene vertex shader bytecode (`vs_compute_morph` in generated package
    /// shaders).
    Win32DesktopPresentationShaderBytecode compute_morph_vertex_shader;
    /// Optional D3D12 compute shader bytecode (`cs_compute_morph_position`) that writes morphed POSITION bytes.
    Win32DesktopPresentationShaderBytecode compute_morph_shader;
    /// Optional D3D12 compute shader bytecode (`cs_compute_morph_skinned_position`) for skinned mesh POSITION output.
    Win32DesktopPresentationShaderBytecode compute_morph_skinned_shader;
    /// Optional directional shadow scene fragment bytecode (`ps_shadow_receiver`) compiled so shadow resources use the
    /// descriptor set after deformation-specific sets. Required when directional shadow smoke is true and the scene
    /// pass uses skinned or graphics morph descriptors at set index 1.
    Win32DesktopPresentationShaderBytecode skinned_scene_fragment_shader;
    Win32DesktopPresentationShaderBytecode postprocess_vertex_shader;
    Win32DesktopPresentationShaderBytecode postprocess_fragment_shader;
    Win32DesktopPresentationShaderBytecode shadow_vertex_shader;
    Win32DesktopPresentationShaderBytecode shadow_fragment_shader;
    Win32DesktopPresentationShaderBytecode native_ui_overlay_vertex_shader;
    Win32DesktopPresentationShaderBytecode native_ui_overlay_fragment_shader;
    Win32DesktopPresentationShaderBytecode cloud_layer_vertex_shader;
    Win32DesktopPresentationShaderBytecode cloud_layer_fragment_shader;
    const runtime::RuntimeAssetPackage* package{nullptr};
    const SceneRenderPacket* packet{nullptr};
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
    std::vector<rhi::VertexBufferLayoutDesc> skinned_vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> skinned_vertex_attributes;
    std::vector<AssetId> morph_mesh_assets;
    std::vector<Win32DesktopPresentationSceneMorphMeshBinding> morph_mesh_bindings;
    std::vector<Win32DesktopPresentationSceneMorphMeshBinding> compute_morph_mesh_bindings;
    std::vector<Win32DesktopPresentationSceneMorphMeshBinding> compute_morph_skinned_mesh_bindings;
    bool enable_compute_morph_tangent_frame_output{false};
    bool enable_postprocess{false};
    bool enable_postprocess_depth_input{false};
    bool enable_environment_fog{false};
    EnvironmentFogPolicyDesc environment_fog;
    bool enable_physical_sky_package_evidence{false};
    PhysicalSkyPolicyDesc physical_sky;
    bool enable_cloud_layer_package_evidence{false};
    bool enable_cloud_layer_renderer_execution{false};
    CloudLayerPolicyDesc cloud_layer;
    bool enable_environment_precipitation_package_evidence{false};
    PrecipitationPolicyDesc environment_precipitation;
    bool enable_environment_volumetric_fog_package_evidence{false};
    VolumetricFogPolicyDesc environment_volumetric_fog;
    bool enable_directional_shadow_smoke{false};
    bool enable_native_ui_overlay{false};
    AssetId native_ui_overlay_atlas_asset;
    bool enable_native_ui_overlay_textures{false};
};

struct Win32DesktopPresentationVulkanSceneRendererDesc {
    Win32DesktopPresentationShaderBytecode vertex_shader;
    Win32DesktopPresentationShaderBytecode fragment_shader;
    Win32DesktopPresentationShaderBytecode skinned_vertex_shader;
    Win32DesktopPresentationShaderBytecode morph_vertex_shader;
    /// Optional Vulkan compute morph scene vertex SPIR-V (`vs_compute_morph` or `vs_compute_morph_tangent_frame` in
    /// generated package shaders).
    Win32DesktopPresentationShaderBytecode compute_morph_vertex_shader;
    /// Optional Vulkan compute shader SPIR-V (`cs_compute_morph_position` or `cs_compute_morph_tangent_frame`) that
    /// writes morphed vertex stream bytes when compute morph bindings are selected. When provided without compute morph
    /// bindings, the runtime host may use it only as the Vulkan IRhiDevice mapping compute-dispatch proof.
    Win32DesktopPresentationShaderBytecode compute_morph_shader;
    /// Optional Vulkan compute shader SPIR-V (`cs_compute_morph_skinned_position`) for skinned mesh POSITION output.
    Win32DesktopPresentationShaderBytecode compute_morph_skinned_shader;
    /// See `Win32DesktopPresentationD3d12SceneRendererDesc::skinned_scene_fragment_shader`.
    Win32DesktopPresentationShaderBytecode skinned_scene_fragment_shader;
    Win32DesktopPresentationShaderBytecode postprocess_vertex_shader;
    Win32DesktopPresentationShaderBytecode postprocess_fragment_shader;
    Win32DesktopPresentationShaderBytecode shadow_vertex_shader;
    Win32DesktopPresentationShaderBytecode shadow_fragment_shader;
    Win32DesktopPresentationShaderBytecode native_ui_overlay_vertex_shader;
    Win32DesktopPresentationShaderBytecode native_ui_overlay_fragment_shader;
    const runtime::RuntimeAssetPackage* package{nullptr};
    const SceneRenderPacket* packet{nullptr};
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
    std::vector<rhi::VertexBufferLayoutDesc> skinned_vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> skinned_vertex_attributes;
    std::vector<AssetId> morph_mesh_assets;
    std::vector<Win32DesktopPresentationSceneMorphMeshBinding> morph_mesh_bindings;
    std::vector<Win32DesktopPresentationSceneMorphMeshBinding> compute_morph_mesh_bindings;
    std::vector<Win32DesktopPresentationSceneMorphMeshBinding> compute_morph_skinned_mesh_bindings;
    bool enable_compute_morph_tangent_frame_output{false};
    bool enable_postprocess{false};
    bool enable_postprocess_depth_input{false};
    bool enable_environment_fog{false};
    EnvironmentFogPolicyDesc environment_fog;
    bool enable_directional_shadow_smoke{false};
    bool enable_native_ui_overlay{false};
    AssetId native_ui_overlay_atlas_asset;
    bool enable_native_ui_overlay_textures{false};
};

struct Win32DesktopPresentationDesc {
    win32::Win32Window* window{nullptr};
    Extent2D extent;
    bool prefer_d3d12{true};
    bool prefer_vulkan{false};
    bool allow_null_fallback{true};
    bool prefer_warp{false};
    bool enable_debug_layer{false};
    bool vsync{true};
    bool request_tearing{false};
    const Win32DesktopPresentationD3d12RendererDesc* d3d12_renderer{nullptr};
    const Win32DesktopPresentationVulkanRendererDesc* vulkan_renderer{nullptr};
    const Win32DesktopPresentationD3d12SceneRendererDesc* d3d12_scene_renderer{nullptr};
    const Win32DesktopPresentationVulkanSceneRendererDesc* vulkan_scene_renderer{nullptr};
};

class Win32DesktopPresentation final {
  public:
    explicit Win32DesktopPresentation(const Win32DesktopPresentationDesc& desc);
    ~Win32DesktopPresentation();

    Win32DesktopPresentation(const Win32DesktopPresentation&) = delete;
    Win32DesktopPresentation& operator=(const Win32DesktopPresentation&) = delete;
    Win32DesktopPresentation(Win32DesktopPresentation&&) = delete;
    Win32DesktopPresentation& operator=(Win32DesktopPresentation&&) = delete;

    [[nodiscard]] IRenderer& renderer() noexcept;
    [[nodiscard]] const IRenderer& renderer() const noexcept;
    [[nodiscard]] Win32DesktopPresentationBackend backend() const noexcept;
    [[nodiscard]] std::string_view backend_name() const noexcept;
    [[nodiscard]] Win32DesktopPresentationReport report() const noexcept;
    [[nodiscard]] std::span<const Win32DesktopPresentationBackendReport> backend_reports() const noexcept;
    [[nodiscard]] std::span<const Win32DesktopPresentationDiagnostic> diagnostics() const noexcept;
    [[nodiscard]] Win32DesktopPresentationSceneGpuBindingStatus scene_gpu_binding_status() const noexcept;
    [[nodiscard]] bool scene_gpu_bindings_ready() const noexcept;
    [[nodiscard]] Win32DesktopPresentationSceneGpuBindingStats scene_gpu_binding_stats() const noexcept;
    [[nodiscard]] rhi::BufferHandle scene_pbr_frame_uniform_buffer() const noexcept;
    [[nodiscard]] rhi::IRhiDevice* scene_pbr_frame_rhi_device() noexcept;
    [[nodiscard]] const rhi::IRhiDevice* scene_pbr_frame_rhi_device() const noexcept;
    [[nodiscard]] std::span<const Win32DesktopPresentationSceneGpuBindingDiagnostic>
    scene_gpu_binding_diagnostics() const noexcept;
    [[nodiscard]] Win32DesktopPresentationPostprocessStatus postprocess_status() const noexcept;
    [[nodiscard]] bool postprocess_ready() const noexcept;
    [[nodiscard]] bool postprocess_depth_input_ready() const noexcept;
    [[nodiscard]] std::span<const Win32DesktopPresentationPostprocessDiagnostic>
    postprocess_diagnostics() const noexcept;
    [[nodiscard]] Win32DesktopPresentationDirectionalShadowStatus directional_shadow_status() const noexcept;
    [[nodiscard]] bool directional_shadow_ready() const noexcept;
    [[nodiscard]] std::span<const Win32DesktopPresentationDirectionalShadowDiagnostic>
    directional_shadow_diagnostics() const noexcept;
    [[nodiscard]] Win32DesktopPresentationNativeUiOverlayStatus native_ui_overlay_status() const noexcept;
    [[nodiscard]] bool native_ui_overlay_ready() const noexcept;
    [[nodiscard]] std::span<const Win32DesktopPresentationNativeUiOverlayDiagnostic>
    native_ui_overlay_diagnostics() const noexcept;
    [[nodiscard]] Win32DesktopPresentationNativeUiTextureOverlayStatus
    native_ui_texture_overlay_status() const noexcept;
    [[nodiscard]] bool native_ui_texture_overlay_atlas_ready() const noexcept;
    [[nodiscard]] std::span<const Win32DesktopPresentationNativeUiTextureOverlayDiagnostic>
    native_ui_texture_overlay_diagnostics() const noexcept;

  private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] Win32D3d12SwapChainPlan plan_win32_d3d12_swapchain(const Win32D3d12SwapChainPlanDesc& desc);
[[nodiscard]] std::string_view
win32_desktop_presentation_backend_name(Win32DesktopPresentationBackend backend) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_backend_report_status_name(Win32DesktopPresentationBackendReportStatus status) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_fallback_reason_name(Win32DesktopPresentationFallbackReason reason) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_present_status_name(Win32DesktopPresentationPresentStatus status) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_resize_status_name(Win32DesktopPresentationResizeStatus status) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_scene_gpu_binding_status_name(Win32DesktopPresentationSceneGpuBindingStatus status) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_postprocess_status_name(Win32DesktopPresentationPostprocessStatus status) noexcept;
[[nodiscard]] std::string_view win32_desktop_presentation_directional_shadow_status_name(
    Win32DesktopPresentationDirectionalShadowStatus status) noexcept;
[[nodiscard]] std::string_view win32_desktop_presentation_directional_shadow_filter_mode_name(
    Win32DesktopPresentationDirectionalShadowFilterMode mode) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_native_ui_overlay_status_name(Win32DesktopPresentationNativeUiOverlayStatus status) noexcept;
[[nodiscard]] std::string_view win32_desktop_presentation_native_ui_texture_overlay_status_name(
    Win32DesktopPresentationNativeUiTextureOverlayStatus status) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_quality_gate_status_name(Win32DesktopPresentationQualityGateStatus status) noexcept;
[[nodiscard]] std::string_view win32_desktop_presentation_postprocess_policy_status_name(
    Win32DesktopPresentationPostprocessPolicyStatus status) noexcept;
[[nodiscard]] std::string_view win32_desktop_presentation_scene_scale_policy_status_name(
    Win32DesktopPresentationSceneScalePolicyStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationPostprocessPolicyReport
evaluate_win32_desktop_presentation_postprocess_policy(const Win32DesktopPresentationReport& report);
[[nodiscard]] Win32DesktopPresentationSceneScalePolicyReport
evaluate_win32_desktop_presentation_scene_scale_policy(const Win32DesktopPresentationReport& report,
                                                       const Win32DesktopPresentationSceneScalePolicyDesc& desc);
[[nodiscard]] std::string_view win32_desktop_presentation_d3d12_postprocess_execution_status_name(
    Win32DesktopPresentationD3d12PostprocessExecutionStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationD3d12PostprocessExecutionReport
evaluate_win32_desktop_presentation_d3d12_postprocess_execution(const Win32DesktopPresentationReport& report,
                                                                std::uint64_t expected_postprocess_passes,
                                                                bool requested);
[[nodiscard]] std::string_view
win32_desktop_presentation_environment_fog_status_name(Win32DesktopPresentationEnvironmentFogStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationEnvironmentFogReport evaluate_win32_desktop_presentation_environment_fog(
    const Win32DesktopPresentationReport& report,
    const Win32DesktopPresentationD3d12PostprocessExecutionReport& d3d12_postprocess_execution, bool requested);
[[nodiscard]] std::string_view win32_desktop_presentation_vulkan_environment_fog_package_status_name(
    Win32DesktopPresentationVulkanEnvironmentFogPackageStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationVulkanEnvironmentFogPackageReport
evaluate_win32_desktop_presentation_vulkan_environment_fog_package(
    const Win32DesktopPresentationReport& report,
    const Win32DesktopPresentationVulkanPostprocessExecutionReport& vulkan_postprocess_execution, bool requested);
[[nodiscard]] std::string_view
win32_desktop_presentation_physical_sky_status_name(Win32DesktopPresentationPhysicalSkyStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationPhysicalSkyReport
evaluate_win32_desktop_presentation_physical_sky(const Win32DesktopPresentationReport& report, bool requested);
[[nodiscard]] std::string_view
win32_desktop_presentation_cloud_layer_status_name(Win32DesktopPresentationCloudLayerStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationCloudLayerReport
evaluate_win32_desktop_presentation_cloud_layer(const Win32DesktopPresentationReport& report, bool requested,
                                                bool require_renderer_execution = false);
[[nodiscard]] std::string_view win32_desktop_presentation_environment_precipitation_status_name(
    Win32DesktopPresentationEnvironmentPrecipitationStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationEnvironmentPrecipitationReport
evaluate_win32_desktop_presentation_environment_precipitation(
    const Win32DesktopPresentationReport& report, bool requested,
    Win32DesktopPresentationEnvironmentPrecipitationExpectation expectation);
[[nodiscard]] std::string_view win32_desktop_presentation_environment_volumetric_fog_status_name(
    Win32DesktopPresentationEnvironmentVolumetricFogStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationEnvironmentVolumetricFogReport
evaluate_win32_desktop_presentation_environment_volumetric_fog(const Win32DesktopPresentationReport& report,
                                                               bool requested);
[[nodiscard]] std::string_view win32_desktop_presentation_vulkan_postprocess_execution_status_name(
    Win32DesktopPresentationVulkanPostprocessExecutionStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationVulkanPostprocessExecutionReport
evaluate_win32_desktop_presentation_vulkan_postprocess_execution(const Win32DesktopPresentationReport& report,
                                                                 std::uint64_t expected_postprocess_passes,
                                                                 bool requested);
[[nodiscard]] std::string_view win32_desktop_presentation_d3d12_instanced_draw_execution_status_name(
    Win32DesktopPresentationD3d12InstancedDrawExecutionStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationD3d12InstancedDrawExecutionReport
evaluate_win32_desktop_presentation_d3d12_instanced_draw_execution(const Win32DesktopPresentationReport& report,
                                                                   std::uint64_t expected_instances_submitted);
[[nodiscard]] std::string_view win32_desktop_presentation_vulkan_instanced_draw_execution_status_name(
    Win32DesktopPresentationVulkanInstancedDrawExecutionStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationVulkanInstancedDrawExecutionReport
evaluate_win32_desktop_presentation_vulkan_instanced_draw_execution(const Win32DesktopPresentationReport& report,
                                                                    std::uint64_t expected_instances_submitted);
[[nodiscard]] std::string_view
win32_desktop_presentation_gpu_memory_policy_status_name(Win32DesktopPresentationGpuMemoryPolicyStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationGpuMemoryPolicyReport
evaluate_win32_desktop_presentation_gpu_memory_policy(const Win32DesktopPresentationReport& report,
                                                      const Win32DesktopPresentationGpuMemoryPolicyDesc& desc);
[[nodiscard]] std::string_view win32_desktop_presentation_d3d12_gpu_memory_execution_status_name(
    Win32DesktopPresentationD3d12GpuMemoryExecutionStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationD3d12GpuMemoryExecutionReport
evaluate_win32_desktop_presentation_d3d12_gpu_memory_execution(const Win32DesktopPresentationReport& report,
                                                               bool requested);
[[nodiscard]] std::string_view win32_desktop_presentation_vulkan_gpu_memory_execution_status_name(
    Win32DesktopPresentationVulkanGpuMemoryExecutionStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationVulkanGpuMemoryExecutionReport
evaluate_win32_desktop_presentation_vulkan_gpu_memory_execution(const Win32DesktopPresentationReport& report,
                                                                bool requested);
[[nodiscard]] std::string_view win32_desktop_presentation_debug_profiling_policy_status_name(
    Win32DesktopPresentationDebugProfilingPolicyStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationDebugProfilingPolicyReport
evaluate_win32_desktop_presentation_debug_profiling_policy(
    const Win32DesktopPresentationReport& report, const Win32DesktopPresentationDebugProfilingPolicyDesc& desc);
[[nodiscard]] std::string_view win32_desktop_presentation_d3d12_debug_profiling_execution_status_name(
    Win32DesktopPresentationD3d12DebugProfilingExecutionStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationD3d12DebugProfilingExecutionReport
evaluate_win32_desktop_presentation_d3d12_debug_profiling_execution(const Win32DesktopPresentationReport& report,
                                                                    bool requested);
[[nodiscard]] std::string_view win32_desktop_presentation_vulkan_debug_profiling_execution_status_name(
    Win32DesktopPresentationVulkanDebugProfilingExecutionStatus status) noexcept;
[[nodiscard]] Win32DesktopPresentationVulkanDebugProfilingExecutionReport
evaluate_win32_desktop_presentation_vulkan_debug_profiling_execution(const Win32DesktopPresentationReport& report,
                                                                     bool requested);
[[nodiscard]] Win32DesktopPresentationQualityGateReport
evaluate_win32_desktop_presentation_quality_gate(const Win32DesktopPresentationReport& report,
                                                 const Win32DesktopPresentationQualityGateDesc& desc) noexcept;

} // namespace mirakana
