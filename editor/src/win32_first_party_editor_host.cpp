// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "win32_first_party_editor_host.hpp"

#include "first_party_editor_document.hpp"
#include "native_editor_app.hpp"
#include "native_editor_text_atlas_handoff.hpp"
#include "native_editor_visible_texture_compositor.hpp"
#include "native_editor_win32_services.hpp"

#include "mirakana/platform/win32/win32_event_pump.hpp"
#include "mirakana/platform/win32/win32_runtime.hpp"
#include "mirakana/platform/win32/win32_window.hpp"
#include "mirakana/platform/window.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifndef MK_EDITOR_VISIBLE_TEXTURE_VERTEX_SHADER_PATH
#define MK_EDITOR_VISIBLE_TEXTURE_VERTEX_SHADER_PATH ""
#endif
#ifndef MK_EDITOR_VISIBLE_TEXTURE_FRAGMENT_SHADER_PATH
#define MK_EDITOR_VISIBLE_TEXTURE_FRAGMENT_SHADER_PATH ""
#endif

namespace mirakana::editor {
namespace {

using Microsoft::WRL::ComPtr;

[[nodiscard]] HWND window_from_token(std::uintptr_t token) noexcept {
    return reinterpret_cast<HWND>(token);
}

[[nodiscard]] std::uint32_t narrow_size(std::size_t value) noexcept {
    return static_cast<std::uint32_t>(
        std::min<std::size_t>(value, static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())));
}

[[nodiscard]] bool adapter_supports_d3d12(IUnknown* adapter) noexcept {
    ComPtr<ID3D12Device> device;
    return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
}

[[nodiscard]] bool find_hardware_adapter(IDXGIFactory6* factory, ComPtr<IDXGIAdapter1>& selected) noexcept {
    for (UINT adapter_index = 0;; ++adapter_index) {
        ComPtr<IDXGIAdapter1> adapter;
        const HRESULT result = factory->EnumAdapters1(adapter_index, &adapter);
        if (result == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(result)) {
            continue;
        }

        DXGI_ADAPTER_DESC1 desc{};
        if (FAILED(adapter->GetDesc1(&desc))) {
            continue;
        }
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0U) {
            continue;
        }
        if (!adapter_supports_d3d12(adapter.Get())) {
            continue;
        }

        selected = adapter;
        return true;
    }
    return false;
}

[[nodiscard]] bool find_warp_adapter(IDXGIFactory6* factory, ComPtr<IDXGIAdapter>& selected) noexcept {
    if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&selected)))) {
        return false;
    }
    return adapter_supports_d3d12(selected.Get());
}

[[nodiscard]] bool enable_debug_layer() noexcept {
    ComPtr<ID3D12Debug> debug;
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        return false;
    }
    debug->EnableDebugLayer();
    return true;
}

[[nodiscard]] Win32FirstPartyEditorAdapterKind select_adapter_kind(bool request_debug_layer,
                                                                   std::string& diagnostic) noexcept {
    UINT factory_flags = 0;
    if (request_debug_layer && enable_debug_layer()) {
        factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory)))) {
        diagnostic = "DXGI factory creation failed; using NullRenderer fallback";
        return Win32FirstPartyEditorAdapterKind::null_renderer;
    }

    ComPtr<IDXGIAdapter1> hardware_adapter;
    if (find_hardware_adapter(factory.Get(), hardware_adapter)) {
        return Win32FirstPartyEditorAdapterKind::hardware;
    }

    ComPtr<IDXGIAdapter> warp_adapter;
    if (find_warp_adapter(factory.Get(), warp_adapter)) {
        return Win32FirstPartyEditorAdapterKind::warp;
    }

    diagnostic = "no D3D12-capable hardware or WARP adapter is available; using NullRenderer fallback";
    return Win32FirstPartyEditorAdapterKind::null_renderer;
}

[[nodiscard]] bool d3d12_adapter_selected(Win32FirstPartyEditorAdapterKind kind) noexcept {
    return kind == Win32FirstPartyEditorAdapterKind::hardware || kind == Win32FirstPartyEditorAdapterKind::warp;
}

[[nodiscard]] mirakana::rhi::Extent2D to_rhi_extent(ViewportExtent extent) noexcept {
    return mirakana::rhi::Extent2D{.width = extent.width, .height = extent.height};
}

