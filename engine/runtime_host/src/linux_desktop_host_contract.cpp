// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/linux/linux_desktop_game_host.hpp"

#if defined(__linux__)
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/rhi/vulkan/vulkan_backend.hpp"
#endif

#include <algorithm>
#if defined(__linux__)
#include <cstdlib>
#include <dlfcn.h>
#endif
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool is_valid_xcb_extent(WindowExtent extent) noexcept {
    return extent.width > 0 && extent.height > 0 &&
           extent.width <= static_cast<std::uint32_t>(std::numeric_limits<std::uint16_t>::max()) &&
           extent.height <= static_cast<std::uint32_t>(std::numeric_limits<std::uint16_t>::max());
}

[[nodiscard]] LinuxDesktopHostReadinessReport invalid_linux_desktop_request(std::string diagnostic) {
    LinuxDesktopHostReadinessReport report;
    report.status = LinuxDesktopHostStatus::invalid_request;
    report.native_handle_access = false;
    report.diagnostic = std::move(diagnostic);
    return report;
}

#if defined(__linux__)
struct xcb_connection_t;
struct xcb_setup_t;

using xcb_colormap_t = std::uint32_t;
using xcb_visualid_t = std::uint32_t;
using xcb_window_t = std::uint32_t;

struct xcb_screen_t {
    xcb_window_t root;
    xcb_colormap_t default_colormap;
    std::uint32_t white_pixel;
    std::uint32_t black_pixel;
    std::uint32_t current_input_masks;
    std::uint16_t width_in_pixels;
    std::uint16_t height_in_pixels;
    std::uint16_t width_in_millimeters;
    std::uint16_t height_in_millimeters;
    std::uint16_t min_installed_maps;
    std::uint16_t max_installed_maps;
    xcb_visualid_t root_visual;
    std::uint8_t backing_stores;
    std::uint8_t save_unders;
    std::uint8_t root_depth;
    std::uint8_t allowed_depths_len;
};

struct xcb_screen_iterator_t {
    xcb_screen_t* data;
    int rem;
    int index;
};

struct xcb_void_cookie_t {
    unsigned int sequence;
};

using xcb_connect_fn = xcb_connection_t* (*)(const char*, int*);
using xcb_connection_has_error_fn = int (*)(xcb_connection_t*);
using xcb_disconnect_fn = void (*)(xcb_connection_t*);
using xcb_get_setup_fn = const xcb_setup_t* (*)(xcb_connection_t*);
using xcb_setup_roots_iterator_fn = xcb_screen_iterator_t (*)(const xcb_setup_t*);
using xcb_generate_id_fn = std::uint32_t (*)(xcb_connection_t*);
using xcb_create_window_fn = xcb_void_cookie_t (*)(xcb_connection_t*, std::uint8_t, xcb_window_t, xcb_window_t,
                                                   std::int16_t, std::int16_t, std::uint16_t, std::uint16_t,
                                                   std::uint16_t, std::uint16_t, xcb_visualid_t, std::uint32_t,
                                                   const void*);
using xcb_map_window_fn = xcb_void_cookie_t (*)(xcb_connection_t*, xcb_window_t);
using xcb_destroy_window_fn = xcb_void_cookie_t (*)(xcb_connection_t*, xcb_window_t);
using xcb_flush_fn = int (*)(xcb_connection_t*);

constexpr std::uint8_t xcb_copy_from_parent = 0;
constexpr std::uint16_t xcb_window_class_input_output = 1;
constexpr std::uint32_t xcb_cw_back_pixel = 1U << 1U;
constexpr std::uint32_t xcb_cw_event_mask = 1U << 11U;
constexpr std::uint32_t xcb_event_mask_exposure = 1U << 15U;
constexpr std::uint32_t xcb_event_mask_structure_notify = 1U << 17U;

class LinuxVulkanPresentationRuntimeError final : public std::runtime_error {
  public:
    LinuxVulkanPresentationRuntimeError(LinuxDesktopVulkanPresentationStatus status, std::string message)
        : std::runtime_error(message), status_(status) {}

    [[nodiscard]] LinuxDesktopVulkanPresentationStatus status() const noexcept {
        return status_;
    }

  private:
    LinuxDesktopVulkanPresentationStatus status_;
};

