// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/linux/linux_desktop_game_host.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct LinuxRuntimeOptions {
    bool smoke{false};
    bool show_help{false};
    bool require_linux_vulkan_presentation_smoke{false};
    bool require_linux_vulkan_readback{false};
    bool require_linux_vulkan_validation_log{false};
    bool emit_vulkan_strict_commercial_host_gate{false};
    bool require_vulkan_renderer{false};
    bool require_scene_gpu_bindings{false};
    bool require_postprocess{false};
    bool require_postprocess_depth_input{false};
    bool require_vulkan_postprocess_evidence{false};
    bool require_environment_vulkan_strict_aggregate{false};
    bool require_gpu_memory_policy{false};
    bool require_memory_diagnostics{false};
    bool require_vulkan_gpu_memory_evidence{false};
    bool require_debug_profiling_policy{false};
    bool require_vulkan_debug_profiling_evidence{false};
    std::uint32_t max_frames{0};
    std::string required_config_path;
    std::string required_scene_package_path;

    [[nodiscard]] bool strict_vulkan_commercial_requirements_requested() const noexcept {
        return require_vulkan_renderer || require_scene_gpu_bindings || require_postprocess ||
               require_postprocess_depth_input || require_vulkan_postprocess_evidence ||
               require_environment_vulkan_strict_aggregate || require_gpu_memory_policy || require_memory_diagnostics ||
               require_vulkan_gpu_memory_evidence || require_debug_profiling_policy ||
               require_vulkan_debug_profiling_evidence;
    }

    [[nodiscard]] bool should_emit_strict_vulkan_commercial_host_gate() const noexcept {
        return emit_vulkan_strict_commercial_host_gate || strict_vulkan_commercial_requirements_requested();
    }
};

void print_usage() {
    std::cout << "sample_desktop_runtime_game --smoke [--require-config <path>] "
                 "[--require-scene-package <path>] [--require-linux-vulkan-presentation-smoke] "
                 "[--require-linux-vulkan-readback] [--require-linux-vulkan-validation-log] "
                 "[--emit-vulkan-strict-commercial-host-gate]\n";
}

bool consume_path_argument(const std::vector<std::string_view>& args, std::size_t& index, std::string& output,
                           std::string_view flag) {
    if (index + 1U >= args.size()) {
        std::cerr << flag << " requires a path\n";
        return false;
    }
    ++index;
    output = std::string(args[index]);
    return true;
}

bool consume_u32_argument(const std::vector<std::string_view>& args, std::size_t& index, std::uint32_t& output,
                          std::string_view flag) {
    if (index + 1U >= args.size()) {
        std::cerr << flag << " requires a positive integer\n";
        return false;
    }
    ++index;
    const auto value = args[index];
    std::uint64_t parsed = 0;
    for (const char digit : value) {
        if (digit < '0' || digit > '9') {
            std::cerr << flag << " requires a positive integer\n";
            return false;
        }
        parsed = (parsed * 10U) + static_cast<std::uint64_t>(digit - '0');
        if (parsed > std::numeric_limits<std::uint32_t>::max()) {
            std::cerr << flag << " is too large\n";
            return false;
        }
    }
    if (parsed == 0U) {
        std::cerr << flag << " requires a positive integer\n";
        return false;
    }
    output = static_cast<std::uint32_t>(parsed);
    return true;
}

