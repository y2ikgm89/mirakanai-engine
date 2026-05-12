// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"

#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/sdl3/sdl_window.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_event_pump.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>

namespace mirakana {
namespace {

[[nodiscard]] std::size_t sanitize_log_capacity(std::size_t capacity) noexcept {
    return std::max<std::size_t>(capacity, 1);
}

[[nodiscard]] SdlDesktopPresentationDesc make_presentation_desc(const SdlDesktopGameHostDesc& desc,
                                                                SdlWindow& window) noexcept {
    return SdlDesktopPresentationDesc{
        .window = &window,
        .extent = Extent2D{desc.extent.width, desc.extent.height},
        .prefer_d3d12 = desc.prefer_d3d12,
        .prefer_vulkan = desc.prefer_vulkan,
        .allow_null_fallback = desc.allow_null_fallback,
        .prefer_warp = desc.prefer_warp,
        .enable_debug_layer = desc.enable_debug_layer,
        .vsync = desc.vsync,
        .d3d12_renderer = desc.d3d12_renderer,
        .vulkan_renderer = desc.vulkan_renderer,
        .d3d12_scene_renderer = desc.d3d12_scene_renderer,
        .vulkan_scene_renderer = desc.vulkan_scene_renderer,
    };
}

} // namespace

struct SdlDesktopGameHost::Impl {
    explicit Impl(const SdlDesktopGameHostDesc& desc)
        : default_logger(sanitize_log_capacity(desc.default_log_capacity)), default_registry(),
          logger(desc.logger != nullptr ? desc.logger : &default_logger),
          registry(desc.registry != nullptr ? desc.registry : &default_registry),
          runtime(SdlRuntimeDesc{.video_driver_hint = desc.video_driver_hint}),
          window(WindowDesc{desc.title, desc.extent}), presentation(make_presentation_desc(desc, window)),
          event_pump(window) {}