struct LinuxPresentationXcbApi {
    void* library{nullptr};
    xcb_connect_fn connect{nullptr};
    xcb_connection_has_error_fn connection_has_error{nullptr};
    xcb_disconnect_fn disconnect{nullptr};
    xcb_get_setup_fn get_setup{nullptr};
    xcb_setup_roots_iterator_fn setup_roots_iterator{nullptr};
    xcb_generate_id_fn generate_id{nullptr};
    xcb_create_window_fn create_window{nullptr};
    xcb_map_window_fn map_window{nullptr};
    xcb_destroy_window_fn destroy_window{nullptr};
    xcb_flush_fn flush{nullptr};

    LinuxPresentationXcbApi() = default;
    LinuxPresentationXcbApi(const LinuxPresentationXcbApi&) = delete;
    LinuxPresentationXcbApi& operator=(const LinuxPresentationXcbApi&) = delete;

    LinuxPresentationXcbApi(LinuxPresentationXcbApi&& other) noexcept
        : library(std::exchange(other.library, nullptr)), connect(std::exchange(other.connect, nullptr)),
          connection_has_error(std::exchange(other.connection_has_error, nullptr)),
          disconnect(std::exchange(other.disconnect, nullptr)), get_setup(std::exchange(other.get_setup, nullptr)),
          setup_roots_iterator(std::exchange(other.setup_roots_iterator, nullptr)),
          generate_id(std::exchange(other.generate_id, nullptr)),
          create_window(std::exchange(other.create_window, nullptr)),
          map_window(std::exchange(other.map_window, nullptr)),
          destroy_window(std::exchange(other.destroy_window, nullptr)), flush(std::exchange(other.flush, nullptr)) {}

    LinuxPresentationXcbApi& operator=(LinuxPresentationXcbApi&& other) noexcept {
        if (this != &other) {
            reset();
            library = std::exchange(other.library, nullptr);
            connect = std::exchange(other.connect, nullptr);
            connection_has_error = std::exchange(other.connection_has_error, nullptr);
            disconnect = std::exchange(other.disconnect, nullptr);
            get_setup = std::exchange(other.get_setup, nullptr);
            setup_roots_iterator = std::exchange(other.setup_roots_iterator, nullptr);
            generate_id = std::exchange(other.generate_id, nullptr);
            create_window = std::exchange(other.create_window, nullptr);
            map_window = std::exchange(other.map_window, nullptr);
            destroy_window = std::exchange(other.destroy_window, nullptr);
            flush = std::exchange(other.flush, nullptr);
        }
        return *this;
    }

    ~LinuxPresentationXcbApi() {
        reset();
    }

    void reset() noexcept {
        if (library != nullptr) {
            dlclose(library);
            library = nullptr;
        }
    }

    [[nodiscard]] bool ready() const noexcept {
        return library != nullptr && connect != nullptr && connection_has_error != nullptr && disconnect != nullptr &&
               get_setup != nullptr && setup_roots_iterator != nullptr && generate_id != nullptr &&
               create_window != nullptr && map_window != nullptr && destroy_window != nullptr && flush != nullptr;
    }
};

template <typename Fn> [[nodiscard]] Fn load_xcb_symbol(void* library, const char* name) noexcept {
    return reinterpret_cast<Fn>(dlsym(library, name));
}

[[nodiscard]] LinuxPresentationXcbApi load_presentation_xcb_api() {
    LinuxPresentationXcbApi api;
    api.library = dlopen("libxcb.so.1", RTLD_NOW | RTLD_LOCAL);
    if (api.library == nullptr) {
        throw LinuxVulkanPresentationRuntimeError(LinuxDesktopVulkanPresentationStatus::xcb_surface_unavailable,
                                                  "libxcb.so.1 could not be loaded");
    }

    api.connect = load_xcb_symbol<xcb_connect_fn>(api.library, "xcb_connect");
    api.connection_has_error = load_xcb_symbol<xcb_connection_has_error_fn>(api.library, "xcb_connection_has_error");
    api.disconnect = load_xcb_symbol<xcb_disconnect_fn>(api.library, "xcb_disconnect");
    api.get_setup = load_xcb_symbol<xcb_get_setup_fn>(api.library, "xcb_get_setup");
    api.setup_roots_iterator = load_xcb_symbol<xcb_setup_roots_iterator_fn>(api.library, "xcb_setup_roots_iterator");
    api.generate_id = load_xcb_symbol<xcb_generate_id_fn>(api.library, "xcb_generate_id");
    api.create_window = load_xcb_symbol<xcb_create_window_fn>(api.library, "xcb_create_window");
    api.map_window = load_xcb_symbol<xcb_map_window_fn>(api.library, "xcb_map_window");
    api.destroy_window = load_xcb_symbol<xcb_destroy_window_fn>(api.library, "xcb_destroy_window");
    api.flush = load_xcb_symbol<xcb_flush_fn>(api.library, "xcb_flush");
    if (!api.ready()) {
        throw LinuxVulkanPresentationRuntimeError(LinuxDesktopVulkanPresentationStatus::xcb_surface_unavailable,
                                                  "required XCB window symbols are unavailable");
    }
    return api;
}

[[nodiscard]] const char* display_name_or_null(const char* display_name) noexcept {
    if (display_name == nullptr || display_name[0] == '\0') {
        return nullptr;
    }
    return display_name;
}

class LinuxPresentationXcbWindow final {
  public:
    explicit LinuxPresentationXcbWindow(const LinuxDesktopVulkanPresentationProbeDesc& desc)
        : api_(load_presentation_xcb_api()) {
        int screen_index = 0;
        connection_ = api_.connect(display_name_or_null(desc.display_name), &screen_index);
        if (connection_ == nullptr || api_.connection_has_error(connection_) != 0) {
            throw LinuxVulkanPresentationRuntimeError(LinuxDesktopVulkanPresentationStatus::xcb_surface_unavailable,
                                                      "xcb_connect could not open a display connection");
        }

        const auto* const setup = api_.get_setup(connection_);
        if (setup == nullptr) {
            fail(LinuxDesktopVulkanPresentationStatus::xcb_surface_unavailable,
                 "xcb_get_setup returned no display setup");
        }

        const auto root = api_.setup_roots_iterator(setup);
        if (root.rem <= 0 || root.data == nullptr) {
            fail(LinuxDesktopVulkanPresentationStatus::xcb_surface_unavailable,
                 "xcb_setup_roots_iterator returned no screen");
        }

        screen_ = root.data;
        window_ = api_.generate_id(connection_);
        const std::uint32_t event_mask = xcb_event_mask_exposure | xcb_event_mask_structure_notify;
        const std::uint32_t values[] = {screen_->black_pixel, event_mask};
        (void)api_.create_window(connection_, xcb_copy_from_parent, window_, screen_->root, 0, 0,
                                 static_cast<std::uint16_t>(desc.extent.width),
                                 static_cast<std::uint16_t>(desc.extent.height), 0, xcb_window_class_input_output,
                                 screen_->root_visual, xcb_cw_back_pixel | xcb_cw_event_mask, values);
        (void)api_.map_window(connection_, window_);
        (void)api_.flush(connection_);
        if (api_.connection_has_error(connection_) != 0) {
            fail(LinuxDesktopVulkanPresentationStatus::xcb_surface_unavailable,
                 "xcb_create_window failed on the display connection");
        }
    }

    LinuxPresentationXcbWindow(const LinuxPresentationXcbWindow&) = delete;
    LinuxPresentationXcbWindow& operator=(const LinuxPresentationXcbWindow&) = delete;

    ~LinuxPresentationXcbWindow() {
        cleanup();
    }

    [[nodiscard]] rhi::SurfaceHandle surface_handle() const noexcept {
        return rhi::SurfaceHandle{
            .value = static_cast<std::uintptr_t>(window_),
            .context = reinterpret_cast<std::uintptr_t>(connection_),
            .platform = rhi::SurfacePlatform::xcb,
        };
    }

  private:
    void cleanup() noexcept {
        if (connection_ != nullptr) {
            if (window_ != 0) {
                (void)api_.destroy_window(connection_, window_);
                (void)api_.flush(connection_);
                window_ = 0;
            }
            api_.disconnect(connection_);
            connection_ = nullptr;
        }
    }

    [[noreturn]] void fail(LinuxDesktopVulkanPresentationStatus status, const char* message) {
        cleanup();
        throw LinuxVulkanPresentationRuntimeError(status, message);
    }

    LinuxPresentationXcbApi api_;
    xcb_connection_t* connection_{nullptr};
    xcb_screen_t* screen_{nullptr};
    xcb_window_t window_{0};
};

