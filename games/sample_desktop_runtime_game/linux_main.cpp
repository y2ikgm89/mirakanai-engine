// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/linux/linux_desktop_game_host.hpp"

#include <filesystem>
#include <iostream>
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
    std::string required_config_path;
    std::string required_scene_package_path;
};

void print_usage() {
    std::cout << "sample_desktop_runtime_game --smoke [--require-config <path>] "
                 "[--require-scene-package <path>] [--require-linux-vulkan-presentation-smoke] "
                 "[--require-linux-vulkan-readback] [--require-linux-vulkan-validation-log]\n";
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
              << " native_handle_access=0\n";

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
    return 0;
}
