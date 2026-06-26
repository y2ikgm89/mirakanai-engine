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

enum class LinuxDesktopVulkanStrictExecutionStatus : std::uint8_t {
    host_evidence_required,
    ready,
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
    bool validation_layer_ready{false};
    std::uint32_t synchronization2_barriers{0};
    std::uint64_t readback_bytes{0};
    bool strict_aggregate_toolchain_ready{false};
    bool strict_aggregate_vulkan_sdk_tools_ready{false};
    bool strict_aggregate_dxc_spirv_codegen_ready{false};
    bool strict_aggregate_spirv_validation_ready{false};
    bool strict_aggregate_device_features_ready{false};
    bool strict_aggregate_postprocess_ready{false};
    bool strict_aggregate_fog_ready{false};
    bool strict_aggregate_physical_sky_ready{false};
    bool strict_aggregate_lighting_ready{false};
    bool strict_aggregate_volumetric_fog_ready{false};
    bool strict_aggregate_volumetric_cloud_ready{false};
    bool strict_aggregate_precipitation_ready{false};
    bool strict_aggregate_quality_budget_ready{false};
    std::uint32_t strict_aggregate_feature_rows{0};
    std::uint32_t strict_aggregate_descriptor_set_bindings{0};
    std::uint32_t strict_aggregate_toolchain_rows{0};
    std::uint32_t strict_aggregate_resource_usage_layout_rows{0};
    std::uint32_t strict_aggregate_attachment_usage_layout_rows{0};
    std::uint32_t strict_aggregate_sampled_texture_usage_layout_rows{0};
    std::uint32_t strict_aggregate_storage_buffer_usage_layout_rows{0};
    std::uint32_t strict_aggregate_cube_map_usage_layout_rows{0};
    std::uint32_t strict_aggregate_weather_texture_usage_layout_rows{0};
    std::uint32_t strict_aggregate_froxel_buffer_usage_layout_rows{0};
    std::uint32_t strict_aggregate_readback_resource_usage_layout_rows{0};
    std::uint32_t strict_aggregate_renderer_draws{0};
    std::uint32_t strict_aggregate_compute_dispatches{0};
    std::uint32_t strict_aggregate_texture_uploads{0};
    std::uint32_t strict_aggregate_readback_rows{0};
    std::uint64_t strict_aggregate_framegraph_render_passes_recorded{0};
    bool vulkan_gpu_memory_committed_byte_estimate_available{false};
    std::uint64_t vulkan_gpu_memory_committed_resources_byte_estimate{0};
    std::uint64_t vulkan_gpu_memory_upload_bytes_written{0};
    std::uint64_t vulkan_gpu_memory_framegraph_barrier_steps_executed{0};
    bool vulkan_gpu_memory_budget_ok{false};
    bool vulkan_gpu_memory_transient_heap_ok{false};
    std::uint64_t debug_profiling_gpu_timestamp_ticks_per_second{0};
    std::uint64_t debug_profiling_gpu_timestamp_query_writes{0};
    std::uint64_t debug_profiling_gpu_timestamp_query_results_read{0};
    std::uint64_t debug_profiling_gpu_timestamp_query_failures{0};
    bool debug_profiling_gpu_debug_markers_ok{false};
    std::uint64_t debug_profiling_framegraph_barrier_steps_executed{0};
    std::uint64_t debug_profiling_framegraph_render_passes_recorded{0};
    bool native_handle_access{false};
};

