// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_host/win32/win32_desktop_game_host.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
#include "mirakana/runtime_host/win32/win32_mavg_payload_io.hpp"

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

#include <atomic>
#include <cstring>
#endif

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

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

#if defined(_WIN32)
class ScopedTempDirectory final {
  public:
    ScopedTempDirectory() {
        path_ = std::filesystem::temp_directory_path() /
                ("mirakanai_mavg_win32_async_io_" + std::to_string(GetCurrentProcessId()) + "_" +
                 std::to_string(GetTickCount64()));
        std::filesystem::create_directories(path_);
    }

    ~ScopedTempDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    ScopedTempDirectory(const ScopedTempDirectory&) = delete;
    ScopedTempDirectory& operator=(const ScopedTempDirectory&) = delete;
    ScopedTempDirectory(ScopedTempDirectory&&) = delete;
    ScopedTempDirectory& operator=(ScopedTempDirectory&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

void write_binary_file(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    MK_REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    output.close();
    MK_REQUIRE(output.good());
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanResult
make_single_win32_payload_request_plan(std::string source_file_path, std::uint64_t destination_offset,
                                       std::uint32_t destination_size) {
    mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanResult result;
    result.requests.push_back(mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestRow{
        .request_index = 0,
        .page_index = 7,
        .source_file_offset = 2,
        .source_size = 4,
        .source_file_path = std::move(source_file_path),
        .destination_offset = destination_offset,
        .destination_size = destination_size,
        .fence_wait_point = mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_destination_write,
        .source_is_file = true,
        .destination_is_memory = true,
        .synchronized_with_fence = false,
        .debug_name = "mavg.payload.page.7",
    });
    result.requested_page_count = 1;
    result.planned_request_count = 1;
    result.total_source_bytes = 4;
    result.total_destination_bytes = destination_size;
    result.requires_native_directstorage_sdk = true;
    return result;
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollResult
poll_until_win32_payload_io_complete(mirakana::runtime::IRuntimeMavgPayloadNativeIoDispatcher& dispatcher,
                                     std::uint64_t ticket) {
    mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollResult status;
    for (std::size_t attempt = 0; attempt < 500U; ++attempt) {
        status = mirakana::runtime::poll_runtime_mavg_payload_native_io_status(
            mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollDesc{
                .dispatcher = &dispatcher,
                .ticket = ticket,
            });
        if (status.complete || status.failed) {
            return status;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return status;
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

MK_TEST("win32 mavg payload async file io dispatcher reads planned byte ranges into destination memory") {
    ScopedTempDirectory temp;
    const std::vector<std::uint8_t> source_bytes{0x10U, 0x11U, 0xa0U, 0xa1U, 0xa2U, 0xa3U, 0xeeU};
    write_binary_file(temp.path() / "payload.bin", source_bytes);

    auto request_plan = make_single_win32_payload_request_plan("payload.bin", 3, 4);
    std::vector<std::uint8_t> destination(16U, 0xcdU);
    mirakana::Win32MavgPayloadAsyncFileIoDispatcher dispatcher{
        mirakana::Win32MavgPayloadAsyncFileIoDispatcherDesc{.root_path = temp.path()}};

    const auto dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &request_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file,
            .submission_tag = 77U,
            .destination_memory = destination,
            .require_native_directstorage = false,
        });

    MK_REQUIRE(dispatch.succeeded());
    MK_REQUIRE(dispatch.ticket != 0U);
    MK_REQUIRE(dispatch.backend == mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file);
    MK_REQUIRE(dispatch.request_count == 1U);
    MK_REQUIRE(dispatch.total_source_bytes == 4U);
    MK_REQUIRE(dispatch.total_destination_bytes == 4U);
    MK_REQUIRE(dispatch.submitted_io_queue);
    MK_REQUIRE(dispatch.enqueued_native_requests);
    MK_REQUIRE(!dispatch.submitted_native_queue);
    MK_REQUIRE(dispatch.used_win32_async_io);
    MK_REQUIRE(!dispatch.used_native_directstorage);
    MK_REQUIRE(!dispatch.executed_background_worker);
    MK_REQUIRE(!dispatch.touched_renderer_or_rhi_handles);

    const auto status = poll_until_win32_payload_io_complete(dispatcher, dispatch.ticket);

    MK_REQUIRE(status.succeeded());
    MK_REQUIRE(status.status == mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::complete);
    MK_REQUIRE(status.complete);
    MK_REQUIRE(!status.failed);
    MK_REQUIRE(status.used_win32_async_io);
    MK_REQUIRE(!status.used_native_directstorage);
    MK_REQUIRE(!status.executed_background_worker);
    MK_REQUIRE(!status.touched_renderer_or_rhi_handles);
    MK_REQUIRE(destination[0] == 0xcdU);
    MK_REQUIRE(destination[2] == 0xcdU);
    MK_REQUIRE(destination[3] == 0xa0U);
    MK_REQUIRE(destination[4] == 0xa1U);
    MK_REQUIRE(destination[5] == 0xa2U);
    MK_REQUIRE(destination[6] == 0xa3U);
    MK_REQUIRE(destination[7] == 0xcdU);
}

MK_TEST("win32 mavg payload async file io dispatcher rejects destination overflow before ticket") {
    ScopedTempDirectory temp;
    const std::vector<std::uint8_t> source_bytes{0x10U, 0x11U, 0xa0U, 0xa1U, 0xa2U, 0xa3U};
    write_binary_file(temp.path() / "payload.bin", source_bytes);

    auto request_plan = make_single_win32_payload_request_plan("payload.bin", 14, 4);
    std::vector<std::uint8_t> destination(16U, 0xcdU);
    mirakana::Win32MavgPayloadAsyncFileIoDispatcher dispatcher{
        mirakana::Win32MavgPayloadAsyncFileIoDispatcherDesc{.root_path = temp.path()}};

    const auto dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &request_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file,
            .destination_memory = destination,
            .require_native_directstorage = false,
        });

    MK_REQUIRE(!dispatch.succeeded());
    MK_REQUIRE(dispatch.ticket == 0U);
    MK_REQUIRE(!dispatch.diagnostics.empty());
    MK_REQUIRE(!dispatch.submitted_io_queue);
    MK_REQUIRE(!dispatch.used_native_directstorage);
    MK_REQUIRE(!dispatch.executed_background_worker);
    MK_REQUIRE(!dispatch.touched_renderer_or_rhi_handles);
    for (const auto byte : destination) {
        MK_REQUIRE(byte == 0xcdU);
    }
}

MK_TEST("win32 mavg payload iocp file io worker reads multiple planned byte ranges into destination memory") {
    ScopedTempDirectory temp;
    const std::vector<std::uint8_t> source_bytes{0x10U, 0x11U, 0xa0U, 0xa1U, 0xa2U, 0xa3U,
                                                 0xeeU, 0xefU, 0xb0U, 0xb1U, 0xb2U, 0xb3U};
    write_binary_file(temp.path() / "payload.bin", source_bytes);

    mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanResult request_plan;
    request_plan.requests.push_back(mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestRow{
        .request_index = 0,
        .page_index = 7,
        .source_file_offset = 2,
        .source_size = 4,
        .source_file_path = "payload.bin",
        .destination_offset = 3,
        .destination_size = 4,
        .fence_wait_point = mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_destination_write,
        .source_is_file = true,
        .destination_is_memory = true,
        .synchronized_with_fence = false,
        .debug_name = "mavg.payload.page.7",
    });
    request_plan.requests.push_back(mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestRow{
        .request_index = 1,
        .page_index = 8,
        .source_file_offset = 8,
        .source_size = 4,
        .source_file_path = "payload.bin",
        .destination_offset = 10,
        .destination_size = 4,
        .fence_wait_point = mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_destination_write,
        .source_is_file = true,
        .destination_is_memory = true,
        .synchronized_with_fence = false,
        .debug_name = "mavg.payload.page.8",
    });
    request_plan.requested_page_count = 2;
    request_plan.planned_request_count = 2;
    request_plan.total_source_bytes = 8;
    request_plan.total_destination_bytes = 8;
    request_plan.requires_native_directstorage_sdk = true;

    std::vector<std::uint8_t> destination(20U, 0xcdU);
    mirakana::Win32MavgPayloadIocpFileIoDispatcher dispatcher{
        mirakana::Win32MavgPayloadIocpFileIoDispatcherDesc{.root_path = temp.path()}};

    const auto dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &request_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file,
            .submission_tag = 88U,
            .destination_memory = destination,
            .require_native_directstorage = false,
        });

    MK_REQUIRE(dispatch.succeeded());
    MK_REQUIRE(dispatch.ticket != 0U);
    MK_REQUIRE(dispatch.backend == mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file);
    MK_REQUIRE(dispatch.request_count == 2U);
    MK_REQUIRE(dispatch.total_source_bytes == 8U);
    MK_REQUIRE(dispatch.total_destination_bytes == 8U);
    MK_REQUIRE(dispatch.submitted_io_queue);
    MK_REQUIRE(dispatch.enqueued_native_requests);
    MK_REQUIRE(!dispatch.submitted_native_queue);
    MK_REQUIRE(dispatch.used_win32_async_io);
    MK_REQUIRE(!dispatch.used_native_directstorage);
    MK_REQUIRE(dispatch.executed_background_worker);
    MK_REQUIRE(!dispatch.touched_renderer_or_rhi_handles);

    const auto status = poll_until_win32_payload_io_complete(dispatcher, dispatch.ticket);

    MK_REQUIRE(status.succeeded());
    MK_REQUIRE(status.status == mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::complete);
    MK_REQUIRE(status.complete);
    MK_REQUIRE(!status.failed);
    MK_REQUIRE(status.used_win32_async_io);
    MK_REQUIRE(!status.used_native_directstorage);
    MK_REQUIRE(status.executed_background_worker);
    MK_REQUIRE(!status.touched_renderer_or_rhi_handles);
    MK_REQUIRE(destination[0] == 0xcdU);
    MK_REQUIRE(destination[2] == 0xcdU);
    MK_REQUIRE(destination[3] == 0xa0U);
    MK_REQUIRE(destination[4] == 0xa1U);
    MK_REQUIRE(destination[5] == 0xa2U);
    MK_REQUIRE(destination[6] == 0xa3U);
    MK_REQUIRE(destination[7] == 0xcdU);
    MK_REQUIRE(destination[9] == 0xcdU);
    MK_REQUIRE(destination[10] == 0xb0U);
    MK_REQUIRE(destination[11] == 0xb1U);
    MK_REQUIRE(destination[12] == 0xb2U);
    MK_REQUIRE(destination[13] == 0xb3U);
    MK_REQUIRE(destination[14] == 0xcdU);
}

MK_TEST("win32 mavg payload iocp file io worker rejects destination overflow before ticket") {
    ScopedTempDirectory temp;
    const std::vector<std::uint8_t> source_bytes{0x10U, 0x11U, 0xa0U, 0xa1U, 0xa2U, 0xa3U};
    write_binary_file(temp.path() / "payload.bin", source_bytes);

    auto request_plan = make_single_win32_payload_request_plan("payload.bin", 14, 4);
    std::vector<std::uint8_t> destination(16U, 0xcdU);
    mirakana::Win32MavgPayloadIocpFileIoDispatcher dispatcher{
        mirakana::Win32MavgPayloadIocpFileIoDispatcherDesc{.root_path = temp.path()}};

    const auto dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &request_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file,
            .destination_memory = destination,
            .require_native_directstorage = false,
        });

    MK_REQUIRE(!dispatch.succeeded());
    MK_REQUIRE(dispatch.ticket == 0U);
    MK_REQUIRE(!dispatch.diagnostics.empty());
    MK_REQUIRE(!dispatch.submitted_io_queue);
    MK_REQUIRE(!dispatch.used_native_directstorage);
    MK_REQUIRE(!dispatch.executed_background_worker);
    MK_REQUIRE(!dispatch.touched_renderer_or_rhi_handles);
    for (const auto byte : destination) {
        MK_REQUIRE(byte == 0xcdU);
    }
}

