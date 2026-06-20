// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/linux/linux_desktop_game_host.hpp"

#include <cstdlib>
#include <dlfcn.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

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

struct xcb_generic_event_t {
    std::uint8_t response_type;
    std::uint8_t pad0;
    std::uint16_t sequence;
    std::uint32_t pad[7];
    std::uint32_t full_sequence;
};

struct xcb_configure_notify_event_t {
    std::uint8_t response_type;
    std::uint8_t pad0;
    std::uint16_t sequence;
    xcb_window_t event;
    xcb_window_t window;
    xcb_window_t above_sibling;
    std::int16_t x;
    std::int16_t y;
    std::uint16_t width;
    std::uint16_t height;
    std::uint16_t border_width;
    std::uint8_t override_redirect;
    std::uint8_t pad1;
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
using xcb_configure_window_fn = xcb_void_cookie_t (*)(xcb_connection_t*, xcb_window_t, std::uint16_t, const void*);
using xcb_flush_fn = int (*)(xcb_connection_t*);
using xcb_poll_for_event_fn = xcb_generic_event_t* (*)(xcb_connection_t*);

constexpr std::uint8_t xcb_copy_from_parent = 0;
constexpr std::uint16_t xcb_window_class_input_output = 1;
constexpr std::uint32_t xcb_cw_back_pixel = 1U << 1U;
constexpr std::uint32_t xcb_cw_event_mask = 1U << 11U;
constexpr std::uint32_t xcb_event_mask_key_press = 1U << 0U;
constexpr std::uint32_t xcb_event_mask_key_release = 1U << 1U;
constexpr std::uint32_t xcb_event_mask_button_press = 1U << 2U;
constexpr std::uint32_t xcb_event_mask_button_release = 1U << 3U;
constexpr std::uint32_t xcb_event_mask_pointer_motion = 1U << 6U;
constexpr std::uint32_t xcb_event_mask_exposure = 1U << 15U;
constexpr std::uint32_t xcb_event_mask_structure_notify = 1U << 17U;
constexpr std::uint16_t xcb_config_window_x = 1U << 0U;
constexpr std::uint16_t xcb_config_window_y = 1U << 1U;
constexpr std::uint16_t xcb_config_window_width = 1U << 2U;
constexpr std::uint16_t xcb_config_window_height = 1U << 3U;
constexpr std::uint8_t xcb_destroy_notify = 17;
constexpr std::uint8_t xcb_configure_notify = 22;
constexpr std::uint8_t xcb_client_message = 33;

class LinuxDesktopHostRuntimeError final : public std::runtime_error {
  public:
    LinuxDesktopHostRuntimeError(LinuxDesktopHostStatus status, std::string message)
        : std::runtime_error(std::move(message)), status_(status) {}

    [[nodiscard]] LinuxDesktopHostStatus status() const noexcept {
        return status_;
    }

  private:
    LinuxDesktopHostStatus status_;
};

struct LinuxXcbApi {
    explicit LinuxXcbApi(void* library_handle) noexcept : library(library_handle) {}

    LinuxXcbApi(const LinuxXcbApi&) = delete;
    LinuxXcbApi& operator=(const LinuxXcbApi&) = delete;

    ~LinuxXcbApi() {
        if (library != nullptr) {
            ::dlclose(library);
        }
    }

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
    xcb_configure_window_fn configure_window{nullptr};
    xcb_flush_fn flush{nullptr};
    xcb_poll_for_event_fn poll_for_event{nullptr};
};

struct LoadedXcbApi {
    LinuxDesktopHostStatus status{LinuxDesktopHostStatus::xcb_runtime_unavailable};
    std::string diagnostic;
    std::shared_ptr<LinuxXcbApi> api;
};

class XcbConnectionOwner final {
  public:
    XcbConnectionOwner(LinuxXcbApi& api, xcb_connection_t* connection) noexcept : api_(&api), connection_(connection) {}

    XcbConnectionOwner(const XcbConnectionOwner&) = delete;
    XcbConnectionOwner& operator=(const XcbConnectionOwner&) = delete;

    ~XcbConnectionOwner() {
        if (connection_ != nullptr) {
            api_->disconnect(connection_);
        }
    }

    [[nodiscard]] xcb_connection_t* get() const noexcept {
        return connection_;
    }

    [[nodiscard]] xcb_connection_t* release() noexcept {
        auto* const result = connection_;
        connection_ = nullptr;
        return result;
    }