    RingBufferLogger default_logger;
    Registry default_registry;
    ILogger* logger{nullptr};
    Registry* registry{nullptr};
    SdlRuntime runtime;
    SdlWindow window;
    SdlDesktopPresentation presentation;
    VirtualInput input;
    VirtualPointerInput pointer_input;
    VirtualGamepadInput gamepad_input;
    VirtualLifecycle lifecycle;
    SdlDesktopEventPump event_pump;
};

SdlDesktopGameHost::SdlDesktopGameHost(const SdlDesktopGameHostDesc& desc) : impl_(std::make_unique<Impl>(desc)) {}

SdlDesktopGameHost::~SdlDesktopGameHost() = default;

IWindow& SdlDesktopGameHost::window() noexcept {
    return impl_->window;
}

const IWindow& SdlDesktopGameHost::window() const noexcept {
    return impl_->window;
}

IRenderer& SdlDesktopGameHost::renderer() noexcept {
    return impl_->presentation.renderer();
}

const IRenderer& SdlDesktopGameHost::renderer() const noexcept {
    return impl_->presentation.renderer();
}

VirtualInput& SdlDesktopGameHost::input() noexcept {
    return impl_->input;
}

VirtualPointerInput& SdlDesktopGameHost::pointer_input() noexcept {
    return impl_->pointer_input;
}

VirtualGamepadInput& SdlDesktopGameHost::gamepad_input() noexcept {
    return impl_->gamepad_input;
}

VirtualLifecycle& SdlDesktopGameHost::lifecycle() noexcept {
    return impl_->lifecycle;
}

SdlDesktopPresentationBackend SdlDesktopGameHost::presentation_backend() const noexcept {
    return impl_->presentation.backend();
}

std::string_view SdlDesktopGameHost::presentation_backend_name() const noexcept {
    return impl_->presentation.backend_name();
}

SdlDesktopPresentationReport SdlDesktopGameHost::presentation_report() const noexcept {
    return impl_->presentation.report();
}

std::span<const SdlDesktopPresentationBackendReport> SdlDesktopGameHost::presentation_backend_reports() const noexcept {
    return impl_->presentation.backend_reports();
}

std::span<const SdlDesktopPresentationDiagnostic> SdlDesktopGameHost::presentation_diagnostics() const noexcept {
    return impl_->presentation.diagnostics();
}

SdlDesktopPresentationSceneGpuBindingStatus SdlDesktopGameHost::scene_gpu_binding_status() const noexcept {
    return impl_->presentation.scene_gpu_binding_status();
}

bool SdlDesktopGameHost::scene_gpu_bindings_ready() const noexcept {
    return impl_->presentation.scene_gpu_bindings_ready();
}

SdlDesktopPresentationSceneGpuBindingStats SdlDesktopGameHost::scene_gpu_binding_stats() const noexcept {
    return impl_->presentation.scene_gpu_binding_stats();
}

std::span<const SdlDesktopPresentationSceneGpuBindingDiagnostic>
SdlDesktopGameHost::scene_gpu_binding_diagnostics() const noexcept {
    return impl_->presentation.scene_gpu_binding_diagnostics();
}

SdlDesktopPresentation& SdlDesktopGameHost::presentation() noexcept {
    return impl_->presentation;
}

const SdlDesktopPresentation& SdlDesktopGameHost::presentation() const noexcept {
    return impl_->presentation;
}

SdlDesktopPresentationPostprocessStatus SdlDesktopGameHost::postprocess_status() const noexcept {
    return impl_->presentation.postprocess_status();
}

bool SdlDesktopGameHost::postprocess_ready() const noexcept {
    return impl_->presentation.postprocess_ready();
}

std::span<const SdlDesktopPresentationPostprocessDiagnostic>
SdlDesktopGameHost::postprocess_diagnostics() const noexcept {
    return impl_->presentation.postprocess_diagnostics();
}

SdlDesktopPresentationDirectionalShadowStatus SdlDesktopGameHost::directional_shadow_status() const noexcept {
    return impl_->presentation.directional_shadow_status();
}

bool SdlDesktopGameHost::directional_shadow_ready() const noexcept {
    return impl_->presentation.directional_shadow_ready();
}

std::span<const SdlDesktopPresentationDirectionalShadowDiagnostic>
SdlDesktopGameHost::directional_shadow_diagnostics() const noexcept {
    return impl_->presentation.directional_shadow_diagnostics();
}

SdlDesktopPresentationNativeUiOverlayStatus SdlDesktopGameHost::native_ui_overlay_status() const noexcept {
    return impl_->presentation.native_ui_overlay_status();
}

bool SdlDesktopGameHost::native_ui_overlay_ready() const noexcept {
    return impl_->presentation.native_ui_overlay_ready();
}

std::span<const SdlDesktopPresentationNativeUiOverlayDiagnostic>
SdlDesktopGameHost::native_ui_overlay_diagnostics() const noexcept {
    return impl_->presentation.native_ui_overlay_diagnostics();
}

SdlDesktopPresentationNativeUiTextureOverlayStatus
SdlDesktopGameHost::native_ui_texture_overlay_status() const noexcept {
    return impl_->presentation.native_ui_texture_overlay_status();
}

bool SdlDesktopGameHost::native_ui_texture_overlay_atlas_ready() const noexcept {
    return impl_->presentation.native_ui_texture_overlay_atlas_ready();
}

std::span<const SdlDesktopPresentationNativeUiTextureOverlayDiagnostic>
SdlDesktopGameHost::native_ui_texture_overlay_diagnostics() const noexcept {
    return impl_->presentation.native_ui_texture_overlay_diagnostics();
}

DesktopRunResult SdlDesktopGameHost::run(GameApp& app, DesktopRunConfig config) {
    DesktopHostServices services{
        .window = &impl_->window,
        .renderer = &impl_->presentation.renderer(),
        .input = &impl_->input,
        .pointer_input = &impl_->pointer_input,
        .gamepad_input = &impl_->gamepad_input,
        .lifecycle = &impl_->lifecycle,
    };
    DesktopGameRunner runner(*impl_->logger, *impl_->registry);
    return runner.run(app, services, config, &impl_->event_pump);
}

} // namespace mirakana
