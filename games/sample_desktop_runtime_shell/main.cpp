// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/application.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

namespace {

struct RuntimeShellOptions {
    bool smoke{false};
    bool show_help{false};
    bool throttle{true};
    bool require_d3d12_shaders{false};
    bool require_d3d12_renderer{false};
    bool require_vulkan_shaders{false};
    bool require_vulkan_renderer{false};
    std::uint32_t max_frames{0};
    std::string video_driver_hint;
};

constexpr std::string_view kRuntimeShellVertexShaderPath{"shaders/runtime_shell.vs.dxil"};
constexpr std::string_view kRuntimeShellFragmentShaderPath{"shaders/runtime_shell.ps.dxil"};
constexpr std::string_view kRuntimeShellVulkanVertexShaderPath{"shaders/runtime_shell.vs.spv"};
constexpr std::string_view kRuntimeShellVulkanFragmentShaderPath{"shaders/runtime_shell.ps.spv"};

class SampleDesktopRuntimeShellGame final : public mirakana::GameApp {
  public:
    SampleDesktopRuntimeShellGame(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle)
        : input_(input), renderer_(renderer), throttle_(throttle) {}

    void on_start(mirakana::EngineContext&) override {
        renderer_.set_clear_color(mirakana::Color{0.03F, 0.04F, 0.05F, 1.0F});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        renderer_.begin_frame();

        const auto axis =
            input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
        transform_.position = transform_.position + axis;
        renderer_.draw_sprite(mirakana::SpriteCommand{transform_, mirakana::Color{0.2F, 0.7F, 1.0F, 1.0F}});

        renderer_.end_frame();
        ++frames_;

        if (throttle_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        return !input_.key_down(mirakana::Key::escape);
    }

    [[nodiscard]] int frames() const noexcept {
        return frames_;
    }

  private:
    mirakana::VirtualInput& input_;
    mirakana::IRenderer& renderer_;
    mirakana::Transform2D transform_;
    bool throttle_{true};
    int frames_{0};
};

[[nodiscard]] bool parse_positive_uint32(std::string_view text, std::uint32_t& value) noexcept {
    std::uint32_t parsed{};
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end || parsed == 0) {
        return false;
    }
    value = parsed;
    return true;
}

void print_usage() {
    std::cout << "sample_desktop_runtime_shell [--smoke] [--max-frames N] [--video-driver NAME] "
                 "[--require-d3d12-shaders] [--require-d3d12-renderer] "
                 "[--require-vulkan-shaders] [--require-vulkan-renderer]\n";
}

[[nodiscard]] bool parse_args(int argc, char** argv, RuntimeShellOptions& options) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view arg{argv[index]};
        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
            return true;
        }
        if (arg == "--smoke") {
            options.smoke = true;
            continue;
        }
        if (arg == "--require-d3d12-shaders") {
            options.require_d3d12_shaders = true;
            continue;
        }
        if (arg == "--require-d3d12-renderer") {
            options.require_d3d12_renderer = true;
            continue;
        }
        if (arg == "--require-vulkan-shaders") {
            options.require_vulkan_shaders = true;
            continue;
        }
        if (arg == "--require-vulkan-renderer") {
            options.require_vulkan_renderer = true;
            options.require_vulkan_shaders = true;
            continue;
        }
        if (arg == "--max-frames") {
            if (index + 1 >= argc || !parse_positive_uint32(argv[index + 1], options.max_frames)) {
                std::cerr << "--max-frames requires a positive integer\n";
                return false;
            }
            ++index;
            continue;
        }
        if (arg == "--video-driver") {
            if (index + 1 >= argc) {
                std::cerr << "--video-driver requires a driver name\n";
                return false;
            }
            options.video_driver_hint = argv[index + 1];
            ++index;
            continue;
        }

        std::cerr << "unknown argument: " << arg << '\n';
        return false;
    }

    if (options.smoke) {
        if (options.max_frames == 0) {
            options.max_frames = 2;
        }
        if (options.video_driver_hint.empty()) {
            options.video_driver_hint = "dummy";
        }
        options.throttle = false;
    }
    return true;
}

[[nodiscard]] std::filesystem::path executable_directory(const char* executable_path) {
    try {
        if (executable_path != nullptr && std::string_view{executable_path}.size() > 0) {
            const auto absolute_path = std::filesystem::absolute(std::filesystem::path{executable_path});
            if (absolute_path.has_parent_path()) {
                return absolute_path.parent_path();
            }
        }
        return std::filesystem::current_path();
    } catch (const std::exception&) {
        return std::filesystem::path{"."};
    }
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeShellVertexShaderPath},
        .fragment_path = std::string{kRuntimeShellFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_vulkan_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeShellVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeShellVulkanFragmentShaderPath},
    });
}

[[nodiscard]] std::string_view status_name(mirakana::DesktopRunStatus status) noexcept {
    switch (status) {
    case mirakana::DesktopRunStatus::completed:
        return "completed";
    case mirakana::DesktopRunStatus::stopped_by_app:
        return "stopped_by_app";
    case mirakana::DesktopRunStatus::window_closed:
        return "window_closed";
    case mirakana::DesktopRunStatus::lifecycle_quit:
        return "lifecycle_quit";
    }
    return "unknown";
}