  private:
    LinuxXcbApi* api_{nullptr};
    xcb_connection_t* connection_{nullptr};
};

template <typename Function>
[[nodiscard]] bool resolve_xcb_symbol(const LinuxXcbApi& api, Function& target, const char* name,
                                      std::string& diagnostic) {
    (void)::dlerror();
    auto* const symbol = ::dlsym(api.library, name);
    const char* const error = ::dlerror();
    if (error != nullptr || symbol == nullptr) {
        diagnostic = std::string("dlsym failed for ") + name;
        if (error != nullptr) {
            diagnostic.append(": ");
            diagnostic.append(error);
        }
        return false;
    }
    target = reinterpret_cast<Function>(symbol);
    return true;
}

[[nodiscard]] LoadedXcbApi load_xcb_api() {
    void* const library = ::dlopen("libxcb.so.1", RTLD_LAZY | RTLD_LOCAL);
    if (library == nullptr) {
        LoadedXcbApi result;
        result.status = LinuxDesktopHostStatus::xcb_runtime_unavailable;
        const char* const error = ::dlerror();
        result.diagnostic =
            error != nullptr ? std::string("dlopen libxcb.so.1 failed: ") + error : "dlopen libxcb.so.1 failed";
        return result;
    }

    auto api = std::make_shared<LinuxXcbApi>(library);
    std::string diagnostic;
    const bool symbols_ready =
        resolve_xcb_symbol(*api, api->connect, "xcb_connect", diagnostic) &&
        resolve_xcb_symbol(*api, api->connection_has_error, "xcb_connection_has_error", diagnostic) &&
        resolve_xcb_symbol(*api, api->disconnect, "xcb_disconnect", diagnostic) &&
        resolve_xcb_symbol(*api, api->get_setup, "xcb_get_setup", diagnostic) &&
        resolve_xcb_symbol(*api, api->setup_roots_iterator, "xcb_setup_roots_iterator", diagnostic) &&
        resolve_xcb_symbol(*api, api->generate_id, "xcb_generate_id", diagnostic) &&
        resolve_xcb_symbol(*api, api->create_window, "xcb_create_window", diagnostic) &&
        resolve_xcb_symbol(*api, api->map_window, "xcb_map_window", diagnostic) &&
        resolve_xcb_symbol(*api, api->destroy_window, "xcb_destroy_window", diagnostic) &&
        resolve_xcb_symbol(*api, api->configure_window, "xcb_configure_window", diagnostic) &&
        resolve_xcb_symbol(*api, api->flush, "xcb_flush", diagnostic) &&
        resolve_xcb_symbol(*api, api->poll_for_event, "xcb_poll_for_event", diagnostic);

    if (!symbols_ready) {
        return LoadedXcbApi{.status = LinuxDesktopHostStatus::xcb_symbols_unavailable,
                            .diagnostic = std::move(diagnostic)};
    }

    return LoadedXcbApi{.status = LinuxDesktopHostStatus::ready, .api = std::move(api)};
}

[[nodiscard]] bool is_valid_xcb_extent(WindowExtent extent) noexcept {
    return extent.width > 0 && extent.height > 0 &&
           extent.width <= static_cast<std::uint32_t>(std::numeric_limits<std::uint16_t>::max()) &&
           extent.height <= static_cast<std::uint32_t>(std::numeric_limits<std::uint16_t>::max());
}

[[nodiscard]] bool is_valid_xcb_position(WindowPosition position) noexcept {
    return position.x >= static_cast<std::int32_t>(std::numeric_limits<std::int16_t>::min()) &&
           position.x <= static_cast<std::int32_t>(std::numeric_limits<std::int16_t>::max()) &&
           position.y >= static_cast<std::int32_t>(std::numeric_limits<std::int16_t>::min()) &&
           position.y <= static_cast<std::int32_t>(std::numeric_limits<std::int16_t>::max());
}

[[nodiscard]] const char* display_name_or_null(const char* display_name) noexcept {
    if (display_name == nullptr || display_name[0] == '\0') {
        return nullptr;
    }
    return display_name;
}

[[nodiscard]] LinuxDesktopHostRequest request_from_desc(const LinuxDesktopGameHostDesc& desc) {
    return LinuxDesktopHostRequest{
        .title = desc.title,
        .extent = desc.extent,
        .allow_null_fallback = desc.allow_null_fallback,
        .require_vulkan_surface = desc.require_vulkan_surface,
    };
}

[[nodiscard]] WindowDesc window_desc_from_host_desc(const LinuxDesktopGameHostDesc& desc) {
    if (!desc.allow_null_fallback) {
        throw LinuxDesktopHostRuntimeError(
            LinuxDesktopHostStatus::host_gated,
            "LinuxDesktopGameHost currently requires NullRenderer fallback until Linux Vulkan presentation is added");
    }
    return WindowDesc{.title = desc.title, .extent = desc.extent};
}

[[nodiscard]] LinuxDesktopHostReadinessReport make_ready_report(const LinuxDesktopGameHostDesc& desc) {
    return LinuxDesktopHostReadinessReport{
        .status = LinuxDesktopHostStatus::ready,
        .linux_host = true,
        .xcb_runtime_loaded = true,
        .xcb_symbols_resolved = true,
        .xcb_display_connected = true,
        .xcb_window_created = true,
        .event_polling_available = true,
        .null_renderer_fallback_available = desc.allow_null_fallback,
        .vulkan_xcb_surface_candidate = desc.require_vulkan_surface,
        .native_handle_access = false,
        .diagnostic = "Linux XCB desktop host is ready behind first-party runtime host contracts",
    };
}

void throw_if_invalid_window_desc(const WindowDesc& desc) {
    if (desc.title.empty()) {
        throw LinuxDesktopHostRuntimeError(LinuxDesktopHostStatus::invalid_request,
                                           "Linux XCB window title must not be empty");
    }
    if (!is_valid_xcb_extent(desc.extent)) {
        throw LinuxDesktopHostRuntimeError(LinuxDesktopHostStatus::invalid_request,
                                           "Linux XCB window extent must fit uint16 dimensions");
    }
    if (!is_valid_xcb_position(desc.position)) {
        throw LinuxDesktopHostRuntimeError(LinuxDesktopHostStatus::invalid_request,
                                           "Linux XCB window position must fit int16 coordinates");
    }
}

} // namespace

