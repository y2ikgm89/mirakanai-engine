// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"

#include "mirakana/renderer/rhi_frame_renderer.hpp"

#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_D3D12)
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"
#endif

#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_extent(Extent2D extent) noexcept {
    return extent.width > 0 && extent.height > 0;
}

[[nodiscard]] bool valid_swapchain_format(rhi::Format format) noexcept {
    return format == rhi::Format::rgba8_unorm || format == rhi::Format::bgra8_unorm;
}

[[nodiscard]] bool has_shader_bytecode(Win32DesktopPresentationShaderBytecode shader) noexcept {
    return !shader.entry_point.empty() && !shader.bytecode.empty();
}

[[nodiscard]] bool valid_d3d12_renderer_request(const Win32DesktopPresentationD3d12RendererDesc* desc) noexcept {
    return desc != nullptr && has_shader_bytecode(desc->vertex_shader) && has_shader_bytecode(desc->fragment_shader);
}

[[nodiscard]] Win32DesktopPresentationBackend requested_backend(const Win32DesktopPresentationDesc& desc) noexcept {
    return desc.prefer_d3d12 ? Win32DesktopPresentationBackend::d3d12 : Win32DesktopPresentationBackend::null_renderer;
}

[[nodiscard]] Win32DesktopPresentationBackendReport
make_backend_report(Win32DesktopPresentationBackend backend, Win32DesktopPresentationBackendReportStatus status,
                    Win32DesktopPresentationFallbackReason fallback_reason, std::string diagnostic = {}) {
    return Win32DesktopPresentationBackendReport{
        .backend = backend,
        .status = status,
        .fallback_reason = fallback_reason,
        .diagnostic = std::move(diagnostic),
    };
}

[[nodiscard]] Win32DesktopPresentationDiagnostic make_diagnostic(Win32DesktopPresentationFallbackReason reason,
                                                                 std::string message) {
    return Win32DesktopPresentationDiagnostic{.reason = reason, .message = std::move(message)};
}

struct NativeRendererCreateResult {
    bool succeeded{false};
    Win32DesktopPresentationFallbackReason failure_reason{Win32DesktopPresentationFallbackReason::none};
    Win32DesktopPresentationBackendReportStatus failure_status{
        Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable};
    std::string diagnostic;
    std::unique_ptr<rhi::IRhiDevice> device;
    std::unique_ptr<IRenderer> renderer;
    Win32D3d12SwapChainPlan swapchain_plan;
};

[[nodiscard]] NativeRendererCreateResult create_d3d12_renderer(const Win32DesktopPresentationDesc& desc,
                                                               rhi::SurfaceHandle surface) {
    NativeRendererCreateResult result;
    result.swapchain_plan = plan_win32_d3d12_swapchain(Win32D3d12SwapChainPlanDesc{
        .extent = desc.extent,
        .format = rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = desc.vsync,
        .request_tearing = desc.request_tearing,
        .tearing_supported = false,
    });
    if (!result.swapchain_plan.succeeded()) {
        result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
        result.failure_status = Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
        result.diagnostic = result.swapchain_plan.diagnostic;
        return result;
    }

#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_D3D12)
    const auto probe = rhi::d3d12::probe_runtime();
    if (!probe.windows_sdk_available || !probe.dxgi_factory_created ||
        (!probe.hardware_device_supported && !probe.warp_device_supported)) {
        result.failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable;
        result.failure_status = Win32DesktopPresentationBackendReportStatus::native_backend_unavailable;
        result.diagnostic = "D3D12 runtime support is unavailable in this build or host; using NullRenderer fallback.";
        return result;
    }

    try {
        auto device = rhi::d3d12::create_rhi_device(rhi::d3d12::DeviceBootstrapDesc{
            .prefer_warp = desc.prefer_warp,
            .enable_debug_layer = desc.enable_debug_layer,
        });
        if (device == nullptr) {
            result.failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable;
            result.failure_status = Win32DesktopPresentationBackendReportStatus::native_backend_unavailable;
            result.diagnostic = "D3D12 device creation is unavailable; using NullRenderer fallback.";
            return result;
        }

        const auto swapchain = device->create_swapchain(rhi::SwapchainDesc{
            .extent = rhi::Extent2D{.width = desc.extent.width, .height = desc.extent.height},
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .surface = surface,
        });
        const auto pipeline_layout =
            device->create_pipeline_layout(rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
        const auto vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = desc.d3d12_renderer->vertex_shader.entry_point,
            .bytecode_size = desc.d3d12_renderer->vertex_shader.bytecode.size(),
            .bytecode = desc.d3d12_renderer->vertex_shader.bytecode.data(),
        });
        const auto fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = desc.d3d12_renderer->fragment_shader.entry_point,
            .bytecode_size = desc.d3d12_renderer->fragment_shader.bytecode.size(),
            .bytecode = desc.d3d12_renderer->fragment_shader.bytecode.data(),
        });
        const auto pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = rhi::Format::bgra8_unorm,
            .depth_format = rhi::Format::unknown,
            .topology = desc.d3d12_renderer->topology,
            .vertex_buffers = desc.d3d12_renderer->vertex_buffers,
            .vertex_attributes = desc.d3d12_renderer->vertex_attributes,
        });
        auto renderer = std::make_unique<RhiFrameRenderer>(RhiFrameRendererDesc{
            .device = device.get(),
            .extent = desc.extent,
            .color_texture = rhi::TextureHandle{},
            .swapchain = swapchain,
            .graphics_pipeline = pipeline,
            .wait_for_completion = true,
        });

        result.succeeded = true;
        result.device = std::move(device);
        result.renderer = std::move(renderer);
        return result;
    } catch (const std::exception&) {
        result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
        result.failure_status = Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
        result.diagnostic = "D3D12 renderer creation failed; using NullRenderer fallback.";
        return result;
    }