bool parse_options(int argc, char** argv, LinuxRuntimeOptions& options) {
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    for (std::size_t index = 0; index < args.size(); ++index) {
        const auto arg = args[index];
        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
            continue;
        }
        if (arg == "--smoke") {
            options.smoke = true;
            continue;
        }
        if (arg == "--require-config") {
            if (!consume_path_argument(args, index, options.required_config_path, arg)) {
                return false;
            }
            continue;
        }
        if (arg == "--require-scene-package") {
            if (!consume_path_argument(args, index, options.required_scene_package_path, arg)) {
                return false;
            }
            continue;
        }
        if (arg == "--require-linux-vulkan-presentation-smoke") {
            options.require_linux_vulkan_presentation_smoke = true;
            continue;
        }
        if (arg == "--require-linux-vulkan-readback") {
            options.require_linux_vulkan_readback = true;
            continue;
        }
        if (arg == "--require-linux-vulkan-validation-log") {
            options.require_linux_vulkan_validation_log = true;
            continue;
        }
        if (arg == "--emit-vulkan-strict-commercial-host-gate") {
            options.emit_vulkan_strict_commercial_host_gate = true;
            continue;
        }
        if (arg == "--max-frames") {
            if (!consume_u32_argument(args, index, options.max_frames, arg)) {
                return false;
            }
            continue;
        }
        if (arg == "--require-vulkan-renderer") {
            options.require_vulkan_renderer = true;
            continue;
        }
        if (arg == "--require-scene-gpu-bindings") {
            options.require_scene_gpu_bindings = true;
            continue;
        }
        if (arg == "--require-postprocess") {
            options.require_postprocess = true;
            continue;
        }
        if (arg == "--require-postprocess-depth-input") {
            options.require_postprocess_depth_input = true;
            continue;
        }
        if (arg == "--require-vulkan-postprocess-evidence") {
            options.require_vulkan_postprocess_evidence = true;
            continue;
        }
        if (arg == "--require-environment-vulkan-strict-aggregate") {
            options.require_environment_vulkan_strict_aggregate = true;
            continue;
        }
        if (arg == "--require-gpu-memory-policy") {
            options.require_gpu_memory_policy = true;
            continue;
        }
        if (arg == "--require-memory-diagnostics") {
            options.require_memory_diagnostics = true;
            continue;
        }
        if (arg == "--require-vulkan-gpu-memory-evidence") {
            options.require_vulkan_gpu_memory_evidence = true;
            continue;
        }
        if (arg == "--require-debug-profiling-policy") {
            options.require_debug_profiling_policy = true;
            continue;
        }
        if (arg == "--require-vulkan-debug-profiling-evidence") {
            options.require_vulkan_debug_profiling_evidence = true;
            continue;
        }

        std::cerr << "Unknown argument: " << arg << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] bool required_file_ready(const std::string& path) {
    return path.empty() || std::filesystem::is_regular_file(std::filesystem::path{path});
}

[[nodiscard]] int bit(bool value) noexcept {
    return value ? 1 : 0;
}

