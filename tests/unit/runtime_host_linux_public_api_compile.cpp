// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/linux/linux_desktop_game_host.hpp"

#include <string_view>

int main() {
    mirakana::LinuxDesktopHostRequest request;
    request.title = "Linux Host";
    request.require_vulkan_surface = true;

    const auto request_report = mirakana::evaluate_linux_desktop_host_request(request);

    mirakana::LinuxDesktopGameHostDesc desc;
    desc.title = "Linux Host";
    desc.require_vulkan_surface = true;
    desc.allow_null_fallback = true;

    const auto probe = mirakana::probe_linux_desktop_host(desc);

    mirakana::LinuxDesktopHostReadinessReport manual;
    manual.status = mirakana::LinuxDesktopHostStatus::ready;
    manual.linux_host = true;
    manual.xcb_runtime_loaded = true;
    manual.xcb_symbols_resolved = true;
    manual.xcb_display_connected = true;
    manual.xcb_window_created = true;
    manual.event_polling_available = true;
    manual.null_renderer_fallback_available = true;
    manual.vulkan_xcb_surface_candidate = true;

    const auto presentation =
        mirakana::evaluate_linux_desktop_vulkan_presentation_request(mirakana::LinuxDesktopVulkanPresentationRequest{
            .linux_host = true,
            .xcb_window_ready = true,
            .vulkan_loader_ready = true,
            .vulkan_xcb_surface_created = true,
            .surface_support_probed = true,
            .swapchain_created = true,
            .frame_acquired = true,
            .frame_presented = true,
            .readback_nonzero = true,
            .validation_log_clean = true,
        });

    return request_report.linux_host && !request_report.native_handle_access && !probe.native_handle_access &&
                   !mirakana::linux_desktop_host_status_name(probe.status).empty() && manual.ready() &&
                   mirakana::linux_desktop_host_status_name(manual.status) == std::string_view{"ready"} &&
                   presentation.ready() && !presentation.environment_platform_windows_vulkan_inferred &&
                   mirakana::linux_desktop_vulkan_presentation_status_name(presentation.status) ==
                       std::string_view{"ready"}
               ? 0
               : 1;
}