#else
    (void)surface;
    result.failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable;
    result.failure_status = Win32DesktopPresentationBackendReportStatus::native_backend_unavailable;
    result.diagnostic = "D3D12 runtime support is unavailable in this build; using NullRenderer fallback.";
    return result;
#endif
}

[[nodiscard]] std::unique_ptr<IRenderer> make_null_renderer(Extent2D extent) {
    return std::make_unique<NullRenderer>(extent);
}

} // namespace

Win32D3d12SwapChainPlan plan_win32_d3d12_swapchain(const Win32D3d12SwapChainPlanDesc& desc) {
    Win32D3d12SwapChainPlan plan{
        .extent = desc.extent,
        .format = desc.format,
        .buffer_count = desc.buffer_count,
        .uses_create_swap_chain_for_hwnd = true,
        .uses_direct_command_queue = true,
        .flip_discard_swap_effect = true,
        .render_target_output = true,
        .resize_buffers_supported = true,
        .requires_present_state_before_present = true,
        .allow_tearing_flag = false,
        .present_sync_interval = desc.vsync ? 1U : 0U,
        .public_native_handles_exposed = false,
    };

    if (!valid_extent(desc.extent)) {
        plan.diagnostic = "win32 d3d12 swapchain extent must be non-zero";
        return plan;
    }
    if (!valid_swapchain_format(desc.format)) {
        plan.diagnostic = "win32 d3d12 swapchain format must be rgba8_unorm or bgra8_unorm";
        return plan;
    }
    if (desc.buffer_count < 2) {
        plan.diagnostic = "win32 d3d12 swapchain buffer count must be at least 2 for flip model";
        return plan;
    }

    plan.allow_tearing_flag = desc.request_tearing && desc.tearing_supported && !desc.vsync;
    return plan;
}

std::string_view win32_desktop_presentation_backend_name(Win32DesktopPresentationBackend backend) noexcept {
    switch (backend) {
    case Win32DesktopPresentationBackend::null_renderer:
        return "null";
    case Win32DesktopPresentationBackend::d3d12:
        return "d3d12";
    }
    return "unknown";
}

std::string_view
win32_desktop_presentation_fallback_reason_name(Win32DesktopPresentationFallbackReason reason) noexcept {
    switch (reason) {
    case Win32DesktopPresentationFallbackReason::none:
        return "none";
    case Win32DesktopPresentationFallbackReason::native_window_unavailable:
        return "native_window_unavailable";
    case Win32DesktopPresentationFallbackReason::native_backend_unavailable:
        return "native_backend_unavailable";
    case Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable:
        return "runtime_pipeline_unavailable";
    }
    return "unknown";
}