[[nodiscard]] bool has_nonzero_byte(const std::vector<std::byte>& bytes) noexcept {
    return std::any_of(bytes.begin(), bytes.end(), [](std::byte value) noexcept { return value != std::byte{0}; });
}

[[nodiscard]] LinuxDesktopVulkanPresentationReport
with_runtime_diagnostic(const LinuxDesktopVulkanPresentationRequest& request, std::string diagnostic) {
    auto report = evaluate_linux_desktop_vulkan_presentation_request(request);
    if (!diagnostic.empty()) {
        report.diagnostic = std::move(diagnostic);
    }
    return report;
}

[[nodiscard]] LinuxDesktopVulkanPresentationReport
execute_linux_desktop_vulkan_presentation_probe(const LinuxDesktopVulkanPresentationProbeDesc& desc) {
    LinuxDesktopVulkanPresentationRequest request{.linux_host = true};
    if (!is_valid_xcb_extent(desc.extent)) {
        return with_runtime_diagnostic(request, "Linux Vulkan presentation extent must fit XCB uint16 dimensions");
    }

    try {
        LinuxPresentationXcbWindow window(desc);
        request.xcb_window_ready = true;
        const auto surface = window.surface_handle();

        rhi::vulkan::VulkanLoaderProbeDesc loader_desc;
        loader_desc.host = rhi::RhiHostPlatform::linux;
        rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
        instance_desc.application_name = "GameEngineLinuxVulkanPackageSmoke";
        instance_desc.api_version = rhi::vulkan::make_vulkan_api_version(1, 3);
        instance_desc.required_extensions = {"VK_EXT_debug_utils"};
        instance_desc.enable_validation = true;

        const auto surface_probe = rhi::vulkan::probe_runtime_surface_support(loader_desc, instance_desc, surface);
        request.vulkan_loader_ready =
            surface_probe.snapshots.count_probe.instance.capabilities.global.loader.runtime_loaded;
        request.vulkan_xcb_surface_created = surface_probe.surface_created;
        request.surface_support_probed = surface_probe.probed;
        if (!surface_probe.probed) {
            return with_runtime_diagnostic(request, surface_probe.diagnostic);
        }

        auto device_result = rhi::vulkan::create_runtime_device(loader_desc, instance_desc, {}, surface);
        request.vulkan_loader_ready =
            request.vulkan_loader_ready ||
            device_result.selection_probe.snapshots.count_probe.instance.capabilities.global.loader.runtime_loaded;
        if (!device_result.created || !device_result.device.owns_device()) {
            return with_runtime_diagnostic(request, device_result.diagnostic);
        }

        const rhi::Extent2D extent{.width = desc.extent.width, .height = desc.extent.height};
        rhi::vulkan::VulkanSwapchainCreatePlan swapchain_plan;
        swapchain_plan.supported = true;
        swapchain_plan.extent = extent;
        swapchain_plan.format = rhi::Format::bgra8_unorm;
        swapchain_plan.image_count = 2;
        swapchain_plan.image_view_count = 2;
        swapchain_plan.present_mode = rhi::vulkan::VulkanPresentMode::fifo;
        swapchain_plan.acquire_before_render = true;
        swapchain_plan.diagnostic = "Linux Vulkan swapchain smoke plan ready";

        auto swapchain_result = rhi::vulkan::create_runtime_swapchain(
            device_result.device, rhi::vulkan::VulkanRuntimeSwapchainDesc{.surface = surface, .plan = swapchain_plan});
        request.swapchain_created = swapchain_result.created;
        if (!swapchain_result.created) {
            return with_runtime_diagnostic(request, swapchain_result.diagnostic);
        }

        auto sync_result = rhi::vulkan::create_runtime_frame_sync(device_result.device);
        auto pool_result = rhi::vulkan::create_runtime_command_pool(device_result.device);
        auto readback_result = rhi::vulkan::create_runtime_readback_buffer(
            device_result.device,
            rhi::vulkan::VulkanRuntimeReadbackBufferDesc{static_cast<std::uint64_t>(extent.width) *
                                                         static_cast<std::uint64_t>(extent.height) * 4ULL});
        if (!sync_result.created || !pool_result.created || !readback_result.created) {
            return with_runtime_diagnostic(
                request, !sync_result.diagnostic.empty()
                             ? sync_result.diagnostic
                             : (!pool_result.diagnostic.empty() ? pool_result.diagnostic : readback_result.diagnostic));
        }

        const auto acquire_result = rhi::vulkan::acquire_next_runtime_swapchain_image(
            device_result.device, swapchain_result.swapchain, sync_result.sync);
        request.frame_acquired = acquire_result.acquired;
        if (!acquire_result.acquired) {
            return with_runtime_diagnostic(request, acquire_result.diagnostic);
        }

        const auto dynamic_plan = rhi::vulkan::build_dynamic_rendering_plan(
            rhi::vulkan::VulkanDynamicRenderingDesc{
                .extent = extent,
                .color_attachments = {rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
                    .format = swapchain_plan.format,
                    .load_action = rhi::LoadAction::clear,
                    .store_action = rhi::StoreAction::store,
                }},
                .has_depth_attachment = false,
                .depth_format = rhi::Format::unknown,
                .depth_load_action = rhi::LoadAction::clear,
                .depth_store_action = rhi::StoreAction::store,
            },
            device_result.device.command_plan());
        rhi::vulkan::VulkanFrameSynchronizationDesc sync_desc;
        sync_desc.readback_required = true;
        sync_desc.present_required = true;
        const auto sync_plan =
            rhi::vulkan::build_frame_synchronization_plan(sync_desc, device_result.device.command_plan());
        if (!dynamic_plan.supported || !sync_plan.supported || sync_plan.barriers.size() < 3U ||
            !pool_result.pool.begin_primary_command_buffer()) {
            return with_runtime_diagnostic(request, !dynamic_plan.diagnostic.empty()
                                                        ? dynamic_plan.diagnostic
                                                        : (!sync_plan.diagnostic.empty()
                                                               ? sync_plan.diagnostic
                                                               : "Linux Vulkan command recording could not begin"));
        }

        rhi::vulkan::VulkanRuntimeSwapchainFrameBarrierDesc barrier_desc;
        barrier_desc.image_index = acquire_result.image_index;
        barrier_desc.barrier = sync_plan.barriers[0];
        barrier_desc.barrier.before = rhi::ResourceState::undefined;
        const auto render_barrier = rhi::vulkan::record_runtime_swapchain_frame_barrier(
            device_result.device, pool_result.pool, swapchain_result.swapchain, barrier_desc);

        rhi::vulkan::VulkanRuntimeDynamicRenderingClearDesc clear_desc;
        clear_desc.dynamic_rendering = dynamic_plan;
        clear_desc.image_index = acquire_result.image_index;
        clear_desc.clear_color = rhi::ClearColorValue{.red = 0.15F, .green = 0.55F, .blue = 0.95F, .alpha = 1.0F};
        const auto clear_result = rhi::vulkan::record_runtime_dynamic_rendering_clear(
            device_result.device, pool_result.pool, swapchain_result.swapchain, clear_desc);

        barrier_desc.barrier = sync_plan.barriers[1];
        const auto readback_barrier = rhi::vulkan::record_runtime_swapchain_frame_barrier(
            device_result.device, pool_result.pool, swapchain_result.swapchain, barrier_desc);

        const auto copy_result = rhi::vulkan::record_runtime_swapchain_image_readback(
            device_result.device, pool_result.pool, swapchain_result.swapchain, readback_result.buffer,
            rhi::vulkan::VulkanRuntimeSwapchainReadbackDesc{
                .image_index = acquire_result.image_index, .extent = extent, .bytes_per_pixel = 4});

        barrier_desc.barrier = sync_plan.barriers[2];
        const auto present_barrier = rhi::vulkan::record_runtime_swapchain_frame_barrier(
            device_result.device, pool_result.pool, swapchain_result.swapchain, barrier_desc);

        if (!render_barrier.recorded || !clear_result.recorded || !readback_barrier.recorded || !copy_result.recorded ||
            !present_barrier.recorded || !pool_result.pool.end_primary_command_buffer()) {
            return with_runtime_diagnostic(
                request, !render_barrier.diagnostic.empty()
                             ? render_barrier.diagnostic
                             : (!clear_result.diagnostic.empty()
                                    ? clear_result.diagnostic
                                    : (!readback_barrier.diagnostic.empty()
                                           ? readback_barrier.diagnostic
                                           : (!copy_result.diagnostic.empty() ? copy_result.diagnostic
                                                                              : present_barrier.diagnostic))));
        }

        const auto submit_result = rhi::vulkan::submit_runtime_command_buffer(
            device_result.device, pool_result.pool, sync_result.sync,
            rhi::vulkan::VulkanRuntimeCommandBufferSubmitDesc{.wait_image_available_semaphore = true,
                                                              .signal_render_finished_semaphore = true,
                                                              .signal_in_flight_fence = false,
                                                              .wait_for_graphics_queue_idle = true});
        if (!submit_result.submitted) {
            return with_runtime_diagnostic(request, submit_result.diagnostic);
        }

        const auto read_result =
            rhi::vulkan::read_runtime_readback_buffer(device_result.device, readback_result.buffer,
                                                      rhi::vulkan::VulkanRuntimeReadbackBufferReadDesc{
                                                          .byte_offset = 0, .byte_count = copy_result.required_bytes});
        request.readback_nonzero = read_result.read && has_nonzero_byte(read_result.bytes);

        const auto present_result = rhi::vulkan::present_runtime_swapchain_image(
            device_result.device, swapchain_result.swapchain, sync_result.sync,
            rhi::vulkan::VulkanRuntimeSwapchainPresentDesc{.image_index = acquire_result.image_index,
                                                           .wait_render_finished_semaphore = true});
        request.frame_presented = present_result.presented;
        const auto validation_log = device_result.device.validation_log_snapshot();
        request.validation_log_clean = validation_log.clean();

        auto report = evaluate_linux_desktop_vulkan_presentation_request(request);
        if (report.linux_package_smoke_ready && request.readback_nonzero && !request.validation_log_clean) {
            report.diagnostic = validation_log.diagnostic;
        } else if (!present_result.diagnostic.empty() && !present_result.presented) {
            report.diagnostic = present_result.diagnostic;
        } else if (!read_result.diagnostic.empty() && !request.readback_nonzero) {
            report.diagnostic = read_result.diagnostic;
        }
        return report;
    } catch (const LinuxVulkanPresentationRuntimeError& error) {
        auto report = evaluate_linux_desktop_vulkan_presentation_request(request);
        report.status = error.status();
        report.diagnostic = error.what();
        return report;
    }
}
#endif

} // namespace