struct LinuxXcbWindow::Impl {
    Impl(WindowDesc desc, const char* display_name, bool map_window)
        : title(std::move(desc.title)), extent(desc.extent), position(desc.position), api(load_api_or_throw()) {
        throw_if_invalid_window_desc(WindowDesc{.title = title, .extent = extent, .position = position});

        int screen_index = 0;
        XcbConnectionOwner pending_connection(*api, api->connect(display_name_or_null(display_name), &screen_index));
        if (pending_connection.get() == nullptr || api->connection_has_error(pending_connection.get()) != 0) {
            throw LinuxDesktopHostRuntimeError(LinuxDesktopHostStatus::xcb_display_unavailable,
                                               "xcb_connect could not open a display connection");
        }

        const auto* const setup = api->get_setup(pending_connection.get());
        if (setup == nullptr) {
            throw LinuxDesktopHostRuntimeError(LinuxDesktopHostStatus::xcb_display_unavailable,
                                               "xcb_get_setup returned no display setup");
        }

        const auto root = api->setup_roots_iterator(setup);
        if (root.rem <= 0 || root.data == nullptr) {
            throw LinuxDesktopHostRuntimeError(LinuxDesktopHostStatus::xcb_display_unavailable,
                                               "xcb_setup_roots_iterator returned no screen");
        }

        screen = root.data;
        window = api->generate_id(pending_connection.get());
        const std::uint32_t event_mask = xcb_event_mask_exposure | xcb_event_mask_structure_notify |
                                         xcb_event_mask_key_press | xcb_event_mask_key_release |
                                         xcb_event_mask_button_press | xcb_event_mask_button_release |
                                         xcb_event_mask_pointer_motion;
        const std::uint32_t values[] = {screen->black_pixel, event_mask};
        (void)api->create_window(pending_connection.get(), xcb_copy_from_parent, window, screen->root,
                                 static_cast<std::int16_t>(position.x), static_cast<std::int16_t>(position.y),
                                 static_cast<std::uint16_t>(extent.width), static_cast<std::uint16_t>(extent.height), 0,
                                 xcb_window_class_input_output, screen->root_visual,
                                 xcb_cw_back_pixel | xcb_cw_event_mask, values);
        if (map_window) {
            (void)api->map_window(pending_connection.get(), window);
        }
        (void)api->flush(pending_connection.get());
        if (api->connection_has_error(pending_connection.get()) != 0) {
            throw LinuxDesktopHostRuntimeError(LinuxDesktopHostStatus::xcb_window_unavailable,
                                               "xcb_create_window failed on the display connection");
        }
        connection = pending_connection.release();
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    ~Impl() {
        if (connection != nullptr) {
            if (window != 0) {
                (void)api->destroy_window(connection, window);
                (void)api->flush(connection);
            }
            api->disconnect(connection);
        }
    }

    [[nodiscard]] static std::shared_ptr<LinuxXcbApi> load_api_or_throw() {
        auto loaded = load_xcb_api();
        if (loaded.status != LinuxDesktopHostStatus::ready || loaded.api == nullptr) {
            throw LinuxDesktopHostRuntimeError(loaded.status, std::move(loaded.diagnostic));
        }
        return std::move(loaded.api);
    }

    void configure(std::uint16_t mask, const std::uint32_t* values) {
        if (connection == nullptr || window == 0) {
            return;
        }
        (void)api->configure_window(connection, window, mask, values);
        (void)api->flush(connection);
    }

    void poll_events(VirtualLifecycle* lifecycle) {
        if (connection == nullptr) {
            return;
        }

        while (auto* event = api->poll_for_event(connection)) {
            std::unique_ptr<xcb_generic_event_t, decltype(&std::free)> event_owner(event, &std::free);
            const auto response_type = static_cast<std::uint8_t>(event->response_type & 0x7FU);
            if (response_type == xcb_destroy_notify || response_type == xcb_client_message) {
                open = false;
                if (lifecycle != nullptr) {
                    lifecycle->push(LifecycleEventKind::quit_requested);
                }
                continue;
            }
            if (response_type == xcb_configure_notify) {
                const auto* const configure = reinterpret_cast<const xcb_configure_notify_event_t*>(event);
                if (configure->width > 0 && configure->height > 0) {
                    extent = WindowExtent{.width = configure->width, .height = configure->height};
                }
                position = WindowPosition{.x = configure->x, .y = configure->y};
            }
        }
    }

    std::string title;
    WindowExtent extent;
    WindowPosition position;
    std::shared_ptr<LinuxXcbApi> api;
    xcb_connection_t* connection{nullptr};
    xcb_screen_t* screen{nullptr};
    xcb_window_t window{0};
    bool open{true};
};

LinuxDesktopHostReadinessReport probe_linux_desktop_host(const LinuxDesktopGameHostDesc& desc) {
    auto report = evaluate_linux_desktop_host_request(request_from_desc(desc));
    if (report.status == LinuxDesktopHostStatus::invalid_request) {
        return report;
    }

    try {
        LinuxXcbWindow window(WindowDesc{.title = desc.title, .extent = desc.extent}, desc.display_name, false);
        (void)window;
        return make_ready_report(desc);
    } catch (const LinuxDesktopHostRuntimeError& error) {
        report.status = error.status();
        report.diagnostic = error.what();
        report.xcb_runtime_loaded = error.status() != LinuxDesktopHostStatus::xcb_runtime_unavailable;
        report.xcb_symbols_resolved = error.status() != LinuxDesktopHostStatus::xcb_runtime_unavailable &&
                                      error.status() != LinuxDesktopHostStatus::xcb_symbols_unavailable;
        report.xcb_display_connected = false;
        report.xcb_window_created = false;
        report.event_polling_available = false;
        report.native_handle_access = false;
        return report;
    }
}

LinuxXcbWindow::LinuxXcbWindow(WindowDesc desc, const char* display_name, bool map_window)
    : impl_(std::make_unique<Impl>(std::move(desc), display_name, map_window)) {}

LinuxXcbWindow::~LinuxXcbWindow() = default;

std::string_view LinuxXcbWindow::title() const noexcept {
    return impl_->title;
}

WindowExtent LinuxXcbWindow::extent() const noexcept {
    return impl_->extent;
}

WindowPosition LinuxXcbWindow::position() const noexcept {
    return impl_->position;
}

bool LinuxXcbWindow::is_open() const noexcept {
    return impl_->open;
}

void LinuxXcbWindow::resize(WindowExtent extent) {
    if (!is_valid_xcb_extent(extent)) {
        throw std::invalid_argument("Linux XCB window extent must fit uint16 dimensions");
    }
    const std::uint32_t values[] = {extent.width, extent.height};
    impl_->configure(xcb_config_window_width | xcb_config_window_height, values);
    impl_->extent = extent;
}

void LinuxXcbWindow::move(WindowPosition position) {
    if (!is_valid_xcb_position(position)) {
        throw std::invalid_argument("Linux XCB window position must fit int16 coordinates");
    }
    const std::uint32_t values[] = {static_cast<std::uint32_t>(position.x), static_cast<std::uint32_t>(position.y)};
    impl_->configure(xcb_config_window_x | xcb_config_window_y, values);
    impl_->position = position;
}

void LinuxXcbWindow::apply_placement(WindowPlacement placement) {
    if (!is_valid_xcb_extent(placement.extent)) {
        throw std::invalid_argument("Linux XCB window extent must fit uint16 dimensions");
    }
    if (!is_valid_xcb_position(placement.position)) {
        throw std::invalid_argument("Linux XCB window position must fit int16 coordinates");
    }
    const std::uint32_t values[] = {static_cast<std::uint32_t>(placement.position.x),
                                    static_cast<std::uint32_t>(placement.position.y), placement.extent.width,
                                    placement.extent.height};
    impl_->configure(xcb_config_window_x | xcb_config_window_y | xcb_config_window_width | xcb_config_window_height,
                     values);
    impl_->position = placement.position;
    impl_->extent = placement.extent;
}

void LinuxXcbWindow::request_close() noexcept {
    impl_->open = false;
}

void LinuxXcbWindow::poll_events(VirtualLifecycle* lifecycle) {
    impl_->poll_events(lifecycle);
}

LinuxDesktopEventPump::LinuxDesktopEventPump(LinuxXcbWindow& window) noexcept : window_(&window) {}

void LinuxDesktopEventPump::pump_events(DesktopHostServices& services) {
    if (window_ == nullptr) {
        return;
    }
    window_->poll_events(services.lifecycle);
}

struct LinuxDesktopGameHost::Impl {
    explicit Impl(LinuxDesktopGameHostDesc desc)
        : default_logger(desc.default_log_capacity), logger(desc.logger != nullptr ? *desc.logger : default_logger),
          registry(desc.registry != nullptr ? *desc.registry : default_registry),
          window(window_desc_from_host_desc(desc), desc.display_name, true),
          renderer(Extent2D{.width = desc.extent.width, .height = desc.extent.height}), event_pump(window),
          runner(logger, registry), readiness(make_ready_report(desc)) {}