struct LinuxDesktopVulkanStrictCommercialEvidence {
    bool selected_strict_aggregate_counters_ready{false};
    bool vulkan_sdk_tools_ready{false};
    bool dxc_spirv_codegen_ready{false};
    bool spirv_validation_ready{false};
    bool device_features_ready{false};
    bool committed_byte_estimate_available{false};
    std::uint64_t committed_resources_byte_estimate{0};
    std::uint64_t upload_bytes_written{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    bool memory_budget_ok{false};
    bool transient_heap_ok{false};
    std::uint64_t gpu_timestamp_ticks_per_second{0};
    std::uint64_t gpu_timestamp_query_writes{0};
    std::uint64_t gpu_timestamp_query_results_read{0};
    std::uint64_t gpu_timestamp_query_failures{0};
    bool gpu_debug_markers_ok{false};
    std::uint64_t framegraph_render_passes_recorded{0};
};

struct LinuxDesktopVulkanPresentationProbeDesc {
    std::string title{"MIRAIKANAI Linux Vulkan Smoke"};
    WindowExtent extent{.width = 64, .height = 64};
    const char* display_name{nullptr};
    bool execute_runtime_smoke{true};
    bool collect_strict_commercial_evidence{false};
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
    bool linux_vulkan_strict_counter_evidence_ready{false};
    bool vulkan_validation_layer_ready{false};
    std::uint32_t vulkan_synchronization2_barriers{0};
    std::uint64_t vulkan_readback_bytes{0};
    bool environment_vulkan_strict_aggregate_ready{false};
    bool strict_aggregate_toolchain_ready{false};
    bool strict_aggregate_vulkan_sdk_tools_ready{false};
    bool strict_aggregate_dxc_spirv_codegen_ready{false};
    bool strict_aggregate_spirv_validation_ready{false};
    bool strict_aggregate_device_features_ready{false};
    bool strict_aggregate_postprocess_ready{false};
    bool strict_aggregate_fog_ready{false};
    bool strict_aggregate_physical_sky_ready{false};
    bool strict_aggregate_lighting_ready{false};
    bool strict_aggregate_volumetric_fog_ready{false};
    bool strict_aggregate_volumetric_cloud_ready{false};
    bool strict_aggregate_precipitation_ready{false};
    bool strict_aggregate_quality_budget_ready{false};
    std::uint32_t strict_aggregate_feature_rows{0};
    std::uint32_t strict_aggregate_descriptor_set_bindings{0};
    std::uint32_t strict_aggregate_toolchain_rows{0};
    bool strict_aggregate_resource_usage_layout_ready{false};
    std::uint32_t strict_aggregate_resource_usage_layout_rows{0};
    std::uint32_t strict_aggregate_attachment_usage_layout_rows{0};
    std::uint32_t strict_aggregate_sampled_texture_usage_layout_rows{0};
    std::uint32_t strict_aggregate_storage_buffer_usage_layout_rows{0};
    std::uint32_t strict_aggregate_cube_map_usage_layout_rows{0};
    std::uint32_t strict_aggregate_weather_texture_usage_layout_rows{0};
    std::uint32_t strict_aggregate_froxel_buffer_usage_layout_rows{0};
    std::uint32_t strict_aggregate_readback_resource_usage_layout_rows{0};
    std::uint32_t strict_aggregate_renderer_draws{0};
    std::uint32_t strict_aggregate_compute_dispatches{0};
    std::uint32_t strict_aggregate_texture_uploads{0};
    std::uint32_t strict_aggregate_readback_rows{0};
    std::uint64_t strict_aggregate_framegraph_render_passes_recorded{0};
    LinuxDesktopVulkanStrictExecutionStatus vulkan_gpu_memory_execution_status{
        LinuxDesktopVulkanStrictExecutionStatus::host_evidence_required};
    bool vulkan_gpu_memory_execution_ready{false};
    bool vulkan_gpu_memory_committed_byte_estimate_available{false};
    std::uint64_t vulkan_gpu_memory_committed_resources_byte_estimate{0};
    std::uint64_t vulkan_gpu_memory_upload_bytes_written{0};
    std::uint64_t vulkan_gpu_memory_framegraph_barrier_steps_executed{0};
    bool vulkan_gpu_memory_budget_ok{false};
    bool vulkan_gpu_memory_transient_heap_ok{false};
    bool debug_profiling_policy_ready{false};
    bool debug_profiling_policy_backend_evidence_ready{false};
    std::uint32_t debug_profiling_policy_gpu_timestamp_requests{0};
    LinuxDesktopVulkanStrictExecutionStatus vulkan_debug_profiling_execution_status{
        LinuxDesktopVulkanStrictExecutionStatus::host_evidence_required};
    bool vulkan_debug_profiling_execution_ready{false};
    std::uint64_t vulkan_debug_profiling_gpu_timestamp_ticks_per_second{0};
    std::uint64_t vulkan_debug_profiling_gpu_timestamp_query_writes{0};
    std::uint64_t vulkan_debug_profiling_gpu_timestamp_query_results_read{0};
    std::uint64_t vulkan_debug_profiling_gpu_timestamp_query_failures{0};
    bool vulkan_debug_profiling_gpu_timestamps_ok{false};
    bool vulkan_debug_profiling_gpu_debug_markers_ok{false};
    bool vulkan_debug_profiling_frame_diagnostics_ok{false};
    std::uint64_t vulkan_debug_profiling_framegraph_barrier_steps_executed{0};
    std::uint64_t vulkan_debug_profiling_framegraph_render_passes_recorded{0};
    bool renderer_vulkan_timestamp_ready{false};
    bool linux_vulkan_strict_commercial_ready{false};
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
[[nodiscard]] std::string_view
linux_desktop_vulkan_strict_execution_status_name(LinuxDesktopVulkanStrictExecutionStatus status) noexcept;
[[nodiscard]] LinuxDesktopHostReadinessReport
evaluate_linux_desktop_host_request(const LinuxDesktopHostRequest& request);
[[nodiscard]] LinuxDesktopVulkanPresentationReport
evaluate_linux_desktop_vulkan_presentation_request(const LinuxDesktopVulkanPresentationRequest& request);
[[nodiscard]] LinuxDesktopVulkanPresentationRequest with_linux_desktop_vulkan_strict_commercial_evidence(
    LinuxDesktopVulkanPresentationRequest request, const LinuxDesktopVulkanStrictCommercialEvidence& evidence) noexcept;
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