std::string_view linux_desktop_host_status_name(LinuxDesktopHostStatus status) noexcept {
    switch (status) {
    case LinuxDesktopHostStatus::ready:
        return "ready";
    case LinuxDesktopHostStatus::invalid_request:
        return "invalid_request";
    case LinuxDesktopHostStatus::host_gated:
        return "host_gated";
    case LinuxDesktopHostStatus::xcb_runtime_unavailable:
        return "xcb_runtime_unavailable";
    case LinuxDesktopHostStatus::xcb_symbols_unavailable:
        return "xcb_symbols_unavailable";
    case LinuxDesktopHostStatus::xcb_display_unavailable:
        return "xcb_display_unavailable";
    case LinuxDesktopHostStatus::xcb_window_unavailable:
        return "xcb_window_unavailable";
    }
    return "unknown";
}

std::string_view linux_desktop_vulkan_presentation_status_name(LinuxDesktopVulkanPresentationStatus status) noexcept {
    switch (status) {
    case LinuxDesktopVulkanPresentationStatus::ready:
        return "ready";
    case LinuxDesktopVulkanPresentationStatus::host_gated:
        return "host_gated";
    case LinuxDesktopVulkanPresentationStatus::xcb_surface_unavailable:
        return "xcb_surface_unavailable";
    case LinuxDesktopVulkanPresentationStatus::swapchain_unavailable:
        return "swapchain_unavailable";
    case LinuxDesktopVulkanPresentationStatus::readback_unavailable:
        return "readback_unavailable";
    case LinuxDesktopVulkanPresentationStatus::validation_log_dirty:
        return "validation_log_dirty";
    case LinuxDesktopVulkanPresentationStatus::native_handle_access:
        return "native_handle_access";
    }
    return "unknown";
}