    RingBufferLogger default_logger;
    Registry default_registry;
    ILogger& logger;
    Registry& registry;
    LinuxXcbWindow window;
    NullRenderer renderer;
    VirtualInput input;
    VirtualPointerInput pointer_input;
    VirtualGamepadInput gamepad_input;
    VirtualLifecycle lifecycle;
    LinuxDesktopEventPump event_pump;
    DesktopGameRunner runner;
    LinuxDesktopHostReadinessReport readiness;
};

LinuxDesktopGameHost::LinuxDesktopGameHost(LinuxDesktopGameHostDesc desc)
    : impl_(std::make_unique<Impl>(std::move(desc))) {}

LinuxDesktopGameHost::~LinuxDesktopGameHost() = default;

IWindow& LinuxDesktopGameHost::window() noexcept {
    return impl_->window;
}

const IWindow& LinuxDesktopGameHost::window() const noexcept {
    return impl_->window;
}

IRenderer& LinuxDesktopGameHost::renderer() noexcept {
    return impl_->renderer;
}

const IRenderer& LinuxDesktopGameHost::renderer() const noexcept {
    return impl_->renderer;
}

VirtualInput& LinuxDesktopGameHost::input() noexcept {
    return impl_->input;
}

VirtualPointerInput& LinuxDesktopGameHost::pointer_input() noexcept {
    return impl_->pointer_input;
}

VirtualGamepadInput& LinuxDesktopGameHost::gamepad_input() noexcept {
    return impl_->gamepad_input;
}

VirtualLifecycle& LinuxDesktopGameHost::lifecycle() noexcept {
    return impl_->lifecycle;
}

LinuxDesktopHostReadinessReport LinuxDesktopGameHost::readiness_report() const {
    return impl_->readiness;
}

DesktopRunResult LinuxDesktopGameHost::run(GameApp& app, DesktopRunConfig config) {
    DesktopHostServices services{
        .window = &impl_->window,
        .renderer = &impl_->renderer,
        .input = &impl_->input,
        .pointer_input = &impl_->pointer_input,
        .gamepad_input = &impl_->gamepad_input,
        .lifecycle = &impl_->lifecycle,
    };
    return impl_->runner.run(app, services, config, &impl_->event_pump);
}

} // namespace mirakana
