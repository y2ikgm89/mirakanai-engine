// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "win32_imgui_d3d12_host.hpp"

#include "native_editor_app.hpp"
#include "native_editor_panels.hpp"
#include "native_editor_win32_services.hpp"
#include "win32_imgui_descriptor_allocator.hpp"
#include "win32_imgui_message_bridge.hpp"

#include "mirakana/platform/win32/win32_event_pump.hpp"
#include "mirakana/platform/win32/win32_runtime.hpp"
#include "mirakana/platform/win32/win32_window.hpp"
#include "mirakana/platform/window.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

using Microsoft::WRL::ComPtr;

inline constexpr UINT frame_count = 2;
inline constexpr UINT imgui_srv_descriptor_count = 32;
inline constexpr DXGI_FORMAT backbuffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;

[[nodiscard]] HWND window_from_token(std::uintptr_t token) noexcept {
    return reinterpret_cast<HWND>(token);
}

[[nodiscard]] std::string last_error_message(std::string_view prefix, HRESULT result) {
    return std::string(prefix) + " failed with HRESULT 0x" + std::to_string(static_cast<unsigned long>(result));
}

[[nodiscard]] bool adapter_supports_d3d12(IUnknown* adapter) noexcept {
    return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr));
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

[[nodiscard]] IUnknown* select_adapter(IDXGIFactory6* factory, ComPtr<IDXGIAdapter1>& hardware_adapter,
                                       ComPtr<IDXGIAdapter>& warp_adapter,
                                       Win32ImguiD3d12AdapterKind& adapter_kind) noexcept {
    adapter_kind = Win32ImguiD3d12AdapterKind::none;
    if (find_hardware_adapter(factory, hardware_adapter)) {
        adapter_kind = Win32ImguiD3d12AdapterKind::hardware;
        return hardware_adapter.Get();
    }
    if (find_warp_adapter(factory, warp_adapter)) {
        adapter_kind = Win32ImguiD3d12AdapterKind::warp;
        return warp_adapter.Get();
    }
    return nullptr;
}

[[nodiscard]] bool enable_debug_layer() noexcept {
    ComPtr<ID3D12Debug> debug;
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        return false;
    }
    debug->EnableDebugLayer();
    return true;
}

[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE offset_cpu_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE base, UINT descriptor_size,
                                                                UINT index) noexcept {
    return D3D12_CPU_DESCRIPTOR_HANDLE{.ptr = base.ptr + static_cast<std::size_t>(descriptor_size) * index};
}

void throw_if_failed(HRESULT result, std::string_view operation) {
    if (FAILED(result)) {
        throw std::runtime_error(last_error_message(operation, result));
    }
}

} // namespace

struct Win32ImguiD3d12Host::Impl {
    struct FrameContext {
        ComPtr<ID3D12CommandAllocator> allocator;
        UINT64 fence_value{0};
    };

    explicit Impl(Win32ImguiD3d12HostDesc host_desc) : desc(std::move(host_desc)) {}

    Win32ImguiD3d12HostDesc desc;
    Win32ImguiD3d12HostRunResult result;
    std::unique_ptr<mirakana::win32::Win32Runtime> runtime;
    std::unique_ptr<mirakana::win32::Win32Window> window;
    std::unique_ptr<NativeEditorWin32Services> services;
    std::unique_ptr<Win32ImguiMessageBridge> message_bridge;
    mirakana::win32::Win32EventPump event_pump;
    HWND hwnd{nullptr};

    ComPtr<IDXGIFactory6> factory;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> command_queue;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12DescriptorHeap> rtv_heap;
    ComPtr<ID3D12DescriptorHeap> srv_heap;
    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12Fence> fence;
    HANDLE fence_event{nullptr};
    UINT64 next_fence_value{1};
    UINT frame_index{0};
    UINT rtv_descriptor_size{0};
    UINT srv_descriptor_size{0};
    bool smoke_resize_completed{false};
    std::array<FrameContext, frame_count> frames{};
    std::array<ComPtr<ID3D12Resource>, frame_count> render_targets{};
    std::unique_ptr<Win32ImguiDescriptorAllocator> srv_allocator;
    bool imgui_context_created{false};
    bool imgui_win32_initialized{false};
    bool imgui_dx12_initialized{false};
    bool initialized{false};

    void initialize(NativeEditorApp& app) {
        if (initialized) {
            return;
        }
        result = Win32ImguiD3d12HostRunResult{};
        create_window(app);
        create_device_and_queue();
        create_swapchain();
        create_render_targets();
        create_imgui_context();
        initialized = true;
    }