std::string_view
win32_desktop_presentation_backend_report_status_name(Win32DesktopPresentationBackendReportStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationBackendReportStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationBackendReportStatus::missing_request:
        return "missing_request";
    case Win32DesktopPresentationBackendReportStatus::native_window_unavailable:
        return "native_window_unavailable";
    case Win32DesktopPresentationBackendReportStatus::native_backend_unavailable:
        return "native_backend_unavailable";
    case Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable:
        return "runtime_pipeline_unavailable";
    case Win32DesktopPresentationBackendReportStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_present_status_name(Win32DesktopPresentationPresentStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationPresentStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationPresentStatus::ready:
        return "ready";
    case Win32DesktopPresentationPresentStatus::failed:
        return "failed";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_resize_status_name(Win32DesktopPresentationResizeStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationResizeStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationResizeStatus::ready:
        return "ready";
    case Win32DesktopPresentationResizeStatus::failed:
        return "failed";
    case Win32DesktopPresentationResizeStatus::recreate_required:
        return "recreate_required";
    }
    return "unknown";
}

struct Win32DesktopPresentation::Impl {
    std::unique_ptr<rhi::IRhiDevice> device;
    std::unique_ptr<IRenderer> renderer;
    Win32DesktopPresentationBackend backend{Win32DesktopPresentationBackend::null_renderer};
    Win32DesktopPresentationReport report;
    std::vector<Win32DesktopPresentationBackendReport> backend_reports;
    std::vector<Win32DesktopPresentationDiagnostic> diagnostics;
};

Win32DesktopPresentation::Win32DesktopPresentation(const Win32DesktopPresentationDesc& desc)
    : impl_(std::make_unique<Impl>()) {
    if (!valid_extent(desc.extent)) {
        throw std::invalid_argument("win32 desktop presentation extent must be non-zero");
    }

    impl_->report.requested_backend = requested_backend(desc);
    impl_->report.selected_backend = Win32DesktopPresentationBackend::null_renderer;
    impl_->report.allow_null_fallback = desc.allow_null_fallback;
    impl_->report.backbuffer_extent = desc.extent;
    impl_->report.d3d12_tearing_requested = desc.request_tearing;

    if (!desc.prefer_d3d12) {
        impl_->renderer = make_null_renderer(desc.extent);
        impl_->backend_reports.push_back(make_backend_report(Win32DesktopPresentationBackend::null_renderer,
                                                             Win32DesktopPresentationBackendReportStatus::not_requested,
                                                             Win32DesktopPresentationFallbackReason::none));
        impl_->report.backend_reports_count = impl_->backend_reports.size();
        return;
    }

    if (desc.window == nullptr || desc.window->native_window_token() == 0) {
        const std::string diagnostic{"Win32 native window is unavailable; using NullRenderer fallback."};
        if (!desc.allow_null_fallback) {
            throw std::invalid_argument(diagnostic);
        }
        impl_->renderer = make_null_renderer(desc.extent);
        impl_->report.fallback_reason = Win32DesktopPresentationFallbackReason::native_window_unavailable;
        impl_->report.used_null_fallback = true;
        impl_->backend_reports.push_back(
            make_backend_report(Win32DesktopPresentationBackend::d3d12,
                                Win32DesktopPresentationBackendReportStatus::native_window_unavailable,
                                Win32DesktopPresentationFallbackReason::native_window_unavailable, diagnostic));
        impl_->diagnostics.push_back(
            make_diagnostic(Win32DesktopPresentationFallbackReason::native_window_unavailable, diagnostic));
        impl_->report.diagnostics_count = impl_->diagnostics.size();
        impl_->report.backend_reports_count = impl_->backend_reports.size();
        return;
    }

    if (!valid_d3d12_renderer_request(desc.d3d12_renderer)) {
        const std::string diagnostic{"D3D12 renderer shader bytecode request is missing; using NullRenderer fallback."};
        if (!desc.allow_null_fallback) {
            throw std::invalid_argument(diagnostic);
        }
        impl_->renderer = make_null_renderer(desc.extent);
        impl_->report.fallback_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
        impl_->report.used_null_fallback = true;
        impl_->report.swapchain_plan = plan_win32_d3d12_swapchain(Win32D3d12SwapChainPlanDesc{
            .extent = desc.extent,
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .request_tearing = desc.request_tearing,
            .tearing_supported = false,
        });
        impl_->backend_reports.push_back(make_backend_report(
            Win32DesktopPresentationBackend::d3d12, Win32DesktopPresentationBackendReportStatus::missing_request,
            Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable, diagnostic));
        impl_->diagnostics.push_back(
            make_diagnostic(Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable, diagnostic));
        impl_->report.diagnostics_count = impl_->diagnostics.size();
        impl_->report.backend_reports_count = impl_->backend_reports.size();
        return;
    }

    auto create_result = create_d3d12_renderer(desc, rhi::SurfaceHandle{desc.window->native_window_token()});
    impl_->report.swapchain_plan = create_result.swapchain_plan;
    impl_->report.d3d12_tearing_supported = create_result.swapchain_plan.allow_tearing_flag;
    impl_->report.d3d12_tearing_active = create_result.swapchain_plan.allow_tearing_flag;
    if (create_result.succeeded) {
        impl_->backend = Win32DesktopPresentationBackend::d3d12;
        impl_->device = std::move(create_result.device);
        impl_->renderer = std::move(create_result.renderer);
        impl_->report.selected_backend = Win32DesktopPresentationBackend::d3d12;
        impl_->backend_reports.push_back(make_backend_report(Win32DesktopPresentationBackend::d3d12,
                                                             Win32DesktopPresentationBackendReportStatus::ready,
                                                             Win32DesktopPresentationFallbackReason::none));
        impl_->report.backend_reports_count = impl_->backend_reports.size();
        return;
    }

    if (!desc.allow_null_fallback) {
        throw std::invalid_argument(create_result.diagnostic);
    }
    impl_->renderer = make_null_renderer(desc.extent);
    impl_->report.fallback_reason = create_result.failure_reason;
    impl_->report.used_null_fallback = true;
    impl_->backend_reports.push_back(make_backend_report(Win32DesktopPresentationBackend::d3d12,
                                                         create_result.failure_status, create_result.failure_reason,
                                                         create_result.diagnostic));
    impl_->diagnostics.push_back(make_diagnostic(create_result.failure_reason, create_result.diagnostic));
    impl_->report.diagnostics_count = impl_->diagnostics.size();
    impl_->report.backend_reports_count = impl_->backend_reports.size();
}

Win32DesktopPresentation::~Win32DesktopPresentation() = default;

IRenderer& Win32DesktopPresentation::renderer() noexcept {
    return *impl_->renderer;
}

const IRenderer& Win32DesktopPresentation::renderer() const noexcept {
    return *impl_->renderer;
}

Win32DesktopPresentationBackend Win32DesktopPresentation::backend() const noexcept {
    return impl_->backend;
}

std::string_view Win32DesktopPresentation::backend_name() const noexcept {
    return win32_desktop_presentation_backend_name(impl_->backend);
}

Win32DesktopPresentationReport Win32DesktopPresentation::report() const {
    auto report = impl_->report;
    if (impl_->renderer != nullptr) {
        report.renderer_stats = impl_->renderer->stats();
        report.backbuffer_extent = impl_->renderer->backbuffer_extent();
    }
    if (impl_->device != nullptr) {
        report.rhi_stats = impl_->device->stats();
        if (report.rhi_stats.present_calls > 0) {
            report.present_status = Win32DesktopPresentationPresentStatus::ready;
        }
        if (report.rhi_stats.swapchain_resizes > 0) {
            report.resize_status = Win32DesktopPresentationResizeStatus::ready;
        }
    }
    report.diagnostics_count = impl_->diagnostics.size();
    report.backend_reports_count = impl_->backend_reports.size();
    return report;
}

const std::vector<Win32DesktopPresentationBackendReport>& Win32DesktopPresentation::backend_reports() const noexcept {
    return impl_->backend_reports;
}

const std::vector<Win32DesktopPresentationDiagnostic>& Win32DesktopPresentation::diagnostics() const noexcept {
    return impl_->diagnostics;
}

} // namespace mirakana
