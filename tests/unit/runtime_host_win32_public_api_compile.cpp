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
    report.selected_backend = mirakana::Win32DesktopPresentationBackend::null_renderer;
    report.swapchain_plan = plan;
    report.scene_gpu_status = mirakana::Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;

    mirakana::Win32DesktopPresentationBackendReport backend_report;
    backend_report.backend = mirakana::Win32DesktopPresentationBackend::d3d12;
    backend_report.status = mirakana::Win32DesktopPresentationBackendReportStatus::ready;

    mirakana::Win32DesktopPresentationSceneGpuBindingDiagnostic scene_gpu_diagnostic;
    scene_gpu_diagnostic.status = mirakana::Win32DesktopPresentationSceneGpuBindingStatus::failed;

    mirakana::Win32DesktopGameHostDesc host_desc;
    host_desc.title = "Win32 Host";
    host_desc.d3d12_renderer = &renderer;
    host_desc.d3d12_scene_renderer = &scene_renderer;
    host_desc.request_tearing = true;

    return mirakana::win32_desktop_presentation_backend_name(report.requested_backend) == "d3d12" &&
                   mirakana::win32_desktop_presentation_backend_report_status_name(backend_report.status) == "ready" &&
                   mirakana::win32_desktop_presentation_fallback_reason_name(report.fallback_reason) == "none" &&
                   mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status) ==
                       "invalid_request" &&
                   plan.uses_create_swap_chain_for_hwnd && plan.uses_direct_command_queue &&
                   plan.flip_discard_swap_effect && plan.render_target_output && plan.resize_buffers_supported &&
                   plan.requires_present_state_before_present && !plan.public_native_handles_exposed &&
                   plan.allow_tearing_flag && host_desc.d3d12_renderer == &renderer &&
                   host_desc.d3d12_scene_renderer == &scene_renderer && presentation_desc.d3d12_renderer == &renderer &&
                   presentation_desc.d3d12_scene_renderer == &scene_renderer &&
                   scene_gpu_diagnostic.status == mirakana::Win32DesktopPresentationSceneGpuBindingStatus::failed
               ? 0
               : 1;
}
