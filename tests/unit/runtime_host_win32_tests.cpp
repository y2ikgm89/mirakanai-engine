// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_host/win32/win32_desktop_game_host.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"

#if defined(_WIN32)
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <cstring>
#endif

#include <cstdint>
#include <string_view>

namespace {

class OneFrameWin32App final : public mirakana::GameApp {
  public:
    OneFrameWin32App(mirakana::VirtualInput& input, mirakana::IRenderer& renderer)
        : input_(input), renderer_(renderer) {}

    bool on_update(mirakana::EngineContext& /*context*/, double /*delta_seconds*/) override {
        ++updates;
        renderer_.begin_frame();
        renderer_.draw_sprite(mirakana::SpriteCommand{});
        renderer_.end_frame();
        return true;
    }

    mirakana::VirtualInput& input_;
    mirakana::IRenderer& renderer_;
    int updates{0};
};

#if defined(_WIN32)
[[nodiscard]] bool d3d12_presentation_test_enabled() noexcept {
    return GetEnvironmentVariableA("MK_ENABLE_WIN32_D3D12_PRESENTATION_TEST", nullptr, 0) > 0;
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_shader(const char* source, const char* entry_point,
                                                              const char* target) {
    Microsoft::WRL::ComPtr<ID3DBlob> bytecode;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry_point, target,
                                      D3DCOMPILE_ENABLE_STRICTNESS, 0, &bytecode, &errors);
    MK_REQUIRE(SUCCEEDED(result));
    return bytecode;
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_triangle_vertex_shader() {
    return compile_shader("struct VsOut {"
                          "  float4 position : SV_Position;"
                          "};"
                          "VsOut vs_main(uint vertex_id : SV_VertexID) {"
                          "  float2 positions[3] = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };"
                          "  VsOut output;"
                          "  output.position = float4(positions[vertex_id], 0.0, 1.0);"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_triangle_pixel_shader() {
    return compile_shader("float4 ps_main(float4 position : SV_Position) : SV_Target {"
                          "  return float4(0.1, 0.2, 0.3, 1.0);"
                          "}",
                          "ps_main", "ps_5_0");
}
#endif

} // namespace

MK_TEST("win32 d3d12 swapchain plan follows official hwnd presentation contract") {
    const auto plan = mirakana::plan_win32_d3d12_swapchain(mirakana::Win32D3d12SwapChainPlanDesc{
        .extent = mirakana::Extent2D{.width = 1280, .height = 720},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .request_tearing = true,
        .tearing_supported = true,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.uses_create_swap_chain_for_hwnd);
    MK_REQUIRE(plan.uses_direct_command_queue);
    MK_REQUIRE(plan.flip_discard_swap_effect);
    MK_REQUIRE(plan.render_target_output);
    MK_REQUIRE(plan.resize_buffers_supported);
    MK_REQUIRE(plan.requires_present_state_before_present);
    MK_REQUIRE(plan.present_sync_interval == 0);
    MK_REQUIRE(plan.allow_tearing_flag);
    MK_REQUIRE(!plan.public_native_handles_exposed);
}

MK_TEST("win32 d3d12 swapchain plan rejects invalid extent format and buffer count") {
    const auto invalid_extent = mirakana::plan_win32_d3d12_swapchain(mirakana::Win32D3d12SwapChainPlanDesc{
        .extent = mirakana::Extent2D{.width = 0, .height = 720},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
    });
    MK_REQUIRE(!invalid_extent.succeeded());
    MK_REQUIRE(!invalid_extent.diagnostic.empty());

    const auto invalid_buffers = mirakana::plan_win32_d3d12_swapchain(mirakana::Win32D3d12SwapChainPlanDesc{
        .extent = mirakana::Extent2D{.width = 1280, .height = 720},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 1,
    });
    MK_REQUIRE(!invalid_buffers.succeeded());

    const auto invalid_format = mirakana::plan_win32_d3d12_swapchain(mirakana::Win32D3d12SwapChainPlanDesc{
        .extent = mirakana::Extent2D{.width = 1280, .height = 720},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .buffer_count = 2,
    });
    MK_REQUIRE(!invalid_format.succeeded());
}

#if defined(_WIN32)
MK_TEST("win32 desktop presentation falls back to null renderer when d3d12 request is missing") {
    mirakana::win32::Win32Runtime runtime(mirakana::win32::Win32RuntimeDesc{
        .window_class_name = "MIRAIKANAI Win32 Presentation Missing Request",
        .dpi_aware = true,
    });
    mirakana::win32::Win32Window window(runtime, mirakana::WindowDesc{
                                                     .title = "Win32 Presentation Missing Request",
                                                     .extent = mirakana::WindowExtent{.width = 320, .height = 180},
                                                 });

    mirakana::Win32DesktopPresentation presentation(mirakana::Win32DesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_d3d12 = true,
        .allow_null_fallback = true,
    });

    MK_REQUIRE(presentation.backend() == mirakana::Win32DesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.renderer().backend_name() == std::string_view{"null"});
    MK_REQUIRE(presentation.renderer().backbuffer_extent().width == 320);
    MK_REQUIRE(presentation.report().requested_backend == mirakana::Win32DesktopPresentationBackend::d3d12);
    MK_REQUIRE(presentation.report().selected_backend == mirakana::Win32DesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.report().fallback_reason ==
               mirakana::Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable);
    MK_REQUIRE(presentation.report().used_null_fallback);
    MK_REQUIRE(!presentation.backend_reports().empty());
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::Win32DesktopPresentationBackendReportStatus::missing_request);
    MK_REQUIRE(!presentation.report().swapchain_plan.public_native_handles_exposed);
}

MK_TEST("win32 desktop presentation public compute morph rows stay backend neutral") {
    mirakana::Win32DesktopPresentationVulkanSceneRendererDesc vulkan_desc;
    vulkan_desc.compute_morph_vertex_shader.entry_point = "vs_compute_morph";
    vulkan_desc.compute_morph_shader.entry_point = "cs_compute_morph_position";
    vulkan_desc.compute_morph_mesh_bindings.push_back({});
    vulkan_desc.compute_morph_skinned_shader.entry_point = "cs_compute_morph_skinned_position";
    vulkan_desc.compute_morph_skinned_mesh_bindings.push_back({});

    const mirakana::rhi::FenceValue graphics_fence{.value = 7, .queue = mirakana::rhi::QueueKind::graphics};
    mirakana::Win32DesktopPresentationSceneGpuBindingStats stats;
    stats.compute_morph_queue_waits = 1;
    stats.compute_morph_async_compute_queue_submits = 1;
    stats.compute_morph_async_last_graphics_submitted_fence_value = graphics_fence.value;

    MK_REQUIRE(vulkan_desc.compute_morph_vertex_shader.entry_point == std::string_view{"vs_compute_morph"});
    MK_REQUIRE(vulkan_desc.compute_morph_shader.entry_point == std::string_view{"cs_compute_morph_position"});
    MK_REQUIRE(vulkan_desc.compute_morph_mesh_bindings.size() == 1);
    MK_REQUIRE(vulkan_desc.compute_morph_skinned_shader.entry_point ==
               std::string_view{"cs_compute_morph_skinned_position"});
    MK_REQUIRE(vulkan_desc.compute_morph_skinned_mesh_bindings.size() == 1);
    MK_REQUIRE(stats.compute_morph_queue_waits == 1);
    MK_REQUIRE(stats.compute_morph_async_compute_queue_submits == 1);
    MK_REQUIRE(stats.compute_morph_async_last_graphics_submitted_fence_value == graphics_fence.value);
}

MK_TEST("win32 vulkan debug profiling execution requires timestamp query readback") {
    mirakana::Win32DesktopPresentationReport report;
    report.selected_backend = mirakana::Win32DesktopPresentationBackend::vulkan;
    report.rhi_gpu_timestamp_ticks_per_second = 1'000'000'000;
    report.rhi_gpu_debug_scopes_begun = 1;
    report.rhi_gpu_debug_scopes_ended = 1;
    report.rhi_gpu_debug_markers_inserted = 1;
    report.renderer_stats.framegraph_barrier_steps_executed = 1;
    report.renderer_stats.framegraph_render_passes_recorded = 1;

    const auto missing_readback =
        mirakana::evaluate_win32_desktop_presentation_vulkan_debug_profiling_execution(report, true);

    MK_REQUIRE(missing_readback.vulkan_backend_selected);
    MK_REQUIRE(!missing_readback.gpu_timestamps_current);
    MK_REQUIRE(missing_readback.gpu_debug_markers_current);
    MK_REQUIRE(missing_readback.frame_diagnostics_current);
    MK_REQUIRE(!missing_readback.ready);
    MK_REQUIRE(missing_readback.status ==
               mirakana::Win32DesktopPresentationVulkanDebugProfilingExecutionStatus::blocked);

    report.rhi_gpu_timestamp_query_writes = 2;
    report.rhi_gpu_timestamp_query_results_read = 1;
    report.rhi_gpu_timestamp_query_failures = 0;
    report.rhi_last_gpu_timestamp_begin = 10;
    report.rhi_last_gpu_timestamp_end = 20;

    const auto ready = mirakana::evaluate_win32_desktop_presentation_vulkan_debug_profiling_execution(report, true);

    MK_REQUIRE(ready.gpu_timestamps_current);
    MK_REQUIRE(ready.ready);
    MK_REQUIRE(ready.status == mirakana::Win32DesktopPresentationVulkanDebugProfilingExecutionStatus::ready);
}

MK_TEST("win32 desktop presentation can create d3d12 rhi frame renderer when shader bytecode is supplied") {
    if (!d3d12_presentation_test_enabled()) {
        return;
    }

    auto vertex_shader = compile_triangle_vertex_shader();
    auto fragment_shader = compile_triangle_pixel_shader();
    const auto* vertex_bytes = static_cast<const std::uint8_t*>(vertex_shader->GetBufferPointer());
    const auto* fragment_bytes = static_cast<const std::uint8_t*>(fragment_shader->GetBufferPointer());
    mirakana::Win32DesktopPresentationD3d12RendererDesc d3d12_renderer{
        .vertex_shader =
            mirakana::Win32DesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = std::span<const std::uint8_t>{vertex_bytes, vertex_shader->GetBufferSize()},
            },
        .fragment_shader =
            mirakana::Win32DesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = std::span<const std::uint8_t>{fragment_bytes, fragment_shader->GetBufferSize()},
            },
    };

