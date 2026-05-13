// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/window.hpp"
#include "mirakana/runtime_host/desktop_runner.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace mirakana {

class ILogger;
class IRenderer;
class Registry;
class VirtualGamepadInput;
class VirtualInput;
class VirtualLifecycle;
class VirtualPointerInput;

struct SdlDesktopGameHostDesc {
    std::string title{"GameEngine Desktop Game"};
    WindowExtent extent{.width = 960, .height = 540};
    std::string video_driver_hint;
    bool prefer_d3d12{true};
    bool prefer_vulkan{false};
    bool allow_null_fallback{true};
    bool prefer_warp{false};
    bool enable_debug_layer{false};
    bool vsync{true};
    const SdlDesktopPresentationD3d12RendererDesc* d3d12_renderer{nullptr};
    const SdlDesktopPresentationVulkanRendererDesc* vulkan_renderer{nullptr};
    const SdlDesktopPresentationD3d12SceneRendererDesc* d3d12_scene_renderer{nullptr};
    const SdlDesktopPresentationVulkanSceneRendererDesc* vulkan_scene_renderer{nullptr};
    ILogger* logger{nullptr};
    Registry* registry{nullptr};
    std::size_t default_log_capacity{32};
};

class SdlDesktopGameHost final {
  public:
    explicit SdlDesktopGameHost(const SdlDesktopGameHostDesc& desc = {});
    ~SdlDesktopGameHost();

    SdlDesktopGameHost(const SdlDesktopGameHost&) = delete;
    SdlDesktopGameHost& operator=(const SdlDesktopGameHost&) = delete;
    SdlDesktopGameHost(SdlDesktopGameHost&&) = delete;
    SdlDesktopGameHost& operator=(SdlDesktopGameHost&&) = delete;

    [[nodiscard]] IWindow& window() noexcept;
    [[nodiscard]] const IWindow& window() const noexcept;
    [[nodiscard]] IRenderer& renderer() noexcept;
    [[nodiscard]] const IRenderer& renderer() const noexcept;
    [[nodiscard]] VirtualInput& input() noexcept;
    [[nodiscard]] VirtualPointerInput& pointer_input() noexcept;
    [[nodiscard]] VirtualGamepadInput& gamepad_input() noexcept;
    [[nodiscard]] VirtualLifecycle& lifecycle() noexcept;
    [[nodiscard]] SdlDesktopPresentationBackend presentation_backend() const noexcept;
    [[nodiscard]] std::string_view presentation_backend_name() const noexcept;
    [[nodiscard]] SdlDesktopPresentationReport presentation_report() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationBackendReport> presentation_backend_reports() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationDiagnostic> presentation_diagnostics() const noexcept;
    [[nodiscard]] SdlDesktopPresentationSceneGpuBindingStatus scene_gpu_binding_status() const noexcept;
    [[nodiscard]] bool scene_gpu_bindings_ready() const noexcept;
    [[nodiscard]] SdlDesktopPresentationSceneGpuBindingStats scene_gpu_binding_stats() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationSceneGpuBindingDiagnostic>
    scene_gpu_binding_diagnostics() const noexcept;
    [[nodiscard]] SdlDesktopPresentationPostprocessStatus postprocess_status() const noexcept;
    [[nodiscard]] bool postprocess_ready() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationPostprocessDiagnostic> postprocess_diagnostics() const noexcept;
    [[nodiscard]] SdlDesktopPresentationDirectionalShadowStatus directional_shadow_status() const noexcept;
    [[nodiscard]] bool directional_shadow_ready() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationDirectionalShadowDiagnostic>
    directional_shadow_diagnostics() const noexcept;
    [[nodiscard]] SdlDesktopPresentationNativeUiOverlayStatus native_ui_overlay_status() const noexcept;
    [[nodiscard]] bool native_ui_overlay_ready() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationNativeUiOverlayDiagnostic>
    native_ui_overlay_diagnostics() const noexcept;
    [[nodiscard]] SdlDesktopPresentationNativeUiTextureOverlayStatus native_ui_texture_overlay_status() const noexcept;
    [[nodiscard]] bool native_ui_texture_overlay_atlas_ready() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationNativeUiTextureOverlayDiagnostic>
    native_ui_texture_overlay_diagnostics() const noexcept;

    [[nodiscard]] SdlDesktopPresentation& presentation() noexcept;
    [[nodiscard]] const SdlDesktopPresentation& presentation() const noexcept;

    [[nodiscard]] DesktopRunResult run(GameApp& app, DesktopRunConfig config = {});

  private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana
