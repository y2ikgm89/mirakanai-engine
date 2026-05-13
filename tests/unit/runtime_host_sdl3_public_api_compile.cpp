// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"

#include <cstddef>

int main() {
    mirakana::SdlDesktopGameHostDesc desc;
    desc.d3d12_scene_renderer = nullptr;
    desc.vulkan_renderer = nullptr;
    desc.vulkan_scene_renderer = nullptr;
    desc.prefer_vulkan = false;

    mirakana::SdlDesktopPresentationSceneGpuBindingStats stats;
    stats.mesh_bindings = 1;
    stats.morph_mesh_bindings = 1;
    stats.material_bindings = 1;
    stats.mesh_uploads = 1;
    stats.morph_mesh_uploads = 1;
    stats.texture_uploads = 1;
    stats.material_uploads = 1;
    stats.material_pipeline_layouts = 1;
    stats.uploaded_texture_bytes = 512;
    stats.uploaded_mesh_bytes = 48;
    stats.uploaded_morph_bytes = 292;
    stats.uploaded_material_factor_bytes = 64;
    stats.morph_mesh_bindings_resolved = 1;
    stats.compute_morph_mesh_bindings = 1;
    stats.compute_morph_mesh_dispatches = 1;
    stats.compute_morph_mesh_draws = 1;
    stats.compute_morph_queue_waits = 1;
    stats.compute_morph_async_compute_queue_submits = 1;
    stats.compute_morph_async_graphics_queue_submits = 1;
    stats.compute_morph_async_graphics_queue_waits = 1;
    stats.compute_morph_async_last_compute_submitted_fence_value = 11;
    stats.compute_morph_async_last_graphics_queue_wait_fence_value = 11;
    stats.compute_morph_async_last_graphics_submitted_fence_value = 12;
    stats.compute_morph_mesh_bindings_resolved = 1;
    stats.compute_morph_skinned_mesh_bindings = 1;
    stats.compute_morph_skinned_mesh_dispatches = 1;
    stats.compute_morph_skinned_queue_waits = 1;
    stats.compute_morph_skinned_mesh_draws = 1;
    stats.compute_morph_skinned_mesh_bindings_resolved = 1;
    stats.compute_morph_output_position_bytes = 36;

    mirakana::SdlDesktopPresentationReport report;
    report.requested_backend = mirakana::SdlDesktopPresentationBackend::vulkan;
    report.selected_backend = mirakana::SdlDesktopPresentationBackend::null_renderer;
    report.used_null_fallback = true;
    report.backend_reports_count = 1;
    report.postprocess_depth_input_requested = true;
    report.postprocess_depth_input_ready = false;
    report.directional_shadow_status = mirakana::SdlDesktopPresentationDirectionalShadowStatus::unavailable;
    report.directional_shadow_requested = true;
    report.directional_shadow_ready = false;
    report.directional_shadow_filter_mode = mirakana::SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3;
    report.directional_shadow_filter_tap_count = 9;
    report.directional_shadow_filter_radius_texels = 1.0F;
    report.native_ui_overlay_status = mirakana::SdlDesktopPresentationNativeUiOverlayStatus::ready;
    report.native_ui_overlay_requested = true;
    report.native_ui_overlay_ready = true;
    report.native_ui_overlay_sprites_submitted = 1;
    report.native_ui_overlay_draws = 1;
    report.native_ui_overlay_diagnostics_count = 0;
    report.directional_shadow_diagnostics_count = 1;

    mirakana::SdlDesktopPresentationQualityGateDesc quality_desc;
    quality_desc.require_scene_gpu_bindings = true;
    quality_desc.require_postprocess = true;
    quality_desc.require_postprocess_depth_input = true;
    quality_desc.require_directional_shadow = true;
    quality_desc.require_directional_shadow_filtering = true;
    quality_desc.expected_frames = 1;
    const auto quality_report = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, quality_desc);

    mirakana::SdlDesktopPresentationBackendReport backend_report;
    backend_report.backend = mirakana::SdlDesktopPresentationBackend::vulkan;
    backend_report.status = mirakana::SdlDesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
    backend_report.fallback_reason = mirakana::SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable;

    const auto status = mirakana::SdlDesktopPresentationSceneGpuBindingStatus::not_requested;
    const auto reason = mirakana::SdlDesktopPresentationFallbackReason::none;
    const auto backend = mirakana::SdlDesktopPresentationBackend::vulkan;
    const auto report_status = mirakana::SdlDesktopPresentationBackendReportStatus::ready;
    mirakana::SdlDesktopPresentationVulkanRendererDesc renderer;
    renderer.vertex_shader.entry_point = "vs_main";
    renderer.fragment_shader.entry_point = "ps_main";
    mirakana::SdlDesktopPresentationVulkanSceneRendererDesc scene_renderer;
    scene_renderer.vertex_shader.entry_point = "vs_main";
    scene_renderer.fragment_shader.entry_point = "ps_main";
    scene_renderer.morph_vertex_shader.entry_point = "vs_morph";
    scene_renderer.morph_mesh_assets.push_back(mirakana::AssetId::from_name("morphs/probe"));
    scene_renderer.morph_mesh_bindings.push_back(mirakana::SdlDesktopPresentationSceneMorphMeshBinding{
        .mesh = mirakana::AssetId::from_name("meshes/probe"),
        .morph_mesh = mirakana::AssetId::from_name("morphs/probe"),
    });
    scene_renderer.compute_morph_vertex_shader.entry_point = "vs_compute_morph";
    scene_renderer.compute_morph_shader.entry_point = "cs_compute_morph_position";
    scene_renderer.compute_morph_mesh_bindings.push_back(mirakana::SdlDesktopPresentationSceneMorphMeshBinding{
        .mesh = mirakana::AssetId::from_name("meshes/probe"),
        .morph_mesh = mirakana::AssetId::from_name("morphs/probe"),
    });
    scene_renderer.compute_morph_skinned_shader.entry_point = "cs_compute_morph_skinned_position";
    scene_renderer.compute_morph_skinned_mesh_bindings.push_back(mirakana::SdlDesktopPresentationSceneMorphMeshBinding{
        .mesh = mirakana::AssetId::from_name("meshes/skinned-probe"),
        .morph_mesh = mirakana::AssetId::from_name("morphs/probe"),
    });
    scene_renderer.enable_compute_morph_tangent_frame_output = true;
    scene_renderer.enable_postprocess_depth_input = true;
    scene_renderer.native_ui_overlay_vertex_shader.entry_point = "vs_native_ui_overlay";
    scene_renderer.native_ui_overlay_fragment_shader.entry_point = "ps_native_ui_overlay";
    scene_renderer.enable_native_ui_overlay = true;
    scene_renderer.shadow_vertex_shader.entry_point = "vs_shadow";
    scene_renderer.shadow_fragment_shader.entry_point = "ps_shadow";
    scene_renderer.enable_directional_shadow_smoke = true;
    scene_renderer.vertex_buffers.push_back(mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12});
    scene_renderer.vertex_attributes.push_back(mirakana::rhi::VertexAttributeDesc{
        .location = 0,
        .binding = 0,
        .offset = 0,
        .format = mirakana::rhi::VertexFormat::float32x3,
        .semantic = mirakana::rhi::VertexSemantic::position,
        .semantic_index = 0,
    });
    mirakana::SdlDesktopPresentationD3d12SceneRendererDesc d3d12_scene_renderer;
    d3d12_scene_renderer.compute_morph_shader.entry_point = "cs_compute_morph_position";
    d3d12_scene_renderer.compute_morph_skinned_shader.entry_point = "cs_compute_morph_skinned_position";
    d3d12_scene_renderer.enable_compute_morph_tangent_frame_output = true;
    d3d12_scene_renderer.compute_morph_mesh_bindings.push_back(mirakana::SdlDesktopPresentationSceneMorphMeshBinding{
        .mesh = mirakana::AssetId::from_name("meshes/probe"),
        .morph_mesh = mirakana::AssetId::from_name("morphs/probe"),
    });
    d3d12_scene_renderer.compute_morph_skinned_mesh_bindings.push_back(
        mirakana::SdlDesktopPresentationSceneMorphMeshBinding{
            .mesh = mirakana::AssetId::from_name("meshes/skinned-probe"),
            .morph_mesh = mirakana::AssetId::from_name("morphs/probe"),
        });
    return status == mirakana::SdlDesktopPresentationSceneGpuBindingStatus::not_requested &&
                   reason == mirakana::SdlDesktopPresentationFallbackReason::none &&
                   backend == mirakana::SdlDesktopPresentationBackend::vulkan &&
                   report_status == mirakana::SdlDesktopPresentationBackendReportStatus::ready &&
                   report.requested_backend == mirakana::SdlDesktopPresentationBackend::vulkan &&
                   backend_report.status ==
                       mirakana::SdlDesktopPresentationBackendReportStatus::runtime_pipeline_unavailable &&
                   mirakana::sdl_desktop_presentation_backend_report_status_name(report_status) == "ready" &&
                   mirakana::sdl_desktop_presentation_directional_shadow_status_name(
                       report.directional_shadow_status) == "unavailable" &&
                   mirakana::sdl_desktop_presentation_directional_shadow_filter_mode_name(
                       report.directional_shadow_filter_mode) == "fixed_pcf_3x3" &&
                   mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status) ==
                       "ready" &&
                   mirakana::sdl_desktop_presentation_quality_gate_status_name(quality_report.status) == "blocked" &&
                   quality_report.status == mirakana::SdlDesktopPresentationQualityGateStatus::blocked &&
                   !quality_report.ready && quality_report.diagnostics_count > 0 && stats.mesh_bindings == 1 &&
                   stats.material_bindings == 1 && stats.mesh_uploads == 1 && stats.morph_mesh_bindings == 1 &&
                   stats.texture_uploads == 1 && stats.material_uploads == 1 && stats.material_pipeline_layouts == 1 &&
                   stats.morph_mesh_uploads == 1 && stats.uploaded_texture_bytes == 512 &&
                   stats.uploaded_mesh_bytes == 48 && stats.uploaded_morph_bytes == 292 &&
                   stats.uploaded_material_factor_bytes == 64 && stats.morph_mesh_bindings_resolved == 1 &&
                   stats.compute_morph_mesh_bindings == 1 && stats.compute_morph_mesh_dispatches == 1 &&
                   stats.compute_morph_mesh_draws == 1 && stats.compute_morph_queue_waits == 1 &&
                   stats.compute_morph_async_compute_queue_submits == 1 &&
                   stats.compute_morph_async_graphics_queue_submits == 1 &&
                   stats.compute_morph_async_graphics_queue_waits == 1 &&
                   stats.compute_morph_async_last_compute_submitted_fence_value == 11 &&
                   stats.compute_morph_async_last_graphics_queue_wait_fence_value == 11 &&
                   stats.compute_morph_async_last_graphics_submitted_fence_value == 12 &&
                   stats.compute_morph_mesh_bindings_resolved == 1 && stats.compute_morph_skinned_mesh_bindings == 1 &&
                   stats.compute_morph_skinned_mesh_dispatches == 1 && stats.compute_morph_skinned_queue_waits == 1 &&
                   stats.compute_morph_skinned_mesh_draws == 1 &&
                   stats.compute_morph_skinned_mesh_bindings_resolved == 1 &&
                   stats.compute_morph_output_position_bytes == 36 && renderer.vertex_shader.entry_point == "vs_main" &&
                   report.postprocess_depth_input_requested && !report.postprocess_depth_input_ready &&
                   report.directional_shadow_requested && !report.directional_shadow_ready &&
                   report.directional_shadow_filter_tap_count == 9 &&
                   report.directional_shadow_filter_radius_texels == 1.0F && report.native_ui_overlay_requested &&
                   report.native_ui_overlay_ready && report.native_ui_overlay_sprites_submitted == 1 &&
                   report.native_ui_overlay_draws == 1 && report.native_ui_overlay_diagnostics_count == 0 &&
                   report.directional_shadow_diagnostics_count == 1 && scene_renderer.enable_postprocess_depth_input &&
                   scene_renderer.enable_native_ui_overlay && scene_renderer.enable_directional_shadow_smoke &&
                   scene_renderer.morph_vertex_shader.entry_point == "vs_morph" &&
                   scene_renderer.morph_mesh_assets.size() == 1 && scene_renderer.morph_mesh_bindings.size() == 1 &&
                   scene_renderer.morph_mesh_bindings.front().mesh == mirakana::AssetId::from_name("meshes/probe") &&
                   scene_renderer.compute_morph_vertex_shader.entry_point == "vs_compute_morph" &&
                   scene_renderer.compute_morph_shader.entry_point == "cs_compute_morph_position" &&
                   scene_renderer.compute_morph_mesh_bindings.size() == 1 &&
                   scene_renderer.compute_morph_skinned_shader.entry_point == "cs_compute_morph_skinned_position" &&
                   scene_renderer.compute_morph_skinned_mesh_bindings.size() == 1 &&
                   scene_renderer.enable_compute_morph_tangent_frame_output &&
                   d3d12_scene_renderer.compute_morph_shader.entry_point == "cs_compute_morph_position" &&
                   d3d12_scene_renderer.compute_morph_skinned_shader.entry_point ==
                       "cs_compute_morph_skinned_position" &&
                   d3d12_scene_renderer.enable_compute_morph_tangent_frame_output &&
                   d3d12_scene_renderer.compute_morph_mesh_bindings.size() == 1 &&
                   d3d12_scene_renderer.compute_morph_skinned_mesh_bindings.size() == 1 &&
                   scene_renderer.vertex_attributes.front().semantic == mirakana::rhi::VertexSemantic::position
               ? 0
               : 1;
}
