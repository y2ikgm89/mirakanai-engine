// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/linux/linux_desktop_game_host.hpp"

#include <limits>
#include <utility>

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

} // namespace mirakana
