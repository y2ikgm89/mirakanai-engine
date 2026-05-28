// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"

#include "mirakana/renderer/rhi_frame_renderer.hpp"
#include "win32_scene_gpu_binding_injecting_renderer.hpp"

#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_D3D12)
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"
#endif

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

using runtime_host_win32_detail::Win32SceneGpuBindingInjectingRenderer;

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

[[nodiscard]] bool
valid_d3d12_scene_renderer_request(const Win32DesktopPresentationD3d12SceneRendererDesc* desc) noexcept {
    return desc != nullptr && has_shader_bytecode(desc->vertex_shader) && has_shader_bytecode(desc->fragment_shader) &&
           desc->package != nullptr && desc->packet != nullptr && !desc->packet->meshes.empty() &&
           !desc->vertex_buffers.empty() && !desc->vertex_attributes.empty();
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

[[nodiscard]] Win32DesktopPresentationSceneGpuBindingDiagnostic
make_scene_gpu_diagnostic(Win32DesktopPresentationSceneGpuBindingStatus status, std::string message) {
    return Win32DesktopPresentationSceneGpuBindingDiagnostic{.status = status, .message = std::move(message)};
}

[[nodiscard]] bool same_vertex_buffer_layout(const rhi::VertexBufferLayoutDesc& lhs,
                                             const rhi::VertexBufferLayoutDesc& rhs) noexcept {
    return lhs.binding == rhs.binding && lhs.stride == rhs.stride && lhs.input_rate == rhs.input_rate;
}

[[nodiscard]] bool same_vertex_attribute(const rhi::VertexAttributeDesc& lhs,
                                         const rhi::VertexAttributeDesc& rhs) noexcept {
    return lhs.location == rhs.location && lhs.binding == rhs.binding && lhs.offset == rhs.offset &&
           lhs.format == rhs.format && lhs.semantic == rhs.semantic && lhs.semantic_index == rhs.semantic_index;
}

[[nodiscard]] bool same_vertex_buffer_layouts(std::span<const rhi::VertexBufferLayoutDesc> lhs,
                                              std::span<const rhi::VertexBufferLayoutDesc> rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (const auto& expected : rhs) {
        const auto found = std::find_if(lhs.begin(), lhs.end(), [&expected](const rhi::VertexBufferLayoutDesc& item) {
            return same_vertex_buffer_layout(item, expected);
        });
        if (found == lhs.end()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool same_vertex_attributes(std::span<const rhi::VertexAttributeDesc> lhs,
                                          std::span<const rhi::VertexAttributeDesc> rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (const auto& expected : rhs) {
        const auto found = std::find_if(lhs.begin(), lhs.end(), [&expected](const rhi::VertexAttributeDesc& item) {
            return same_vertex_attribute(item, expected);
        });
        if (found == lhs.end()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string
validate_static_scene_renderer_mesh_layout_request(const runtime::RuntimeAssetPackage& package,
                                                   const SceneRenderPacket& packet,
                                                   std::span<const rhi::VertexBufferLayoutDesc> static_vertex_buffers,
                                                   std::span<const rhi::VertexAttributeDesc> static_vertex_attributes) {
    for (const auto& mesh : packet.meshes) {
        const auto* record = package.find(mesh.renderer.mesh);
        if (record == nullptr) {
            return "Win32 D3D12 scene renderer request references a mesh without cooked payload layout metadata.";
        }
        if (record->kind != AssetKind::mesh) {
            return "Win32 D3D12 scene renderer request currently supports static mesh package rows only.";
        }
        const auto payload = runtime::runtime_mesh_payload(*record);
        if (!payload.succeeded()) {
            return "Win32 D3D12 scene renderer request mesh payload is invalid: " + payload.diagnostic;
        }
        const auto layout = runtime_rhi::make_runtime_mesh_vertex_layout_desc(payload.payload);
        if (!layout.succeeded()) {
            return "Win32 D3D12 scene renderer request mesh layout is invalid: " + layout.diagnostic;
        }
        if (!same_vertex_buffer_layouts(static_vertex_buffers, layout.vertex_buffers) ||
            !same_vertex_attributes(static_vertex_attributes, layout.vertex_attributes)) {
            return "Win32 D3D12 scene renderer request vertex input does not match cooked mesh payload layout.";
        }
    }
    return {};
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
    Win32DesktopPresentationSceneGpuBindingStatus scene_gpu_status{
        Win32DesktopPresentationSceneGpuBindingStatus::not_requested};
    std::vector<Win32DesktopPresentationSceneGpuBindingDiagnostic> scene_gpu_diagnostics;
    Win32SceneGpuBindingInjectingRenderer* scene_gpu_renderer{nullptr};
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

[[nodiscard]] NativeRendererCreateResult missing_d3d12_scene_renderer_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .failure_status = Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable,
        .diagnostic = "Win32 D3D12 scene renderer creation requires non-empty shader bytecode, package, render packet, "
                      "and vertex input metadata; using NullRenderer fallback.",
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
    return result;
}

[[nodiscard]] NativeRendererCreateResult create_d3d12_scene_renderer(const Win32DesktopPresentationDesc& desc,
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
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        return result;
    }
    if (!valid_d3d12_scene_renderer_request(desc.d3d12_scene_renderer)) {
        result = missing_d3d12_scene_renderer_request();
        result.swapchain_plan = plan_win32_d3d12_swapchain(Win32D3d12SwapChainPlanDesc{
            .extent = desc.extent,
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .request_tearing = desc.request_tearing,
            .tearing_supported = false,
        });
        return result;
    }

#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_D3D12)
    const auto probe = rhi::d3d12::probe_runtime();
    if (!probe.windows_sdk_available || !probe.dxgi_factory_created ||
        (!probe.hardware_device_supported && !probe.warp_device_supported)) {
        result.failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable;
        result.failure_status = Win32DesktopPresentationBackendReportStatus::native_backend_unavailable;
        result.diagnostic = "D3D12 runtime support is unavailable in this build or host; using NullRenderer fallback.";
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
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
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            return result;
        }

        const auto layout_diagnostic = validate_static_scene_renderer_mesh_layout_request(
            *desc.d3d12_scene_renderer->package, *desc.d3d12_scene_renderer->packet,
            desc.d3d12_scene_renderer->vertex_buffers, desc.d3d12_scene_renderer->vertex_attributes);
        if (!layout_diagnostic.empty()) {
            result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
            result.failure_status = Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
            result.diagnostic = layout_diagnostic + " Using NullRenderer fallback.";
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            return result;
        }

        const auto swapchain = device->create_swapchain(rhi::SwapchainDesc{
            .extent = rhi::Extent2D{.width = desc.extent.width, .height = desc.extent.height},
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .surface = surface,
        });
        auto gpu_upload =
            runtime_scene_rhi::execute_runtime_scene_gpu_upload(runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc{
                .device = device.get(),
                .package = desc.d3d12_scene_renderer->package,
                .packet = desc.d3d12_scene_renderer->packet,
                .binding_options = {},
            });
        auto& gpu_bindings = gpu_upload.bindings;
        if (!gpu_bindings.succeeded()) {
            result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
            result.failure_status = Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
            result.diagnostic = "Win32 D3D12 scene GPU binding creation failed; using NullRenderer fallback.";
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            for (const auto& failure : gpu_bindings.failures) {
                result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(
                    result.scene_gpu_status, "scene GPU binding failure asset=" + std::to_string(failure.asset.value) +
                                                 ": " + failure.diagnostic));
            }
            return result;
        }
        if (gpu_bindings.material_pipeline_layouts.empty()) {
            result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
            result.failure_status = Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
            result.diagnostic =
                "Win32 D3D12 scene GPU binding creation did not produce a material pipeline layout; using "
                "NullRenderer fallback.";
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            return result;
        }

        const auto vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = desc.d3d12_scene_renderer->vertex_shader.entry_point,
            .bytecode_size = desc.d3d12_scene_renderer->vertex_shader.bytecode.size(),
            .bytecode = desc.d3d12_scene_renderer->vertex_shader.bytecode.data(),
        });
        const auto fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = desc.d3d12_scene_renderer->fragment_shader.entry_point,
            .bytecode_size = desc.d3d12_scene_renderer->fragment_shader.bytecode.size(),
            .bytecode = desc.d3d12_scene_renderer->fragment_shader.bytecode.data(),
        });
        const auto pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = gpu_bindings.material_pipeline_layouts.front(),
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = rhi::Format::bgra8_unorm,
            .depth_format = rhi::Format::unknown,
            .topology = desc.d3d12_scene_renderer->topology,
            .vertex_buffers = desc.d3d12_scene_renderer->vertex_buffers,
            .vertex_attributes = desc.d3d12_scene_renderer->vertex_attributes,
        });
        auto frame_renderer = std::make_unique<RhiFrameRenderer>(RhiFrameRendererDesc{
            .device = device.get(),
            .extent = desc.extent,
            .color_texture = rhi::TextureHandle{},
            .swapchain = swapchain,
            .graphics_pipeline = pipeline,
            .wait_for_completion = true,
        });
        auto scene_renderer =
            std::make_unique<Win32SceneGpuBindingInjectingRenderer>(std::move(frame_renderer), std::move(gpu_bindings));
        auto* scene_renderer_ptr = scene_renderer.get();

        result.succeeded = true;
        result.device = std::move(device);
        result.renderer = std::move(scene_renderer);
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::ready;
        result.scene_gpu_renderer = scene_renderer_ptr;
        return result;
    } catch (const std::exception& exception) {
        result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
        result.failure_status = Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
        result.diagnostic = "Win32 D3D12 scene renderer creation failed: " + std::string(exception.what()) +
                            "; using NullRenderer fallback.";
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        return result;
    }