MK_TEST("win32 mavg payload iocp file io worker releases inflight slot after read failure") {
    ScopedTempDirectory temp;
    const std::vector<std::uint8_t> source_bytes{0x10U, 0x11U, 0xa0U, 0xa1U, 0xa2U, 0xa3U, 0xeeU};
    write_binary_file(temp.path() / "payload.bin", source_bytes);

    auto failing_plan = make_single_win32_payload_request_plan("payload.bin", 3, 4);
    failing_plan.requests.front().source_file_offset = 4096U;
    std::vector<std::uint8_t> failing_destination(16U, 0xcdU);
    mirakana::Win32MavgPayloadIocpFileIoDispatcher dispatcher{mirakana::Win32MavgPayloadIocpFileIoDispatcherDesc{
        .root_path = temp.path(),
        .max_inflight_submissions = 1U,
    }};

    const auto failing_dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &failing_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file,
            .destination_memory = failing_destination,
            .require_native_directstorage = false,
        });

    if (failing_dispatch.succeeded()) {
        MK_REQUIRE(failing_dispatch.ticket != 0U);
        const auto failing_status = poll_until_win32_payload_io_complete(dispatcher, failing_dispatch.ticket);
        MK_REQUIRE(failing_status.failed);
    } else {
        MK_REQUIRE(failing_dispatch.ticket == 0U);
        MK_REQUIRE(!failing_dispatch.diagnostics.empty());
    }

    auto successful_plan = make_single_win32_payload_request_plan("payload.bin", 3, 4);
    std::vector<std::uint8_t> successful_destination(16U, 0xcdU);
    const auto successful_dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &successful_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file,
            .destination_memory = successful_destination,
            .require_native_directstorage = false,
        });

    MK_REQUIRE(successful_dispatch.succeeded());
    MK_REQUIRE(successful_dispatch.ticket != 0U);

    const auto status = poll_until_win32_payload_io_complete(dispatcher, successful_dispatch.ticket);
    MK_REQUIRE(status.succeeded());
    MK_REQUIRE(status.complete);
    MK_REQUIRE(successful_destination[3] == 0xa0U);
    MK_REQUIRE(successful_destination[6] == 0xa3U);
}

