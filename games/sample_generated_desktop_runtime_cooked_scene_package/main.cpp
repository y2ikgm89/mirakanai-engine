// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

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
#include <utility>
#include <vector>

namespace {

struct DesktopRuntimeOptions {
    bool smoke{false};
    bool show_help{false};
    bool throttle{true};
    bool require_d3d12_scene_shaders{false};
    bool require_vulkan_scene_shaders{false};
    bool require_d3d12_renderer{false};
    bool require_vulkan_renderer{false};
    bool require_scene_gpu_bindings{false};
    bool require_postprocess{false};
    std::uint32_t max_frames{0};
    std::string video_driver_hint;
    std::string required_config_path;
    std::string required_scene_package_path;
};

constexpr std::string_view kExpectedConfigFormat{
    "format=GameEngine.GeneratedDesktopRuntimeCookedScenePackage.Config.v1"};
constexpr std::string_view kRuntimeSceneVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_cooked_scene_package_scene.vs.dxil"};
constexpr std::string_view kRuntimeSceneFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_cooked_scene_package_scene.ps.dxil"};
constexpr std::string_view kRuntimeSceneVulkanVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_cooked_scene_package_scene.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_cooked_scene_package_scene.ps.spv"};
constexpr std::string_view kRuntimePostprocessVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_cooked_scene_package_postprocess.vs.dxil"};
constexpr std::string_view kRuntimePostprocessFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_cooked_scene_package_postprocess.ps.dxil"};
constexpr std::string_view kRuntimePostprocessVulkanVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_cooked_scene_package_postprocess.vs.spv"};
constexpr std::string_view kRuntimePostprocessVulkanFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_cooked_scene_package_postprocess.ps.spv"};
constexpr std::uint32_t kRuntimeScenePositionNormalUvStrideBytes{32};

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-cooked-scene-package/scenes/packaged-scene");
}

[[nodiscard]] std::vector<mirakana::rhi::VertexBufferLayoutDesc> runtime_scene_vertex_buffers() {
    return {mirakana::rhi::VertexBufferLayoutDesc{
        .binding = 0,
        .stride = kRuntimeScenePositionNormalUvStrideBytes,
        .input_rate = mirakana::rhi::VertexInputRate::vertex,
    }};
}

[[nodiscard]] std::vector<mirakana::rhi::VertexAttributeDesc> runtime_scene_vertex_attributes() {
    return {
        mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
        },
        mirakana::rhi::VertexAttributeDesc{
            .location = 1,
            .binding = 0,
            .offset = 12,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::normal,
        },
        mirakana::rhi::VertexAttributeDesc{
            .location = 2,
            .binding = 0,
            .offset = 24,
            .format = mirakana::rhi::VertexFormat::float32x2,
            .semantic = mirakana::rhi::VertexSemantic::texcoord,
        },
    };
}