#else
    (void)surface;
    result.failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable;
    result.failure_status = Win32DesktopPresentationBackendReportStatus::native_backend_unavailable;
    result.diagnostic = "D3D12 runtime support is unavailable in this build; using NullRenderer fallback.";
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
    result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
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

std::string_view win32_desktop_presentation_scene_gpu_binding_status_name(
    Win32DesktopPresentationSceneGpuBindingStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationSceneGpuBindingStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationSceneGpuBindingStatus::unavailable:
        return "unavailable";
    case Win32DesktopPresentationSceneGpuBindingStatus::invalid_request:
        return "invalid_request";
    case Win32DesktopPresentationSceneGpuBindingStatus::failed:
        return "failed";
    case Win32DesktopPresentationSceneGpuBindingStatus::ready:
        return "ready";
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
    Win32DesktopPresentationSceneGpuBindingStatus scene_gpu_status{
        Win32DesktopPresentationSceneGpuBindingStatus::not_requested};
    std::vector<Win32DesktopPresentationSceneGpuBindingDiagnostic> scene_gpu_diagnostics;
    Win32SceneGpuBindingInjectingRenderer* scene_gpu_renderer{nullptr};
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

    if (desc.d3d12_scene_renderer != nullptr) {
        auto create_result = create_d3d12_scene_renderer(desc, rhi::SurfaceHandle{desc.window->native_window_token()});
        impl_->report.swapchain_plan = create_result.swapchain_plan;
        impl_->report.d3d12_tearing_supported = create_result.swapchain_plan.allow_tearing_flag;
        impl_->report.d3d12_tearing_active = create_result.swapchain_plan.allow_tearing_flag;
        impl_->scene_gpu_status = create_result.scene_gpu_status;
        impl_->scene_gpu_diagnostics = std::move(create_result.scene_gpu_diagnostics);
        impl_->scene_gpu_renderer = create_result.scene_gpu_renderer;
        if (create_result.succeeded) {
            impl_->backend = Win32DesktopPresentationBackend::d3d12;
            impl_->device = std::move(create_result.device);
            impl_->renderer = std::move(create_result.renderer);
            impl_->report.selected_backend = Win32DesktopPresentationBackend::d3d12;
            impl_->backend_reports.push_back(make_backend_report(Win32DesktopPresentationBackend::d3d12,
                                                                 Win32DesktopPresentationBackendReportStatus::ready,
                                                                 Win32DesktopPresentationFallbackReason::none));
            impl_->report.backend_reports_count = impl_->backend_reports.size();
            impl_->report.scene_gpu_diagnostics_count = impl_->scene_gpu_diagnostics.size();
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
        impl_->report.scene_gpu_diagnostics_count = impl_->scene_gpu_diagnostics.size();
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
    report.scene_gpu_status = impl_->scene_gpu_status;
    report.scene_gpu_stats = scene_gpu_binding_stats();
    report.diagnostics_count = impl_->diagnostics.size();
    report.backend_reports_count = impl_->backend_reports.size();
    report.scene_gpu_diagnostics_count = impl_->scene_gpu_diagnostics.size();
    return report;
}

const std::vector<Win32DesktopPresentationBackendReport>& Win32DesktopPresentation::backend_reports() const noexcept {
    return impl_->backend_reports;
}

const std::vector<Win32DesktopPresentationDiagnostic>& Win32DesktopPresentation::diagnostics() const noexcept {
    return impl_->diagnostics;
}

Win32DesktopPresentationSceneGpuBindingStatus Win32DesktopPresentation::scene_gpu_binding_status() const noexcept {
    return impl_->scene_gpu_status;
}

bool Win32DesktopPresentation::scene_gpu_bindings_ready() const noexcept {
    return impl_->scene_gpu_status == Win32DesktopPresentationSceneGpuBindingStatus::ready;
}

Win32DesktopPresentationSceneGpuBindingStats Win32DesktopPresentation::scene_gpu_binding_stats() const noexcept {
    if (impl_->scene_gpu_renderer != nullptr) {
        return impl_->scene_gpu_renderer->scene_gpu_binding_stats();
    }
    return {};
}

std::span<const Win32DesktopPresentationSceneGpuBindingDiagnostic>
Win32DesktopPresentation::scene_gpu_binding_diagnostics() const noexcept {
    return impl_->scene_gpu_diagnostics;
}

} // namespace mirakana