void print_vulkan_strict_commercial_host_gate(const LinuxRuntimeOptions& options,
                                              const mirakana::LinuxDesktopVulkanPresentationReport& presentation) {
    const bool strict_requirements_requested = options.strict_vulkan_commercial_requirements_requested();
    const bool linux_vulkan_platform_ready = presentation.ready();

    std::cout << " renderer_vulkan_strict_linux_gate_status=host_evidence_required"
              << " renderer_vulkan_strict_linux_gate_requested=" << bit(strict_requirements_requested)
              << " renderer_vulkan_strict_linux_gate_emit_only="
              << bit(options.emit_vulkan_strict_commercial_host_gate && !strict_requirements_requested)
              << " renderer_vulkan_strict_linux_gate_runtime_counters_ready="
              << bit(presentation.linux_vulkan_strict_counter_evidence_ready)
              << " renderer_vulkan_strict_linux_gate_readback_bytes=" << presentation.vulkan_readback_bytes
              << " environment_vulkan_strict_aggregate_status=host_evidence_required"
              << " environment_vulkan_strict_aggregate_ready=0"
              << " environment_vulkan_strict_aggregate_profile_v2=0"
              << " environment_vulkan_strict_aggregate_selected_backend="
              << (linux_vulkan_platform_ready ? "vulkan" : "not_vulkan")
              << " environment_vulkan_strict_aggregate_postprocess=0"
              << " environment_vulkan_strict_aggregate_fog=0"
              << " environment_vulkan_strict_aggregate_physical_sky=0"
              << " environment_vulkan_strict_aggregate_lighting=0"
              << " environment_vulkan_strict_aggregate_volumetric_fog=0"
              << " environment_vulkan_strict_aggregate_volumetric_cloud=0"
              << " environment_vulkan_strict_aggregate_precipitation=0"
              << " environment_vulkan_strict_aggregate_quality_budget=0"
              << " environment_vulkan_strict_aggregate_feature_rows=0"
              << " environment_vulkan_strict_aggregate_descriptor_set_bindings=0"
              << " environment_vulkan_strict_aggregate_toolchain_ready=0"
              << " environment_vulkan_strict_aggregate_vulkan_sdk_tools_ready=0"
              << " environment_vulkan_strict_aggregate_dxc_spirv_codegen_ready=0"
              << " environment_vulkan_strict_aggregate_spirv_validation_ready=0"
              << " environment_vulkan_strict_aggregate_validation_layers_ready="
              << bit(presentation.vulkan_validation_layer_ready)
              << " environment_vulkan_strict_aggregate_device_features_ready=0"
              << " environment_vulkan_strict_aggregate_toolchain_rows=0"
              << " environment_vulkan_strict_aggregate_missing_toolchain_rows=6"
              << " environment_vulkan_strict_aggregate_missing_validation_layer_rows="
              << bit(!presentation.vulkan_validation_layer_ready)
              << " environment_vulkan_strict_aggregate_missing_spirv_validation_rows=1"
              << " environment_vulkan_strict_aggregate_unsupported_feature_device_rows=0"
              << " environment_vulkan_strict_aggregate_synchronization2_barriers="
              << presentation.vulkan_synchronization2_barriers
              << " environment_vulkan_strict_aggregate_resource_usage_layout_ready=0"
              << " environment_vulkan_strict_aggregate_resource_usage_layout_rows=0"
              << " environment_vulkan_strict_aggregate_attachment_usage_layout_rows=0"
              << " environment_vulkan_strict_aggregate_sampled_texture_usage_layout_rows=0"
              << " environment_vulkan_strict_aggregate_storage_buffer_usage_layout_rows=0"
              << " environment_vulkan_strict_aggregate_cube_map_usage_layout_rows=0"
              << " environment_vulkan_strict_aggregate_weather_texture_usage_layout_rows=0"
              << " environment_vulkan_strict_aggregate_froxel_buffer_usage_layout_rows=0"
              << " environment_vulkan_strict_aggregate_readback_resource_usage_layout_rows="
              << bit(presentation.vulkan_readback_bytes > 0U) << " environment_vulkan_strict_aggregate_renderer_draws=0"
              << " environment_vulkan_strict_aggregate_compute_dispatches=0"
              << " environment_vulkan_strict_aggregate_texture_uploads=0"
              << " environment_vulkan_strict_aggregate_readback_rows=" << bit(presentation.vulkan_readback_bytes > 0U)
              << " environment_vulkan_strict_aggregate_native_handle_access=0"
              << " environment_vulkan_strict_aggregate_d3d12_fallback=0"
              << " environment_vulkan_strict_aggregate_metal_fallback=0"
              << " environment_vulkan_strict_aggregate_backend_parity=0"
              << " environment_vulkan_strict_aggregate_broad_optimization_claimed=0"
              << " environment_vulkan_strict_aggregate_diagnostics=1"
              << " vulkan_gpu_memory_execution_status=host_evidence_required"
              << " vulkan_gpu_memory_execution_ready=0"
              << " vulkan_gpu_memory_execution_selected=" << bit(linux_vulkan_platform_ready)
              << " vulkan_gpu_memory_execution_committed_byte_estimate_available=0"
              << " vulkan_gpu_memory_execution_committed_resources_byte_estimate=0"
              << " vulkan_gpu_memory_execution_upload_bytes_written=0"
              << " vulkan_gpu_memory_execution_framegraph_barrier_steps_executed=0"
              << " vulkan_gpu_memory_execution_budget_ok=0"
              << " vulkan_gpu_memory_execution_transient_heap_ok=0"
              << " debug_profiling_policy_status=host_evidence_required"
              << " debug_profiling_policy_ready=0"
              << " debug_profiling_policy_backend_profiling_evidence_required=1"
              << " debug_profiling_policy_backend_profiling_evidence_ready=0"
              << " debug_profiling_policy_gpu_timestamp_ticks_per_second=0"
              << " debug_profiling_policy_gpu_timestamp_requests=1"
              << " vulkan_debug_profiling_execution_status=host_evidence_required"
              << " vulkan_debug_profiling_execution_ready=0"
              << " vulkan_debug_profiling_execution_selected=" << bit(linux_vulkan_platform_ready)
              << " vulkan_debug_profiling_execution_gpu_timestamp_ticks_per_second=0"
              << " vulkan_debug_profiling_execution_gpu_timestamps_ok=0"
              << " vulkan_debug_profiling_execution_gpu_debug_markers_ok=0"
              << " vulkan_debug_profiling_execution_frame_diagnostics_ok=0"
              << " vulkan_debug_profiling_execution_framegraph_barrier_steps_executed=0"
              << " renderer_vulkan_timestamp_ready=0"
              << " renderer_commercial_readiness=0";
}

} // namespace