MK_TEST("win32 mavg payload iocp file io worker keeps concurrent dispatch under inflight limit") {
    ScopedTempDirectory temp;
    const std::vector<std::uint8_t> source_bytes{0x10U, 0x11U, 0xa0U, 0xa1U, 0xa2U, 0xa3U, 0xeeU};
    write_binary_file(temp.path() / "payload.bin", source_bytes);

    mirakana::Win32MavgPayloadIocpFileIoDispatcher dispatcher{mirakana::Win32MavgPayloadIocpFileIoDispatcherDesc{
        .root_path = temp.path(),
        .max_inflight_submissions = 1U,
        .worker_thread_count = 1U,
    }};

    constexpr std::size_t thread_count = 8U;
    std::atomic_bool start{false};
    std::atomic<std::size_t> successful_dispatch_count{0U};
    std::vector<std::uint64_t> tickets(thread_count, 0U);
    std::vector<std::vector<std::uint8_t>> destinations(thread_count, std::vector<std::uint8_t>(16U, 0xcdU));
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (std::size_t index = 0; index < thread_count; ++index) {
        threads.emplace_back([&dispatcher, &start, &successful_dispatch_count, &tickets, &destinations, index]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            auto request_plan = make_single_win32_payload_request_plan("payload.bin", 3, 4);
            const auto dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
                mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
                    .dispatcher = &dispatcher,
                    .request_plan = &request_plan,
                    .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file,
                    .destination_memory = destinations[index],
                    .require_native_directstorage = false,
                });
            if (dispatch.succeeded()) {
                tickets[index] = dispatch.ticket;
                successful_dispatch_count.fetch_add(1U, std::memory_order_relaxed);
            }
        });
    }

    start.store(true, std::memory_order_release);
    for (auto& thread : threads) {
        thread.join();
    }

    MK_REQUIRE(successful_dispatch_count.load(std::memory_order_relaxed) <= 1U);
    for (const auto ticket : tickets) {
        if (ticket != 0U) {
            const auto status = poll_until_win32_payload_io_complete(dispatcher, ticket);
            MK_REQUIRE(status.succeeded());
            MK_REQUIRE(status.complete);
        }
    }
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