    mirakana::win32::Win32Runtime runtime(mirakana::win32::Win32RuntimeDesc{
        .window_class_name = "MIRAIKANAI Win32 D3D12 Presentation",
        .dpi_aware = true,
    });
    mirakana::win32::Win32Window window(runtime, mirakana::WindowDesc{
                                                     .title = "Win32 D3D12 Presentation",
                                                     .extent = mirakana::WindowExtent{.width = 320, .height = 180},
                                                 });
    mirakana::Win32DesktopPresentation presentation(mirakana::Win32DesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_d3d12 = true,
        .allow_null_fallback = true,
        .prefer_warp = true,
        .enable_debug_layer = false,
        .vsync = false,
        .request_tearing = false,
        .d3d12_renderer = &d3d12_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::Win32DesktopPresentationBackend::d3d12);
    MK_REQUIRE(presentation.renderer().backend_name() == std::string_view{"d3d12"});
    MK_REQUIRE(presentation.report().selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12);
    MK_REQUIRE(!presentation.report().used_null_fallback);
    MK_REQUIRE(presentation.report().rhi_stats.swapchains_created == 1);
    MK_REQUIRE(presentation.report().swapchain_plan.present_sync_interval == 0);

    presentation.renderer().begin_frame();
    presentation.renderer().draw_sprite(mirakana::SpriteCommand{});
    presentation.renderer().end_frame();