[[nodiscard]] std::vector<std::uint8_t> read_shader_bytecode(std::string_view path) {
    if (path.empty()) {
        return {};
    }
    std::ifstream file{std::string{path}, std::ios::binary};
    if (!file) {
        return {};
    }
    return std::vector<std::uint8_t>{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

} // namespace

struct Win32FirstPartyEditorHost::Impl {
    explicit Impl(Win32FirstPartyEditorHostDesc host_desc) : desc(host_desc) {}

    Win32FirstPartyEditorHostDesc desc;
    Win32FirstPartyEditorRunResult result;
    std::unique_ptr<mirakana::win32::Win32Runtime> runtime;
    std::unique_ptr<mirakana::win32::Win32Window> window;
    std::unique_ptr<NativeEditorWin32Services> services;
    std::unique_ptr<mirakana::NullRenderer> renderer;
    std::unique_ptr<mirakana::rhi::IRhiDevice> visible_texture_device;
    mirakana::rhi::SwapchainHandle visible_texture_swapchain;
    std::unique_ptr<NativeTextureDisplayAdapter> viewport_texture_adapter;
    std::unique_ptr<NativeTextureDisplayAdapter> material_preview_texture_adapter;
    std::unique_ptr<NativeEditorVisibleTextureCompositor> visible_texture_compositor;
    std::optional<mirakana::ui::RetainedUiSnapshot> previous_retained_ui_snapshot;
    mirakana::win32::Win32EventPump event_pump;
    HWND hwnd{nullptr};
    bool smoke_resize_completed{false};
    bool initialized{false};

    void initialize(NativeEditorApp& app) {
        if (initialized) {
            return;
        }
        result = Win32FirstPartyEditorRunResult{};
        previous_retained_ui_snapshot.reset();
        create_window(app);
        create_renderer_probe();
        initialized = true;
    }

    void create_window(NativeEditorApp& app) {
        runtime = std::make_unique<mirakana::win32::Win32Runtime>(mirakana::win32::Win32RuntimeDesc{
            .window_class_name = "MIRAIKANAI First-Party Editor Shell",
            .dpi_aware = true,
        });
        window = std::make_unique<mirakana::win32::Win32Window>(
            *runtime, mirakana::WindowDesc{
                          .title = "MIRAIKANAI Editor",
                          .extent = mirakana::WindowExtent{.width = desc.launch.width, .height = desc.launch.height},
                      });
        hwnd = window_from_token(window->native_window_token());
        if (hwnd == nullptr) {
            throw std::runtime_error("first-party editor window creation returned a null HWND");
        }
        services = std::make_unique<NativeEditorWin32Services>(window->native_window_token());
        services->bind(app);
        if (desc.launch.smoke_frames < 0) {
            ShowWindow(hwnd, SW_SHOWDEFAULT);
            UpdateWindow(hwnd);
        }
    }

    void create_renderer_probe() {
        std::string adapter_diagnostic;
        result.adapter_kind = select_adapter_kind(desc.enable_debug_layer, adapter_diagnostic);
        if (!adapter_diagnostic.empty()) {
            result.diagnostic = std::move(adapter_diagnostic);
        }
        renderer = std::make_unique<mirakana::NullRenderer>(
            mirakana::Extent2D{.width = desc.launch.width, .height = desc.launch.height});
        renderer->set_clear_color(mirakana::Color{.r = 0.06F, .g = 0.07F, .b = 0.08F, .a = 1.0F});
        create_visible_texture_compositor();
    }

    void create_visible_texture_compositor() {
        if (!d3d12_adapter_selected(result.adapter_kind)) {
            return;
        }
        const auto vertex_shader = read_shader_bytecode(MK_EDITOR_VISIBLE_TEXTURE_VERTEX_SHADER_PATH);
        const auto fragment_shader = read_shader_bytecode(MK_EDITOR_VISIBLE_TEXTURE_FRAGMENT_SHADER_PATH);
        if (vertex_shader.empty() || fragment_shader.empty()) {
            result.diagnostic = "native editor visible texture compositor shader bytecode is unavailable";
            return;
        }

        try {
            auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
                .prefer_warp = result.adapter_kind == Win32FirstPartyEditorAdapterKind::warp,
                .enable_debug_layer = desc.enable_debug_layer,
            });
            if (device == nullptr) {
                result.diagnostic = "native editor visible texture compositor D3D12 device creation failed";
                return;
            }
            const auto extent =
                mirakana::editor::ViewportExtent{.width = desc.launch.width, .height = desc.launch.height};
            const auto swapchain = device->create_swapchain(mirakana::rhi::SwapchainDesc{
                .extent = to_rhi_extent(extent),
                .format = mirakana::rhi::Format::bgra8_unorm,
                .buffer_count = 2,
                .vsync = true,
                .surface = mirakana::rhi::SurfaceHandle{window->native_window_token()},
            });
            viewport_texture_adapter = std::make_unique<NativeTextureDisplayAdapter>(NativeTextureDisplayAdapterDesc{
                .device = device.get(),
                .extent = extent,
                .d3d12_host_available = true,
                .renderer_output_available = true,
                .backend_id = "d3d12",
            });
            material_preview_texture_adapter =
                std::make_unique<NativeTextureDisplayAdapter>(NativeTextureDisplayAdapterDesc{
                    .device = device.get(),
                    .extent = extent,
                    .d3d12_host_available = true,
                    .renderer_output_available = true,
                    .shader_artifacts_available = true,
                    .gpu_payload_available = true,
                    .backend_id = "d3d12",
                });
            visible_texture_compositor =
                std::make_unique<NativeEditorVisibleTextureCompositor>(NativeEditorVisibleTextureCompositorDesc{
                    .device = device.get(),
                    .swapchain = swapchain,
                    .extent = extent,
                    .vertex_shader_entry_point = "vs_main",
                    .vertex_shader_bytecode = vertex_shader,
                    .fragment_shader_entry_point = "ps_main",
                    .fragment_shader_bytecode = fragment_shader,
                    .backend_id = "d3d12",
                });
            visible_texture_swapchain = swapchain;
            visible_texture_device = std::move(device);
        } catch (const std::exception& error) {
            result.diagnostic = error.what();
            visible_texture_compositor.reset();
            material_preview_texture_adapter.reset();
            viewport_texture_adapter.reset();
            visible_texture_device.reset();
            visible_texture_swapchain = {};
        }
    }

    void poll_window_messages() {
        const auto events = event_pump.poll();
        for (const auto& event : events) {
            if (window != nullptr) {
                window->handle_event(event);
            }
            if (event.kind == mirakana::win32::Win32WindowEventKind::resized ||
                event.kind == mirakana::win32::Win32WindowEventKind::restored ||
                event.kind == mirakana::win32::Win32WindowEventKind::maximized) {
                if (event.extent.width > 0 && event.extent.height > 0 &&
                    (event.extent.width != desc.launch.width || event.extent.height != desc.launch.height)) {
                    resize(event.extent.width, event.extent.height);
                }
            }
        }
    }

    void resize(std::uint32_t width, std::uint32_t height) {
        desc.launch.width = width;
        desc.launch.height = height;
        if (renderer != nullptr) {
            renderer->resize(mirakana::Extent2D{.width = width, .height = height});
        }
        if (visible_texture_device != nullptr && visible_texture_swapchain.value != 0U) {
            visible_texture_device->resize_swapchain(visible_texture_swapchain,
                                                     mirakana::rhi::Extent2D{.width = width, .height = height});
        }
        const auto extent = mirakana::editor::ViewportExtent{.width = width, .height = height};
        if (viewport_texture_adapter != nullptr) {
            viewport_texture_adapter->resize(extent);
        }
        if (material_preview_texture_adapter != nullptr) {
            material_preview_texture_adapter->resize(extent);
        }
        if (visible_texture_compositor != nullptr) {
            visible_texture_compositor->resize(extent);
        }
        ++result.resize_count;
    }

    void maybe_run_smoke_resize() {
        if (!desc.launch.smoke_resize || smoke_resize_completed || result.frames_rendered != 1U) {
            return;
        }

        const mirakana::WindowExtent resized{
            .width = desc.launch.width + 64U,
            .height = desc.launch.height + 64U,
        };
        window->resize(resized);
        resize(resized.width, resized.height);
        smoke_resize_completed = true;
    }

    [[nodiscard]] bool render_one_frame(NativeEditorApp& app) {
        poll_window_messages();
        if (!window->is_open()) {
            return false;
        }
        maybe_run_smoke_resize();

        app.record_native_frame();
        if (d3d12_adapter_selected(result.adapter_kind)) {
            app.record_native_resource_device_ready(result.frames_rendered + 1U);
            if (visible_texture_compositor != nullptr && viewport_texture_adapter != nullptr &&
                material_preview_texture_adapter != nullptr) {
                app.record_native_viewport_texture_display(visible_texture_compositor->render_viewport_frame(
                    *viewport_texture_adapter, result.frames_rendered + 1U));
                app.record_native_material_preview_texture_display(
                    visible_texture_compositor->render_material_preview_frame(*material_preview_texture_adapter,
                                                                              result.frames_rendered + 1U));
            } else {
                app.record_native_viewport_d3d12_host_ready(result.frames_rendered + 1U);
                app.record_native_material_preview_d3d12_host_ready(result.frames_rendered + 1U);
            }
            app.record_native_text_atlas_handoff_evidence(
                make_native_editor_directwrite_text_atlas_handoff_evidence(NativeEditorTextAtlasHandoffDesc{}));
        }

        const auto document = make_first_party_editor_document(app);
        auto retained_ui_snapshot =
            mirakana::ui::make_retained_ui_snapshot(document.document, document.layout, document.renderer_submission);
        mirakana::ui::RetainedUiDiffRequest retained_ui_diff_request;
        retained_ui_diff_request.has_previous = previous_retained_ui_snapshot.has_value();
        if (previous_retained_ui_snapshot.has_value()) {
            retained_ui_diff_request.previous = *previous_retained_ui_snapshot;
        }
        retained_ui_diff_request.current = retained_ui_snapshot;
        app.record_native_retained_ui_diff(mirakana::ui::diff_retained_ui_snapshots(retained_ui_diff_request));
        previous_retained_ui_snapshot = std::move(retained_ui_snapshot);

        app.record_native_panels_rendered(document.panel_root_count);
        app.record_native_docking_frame(document.docking_status, document.tab_header_count, document.split_gutter_count,
                                        document.active_panel_count, document.focusable_dock_control_count);
        (void)app.publish_native_accessibility_payload(
            mirakana::ui::build_accessibility_payload(document.renderer_submission), document.focused_element);

        renderer->begin_frame();
        const auto submit = mirakana::submit_ui_renderer_submission(*renderer, document.renderer_submission,
                                                                    mirakana::UiRenderSubmitDesc{});
        renderer->end_frame();

        result.renderer_boxes_submitted = narrow_size(submit.boxes_submitted);
        result.renderer_text_runs_available = narrow_size(submit.text_runs_available);
        ++result.frames_rendered;
        return true;
    }

    Win32FirstPartyEditorRunResult run(NativeEditorApp& app) {
        try {
            initialize(app);
            const auto target_frames =
                desc.launch.smoke_frames > 0 ? static_cast<std::uint32_t>(desc.launch.smoke_frames) : 0U;
            if (target_frames > 0) {
                while (window->is_open() && result.frames_rendered < target_frames) {
                    if (!render_one_frame(app)) {
                        break;
                    }
                }
                result.succeeded = result.frames_rendered == target_frames;
                result.exit_code = result.succeeded ? 0 : 1;
                if (!result.succeeded && result.diagnostic.empty()) {
                    result.diagnostic = "first-party editor smoke loop exited before the requested frame count";
                }
                return result;
            }

            while (window->is_open()) {
                if (!render_one_frame(app)) {
                    break;
                }
            }
            result.succeeded = true;
            result.exit_code = 0;
        } catch (const std::exception& error) {
            result.succeeded = false;
            result.exit_code = 1;
            if (result.diagnostic.empty()) {
                result.diagnostic = error.what();
            }
        }
        return result;
    }

    void shutdown() noexcept {
        visible_texture_compositor.reset();
        material_preview_texture_adapter.reset();
        viewport_texture_adapter.reset();
        visible_texture_device.reset();
        renderer.reset();
        services.reset();
        window.reset();
        runtime.reset();
        hwnd = nullptr;
        initialized = false;
    }
};

Win32FirstPartyEditorHost::Win32FirstPartyEditorHost(Win32FirstPartyEditorHostDesc desc)
    : impl_(std::make_unique<Impl>(desc)) {}

Win32FirstPartyEditorHost::~Win32FirstPartyEditorHost() {
    if (impl_ != nullptr) {
        impl_->shutdown();
    }
}

Win32FirstPartyEditorRunResult Win32FirstPartyEditorHost::run(NativeEditorApp& app) {
    return impl_->run(app);
}

} // namespace mirakana::editor
