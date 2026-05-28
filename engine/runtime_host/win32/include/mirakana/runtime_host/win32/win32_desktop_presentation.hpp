// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/win32/win32_window.hpp"
#include "mirakana/renderer/renderer.hpp"
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
    std::size_t material_bindings{0};
    std::size_t mesh_uploads{0};
    std::size_t texture_uploads{0};
    std::size_t material_uploads{0};
    std::size_t material_pipeline_layouts{0};
    std::uint64_t uploaded_texture_bytes{0};
    std::uint64_t uploaded_mesh_bytes{0};
    std::uint64_t uploaded_material_factor_bytes{0};
    std::size_t mesh_bindings_resolved{0};
    std::size_t material_bindings_resolved{0};
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

struct Win32DesktopPresentationShaderBytecode {
    std::string_view entry_point;
    std::span<const std::uint8_t> bytecode;
};

struct Win32DesktopPresentationD3d12RendererDesc {
    Win32DesktopPresentationShaderBytecode vertex_shader;
    Win32DesktopPresentationShaderBytecode fragment_shader;
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
};

struct Win32DesktopPresentationD3d12SceneRendererDesc {
    Win32DesktopPresentationShaderBytecode vertex_shader;
    Win32DesktopPresentationShaderBytecode fragment_shader;
    const runtime::RuntimeAssetPackage* package{nullptr};
    const SceneRenderPacket* packet{nullptr};
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
};

struct Win32DesktopPresentationBackendReport {
    Win32DesktopPresentationBackend backend{Win32DesktopPresentationBackend::null_renderer};
    Win32DesktopPresentationBackendReportStatus status{Win32DesktopPresentationBackendReportStatus::not_requested};
    Win32DesktopPresentationFallbackReason fallback_reason{Win32DesktopPresentationFallbackReason::none};
    std::string diagnostic;
};

struct Win32DesktopPresentationDiagnostic {
    Win32DesktopPresentationFallbackReason reason{Win32DesktopPresentationFallbackReason::none};
    std::string message;
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
    RendererStats renderer_stats;
    rhi::RhiStats rhi_stats;
    Extent2D backbuffer_extent;
    std::size_t diagnostics_count{0};
    std::size_t backend_reports_count{0};
    std::size_t scene_gpu_diagnostics_count{0};
};

struct Win32DesktopPresentationDesc {
    win32::Win32Window* window{nullptr};
    Extent2D extent;
    bool prefer_d3d12{true};
    bool allow_null_fallback{true};
    bool prefer_warp{false};
    bool enable_debug_layer{false};
    bool vsync{true};
    bool request_tearing{false};
    const Win32DesktopPresentationD3d12RendererDesc* d3d12_renderer{nullptr};
    const Win32DesktopPresentationD3d12SceneRendererDesc* d3d12_scene_renderer{nullptr};
};

[[nodiscard]] Win32D3d12SwapChainPlan plan_win32_d3d12_swapchain(const Win32D3d12SwapChainPlanDesc& desc);
[[nodiscard]] std::string_view
win32_desktop_presentation_backend_name(Win32DesktopPresentationBackend backend) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_fallback_reason_name(Win32DesktopPresentationFallbackReason reason) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_backend_report_status_name(Win32DesktopPresentationBackendReportStatus status) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_present_status_name(Win32DesktopPresentationPresentStatus status) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_resize_status_name(Win32DesktopPresentationResizeStatus status) noexcept;
[[nodiscard]] std::string_view
win32_desktop_presentation_scene_gpu_binding_status_name(Win32DesktopPresentationSceneGpuBindingStatus status) noexcept;

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
    [[nodiscard]] Win32DesktopPresentationReport report() const;
    [[nodiscard]] const std::vector<Win32DesktopPresentationBackendReport>& backend_reports() const noexcept;
    [[nodiscard]] const std::vector<Win32DesktopPresentationDiagnostic>& diagnostics() const noexcept;
    [[nodiscard]] Win32DesktopPresentationSceneGpuBindingStatus scene_gpu_binding_status() const noexcept;
    [[nodiscard]] bool scene_gpu_bindings_ready() const noexcept;
    [[nodiscard]] Win32DesktopPresentationSceneGpuBindingStats scene_gpu_binding_stats() const noexcept;
    [[nodiscard]] std::span<const Win32DesktopPresentationSceneGpuBindingDiagnostic>
    scene_gpu_binding_diagnostics() const noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana
