// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/window.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime_host/desktop_runner.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace mirakana {

enum class LinuxDesktopHostStatus : std::uint8_t {
    ready,
    invalid_request,
    host_gated,
    xcb_runtime_unavailable,
    xcb_symbols_unavailable,
    xcb_display_unavailable,
    xcb_window_unavailable,
};

enum class LinuxDesktopVulkanPresentationStatus : std::uint8_t {
    ready,
    host_gated,
    xcb_surface_unavailable,
    swapchain_unavailable,
    readback_unavailable,
    validation_log_dirty,
    native_handle_access,
};

struct LinuxDesktopHostRequest {
    std::string title{"MIRAIKANAI"};
    WindowExtent extent{.width = 1280, .height = 720};
    bool allow_null_fallback{true};
    bool require_vulkan_surface{false};
};

struct LinuxDesktopVulkanPresentationRequest {
    bool linux_host{false};
    bool xcb_window_ready{false};
    bool vulkan_loader_ready{false};
    bool vulkan_xcb_surface_created{false};
    bool surface_support_probed{false};
    bool swapchain_created{false};
    bool frame_acquired{false};
    bool frame_presented{false};
    bool readback_nonzero{false};
    bool validation_log_clean{false};
    bool native_handle_access{false};
};

struct LinuxDesktopVulkanPresentationProbeDesc {
    std::string title{"MIRAIKANAI Linux Vulkan Smoke"};
    WindowExtent extent{.width = 64, .height = 64};
    const char* display_name{nullptr};
    bool execute_runtime_smoke{true};
};

struct LinuxDesktopHostReadinessReport {
    LinuxDesktopHostStatus status{LinuxDesktopHostStatus::host_gated};
    bool linux_host{false};
    bool xcb_runtime_loaded{false};
    bool xcb_symbols_resolved{false};
    bool xcb_display_connected{false};
    bool xcb_window_created{false};
    bool event_polling_available{false};
    bool null_renderer_fallback_available{false};
    bool vulkan_xcb_surface_candidate{false};
    bool native_handle_access{false};
    std::string diagnostic;

    [[nodiscard]] bool ready() const noexcept {
        return status == LinuxDesktopHostStatus::ready;
    }
};

struct LinuxDesktopVulkanPresentationReport {
    LinuxDesktopVulkanPresentationStatus status{LinuxDesktopVulkanPresentationStatus::host_gated};
    bool linux_package_smoke_ready{false};
    bool linux_vulkan_readback_ready{false};
    bool linux_vulkan_validation_log_clean{false};
    bool environment_platform_linux_vulkan_ready{false};
    bool environment_platform_windows_vulkan_inferred{false};
    bool native_handle_access{false};
    std::string diagnostic;

    [[nodiscard]] bool ready() const noexcept {
        return status == LinuxDesktopVulkanPresentationStatus::ready && environment_platform_linux_vulkan_ready;
    }
};

struct LinuxDesktopGameHostDesc {
    std::string title{"MIRAIKANAI"};
    WindowExtent extent{.width = 1280, .height = 720};
    bool allow_null_fallback{true};
    bool require_vulkan_surface{false};
    const char* display_name{nullptr};
    ILogger* logger{nullptr};
    Registry* registry{nullptr};
    std::size_t default_log_capacity{256};
};

[[nodiscard]] std::string_view linux_desktop_host_status_name(LinuxDesktopHostStatus status) noexcept;
[[nodiscard]] std::string_view
linux_desktop_vulkan_presentation_status_name(LinuxDesktopVulkanPresentationStatus status) noexcept;
[[nodiscard]] LinuxDesktopHostReadinessReport
evaluate_linux_desktop_host_request(const LinuxDesktopHostRequest& request);
[[nodiscard]] LinuxDesktopVulkanPresentationReport
evaluate_linux_desktop_vulkan_presentation_request(const LinuxDesktopVulkanPresentationRequest& request);
[[nodiscard]] LinuxDesktopVulkanPresentationReport
probe_linux_desktop_vulkan_presentation(const LinuxDesktopVulkanPresentationProbeDesc& desc = {});
[[nodiscard]] LinuxDesktopHostReadinessReport probe_linux_desktop_host(const LinuxDesktopGameHostDesc& desc = {});

class LinuxXcbWindow final : public IWindow {
  public:
    explicit LinuxXcbWindow(WindowDesc desc, const char* display_name = nullptr, bool map_window = true);
    ~LinuxXcbWindow() override;

    LinuxXcbWindow(const LinuxXcbWindow&) = delete;
    LinuxXcbWindow& operator=(const LinuxXcbWindow&) = delete;
    LinuxXcbWindow(LinuxXcbWindow&&) = delete;
    LinuxXcbWindow& operator=(LinuxXcbWindow&&) = delete;

    [[nodiscard]] std::string_view title() const noexcept override;
    [[nodiscard]] WindowExtent extent() const noexcept override;
    [[nodiscard]] WindowPosition position() const noexcept override;
    [[nodiscard]] bool is_open() const noexcept override;

    void resize(WindowExtent extent) override;
    void move(WindowPosition position) override;
    void apply_placement(WindowPlacement placement) override;
    void request_close() noexcept override;

    void poll_events(VirtualLifecycle* lifecycle);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class LinuxDesktopEventPump final : public IDesktopEventPump {
  public:
    explicit LinuxDesktopEventPump(LinuxXcbWindow& window) noexcept;

    void pump_events(DesktopHostServices& services) override;

  private:
    LinuxXcbWindow* window_{nullptr};
};

class LinuxDesktopGameHost final {
  public:
    explicit LinuxDesktopGameHost(LinuxDesktopGameHostDesc desc = {});
    ~LinuxDesktopGameHost();

    LinuxDesktopGameHost(const LinuxDesktopGameHost&) = delete;
    LinuxDesktopGameHost& operator=(const LinuxDesktopGameHost&) = delete;
    LinuxDesktopGameHost(LinuxDesktopGameHost&&) = delete;
    LinuxDesktopGameHost& operator=(LinuxDesktopGameHost&&) = delete;

    [[nodiscard]] IWindow& window() noexcept;
    [[nodiscard]] const IWindow& window() const noexcept;
    [[nodiscard]] IRenderer& renderer() noexcept;
    [[nodiscard]] const IRenderer& renderer() const noexcept;
    [[nodiscard]] VirtualInput& input() noexcept;
    [[nodiscard]] VirtualPointerInput& pointer_input() noexcept;
    [[nodiscard]] VirtualGamepadInput& gamepad_input() noexcept;
    [[nodiscard]] VirtualLifecycle& lifecycle() noexcept;
    [[nodiscard]] LinuxDesktopHostReadinessReport readiness_report() const;

    [[nodiscard]] DesktopRunResult run(GameApp& app, DesktopRunConfig config = {});

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana
