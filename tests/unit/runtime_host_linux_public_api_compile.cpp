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
            .validation_layer_ready = true,
            .synchronization2_barriers = 3U,
            .readback_bytes = 64U,
            .strict_aggregate_toolchain_ready = true,
            .strict_aggregate_vulkan_sdk_tools_ready = true,
            .strict_aggregate_dxc_spirv_codegen_ready = true,
            .strict_aggregate_spirv_validation_ready = true,
            .strict_aggregate_device_features_ready = true,
            .strict_aggregate_postprocess_ready = true,
            .strict_aggregate_fog_ready = true,
            .strict_aggregate_physical_sky_ready = true,
            .strict_aggregate_lighting_ready = true,
            .strict_aggregate_volumetric_fog_ready = true,
            .strict_aggregate_volumetric_cloud_ready = true,
            .strict_aggregate_precipitation_ready = true,
            .strict_aggregate_quality_budget_ready = true,
            .strict_aggregate_feature_rows = 6U,
            .strict_aggregate_descriptor_set_bindings = 15U,
            .strict_aggregate_toolchain_rows = 6U,
            .strict_aggregate_resource_usage_layout_rows = 20U,
            .strict_aggregate_attachment_usage_layout_rows = 2U,
            .strict_aggregate_sampled_texture_usage_layout_rows = 6U,
            .strict_aggregate_storage_buffer_usage_layout_rows = 2U,
            .strict_aggregate_cube_map_usage_layout_rows = 1U,
            .strict_aggregate_weather_texture_usage_layout_rows = 3U,
            .strict_aggregate_froxel_buffer_usage_layout_rows = 1U,
            .strict_aggregate_readback_resource_usage_layout_rows = 5U,
            .strict_aggregate_renderer_draws = 2U,
            .strict_aggregate_compute_dispatches = 1U,
            .strict_aggregate_texture_uploads = 3U,
            .strict_aggregate_readback_rows = 5U,
            .strict_aggregate_framegraph_render_passes_recorded = 3U,
            .vulkan_gpu_memory_committed_byte_estimate_available = true,
            .vulkan_gpu_memory_committed_resources_byte_estimate = 4096U,
            .vulkan_gpu_memory_upload_bytes_written = 2048U,
            .vulkan_gpu_memory_framegraph_barrier_steps_executed = 7U,
            .vulkan_gpu_memory_budget_ok = true,
            .vulkan_gpu_memory_transient_heap_ok = true,
            .debug_profiling_gpu_timestamp_ticks_per_second = 1'000'000'000ULL,
            .debug_profiling_gpu_timestamp_query_writes = 2U,
            .debug_profiling_gpu_timestamp_query_results_read = 1U,
            .debug_profiling_gpu_debug_markers_ok = true,
            .debug_profiling_framegraph_barrier_steps_executed = 7U,
            .debug_profiling_framegraph_render_passes_recorded = 3U,
        });

    return request_report.linux_host && !request_report.native_handle_access && !probe.native_handle_access &&
                   !mirakana::linux_desktop_host_status_name(probe.status).empty() && manual.ready() &&
                   mirakana::linux_desktop_host_status_name(manual.status) == std::string_view{"ready"} &&
                   presentation.ready() && !presentation.environment_platform_windows_vulkan_inferred &&
                   presentation.linux_vulkan_strict_counter_evidence_ready &&
                   presentation.vulkan_validation_layer_ready && presentation.vulkan_synchronization2_barriers == 3U &&
                   presentation.vulkan_readback_bytes == 64U && presentation.linux_vulkan_strict_commercial_ready &&
                   presentation.strict_aggregate_resource_usage_layout_ready &&
                   presentation.vulkan_gpu_memory_execution_status ==
                       mirakana::LinuxDesktopVulkanStrictExecutionStatus::ready &&
                   presentation.vulkan_debug_profiling_execution_status ==
                       mirakana::LinuxDesktopVulkanStrictExecutionStatus::ready &&
                   mirakana::linux_desktop_vulkan_strict_execution_status_name(
                       presentation.vulkan_debug_profiling_execution_status) == std::string_view{"ready"} &&
                   presentation.renderer_vulkan_timestamp_ready &&
                   mirakana::linux_desktop_vulkan_presentation_status_name(presentation.status) ==
                       std::string_view{"ready"}
               ? 0
               : 1;
}