void print_presentation_report(std::string_view prefix, const mirakana::SdlDesktopGameHost& host) {
    const auto report = host.presentation_report();
    std::cout << prefix << " presentation_report=requested="
              << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
              << " selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " fallback=" << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " diagnostics=" << report.diagnostics_count << " backend_reports=" << report.backend_reports_count
              << " scene_gpu_status="
              << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " renderer_frames_finished=" << report.renderer_stats.frames_finished << '\n';
    for (const auto& backend_report : host.presentation_backend_reports()) {
        std::cout << prefix << " presentation_backend_report="
                  << mirakana::sdl_desktop_presentation_backend_name(backend_report.backend) << ":"
                  << mirakana::sdl_desktop_presentation_backend_report_status_name(backend_report.status) << ":"
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(backend_report.fallback_reason) << ": "
                  << backend_report.message << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    RuntimeShellOptions options;
    if (!parse_args(argc, argv, options)) {
        print_usage();
        return 2;
    }
    if (options.show_help) {
        print_usage();
        return 0;
    }

    auto shader_bytecode = load_packaged_d3d12_shaders(argc > 0 ? argv[0] : nullptr);
    if (!shader_bytecode.ready()) {
        std::cout << "sample_desktop_runtime_shell shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(shader_bytecode.status) << ": "
                  << shader_bytecode.diagnostic << '\n';
        if (options.require_d3d12_shaders) {
            return 4;
        }
    }

    auto vulkan_shader_bytecode = load_packaged_vulkan_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shader_bytecode.ready()) {
        if (options.require_vulkan_shaders) {
            std::cout << "sample_desktop_runtime_shell vulkan_shader_diagnostic="
                      << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shader_bytecode.status) << ": "
                      << vulkan_shader_bytecode.diagnostic << '\n';
            return 6;
        }
    }

    std::optional<mirakana::SdlDesktopPresentationD3d12RendererDesc> d3d12_renderer;
    if (shader_bytecode.ready()) {
        d3d12_renderer.emplace(mirakana::SdlDesktopPresentationD3d12RendererDesc{
            .vertex_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = shader_bytecode.vertex_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{shader_bytecode.vertex_shader.bytecode.data(),
                                                              shader_bytecode.vertex_shader.bytecode.size()},
                },
            .fragment_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = shader_bytecode.fragment_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{shader_bytecode.fragment_shader.bytecode.data(),
                                                              shader_bytecode.fragment_shader.bytecode.size()},
                },
        });
    }

    std::optional<mirakana::SdlDesktopPresentationVulkanRendererDesc> vulkan_renderer;
    if (vulkan_shader_bytecode.ready()) {
        vulkan_renderer.emplace(mirakana::SdlDesktopPresentationVulkanRendererDesc{
            .vertex_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = vulkan_shader_bytecode.vertex_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{vulkan_shader_bytecode.vertex_shader.bytecode.data(),
                                                              vulkan_shader_bytecode.vertex_shader.bytecode.size()},
                },
            .fragment_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = vulkan_shader_bytecode.fragment_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{vulkan_shader_bytecode.fragment_shader.bytecode.data(),
                                                              vulkan_shader_bytecode.fragment_shader.bytecode.size()},
                },
        });
    }

    mirakana::SdlDesktopGameHostDesc host_desc{
        .title = "Sample Desktop Runtime Shell",
        .extent = mirakana::WindowExtent{960, 540},
        .video_driver_hint = options.video_driver_hint,
        .prefer_vulkan = options.require_vulkan_renderer,
    };
    if (d3d12_renderer.has_value()) {
        host_desc.d3d12_renderer = &*d3d12_renderer;
    }
    if (vulkan_renderer.has_value()) {
        host_desc.vulkan_renderer = &*vulkan_renderer;
    }

    mirakana::SdlDesktopGameHost host(host_desc);
    if (options.require_d3d12_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::d3d12) {
        std::cout << "sample_desktop_runtime_shell required_d3d12_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_desktop_runtime_shell", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_shell presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        return 5;
    }
    if (options.require_vulkan_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::vulkan) {
        std::cout << "sample_desktop_runtime_shell required_vulkan_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_desktop_runtime_shell", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_shell presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        return 7;
    }

    mirakana::IRenderer& renderer = host.renderer();
    SampleDesktopRuntimeShellGame game(host.input(), renderer, options.throttle);
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();

    std::cout << "sample_desktop_runtime_shell status=" << status_name(result.status)
              << " renderer=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_requested=" << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
              << " presentation_selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_fallback="
              << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " presentation_backend_reports=" << report.backend_reports_count
              << " presentation_diagnostics=" << report.diagnostics_count << " scene_gpu_status="
              << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " frames=" << result.frames_run << " game_frames=" << game.frames() << '\n';
    print_presentation_report("sample_desktop_runtime_shell", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "sample_desktop_runtime_shell presentation_diagnostic="
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                  << diagnostic.message << '\n';
    }

    if (options.smoke &&
        (result.status != mirakana::DesktopRunStatus::completed || result.frames_run != options.max_frames ||
         game.frames() != static_cast<int>(options.max_frames))) {
        return 3;
    }
    return 0;
}