    void create_window(NativeEditorApp& app) {
        runtime = std::make_unique<mirakana::win32::Win32Runtime>(mirakana::win32::Win32RuntimeDesc{
            .window_class_name = "MIRAIKANAI Native Win32 Editor Shell",
            .dpi_aware = true,
        });
        window = std::make_unique<mirakana::win32::Win32Window>(
            *runtime, mirakana::WindowDesc{
                          .title = "MIRAIKANAI Editor",
                          .extent = mirakana::WindowExtent{.width = desc.launch.width, .height = desc.launch.height},
                      });
        hwnd = window_from_token(window->native_window_token());
        if (hwnd == nullptr) {
            throw std::runtime_error("native editor window creation returned a null HWND");
        }
        services = std::make_unique<NativeEditorWin32Services>(window->native_window_token());
        services->bind(app);
        if (desc.launch.smoke_frames < 0) {
            ShowWindow(hwnd, SW_SHOWDEFAULT);
            UpdateWindow(hwnd);
        }
    }

    void create_device_and_queue() {
        UINT factory_flags = 0;
        if (desc.enable_debug_layer && enable_debug_layer()) {
            factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
        throw_if_failed(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory)), "CreateDXGIFactory2");

        ComPtr<IDXGIAdapter1> hardware_adapter;
        ComPtr<IDXGIAdapter> warp_adapter;
        IUnknown* const selected_adapter =
            select_adapter(factory.Get(), hardware_adapter, warp_adapter, result.adapter_kind);
        if (selected_adapter == nullptr) {
            throw std::runtime_error("no D3D12-capable hardware or WARP adapter is available");
        }
        throw_if_failed(D3D12CreateDevice(selected_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)),
                        "D3D12CreateDevice");

        D3D12_COMMAND_QUEUE_DESC queue_desc{};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        throw_if_failed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)),
                        "ID3D12Device::CreateCommandQueue");

        for (auto& frame : frames) {
            throw_if_failed(
                device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.allocator)),
                "ID3D12Device::CreateCommandAllocator");
        }
        throw_if_failed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames[0].allocator.Get(), nullptr,
                                                  IID_PPV_ARGS(&command_list)),
                        "ID3D12Device::CreateCommandList");
        throw_if_failed(command_list->Close(), "ID3D12GraphicsCommandList::Close");

        throw_if_failed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
                        "ID3D12Device::CreateFence");
        fence_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (fence_event == nullptr) {
            throw std::runtime_error("CreateEventW failed for native editor D3D12 fence");
        }

        D3D12_DESCRIPTOR_HEAP_DESC srv_desc{};
        srv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srv_desc.NumDescriptors = imgui_srv_descriptor_count;
        srv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        throw_if_failed(device->CreateDescriptorHeap(&srv_desc, IID_PPV_ARGS(&srv_heap)),
                        "ID3D12Device::CreateDescriptorHeap(SRV)");
        srv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        srv_allocator = std::make_unique<Win32ImguiDescriptorAllocator>(Win32ImguiDescriptorAllocatorDesc{
            .cpu_descriptor_base = srv_heap->GetCPUDescriptorHandleForHeapStart().ptr,
            .gpu_descriptor_base = srv_heap->GetGPUDescriptorHandleForHeapStart().ptr,
            .descriptor_size = srv_descriptor_size,
            .capacity = imgui_srv_descriptor_count,
        });
    }

    void create_swapchain() {
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc{};
        swapchain_desc.Width = desc.launch.width;
        swapchain_desc.Height = desc.launch.height;
        swapchain_desc.Format = backbuffer_format;
        swapchain_desc.Stereo = FALSE;
        swapchain_desc.SampleDesc.Count = 1;
        swapchain_desc.SampleDesc.Quality = 0;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = frame_count;
        swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapchain_desc.Flags = 0;

        ComPtr<IDXGISwapChain1> swapchain1;
        throw_if_failed(
            factory->CreateSwapChainForHwnd(command_queue.Get(), hwnd, &swapchain_desc, nullptr, nullptr, &swapchain1),
            "IDXGIFactory::CreateSwapChainForHwnd");
        throw_if_failed(swapchain1.As(&swapchain), "IDXGISwapChain1::QueryInterface(IDXGISwapChain3)");
        (void)factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
        frame_index = swapchain->GetCurrentBackBufferIndex();
    }

    void create_render_targets() {
        D3D12_DESCRIPTOR_HEAP_DESC rtv_desc{};
        rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_desc.NumDescriptors = frame_count;
        rtv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        throw_if_failed(device->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(&rtv_heap)),
                        "ID3D12Device::CreateDescriptorHeap(RTV)");
        rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        refresh_render_targets();
    }

    void refresh_render_targets() {
        const auto rtv_start = rtv_heap->GetCPUDescriptorHandleForHeapStart();
        for (UINT index = 0; index < frame_count; ++index) {
            throw_if_failed(swapchain->GetBuffer(index, IID_PPV_ARGS(&render_targets[index])),
                            "IDXGISwapChain::GetBuffer");
            device->CreateRenderTargetView(render_targets[index].Get(), nullptr,
                                           offset_cpu_descriptor(rtv_start, rtv_descriptor_size, index));
        }
    }

    static void allocate_srv_descriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu,
                                        D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu) {
        auto* const self = static_cast<Impl*>(info->UserData);
        if (self == nullptr || self->srv_allocator == nullptr) {
            *out_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{};
            *out_gpu = D3D12_GPU_DESCRIPTOR_HANDLE{};
            return;
        }

        const auto lease = self->srv_allocator->allocate();
        if (!lease.valid) {
            self->result.diagnostic = lease.diagnostic;
            *out_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{};
            *out_gpu = D3D12_GPU_DESCRIPTOR_HANDLE{};
            return;
        }
        *out_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{.ptr = lease.cpu_descriptor};
        *out_gpu = D3D12_GPU_DESCRIPTOR_HANDLE{.ptr = lease.gpu_descriptor};
    }

    static void free_srv_descriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu,
                                    D3D12_GPU_DESCRIPTOR_HANDLE /*gpu*/) {
        auto* const self = static_cast<Impl*>(info->UserData);
        if (self != nullptr && self->srv_allocator != nullptr) {
            self->srv_allocator->release_cpu_descriptor(cpu.ptr);
        }
    }

    void create_imgui_context() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        imgui_context_created = true;
        ImGuiIO& io = ImGui::GetIO();
        const auto user_config_policy = make_native_editor_imgui_user_config_policy(desc.launch);
        if (!user_config_policy.ini_file_enabled) {
            io.IniFilename = nullptr;
        }
        if (!user_config_policy.log_file_enabled) {
            io.LogFilename = nullptr;
        }
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui::StyleColorsDark();

        if (!ImGui_ImplWin32_Init(hwnd)) {
            throw std::runtime_error("ImGui_ImplWin32_Init failed");
        }
        imgui_win32_initialized = true;
        message_bridge = std::make_unique<Win32ImguiMessageBridge>(window->native_window_token());

        ImGui_ImplDX12_InitInfo init_info{};
        init_info.Device = device.Get();
        init_info.CommandQueue = command_queue.Get();
        init_info.NumFramesInFlight = frame_count;
        init_info.RTVFormat = backbuffer_format;
        init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
        init_info.UserData = this;
        init_info.SrvDescriptorHeap = srv_heap.Get();
        init_info.SrvDescriptorAllocFn = allocate_srv_descriptor;
        init_info.SrvDescriptorFreeFn = free_srv_descriptor;
        if (!ImGui_ImplDX12_Init(&init_info)) {
            throw std::runtime_error("ImGui_ImplDX12_Init failed");
        }
        imgui_dx12_initialized = true;
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
        if (message_bridge != nullptr) {
            (void)message_bridge->drain_messages();
        }
    }

    void resize(std::uint32_t width, std::uint32_t height) {
        wait_for_gpu();
        for (auto& target : render_targets) {
            target.Reset();
        }
        throw_if_failed(swapchain->ResizeBuffers(frame_count, width, height, backbuffer_format, 0),
                        "IDXGISwapChain::ResizeBuffers");
        desc.launch.width = width;
        desc.launch.height = height;
        refresh_render_targets();
        frame_index = swapchain->GetCurrentBackBufferIndex();
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

        auto& frame = frames[frame_index];
        wait_for_frame(frame);
        throw_if_failed(frame.allocator->Reset(), "ID3D12CommandAllocator::Reset");
        throw_if_failed(command_list->Reset(frame.allocator.Get(), nullptr), "ID3D12GraphicsCommandList::Reset");

        D3D12_RESOURCE_BARRIER barrier_to_render{};
        barrier_to_render.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier_to_render.Transition.pResource = render_targets[frame_index].Get();
        barrier_to_render.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier_to_render.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier_to_render.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        command_list->ResourceBarrier(1, &barrier_to_render);

        const auto rtv =
            offset_cpu_descriptor(rtv_heap->GetCPUDescriptorHandleForHeapStart(), rtv_descriptor_size, frame_index);
        const std::array<float, 4> clear_color{0.06F, 0.07F, 0.08F, 1.0F};
        command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        command_list->ClearRenderTargetView(rtv, clear_color.data(), 0, nullptr);

        ID3D12DescriptorHeap* heaps[] = {srv_heap.Get()};
        command_list->SetDescriptorHeaps(1, heaps);

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app.record_native_frame();
        app.record_native_resource_device_ready(result.frames_rendered + 1U);
        app.record_native_viewport_d3d12_host_ready(result.frames_rendered + 1U);
        app.record_native_material_preview_d3d12_host_ready(result.frames_rendered + 1U);
        (void)render_native_editor_panels(app);
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list.Get());

        D3D12_RESOURCE_BARRIER barrier_to_present{};
        barrier_to_present.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier_to_present.Transition.pResource = render_targets[frame_index].Get();
        barrier_to_present.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier_to_present.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier_to_present.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        command_list->ResourceBarrier(1, &barrier_to_present);

        throw_if_failed(command_list->Close(), "ID3D12GraphicsCommandList::Close");
        ID3D12CommandList* command_lists[] = {command_list.Get()};
        command_queue->ExecuteCommandLists(1, command_lists);
        throw_if_failed(swapchain->Present(desc.vsync ? 1U : 0U, 0), "IDXGISwapChain::Present");

        frame.fence_value = next_fence_value;
        throw_if_failed(command_queue->Signal(fence.Get(), next_fence_value), "ID3D12CommandQueue::Signal");
        ++next_fence_value;
        frame_index = swapchain->GetCurrentBackBufferIndex();
        ++result.frames_rendered;
        return true;
    }

    void wait_for_frame(FrameContext& frame) {
        if (frame.fence_value == 0 || fence->GetCompletedValue() >= frame.fence_value) {
            return;
        }
        throw_if_failed(fence->SetEventOnCompletion(frame.fence_value, fence_event),
                        "ID3D12Fence::SetEventOnCompletion");
        WaitForSingleObject(fence_event, INFINITE);
    }

    void wait_for_gpu() {
        const UINT64 fence_value = next_fence_value;
        throw_if_failed(command_queue->Signal(fence.Get(), fence_value), "ID3D12CommandQueue::Signal");
        ++next_fence_value;
        if (fence->GetCompletedValue() < fence_value) {
            throw_if_failed(fence->SetEventOnCompletion(fence_value, fence_event), "ID3D12Fence::SetEventOnCompletion");
            WaitForSingleObject(fence_event, INFINITE);
        }
        for (auto& frame : frames) {
            frame.fence_value = 0;
        }
    }

    Win32ImguiD3d12HostRunResult run(NativeEditorApp& app) {
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
                    result.diagnostic = "native editor smoke loop exited before the requested frame count";
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
        if (device != nullptr && command_queue != nullptr && fence != nullptr && fence_event != nullptr) {
            try {
                wait_for_gpu();
            } catch (const std::exception& error) {
                if (result.diagnostic.empty()) {
                    result.diagnostic = error.what();
                }
            } catch (...) {
                if (result.diagnostic.empty()) {
                    result.diagnostic = "native editor host shutdown failed";
                }
            }
        }

        if (imgui_dx12_initialized) {
            ImGui_ImplDX12_Shutdown();
            imgui_dx12_initialized = false;
        }
        message_bridge.reset();
        if (imgui_win32_initialized) {
            ImGui_ImplWin32_Shutdown();
            imgui_win32_initialized = false;
        }
        if (imgui_context_created) {
            ImGui::DestroyContext();
            imgui_context_created = false;
        }

        for (auto& target : render_targets) {
            target.Reset();
        }
        srv_allocator.reset();
        command_list.Reset();
        for (auto& frame : frames) {
            frame.allocator.Reset();
        }
        srv_heap.Reset();
        rtv_heap.Reset();
        swapchain.Reset();
        command_queue.Reset();
        fence.Reset();
        device.Reset();
        factory.Reset();
        if (fence_event != nullptr) {
            CloseHandle(fence_event);
            fence_event = nullptr;
        }
        services.reset();
        window.reset();
        runtime.reset();
        initialized = false;
    }
};

Win32ImguiD3d12Host::Win32ImguiD3d12Host(Win32ImguiD3d12HostDesc desc)
    : impl_(std::make_unique<Impl>(std::move(desc))) {}

Win32ImguiD3d12Host::~Win32ImguiD3d12Host() {
    if (impl_ != nullptr) {
        impl_->shutdown();
    }
}

Win32ImguiD3d12HostRunResult Win32ImguiD3d12Host::run(NativeEditorApp& app) {
    return impl_->run(app);
}

} // namespace mirakana::editor
