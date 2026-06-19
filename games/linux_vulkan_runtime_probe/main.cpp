// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/linux/linux_platform_probe.hpp"
#include "mirakana/runtime_host/linux/linux_vulkan_runtime_probe.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Options {
    bool require_probe{false};
    std::filesystem::path shader_path;
};

[[nodiscard]] std::filesystem::path default_shader_path(const char* executable_path) {
    const auto executable = std::filesystem::absolute(std::filesystem::path{executable_path});
    return executable.parent_path() / "shaders" / "linux_vulkan_runtime_probe_environment_weather_solver.cs.spv";
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
    Options options{};
    if (argc > 0) {
        options.shader_path = default_shader_path(argv[0]);
    }

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument{argv[index]};
        if (argument == "--require-linux-vulkan-runtime-probe") {
            options.require_probe = true;
        } else if (argument == "--weather-solver-spv" && index + 1 < argc) {
            ++index;
            options.shader_path = std::filesystem::path{argv[index]};
        } else if (argument == "--help") {
            std::cout << "usage: linux_vulkan_runtime_probe [--require-linux-vulkan-runtime-probe] "
                         "[--weather-solver-spv <path>]\n";
            std::exit(0);
        }
    }
    return options;
}

[[nodiscard]] std::vector<std::uint32_t> read_spirv_words(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary | std::ios::ate};
    if (!input) {
        return {};
    }
    const auto size = input.tellg();
    if (size <= 0 || (size % static_cast<std::streamoff>(sizeof(std::uint32_t))) != 0) {
        return {};
    }
    input.seekg(0, std::ios::beg);
    std::vector<std::uint32_t> words(static_cast<std::size_t>(size / sizeof(std::uint32_t)));
    input.read(reinterpret_cast<char*>(words.data()), size);
    if (!input) {
        return {};
    }
    return words;
}

void print_result(const mirakana::LinuxVulkanRuntimeProbeResult& result) {
    const auto surface_family = mirakana::platform::linux::linux_vulkan_surface_family_name(result.surface_family);
    std::cout << "validation_recipe=linux-vulkan-runtime-probe"
              << " linux_vulkan_runtime_probe_status=" << (result.probe_ready ? "ready" : "blocked")
              << " linux_vulkan_runtime_probe_ready=" << (result.probe_ready ? 1 : 0)
              << " linux_vulkan_runtime_readback_ready=" << (result.readback_ready ? 1 : 0)
              << " linux_vulkan_runtime_probe_surface_family=" << surface_family
              << " linux_vulkan_runtime_probe_cells=" << result.cell_count
              << " linux_vulkan_runtime_probe_dispatches=" << result.compute_dispatches
              << " linux_vulkan_runtime_probe_descriptor_set_bindings=" << result.descriptor_set_bindings
              << " linux_vulkan_runtime_probe_barriers=" << result.resource_barriers_recorded
              << " linux_vulkan_runtime_probe_failure_stage=" << result.failure_stage
              << " linux_vulkan_runtime_probe_hash=" << result.output_checksum
              << " first_party_linux_runtime_host_ready=" << (result.first_party_runtime_host_ready ? 1 : 0)
              << " native_handle_access=" << (result.exposes_native_handles ? 1 : 0)
              << " windows_vulkan_inferred=0 android_vulkan_inferred=0"
              << " environment_all_platform_unconditional_ready=0"
              << " environment_commercial_ready=0 environment_ready=0\n";
}

} // namespace

int main(int argc, char** argv) {
    const auto options = parse_options(argc, argv);
    const auto spirv_words = read_spirv_words(options.shader_path);
    auto result = mirakana::run_linux_vulkan_runtime_probe(
        mirakana::LinuxVulkanRuntimeProbeDesc{.weather_solver_spirv = std::span<const std::uint32_t>{spirv_words}});
    print_result(result);
    if (options.require_probe && !result.probe_ready) {
        return 2;
    }
    return 0;
}
