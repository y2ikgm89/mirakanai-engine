// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/win32/win32_desktop_game_host.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"

int main() {
    mirakana::Win32DesktopPresentationShaderBytecode shader;
    shader.entry_point = "vs_main";

    mirakana::Win32DesktopPresentationD3d12RendererDesc renderer;
    renderer.vertex_shader = shader;
    renderer.fragment_shader.entry_point = "ps_main";

    mirakana::Win32DesktopPresentationD3d12SceneRendererDesc scene_renderer;
    scene_renderer.vertex_shader = shader;
    scene_renderer.fragment_shader.entry_point = "ps_main";
    scene_renderer.compute_morph_vertex_shader.entry_point = "vs_compute_morph";
    scene_renderer.compute_morph_shader.entry_point = "cs_compute_morph_position";
    scene_renderer.compute_morph_mesh_bindings.push_back({});
    scene_renderer.compute_morph_skinned_shader.entry_point = "cs_compute_morph_skinned_position";
    scene_renderer.compute_morph_skinned_mesh_bindings.push_back({});
    scene_renderer.postprocess_vertex_shader.entry_point = "vs_postprocess";
    scene_renderer.postprocess_fragment_shader.entry_point = "ps_postprocess";
    scene_renderer.enable_postprocess = true;
    scene_renderer.enable_postprocess_depth_input = true;
    scene_renderer.enable_environment_fog = true;
    scene_renderer.environment_fog.mode = mirakana::EnvironmentFogMode::exponential_height;
    scene_renderer.environment_fog.density = 0.08F;
    scene_renderer.environment_fog.scene_depth_available = true;
    scene_renderer.environment_fog.shader_contract_evidence_ready = true;
    scene_renderer.enable_physical_sky_package_evidence = true;
    scene_renderer.physical_sky.shader_contract_evidence_ready = true;
    scene_renderer.physical_sky.package_evidence_ready = true;
    scene_renderer.physical_sky.execution_evidence_ready = true;
    scene_renderer.physical_sky.request_ready_promotion = true;
    scene_renderer.enable_cloud_layer_package_evidence = true;
    scene_renderer.cloud_layer.layer.coverage = 0.45F;
    scene_renderer.cloud_layer.layer.opacity = 0.8F;
    scene_renderer.cloud_layer.layer.altitude_m = 2400.0F;
    scene_renderer.cloud_layer.layer.wind_velocity_mps = mirakana::Vec2{.x = 8.0F, .y = 1.5F};
    scene_renderer.cloud_layer.layer.cloud_map_asset_ref = "sample/desktop-runtime/texture";
    scene_renderer.cloud_layer.layer.flow_map_asset_ref = "sample/desktop-runtime/texture";
    scene_renderer.cloud_layer.layer.ibl_contribution_mode =
        mirakana::EnvironmentCloudIblContributionMode::sky_tint_only;
    scene_renderer.cloud_layer.layer.ibl_contribution = 0.25F;
    scene_renderer.cloud_layer.shader_contract_evidence_ready = true;
    scene_renderer.cloud_layer.package_evidence_ready = true;
    scene_renderer.cloud_layer.execution_evidence_ready = true;
    scene_renderer.cloud_layer.request_ready_promotion = true;
    scene_renderer.enable_environment_precipitation_package_evidence = true;
    scene_renderer.environment_precipitation.environment_plan.weather_rows.push_back(mirakana::EnvironmentWeatherRow{
        .weather = mirakana::EnvironmentWeatherKind::storm,
        .precipitation_kind = mirakana::EnvironmentPrecipitationKind::rain,
        .intensity = 0.8F,
        .precipitation_enabled = true,
        .wet_surface_enabled = true,
        .audio_handoff_enabled = true,
    });
    scene_renderer.environment_precipitation.environment_plan.particle_rows.push_back(
        mirakana::EnvironmentPrecipitationParticleRow{
            .kind = mirakana::EnvironmentPrecipitationKind::rain,
            .intensity = 0.8F,
            .spawn_rate_per_second = 2800.0F,
            .particle_radius_mm = 0.8F,
            .fall_speed_mps = 8.5F,
            .wind_speed_mps = 6.0F,
            .camera_near_only = true,
            .gpu_particle_intent = true,
        });
    scene_renderer.environment_precipitation.environment_plan.wetness_rows.push_back(
        mirakana::EnvironmentSurfaceWetnessRow{
            .enabled = true,
            .intensity = 0.8F,
            .splash_intent = true,
            .ripple_intent = true,
            .mutates_materials = false,
        });
    scene_renderer.environment_precipitation.environment_plan.occlusion_rows.push_back(
        mirakana::EnvironmentPrecipitationOcclusionRow{
            .required = true,
            .available = true,
            .uses_scene_geometry_depth_mask = true,
            .uses_indoor_volume_mask = true,
        });
    scene_renderer.environment_precipitation.environment_plan.audio_handoff_rows.push_back(
        mirakana::EnvironmentPrecipitationAudioHandoffRow{
            .cue = mirakana::EnvironmentPrecipitationAudioCueKind::rain_loop,
            .intensity = 0.8F,
            .delay_seconds = 0.0F,
            .handoff_only = true,
        });
    scene_renderer.environment_precipitation.environment_plan.status =
        mirakana::EnvironmentPrecipitationPlanStatus::planned;
    scene_renderer.environment_precipitation.shader_contract_evidence_ready = true;
    scene_renderer.environment_precipitation.package_evidence_ready = true;
    scene_renderer.environment_precipitation.execution_evidence_ready = true;
    scene_renderer.environment_precipitation.request_ready_promotion = true;

    mirakana::Win32DesktopPresentationDesc presentation_desc;
    presentation_desc.prefer_d3d12 = true;
    presentation_desc.d3d12_renderer = &renderer;
    presentation_desc.d3d12_scene_renderer = &scene_renderer;
    presentation_desc.request_tearing = true;

    const auto plan = mirakana::plan_win32_d3d12_swapchain(mirakana::Win32D3d12SwapChainPlanDesc{
        .extent = mirakana::Extent2D{.width = 640, .height = 360},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .request_tearing = true,
        .tearing_supported = true,
    });

    mirakana::Win32DesktopPresentationReport report;
    report.requested_backend = mirakana::Win32DesktopPresentationBackend::d3d12;
    report.selected_backend = mirakana::Win32DesktopPresentationBackend::d3d12;
    report.swapchain_plan = plan;
    report.scene_gpu_status = mirakana::Win32DesktopPresentationSceneGpuBindingStatus::ready;
    auto& stats = report.scene_gpu_stats;
    stats.mesh_bindings = 1;
    stats.material_bindings = 1;
    stats.compute_morph_queue_waits = 1;
    stats.compute_morph_async_compute_queue_submits = 1;
    report.postprocess_status = mirakana::Win32DesktopPresentationPostprocessStatus::ready;
    report.postprocess_depth_input_ready = true;
    report.environment_fog_requested = true;
    report.environment_fog_constant_buffer_ready = true;
    report.environment_fog_constant_buffer_bytes = mirakana::environment_fog_constants_byte_size();
    report.environment_fog_vulkan_package_requested = true;
    report.environment_fog_vulkan_package_shader_contract_evidence_ready = true;
    report.environment_fog_vulkan_package_evidence_ready = true;
    report.environment_fog_vulkan_package_constant_buffer_ready = true;
    report.environment_fog_vulkan_package_constant_buffer_bytes = mirakana::environment_fog_constants_byte_size();
    report.physical_sky_requested = true;
    report.physical_sky_shader_contract_evidence_ready = true;
    report.physical_sky_package_evidence_ready = true;
    report.physical_sky_execution_evidence_ready = true;
    report.physical_sky_constant_buffer_ready = true;
    report.physical_sky_constant_buffer_bytes = mirakana::physical_sky_constants_byte_size();
    report.physical_sky_constant_layout_rows = 8;
    report.physical_sky_lut_intent_rows = 4;
    report.cloud_layer_requested = true;
    report.cloud_layer_shader_contract_evidence_ready = true;
    report.cloud_layer_package_evidence_ready = true;
    report.cloud_layer_execution_evidence_ready = true;
    report.cloud_layer_uses_latlong_projection = true;
    report.cloud_layer_uses_flow_map = true;
    report.cloud_layer_texture_rows = 1;
    report.cloud_layer_visual_rows = 1;
    report.cloud_layer_ibl_rows = 1;
    report.cloud_layer_shader_contract_rows = 1;
    report.cloud_layer_quality_rows = 1;
    report.environment_precipitation_requested = true;
    report.environment_precipitation_weather = mirakana::EnvironmentWeatherKind::storm;
    report.environment_precipitation_kind = mirakana::EnvironmentPrecipitationKind::rain;
    report.environment_precipitation_shader_contract_evidence_ready = true;
    report.environment_precipitation_package_evidence_ready = true;
    report.environment_precipitation_execution_evidence_ready = true;
    report.environment_precipitation_uses_camera_near_particles = true;
    report.environment_precipitation_uses_scene_depth_occlusion = true;
    report.environment_precipitation_weather_rows = 1;
    report.environment_precipitation_particle_rows = 1;
    report.environment_precipitation_occlusion_rows = 1;
    report.environment_precipitation_wetness_rows = 1;
    report.environment_precipitation_audio_handoff_rows = 1;
    report.environment_precipitation_shader_rows = 1;
    report.environment_precipitation_quality_rows = 1;
    report.environment_volumetric_fog_requested = true;
    report.environment_volumetric_fog_shader_contract_evidence_ready = true;
    report.environment_volumetric_fog_package_evidence_ready = true;
    report.environment_volumetric_fog_execution_evidence_ready = true;
    report.environment_volumetric_fog_froxel_output_ready = true;
    report.environment_volumetric_fog_scene_depth_ready = true;
    report.environment_volumetric_fog_compute_dispatches = 1;
    report.environment_volumetric_fog_exposes_native_handles = false;
    report.environment_volumetric_fog_policy_diagnostics_count = 0;
    report.framegraph_passes = 2;
    report.renderer_stats.postprocess_passes_executed = 2;
    report.renderer_stats.framegraph_render_passes_recorded = 2;
    report.renderer_stats.framegraph_barrier_steps_executed = 2;
    auto vulkan_report = report;
    vulkan_report.selected_backend = mirakana::Win32DesktopPresentationBackend::vulkan;

    mirakana::Win32DesktopPresentationBackendReport backend_report;
    backend_report.backend = mirakana::Win32DesktopPresentationBackend::d3d12;
    backend_report.status = mirakana::Win32DesktopPresentationBackendReportStatus::ready;

    mirakana::Win32DesktopGameHostDesc host_desc;
    host_desc.title = "Win32 Host";
    host_desc.d3d12_renderer = &renderer;
    host_desc.d3d12_scene_renderer = &scene_renderer;
    host_desc.request_tearing = true;

    const auto quality = mirakana::evaluate_win32_desktop_presentation_quality_gate(
        report, mirakana::Win32DesktopPresentationQualityGateDesc{.require_scene_gpu_bindings = true,
                                                                  .require_postprocess = true});
    const auto d3d12_postprocess =
        mirakana::evaluate_win32_desktop_presentation_d3d12_postprocess_execution(report, 2, true);
    const auto vulkan_postprocess =
        mirakana::evaluate_win32_desktop_presentation_vulkan_postprocess_execution(vulkan_report, 2, true);
    const auto fog = mirakana::evaluate_win32_desktop_presentation_environment_fog(report, d3d12_postprocess, true);
    const auto vulkan_fog = mirakana::evaluate_win32_desktop_presentation_vulkan_environment_fog_package(
        vulkan_report, vulkan_postprocess, true);
    const auto physical_sky = mirakana::evaluate_win32_desktop_presentation_physical_sky(report, true);
    auto missing_physical_sky_package_report = report;
    missing_physical_sky_package_report.physical_sky_package_evidence_ready = false;
    const auto missing_physical_sky_package =
        mirakana::evaluate_win32_desktop_presentation_physical_sky(missing_physical_sky_package_report, true);
    auto missing_physical_sky_rows_report = report;
    missing_physical_sky_rows_report.physical_sky_constant_layout_rows = 0;
    missing_physical_sky_rows_report.physical_sky_lut_intent_rows = 0;
    const auto missing_physical_sky_rows =
        mirakana::evaluate_win32_desktop_presentation_physical_sky(missing_physical_sky_rows_report, true);
    const auto cloud_layer = mirakana::evaluate_win32_desktop_presentation_cloud_layer(report, true);
    auto missing_cloud_layer_package_report = report;
    missing_cloud_layer_package_report.cloud_layer_package_evidence_ready = false;
    const auto missing_cloud_layer_package =
        mirakana::evaluate_win32_desktop_presentation_cloud_layer(missing_cloud_layer_package_report, true);
    auto missing_cloud_layer_rows_report = report;
    missing_cloud_layer_rows_report.cloud_layer_uses_latlong_projection = false;
    missing_cloud_layer_rows_report.cloud_layer_uses_flow_map = false;
    missing_cloud_layer_rows_report.cloud_layer_texture_rows = 0;
    missing_cloud_layer_rows_report.cloud_layer_visual_rows = 0;
    missing_cloud_layer_rows_report.cloud_layer_ibl_rows = 0;
    missing_cloud_layer_rows_report.cloud_layer_shader_contract_rows = 0;
    missing_cloud_layer_rows_report.cloud_layer_quality_rows = 0;
    const auto missing_cloud_layer_rows =
        mirakana::evaluate_win32_desktop_presentation_cloud_layer(missing_cloud_layer_rows_report, true);
    const auto precipitation = mirakana::evaluate_win32_desktop_presentation_environment_precipitation(report, true);
    auto missing_precipitation_package_report = report;
    missing_precipitation_package_report.environment_precipitation_package_evidence_ready = false;
    const auto missing_precipitation_package = mirakana::evaluate_win32_desktop_presentation_environment_precipitation(
        missing_precipitation_package_report, true);
    auto missing_precipitation_rows_report = report;
    missing_precipitation_rows_report.environment_precipitation_weather_rows = 0;
    missing_precipitation_rows_report.environment_precipitation_particle_rows = 0;
    missing_precipitation_rows_report.environment_precipitation_occlusion_rows = 0;
    missing_precipitation_rows_report.environment_precipitation_wetness_rows = 0;
    missing_precipitation_rows_report.environment_precipitation_audio_handoff_rows = 0;
    missing_precipitation_rows_report.environment_precipitation_shader_rows = 0;
    missing_precipitation_rows_report.environment_precipitation_quality_rows = 0;
    const auto missing_precipitation_rows = mirakana::evaluate_win32_desktop_presentation_environment_precipitation(
        missing_precipitation_rows_report, true);
    const auto volumetric_fog = mirakana::evaluate_win32_desktop_presentation_environment_volumetric_fog(report, true);
    auto missing_volumetric_package_report = report;
    missing_volumetric_package_report.environment_volumetric_fog_package_evidence_ready = false;
    const auto missing_volumetric_package = mirakana::evaluate_win32_desktop_presentation_environment_volumetric_fog(
        missing_volumetric_package_report, true);

    return mirakana::win32_desktop_presentation_backend_name(report.requested_backend) == "d3d12" &&
                   mirakana::win32_desktop_presentation_backend_report_status_name(backend_report.status) == "ready" &&
                   mirakana::win32_desktop_presentation_fallback_reason_name(report.fallback_reason) == "none" &&
                   mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status) ==
                       "ready" &&
                   mirakana::win32_desktop_presentation_postprocess_status_name(report.postprocess_status) == "ready" &&
                   plan.uses_create_swap_chain_for_hwnd && plan.uses_direct_command_queue &&
                   plan.flip_discard_swap_effect && plan.render_target_output && plan.resize_buffers_supported &&
                   plan.requires_present_state_before_present && !plan.public_native_handles_exposed &&
                   plan.allow_tearing_flag && host_desc.d3d12_renderer == &renderer &&
                   stats.compute_morph_queue_waits == 1 && stats.compute_morph_async_compute_queue_submits == 1 &&
                   scene_renderer.compute_morph_vertex_shader.entry_point == "vs_compute_morph" &&
                   scene_renderer.compute_morph_shader.entry_point == "cs_compute_morph_position" &&
                   scene_renderer.compute_morph_mesh_bindings.size() == 1 &&
                   scene_renderer.compute_morph_skinned_shader.entry_point == "cs_compute_morph_skinned_position" &&
                   scene_renderer.enable_environment_fog &&
                   scene_renderer.environment_fog.mode == mirakana::EnvironmentFogMode::exponential_height &&
                   mirakana::win32_desktop_presentation_environment_fog_status_name(fog.status) == "ready" &&
                   fog.ready && fog.constant_buffer_ready &&
                   fog.constants_binding == mirakana::environment_fog_constants_binding() &&
                   fog.constant_buffer_bytes == mirakana::environment_fog_constants_byte_size() &&
                   mirakana::win32_desktop_presentation_vulkan_environment_fog_package_status_name(vulkan_fog.status) ==
                       "ready" &&
                   vulkan_fog.ready && vulkan_fog.package_evidence_ready && vulkan_fog.shader_contract_evidence_ready &&
                   vulkan_fog.constant_buffer_ready &&
                   vulkan_fog.constants_binding == mirakana::environment_fog_constants_binding() &&
                   vulkan_fog.constant_buffer_bytes == mirakana::environment_fog_constants_byte_size() &&
                   !vulkan_fog.exposes_native_handles && vulkan_fog.diagnostics_count == 0 &&
                   scene_renderer.enable_physical_sky_package_evidence &&
                   mirakana::win32_desktop_presentation_physical_sky_status_name(physical_sky.status) == "ready" &&
                   physical_sky.ready && physical_sky.package_evidence_ready &&
                   physical_sky.shader_contract_evidence_ready && physical_sky.execution_evidence_ready &&
                   physical_sky.constant_buffer_ready &&
                   physical_sky.constants_binding == mirakana::physical_sky_constants_binding() &&
                   physical_sky.constant_buffer_bytes == mirakana::physical_sky_constants_byte_size() &&
                   physical_sky.constant_layout_rows == 8 && physical_sky.lut_intent_rows == 4 &&
                   !physical_sky.allocates_lut_textures && !physical_sky.invokes_backend &&
                   !physical_sky.exposes_native_handles && physical_sky.diagnostics_count == 0 &&
                   mirakana::win32_desktop_presentation_physical_sky_status_name(missing_physical_sky_package.status) ==
                       "blocked" &&
                   !missing_physical_sky_package.ready && !missing_physical_sky_package.package_evidence_ready &&
                   missing_physical_sky_package.diagnostics_count > 0 &&
                   mirakana::win32_desktop_presentation_physical_sky_status_name(missing_physical_sky_rows.status) ==
                       "blocked" &&
                   !missing_physical_sky_rows.ready && missing_physical_sky_rows.constant_layout_rows == 0 &&
                   missing_physical_sky_rows.lut_intent_rows == 0 && missing_physical_sky_rows.diagnostics_count > 0 &&
                   scene_renderer.enable_cloud_layer_package_evidence &&
                   scene_renderer.cloud_layer.layer.mode == mirakana::EnvironmentCloudLayerMode::equirectangular_2d &&
                   mirakana::win32_desktop_presentation_cloud_layer_status_name(cloud_layer.status) == "ready" &&
                   cloud_layer.ready && cloud_layer.package_evidence_ready &&
                   cloud_layer.shader_contract_evidence_ready && cloud_layer.execution_evidence_ready &&
                   cloud_layer.cloud_map_binding == mirakana::cloud_layer_cloud_map_binding() &&
                   cloud_layer.flow_map_binding == mirakana::cloud_layer_flow_map_binding() &&
                   cloud_layer.sampler_binding == mirakana::cloud_layer_sampler_binding() &&
                   cloud_layer.constants_binding == mirakana::cloud_layer_constants_binding() &&
                   cloud_layer.uses_latlong_projection && cloud_layer.uses_flow_map && cloud_layer.texture_rows == 1 &&
                   cloud_layer.visual_rows == 1 && cloud_layer.ibl_rows == 1 && cloud_layer.shader_contract_rows == 1 &&
                   cloud_layer.quality_rows == 1 && !cloud_layer.uploads_textures && !cloud_layer.invokes_backend &&
                   !cloud_layer.exposes_native_handles && !cloud_layer.uses_volumetric_clouds &&
                   cloud_layer.diagnostics_count == 0 &&
                   mirakana::win32_desktop_presentation_cloud_layer_status_name(missing_cloud_layer_package.status) ==
                       "blocked" &&
                   !missing_cloud_layer_package.ready && !missing_cloud_layer_package.package_evidence_ready &&
                   missing_cloud_layer_package.diagnostics_count > 0 &&
                   mirakana::win32_desktop_presentation_cloud_layer_status_name(missing_cloud_layer_rows.status) ==
                       "blocked" &&
                   !missing_cloud_layer_rows.ready && !missing_cloud_layer_rows.uses_latlong_projection &&
                   !missing_cloud_layer_rows.uses_flow_map && missing_cloud_layer_rows.texture_rows == 0 &&
                   missing_cloud_layer_rows.visual_rows == 0 && missing_cloud_layer_rows.ibl_rows == 0 &&
                   missing_cloud_layer_rows.shader_contract_rows == 0 && missing_cloud_layer_rows.quality_rows == 0 &&
                   missing_cloud_layer_rows.diagnostics_count > 0 &&
                   scene_renderer.enable_environment_precipitation_package_evidence &&
                   scene_renderer.environment_precipitation.environment_plan.particle_rows.size() == 1 &&
                   mirakana::win32_desktop_presentation_environment_precipitation_status_name(precipitation.status) ==
                       "ready" &&
                   precipitation.ready && precipitation.weather == mirakana::EnvironmentWeatherKind::storm &&
                   precipitation.kind == mirakana::EnvironmentPrecipitationKind::rain &&
                   precipitation.package_evidence_ready && precipitation.shader_contract_evidence_ready &&
                   precipitation.execution_evidence_ready &&
                   precipitation.particle_texture_binding == mirakana::precipitation_particle_texture_binding() &&
                   precipitation.scene_depth_texture_binding == mirakana::precipitation_scene_depth_texture_binding() &&
                   precipitation.sampler_binding == mirakana::precipitation_sampler_binding() &&
                   precipitation.constants_binding == mirakana::precipitation_constants_binding() &&
                   precipitation.uses_camera_near_particles && precipitation.uses_scene_depth_occlusion &&
                   precipitation.weather_rows == 1 && precipitation.particle_rows == 1 &&
                   precipitation.occlusion_rows == 1 && precipitation.wetness_rows == 1 &&
                   precipitation.audio_handoff_rows == 1 && precipitation.shader_rows == 1 &&
                   precipitation.quality_rows == 1 && !precipitation.uploads_particle_buffers &&
                   !precipitation.invokes_backend && !precipitation.exposes_native_handles &&
                   !precipitation.mutates_materials && !precipitation.plays_audio &&
                   precipitation.diagnostics_count == 0 &&
                   mirakana::win32_desktop_presentation_environment_precipitation_status_name(
                       missing_precipitation_package.status) == "blocked" &&
                   !missing_precipitation_package.ready && !missing_precipitation_package.package_evidence_ready &&
                   missing_precipitation_package.diagnostics_count > 0 &&
                   mirakana::win32_desktop_presentation_environment_precipitation_status_name(
                       missing_precipitation_rows.status) == "blocked" &&
                   !missing_precipitation_rows.ready && missing_precipitation_rows.weather_rows == 0 &&
                   missing_precipitation_rows.particle_rows == 0 && missing_precipitation_rows.occlusion_rows == 0 &&
                   missing_precipitation_rows.wetness_rows == 0 && missing_precipitation_rows.audio_handoff_rows == 0 &&
                   missing_precipitation_rows.shader_rows == 0 && missing_precipitation_rows.quality_rows == 0 &&
                   missing_precipitation_rows.diagnostics_count > 0 &&
                   mirakana::win32_desktop_presentation_environment_volumetric_fog_status_name(volumetric_fog.status) ==
                       "ready" &&
                   volumetric_fog.ready && volumetric_fog.d3d12_backend_selected && volumetric_fog.scene_depth_ready &&
                   volumetric_fog.froxel_output_ready && volumetric_fog.shader_contract_evidence_ready &&
                   volumetric_fog.package_evidence_ready && volumetric_fog.execution_evidence_ready &&
                   volumetric_fog.compute_dispatches > 0 &&
                   volumetric_fog.constants_binding == mirakana::volumetric_fog_constants_binding() &&
                   volumetric_fog.constant_buffer_bytes == mirakana::volumetric_fog_constants_byte_size() &&
                   volumetric_fog.froxel_output_buffer_binding ==
                       mirakana::volumetric_fog_froxel_output_buffer_binding() &&
                   !volumetric_fog.exposes_native_handles && volumetric_fog.diagnostics_count == 0 &&
                   mirakana::win32_desktop_presentation_environment_volumetric_fog_status_name(
                       missing_volumetric_package.status) == "blocked" &&
                   !missing_volumetric_package.ready && !missing_volumetric_package.package_evidence_ready &&
                   missing_volumetric_package.diagnostics_count > 0 &&
                   scene_renderer.compute_morph_skinned_mesh_bindings.size() == 1 &&
                   host_desc.d3d12_scene_renderer == &scene_renderer && presentation_desc.d3d12_renderer == &renderer &&
                   presentation_desc.d3d12_scene_renderer == &scene_renderer && quality.ready
               ? 0
               : 1;
}