class GeneratedDesktopRuntimeCookedSceneGame final : public mirakana::GameApp {
  public:
    GeneratedDesktopRuntimeCookedSceneGame(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle,
                                           std::optional<mirakana::RuntimeSceneRenderInstance> scene)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)) {}

    void on_start(mirakana::EngineContext&) override {
        renderer_.set_clear_color(mirakana::Color{0.025F, 0.035F, 0.045F, 1.0F});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        renderer_.begin_frame();

        if (scene_.has_value()) {
            const auto scene_submit = mirakana::submit_scene_render_packet(
                renderer_, scene_->render_packet,
                mirakana::SceneRenderSubmitDesc{
                    .fallback_mesh_color = mirakana::Color{0.35F, 0.75F, 0.45F, 1.0F},
                    .material_palette = &scene_->material_palette,
                });
            scene_meshes_submitted_ += scene_submit.meshes_submitted;
            scene_materials_resolved_ += scene_submit.material_colors_resolved;
        } else {
            const auto axis =
                input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
            transform_.position = transform_.position + axis;
            renderer_.draw_sprite(mirakana::SpriteCommand{transform_, mirakana::Color{0.35F, 0.75F, 0.45F, 1.0F}});
        }

        renderer_.end_frame();
        ++frames_;

        if (throttle_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        return !input_.key_down(mirakana::Key::escape);
    }

    [[nodiscard]] std::uint32_t frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] std::size_t scene_meshes_submitted() const noexcept {
        return scene_meshes_submitted_;
    }

    [[nodiscard]] std::size_t scene_materials_resolved() const noexcept {
        return scene_materials_resolved_;
    }

  private:
    mirakana::VirtualInput& input_;
    mirakana::IRenderer& renderer_;
    mirakana::Transform2D transform_;
    bool throttle_{true};
    std::optional<mirakana::RuntimeSceneRenderInstance> scene_;
    std::uint32_t frames_{0};
    std::size_t scene_meshes_submitted_{0};
    std::size_t scene_materials_resolved_{0};
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
    std::cout
        << "sample_generated_desktop_runtime_cooked_scene_package [--smoke] [--max-frames N] [--video-driver NAME] "
           "[--require-config PATH] [--require-scene-package PATH] [--require-d3d12-scene-shaders] "
           "[--require-vulkan-scene-shaders] [--require-d3d12-renderer] [--require-vulkan-renderer] "
           "[--require-scene-gpu-bindings] [--require-postprocess]\n";
}