LinuxDesktopHostReadinessReport evaluate_linux_desktop_host_request(const LinuxDesktopHostRequest& request) {
    if (request.title.empty()) {
        return invalid_linux_desktop_request("Linux desktop host window title must not be empty");
    }
    if (!is_valid_xcb_extent(request.extent)) {
        return invalid_linux_desktop_request("Linux desktop host window extent must fit XCB uint16 dimensions");
    }

    LinuxDesktopHostReadinessReport report;
    report.status = LinuxDesktopHostStatus::host_gated;
    report.null_renderer_fallback_available = request.allow_null_fallback;
    report.native_handle_access = false;
#if defined(__linux__)
    report.linux_host = true;
    report.diagnostic = "Linux desktop host request is valid; XCB runtime probing is required";
#else
    report.linux_host = false;
    report.diagnostic = "Linux desktop host requires a Linux host";
#endif
    report.vulkan_xcb_surface_candidate = request.require_vulkan_surface && report.linux_host;
    return report;
}

LinuxDesktopVulkanPresentationReport
evaluate_linux_desktop_vulkan_presentation_request(const LinuxDesktopVulkanPresentationRequest& request) {
    LinuxDesktopVulkanPresentationReport report;
    report.native_handle_access = request.native_handle_access;
    report.environment_platform_windows_vulkan_inferred = false;

    if (request.native_handle_access) {
        report.status = LinuxDesktopVulkanPresentationStatus::native_handle_access;
        report.diagnostic = "Linux Vulkan presentation evidence must not expose native handles";
        return report;
    }

    if (!request.linux_host) {
        report.status = LinuxDesktopVulkanPresentationStatus::host_gated;
        report.diagnostic = "Linux Vulkan presentation requires a Linux host";
        return report;
    }

    if (!request.xcb_window_ready || !request.vulkan_loader_ready || !request.vulkan_xcb_surface_created ||
        !request.surface_support_probed) {
        report.status = LinuxDesktopVulkanPresentationStatus::xcb_surface_unavailable;
        report.diagnostic = "Linux Vulkan presentation requires XCB window, loader, XCB surface, and surface support";
        return report;
    }

    if (!request.swapchain_created || !request.frame_acquired || !request.frame_presented) {
        report.linux_package_smoke_ready =
            request.swapchain_created && request.frame_acquired && request.frame_presented;
        report.status = LinuxDesktopVulkanPresentationStatus::swapchain_unavailable;
        report.diagnostic = "Linux Vulkan package smoke requires swapchain acquire and present";
        return report;
    }

    report.linux_package_smoke_ready = true;

    if (!request.readback_nonzero) {
        report.status = LinuxDesktopVulkanPresentationStatus::readback_unavailable;
        report.diagnostic = "Linux Vulkan readiness requires nonzero swapchain readback evidence";
        return report;
    }

    report.linux_vulkan_readback_ready = true;

    if (!request.validation_log_clean) {
        report.status = LinuxDesktopVulkanPresentationStatus::validation_log_dirty;
        report.diagnostic = "Linux Vulkan readiness requires clean VK_LAYER_KHRONOS_validation logs";
        return report;
    }

    report.status = LinuxDesktopVulkanPresentationStatus::ready;
    report.linux_vulkan_validation_log_clean = true;
    report.environment_platform_linux_vulkan_ready = true;
    report.diagnostic = "Linux Vulkan presentation package smoke, readback, and validation logs are ready";
    return report;
}

LinuxDesktopVulkanPresentationReport
probe_linux_desktop_vulkan_presentation(const LinuxDesktopVulkanPresentationProbeDesc& desc) {
    if (!desc.execute_runtime_smoke) {
        auto report = evaluate_linux_desktop_vulkan_presentation_request(LinuxDesktopVulkanPresentationRequest{
#if defined(__linux__)
            .linux_host = true,
#else
            .linux_host = false,
#endif
        });
        report.status = LinuxDesktopVulkanPresentationStatus::host_gated;
        report.native_handle_access = false;
        report.environment_platform_windows_vulkan_inferred = false;
        report.diagnostic = "Linux Vulkan presentation runtime smoke was not requested";
        return report;
    }

#if defined(__linux__)
    return execute_linux_desktop_vulkan_presentation_probe(desc);
#else
    auto report = evaluate_linux_desktop_vulkan_presentation_request(LinuxDesktopVulkanPresentationRequest{
        .linux_host = false,
    });
    report.native_handle_access = false;
    report.environment_platform_windows_vulkan_inferred = false;
    static_cast<void>(desc);
    return report;
#endif
}

} // namespace mirakana
