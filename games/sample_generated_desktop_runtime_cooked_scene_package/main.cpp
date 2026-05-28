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
#include "mirakana/runtime_host/win32/win32_desktop_game_host.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
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
    std::uint32_t max_frames{0};
    std::string required_config_path;
    std::string required_scene_package_path;
};

constexpr std::string_view kExpectedConfigFormat{
    "format=GameEngine.GeneratedDesktopRuntimeCookedScenePackage.Config.v1"};

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-cooked-scene-package/scenes/packaged-scene");
}

class GeneratedDesktopRuntimeCookedSceneGame final : public mirakana::GameApp {
  public:
    GeneratedDesktopRuntimeCookedSceneGame(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle,
                                           std::optional<mirakana::RuntimeSceneRenderInstance> scene)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)) {}

    void on_start(mirakana::EngineContext&) override {
        renderer_.set_clear_color(mirakana::Color{.r = 0.025F, .g = 0.035F, .b = 0.045F, .a = 1.0F});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        renderer_.begin_frame();

        if (scene_.has_value()) {
            const auto scene_submit = mirakana::submit_scene_render_packet(
                renderer_, scene_->render_packet,
                mirakana::SceneRenderSubmitDesc{
                    .fallback_mesh_color = mirakana::Color{.r = 0.35F, .g = 0.75F, .b = 0.45F, .a = 1.0F},
                    .material_palette = &scene_->material_palette,
                });
            scene_meshes_submitted_ += scene_submit.meshes_submitted;
            scene_materials_resolved_ += scene_submit.material_colors_resolved;
        } else {
            const auto axis =
                input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
            transform_.position = transform_.position + axis;
            renderer_.draw_sprite(mirakana::SpriteCommand{
                .transform = transform_, .color = mirakana::Color{.r = 0.35F, .g = 0.75F, .b = 0.45F, .a = 1.0F}});
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

[[nodiscard]] bool parse_cooked_scene_positive_uint32(std::string_view text, std::uint32_t& value) noexcept {
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

void print_cooked_scene_usage() {
    std::cout << "sample_generated_desktop_runtime_cooked_scene_package [--smoke] [--max-frames N] "
                 "[--require-config PATH] [--require-scene-package PATH]\n";
}

[[nodiscard]] bool parse_cooked_scene_args(int argc, char** argv, DesktopRuntimeOptions& options) {
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
        if (arg == "--max-frames") {
            if (index + 1 >= argc || !parse_cooked_scene_positive_uint32(argv[index + 1], options.max_frames)) {
                std::cerr << "--max-frames requires a positive integer\n";
                return false;
            }
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

    if (options.smoke) {
        if (options.max_frames == 0) {
            options.max_frames = 2;
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

[[nodiscard]] std::filesystem::path cooked_scene_executable_directory(const char* executable_path) {
    try {
        if (executable_path != nullptr && !std::string_view{executable_path}.empty()) {
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

[[nodiscard]] bool verify_cooked_scene_required_config(const char* executable_path, std::string_view config_path) {
    if (config_path.empty()) {
        return true;
    }

    try {
        mirakana::RootedFileSystem filesystem(cooked_scene_executable_directory(executable_path));
        if (!filesystem.exists(config_path)) {
            std::cerr << "required config was not found: " << config_path << '\n';
            return false;
        }

        const auto config_text = filesystem.read_text(config_path);
        if (config_text.empty()) {
            std::cerr << "required config is empty: " << config_path << '\n';
            return false;
        }
        if (!config_text.starts_with(kExpectedConfigFormat)) {
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
        mirakana::RootedFileSystem filesystem(cooked_scene_executable_directory(executable_path));
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

[[nodiscard]] std::string_view cooked_scene_status_name(mirakana::DesktopRunStatus status) noexcept {
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

void print_presentation_report(std::string_view prefix, const mirakana::Win32DesktopGameHost& host) {
    const auto report = host.presentation_report();
    std::cout << prefix << " presentation_report=requested="
              << mirakana::win32_desktop_presentation_backend_name(report.requested_backend)
              << " selected=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
              << " fallback=" << mirakana::win32_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " diagnostics=" << report.diagnostics_count << " backend_reports=" << report.backend_reports_count
              << " present_status=" << mirakana::win32_desktop_presentation_present_status_name(report.present_status)
              << " resize_status=" << mirakana::win32_desktop_presentation_resize_status_name(report.resize_status)
              << " renderer_frames_finished=" << report.renderer_stats.frames_finished << '\n';
    for (const auto& backend_report : host.presentation_backend_reports()) {
        std::cout << prefix << " presentation_backend_report="
                  << mirakana::win32_desktop_presentation_backend_name(backend_report.backend) << ":"
                  << mirakana::win32_desktop_presentation_backend_report_status_name(backend_report.status) << ":"
                  << mirakana::win32_desktop_presentation_fallback_reason_name(backend_report.fallback_reason) << ": "
                  << backend_report.diagnostic << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    DesktopRuntimeOptions options;
    if (!parse_cooked_scene_args(argc, argv, options)) {
        print_cooked_scene_usage();
        return 2;
    }
    if (options.show_help) {
        print_cooked_scene_usage();
        return 0;
    }
    if (!verify_cooked_scene_required_config(argc > 0 ? argv[0] : nullptr, options.required_config_path)) {
        return 4;
    }

    std::optional<mirakana::runtime::RuntimeAssetPackage> runtime_package;
    std::optional<mirakana::RuntimeSceneRenderInstance> packaged_scene;
    if (!load_required_scene_package(argc > 0 ? argv[0] : nullptr, options.required_scene_package_path, runtime_package,
                                     packaged_scene)) {
        return 4;
    }
    mirakana::Win32DesktopGameHostDesc host_desc{
        .title = "sample-generated-desktop-runtime-cooked-scene-package",
        .extent = mirakana::WindowExtent{.width = 960, .height = 540},
        .prefer_d3d12 = false,
    };

    mirakana::Win32DesktopGameHost host(host_desc);
    GeneratedDesktopRuntimeCookedSceneGame game(host.input(), host.renderer(), options.throttle,
                                                std::move(packaged_scene));
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();

    std::cout << "sample_generated_desktop_runtime_cooked_scene_package status="
              << cooked_scene_status_name(result.status)
              << " renderer=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_requested="
              << mirakana::win32_desktop_presentation_backend_name(report.requested_backend)
              << " presentation_selected=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_fallback="
              << mirakana::win32_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " presentation_backend_reports=" << report.backend_reports_count
              << " presentation_diagnostics=" << report.diagnostics_count << " presentation_present_status="
              << mirakana::win32_desktop_presentation_present_status_name(report.present_status)
              << " presentation_resize_status="
              << mirakana::win32_desktop_presentation_resize_status_name(report.resize_status)
              << " frames=" << result.frames_run << " game_frames=" << game.frames()
              << " scene_meshes=" << game.scene_meshes_submitted()
              << " scene_materials=" << game.scene_materials_resolved() << '\n';
    print_presentation_report("sample_generated_desktop_runtime_cooked_scene_package", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "sample_generated_desktop_runtime_cooked_scene_package presentation_diagnostic="
                  << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
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
    }
    return 0;
}