[[nodiscard]] bool parse_args(int argc, char** argv, DesktopRuntimeOptions& options) {
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
        if (arg == "--require-d3d12-scene-shaders") {
            options.require_d3d12_scene_shaders = true;
            continue;
        }
        if (arg == "--require-vulkan-scene-shaders") {
            options.require_vulkan_scene_shaders = true;
            continue;
        }
        if (arg == "--require-d3d12-renderer") {
            options.require_d3d12_renderer = true;
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
        if (arg == "--require-config") {
            if (index + 1 >= argc) {
                std::cerr << "--require-config requires a relative path\n";
                return false;
            }
            options.required_config_path = argv[index + 1];
            ++index;
            continue;
        }
        if (arg == "--require-scene-package") {
            if (index + 1 >= argc) {
                std::cerr << "--require-scene-package requires a relative path\n";
                return false;
            }
            options.required_scene_package_path = argv[index + 1];
            ++index;
            continue;
        }

        std::cerr << "unknown argument: " << arg << '\n';
        return false;
    }

    if (options.require_d3d12_renderer && options.require_vulkan_renderer) {
        std::cerr << "--require-d3d12-renderer and --require-vulkan-renderer are mutually exclusive\n";
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

void print_package_failures(const std::vector<mirakana::runtime::RuntimeAssetPackageLoadFailure>& failures) {
    for (const auto& failure : failures) {
        std::cerr << "runtime package failure asset=" << failure.asset.value << " path=" << failure.path << ": "
                  << failure.diagnostic << '\n';
    }
}

void print_scene_failures(const std::vector<mirakana::RuntimeSceneRenderLoadFailure>& failures) {
    for (const auto& failure : failures) {
        std::cerr << "runtime scene failure asset=" << failure.asset.value << ": " << failure.diagnostic << '\n';
    }
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

[[nodiscard]] bool verify_required_config(const char* executable_path, std::string_view config_path) {
    if (config_path.empty()) {
        return true;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        if (!filesystem.exists(config_path)) {
            std::cerr << "required config was not found: " << config_path << '\n';
            return false;
        }

        const auto config_text = filesystem.read_text(config_path);
        if (config_text.empty()) {
            std::cerr << "required config is empty: " << config_path << '\n';
            return false;
        }
        if (config_text.rfind(kExpectedConfigFormat, 0) != 0) {
            std::cerr << "required config has unexpected format: " << config_path << '\n';
            return false;
        }
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required config '" << config_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] bool load_required_scene_package(const char* executable_path, std::string_view package_path,
                                               std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package,
                                               std::optional<mirakana::RuntimeSceneRenderInstance>& scene) {
    if (package_path.empty()) {
        return true;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        if (!filesystem.exists(package_path)) {
            std::cerr << "required scene package was not found: " << package_path << '\n';
            return false;
        }

        auto package_result =
            mirakana::runtime::load_runtime_asset_package(filesystem, mirakana::runtime::RuntimeAssetPackageDesc{
                                                                          .index_path = std::string{package_path},
                                                                          .content_root = {},
                                                                      });
        if (!package_result.succeeded()) {
            print_package_failures(package_result.failures);
            return false;
        }

        auto instance =
            mirakana::instantiate_runtime_scene_render_data(package_result.package, packaged_scene_asset_id());
        if (!instance.succeeded()) {
            print_scene_failures(instance.failures);
            return false;
        }
        if (instance.render_packet.meshes.empty()) {
            std::cerr << "runtime scene package did not produce renderable meshes: " << package_path << '\n';
            return false;
        }
        if (instance.material_palette.count() == 0) {
            std::cerr << "runtime scene package did not resolve scene materials: " << package_path << '\n';
            return false;
        }

        runtime_package = std::move(package_result.package);
        scene = std::move(instance);
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required scene package '" << package_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_postprocess_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimePostprocessVertexShaderPath},
        .fragment_path = std::string{kRuntimePostprocessFragmentShaderPath},
        .vertex_entry_point = "vs_postprocess",
        .fragment_entry_point = "ps_postprocess",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_postprocess_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimePostprocessVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimePostprocessVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_postprocess",
        .fragment_entry_point = "ps_postprocess",
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
    DesktopRuntimeOptions options;
    if (!parse_args(argc, argv, options)) {
        print_usage();
        return 2;
    }
    if (options.show_help) {
        print_usage();
        return 0;
    }
    if (!verify_required_config(argc > 0 ? argv[0] : nullptr, options.required_config_path)) {
        return 4;
    }

    std::optional<mirakana::runtime::RuntimeAssetPackage> runtime_package;
    std::optional<mirakana::RuntimeSceneRenderInstance> packaged_scene;
    if (!load_required_scene_package(argc > 0 ? argv[0] : nullptr, options.required_scene_package_path, runtime_package,
                                     packaged_scene)) {
        return 4;
    }
    if (options.require_scene_gpu_bindings && (!runtime_package.has_value() || !packaged_scene.has_value())) {
        std::cerr << "--require-scene-gpu-bindings requires --require-scene-package\n";
        return 4;
    }

    auto d3d12_shader_bytecode = load_packaged_d3d12_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_shader_bytecode.ready() && options.require_d3d12_scene_shaders) {
        std::cout << "sample_generated_desktop_runtime_cooked_scene_package shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_shader_bytecode.status) << ": "
                  << d3d12_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_postprocess_bytecode = load_packaged_d3d12_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_postprocess_bytecode.ready() && options.require_postprocess && !options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_cooked_scene_package postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_postprocess_bytecode.status) << ": "
                  << d3d12_postprocess_bytecode.diagnostic << '\n';
        return 4;
    }

    auto vulkan_shader_bytecode = load_packaged_vulkan_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shader_bytecode.ready() && options.require_vulkan_scene_shaders) {
        std::cout << "sample_generated_desktop_runtime_cooked_scene_package vulkan_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shader_bytecode.status) << ": "
                  << vulkan_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_postprocess_bytecode = load_packaged_vulkan_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_postprocess_bytecode.ready() && options.require_postprocess && options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_cooked_scene_package vulkan_postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_postprocess_bytecode.status) << ": "
                  << vulkan_postprocess_bytecode.diagnostic << '\n';
        return 6;
    }

    std::optional<mirakana::SdlDesktopPresentationD3d12SceneRendererDesc> d3d12_scene_renderer;
    if (d3d12_shader_bytecode.ready() && d3d12_postprocess_bytecode.ready() && runtime_package.has_value() &&
        packaged_scene.has_value()) {
        d3d12_scene_renderer.emplace(mirakana::SdlDesktopPresentationD3d12SceneRendererDesc{
            .vertex_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = d3d12_shader_bytecode.vertex_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{d3d12_shader_bytecode.vertex_shader.bytecode.data(),
                                                              d3d12_shader_bytecode.vertex_shader.bytecode.size()},
                },
            .fragment_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = d3d12_shader_bytecode.fragment_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{d3d12_shader_bytecode.fragment_shader.bytecode.data(),
                                                              d3d12_shader_bytecode.fragment_shader.bytecode.size()},
                },
            .postprocess_vertex_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = d3d12_postprocess_bytecode.vertex_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{d3d12_postprocess_bytecode.vertex_shader.bytecode.data(),
                                                              d3d12_postprocess_bytecode.vertex_shader.bytecode.size()},
                },
            .postprocess_fragment_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = d3d12_postprocess_bytecode.fragment_shader.entry_point,
                    .bytecode =
                        std::span<const std::uint8_t>{d3d12_postprocess_bytecode.fragment_shader.bytecode.data(),
                                                      d3d12_postprocess_bytecode.fragment_shader.bytecode.size()},
                },
            .package = &*runtime_package,
            .packet = &packaged_scene->render_packet,
            .vertex_buffers = runtime_scene_vertex_buffers(),
            .vertex_attributes = runtime_scene_vertex_attributes(),
            .enable_postprocess = true,
        });
    }

    std::optional<mirakana::SdlDesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;
    if (vulkan_shader_bytecode.ready() && vulkan_postprocess_bytecode.ready() && runtime_package.has_value() &&
        packaged_scene.has_value()) {
        vulkan_scene_renderer.emplace(mirakana::SdlDesktopPresentationVulkanSceneRendererDesc{
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
            .postprocess_vertex_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = vulkan_postprocess_bytecode.vertex_shader.entry_point,
                    .bytecode =
                        std::span<const std::uint8_t>{vulkan_postprocess_bytecode.vertex_shader.bytecode.data(),
                                                      vulkan_postprocess_bytecode.vertex_shader.bytecode.size()},
                },
            .postprocess_fragment_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = vulkan_postprocess_bytecode.fragment_shader.entry_point,
                    .bytecode =
                        std::span<const std::uint8_t>{vulkan_postprocess_bytecode.fragment_shader.bytecode.data(),
                                                      vulkan_postprocess_bytecode.fragment_shader.bytecode.size()},
                },
            .package = &*runtime_package,
            .packet = &packaged_scene->render_packet,
            .vertex_buffers = runtime_scene_vertex_buffers(),
            .vertex_attributes = runtime_scene_vertex_attributes(),
            .enable_postprocess = true,
        });
    }

    mirakana::SdlDesktopGameHostDesc host_desc{
        .title = "sample-generated-desktop-runtime-cooked-scene-package",
        .extent = mirakana::WindowExtent{960, 540},
        .video_driver_hint = options.video_driver_hint,
        .prefer_vulkan = options.require_vulkan_renderer,
    };
    if (d3d12_scene_renderer.has_value()) {
        host_desc.d3d12_scene_renderer = &*d3d12_scene_renderer;
    }
    if (vulkan_scene_renderer.has_value()) {
        host_desc.vulkan_scene_renderer = &*vulkan_scene_renderer;
    }

    mirakana::SdlDesktopGameHost host(host_desc);
    if (options.require_d3d12_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::d3d12) {
        std::cout
            << "sample_generated_desktop_runtime_cooked_scene_package required_d3d12_renderer_unavailable renderer="
            << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_generated_desktop_runtime_cooked_scene_package", host);
        return 5;
    }
    if (options.require_vulkan_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::vulkan) {
        std::cout
            << "sample_generated_desktop_runtime_cooked_scene_package required_vulkan_renderer_unavailable renderer="
            << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_generated_desktop_runtime_cooked_scene_package", host);
        return 7;
    }
    if (options.require_scene_gpu_bindings && !host.scene_gpu_bindings_ready()) {
        std::cout
            << "sample_generated_desktop_runtime_cooked_scene_package required_scene_gpu_bindings_unavailable status="
            << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(host.scene_gpu_binding_status())
            << '\n';
        print_presentation_report("sample_generated_desktop_runtime_cooked_scene_package", host);
        return 5;
    }
    if (options.require_postprocess &&
        host.presentation_report().postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready) {
        std::cout << "sample_generated_desktop_runtime_cooked_scene_package required_postprocess_unavailable status="
                  << mirakana::sdl_desktop_presentation_postprocess_status_name(
                         host.presentation_report().postprocess_status)
                  << '\n';
        print_presentation_report("sample_generated_desktop_runtime_cooked_scene_package", host);
        return 8;
    }

    GeneratedDesktopRuntimeCookedSceneGame game(host.input(), host.renderer(), options.throttle,
                                                std::move(packaged_scene));
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();
    const auto scene_gpu_stats = report.scene_gpu_stats;

    std::cout << "sample_generated_desktop_runtime_cooked_scene_package status=" << status_name(result.status)
              << " renderer=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_requested=" << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
              << " presentation_selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_fallback="
              << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " presentation_backend_reports=" << report.backend_reports_count
              << " presentation_diagnostics=" << report.diagnostics_count << " scene_gpu_status="
              << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " scene_gpu_mesh_bindings=" << scene_gpu_stats.mesh_bindings
              << " scene_gpu_material_bindings=" << scene_gpu_stats.material_bindings
              << " scene_gpu_mesh_uploads=" << scene_gpu_stats.mesh_uploads
              << " scene_gpu_texture_uploads=" << scene_gpu_stats.texture_uploads
              << " scene_gpu_material_uploads=" << scene_gpu_stats.material_uploads
              << " scene_gpu_material_pipeline_layouts=" << scene_gpu_stats.material_pipeline_layouts
              << " scene_gpu_uploaded_texture_bytes=" << scene_gpu_stats.uploaded_texture_bytes
              << " scene_gpu_uploaded_mesh_bytes=" << scene_gpu_stats.uploaded_mesh_bytes
              << " scene_gpu_uploaded_material_factor_bytes=" << scene_gpu_stats.uploaded_material_factor_bytes
              << " scene_gpu_mesh_resolved=" << scene_gpu_stats.mesh_bindings_resolved
              << " scene_gpu_material_resolved=" << scene_gpu_stats.material_bindings_resolved << " postprocess_status="
              << mirakana::sdl_desktop_presentation_postprocess_status_name(report.postprocess_status)
              << " framegraph_passes=" << report.framegraph_passes << " frames=" << result.frames_run
              << " game_frames=" << game.frames() << " scene_meshes=" << game.scene_meshes_submitted()
              << " scene_materials=" << game.scene_materials_resolved() << '\n';
    print_presentation_report("sample_generated_desktop_runtime_cooked_scene_package", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "sample_generated_desktop_runtime_cooked_scene_package presentation_diagnostic="
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                  << diagnostic.message << '\n';
    }

    if (options.smoke) {
        if (result.status != mirakana::DesktopRunStatus::completed || result.frames_run != options.max_frames ||
            game.frames() != options.max_frames) {
            return 3;
        }
        if (!options.required_scene_package_path.empty() &&
            (game.scene_meshes_submitted() != static_cast<std::size_t>(options.max_frames) ||
             game.scene_materials_resolved() != static_cast<std::size_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_scene_gpu_bindings &&
            (scene_gpu_stats.mesh_bindings == 0 || scene_gpu_stats.material_bindings == 0 ||
             scene_gpu_stats.mesh_bindings_resolved != static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.material_bindings_resolved != static_cast<std::size_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_postprocess &&
            (report.postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready ||
             report.framegraph_passes != 2 ||
             report.renderer_stats.framegraph_passes_executed != static_cast<std::uint64_t>(options.max_frames) * 2U ||
             report.renderer_stats.postprocess_passes_executed != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
    }
    return 0;
}