int main(int argc, char** argv) {
    LinuxRuntimeOptions options;
    if (!parse_options(argc, argv, options)) {
        print_usage();
        return 2;
    }
    if (options.show_help || !options.smoke) {
        print_usage();
        return options.show_help ? 0 : 2;
    }

    const bool config_ready = required_file_ready(options.required_config_path);
    const bool scene_package_ready = required_file_ready(options.required_scene_package_path);

    const auto presentation =
        mirakana::probe_linux_desktop_vulkan_presentation(mirakana::LinuxDesktopVulkanPresentationProbeDesc{
            .title = "MIRAIKANAI Linux Vulkan Package Smoke",
            .extent = mirakana::WindowExtent{.width = 64, .height = 64},
            .display_name = nullptr,
            .execute_runtime_smoke = true,
        });

    std::cout << "sample_desktop_runtime_game status="
              << mirakana::linux_desktop_vulkan_presentation_status_name(presentation.status)
              << " config_ready=" << bit(config_ready) << " scene_package_ready=" << bit(scene_package_ready)
              << " linux_package_smoke_ready=" << bit(presentation.linux_package_smoke_ready)
              << " linux_vulkan_readback_ready=" << bit(presentation.linux_vulkan_readback_ready)
              << " linux_vulkan_validation_log_clean=" << bit(presentation.linux_vulkan_validation_log_clean)
              << " environment_platform_linux_vulkan_ready="
              << bit(presentation.environment_platform_linux_vulkan_ready)
              << " environment_platform_windows_vulkan_inferred=0"
              << " VK_LAYER_KHRONOS_validation_ready=" << bit(presentation.linux_vulkan_validation_log_clean)
              << " native_handle_access=0";
    if (options.should_emit_strict_vulkan_commercial_host_gate()) {
        print_vulkan_strict_commercial_host_gate(options, presentation);
    }
    std::cout << '\n';
    if (!presentation.ready() && !presentation.diagnostic.empty()) {
        std::cerr << "sample_desktop_runtime_game diagnostic: " << presentation.diagnostic << '\n';
    }

    if (!config_ready || !scene_package_ready) {
        return 1;
    }
    if (options.require_linux_vulkan_presentation_smoke && !presentation.linux_package_smoke_ready) {
        return 1;
    }
    if (options.require_linux_vulkan_readback && !presentation.linux_vulkan_readback_ready) {
        return 1;
    }
    if (options.require_linux_vulkan_validation_log && !presentation.linux_vulkan_validation_log_clean) {
        return 1;
    }
    if (options.strict_vulkan_commercial_requirements_requested()) {
        std::cerr << "sample_desktop_runtime_game diagnostic: Linux strict Vulkan commercial evidence requires "
                     "aggregate, GPU memory, and debug profiling execution rows that are not implemented yet\n";
        return 1;
    }
    return 0;
}
