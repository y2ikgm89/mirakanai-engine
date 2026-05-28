// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/win32/win32_window.hpp"
#include "mirakana/runtime_host/desktop_runner.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace mirakana {

struct Win32DesktopGameHostDesc {
    std::string title{"MIRAIKANAI"};
    WindowExtent extent{.width = 1280, .height = 720};
    bool prefer_d3d12{true};
    bool allow_null_fallback{true};
    bool prefer_warp{false};
    bool enable_debug_layer{false};
    bool vsync{true};
    bool request_tearing{false};
    const Win32DesktopPresentationD3d12RendererDesc* d3d12_renderer{nullptr};
    const Win32DesktopPresentationD3d12SceneRendererDesc* d3d12_scene_renderer{nullptr};
    ILogger* logger{nullptr};
    Registry* registry{nullptr};
    std::size_t default_log_capacity{256};
};

class Win32DesktopGameHost final {
  public:
    explicit Win32DesktopGameHost(Win32DesktopGameHostDesc desc = {});
    ~Win32DesktopGameHost();

    Win32DesktopGameHost(const Win32DesktopGameHost&) = delete;
    Win32DesktopGameHost& operator=(const Win32DesktopGameHost&) = delete;
    Win32DesktopGameHost(Win32DesktopGameHost&&) = delete;
    Win32DesktopGameHost& operator=(Win32DesktopGameHost&&) = delete;

    [[nodiscard]] IWindow& window() noexcept;
    [[nodiscard]] const IWindow& window() const noexcept;
    [[nodiscard]] IRenderer& renderer() noexcept;
    [[nodiscard]] const IRenderer& renderer() const noexcept;
    [[nodiscard]] VirtualInput& input() noexcept;
    [[nodiscard]] VirtualPointerInput& pointer_input() noexcept;
    [[nodiscard]] VirtualGamepadInput& gamepad_input() noexcept;
    [[nodiscard]] VirtualLifecycle& lifecycle() noexcept;
    [[nodiscard]] Win32DesktopPresentationBackend presentation_backend() const noexcept;
    [[nodiscard]] std::string_view presentation_backend_name() const noexcept;
    [[nodiscard]] Win32DesktopPresentationReport presentation_report() const;
    [[nodiscard]] std::span<const Win32DesktopPresentationBackendReport> presentation_backend_reports() const noexcept;
    [[nodiscard]] std::span<const Win32DesktopPresentationDiagnostic> presentation_diagnostics() const noexcept;
    [[nodiscard]] Win32DesktopPresentation& presentation() noexcept;
    [[nodiscard]] const Win32DesktopPresentation& presentation() const noexcept;

    [[nodiscard]] DesktopRunResult run(GameApp& app, DesktopRunConfig config = {});

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana
