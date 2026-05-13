#requires -Version 7.0
#requires -PSEdition Core

param(
    [Parameter(Mandatory = $true)]
    [string]$Name,

    [string]$DisplayName = "",

    [ValidateSet("Headless", "DesktopRuntimePackage", "DesktopRuntimeCookedScenePackage", "DesktopRuntimeMaterialShaderPackage", "DesktopRuntime2DPackage", "DesktopRuntime3DPackage")]
    [string]$Template = "Headless",

    [string]$RepositoryRoot = "",

    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$scriptRepositoryRoot = Get-RepoRoot

. (Join-Path $PSScriptRoot "new-game-helpers.ps1")

if ([string]::IsNullOrWhiteSpace($RepositoryRoot)) {
    $root = $scriptRepositoryRoot
} else {
    $root = (Resolve-Path -LiteralPath $RepositoryRoot).Path
}

if ($Name -notmatch "^[a-z][a-z0-9_]*$") {
    Write-Error "Game name must match ^[a-z][a-z0-9_]*$"
}

$targetName = $Name
$manifestName = $Name.Replace("_", "-")
$gameDir = Join-Path $root (Join-Path "games" $Name)
$runtimeDir = Join-Path $gameDir "runtime"
$gamesCmake = Join-Path $root "games/CMakeLists.txt"

if ([string]::IsNullOrWhiteSpace($DisplayName)) {
    $DisplayName = $manifestName
}

if (Test-ContainsControlCharacter -Text $DisplayName) {
    Write-Error "DisplayName must not contain control characters."
}

if (-not (Test-Path -LiteralPath $gamesCmake)) {
    Write-Error "games/CMakeLists.txt does not exist under repository root: $root"
}

if (Test-Path -LiteralPath $gameDir) {
    Write-Error "Game directory already exists: $gameDir"
}

$gamesCmakeContent = Get-Content -LiteralPath $gamesCmake -Raw
$escapedTargetName = [System.Text.RegularExpressions.Regex]::Escape($targetName)
if ($gamesCmakeContent -match "(?m)^\s*MK_add_(?:desktop_runtime_)?game\(\s*$escapedTargetName(?=\s|\))") {
    Write-Error "Game target already exists in games/CMakeLists.txt: $targetName"
}

function New-HeadlessMainCpp {
    param(
        [string]$GameName,
        [string]$TargetName
    )

    return @"
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"

#include <iostream>

namespace {

class ${TargetName}_Game final : public mirakana::GameApp {
public:
    void on_start(mirakana::EngineContext& context) override {
        context.logger.write(mirakana::LogRecord{mirakana::LogLevel::info, "$GameName", "started"});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        ++frames_;
        return frames_ < 1;
    }

    [[nodiscard]] int frames() const noexcept {
        return frames_;
    }

private:
    int frames_{0};
};

} // namespace

int main() {
    mirakana::RingBufferLogger logger(16);
    mirakana::Registry registry;
    mirakana::HeadlessRunner runner(logger, registry);
    ${TargetName}_Game game;

    const auto result = runner.run(game, mirakana::RunConfig{1, 1.0 / 60.0});
    std::cout << "$GameName frames=" << result.frames_run << '\n';

    return result.frames_run == 1 && game.frames() == 1 ? 0 : 1;
}
"@
}

function New-DesktopRuntimeMainCpp {
    param(
        [string]$GameName,
        [string]$TargetName,
        [string]$Title
    )

    $escapedTitle = ConvertTo-CppStringLiteralContent -Text $Title

    return @"
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/application.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

namespace {

struct DesktopRuntimeOptions {
    bool smoke{false};
    bool show_help{false};
    bool throttle{true};
    std::uint32_t max_frames{0};
    std::string video_driver_hint;
    std::string required_config_path;
};

constexpr std::string_view kExpectedConfigFormat{"format=GameEngine.GeneratedDesktopRuntimePackage.Config.v1"};

class ${TargetName}_Game final : public mirakana::GameApp {
  public:
    ${TargetName}_Game(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle)
        : input_(input), renderer_(renderer), throttle_(throttle) {}

    void on_start(mirakana::EngineContext&) override {
        renderer_.set_clear_color(mirakana::Color{0.025F, 0.035F, 0.045F, 1.0F});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        renderer_.begin_frame();

        const auto axis = input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
        transform_.position = transform_.position + axis;
        renderer_.draw_sprite(mirakana::SpriteCommand{transform_, mirakana::Color{0.35F, 0.75F, 0.45F, 1.0F}});

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

  private:
    mirakana::VirtualInput& input_;
    mirakana::IRenderer& renderer_;
    mirakana::Transform2D transform_;
    bool throttle_{true};
    std::uint32_t frames_{0};
};

[[nodiscard]] bool parse_positive_uint32(std::string_view text, std::uint32_t& value) noexcept {
    std::uint32_t parsed{};
    const auto* begin = text.data();
    const auto* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end || parsed == 0) {
        return false;
    }
    value = parsed;
    return true;
}

void print_usage() {
    std::cout << "$TargetName [--smoke] [--max-frames N] [--video-driver NAME] "
                 "[--require-config PATH]\n";
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

    mirakana::SdlDesktopGameHost host(mirakana::SdlDesktopGameHostDesc{
        .title = "$escapedTitle",
        .extent = mirakana::WindowExtent{960, 540},
        .video_driver_hint = options.video_driver_hint,
    });

    ${TargetName}_Game game(host.input(), host.renderer(), options.throttle);
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();

    std::cout << "$TargetName status=" << status_name(result.status)
              << " renderer=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_requested=" << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
              << " presentation_selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_fallback=" << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " presentation_backend_reports=" << report.backend_reports_count
              << " presentation_diagnostics=" << report.diagnostics_count << " scene_gpu_status="
              << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " frames=" << result.frames_run << " game_frames=" << game.frames() << '\n';
    print_presentation_report("$TargetName", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "$TargetName presentation_diagnostic="
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": " << diagnostic.message
                  << '\n';
    }

    if (options.smoke && (result.status != mirakana::DesktopRunStatus::completed || result.frames_run != options.max_frames ||
                          game.frames() != options.max_frames)) {
        return 3;
    }
    return 0;
}
"@
}

function New-DesktopRuntimeCookedSceneMainCpp {
    param(
        [string]$GameName,
        [string]$TargetName,
        [string]$Title,
        [string]$SceneAssetName
    )

    $escapedTitle = ConvertTo-CppStringLiteralContent -Text $Title
    $escapedSceneAssetName = ConvertTo-CppStringLiteralContent -Text $SceneAssetName

    return @"
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
constexpr std::string_view kRuntimeSceneVertexShaderPath{"shaders/${TargetName}_scene.vs.dxil"};
constexpr std::string_view kRuntimeSceneFragmentShaderPath{"shaders/${TargetName}_scene.ps.dxil"};
constexpr std::string_view kRuntimeSceneVulkanVertexShaderPath{"shaders/${TargetName}_scene.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanFragmentShaderPath{"shaders/${TargetName}_scene.ps.spv"};
constexpr std::string_view kRuntimePostprocessVertexShaderPath{"shaders/${TargetName}_postprocess.vs.dxil"};
constexpr std::string_view kRuntimePostprocessFragmentShaderPath{"shaders/${TargetName}_postprocess.ps.dxil"};
constexpr std::string_view kRuntimePostprocessVulkanVertexShaderPath{"shaders/${TargetName}_postprocess.vs.spv"};
constexpr std::string_view kRuntimePostprocessVulkanFragmentShaderPath{"shaders/${TargetName}_postprocess.ps.spv"};
constexpr std::uint32_t kRuntimeSceneTangentSpaceStrideBytes{48};

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("$escapedSceneAssetName");
}

[[nodiscard]] std::vector<mirakana::rhi::VertexBufferLayoutDesc> runtime_scene_vertex_buffers() {
    return {mirakana::rhi::VertexBufferLayoutDesc{
        .binding = 0,
        .stride = kRuntimeSceneTangentSpaceStrideBytes,
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
        mirakana::rhi::VertexAttributeDesc{
            .location = 3,
            .binding = 0,
            .offset = 32,
            .format = mirakana::rhi::VertexFormat::float32x4,
            .semantic = mirakana::rhi::VertexSemantic::tangent,
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
            const auto scene_submit =
                mirakana::submit_scene_render_packet(renderer_, scene_->render_packet,
                                               mirakana::SceneRenderSubmitDesc{
                                                   .fallback_mesh_color = mirakana::Color{0.35F, 0.75F, 0.45F, 1.0F},
                                                   .material_palette = &scene_->material_palette,
                                               });
            scene_meshes_submitted_ += scene_submit.meshes_submitted;
            scene_materials_resolved_ += scene_submit.material_colors_resolved;
        } else {
            const auto axis = input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
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
    std::optional<mirakana::AnimationFloatClipSourceDocument> animation_clip_;
    mirakana::AnimationTransformBindingSourceDocument animation_bindings_;
    std::uint32_t frames_{0};
    std::size_t scene_meshes_submitted_{0};
    std::size_t scene_materials_resolved_{0};
};

[[nodiscard]] bool parse_positive_uint32(std::string_view text, std::uint32_t& value) noexcept {
    std::uint32_t parsed{};
    const auto* begin = text.data();
    const auto* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end || parsed == 0) {
        return false;
    }
    value = parsed;
    return true;
}

void print_usage() {
    std::cout
        << "$TargetName [--smoke] [--max-frames N] [--video-driver NAME] "
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

        auto instance = mirakana::instantiate_runtime_scene_render_data(package_result.package, packaged_scene_asset_id());
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

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_vulkan_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_postprocess_shaders(const char* executable_path) {
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
        std::cout << "$TargetName shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_shader_bytecode.status) << ": "
                  << d3d12_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_postprocess_bytecode = load_packaged_d3d12_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_postprocess_bytecode.ready() && options.require_postprocess && !options.require_vulkan_renderer) {
        std::cout << "$TargetName postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_postprocess_bytecode.status) << ": "
                  << d3d12_postprocess_bytecode.diagnostic << '\n';
        return 4;
    }

    auto vulkan_shader_bytecode = load_packaged_vulkan_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shader_bytecode.ready() && options.require_vulkan_scene_shaders) {
        std::cout << "$TargetName vulkan_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shader_bytecode.status) << ": "
                  << vulkan_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_postprocess_bytecode = load_packaged_vulkan_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_postprocess_bytecode.ready() && options.require_postprocess && options.require_vulkan_renderer) {
        std::cout << "$TargetName vulkan_postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_postprocess_bytecode.status) << ": "
                  << vulkan_postprocess_bytecode.diagnostic << '\n';
        return 6;
    }

    std::optional<mirakana::SdlDesktopPresentationD3d12SceneRendererDesc> d3d12_scene_renderer;
    if (d3d12_shader_bytecode.ready() && d3d12_postprocess_bytecode.ready() && runtime_package.has_value() &&
        packaged_scene.has_value()) {
        d3d12_scene_renderer.emplace(mirakana::SdlDesktopPresentationD3d12SceneRendererDesc{
            .vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_shader_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{d3d12_shader_bytecode.vertex_shader.bytecode.data(),
                                                          d3d12_shader_bytecode.vertex_shader.bytecode.size()},
            },
            .fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_shader_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{d3d12_shader_bytecode.fragment_shader.bytecode.data(),
                                                          d3d12_shader_bytecode.fragment_shader.bytecode.size()},
            },
            .postprocess_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_postprocess_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{d3d12_postprocess_bytecode.vertex_shader.bytecode.data(),
                                                          d3d12_postprocess_bytecode.vertex_shader.bytecode.size()},
            },
            .postprocess_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_postprocess_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{d3d12_postprocess_bytecode.fragment_shader.bytecode.data(),
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
            .vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_shader_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{vulkan_shader_bytecode.vertex_shader.bytecode.data(),
                                                          vulkan_shader_bytecode.vertex_shader.bytecode.size()},
            },
            .fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_shader_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{vulkan_shader_bytecode.fragment_shader.bytecode.data(),
                                                          vulkan_shader_bytecode.fragment_shader.bytecode.size()},
            },
            .postprocess_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_postprocess_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{vulkan_postprocess_bytecode.vertex_shader.bytecode.data(),
                                                          vulkan_postprocess_bytecode.vertex_shader.bytecode.size()},
            },
            .postprocess_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_postprocess_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{vulkan_postprocess_bytecode.fragment_shader.bytecode.data(),
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
        .title = "$escapedTitle",
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
    if (options.require_d3d12_renderer && host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::d3d12) {
        std::cout << "$TargetName required_d3d12_renderer_unavailable renderer=" << host.presentation_backend_name()
                  << '\n';
        print_presentation_report("$TargetName", host);
        return 5;
    }
    if (options.require_vulkan_renderer && host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::vulkan) {
        std::cout << "$TargetName required_vulkan_renderer_unavailable renderer=" << host.presentation_backend_name()
                  << '\n';
        print_presentation_report("$TargetName", host);
        return 7;
    }
    if (options.require_scene_gpu_bindings && !host.scene_gpu_bindings_ready()) {
        std::cout << "$TargetName required_scene_gpu_bindings_unavailable status="
                  << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(host.scene_gpu_binding_status())
                  << '\n';
        print_presentation_report("$TargetName", host);
        return 5;
    }
    if (options.require_postprocess &&
        host.presentation_report().postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready) {
        std::cout << "$TargetName required_postprocess_unavailable status="
                  << mirakana::sdl_desktop_presentation_postprocess_status_name(host.presentation_report().postprocess_status)
                  << '\n';
        print_presentation_report("$TargetName", host);
        return 8;
    }

    GeneratedDesktopRuntimeCookedSceneGame game(host.input(), host.renderer(), options.throttle,
                                                std::move(packaged_scene));
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();
    const auto scene_gpu_stats = report.scene_gpu_stats;

    std::cout << "$TargetName status=" << status_name(result.status)
              << " renderer=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_requested=" << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
              << " presentation_selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_fallback=" << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " presentation_backend_reports=" << report.backend_reports_count
              << " presentation_diagnostics=" << report.diagnostics_count << " scene_gpu_status="
              << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " scene_gpu_mesh_bindings=" << scene_gpu_stats.mesh_bindings
              << " scene_gpu_skinned_mesh_bindings=" << scene_gpu_stats.skinned_mesh_bindings
              << " scene_gpu_material_bindings=" << scene_gpu_stats.material_bindings
              << " scene_gpu_mesh_uploads=" << scene_gpu_stats.mesh_uploads
              << " scene_gpu_skinned_mesh_uploads=" << scene_gpu_stats.skinned_mesh_uploads
              << " scene_gpu_texture_uploads=" << scene_gpu_stats.texture_uploads
              << " scene_gpu_material_uploads=" << scene_gpu_stats.material_uploads
              << " scene_gpu_material_pipeline_layouts=" << scene_gpu_stats.material_pipeline_layouts
              << " scene_gpu_uploaded_texture_bytes=" << scene_gpu_stats.uploaded_texture_bytes
              << " scene_gpu_uploaded_mesh_bytes=" << scene_gpu_stats.uploaded_mesh_bytes
              << " scene_gpu_uploaded_material_factor_bytes=" << scene_gpu_stats.uploaded_material_factor_bytes
              << " scene_gpu_mesh_resolved=" << scene_gpu_stats.mesh_bindings_resolved
              << " scene_gpu_skinned_mesh_resolved=" << scene_gpu_stats.skinned_mesh_bindings_resolved
              << " scene_gpu_material_resolved=" << scene_gpu_stats.material_bindings_resolved
              << " postprocess_status="
              << mirakana::sdl_desktop_presentation_postprocess_status_name(report.postprocess_status)
              << " framegraph_passes=" << report.framegraph_passes
              << " frames=" << result.frames_run << " game_frames=" << game.frames()
              << " scene_meshes=" << game.scene_meshes_submitted()
              << " scene_materials=" << game.scene_materials_resolved() << '\n';
    print_presentation_report("$TargetName", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "$TargetName presentation_diagnostic="
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": " << diagnostic.message
                  << '\n';
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
"@
}

function New-DesktopRuntime3DMainCpp {
    param(
        [string]$GameName,
        [string]$TargetName,
        [string]$Title,
        [string]$SceneAssetName
    )

    $escapedSceneAssetName = ConvertTo-CppStringLiteralContent -Text $SceneAssetName
    $assetKeyPrefix = $GameName.Replace("_", "-")
    $escapedMeshAssetName = ConvertTo-CppStringLiteralContent -Text "$assetKeyPrefix/meshes/triangle"
    $escapedSkinnedMeshAssetName = ConvertTo-CppStringLiteralContent -Text "$assetKeyPrefix/meshes/skinned-triangle"
    $escapedAnimationAssetName = ConvertTo-CppStringLiteralContent -Text "$assetKeyPrefix/animations/packaged-mesh-bob"
    $escapedMorphMeshAssetName = ConvertTo-CppStringLiteralContent -Text "$assetKeyPrefix/morphs/packaged-mesh"
    $escapedMorphAnimationAssetName = ConvertTo-CppStringLiteralContent -Text "$assetKeyPrefix/animations/packaged-mesh-morph-weights"
    $escapedQuaternionAnimationAssetName = ConvertTo-CppStringLiteralContent -Text "$assetKeyPrefix/animations/packaged-pose"
    $text = New-DesktopRuntimeCookedSceneMainCpp `
        -GameName $GameName `
        -TargetName $TargetName `
        -Title $Title `
        -SceneAssetName $SceneAssetName
    $text = $text.Replace('#include "mirakana/assets/asset_registry.hpp"', "#include `"mirakana/assets/asset_registry.hpp`"`n#include `"mirakana/assets/asset_source_format.hpp`"`n#include `"mirakana/animation/skeleton.hpp`"")
    $text = $text.Replace('#include "mirakana/runtime/asset_runtime.hpp"', "#include `"mirakana/runtime/asset_runtime.hpp`"`n#include `"mirakana/runtime/package_streaming.hpp`"`n#include `"mirakana/runtime/resource_runtime.hpp`"")
    $text = $text.Replace('#include "mirakana/runtime_host/shader_bytecode.hpp"', "#include `"mirakana/runtime_host/shader_bytecode.hpp`"`n#include `"mirakana/runtime_rhi/runtime_upload.hpp`"")
    $text = $text.Replace("#include <chrono>`n", "#include <chrono>`n#include <cmath>`n")
    $text = $text.Replace("GeneratedDesktopRuntimeCookedSceneGame", "GeneratedDesktopRuntime3DPackageGame")
    $text = $text.Replace(
        "GameEngine.GeneratedDesktopRuntimeCookedScenePackage.Config.v1",
        "GameEngine.GeneratedDesktopRuntime3DPackage.Config.v1")
    $text = $text.Replace(@"
constexpr std::string_view kRuntimeSceneVertexShaderPath{"shaders/${TargetName}_scene.vs.dxil"};
constexpr std::string_view kRuntimeSceneFragmentShaderPath{"shaders/${TargetName}_scene.ps.dxil"};
"@, @"
constexpr std::string_view kRuntimeSceneVertexShaderPath{"shaders/${TargetName}_scene.vs.dxil"};
constexpr std::string_view kRuntimeSceneMorphVertexShaderPath{"shaders/${TargetName}_scene_morph.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphVertexShaderPath{"shaders/${TargetName}_scene_compute_morph.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphShaderPath{"shaders/${TargetName}_scene_compute_morph.cs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphTangentFrameVertexShaderPath{"shaders/${TargetName}_scene_compute_morph_tangent_frame.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphTangentFrameShaderPath{"shaders/${TargetName}_scene_compute_morph_tangent_frame.cs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphSkinnedVertexShaderPath{"shaders/${TargetName}_scene_compute_morph_skinned.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphSkinnedShaderPath{"shaders/${TargetName}_scene_compute_morph_skinned.cs.dxil"};
constexpr std::string_view kRuntimeSceneFragmentShaderPath{"shaders/${TargetName}_scene.ps.dxil"};
constexpr std::string_view kRuntimeShadowReceiverFragmentShaderPath{"shaders/${TargetName}_shadow_receiver.ps.dxil"};
constexpr std::string_view kRuntimeShiftedShadowReceiverFragmentShaderPath{"shaders/${TargetName}_shadow_receiver_shifted.ps.dxil"};
constexpr std::string_view kRuntimeShadowVertexShaderPath{"shaders/${TargetName}_shadow.vs.dxil"};
constexpr std::string_view kRuntimeShadowFragmentShaderPath{"shaders/${TargetName}_shadow.ps.dxil"};
"@)
    $text = $text.Replace(@"
constexpr std::string_view kRuntimeSceneVulkanVertexShaderPath{"shaders/${TargetName}_scene.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanFragmentShaderPath{"shaders/${TargetName}_scene.ps.spv"};
"@, @"
constexpr std::string_view kRuntimeSceneVulkanVertexShaderPath{"shaders/${TargetName}_scene.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanMorphVertexShaderPath{"shaders/${TargetName}_scene_morph.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphVertexShaderPath{"shaders/${TargetName}_scene_compute_morph.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphShaderPath{"shaders/${TargetName}_scene_compute_morph.cs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphTangentFrameVertexShaderPath{"shaders/${TargetName}_scene_compute_morph_tangent_frame.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphTangentFrameShaderPath{"shaders/${TargetName}_scene_compute_morph_tangent_frame.cs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphSkinnedVertexShaderPath{"shaders/${TargetName}_scene_compute_morph_skinned.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphSkinnedShaderPath{"shaders/${TargetName}_scene_compute_morph_skinned.cs.spv"};
constexpr std::string_view kRuntimeSceneVulkanFragmentShaderPath{"shaders/${TargetName}_scene.ps.spv"};
constexpr std::string_view kRuntimeShadowReceiverVulkanFragmentShaderPath{"shaders/${TargetName}_shadow_receiver.ps.spv"};
constexpr std::string_view kRuntimeShiftedShadowReceiverVulkanFragmentShaderPath{"shaders/${TargetName}_shadow_receiver_shifted.ps.spv"};
constexpr std::string_view kRuntimeShadowVulkanVertexShaderPath{"shaders/${TargetName}_shadow.vs.spv"};
constexpr std::string_view kRuntimeShadowVulkanFragmentShaderPath{"shaders/${TargetName}_shadow.ps.spv"};
"@)
    $text = $text -replace 'bool require_postprocess\{false\};\r?\n',
        "bool require_postprocess{false};`n    bool require_postprocess_depth_input{false};`n    bool require_directional_shadow{false};`n    bool require_directional_shadow_filtering{false};`n    bool require_shadow_morph_composition{false};`n    bool require_renderer_quality_gates{false};`n    bool require_playable_3d_slice{false};`n    bool require_primary_camera_controller{false};`n    bool require_transform_animation{false};`n    bool require_morph_package{false};`n    bool require_compute_morph{false};`n    bool require_compute_morph_normal_tangent{false};`n    bool require_compute_morph_skin{false};`n    bool require_compute_morph_async_telemetry{false};`n    bool require_quaternion_animation{false};`n    bool require_package_streaming_safe_point{false};`n"
    $text = $text.Replace(
        "constexpr std::uint32_t kRuntimeSceneTangentSpaceStrideBytes{48};",
        "constexpr std::uint32_t kRuntimeSceneTangentSpaceStrideBytes{48};`nconstexpr std::uint64_t kPackageStreamingResidentBudgetBytes{67108864};")

    $sceneAssetFunction = @"
[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("$escapedSceneAssetName");
}
"@
    $sceneAssetFunctionWithCamera = @"
[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("$escapedSceneAssetName");
}

[[nodiscard]] mirakana::AssetId packaged_animation_asset_id() {
    return asset_id_from_game_asset_key("$escapedAnimationAssetName");
}

[[nodiscard]] mirakana::AssetId packaged_mesh_asset_id() {
    return asset_id_from_game_asset_key("$escapedMeshAssetName");
}

[[nodiscard]] mirakana::AssetId packaged_skinned_mesh_asset_id() {
    return asset_id_from_game_asset_key("$escapedSkinnedMeshAssetName");
}

[[nodiscard]] mirakana::AssetId packaged_morph_mesh_asset_id() {
    return asset_id_from_game_asset_key("$escapedMorphMeshAssetName");
}

[[nodiscard]] mirakana::AssetId packaged_morph_animation_asset_id() {
    return asset_id_from_game_asset_key("$escapedMorphAnimationAssetName");
}

[[nodiscard]] mirakana::AssetId packaged_quaternion_animation_asset_id() {
    return asset_id_from_game_asset_key("$escapedQuaternionAnimationAssetName");
}

[[nodiscard]] std::string_view package_streaming_status_name(
    mirakana::runtime::RuntimePackageStreamingExecutionStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::invalid_descriptor:
        return "invalid_descriptor";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::validation_preflight_required:
        return "validation_preflight_required";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::package_load_failed:
        return "package_load_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::residency_hint_failed:
        return "residency_hint_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent:
        return "over_budget_intent";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::safe_point_replacement_failed:
        return "safe_point_replacement_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed:
        return "committed";
    }
    return "unknown";
}

[[nodiscard]] mirakana::AnimationSkeleton3dDesc packaged_quaternion_animation_skeleton() {
    return mirakana::AnimationSkeleton3dDesc{
        {mirakana::AnimationSkeleton3dJointDesc{.name = "PackagedMesh", .parent_index = mirakana::animation_no_parent}},
    };
}

[[nodiscard]] std::vector<mirakana::AnimationJointTrack3dDesc>
make_quaternion_animation_tracks(const mirakana::runtime::RuntimeAnimationQuaternionClipPayload& payload) {
    std::vector<mirakana::AnimationJointTrack3dByteSource> sources;
    sources.reserve(payload.clip.tracks.size());
    for (const auto& track : payload.clip.tracks) {
        sources.push_back(mirakana::AnimationJointTrack3dByteSource{
            .joint_index = track.joint_index,
            .target = track.target,
            .translation_time_seconds_bytes = track.translation_time_seconds_bytes,
            .translation_xyz_bytes = track.translation_xyz_bytes,
            .rotation_time_seconds_bytes = track.rotation_time_seconds_bytes,
            .rotation_xyzw_bytes = track.rotation_xyzw_bytes,
            .scale_time_seconds_bytes = track.scale_time_seconds_bytes,
            .scale_xyz_bytes = track.scale_xyz_bytes,
        });
    }
    return mirakana::make_animation_joint_tracks_3d_from_f32_bytes(sources);
}

[[nodiscard]] mirakana::AnimationTransformBindingSourceDocument packaged_animation_bindings() {
    mirakana::AnimationTransformBindingSourceDocument document;
    document.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        "gltf/node/0/translation/x",
        "PackagedMesh",
        mirakana::AnimationTransformBindingComponent::translation_x,
    });
    return document;
}

constexpr mirakana::SceneNodeId kPackagedMeshNode{1};
constexpr mirakana::SceneNodeId kPrimaryCameraNode{3};

constexpr std::uint32_t kRuntimeScenePositionStrideBytes{12};
constexpr std::uint32_t kRuntimeSceneSkinnedStrideBytes{mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes};
constexpr std::uint32_t kRuntimeSceneSkinnedJointIndicesOffsetBytes{48};
constexpr std::uint32_t kRuntimeSceneSkinnedJointWeightsOffsetBytes{56};

[[nodiscard]] std::vector<mirakana::rhi::VertexBufferLayoutDesc> runtime_compute_morph_skinned_scene_vertex_buffers() {
    return {
        mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0,
            .stride = kRuntimeScenePositionStrideBytes,
            .input_rate = mirakana::rhi::VertexInputRate::vertex,
        },
        mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 3,
            .stride = kRuntimeSceneSkinnedStrideBytes,
            .input_rate = mirakana::rhi::VertexInputRate::vertex,
        },
    };
}

[[nodiscard]] std::vector<mirakana::rhi::VertexAttributeDesc> runtime_compute_morph_skinned_scene_vertex_attributes() {
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
            .binding = 3,
            .offset = kRuntimeSceneSkinnedJointIndicesOffsetBytes,
            .format = mirakana::rhi::VertexFormat::uint16x4,
            .semantic = mirakana::rhi::VertexSemantic::joint_indices,
        },
        mirakana::rhi::VertexAttributeDesc{
            .location = 2,
            .binding = 3,
            .offset = kRuntimeSceneSkinnedJointWeightsOffsetBytes,
            .format = mirakana::rhi::VertexFormat::float32x4,
            .semantic = mirakana::rhi::VertexSemantic::joint_weights,
        },
    };
}

[[nodiscard]] bool activate_compute_morph_skinned_scene(std::optional<mirakana::RuntimeSceneRenderInstance>& scene) {
    if (!scene.has_value() || !scene->scene.has_value()) {
        return false;
    }
    auto* mesh_node = scene->scene->find_node(kPackagedMeshNode);
    if (mesh_node == nullptr || !mesh_node->components.mesh_renderer.has_value()) {
        return false;
    }
    mesh_node->components.mesh_renderer->mesh = packaged_skinned_mesh_asset_id();
    scene->render_packet = mirakana::build_scene_render_packet(*scene->scene);
    return true;
}
"@
    $text = $text.Replace($sceneAssetFunction, $sceneAssetFunctionWithCamera)
    $text = $text -replace 'renderer_\.set_clear_color\(mirakana::Color\{0\.025F, 0\.035F, 0\.045F, 1\.0F\}\);\r?\n    \}',
        "renderer_.set_clear_color(mirakana::Color{0.025F, 0.035F, 0.045F, 1.0F});`n        input_.press(mirakana::Key::right);`n    }"
    $text = $text.Replace(@"
    GeneratedDesktopRuntime3DPackageGame(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle,
                                           std::optional<mirakana::RuntimeSceneRenderInstance> scene)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)) {}
"@, @"
    GeneratedDesktopRuntime3DPackageGame(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle,
                                         std::optional<mirakana::RuntimeSceneRenderInstance> scene,
                                         std::optional<mirakana::AnimationFloatClipSourceDocument> animation_clip,
                                         mirakana::AnimationTransformBindingSourceDocument animation_bindings,
                                         std::optional<mirakana::runtime::RuntimeMorphMeshCpuPayload> morph_payload,
                                         std::optional<mirakana::AnimationFloatClipSourceDocument> morph_animation_clip,
                                         std::vector<mirakana::AnimationJointTrack3dDesc> quaternion_animation_tracks)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)),
          animation_clip_(std::move(animation_clip)), animation_bindings_(std::move(animation_bindings)),
          morph_payload_(std::move(morph_payload)), morph_animation_clip_(std::move(morph_animation_clip)),
          quaternion_animation_tracks_(std::move(quaternion_animation_tracks)) {}
"@)

    $oldSceneSubmit = @"
        if (scene_.has_value()) {
            const auto scene_submit =
                mirakana::submit_scene_render_packet(renderer_, scene_->render_packet,
                                               mirakana::SceneRenderSubmitDesc{
                                                   .fallback_mesh_color = mirakana::Color{0.35F, 0.75F, 0.45F, 1.0F},
                                                   .material_palette = &scene_->material_palette,
                                               });
            scene_meshes_submitted_ += scene_submit.meshes_submitted;
            scene_materials_resolved_ += scene_submit.material_colors_resolved;
        } else {
            const auto axis = input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
            transform_.position = transform_.position + axis;
            renderer_.draw_sprite(mirakana::SpriteCommand{transform_, mirakana::Color{0.35F, 0.75F, 0.45F, 1.0F}});
        }
"@
    $newSceneSubmit = @"
        const auto axis = input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
        if (scene_.has_value()) {
            std::optional<mirakana::SceneRenderPacket> rebuilt_packet;
            const auto* render_packet = &scene_->render_packet;
            if (scene_->scene.has_value()) {
                if (auto* camera = scene_->scene->find_node(kPrimaryCameraNode);
                    camera != nullptr && camera->components.camera.has_value() && camera->components.camera->primary) {
                    camera->transform.position.x += axis.x;
                    final_camera_x_ = camera->transform.position.x;
                    ++camera_controller_ticks_;
                    primary_camera_seen_ = true;
                }
                if (animation_clip_.has_value()) {
                    const auto animation_result = mirakana::sample_and_apply_runtime_scene_render_animation_float_clip(
                        *scene_, *animation_clip_, animation_bindings_, 1.0F);
                    transform_animation_samples_ += animation_result.sampled_track_count;
                    transform_animation_applied_ += animation_result.applied_sample_count;
                    if (animation_result.succeeded) {
                        ++transform_animation_ticks_;
                        transform_animation_seen_ = true;
                        if (const auto* animated_mesh = scene_->scene->find_node(kPackagedMeshNode);
                            animated_mesh != nullptr) {
                            final_mesh_x_ = animated_mesh->transform.position.x;
                        }
                    } else {
                        ++transform_animation_failures_;
                    }
                }
                if (morph_payload_.has_value() && morph_animation_clip_.has_value()) {
                    const auto morph_result = mirakana::sample_runtime_morph_mesh_cpu_animation_float_clip(
                        *morph_payload_, *morph_animation_clip_, "gltf/node/0/weights/", 1.0F);
                    morph_package_samples_ += morph_result.sampled_track_count;
                    morph_package_weights_ += morph_result.applied_weight_count;
                    morph_package_vertices_ += morph_result.morphed_positions.size();
                    if (morph_result.succeeded && !morph_result.morphed_positions.empty()) {
                        const auto vertex_count = static_cast<std::uint64_t>(morph_result.morphed_positions.size());
                        if (morph_package_vertices_per_sample_ == 0U) {
                            morph_package_vertices_per_sample_ = vertex_count;
                        }
                        if (morph_package_vertices_per_sample_ == vertex_count) {
                            ++morph_package_ticks_;
                            morph_package_seen_ = true;
                            morph_first_position_x_ = morph_result.morphed_positions.front().x;
                        } else {
                            ++morph_package_failures_;
                        }
                    } else {
                        ++morph_package_failures_;
                    }
                }
                if (!quaternion_animation_tracks_.empty()) {
                    const auto apply_result = mirakana::sample_and_apply_runtime_scene_render_animation_pose_3d(
                        *scene_, packaged_quaternion_animation_skeleton(), quaternion_animation_tracks_, 1.0F);
                    quaternion_animation_tracks_sampled_ += apply_result.sampled_track_count;
                    if (apply_result.succeeded) {
                        const auto pose = mirakana::sample_animation_local_pose_3d(packaged_quaternion_animation_skeleton(),
                                                                             quaternion_animation_tracks_, 1.0F);
                        const auto* animated_mesh = scene_->scene->find_node(kPackagedMeshNode);
                        quaternion_animation_scene_applied_ +=
                            static_cast<std::uint32_t>(apply_result.applied_sample_count);
                        if (pose.joints.size() == 1U && animated_mesh != nullptr) {
                            ++quaternion_animation_ticks_;
                            quaternion_animation_seen_ = true;
                            final_quaternion_z_ = pose.joints[0].rotation.z;
                            final_quaternion_w_ = pose.joints[0].rotation.w;
                            final_quaternion_scene_rotation_z_ = animated_mesh->transform.rotation_radians.z;
                        } else {
                            ++quaternion_animation_failures_;
                        }
                    } else {
                        ++quaternion_animation_failures_;
                    }
                }
                rebuilt_packet = mirakana::build_scene_render_packet(*scene_->scene);
                render_packet = &*rebuilt_packet;
            }

            const auto mesh_plan = mirakana::plan_scene_mesh_draws(*render_packet);
            scene_mesh_plan_ok_ = scene_mesh_plan_ok_ && mesh_plan.succeeded();
            scene_mesh_plan_meshes_ += mesh_plan.mesh_count;
            scene_mesh_plan_draws_ += mesh_plan.draw_count;
            scene_mesh_plan_unique_meshes_ += mesh_plan.unique_mesh_count;
            scene_mesh_plan_unique_materials_ += mesh_plan.unique_material_count;
            scene_mesh_plan_diagnostics_ += mesh_plan.diagnostics.size();
            const auto scene_submit =
                mirakana::submit_scene_render_packet(renderer_, *render_packet,
                                               mirakana::SceneRenderSubmitDesc{
                                                   .fallback_mesh_color = mirakana::Color{0.35F, 0.75F, 0.45F, 1.0F},
                                                   .material_palette = &scene_->material_palette,
                                               });
            scene_meshes_submitted_ += scene_submit.meshes_submitted;
            scene_materials_resolved_ += scene_submit.material_colors_resolved;
            primary_camera_seen_ = primary_camera_seen_ || scene_submit.has_primary_camera;
        } else {
            transform_.position = transform_.position + axis;
            renderer_.draw_sprite(mirakana::SpriteCommand{transform_, mirakana::Color{0.35F, 0.75F, 0.45F, 1.0F}});
        }
"@
    $text = $text.Replace($oldSceneSubmit, $newSceneSubmit)
    $text = $text.Replace(@"
    [[nodiscard]] std::size_t scene_materials_resolved() const noexcept {
        return scene_materials_resolved_;
    }
"@, @"
    [[nodiscard]] std::size_t scene_materials_resolved() const noexcept {
        return scene_materials_resolved_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_meshes() const noexcept {
        return scene_mesh_plan_meshes_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_draws() const noexcept {
        return scene_mesh_plan_draws_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_unique_meshes() const noexcept {
        return scene_mesh_plan_unique_meshes_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_unique_materials() const noexcept {
        return scene_mesh_plan_unique_materials_;
    }

    [[nodiscard]] std::size_t scene_mesh_plan_diagnostics() const noexcept {
        return scene_mesh_plan_diagnostics_;
    }

    [[nodiscard]] bool scene_mesh_plan_succeeded() const noexcept {
        return scene_mesh_plan_ok_;
    }

    [[nodiscard]] bool primary_camera_controller_passed(std::uint32_t expected_frames) const noexcept {
        return primary_camera_seen_ && camera_controller_ticks_ == expected_frames &&
               final_camera_x_ == static_cast<float>(expected_frames);
    }

    [[nodiscard]] std::uint32_t camera_controller_ticks() const noexcept {
        return camera_controller_ticks_;
    }

    [[nodiscard]] float final_camera_x() const noexcept {
        return final_camera_x_;
    }

    [[nodiscard]] bool transform_animation_passed(std::uint32_t expected_frames) const noexcept {
        return transform_animation_seen_ && transform_animation_ticks_ == expected_frames &&
               transform_animation_samples_ == static_cast<std::uint64_t>(expected_frames) &&
               transform_animation_applied_ == static_cast<std::uint64_t>(expected_frames) &&
               transform_animation_failures_ == 0 && final_mesh_x_ == 0.5F;
    }

    [[nodiscard]] std::uint32_t transform_animation_ticks() const noexcept {
        return transform_animation_ticks_;
    }

    [[nodiscard]] std::uint64_t transform_animation_samples() const noexcept {
        return transform_animation_samples_;
    }

    [[nodiscard]] std::uint64_t transform_animation_applied() const noexcept {
        return transform_animation_applied_;
    }

    [[nodiscard]] std::uint32_t transform_animation_failures() const noexcept {
        return transform_animation_failures_;
    }

    [[nodiscard]] float final_mesh_x() const noexcept {
        return final_mesh_x_;
    }

    [[nodiscard]] bool morph_package_passed(std::uint32_t expected_frames) const noexcept {
        return morph_package_seen_ && morph_package_ticks_ == expected_frames &&
               morph_package_samples_ == static_cast<std::uint64_t>(expected_frames) &&
               morph_package_weights_ == static_cast<std::uint64_t>(expected_frames) &&
               morph_package_vertices_per_sample_ > 0U &&
               morph_package_vertices_ ==
                   static_cast<std::uint64_t>(expected_frames) * morph_package_vertices_per_sample_ &&
               morph_package_failures_ == 0 && morph_first_position_x_ == 0.0F;
    }

    [[nodiscard]] std::uint32_t morph_package_ticks() const noexcept {
        return morph_package_ticks_;
    }

    [[nodiscard]] std::uint64_t morph_package_samples() const noexcept {
        return morph_package_samples_;
    }

    [[nodiscard]] std::uint64_t morph_package_weights() const noexcept {
        return morph_package_weights_;
    }

    [[nodiscard]] std::uint64_t morph_package_vertices() const noexcept {
        return morph_package_vertices_;
    }

    [[nodiscard]] std::uint32_t morph_package_failures() const noexcept {
        return morph_package_failures_;
    }

    [[nodiscard]] float morph_first_position_x() const noexcept {
        return morph_first_position_x_;
    }

    [[nodiscard]] bool quaternion_animation_passed(std::uint32_t expected_frames) const noexcept {
        return quaternion_animation_seen_ && quaternion_animation_ticks_ == expected_frames &&
               quaternion_animation_tracks_sampled_ ==
                   static_cast<std::uint64_t>(expected_frames) * quaternion_animation_tracks_.size() &&
               quaternion_animation_scene_applied_ == expected_frames && quaternion_animation_failures_ == 0 &&
               std::abs(final_quaternion_z_ - 1.0F) < 0.0001F && std::abs(final_quaternion_w_) < 0.0001F &&
               std::abs(final_quaternion_scene_rotation_z_ - 3.14159265F) < 0.0001F;
    }

    [[nodiscard]] std::uint32_t quaternion_animation_ticks() const noexcept {
        return quaternion_animation_ticks_;
    }

    [[nodiscard]] std::uint64_t quaternion_animation_tracks_sampled() const noexcept {
        return quaternion_animation_tracks_sampled_;
    }

    [[nodiscard]] std::uint32_t quaternion_animation_failures() const noexcept {
        return quaternion_animation_failures_;
    }

    [[nodiscard]] std::uint32_t quaternion_animation_scene_applied() const noexcept {
        return quaternion_animation_scene_applied_;
    }

    [[nodiscard]] float final_quaternion_z() const noexcept {
        return final_quaternion_z_;
    }

    [[nodiscard]] float final_quaternion_w() const noexcept {
        return final_quaternion_w_;
    }

    [[nodiscard]] float final_quaternion_scene_rotation_z() const noexcept {
        return final_quaternion_scene_rotation_z_;
    }
"@)
    $text = $text.Replace(@"
    std::uint32_t frames_{0};
    std::size_t scene_meshes_submitted_{0};
    std::size_t scene_materials_resolved_{0};
"@, @"
    std::uint32_t frames_{0};
    std::size_t scene_meshes_submitted_{0};
    std::size_t scene_materials_resolved_{0};
    std::uint64_t scene_mesh_plan_meshes_{0};
    std::uint64_t scene_mesh_plan_draws_{0};
    std::uint64_t scene_mesh_plan_unique_meshes_{0};
    std::uint64_t scene_mesh_plan_unique_materials_{0};
    std::size_t scene_mesh_plan_diagnostics_{0};
    std::uint32_t camera_controller_ticks_{0};
    float final_camera_x_{0.0F};
    std::uint32_t transform_animation_ticks_{0};
    std::uint64_t transform_animation_samples_{0};
    std::uint64_t transform_animation_applied_{0};
    std::uint32_t transform_animation_failures_{0};
    float final_mesh_x_{0.0F};
    std::uint32_t morph_package_ticks_{0};
    std::uint64_t morph_package_samples_{0};
    std::uint64_t morph_package_weights_{0};
    std::uint64_t morph_package_vertices_{0};
    std::uint64_t morph_package_vertices_per_sample_{0};
    std::uint32_t morph_package_failures_{0};
    float morph_first_position_x_{0.0F};
    std::uint32_t quaternion_animation_ticks_{0};
    std::uint64_t quaternion_animation_tracks_sampled_{0};
    std::uint32_t quaternion_animation_failures_{0};
    std::uint32_t quaternion_animation_scene_applied_{0};
    float final_quaternion_z_{0.0F};
    float final_quaternion_w_{1.0F};
    float final_quaternion_scene_rotation_z_{0.0F};
    bool primary_camera_seen_{false};
    bool transform_animation_seen_{false};
    bool morph_package_seen_{false};
    bool quaternion_animation_seen_{false};
    bool scene_mesh_plan_ok_{true};
"@)
    $text = $text.Replace(@"
    std::optional<mirakana::AnimationFloatClipSourceDocument> animation_clip_;
    mirakana::AnimationTransformBindingSourceDocument animation_bindings_;
"@, @"
    std::optional<mirakana::AnimationFloatClipSourceDocument> animation_clip_;
    mirakana::AnimationTransformBindingSourceDocument animation_bindings_;
    std::optional<mirakana::runtime::RuntimeMorphMeshCpuPayload> morph_payload_;
    std::optional<mirakana::AnimationFloatClipSourceDocument> morph_animation_clip_;
    std::vector<mirakana::AnimationJointTrack3dDesc> quaternion_animation_tracks_;
"@)
    $text = $text.Replace("[--require-scene-gpu-bindings] [--require-postprocess]\n", "[--require-scene-gpu-bindings] [--require-postprocess] [--require-postprocess-depth-input] [--require-directional-shadow] [--require-directional-shadow-filtering] [--require-shadow-morph-composition] [--require-renderer-quality-gates] [--require-playable-3d-slice] [--require-primary-camera-controller] [--require-transform-animation] [--require-morph-package] [--require-compute-morph] [--require-compute-morph-skin] [--require-compute-morph-async-telemetry] [--require-quaternion-animation] [--require-package-streaming-safe-point]\n")
    $text = $text.Replace(@"
    if (options.require_d3d12_renderer && options.require_vulkan_renderer) {
        std::cerr << "--require-d3d12-renderer and --require-vulkan-renderer are mutually exclusive\n";
        return false;
    }
"@, @"
    if (options.require_d3d12_renderer && options.require_vulkan_renderer) {
        std::cerr << "--require-d3d12-renderer and --require-vulkan-renderer are mutually exclusive\n";
        return false;
    }
    if (options.require_shadow_morph_composition &&
        (options.require_playable_3d_slice || options.require_compute_morph)) {
        std::cerr << "--require-shadow-morph-composition is a selected graphics morph smoke and cannot be combined "
                     "with --require-playable-3d-slice or compute morph flags\n";
        return false;
    }
    if (options.require_directional_shadow && !options.require_shadow_morph_composition &&
        (options.require_playable_3d_slice || options.require_morph_package || options.require_compute_morph)) {
        std::cerr << "--require-directional-shadow is a selected renderer smoke and cannot be combined with "
                     "--require-playable-3d-slice, --require-morph-package, or compute morph flags\n";
        return false;
    }
"@)
    $text = $text.Replace(@"
[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_vulkan_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanFragmentShaderPath},
    });
}
"@, @"
[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_scene_morph_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneMorphVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneFragmentShaderPath},
        .vertex_entry_point = "vs_morph",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_scene_compute_morph_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneComputeMorphVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneComputeMorphShaderPath},
        .vertex_entry_point = "vs_compute_morph",
        .fragment_entry_point = "cs_compute_morph_position",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_scene_compute_morph_tangent_frame_shaders(
    const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneComputeMorphTangentFrameVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneComputeMorphTangentFrameShaderPath},
        .vertex_entry_point = "vs_compute_morph_tangent_frame",
        .fragment_entry_point = "cs_compute_morph_tangent_frame",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_scene_compute_morph_skinned_shaders(
    const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneComputeMorphSkinnedVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneComputeMorphSkinnedShaderPath},
        .vertex_entry_point = "vs_compute_morph_skinned",
        .fragment_entry_point = "cs_compute_morph_skinned_position",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_shadow_receiver_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowReceiverFragmentShaderPath},
        .fragment_entry_point = "ps_shadow_receiver",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_shifted_shadow_receiver_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVertexShaderPath},
        .fragment_path = std::string{kRuntimeShiftedShadowReceiverFragmentShaderPath},
        .fragment_entry_point = "ps_shadow_receiver",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_shadow_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeShadowVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowFragmentShaderPath},
        .vertex_entry_point = "vs_shadow",
        .fragment_entry_point = "ps_shadow",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_vulkan_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_vulkan_scene_morph_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanMorphVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_morph",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_scene_compute_morph_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanComputeMorphVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanComputeMorphShaderPath},
        .vertex_entry_point = "vs_compute_morph",
        .fragment_entry_point = "cs_compute_morph_position",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_scene_compute_morph_tangent_frame_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanComputeMorphTangentFrameVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanComputeMorphTangentFrameShaderPath},
        .vertex_entry_point = "vs_compute_morph_tangent_frame",
        .fragment_entry_point = "cs_compute_morph_tangent_frame",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_scene_compute_morph_skinned_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanComputeMorphSkinnedVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanComputeMorphSkinnedShaderPath},
        .vertex_entry_point = "vs_compute_morph_skinned",
        .fragment_entry_point = "cs_compute_morph_skinned_position",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_shadow_receiver_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowReceiverVulkanFragmentShaderPath},
        .fragment_entry_point = "ps_shadow_receiver",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_shifted_shadow_receiver_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeShiftedShadowReceiverVulkanFragmentShaderPath},
        .fragment_entry_point = "ps_shadow_receiver",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_vulkan_shadow_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeShadowVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_shadow",
        .fragment_entry_point = "ps_shadow",
    });
}
"@)
    $text = $text.Replace(@"
    auto d3d12_postprocess_bytecode = load_packaged_d3d12_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_postprocess_bytecode.ready() && options.require_postprocess && !options.require_vulkan_renderer) {
        std::cout << "$TargetName postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_postprocess_bytecode.status) << ": "
                  << d3d12_postprocess_bytecode.diagnostic << '\n';
        return 4;
    }

    auto vulkan_shader_bytecode = load_packaged_vulkan_scene_shaders(argc > 0 ? argv[0] : nullptr);
"@, @"
    auto d3d12_postprocess_bytecode = load_packaged_d3d12_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_postprocess_bytecode.ready() && options.require_postprocess && !options.require_vulkan_renderer) {
        std::cout << "$TargetName postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_postprocess_bytecode.status) << ": "
                  << d3d12_postprocess_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_morph_shader_bytecode = load_packaged_d3d12_scene_morph_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_morph_shader_bytecode.ready() && options.require_d3d12_scene_shaders) {
        std::cout << "$TargetName morph_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_morph_shader_bytecode.status) << ": "
                  << d3d12_morph_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_compute_morph_shader_bytecode =
        load_packaged_d3d12_scene_compute_morph_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_compute_morph_shader_bytecode.ready() &&
        (options.require_d3d12_scene_shaders || (options.require_compute_morph && !options.require_vulkan_renderer))) {
        std::cout << "$TargetName compute_morph_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_compute_morph_shader_bytecode.status) << ": "
                  << d3d12_compute_morph_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_compute_morph_tangent_frame_shader_bytecode =
        load_packaged_d3d12_scene_compute_morph_tangent_frame_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_compute_morph_tangent_frame_shader_bytecode.ready() &&
        (options.require_d3d12_scene_shaders ||
         (options.require_compute_morph_normal_tangent && !options.require_vulkan_renderer))) {
        std::cout << "$TargetName compute_morph_tangent_frame_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(
                         d3d12_compute_morph_tangent_frame_shader_bytecode.status)
                  << ": " << d3d12_compute_morph_tangent_frame_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_compute_morph_skinned_shader_bytecode =
        load_packaged_d3d12_scene_compute_morph_skinned_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_compute_morph_skinned_shader_bytecode.ready() &&
        (options.require_d3d12_scene_shaders || (options.require_compute_morph_skin && !options.require_vulkan_renderer))) {
        std::cout << "$TargetName compute_morph_skinned_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_compute_morph_skinned_shader_bytecode.status)
                  << ": " << d3d12_compute_morph_skinned_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_shadow_receiver_bytecode =
        load_packaged_d3d12_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_shadow_receiver_bytecode.ready() && options.require_directional_shadow &&
        !options.require_vulkan_renderer) {
        std::cout << "$TargetName shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_shadow_receiver_bytecode.status) << ": "
                  << d3d12_shadow_receiver_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_shifted_shadow_receiver_bytecode =
        load_packaged_d3d12_shifted_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_shifted_shadow_receiver_bytecode.ready() && options.require_shadow_morph_composition &&
        !options.require_vulkan_renderer) {
        std::cout << "$TargetName shifted_shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_shifted_shadow_receiver_bytecode.status)
                  << ": " << d3d12_shifted_shadow_receiver_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_shadow_bytecode = load_packaged_d3d12_shadow_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_shadow_bytecode.ready() && options.require_directional_shadow && !options.require_vulkan_renderer) {
        std::cout << "$TargetName shadow_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_shadow_bytecode.status) << ": "
                  << d3d12_shadow_bytecode.diagnostic << '\n';
        return 4;
    }

    auto vulkan_shader_bytecode = load_packaged_vulkan_scene_shaders(argc > 0 ? argv[0] : nullptr);
"@)
    $text = $text.Replace(@"
    auto vulkan_postprocess_bytecode = load_packaged_vulkan_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_postprocess_bytecode.ready() && options.require_postprocess && options.require_vulkan_renderer) {
        std::cout << "$TargetName vulkan_postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_postprocess_bytecode.status) << ": "
                  << vulkan_postprocess_bytecode.diagnostic << '\n';
        return 6;
    }

    std::optional<mirakana::SdlDesktopPresentationD3d12SceneRendererDesc> d3d12_scene_renderer;
"@, @"
    auto vulkan_postprocess_bytecode = load_packaged_vulkan_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_postprocess_bytecode.ready() && options.require_postprocess && options.require_vulkan_renderer) {
        std::cout << "$TargetName vulkan_postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_postprocess_bytecode.status) << ": "
                  << vulkan_postprocess_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_morph_shader_bytecode = load_packaged_vulkan_scene_morph_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_morph_shader_bytecode.ready() && options.require_vulkan_scene_shaders) {
        std::cout << "$TargetName vulkan_morph_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_morph_shader_bytecode.status) << ": "
                  << vulkan_morph_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_compute_morph_shader_bytecode =
        load_packaged_vulkan_scene_compute_morph_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_compute_morph_shader_bytecode.ready() && options.require_vulkan_renderer &&
        ((options.require_compute_morph && !options.require_compute_morph_normal_tangent &&
          !options.require_compute_morph_skin) ||
         options.require_directional_shadow)) {
        std::cout << "$TargetName vulkan_compute_morph_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_compute_morph_shader_bytecode.status)
                  << ": " << vulkan_compute_morph_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_compute_morph_tangent_frame_shader_bytecode =
        load_packaged_vulkan_scene_compute_morph_tangent_frame_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_compute_morph_tangent_frame_shader_bytecode.ready() && options.require_vulkan_renderer &&
        options.require_compute_morph_normal_tangent && !options.require_compute_morph_skin) {
        std::cout << "$TargetName vulkan_compute_morph_tangent_frame_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(
                         vulkan_compute_morph_tangent_frame_shader_bytecode.status)
                  << ": " << vulkan_compute_morph_tangent_frame_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_compute_morph_skinned_shader_bytecode =
        load_packaged_vulkan_scene_compute_morph_skinned_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_compute_morph_skinned_shader_bytecode.ready() && options.require_vulkan_renderer &&
        options.require_compute_morph_skin) {
        std::cout << "$TargetName vulkan_compute_morph_skinned_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_compute_morph_skinned_shader_bytecode.status)
                  << ": " << vulkan_compute_morph_skinned_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_shadow_receiver_bytecode =
        load_packaged_vulkan_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shadow_receiver_bytecode.ready() && options.require_directional_shadow &&
        options.require_vulkan_renderer) {
        std::cout << "$TargetName vulkan_shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shadow_receiver_bytecode.status) << ": "
                  << vulkan_shadow_receiver_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_shifted_shadow_receiver_bytecode =
        load_packaged_vulkan_shifted_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shifted_shadow_receiver_bytecode.ready() && options.require_shadow_morph_composition &&
        options.require_vulkan_renderer) {
        std::cout << "$TargetName vulkan_shifted_shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shifted_shadow_receiver_bytecode.status)
                  << ": " << vulkan_shifted_shadow_receiver_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_shadow_bytecode = load_packaged_vulkan_shadow_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shadow_bytecode.ready() && options.require_directional_shadow && options.require_vulkan_renderer) {
        std::cout << "$TargetName vulkan_shadow_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shadow_bytecode.status) << ": "
                  << vulkan_shadow_bytecode.diagnostic << '\n';
        return 6;
    }

    std::optional<mirakana::SdlDesktopPresentationD3d12SceneRendererDesc> d3d12_scene_renderer;
"@)
    $text = $text.Replace(
        "if (d3d12_shader_bytecode.ready() && d3d12_postprocess_bytecode.ready() && runtime_package.has_value() &&",
        "if (d3d12_shader_bytecode.ready() && d3d12_morph_shader_bytecode.ready() &&`n        (!options.require_compute_morph || d3d12_compute_morph_shader_bytecode.ready()) &&`n        (!options.require_compute_morph_normal_tangent || d3d12_compute_morph_tangent_frame_shader_bytecode.ready()) &&`n        (!options.require_compute_morph_skin || d3d12_compute_morph_skinned_shader_bytecode.ready()) &&`n        d3d12_postprocess_bytecode.ready() && runtime_package.has_value() &&")
    $text = $text.Replace(
        "if (vulkan_shader_bytecode.ready() && vulkan_postprocess_bytecode.ready() && runtime_package.has_value() &&",
        "if (vulkan_shader_bytecode.ready() && vulkan_morph_shader_bytecode.ready() &&`n        (!(options.require_compute_morph && !options.require_compute_morph_skin) ||`n         (options.require_compute_morph_normal_tangent ? vulkan_compute_morph_tangent_frame_shader_bytecode.ready()`n                                                       : vulkan_compute_morph_shader_bytecode.ready())) &&`n        (!options.require_compute_morph_skin || vulkan_compute_morph_skinned_shader_bytecode.ready()) &&`n        (!options.require_directional_shadow || vulkan_compute_morph_shader_bytecode.ready()) &&`n        vulkan_postprocess_bytecode.ready() && runtime_package.has_value() &&")
    $text = $text.Replace(@"
            .enable_postprocess = true,
        });
    }

    std::optional<mirakana::SdlDesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;
"@, @"
            .enable_postprocess = true,
        });
        if (options.require_compute_morph_skin) {
            d3d12_scene_renderer->skinned_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_compute_morph_skinned_shader_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{
                    d3d12_compute_morph_skinned_shader_bytecode.vertex_shader.bytecode.data(),
                    d3d12_compute_morph_skinned_shader_bytecode.vertex_shader.bytecode.size()},
            };
            d3d12_scene_renderer->compute_morph_skinned_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_compute_morph_skinned_shader_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{
                    d3d12_compute_morph_skinned_shader_bytecode.fragment_shader.bytecode.data(),
                    d3d12_compute_morph_skinned_shader_bytecode.fragment_shader.bytecode.size()},
            };
            d3d12_scene_renderer->skinned_vertex_buffers = runtime_compute_morph_skinned_scene_vertex_buffers();
            d3d12_scene_renderer->skinned_vertex_attributes = runtime_compute_morph_skinned_scene_vertex_attributes();
            d3d12_scene_renderer->compute_morph_skinned_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_skinned_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        } else if (options.require_compute_morph) {
            const auto& selected_compute_morph_shader_bytecode =
                options.require_compute_morph_normal_tangent ? d3d12_compute_morph_tangent_frame_shader_bytecode
                                                             : d3d12_compute_morph_shader_bytecode;
            d3d12_scene_renderer->compute_morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = selected_compute_morph_shader_bytecode.vertex_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{selected_compute_morph_shader_bytecode.vertex_shader.bytecode.data(),
                                                  selected_compute_morph_shader_bytecode.vertex_shader.bytecode.size()},
            };
            d3d12_scene_renderer->compute_morph_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = selected_compute_morph_shader_bytecode.fragment_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{selected_compute_morph_shader_bytecode.fragment_shader.bytecode.data(),
                                                  selected_compute_morph_shader_bytecode.fragment_shader.bytecode.size()},
            };
            d3d12_scene_renderer->enable_compute_morph_tangent_frame_output =
                options.require_compute_morph_normal_tangent;
            d3d12_scene_renderer->compute_morph_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        } else {
            d3d12_scene_renderer->morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_morph_shader_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{d3d12_morph_shader_bytecode.vertex_shader.bytecode.data(),
                                                          d3d12_morph_shader_bytecode.vertex_shader.bytecode.size()},
            };
            d3d12_scene_renderer->morph_mesh_assets = {packaged_morph_mesh_asset_id()};
            d3d12_scene_renderer->morph_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        }
    }

    std::optional<mirakana::SdlDesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;
"@)
    $text = $text.Replace(@"
            .enable_postprocess = true,
        });
    }

    mirakana::SdlDesktopGameHostDesc host_desc{
"@, @"
            .enable_postprocess = true,
        });
        if (options.require_compute_morph_skin) {
            vulkan_scene_renderer->skinned_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_compute_morph_skinned_shader_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{
                    vulkan_compute_morph_skinned_shader_bytecode.vertex_shader.bytecode.data(),
                    vulkan_compute_morph_skinned_shader_bytecode.vertex_shader.bytecode.size()},
            };
            vulkan_scene_renderer->compute_morph_skinned_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_compute_morph_skinned_shader_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{
                    vulkan_compute_morph_skinned_shader_bytecode.fragment_shader.bytecode.data(),
                    vulkan_compute_morph_skinned_shader_bytecode.fragment_shader.bytecode.size()},
            };
            vulkan_scene_renderer->skinned_vertex_buffers = runtime_compute_morph_skinned_scene_vertex_buffers();
            vulkan_scene_renderer->skinned_vertex_attributes = runtime_compute_morph_skinned_scene_vertex_attributes();
            vulkan_scene_renderer->compute_morph_skinned_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_skinned_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        } else if (options.require_compute_morph) {
            const auto& selected_compute_morph_shader_bytecode =
                options.require_compute_morph_normal_tangent ? vulkan_compute_morph_tangent_frame_shader_bytecode
                                                             : vulkan_compute_morph_shader_bytecode;
            vulkan_scene_renderer->compute_morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = selected_compute_morph_shader_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{
                    selected_compute_morph_shader_bytecode.vertex_shader.bytecode.data(),
                    selected_compute_morph_shader_bytecode.vertex_shader.bytecode.size()},
            };
            vulkan_scene_renderer->compute_morph_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = selected_compute_morph_shader_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{
                    selected_compute_morph_shader_bytecode.fragment_shader.bytecode.data(),
                    selected_compute_morph_shader_bytecode.fragment_shader.bytecode.size()},
            };
            vulkan_scene_renderer->enable_compute_morph_tangent_frame_output =
                options.require_compute_morph_normal_tangent;
            vulkan_scene_renderer->compute_morph_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        } else {
            vulkan_scene_renderer->morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_morph_shader_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{vulkan_morph_shader_bytecode.vertex_shader.bytecode.data(),
                                                          vulkan_morph_shader_bytecode.vertex_shader.bytecode.size()},
            };
            vulkan_scene_renderer->morph_mesh_assets = {packaged_morph_mesh_asset_id()};
            vulkan_scene_renderer->morph_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        }
    }

    mirakana::SdlDesktopGameHostDesc host_desc{
"@)
    $text = $text.Replace(@"
              << " scene_gpu_material_uploads=" << scene_gpu_stats.material_uploads
              << " scene_gpu_material_pipeline_layouts=" << scene_gpu_stats.material_pipeline_layouts
              << " scene_gpu_uploaded_texture_bytes=" << scene_gpu_stats.uploaded_texture_bytes
"@, @"
              << " scene_gpu_material_uploads=" << scene_gpu_stats.material_uploads
              << " scene_gpu_material_pipeline_layouts=" << scene_gpu_stats.material_pipeline_layouts
              << " scene_gpu_morph_mesh_bindings=" << scene_gpu_stats.morph_mesh_bindings
              << " scene_gpu_morph_mesh_uploads=" << scene_gpu_stats.morph_mesh_uploads
              << " scene_gpu_morph_mesh_resolved=" << scene_gpu_stats.morph_mesh_bindings_resolved
              << " scene_gpu_uploaded_morph_bytes=" << scene_gpu_stats.uploaded_morph_bytes
              << " renderer_gpu_morph_draws=" << report.renderer_stats.gpu_morph_draws
              << " renderer_morph_descriptor_binds=" << report.renderer_stats.morph_descriptor_binds
              << " scene_gpu_compute_morph_mesh_bindings=" << scene_gpu_stats.compute_morph_mesh_bindings
              << " scene_gpu_compute_morph_dispatches=" << scene_gpu_stats.compute_morph_mesh_dispatches
              << " scene_gpu_compute_morph_queue_waits=" << scene_gpu_stats.compute_morph_queue_waits
              << " scene_gpu_compute_morph_async_compute_queue_submits="
              << scene_gpu_stats.compute_morph_async_compute_queue_submits
              << " scene_gpu_compute_morph_async_graphics_queue_waits="
              << scene_gpu_stats.compute_morph_async_graphics_queue_waits
              << " scene_gpu_compute_morph_async_graphics_queue_submits="
              << scene_gpu_stats.compute_morph_async_graphics_queue_submits
              << " scene_gpu_compute_morph_async_last_compute_fence="
              << scene_gpu_stats.compute_morph_async_last_compute_submitted_fence_value
              << " scene_gpu_compute_morph_async_last_graphics_wait_fence="
              << scene_gpu_stats.compute_morph_async_last_graphics_queue_wait_fence_value
              << " scene_gpu_compute_morph_async_last_graphics_submit_fence="
              << scene_gpu_stats.compute_morph_async_last_graphics_submitted_fence_value
              << " scene_gpu_compute_morph_mesh_resolved=" << scene_gpu_stats.compute_morph_mesh_bindings_resolved
              << " scene_gpu_compute_morph_draws=" << scene_gpu_stats.compute_morph_mesh_draws
              << " scene_gpu_compute_morph_tangent_frame_output="
              << (options.require_compute_morph_normal_tangent && !options.require_compute_morph_skin ? 1 : 0)
              << " scene_gpu_compute_morph_skinned_mesh_bindings="
              << scene_gpu_stats.compute_morph_skinned_mesh_bindings
              << " scene_gpu_compute_morph_skinned_dispatches="
              << scene_gpu_stats.compute_morph_skinned_mesh_dispatches
              << " scene_gpu_compute_morph_skinned_queue_waits="
              << scene_gpu_stats.compute_morph_skinned_queue_waits
              << " scene_gpu_compute_morph_skinned_mesh_resolved="
              << scene_gpu_stats.compute_morph_skinned_mesh_bindings_resolved
              << " scene_gpu_compute_morph_skinned_draws=" << scene_gpu_stats.compute_morph_skinned_mesh_draws
              << " scene_gpu_compute_morph_output_position_bytes="
              << scene_gpu_stats.compute_morph_output_position_bytes
              << " renderer_gpu_skinning_draws=" << report.renderer_stats.gpu_skinning_draws
              << " renderer_skinned_palette_descriptor_binds="
              << report.renderer_stats.skinned_palette_descriptor_binds
              << " scene_gpu_uploaded_texture_bytes=" << scene_gpu_stats.uploaded_texture_bytes
"@)
    $text = $text.Replace(@"
        if (arg == "--require-postprocess") {
            options.require_postprocess = true;
            continue;
        }
"@, @"
        if (arg == "--require-postprocess") {
            options.require_postprocess = true;
            continue;
        }
        if (arg == "--require-postprocess-depth-input") {
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            continue;
        }
        if (arg == "--require-directional-shadow") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            continue;
        }
        if (arg == "--require-directional-shadow-filtering") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            continue;
        }
        if (arg == "--require-shadow-morph-composition") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            options.require_shadow_morph_composition = true;
            options.require_renderer_quality_gates = true;
            options.require_morph_package = true;
            continue;
        }
        if (arg == "--require-renderer-quality-gates") {
            options.require_renderer_quality_gates = true;
            continue;
        }
        if (arg == "--require-playable-3d-slice") {
            options.require_playable_3d_slice = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_renderer_quality_gates = true;
            options.require_primary_camera_controller = true;
            options.require_transform_animation = true;
            options.require_morph_package = true;
            options.require_quaternion_animation = true;
            options.require_package_streaming_safe_point = true;
            continue;
        }
        if (arg == "--require-primary-camera-controller") {
            options.require_primary_camera_controller = true;
            continue;
        }
        if (arg == "--require-transform-animation") {
            options.require_transform_animation = true;
            continue;
        }
        if (arg == "--require-morph-package") {
            options.require_morph_package = true;
            continue;
        }
        if (arg == "--require-compute-morph") {
            options.require_compute_morph = true;
            continue;
        }
        if (arg == "--require-compute-morph-normal-tangent") {
            options.require_compute_morph = true;
            options.require_compute_morph_normal_tangent = true;
            continue;
        }
        if (arg == "--require-compute-morph-skin") {
            options.require_compute_morph = true;
            options.require_compute_morph_skin = true;
            continue;
        }
        if (arg == "--require-compute-morph-async-telemetry") {
            options.require_compute_morph = true;
            options.require_compute_morph_async_telemetry = true;
            continue;
        }
        if (arg == "--require-quaternion-animation") {
            options.require_quaternion_animation = true;
            continue;
        }
        if (arg == "--require-package-streaming-safe-point") {
            options.require_package_streaming_safe_point = true;
            continue;
        }
"@)
    $text = $text.Replace(@"
              << " postprocess_status="
              << mirakana::sdl_desktop_presentation_postprocess_status_name(report.postprocess_status)
              << " framegraph_passes=" << report.framegraph_passes
              << " frames=" << result.frames_run << " game_frames=" << game.frames()
              << " scene_meshes=" << game.scene_meshes_submitted()
              << " scene_materials=" << game.scene_materials_resolved() << '\n';
"@, @"
              << " postprocess_status="
              << mirakana::sdl_desktop_presentation_postprocess_status_name(report.postprocess_status)
              << " framegraph_passes=" << report.framegraph_passes
              << " frames=" << result.frames_run << " game_frames=" << game.frames()
              << " scene_meshes=" << game.scene_meshes_submitted()
              << " scene_materials=" << game.scene_materials_resolved()
              << " scene_mesh_plan_meshes=" << game.scene_mesh_plan_meshes()
              << " scene_mesh_plan_draws=" << game.scene_mesh_plan_draws()
              << " scene_mesh_plan_unique_meshes=" << game.scene_mesh_plan_unique_meshes()
              << " scene_mesh_plan_unique_materials=" << game.scene_mesh_plan_unique_materials()
              << " scene_mesh_plan_diagnostics=" << game.scene_mesh_plan_diagnostics()
              << " camera_primary=" << (game.primary_camera_controller_passed(options.max_frames) ? 1 : 0)
              << " camera_controller_ticks=" << game.camera_controller_ticks()
              << " final_camera_x=" << game.final_camera_x() << '\n';
"@)
    $text = $text.Replace(@"
        if (!options.required_scene_package_path.empty() &&
            (game.scene_meshes_submitted() != static_cast<std::size_t>(options.max_frames) ||
             game.scene_materials_resolved() != static_cast<std::size_t>(options.max_frames))) {
            return 3;
        }
"@, @"
        if (!options.required_scene_package_path.empty() &&
            (game.scene_meshes_submitted() != static_cast<std::size_t>(options.max_frames) ||
             game.scene_materials_resolved() != static_cast<std::size_t>(options.max_frames) ||
             game.scene_mesh_plan_meshes() != static_cast<std::uint64_t>(options.max_frames) ||
             game.scene_mesh_plan_draws() != static_cast<std::uint64_t>(options.max_frames) ||
             game.scene_mesh_plan_unique_meshes() != static_cast<std::uint64_t>(options.max_frames) ||
             game.scene_mesh_plan_unique_materials() != static_cast<std::uint64_t>(options.max_frames) ||
             !game.scene_mesh_plan_succeeded() ||
             game.scene_mesh_plan_diagnostics() != 0)) {
            return 3;
        }
"@)
    $text = $text.Replace(@"
        if (options.require_scene_gpu_bindings &&
            (scene_gpu_stats.mesh_bindings == 0 || scene_gpu_stats.material_bindings == 0 ||
             scene_gpu_stats.mesh_bindings_resolved != static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.material_bindings_resolved != static_cast<std::size_t>(options.max_frames))) {
            return 3;
        }
"@, @"
        if (options.require_primary_camera_controller && !game.primary_camera_controller_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_scene_gpu_bindings &&
            (scene_gpu_stats.mesh_bindings == 0 || scene_gpu_stats.material_bindings == 0 ||
             scene_gpu_stats.mesh_bindings_resolved != static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.material_bindings_resolved != static_cast<std::size_t>(options.max_frames))) {
            return 3;
        }
"@)
    $text = $text.Replace(@"
[[nodiscard]] bool load_required_scene_package(const char* executable_path, std::string_view package_path,
                                               std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package,
                                               std::optional<mirakana::RuntimeSceneRenderInstance>& scene) {
"@, @"
[[nodiscard]] bool load_required_scene_package(const char* executable_path, std::string_view package_path,
                                               std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package,
                                               std::optional<mirakana::RuntimeSceneRenderInstance>& scene,
                                               std::optional<mirakana::AnimationFloatClipSourceDocument>& animation_clip,
                                               std::optional<mirakana::runtime::RuntimeMorphMeshCpuPayload>& morph_payload,
                                               std::optional<mirakana::AnimationFloatClipSourceDocument>& morph_animation_clip,
                                               std::vector<mirakana::AnimationJointTrack3dDesc>& quaternion_animation_tracks) {
"@)
    $text = $text.Replace(@"
        if (instance.material_palette.count() == 0) {
            std::cerr << "runtime scene package did not resolve scene materials: " << package_path << '\n';
            return false;
        }

        runtime_package = std::move(package_result.package);
        scene = std::move(instance);
"@, @"
        if (instance.material_palette.count() == 0) {
            std::cerr << "runtime scene package did not resolve scene materials: " << package_path << '\n';
            return false;
        }
        const auto* animation_record = package_result.package.find(packaged_animation_asset_id());
        if (animation_record == nullptr) {
            std::cerr << "runtime scene package did not include packaged transform animation clip: " << package_path
                      << '\n';
            return false;
        }
        const auto animation_payload = mirakana::runtime::runtime_animation_float_clip_payload(*animation_record);
        if (!animation_payload.succeeded()) {
            std::cerr << "runtime scene package transform animation clip is invalid: " << animation_payload.diagnostic
                      << '\n';
            return false;
        }
        const auto* morph_record = package_result.package.find(packaged_morph_mesh_asset_id());
        if (morph_record == nullptr) {
            std::cerr << "runtime scene package did not include packaged morph mesh CPU payload: " << package_path
                      << '\n';
            return false;
        }
        const auto morph_result = mirakana::runtime::runtime_morph_mesh_cpu_payload(*morph_record);
        if (!morph_result.succeeded()) {
            std::cerr << "runtime scene package morph mesh CPU payload is invalid: " << morph_result.diagnostic
                      << '\n';
            return false;
        }
        const auto* morph_animation_record = package_result.package.find(packaged_morph_animation_asset_id());
        if (morph_animation_record == nullptr) {
            std::cerr << "runtime scene package did not include packaged morph weight animation clip: " << package_path
                      << '\n';
            return false;
        }
        const auto morph_animation_result =
            mirakana::runtime::runtime_animation_float_clip_payload(*morph_animation_record);
        if (!morph_animation_result.succeeded()) {
            std::cerr << "runtime scene package morph weight animation clip is invalid: "
                      << morph_animation_result.diagnostic << '\n';
            return false;
        }
        const auto* quaternion_animation_record = package_result.package.find(packaged_quaternion_animation_asset_id());
        if (quaternion_animation_record == nullptr) {
            std::cerr << "runtime scene package did not include packaged quaternion animation clip: " << package_path
                      << '\n';
            return false;
        }
        const auto quaternion_payload =
            mirakana::runtime::runtime_animation_quaternion_clip_payload(*quaternion_animation_record);
        if (!quaternion_payload.succeeded()) {
            std::cerr << "runtime scene package quaternion animation clip is invalid: "
                      << quaternion_payload.diagnostic << '\n';
            return false;
        }
        auto decoded_quaternion_tracks = make_quaternion_animation_tracks(quaternion_payload.payload);
        if (!mirakana::is_valid_animation_joint_tracks_3d(packaged_quaternion_animation_skeleton(),
                                                    decoded_quaternion_tracks)) {
            std::cerr << "runtime scene package quaternion animation clip does not match its smoke skeleton: "
                      << package_path << '\n';
            return false;
        }

        animation_clip = std::move(animation_payload.payload.clip);
        morph_payload = std::move(morph_result.payload);
        morph_animation_clip = std::move(morph_animation_result.payload.clip);
        quaternion_animation_tracks = std::move(decoded_quaternion_tracks);
        runtime_package = std::move(package_result.package);
        scene = std::move(instance);
"@)
    $text = $text.Replace(@"
    return true;
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_scene_shaders(const char* executable_path) {
"@, @"
    return true;
}

[[nodiscard]] mirakana::runtime::RuntimePackageStreamingExecutionResult execute_package_streaming_safe_point_smoke(
    const mirakana::runtime::RuntimeAssetPackage& package, std::string_view package_path) {
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = package,
        .failures = {},
    };

    return mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        store, catalog,
        mirakana::runtime::RuntimePackageStreamingExecutionDesc{
            .target_id = "packaged-3d-residency-budget",
            .package_index_path = std::string{package_path},
            .content_root = {},
            .runtime_scene_validation_target_id = "packaged-3d-scene",
            .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point,
            .resident_budget_bytes = kPackageStreamingResidentBudgetBytes,
            .safe_point_required = true,
            .runtime_scene_validation_succeeded = true,
            .required_preload_assets = {packaged_scene_asset_id()},
            .resident_resource_kinds =
                {
                    mirakana::AssetKind::texture,
                    mirakana::AssetKind::mesh,
                    mirakana::AssetKind::skinned_mesh,
                    mirakana::AssetKind::morph_mesh_cpu,
                    mirakana::AssetKind::material,
                    mirakana::AssetKind::animation_float_clip,
                    mirakana::AssetKind::animation_quaternion_clip,
                    mirakana::AssetKind::scene,
                },
            .max_resident_packages = 1,
        },
        std::move(loaded_package));
}

void print_package_streaming_diagnostics(
    const mirakana::runtime::RuntimePackageStreamingExecutionResult& package_streaming_result) {
    for (const auto& diagnostic : package_streaming_result.diagnostics) {
        std::cout << "${TargetName} package_streaming_diagnostic=" << diagnostic.code
                  << ": " << diagnostic.message << '\n';
    }
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_scene_shaders(const char* executable_path) {
"@)
    $text = $text.Replace(@"
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
"@, @"
    std::optional<mirakana::runtime::RuntimeAssetPackage> runtime_package;
    std::optional<mirakana::RuntimeSceneRenderInstance> packaged_scene;
    std::optional<mirakana::AnimationFloatClipSourceDocument> packaged_animation_clip;
    std::optional<mirakana::runtime::RuntimeMorphMeshCpuPayload> packaged_morph_payload;
    std::optional<mirakana::AnimationFloatClipSourceDocument> packaged_morph_animation_clip;
    std::vector<mirakana::AnimationJointTrack3dDesc> packaged_quaternion_animation_tracks;
    if (!load_required_scene_package(argc > 0 ? argv[0] : nullptr, options.required_scene_package_path, runtime_package,
                                     packaged_scene, packaged_animation_clip, packaged_morph_payload,
                                     packaged_morph_animation_clip, packaged_quaternion_animation_tracks)) {
        return 4;
    }
    if (options.require_scene_gpu_bindings && (!runtime_package.has_value() || !packaged_scene.has_value())) {
        std::cerr << "--require-scene-gpu-bindings requires --require-scene-package\n";
        return 4;
    }
    if (options.require_transform_animation && (!packaged_scene.has_value() || !packaged_animation_clip.has_value())) {
        std::cerr << "--require-transform-animation requires --require-scene-package\n";
        return 4;
    }
    if (options.require_morph_package &&
        (!packaged_scene.has_value() || !packaged_morph_payload.has_value() ||
         !packaged_morph_animation_clip.has_value())) {
        std::cerr << "--require-morph-package requires --require-scene-package\n";
        return 4;
    }
    if (options.require_quaternion_animation && packaged_quaternion_animation_tracks.empty()) {
        std::cerr << "--require-quaternion-animation requires --require-scene-package\n";
        return 4;
    }
    if (options.require_package_streaming_safe_point &&
        (!runtime_package.has_value() || options.required_scene_package_path.empty())) {
        std::cerr << "--require-package-streaming-safe-point requires --require-scene-package\n";
        return 4;
    }
    if (options.require_compute_morph_skin && !activate_compute_morph_skinned_scene(packaged_scene)) {
        std::cerr << "--require-compute-morph-skin requires a packaged scene mesh node and skinned mesh package asset\n";
        return 4;
    }
    if (options.require_vulkan_renderer && options.require_compute_morph_async_telemetry) {
        std::cerr << "Vulkan compute morph package smoke does not support async telemetry requirements; use the D3D12 "
                     "lane for that package smoke\n";
        return 4;
    }
    mirakana::runtime::RuntimePackageStreamingExecutionResult package_streaming_result;
    if (options.require_package_streaming_safe_point) {
        package_streaming_result =
            execute_package_streaming_safe_point_smoke(*runtime_package, options.required_scene_package_path);
        if (!package_streaming_result.succeeded()) {
            print_package_streaming_diagnostics(package_streaming_result);
            return 4;
        }
    }
"@)
    $text = $text.Replace(@"
    GeneratedDesktopRuntime3DPackageGame game(host.input(), host.renderer(), options.throttle,
                                                std::move(packaged_scene));
"@, @"
    GeneratedDesktopRuntime3DPackageGame game(host.input(), host.renderer(), options.throttle,
                                              std::move(packaged_scene), std::move(packaged_animation_clip),
                                              packaged_animation_bindings(), std::move(packaged_morph_payload),
                                              std::move(packaged_morph_animation_clip),
                                              std::move(packaged_quaternion_animation_tracks));
"@)
    $text = $text.Replace(@"
              << " camera_primary=" << (game.primary_camera_controller_passed(options.max_frames) ? 1 : 0)
              << " camera_controller_ticks=" << game.camera_controller_ticks()
              << " final_camera_x=" << game.final_camera_x() << '\n';
"@, @"
              << " camera_primary=" << (game.primary_camera_controller_passed(options.max_frames) ? 1 : 0)
              << " camera_controller_ticks=" << game.camera_controller_ticks()
              << " final_camera_x=" << game.final_camera_x()
              << " package_streaming_status=" << package_streaming_status_name(package_streaming_result.status)
              << " package_streaming_ready=" << (package_streaming_result.succeeded() ? 1 : 0)
              << " package_streaming_resident_bytes=" << package_streaming_result.estimated_resident_bytes
              << " package_streaming_resident_budget_bytes=" << package_streaming_result.resident_budget_bytes
              << " package_streaming_committed_records="
              << package_streaming_result.replacement.committed_record_count
              << " package_streaming_stale_handles=" << package_streaming_result.replacement.stale_handle_count
              << " package_streaming_required_preload_assets=" << package_streaming_result.required_preload_asset_count
              << " package_streaming_resident_resource_kinds=" << package_streaming_result.resident_resource_kind_count
              << " package_streaming_resident_packages=" << package_streaming_result.resident_package_count
              << " package_streaming_diagnostics=" << package_streaming_result.diagnostics.size()
              << " transform_animation=" << (game.transform_animation_passed(options.max_frames) ? 1 : 0)
              << " transform_animation_ticks=" << game.transform_animation_ticks()
              << " transform_animation_samples=" << game.transform_animation_samples()
              << " transform_animation_applied=" << game.transform_animation_applied()
              << " transform_animation_failures=" << game.transform_animation_failures()
              << " final_mesh_x=" << game.final_mesh_x()
              << " morph_package=" << (game.morph_package_passed(options.max_frames) ? 1 : 0)
              << " morph_package_ticks=" << game.morph_package_ticks()
              << " morph_package_samples=" << game.morph_package_samples()
              << " morph_package_weights=" << game.morph_package_weights()
              << " morph_package_vertices=" << game.morph_package_vertices()
              << " morph_package_failures=" << game.morph_package_failures()
              << " morph_first_position_x=" << game.morph_first_position_x()
              << " quaternion_animation=" << (game.quaternion_animation_passed(options.max_frames) ? 1 : 0)
              << " quaternion_animation_ticks=" << game.quaternion_animation_ticks()
              << " quaternion_animation_tracks=" << game.quaternion_animation_tracks_sampled()
              << " quaternion_animation_failures=" << game.quaternion_animation_failures()
              << " quaternion_animation_scene_applied=" << game.quaternion_animation_scene_applied()
              << " quaternion_animation_final_z=" << game.final_quaternion_z()
              << " quaternion_animation_final_w=" << game.final_quaternion_w()
              << " quaternion_animation_scene_rotation_z=" << game.final_quaternion_scene_rotation_z() << '\n';
"@)
    $text = $text.Replace(@"
        if (options.require_primary_camera_controller && !game.primary_camera_controller_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_scene_gpu_bindings &&
"@, @"
        if (options.require_primary_camera_controller && !game.primary_camera_controller_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_transform_animation && !game.transform_animation_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_morph_package && !game.morph_package_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_quaternion_animation && !game.quaternion_animation_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_package_streaming_safe_point &&
            (!package_streaming_result.succeeded() || package_streaming_result.estimated_resident_bytes == 0 ||
             package_streaming_result.replacement.committed_record_count == 0 ||
             package_streaming_result.replacement.committed_record_count != runtime_package->records().size() ||
             package_streaming_result.required_preload_asset_count != 1 ||
             package_streaming_result.resident_resource_kind_count != 8 ||
             package_streaming_result.resident_package_count != 1 || package_streaming_result.diagnostics.size() != 0)) {
            return 3;
        }
        if (options.require_morph_package && options.require_scene_gpu_bindings && !options.require_compute_morph &&
            (scene_gpu_stats.morph_mesh_bindings < 1 || scene_gpu_stats.morph_mesh_uploads < 1 ||
             scene_gpu_stats.morph_mesh_bindings_resolved != static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.uploaded_morph_bytes == 0 ||
             report.renderer_stats.gpu_morph_draws != static_cast<std::uint64_t>(options.max_frames) ||
             report.renderer_stats.morph_descriptor_binds != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_compute_morph && !options.require_compute_morph_skin && options.require_scene_gpu_bindings &&
            (scene_gpu_stats.compute_morph_mesh_bindings < 1 ||
             scene_gpu_stats.compute_morph_mesh_dispatches < 1 ||
             scene_gpu_stats.compute_morph_queue_waits < 1 ||
             scene_gpu_stats.compute_morph_mesh_bindings_resolved != static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.compute_morph_mesh_draws != static_cast<std::size_t>(options.max_frames) ||
             report.renderer_stats.gpu_morph_draws != 0 ||
             report.renderer_stats.morph_descriptor_binds != 0)) {
            return 3;
        }
        if (options.require_compute_morph_skin && options.require_scene_gpu_bindings &&
            (scene_gpu_stats.compute_morph_skinned_mesh_bindings < 1 ||
             scene_gpu_stats.compute_morph_skinned_mesh_dispatches < 1 ||
             scene_gpu_stats.compute_morph_skinned_queue_waits < 1 ||
             scene_gpu_stats.compute_morph_skinned_mesh_bindings_resolved !=
                 static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.compute_morph_skinned_mesh_draws != static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.compute_morph_output_position_bytes == 0 ||
             report.renderer_stats.gpu_skinning_draws != static_cast<std::uint64_t>(options.max_frames) ||
             report.renderer_stats.skinned_palette_descriptor_binds !=
                 static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_compute_morph_async_telemetry && options.require_scene_gpu_bindings &&
            (scene_gpu_stats.compute_morph_async_compute_queue_submits < 1 ||
             scene_gpu_stats.compute_morph_async_graphics_queue_waits < 1 ||
             scene_gpu_stats.compute_morph_async_graphics_queue_submits < 1 ||
             scene_gpu_stats.compute_morph_async_last_compute_submitted_fence_value == 0 ||
             scene_gpu_stats.compute_morph_async_last_graphics_queue_wait_fence_value !=
                 scene_gpu_stats.compute_morph_async_last_compute_submitted_fence_value ||
             scene_gpu_stats.compute_morph_async_last_graphics_submitted_fence_value == 0)) {
            return 3;
        }
        if (options.require_scene_gpu_bindings &&
"@)
    $text = $text.Replace(@"
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "${TargetName} presentation_diagnostic="
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": " << diagnostic.message
                  << '\n';
    }

    if (options.smoke) {
"@, @"
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "${TargetName} presentation_diagnostic="
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": " << diagnostic.message
                  << '\n';
    }
    print_package_streaming_diagnostics(package_streaming_result);

    if (options.smoke) {
"@)
    $text = $text.Replace(@"
        if (options.require_scene_gpu_bindings &&
            (scene_gpu_stats.mesh_bindings == 0 || scene_gpu_stats.material_bindings == 0 ||
             scene_gpu_stats.mesh_bindings_resolved != static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.material_bindings_resolved != static_cast<std::size_t>(options.max_frames))) {
            return 3;
        }
"@, @"
        if (options.require_scene_gpu_bindings &&
            ((scene_gpu_stats.mesh_bindings == 0 && scene_gpu_stats.skinned_mesh_bindings == 0) ||
             scene_gpu_stats.material_bindings == 0 ||
             (scene_gpu_stats.mesh_bindings_resolved + scene_gpu_stats.skinned_mesh_bindings_resolved) !=
                 static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.material_bindings_resolved != static_cast<std::size_t>(options.max_frames))) {
            return 3;
        }
"@)
    $text = $text.Replace(@"
        if (options.require_postprocess &&
            (report.postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready ||
             report.framegraph_passes != 2 ||
             report.renderer_stats.framegraph_passes_executed != static_cast<std::uint64_t>(options.max_frames) * 2U ||
             report.renderer_stats.postprocess_passes_executed != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
"@, @"
        if (options.require_postprocess &&
            (report.postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready ||
             report.framegraph_passes != 2 ||
             report.renderer_stats.framegraph_passes_executed != static_cast<std::uint64_t>(options.max_frames) * 2U ||
             report.renderer_stats.postprocess_passes_executed != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_postprocess_depth_input && !report.postprocess_depth_input_ready) {
            return 3;
        }
"@)

    $text = $text.Replace(@"
void print_package_failures(const std::vector<mirakana::runtime::RuntimeAssetPackageLoadFailure>& failures) {
"@, @"
[[nodiscard]] mirakana::SdlDesktopPresentationQualityGateDesc
make_renderer_quality_gate_desc(const DesktopRuntimeOptions& options) noexcept {
    mirakana::SdlDesktopPresentationQualityGateDesc desc;
    if (options.require_renderer_quality_gates) {
        desc.require_scene_gpu_bindings = true;
        desc.require_postprocess = true;
        desc.require_postprocess_depth_input = options.require_postprocess_depth_input;
        desc.expected_frames = options.max_frames;
    }
    return desc;
}

enum class Playable3dSliceStatus {
    not_requested,
    ready,
    diagnostics,
};

[[nodiscard]] std::string_view playable_3d_slice_status_name(Playable3dSliceStatus status) noexcept {
    switch (status) {
    case Playable3dSliceStatus::not_requested:
        return "not_requested";
    case Playable3dSliceStatus::ready:
        return "ready";
    case Playable3dSliceStatus::diagnostics:
        return "diagnostics";
    }
    return "unknown";
}

struct Playable3dSliceReport {
    Playable3dSliceStatus status{Playable3dSliceStatus::not_requested};
    bool ready{false};
    std::size_t diagnostics_count{0};
    std::uint32_t expected_frames{0};
    bool frames_ok{false};
    bool game_frames_ok{false};
    bool scene_mesh_plan_ready{false};
    bool camera_controller_ready{false};
    bool animation_ready{false};
    bool morph_ready{false};
    bool quaternion_ready{false};
    bool package_streaming_ready{false};
    bool scene_gpu_ready{false};
    bool postprocess_ready{false};
    bool renderer_quality_ready{false};
    bool compute_morph_selected{false};
    bool compute_morph_ready{false};
    bool compute_morph_normal_tangent_selected{false};
    bool compute_morph_normal_tangent_ready{false};
    bool compute_morph_skin_selected{false};
    bool compute_morph_skin_ready{false};
    bool compute_morph_async_telemetry_selected{false};
    bool compute_morph_async_telemetry_ready{false};
};

[[nodiscard]] bool package_streaming_smoke_ready(
    const mirakana::runtime::RuntimePackageStreamingExecutionResult& package_streaming_result,
    std::size_t expected_records) noexcept {
    return package_streaming_result.succeeded() && package_streaming_result.estimated_resident_bytes > 0 &&
           package_streaming_result.replacement.committed_record_count > 0 &&
           package_streaming_result.replacement.committed_record_count == expected_records &&
           package_streaming_result.required_preload_asset_count == 1 &&
           package_streaming_result.resident_resource_kind_count == 8 &&
           package_streaming_result.resident_package_count == 1 && package_streaming_result.diagnostics.empty();
}

[[nodiscard]] bool scene_mesh_plan_ready(const GeneratedDesktopRuntime3DPackageGame& game,
                                         std::uint32_t expected_frames) noexcept {
    return game.scene_meshes_submitted() == static_cast<std::size_t>(expected_frames) &&
           game.scene_materials_resolved() == static_cast<std::size_t>(expected_frames) &&
           game.scene_mesh_plan_meshes() == static_cast<std::uint64_t>(expected_frames) &&
           game.scene_mesh_plan_draws() == static_cast<std::uint64_t>(expected_frames) &&
           game.scene_mesh_plan_unique_meshes() == static_cast<std::uint64_t>(expected_frames) &&
           game.scene_mesh_plan_unique_materials() == static_cast<std::uint64_t>(expected_frames) &&
           game.scene_mesh_plan_succeeded() && game.scene_mesh_plan_diagnostics() == 0;
}

[[nodiscard]] bool scene_gpu_ready(const mirakana::SdlDesktopPresentationReport& report,
                                   std::uint32_t expected_frames) noexcept {
    const auto& stats = report.scene_gpu_stats;
    return (stats.mesh_bindings > 0 || stats.skinned_mesh_bindings > 0) && stats.material_bindings > 0 &&
           (stats.mesh_bindings_resolved + stats.skinned_mesh_bindings_resolved) ==
               static_cast<std::size_t>(expected_frames) &&
           stats.material_bindings_resolved == static_cast<std::size_t>(expected_frames);
}

[[nodiscard]] bool postprocess_ready(const mirakana::SdlDesktopPresentationReport& report,
                                     std::uint32_t expected_frames) noexcept {
    return report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::ready &&
           report.framegraph_passes == 2 &&
           report.renderer_stats.framegraph_passes_executed == static_cast<std::uint64_t>(expected_frames) * 2U &&
           report.renderer_stats.postprocess_passes_executed == static_cast<std::uint64_t>(expected_frames);
}

[[nodiscard]] bool compute_morph_ready(const mirakana::SdlDesktopPresentationReport& report,
                                       const DesktopRuntimeOptions& options) noexcept {
    const auto& stats = report.scene_gpu_stats;
    if (!options.require_compute_morph || options.require_compute_morph_skin) {
        return false;
    }
    return stats.compute_morph_mesh_bindings > 0 && stats.compute_morph_mesh_dispatches > 0 &&
           stats.compute_morph_queue_waits > 0 &&
           stats.compute_morph_mesh_bindings_resolved == static_cast<std::size_t>(options.max_frames) &&
           stats.compute_morph_mesh_draws == static_cast<std::size_t>(options.max_frames) &&
           report.renderer_stats.gpu_morph_draws == 0 && report.renderer_stats.morph_descriptor_binds == 0;
}

[[nodiscard]] bool compute_morph_skin_ready(const mirakana::SdlDesktopPresentationReport& report,
                                            const DesktopRuntimeOptions& options) noexcept {
    const auto& stats = report.scene_gpu_stats;
    if (!options.require_compute_morph_skin) {
        return false;
    }
    return stats.compute_morph_skinned_mesh_bindings > 0 && stats.compute_morph_skinned_mesh_dispatches > 0 &&
           stats.compute_morph_skinned_queue_waits > 0 &&
           stats.compute_morph_skinned_mesh_bindings_resolved == static_cast<std::size_t>(options.max_frames) &&
           stats.compute_morph_skinned_mesh_draws == static_cast<std::size_t>(options.max_frames) &&
           stats.compute_morph_output_position_bytes > 0 &&
           report.renderer_stats.gpu_skinning_draws == static_cast<std::uint64_t>(options.max_frames) &&
           report.renderer_stats.skinned_palette_descriptor_binds == static_cast<std::uint64_t>(options.max_frames);
}

[[nodiscard]] bool compute_morph_async_telemetry_ready(const mirakana::SdlDesktopPresentationReport& report) noexcept {
    const auto& stats = report.scene_gpu_stats;
    return stats.compute_morph_async_compute_queue_submits > 0 &&
           stats.compute_morph_async_graphics_queue_waits > 0 &&
           stats.compute_morph_async_graphics_queue_submits > 0 &&
           stats.compute_morph_async_last_compute_submitted_fence_value > 0 &&
           stats.compute_morph_async_last_graphics_queue_wait_fence_value ==
               stats.compute_morph_async_last_compute_submitted_fence_value &&
           stats.compute_morph_async_last_graphics_submitted_fence_value > 0;
}

[[nodiscard]] Playable3dSliceReport evaluate_playable_3d_slice(
    const DesktopRuntimeOptions& options,
    const mirakana::DesktopRunResult& result,
    const GeneratedDesktopRuntime3DPackageGame& game,
    const mirakana::runtime::RuntimePackageStreamingExecutionResult& package_streaming_result,
    std::size_t expected_package_records,
    const mirakana::SdlDesktopPresentationReport& presentation_report,
    const mirakana::SdlDesktopPresentationQualityGateReport& renderer_quality) noexcept {
    Playable3dSliceReport report;
    report.expected_frames = options.max_frames;
    report.frames_ok = result.status == mirakana::DesktopRunStatus::completed && result.frames_run == options.max_frames;
    report.game_frames_ok = game.frames() == options.max_frames;
    report.scene_mesh_plan_ready = scene_mesh_plan_ready(game, options.max_frames);
    report.camera_controller_ready = game.primary_camera_controller_passed(options.max_frames);
    report.animation_ready = game.transform_animation_passed(options.max_frames);
    report.morph_ready = game.morph_package_passed(options.max_frames);
    report.quaternion_ready = game.quaternion_animation_passed(options.max_frames);
    report.package_streaming_ready = package_streaming_smoke_ready(package_streaming_result, expected_package_records);
    report.scene_gpu_ready = scene_gpu_ready(presentation_report, options.max_frames);
    report.postprocess_ready = postprocess_ready(presentation_report, options.max_frames);
    report.renderer_quality_ready = renderer_quality.ready;
    report.compute_morph_selected = options.require_compute_morph;
    report.compute_morph_skin_selected = options.require_compute_morph_skin;
    report.compute_morph_normal_tangent_selected =
        options.require_compute_morph_normal_tangent && !options.require_compute_morph_skin;
    report.compute_morph_async_telemetry_selected = options.require_compute_morph_async_telemetry;
    report.compute_morph_skin_ready = compute_morph_skin_ready(presentation_report, options);
    report.compute_morph_ready =
        options.require_compute_morph_skin ? report.compute_morph_skin_ready : compute_morph_ready(presentation_report, options);
    report.compute_morph_normal_tangent_ready =
        report.compute_morph_normal_tangent_selected && report.compute_morph_ready;
    report.compute_morph_async_telemetry_ready =
        options.require_compute_morph_async_telemetry && compute_morph_async_telemetry_ready(presentation_report);

    if (!options.require_playable_3d_slice) {
        return report;
    }

    const bool required_checks[] = {
        report.frames_ok,
        report.game_frames_ok,
        report.scene_mesh_plan_ready,
        report.camera_controller_ready,
        report.animation_ready,
        report.morph_ready,
        report.quaternion_ready,
        report.package_streaming_ready,
        report.scene_gpu_ready,
        report.postprocess_ready,
        report.renderer_quality_ready,
    };
    for (const bool check : required_checks) {
        if (!check) {
            ++report.diagnostics_count;
        }
    }
    if (report.compute_morph_selected && !report.compute_morph_ready) {
        ++report.diagnostics_count;
    }
    if (report.compute_morph_normal_tangent_selected && !report.compute_morph_normal_tangent_ready) {
        ++report.diagnostics_count;
    }
    if (report.compute_morph_skin_selected && !report.compute_morph_skin_ready) {
        ++report.diagnostics_count;
    }
    if (report.compute_morph_async_telemetry_selected && !report.compute_morph_async_telemetry_ready) {
        ++report.diagnostics_count;
    }
    report.ready = report.diagnostics_count == 0;
    report.status = report.ready ? Playable3dSliceStatus::ready : Playable3dSliceStatus::diagnostics;
    return report;
}

void print_package_failures(const std::vector<mirakana::runtime::RuntimeAssetPackageLoadFailure>& failures) {
"@)
    $text = $text.Replace(
        "    const auto scene_gpu_stats = report.scene_gpu_stats;`n",
        "    const auto scene_gpu_stats = report.scene_gpu_stats;`n    const auto renderer_quality =`n        mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, make_renderer_quality_gate_desc(options));`n    const auto playable_3d = evaluate_playable_3d_slice(`n        options, result, game, package_streaming_result, runtime_package ? runtime_package->records().size() : 0U,`n        report, renderer_quality);`n")
    $text = $text.Replace(@"
              << " framegraph_passes=" << report.framegraph_passes
              << " frames=" << result.frames_run
"@, @"
              << " postprocess_depth_input_ready=" << (report.postprocess_depth_input_ready ? 1 : 0)
              << " framegraph_passes=" << report.framegraph_passes
              << " renderer_quality_status="
              << mirakana::sdl_desktop_presentation_quality_gate_status_name(renderer_quality.status)
              << " renderer_quality_ready=" << (renderer_quality.ready ? 1 : 0)
              << " renderer_quality_diagnostics=" << renderer_quality.diagnostics_count
              << " renderer_quality_expected_framegraph_passes="
              << renderer_quality.expected_framegraph_passes
              << " renderer_quality_framegraph_passes_ok="
              << (renderer_quality.framegraph_passes_current ? 1 : 0)
              << " renderer_quality_framegraph_execution_budget_ok="
              << (renderer_quality.framegraph_execution_budget_current ? 1 : 0)
              << " renderer_quality_scene_gpu_ready=" << (renderer_quality.scene_gpu_ready ? 1 : 0)
              << " renderer_quality_postprocess_ready=" << (renderer_quality.postprocess_ready ? 1 : 0)
              << " renderer_quality_postprocess_depth_input_ready="
              << (renderer_quality.postprocess_depth_input_ready ? 1 : 0)
              << " renderer_quality_directional_shadow_ready="
              << (renderer_quality.directional_shadow_ready ? 1 : 0)
              << " renderer_quality_directional_shadow_filter_ready="
              << (renderer_quality.directional_shadow_filter_ready ? 1 : 0)
              << " playable_3d_status=" << playable_3d_slice_status_name(playable_3d.status)
              << " playable_3d_ready=" << (playable_3d.ready ? 1 : 0)
              << " playable_3d_diagnostics=" << playable_3d.diagnostics_count
              << " playable_3d_expected_frames=" << playable_3d.expected_frames
              << " playable_3d_frames_ok=" << (playable_3d.frames_ok ? 1 : 0)
              << " playable_3d_game_frames_ok=" << (playable_3d.game_frames_ok ? 1 : 0)
              << " playable_3d_scene_mesh_plan_ready=" << (playable_3d.scene_mesh_plan_ready ? 1 : 0)
              << " playable_3d_camera_controller_ready=" << (playable_3d.camera_controller_ready ? 1 : 0)
              << " playable_3d_animation_ready=" << (playable_3d.animation_ready ? 1 : 0)
              << " playable_3d_morph_ready=" << (playable_3d.morph_ready ? 1 : 0)
              << " playable_3d_quaternion_ready=" << (playable_3d.quaternion_ready ? 1 : 0)
              << " playable_3d_package_streaming_ready=" << (playable_3d.package_streaming_ready ? 1 : 0)
              << " playable_3d_scene_gpu_ready=" << (playable_3d.scene_gpu_ready ? 1 : 0)
              << " playable_3d_postprocess_ready=" << (playable_3d.postprocess_ready ? 1 : 0)
              << " playable_3d_renderer_quality_ready=" << (playable_3d.renderer_quality_ready ? 1 : 0)
              << " playable_3d_compute_morph_selected=" << (playable_3d.compute_morph_selected ? 1 : 0)
              << " playable_3d_compute_morph_ready=" << (playable_3d.compute_morph_ready ? 1 : 0)
              << " playable_3d_compute_morph_normal_tangent_selected="
              << (playable_3d.compute_morph_normal_tangent_selected ? 1 : 0)
              << " playable_3d_compute_morph_normal_tangent_ready="
              << (playable_3d.compute_morph_normal_tangent_ready ? 1 : 0)
              << " playable_3d_compute_morph_skin_selected="
              << (playable_3d.compute_morph_skin_selected ? 1 : 0)
              << " playable_3d_compute_morph_skin_ready=" << (playable_3d.compute_morph_skin_ready ? 1 : 0)
              << " playable_3d_compute_morph_async_telemetry_selected="
              << (playable_3d.compute_morph_async_telemetry_selected ? 1 : 0)
              << " playable_3d_compute_morph_async_telemetry_ready="
              << (playable_3d.compute_morph_async_telemetry_ready ? 1 : 0)
              << " frames=" << result.frames_run
"@)
    $text = $text.Replace(@"
        if (options.require_package_streaming_safe_point &&
            (!package_streaming_result.succeeded() || package_streaming_result.estimated_resident_bytes == 0 ||
             package_streaming_result.replacement.committed_record_count == 0 ||
             package_streaming_result.replacement.committed_record_count != runtime_package->records().size() ||
             package_streaming_result.required_preload_asset_count != 1 ||
             package_streaming_result.resident_resource_kind_count != 8 ||
             package_streaming_result.resident_package_count != 1 || package_streaming_result.diagnostics.size() != 0)) {
            return 3;
        }
        if (options.require_morph_package && options.require_scene_gpu_bindings && !options.require_compute_morph &&
"@, @"
        if (options.require_package_streaming_safe_point &&
            (!package_streaming_result.succeeded() || package_streaming_result.estimated_resident_bytes == 0 ||
             package_streaming_result.replacement.committed_record_count == 0 ||
             package_streaming_result.replacement.committed_record_count != runtime_package->records().size() ||
             package_streaming_result.required_preload_asset_count != 1 ||
             package_streaming_result.resident_resource_kind_count != 8 ||
             package_streaming_result.resident_package_count != 1 || package_streaming_result.diagnostics.size() != 0)) {
            return 3;
        }
        if (options.require_renderer_quality_gates && !renderer_quality.ready) {
            return 3;
        }
        if (options.require_playable_3d_slice && !playable_3d.ready) {
            return 3;
        }
        if (options.require_morph_package && options.require_scene_gpu_bindings && !options.require_compute_morph &&
"@)

    $text = $text.Replace(
        "            .enable_postprocess = true,`n",
        "            .enable_postprocess = true,`n            .enable_postprocess_depth_input = options.require_postprocess_depth_input,`n")
    $text = $text.Replace(
        "        desc.require_postprocess_depth_input = options.require_postprocess_depth_input;`n        desc.expected_frames = options.max_frames;",
        "        desc.require_postprocess_depth_input = options.require_postprocess_depth_input;`n        desc.require_directional_shadow = options.require_directional_shadow;`n        desc.require_directional_shadow_filtering = options.require_directional_shadow_filtering;`n        desc.expected_frames = options.max_frames;")
    $text = $text.Replace(
        "    std::optional<mirakana::SdlDesktopPresentationD3d12SceneRendererDesc> d3d12_scene_renderer;`n    if (d3d12_shader_bytecode.ready() && d3d12_morph_shader_bytecode.ready() &&",
        "    std::optional<mirakana::SdlDesktopPresentationD3d12SceneRendererDesc> d3d12_scene_renderer;`n    const auto& d3d12_scene_bytecode =`n        options.require_directional_shadow ? d3d12_shadow_receiver_bytecode : d3d12_shader_bytecode;`n    const bool require_graphics_morph_scene =`n        options.require_morph_package && options.require_scene_gpu_bindings && !options.require_compute_morph;`n    const bool d3d12_morph_ready = !require_graphics_morph_scene || d3d12_morph_shader_bytecode.ready();`n    const bool d3d12_shadow_ready = !options.require_directional_shadow || d3d12_shadow_bytecode.ready();`n    if (d3d12_scene_bytecode.ready() && d3d12_morph_ready && d3d12_shadow_ready &&")
    $text = $text.Replace("d3d12_shader_bytecode.vertex_shader", "d3d12_scene_bytecode.vertex_shader")
    $text = $text.Replace("d3d12_shader_bytecode.fragment_shader", "d3d12_scene_bytecode.fragment_shader")
    $text = $text.Replace(
        "        } else {`n            d3d12_scene_renderer->morph_vertex_shader",
        "        } else if (options.require_morph_package) {`n            d3d12_scene_renderer->morph_vertex_shader")
    $text = $text.Replace(
        "            d3d12_scene_renderer->morph_mesh_bindings = {`n                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),`n                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},`n            };`n        }`n    }`n`n    std::optional<mirakana::SdlDesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;",
        "            d3d12_scene_renderer->morph_mesh_bindings = {`n                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),`n                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},`n            };`n        }`n        if (options.require_directional_shadow) {`n            if (require_graphics_morph_scene) {`n                d3d12_scene_renderer->skinned_scene_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{`n                    .entry_point = d3d12_shifted_shadow_receiver_bytecode.fragment_shader.entry_point,`n                    .bytecode = std::span<const std::uint8_t>{`n                        d3d12_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.data(),`n                        d3d12_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.size()},`n                };`n            }`n            d3d12_scene_renderer->shadow_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{`n                .entry_point = d3d12_shadow_bytecode.vertex_shader.entry_point,`n                .bytecode = std::span<const std::uint8_t>{d3d12_shadow_bytecode.vertex_shader.bytecode.data(),`n                                                          d3d12_shadow_bytecode.vertex_shader.bytecode.size()},`n            };`n            d3d12_scene_renderer->shadow_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{`n                .entry_point = d3d12_shadow_bytecode.fragment_shader.entry_point,`n                .bytecode = std::span<const std::uint8_t>{d3d12_shadow_bytecode.fragment_shader.bytecode.data(),`n                                                          d3d12_shadow_bytecode.fragment_shader.bytecode.size()},`n            };`n        }`n    }`n`n    std::optional<mirakana::SdlDesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;")
    $d3d12ShadowPattern = '(?s)(        \} else if \(options\.require_morph_package\) \{\r?\n            d3d12_scene_renderer->morph_vertex_shader.*?            \};\r?\n        \})(\r?\n    \}\r?\n\r?\n    std::optional<mirakana::SdlDesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;)'
    $d3d12ShadowReplacement = @'
${1}
        if (options.require_directional_shadow) {
            if (require_graphics_morph_scene) {
                d3d12_scene_renderer->skinned_scene_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = d3d12_shifted_shadow_receiver_bytecode.fragment_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{
                        d3d12_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.data(),
                        d3d12_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.size()},
                };
            }
            d3d12_scene_renderer->shadow_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_shadow_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{d3d12_shadow_bytecode.vertex_shader.bytecode.data(),
                                                          d3d12_shadow_bytecode.vertex_shader.bytecode.size()},
            };
            d3d12_scene_renderer->shadow_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_shadow_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{d3d12_shadow_bytecode.fragment_shader.bytecode.data(),
                                                          d3d12_shadow_bytecode.fragment_shader.bytecode.size()},
            };
        }${2}
'@
    $text = [regex]::Replace($text, $d3d12ShadowPattern, $d3d12ShadowReplacement, 1)
    $text = $text.Replace(
        "    std::optional<mirakana::SdlDesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;`n    if (vulkan_shader_bytecode.ready() && vulkan_morph_shader_bytecode.ready() &&",
        "    std::optional<mirakana::SdlDesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;`n    const auto& vulkan_scene_bytecode =`n        options.require_directional_shadow ? vulkan_shadow_receiver_bytecode : vulkan_shader_bytecode;`n    const bool vulkan_morph_ready = !require_graphics_morph_scene || vulkan_morph_shader_bytecode.ready();`n    const bool vulkan_shadow_ready = !options.require_directional_shadow || vulkan_shadow_bytecode.ready();`n    if (vulkan_scene_bytecode.ready() && vulkan_morph_ready && vulkan_shadow_ready &&")
    $text = $text.Replace("vulkan_shader_bytecode.vertex_shader", "vulkan_scene_bytecode.vertex_shader")
    $text = $text.Replace("vulkan_shader_bytecode.fragment_shader", "vulkan_scene_bytecode.fragment_shader")
    $text = $text.Replace(
        "        } else {`n            vulkan_scene_renderer->morph_vertex_shader",
        "        } else if (options.require_morph_package) {`n            vulkan_scene_renderer->morph_vertex_shader")
    $text = $text.Replace(
        "            vulkan_scene_renderer->morph_mesh_bindings = {`n                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),`n                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},`n            };`n        }`n    }`n`n    mirakana::SdlDesktopGameHostDesc host_desc{",
        "            vulkan_scene_renderer->morph_mesh_bindings = {`n                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),`n                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},`n            };`n        }`n        if (options.require_directional_shadow) {`n            if (require_graphics_morph_scene) {`n                vulkan_scene_renderer->skinned_scene_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{`n                    .entry_point = vulkan_shifted_shadow_receiver_bytecode.fragment_shader.entry_point,`n                    .bytecode = std::span<const std::uint8_t>{`n                        vulkan_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.data(),`n                        vulkan_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.size()},`n                };`n            }`n            vulkan_scene_renderer->compute_morph_shader = mirakana::SdlDesktopPresentationShaderBytecode{`n                .entry_point = vulkan_compute_morph_shader_bytecode.fragment_shader.entry_point,`n                .bytecode = std::span<const std::uint8_t>{`n                    vulkan_compute_morph_shader_bytecode.fragment_shader.bytecode.data(),`n                    vulkan_compute_morph_shader_bytecode.fragment_shader.bytecode.size()},`n            };`n            vulkan_scene_renderer->shadow_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{`n                .entry_point = vulkan_shadow_bytecode.vertex_shader.entry_point,`n                .bytecode = std::span<const std::uint8_t>{vulkan_shadow_bytecode.vertex_shader.bytecode.data(),`n                                                          vulkan_shadow_bytecode.vertex_shader.bytecode.size()},`n            };`n            vulkan_scene_renderer->shadow_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{`n                .entry_point = vulkan_shadow_bytecode.fragment_shader.entry_point,`n                .bytecode = std::span<const std::uint8_t>{vulkan_shadow_bytecode.fragment_shader.bytecode.data(),`n                                                          vulkan_shadow_bytecode.fragment_shader.bytecode.size()},`n            };`n        }`n    }`n`n    mirakana::SdlDesktopGameHostDesc host_desc{")
    $vulkanShadowPattern = '(?s)(        \} else if \(options\.require_morph_package\) \{\r?\n            vulkan_scene_renderer->morph_vertex_shader.*?            \};\r?\n        \})(\r?\n    \}\r?\n\r?\n    mirakana::SdlDesktopGameHostDesc host_desc\{)'
    $vulkanShadowReplacement = @'
${1}
        if (options.require_directional_shadow) {
            if (require_graphics_morph_scene) {
                vulkan_scene_renderer->skinned_scene_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = vulkan_shifted_shadow_receiver_bytecode.fragment_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{
                        vulkan_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.data(),
                        vulkan_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.size()},
                };
            }
            vulkan_scene_renderer->compute_morph_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_compute_morph_shader_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{
                    vulkan_compute_morph_shader_bytecode.fragment_shader.bytecode.data(),
                    vulkan_compute_morph_shader_bytecode.fragment_shader.bytecode.size()},
            };
            vulkan_scene_renderer->shadow_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_shadow_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{vulkan_shadow_bytecode.vertex_shader.bytecode.data(),
                                                          vulkan_shadow_bytecode.vertex_shader.bytecode.size()},
            };
            vulkan_scene_renderer->shadow_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_shadow_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{vulkan_shadow_bytecode.fragment_shader.bytecode.data(),
                                                          vulkan_shadow_bytecode.fragment_shader.bytecode.size()},
            };
        }${2}
'@
    $text = [regex]::Replace($text, $vulkanShadowPattern, $vulkanShadowReplacement, 1)
    $text = $text.Replace(
        "            .enable_postprocess_depth_input = options.require_postprocess_depth_input,`n        });",
        "            .enable_postprocess_depth_input = options.require_postprocess_depth_input,`n            .enable_directional_shadow_smoke = options.require_directional_shadow,`n        });")
    $text = $text.Replace(
        "    if (options.require_postprocess &&`n        host.presentation_report().postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready) {`n        std::cout << `"$TargetName required_postprocess_unavailable status=`"`n                  << mirakana::sdl_desktop_presentation_postprocess_status_name(host.presentation_report().postprocess_status)`n                  << '\n';`n        print_presentation_report(`"$TargetName`", host);`n        return 8;`n    }`n`n    GeneratedDesktopRuntime3DPackageGame game(",
        "    if (options.require_postprocess &&`n        host.presentation_report().postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready) {`n        std::cout << `"$TargetName required_postprocess_unavailable status=`"`n                  << mirakana::sdl_desktop_presentation_postprocess_status_name(host.presentation_report().postprocess_status)`n                  << '\n';`n        print_presentation_report(`"$TargetName`", host);`n        return 8;`n    }`n    if (options.require_postprocess_depth_input && !host.presentation_report().postprocess_depth_input_ready) {`n        std::cout << `"$TargetName required_postprocess_depth_input_unavailable\n`";`n        print_presentation_report(`"$TargetName`", host);`n        return 8;`n    }`n    if (options.require_directional_shadow && !host.presentation_report().directional_shadow_ready) {`n        std::cout << `"$TargetName required_directional_shadow_unavailable status=`"`n                  << mirakana::sdl_desktop_presentation_directional_shadow_status_name(`n                         host.presentation_report().directional_shadow_status)`n                  << '\n';`n        print_presentation_report(`"$TargetName`", host);`n        for (const auto& diagnostic : host.directional_shadow_diagnostics()) {`n            std::cout << `"$TargetName directional_shadow_diagnostic=`"`n                      << mirakana::sdl_desktop_presentation_directional_shadow_status_name(diagnostic.status) << `": `"`n                      << diagnostic.message << '\n';`n        }`n        return 9;`n    }`n    if (options.require_directional_shadow_filtering) {`n        const auto report = host.presentation_report();`n        if (report.directional_shadow_filter_mode !=`n                mirakana::SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 ||`n            report.directional_shadow_filter_tap_count != 9 || report.directional_shadow_filter_radius_texels != 1.0F) {`n            std::cout << `"$TargetName required_directional_shadow_filtering_unavailable mode=`"`n                      << mirakana::sdl_desktop_presentation_directional_shadow_filter_mode_name(`n                             report.directional_shadow_filter_mode)`n                      << `" taps=`" << report.directional_shadow_filter_tap_count`n                      << `" radius_texels=`" << report.directional_shadow_filter_radius_texels << '\n';`n            print_presentation_report(`"$TargetName`", host);`n            return 9;`n        }`n    }`n`n    GeneratedDesktopRuntime3DPackageGame game(")
    $text = $text.Replace(
        "              << `" postprocess_depth_input_ready=`" << (report.postprocess_depth_input_ready ? 1 : 0)`n              << `" framegraph_passes=`" << report.framegraph_passes",
        "              << `" postprocess_depth_input_ready=`" << (report.postprocess_depth_input_ready ? 1 : 0)`n              << `" directional_shadow_status=`"`n              << mirakana::sdl_desktop_presentation_directional_shadow_status_name(report.directional_shadow_status)`n              << `" directional_shadow_requested=`" << (report.directional_shadow_requested ? 1 : 0)`n              << `" directional_shadow_ready=`" << (report.directional_shadow_ready ? 1 : 0)`n              << `" directional_shadow_filter_mode=`"`n              << mirakana::sdl_desktop_presentation_directional_shadow_filter_mode_name(report.directional_shadow_filter_mode)`n              << `" directional_shadow_filter_taps=`" << report.directional_shadow_filter_tap_count`n              << `" directional_shadow_filter_radius_texels=`" << report.directional_shadow_filter_radius_texels`n              << `" framegraph_passes=`" << report.framegraph_passes")
    $text = $text.Replace(@"
        if (options.require_postprocess &&
            (report.postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready ||
             report.framegraph_passes != 2 ||
             report.renderer_stats.framegraph_passes_executed != static_cast<std::uint64_t>(options.max_frames) * 2U ||
             report.renderer_stats.postprocess_passes_executed != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_postprocess_depth_input && !report.postprocess_depth_input_ready) {
            return 3;
        }
"@, @"
        if (options.require_postprocess) {
            const std::uint32_t expected_framegraph_passes = options.require_directional_shadow ? 3U : 2U;
            if (report.postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready ||
                report.framegraph_passes != expected_framegraph_passes ||
                report.renderer_stats.framegraph_passes_executed !=
                    static_cast<std::uint64_t>(options.max_frames) * expected_framegraph_passes ||
                report.renderer_stats.postprocess_passes_executed != static_cast<std::uint64_t>(options.max_frames)) {
                return 3;
            }
        }
        if (options.require_postprocess_depth_input && !report.postprocess_depth_input_ready) {
            return 3;
        }
        if (options.require_directional_shadow &&
            (report.directional_shadow_status != mirakana::SdlDesktopPresentationDirectionalShadowStatus::ready ||
             !report.directional_shadow_ready)) {
            return 3;
        }
        if (options.require_directional_shadow_filtering &&
            (report.directional_shadow_filter_mode !=
                 mirakana::SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 ||
             report.directional_shadow_filter_tap_count != 9 ||
             report.directional_shadow_filter_radius_texels != 1.0F)) {
            return 3;
        }
"@)

    $templatePath = Join-Path $scriptRepositoryRoot "games/sample_generated_desktop_runtime_3d_package/main.cpp"
    if (Test-Path -LiteralPath $templatePath) {
        $text = Get-Content -LiteralPath $templatePath -Raw
        $text = $text.Replace("sample_generated_desktop_runtime_3d_package", $TargetName)
        $text = $text.Replace("sample-generated-desktop-runtime-3d-package", $assetKeyPrefix)
    }

    return ConvertTo-LfText -Text $text
}

function New-DesktopRuntimeCookedScenePackageFiles {
    param(
        [string]$GameName,
        [string]$DisplayTitle
    )

    $manifestName = $GameName.Replace("_", "-")
    $textureName = "$manifestName/textures/base-color"
    $meshName = "$manifestName/meshes/triangle"
    $materialName = "$manifestName/materials/lit"
    $sceneName = "$manifestName/scenes/packaged-scene"

    $textureId = Get-Fnv1a64Decimal -Text $textureName
    $meshId = Get-Fnv1a64Decimal -Text $meshName
    $materialId = Get-Fnv1a64Decimal -Text $materialName
    $sceneId = Get-Fnv1a64Decimal -Text $sceneName

    $texture = @"
format=GameEngine.CookedTexture.v1
asset.id=$textureId
asset.kind=texture
source.path=source/$GameName/base_color.texture
texture.width=1
texture.height=1
texture.pixel_format=rgba8_unorm
texture.source_bytes=4
texture.data_hex=59bf7aff
"@
    $texture = ConvertTo-LfText -Text $texture

    $mesh = @"
format=GameEngine.CookedMesh.v2
asset.id=$meshId
asset.kind=mesh
source.path=source/$GameName/triangle.mesh
mesh.vertex_count=3
mesh.index_count=3
mesh.has_normals=true
mesh.has_uvs=true
mesh.has_tangent_frame=true
mesh.vertex_data_hex=000000bf000000bf0000000000000000000000000000803f000000000000803f0000803f00000000000000000000803f0000003f000000bf0000000000000000000000000000803f0000803f0000803f0000803f00000000000000000000803f000000000000003f0000000000000000000000000000803f0000003f000000000000803f00000000000000000000803f
mesh.index_data_hex=000000000100000002000000
"@
    $mesh = ConvertTo-LfText -Text $mesh

    $material = @"
format=GameEngine.Material.v1
material.id=$materialId
material.name=$DisplayTitle Lit Material
material.shading=lit
material.surface=opaque
material.double_sided=false
factor.base_color=0.35,0.75,0.45,1
factor.emissive=0,0,0
factor.metallic=0
factor.roughness=1
texture.count=1
texture.1.slot=base_color
texture.1.id=$textureId
"@
    $material = ConvertTo-LfText -Text $material

    $scene = @"
format=GameEngine.Scene.v1
scene.name=$DisplayTitle Generated Scene
node.count=2
node.1.name=PackagedMesh
node.1.parent=0
node.1.position=0,0,0
node.1.scale=1,1,1
node.1.rotation=0,0,0
node.1.mesh_renderer.mesh=$meshId
node.1.mesh_renderer.material=$materialId
node.1.mesh_renderer.visible=true
node.2.name=KeyLight
node.2.parent=0
node.2.position=0,0,1
node.2.scale=1,1,1
node.2.rotation=0,0,0
node.2.light.type=directional
node.2.light.color=1,0.95,0.9
node.2.light.intensity=1
node.2.light.range=100
node.2.light.inner_cone_radians=0
node.2.light.outer_cone_radians=0
node.2.light.casts_shadows=false
"@
    $scene = ConvertTo-LfText -Text $scene

    $sourceTexture = @"
format=GameEngine.TextureSource.v1
texture.width=1
texture.height=1
texture.pixel_format=rgba8_unorm
texture.data_hex=59bf7aff
"@
    $sourceTexture = ConvertTo-LfText -Text $sourceTexture

    $sourceMesh = @"
format=GameEngine.MeshSource.v2
mesh.vertex_count=3
mesh.index_count=3
mesh.has_normals=true
mesh.has_uvs=true
mesh.has_tangent_frame=true
mesh.vertex_data_hex=000000bf000000bf0000000000000000000000000000803f000000000000803f0000803f00000000000000000000803f0000003f000000bf0000000000000000000000000000803f0000803f0000803f0000803f00000000000000000000803f000000000000003f0000000000000000000000000000803f0000003f000000000000803f00000000000000000000803f
mesh.index_data_hex=000000000100000002000000
"@
    $sourceMesh = ConvertTo-LfText -Text $sourceMesh

    $sourceScene = @"
format=GameEngine.Scene.v2
scene.name=$DisplayTitle Authored 3D Scene
node.0.id=node/root
node.0.name=Root
node.0.parent=
node.0.position=0 0 0
node.0.rotation=0 0 0
node.0.scale=1 1 1
node.1.id=node/packaged-mesh
node.1.name=PackagedMesh
node.1.parent=node/root
node.1.position=0 0 0
node.1.rotation=0 0 0
node.1.scale=1 1 1
node.2.id=node/key-light
node.2.name=KeyLight
node.2.parent=node/root
node.2.position=0 0 1
node.2.rotation=0 0 0
node.2.scale=1 1 1
node.3.id=node/primary-camera
node.3.name=PrimaryCamera
node.3.parent=node/root
node.3.position=0 0 -3
node.3.rotation=0 0 0
node.3.scale=1 1 1
component.0.id=component/packaged-mesh/mesh-renderer
component.0.node=node/packaged-mesh
component.0.type=mesh_renderer
component.0.property.0.name=material
component.0.property.0.value=$materialName
component.0.property.1.name=mesh
component.0.property.1.value=$meshName
component.0.property.2.name=visible
component.0.property.2.value=true
component.1.id=component/key-light/light
component.1.node=node/key-light
component.1.type=light
component.1.property.0.name=casts_shadows
component.1.property.0.value=false
component.1.property.1.name=color
component.1.property.1.value=1,0.95,0.9
component.1.property.2.name=inner_cone_radians
component.1.property.2.value=0
component.1.property.3.name=intensity
component.1.property.3.value=1
component.1.property.4.name=outer_cone_radians
component.1.property.4.value=0
component.1.property.5.name=range
component.1.property.5.value=100
component.1.property.6.name=type
component.1.property.6.value=directional
component.2.id=component/primary-camera/camera
component.2.node=node/primary-camera
component.2.type=camera
component.2.property.0.name=far_plane
component.2.property.0.value=100
component.2.property.1.name=near_plane
component.2.property.1.value=0.1
component.2.property.2.name=orthographic_height
component.2.property.2.value=10
component.2.property.3.name=primary
component.2.property.3.value=true
component.2.property.4.name=projection
component.2.property.4.value=perspective
component.2.property.5.name=vertical_fov_radians
component.2.property.5.value=1.04719758
"@
    $sourceScene = ConvertTo-LfText -Text $sourceScene

    $sourcePrefab = @"
format=GameEngine.Prefab.v2
prefab.name=$DisplayTitle Static Prop Prefab
scene.name=$DisplayTitle Static Prop Prefab Scene
node.0.id=node/static-prop
node.0.name=StaticProp
node.0.parent=
node.0.position=0 0 0
node.0.rotation=0 0 0
node.0.scale=1 1 1
component.0.id=component/static-prop/mesh-renderer
component.0.node=node/static-prop
component.0.type=mesh_renderer
component.0.property.0.name=material
component.0.property.0.value=$materialName
component.0.property.1.name=mesh
component.0.property.1.value=$meshName
component.0.property.2.name=visible
component.0.property.2.value=true
"@
    $sourcePrefab = ConvertTo-LfText -Text $sourcePrefab

    $sourceRegistry = @"
format=GameEngine.SourceAssetRegistry.v1
asset.0.key=$textureName
asset.0.id=$textureId
asset.0.kind=texture
asset.0.source=source/textures/base_color.texture_source
asset.0.source_format=GameEngine.TextureSource.v1
asset.0.imported=runtime/assets/3d/base_color.texture.geasset
asset.1.key=$meshName
asset.1.id=$meshId
asset.1.kind=mesh
asset.1.source=source/meshes/triangle.mesh_source
asset.1.source_format=GameEngine.MeshSource.v2
asset.1.imported=runtime/assets/3d/triangle.mesh
asset.2.key=$materialName
asset.2.id=$materialId
asset.2.kind=material
asset.2.source=source/materials/lit.material
asset.2.source_format=GameEngine.Material.v1
asset.2.imported=runtime/assets/3d/lit.material
asset.2.dependency.0.kind=material_texture
asset.2.dependency.0.key=$textureName
"@
    $sourceRegistry = ConvertTo-LfText -Text $sourceRegistry

    $gitAttributes = @"
# Generated cooked package files are hashed byte-for-byte by the runtime.
*.geindex text eol=lf
*.geasset text eol=lf
*.mesh text eol=lf
*.material text eol=lf
*.scene text eol=lf
"@
    $gitAttributes = ConvertTo-LfText -Text $gitAttributes

    $textureHash = Get-Fnv1a64Decimal -Text $texture
    $meshHash = Get-Fnv1a64Decimal -Text $mesh
    $materialHash = Get-Fnv1a64Decimal -Text $material
    $sceneHash = Get-Fnv1a64Decimal -Text $scene

    $index = @"
format=GameEngine.CookedPackageIndex.v1
entry.count=4
entry.0.asset=$textureId
entry.0.kind=texture
entry.0.path=runtime/assets/generated/base_color.texture.geasset
entry.0.content_hash=$textureHash
entry.0.source_revision=1
entry.0.dependencies=
entry.1.asset=$sceneId
entry.1.kind=scene
entry.1.path=runtime/assets/generated/packaged_scene.scene
entry.1.content_hash=$sceneHash
entry.1.source_revision=1
entry.1.dependencies=$materialId,$meshId
entry.2.asset=$meshId
entry.2.kind=mesh
entry.2.path=runtime/assets/generated/triangle.mesh
entry.2.content_hash=$meshHash
entry.2.source_revision=1
entry.2.dependencies=
entry.3.asset=$materialId
entry.3.kind=material
entry.3.path=runtime/assets/generated/lit.material
entry.3.content_hash=$materialHash
entry.3.source_revision=1
entry.3.dependencies=$textureId
dependency.count=3
dependency.0.asset=$materialId
dependency.0.dependency=$textureId
dependency.0.kind=material_texture
dependency.0.path=runtime/assets/generated/base_color.texture.geasset
dependency.1.asset=$sceneId
dependency.1.dependency=$materialId
dependency.1.kind=scene_material
dependency.1.path=runtime/assets/generated/lit.material
dependency.2.asset=$sceneId
dependency.2.dependency=$meshId
dependency.2.kind=scene_mesh
dependency.2.path=runtime/assets/generated/triangle.mesh
"@
    $index = ConvertTo-LfText -Text $index

    return [ordered]@{
        SceneAssetName = $sceneName
        Files = [ordered]@{
            "runtime/.gitattributes" = $gitAttributes
            "runtime/assets/generated/base_color.texture.geasset" = $texture
            "runtime/assets/generated/triangle.mesh" = $mesh
            "runtime/assets/generated/lit.material" = $material
            "runtime/assets/generated/packaged_scene.scene" = $scene
            "runtime/$GameName.geindex" = $index
        }
    }
}

function New-DesktopRuntime2DMainCpp {
    param(
        [string]$GameName,
        [string]$TargetName,
        [string]$Title
    )

    $templatePath = Join-Path $scriptRepositoryRoot "games/sample_2d_desktop_runtime_package/main.cpp"
    if (-not (Test-Path -LiteralPath $templatePath)) {
        Write-Error "DesktopRuntime2DPackage template source is missing: $templatePath"
    }

    $assetKeyPrefix = $GameName.Replace("_", "-")
    $titleLiteral = ConvertTo-CppStringLiteralContent -Text $Title
    $text = Get-Content -LiteralPath $templatePath -Raw
    $text = $text.Replace("sample_2d_desktop_runtime_package", $TargetName)
    $text = $text.Replace("Sample 2D Desktop Runtime Package", $titleLiteral)
    $text = $text.Replace("GameEngine.Sample2DDesktopRuntimePackage.Config.v1", "GameEngine.GeneratedDesktopRuntime2DPackage.Config.v1")
    $text = $text.Replace("sample/2d-desktop-runtime-package/scene", "$assetKeyPrefix/scenes/packaged-2d-scene")
    $text = $text.Replace("sample/2d-desktop-runtime-package/jump-audio", "$assetKeyPrefix/audio/jump")
    $text = $text.Replace("sample/2d-desktop-runtime-package/player-sprite-animation", "$assetKeyPrefix/animations/player-sprite-animation")
    $text = $text.Replace("sample/2d-desktop-runtime-package/tilemap", "$assetKeyPrefix/tilemaps/level")
    $text = $text.Replace("sample/2d-desktop-runtime-package/player-sprite", "$assetKeyPrefix/textures/player")
    return ConvertTo-LfText -Text $text
}

function New-DesktopRuntime3DPackageFiles {
    param(
        [string]$GameName,
        [string]$DisplayTitle
    )

    $assetKeyPrefix = $GameName.Replace("_", "-")
    $textureName = "$assetKeyPrefix/textures/base-color"
    $meshName = "$assetKeyPrefix/meshes/triangle"
    $skinnedMeshName = "$assetKeyPrefix/meshes/skinned-triangle"
    $morphName = "$assetKeyPrefix/morphs/packaged-mesh"
    $materialName = "$assetKeyPrefix/materials/lit"
    $animationName = "$assetKeyPrefix/animations/packaged-mesh-bob"
    $morphAnimationName = "$assetKeyPrefix/animations/packaged-mesh-morph-weights"
    $quaternionAnimationName = "$assetKeyPrefix/animations/packaged-pose"
    $uiAtlasName = "$assetKeyPrefix/ui/hud-atlas"
    $uiTextGlyphAtlasName = "$assetKeyPrefix/ui/hud-text-glyph-atlas"
    $sceneName = "$assetKeyPrefix/scenes/packaged-3d-scene"
    $collisionName = "$assetKeyPrefix/physics/collision"

    $textureId = Get-Fnv1a64Decimal -Text $textureName
    $meshId = Get-Fnv1a64Decimal -Text $meshName
    $skinnedMeshId = Get-Fnv1a64Decimal -Text $skinnedMeshName
    $morphId = Get-Fnv1a64Decimal -Text $morphName
    $materialId = Get-Fnv1a64Decimal -Text $materialName
    $animationId = Get-Fnv1a64Decimal -Text $animationName
    $morphAnimationId = Get-Fnv1a64Decimal -Text $morphAnimationName
    $quaternionAnimationId = Get-Fnv1a64Decimal -Text $quaternionAnimationName
    $uiAtlasId = Get-Fnv1a64Decimal -Text $uiAtlasName
    $uiTextGlyphAtlasId = Get-Fnv1a64Decimal -Text $uiTextGlyphAtlasName
    $sceneId = Get-Fnv1a64Decimal -Text $sceneName
    $collisionId = Get-Fnv1a64Decimal -Text $collisionName

    $texture = @"
format=GameEngine.CookedTexture.v1
asset.id=$textureId
asset.kind=texture
source.path=source/$GameName/base_color.texture
texture.width=1
texture.height=1
texture.pixel_format=rgba8_unorm
texture.source_bytes=4
texture.data_hex=59bf7aff
"@
    $texture = ConvertTo-LfText -Text $texture

    $mesh = @"
format=GameEngine.CookedMesh.v2
asset.id=$meshId
asset.kind=mesh
source.path=source/$GameName/triangle.mesh
mesh.vertex_count=3
mesh.index_count=3
mesh.has_normals=true
mesh.has_uvs=true
mesh.has_tangent_frame=true
mesh.vertex_data_hex=000000bf000000bf0000000000000000000000000000803f000000000000803f0000803f00000000000000000000803f0000003f000000bf0000000000000000000000000000803f0000803f0000803f0000803f00000000000000000000803f000000000000003f0000000000000000000000000000803f0000003f000000000000803f00000000000000000000803f
mesh.index_data_hex=000000000100000002000000
"@
    $mesh = ConvertTo-LfText -Text $mesh

    $skinnedMesh = @"
format=GameEngine.CookedSkinnedMesh.v1
asset.id=$skinnedMeshId
asset.kind=skinned_mesh
source.path=source/$GameName/skinned_triangle.skinned_mesh
skinned_mesh.vertex_count=3
skinned_mesh.index_count=3
skinned_mesh.joint_count=1
skinned_mesh.vertex_data_hex=000000bf000000bf0000000000000000000000000000803f000000000000803f0000803f00000000000000000000803f00000000000000000000803f0000000000000000000000000000003f000000bf0000000000000000000000000000803f0000803f0000803f0000803f00000000000000000000803f00000000000000000000803f000000000000000000000000000000000000003f0000000000000000000000000000803f0000003f000000000000803f00000000000000000000803f00000000000000000000803f000000000000000000000000
skinned_mesh.index_data_hex=000000000100000002000000
skinned_mesh.joint_palette_hex=0000803f000000000000000000000000000000000000803f000000000000000000000000000000000000803f000000000000000000000000000000000000803f
"@
    $skinnedMesh = ConvertTo-LfText -Text $skinnedMesh

    $morph = @"
format=GameEngine.CookedMorphMeshCpu.v1
asset.id=$morphId
asset.kind=morph_mesh_cpu
source.path=source/morphs/packaged_mesh.morph_mesh_cpu_source
morph.vertex_count=3
morph.target_count=1
morph.bind_positions_hex=000000bf000000bf000000000000003f000000bf00000000000000000000003f00000000
morph.bind_normals_hex=00000000000000000000803f00000000000000000000803f00000000000000000000803f
morph.bind_tangents_hex=0000803f00000000000000000000803f00000000000000000000803f0000000000000000
morph.target_weights_hex=0000803f
morph.target.0.position_deltas_hex=0000803f0000000000000000000000000000000000000000000000000000000000000000
morph.target.0.normal_deltas_hex=000000000000803f000080bf000000000000803f000080bf000000000000803f000080bf
morph.target.0.tangent_deltas_hex=000080bf0000803f00000000000080bf0000803f00000000000080bf0000803f00000000
"@
    $morph = ConvertTo-LfText -Text $morph

    $material = @"
format=GameEngine.Material.v1
material.id=$materialId
material.name=$DisplayTitle Lit Material
material.shading=lit
material.surface=opaque
material.double_sided=false
factor.base_color=0.35,0.75,0.45,1
factor.emissive=0,0,0
factor.metallic=0
factor.roughness=1
texture.count=1
texture.1.slot=base_color
texture.1.id=$textureId
"@
    $material = ConvertTo-LfText -Text $material

    $animation = @"
format=GameEngine.CookedAnimationFloatClip.v1
asset.id=$animationId
asset.kind=animation_float_clip
source.path=source/animations/packaged_mesh_bob.animation_float_clip_source
clip.track_count=1
clip.track.0.target=gltf/node/0/translation/x
clip.track.0.keyframe_count=2
clip.track.0.times_hex=000000000000803f
clip.track.0.values_hex=000000000000003f
"@
    $animation = ConvertTo-LfText -Text $animation

    $morphAnimation = @"
format=GameEngine.CookedAnimationFloatClip.v1
asset.id=$morphAnimationId
asset.kind=animation_float_clip
source.path=source/animations/packaged_mesh_morph_weights.animation_float_clip_source
clip.track_count=1
clip.track.0.target=gltf/node/0/weights/0
clip.track.0.keyframe_count=2
clip.track.0.times_hex=000000000000803f
clip.track.0.values_hex=000000000000003f
"@
    $morphAnimation = ConvertTo-LfText -Text $morphAnimation

    $quaternionAnimation = @"
format=GameEngine.CookedAnimationQuaternionClip.v1
asset.id=$quaternionAnimationId
asset.kind=animation_quaternion_clip
source.path=source/animations/packaged_pose.animation_quaternion_clip_source
clip.track_count=1
clip.track.0.target=$assetKeyPrefix/pose/root
clip.track.0.joint_index=0
clip.track.0.translation_keyframe_count=0
clip.track.0.translation_times_hex=
clip.track.0.translations_xyz_hex=
clip.track.0.rotation_keyframe_count=2
clip.track.0.rotation_times_hex=000000000000803f
clip.track.0.rotations_xyzw_hex=0000000000000000000000000000803f00000000000000000000803f00000000
clip.track.0.scale_keyframe_count=0
clip.track.0.scale_times_hex=
clip.track.0.scales_xyz_hex=
"@
    $quaternionAnimation = ConvertTo-LfText -Text $quaternionAnimation

    $uiAtlas = @"
format=GameEngine.UiAtlas.v1
asset.id=$uiAtlasId
asset.kind=ui_atlas
source.decoding=unsupported
atlas.packing=unsupported
page.count=1
page.0.asset=$textureId
page.0.asset_uri=runtime/assets/3d/base_color.texture.geasset
image.count=1
image.0.resource_id=hud.texture_atlas_proof
image.0.asset_uri=runtime/assets/3d/base_color.texture.geasset
image.0.page=$textureId
image.0.u0=0
image.0.v0=0
image.0.u1=1
image.0.v1=1
image.0.color=1,1,1,1
"@
    $uiAtlas = ConvertTo-LfText -Text $uiAtlas

    $uiTextGlyphAtlas = @"
format=GameEngine.UiAtlas.v1
asset.id=$uiTextGlyphAtlasId
asset.kind=ui_atlas
source.decoding=unsupported
atlas.packing=unsupported
page.count=1
page.0.asset=$textureId
page.0.asset_uri=runtime/assets/3d/base_color.texture.geasset
image.count=0
glyph.count=1
glyph.0.font_family=engine-default
glyph.0.glyph=65
glyph.0.page=$textureId
glyph.0.u0=0
glyph.0.v0=0
glyph.0.u1=1
glyph.0.v1=1
glyph.0.color=1,1,1,1
"@
    $uiTextGlyphAtlas = ConvertTo-LfText -Text $uiTextGlyphAtlas

    $collision = @"
format=GameEngine.PhysicsCollisionScene3D.v1
asset.id=$collisionId
asset.kind=physics_collision_scene
backend.native=unsupported
world.gravity=0,-9.80665,0
body.count=3
body.0.name=floor
body.0.shape=aabb
body.0.position=0,0,0
body.0.velocity=0,0,0
body.0.dynamic=false
body.0.mass=1
body.0.linear_damping=0
body.0.half_extents=8,0.5,8
body.0.radius=0.5
body.0.half_height=0.5
body.0.layer=1
body.0.mask=4294967295
body.0.trigger=false
body.0.material=stone
body.0.compound=level_static
body.1.name=collision_probe
body.1.shape=aabb
body.1.position=0,0.25,0
body.1.velocity=0,0,0
body.1.dynamic=false
body.1.mass=1
body.1.linear_damping=0
body.1.half_extents=0.5,0.5,0.5
body.1.radius=0.5
body.1.half_height=0.5
body.1.layer=4
body.1.mask=1
body.1.trigger=false
body.1.material=metal
body.1.compound=level_static
body.2.name=pickup_trigger
body.2.shape=sphere
body.2.position=2,0.25,0
body.2.velocity=0,0,0
body.2.dynamic=false
body.2.mass=1
body.2.linear_damping=0
body.2.half_extents=0.5,0.5,0.5
body.2.radius=0.75
body.2.half_height=0.5
body.2.layer=2
body.2.mask=1
body.2.trigger=true
body.2.material=trigger
body.2.compound=interaction_triggers
"@
    $collision = ConvertTo-LfText -Text $collision

    $scene = @"
format=GameEngine.Scene.v1
scene.name=$DisplayTitle Generated 3D Scene
node.count=3
node.1.name=PackagedMesh
node.1.parent=0
node.1.position=0,0,0
node.1.scale=1,1,1
node.1.rotation=0,0,0
node.1.mesh_renderer.mesh=$meshId
node.1.mesh_renderer.material=$materialId
node.1.mesh_renderer.visible=true
node.2.name=KeyLight
node.2.parent=0
node.2.position=0,0,1
node.2.scale=1,1,1
node.2.rotation=0,0,0
node.2.light.type=directional
node.2.light.color=1,0.95,0.9
node.2.light.intensity=1
node.2.light.range=100
node.2.light.inner_cone_radians=0
node.2.light.outer_cone_radians=0
node.2.light.casts_shadows=true
node.3.name=PrimaryCamera
node.3.parent=0
node.3.position=0,0,-3
node.3.scale=1,1,1
node.3.rotation=0,0,0
node.3.camera.projection=perspective
node.3.camera.primary=true
node.3.camera.vertical_fov_radians=1.04719758
node.3.camera.orthographic_height=10
node.3.camera.near=0.1
node.3.camera.far=100
"@
    $scene = ConvertTo-LfText -Text $scene

    $sourceTexture = @"
format=GameEngine.TextureSource.v1
texture.width=1
texture.height=1
texture.pixel_format=rgba8_unorm
texture.data_hex=59bf7aff
"@
    $sourceTexture = ConvertTo-LfText -Text $sourceTexture

    $sourceMesh = @"
format=GameEngine.MeshSource.v2
mesh.vertex_count=3
mesh.index_count=3
mesh.has_normals=true
mesh.has_uvs=true
mesh.has_tangent_frame=true
mesh.vertex_data_hex=000000bf000000bf0000000000000000000000000000803f000000000000803f0000803f00000000000000000000803f0000003f000000bf0000000000000000000000000000803f0000803f0000803f0000803f00000000000000000000803f000000000000003f0000000000000000000000000000803f0000003f000000000000803f00000000000000000000803f
mesh.index_data_hex=000000000100000002000000
"@
    $sourceMesh = ConvertTo-LfText -Text $sourceMesh

    $sourceMorph = @"
format=GameEngine.MorphMeshCpuSource.v1
morph.vertex_count=3
morph.target_count=1
morph.bind_positions_hex=000000bf000000bf000000000000003f000000bf00000000000000000000003f00000000
morph.bind_normals_hex=00000000000000000000803f00000000000000000000803f00000000000000000000803f
morph.bind_tangents_hex=0000803f00000000000000000000803f00000000000000000000803f0000000000000000
morph.target_weights_hex=0000803f
morph.target.0.position_deltas_hex=0000803f0000000000000000000000000000000000000000000000000000000000000000
morph.target.0.normal_deltas_hex=000000000000803f000080bf000000000000803f000080bf000000000000803f000080bf
morph.target.0.tangent_deltas_hex=000080bf0000803f00000000000080bf0000803f00000000000080bf0000803f00000000
"@
    $sourceMorph = ConvertTo-LfText -Text $sourceMorph

    $sourceAnimation = @"
format=GameEngine.AnimationFloatClipSource.v1
clip.track_count=1
clip.track.0.target=gltf/node/0/translation/x
clip.track.0.keyframe_count=2
clip.track.0.times_hex=000000000000803f
clip.track.0.values_hex=000000000000003f
"@
    $sourceAnimation = ConvertTo-LfText -Text $sourceAnimation

    $sourceMorphAnimation = @"
format=GameEngine.AnimationFloatClipSource.v1
clip.track_count=1
clip.track.0.target=gltf/node/0/weights/0
clip.track.0.keyframe_count=2
clip.track.0.times_hex=000000000000803f
clip.track.0.values_hex=000000000000003f
"@
    $sourceMorphAnimation = ConvertTo-LfText -Text $sourceMorphAnimation

    $sourceQuaternionAnimation = @"
format=GameEngine.AnimationQuaternionClipSource.v1
clip.track_count=1
clip.track.0.target=$assetKeyPrefix/pose/root
clip.track.0.joint_index=0
clip.track.0.translation_keyframe_count=0
clip.track.0.translation_times_hex=
clip.track.0.translations_xyz_hex=
clip.track.0.rotation_keyframe_count=2
clip.track.0.rotation_times_hex=000000000000803f
clip.track.0.rotations_xyzw_hex=0000000000000000000000000000803f00000000000000000000803f00000000
clip.track.0.scale_keyframe_count=0
clip.track.0.scale_times_hex=
clip.track.0.scales_xyz_hex=
"@
    $sourceQuaternionAnimation = ConvertTo-LfText -Text $sourceQuaternionAnimation

    $sourceScene = @"
format=GameEngine.Scene.v2
scene.name=$DisplayTitle Generated 3D Scene
node.0.id=node/packaged-mesh
node.0.name=PackagedMesh
node.0.parent=
node.0.position=0 0 0
node.0.rotation=0 0 0
node.0.scale=1 1 1
node.1.id=node/key-light
node.1.name=KeyLight
node.1.parent=
node.1.position=0 0 1
node.1.rotation=0 0 0
node.1.scale=1 1 1
node.2.id=node/primary-camera
node.2.name=PrimaryCamera
node.2.parent=
node.2.position=0 0 -3
node.2.rotation=0 0 0
node.2.scale=1 1 1
component.0.id=component/packaged-mesh/mesh-renderer
component.0.node=node/packaged-mesh
component.0.type=mesh_renderer
component.0.property.0.name=material
component.0.property.0.value=$materialName
component.0.property.1.name=mesh
component.0.property.1.value=$meshName
component.0.property.2.name=visible
component.0.property.2.value=true
component.1.id=component/key-light/light
component.1.node=node/key-light
component.1.type=light
component.1.property.0.name=casts_shadows
component.1.property.0.value=true
component.1.property.1.name=color
component.1.property.1.value=1,0.95,0.9
component.1.property.2.name=inner_cone_radians
component.1.property.2.value=0
component.1.property.3.name=intensity
component.1.property.3.value=1
component.1.property.4.name=outer_cone_radians
component.1.property.4.value=0
component.1.property.5.name=range
component.1.property.5.value=100
component.1.property.6.name=type
component.1.property.6.value=directional
component.2.id=component/primary-camera/camera
component.2.node=node/primary-camera
component.2.type=camera
component.2.property.0.name=far_plane
component.2.property.0.value=100
component.2.property.1.name=near_plane
component.2.property.1.value=0.1
component.2.property.2.name=orthographic_height
component.2.property.2.value=10
component.2.property.3.name=primary
component.2.property.3.value=true
component.2.property.4.name=projection
component.2.property.4.value=perspective
component.2.property.5.name=vertical_fov_radians
component.2.property.5.value=1.04719758
"@
    $sourceScene = ConvertTo-LfText -Text $sourceScene

    $sourcePrefab = @"
format=GameEngine.Prefab.v2
prefab.name=$DisplayTitle Static Prop Prefab
scene.name=$DisplayTitle Static Prop Prefab Scene
node.0.id=node/static-prop
node.0.name=StaticProp
node.0.parent=
node.0.position=0 0 0
node.0.rotation=0 0 0
node.0.scale=1 1 1
component.0.id=component/static-prop/mesh-renderer
component.0.node=node/static-prop
component.0.type=mesh_renderer
component.0.property.0.name=material
component.0.property.0.value=$materialName
component.0.property.1.name=mesh
component.0.property.1.value=$meshName
component.0.property.2.name=visible
component.0.property.2.value=true
"@
    $sourcePrefab = ConvertTo-LfText -Text $sourcePrefab

    $sourceRegistry = @"
format=GameEngine.SourceAssetRegistry.v1
asset.0.key=$textureName
asset.0.id=$textureId
asset.0.kind=texture
asset.0.source=source/textures/base_color.texture_source
asset.0.source_format=GameEngine.TextureSource.v1
asset.0.imported=runtime/assets/3d/base_color.texture.geasset
asset.1.key=$meshName
asset.1.id=$meshId
asset.1.kind=mesh
asset.1.source=source/meshes/triangle.mesh_source
asset.1.source_format=GameEngine.MeshSource.v2
asset.1.imported=runtime/assets/3d/triangle.mesh
asset.2.key=$morphName
asset.2.id=$morphId
asset.2.kind=morph_mesh_cpu
asset.2.source=source/morphs/packaged_mesh.morph_mesh_cpu_source
asset.2.source_format=GameEngine.MorphMeshCpuSource.v1
asset.2.imported=runtime/assets/3d/packaged_mesh.morph_mesh_cpu
asset.3.key=$materialName
asset.3.id=$materialId
asset.3.kind=material
asset.3.source=source/materials/lit.material
asset.3.source_format=GameEngine.Material.v1
asset.3.imported=runtime/assets/3d/lit.material
asset.3.dependency.0.kind=material_texture
asset.3.dependency.0.key=$textureName
asset.4.key=$animationName
asset.4.id=$animationId
asset.4.kind=animation_float_clip
asset.4.source=source/animations/packaged_mesh_bob.animation_float_clip_source
asset.4.source_format=GameEngine.AnimationFloatClipSource.v1
asset.4.imported=runtime/assets/3d/packaged_mesh_bob.animation_float_clip
asset.5.key=$morphAnimationName
asset.5.id=$morphAnimationId
asset.5.kind=animation_float_clip
asset.5.source=source/animations/packaged_mesh_morph_weights.animation_float_clip_source
asset.5.source_format=GameEngine.AnimationFloatClipSource.v1
asset.5.imported=runtime/assets/3d/packaged_mesh_morph_weights.animation_float_clip
asset.6.key=$quaternionAnimationName
asset.6.id=$quaternionAnimationId
asset.6.kind=animation_quaternion_clip
asset.6.source=source/animations/packaged_pose.animation_quaternion_clip_source
asset.6.source_format=GameEngine.AnimationQuaternionClipSource.v1
asset.6.imported=runtime/assets/3d/packaged_pose.animation_quaternion_clip
"@
    $sourceRegistry = ConvertTo-LfText -Text $sourceRegistry

    $gitAttributes = @"
# Generated cooked package files are hashed byte-for-byte by the runtime.
*.geindex text eol=lf
*.geasset text eol=lf
*.mesh text eol=lf
*.skinned_mesh text eol=lf
*.morph_mesh_cpu text eol=lf
*.morph_mesh_cpu_source text eol=lf
*.material text eol=lf
*.scene text eol=lf
*.collision3d text eol=lf
*.uiatlas text eol=lf
*.animation_float_clip text eol=lf
*.animation_float_clip_source text eol=lf
*.animation_quaternion_clip text eol=lf
*.animation_quaternion_clip_source text eol=lf
"@
    $gitAttributes = ConvertTo-LfText -Text $gitAttributes

    $textureHash = Get-Fnv1a64Decimal -Text $texture
    $meshHash = Get-Fnv1a64Decimal -Text $mesh
    $skinnedMeshHash = Get-Fnv1a64Decimal -Text $skinnedMesh
    $morphHash = Get-Fnv1a64Decimal -Text $morph
    $materialHash = Get-Fnv1a64Decimal -Text $material
    $animationHash = Get-Fnv1a64Decimal -Text $animation
    $morphAnimationHash = Get-Fnv1a64Decimal -Text $morphAnimation
    $quaternionAnimationHash = Get-Fnv1a64Decimal -Text $quaternionAnimation
    $uiAtlasHash = Get-Fnv1a64Decimal -Text $uiAtlas
    $uiTextGlyphAtlasHash = Get-Fnv1a64Decimal -Text $uiTextGlyphAtlas
    $sceneHash = Get-Fnv1a64Decimal -Text $scene
    $collisionHash = Get-Fnv1a64Decimal -Text $collision

    $index = @"
format=GameEngine.CookedPackageIndex.v1
entry.count=12
entry.0.asset=$textureId
entry.0.kind=texture
entry.0.path=runtime/assets/3d/base_color.texture.geasset
entry.0.content_hash=$textureHash
entry.0.source_revision=1
entry.0.dependencies=
entry.1.asset=$sceneId
entry.1.kind=scene
entry.1.path=runtime/assets/3d/packaged_scene.scene
entry.1.content_hash=$sceneHash
entry.1.source_revision=1
entry.1.dependencies=$materialId,$meshId
entry.2.asset=$meshId
entry.2.kind=mesh
entry.2.path=runtime/assets/3d/triangle.mesh
entry.2.content_hash=$meshHash
entry.2.source_revision=1
entry.2.dependencies=
entry.3.asset=$morphId
entry.3.kind=morph_mesh_cpu
entry.3.path=runtime/assets/3d/packaged_mesh.morph_mesh_cpu
entry.3.content_hash=$morphHash
entry.3.source_revision=1
entry.3.dependencies=
entry.4.asset=$materialId
entry.4.kind=material
entry.4.path=runtime/assets/3d/lit.material
entry.4.content_hash=$materialHash
entry.4.source_revision=1
entry.4.dependencies=$textureId
entry.5.asset=$animationId
entry.5.kind=animation_float_clip
entry.5.path=runtime/assets/3d/packaged_mesh_bob.animation_float_clip
entry.5.content_hash=$animationHash
entry.5.source_revision=1
entry.5.dependencies=
entry.6.asset=$morphAnimationId
entry.6.kind=animation_float_clip
entry.6.path=runtime/assets/3d/packaged_mesh_morph_weights.animation_float_clip
entry.6.content_hash=$morphAnimationHash
entry.6.source_revision=1
entry.6.dependencies=
entry.7.asset=$quaternionAnimationId
entry.7.kind=animation_quaternion_clip
entry.7.path=runtime/assets/3d/packaged_pose.animation_quaternion_clip
entry.7.content_hash=$quaternionAnimationHash
entry.7.source_revision=1
entry.7.dependencies=
entry.8.asset=$skinnedMeshId
entry.8.kind=skinned_mesh
entry.8.path=runtime/assets/3d/skinned_triangle.skinned_mesh
entry.8.content_hash=$skinnedMeshHash
entry.8.source_revision=1
entry.8.dependencies=
entry.9.asset=$uiAtlasId
entry.9.kind=ui_atlas
entry.9.path=runtime/assets/3d/hud.uiatlas
entry.9.content_hash=$uiAtlasHash
entry.9.source_revision=1
entry.9.dependencies=$textureId
entry.10.asset=$uiTextGlyphAtlasId
entry.10.kind=ui_atlas
entry.10.path=runtime/assets/3d/hud_text.uiatlas
entry.10.content_hash=$uiTextGlyphAtlasHash
entry.10.source_revision=1
entry.10.dependencies=$textureId
entry.11.asset=$collisionId
entry.11.kind=physics_collision_scene
entry.11.path=runtime/assets/3d/collision.collision3d
entry.11.content_hash=$collisionHash
entry.11.source_revision=1
entry.11.dependencies=
dependency.count=5
dependency.0.asset=$materialId
dependency.0.dependency=$textureId
dependency.0.kind=material_texture
dependency.0.path=runtime/assets/3d/base_color.texture.geasset
dependency.1.asset=$sceneId
dependency.1.dependency=$materialId
dependency.1.kind=scene_material
dependency.1.path=runtime/assets/3d/lit.material
dependency.2.asset=$sceneId
dependency.2.dependency=$meshId
dependency.2.kind=scene_mesh
dependency.2.path=runtime/assets/3d/triangle.mesh
dependency.3.asset=$uiAtlasId
dependency.3.dependency=$textureId
dependency.3.kind=ui_atlas_texture
dependency.3.path=runtime/assets/3d/base_color.texture.geasset
dependency.4.asset=$uiTextGlyphAtlasId
dependency.4.dependency=$textureId
dependency.4.kind=ui_atlas_texture
dependency.4.path=runtime/assets/3d/base_color.texture.geasset
"@
    $index = ConvertTo-LfText -Text $index

    return [ordered]@{
        SceneAssetName = $sceneName
        Files = [ordered]@{
            "runtime/.gitattributes" = $gitAttributes
            "runtime/assets/3d/base_color.texture.geasset" = $texture
            "runtime/assets/3d/triangle.mesh" = $mesh
            "runtime/assets/3d/skinned_triangle.skinned_mesh" = $skinnedMesh
            "runtime/assets/3d/packaged_mesh.morph_mesh_cpu" = $morph
            "runtime/assets/3d/lit.material" = $material
            "runtime/assets/3d/packaged_mesh_bob.animation_float_clip" = $animation
            "runtime/assets/3d/packaged_mesh_morph_weights.animation_float_clip" = $morphAnimation
            "runtime/assets/3d/packaged_pose.animation_quaternion_clip" = $quaternionAnimation
            "runtime/assets/3d/hud.uiatlas" = $uiAtlas
            "runtime/assets/3d/hud_text.uiatlas" = $uiTextGlyphAtlas
            "runtime/assets/3d/collision.collision3d" = $collision
            "runtime/assets/3d/packaged_scene.scene" = $scene
            "runtime/$GameName.geindex" = $index
            "source/assets/package.geassets" = $sourceRegistry
            "source/scenes/packaged_scene.scene" = $sourceScene
            "source/prefabs/static_prop.prefab" = $sourcePrefab
            "source/textures/base_color.texture_source" = $sourceTexture
            "source/meshes/triangle.mesh_source" = $sourceMesh
            "source/morphs/packaged_mesh.morph_mesh_cpu_source" = $sourceMorph
            "source/animations/packaged_mesh_bob.animation_float_clip_source" = $sourceAnimation
            "source/animations/packaged_mesh_morph_weights.animation_float_clip_source" = $sourceMorphAnimation
            "source/animations/packaged_pose.animation_quaternion_clip_source" = $sourceQuaternionAnimation
        }
    }
}

function New-DesktopRuntime2DPackageFiles {
    param(
        [string]$GameName,
        [string]$DisplayTitle
    )

    $assetKeyPrefix = $GameName.Replace("_", "-")
    $textureName = "$assetKeyPrefix/textures/player"
    $materialName = "$assetKeyPrefix/materials/player"
    $audioName = "$assetKeyPrefix/audio/jump"
    $sceneName = "$assetKeyPrefix/scenes/packaged-2d-scene"
    $tilemapName = "$assetKeyPrefix/tilemaps/level"
    $spriteAnimationName = "$assetKeyPrefix/animations/player-sprite-animation"

    $textureId = Get-Fnv1a64Decimal -Text $textureName
    $materialId = Get-Fnv1a64Decimal -Text $materialName
    $audioId = Get-Fnv1a64Decimal -Text $audioName
    $sceneId = Get-Fnv1a64Decimal -Text $sceneName
    $tilemapId = Get-Fnv1a64Decimal -Text $tilemapName
    $spriteAnimationId = Get-Fnv1a64Decimal -Text $spriteAnimationName

    $texture = @"
format=GameEngine.CookedTexture.v1
asset.id=$textureId
asset.kind=texture
source.path=source/$GameName/player.texture
texture.width=1
texture.height=1
texture.pixel_format=rgba8_unorm
texture.source_bytes=4
texture.data_hex=33b3ffff
"@
    $texture = ConvertTo-LfText -Text $texture

    $material = @"
format=GameEngine.Material.v1
material.id=$materialId
material.name=$DisplayTitle Player Material
material.shading=unlit
material.surface=transparent
material.double_sided=true
factor.base_color=0.2,0.7,1,1
factor.emissive=0,0,0
factor.metallic=0
factor.roughness=1
texture.count=1
texture.1.slot=base_color
texture.1.id=$textureId
"@
    $material = ConvertTo-LfText -Text $material

    $audio = @"
format=GameEngine.CookedAudio.v1
asset.id=$audioId
asset.kind=audio
source.path=source/$GameName/jump.audio
audio.sample_rate=48000
audio.channel_count=1
audio.frame_count=4
audio.sample_format=float32
audio.source_bytes=16
audio.data_hex=0000803e0000003f0000803e00000000
"@
    $audio = ConvertTo-LfText -Text $audio

    $scene = @"
format=GameEngine.Scene.v1
scene.name=$DisplayTitle Generated 2D Scene
node.count=2
node.1.name=Main Camera
node.1.parent=0
node.1.position=0,0,10
node.1.scale=1,1,1
node.1.rotation=0,0,0
node.1.camera.projection=orthographic
node.1.camera.primary=true
node.1.camera.vertical_fov_radians=1.04719758
node.1.camera.orthographic_height=12
node.1.camera.near=0.1
node.1.camera.far=100
node.2.name=Player
node.2.parent=0
node.2.position=0,0,0
node.2.scale=1,1,1
node.2.rotation=0,0,0
node.2.sprite_renderer.sprite=$textureId
node.2.sprite_renderer.material=$materialId
node.2.sprite_renderer.size=1.5,2
node.2.sprite_renderer.tint=0.2,0.7,1,1
node.2.sprite_renderer.visible=true
"@
    $scene = ConvertTo-LfText -Text $scene

    $tilemap = @"
format=GameEngine.Tilemap.v1
asset.id=$tilemapId
asset.kind=tilemap
source.decoding=unsupported
atlas.packing=unsupported
native_gpu_sprite_batching=unsupported
atlas.page.asset=$textureId
atlas.page.asset_uri=runtime/assets/2d/player.texture.geasset
tile.size=16,16
tile.count=2
tile.0.id=grass
tile.0.page=$textureId
tile.0.u0=0
tile.0.v0=0
tile.0.u1=0.5
tile.0.v1=0.5
tile.0.color=0.5,1,0.25,1
tile.1.id=water
tile.1.page=$textureId
tile.1.u0=0.5
tile.1.v0=0
tile.1.u1=1
tile.1.v1=0.5
tile.1.color=0.25,0.5,1,1
layer.count=1
layer.0.name=ground
layer.0.width=2
layer.0.height=2
layer.0.visible=true
layer.0.cells=grass,water,,grass
"@
    $tilemap = ConvertTo-LfText -Text $tilemap

    $spriteAnimation = @"
format=GameEngine.CookedSpriteAnimation.v1
asset.id=$spriteAnimationId
asset.kind=sprite_animation
target.node=Player
playback.loop=true
frame.count=2
frame.0.duration_seconds=0.25
frame.0.sprite=$textureId
frame.0.material=$materialId
frame.0.size=1.5,2
frame.0.tint=0.2,0.7,1,1
frame.1.duration_seconds=0.25
frame.1.sprite=$textureId
frame.1.material=$materialId
frame.1.size=2,1.25
frame.1.tint=1,0.4,0.2,1
"@
    $spriteAnimation = ConvertTo-LfText -Text $spriteAnimation

    $spriteShader = @"
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct VsOut {
    float4 position : SV_Position;
    float4 color : COLOR0;
};

VsOut vs_main(uint vertex_id : SV_VertexID) {
    const float2 positions[3] = {
        float2(0.0, 0.62),
        float2(0.68, -0.58),
        float2(-0.68, -0.58),
    };
    const float4 colors[3] = {
        float4(0.15, 0.68, 1.0, 1.0),
        float4(0.95, 0.85, 0.22, 1.0),
        float4(0.2, 0.9, 0.45, 1.0),
    };

    VsOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.color = colors[vertex_id];
    return output;
}

float4 ps_main(VsOut input) : SV_Target {
    return input.color;
}

struct NativeSpriteOverlayVertexIn {
    float2 position : POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float2 texture_flags : TEXCOORD1;
};

struct NativeSpriteOverlayVertexOut {
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float2 texture_flags : TEXCOORD1;
};

Texture2D sprite_atlas : register(t0);
SamplerState sprite_sampler : register(s1);

NativeSpriteOverlayVertexOut vs_native_sprite_overlay(NativeSpriteOverlayVertexIn input) {
    NativeSpriteOverlayVertexOut output;
    output.position = float4(input.position, 0.0, 1.0);
    output.color = saturate(input.color);
    output.uv = saturate(input.uv);
    output.texture_flags = input.texture_flags;
    return output;
}

float4 ps_native_sprite_overlay(NativeSpriteOverlayVertexOut input) : SV_Target {
    if (input.texture_flags.x > 0.5) {
        return saturate(sprite_atlas.Sample(sprite_sampler, input.uv) * input.color);
    }
    return input.color;
}
"@
    $spriteShader = ConvertTo-LfText -Text $spriteShader

    $gitAttributes = @"
# Generated cooked package files are hashed byte-for-byte by the runtime.
*.geindex text eol=lf
*.geasset text eol=lf
*.material text eol=lf
*.scene text eol=lf
*.tilemap text eol=lf
*.sprite_animation text eol=lf
"@
    $gitAttributes = ConvertTo-LfText -Text $gitAttributes

    $textureHash = Get-Fnv1a64Decimal -Text $texture
    $materialHash = Get-Fnv1a64Decimal -Text $material
    $audioHash = Get-Fnv1a64Decimal -Text $audio
    $sceneHash = Get-Fnv1a64Decimal -Text $scene
    $tilemapHash = Get-Fnv1a64Decimal -Text $tilemap
    $spriteAnimationHash = Get-Fnv1a64Decimal -Text $spriteAnimation

    $index = @"
format=GameEngine.CookedPackageIndex.v1
entry.count=6
entry.0.asset=$sceneId
entry.0.kind=scene
entry.0.path=runtime/assets/2d/playable.scene
entry.0.content_hash=$sceneHash
entry.0.source_revision=1
entry.0.dependencies=$textureId,$materialId
entry.1.asset=$textureId
entry.1.kind=texture
entry.1.path=runtime/assets/2d/player.texture.geasset
entry.1.content_hash=$textureHash
entry.1.source_revision=1
entry.1.dependencies=
entry.2.asset=$materialId
entry.2.kind=material
entry.2.path=runtime/assets/2d/player.material
entry.2.content_hash=$materialHash
entry.2.source_revision=1
entry.2.dependencies=$textureId
entry.3.asset=$audioId
entry.3.kind=audio
entry.3.path=runtime/assets/2d/jump.audio.geasset
entry.3.content_hash=$audioHash
entry.3.source_revision=1
entry.3.dependencies=
entry.4.asset=$tilemapId
entry.4.kind=tilemap
entry.4.path=runtime/assets/2d/level.tilemap
entry.4.content_hash=$tilemapHash
entry.4.source_revision=1
entry.4.dependencies=$textureId
entry.5.asset=$spriteAnimationId
entry.5.kind=sprite_animation
entry.5.path=runtime/assets/2d/player.sprite_animation
entry.5.content_hash=$spriteAnimationHash
entry.5.source_revision=1
entry.5.dependencies=$textureId,$materialId
dependency.count=6
dependency.0.asset=$sceneId
dependency.0.dependency=$textureId
dependency.0.kind=scene_sprite
dependency.0.path=runtime/assets/2d/player.texture.geasset
dependency.1.asset=$sceneId
dependency.1.dependency=$materialId
dependency.1.kind=scene_material
dependency.1.path=runtime/assets/2d/player.material
dependency.2.asset=$materialId
dependency.2.dependency=$textureId
dependency.2.kind=material_texture
dependency.2.path=runtime/assets/2d/player.texture.geasset
dependency.3.asset=$tilemapId
dependency.3.dependency=$textureId
dependency.3.kind=tilemap_texture
dependency.3.path=runtime/assets/2d/level.tilemap
dependency.4.asset=$spriteAnimationId
dependency.4.dependency=$textureId
dependency.4.kind=sprite_animation_texture
dependency.4.path=runtime/assets/2d/player.sprite_animation
dependency.5.asset=$spriteAnimationId
dependency.5.dependency=$materialId
dependency.5.kind=sprite_animation_material
dependency.5.path=runtime/assets/2d/player.sprite_animation
"@
    $index = ConvertTo-LfText -Text $index

    return [ordered]@{
        SceneAssetName = $sceneName
        AudioAssetName = $audioName
        TilemapAssetName = $tilemapName
        SpriteAnimationAssetName = $spriteAnimationName
        Files = [ordered]@{
            "runtime/.gitattributes" = $gitAttributes
            "runtime/assets/2d/player.texture.geasset" = $texture
            "runtime/assets/2d/player.material" = $material
            "runtime/assets/2d/jump.audio.geasset" = $audio
            "runtime/assets/2d/level.tilemap" = $tilemap
            "runtime/assets/2d/player.sprite_animation" = $spriteAnimation
            "runtime/assets/2d/playable.scene" = $scene
            "runtime/$GameName.geindex" = $index
            "shaders/runtime_2d_sprite.hlsl" = $spriteShader
        }
    }
}

function New-DesktopRuntimeMaterialShaderPackageFiles {
    param(
        [string]$GameName,
        [string]$DisplayTitle
    )

    $package = New-DesktopRuntimeCookedScenePackageFiles -GameName $GameName -DisplayTitle $DisplayTitle
    $sceneShader = @"
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

cbuffer MaterialFactors : register(b0) {
    float4 base_color;
    float3 emissive;
    float metallic;
    float roughness;
};

Texture2D base_color_texture : register(t1);
SamplerState base_color_sampler : register(s16);
#ifndef MK_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS
#define MK_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS 0
#endif
#if MK_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS
Texture2D<float> shadow_depth_texture : register(t0, space2);
SamplerState shadow_sampler : register(s1, space2);
#else
Texture2D<float> shadow_depth_texture : register(t0, space1);
SamplerState shadow_sampler : register(s1, space1);
#endif
RWByteAddressBuffer compute_base_vertices : register(u0, space0);
RWByteAddressBuffer compute_position_deltas : register(u1, space0);
RWByteAddressBuffer compute_output_positions : register(u3, space0);
RWByteAddressBuffer compute_normal_deltas : register(u4, space0);
RWByteAddressBuffer compute_tangent_deltas : register(u5, space0);
RWByteAddressBuffer compute_output_normals : register(u6, space0);
RWByteAddressBuffer compute_output_tangents : register(u7, space0);
RWByteAddressBuffer morph_position_deltas : register(u0, space1);
RWByteAddressBuffer morph_normal_deltas : register(u2, space1);
RWByteAddressBuffer morph_tangent_deltas : register(u3, space1);

cbuffer ComputeMorphWeights : register(b2, space0) {
    float compute_morph_weights[64];
};

cbuffer MorphWeights : register(b1, space1) {
    float morph_weights[64];
};

#if MK_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS
cbuffer ShadowReceiverConstants : register(b2, space2) {
#else
cbuffer ShadowReceiverConstants : register(b2, space1) {
#endif
    uint shadow_cascade_count;
    uint3 shadow_receiver_pad0;
    float4 shadow_cascade_splits0;
    float4 shadow_cascade_splits1;
    float4 shadow_cascade_splits2;
    row_major float4x4 shadow_clip_from_world_cascades[8];
    row_major float4x4 shadow_camera_view_from_world;
    uint4 shadow_receiver_pad1[8];
};

#define SKINNED_MAX_JOINTS 256
cbuffer JointPalette : register(b0, space1) {
    row_major float4x4 joint_from_rest[SKINNED_MAX_JOINTS];
};

struct VsIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct ComputeMorphVsIn {
    float3 position : POSITION;
};

struct ComputeMorphTangentFrameVsIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct ComputeMorphSkinnedVsIn {
    float3 position : POSITION;
    uint4 joint_indices : BLENDINDICES;
    float4 joint_weights : BLENDWEIGHT;
};

struct VsOut {
    float4 position : SV_Position;
    float3 world_position : TEXCOORD1;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
};

VsOut vs_main(VsIn input) {
    VsOut output;
    output.position = float4(input.position.xy, input.position.z, 1.0);
    output.world_position = input.position;
    output.normal = normalize(input.normal);
    output.uv = input.uv;
    output.tangent = normalize(input.tangent.xyz);
    return output;
}

VsOut vs_morph(VsIn input, uint vertex_id : SV_VertexID) {
    VsOut output;
    uint delta_offset = vertex_id * 12;
    float weight = morph_weights[0];
    float3 position_delta = asfloat(morph_position_deltas.Load3(delta_offset));
    float3 normal_delta = asfloat(morph_normal_deltas.Load3(delta_offset));
    float3 tangent_delta = asfloat(morph_tangent_deltas.Load3(delta_offset));
    float3 morphed_position = input.position + position_delta * weight;
    float3 morphed_tangent = normalize(input.tangent.xyz + tangent_delta * weight);
    float3 morphed_normal = normalize(input.normal + normal_delta * weight);
    output.position = float4(morphed_position.xy, morphed_position.z, 1.0);
    output.world_position = morphed_position;
    output.normal = morphed_normal;
    output.uv = input.uv;
    output.tangent = morphed_tangent;
    return output;
}

VsOut vs_compute_morph(ComputeMorphVsIn input, uint vertex_id : SV_VertexID) {
    float2 uvs[3] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.5, 1.0),
    };

    VsOut output;
    output.position = float4(input.position.xy, input.position.z, 1.0);
    output.world_position = input.position;
    output.normal = float3(0.0, 0.0, 1.0);
    output.uv = uvs[vertex_id];
    output.tangent = float3(1.0, 0.0, 0.0);
    return output;
}

VsOut vs_compute_morph_tangent_frame(ComputeMorphTangentFrameVsIn input, uint vertex_id : SV_VertexID) {
    float2 uvs[3] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.5, 1.0),
    };

    VsOut output;
    output.position = float4(input.position.xy, input.position.z, 1.0);
    output.world_position = input.position;
    output.normal = normalize(input.normal);
    output.uv = uvs[vertex_id];
    output.tangent = normalize(input.tangent);
    return output;
}

VsOut vs_compute_morph_skinned(ComputeMorphSkinnedVsIn input, uint vertex_id : SV_VertexID) {
    float2 uvs[3] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.5, 1.0),
    };

    const float4 bind_pos = float4(input.position, 1.0);
    const float4 model_pos = mul(bind_pos, joint_from_rest[input.joint_indices.x]) * input.joint_weights.x +
                             mul(bind_pos, joint_from_rest[input.joint_indices.y]) * input.joint_weights.y +
                             mul(bind_pos, joint_from_rest[input.joint_indices.z]) * input.joint_weights.z +
                             mul(bind_pos, joint_from_rest[input.joint_indices.w]) * input.joint_weights.w;

    VsOut output;
    output.position = float4(model_pos.xyz, 1.0);
    output.world_position = model_pos.xyz;
    output.normal = float3(0.0, 0.0, 1.0);
    output.uv = uvs[vertex_id];
    output.tangent = float3(1.0, 0.0, 0.0);
    return output;
}

[numthreads(1, 1, 1)]
void cs_compute_morph_position(uint3 dispatch_id : SV_DispatchThreadID) {
    uint vertex_id = dispatch_id.x;
    uint source_offset = vertex_id * 48;
    uint position_offset = vertex_id * 12;
    float3 base_position = asfloat(compute_base_vertices.Load3(source_offset));
    float3 position_delta = asfloat(compute_position_deltas.Load3(position_offset));
    float3 morphed_position = base_position + position_delta * compute_morph_weights[0];
    compute_output_positions.Store3(position_offset, asuint(morphed_position));
}

[numthreads(1, 1, 1)]
void cs_compute_morph_tangent_frame(uint3 dispatch_id : SV_DispatchThreadID) {
    uint vertex_id = dispatch_id.x;
    uint source_offset = vertex_id * 48;
    uint tangent_offset = source_offset + 32;
    uint position_offset = vertex_id * 12;
    float weight = compute_morph_weights[0];
    float3 base_position = asfloat(compute_base_vertices.Load3(source_offset));
    float3 base_normal = asfloat(compute_base_vertices.Load3(source_offset + 12));
    float3 base_tangent = asfloat(compute_base_vertices.Load3(tangent_offset));
    float3 position_delta = asfloat(compute_position_deltas.Load3(position_offset));
    float3 normal_delta = asfloat(compute_normal_deltas.Load3(position_offset));
    float3 tangent_delta = asfloat(compute_tangent_deltas.Load3(position_offset));
    float3 morphed_position = base_position + position_delta * weight;
    float3 morphed_normal = normalize(base_normal + normal_delta * weight);
    float3 morphed_tangent = normalize(base_tangent + tangent_delta * weight);
    compute_output_positions.Store3(position_offset, asuint(morphed_position));
    compute_output_normals.Store3(position_offset, asuint(morphed_normal));
    compute_output_tangents.Store3(position_offset, asuint(morphed_tangent));
}

[numthreads(1, 1, 1)]
void cs_compute_morph_skinned_position(uint3 dispatch_id : SV_DispatchThreadID) {
    uint vertex_id = dispatch_id.x;
    uint source_offset = vertex_id * 12;
    uint position_offset = source_offset;
    float3 base_position = asfloat(compute_base_vertices.Load3(source_offset));
    float3 position_delta = asfloat(compute_position_deltas.Load3(position_offset));
    float3 morphed_position = base_position + position_delta * compute_morph_weights[0];
    compute_output_positions.Store3(position_offset, asuint(morphed_position));
}

float3 evaluate_lit_color(VsOut input, float4 sampled, float shadow_scale) {
    float3 normal = normalize(input.normal);
    float3 tangent = normalize(input.tangent);
    float3 light_direction = normalize(float3(0.25, 0.35, 0.9));
    float direct_light = saturate(dot(normal, light_direction)) * shadow_scale;
    float tangent_light = 0.65 + 0.35 * saturate(abs(tangent.y));
    return sampled.rgb * base_color.rgb * (0.18 + 0.82 * direct_light) * tangent_light + emissive.rgb;
}

float4 ps_main(VsOut input) : SV_Target {
    float4 sampled = base_color_texture.Sample(base_color_sampler, input.uv);
    return float4(saturate(evaluate_lit_color(input, sampled, 1.0)), sampled.a * base_color.a);
}

float shadow_split_distance(uint index) {
    float splits[12] = {shadow_cascade_splits0.x, shadow_cascade_splits0.y, shadow_cascade_splits0.z,
                        shadow_cascade_splits0.w, shadow_cascade_splits1.x, shadow_cascade_splits1.y,
                        shadow_cascade_splits1.z, shadow_cascade_splits1.w, shadow_cascade_splits2.x,
                        shadow_cascade_splits2.y, shadow_cascade_splits2.z, shadow_cascade_splits2.w};
    return splits[index];
}

uint select_shadow_cascade(float view_depth_along_forward) {
    if (shadow_cascade_count <= 1u) {
        return 0u;
    }
    const uint last = shadow_cascade_count - 1u;
    for (uint i = 0u; i < last; ++i) {
        if (view_depth_along_forward < shadow_split_distance(i + 1u)) {
            return i;
        }
    }
    return last;
}

float shadow_view_depth_along_forward(float3 world_position) {
    return world_position.x * shadow_camera_view_from_world._m20 + world_position.y * shadow_camera_view_from_world._m21 +
           world_position.z * shadow_camera_view_from_world._m22 + shadow_camera_view_from_world._m23;
}

float2 shadow_tile_uv_from_clip(float4 light_clip) {
    const float inv_w = 1.0 / max(abs(light_clip.w), 1e-6);
    const float2 ndc = light_clip.xy * inv_w;
    const float u = ndc.x * 0.5 + 0.5;
    const float v = 0.5 - ndc.y * 0.5;
    return float2(u, v);
}

float2 shadow_atlas_uv(uint cascade_index, float2 tile_uv) {
    const float inv_cc = 1.0 / max(float(shadow_cascade_count), 1.0);
    return float2((float(cascade_index) + tile_uv.x) * inv_cc, tile_uv.y);
}

float directional_shadow_pcf_3x3(float2 atlas_uv, float receiver_depth) {
    uint width = 1;
    uint height = 1;
    shadow_depth_texture.GetDimensions(width, height);

    float safe_width = width == 0 ? 1.0 : (float)width;
    float safe_height = height == 0 ? 1.0 : (float)height;
    float2 texel = 1.0 / float2(safe_width, safe_height);
    float occlusion = 0.0;

    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            float sample_depth =
                shadow_depth_texture.SampleLevel(shadow_sampler, atlas_uv + float2((float)x, (float)y) * texel, 0.0);
            occlusion += sample_depth + 0.003 < receiver_depth ? 1.0 : 0.0;
        }
    }
    return occlusion / 9.0;
}

float4 ps_shadow_receiver(VsOut input) : SV_Target {
    const uint cascade = select_shadow_cascade(shadow_view_depth_along_forward(input.world_position));
    const float4 light_clip =
        mul(float4(input.world_position, 1.0), shadow_clip_from_world_cascades[cascade]);
    const float inv_w = 1.0 / max(abs(light_clip.w), 1e-6);
    const float receiver_depth = saturate(light_clip.z * inv_w);
    const float2 tile_uv = shadow_tile_uv_from_clip(light_clip);
    const float2 atlas_uv = shadow_atlas_uv(cascade, tile_uv);
    float4 sampled = base_color_texture.Sample(base_color_sampler, input.uv);
    const float shadow_factor = lerp(1.0, 0.42, directional_shadow_pcf_3x3(atlas_uv, receiver_depth));
    return float4(saturate(evaluate_lit_color(input, sampled, shadow_factor)), sampled.a * base_color.a);
}
"@
    $postprocessShader = @"
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

Texture2D scene_color_texture : register(t0);
SamplerState scene_color_sampler : register(s1);

struct VsOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VsOut vs_postprocess(uint vertex_id : SV_VertexID) {
    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0),
    };
    float2 uvs[3] = {
        float2(0.0, 1.0),
        float2(0.0, -1.0),
        float2(2.0, 1.0),
    };

    VsOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.uv = uvs[vertex_id];
    return output;
}

float4 ps_postprocess(VsOut input) : SV_Target {
    float4 scene = scene_color_texture.Sample(scene_color_sampler, input.uv);
    float3 graded = saturate(scene.rgb * float3(1.04, 1.02, 0.98) + float3(0.012, 0.008, 0.0));
    return float4(graded, scene.a);
}
"@

    $package.Files["source/materials/lit.material"] = $package.Files["runtime/assets/generated/lit.material"]
    $package.Files["shaders/runtime_scene.hlsl"] = ConvertTo-LfText -Text $sceneShader
    $package.Files["shaders/runtime_postprocess.hlsl"] = ConvertTo-LfText -Text $postprocessShader
    return $package
}

function New-HeadlessReadme {
    param(
        [string]$Title
    )

    $template = @'
# __TITLE__

## Goal

Describe the game goal here before expanding gameplay.

## Current Runtime

This game uses the headless GameEngine runtime:

- `mirakana::GameApp`
- `mirakana::HeadlessRunner`
- `mirakana::Registry`
- `mirakana::ILogger`

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```
'@
    return $template.Replace("__TITLE__", $Title)
}

function New-DesktopRuntimeReadme {
    param(
        [string]$Title,
        [string]$TargetName
    )

    $template = @'
# __TITLE__

## Goal

Describe the desktop runtime game goal here before expanding gameplay.

## Current Runtime

This game uses the optional desktop runtime package lane:

- `mirakana::GameApp`
- `mirakana::SdlDesktopGameHost`
- `mirakana::IRenderer` with deterministic NullRenderer fallback unless host-owned shader artifacts are added later
- `game.agent.json.runtimePackageFiles` plus `PACKAGE_FILES_FROM_MANIFEST`

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget __TARGET_NAME__
```
'@
    return $template.Replace("__TITLE__", $Title).Replace("__TARGET_NAME__", $TargetName)
}

function New-DesktopRuntimeCookedSceneReadme {
    param(
        [string]$Title,
        [string]$TargetName,
        [string]$GameName
    )

    $template = @'
# __TITLE__

## Goal

Describe the desktop runtime game goal here before expanding gameplay.

## Current Runtime

This game uses the optional desktop runtime package lane with a first-party cooked scene package:

- `mirakana::GameApp`
- `mirakana::SdlDesktopGameHost`
- `mirakana::RootedFileSystem`
- `mirakana::runtime::load_runtime_asset_package`
- `mirakana::instantiate_runtime_scene_render_data`
- `mirakana::submit_scene_render_packet`
- deterministic `NullRenderer` fallback unless host-owned shader artifacts are added later
- `game.agent.json.runtimePackageFiles` plus `PACKAGE_FILES_FROM_MANIFEST`
- `game.agent.json.runtimeSceneValidationTargets`
- `game.agent.json.packageStreamingResidencyTargets` as host-gated safe-point package streaming intent

The generated package proves config and cooked scene loading only. It does not generate D3D12/Vulkan/Metal shader artifacts, execute package streaming, or claim scene GPU binding.

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget __TARGET_NAME__
```

The installed package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex
```
'@
    return $template.Replace("__TITLE__", $Title).Replace("__TARGET_NAME__", $TargetName).Replace("__GAME_NAME__", $GameName)
}

function New-DesktopRuntime2DReadme {
    param(
        [string]$Title,
        [string]$TargetName,
        [string]$GameName
    )

    $template = @'
# __TITLE__

## Goal

Describe the packaged 2D game goal here before expanding gameplay.

## Current Runtime

This game uses the optional desktop runtime package lane with a first-party cooked 2D scene package:

- `mirakana::GameApp`
- `mirakana::SdlDesktopGameHost`
- `mirakana::runtime::RuntimeInputActionMap`
- `mirakana::runtime::load_runtime_asset_package`
- `mirakana::runtime::runtime_sprite_animation_payload`
- `mirakana::instantiate_runtime_scene_render_data`
- `mirakana::validate_playable_2d_scene`
- `mirakana::sample_and_apply_runtime_scene_render_sprite_animation`
- `mirakana::submit_scene_render_packet`
- `mirakana::ui::UiDocument` plus `mirakana::submit_ui_renderer_submission`
- `mirakana::AudioMixer` over a cooked audio payload
- host-gated native RHI 2D sprite overlay package smoke with generated shader artifacts
- deterministic `NullRenderer` fallback
- `game.agent.json.runtimePackageFiles`
- `game.agent.json.atlasTilemapAuthoringTargets` for deterministic package data only
- `game.agent.json.runtimeSceneValidationTargets`
- `game.agent.json.packageStreamingResidencyTargets` as host-gated safe-point package streaming intent
- `PACKAGE_FILES_FROM_MANIFEST`

The generated package proves cooked sprite/material/audio/scene loading, first-party cooked sprite animation frame sampling and application, deterministic data-only tilemap metadata with visible-cell runtime sampling counters through `--require-tilemap-runtime-ux`, 2D scene validation, HUD submission, audio cue intent, package smoke validation, and a host-gated D3D12 native 2D sprite overlay smoke through `--require-native-2d-sprites`. It does not claim production atlas packing, full tilemap editor UX, runtime image decoding, production sprite batching, package streaming execution, Metal readiness, public native/RHI handles, or general renderer quality.

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget __TARGET_NAME__
```

The installed package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex --require-d3d12-shaders --video-driver windows --require-d3d12-renderer --require-native-2d-sprites --require-sprite-animation --require-tilemap-runtime-ux
```
'@
    return $template.Replace("__TITLE__", $Title).Replace("__TARGET_NAME__", $TargetName).Replace("__GAME_NAME__", $GameName)
}

function New-DesktopRuntime3DReadme {
    param(
        [string]$Title,
        [string]$TargetName,
        [string]$GameName
    )

    $template = @'
# __TITLE__

## Goal

Describe the packaged 3D game goal here before expanding gameplay.

## Current Runtime

This `DesktopRuntime3DPackage` game uses the optional desktop runtime package lane with a first-party cooked 3D scene package:

- `mirakana::GameApp`
- `mirakana::SdlDesktopGameHost`
- `mirakana::runtime::load_runtime_asset_package`
- `mirakana::instantiate_runtime_scene_render_data`
- `mirakana::sample_and_apply_runtime_scene_render_animation_float_clip`
- `mirakana::sample_runtime_morph_mesh_cpu_animation_float_clip`
- `mirakana::runtime::runtime_animation_quaternion_clip_payload`
- `mirakana::sample_animation_local_pose_3d`
- deterministic public gameplay systems composition through `MK_physics`, `MK_navigation`, `MK_ai`, `MK_audio`, and `MK_animation`
- `mirakana::submit_scene_render_packet`
- primary camera/controller movement over cooked scene data
- cooked scalar transform animation applied to the packaged mesh through first-party binding rows
- cooked morph mesh CPU payload consumed with scalar morph-weight animation smoke counters
- cooked quaternion local-pose animation consumed through first-party `MK_animation` sampling counters
- selected host-gated package streaming safe-point smoke over the packaged scene validation target
- selected generated 3D renderer quality smoke over scene GPU, depth-aware postprocess, and framegraph=2 counters
- selected generated 3D postprocess depth-input smoke with postprocess_depth_input_ready=1 and renderer_quality_postprocess_depth_input_ready=1
- selected generated 3D playable package smoke through `--require-playable-3d-slice`
- selected generated 3D directional shadow package smoke with `directional_shadow_ready=1` and fixed PCF 3x3 filtering counters
- selected D3D12 generated 3D graphics morph + directional shadow receiver package smoke through `--require-shadow-morph-composition`
- selected generated 3D gameplay systems package smoke with `gameplay_systems_status=ready`, `gameplay_systems_ready=1`, physics authored collision/controller counters, navigation, AI perception/behavior, audio stream, and animation/lifecycle counters
- selected generated 3D scene collision package smoke with `--require-scene-collision-package`, `collision_package_status=ready`, `collision_package_bodies=3`, `collision_package_trigger_overlaps=1`, and `gameplay_systems_collision_package_ready=1`
- selected D3D12 visible generated 3D production-style package proof with `--require-visible-3d-production-proof`, `visible_3d_status=ready`, `visible_3d_presented_frames=2`, D3D12 selection, scene GPU, postprocess, renderer quality, playable aggregate, and native UI overlay readiness counters
- selected D3D12 generated 3D native UI overlay HUD box package smoke with `--require-native-ui-overlay`, `hud_boxes=2`, `ui_overlay_ready=1`, `ui_overlay_sprites_submitted=2`, and `ui_overlay_draws=2`
- selected D3D12 generated 3D cooked UI atlas image sprite package smoke with `--require-native-ui-textured-sprite-atlas`, `hud_images=2`, `ui_atlas_metadata_status=ready`, `ui_texture_overlay_atlas_ready=1`, `ui_texture_overlay_sprites_submitted=2`, `ui_texture_overlay_texture_binds=2`, and `ui_texture_overlay_draws=2`
- selected D3D12 generated 3D cooked UI atlas text glyph package smoke with `--require-native-ui-text-glyph-atlas`, `hud_text_glyphs=2`, `text_glyphs_resolved=2`, `text_glyphs_missing=0`, `ui_atlas_metadata_glyphs=1`, `ui_texture_overlay_sprites_submitted=2`, `ui_texture_overlay_texture_binds=2`, and `ui_texture_overlay_draws=2`
- static mesh, material, animation, morph, quaternion animation, and directional light package payloads
- host-built D3D12 scene/postprocess/shadow/native UI overlay shader artifacts when the selected package target is validated
- Vulkan SPIR-V artifacts only when DXC SPIR-V CodeGen and `spirv-val` are available and requested
- deterministic `NullRenderer` fallback
- `game.agent.json.runtimePackageFiles`
- `game.agent.json.runtimeSceneValidationTargets`
- `game.agent.json.packageStreamingResidencyTargets` as host-gated safe-point package streaming intent
- `PACKAGE_FILES_FROM_MANIFEST`

The generated package proves cooked texture/mesh/skinned-mesh/material/animation/morph/quaternion-animation/scene/physics-collision loading, runtime scene validation target descriptors, camera/controller package smoke validation, transform animation binding smoke validation, morph package consumption smoke validation, quaternion local-pose sampling smoke validation, selected host-gated safe-point package streaming counters through `--require-package-streaming-safe-point`, selected generated gameplay systems counters through `--require-gameplay-systems`, selected package collision counters through `--require-scene-collision-package`, generated 3D renderer quality counters through `--require-renderer-quality-gates` for scene GPU + depth-aware postprocess + framegraph=2 only, generated 3D postprocess depth-input counters through `--require-postprocess-depth-input`, selected generated 3D directional shadow counters through `--require-directional-shadow --require-directional-shadow-filtering`, selected D3D12 generated 3D graphics morph + directional shadow receiver counters through `--require-shadow-morph-composition`, selected D3D12 visible generated 3D production-style package counters through `--require-visible-3d-production-proof`, selected D3D12 generated 3D native UI overlay HUD box counters through `--require-native-ui-overlay`, selected D3D12 generated 3D cooked UI atlas image sprite counters through `--require-native-ui-textured-sprite-atlas`, selected D3D12 generated 3D cooked UI atlas text glyph counters through `--require-native-ui-text-glyph-atlas`, selected generated 3D playable package counters through `--require-playable-3d-slice`, D3D12 compute morph dispatch into renderer-consumed POSITION/NORMAL/TANGENT buffers, generated D3D12 skin+compute package smoke counters, Vulkan POSITION/NORMAL/TANGENT compute morph package smoke through explicit SPIR-V artifacts, Vulkan skin+compute package smoke counters through explicit SPIR-V artifacts, and selected-target shader artifact metadata. It does not claim runtime source parsing, broad dependency cooking, broad async/background package streaming, scene/physics perception integration, navmesh/crowd, middleware, production physics middleware/native backend readiness, CCD, joints, production text shaping, font rasterization, glyph atlas generation, runtime source image decoding, source image atlas packing, authored animation graph workflows, broad skeletal renderer deformation, broad directional shadow production quality, morph-deformed shadow-caster silhouettes, compute morph + shadow composition, broad shadow+morph composition, Metal compute morph deformation, async compute overlap/performance, broad frame graph scheduling, graphics morph+skin composition beyond the host-owned skin+compute package smoke, material/shader graphs, live shader generation, editor productization, native/RHI handle exposure, Vulkan/Metal parity for the visible proof, general renderer quality, or broad generated 3D production readiness.

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget __TARGET_NAME__
```

The installed D3D12 package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice
```

The selected D3D12 directional shadow package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-directional-shadow --require-directional-shadow-filtering --require-renderer-quality-gates
```

The selected D3D12 shadow + graphics morph package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-shadow-morph-composition
```

The selected D3D12 native UI overlay HUD box package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay
```

The selected D3D12 visible generated 3D production-style package proof uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay --require-visible-3d-production-proof
```

The selected scene collision package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex --require-scene-collision-package
```

The selected D3D12 native UI textured sprite atlas package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay --require-native-ui-textured-sprite-atlas
```

The selected D3D12 native UI text glyph atlas package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay --require-native-ui-text-glyph-atlas
```

The Vulkan package lane is toolchain-gated:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget __TARGET_NAME__ -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/__GAME_NAME__.config', '--require-scene-package', 'runtime/__GAME_NAME__.geindex', '--require-primary-camera-controller', '--require-transform-animation', '--require-morph-package', '--require-compute-morph', '--require-compute-morph-normal-tangent', '--require-quaternion-animation', '--require-package-streaming-safe-point', '--require-gameplay-systems', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-renderer-quality-gates', '--require-playable-3d-slice')
```

The selected Vulkan directional shadow package lane is toolchain-gated:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget __TARGET_NAME__ -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/__GAME_NAME__.config', '--require-scene-package', 'runtime/__GAME_NAME__.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-directional-shadow', '--require-directional-shadow-filtering', '--require-renderer-quality-gates')
```
'@
    return $template.Replace("__TITLE__", $Title).Replace("__TARGET_NAME__", $TargetName).Replace("__GAME_NAME__", $GameName)
}

function New-HeadlessManifest {
    param(
        [string]$GameName,
        [string]$DisplayTitle,
        [string]$TargetName
    )

    $manifestName = $GameName.Replace("_", "-")

    return [ordered]@{
        schemaVersion = 1
        name = $manifestName
        displayName = $DisplayTitle
        language = "C++23"
        entryPoint = "games/$GameName/main.cpp"
        target = $TargetName
        engineModules = @("MK_core", "MK_assets", "MK_math", "MK_platform", "MK_renderer", "MK_scene")
        aiWorkflow = [ordered]@{
            spec = "games/$GameName/README.md"
            validate = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1"
            allowedEditRoots = @("games/$GameName", "tests")
        }
        gameplayContract = [ordered]@{
            appType = "mirakana::GameApp"
            runner = "mirakana::HeadlessRunner"
            currentRuntime = "headless"
        }
        backendReadiness = [ordered]@{
            platform = "headless"
            graphics = "null"
            audio = "device-independent"
            ui = "MK_ui-headless"
            physics = "not-required"
        }
        importerRequirements = [ordered]@{
            sourceFormats = @("first-party")
            cookedOnlyRuntime = $true
            externalImportersRequired = @()
        }
        packagingTargets = @("source-tree-default")
        validationRecipes = @(
            [ordered]@{
                name = "default"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1"
            }
        )
    }
}

function New-DesktopRuntimeManifest {
    param(
        [string]$GameName,
        [string]$DisplayTitle,
        [string]$TargetName
    )

    $manifestName = $GameName.Replace("_", "-")

    return [ordered]@{
        schemaVersion = 1
        name = $manifestName
        displayName = $DisplayTitle
        language = "C++23"
        entryPoint = "games/$GameName/main.cpp"
        target = $TargetName
        engineModules = @(
            "MK_core",
            "MK_math",
            "MK_platform",
            "MK_platform_sdl3",
            "MK_renderer",
            "MK_runtime_host",
            "MK_runtime_host_sdl3",
            "MK_runtime_host_sdl3_presentation"
        )
        aiWorkflow = [ordered]@{
            spec = "games/$GameName/README.md"
            validate = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1"
            allowedEditRoots = @("games/$GameName", "tests")
        }
        gameplayContract = [ordered]@{
            appType = "mirakana::GameApp"
            runner = "mirakana::SdlDesktopGameHost"
            currentRuntime = "desktop-windowed-sdl3-host-with-null-fallback-packaged-config-smoke"
        }
        backendReadiness = [ordered]@{
            platform = "sdl3-desktop"
            graphics = "null-fallback; generated scaffold does not create D3D12/Vulkan shader artifacts"
            audio = "device-independent"
            ui = "MK_ui-headless"
            physics = "not-required"
        }
        importerRequirements = [ordered]@{
            sourceFormats = @("first-party")
            cookedOnlyRuntime = $true
            externalImportersRequired = @()
        }
        packagingTargets = @("desktop-game-runtime", "desktop-runtime-release")
        runtimePackageFiles = @("runtime/$GameName.config")
        validationRecipes = @(
            [ordered]@{
                name = "desktop-game-runtime"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1"
            },
            [ordered]@{
                name = "desktop-runtime-release-target"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget $TargetName"
            },
            [ordered]@{
                name = "installed-config-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config"
            }
        )
    }
}

function New-DesktopRuntimeCookedSceneManifest {
    param(
        [string]$GameName,
        [string]$DisplayTitle,
        [string]$TargetName
    )

    $manifestName = $GameName.Replace("_", "-")

    return [ordered]@{
        schemaVersion = 1
        name = $manifestName
        displayName = $DisplayTitle
        language = "C++23"
        entryPoint = "games/$GameName/main.cpp"
        target = $TargetName
        engineModules = @(
            "MK_core",
            "MK_math",
            "MK_platform",
            "MK_platform_sdl3",
            "MK_renderer",
            "MK_runtime",
            "MK_runtime_rhi",
            "MK_runtime_host",
            "MK_runtime_host_sdl3",
            "MK_runtime_host_sdl3_presentation",
            "MK_scene",
            "MK_scene_renderer"
        )
        aiWorkflow = [ordered]@{
            spec = "games/$GameName/README.md"
            validate = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1"
            allowedEditRoots = @("games/$GameName", "tests")
        }
        gameplayContract = [ordered]@{
            appType = "mirakana::GameApp"
            runner = "mirakana::SdlDesktopGameHost"
            currentRuntime = "desktop-windowed-sdl3-host-with-null-fallback-packaged-config-cooked-scene-smoke"
        }
        backendReadiness = [ordered]@{
            platform = "sdl3-desktop"
            graphics = "null-fallback; generated scaffold does not create D3D12/Vulkan/Metal shader artifacts or scene GPU bindings"
            audio = "device-independent"
            ui = "MK_ui-headless"
            physics = "not-required"
        }
        importerRequirements = [ordered]@{
            sourceFormats = @("first-party-cooked-fixture")
            cookedOnlyRuntime = $true
            externalImportersRequired = @()
        }
        packagingTargets = @("desktop-game-runtime", "desktop-runtime-release")
        runtimePackageFiles = @(
            "runtime/$GameName.config",
            "runtime/$GameName.geindex",
            "runtime/assets/generated/base_color.texture.geasset",
            "runtime/assets/generated/triangle.mesh",
            "runtime/assets/generated/lit.material",
            "runtime/assets/generated/packaged_scene.scene"
        )
        runtimeSceneValidationTargets = @(
            [ordered]@{
                id = "packaged-scene"
                packageIndexPath = "runtime/$GameName.geindex"
                sceneAssetKey = "$manifestName/scenes/packaged-scene"
                validateAssetReferences = $true
                requireUniqueNodeNames = $true
            }
        )
        packageStreamingResidencyTargets = @(
            [ordered]@{
                id = "packaged-scene-residency-budget"
                packageIndexPath = "runtime/$GameName.geindex"
                runtimeSceneValidationTargetId = "packaged-scene"
                mode = "host-gated-safe-point"
                residentBudgetBytes = 33554432
                safePointRequired = $true
                maxResidentPackages = 1
                preloadAssetKeys = @("$manifestName/scenes/packaged-scene")
                residentResourceKinds = @("texture", "mesh", "material", "scene")
                preflightRecipeIds = @("desktop-game-runtime", "desktop-runtime-release-target", "installed-scene-package-smoke")
            }
        )
        validationRecipes = @(
            [ordered]@{
                name = "desktop-game-runtime"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1"
            },
            [ordered]@{
                name = "desktop-runtime-release-target"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget $TargetName"
            },
            [ordered]@{
                name = "installed-scene-package-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex"
            }
        )
    }
}

function New-DesktopRuntime2DManifest {
    param(
        [string]$GameName,
        [string]$DisplayTitle,
        [string]$TargetName
    )

    $assetKeyPrefix = $GameName.Replace("_", "-")

    return [ordered]@{
        schemaVersion = 1
        name = $GameName
        displayName = $DisplayTitle
        language = "C++23"
        entryPoint = "games/$GameName/main.cpp"
        target = $TargetName
        engineModules = @(
            "MK_core",
            "MK_math",
            "MK_platform",
            "MK_platform_sdl3",
            "MK_renderer",
            "MK_runtime",
            "MK_runtime_scene",
            "MK_runtime_host",
            "MK_runtime_host_sdl3",
            "MK_runtime_host_sdl3_presentation",
            "MK_scene",
            "MK_scene_renderer",
            "MK_ui",
            "MK_ui_renderer",
            "MK_audio"
        )
        aiWorkflow = [ordered]@{
            spec = "games/$GameName/README.md"
            validate = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1"
            allowedEditRoots = @("games/$GameName", "tests")
        }
        gameplayContract = [ordered]@{
            productionRecipe = "2d-desktop-runtime-package"
            appType = "mirakana::GameApp"
            runner = "mirakana::SdlDesktopGameHost"
            input = "mirakana::runtime::RuntimeInputActionMap over mirakana::VirtualInput"
            scene = "GameEngine.Scene.v1 loaded from a cooked package and validated by mirakana::validate_playable_2d_scene"
            sceneRenderer = "mirakana::submit_scene_render_packet"
            ui = "mirakana::ui::UiDocument through mirakana::submit_ui_renderer_submission"
            audio = "mirakana::AudioMixer using a cooked audio payload"
            renderer = "mirakana::IRenderer from the desktop host with deterministic NullRenderer fallback or host-owned RHI-backed native 2D sprite overlay when packaged shader artifacts are present"
            currentRuntime = "generated host-gated SDL3 desktop runtime package proof for 2D gameplay. D3D12 package smoke uses generated shader artifacts and --require-native-2d-sprites so cooked scene sprite texture/material identity and HUD submission flow through the host-owned native RHI sprite overlay path with native_2d_sprite_batches_executed counters. The sprite animation package proof uses a first-party cooked sprite_animation payload and --require-sprite-animation so deterministic frame sampling and sprite frame application emit sprite_animation_frames_sampled and sprite_animation_frames_applied counters. The tilemap runtime UX proof uses first-party GameEngine.Tilemap.v1 metadata and --require-tilemap-runtime-ux so visible tile cells emit tilemap_cells_sampled and tilemap_diagnostics counters without claiming production atlas packing or full tilemap editor UX. public native or RHI handle access remains unsupported, broad production sprite batching readiness remains unsupported, and general production renderer quality remains unsupported."
        }
        backendReadiness = [ordered]@{
            platform = "sdl3-desktop-host-gated"
            graphics = "NullRenderer fallback plus host-gated D3D12 native 2D sprite batch execution counters through generated shader artifacts. public native or RHI handle access remains unsupported, broad production sprite batching readiness remains unsupported, and general production renderer quality remains unsupported."
            audio = "device-independent cooked audio payload mixed through MK_audio"
            ui = "MK_ui-headless renderer submission"
            physics = "not-required"
        }
        importerRequirements = [ordered]@{
            sourceFormats = @("first-party-cooked-fixture")
            cookedOnlyRuntime = $true
            externalImportersRequired = @()
        }
        packagingTargets = @("desktop-game-runtime", "desktop-runtime-release")
        runtimePackageFiles = @(
            "runtime/$GameName.config",
            "runtime/$GameName.geindex",
            "runtime/assets/2d/player.texture.geasset",
            "runtime/assets/2d/player.material",
            "runtime/assets/2d/jump.audio.geasset",
            "runtime/assets/2d/level.tilemap",
            "runtime/assets/2d/player.sprite_animation",
            "runtime/assets/2d/playable.scene"
        )
        atlasTilemapAuthoringTargets = @(
            [ordered]@{
                id = "packaged-2d-tilemap"
                packageIndexPath = "runtime/$GameName.geindex"
                tilemapPath = "runtime/assets/2d/level.tilemap"
                atlasTexturePath = "runtime/assets/2d/player.texture.geasset"
                tilemapAssetKey = "$assetKeyPrefix/tilemaps/level"
                atlasTextureAssetKey = "$assetKeyPrefix/textures/player"
                mode = "deterministic-package-data"
                sourceDecoding = "unsupported"
                atlasPacking = "unsupported"
                nativeGpuSpriteBatching = "unsupported"
                preflightRecipeIds = @("desktop-game-runtime", "desktop-runtime-release-target", "installed-2d-package-smoke", "installed-2d-sprite-animation-smoke", "installed-2d-tilemap-runtime-ux-smoke", "installed-native-2d-sprite-smoke")
            }
        )
        runtimeSceneValidationTargets = @(
            [ordered]@{
                id = "packaged-2d-scene"
                packageIndexPath = "runtime/$GameName.geindex"
                sceneAssetKey = "$assetKeyPrefix/scenes/packaged-2d-scene"
                validateAssetReferences = $true
                requireUniqueNodeNames = $true
            }
        )
        packageStreamingResidencyTargets = @(
            [ordered]@{
                id = "packaged-2d-residency-budget"
                packageIndexPath = "runtime/$GameName.geindex"
                runtimeSceneValidationTargetId = "packaged-2d-scene"
                mode = "host-gated-safe-point"
                residentBudgetBytes = 33554432
                safePointRequired = $true
                maxResidentPackages = 1
                preloadAssetKeys = @("$assetKeyPrefix/scenes/packaged-2d-scene", "$assetKeyPrefix/tilemaps/level", "$assetKeyPrefix/animations/player-sprite-animation")
                residentResourceKinds = @("texture", "material", "scene", "audio", "tilemap", "sprite_animation")
                preflightRecipeIds = @("desktop-game-runtime", "desktop-runtime-release-target", "installed-2d-package-smoke", "installed-2d-sprite-animation-smoke", "installed-2d-tilemap-runtime-ux-smoke", "installed-native-2d-sprite-smoke")
            }
        )
        validationRecipes = @(
            [ordered]@{
                name = "desktop-game-runtime"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1"
            },
            [ordered]@{
                name = "desktop-runtime-release-target"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget $TargetName"
            },
            [ordered]@{
                name = "installed-2d-package-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex"
            },
            [ordered]@{
                name = "installed-2d-sprite-animation-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-sprite-animation"
            },
            [ordered]@{
                name = "installed-2d-tilemap-runtime-ux-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-tilemap-runtime-ux"
            },
            [ordered]@{
                name = "installed-native-2d-sprite-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-d3d12-shaders --video-driver windows --require-d3d12-renderer --require-native-2d-sprites --require-sprite-animation --require-tilemap-runtime-ux"
            }
        )
    }
}

function New-DesktopRuntime3DManifest {
    param(
        [string]$GameName,
        [string]$DisplayTitle,
        [string]$TargetName
    )

    $assetKeyPrefix = $GameName.Replace("_", "-")

    return [ordered]@{
        schemaVersion = 1
        name = $assetKeyPrefix
        displayName = $DisplayTitle
        language = "C++23"
        entryPoint = "games/$GameName/main.cpp"
        target = $TargetName
        engineModules = @(
            "MK_ai",
            "MK_animation",
            "MK_audio",
            "MK_core",
            "MK_math",
            "MK_navigation",
            "MK_platform",
            "MK_platform_sdl3",
            "MK_physics",
            "MK_renderer",
            "MK_runtime",
            "MK_runtime_rhi",
            "MK_runtime_scene",
            "MK_runtime_scene_rhi",
            "MK_runtime_host",
            "MK_runtime_host_sdl3",
            "MK_runtime_host_sdl3_presentation",
            "MK_scene",
            "MK_scene_renderer",
            "MK_ui",
            "MK_ui_renderer"
        )
        aiWorkflow = [ordered]@{
            spec = "games/$GameName/README.md"
            validate = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1"
            allowedEditRoots = @("games/$GameName", "tests")
        }
        gameplayContract = [ordered]@{
            productionRecipe = "3d-playable-desktop-package"
            appType = "mirakana::GameApp"
            runner = "mirakana::SdlDesktopGameHost"
            input = "mirakana::VirtualInput deterministic right-key camera/controller movement over the cooked primary camera node"
            scene = "GameEngine.Scene.v1 loaded from a cooked package with static mesh, primary perspective camera, and directional light metadata"
            sceneRenderer = "mirakana::submit_scene_render_packet with material instance intent through first-party scene/material contracts"
            renderer = "mirakana::IRenderer from the desktop host with deterministic NullRenderer fallback and host-gated scene GPU binding when selected"
        currentRuntime = "generated host-gated SDL3 desktop runtime package proof for 3D gameplay with camera/controller movement, transform/quaternion animation, morph and compute morph package smokes, selected host-gated package streaming safe-point smoke, selected generated 3D renderer quality smoke over scene GPU + depth-aware postprocess + framegraph=2 counters, selected generated 3D postprocess depth-input smoke through postprocess_depth_input_ready=1 and renderer_quality_postprocess_depth_input_ready=1, selected generated 3D playable package smoke through playable_3d_* counters, selected generated 3D directional shadow package smoke through directional_shadow_* counters with fixed_pcf_3x3 filtering, selected D3D12 generated 3D graphics morph + directional shadow receiver smoke through --require-shadow-morph-composition, selected generated 3D gameplay systems package smoke through gameplay_systems_* counters over deterministic public physics, navigation, AI, audio, animation, and lifecycle APIs, selected generated 3D scene collision package smoke through --require-scene-collision-package, collision_package_status=ready, collision_package_bodies=3, collision_package_trigger_overlaps=1, and gameplay_systems_collision_package_ready=1, selected D3D12 visible generated 3D production-style package proof through --require-visible-3d-production-proof and visible_3d_* counters, selected D3D12 generated 3D native UI overlay HUD box package smoke through --require-native-ui-overlay, hud_boxes=2, ui_overlay_ready=1, ui_overlay_sprites_submitted=2, and ui_overlay_draws=2, selected D3D12 generated 3D cooked UI atlas image sprite package smoke through --require-native-ui-textured-sprite-atlas, hud_images=2, ui_atlas_metadata_status=ready, ui_texture_overlay_atlas_ready=1, ui_texture_overlay_sprites_submitted=2, ui_texture_overlay_texture_binds=2, and ui_texture_overlay_draws=2, and selected D3D12 generated 3D cooked UI atlas text glyph package smoke through --require-native-ui-text-glyph-atlas, hud_text_glyphs=2, text_glyphs_resolved=2, text_glyphs_missing=0, ui_atlas_metadata_glyphs=1, ui_texture_overlay_sprites_submitted=2, ui_texture_overlay_texture_binds=2, and ui_texture_overlay_draws=2; runtime source asset parsing remains unsupported; broad dependency cooking remains unsupported; broad async/background package streaming remains unsupported; scene/physics perception integration remains unsupported; navmesh/crowd remains unsupported; middleware remains unsupported; production physics middleware/native backend readiness, CCD, and joints remain unsupported; production text shaping, font rasterization, glyph atlas generation, runtime source image decoding, source image atlas packing, material graph, and live shader generation remain unsupported; broad directional shadow production quality, morph-deformed shadow-caster silhouettes, compute morph + shadow composition, and broad shadow+morph composition remain unsupported for this generated sample; public native or RHI handle access remains unsupported; Vulkan/Metal parity for the visible proof remains unsupported; Metal readiness remains unsupported; broad generated 3D production readiness remains unsupported"
        }
        backendReadiness = [ordered]@{
            platform = "sdl3-desktop-host-gated"
            graphics = "host-built D3D12 DXIL shader artifacts for selected package validation with optional generated 3D renderer quality counters for scene GPU + depth-aware postprocess + framegraph=2, selected visible generated 3D production-style counters, selected directional shadow smoke counters, selected D3D12 graphics morph + directional shadow receiver counters, selected D3D12 native UI overlay HUD box counters, selected D3D12 cooked UI atlas image sprite and text glyph texture overlay counters, and selected playable_3d_* package counters; Vulkan SPIR-V artifacts are toolchain-gated for generated 3D quality, shadow, playable, and native UI overlay artifacts, while visible proof, shadow-morph composition, native UI overlay readiness, and generated 3D cooked UI atlas texture overlay readiness remain D3D12-selected until separate Vulkan recipes land; NullRenderer fallback remains available; Metal readiness, broad shadow quality, morph-deformed shadow-caster silhouettes, compute morph + shadow composition, broad shadow+morph composition, production text/font UI, runtime source image decoding, source image atlas packing, GPU timestamps, backend-native stats, broad renderer quality, and broad generated 3D readiness remain unsupported"
            audio = "device-independent gameplay systems package smoke plus deterministic NullRenderer fallback"
            ui = "MK_ui HUD box submission plus one cooked GameEngine.UiAtlas.v1 image sprite and one cooked GameEngine.UiAtlas.v1 text glyph are validated through selected D3D12 host-owned native UI overlay package smokes; production text shaping, font rasterization, glyph atlas generation, runtime source image decoding, source image atlas packing, native accessibility bridges, Metal overlay readiness, and broad UI renderer quality remain unsupported"
        physics = "first-party deterministic authored-collision/controller package smoke plus selected GameEngine.PhysicsCollisionScene3D.v1 package collision smoke only; no middleware, native physics backend, joints, CCD, or scene/physics perception integration"
        }
        importerRequirements = [ordered]@{
            sourceFormats = @(
                "GameEngine.TextureSource.v1",
                "GameEngine.MeshSource.v2",
                "GameEngine.MorphMeshCpuSource.v1",
                "GameEngine.AnimationFloatClipSource.v1",
                "GameEngine.AnimationQuaternionClipSource.v1",
                "GameEngine.Material.v1",
                "GameEngine.SourceAssetRegistry.v1",
                "GameEngine.Scene.v2",
                "GameEngine.Prefab.v2",
                "first-party-material-source",
                "hlsl-source",
                "first-party-cooked-fixture"
            )
            cookedOnlyRuntime = $true
            externalImportersRequired = @()
        }
        packagingTargets = @("desktop-game-runtime", "desktop-runtime-release")
        runtimePackageFiles = @(
            "runtime/$GameName.config",
            "runtime/$GameName.geindex",
            "runtime/assets/3d/base_color.texture.geasset",
            "runtime/assets/3d/triangle.mesh",
            "runtime/assets/3d/packaged_mesh.morph_mesh_cpu",
            "runtime/assets/3d/lit.material",
            "runtime/assets/3d/packaged_mesh_bob.animation_float_clip",
            "runtime/assets/3d/packaged_mesh_morph_weights.animation_float_clip",
            "runtime/assets/3d/packaged_pose.animation_quaternion_clip",
            "runtime/assets/3d/skinned_triangle.skinned_mesh",
            "runtime/assets/3d/packaged_scene.scene",
            "runtime/assets/3d/hud.uiatlas",
            "runtime/assets/3d/hud_text.uiatlas",
            "runtime/assets/3d/collision.collision3d"
        )
        runtimeSceneValidationTargets = @(
            [ordered]@{
                id = "packaged-3d-scene"
                packageIndexPath = "runtime/$GameName.geindex"
                sceneAssetKey = "$assetKeyPrefix/scenes/packaged-3d-scene"
                validateAssetReferences = $true
                requireUniqueNodeNames = $true
            }
        )
        packageStreamingResidencyTargets = @(
            [ordered]@{
                id = "packaged-3d-residency-budget"
                packageIndexPath = "runtime/$GameName.geindex"
                runtimeSceneValidationTargetId = "packaged-3d-scene"
                mode = "host-gated-safe-point"
                residentBudgetBytes = 67108864
                safePointRequired = $true
                maxResidentPackages = 1
                preloadAssetKeys = @("$assetKeyPrefix/scenes/packaged-3d-scene")
                residentResourceKinds = @("texture", "mesh", "skinned_mesh", "morph_mesh_cpu", "material", "animation_float_clip", "animation_quaternion_clip", "ui_atlas", "scene", "physics_collision_scene")
                preflightRecipeIds = @("desktop-game-runtime", "desktop-runtime-release-target", "installed-d3d12-3d-package-smoke", "installed-d3d12-3d-directional-shadow-smoke", "installed-d3d12-3d-shadow-morph-composition-smoke", "installed-d3d12-3d-native-ui-overlay-smoke", "installed-d3d12-3d-visible-production-proof-smoke", "installed-d3d12-3d-scene-collision-package-smoke", "installed-d3d12-3d-native-ui-textured-sprite-atlas-smoke", "installed-d3d12-3d-native-ui-text-glyph-atlas-smoke")
            }
        )
        materialShaderAuthoringTargets = @(
            [ordered]@{
                id = "packaged-3d-lit-material-shaders"
                sourceMaterialPath = "source/materials/lit.material"
                runtimeMaterialPath = "runtime/assets/3d/lit.material"
                packageIndexPath = "runtime/$GameName.geindex"
                shaderSourcePaths = @(
                    "shaders/runtime_scene.hlsl",
                    "shaders/runtime_postprocess.hlsl",
                    "shaders/runtime_shadow.hlsl",
                    "shaders/runtime_ui_overlay.hlsl"
                )
                d3d12ShaderArtifactPaths = @(
                    "shaders/${TargetName}_scene.vs.dxil",
                    "shaders/${TargetName}_scene_morph.vs.dxil",
                    "shaders/${TargetName}_scene_compute_morph.vs.dxil",
                    "shaders/${TargetName}_scene_compute_morph.cs.dxil",
                    "shaders/${TargetName}_scene_compute_morph_tangent_frame.vs.dxil",
                    "shaders/${TargetName}_scene_compute_morph_tangent_frame.cs.dxil",
                    "shaders/${TargetName}_scene_compute_morph_skinned.vs.dxil",
                    "shaders/${TargetName}_scene_compute_morph_skinned.cs.dxil",
                    "shaders/${TargetName}_scene.ps.dxil",
                    "shaders/${TargetName}_postprocess.vs.dxil",
                    "shaders/${TargetName}_postprocess.ps.dxil",
                    "shaders/${TargetName}_ui_overlay.vs.dxil",
                    "shaders/${TargetName}_ui_overlay.ps.dxil",
                    "shaders/${TargetName}_shadow_receiver.ps.dxil",
                    "shaders/${TargetName}_shadow_receiver_shifted.ps.dxil",
                    "shaders/${TargetName}_shadow.vs.dxil",
                    "shaders/${TargetName}_shadow.ps.dxil"
                )
                vulkanShaderArtifactPaths = @(
                    "shaders/${TargetName}_scene.vs.spv",
                    "shaders/${TargetName}_scene_morph.vs.spv",
                    "shaders/${TargetName}_scene_compute_morph.vs.spv",
                    "shaders/${TargetName}_scene_compute_morph.cs.spv",
                    "shaders/${TargetName}_scene_compute_morph_tangent_frame.vs.spv",
                    "shaders/${TargetName}_scene_compute_morph_tangent_frame.cs.spv",
                    "shaders/${TargetName}_scene_compute_morph_skinned.vs.spv",
                    "shaders/${TargetName}_scene_compute_morph_skinned.cs.spv",
                    "shaders/${TargetName}_scene.ps.spv",
                    "shaders/${TargetName}_postprocess.vs.spv",
                    "shaders/${TargetName}_postprocess.ps.spv",
                    "shaders/${TargetName}_ui_overlay.vs.spv",
                    "shaders/${TargetName}_ui_overlay.ps.spv",
                    "shaders/${TargetName}_shadow_receiver.ps.spv",
                    "shaders/${TargetName}_shadow_receiver_shifted.ps.spv",
                    "shaders/${TargetName}_shadow.vs.spv",
                    "shaders/${TargetName}_shadow.ps.spv"
                )
                validateMaterialTextures = $true
                validateShaderArtifacts = $true
            }
        )
        prefabScenePackageAuthoringTargets = @(
            [ordered]@{
                id = "packaged-3d-prefab-scene"
                mode = "deterministic-3d-prefab-scene-package-data"
                sceneAuthoringPath = "source/scenes/packaged_scene.scene"
                prefabAuthoringPath = "source/prefabs/static_prop.prefab"
                sourceRegistryPath = "source/assets/package.geassets"
                packageIndexPath = "runtime/$GameName.geindex"
                outputScenePath = "runtime/assets/3d/packaged_scene.scene"
                sceneAssetKey = "$assetKeyPrefix/scenes/packaged-3d-scene"
                runtimeSceneValidationTargetId = "packaged-3d-scene"
                authoringCommandRows = @(
                    [ordered]@{
                        id = "create-packaged-scene"
                        surface = "GameEngine.Scene.v2"
                        operation = "create-scene"
                    },
                    [ordered]@{
                        id = "add-static-mesh-node"
                        surface = "GameEngine.Scene.v2"
                        operation = "add-scene-node"
                    },
                    [ordered]@{
                        id = "add-static-mesh-renderer"
                        surface = "GameEngine.Scene.v2"
                        operation = "add-or-update-component"
                    },
                    [ordered]@{
                        id = "create-static-prop-prefab"
                        surface = "GameEngine.Prefab.v2"
                        operation = "create-prefab"
                    },
                    [ordered]@{
                        id = "instantiate-static-prop-prefab"
                        surface = "GameEngine.Scene.v2"
                        operation = "instantiate-prefab"
                    }
                )
                selectedSourceAssetKeys = @(
                    "$assetKeyPrefix/textures/base-color",
                    "$assetKeyPrefix/meshes/triangle",
                    "$assetKeyPrefix/morphs/packaged-mesh",
                    "$assetKeyPrefix/materials/lit",
                    "$assetKeyPrefix/animations/packaged-mesh-bob",
                    "$assetKeyPrefix/animations/packaged-mesh-morph-weights",
                    "$assetKeyPrefix/animations/packaged-pose"
                )
                sourceCookMode = "selected-source-registry-rows"
                sceneMigration = "migrate-scene-v2-runtime-package"
                runtimeSceneValidation = "validate-runtime-scene-package"
                hostGatedSmokeRecipeIds = @(
                    "desktop-game-runtime",
                    "desktop-runtime-release-target",
                    "installed-d3d12-3d-package-smoke",
                    "installed-d3d12-3d-directional-shadow-smoke",
                    "installed-d3d12-3d-shadow-morph-composition-smoke",
                    "installed-d3d12-3d-native-ui-overlay-smoke",
                    "installed-d3d12-3d-visible-production-proof-smoke",
                    "installed-d3d12-3d-scene-collision-package-smoke",
                    "installed-d3d12-3d-native-ui-textured-sprite-atlas-smoke",
                    "installed-d3d12-3d-native-ui-text-glyph-atlas-smoke"
                )
                broadImporterExecution = "unsupported"
                broadDependencyCooking = "unsupported"
                runtimeSourceParsing = "unsupported"
                materialGraph = "unsupported"
                shaderGraph = "unsupported"
                liveShaderGeneration = "unsupported"
                skeletalAnimation = "unsupported"
                gpuSkinning = "unsupported"
                publicNativeRhiHandles = "unsupported"
                metalReadiness = "host-gated"
                rendererQuality = "unsupported"
            }
        )
        registeredSourceAssetCookTargets = @(
            [ordered]@{
                id = "packaged-3d-registered-source-cook"
                mode = "descriptor-only-cook-registered-source-assets"
                cookCommandId = "cook-registered-source-assets"
                prefabScenePackageAuthoringTargetId = "packaged-3d-prefab-scene"
                sourceRegistryPath = "source/assets/package.geassets"
                packageIndexPath = "runtime/$GameName.geindex"
                selectedAssetKeys = @("$assetKeyPrefix/materials/lit")
                dependencyExpansion = "registered_source_registry_closure"
                dependencyCooking = "registry_closure"
                externalImporterExecution = "unsupported"
                rendererRhiResidency = "unsupported"
                packageStreaming = "unsupported"
                materialGraph = "unsupported"
                shaderGraph = "unsupported"
                liveShaderGeneration = "unsupported"
                editorProductization = "unsupported"
                metalReadiness = "host-gated"
                publicNativeRhiHandles = "unsupported"
                generalProductionRendererQuality = "unsupported"
                arbitraryShell = "unsupported"
                freeFormEdit = "unsupported"
            }
        )
        validationRecipes = @(
            [ordered]@{
                name = "desktop-game-runtime"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1"
            },
            [ordered]@{
                name = "desktop-runtime-release-target"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget $TargetName"
            },
            [ordered]@{
                name = "installed-d3d12-3d-package-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice"
            },
            [ordered]@{
                name = "installed-d3d12-3d-native-ui-overlay-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay"
            },
            [ordered]@{
                name = "installed-d3d12-3d-visible-production-proof-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay --require-visible-3d-production-proof"
            },
            [ordered]@{
                name = "installed-d3d12-3d-scene-collision-package-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-scene-collision-package"
            },
            [ordered]@{
                name = "installed-d3d12-3d-native-ui-textured-sprite-atlas-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay --require-native-ui-textured-sprite-atlas"
            },
            [ordered]@{
                name = "installed-d3d12-3d-native-ui-text-glyph-atlas-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-primary-camera-controller --require-transform-animation --require-morph-package --require-compute-morph --require-compute-morph-normal-tangent --require-compute-morph-skin --require-compute-morph-async-telemetry --require-quaternion-animation --require-package-streaming-safe-point --require-gameplay-systems --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-renderer-quality-gates --require-playable-3d-slice --require-native-ui-overlay --require-native-ui-text-glyph-atlas"
            },
            [ordered]@{
                name = "installed-d3d12-3d-directional-shadow-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess --require-postprocess-depth-input --require-directional-shadow --require-directional-shadow-filtering --require-renderer-quality-gates"
            },
            [ordered]@{
                name = "installed-d3d12-3d-shadow-morph-composition-smoke"
                command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-shadow-morph-composition"
            },
            [ordered]@{
                name = "desktop-runtime-release-target-vulkan-toolchain-gated"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget $TargetName -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/$GameName.config', '--require-scene-package', 'runtime/$GameName.geindex', '--require-primary-camera-controller', '--require-transform-animation', '--require-morph-package', '--require-compute-morph', '--require-compute-morph-normal-tangent', '--require-quaternion-animation', '--require-package-streaming-safe-point', '--require-gameplay-systems', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-renderer-quality-gates', '--require-playable-3d-slice')"
            },
            [ordered]@{
                name = "desktop-runtime-release-target-vulkan-directional-shadow-toolchain-gated"
                command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget $TargetName -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/$GameName.config', '--require-scene-package', 'runtime/$GameName.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-directional-shadow', '--require-directional-shadow-filtering', '--require-renderer-quality-gates')"
            }
        )
    }
}

function New-DesktopRuntimeMaterialShaderManifest {
    param(
        [string]$GameName,
        [string]$DisplayTitle,
        [string]$TargetName
    )

    $manifestName = $GameName.Replace("_", "-")
    $manifest = New-DesktopRuntimeCookedSceneManifest -GameName $GameName -DisplayTitle $DisplayTitle -TargetName $TargetName
    $manifest.gameplayContract.currentRuntime =
        "desktop-windowed-sdl3-host-with-host-built-d3d12-vulkan-shader-artifacts-and-null-fallback"
    $manifest.backendReadiness.graphics =
        "host-built D3D12 DXIL shader artifacts; Vulkan SPIR-V artifacts are toolchain-gated; NullRenderer fallback remains available"
    $manifest.importerRequirements.sourceFormats = @(
        "first-party-material-source",
        "hlsl-source",
        "first-party-cooked-fixture"
    )
    $manifest.materialShaderAuthoringTargets = @(
        [ordered]@{
            id = "generated-lit-material-shaders"
            sourceMaterialPath = "source/materials/lit.material"
            runtimeMaterialPath = "runtime/assets/generated/lit.material"
            packageIndexPath = "runtime/$GameName.geindex"
            shaderSourcePaths = @(
                "shaders/runtime_scene.hlsl",
                "shaders/runtime_postprocess.hlsl"
            )
            d3d12ShaderArtifactPaths = @(
                "shaders/${TargetName}_scene.vs.dxil",
                "shaders/${TargetName}_scene.ps.dxil",
                "shaders/${TargetName}_postprocess.vs.dxil",
                "shaders/${TargetName}_postprocess.ps.dxil"
            )
            vulkanShaderArtifactPaths = @(
                "shaders/${TargetName}_scene.vs.spv",
                "shaders/${TargetName}_scene.ps.spv",
                "shaders/${TargetName}_postprocess.vs.spv",
                "shaders/${TargetName}_postprocess.ps.spv"
            )
            validateMaterialTextures = $true
            validateShaderArtifacts = $true
        }
    )
    $manifest.validationRecipes = @(
        [ordered]@{
            name = "desktop-game-runtime"
            command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1"
        },
        [ordered]@{
            name = "desktop-runtime-release-target"
            command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget $TargetName"
        },
        [ordered]@{
            name = "installed-d3d12-scene-gpu-postprocess-smoke"
            command = "out\install\desktop-runtime-release\bin\$TargetName.exe --smoke --require-config runtime/$GameName.config --require-scene-package runtime/$GameName.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess"
        },
        [ordered]@{
            name = "desktop-runtime-release-target-vulkan-toolchain-gated"
            command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget $TargetName -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/$GameName.config', '--require-scene-package', 'runtime/$GameName.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess')"
        }
    )
    $manifest.packageStreamingResidencyTargets = @(
        [ordered]@{
            id = "packaged-scene-residency-budget"
            packageIndexPath = "runtime/$GameName.geindex"
            runtimeSceneValidationTargetId = "packaged-scene"
            mode = "host-gated-safe-point"
            residentBudgetBytes = 67108864
            safePointRequired = $true
            maxResidentPackages = 1
            preloadAssetKeys = @("$manifestName/scenes/packaged-scene")
            residentResourceKinds = @("texture", "mesh", "material", "scene")
            preflightRecipeIds = @("desktop-game-runtime", "desktop-runtime-release-target", "installed-d3d12-scene-gpu-postprocess-smoke")
        }
    )
    return $manifest
}

function New-DesktopRuntimeMaterialShaderReadme {
    param(
        [string]$Title,
        [string]$TargetName,
        [string]$GameName
    )

    $template = @'
# __TITLE__

## Goal

Describe the desktop runtime game goal here before expanding gameplay.

## Current Runtime

This game uses the optional desktop runtime package lane with first-party material/shader authoring inputs and a cooked scene package:

- `mirakana::GameApp`
- `mirakana::SdlDesktopGameHost`
- `mirakana::RootedFileSystem`
- `mirakana::runtime::load_runtime_asset_package`
- `mirakana::instantiate_runtime_scene_render_data`
- `mirakana::submit_scene_render_packet`
- `source/materials/lit.material` as the authoring material mirror for the cooked runtime material
- `shaders/runtime_scene.hlsl` and `shaders/runtime_postprocess.hlsl` as host-built shader inputs
- D3D12 DXIL artifacts installed by the selected desktop runtime package target
- Vulkan SPIR-V artifacts only when DXC SPIR-V CodeGen and `spirv-val` are available and requested
- deterministic `NullRenderer` fallback when native presentation gates are unavailable
- `game.agent.json.runtimePackageFiles` plus `PACKAGE_FILES_FROM_MANIFEST`
- `game.agent.json.materialShaderAuthoringTargets` for the source material, cooked runtime material, fixed HLSL inputs, and selected shader artifact paths
- `game.agent.json.packageStreamingResidencyTargets` as host-gated safe-point package streaming intent

The generated game does not runtime-compile shaders, expose native handles to gameplay, create a shader graph, generate Metal libraries, execute package streaming, or ship source material/HLSL files as runtime package payloads.

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget __TARGET_NAME__
```

The installed D3D12 package smoke uses:

```powershell
out\install\desktop-runtime-release\bin\__TARGET_NAME__.exe --smoke --require-config runtime/__GAME_NAME__.config --require-scene-package runtime/__GAME_NAME__.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-scene-gpu-bindings --require-postprocess
```

The Vulkan package lane is toolchain-gated:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget __TARGET_NAME__ -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/__GAME_NAME__.config', '--require-scene-package', 'runtime/__GAME_NAME__.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess')
```
'@
    return $template.Replace("__TITLE__", $Title).Replace("__TARGET_NAME__", $TargetName).Replace("__GAME_NAME__", $GameName)
}

function New-HeadlessRegistration {
    param(
        [string]$GameName,
        [string]$TargetName
    )

    return @"

MK_add_game($TargetName
    SOURCES
        $GameName/main.cpp
)
"@
}

function New-DesktopRuntimeRegistration {
    param(
        [string]$GameName,
        [string]$TargetName
    )

    return @"

if(MK_DESKTOP_RUNTIME_ENABLED)
    MK_add_desktop_runtime_game($TargetName
        SOURCES
            $GameName/main.cpp
        GAME_MANIFEST
            games/$GameName/game.agent.json
        SMOKE_ARGS
            --smoke
            --require-config
            runtime/$GameName.config
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/$GameName.config
        PACKAGE_FILES_FROM_MANIFEST
    )
endif()
"@
}

function New-DesktopRuntimeCookedSceneRegistration {
    param(
        [string]$GameName,
        [string]$TargetName
    )

    return @"

if(MK_DESKTOP_RUNTIME_ENABLED)
    MK_add_desktop_runtime_game($TargetName
        SOURCES
            $GameName/main.cpp
        GAME_MANIFEST
            games/$GameName/game.agent.json
        SMOKE_ARGS
            --smoke
            --require-config
            runtime/$GameName.config
            --require-scene-package
            runtime/$GameName.geindex
            --require-sprite-animation
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/$GameName.config
            --require-scene-package
            runtime/$GameName.geindex
        PACKAGE_FILES_FROM_MANIFEST
    )
    target_link_libraries($TargetName
        PRIVATE
            MK_runtime
            MK_scene
            MK_scene_renderer
    )
endif()
"@
}

function New-DesktopRuntime2DRegistration {
    param(
        [string]$GameName,
        [string]$TargetName
    )

    return @"

if(MK_DESKTOP_RUNTIME_ENABLED)
    MK_add_desktop_runtime_game($TargetName
        SOURCES
            $GameName/main.cpp
        GAME_MANIFEST
            games/$GameName/game.agent.json
        SMOKE_ARGS
            --smoke
            --require-config
            runtime/$GameName.config
            --require-scene-package
            runtime/$GameName.geindex
            --require-sprite-animation
            --require-tilemap-runtime-ux
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/$GameName.config
            --require-scene-package
            runtime/$GameName.geindex
            --require-d3d12-shaders
            --video-driver
            windows
            --require-d3d12-renderer
            --require-native-2d-sprites
            --require-sprite-animation
            --require-tilemap-runtime-ux
        REQUIRES_D3D12_SHADERS
        PACKAGE_FILES_FROM_MANIFEST
    )
    target_link_libraries($TargetName
        PRIVATE
            MK_runtime
            MK_runtime_scene
            MK_scene
            MK_scene_renderer
            MK_ui
            MK_ui_renderer
            MK_audio
    )
    MK_configure_desktop_runtime_2d_sprite_shader_artifacts(
        TARGET $TargetName
        SPRITE_SHADER_SOURCE "`$`{CMAKE_CURRENT_SOURCE_DIR`}/$GameName/shaders/runtime_2d_sprite.hlsl"
    )
endif()
"@
}

function New-DesktopRuntime3DRegistration {
    param(
        [string]$GameName,
        [string]$TargetName
    )

    return @"

if(MK_DESKTOP_RUNTIME_ENABLED)
    MK_add_desktop_runtime_game($TargetName
        SOURCES
            $GameName/main.cpp
        GAME_MANIFEST
            games/$GameName/game.agent.json
        SMOKE_ARGS
            --smoke
            --require-config
            runtime/$GameName.config
            --require-scene-package
            runtime/$GameName.geindex
            --require-primary-camera-controller
            --require-transform-animation
            --require-morph-package
            --require-quaternion-animation
            --require-package-streaming-safe-point
            --require-gameplay-systems
            --require-scene-collision-package
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/$GameName.config
            --require-scene-package
            runtime/$GameName.geindex
            --require-primary-camera-controller
            --require-transform-animation
            --require-morph-package
            --require-compute-morph
            --require-compute-morph-normal-tangent
            --require-compute-morph-skin
            --require-compute-morph-async-telemetry
            --require-quaternion-animation
            --require-package-streaming-safe-point
            --require-gameplay-systems
            --require-d3d12-scene-shaders
            --video-driver
            windows
            --require-d3d12-renderer
            --require-scene-gpu-bindings
            --require-postprocess
            --require-postprocess-depth-input
            --require-renderer-quality-gates
            --require-playable-3d-slice
            --require-native-ui-overlay
            --require-visible-3d-production-proof
            --require-native-ui-textured-sprite-atlas
            --require-scene-collision-package
        REQUIRES_D3D12_SHADERS
        PACKAGE_FILES_FROM_MANIFEST
    )
    target_link_libraries($TargetName
        PRIVATE
            MK_ai
            MK_animation
            MK_audio
            MK_navigation
            MK_physics
            MK_runtime
            MK_runtime_rhi
            MK_runtime_scene
            MK_scene
            MK_scene_renderer
            MK_ui
            MK_ui_renderer
    )
    MK_configure_desktop_runtime_scene_shader_artifacts(
        TARGET $TargetName
        GAME_NAME $GameName
        SCENE_SHADER_SOURCE "`$`{CMAKE_CURRENT_SOURCE_DIR`}/$GameName/shaders/runtime_scene.hlsl"
        POSTPROCESS_SHADER_SOURCE "`$`{CMAKE_CURRENT_SOURCE_DIR`}/$GameName/shaders/runtime_postprocess.hlsl"
        UI_OVERLAY_SHADER_SOURCE "`$`{CMAKE_CURRENT_SOURCE_DIR`}/$GameName/shaders/runtime_ui_overlay.hlsl"
        SHADOW_SHADER_SOURCE "`$`{CMAKE_CURRENT_SOURCE_DIR`}/$GameName/shaders/runtime_shadow.hlsl"
        SHADOW_RECEIVER_ENTRY ps_shadow_receiver
        SHADOW_VERTEX_ENTRY vs_shadow
        SHADOW_FRAGMENT_ENTRY ps_shadow
        MORPH_VERTEX_ENTRY vs_morph
        COMPUTE_MORPH_VERTEX_ENTRY vs_compute_morph
        COMPUTE_MORPH_ENTRY cs_compute_morph_position
        COMPUTE_MORPH_TANGENT_VERTEX_ENTRY vs_compute_morph_tangent_frame
        COMPUTE_MORPH_TANGENT_ENTRY cs_compute_morph_tangent_frame
        COMPUTE_MORPH_SKINNED_VERTEX_ENTRY vs_compute_morph_skinned
        COMPUTE_MORPH_SKINNED_ENTRY cs_compute_morph_skinned_position
    )
    # Vulkan strict package smoke can use --require-vulkan-scene-shaders with -RequireVulkanShaders.
endif()
"@
}

function New-DesktopRuntimeMaterialShaderRegistration {
    param(
        [string]$GameName,
        [string]$TargetName
    )

    return @"

if(MK_DESKTOP_RUNTIME_ENABLED)
    MK_add_desktop_runtime_game($TargetName
        SOURCES
            $GameName/main.cpp
        GAME_MANIFEST
            games/$GameName/game.agent.json
        SMOKE_ARGS
            --smoke
            --require-config
            runtime/$GameName.config
            --require-scene-package
            runtime/$GameName.geindex
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/$GameName.config
            --require-scene-package
            runtime/$GameName.geindex
            --require-d3d12-scene-shaders
            --video-driver
            windows
            --require-d3d12-renderer
            --require-scene-gpu-bindings
            --require-postprocess
        REQUIRES_D3D12_SHADERS
        PACKAGE_FILES_FROM_MANIFEST
    )
    target_link_libraries($TargetName
        PRIVATE
            MK_runtime
            MK_scene
            MK_scene_renderer
    )
    MK_configure_desktop_runtime_scene_shader_artifacts(
        TARGET $TargetName
        GAME_NAME $GameName
        SCENE_SHADER_SOURCE "`$`{CMAKE_CURRENT_SOURCE_DIR`}/$GameName/shaders/runtime_scene.hlsl"
        POSTPROCESS_SHADER_SOURCE "`$`{CMAKE_CURRENT_SOURCE_DIR`}/$GameName/shaders/runtime_postprocess.hlsl"
    )
    # Vulkan strict package smoke can use --require-vulkan-scene-shaders with -RequireVulkanShaders.
endif()
"@
}

$planned = @(
    $gameDir,
    (Join-Path $gameDir "main.cpp"),
    (Join-Path $gameDir "README.md"),
    (Join-Path $gameDir "game.agent.json")
)

if ($Template -ne "Headless") {
    $planned += @(
        $runtimeDir,
        (Join-Path $runtimeDir "$Name.config")
    )
}
if ($Template -eq "DesktopRuntimeCookedScenePackage" -or $Template -eq "DesktopRuntimeMaterialShaderPackage") {
    $planned += @(
        (Join-Path $runtimeDir ".gitattributes"),
        (Join-Path $runtimeDir "$Name.geindex"),
        (Join-Path $runtimeDir "assets/generated/base_color.texture.geasset"),
        (Join-Path $runtimeDir "assets/generated/triangle.mesh"),
        (Join-Path $runtimeDir "assets/generated/lit.material"),
        (Join-Path $runtimeDir "assets/generated/packaged_scene.scene")
    )
}
if ($Template -eq "DesktopRuntime3DPackage") {
    $planned += @(
        (Join-Path $runtimeDir ".gitattributes"),
        (Join-Path $runtimeDir "$Name.geindex"),
        (Join-Path $runtimeDir "assets/3d/base_color.texture.geasset"),
        (Join-Path $runtimeDir "assets/3d/triangle.mesh"),
        (Join-Path $runtimeDir "assets/3d/skinned_triangle.skinned_mesh"),
        (Join-Path $runtimeDir "assets/3d/lit.material"),
        (Join-Path $runtimeDir "assets/3d/packaged_scene.scene"),
        (Join-Path $gameDir "source/assets/package.geassets"),
        (Join-Path $gameDir "source/scenes/packaged_scene.scene"),
        (Join-Path $gameDir "source/prefabs/static_prop.prefab"),
        (Join-Path $gameDir "source/textures/base_color.texture_source"),
        (Join-Path $gameDir "source/meshes/triangle.mesh_source")
    )
}
if ($Template -eq "DesktopRuntime2DPackage") {
    $planned += @(
        (Join-Path $runtimeDir ".gitattributes"),
        (Join-Path $runtimeDir "$Name.geindex"),
        (Join-Path $runtimeDir "assets/2d/player.texture.geasset"),
        (Join-Path $runtimeDir "assets/2d/player.material"),
        (Join-Path $runtimeDir "assets/2d/jump.audio.geasset"),
        (Join-Path $runtimeDir "assets/2d/level.tilemap"),
        (Join-Path $runtimeDir "assets/2d/player.sprite_animation"),
        (Join-Path $runtimeDir "assets/2d/playable.scene"),
        (Join-Path $gameDir "shaders/runtime_2d_sprite.hlsl")
    )
}
if ($Template -eq "DesktopRuntimeMaterialShaderPackage" -or $Template -eq "DesktopRuntime3DPackage") {
    $planned += @(
        (Join-Path $gameDir "source/materials/lit.material"),
        (Join-Path $gameDir "shaders/runtime_scene.hlsl"),
        (Join-Path $gameDir "shaders/runtime_postprocess.hlsl")
    )
}
if ($Template -eq "DesktopRuntime3DPackage") {
    $planned += @(
        (Join-Path $gameDir "shaders/runtime_shadow.hlsl"),
        (Join-Path $gameDir "shaders/runtime_ui_overlay.hlsl")
    )
}
$planned += $gamesCmake

if ($DryRun) {
    $planned | ForEach-Object { Write-Host "would create/update: $_" }
    exit 0
}

New-Item -ItemType Directory -Path $gameDir | Out-Null

if ($Template -eq "Headless") {
    $mainCpp = New-HeadlessMainCpp -GameName $Name -TargetName $targetName
    $readme = New-HeadlessReadme -Title $DisplayName
    $manifest = New-HeadlessManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-HeadlessRegistration -GameName $Name -TargetName $targetName
} elseif ($Template -eq "DesktopRuntimePackage") {
    New-Item -ItemType Directory -Path $runtimeDir | Out-Null
    $mainCpp = New-DesktopRuntimeMainCpp -GameName $Name -TargetName $targetName -Title $DisplayName
    $readme = New-DesktopRuntimeReadme -Title $DisplayName -TargetName $targetName
    $manifest = New-DesktopRuntimeManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-DesktopRuntimeRegistration -GameName $Name -TargetName $targetName
    $runtimeConfig = @"
format=GameEngine.GeneratedDesktopRuntimePackage.Config.v1
name=$Name
displayName=$DisplayName
renderer=null-fallback
"@
    Set-Content -LiteralPath (Join-Path $runtimeDir "$Name.config") -Value $runtimeConfig -NoNewline
} elseif ($Template -eq "DesktopRuntimeCookedScenePackage") {
    New-Item -ItemType Directory -Path (Join-Path $runtimeDir "assets/generated") -Force | Out-Null
    $cookedScenePackage = New-DesktopRuntimeCookedScenePackageFiles -GameName $Name -DisplayTitle $DisplayName
    $mainCpp = New-DesktopRuntimeCookedSceneMainCpp -GameName $Name -TargetName $targetName -Title $DisplayName -SceneAssetName $cookedScenePackage.SceneAssetName
    $readme = New-DesktopRuntimeCookedSceneReadme -Title $DisplayName -TargetName $targetName -GameName $Name
    $manifest = New-DesktopRuntimeCookedSceneManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-DesktopRuntimeCookedSceneRegistration -GameName $Name -TargetName $targetName
    $runtimeConfig = @"
format=GameEngine.GeneratedDesktopRuntimeCookedScenePackage.Config.v1
name=$Name
displayName=$DisplayName
renderer=null-fallback
scenePackage=runtime/$Name.geindex
"@
    Set-Content -LiteralPath (Join-Path $runtimeDir "$Name.config") -Value $runtimeConfig -NoNewline
    foreach ($entry in $cookedScenePackage.Files.GetEnumerator()) {
        $path = Join-Path $gameDir $entry.Key
        $directory = Split-Path -Parent $path
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        Set-Content -LiteralPath $path -Value $entry.Value -NoNewline
    }
} elseif ($Template -eq "DesktopRuntime2DPackage") {
    New-Item -ItemType Directory -Path (Join-Path $runtimeDir "assets/2d") -Force | Out-Null
    $desktop2dPackage = New-DesktopRuntime2DPackageFiles -GameName $Name -DisplayTitle $DisplayName
    $mainCpp = New-DesktopRuntime2DMainCpp -GameName $Name -TargetName $targetName -Title $DisplayName
    $readme = New-DesktopRuntime2DReadme -Title $DisplayName -TargetName $targetName -GameName $Name
    $manifest = New-DesktopRuntime2DManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-DesktopRuntime2DRegistration -GameName $Name -TargetName $targetName
    $runtimeConfig = @"
format=GameEngine.GeneratedDesktopRuntime2DPackage.Config.v1
name=$Name
displayName=$DisplayName
renderer=null-fallback
scenePackage=runtime/$Name.geindex
"@
    Set-Content -LiteralPath (Join-Path $runtimeDir "$Name.config") -Value $runtimeConfig -NoNewline
    foreach ($entry in $desktop2dPackage.Files.GetEnumerator()) {
        $path = Join-Path $gameDir $entry.Key
        $directory = Split-Path -Parent $path
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        Set-Content -LiteralPath $path -Value $entry.Value -NoNewline
    }
} elseif ($Template -eq "DesktopRuntime3DPackage") {
    New-Item -ItemType Directory -Path (Join-Path $runtimeDir "assets/3d") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $gameDir "source/materials") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $gameDir "shaders") -Force | Out-Null
    $desktop3dPackage = New-DesktopRuntime3DPackageFiles -GameName $Name -DisplayTitle $DisplayName
    $shaderAuthoringPackage = New-DesktopRuntimeMaterialShaderPackageFiles -GameName $Name -DisplayTitle $DisplayName
    $desktop3dPackage.Files["source/materials/lit.material"] = $desktop3dPackage.Files["runtime/assets/3d/lit.material"]
    $desktop3dPackage.Files["shaders/runtime_scene.hlsl"] = $shaderAuthoringPackage.Files["shaders/runtime_scene.hlsl"]
    $desktop3dPackage.Files["shaders/runtime_postprocess.hlsl"] = $shaderAuthoringPackage.Files["shaders/runtime_postprocess.hlsl"]
    $desktop3dPackage.Files["shaders/runtime_postprocess.hlsl"] = $desktop3dPackage.Files["shaders/runtime_postprocess.hlsl"].Replace(
        "Texture2D scene_color_texture : register(t0);`nSamplerState scene_color_sampler : register(s1);",
        "Texture2D scene_color_texture : register(t0);`nSamplerState scene_color_sampler : register(s1);`nTexture2D<float> scene_depth_texture : register(t2);`nSamplerState scene_depth_sampler : register(s3);")
    $desktop3dPackage.Files["shaders/runtime_postprocess.hlsl"] = $desktop3dPackage.Files["shaders/runtime_postprocess.hlsl"].Replace(
        "    float3 graded = saturate(scene.rgb * float3(1.04, 1.02, 0.98) + float3(0.012, 0.008, 0.0));",
        "    float scene_depth = scene_depth_texture.Sample(scene_depth_sampler, input.uv);`n    float near_depth_grade = saturate(1.0 - scene_depth) * 0.08;`n    float3 graded = saturate(scene.rgb * float3(1.04, 1.02, 0.98) +`n                             float3(0.012 + near_depth_grade, 0.008 + near_depth_grade * 0.55,`n                                    near_depth_grade * 0.25));")
    $shadowShader = @"
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct VsIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct VsOut {
    float4 position : SV_Position;
};

VsOut vs_shadow(VsIn input) {
    VsOut output;
    output.position = float4(input.position.xy, 0.35, 1.0);
    return output;
}

float4 ps_shadow(VsOut input) : SV_Target {
    float depth_tint = saturate(input.position.z);
    return float4(depth_tint * 0.0, 0.0, 0.0, 1.0);
}
"@
    $desktop3dPackage.Files["shaders/runtime_shadow.hlsl"] = ConvertTo-LfText -Text $shadowShader
    $nativeUiOverlayShaderPath = Join-Path $scriptRepositoryRoot "games/sample_generated_desktop_runtime_3d_package/shaders/runtime_ui_overlay.hlsl"
    if (Test-Path -LiteralPath $nativeUiOverlayShaderPath) {
        $desktop3dPackage.Files["shaders/runtime_ui_overlay.hlsl"] =
            ConvertTo-LfText -Text (Get-Content -LiteralPath $nativeUiOverlayShaderPath -Raw)
    } else {
        $nativeUiOverlayShader = @"
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct NativeUiOverlayVertexIn {
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    uint texture_index : TEXCOORD1;
};

struct NativeUiOverlayVertexOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    uint texture_index : TEXCOORD1;
};

NativeUiOverlayVertexOut vs_native_ui_overlay(NativeUiOverlayVertexIn input) {
    NativeUiOverlayVertexOut output;
    output.position = float4(input.position, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    output.texture_index = input.texture_index;
    return output;
}

float4 ps_native_ui_overlay(NativeUiOverlayVertexOut input) : SV_Target {
    return saturate(input.color);
}
"@
        $desktop3dPackage.Files["shaders/runtime_ui_overlay.hlsl"] = ConvertTo-LfText -Text $nativeUiOverlayShader
    }
    $mainCpp = New-DesktopRuntime3DMainCpp -GameName $Name -TargetName $targetName -Title $DisplayName -SceneAssetName $desktop3dPackage.SceneAssetName
    $readme = New-DesktopRuntime3DReadme -Title $DisplayName -TargetName $targetName -GameName $Name
    $manifest = New-DesktopRuntime3DManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-DesktopRuntime3DRegistration -GameName $Name -TargetName $targetName
    $runtimeConfig = @"
format=GameEngine.GeneratedDesktopRuntime3DPackage.Config.v1
name=$Name
displayName=$DisplayName
renderer=host-built-d3d12-vulkan-shader-artifacts-with-null-fallback
scenePackage=runtime/$Name.geindex
"@
    Set-Content -LiteralPath (Join-Path $runtimeDir "$Name.config") -Value $runtimeConfig -NoNewline
    foreach ($entry in $desktop3dPackage.Files.GetEnumerator()) {
        $path = Join-Path $gameDir $entry.Key
        $directory = Split-Path -Parent $path
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        Set-Content -LiteralPath $path -Value $entry.Value -NoNewline
    }
} else {
    New-Item -ItemType Directory -Path (Join-Path $runtimeDir "assets/generated") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $gameDir "source/materials") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $gameDir "shaders") -Force | Out-Null
    $materialShaderPackage = New-DesktopRuntimeMaterialShaderPackageFiles -GameName $Name -DisplayTitle $DisplayName
    $mainCpp = New-DesktopRuntimeCookedSceneMainCpp -GameName $Name -TargetName $targetName -Title $DisplayName -SceneAssetName $materialShaderPackage.SceneAssetName
    $readme = New-DesktopRuntimeMaterialShaderReadme -Title $DisplayName -TargetName $targetName -GameName $Name
    $manifest = New-DesktopRuntimeMaterialShaderManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-DesktopRuntimeMaterialShaderRegistration -GameName $Name -TargetName $targetName
    $runtimeConfig = @"
format=GameEngine.GeneratedDesktopRuntimeCookedScenePackage.Config.v1
name=$Name
displayName=$DisplayName
renderer=host-built-d3d12-vulkan-shader-artifacts-with-null-fallback
scenePackage=runtime/$Name.geindex
"@
    Set-Content -LiteralPath (Join-Path $runtimeDir "$Name.config") -Value $runtimeConfig -NoNewline
    foreach ($entry in $materialShaderPackage.Files.GetEnumerator()) {
        $path = Join-Path $gameDir $entry.Key
        $directory = Split-Path -Parent $path
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        Set-Content -LiteralPath $path -Value $entry.Value -NoNewline
    }
}

$mainCpp = Format-CppSourceText -Text $mainCpp
Set-Content -LiteralPath (Join-Path $gameDir "main.cpp") -Value $mainCpp -NoNewline
Set-Content -LiteralPath (Join-Path $gameDir "README.md") -Value $readme -NoNewline
Set-Content -LiteralPath (Join-Path $gameDir "game.agent.json") -Value ($manifest | ConvertTo-Json -Depth 12) -NoNewline

Add-Content -LiteralPath $gamesCmake -Value $registration

Write-Host "created game: games/$Name"
Write-Host "target: $targetName"
Write-Host "template: $Template"