    window.resize(mirakana::WindowExtent{.width = 400, .height = 220});
    presentation.renderer().resize(mirakana::Extent2D{.width = 400, .height = 220});

    MK_REQUIRE(presentation.renderer().backbuffer_extent().width == 400);
    MK_REQUIRE(presentation.report().renderer_stats.frames_finished == 1);
    MK_REQUIRE(presentation.report().rhi_stats.swapchain_resizes == 1);
    MK_REQUIRE(presentation.report().rhi_stats.present_calls == 1);
}

MK_TEST("win32 desktop game host runs a game through native windowed services") {
    mirakana::Win32DesktopGameHost host(mirakana::Win32DesktopGameHostDesc{
        .title = "Win32 Desktop Game Host Test",
        .extent = mirakana::WindowExtent{.width = 320, .height = 180},
        .prefer_d3d12 = false,
    });
    OneFrameWin32App app(host.input(), host.renderer());

    const auto result = host.run(app, mirakana::DesktopRunConfig{.max_frames = 2});

    MK_REQUIRE(result.status == mirakana::DesktopRunStatus::completed);
    MK_REQUIRE(result.frames_run == 2);
    MK_REQUIRE(app.updates == 2);
    MK_REQUIRE(host.presentation_backend() == mirakana::Win32DesktopPresentationBackend::null_renderer);
    MK_REQUIRE(host.presentation_backend_name() == std::string_view{"null"});
    MK_REQUIRE(host.renderer().stats().frames_finished == 2);
    MK_REQUIRE(host.presentation_report().renderer_stats.frames_finished == 2);
}
#endif

int main() {
    return mirakana::test::run_all();
}
