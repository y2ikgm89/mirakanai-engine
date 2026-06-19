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

} // namespace mirakana
