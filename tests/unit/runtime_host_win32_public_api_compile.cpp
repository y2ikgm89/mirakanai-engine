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
    report.framegraph_passes = 2;
    report.renderer_stats.postprocess_passes_executed = 2;
    report.renderer_stats.framegraph_render_passes_recorded = 2;
    report.renderer_stats.framegraph_barrier_steps_executed = 2;

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
    const auto fog = mirakana::evaluate_win32_desktop_presentation_environment_fog(report, d3d12_postprocess, true);
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
                   scene_renderer.compute_morph_skinned_mesh_bindings.size() == 1 &&
                   host_desc.d3d12_scene_renderer == &scene_renderer && presentation_desc.d3d12_renderer == &renderer &&
                   presentation_desc.d3d12_scene_renderer == &scene_renderer && quality.ready
               ? 0
               : 1;
}
