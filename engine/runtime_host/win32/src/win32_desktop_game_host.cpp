// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/win32/win32_desktop_game_host.hpp"

#include "mirakana/runtime_host/win32/win32_desktop_event_pump.hpp"

#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] Win32DesktopPresentationDesc make_presentation_desc(win32::Win32Window& window,
                                                                  const Win32DesktopGameHostDesc& desc) {
    return Win32DesktopPresentationDesc{
        .window = &window,
        .extent = Extent2D{.width = desc.extent.width, .height = desc.extent.height},
        .prefer_d3d12 = desc.prefer_d3d12,
        .allow_null_fallback = desc.allow_null_fallback,
        .prefer_warp = desc.prefer_warp,
        .enable_debug_layer = desc.enable_debug_layer,
        .vsync = desc.vsync,
        .request_tearing = desc.request_tearing,
        .d3d12_renderer = desc.d3d12_renderer,
        .d3d12_scene_renderer = desc.d3d12_scene_renderer,
    };
}

} // namespace

struct Win32DesktopGameHost::Impl {
    explicit Impl(Win32DesktopGameHostDesc desc)
        : default_logger(desc.default_log_capacity), logger(desc.logger != nullptr ? *desc.logger : default_logger),
          registry(desc.registry != nullptr ? *desc.registry : default_registry),
          runtime(win32::Win32RuntimeDesc{.window_class_name = "MIRAIKANAI Win32 Desktop Game Host"}),
          window(runtime, WindowDesc{.title = std::move(desc.title), .extent = desc.extent}),
          presentation(make_presentation_desc(window, desc)), event_pump(window), runner(logger, registry) {}

    RingBufferLogger default_logger;
    Registry default_registry;
    ILogger& logger;
    Registry& registry;
    win32::Win32Runtime runtime;
    win32::Win32Window window;
    Win32DesktopPresentation presentation;
    VirtualInput input;
    VirtualPointerInput pointer_input;
    VirtualGamepadInput gamepad_input;
    VirtualLifecycle lifecycle;
    Win32DesktopEventPump event_pump;
    DesktopGameRunner runner;
};

Win32DesktopGameHost::Win32DesktopGameHost(Win32DesktopGameHostDesc desc)
    : impl_(std::make_unique<Impl>(std::move(desc))) {}

Win32DesktopGameHost::~Win32DesktopGameHost() = default;

IWindow& Win32DesktopGameHost::window() noexcept {
    return impl_->window;
}

const IWindow& Win32DesktopGameHost::window() const noexcept {
    return impl_->window;
}

IRenderer& Win32DesktopGameHost::renderer() noexcept {
    return impl_->presentation.renderer();
}

const IRenderer& Win32DesktopGameHost::renderer() const noexcept {
    return impl_->presentation.renderer();
}

VirtualInput& Win32DesktopGameHost::input() noexcept {
    return impl_->input;
}

VirtualPointerInput& Win32DesktopGameHost::pointer_input() noexcept {
    return impl_->pointer_input;
}

VirtualGamepadInput& Win32DesktopGameHost::gamepad_input() noexcept {
    return impl_->gamepad_input;
}

VirtualLifecycle& Win32DesktopGameHost::lifecycle() noexcept {
    return impl_->lifecycle;
}

Win32DesktopPresentationBackend Win32DesktopGameHost::presentation_backend() const noexcept {
    return impl_->presentation.backend();
}

std::string_view Win32DesktopGameHost::presentation_backend_name() const noexcept {
    return impl_->presentation.backend_name();
}

Win32DesktopPresentationReport Win32DesktopGameHost::presentation_report() const {
    return impl_->presentation.report();
}

std::span<const Win32DesktopPresentationBackendReport>
Win32DesktopGameHost::presentation_backend_reports() const noexcept {
    return impl_->presentation.backend_reports();
}

std::span<const Win32DesktopPresentationDiagnostic> Win32DesktopGameHost::presentation_diagnostics() const noexcept {
    return impl_->presentation.diagnostics();
}

Win32DesktopPresentation& Win32DesktopGameHost::presentation() noexcept {
    return impl_->presentation;
}

const Win32DesktopPresentation& Win32DesktopGameHost::presentation() const noexcept {
    return impl_->presentation;
}

DesktopRunResult Win32DesktopGameHost::run(GameApp& app, DesktopRunConfig config) {
    DesktopHostServices services{
        .window = &impl_->window,
        .renderer = &impl_->presentation.renderer(),
        .input = &impl_->input,
        .pointer_input = &impl_->pointer_input,
        .gamepad_input = &impl_->gamepad_input,
        .lifecycle = &impl_->lifecycle,
    };
    return impl_->runner.run(app, services, config, &impl_->event_pump);
}

} // namespace mirakana
