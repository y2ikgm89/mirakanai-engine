// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_metadata.hpp"
#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/editor/ai_command_panel.hpp"
#include "mirakana/editor/asset_pipeline.hpp"
#include "mirakana/editor/command.hpp"
#include "mirakana/editor/content_browser.hpp"
#include "mirakana/editor/content_browser_import_panel.hpp"
#include "mirakana/editor/game_module_driver.hpp"
#include "mirakana/editor/gltf_mesh_catalog.hpp"
#include "mirakana/editor/history.hpp"
#include "mirakana/editor/input_rebinding.hpp"
#include "mirakana/editor/io.hpp"
#include "mirakana/editor/material_asset_preview_panel.hpp"
#include "mirakana/editor/palette.hpp"
#include "mirakana/editor/play_in_editor.hpp"
#include "mirakana/editor/prefab_variant_authoring.hpp"
#include "mirakana/editor/profiler.hpp"
#include "mirakana/editor/project.hpp"
#include "mirakana/editor/project_wizard.hpp"
#include "mirakana/editor/render_backend.hpp"
#include "mirakana/editor/resource_panel.hpp"
#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/editor/scene_edit.hpp"
#include "mirakana/editor/shader_compile.hpp"
#include "mirakana/editor/shader_tool_discovery.hpp"
#include "mirakana/editor/source_registry_browser.hpp"
#include "mirakana/editor/ui_model.hpp"
#include "mirakana/editor/viewport.hpp"
#include "mirakana/editor/viewport_shader_artifacts.hpp"
#include "mirakana/editor/workspace.hpp"
#include "mirakana/platform/dynamic_library.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/platform/process.hpp"
#include "mirakana/platform/sdl3/sdl_file_dialog.hpp"
#include "mirakana/platform/sdl3/sdl_window.hpp"
#if defined(_WIN32)
#include "mirakana/platform/win32_process.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include "mirakana/editor/material_preview_gpu_cache.hpp"
#include "mirakana/platform/window.hpp"
#include "mirakana/renderer/rhi_viewport_surface.hpp"
#include "mirakana/rhi/rhi.hpp"
#if defined(MK_EDITOR_ENABLE_D3D12)
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"

#include <d3d12.h>
#include <wrl/client.h>
#endif
#if defined(MK_EDITOR_ENABLE_VULKAN)
#include "mirakana/rhi/vulkan/vulkan_backend.hpp"
#endif
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/tools/asset_import_adapters.hpp"
#include "mirakana/tools/asset_import_tool.hpp"
#include "mirakana/tools/gltf_mesh_inspect.hpp"
#include "mirakana/tools/shader_compile_action.hpp"
#include "mirakana/tools/shader_tool_process.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

constexpr float pi = 3.14159265358979323846F;
constexpr float radians_to_degrees = 180.0F / pi;
constexpr std::string_view default_prefab_path = "assets/prefabs/selected.prefab";
constexpr std::string_view default_prefab_variant_path = "assets/prefabs/selected.prefabvariant";
constexpr std::string_view default_cooked_scene_path = "runtime/scenes/start.scene";
constexpr std::string_view default_package_index_path = "runtime/start.geindex";

[[nodiscard]] SDL_Window* native_sdl_window(mirakana::SdlWindow& window) noexcept {
    return std::bit_cast<SDL_Window*>(window.native_window().value);
}
constexpr float degrees_to_radians = pi / 180.0F;

inline void imgui_text_unformatted(std::string_view text) noexcept {
    if (text.empty()) {
        return;
    }
    const char* const begin = text.data();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char* const end = begin + text.size();
    ImGui::TextUnformatted(begin, end);
}

[[nodiscard]] inline std::string
prefab_refresh_blocked_diagnostic(const mirakana::editor::ScenePrefabInstanceRefreshBatchPlan& plan,
                                  std::string_view fallback) {
    if (!plan.batch_diagnostics.empty()) {
        return plan.batch_diagnostics.front();
    }
    if (!plan.targets.empty() && !plan.targets.front().rows.empty()) {
        return plan.targets.front().rows.front().diagnostic;
    }
    return std::string(fallback);
}

struct RhiResourceKindDescriptor {
    mirakana::rhi::RhiResourceKind kind{mirakana::rhi::RhiResourceKind::unknown};
    std::string_view id;
    std::string_view label;
};

constexpr std::array<RhiResourceKindDescriptor, 10> rhi_resource_kind_descriptors{{
    RhiResourceKindDescriptor{mirakana::rhi::RhiResourceKind::buffer, "buffer", "Buffers"},
    RhiResourceKindDescriptor{mirakana::rhi::RhiResourceKind::texture, "texture", "Textures"},
    RhiResourceKindDescriptor{mirakana::rhi::RhiResourceKind::sampler, "sampler", "Samplers"},
    RhiResourceKindDescriptor{mirakana::rhi::RhiResourceKind::swapchain, "swapchain", "Swapchains"},
    RhiResourceKindDescriptor{mirakana::rhi::RhiResourceKind::shader, "shader", "Shaders"},
    RhiResourceKindDescriptor{mirakana::rhi::RhiResourceKind::descriptor_set_layout, "descriptor_set_layout",
                              "Descriptor set layouts"},
    RhiResourceKindDescriptor{mirakana::rhi::RhiResourceKind::descriptor_set, "descriptor_set", "Descriptor sets"},
    RhiResourceKindDescriptor{mirakana::rhi::RhiResourceKind::pipeline_layout, "pipeline_layout", "Pipeline layouts"},
    RhiResourceKindDescriptor{mirakana::rhi::RhiResourceKind::graphics_pipeline, "graphics_pipeline",
                              "Graphics pipelines"},
    RhiResourceKindDescriptor{mirakana::rhi::RhiResourceKind::compute_pipeline, "compute_pipeline",
                              "Compute pipelines"},
}};

[[nodiscard]] std::string_view rhi_resource_kind_id(mirakana::rhi::RhiResourceKind kind) noexcept {
    const auto it = std::ranges::find_if(rhi_resource_kind_descriptors,
                                         [kind](const RhiResourceKindDescriptor& desc) { return desc.kind == kind; });
    return it == rhi_resource_kind_descriptors.end() ? "unknown" : it->id;
}

[[nodiscard]] std::string_view rhi_resource_kind_label(mirakana::rhi::RhiResourceKind kind) noexcept {
    const auto it = std::ranges::find_if(rhi_resource_kind_descriptors,
                                         [kind](const RhiResourceKindDescriptor& desc) { return desc.kind == kind; });
    return it == rhi_resource_kind_descriptors.end() ? "Unknown" : it->label;
}

/// Returns true when `path` ends with `suffix` using ASCII case folding (for `.gltf` / `.glb` detection).
[[nodiscard]] bool path_ends_with_ascii_case_insensitive(std::string_view path, std::string_view suffix) noexcept {
    if (suffix.size() > path.size()) {
        return false;
    }
    const auto start = path.size() - suffix.size();
    for (std::size_t i = 0; i < suffix.size(); ++i) {
        const auto a = static_cast<unsigned char>(path[start + i]);
        const auto b = static_cast<unsigned char>(suffix[i]);
        if (std::tolower(a) != std::tolower(b)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] const char* scene_reparent_combo_item_callback(void* data, int idx) noexcept {
    const auto* opts = static_cast<const std::vector<mirakana::editor::SceneReparentParentOption>*>(data);
    if (idx < 0 || static_cast<std::size_t>(idx) >= opts->size()) {
        return nullptr;
    }
    return (*opts)[static_cast<std::size_t>(idx)].label.c_str();
}

/// Reads bytes for `inspect_gltf_mesh_primitives`: UTF-8 JSON for `.gltf`, raw bytes for `.glb`, via the project text
/// store root.
[[nodiscard]] std::string read_gltf_document_bytes_for_inspect(mirakana::editor::FileTextStore& project_store,
                                                               const std::string& store_path) {
    if (path_ends_with_ascii_case_insensitive(store_path, ".glb")) {
        const auto full_path = std::filesystem::path(".") / std::filesystem::path(store_path);
        std::ifstream input(full_path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("failed to open .glb for binary read");
        }
        return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }
    return project_store.read_text(store_path);
}

constexpr std::string_view editor_default_shader_source = R"(struct VsOut {
    float4 position : SV_Position;
};

VsOut vs_main(uint vertex_id : SV_VertexID) {
    float2 positions[3] = {
        float2(0.0, -0.5),
        float2(0.5, 0.5),
        float2(-0.5, 0.5),
    };
    VsOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    return output;
}

float4 ps_main(VsOut input) : SV_Target0 {
    return float4(0.20, 0.55, 0.95, 1.0);
}
)";

constexpr std::string_view editor_material_preview_shader_source = R"(cbuffer MaterialFactors : register(b0) {
    float4 base_color;
    float3 emissive;
    float metallic;
    float roughness;
    float shading_lit;
    float3 _pad_material;
};

cbuffer ScenePbrFrame : register(b6) {
    row_major float4x4 clip_from_world;
    row_major float4x4 world_from_object;
    float4 camera_position_aspect;
    float4 light_dir_intensity;
    float4 light_color_pad;
    float4 ambient_pad;
};

Texture2D<float4> base_color_texture : register(t1);
SamplerState base_color_sampler : register(s16);

static const float k_pi = 3.14159265358979323846;

struct VsOut {
    float4 position : SV_Position;
    float3 world_position : TEXCOORD1;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

VsOut vs_main(uint vertex_id : SV_VertexID) {
    float2 positions[3] = {
        float2(-0.75, -0.75),
        float2(0.75, -0.75),
        float2(0.0, 0.75),
    };
    float2 uvs[3] = {
        float2(0.0, 1.0),
        float2(1.0, 1.0),
        float2(0.5, 0.0),
    };
    float3 normals[3] = {
        float3(0.0, 0.0, 1.0),
        float3(0.0, 0.0, 1.0),
        float3(0.0, 0.0, 1.0),
    };
    VsOut output;
    float4 object_pos = float4(positions[vertex_id], 0.0, 1.0);
    float4 world_pos = mul(object_pos, world_from_object);
    output.position = mul(world_pos, clip_from_world);
    output.world_position = world_pos.xyz;
    output.normal = normalize(mul(float4(normals[vertex_id], 0.0), world_from_object).xyz);
    output.uv = uvs[vertex_id];
    return output;
}

float3 fresnel_schlick(float cos_theta, float3 f0) {
    return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);
}

float distribution_ggx(float n_dot_h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = (n_dot_h * n_dot_h) * (a2 - 1.0) + 1.0;
    return a2 / max(k_pi * denom * denom, 1e-6);
}

float geometry_schlick_ggx(float n_dot_x, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return n_dot_x / max(n_dot_x * (1.0 - k) + k, 1e-6);
}

float geometry_smith(float n_dot_v, float n_dot_l, float roughness) {
    return geometry_schlick_ggx(n_dot_v, roughness) * geometry_schlick_ggx(n_dot_l, roughness);
}

float3 evaluate_lit_color(VsOut input, float4 sampled) {
    if (shading_lit < 0.5) {
        return sampled.rgb * base_color.rgb + emissive.rgb;
    }
    float3 albedo = sampled.rgb * base_color.rgb;
    float3 n = normalize(input.normal);
    float3 v = normalize(camera_position_aspect.xyz - input.world_position);
    float3 l = normalize(light_dir_intensity.xyz);
    float3 h = normalize(v + l);
    float n_dot_v = saturate(dot(n, v));
    float n_dot_l = saturate(dot(n, l));
    float n_dot_h = saturate(dot(n, h));
    float h_dot_v = saturate(dot(h, v));
    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 fresnel = fresnel_schlick(h_dot_v, f0);
    float alpha = max(roughness, 0.04);
    float d = distribution_ggx(n_dot_h, alpha);
    float g = geometry_smith(n_dot_v, n_dot_l, alpha);
    float3 specular = (d * g) * fresnel / max(4.0 * n_dot_v * n_dot_l, 1e-4);
    float3 kd = (1.0 - fresnel) * (1.0 - metallic);
    float3 diffuse = kd * albedo / k_pi;
    float3 radiance = light_color_pad.rgb * light_dir_intensity.w;
    float3 direct = (diffuse + specular) * radiance * n_dot_l;
    float3 ambient = ambient_pad.rgb * albedo;
    return direct + ambient + emissive.rgb;
}

float4 ps_main(VsOut input) : SV_Target0 {
    float4 sampled = float4(1.0, 1.0, 1.0, 1.0);
#if defined(MK_MATERIAL_PREVIEW_TEXTURED)
    sampled = base_color_texture.Sample(base_color_sampler, input.uv);
#endif
    float3 color = evaluate_lit_color(input, sampled);
    return float4(saturate(color), sampled.a * base_color.a);
}
)";

[[nodiscard]] mirakana::AssetId material_preview_factor_shader_id() {
    return mirakana::AssetId::from_name("editor.material_preview.factor.shader");
}

[[nodiscard]] mirakana::AssetId material_preview_textured_shader_id() {
    return mirakana::AssetId::from_name("editor.material_preview.textured.shader");
}

[[nodiscard]] bool is_material_preview_shader_request(const mirakana::ShaderCompileRequest& request) {
    return request.source.id == material_preview_factor_shader_id() ||
           request.source.id == material_preview_textured_shader_id();
}

[[nodiscard]] std::string_view editor_shader_source_for_request(const mirakana::ShaderCompileRequest& request) {
    return is_material_preview_shader_request(request) ? editor_material_preview_shader_source
                                                       : editor_default_shader_source;
}

[[nodiscard]] bool file_exists_noexcept(const std::filesystem::path& path) noexcept {
    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

[[nodiscard]] bool directory_exists_noexcept(const std::filesystem::path& path) noexcept {
    std::error_code error;
    return std::filesystem::is_directory(path, error);
}

[[nodiscard]] bool has_shader_tool_path(const std::vector<mirakana::ShaderToolDescriptor>& tools,
                                        std::string_view executable_path) {
    return std::ranges::any_of(tools, [executable_path](const mirakana::ShaderToolDescriptor& tool) {
        return tool.executable_path == executable_path;
    });
}

[[nodiscard]] std::optional<std::string> environment_variable_value(const char* name) {
#if defined(_WIN32)
    const DWORD required_with_null = GetEnvironmentVariableA(name, nullptr, 0);
    if (required_with_null == 0U) {
        return std::nullopt;
    }
    std::string result(static_cast<std::size_t>(required_with_null), '\0');
    const DWORD written = GetEnvironmentVariableA(name, result.data(), static_cast<DWORD>(result.size()));
    if (written == 0U || written >= required_with_null) {
        return std::nullopt;
    }
    result.resize(static_cast<std::size_t>(written));
    return result.empty() ? std::nullopt : std::optional<std::string>{std::move(result)};
#else
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }
    return std::string(value);
#endif
}

void append_known_shader_tool(std::vector<mirakana::ShaderToolDescriptor>& tools, mirakana::ShaderToolKind kind,
                              const std::filesystem::path& executable_path, std::string version) {
    if (!file_exists_noexcept(executable_path)) {
        return;
    }

    const auto portable_path = executable_path.generic_string();
    if (has_shader_tool_path(tools, portable_path)) {
        return;
    }

    tools.push_back(mirakana::ShaderToolDescriptor{
        kind,
        portable_path,
        std::move(version),
    });
}

[[nodiscard]] std::filesystem::path shader_tool_executable_filename(std::string_view name) {
#if defined(_WIN32)
    return std::filesystem::path{std::string{name} + ".exe"};
#else
    return std::filesystem::path{std::string{name}};
#endif
}

void append_shader_tool_root(std::vector<mirakana::ShaderToolDescriptor>& tools, const std::filesystem::path& root,
                             std::string_view version_prefix) {
    if (root.empty()) {
        return;
    }

    const std::string version{version_prefix};
    const auto dxc = shader_tool_executable_filename("dxc");
    const auto spirv_val = shader_tool_executable_filename("spirv-val");
    const auto metal = shader_tool_executable_filename("metal");
    const auto metallib = shader_tool_executable_filename("metallib");

    append_known_shader_tool(tools, mirakana::ShaderToolKind::dxc, root / dxc, version + " root");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::dxc, root / "bin" / dxc, version + " bin");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::dxc, root / "Bin" / dxc, version + " Bin");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::dxc, root / "dxc/bin" / dxc, version + " dxc/bin");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::dxc, root / "directx-dxc" / dxc,
                             version + " directx-dxc");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::dxc, root / "tools/directx-dxc" / dxc,
                             version + " tools/directx-dxc");

    append_known_shader_tool(tools, mirakana::ShaderToolKind::spirv_val, root / spirv_val, version + " root");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::spirv_val, root / "bin" / spirv_val, version + " bin");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::spirv_val, root / "Bin" / spirv_val, version + " Bin");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::spirv_val, root / "vulkan/bin" / spirv_val,
                             version + " vulkan/bin");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::spirv_val, root / "spirv-tools" / spirv_val,
                             version + " spirv-tools");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::spirv_val, root / "tools/spirv-tools" / spirv_val,
                             version + " tools/spirv-tools");

    append_known_shader_tool(tools, mirakana::ShaderToolKind::metal, root / metal, version + " root");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::metal, root / "bin" / metal, version + " bin");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::metal, root / "apple/bin" / metal,
                             version + " apple/bin");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::metallib, root / metallib, version + " root");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::metallib, root / "bin" / metallib, version + " bin");
    append_known_shader_tool(tools, mirakana::ShaderToolKind::metallib, root / "apple/bin" / metallib,
                             version + " apple/bin");
}

void append_env_shader_tool_root(std::vector<mirakana::ShaderToolDescriptor>& tools, const char* environment_variable,
                                 std::string_view version) {
    const auto value = environment_variable_value(environment_variable);
    if (!value.has_value()) {
        return;
    }

    append_shader_tool_root(tools, std::filesystem::path{*value}, version);
}

void append_env_shader_tool(std::vector<mirakana::ShaderToolDescriptor>& tools, const char* environment_variable,
                            std::string_view child_path, mirakana::ShaderToolKind kind, std::string version) {
    const auto value = environment_variable_value(environment_variable);
    if (!value.has_value()) {
        return;
    }

    append_known_shader_tool(tools, kind, std::filesystem::path{*value} / std::filesystem::path{child_path},
                             std::move(version));
}

void append_known_installed_shader_tools(std::vector<mirakana::ShaderToolDescriptor>& tools) {
    append_env_shader_tool_root(tools, "MK_SHADER_TOOLCHAIN_ROOT", "MK_SHADER_TOOLCHAIN_ROOT known location");
    append_env_shader_tool_root(tools, "VULKAN_SDK", "VULKAN_SDK known root");
    append_env_shader_tool_root(tools, "VK_SDK_PATH", "VK_SDK_PATH known root");
    append_env_shader_tool(tools, "VULKAN_SDK", "Bin/spirv-val.exe", mirakana::ShaderToolKind::spirv_val,
                           "VULKAN_SDK known location");
    append_env_shader_tool(tools, "VK_SDK_PATH", "Bin/spirv-val.exe", mirakana::ShaderToolKind::spirv_val,
                           "VK_SDK_PATH known location");

#if defined(_WIN32)
    if (const auto vcpkg_root = environment_variable_value("VCPKG_ROOT"); vcpkg_root.has_value()) {
        append_shader_tool_root(tools, std::filesystem::path{*vcpkg_root} / "installed/x64-windows/tools",
                                "VCPKG_ROOT x64-windows tools");
    }
    if (const auto vcpkg_installation_root = environment_variable_value("VCPKG_INSTALLATION_ROOT");
        vcpkg_installation_root.has_value()) {
        append_shader_tool_root(tools, std::filesystem::path{*vcpkg_installation_root} / "installed/x64-windows/tools",
                                "VCPKG_INSTALLATION_ROOT x64-windows tools");
    }

    const std::filesystem::path windows_sdk_bin_root{"C:/Program Files (x86)/Windows Kits/10/bin"};
    if (!directory_exists_noexcept(windows_sdk_bin_root)) {
        return;
    }

    std::vector<std::filesystem::path> dxc_candidates;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(windows_sdk_bin_root, error)) {
        if (!entry.is_directory(error)) {
            continue;
        }
        const auto candidate = entry.path() / "x64/dxc.exe";
        if (file_exists_noexcept(candidate)) {
            dxc_candidates.push_back(candidate);
        }
    }
    std::ranges::sort(dxc_candidates);
    for (const auto& candidate : dxc_candidates) {
        append_known_shader_tool(tools, mirakana::ShaderToolKind::dxc, candidate, "Windows SDK known location");
    }
#endif
}

[[nodiscard]] mirakana::MaterialDefinition make_editor_default_material_definition() {
    const auto texture_id = mirakana::AssetId::from_name("editor.default.texture");
    const auto material_id = mirakana::AssetId::from_name("editor.default.material");

    mirakana::MaterialDefinition material;
    material.id = material_id;
    material.name = "Editor Default Material";
    material.shading_model = mirakana::MaterialShadingModel::unlit;
    material.surface_mode = mirakana::MaterialSurfaceMode::opaque;
    material.texture_bindings.push_back(
        mirakana::MaterialTextureBinding{mirakana::MaterialTextureSlot::base_color, texture_id});
    return material;
}

[[nodiscard]] const char* material_binding_resource_kind_label(mirakana::MaterialBindingResourceKind kind) noexcept {
    switch (kind) {
    case mirakana::MaterialBindingResourceKind::uniform_buffer:
        return "Uniform Buffer";
    case mirakana::MaterialBindingResourceKind::sampled_texture:
        return "Sampled Texture";
    case mirakana::MaterialBindingResourceKind::sampler:
        return "Sampler";
    case mirakana::MaterialBindingResourceKind::unknown:
        break;
    }
    return "Unknown";
}

struct ScenePrefabRefreshReviewState {
    mirakana::editor::ScenePrefabInstanceRefreshPolicy policy{};
    mirakana::editor::ScenePrefabInstanceRefreshBatchPlan batch_plan{};
};

namespace {

[[nodiscard]] std::optional<std::filesystem::path> find_engine_repository_root_containing_tools_script() noexcept {
    std::error_code error{};
    auto directory = std::filesystem::current_path(error);
    if (error) {
        return std::nullopt;
    }
    for (int depth = 0; depth < 24; ++depth) {
        const auto candidate = directory / "tools" / "launch-pix-host-helper.ps1";
        if (std::filesystem::is_regular_file(candidate, error)) {
            return directory;
        }
        if (!directory.has_parent_path()) {
            break;
        }
        directory = directory.parent_path();
    }
    return std::nullopt;
}

[[nodiscard]] std::string resolve_pwsh_executable_for_reviewed_host_scripts() {
#if defined(_WIN32)
    if (const auto program_files = environment_variable_value("ProgramFiles"); program_files.has_value()) {
        std::error_code error{};
        const auto candidate = std::filesystem::path(*program_files) / "PowerShell" / "7" / "pwsh.exe";
        if (std::filesystem::is_regular_file(candidate, error)) {
            return candidate.generic_string();
        }
    }
#endif
    return "pwsh";
}

[[nodiscard]] std::optional<std::string> parse_pix_host_helper_session_dir(std::string_view stdout_text) {
    constexpr std::string_view needle = "Session scratch (not in repo): ";
    const auto position = stdout_text.find(needle);
    if (position == std::string_view::npos) {
        return std::nullopt;
    }
    auto remainder = stdout_text.substr(position + needle.size());
    const auto line_end = remainder.find_first_of("\r\n");
    if (line_end != std::string_view::npos) {
        remainder = remainder.substr(0, line_end);
    }
    while (!remainder.empty() && (remainder.front() == ' ' || remainder.front() == '\t')) {
        remainder.remove_prefix(1U);
    }
    if (remainder.empty()) {
        return std::nullopt;
    }
    return std::string{remainder};
}

} // namespace

template <std::size_t N>
[[nodiscard]] constexpr std::array<char, N> copy_chars_to_fixed_array(std::string_view sv) noexcept {
    std::array<char, N> out{};
    if constexpr (N == 0) {
        return out;
    }
    const std::size_t limit = N - 1;
    const std::size_t copy_len = sv.size() < limit ? sv.size() : limit;
    auto out_it = out.begin();
    for (std::size_t i = 0; i < copy_len; ++i, ++out_it) {
        *out_it = sv[i];
    }
    *out_it = '\0';
    return out;
}

class EditorState {
  public:
    explicit EditorState(SDL_Renderer* renderer, mirakana::IFileDialogService* file_dialogs)
        : project_(mirakana::editor::ProjectDocument{
              .name = "Untitled Project",
              .root_path = ".",
              .asset_root = "assets",
              .source_registry_path = "source/assets/package.geassets",
              .game_manifest_path = "game.agent.json",
              .startup_scene_path = "scenes/start.scene",
              .shader_tool = mirakana::editor::ProjectShaderToolSettings{},
              .render_backend = mirakana::editor::EditorRenderBackend::automatic,
          }),
          project_settings_draft_(mirakana::editor::ProjectSettingsDraft::from_project(project_)),
          workspace_(mirakana::editor::Workspace::create_default(
              mirakana::editor::ProjectInfo{project_.name, project_.root_path})),
          window_desc_{"GameEngine Editor", mirakana::WindowExtent{1280, 720}},
          scene_document_(make_default_scene_document(project_paths_.scene_path)), sdl_renderer_(renderer),
          file_dialogs_(file_dialogs) {
        set_scene_file_dialog_state(scene_open_dialog_, mirakana::editor::EditorSceneFileDialogMode::open,
                                    "Scene open dialog idle", {}, 0, -1, {});
        set_scene_file_dialog_state(scene_save_dialog_, mirakana::editor::EditorSceneFileDialogMode::save,
                                    "Scene save dialog idle", {}, 0, -1, {});
        set_project_file_dialog_state(project_open_dialog_, mirakana::editor::EditorProjectFileDialogMode::open,
                                      "Project open dialog idle", {}, 0, -1, {});
        set_project_file_dialog_state(project_save_dialog_, mirakana::editor::EditorProjectFileDialogMode::save,
                                      "Project save dialog idle", {}, 0, -1, {});
        set_profiler_trace_open_dialog_state("Trace open dialog idle", {}, 0, -1, {});
        set_profiler_trace_save_dialog_state("Trace save dialog idle", {}, 0, -1, {});
        set_asset_import_open_dialog_state("Asset import open dialog idle", {}, 0, -1, {});
        sync_scene_rename_buffer();
        reset_prefab_variant_document();
        configure_default_material_palette();
        register_commands();
        assets_.add(mirakana::AssetRecord{mirakana::AssetId::from_name("editor.default.mesh"),
                                          mirakana::AssetKind::mesh, "builtin/default.mesh"});
        assets_.add(mirakana::AssetRecord{mirakana::AssetId::from_name("editor.default.material"),
                                          mirakana::AssetKind::material, "builtin/default.material"});
        assets_.add(mirakana::AssetRecord{mirakana::AssetId::from_name("editor.default.texture"),
                                          mirakana::AssetKind::texture, "builtin/default.texture"});
        assets_.add(mirakana::AssetRecord{mirakana::AssetId::from_name("editor.default.sprite"),
                                          mirakana::AssetKind::texture, "builtin/default.sprite"});
        assets_.add(mirakana::AssetRecord{mirakana::AssetId::from_name("editor.default.audio"),
                                          mirakana::AssetKind::audio, "builtin/default.audio"});
        assets_.add(mirakana::AssetRecord{mirakana::AssetId::from_name("editor.default.shader"),
                                          mirakana::AssetKind::shader, "builtin/default.shader"});
        assets_.add(mirakana::AssetRecord{mirakana::AssetId::from_name("scenes/start.scene"),
                                          mirakana::AssetKind::scene, "scenes/start.scene"});
        initialize_asset_pipeline();
        refresh_content_browser_from_project_or_cooked_assets();
        initialize_shader_compile_state();
        initialize_shader_tool_discovery();
        configure_viewport_render_backend();
        configure_default_input_rebinding_profile();
        viewport_.resize(mirakana::editor::ViewportExtent{window_desc_.extent.width, window_desc_.extent.height});
        recreate_viewport_render_resources(mirakana::Extent2D{window_desc_.extent.width, window_desc_.extent.height});
        log_.log(mirakana::LogLevel::info, "editor", "Editor started");
    }

    void draw() {
        [[maybe_unused]] mirakana::ScopedProfileZone profiler_frame_scope(profiler_recorder_, profiler_clock_,
                                                                          "editor.frame", profiler_frame_index_);
        record_editor_profiler_counters();
        poll_scene_file_dialogs();
        poll_project_file_dialogs();
        poll_profiler_trace_open_dialog();
        const auto profiler_dialog_capture = profiler_recorder_.snapshot();
        poll_profiler_trace_save_dialog(profiler_dialog_capture);
        poll_asset_import_open_dialog();
        poll_prefab_variant_file_dialogs();

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        draw_main_menu();
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::scene)) {
            draw_scene_panel();
        }
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::inspector)) {
            draw_inspector_panel();
        }
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::assets)) {
            draw_assets_panel();
        }
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::console)) {
            draw_console_panel();
        }
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::viewport)) {
            draw_viewport_panel();
        }
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::resources)) {
            draw_resources_panel();
        }
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::ai_commands)) {
            draw_ai_commands_panel();
        }
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::input_rebinding)) {
            draw_input_rebinding_panel();
        }
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::profiler)) {
            draw_profiler_panel();
        }
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::project_settings)) {
            draw_project_settings_panel();
        }
        if (workspace_.is_panel_visible(mirakana::editor::PanelId::timeline)) {
            draw_timeline_panel();
        }
        if (show_command_palette_) {
            draw_command_palette();
        }
        if (show_project_wizard_) {
            draw_project_wizard();
        }
        ++profiler_frame_index_;
    }

    void begin_input_frame() noexcept {
        ++editor_input_frame_;
        editor_keyboard_input_.begin_frame();
        editor_pointer_input_.begin_frame();
        editor_gamepad_input_.begin_frame();
    }

    [[nodiscard]] mirakana::VirtualInput& keyboard_input() noexcept {
        return editor_keyboard_input_;
    }

    [[nodiscard]] mirakana::VirtualPointerInput& pointer_input() noexcept {
        return editor_pointer_input_;
    }

    [[nodiscard]] mirakana::VirtualGamepadInput& gamepad_input() noexcept {
        return editor_gamepad_input_;
    }

  private:
    enum class InputRebindingCaptureKind : std::uint8_t { action, axis };
    struct InputRebindingCaptureTarget {
        InputRebindingCaptureKind kind{InputRebindingCaptureKind::action};
        std::string context;
        std::string action;
        std::uint64_t armed_frame{0};
    };

    struct AssetImportExternalSourceCopySelection {
        std::filesystem::path source_path;
        std::string target_project_path;
    };

    [[nodiscard]] static std::string trace_dialog_row_value(std::string_view value) {
        return value.empty() ? "-" : std::string(value);
    }

    [[nodiscard]] static std::string trace_dialog_selected_filter_value(int selected_filter) {
        return selected_filter < 0 ? "-" : std::to_string(selected_filter);
    }

    [[nodiscard]] static std::string editor_dialog_row_value(std::string_view value) {
        return value.empty() ? "-" : std::string(value);
    }

    static void set_scene_file_dialog_state(mirakana::editor::EditorSceneFileDialogModel& model,
                                            mirakana::editor::EditorSceneFileDialogMode mode, std::string status_label,
                                            std::string selected_path, std::size_t selected_count, int selected_filter,
                                            std::vector<std::string> diagnostics) {
        model = mirakana::editor::EditorSceneFileDialogModel{};
        model.mode = mode;
        model.status_label = std::move(status_label);
        model.selected_path = std::move(selected_path);
        model.diagnostics = std::move(diagnostics);
        model.accepted =
            model.status_label == "Scene open dialog accepted" || model.status_label == "Scene save dialog accepted";
        model.rows = {
            mirakana::editor::EditorSceneFileDialogRow{"status", "Status", model.status_label},
            mirakana::editor::EditorSceneFileDialogRow{"selected_path", "Selected path",
                                                       editor_dialog_row_value(model.selected_path)},
            mirakana::editor::EditorSceneFileDialogRow{"selected_count", "Selected count",
                                                       std::to_string(selected_count)},
            mirakana::editor::EditorSceneFileDialogRow{"selected_filter", "Selected filter",
                                                       trace_dialog_selected_filter_value(selected_filter)},
        };
    }

    static void set_project_file_dialog_state(mirakana::editor::EditorProjectFileDialogModel& model,
                                              mirakana::editor::EditorProjectFileDialogMode mode,
                                              std::string status_label, std::string selected_path,
                                              std::size_t selected_count, int selected_filter,
                                              std::vector<std::string> diagnostics) {
        model = mirakana::editor::EditorProjectFileDialogModel{};
        model.mode = mode;
        model.status_label = std::move(status_label);
        model.selected_path = std::move(selected_path);
        model.diagnostics = std::move(diagnostics);
        model.accepted = model.status_label == "Project open dialog accepted" ||
                         model.status_label == "Project save dialog accepted";
        model.rows = {
            mirakana::editor::EditorProjectFileDialogRow{"status", "Status", model.status_label},
            mirakana::editor::EditorProjectFileDialogRow{"selected_path", "Selected path",
                                                         editor_dialog_row_value(model.selected_path)},
            mirakana::editor::EditorProjectFileDialogRow{"selected_count", "Selected count",
                                                         std::to_string(selected_count)},
            mirakana::editor::EditorProjectFileDialogRow{"selected_filter", "Selected filter",
                                                         trace_dialog_selected_filter_value(selected_filter)},
        };
    }

    static void set_prefab_variant_file_dialog_state(mirakana::editor::EditorPrefabVariantFileDialogModel& model,
                                                     mirakana::editor::EditorPrefabVariantFileDialogMode mode,
                                                     std::string status_label, std::string selected_path,
                                                     std::size_t selected_count, int selected_filter,
                                                     std::vector<std::string> diagnostics) {
        model = mirakana::editor::EditorPrefabVariantFileDialogModel{};
        model.mode = mode;
        model.status_label = std::move(status_label);
        model.selected_path = std::move(selected_path);
        model.diagnostics = std::move(diagnostics);
        model.accepted = model.status_label == "Prefab variant open dialog accepted" ||
                         model.status_label == "Prefab variant save dialog accepted";
        model.rows = {
            mirakana::editor::EditorPrefabVariantFileDialogRow{"status", "Status", model.status_label},
            mirakana::editor::EditorPrefabVariantFileDialogRow{"selected_path", "Selected path",
                                                               editor_dialog_row_value(model.selected_path)},
            mirakana::editor::EditorPrefabVariantFileDialogRow{"selected_count", "Selected count",
                                                               std::to_string(selected_count)},
            mirakana::editor::EditorPrefabVariantFileDialogRow{"selected_filter", "Selected filter",
                                                               trace_dialog_selected_filter_value(selected_filter)},
        };
    }

    void set_profiler_trace_open_dialog_state(std::string status_label, std::string selected_path,
                                              std::size_t selected_count, int selected_filter,
                                              std::vector<std::string> diagnostics) {
        profiler_trace_open_dialog_ = mirakana::editor::EditorProfilerTraceOpenDialogModel{};
        profiler_trace_open_dialog_.status_label = std::move(status_label);
        profiler_trace_open_dialog_.selected_path = std::move(selected_path);
        profiler_trace_open_dialog_.diagnostics = std::move(diagnostics);
        profiler_trace_open_dialog_.accepted = profiler_trace_open_dialog_.status_label == "Trace open dialog accepted";
        profiler_trace_open_dialog_.rows = {
            mirakana::editor::EditorProfilerKeyValueRow{"status", "Status", profiler_trace_open_dialog_.status_label},
            mirakana::editor::EditorProfilerKeyValueRow{
                "selected_path", "Selected path", trace_dialog_row_value(profiler_trace_open_dialog_.selected_path)},
            mirakana::editor::EditorProfilerKeyValueRow{"selected_count", "Selected count",
                                                        std::to_string(selected_count)},
            mirakana::editor::EditorProfilerKeyValueRow{"selected_filter", "Selected filter",
                                                        trace_dialog_selected_filter_value(selected_filter)},
        };
    }

    void set_profiler_trace_save_dialog_state(std::string status_label, std::string selected_path,
                                              std::size_t selected_count, int selected_filter,
                                              std::vector<std::string> diagnostics) {
        profiler_trace_save_dialog_ = mirakana::editor::EditorProfilerTraceSaveDialogModel{};
        profiler_trace_save_dialog_.status_label = std::move(status_label);
        profiler_trace_save_dialog_.selected_path = std::move(selected_path);
        profiler_trace_save_dialog_.diagnostics = std::move(diagnostics);
        profiler_trace_save_dialog_.accepted = profiler_trace_save_dialog_.status_label == "Trace save dialog accepted";
        profiler_trace_save_dialog_.rows = {
            mirakana::editor::EditorProfilerKeyValueRow{"status", "Status", profiler_trace_save_dialog_.status_label},
            mirakana::editor::EditorProfilerKeyValueRow{
                "selected_path", "Selected path", trace_dialog_row_value(profiler_trace_save_dialog_.selected_path)},
            mirakana::editor::EditorProfilerKeyValueRow{"selected_count", "Selected count",
                                                        std::to_string(selected_count)},
            mirakana::editor::EditorProfilerKeyValueRow{"selected_filter", "Selected filter",
                                                        trace_dialog_selected_filter_value(selected_filter)},
        };
    }

    void set_asset_import_open_dialog_state(std::string status_label, std::vector<std::string> selected_paths,
                                            std::size_t selected_count, int selected_filter,
                                            std::vector<std::string> diagnostics) {
        asset_import_open_dialog_ = mirakana::editor::EditorContentBrowserImportOpenDialogModel{};
        asset_import_open_dialog_.status_label = std::move(status_label);
        asset_import_open_dialog_.selected_paths = std::move(selected_paths);
        asset_import_open_dialog_.diagnostics = std::move(diagnostics);
        asset_import_open_dialog_.accepted =
            asset_import_open_dialog_.status_label == "Asset import open dialog accepted";
        asset_import_open_dialog_.rows = {
            mirakana::editor::EditorContentBrowserImportOpenDialogRow{"status", "Status",
                                                                      asset_import_open_dialog_.status_label},
            mirakana::editor::EditorContentBrowserImportOpenDialogRow{"selected_count", "Selected count",
                                                                      std::to_string(selected_count)},
            mirakana::editor::EditorContentBrowserImportOpenDialogRow{
                "selected_filter", "Selected filter", trace_dialog_selected_filter_value(selected_filter)},
        };

        std::size_t path_index = 1;
        for (const auto& path : asset_import_open_dialog_.selected_paths) {
            asset_import_open_dialog_.rows.push_back(mirakana::editor::EditorContentBrowserImportOpenDialogRow{
                "path." + std::to_string(path_index),
                "Path " + std::to_string(path_index),
                editor_dialog_row_value(path),
            });
            ++path_index;
        }

        std::size_t diagnostic_index = 1;
        for (const auto& diagnostic : asset_import_open_dialog_.diagnostics) {
            asset_import_open_dialog_.rows.push_back(mirakana::editor::EditorContentBrowserImportOpenDialogRow{
                "diagnostic." + std::to_string(diagnostic_index),
                "Diagnostic " + std::to_string(diagnostic_index),
                editor_dialog_row_value(diagnostic),
            });
            ++diagnostic_index;
        }
    }

    [[nodiscard]] static std::filesystem::path normalized_absolute_path(const std::filesystem::path& path) {
        std::error_code error;
        auto result = std::filesystem::weakly_canonical(path, error);
        if (!error) {
            return result.lexically_normal();
        }

        error.clear();
        result = std::filesystem::absolute(path, error);
        return error ? path.lexically_normal() : result.lexically_normal();
    }

    [[nodiscard]] static std::optional<std::string>
    project_store_relative_trace_json_path(std::string_view selected_path, std::string& diagnostic,
                                           std::string_view subject) {
        if (selected_path.empty()) {
            diagnostic = std::string(subject) + " must not be empty";
            return std::nullopt;
        }

        const auto store_root = normalized_absolute_path(".");
        const std::filesystem::path selected{std::string(selected_path)};
        const auto selected_absolute =
            normalized_absolute_path(selected.is_absolute() ? selected : store_root / selected);
        const auto relative = selected_absolute.lexically_relative(store_root);
        auto generic = relative.generic_string();

        if (generic.empty() || relative.is_absolute() || generic == ".." || generic.starts_with("../")) {
            diagnostic = std::string(subject) + " must be inside the project store";
            return std::nullopt;
        }
        if (!generic.ends_with(".json")) {
            diagnostic = std::string(subject) + " must end with .json";
            return std::nullopt;
        }
        return generic;
    }

    [[nodiscard]] static std::optional<std::string> project_store_relative_project_path(std::string_view selected_path,
                                                                                        std::string& diagnostic,
                                                                                        std::string_view subject) {
        if (selected_path.empty()) {
            diagnostic = std::string(subject) + " must not be empty";
            return std::nullopt;
        }

        const auto store_root = normalized_absolute_path(".");
        const std::filesystem::path selected{std::string(selected_path)};
        const auto selected_absolute =
            normalized_absolute_path(selected.is_absolute() ? selected : store_root / selected);
        const auto relative = selected_absolute.lexically_relative(store_root);
        auto generic = relative.generic_string();

        if (generic.empty() || relative.is_absolute() || generic == ".." || generic.starts_with("../")) {
            diagnostic = std::string(subject) + " must be inside the project store";
            return std::nullopt;
        }
        if (!generic.ends_with(".geproject")) {
            diagnostic = std::string(subject) + " must end with .geproject";
            return std::nullopt;
        }
        return generic;
    }

    [[nodiscard]] static std::optional<std::string> project_store_relative_scene_path(std::string_view selected_path,
                                                                                      std::string& diagnostic,
                                                                                      std::string_view subject) {
        if (selected_path.empty()) {
            diagnostic = std::string(subject) + " must not be empty";
            return std::nullopt;
        }

        const auto store_root = normalized_absolute_path(".");
        const std::filesystem::path selected{std::string(selected_path)};
        const auto selected_absolute =
            normalized_absolute_path(selected.is_absolute() ? selected : store_root / selected);
        const auto relative = selected_absolute.lexically_relative(store_root);
        auto generic = relative.generic_string();

        if (generic.empty() || relative.is_absolute() || generic == ".." || generic.starts_with("../")) {
            diagnostic = std::string(subject) + " must be inside the project store";
            return std::nullopt;
        }
        if (!generic.ends_with(".scene")) {
            diagnostic = std::string(subject) + " must end with .scene";
            return std::nullopt;
        }
        return generic;
    }

    [[nodiscard]] static std::optional<std::string>
    project_store_relative_prefab_variant_path(std::string_view selected_path, std::string& diagnostic,
                                               std::string_view subject) {
        if (selected_path.empty()) {
            diagnostic = std::string(subject) + " must not be empty";
            return std::nullopt;
        }

        const auto store_root = normalized_absolute_path(".");
        const std::filesystem::path selected{std::string(selected_path)};
        const auto selected_absolute =
            normalized_absolute_path(selected.is_absolute() ? selected : store_root / selected);
        const auto relative = selected_absolute.lexically_relative(store_root);
        auto generic = relative.generic_string();

        if (generic.empty() || relative.is_absolute() || generic == ".." || generic.starts_with("../")) {
            diagnostic = std::string(subject) + " must be inside the project store";
            return std::nullopt;
        }
        if (!generic.ends_with(".prefabvariant")) {
            diagnostic = std::string(subject) + " must end with .prefabvariant";
            return std::nullopt;
        }
        return generic;
    }

    [[nodiscard]] static bool is_supported_asset_import_source_path(std::string_view path) noexcept {
        return path_ends_with_ascii_case_insensitive(path, ".texture") ||
               path_ends_with_ascii_case_insensitive(path, ".mesh") ||
               path_ends_with_ascii_case_insensitive(path, ".material") ||
               path_ends_with_ascii_case_insensitive(path, ".scene") ||
               path_ends_with_ascii_case_insensitive(path, ".audio_source") ||
               path_ends_with_ascii_case_insensitive(path, ".png") ||
               path_ends_with_ascii_case_insensitive(path, ".gltf") ||
               path_ends_with_ascii_case_insensitive(path, ".glb") ||
               path_ends_with_ascii_case_insensitive(path, ".wav") ||
               path_ends_with_ascii_case_insensitive(path, ".mp3") ||
               path_ends_with_ascii_case_insensitive(path, ".flac");
    }

    [[nodiscard]] static std::optional<std::string>
    project_store_relative_asset_import_source_path(std::string_view selected_path, std::string& diagnostic,
                                                    std::string_view subject) {
        if (selected_path.empty()) {
            diagnostic = std::string(subject) + " must not be empty";
            return std::nullopt;
        }

        const auto store_root = normalized_absolute_path(".");
        const std::filesystem::path selected{std::string(selected_path)};
        const auto selected_absolute =
            normalized_absolute_path(selected.is_absolute() ? selected : store_root / selected);
        const auto relative = selected_absolute.lexically_relative(store_root);
        auto generic = relative.generic_string();

        if (generic.empty() || relative.is_absolute() || generic == ".." || generic.starts_with("../")) {
            diagnostic = std::string(subject) + " must be inside the project store";
            return std::nullopt;
        }
        if (!is_supported_asset_import_source_path(generic)) {
            diagnostic = std::string(subject) + " must be a first-party source document or supported codec source";
            return std::nullopt;
        }
        return generic;
    }

    [[nodiscard]] static std::string asset_import_external_copy_target_path(std::string_view asset_root,
                                                                            std::string_view selected_path) {
        const std::filesystem::path source{std::string(selected_path)};
        const auto filename = source.filename().generic_string();
        if (filename.empty()) {
            throw std::invalid_argument("asset import external source copy filename is empty");
        }

        std::filesystem::path target{std::string(asset_root)};
        target /= "imported_sources";
        target /= filename;
        return target.generic_string();
    }

    [[nodiscard]] static std::filesystem::path project_store_absolute_path(std::string_view project_path) {
        return (normalized_absolute_path(".") / std::filesystem::path{std::string(project_path)}).lexically_normal();
    }

    [[nodiscard]] mirakana::editor::EditorContentBrowserImportExternalSourceCopyInput
    make_asset_import_external_copy_input(const AssetImportExternalSourceCopySelection& selection, bool copied = false,
                                          bool copy_failed = false, std::string diagnostic = {}) const {
        mirakana::editor::EditorContentBrowserImportExternalSourceCopyInput input;
        input.source_path = selection.source_path.generic_string();
        input.target_project_path = selection.target_project_path;
        input.diagnostic = std::move(diagnostic);
        input.source_exists = std::filesystem::exists(selection.source_path);
        input.target_exists = project_store_.exists(selection.target_project_path);
        input.copied = copied;
        input.copy_failed = copy_failed;
        return input;
    }

    [[nodiscard]] static std::string workspace_path_for_project_path(std::string_view project_path) {
        std::filesystem::path path{std::string(project_path)};
        path.replace_extension(".geworkspace");
        return path.generic_string();
    }

    [[nodiscard]] static std::string scene_path_for_project_document(const mirakana::editor::ProjectDocument& project) {
        return join_project_path(project.root_path, project.startup_scene_path);
    }

    [[nodiscard]] static std::string project_relative_path_from_store_path(std::string_view project_root,
                                                                           std::string_view store_path) {
        if (project_root.empty() || project_root == ".") {
            return std::string(store_path);
        }

        const std::filesystem::path root{std::string(project_root)};
        const std::filesystem::path path{std::string(store_path)};
        const auto relative = path.lexically_relative(root);
        auto generic = relative.generic_string();
        if (generic.empty() || relative.is_absolute() || generic == ".." || generic.starts_with("../")) {
            return std::string(store_path);
        }
        return generic;
    }

    void import_profiler_trace_json_from_path(std::string_view import_path) {
        mirakana::editor::EditorProfilerTraceFileImportRequest request;
        request.input_path = std::string(import_path);
        profiler_trace_file_import_ = mirakana::editor::import_editor_profiler_trace_json(project_store_, request);
        profiler_trace_import_review_ = profiler_trace_file_import_.review;
        if (profiler_trace_file_import_.imported) {
            profiler_trace_import_status_ = "Trace JSON imported from " + profiler_trace_file_import_.input_path +
                                            " (" + std::to_string(profiler_trace_file_import_.payload_bytes) +
                                            " bytes)";
        } else if (!profiler_trace_file_import_.diagnostics.empty()) {
            profiler_trace_import_status_ = profiler_trace_file_import_.diagnostics.front();
        } else {
            profiler_trace_import_status_ = profiler_trace_file_import_.status_label;
        }
    }

    void save_profiler_trace_json_to_path(const mirakana::DiagnosticCapture& capture, std::string_view output_path) {
        mirakana::editor::EditorProfilerTraceFileSaveRequest request;
        request.output_path = std::string(output_path);
        const auto save_result = mirakana::editor::save_editor_profiler_trace_json(project_store_, capture, request);
        if (save_result.saved) {
            profiler_trace_export_status_ = "Trace JSON saved to " + save_result.output_path + " (" +
                                            std::to_string(save_result.payload_bytes) + " bytes)";
        } else if (!save_result.diagnostics.empty()) {
            profiler_trace_export_status_ = save_result.diagnostics.front();
        } else {
            profiler_trace_export_status_ = save_result.status_label;
        }
    }

    [[nodiscard]] static mirakana::AssetImportActionKind asset_import_kind_for_source_path(std::string_view path) {
        if (path_ends_with_ascii_case_insensitive(path, ".texture") ||
            path_ends_with_ascii_case_insensitive(path, ".png")) {
            return mirakana::AssetImportActionKind::texture;
        }
        if (path_ends_with_ascii_case_insensitive(path, ".mesh") ||
            path_ends_with_ascii_case_insensitive(path, ".gltf") ||
            path_ends_with_ascii_case_insensitive(path, ".glb")) {
            return mirakana::AssetImportActionKind::mesh;
        }
        if (path_ends_with_ascii_case_insensitive(path, ".material")) {
            return mirakana::AssetImportActionKind::material;
        }
        if (path_ends_with_ascii_case_insensitive(path, ".scene")) {
            return mirakana::AssetImportActionKind::scene;
        }
        if (path_ends_with_ascii_case_insensitive(path, ".audio_source") ||
            path_ends_with_ascii_case_insensitive(path, ".wav") ||
            path_ends_with_ascii_case_insensitive(path, ".mp3") ||
            path_ends_with_ascii_case_insensitive(path, ".flac")) {
            return mirakana::AssetImportActionKind::audio;
        }
        return mirakana::AssetImportActionKind::unknown;
    }

    [[nodiscard]] static std::string asset_import_output_path_for_source_path(std::string_view path,
                                                                              mirakana::AssetImportActionKind kind) {
        std::filesystem::path source{std::string(path)};
        std::filesystem::path output{"imported"};
        output /= source.stem();
        switch (kind) {
        case mirakana::AssetImportActionKind::texture:
            output.replace_extension(".texture");
            break;
        case mirakana::AssetImportActionKind::mesh:
            output.replace_extension(".mesh");
            break;
        case mirakana::AssetImportActionKind::material:
            output.replace_extension(".material");
            break;
        case mirakana::AssetImportActionKind::scene:
            output.replace_extension(".scene");
            break;
        case mirakana::AssetImportActionKind::audio:
            output.replace_extension(".audio");
            break;
        default:
            output.replace_extension(".asset");
            break;
        }
        return output.generic_string();
    }

    void rebuild_asset_import_plan_from_sources(std::span<const std::string> source_paths) {
        if (source_paths.empty()) {
            throw std::invalid_argument("asset import dialog selection is empty");
        }

        mirakana::AssetImportMetadataRegistry imports;
        for (const auto& source_path : source_paths) {
            const auto kind = asset_import_kind_for_source_path(source_path);
            if (kind == mirakana::AssetImportActionKind::unknown) {
                throw std::invalid_argument(
                    "asset import dialog selection has an unsupported first-party or codec source type");
            }

            const auto asset = mirakana::AssetId::from_name(source_path);
            const auto output_path = asset_import_output_path_for_source_path(source_path, kind);
            switch (kind) {
            case mirakana::AssetImportActionKind::texture:
                imports.add_texture(mirakana::TextureImportMetadata{
                    asset,
                    source_path,
                    output_path,
                    mirakana::TextureColorSpace::srgb,
                    true,
                    mirakana::TextureCompression::none,
                });
                break;
            case mirakana::AssetImportActionKind::mesh:
                imports.add_mesh(mirakana::MeshImportMetadata{
                    asset,
                    source_path,
                    output_path,
                    1.0F,
                    false,
                    true,
                });
                break;
            case mirakana::AssetImportActionKind::material:
                imports.add_material(mirakana::MaterialImportMetadata{
                    asset,
                    source_path,
                    output_path,
                    {},
                });
                break;
            case mirakana::AssetImportActionKind::scene:
                imports.add_scene(mirakana::SceneImportMetadata{
                    asset,
                    source_path,
                    output_path,
                    {},
                    {},
                    {},
                });
                break;
            case mirakana::AssetImportActionKind::audio:
                imports.add_audio(mirakana::AudioImportMetadata{
                    asset,
                    source_path,
                    output_path,
                    false,
                });
                break;
            default:
                break;
            }
        }

        asset_import_plan_ = mirakana::build_asset_import_plan(imports);
        asset_pipeline_.set_import_plan(asset_import_plan_);
        asset_import_open_dialog_status_ =
            "Import plan updated from " + std::to_string(source_paths.size()) + " reviewed source selection(s)";
    }

    void reset_asset_import_external_copy_state() {
        asset_import_external_copy_ = mirakana::editor::EditorContentBrowserImportExternalSourceCopyModel{};
        asset_import_pending_project_source_paths_.clear();
        asset_import_pending_external_copies_.clear();
    }

    void refresh_asset_import_external_copy_model() {
        std::vector<mirakana::editor::EditorContentBrowserImportExternalSourceCopyInput> inputs;
        inputs.reserve(asset_import_pending_external_copies_.size());
        for (const auto& selection : asset_import_pending_external_copies_) {
            inputs.push_back(make_asset_import_external_copy_input(selection));
        }
        asset_import_external_copy_ = mirakana::editor::make_content_browser_import_external_source_copy_model(inputs);
    }

    [[nodiscard]] std::optional<AssetImportExternalSourceCopySelection>
    make_asset_import_external_copy_selection(std::string_view selected_path, std::string& diagnostic) const {
        if (selected_path.empty()) {
            diagnostic = "asset import external source copy selection must not be empty";
            return std::nullopt;
        }
        if (!is_supported_asset_import_source_path(selected_path)) {
            diagnostic = "asset import external source copy selection must be a first-party source document or "
                         "supported codec source";
            return std::nullopt;
        }

        const auto store_root = normalized_absolute_path(".");
        const std::filesystem::path selected{std::string(selected_path)};
        const auto selected_absolute =
            normalized_absolute_path(selected.is_absolute() ? selected : store_root / selected);
        const auto relative = selected_absolute.lexically_relative(store_root);
        auto generic = relative.generic_string();
        if (!generic.empty() && !relative.is_absolute() && generic != ".." && !generic.starts_with("../")) {
            diagnostic = "asset import external source copy selection is already inside the project store";
            return std::nullopt;
        }

        AssetImportExternalSourceCopySelection selection;
        selection.source_path = selected_absolute;
        selection.target_project_path = asset_import_external_copy_target_path(project_.asset_root, selected_path);
        return selection;
    }

    void copy_asset_import_external_sources() {
        if (!asset_import_external_copy_.can_copy || asset_import_pending_external_copies_.empty()) {
            asset_import_open_dialog_status_ = "External import source copy is not ready";
            return;
        }

        std::vector<std::string> import_sources = asset_import_pending_project_source_paths_;
        std::vector<mirakana::editor::EditorContentBrowserImportExternalSourceCopyInput> copy_inputs;
        copy_inputs.reserve(asset_import_pending_external_copies_.size());
        for (const auto& selection : asset_import_pending_external_copies_) {
            copy_inputs.push_back(make_asset_import_external_copy_input(selection));
        }

        for (std::size_t index = 0; index < asset_import_pending_external_copies_.size(); ++index) {
            const auto& selection = asset_import_pending_external_copies_[index];
            try {
                const auto target_absolute = project_store_absolute_path(selection.target_project_path);
                if (const auto parent = target_absolute.parent_path(); !parent.empty()) {
                    std::filesystem::create_directories(parent);
                }
                std::filesystem::copy_file(selection.source_path, target_absolute, std::filesystem::copy_options::none);
                copy_inputs[index] = make_asset_import_external_copy_input(selection, true);
                import_sources.push_back(selection.target_project_path);
            } catch (const std::exception& error) {
                copy_inputs[index] = make_asset_import_external_copy_input(selection, false, true, error.what());
                asset_import_external_copy_ =
                    mirakana::editor::make_content_browser_import_external_source_copy_model(copy_inputs);
                asset_import_open_dialog_status_ = error.what();
                log_.log(mirakana::LogLevel::warn, "editor", error.what());
                return;
            }
        }

        asset_import_external_copy_ =
            mirakana::editor::make_content_browser_import_external_source_copy_model(copy_inputs);

        try {
            rebuild_asset_import_plan_from_sources(import_sources);
            asset_import_open_dialog_status_ = "External import source copy completed for " +
                                               std::to_string(asset_import_pending_external_copies_.size()) +
                                               " source selection(s)";
            reset_material_preview_gpu_cache();
        } catch (const std::exception& error) {
            asset_import_open_dialog_status_ = error.what();
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void handle_asset_import_open_dialog_result(const mirakana::FileDialogResult& result) {
        asset_import_open_dialog_ = mirakana::editor::make_content_browser_import_open_dialog_model(result);
        if (!asset_import_open_dialog_.accepted) {
            reset_asset_import_external_copy_state();
            asset_import_open_dialog_status_ = asset_import_open_dialog_.diagnostics.empty()
                                                   ? asset_import_open_dialog_.status_label
                                                   : asset_import_open_dialog_.diagnostics.front();
            log_.log(mirakana::LogLevel::warn, "editor", asset_import_open_dialog_status_);
            return;
        }

        std::vector<std::string> relative_paths;
        relative_paths.reserve(asset_import_open_dialog_.selected_paths.size());
        reset_asset_import_external_copy_state();
        for (const auto& selected_path : asset_import_open_dialog_.selected_paths) {
            std::string diagnostic;
            const auto relative_path = project_store_relative_asset_import_source_path(
                selected_path, diagnostic, "asset import open dialog selection");
            if (!relative_path.has_value()) {
                const auto copy_selection = make_asset_import_external_copy_selection(selected_path, diagnostic);
                if (!copy_selection.has_value()) {
                    set_asset_import_open_dialog_state("Asset import open dialog blocked",
                                                       asset_import_open_dialog_.selected_paths, result.paths.size(),
                                                       result.selected_filter, {diagnostic});
                    asset_import_open_dialog_status_ = diagnostic;
                    log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
                    return;
                }
                asset_import_pending_external_copies_.push_back(*copy_selection);
                continue;
            }
            relative_paths.push_back(*relative_path);
        }

        if (!asset_import_pending_external_copies_.empty()) {
            asset_import_pending_project_source_paths_ = relative_paths;
            refresh_asset_import_external_copy_model();
            if (!asset_import_external_copy_.can_copy) {
                asset_import_open_dialog_status_ = asset_import_external_copy_.diagnostics.empty()
                                                       ? asset_import_external_copy_.status_label
                                                       : asset_import_external_copy_.diagnostics.front();
                log_.log(mirakana::LogLevel::warn, "editor", asset_import_open_dialog_status_);
                return;
            }
            asset_import_open_dialog_status_ = "External import source copy review ready for " +
                                               std::to_string(asset_import_pending_external_copies_.size()) +
                                               " source selection(s)";
            return;
        }

        try {
            rebuild_asset_import_plan_from_sources(relative_paths);
            set_asset_import_open_dialog_state("Asset import open dialog accepted", std::move(relative_paths),
                                               result.paths.size(), result.selected_filter, {});
            reset_material_preview_gpu_cache();
        } catch (const std::exception& error) {
            set_asset_import_open_dialog_state("Asset import open dialog blocked",
                                               asset_import_open_dialog_.selected_paths, result.paths.size(),
                                               result.selected_filter, {error.what()});
            asset_import_open_dialog_status_ = error.what();
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void poll_asset_import_open_dialog() {
        if (asset_import_open_dialog_id_.has_value() && file_dialogs_ != nullptr) {
            const auto result = file_dialogs_->poll_result(*asset_import_open_dialog_id_);
            if (result.has_value()) {
                asset_import_open_dialog_id_.reset();
                handle_asset_import_open_dialog_result(*result);
            }
        }
    }

    void show_asset_import_open_dialog() {
        if (file_dialogs_ == nullptr) {
            set_asset_import_open_dialog_state("Asset import open dialog failed", {}, 0, -1,
                                               {"asset import open dialog service is unavailable"});
            asset_import_open_dialog_status_ = "asset import open dialog service is unavailable";
            log_.log(mirakana::LogLevel::warn, "editor", asset_import_open_dialog_status_);
            return;
        }
        if (asset_import_open_dialog_id_.has_value()) {
            log_.log(mirakana::LogLevel::info, "editor", "Asset import open dialog pending");
            return;
        }

        try {
            const auto request = mirakana::editor::make_content_browser_import_open_dialog_request(project_.asset_root);
            asset_import_open_dialog_id_ = file_dialogs_->show(request);
            reset_asset_import_external_copy_state();
            set_asset_import_open_dialog_state("Asset import open dialog pending", {}, 0, -1, {});
            asset_import_open_dialog_status_ = "Asset import open dialog pending";
        } catch (const std::exception& exception) {
            set_asset_import_open_dialog_state("Asset import open dialog failed", {}, 0, -1, {exception.what()});
            asset_import_open_dialog_status_ = exception.what();
            log_.log(mirakana::LogLevel::warn, "editor", exception.what());
        }
    }

    void handle_project_open_dialog_result(const mirakana::FileDialogResult& result) {
        project_open_dialog_ = mirakana::editor::make_project_open_dialog_model(result);
        if (!project_open_dialog_.accepted) {
            log_.log(mirakana::LogLevel::warn, "editor",
                     project_open_dialog_.diagnostics.empty() ? project_open_dialog_.status_label
                                                              : project_open_dialog_.diagnostics.front());
            return;
        }

        std::string diagnostic;
        const auto relative_path = project_store_relative_project_path(project_open_dialog_.selected_path, diagnostic,
                                                                       "project open dialog selection");
        if (!relative_path.has_value()) {
            set_project_file_dialog_state(project_open_dialog_, mirakana::editor::EditorProjectFileDialogMode::open,
                                          "Project open dialog blocked", project_open_dialog_.selected_path,
                                          result.paths.size(), result.selected_filter, {diagnostic});
            log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
            return;
        }

        try {
            const auto project =
                mirakana::editor::deserialize_project_document(project_store_.read_text(*relative_path));
            const mirakana::editor::ProjectBundlePaths paths{
                *relative_path,
                workspace_path_for_project_path(*relative_path),
                scene_path_for_project_document(project),
            };
            if (!open_project_bundle_from_paths(paths, &diagnostic)) {
                set_project_file_dialog_state(project_open_dialog_, mirakana::editor::EditorProjectFileDialogMode::open,
                                              "Project open dialog blocked", project_open_dialog_.selected_path,
                                              result.paths.size(), result.selected_filter, {diagnostic});
            }
        } catch (const std::exception& error) {
            set_project_file_dialog_state(project_open_dialog_, mirakana::editor::EditorProjectFileDialogMode::open,
                                          "Project open dialog blocked", project_open_dialog_.selected_path,
                                          result.paths.size(), result.selected_filter, {error.what()});
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void handle_project_save_dialog_result(const mirakana::FileDialogResult& result) {
        project_save_dialog_ = mirakana::editor::make_project_save_dialog_model(result);
        if (!project_save_dialog_.accepted) {
            log_.log(mirakana::LogLevel::warn, "editor",
                     project_save_dialog_.diagnostics.empty() ? project_save_dialog_.status_label
                                                              : project_save_dialog_.diagnostics.front());
            return;
        }

        std::string diagnostic;
        const auto relative_path = project_store_relative_project_path(project_save_dialog_.selected_path, diagnostic,
                                                                       "project save dialog selection");
        if (!relative_path.has_value()) {
            set_project_file_dialog_state(project_save_dialog_, mirakana::editor::EditorProjectFileDialogMode::save,
                                          "Project save dialog blocked", project_save_dialog_.selected_path,
                                          result.paths.size(), result.selected_filter, {diagnostic});
            log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
            return;
        }

        const mirakana::editor::ProjectBundlePaths paths{
            *relative_path,
            workspace_path_for_project_path(*relative_path),
            scene_path_for_project_document(project_),
        };
        if (save_project_bundle_to_paths(paths, &diagnostic)) {
            project_paths_ = paths;
        } else {
            set_project_file_dialog_state(project_save_dialog_, mirakana::editor::EditorProjectFileDialogMode::save,
                                          "Project save dialog blocked", project_save_dialog_.selected_path,
                                          result.paths.size(), result.selected_filter, {diagnostic});
        }
    }

    void poll_project_file_dialogs() {
        if (project_open_dialog_id_.has_value() && file_dialogs_ != nullptr) {
            const auto result = file_dialogs_->poll_result(*project_open_dialog_id_);
            if (result.has_value()) {
                project_open_dialog_id_.reset();
                handle_project_open_dialog_result(*result);
            }
        }

        if (project_save_dialog_id_.has_value() && file_dialogs_ != nullptr) {
            const auto result = file_dialogs_->poll_result(*project_save_dialog_id_);
            if (result.has_value()) {
                project_save_dialog_id_.reset();
                handle_project_save_dialog_result(*result);
            }
        }
    }

    void show_project_open_dialog() {
        if (file_dialogs_ == nullptr) {
            set_project_file_dialog_state(project_open_dialog_, mirakana::editor::EditorProjectFileDialogMode::open,
                                          "Project open dialog failed", {}, 0, -1,
                                          {"project open dialog service is unavailable"});
            log_.log(mirakana::LogLevel::warn, "editor", "project open dialog service is unavailable");
            return;
        }
        if (project_open_dialog_id_.has_value()) {
            log_.log(mirakana::LogLevel::info, "editor", "Project open dialog pending");
            return;
        }

        try {
            const auto request = mirakana::editor::make_project_open_dialog_request(project_.root_path);
            project_open_dialog_id_ = file_dialogs_->show(request);
            set_project_file_dialog_state(project_open_dialog_, mirakana::editor::EditorProjectFileDialogMode::open,
                                          "Project open dialog pending", {}, 0, -1, {});
        } catch (const std::exception& exception) {
            set_project_file_dialog_state(project_open_dialog_, mirakana::editor::EditorProjectFileDialogMode::open,
                                          "Project open dialog failed", {}, 0, -1, {exception.what()});
            log_.log(mirakana::LogLevel::warn, "editor", exception.what());
        }
    }

    void show_project_save_dialog() {
        if (file_dialogs_ == nullptr) {
            set_project_file_dialog_state(project_save_dialog_, mirakana::editor::EditorProjectFileDialogMode::save,
                                          "Project save dialog failed", {}, 0, -1,
                                          {"project save dialog service is unavailable"});
            log_.log(mirakana::LogLevel::warn, "editor", "project save dialog service is unavailable");
            return;
        }
        if (project_save_dialog_id_.has_value()) {
            log_.log(mirakana::LogLevel::info, "editor", "Project save dialog pending");
            return;
        }

        try {
            const auto request = mirakana::editor::make_project_save_dialog_request(project_paths_.project_path);
            project_save_dialog_id_ = file_dialogs_->show(request);
            set_project_file_dialog_state(project_save_dialog_, mirakana::editor::EditorProjectFileDialogMode::save,
                                          "Project save dialog pending", {}, 0, -1, {});
        } catch (const std::exception& exception) {
            set_project_file_dialog_state(project_save_dialog_, mirakana::editor::EditorProjectFileDialogMode::save,
                                          "Project save dialog failed", {}, 0, -1, {exception.what()});
            log_.log(mirakana::LogLevel::warn, "editor", exception.what());
        }
    }

    void handle_scene_open_dialog_result(const mirakana::FileDialogResult& result) {
        scene_open_dialog_ = mirakana::editor::make_scene_open_dialog_model(result);
        if (!scene_open_dialog_.accepted) {
            log_.log(mirakana::LogLevel::warn, "editor",
                     scene_open_dialog_.diagnostics.empty() ? scene_open_dialog_.status_label
                                                            : scene_open_dialog_.diagnostics.front());
            return;
        }

        std::string diagnostic;
        const auto relative_path = project_store_relative_scene_path(scene_open_dialog_.selected_path, diagnostic,
                                                                     "scene open dialog selection");
        if (!relative_path.has_value()) {
            set_scene_file_dialog_state(scene_open_dialog_, mirakana::editor::EditorSceneFileDialogMode::open,
                                        "Scene open dialog blocked", scene_open_dialog_.selected_path,
                                        result.paths.size(), result.selected_filter, {diagnostic});
            log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
            return;
        }

        try {
            auto document = mirakana::editor::load_scene_authoring_document(project_store_, *relative_path);
            project_paths_.scene_path = *relative_path;
            project_.startup_scene_path = project_relative_path_from_store_path(project_.root_path, *relative_path);
            replace_scene_document(std::move(document), false);
            log_.log(mirakana::LogLevel::info, "editor", "Scene opened");
        } catch (const std::exception& error) {
            set_scene_file_dialog_state(scene_open_dialog_, mirakana::editor::EditorSceneFileDialogMode::open,
                                        "Scene open dialog blocked", scene_open_dialog_.selected_path,
                                        result.paths.size(), result.selected_filter, {error.what()});
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void handle_scene_save_dialog_result(const mirakana::FileDialogResult& result) {
        scene_save_dialog_ = mirakana::editor::make_scene_save_dialog_model(result);
        if (!scene_save_dialog_.accepted) {
            log_.log(mirakana::LogLevel::warn, "editor",
                     scene_save_dialog_.diagnostics.empty() ? scene_save_dialog_.status_label
                                                            : scene_save_dialog_.diagnostics.front());
            return;
        }

        std::string diagnostic;
        const auto relative_path = project_store_relative_scene_path(scene_save_dialog_.selected_path, diagnostic,
                                                                     "scene save dialog selection");
        if (!relative_path.has_value()) {
            set_scene_file_dialog_state(scene_save_dialog_, mirakana::editor::EditorSceneFileDialogMode::save,
                                        "Scene save dialog blocked", scene_save_dialog_.selected_path,
                                        result.paths.size(), result.selected_filter, {diagnostic});
            log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
            return;
        }

        const auto previous_scene_path = project_paths_.scene_path;
        const auto previous_startup_scene_path = project_.startup_scene_path;
        const auto previous_document_path = std::string(scene_document_.scene_path());

        project_paths_.scene_path = *relative_path;
        project_.startup_scene_path = project_relative_path_from_store_path(project_.root_path, *relative_path);
        scene_document_.set_scene_path(*relative_path);

        if (!save_current_scene_to_path(*relative_path)) {
            project_paths_.scene_path = previous_scene_path;
            project_.startup_scene_path = previous_startup_scene_path;
            scene_document_.set_scene_path(previous_document_path);
            set_scene_file_dialog_state(scene_save_dialog_, mirakana::editor::EditorSceneFileDialogMode::save,
                                        "Scene save dialog blocked", scene_save_dialog_.selected_path,
                                        result.paths.size(), result.selected_filter,
                                        {"scene save dialog selection could not be written"});
        }
    }

    void poll_scene_file_dialogs() {
        if (scene_open_dialog_id_.has_value() && file_dialogs_ != nullptr) {
            const auto result = file_dialogs_->poll_result(*scene_open_dialog_id_);
            if (result.has_value()) {
                scene_open_dialog_id_.reset();
                handle_scene_open_dialog_result(*result);
            }
        }

        if (scene_save_dialog_id_.has_value() && file_dialogs_ != nullptr) {
            const auto result = file_dialogs_->poll_result(*scene_save_dialog_id_);
            if (result.has_value()) {
                scene_save_dialog_id_.reset();
                handle_scene_save_dialog_result(*result);
            }
        }
    }

    void show_scene_open_dialog() {
        if (file_dialogs_ == nullptr) {
            set_scene_file_dialog_state(scene_open_dialog_, mirakana::editor::EditorSceneFileDialogMode::open,
                                        "Scene open dialog failed", {}, 0, -1,
                                        {"scene open dialog service is unavailable"});
            log_.log(mirakana::LogLevel::warn, "editor", "scene open dialog service is unavailable");
            return;
        }
        if (scene_open_dialog_id_.has_value()) {
            log_.log(mirakana::LogLevel::info, "editor", "Scene open dialog pending");
            return;
        }

        try {
            const auto request = mirakana::editor::make_scene_open_dialog_request(project_.root_path);
            scene_open_dialog_id_ = file_dialogs_->show(request);
            set_scene_file_dialog_state(scene_open_dialog_, mirakana::editor::EditorSceneFileDialogMode::open,
                                        "Scene open dialog pending", {}, 0, -1, {});
        } catch (const std::exception& exception) {
            set_scene_file_dialog_state(scene_open_dialog_, mirakana::editor::EditorSceneFileDialogMode::open,
                                        "Scene open dialog failed", {}, 0, -1, {exception.what()});
            log_.log(mirakana::LogLevel::warn, "editor", exception.what());
        }
    }

    void show_scene_save_dialog() {
        if (file_dialogs_ == nullptr) {
            set_scene_file_dialog_state(scene_save_dialog_, mirakana::editor::EditorSceneFileDialogMode::save,
                                        "Scene save dialog failed", {}, 0, -1,
                                        {"scene save dialog service is unavailable"});
            log_.log(mirakana::LogLevel::warn, "editor", "scene save dialog service is unavailable");
            return;
        }
        if (scene_save_dialog_id_.has_value()) {
            log_.log(mirakana::LogLevel::info, "editor", "Scene save dialog pending");
            return;
        }

        try {
            const auto request = mirakana::editor::make_scene_save_dialog_request(project_paths_.scene_path);
            scene_save_dialog_id_ = file_dialogs_->show(request);
            set_scene_file_dialog_state(scene_save_dialog_, mirakana::editor::EditorSceneFileDialogMode::save,
                                        "Scene save dialog pending", {}, 0, -1, {});
        } catch (const std::exception& exception) {
            set_scene_file_dialog_state(scene_save_dialog_, mirakana::editor::EditorSceneFileDialogMode::save,
                                        "Scene save dialog failed", {}, 0, -1, {exception.what()});
            log_.log(mirakana::LogLevel::warn, "editor", exception.what());
        }
    }

    void handle_profiler_trace_open_dialog_result(const mirakana::FileDialogResult& result) {
        profiler_trace_open_dialog_ = mirakana::editor::make_editor_profiler_trace_open_dialog_model(result);
        if (!profiler_trace_open_dialog_.accepted) {
            if (!profiler_trace_open_dialog_.diagnostics.empty()) {
                profiler_trace_import_status_ = profiler_trace_open_dialog_.diagnostics.front();
            } else {
                profiler_trace_import_status_ = profiler_trace_open_dialog_.status_label;
            }
            return;
        }

        std::string diagnostic;
        const auto relative_path = project_store_relative_trace_json_path(profiler_trace_open_dialog_.selected_path,
                                                                          diagnostic, "trace open dialog selection");
        if (!relative_path.has_value()) {
            set_profiler_trace_open_dialog_state("Trace open dialog blocked", profiler_trace_open_dialog_.selected_path,
                                                 result.paths.size(), result.selected_filter, {diagnostic});
            profiler_trace_import_status_ = diagnostic;
            return;
        }

        if (relative_path->size() >= profiler_trace_import_path_.size()) {
            diagnostic = "trace open dialog selection path is too long";
            set_profiler_trace_open_dialog_state("Trace open dialog blocked", profiler_trace_open_dialog_.selected_path,
                                                 result.paths.size(), result.selected_filter, {diagnostic});
            profiler_trace_import_status_ = diagnostic;
            return;
        }

        copy_to_buffer(profiler_trace_import_path_, *relative_path);
        import_profiler_trace_json_from_path(*relative_path);
    }

    void handle_profiler_trace_save_dialog_result(const mirakana::FileDialogResult& result,
                                                  const mirakana::DiagnosticCapture& capture) {
        profiler_trace_save_dialog_ = mirakana::editor::make_editor_profiler_trace_save_dialog_model(result);
        if (!profiler_trace_save_dialog_.accepted) {
            if (!profiler_trace_save_dialog_.diagnostics.empty()) {
                profiler_trace_export_status_ = profiler_trace_save_dialog_.diagnostics.front();
            } else {
                profiler_trace_export_status_ = profiler_trace_save_dialog_.status_label;
            }
            return;
        }

        std::string diagnostic;
        const auto relative_path = project_store_relative_trace_json_path(profiler_trace_save_dialog_.selected_path,
                                                                          diagnostic, "trace save dialog selection");
        if (!relative_path.has_value()) {
            set_profiler_trace_save_dialog_state("Trace save dialog blocked", profiler_trace_save_dialog_.selected_path,
                                                 result.paths.size(), result.selected_filter, {diagnostic});
            profiler_trace_export_status_ = diagnostic;
            return;
        }

        if (relative_path->size() >= profiler_trace_export_path_.size()) {
            diagnostic = "trace save dialog selection path is too long";
            set_profiler_trace_save_dialog_state("Trace save dialog blocked", profiler_trace_save_dialog_.selected_path,
                                                 result.paths.size(), result.selected_filter, {diagnostic});
            profiler_trace_export_status_ = diagnostic;
            return;
        }

        copy_to_buffer(profiler_trace_export_path_, *relative_path);
        save_profiler_trace_json_to_path(capture, *relative_path);
    }

    void poll_profiler_trace_open_dialog() {
        if (!profiler_trace_open_dialog_id_.has_value() || file_dialogs_ == nullptr) {
            return;
        }

        const auto result = file_dialogs_->poll_result(*profiler_trace_open_dialog_id_);
        if (!result.has_value()) {
            return;
        }

        profiler_trace_open_dialog_id_.reset();
        handle_profiler_trace_open_dialog_result(*result);
    }

    void poll_profiler_trace_save_dialog(const mirakana::DiagnosticCapture& capture) {
        if (!profiler_trace_save_dialog_id_.has_value() || file_dialogs_ == nullptr) {
            return;
        }

        const auto result = file_dialogs_->poll_result(*profiler_trace_save_dialog_id_);
        if (!result.has_value()) {
            return;
        }

        profiler_trace_save_dialog_id_.reset();
        handle_profiler_trace_save_dialog_result(*result, capture);
    }

    void show_profiler_trace_open_dialog() {
        if (file_dialogs_ == nullptr) {
            set_profiler_trace_open_dialog_state("Trace open dialog failed", {}, 0, -1,
                                                 {"trace open dialog service is unavailable"});
            profiler_trace_import_status_ = "trace open dialog service is unavailable";
            return;
        }
        if (profiler_trace_open_dialog_id_.has_value()) {
            profiler_trace_import_status_ = "Trace open dialog pending";
            return;
        }

        try {
            const auto request = mirakana::editor::make_editor_profiler_trace_open_dialog_request("diagnostics");
            profiler_trace_open_dialog_id_ = file_dialogs_->show(request);
            set_profiler_trace_open_dialog_state("Trace open dialog pending", {}, 0, -1, {});
            profiler_trace_import_status_ = "Trace open dialog pending";
        } catch (const std::exception& exception) {
            set_profiler_trace_open_dialog_state("Trace open dialog failed", {}, 0, -1, {exception.what()});
            profiler_trace_import_status_ = exception.what();
        }
    }

    void show_profiler_trace_save_dialog() {
        if (file_dialogs_ == nullptr) {
            set_profiler_trace_save_dialog_state("Trace save dialog failed", {}, 0, -1,
                                                 {"trace save dialog service is unavailable"});
            profiler_trace_export_status_ = "trace save dialog service is unavailable";
            return;
        }
        if (profiler_trace_save_dialog_id_.has_value()) {
            profiler_trace_export_status_ = "Trace save dialog pending";
            return;
        }

        try {
            const auto request =
                mirakana::editor::make_editor_profiler_trace_save_dialog_request(profiler_trace_export_path_.data());
            profiler_trace_save_dialog_id_ = file_dialogs_->show(request);
            set_profiler_trace_save_dialog_state("Trace save dialog pending", {}, 0, -1, {});
            profiler_trace_export_status_ = "Trace save dialog pending";
        } catch (const std::exception& exception) {
            set_profiler_trace_save_dialog_state("Trace save dialog failed", {}, 0, -1, {exception.what()});
            profiler_trace_export_status_ = exception.what();
        }
    }

    void handle_prefab_variant_open_dialog_result(const mirakana::FileDialogResult& result) {
        prefab_variant_open_dialog_ = mirakana::editor::make_prefab_variant_open_dialog_model(result);
        if (!prefab_variant_open_dialog_.accepted) {
            log_.log(mirakana::LogLevel::warn, "editor",
                     prefab_variant_open_dialog_.diagnostics.empty() ? prefab_variant_open_dialog_.status_label
                                                                     : prefab_variant_open_dialog_.diagnostics.front());
            return;
        }

        std::string diagnostic;
        const auto relative_path = project_store_relative_prefab_variant_path(
            prefab_variant_open_dialog_.selected_path, diagnostic, "prefab variant open dialog selection");
        if (!relative_path.has_value()) {
            set_prefab_variant_file_dialog_state(
                prefab_variant_open_dialog_, mirakana::editor::EditorPrefabVariantFileDialogMode::open,
                "Prefab variant open dialog blocked", prefab_variant_open_dialog_.selected_path, result.paths.size(),
                result.selected_filter, {diagnostic});
            log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
            return;
        }

        if (relative_path->size() >= prefab_variant_path_.size()) {
            diagnostic = "prefab variant open dialog selection path is too long";
            set_prefab_variant_file_dialog_state(
                prefab_variant_open_dialog_, mirakana::editor::EditorPrefabVariantFileDialogMode::open,
                "Prefab variant open dialog blocked", prefab_variant_open_dialog_.selected_path, result.paths.size(),
                result.selected_filter, {diagnostic});
            log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
            return;
        }

        copy_to_buffer(prefab_variant_path_, *relative_path);
        load_prefab_variant_document();
    }

    void handle_prefab_variant_save_dialog_result(const mirakana::FileDialogResult& result) {
        prefab_variant_save_dialog_ = mirakana::editor::make_prefab_variant_save_dialog_model(result);
        if (!prefab_variant_save_dialog_.accepted) {
            log_.log(mirakana::LogLevel::warn, "editor",
                     prefab_variant_save_dialog_.diagnostics.empty() ? prefab_variant_save_dialog_.status_label
                                                                     : prefab_variant_save_dialog_.diagnostics.front());
            return;
        }

        if (!prefab_variant_document_.has_value()) {
            const std::string diagnostic = "prefab variant save dialog requires an active variant document";
            set_prefab_variant_file_dialog_state(
                prefab_variant_save_dialog_, mirakana::editor::EditorPrefabVariantFileDialogMode::save,
                "Prefab variant save dialog blocked", prefab_variant_save_dialog_.selected_path, result.paths.size(),
                result.selected_filter, {diagnostic});
            log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
            return;
        }

        std::string diagnostic;
        const auto relative_path = project_store_relative_prefab_variant_path(
            prefab_variant_save_dialog_.selected_path, diagnostic, "prefab variant save dialog selection");
        if (!relative_path.has_value()) {
            set_prefab_variant_file_dialog_state(
                prefab_variant_save_dialog_, mirakana::editor::EditorPrefabVariantFileDialogMode::save,
                "Prefab variant save dialog blocked", prefab_variant_save_dialog_.selected_path, result.paths.size(),
                result.selected_filter, {diagnostic});
            log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
            return;
        }

        if (relative_path->size() >= prefab_variant_path_.size()) {
            diagnostic = "prefab variant save dialog selection path is too long";
            set_prefab_variant_file_dialog_state(
                prefab_variant_save_dialog_, mirakana::editor::EditorPrefabVariantFileDialogMode::save,
                "Prefab variant save dialog blocked", prefab_variant_save_dialog_.selected_path, result.paths.size(),
                result.selected_filter, {diagnostic});
            log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
            return;
        }

        copy_to_buffer(prefab_variant_path_, *relative_path);
        save_prefab_variant_document();
    }

    void poll_prefab_variant_file_dialogs() {
        if (prefab_variant_open_dialog_id_.has_value() && file_dialogs_ != nullptr) {
            const auto result = file_dialogs_->poll_result(*prefab_variant_open_dialog_id_);
            if (result.has_value()) {
                prefab_variant_open_dialog_id_.reset();
                handle_prefab_variant_open_dialog_result(*result);
            }
        }

        if (prefab_variant_save_dialog_id_.has_value() && file_dialogs_ != nullptr) {
            const auto result = file_dialogs_->poll_result(*prefab_variant_save_dialog_id_);
            if (result.has_value()) {
                prefab_variant_save_dialog_id_.reset();
                handle_prefab_variant_save_dialog_result(*result);
            }
        }
    }

    void show_prefab_variant_open_dialog() {
        if (file_dialogs_ == nullptr) {
            set_prefab_variant_file_dialog_state(
                prefab_variant_open_dialog_, mirakana::editor::EditorPrefabVariantFileDialogMode::open,
                "Prefab variant open dialog failed", {}, 0, -1, {"prefab variant open dialog service is unavailable"});
            log_.log(mirakana::LogLevel::warn, "editor", "prefab variant open dialog service is unavailable");
            return;
        }
        if (prefab_variant_open_dialog_id_.has_value()) {
            log_.log(mirakana::LogLevel::info, "editor", "Prefab variant open dialog pending");
            return;
        }

        try {
            const auto request = mirakana::editor::make_prefab_variant_open_dialog_request("assets/prefabs");
            prefab_variant_open_dialog_id_ = file_dialogs_->show(request);
            set_prefab_variant_file_dialog_state(prefab_variant_open_dialog_,
                                                 mirakana::editor::EditorPrefabVariantFileDialogMode::open,
                                                 "Prefab variant open dialog pending", {}, 0, -1, {});
        } catch (const std::exception& exception) {
            set_prefab_variant_file_dialog_state(prefab_variant_open_dialog_,
                                                 mirakana::editor::EditorPrefabVariantFileDialogMode::open,
                                                 "Prefab variant open dialog failed", {}, 0, -1, {exception.what()});
            log_.log(mirakana::LogLevel::warn, "editor", exception.what());
        }
    }

    void show_prefab_variant_save_dialog() {
        if (!prefab_variant_document_.has_value()) {
            log_.log(mirakana::LogLevel::warn, "editor", "Prefab variant save dialog requires an active document");
            return;
        }
        if (file_dialogs_ == nullptr) {
            set_prefab_variant_file_dialog_state(
                prefab_variant_save_dialog_, mirakana::editor::EditorPrefabVariantFileDialogMode::save,
                "Prefab variant save dialog failed", {}, 0, -1, {"prefab variant save dialog service is unavailable"});
            log_.log(mirakana::LogLevel::warn, "editor", "prefab variant save dialog service is unavailable");
            return;
        }
        if (prefab_variant_save_dialog_id_.has_value()) {
            log_.log(mirakana::LogLevel::info, "editor", "Prefab variant save dialog pending");
            return;
        }

        try {
            const auto request = mirakana::editor::make_prefab_variant_save_dialog_request(prefab_variant_path_.data());
            prefab_variant_save_dialog_id_ = file_dialogs_->show(request);
            set_prefab_variant_file_dialog_state(prefab_variant_save_dialog_,
                                                 mirakana::editor::EditorPrefabVariantFileDialogMode::save,
                                                 "Prefab variant save dialog pending", {}, 0, -1, {});
        } catch (const std::exception& exception) {
            set_prefab_variant_file_dialog_state(prefab_variant_save_dialog_,
                                                 mirakana::editor::EditorPrefabVariantFileDialogMode::save,
                                                 "Prefab variant save dialog failed", {}, 0, -1, {exception.what()});
            log_.log(mirakana::LogLevel::warn, "editor", exception.what());
        }
    }

    [[nodiscard]] static const mirakana::AssetHotReloadRecookRequest*
    find_recook_request(const std::vector<mirakana::AssetHotReloadRecookRequest>& requests,
                        mirakana::AssetId asset) noexcept {
        const auto it = std::ranges::find_if(
            requests, [asset](const mirakana::AssetHotReloadRecookRequest& request) { return request.asset == asset; });
        return it == requests.end() ? nullptr : &*it;
    }

    [[nodiscard]] static mirakana::editor::SceneAuthoringDocument make_default_scene_document(std::string scene_path) {
        mirakana::Scene scene("Untitled Scene");
        const auto camera_id = scene.create_node("Camera");
        const auto player_id = scene.create_node("Player");
        const auto light_id = scene.create_node("Key Light");
        const auto sprite_id = scene.create_node("Nameplate");

        if (auto* camera = scene.find_node(camera_id); camera != nullptr) {
            camera->transform.position = mirakana::Vec3{0.0F, 0.0F, 5.0F};
            mirakana::SceneNodeComponents components;
            components.camera = mirakana::CameraComponent{
                mirakana::CameraProjectionMode::perspective, 1.04719758F, 10.0F, 0.1F, 500.0F, true,
            };
            scene.set_components(camera->id, components);
        }

        if (auto* player = scene.find_node(player_id); player != nullptr) {
            player->transform.scale = mirakana::Vec3{1.25F, 1.25F, 1.25F};
            mirakana::SceneNodeComponents components;
            components.mesh_renderer = mirakana::MeshRendererComponent{
                mirakana::AssetId::from_name("editor.default.mesh"),
                mirakana::AssetId::from_name("editor.default.material"),
                true,
            };
            scene.set_components(player->id, components);
        }

        if (auto* light = scene.find_node(light_id); light != nullptr) {
            light->transform.position = mirakana::Vec3{2.0F, 4.0F, 3.0F};
            mirakana::SceneNodeComponents components;
            components.light = mirakana::LightComponent{
                mirakana::LightType::directional, mirakana::Vec3{1.0F, 0.96F, 0.82F}, 2.0F, 100.0F, 0.0F, 0.0F, false,
            };
            scene.set_components(light->id, components);
        }

        if (auto* sprite = scene.find_node(sprite_id); sprite != nullptr) {
            sprite->transform.position = mirakana::Vec3{0.0F, -1.75F, 0.0F};
            mirakana::SceneNodeComponents components;
            components.sprite_renderer = mirakana::SpriteRendererComponent{
                mirakana::AssetId::from_name("editor.default.sprite"),
                mirakana::AssetId::from_name("editor.default.material"),
                mirakana::Vec2{2.5F, 0.4F},
                {0.95F, 0.75F, 0.25F, 1.0F},
                true,
            };
            scene.set_components(sprite->id, components);
        }

        auto document = mirakana::editor::SceneAuthoringDocument::from_scene(std::move(scene), std::move(scene_path));
        (void)document.select_node(camera_id);
        return document;
    }

    [[nodiscard]] const mirakana::Scene& active_scene() const noexcept {
        return scene_document_.scene();
    }

    [[nodiscard]] const mirakana::Scene& viewport_scene() const noexcept {
        if (const auto* simulation = play_session_.simulation_scene(); simulation != nullptr) {
            return *simulation;
        }
        return active_scene();
    }

    [[nodiscard]] mirakana::SceneNodeId selected_node() const noexcept {
        return scene_document_.selected_node();
    }

    void draw_main_menu() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                execute_menu_item("New Project", "file.new_project");
                execute_menu_item("New Scene", "file.new_scene");
                execute_menu_item("Open Scene...", "file.open_scene_dialog");
                execute_menu_item("Open Project...", "file.open_project");
                execute_menu_item("Save Project", "file.save_project");
                execute_menu_item("Save Project As...", "file.save_project_as");
                execute_menu_item("Save Scene", "file.save_scene");
                execute_menu_item("Save Scene As...", "file.save_scene_dialog");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                execute_menu_item("Command Palette", "palette.open", "Ctrl+P");
                ImGui::Separator();
                const bool source_edits_blocked = play_session_.source_scene_edits_blocked();
                const auto undo_label = std::string("Undo ") + std::string(history_.undo_label());
                if (ImGui::MenuItem(undo_label.c_str(), "Ctrl+Z", false,
                                    history_.can_undo() && !source_edits_blocked)) {
                    if (history_.undo()) {
                        dirty_state_.mark_dirty();
                        sync_scene_rename_buffer();
                        log_.log(mirakana::LogLevel::info, "editor", "Undo");
                    }
                }
                const auto redo_label = std::string("Redo ") + std::string(history_.redo_label());
                if (ImGui::MenuItem(redo_label.c_str(), "Ctrl+Y", false,
                                    history_.can_redo() && !source_edits_blocked)) {
                    if (history_.redo()) {
                        dirty_state_.mark_dirty();
                        sync_scene_rename_buffer();
                        log_.log(mirakana::LogLevel::info, "editor", "Redo");
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Scene")) {
                execute_menu_item("Add Empty Node", "scene.add_empty");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Run")) {
                const auto controls = play_session_controls_model();
                if (ImGui::MenuItem("Play", nullptr, false,
                                    play_session_control_enabled(
                                        controls, mirakana::editor::EditorPlaySessionControlCommand::play))) {
                    execute_command("run.play");
                }
                if (ImGui::MenuItem("Pause", nullptr, false,
                                    play_session_control_enabled(
                                        controls, mirakana::editor::EditorPlaySessionControlCommand::pause))) {
                    execute_command("run.pause");
                }
                if (ImGui::MenuItem("Resume", nullptr, false,
                                    play_session_control_enabled(
                                        controls, mirakana::editor::EditorPlaySessionControlCommand::resume))) {
                    execute_command("run.resume");
                }
                if (ImGui::MenuItem("Stop", nullptr, false,
                                    play_session_control_enabled(
                                        controls, mirakana::editor::EditorPlaySessionControlCommand::stop))) {
                    execute_command("run.stop");
                }
                const auto in_process_host = make_in_process_runtime_host_model();
                if (ImGui::MenuItem("Begin In-Process Runtime Host", nullptr, false, in_process_host.can_begin)) {
                    begin_in_process_runtime_host();
                }
                const auto runtime_host_launch = make_runtime_host_playtest_launch_model();
                if (ImGui::MenuItem("Execute Runtime Host", nullptr, false, runtime_host_launch.can_execute)) {
                    execute_runtime_host_playtest_launch(runtime_host_launch);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                draw_panel_command_toggle("Scene", "view.scene", mirakana::editor::PanelId::scene);
                draw_panel_command_toggle("Inspector", "view.inspector", mirakana::editor::PanelId::inspector);
                draw_panel_command_toggle("Assets", "view.assets", mirakana::editor::PanelId::assets);
                draw_panel_command_toggle("Console", "view.console", mirakana::editor::PanelId::console);
                draw_panel_command_toggle("Viewport", "view.viewport", mirakana::editor::PanelId::viewport);
                draw_panel_command_toggle("Resources", "view.resources", mirakana::editor::PanelId::resources);
                draw_panel_command_toggle("AI Commands", "view.ai_commands", mirakana::editor::PanelId::ai_commands);
                draw_panel_command_toggle("Input Rebinding", "view.input_rebinding",
                                          mirakana::editor::PanelId::input_rebinding);
                draw_panel_command_toggle("Profiler", "view.profiler", mirakana::editor::PanelId::profiler);
                draw_panel_command_toggle("Project Settings", "view.project_settings",
                                          mirakana::editor::PanelId::project_settings);
                draw_panel_command_toggle("Timeline", "view.timeline", mirakana::editor::PanelId::timeline);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void register_commands() {
        register_command("file.new_project", "New Project", [this]() { open_project_wizard(); });
        register_command("file.new_scene", "New Scene", [this]() {
            replace_scene_document(make_default_scene_document(project_paths_.scene_path), true);
        });
        register_command("file.open_project", "Open Project...", [this]() { show_project_open_dialog(); });
        register_command("file.save_project", "Save Project", [this]() { save_project_bundle(); });
        register_command("file.save_project_as", "Save Project As...", [this]() { show_project_save_dialog(); });
        register_command("file.save_scene", "Save Scene", [this]() { save_current_scene(); });
        register_command("file.open_scene_dialog", "Open Scene...", [this]() { show_scene_open_dialog(); });
        register_command("file.save_scene_dialog", "Save Scene As...", [this]() { show_scene_save_dialog(); });
        register_command("scene.add_empty", "Add Empty Node", [this]() { add_empty_node(); });
        register_command("palette.open", "Open Command Palette", [this]() { show_command_palette_ = true; });
        register_command("run.play", "Play", [this]() { play_viewport(); });
        register_command("run.pause", "Pause", [this]() { pause_viewport(); });
        register_command("run.resume", "Resume", [this]() { resume_viewport(); });
        register_command("run.stop", "Stop", [this]() { stop_viewport(); });
        register_command("viewport.tool.select", "Viewport Tool: Select",
                         [this]() { set_viewport_tool(mirakana::editor::ViewportTool::select); });
        register_command("viewport.tool.translate", "Viewport Tool: Move",
                         [this]() { set_viewport_tool(mirakana::editor::ViewportTool::translate); });
        register_command("viewport.tool.rotate", "Viewport Tool: Rotate",
                         [this]() { set_viewport_tool(mirakana::editor::ViewportTool::rotate); });
        register_command("viewport.tool.scale", "Viewport Tool: Scale",
                         [this]() { set_viewport_tool(mirakana::editor::ViewportTool::scale); });
        register_panel_command("view.scene", "Toggle Scene", mirakana::editor::PanelId::scene);
        register_panel_command("view.inspector", "Toggle Inspector", mirakana::editor::PanelId::inspector);
        register_panel_command("view.assets", "Toggle Assets", mirakana::editor::PanelId::assets);
        register_panel_command("view.console", "Toggle Console", mirakana::editor::PanelId::console);
        register_panel_command("view.viewport", "Toggle Viewport", mirakana::editor::PanelId::viewport);
        register_panel_command("view.resources", "Toggle Resources", mirakana::editor::PanelId::resources);
        register_panel_command("view.ai_commands", "Toggle AI Commands", mirakana::editor::PanelId::ai_commands);
        register_panel_command("view.input_rebinding", "Toggle Input Rebinding",
                               mirakana::editor::PanelId::input_rebinding);
        register_panel_command("view.profiler", "Toggle Profiler", mirakana::editor::PanelId::profiler);
        register_panel_command("view.project_settings", "Toggle Project Settings",
                               mirakana::editor::PanelId::project_settings);
        register_panel_command("view.timeline", "Toggle Timeline", mirakana::editor::PanelId::timeline);
    }

    void register_command(std::string id, std::string label, std::function<void()> action) {
        const bool registered = commands_.try_add(mirakana::editor::Command{
            std::move(id),
            std::move(label),
            std::move(action),
        });
        if (!registered) {
            log_.log(mirakana::LogLevel::warn, "editor", "Command registration failed");
        }
    }

    void register_panel_command(std::string id, std::string label, mirakana::editor::PanelId panel) {
        register_command(std::move(id), std::move(label),
                         [this, panel]() { workspace_.set_panel_visible(panel, !workspace_.is_panel_visible(panel)); });
    }

    void execute_menu_item(const char* label, std::string_view command_id, const char* shortcut = nullptr) {
        if (ImGui::MenuItem(label, shortcut)) {
            execute_command(command_id);
        }
    }

    void execute_command(std::string_view command_id) {
        const bool executed = mirakana::editor::execute_palette_command(commands_, command_id);
        if (!executed) {
            log_.log(mirakana::LogLevel::warn, "editor", "Command execution failed");
        }
    }

    void draw_panel_command_toggle(const char* label, std::string_view command_id, mirakana::editor::PanelId panel) {
        const bool visible = workspace_.is_panel_visible(panel);
        if (ImGui::MenuItem(label, nullptr, visible)) {
            execute_command(command_id);
        }
    }

    void draw_scene_panel() {
        ImGui::Begin("Scene");
        imgui_text_unformatted(active_scene().name());
        ImGui::Text("Path: %s", project_paths_.scene_path.c_str());
        if (ImGui::Button("Add Empty")) {
            add_empty_node();
        }
        ImGui::SameLine();
        if (ImGui::Button("Duplicate")) {
            duplicate_selected_node();
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            delete_selected_node();
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse Open Scene")) {
            show_scene_open_dialog();
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse Save Scene As")) {
            show_scene_save_dialog();
        }
        draw_scene_file_dialog_table("Scene Open Dialog", scene_open_dialog_);
        draw_scene_file_dialog_table("Scene Save Dialog", scene_save_dialog_);

        const auto rows = scene_document_.hierarchy_rows();
        if (ImGui::BeginChild("Scene Hierarchy", ImVec2{0.0F, 180.0F}, ImGuiChildFlags_Borders)) {
            for (const auto& row : rows) {
                ImGui::Indent(static_cast<float>(row.depth) * 16.0F);
                const auto label = row.name + "##scene-node-" + std::to_string(row.node.value);
                if (ImGui::Selectable(label.c_str(), row.selected)) {
                    if (scene_document_.select_node(row.node)) {
                        sync_scene_rename_buffer();
                    }
                }
                if (row.has_camera || row.has_light || row.has_mesh_renderer || row.has_sprite_renderer) {
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("camera:%s light:%s mesh:%s sprite:%s", row.has_camera ? "yes" : "no",
                                          row.has_light ? "yes" : "no", row.has_mesh_renderer ? "yes" : "no",
                                          row.has_sprite_renderer ? "yes" : "no");
                    }
                }
                ImGui::Unindent(static_cast<float>(row.depth) * 16.0F);
            }
        }
        ImGui::EndChild();

        if (const auto* node = active_scene().find_node(selected_node())) {
            ImGui::SeparatorText("Selected Node");
            ImGui::InputText("Name", scene_rename_buffer_.data(), scene_rename_buffer_.size());
            if (ImGui::Button("Apply Rename")) {
                execute_scene_authoring_action(mirakana::editor::make_scene_authoring_rename_node_action(
                    scene_document_, node->id, std::string(scene_rename_buffer_.data())));
                sync_scene_rename_buffer();
            }
            ImGui::SameLine();
            if (ImGui::Button("Save Prefab")) {
                save_selected_prefab();
            }
            ImGui::SameLine();
            if (ImGui::Button("Instantiate Prefab")) {
                instantiate_default_prefab();
            }
            ImGui::SeparatorText("Reparent");
            scene_reparent_parent_options_ =
                mirakana::editor::make_scene_authoring_reparent_parent_options(scene_document_, node->id);
            if (scene_reparent_combo_idx_ >= static_cast<int>(scene_reparent_parent_options_.size())) {
                scene_reparent_combo_idx_ = 0;
            }
            if (scene_reparent_parent_options_.empty()) {
                ImGui::TextDisabled("No alternate parent targets.");
            } else {
                ImGui::Combo("New parent", &scene_reparent_combo_idx_, scene_reparent_combo_item_callback,
                             static_cast<void*>(&scene_reparent_parent_options_),
                             static_cast<int>(scene_reparent_parent_options_.size()));
                if (ImGui::Button("Reparent")) {
                    const auto& choice =
                        scene_reparent_parent_options_[static_cast<std::size_t>(scene_reparent_combo_idx_)];
                    (void)execute_scene_authoring_action(mirakana::editor::make_scene_authoring_reparent_node_action(
                        scene_document_, node->id, choice.parent));
                }
            }
            const bool can_refresh_prefab_instance =
                node->prefab_source.has_value() && mirakana::is_valid_scene_prefab_source_link(*node->prefab_source) &&
                node->prefab_source->source_node_index == 1U && !node->prefab_source->prefab_path.empty();
            if (!can_refresh_prefab_instance) {
                ImGui::BeginDisabled();
            }
            ImGui::Checkbox("Keep Local Children", &scene_prefab_refresh_keep_local_children_);
            ImGui::SameLine();
            ImGui::Checkbox("Keep Stale Source Nodes", &scene_prefab_refresh_keep_stale_source_nodes_);
            ImGui::SameLine();
            ImGui::Checkbox("Keep Nested Prefab Instances", &scene_prefab_refresh_keep_nested_prefab_instances_);
            ImGui::SameLine();
            ImGui::Checkbox("Apply Nested Prefab Propagation", &scene_prefab_refresh_apply_nested_prefab_propagation_);
            ImGui::SameLine();
            if (ImGui::Button("Refresh Prefab Instance")) {
                refresh_selected_prefab_instance();
            }
            if (!can_refresh_prefab_instance) {
                ImGui::EndDisabled();
            }
        } else {
            ImGui::TextDisabled("No selected node");
            if (ImGui::Button("Instantiate Prefab")) {
                instantiate_default_prefab();
            }
        }

        const auto source_links = mirakana::editor::make_scene_prefab_instance_source_link_model(scene_document_);
        if (source_links.has_links) {
            ImGui::SeparatorText("Prefab Source Links");
            ImGui::Text("Linked: %zu  Stale: %zu", source_links.linked_count, source_links.stale_count);
            if (ImGui::Button("Review batch prefab refresh")) {
                refresh_batch_prefab_instances_review();
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear batch selection")) {
                scene_prefab_batch_refresh_node_ids_.clear();
            }
            if (ImGui::BeginChild("Prefab Source Link Rows", ImVec2{0.0F, 120.0F}, ImGuiChildFlags_Borders)) {
                for (const auto& row : source_links.rows) {
                    ImGui::PushID(row.id.c_str());
                    const bool refresh_eligible =
                        row.status == mirakana::editor::ScenePrefabInstanceSourceLinkStatus::linked &&
                        row.source_node_index == 1U && !row.prefab_path.empty();
                    if (refresh_eligible) {
                        bool batch_selected = scene_prefab_batch_refresh_node_ids_.contains(row.node.value);
                        if (ImGui::Checkbox("Batch", &batch_selected)) {
                            if (batch_selected) {
                                scene_prefab_batch_refresh_node_ids_.insert(row.node.value);
                            } else {
                                scene_prefab_batch_refresh_node_ids_.erase(row.node.value);
                            }
                        }
                        ImGui::SameLine();
                    }
                    ImGui::Text("%s: %s", row.node_name.c_str(), row.status_label.c_str());
                    ImGui::TextDisabled("Prefab: %s", row.prefab_name.c_str());
                    ImGui::TextDisabled("Path: %s", row.prefab_path.empty() ? "-" : row.prefab_path.c_str());
                    ImGui::TextDisabled("Source: %u:%s", row.source_node_index, row.source_node_name.c_str());
                    if (!row.diagnostic.empty()) {
                        ImGui::TextWrapped("%s", row.diagnostic.c_str());
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
        }
        draw_scene_prefab_refresh_review();
        ImGui::End();
    }

    void draw_scene_prefab_refresh_review() {
        if (!scene_prefab_refresh_review_.has_value()) {
            return;
        }

        auto& review = *scene_prefab_refresh_review_;
        const auto& batch = review.batch_plan;
        ImGui::SeparatorText("Prefab Refresh Review");
        ImGui::Text("Batch status: %s", batch.status_label.c_str());
        ImGui::Text("Targets: %zu  Blocking: %zu  Warnings: %zu  Ready: %zu", batch.target_count,
                    batch.blocking_target_count, batch.warning_target_count, batch.ready_target_count);
        for (const auto& diagnostic : batch.batch_diagnostics) {
            ImGui::TextWrapped("%s", diagnostic.c_str());
        }
        if (!batch.can_apply) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Apply Reviewed Prefab Refresh")) {
            apply_reviewed_prefab_instance_refresh();
        }
        if (!batch.can_apply) {
            ImGui::EndDisabled();
        }

        if (ImGui::BeginChild("Prefab Refresh Batch Targets", ImVec2{0.0F, 160.0F}, ImGuiChildFlags_Borders)) {
            for (const auto& sub_plan : batch.targets) {
                ImGui::PushID(static_cast<int>(sub_plan.instance_root.value));
                ImGui::Text("Root %u  %s  %s", sub_plan.instance_root.value, sub_plan.prefab_name.c_str(),
                            sub_plan.status_label.c_str());
                ImGui::Indent();
                for (const auto& row : sub_plan.rows) {
                    ImGui::PushID(row.id.c_str());
                    ImGui::Text("%s: %s", row.kind_label.c_str(), row.status_label.c_str());
                    ImGui::TextDisabled("Current: %u %s", row.current_node.value, row.current_node_name.c_str());
                    ImGui::TextDisabled("Source: %s", row.source_node_name.c_str());
                    if (!row.diagnostic.empty()) {
                        ImGui::TextWrapped("%s", row.diagnostic.c_str());
                    }
                    ImGui::PopID();
                }
                ImGui::Unindent();
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
    }

    /// Loads `gltf_mesh_inspect_path_` from the project text store, runs `mirakana::inspect_gltf_mesh_primitives`, and
    /// stores the report for the Inspector UI.
    void refresh_gltf_mesh_inspect_report() {
        gltf_mesh_inspect_report_ = {};
        try {
            const std::string store_path = join_project_path(project_.root_path, gltf_mesh_inspect_path_.data());
            if (!project_store_.exists(store_path)) {
                gltf_mesh_inspect_report_.parse_succeeded = false;
                gltf_mesh_inspect_report_.diagnostic = "project file does not exist at resolved path";
            } else {
                const std::string bytes = read_gltf_document_bytes_for_inspect(project_store_, store_path);
                gltf_mesh_inspect_report_ = mirakana::inspect_gltf_mesh_primitives(bytes, store_path);
            }
        } catch (const std::exception& error) {
            gltf_mesh_inspect_report_.parse_succeeded = false;
            gltf_mesh_inspect_report_.diagnostic = error.what();
        }
        gltf_mesh_inspect_ui_snapshot_ = mirakana::editor::serialize_editor_ui_model(
            mirakana::editor::make_gltf_mesh_inspect_ui_model(gltf_mesh_inspect_report_));
    }

    void draw_inspector_panel() {
        ImGui::Begin("Inspector");
        ImGui::Text("Window: %s", window_desc_.title.c_str());
        ImGui::Text("Size: %u x %u", window_desc_.extent.width, window_desc_.extent.height);
        if (const auto* node = active_scene().find_node(selected_node())) {
            ImGui::SeparatorText("Selection");
            ImGui::Text("Node: %s", node->name.c_str());
            draw_selected_node_transform_editor();
            draw_selected_node_component_editor();
        } else {
            ImGui::SeparatorText("Selection");
            ImGui::TextDisabled("No selected node");
        }

        ImGui::SeparatorText("glTF mesh inspect");
        ImGui::Checkbox("Follow Assets selection (.gltf / .glb)", &gltf_mesh_inspect_follow_assets_selection_);
        ImGui::TextWrapped("Path relative to project root (e.g. source/meshes/model.gltf). .glb uses a binary read.");
        ImGui::InputText("glTF path", gltf_mesh_inspect_path_.data(), gltf_mesh_inspect_path_.size());
        if (ImGui::Button("Inspect")) {
            refresh_gltf_mesh_inspect_report();
        }
        ImGui::SameLine();
        if (const auto* asset = content_browser_.selected_asset();
            asset != nullptr && mirakana::editor::editor_asset_path_supports_gltf_mesh_inspect(asset->path)) {
            if (ImGui::Button("Use Assets selection path")) {
                copy_to_buffer(gltf_mesh_inspect_path_, asset->path);
                refresh_gltf_mesh_inspect_report();
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::Button("Use Assets selection path");
            ImGui::EndDisabled();
        }

        const auto inspect_rows =
            mirakana::editor::gltf_mesh_inspect_report_to_inspector_rows(gltf_mesh_inspect_report_);
        if (ImGui::BeginChild("gltf_mesh_inspect_rows", ImVec2{0.0F, 140.0F}, ImGuiChildFlags_Borders)) {
            for (const auto& row : inspect_rows) {
                ImGui::PushID(row.id.c_str());
                ImGui::TextWrapped("%s: %s", row.label.c_str(), row.value.c_str());
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        if (ImGui::CollapsingHeader("glTF mesh inspect (retained MK_ui snapshot)")) {
            ImGui::BeginDisabled(gltf_mesh_inspect_ui_snapshot_.empty());
            if (ImGui::Button("Copy retained snapshot")) {
                ImGui::SetClipboardText(gltf_mesh_inspect_ui_snapshot_.c_str());
            }
            ImGui::EndDisabled();
            if (ImGui::BeginChild("gltf_mesh_inspect_ui_snapshot", ImVec2{0.0F, 160.0F}, ImGuiChildFlags_Borders,
                                  ImGuiWindowFlags_HorizontalScrollbar)) {
                ImGui::PushTextWrapPos(0.0F);
                ImGui::TextUnformatted(gltf_mesh_inspect_ui_snapshot_.empty() ? "(run Inspect to populate snapshot)"
                                                                              : gltf_mesh_inspect_ui_snapshot_.c_str());
                ImGui::PopTextWrapPos();
            }
            ImGui::EndChild();
        }

        ImGui::End();
    }

    void draw_assets_panel() {
        ImGui::Begin("Assets");

        if (ImGui::InputText("Filter", asset_filter_.data(), asset_filter_.size())) {
            content_browser_.set_text_filter(std::string(asset_filter_.data()));
        }

        static constexpr std::array<const char*, 8> kind_labels = {"All",   "Texture", "Mesh",   "Material",
                                                                   "Scene", "Audio",   "Script", "Shader"};
        if (ImGui::Combo("Kind", &asset_kind_filter_index_, kind_labels.data(), static_cast<int>(kind_labels.size()))) {
            content_browser_.set_kind_filter(asset_kind_from_filter_index(asset_kind_filter_index_));
        }

        auto asset_panel_model = mirakana::editor::make_editor_content_browser_import_panel_model(
            content_browser_, asset_pipeline_, asset_import_plan_, {});
        std::optional<mirakana::editor::EditorMaterialAssetPreviewPanelModel> material_preview_panel_model;
        ImGui::Text("Visible: %zu / %zu", asset_panel_model.visible_asset_count, asset_panel_model.total_asset_count);
        ImGui::Text("Source Registry: %s", project_.source_registry_path.c_str());
        const auto source_registry_status = source_registry_browser_refresh_.status_label.empty()
                                                ? std::string{"Source asset registry not loaded"}
                                                : source_registry_browser_refresh_.status_label;
        ImGui::Text("Registry Status: %s", source_registry_status.c_str());
        ImGui::Text("Registry Assets: %zu", source_registry_browser_refresh_.asset_count);
        if (ImGui::Button("Reload Source Registry")) {
            refresh_content_browser_from_project_or_cooked_assets();
            reset_material_preview_gpu_cache();
            asset_panel_model = mirakana::editor::make_editor_content_browser_import_panel_model(
                content_browser_, asset_pipeline_, asset_import_plan_, {});
        }
        if (!source_registry_browser_refresh_.diagnostics.empty()) {
            if (ImGui::BeginChild("Source Registry Diagnostics", ImVec2{0.0F, 58.0F}, ImGuiChildFlags_Borders)) {
                for (const auto& diagnostic : source_registry_browser_refresh_.diagnostics) {
                    ImGui::TextWrapped("%s", diagnostic.c_str());
                }
            }
            ImGui::EndChild();
        }
        ImGui::Separator();
        bool selection_changed = false;
        if (ImGui::BeginChild("Asset List", ImVec2{0.0F, 180.0F}, ImGuiChildFlags_Borders)) {
            for (const auto& asset : asset_panel_model.assets) {
                const auto label = asset.display_name + "##" + asset.path;
                if (ImGui::Selectable(label.c_str(), asset.selected)) {
                    if (!content_browser_.select(asset.asset)) {
                        log_.log(mirakana::LogLevel::warn, "editor", "Asset selection failed");
                    } else {
                        reset_material_preview_gpu_cache();
                        selection_changed = true;
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", asset.path.c_str());
                }
            }
        }
        ImGui::EndChild();

        if (selection_changed) {
            asset_panel_model = mirakana::editor::make_editor_content_browser_import_panel_model(
                content_browser_, asset_pipeline_, asset_import_plan_, {});
        }

        if (selection_changed && gltf_mesh_inspect_follow_assets_selection_) {
            if (const auto* asset = content_browser_.selected_asset();
                asset != nullptr && mirakana::editor::editor_asset_path_supports_gltf_mesh_inspect(asset->path)) {
                if (asset->path != std::string_view(gltf_mesh_inspect_path_.data())) {
                    copy_to_buffer(gltf_mesh_inspect_path_, asset->path);
                    refresh_gltf_mesh_inspect_report();
                }
            }
        }

        if (asset_panel_model.has_selected_asset) {
            const auto& selected = asset_panel_model.selected_asset;
            ImGui::SeparatorText("Selection");
            ImGui::Text("Name: %s", selected.display_name.c_str());
            ImGui::Text("Kind: %s", selected.kind_label.c_str());
            ImGui::Text("Path: %s", selected.path.c_str());
            if (!selected.asset_key_label.empty()) {
                ImGui::Text("Asset Key: %s", selected.asset_key_label.c_str());
            }
            if (!selected.identity_source_path.empty()) {
                ImGui::Text("Source: %s", selected.identity_source_path.c_str());
            }
            if (!selected.directory.empty()) {
                ImGui::Text("Directory: %s", selected.directory.c_str());
            }
            if (selected.kind == mirakana::AssetKind::material) {
                material_preview_panel_model = mirakana::editor::make_editor_material_asset_preview_panel_model(
                    tool_filesystem_, assets_, selected.asset, material_preview_shader_artifacts_);
                asset_panel_model.pipeline.material_previews.push_back(material_preview_panel_model->preview);
            }
        }

        const auto scene_reference_diagnostics =
            mirakana::editor::validate_scene_authoring_references(active_scene(), assets_);
        if (!scene_reference_diagnostics.empty()) {
            ImGui::SeparatorText("Scene Reference Diagnostics");
            if (ImGui::BeginChild("Scene Reference Diagnostics", ImVec2{0.0F, 72.0F}, ImGuiChildFlags_Borders)) {
                for (const auto& diagnostic : scene_reference_diagnostics) {
                    ImGui::TextWrapped(
                        "%s node #%u %s asset #%llu: %s", scene_authoring_diagnostic_kind_label(diagnostic.kind),
                        diagnostic.node.value, diagnostic.field.c_str(),
                        static_cast<unsigned long long>(diagnostic.asset.value), diagnostic.diagnostic.c_str());
                }
            }
            ImGui::EndChild();
        }

        const auto prefab_path = current_prefab_path();
        const auto cooked_scene_path = current_cooked_scene_path();
        const auto package_index_path = current_package_index_path();
        const auto package_candidates = mirakana::editor::make_scene_package_candidate_rows(
            scene_document_, cooked_scene_path, package_index_path, {std::string_view(prefab_path)});
        const auto package_registration_draft = mirakana::editor::make_scene_package_registration_draft_rows(
            package_candidates, project_.root_path, current_runtime_package_files());
        const auto package_registration_apply_plan = mirakana::editor::make_scene_package_registration_apply_plan(
            package_registration_draft, project_.root_path, project_.game_manifest_path);
        ImGui::SeparatorText("Scene Package Candidates");
        if (ImGui::BeginTable("Scene Package Candidates", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Kind");
            ImGui::TableSetupColumn("Runtime");
            ImGui::TableSetupColumn("Path");
            ImGui::TableHeadersRow();
            for (const auto& candidate : package_candidates) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(scene_package_candidate_kind_label(candidate.kind));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(candidate.runtime_file ? "yes" : "no");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(candidate.path.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::SeparatorText("Package Registration Draft");
        if (ImGui::BeginTable("Package Registration Draft", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Kind");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Runtime Path");
            ImGui::TableSetupColumn("Diagnostic");
            ImGui::TableHeadersRow();
            for (const auto& row : package_registration_draft) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(scene_package_candidate_kind_label(row.kind));
                ImGui::TableSetColumnIndex(1);
                imgui_text_unformatted(mirakana::editor::scene_package_registration_draft_status_label(row.status));
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.runtime_package_path.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.diagnostic.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::SeparatorText("Package Registration Apply");
        ImGui::Text("Manifest: %s", package_registration_apply_plan.game_manifest_path.c_str());
        ImGui::Text("Reviewed additions: %zu", package_registration_apply_plan.runtime_package_files.size());
        if (package_registration_apply_plan.can_apply) {
            if (ImGui::Button("Apply Package Registration")) {
                apply_package_registration(package_registration_apply_plan);
            }
        } else {
            ImGui::TextDisabled("Apply Package Registration");
        }
        if (!package_registration_apply_plan.diagnostic.empty()) {
            ImGui::TextWrapped("%s", package_registration_apply_plan.diagnostic.c_str());
        }
        if (!package_registration_apply_status_.empty()) {
            ImGui::TextWrapped("Last apply: %s", package_registration_apply_status_.c_str());
        }

        draw_prefab_variant_panel();

        ImGui::SeparatorText("Import Queue");
        const auto& progress = asset_panel_model.pipeline.progress;
        ImGui::Text("Pending: %zu  Imported: %zu  Failed: %zu", progress.pending_count, progress.imported_count,
                    progress.failed_count);
        const std::array<char, 64> progress_label = [&progress] {
            std::array<char, 64> label{};
            std::snprintf(label.data(), label.size(), "%zu / %zu", progress.completed_count, progress.total_count);
            return label;
        }();
        ImGui::ProgressBar(progress.completion_ratio, ImVec2{-1.0F, 0.0F}, progress_label.data());
        if (ImGui::Button("Import Assets")) {
            execute_editor_asset_import();
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse Import Sources")) {
            show_asset_import_open_dialog();
        }
        ImGui::SameLine();
        if (ImGui::Button("Simulate Hot Reload")) {
            simulate_asset_hot_reload();
        }
        if (!asset_import_open_dialog_status_.empty()) {
            ImGui::TextWrapped("%s", asset_import_open_dialog_status_.c_str());
        }
        if (!asset_import_open_dialog_.rows.empty() &&
            ImGui::BeginTable("Asset Import Open Dialog", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : asset_import_open_dialog_.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }
        if (!asset_import_external_copy_.rows.empty()) {
            if (asset_import_external_copy_.can_copy) {
                if (ImGui::Button("Copy External Sources")) {
                    copy_asset_import_external_sources();
                }
            } else {
                ImGui::TextDisabled("Copy External Sources");
            }
            if (ImGui::BeginTable("Asset Import External Source Copy", 4,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Status");
                ImGui::TableSetupColumn("Source");
                ImGui::TableSetupColumn("Target");
                ImGui::TableSetupColumn("Diagnostic");
                ImGui::TableHeadersRow();
                for (const auto& row : asset_import_external_copy_.rows) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(row.status_label.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(row.source_path.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(row.target_project_path.c_str());
                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextUnformatted(row.diagnostic.c_str());
                }
                ImGui::EndTable();
            }
        }
        if (ImGui::BeginChild("Asset Import Queue", ImVec2{0.0F, 96.0F}, ImGuiChildFlags_Borders)) {
            for (const auto& item : asset_panel_model.import_queue) {
                ImGui::Text("%s  %s", item.status_label.c_str(), item.output_path.c_str());
                if (!item.diagnostic.empty() && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", item.diagnostic.c_str());
                }
            }
        }
        ImGui::EndChild();

        if (!asset_panel_model.pipeline.diagnostics.empty()) {
            ImGui::SeparatorText("Import Diagnostics");
            if (ImGui::BeginChild("Asset Import Diagnostics", ImVec2{0.0F, 72.0F}, ImGuiChildFlags_Borders)) {
                for (const auto& diagnostic : asset_panel_model.pipeline.diagnostics) {
                    ImGui::TextWrapped("%s: %s", diagnostic.output_path.c_str(), diagnostic.diagnostic.c_str());
                }
            }
            ImGui::EndChild();
        }

        if (!asset_panel_model.pipeline.dependencies.empty()) {
            ImGui::SeparatorText("Import Dependencies");
            if (ImGui::BeginChild("Asset Import Dependencies", ImVec2{0.0F, 72.0F}, ImGuiChildFlags_Borders)) {
                for (const auto& dependency : asset_panel_model.pipeline.dependencies) {
                    ImGui::Text("asset #%llu -> dependency #%llu",
                                static_cast<unsigned long long>(dependency.asset.value),
                                static_cast<unsigned long long>(dependency.dependency.value));
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", dependency.path.c_str());
                    }
                }
            }
            ImGui::EndChild();
        }

        if (!asset_panel_model.pipeline.thumbnail_requests.empty()) {
            ImGui::SeparatorText("Thumbnail Requests");
            if (ImGui::BeginChild("Asset Thumbnail Requests", ImVec2{0.0F, 88.0F}, ImGuiChildFlags_Borders)) {
                for (const auto& request : asset_panel_model.pipeline.thumbnail_requests) {
                    ImGui::Text("%s  %s", request.label.c_str(), request.output_path.c_str());
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("source: %s", request.source_path.c_str());
                    }
                }
            }
            ImGui::EndChild();
        }

        if (material_preview_panel_model.has_value()) {
            draw_material_asset_preview_panel(*material_preview_panel_model);
        } else if (!asset_panel_model.pipeline.material_previews.empty()) {
            ImGui::SeparatorText("Material Preview");
            for (const auto& preview : asset_panel_model.pipeline.material_previews) {
                const auto preview_label = preview.name.empty() ? preview.artifact_path : preview.name;
                const auto tree_label = (preview_label.empty() ? std::string{"Selected Material"} : preview_label) +
                                        "##preview-" + std::to_string(preview.material.value);
                if (!ImGui::TreeNode(tree_label.c_str())) {
                    continue;
                }
                const std::string preview_status{
                    mirakana::editor::editor_material_preview_status_label(preview.status)};
                ImGui::Text("Status: %s", preview_status.c_str());
                if (!preview.artifact_path.empty()) {
                    ImGui::Text("Artifact: %s", preview.artifact_path.c_str());
                }
                if (!preview.diagnostic.empty()) {
                    ImGui::TextWrapped("%s", preview.diagnostic.c_str());
                }
                ImGui::Text("Uniform: %llu bytes", static_cast<unsigned long long>(preview.material_uniform_bytes));
                ImGui::ColorButton("Base Color", ImVec4{preview.base_color[0], preview.base_color[1],
                                                        preview.base_color[2], preview.base_color[3]});
                ImGui::SameLine();
                ImGui::Text("metallic %.2f  roughness %.2f", preview.metallic, preview.roughness);
                ImGui::Text("emissive %.2f %.2f %.2f", preview.emissive[0], preview.emissive[1], preview.emissive[2]);
                ImGui::Text("Double sided: %s  Alpha test: %s  Alpha blend: %s", preview.double_sided ? "yes" : "no",
                            preview.requires_alpha_test ? "yes" : "no", preview.requires_alpha_blending ? "yes" : "no");
                if (!preview.texture_rows.empty() &&
                    ImGui::BeginTable("Material Texture Dependencies", 4, ImGuiTableFlags_BordersInnerV)) {
                    ImGui::TableSetupColumn("Slot");
                    ImGui::TableSetupColumn("Asset");
                    ImGui::TableSetupColumn("Status");
                    ImGui::TableSetupColumn("Path");
                    ImGui::TableHeadersRow();
                    for (const auto& texture : preview.texture_rows) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        imgui_text_unformatted(mirakana::editor::editor_material_texture_slot_label(texture.slot));
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%llu", static_cast<unsigned long long>(texture.texture.value));
                        ImGui::TableSetColumnIndex(2);
                        imgui_text_unformatted(
                            mirakana::editor::editor_material_preview_texture_status_label(texture.status));
                        if (!texture.diagnostic.empty() && ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", texture.diagnostic.c_str());
                        }
                        ImGui::TableSetColumnIndex(3);
                        ImGui::TextUnformatted(texture.artifact_path.c_str());
                    }
                    ImGui::EndTable();
                }
                if (ImGui::BeginTable("Material Binding Metadata", 4, ImGuiTableFlags_BordersInnerV)) {
                    ImGui::TableSetupColumn("Set");
                    ImGui::TableSetupColumn("Binding");
                    ImGui::TableSetupColumn("Resource");
                    ImGui::TableSetupColumn("Semantic");
                    ImGui::TableHeadersRow();
                    for (const auto& binding : preview.bindings) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%u", binding.set);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%u", binding.binding);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(material_binding_resource_kind_label(binding.resource_kind));
                        ImGui::TableSetColumnIndex(3);
                        ImGui::TextUnformatted(binding.semantic.c_str());
                    }
                    ImGui::EndTable();
                }
                mirakana::editor::EditorMaterialAssetPreviewPanelModel fallback_preview_model;
                fallback_preview_model.material = preview.material;
                const auto execution = refresh_material_gpu_preview_execution(preview.material);
                mirakana::editor::apply_editor_material_gpu_preview_execution_snapshot(fallback_preview_model,
                                                                                       execution);
                draw_material_gpu_preview(fallback_preview_model);
                ImGui::TreePop();
            }
        }

        if (std::ranges::any_of(asset_panel_model.hot_reload_summary_rows,
                                [](const auto& row) { return row.count > 0; })) {
            ImGui::SeparatorText("Hot Reload");
            if (ImGui::BeginTable("Hot Reload Summary", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Kind");
                ImGui::TableSetupColumn("Count");
                ImGui::TableHeadersRow();
                for (const auto& row : asset_panel_model.hot_reload_summary_rows) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(row.label.c_str());
                    ImGui::TableSetColumnIndex(1);
                    if (row.attention) {
                        ImGui::TextColored(ImVec4{1.0F, 0.55F, 0.25F, 1.0F}, "%zu", row.count);
                    } else {
                        ImGui::Text("%zu", row.count);
                    }
                }
                ImGui::EndTable();
            }
        }

        ImGui::SeparatorText("Shader Compile");
        ImGui::Text("Pending: %zu  Cached: %zu  Compiled: %zu  Failed: %zu", shader_compiles_.pending_count(),
                    shader_compiles_.cached_count(), shader_compiles_.compiled_count(),
                    shader_compiles_.failed_count());
        if (ImGui::Button("Compile Shader")) {
            compile_editor_shaders();
        }
        ImGui::SameLine();
        if (ImGui::Button("Mark Cache Hit")) {
            complete_demo_shader_compile(true);
        }
        if (ImGui::BeginChild("Shader Compile Queue", ImVec2{0.0F, 72.0F}, ImGuiChildFlags_Borders)) {
            for (const auto& item : shader_compiles_.items()) {
                const std::string status_label{mirakana::editor::editor_shader_compile_status_label(item.status)};
                ImGui::Text("%s  %s", status_label.c_str(), item.output_path.c_str());
                if (!item.diagnostic.empty() && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", item.diagnostic.c_str());
                }
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }

    void draw_material_asset_preview_panel(mirakana::editor::EditorMaterialAssetPreviewPanelModel model) {
        ImGui::SeparatorText("Material Preview");
        const auto preview_label = model.material_name.empty() ? model.material_path : model.material_name;
        const auto tree_label = (preview_label.empty() ? std::string{"Selected Material"} : preview_label) +
                                "##material-asset-preview-" + std::to_string(model.material.value);
        if (!ImGui::TreeNode(tree_label.c_str())) {
            return;
        }

        ImGui::Text("Status: %s", model.status_label.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("GPU payload: %s", model.gpu_status_label.c_str());
        if (!model.material_path.empty()) {
            ImGui::Text("Path: %s", model.material_path.c_str());
        }
        if (!model.preview.artifact_path.empty()) {
            ImGui::Text("Artifact: %s", model.preview.artifact_path.c_str());
        }
        if (!model.preview.diagnostic.empty()) {
            ImGui::TextWrapped("%s", model.preview.diagnostic.c_str());
        }

        ImGui::Text("Uniform: %llu bytes", static_cast<unsigned long long>(model.preview.material_uniform_bytes));
        ImGui::ColorButton("Base Color", ImVec4{model.preview.base_color[0], model.preview.base_color[1],
                                                model.preview.base_color[2], model.preview.base_color[3]});
        ImGui::SameLine();
        ImGui::Text("metallic %.2f  roughness %.2f", model.preview.metallic, model.preview.roughness);
        ImGui::Text("emissive %.2f %.2f %.2f", model.preview.emissive[0], model.preview.emissive[1],
                    model.preview.emissive[2]);
        ImGui::Text("Double sided: %s  Alpha test: %s  Alpha blend: %s", model.preview.double_sided ? "yes" : "no",
                    model.preview.requires_alpha_test ? "yes" : "no",
                    model.preview.requires_alpha_blending ? "yes" : "no");

        if (!model.preview.texture_rows.empty() &&
            ImGui::BeginTable("Material Texture Dependencies", 4, ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("Slot");
            ImGui::TableSetupColumn("Asset");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Path");
            ImGui::TableHeadersRow();
            for (const auto& texture : model.preview.texture_rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                imgui_text_unformatted(mirakana::editor::editor_material_texture_slot_label(texture.slot));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%llu", static_cast<unsigned long long>(texture.texture.value));
                ImGui::TableSetColumnIndex(2);
                imgui_text_unformatted(mirakana::editor::editor_material_preview_texture_status_label(texture.status));
                if (!texture.diagnostic.empty() && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", texture.diagnostic.c_str());
                }
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(texture.artifact_path.c_str());
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginTable("Material Binding Metadata", 4, ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("Set");
            ImGui::TableSetupColumn("Binding");
            ImGui::TableSetupColumn("Resource");
            ImGui::TableSetupColumn("Semantic");
            ImGui::TableHeadersRow();
            for (const auto& binding : model.preview.bindings) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", binding.set);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", binding.binding);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(material_binding_resource_kind_label(binding.resource_kind));
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(binding.semantic.c_str());
            }
            ImGui::EndTable();
        }

        ImGui::SeparatorText("GPU Payload");
        ImGui::Text("Plan: %s", model.gpu_status_label.c_str());
        if (!model.gpu_plan.diagnostic.empty()) {
            ImGui::TextWrapped("%s", model.gpu_plan.diagnostic.c_str());
        }
        if (!model.texture_payload_rows.empty() &&
            ImGui::BeginTable("Material GPU Texture Payloads", 6,
                              ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Slot");
            ImGui::TableSetupColumn("Asset");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("Bytes");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Path");
            ImGui::TableHeadersRow();
            for (const auto& row : model.texture_payload_rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.slot_label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%llu", static_cast<unsigned long long>(row.texture.value));
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%u x %u", row.width, row.height);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%zu", row.byte_count);
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(row.ready ? "ready" : "attention");
                if (!row.diagnostic.empty() && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", row.diagnostic.c_str());
                }
                ImGui::TableSetColumnIndex(5);
                ImGui::TextUnformatted(row.artifact_path.c_str());
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginTable("Material Preview Shader Readiness", 5,
                              ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Variant");
            ImGui::TableSetupColumn("Target");
            ImGui::TableSetupColumn("Vertex");
            ImGui::TableSetupColumn("Fragment");
            ImGui::TableSetupColumn("Use");
            ImGui::TableHeadersRow();
            for (const auto& row : model.shader_rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.target_label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.vertex_status_label.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.fragment_status_label.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(row.required ? "required" : "optional");
            }
            ImGui::EndTable();
        }

        const auto execution = refresh_material_gpu_preview_execution(model.material);
        mirakana::editor::apply_editor_material_gpu_preview_execution_snapshot(model, execution);
        draw_material_gpu_preview(model);
        ImGui::TreePop();
    }

    [[nodiscard]] mirakana::editor::EditorMaterialGpuPreviewExecutionSnapshot
    refresh_material_gpu_preview_execution(mirakana::AssetId material) {
        auto& cache = ensure_material_preview_gpu_cache(material);
        if (cache.ready()) {
            render_material_preview_gpu_cache(cache);
        }
        return mirakana::editor::make_material_gpu_preview_execution_snapshot(cache);
    }

    void draw_material_gpu_preview(const mirakana::editor::EditorMaterialAssetPreviewPanelModel& model) {
        auto& cache = material_preview_gpu_;
        ImGui::SeparatorText("GPU Binding Preview");
        ImGui::Text("Status: %s", model.gpu_execution_status_label.c_str());
        if (!model.gpu_execution_diagnostic.empty()) {
            ImGui::TextWrapped("%s", model.gpu_execution_diagnostic.c_str());
        }
        if (!model.gpu_execution_ready) {
            return;
        }

        if (cache.ready()) {
            const auto* display = cache.display_texture();
            ImGui::Image(ImTextureRef(display->imgui_texture_id()), ImVec2{128.0F, 128.0F});
            ImGui::Text("Backend: %s  Frame: %llu  Display: %s", model.gpu_execution_backend_label.c_str(),
                        static_cast<unsigned long long>(model.gpu_execution_frames_rendered),
                        model.gpu_execution_display_path_label.c_str());
            if (model.gpu_execution_vulkan_visible_refresh_evidence != "not-applicable") {
                ImGui::Text("Vulkan-scope visible refresh: %s",
                            model.gpu_execution_vulkan_visible_refresh_evidence.c_str());
            }
            if (model.gpu_execution_metal_visible_refresh_evidence != "not-applicable") {
                ImGui::Text("Metal-scope visible refresh: %s",
                            model.gpu_execution_metal_visible_refresh_evidence.c_str());
            }
            const char* material_display_diag = display->diagnostic();
            if (material_display_diag != nullptr && *material_display_diag != '\0') {
                ImGui::TextDisabled("%s", material_display_diag);
            }
        }
    }

    void draw_console_panel() {
        ImGui::Begin("Console");
        for (const auto& record : log_.records()) {
            ImGui::Text("[%s] %s", record.category.c_str(), record.message.c_str());
        }
        ImGui::End();
    }

    void draw_viewport_panel() {
        ImGui::Begin("Viewport");
        viewport_.set_focused(ImGui::IsWindowFocused());
        viewport_.set_hovered(ImGui::IsWindowHovered());
        draw_viewport_toolbar();
        draw_game_module_driver_controls();
        draw_in_process_runtime_host_controls();
        draw_runtime_host_playtest_launch_controls();

        ImVec2 available = ImGui::GetContentRegionAvail();
        const float status_height = 64.0F;
        float surface_height = std::max(96.0F, available.y - status_height);
        if (available.x >= 1.0F && surface_height >= 1.0F) {
            const auto next_extent = mirakana::Extent2D{
                static_cast<std::uint32_t>(available.x),
                static_cast<std::uint32_t>(surface_height),
            };
            viewport_.resize(mirakana::editor::ViewportExtent{next_extent.width, next_extent.height});
            if (viewport_surface_ != nullptr) {
                viewport_surface_->resize(next_extent);
            }
            if (viewport_display_texture_ != nullptr) {
                viewport_display_texture_->resize(next_extent);
            }
        }

        const std::string renderer_name{viewport_.renderer_name()};
        ImGui::Text("Renderer: %s", renderer_name.c_str());
        ImGui::Text("Size: %u x %u  Frames: %llu  Sim: %llu", viewport_.extent().width, viewport_.extent().height,
                    static_cast<unsigned long long>(viewport_.rendered_frame_count()),
                    static_cast<unsigned long long>(viewport_.simulation_frame_count()));
        const std::string viewport_mode_label{mirakana::editor::viewport_run_mode_label(viewport_.run_mode())};
        const std::string viewport_tool_label_str{mirakana::editor::viewport_tool_label(viewport_.active_tool())};
        ImGui::Text("Mode: %s  Tool: %s", viewport_mode_label.c_str(), viewport_tool_label_str.c_str());
        if (viewport_surface_ != nullptr) {
            ImGui::Text("Target: texture #%u  RHI frames: %llu", viewport_surface_->color_texture().value,
                        static_cast<unsigned long long>(viewport_surface_->frames_rendered()));
        }
        ImGui::Text("Scene draw: %zu mesh(es), %zu sprite(s), %zu light(s), primary camera: %s",
                    last_viewport_submit_.meshes_submitted, last_viewport_submit_.sprites_submitted,
                    last_viewport_submit_.lights_available, last_viewport_submit_.has_primary_camera ? "yes" : "no");
        ImGui::Text("Viewport shaders: D3D12 %s  Vulkan %s  Ready: %zu/%zu",
                    viewport_shader_artifacts_.ready_for_d3d12() ? "ready" : "not ready",
                    viewport_shader_artifacts_.ready_for_vulkan() ? "ready" : "not ready",
                    viewport_shader_artifacts_.ready_count(), viewport_shader_artifacts_.item_count());
        if (viewport_display_texture_ != nullptr) {
            ImGui::Text("Display texture: %u x %u  Version: %llu  Path: %s", viewport_display_texture_->extent().width,
                        viewport_display_texture_->extent().height,
                        static_cast<unsigned long long>(viewport_display_texture_->version()),
                        viewport_display_texture_->display_path_label());
            const char* display_diag = viewport_display_texture_->diagnostic();
            if (display_diag != nullptr && *display_diag != '\0') {
                ImGui::TextDisabled("%s", display_diag);
            }
        }

        if (ImGui::BeginChild("Viewport Surface", ImVec2{0.0F, surface_height}, ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_NoScrollbar)) {
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            const ImVec2 size = ImGui::GetContentRegionAvail();
            const ImVec2 bottom_right{origin.x + size.x, origin.y + size.y};
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            if (viewport_surface_ != nullptr && viewport_display_texture_ != nullptr) {
                render_editor_scene_viewport();
                bool display_updated = false;
                bool attempted_native_display = false;
#if defined(MK_EDITOR_ENABLE_D3D12)
                if (viewport_device_ != nullptr &&
                    viewport_.active_render_backend() == mirakana::editor::EditorRenderBackend::d3d12) {
                    attempted_native_display = true;
                    const auto display_frame = viewport_surface_->prepare_display_frame();
                    display_updated =
                        viewport_display_texture_->update_from_d3d12_shared_texture(*viewport_device_, display_frame);
                }
#endif
                if (!display_updated) {
                    const auto frame = viewport_surface_->readback_color_frame();
                    (void)viewport_display_texture_->update_from_frame(frame, attempted_native_display);
                }
                ImGui::Image(ImTextureRef(viewport_display_texture_->imgui_texture_id()), size);
            } else {
                draw_list->AddRectFilled(origin, bottom_right, IM_COL32(26, 28, 32, 255));
                ImGui::InvisibleButton("viewport-surface-hitbox", size);
            }
            draw_list->AddRect(origin, bottom_right, IM_COL32(82, 92, 110, 255));
            const auto scene_name = viewport_scene().name();
            const char* const scene_text_begin = scene_name.data();
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const char* const scene_text_end = scene_text_begin + scene_name.size();
            draw_list->AddText(ImVec2{origin.x + 12.0F, origin.y + 12.0F}, IM_COL32(230, 235, 242, 255),
                               scene_text_begin, scene_text_end);
            if (const auto* node = viewport_scene().find_node(selected_node())) {
                draw_list->AddText(ImVec2{origin.x + 12.0F, origin.y + 32.0F}, IM_COL32(180, 190, 205, 255),
                                   node->name.c_str());
            }
            viewport_.mark_frame_rendered();
            tick_play_session_from_viewport_frame();
        }
        ImGui::EndChild();
        ImGui::End();
    }

    void draw_viewport_toolbar() {
        draw_viewport_tool_button("Select", "viewport.tool.select", mirakana::editor::ViewportTool::select);
        ImGui::SameLine();
        draw_viewport_tool_button("Move", "viewport.tool.translate", mirakana::editor::ViewportTool::translate);
        ImGui::SameLine();
        draw_viewport_tool_button("Rotate", "viewport.tool.rotate", mirakana::editor::ViewportTool::rotate);
        ImGui::SameLine();
        draw_viewport_tool_button("Scale", "viewport.tool.scale", mirakana::editor::ViewportTool::scale);
        ImGui::SameLine();
        ImGui::TextUnformatted("|");
        ImGui::SameLine();
        draw_viewport_transform_delta_controls();
        ImGui::SameLine();
        ImGui::TextUnformatted("|");
        ImGui::SameLine();

        const auto controls = play_session_controls_model();
        if (play_session_control_enabled(controls, mirakana::editor::EditorPlaySessionControlCommand::play)) {
            if (ImGui::Button("Play")) {
                execute_command("run.play");
            }
        } else if (play_session_control_enabled(controls, mirakana::editor::EditorPlaySessionControlCommand::pause)) {
            if (ImGui::Button("Pause")) {
                execute_command("run.pause");
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                execute_command("run.stop");
            }
        } else {
            const bool can_resume =
                play_session_control_enabled(controls, mirakana::editor::EditorPlaySessionControlCommand::resume);
            if (!can_resume) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Resume")) {
                execute_command("run.resume");
            }
            if (!can_resume) {
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            const bool can_stop =
                play_session_control_enabled(controls, mirakana::editor::EditorPlaySessionControlCommand::stop);
            if (!can_stop) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Stop")) {
                execute_command("run.stop");
            }
            if (!can_stop) {
                ImGui::EndDisabled();
            }
        }
        ImGui::Separator();
    }

    void draw_viewport_tool_button(const char* label, std::string_view command_id,
                                   mirakana::editor::ViewportTool tool) {
        const bool selected = viewport_.active_tool() == tool;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (ImGui::Button(label)) {
            execute_command(command_id);
        }
        if (selected) {
            ImGui::PopStyleColor();
        }
    }

    void render_editor_scene_viewport() {
        if (viewport_surface_ == nullptr || viewport_pipeline_.value == 0) {
            return;
        }

        const auto packet = mirakana::build_scene_render_packet(viewport_scene());
        mirakana::SceneRenderSubmitResult submit_result{};
        viewport_surface_->render_frame(
            mirakana::RhiViewportRenderDesc{viewport_pipeline_, mirakana::Color{0.03F, 0.035F, 0.045F, 1.0F}},
            [this, &packet, &submit_result](mirakana::IRenderer& renderer) {
                submit_result = mirakana::submit_scene_render_packet(renderer, packet,
                                                                     mirakana::SceneRenderSubmitDesc{
                                                                         mirakana::Color{0.76F, 0.86F, 1.0F, 1.0F},
                                                                         &viewport_materials_,
                                                                     });
            });
        last_viewport_submit_ = submit_result;
    }

    void draw_viewport_transform_delta_controls() {
        if (selected_node() == mirakana::null_scene_node ||
            viewport_.active_tool() == mirakana::editor::ViewportTool::select) {
            ImGui::TextDisabled("No transform delta");
            return;
        }

        const auto delta = viewport_delta_step(viewport_.active_tool());
        draw_viewport_axis_delta_button("-X", mirakana::Vec3{-delta.x, 0.0F, 0.0F});
        ImGui::SameLine();
        draw_viewport_axis_delta_button("+X", mirakana::Vec3{delta.x, 0.0F, 0.0F});
        ImGui::SameLine();
        draw_viewport_axis_delta_button("-Y", mirakana::Vec3{0.0F, -delta.y, 0.0F});
        ImGui::SameLine();
        draw_viewport_axis_delta_button("+Y", mirakana::Vec3{0.0F, delta.y, 0.0F});
        ImGui::SameLine();
        draw_viewport_axis_delta_button("-Z", mirakana::Vec3{0.0F, 0.0F, -delta.z});
        ImGui::SameLine();
        draw_viewport_axis_delta_button("+Z", mirakana::Vec3{0.0F, 0.0F, delta.z});
    }

    void draw_viewport_axis_delta_button(const char* label, mirakana::Vec3 delta) {
        if (ImGui::SmallButton(label)) {
            apply_viewport_transform_delta(delta);
        }
    }

    static mirakana::Vec3 viewport_delta_step(mirakana::editor::ViewportTool tool) noexcept {
        switch (tool) {
        case mirakana::editor::ViewportTool::translate:
            return mirakana::Vec3{0.25F, 0.25F, 0.25F};
        case mirakana::editor::ViewportTool::rotate:
            return mirakana::Vec3{15.0F * degrees_to_radians, 15.0F * degrees_to_radians, 15.0F * degrees_to_radians};
        case mirakana::editor::ViewportTool::scale:
            return mirakana::Vec3{0.1F, 0.1F, 0.1F};
        case mirakana::editor::ViewportTool::select:
            return mirakana::Vec3{0.0F, 0.0F, 0.0F};
        }
        return mirakana::Vec3{0.0F, 0.0F, 0.0F};
    }

    [[nodiscard]] static const char*
    scene_authoring_diagnostic_kind_label(mirakana::editor::SceneAuthoringDiagnosticKind kind) noexcept {
        switch (kind) {
        case mirakana::editor::SceneAuthoringDiagnosticKind::missing_asset:
            return "missing_asset";
        case mirakana::editor::SceneAuthoringDiagnosticKind::wrong_asset_kind:
            return "wrong_asset_kind";
        }
        return "unknown";
    }

    [[nodiscard]] static const char*
    scene_package_candidate_kind_label(mirakana::editor::ScenePackageCandidateKind kind) noexcept {
        switch (kind) {
        case mirakana::editor::ScenePackageCandidateKind::scene_source:
            return "scene_source";
        case mirakana::editor::ScenePackageCandidateKind::scene_cooked:
            return "scene_cooked";
        case mirakana::editor::ScenePackageCandidateKind::package_index:
            return "package_index";
        case mirakana::editor::ScenePackageCandidateKind::prefab_source:
            return "prefab_source";
        }
        return "unknown";
    }

    void draw_selected_node_transform_editor() {
        auto draft = mirakana::editor::make_scene_node_transform_draft(active_scene(), selected_node());
        if (!draft.has_value()) {
            return;
        }

        ImGui::SeparatorText("Transform");

        std::array<float, 3> position{draft->position.x, draft->position.y, draft->position.z};
        if (ImGui::DragFloat3("Position", position.data(), 0.05F)) {
            draft->position = mirakana::Vec3{position[0], position[1], position[2]};
            apply_selected_transform_draft(*draft);
            return;
        }

        std::array<float, 3> rotation_degrees{
            draft->rotation_radians.x * radians_to_degrees,
            draft->rotation_radians.y * radians_to_degrees,
            draft->rotation_radians.z * radians_to_degrees,
        };
        if (ImGui::DragFloat3("Rotation", rotation_degrees.data(), 0.5F)) {
            draft->rotation_radians = mirakana::Vec3{
                rotation_degrees[0] * degrees_to_radians,
                rotation_degrees[1] * degrees_to_radians,
                rotation_degrees[2] * degrees_to_radians,
            };
            apply_selected_transform_draft(*draft);
            return;
        }

        std::array<float, 3> scale{draft->scale.x, draft->scale.y, draft->scale.z};
        if (ImGui::DragFloat3("Scale", scale.data(), 0.05F, 0.001F, 1000.0F)) {
            draft->scale = mirakana::Vec3{scale[0], scale[1], scale[2]};
            apply_selected_transform_draft(*draft);
        }
    }

    void draw_selected_node_component_editor() {
        auto draft = mirakana::editor::make_scene_node_component_draft(active_scene(), selected_node());
        if (!draft.has_value()) {
            return;
        }

        ImGui::SeparatorText("Components");

        bool has_camera = draft->components.camera.has_value();
        if (ImGui::Checkbox("Camera", &has_camera)) {
            if (has_camera) {
                draft->components.camera = mirakana::CameraComponent{
                    mirakana::CameraProjectionMode::perspective, 1.04719758F, 10.0F, 0.1F, 500.0F, false};
            } else {
                draft->components.camera.reset();
            }
            apply_selected_component_draft(*draft);
            return;
        }
        if (draft->components.camera.has_value() && draw_camera_component_editor(*draft)) {
            return;
        }

        bool has_light = draft->components.light.has_value();
        if (ImGui::Checkbox("Light", &has_light)) {
            if (has_light) {
                draft->components.light = mirakana::LightComponent{mirakana::LightType::directional,
                                                                   mirakana::Vec3{1.0F, 0.95F, 0.85F},
                                                                   1.0F,
                                                                   10.0F,
                                                                   0.0F,
                                                                   0.0F,
                                                                   false};
            } else {
                draft->components.light.reset();
            }
            apply_selected_component_draft(*draft);
            return;
        }
        if (draft->components.light.has_value() && draw_light_component_editor(*draft)) {
            return;
        }

        bool has_mesh_renderer = draft->components.mesh_renderer.has_value();
        if (ImGui::Checkbox("Mesh Renderer", &has_mesh_renderer)) {
            if (has_mesh_renderer) {
                draft->components.mesh_renderer = mirakana::MeshRendererComponent{
                    mirakana::AssetId::from_name("editor.default.mesh"),
                    mirakana::AssetId::from_name("editor.default.material"),
                    true,
                };
            } else {
                draft->components.mesh_renderer.reset();
            }
            apply_selected_component_draft(*draft);
            return;
        }
        if (draft->components.mesh_renderer.has_value()) {
            auto mesh_renderer = *draft->components.mesh_renderer;
            bool visible = mesh_renderer.visible;
            if (ImGui::Checkbox("Visible", &visible)) {
                mesh_renderer.visible = visible;
                draft->components.mesh_renderer = mesh_renderer;
                apply_selected_component_draft(*draft);
                return;
            }
            ImGui::Text("Mesh: #%llu", static_cast<unsigned long long>(mesh_renderer.mesh.value));
            ImGui::Text("Material: #%llu", static_cast<unsigned long long>(mesh_renderer.material.value));
            if (const auto* picked = content_browser_.selected_asset();
                picked != nullptr && picked->kind == mirakana::AssetKind::mesh) {
                if (ImGui::Button("Assign selected mesh asset")) {
                    mesh_renderer.mesh = picked->id;
                    draft->components.mesh_renderer = mesh_renderer;
                    apply_selected_component_draft(*draft);
                    return;
                }
            }
            if (const auto* picked = content_browser_.selected_asset();
                picked != nullptr && picked->kind == mirakana::AssetKind::material) {
                if (ImGui::Button("Assign selected material asset")) {
                    mesh_renderer.material = picked->id;
                    draft->components.mesh_renderer = mesh_renderer;
                    apply_selected_component_draft(*draft);
                    return;
                }
            }
        }

        bool has_sprite_renderer = draft->components.sprite_renderer.has_value();
        if (ImGui::Checkbox("Sprite Renderer", &has_sprite_renderer)) {
            if (has_sprite_renderer) {
                draft->components.sprite_renderer = mirakana::SpriteRendererComponent{
                    mirakana::AssetId::from_name("editor.default.sprite"),
                    mirakana::AssetId::from_name("editor.default.material"),
                    mirakana::Vec2{1.0F, 1.0F},
                    {1.0F, 1.0F, 1.0F, 1.0F},
                    true,
                };
            } else {
                draft->components.sprite_renderer.reset();
            }
            apply_selected_component_draft(*draft);
            return;
        }
        if (draft->components.sprite_renderer.has_value()) {
            (void)draw_sprite_component_editor(*draft);
        }
    }

    bool draw_camera_component_editor(mirakana::editor::SceneNodeComponentDraft& draft) {
        auto camera = *draft.components.camera;
        int projection = camera.projection == mirakana::CameraProjectionMode::orthographic ? 1 : 0;
        static constexpr std::array<const char*, 2> projection_labels = {"Perspective", "Orthographic"};
        if (ImGui::Combo("Projection", &projection, projection_labels.data(),
                         static_cast<int>(projection_labels.size()))) {
            camera.projection = projection == 1 ? mirakana::CameraProjectionMode::orthographic
                                                : mirakana::CameraProjectionMode::perspective;
            draft.components.camera = camera;
            apply_selected_component_draft(draft);
            return true;
        }

        bool primary = camera.primary;
        if (ImGui::Checkbox("Primary", &primary)) {
            camera.primary = primary;
            draft.components.camera = camera;
            apply_selected_component_draft(draft);
            return true;
        }

        float fov_degrees = camera.vertical_fov_radians * radians_to_degrees;
        if (ImGui::DragFloat("Vertical FOV", &fov_degrees, 0.5F, 1.0F, 179.0F)) {
            camera.vertical_fov_radians = fov_degrees * degrees_to_radians;
            draft.components.camera = camera;
            apply_selected_component_draft(draft);
            return true;
        }

        if (ImGui::DragFloat("Ortho Height", &camera.orthographic_height, 0.1F, 0.001F, 1000000.0F)) {
            draft.components.camera = camera;
            apply_selected_component_draft(draft);
            return true;
        }

        if (ImGui::DragFloat("Near Plane", &camera.near_plane, 0.01F, 0.0001F, camera.far_plane - 0.0001F) ||
            ImGui::DragFloat("Far Plane", &camera.far_plane, 1.0F, camera.near_plane + 0.0001F, 100000000.0F)) {
            draft.components.camera = camera;
            apply_selected_component_draft(draft);
            return true;
        }

        return false;
    }

    bool draw_light_component_editor(mirakana::editor::SceneNodeComponentDraft& draft) {
        auto light = *draft.components.light;
        int type = light_type_index(light.type);
        static constexpr std::array<const char*, 3> type_labels = {"Directional", "Point", "Spot"};
        if (ImGui::Combo("Light Type", &type, type_labels.data(), static_cast<int>(type_labels.size()))) {
            light.type = light_type_from_index(type);
            if (light.type == mirakana::LightType::point && light.range <= 0.0F) {
                light.range = 10.0F;
            }
            if (light.type == mirakana::LightType::spot && light.outer_cone_radians <= light.inner_cone_radians) {
                light.inner_cone_radians = 0.35F;
                light.outer_cone_radians = 0.7F;
                light.range = light.range > 0.0F ? light.range : 10.0F;
            }
            draft.components.light = light;
            apply_selected_component_draft(draft);
            return true;
        }

        std::array<float, 3> color{light.color.x, light.color.y, light.color.z};
        if (ImGui::DragFloat3("Light Color", color.data(), 0.01F, 0.0F, 64.0F)) {
            light.color = mirakana::Vec3{color[0], color[1], color[2]};
            draft.components.light = light;
            apply_selected_component_draft(draft);
            return true;
        }

        if (ImGui::DragFloat("Intensity", &light.intensity, 0.1F, 0.0F, 100000000.0F) ||
            ImGui::DragFloat("Range", &light.range, 0.1F, 0.0F, 100000000.0F)) {
            draft.components.light = light;
            apply_selected_component_draft(draft);
            return true;
        }

        bool casts_shadows = light.casts_shadows;
        if (ImGui::Checkbox("Casts Shadows", &casts_shadows)) {
            light.casts_shadows = casts_shadows;
            draft.components.light = light;
            apply_selected_component_draft(draft);
            return true;
        }

        if (light.type == mirakana::LightType::spot) {
            float inner_degrees = light.inner_cone_radians * radians_to_degrees;
            float outer_degrees = light.outer_cone_radians * radians_to_degrees;
            if (ImGui::DragFloat("Inner Cone", &inner_degrees, 0.5F, 0.0F, 179.0F) ||
                ImGui::DragFloat("Outer Cone", &outer_degrees, 0.5F, inner_degrees + 0.1F, 180.0F)) {
                light.inner_cone_radians = inner_degrees * degrees_to_radians;
                light.outer_cone_radians = outer_degrees * degrees_to_radians;
                draft.components.light = light;
                apply_selected_component_draft(draft);
                return true;
            }
        }

        return false;
    }

    bool draw_sprite_component_editor(mirakana::editor::SceneNodeComponentDraft& draft) {
        auto sprite = *draft.components.sprite_renderer;
        bool visible = sprite.visible;
        if (ImGui::Checkbox("Sprite Visible", &visible)) {
            sprite.visible = visible;
            draft.components.sprite_renderer = sprite;
            apply_selected_component_draft(draft);
            return true;
        }

        std::array<float, 2> size{sprite.size.x, sprite.size.y};
        if (ImGui::DragFloat2("Sprite Size", size.data(), 0.05F, 0.0001F, 1000000.0F)) {
            sprite.size = mirakana::Vec2{size[0], size[1]};
            draft.components.sprite_renderer = sprite;
            apply_selected_component_draft(draft);
            return true;
        }

        std::array<float, 4> tint{sprite.tint[0], sprite.tint[1], sprite.tint[2], sprite.tint[3]};
        if (ImGui::ColorEdit4("Sprite Tint", tint.data())) {
            sprite.tint = {tint[0], tint[1], tint[2], tint[3]};
            draft.components.sprite_renderer = sprite;
            apply_selected_component_draft(draft);
            return true;
        }

        ImGui::Text("Sprite: #%llu", static_cast<unsigned long long>(sprite.sprite.value));
        ImGui::Text("Material: #%llu", static_cast<unsigned long long>(sprite.material.value));
        return false;
    }

    [[nodiscard]] static mirakana::AssetKind asset_kind_from_filter_index(int index) noexcept {
        switch (index) {
        case 1:
            return mirakana::AssetKind::texture;
        case 2:
            return mirakana::AssetKind::mesh;
        case 3:
            return mirakana::AssetKind::material;
        case 4:
            return mirakana::AssetKind::scene;
        case 5:
            return mirakana::AssetKind::audio;
        case 6:
            return mirakana::AssetKind::script;
        case 7:
            return mirakana::AssetKind::shader;
        default:
            return mirakana::AssetKind::unknown;
        }
    }

    [[nodiscard]] mirakana::editor::EditorPlaySessionControlsModel play_session_controls_model() const {
        return mirakana::editor::make_editor_play_session_controls_model(play_session_, scene_document_);
    }

    [[nodiscard]] mirakana::editor::EditorInProcessRuntimeHostDesc make_in_process_runtime_host_desc() const {
        mirakana::editor::EditorInProcessRuntimeHostDesc desc;
        desc.id = "linked_gameplay_driver";
        desc.label = "Loaded Game Module Driver";
        desc.linked_gameplay_driver_available = game_module_driver_ != nullptr;
        return desc;
    }

    [[nodiscard]] mirakana::editor::EditorInProcessRuntimeHostModel make_in_process_runtime_host_model() const {
        const auto desc = make_in_process_runtime_host_desc();
        return mirakana::editor::make_editor_in_process_runtime_host_model(play_session_, scene_document_, desc);
    }

    [[nodiscard]] static bool
    play_session_control_enabled(const mirakana::editor::EditorPlaySessionControlsModel& model,
                                 mirakana::editor::EditorPlaySessionControlCommand command) noexcept {
        const auto it = std::find_if(model.controls.begin(), model.controls.end(),
                                     [command](const auto& row) { return row.command == command; });
        return it != model.controls.end() && it->enabled;
    }

    void log_play_session_rejection(std::string_view action, mirakana::editor::EditorPlaySessionActionStatus status) {
        log_.log(mirakana::LogLevel::warn, "editor",
                 std::string(action) +
                     " rejected: " + std::string(mirakana::editor::editor_play_session_action_status_label(status)));
    }

    void play_viewport() {
        const auto status = play_session_.begin(scene_document_);
        if (status == mirakana::editor::EditorPlaySessionActionStatus::applied) {
            (void)viewport_.play();
            log_.log(mirakana::LogLevel::info, "editor", "Play started");
        } else {
            log_play_session_rejection("Play command", status);
        }
    }

    void pause_viewport() {
        const auto status = play_session_.pause();
        if (status == mirakana::editor::EditorPlaySessionActionStatus::applied) {
            (void)viewport_.pause();
            log_.log(mirakana::LogLevel::info, "editor", "Play paused");
        } else {
            log_play_session_rejection("Pause command", status);
        }
    }

    void resume_viewport() {
        const auto status = play_session_.resume();
        if (status == mirakana::editor::EditorPlaySessionActionStatus::applied) {
            (void)viewport_.resume();
            log_.log(mirakana::LogLevel::info, "editor", "Play resumed");
        } else {
            log_play_session_rejection("Resume command", status);
        }
    }

    void stop_viewport() {
        const auto status = play_session_.stop();
        if (status == mirakana::editor::EditorPlaySessionActionStatus::applied) {
            (void)viewport_.stop();
            log_.log(mirakana::LogLevel::info, "editor", "Play stopped");
        } else {
            log_play_session_rejection("Stop command", status);
        }
    }

    void begin_in_process_runtime_host() {
        if (game_module_driver_ == nullptr) {
            log_.log(mirakana::LogLevel::warn, "editor", "No game module driver is loaded");
            return;
        }

        auto result = mirakana::editor::begin_editor_in_process_runtime_host_session(
            play_session_, scene_document_, *game_module_driver_, make_in_process_runtime_host_desc());
        if (result.action_status == mirakana::editor::EditorPlaySessionActionStatus::applied) {
            (void)viewport_.play();
            log_.log(mirakana::LogLevel::info, "editor",
                     "In-process runtime host started with loaded game module driver");
            return;
        }

        log_.log(mirakana::LogLevel::warn, "editor",
                 result.diagnostic.empty() ? "In-process runtime host did not start" : result.diagnostic);
        log_play_session_rejection("Begin in-process runtime host", result.action_status);
    }

    void tick_play_session_from_viewport_frame() {
        if (play_session_.state() != mirakana::editor::EditorPlaySessionState::play) {
            return;
        }
        const float imgui_delta = ImGui::GetIO().DeltaTime;
        const double delta_seconds = imgui_delta > 0.0F ? static_cast<double>(imgui_delta) : (1.0 / 60.0);
        const auto status = play_session_.tick(delta_seconds);
        if (status == mirakana::editor::EditorPlaySessionActionStatus::applied) {
            (void)viewport_.mark_simulation_tick();
        }
    }

    void set_viewport_tool(mirakana::editor::ViewportTool tool) {
        viewport_.set_active_tool(tool);
    }

    void configure_default_material_palette() {
        mirakana::MaterialDefinition material;
        material.id = mirakana::AssetId::from_name("editor.default.material");
        material.name = "Editor Default";
        material.shading_model = mirakana::MaterialShadingModel::lit;
        material.surface_mode = mirakana::MaterialSurfaceMode::opaque;
        material.factors.base_color = {0.2F, 0.55F, 0.95F, 1.0F};
        viewport_materials_.add(material);
    }

    [[nodiscard]] static mirakana::runtime::RuntimeInputActionTrigger
    make_gamepad_button_trigger(mirakana::GamepadId gamepad_id, mirakana::GamepadButton button) noexcept {
        return mirakana::runtime::RuntimeInputActionTrigger{
            mirakana::runtime::RuntimeInputActionTriggerKind::gamepad_button,
            mirakana::Key::unknown,
            0,
            gamepad_id,
            button,
        };
    }

    [[nodiscard]] static mirakana::runtime::RuntimeInputAxisSource
    make_gamepad_axis_source(mirakana::GamepadId gamepad_id, mirakana::GamepadAxis axis, float scale,
                             float deadzone) noexcept {
        mirakana::runtime::RuntimeInputAxisSource source;
        source.kind = mirakana::runtime::RuntimeInputAxisSourceKind::gamepad_axis;
        source.gamepad_id = gamepad_id;
        source.gamepad_axis = axis;
        source.scale = scale;
        source.deadzone = deadzone;
        return source;
    }

    void configure_default_input_rebinding_profile() {
        input_rebinding_base_actions_.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
        input_rebinding_base_actions_.bind_key_in_context("gameplay", "cancel", mirakana::Key::escape);
        input_rebinding_base_actions_.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left,
                                                               mirakana::Key::right);

        input_rebinding_profile_.profile_id = "player_one";
        input_rebinding_profile_.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
            "gameplay",
            "confirm",
            {make_gamepad_button_trigger(mirakana::GamepadId{1}, mirakana::GamepadButton::south)},
        });
        input_rebinding_profile_.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
            "gameplay",
            "move_x",
            {make_gamepad_axis_source(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F)},
        });
    }

    static mirakana::rhi::GraphicsPipelineHandle
    create_viewport_graphics_pipeline(mirakana::rhi::IRhiDevice& device, mirakana::rhi::Format color_format,
                                      const std::string* vertex_bytecode = nullptr,
                                      const std::string* fragment_bytecode = nullptr) {
        const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
            mirakana::rhi::ShaderStage::vertex,
            "vs_main",
            vertex_bytecode == nullptr ? 64U : static_cast<std::uint64_t>(vertex_bytecode->size()),
            vertex_bytecode == nullptr ? nullptr : vertex_bytecode->data(),
        });
        const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
            mirakana::rhi::ShaderStage::fragment,
            "ps_main",
            fragment_bytecode == nullptr ? 64U : static_cast<std::uint64_t>(fragment_bytecode->size()),
            fragment_bytecode == nullptr ? nullptr : fragment_bytecode->data(),
        });
        const auto layout = device.create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{{}, 0});
        return device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
            .layout = layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = color_format,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
            .vertex_buffers = {},
            .vertex_attributes = {},
            .depth_state = {},
        });
    }

    static int light_type_index(mirakana::LightType type) noexcept {
        switch (type) {
        case mirakana::LightType::point:
            return 1;
        case mirakana::LightType::spot:
            return 2;
        case mirakana::LightType::directional:
        case mirakana::LightType::unknown:
            return 0;
        }
        return 0;
    }

    static mirakana::LightType light_type_from_index(int index) noexcept {
        switch (index) {
        case 1:
            return mirakana::LightType::point;
        case 2:
            return mirakana::LightType::spot;
        default:
            return mirakana::LightType::directional;
        }
    }

    bool execute_scene_authoring_action(mirakana::editor::UndoableAction action) {
        if (play_session_.source_scene_edits_blocked()) {
            log_.log(mirakana::LogLevel::warn, "editor", "Scene authoring is blocked while Play-In-Editor is active");
            return false;
        }
        if (!history_.execute(std::move(action))) {
            log_.log(mirakana::LogLevel::warn, "editor", "Scene authoring action was rejected");
            return false;
        }
        dirty_state_.mark_dirty();
        sync_scene_rename_buffer();
        scene_prefab_refresh_review_.reset();
        return true;
    }

    void apply_selected_transform_draft(const mirakana::editor::SceneNodeTransformDraft& draft) {
        (void)execute_scene_authoring_action(
            mirakana::editor::make_scene_authoring_transform_edit_action(scene_document_, draft));
    }

    void apply_selected_component_draft(const mirakana::editor::SceneNodeComponentDraft& draft) {
        (void)execute_scene_authoring_action(
            mirakana::editor::make_scene_authoring_component_edit_action(scene_document_, draft));
    }

    void apply_viewport_transform_delta(mirakana::Vec3 delta) {
        auto draft = mirakana::editor::make_scene_node_transform_draft(active_scene(), selected_node());
        if (!draft.has_value()) {
            log_.log(mirakana::LogLevel::warn, "editor", "Viewport transform edit was rejected");
            return;
        }

        switch (viewport_.active_tool()) {
        case mirakana::editor::ViewportTool::translate:
            draft->position = draft->position + delta;
            break;
        case mirakana::editor::ViewportTool::rotate:
            draft->rotation_radians = draft->rotation_radians + delta;
            break;
        case mirakana::editor::ViewportTool::scale:
            draft->scale = draft->scale + delta;
            break;
        case mirakana::editor::ViewportTool::select:
            return;
        }

        (void)execute_scene_authoring_action(
            mirakana::editor::make_scene_authoring_transform_edit_action(scene_document_, *draft));
    }

    [[nodiscard]] mirakana::editor::EditorProfilerStatus make_profiler_status() const {
        return mirakana::editor::EditorProfilerStatus{
            log_.records().size(),
            history_.undo_count(),
            history_.redo_count(),
            asset_pipeline_.item_count(),
            asset_pipeline_.hot_reload_events().size(),
            shader_compiles_.item_count(),
            dirty_state_.dirty(),
            dirty_state_.revision(),
        };
    }

    void record_editor_profiler_counters() {
        const auto status = make_profiler_status();
        profiler_recorder_.record_counter(mirakana::CounterSample{
            "editor.log_records", static_cast<double>(status.log_records), profiler_frame_index_});
        profiler_recorder_.record_counter(mirakana::CounterSample{
            "editor.undo_stack", static_cast<double>(status.undo_stack), profiler_frame_index_});
        profiler_recorder_.record_counter(mirakana::CounterSample{
            "editor.redo_stack", static_cast<double>(status.redo_stack), profiler_frame_index_});
        profiler_recorder_.record_counter(mirakana::CounterSample{
            "editor.asset_imports", static_cast<double>(status.asset_imports), profiler_frame_index_});
        profiler_recorder_.record_counter(mirakana::CounterSample{
            "editor.hot_reload_events", static_cast<double>(status.hot_reload_events), profiler_frame_index_});
        profiler_recorder_.record_counter(mirakana::CounterSample{
            "editor.shader_compiles", static_cast<double>(status.shader_compiles), profiler_frame_index_});
        profiler_recorder_.record_counter(
            mirakana::CounterSample{"editor.dirty", status.dirty ? 1.0 : 0.0, profiler_frame_index_});
        profiler_recorder_.record_counter(
            mirakana::CounterSample{"editor.revision", static_cast<double>(status.revision), profiler_frame_index_});
    }

    [[nodiscard]] mirakana::editor::EditorResourcePanelInput make_resource_panel_input() const {
        mirakana::editor::EditorResourcePanelInput input;
        input.frame_index = profiler_frame_index_;
        if (viewport_device_ == nullptr) {
            append_resource_capture_requests(input);
            append_resource_capture_execution_snapshots(input);
            return input;
        }

        input.device_available = true;
        input.backend_id = std::string(viewport_device_->backend_name());
        input.backend_label = std::string(viewport_device_->backend_name());

        const auto stats = viewport_device_->stats();
        input.rhi_counters = {
            mirakana::editor::EditorResourceCounterInput{"buffers_created", "Buffers created", stats.buffers_created},
            mirakana::editor::EditorResourceCounterInput{"textures_created", "Textures created",
                                                         stats.textures_created},
            mirakana::editor::EditorResourceCounterInput{"samplers_created", "Samplers created",
                                                         stats.samplers_created},
            mirakana::editor::EditorResourceCounterInput{"shader_modules_created", "Shader modules created",
                                                         stats.shader_modules_created},
            mirakana::editor::EditorResourceCounterInput{"descriptor_writes", "Descriptor writes",
                                                         stats.descriptor_writes},
            mirakana::editor::EditorResourceCounterInput{"graphics_pipelines_created", "Graphics pipelines created",
                                                         stats.graphics_pipelines_created},
            mirakana::editor::EditorResourceCounterInput{"compute_pipelines_created", "Compute pipelines created",
                                                         stats.compute_pipelines_created},
            mirakana::editor::EditorResourceCounterInput{"command_lists_submitted", "Command lists submitted",
                                                         stats.command_lists_submitted},
            mirakana::editor::EditorResourceCounterInput{"resource_transitions", "Resource transitions",
                                                         stats.resource_transitions},
            mirakana::editor::EditorResourceCounterInput{"draw_calls", "Draw calls", stats.draw_calls},
            mirakana::editor::EditorResourceCounterInput{"indexed_draw_calls", "Indexed draw calls",
                                                         stats.indexed_draw_calls},
            mirakana::editor::EditorResourceCounterInput{"compute_dispatches", "Compute dispatches",
                                                         stats.compute_dispatches},
            mirakana::editor::EditorResourceCounterInput{"bytes_written", "Bytes written", stats.bytes_written},
            mirakana::editor::EditorResourceCounterInput{"bytes_copied", "Bytes copied", stats.bytes_copied},
            mirakana::editor::EditorResourceCounterInput{"bytes_read", "Bytes read", stats.bytes_read},
            mirakana::editor::EditorResourceCounterInput{"shared_texture_exports", "Shared texture exports",
                                                         stats.shared_texture_exports},
            mirakana::editor::EditorResourceCounterInput{"shared_texture_export_failures",
                                                         "Shared texture export failures",
                                                         stats.shared_texture_export_failures},
            mirakana::editor::EditorResourceCounterInput{"gpu_debug_scopes_begun", "GPU debug scopes begun",
                                                         stats.gpu_debug_scopes_begun},
            mirakana::editor::EditorResourceCounterInput{"gpu_debug_markers_inserted", "GPU debug markers inserted",
                                                         stats.gpu_debug_markers_inserted},
        };

        const auto memory = viewport_device_->memory_diagnostics();
        input.memory.os_video_memory_budget_available = memory.os_video_memory_budget_available;
        input.memory.local_video_memory_budget_bytes = memory.local_video_memory_budget_bytes;
        input.memory.local_video_memory_usage_bytes = memory.local_video_memory_usage_bytes;
        input.memory.non_local_video_memory_budget_bytes = memory.non_local_video_memory_budget_bytes;
        input.memory.non_local_video_memory_usage_bytes = memory.non_local_video_memory_usage_bytes;
        input.memory.committed_resources_byte_estimate_available = memory.committed_resources_byte_estimate_available;
        input.memory.committed_resources_byte_estimate = memory.committed_resources_byte_estimate;

        const auto* lifetime = viewport_device_->resource_lifetime_registry();
        if (lifetime != nullptr) {
            for (const auto& record : lifetime->records()) {
                if (record.state == mirakana::rhi::RhiResourceLifetimeState::live) {
                    ++input.lifetime.live_resources;
                } else {
                    ++input.lifetime.deferred_release_resources;
                }
            }
            input.lifetime.lifetime_events = lifetime->events().size();
            for (const auto& desc : rhi_resource_kind_descriptors) {
                const auto count = static_cast<std::uint64_t>(std::ranges::count_if(
                    lifetime->records(), [kind = desc.kind](const mirakana::rhi::RhiResourceLifetimeRecord& record) {
                        return record.kind == kind;
                    }));
                if (count > 0U) {
                    input.lifetime.resources_by_kind.push_back(mirakana::editor::EditorResourceCounterInput{
                        std::string(rhi_resource_kind_id(desc.kind)),
                        std::string(rhi_resource_kind_label(desc.kind)),
                        count,
                    });
                }
            }
        }

        append_resource_capture_requests(input);
        append_resource_capture_execution_snapshots(input);
        return input;
    }

    void append_resource_capture_requests(mirakana::editor::EditorResourcePanelInput& input) const {
        const bool d3d12_viewport = input.device_available && input.backend_id == "d3d12";
        input.capture_requests.push_back(mirakana::editor::EditorResourceCaptureRequestInput{
            "d3d12_debug_layer_gpu_validation",
            "D3D12 Debug Layer / GPU Validation",
            "Windows Graphics Tools",
            "Request debug validation prep",
            {"d3d12-debug-layer"},
            d3d12_viewport,
            resource_capture_request_acknowledged("d3d12_debug_layer_gpu_validation"),
            d3d12_viewport ? "Prepare D3D12 debug layer and GPU-based validation through external host tooling"
                           : "D3D12 debug validation handoff requires an active D3D12 viewport device",
        });
        input.capture_requests.push_back(mirakana::editor::EditorResourceCaptureRequestInput{
            "pix_gpu_capture",
            "PIX GPU Capture",
            "PIX on Windows",
            "Request PIX Capture",
            {"d3d12-windows-primary", "pix-installed"},
            d3d12_viewport,
            resource_capture_request_acknowledged("pix_gpu_capture"),
            d3d12_viewport
                ? "Launch or attach PIX externally; validate D3D12 usage with the debug layer/GPU-based validation if "
                  "capture fails"
                : "PIX GPU capture handoff requires an active D3D12 viewport device",
        });
    }

    void append_resource_capture_execution_snapshots(mirakana::editor::EditorResourcePanelInput& input) const {
        const bool d3d12_viewport = input.device_available && input.backend_id == "d3d12";
        if (d3d12_viewport && resource_capture_request_acknowledged("d3d12_debug_layer_gpu_validation")) {
            input.capture_execution_snapshots.push_back(mirakana::editor::EditorResourceCaptureExecutionInput{
                .id = "d3d12_debug_layer_gpu_validation",
                .label = "D3D12 Debug Layer / GPU Validation",
                .tool_label = "Windows Graphics Tools",
                .requested = true,
                .host_gated = true,
                .capture_started = false,
                .capture_completed = false,
                .capture_failed = false,
                .editor_core_executed = false,
                .exposes_native_handles = false,
                .host_gates = {"d3d12-debug-layer"},
                .artifact_path = "",
                .diagnostic =
                    "Reviewed request acknowledged; waiting for external host debug-layer/GPU-validation evidence",
            });
        }
        if (d3d12_viewport && resource_capture_request_acknowledged("pix_gpu_capture")) {
            bool host_gated = true;
            bool capture_completed = false;
            bool capture_failed = false;
            std::string artifact_path;
            std::string diagnostic = "Reviewed request acknowledged; waiting for external host PIX capture evidence";
            if (pix_host_helper_last_run_valid_) {
                host_gated = false;
                capture_completed = pix_host_helper_last_run_succeeded_;
                capture_failed = !pix_host_helper_last_run_succeeded_;
                artifact_path = pix_host_helper_last_session_dir_;
                diagnostic = "MK_editor reviewed pwsh tools/launch-pix-host-helper.ps1";
                if (pix_host_helper_last_run_skip_launch_.value_or(true)) {
                    diagnostic += " -SkipLaunch";
                } else {
                    diagnostic += " (launch enabled; operator confirmed)";
                }
                diagnostic += "; stdout: ";
                diagnostic += pix_host_helper_last_stdout_summary_;
                if (!pix_host_helper_last_stderr_summary_.empty()) {
                    diagnostic += "; stderr: ";
                    diagnostic += pix_host_helper_last_stderr_summary_;
                }
                diagnostic += "; external PIX capture workflow remains operator-owned";
            }
            input.capture_execution_snapshots.push_back(mirakana::editor::EditorResourceCaptureExecutionInput{
                .id = "pix_gpu_capture",
                .label = "PIX GPU Capture",
                .tool_label = "PIX on Windows",
                .requested = true,
                .host_gated = host_gated,
                .capture_started = false,
                .capture_completed = capture_completed,
                .capture_failed = capture_failed,
                .editor_core_executed = false,
                .exposes_native_handles = false,
                .host_gates = {"d3d12-windows-primary", "pix-installed"},
                .artifact_path = std::move(artifact_path),
                .diagnostic = std::move(diagnostic),
            });
        }
    }

    void execute_pix_host_helper_reviewed(bool skip_launch) {
#if defined(_WIN32)
        pix_host_helper_last_run_skip_launch_ = skip_launch;
        const auto root = find_engine_repository_root_containing_tools_script();
        if (!root.has_value()) {
            pix_host_helper_last_run_valid_ = true;
            pix_host_helper_last_run_succeeded_ = false;
            pix_host_helper_last_exit_code_ = -1;
            pix_host_helper_last_session_dir_.clear();
            pix_host_helper_last_stdout_summary_.clear();
            pix_host_helper_last_stderr_summary_ =
                "tools/launch-pix-host-helper.ps1 not found when walking parents of cwd";
            log_.log(mirakana::LogLevel::warn, "editor", pix_host_helper_last_stderr_summary_);
            return;
        }

        std::vector<std::string> arguments = {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                                              "tools/launch-pix-host-helper.ps1"};
        if (skip_launch) {
            arguments.push_back("-SkipLaunch");
        }
        mirakana::ProcessCommand command{.executable = resolve_pwsh_executable_for_reviewed_host_scripts(),
                                         .arguments = std::move(arguments),
                                         .working_directory = root->generic_string()};
        try {
            mirakana::Win32ProcessRunner runner;
            const auto result = mirakana::run_process_command(runner, command);
            pix_host_helper_last_run_valid_ = true;
            pix_host_helper_last_run_succeeded_ = result.succeeded();
            pix_host_helper_last_exit_code_ = result.exit_code;
            pix_host_helper_last_stdout_summary_ = summarize_ai_process_text(result.stdout_text);
            pix_host_helper_last_stderr_summary_ = summarize_ai_process_text(result.stderr_text);
            if (!result.diagnostic.empty()) {
                pix_host_helper_last_stderr_summary_ =
                    summarize_ai_process_text(result.diagnostic + "\n" + pix_host_helper_last_stderr_summary_);
            }
            if (result.succeeded()) {
                const auto session = parse_pix_host_helper_session_dir(result.stdout_text);
                pix_host_helper_last_session_dir_ = session.value_or(std::string{});
            } else {
                pix_host_helper_last_session_dir_.clear();
            }
            log_.log(result.succeeded() ? mirakana::LogLevel::info : mirakana::LogLevel::warn, "editor",
                     std::string{"PIX host helper: "} + (result.succeeded() ? "ok" : "failed"));
        } catch (const std::exception& error) {
            pix_host_helper_last_run_valid_ = true;
            pix_host_helper_last_run_succeeded_ = false;
            pix_host_helper_last_exit_code_ = -1;
            pix_host_helper_last_session_dir_.clear();
            pix_host_helper_last_stdout_summary_.clear();
            pix_host_helper_last_stderr_summary_ = summarize_ai_process_text(error.what());
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
#else
        log_.log(mirakana::LogLevel::warn, "editor", "PIX host helper execution is host-gated on this editor platform");
#endif
    }

    [[nodiscard]] mirakana::editor::EditorAiPackageAuthoringDiagnosticsModel make_ai_package_diagnostics_model() const {
        const auto prefab_path = current_prefab_path();
        const auto package_candidates = mirakana::editor::make_scene_package_candidate_rows(
            scene_document_, current_cooked_scene_path(), current_package_index_path(),
            {std::string_view(prefab_path)});
        const auto package_registration_draft = mirakana::editor::make_scene_package_registration_draft_rows(
            package_candidates, project_.root_path, current_runtime_package_files());

        const std::vector<mirakana::editor::RuntimeSceneValidationTargetRow> validation_targets{
            mirakana::editor::RuntimeSceneValidationTargetRow{
                "current-scene-package",
                std::string(default_package_index_path),
                "scene.start",
                "",
                true,
                true,
            },
        };

        const bool package_index_registered =
            std::ranges::any_of(current_runtime_package_files(),
                                [](const std::string& file) { return file == default_package_index_path; });

        return mirakana::editor::make_editor_ai_package_authoring_diagnostics_model(
            mirakana::editor::EditorAiPackageAuthoringDiagnosticsDesc{
                mirakana::editor::EditorPlaytestPackageReviewDesc{
                    package_registration_draft,
                    validation_targets,
                    "current-scene-package",
                    {"desktop-runtime-sample-game-scene-gpu-package"},
                },
                {
                    mirakana::editor::EditorAiPackageDescriptorDiagnosticRow{
                        "runtime-scene-validation-target",
                        "game.agent.json.runtimeSceneValidationTargets",
                        mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
                        false,
                        false,
                        "current scene validation target is represented as reviewed data",
                    },
                    mirakana::editor::EditorAiPackageDescriptorDiagnosticRow{
                        "run-validation-recipe",
                        "aiOperableProductionLoop.commandSurfaces.run-validation-recipe",
                        mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
                        false,
                        false,
                        "reviewed validation recipe runner is shown as dry-run/operator data only",
                    },
                },
                {
                    mirakana::editor::EditorAiPackagePayloadDiagnosticRow{
                        "runtime-package-files",
                        package_index_registered ? mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready
                                                 : mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::blocked,
                        package_index_registered ? "runtime package index is registered"
                                                 : "runtime package index is not registered",
                    },
                },
                {
                    mirakana::editor::EditorAiPackageValidationRecipeDiagnosticRow{
                        "agent-contract",
                        mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
                        false,
                        false,
                        "agent contract dry-run command data is reviewed",
                    },
                    mirakana::editor::EditorAiPackageValidationRecipeDiagnosticRow{
                        "desktop-runtime-sample-game-scene-gpu-package",
                        mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
                        true,
                        false,
                        "desktop runtime package smoke stays host-gated and external",
                    },
                },
            });
    }

    [[nodiscard]] static std::vector<mirakana::editor::EditorAiValidationRecipeDryRunPlanRow>
    make_reviewed_validation_recipe_dry_run_rows() {
        return {
            mirakana::editor::EditorAiValidationRecipeDryRunPlanRow{
                "agent-contract",
                "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe "
                "agent-contract",
                {},
                {},
                false,
                "agent contract dry-run plan is reviewed",
                {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "agent-contract"},
            },
            mirakana::editor::EditorAiValidationRecipeDryRunPlanRow{
                "desktop-runtime-sample-game-scene-gpu-package",
                "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe "
                "desktop-runtime-sample-game-scene-gpu-package -GameTarget sample_desktop_runtime_game",
                {"d3d12-windows-primary"},
                {},
                false,
                "D3D12 desktop package dry-run plan is host-gated",
                {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "desktop-runtime-sample-game-scene-gpu-package", "-GameTarget",
                 "sample_desktop_runtime_game"},
            },
        };
    }

    [[nodiscard]] static std::vector<std::string> make_ai_validation_recipe_ids() {
        return {
            "agent-contract",
            "desktop-runtime-sample-game-scene-gpu-package",
        };
    }

    struct AiCommandPanelContext {
        mirakana::editor::EditorAiPlaytestOperatorHandoffModel operator_handoff;
        mirakana::editor::EditorAiCommandPanelModel panel_model;
    };

    [[nodiscard]] mirakana::editor::EditorAiPlaytestEvidenceImportModel make_ai_evidence_import_model() const {
        mirakana::editor::EditorAiPlaytestEvidenceImportDesc desc;
        desc.text = ai_evidence_import_text_.data();
        desc.expected_recipe_ids = make_ai_validation_recipe_ids();
        return mirakana::editor::make_editor_ai_playtest_evidence_import_model(desc);
    }

    [[nodiscard]] AiCommandPanelContext make_ai_command_panel_context() const {
        const auto package_diagnostics = make_ai_package_diagnostics_model();
        const auto recipe_ids = make_ai_validation_recipe_ids();
        const auto validation_preflight = mirakana::editor::make_editor_ai_validation_recipe_preflight_model(
            mirakana::editor::EditorAiValidationRecipePreflightDesc{
                recipe_ids,
                recipe_ids,
                make_reviewed_validation_recipe_dry_run_rows(),
            });
        const auto readiness_report = mirakana::editor::make_editor_ai_playtest_readiness_report_model(
            mirakana::editor::EditorAiPlaytestReadinessReportDesc{package_diagnostics, validation_preflight});
        const auto operator_handoff = mirakana::editor::make_editor_ai_playtest_operator_handoff_model(
            mirakana::editor::EditorAiPlaytestOperatorHandoffDesc{readiness_report, validation_preflight});
        const auto evidence_summary = mirakana::editor::make_editor_ai_playtest_evidence_summary_model(
            mirakana::editor::EditorAiPlaytestEvidenceSummaryDesc{operator_handoff, ai_playtest_evidence_rows_});
        const auto remediation_queue = mirakana::editor::make_editor_ai_playtest_remediation_queue_model(
            mirakana::editor::EditorAiPlaytestRemediationQueueDesc{evidence_summary});
        const auto remediation_handoff = mirakana::editor::make_editor_ai_playtest_remediation_handoff_model(
            mirakana::editor::EditorAiPlaytestRemediationHandoffDesc{remediation_queue});
        const auto workflow_report = mirakana::editor::make_editor_ai_playtest_operator_workflow_report_model(
            mirakana::editor::EditorAiPlaytestOperatorWorkflowReportDesc{
                package_diagnostics,
                validation_preflight,
                readiness_report,
                operator_handoff,
                evidence_summary,
                remediation_queue,
                remediation_handoff,
            });
        auto panel_model = mirakana::editor::make_editor_ai_command_panel_model(
            mirakana::editor::EditorAiCommandPanelDesc{workflow_report, operator_handoff, evidence_summary});
        return AiCommandPanelContext{
            operator_handoff,
            std::move(panel_model),
        };
    }

    static std::string join_display_values(std::span<const std::string> values) {
        if (values.empty()) {
            return "-";
        }
        std::string text;
        for (const auto& value : values) {
            if (!text.empty()) {
                text += ", ";
            }
            text += value;
        }
        return text;
    }

    static void draw_resource_rows_table(const char* table_id,
                                         std::span<const mirakana::editor::EditorResourceRow> rows) {
        if (rows.empty()) {
            ImGui::TextUnformatted("No diagnostics");
            return;
        }

        if (ImGui::BeginTable(table_id, 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Value");
            ImGui::TableSetupColumn("Status");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.available ? "Available" : "Unavailable");
            }
            ImGui::EndTable();
        }
    }

    [[nodiscard]] bool resource_capture_request_acknowledged(std::string_view id) const {
        return std::ranges::find(resource_acknowledged_capture_request_ids_, id) !=
               resource_acknowledged_capture_request_ids_.end();
    }

    void set_resource_capture_request_acknowledged(std::string_view id, bool acknowledged) {
        const auto it = std::ranges::find(resource_acknowledged_capture_request_ids_, id);
        if (acknowledged) {
            if (it == resource_acknowledged_capture_request_ids_.end()) {
                resource_acknowledged_capture_request_ids_.push_back(std::string(id));
            }
        } else if (it != resource_acknowledged_capture_request_ids_.end()) {
            resource_acknowledged_capture_request_ids_.erase(it);
        }
    }

    void
    draw_resource_capture_request_rows_table(std::span<const mirakana::editor::EditorResourceCaptureRequestRow> rows) {
        if (rows.empty()) {
            ImGui::TextUnformatted("No capture request rows");
            return;
        }

        if (ImGui::BeginTable("Resource Capture Requests", 7,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Request");
            ImGui::TableSetupColumn("Tool");
            ImGui::TableSetupColumn("Host Gates");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Acknowledgement");
            ImGui::TableSetupColumn("Diagnostic");
            ImGui::TableSetupColumn("Action");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.tool_label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.host_gates_label.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.status_label.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(row.acknowledgement_label.c_str());
                ImGui::TableSetColumnIndex(5);
                ImGui::TextWrapped("%s", row.diagnostic.c_str());
                ImGui::TableSetColumnIndex(6);
                if (row.request_available && !row.request_acknowledged) {
                    const auto button_id = row.action_label + "##resource_capture." + row.id;
                    if (ImGui::Button(button_id.c_str())) {
                        set_resource_capture_request_acknowledged(row.id, true);
                        log_.log(mirakana::LogLevel::info, "editor",
                                 row.label + " reviewed capture request acknowledged for external host workflow");
                    }
                } else {
                    ImGui::TextUnformatted(row.request_acknowledged ? "Acknowledged" : "Unavailable");
                }
            }
            ImGui::EndTable();
        }
    }

    void draw_resource_capture_execution_rows_table(
        std::span<const mirakana::editor::EditorResourceCaptureExecutionRow> rows) {
        if (rows.empty()) {
            ImGui::TextUnformatted("No capture execution evidence rows");
            return;
        }

        imgui_text_unformatted(mirakana::editor::editor_resources_capture_execution_contract_v1());
        imgui_text_unformatted(
            mirakana::editor::editor_resources_capture_operator_validated_launch_workflow_contract_v1());

#if defined(_WIN32)
        if (ImGui::BeginTable("Resource Capture Execution Evidence", 8,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Request");
            ImGui::TableSetupColumn("Tool");
            ImGui::TableSetupColumn("Phase");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Host Gates");
            ImGui::TableSetupColumn("Artifact");
            ImGui::TableSetupColumn("Diagnostic");
            ImGui::TableSetupColumn("Host helper");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.tool_label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.phase_code.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.status_label.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(row.host_gates_label.c_str());
                ImGui::TableSetColumnIndex(5);
                ImGui::TextUnformatted(row.artifact_path.c_str());
                ImGui::TableSetColumnIndex(6);
                ImGui::TextWrapped("%s", row.diagnostic.c_str());
                ImGui::TableSetColumnIndex(7);
                if (row.id == "pix_gpu_capture" && resource_capture_request_acknowledged("pix_gpu_capture")) {
                    ImGui::PushID("pix_capture_host_helper");
                    const std::string skip_button_label =
                        std::string{"Run helper (-SkipLaunch)##resource_capture_exec."} + row.id;
                    if (ImGui::Button(skip_button_label.c_str())) {
                        execute_pix_host_helper_reviewed(true);
                    }
                    ImGui::Spacing();
                    const std::string launch_button_label =
                        std::string{"Run helper (launch PIX)##resource_capture_exec."} + row.id;
                    if (ImGui::Button(launch_button_label.c_str())) {
                        ImGui::OpenPopup("pix_host_helper_launch_confirm");
                    }
                    if (ImGui::BeginPopupModal("pix_host_helper_launch_confirm", nullptr,
                                               ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::TextUnformatted(
                            "Runs pwsh tools/launch-pix-host-helper.ps1 without -SkipLaunch from the reviewed "
                            "repository root.");
                        ImGui::TextUnformatted(
                            "When PIX is installed, the Microsoft PIX UI process may start. Confirm only on your "
                            "workstation.");
                        if (ImGui::Button("Cancel", ImVec2(120.0F, 0.0F))) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Confirm launch", ImVec2(160.0F, 0.0F))) {
                            execute_pix_host_helper_reviewed(false);
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();
                } else {
                    ImGui::TextUnformatted("-");
                }
            }
            ImGui::EndTable();
        }
#else
        if (ImGui::BeginTable("Resource Capture Execution Evidence", 7,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Request");
            ImGui::TableSetupColumn("Tool");
            ImGui::TableSetupColumn("Phase");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Host Gates");
            ImGui::TableSetupColumn("Artifact");
            ImGui::TableSetupColumn("Diagnostic");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.tool_label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.phase_code.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.status_label.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(row.host_gates_label.c_str());
                ImGui::TableSetColumnIndex(5);
                ImGui::TextUnformatted(row.artifact_path.c_str());
                ImGui::TableSetColumnIndex(6);
                ImGui::TextWrapped("%s", row.diagnostic.c_str());
            }
            ImGui::EndTable();
        }
#endif
    }

    void draw_resources_panel() {
        const auto model = mirakana::editor::make_editor_resource_panel_model(make_resource_panel_input());

        ImGui::Begin("Resources");
        ImGui::TextUnformatted(model.status.c_str());
        if (!model.device_available) {
            ImGui::TextUnformatted("No RHI device diagnostics available");
        }

        ImGui::SeparatorText("Status");
        draw_resource_rows_table("Resource Status", model.status_rows);
        ImGui::SeparatorText("Counters");
        draw_resource_rows_table("Resource Counters", model.counter_rows);
        ImGui::SeparatorText("Memory");
        draw_resource_rows_table("Resource Memory", model.memory_rows);
        ImGui::SeparatorText("Lifetime");
        draw_resource_rows_table("Resource Lifetime", model.lifetime_rows);
        ImGui::SeparatorText("Capture Requests");
        draw_resource_capture_request_rows_table(model.capture_request_rows);
        ImGui::SeparatorText("Capture Execution Evidence");
        draw_resource_capture_execution_rows_table(model.capture_execution_rows);
        ImGui::End();
    }

    static void draw_ai_command_stage_table(std::span<const mirakana::editor::EditorAiCommandPanelStageRow> rows) {
        if (rows.empty()) {
            ImGui::TextUnformatted("No workflow stages");
            return;
        }
        if (ImGui::BeginTable("AI Command Stages", 5,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Stage");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Rows");
            ImGui::TableSetupColumn("Host Gates");
            ImGui::TableSetupColumn("Diagnostic");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.id.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.status_label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%zu", row.source_row_count);
                ImGui::TableSetColumnIndex(3);
                const auto host_gates = join_display_values(row.host_gates);
                ImGui::TextUnformatted(host_gates.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(row.diagnostic.c_str());
            }
            ImGui::EndTable();
        }
    }

    static void draw_ai_command_rows_table(std::span<const mirakana::editor::EditorAiCommandPanelCommandRow> rows) {
        if (rows.empty()) {
            ImGui::TextUnformatted("No reviewed command rows");
            return;
        }
        if (ImGui::BeginTable("AI Command Rows", 5,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Recipe");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Command");
            ImGui::TableSetupColumn("Host Gates");
            ImGui::TableSetupColumn("Blocked By");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.recipe_id.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.status_label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.command_display.c_str());
                ImGui::TableSetColumnIndex(3);
                const auto host_gates = join_display_values(row.host_gates);
                ImGui::TextUnformatted(host_gates.c_str());
                ImGui::TableSetColumnIndex(4);
                const auto blocked_by = join_display_values(row.blocked_by);
                ImGui::TextUnformatted(blocked_by.c_str());
            }
            ImGui::EndTable();
        }
    }

    static void draw_ai_evidence_rows_table(std::span<const mirakana::editor::EditorAiCommandPanelEvidenceRow> rows) {
        if (rows.empty()) {
            ImGui::TextUnformatted("No external evidence rows");
            return;
        }
        if (ImGui::BeginTable("AI Evidence Rows", 5,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Recipe");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Exit");
            ImGui::TableSetupColumn("Summary");
            ImGui::TableSetupColumn("Blocked By");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.recipe_id.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.status_label.c_str());
                ImGui::TableSetColumnIndex(2);
                if (row.exit_code.has_value()) {
                    ImGui::Text("%d", *row.exit_code);
                } else {
                    ImGui::TextUnformatted("-");
                }
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.summary.c_str());
                ImGui::TableSetColumnIndex(4);
                const auto blocked_by = join_display_values(row.blocked_by);
                ImGui::TextUnformatted(blocked_by.c_str());
            }
            ImGui::EndTable();
        }
    }

    static void draw_ai_evidence_import_review_table(
        std::span<const mirakana::editor::EditorAiPlaytestEvidenceImportReviewRow> rows) {
        if (rows.empty()) {
            ImGui::TextUnformatted("No evidence import review rows");
            return;
        }

        if (ImGui::BeginTable("AI Evidence Import Review", 5,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Recipe");
            ImGui::TableSetupColumn("Import");
            ImGui::TableSetupColumn("Evidence");
            ImGui::TableSetupColumn("Summary");
            ImGui::TableSetupColumn("Diagnostic");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.recipe_id.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.status_label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.evidence_status_label.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.summary.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(row.diagnostic.c_str());
            }
            ImGui::EndTable();
        }
    }

    [[nodiscard]] mirakana::editor::EditorAiReviewedValidationExecutionModel
    make_ai_reviewed_execution_plan(const mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow& row) const {
        mirakana::editor::EditorAiReviewedValidationExecutionDesc desc;
        desc.command_row = row;
        desc.working_directory = std::filesystem::current_path().generic_string();
        desc.acknowledge_host_gates = ai_host_gate_acknowledged(row.recipe_id);
        if (desc.acknowledge_host_gates) {
            desc.acknowledged_host_gates = row.host_gates;
        }
        return mirakana::editor::make_editor_ai_reviewed_validation_execution_plan(desc);
    }

    [[nodiscard]] mirakana::editor::EditorAiReviewedValidationExecutionBatchModel make_ai_reviewed_execution_batch(
        std::span<const mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow> rows) const {
        mirakana::editor::EditorAiReviewedValidationExecutionBatchDesc desc;
        desc.command_rows.assign(rows.begin(), rows.end());
        desc.working_directory = std::filesystem::current_path().generic_string();
        desc.acknowledged_host_gate_recipe_ids = ai_acknowledged_host_gate_recipe_ids_;
        return mirakana::editor::make_editor_ai_reviewed_validation_execution_batch(desc);
    }

    [[nodiscard]] mirakana::editor::EditorRuntimeScenePackageValidationExecutionModel
    make_runtime_scene_package_validation_execution_model() const {
        mirakana::editor::EditorRuntimeScenePackageValidationExecutionDesc desc;
        desc.playtest_review = make_ai_package_diagnostics_model().playtest_review;
        return mirakana::editor::make_editor_runtime_scene_package_validation_execution_model(desc);
    }

    void execute_runtime_scene_package_validation(
        const mirakana::editor::EditorRuntimeScenePackageValidationExecutionModel& model) {
        runtime_scene_package_validation_result_ =
            mirakana::editor::execute_editor_runtime_scene_package_validation(tool_filesystem_, model);
        const auto& result = *runtime_scene_package_validation_result_;
        log_.log(result.status == mirakana::editor::EditorRuntimeScenePackageValidationExecutionStatus::passed
                     ? mirakana::LogLevel::info
                     : mirakana::LogLevel::warn,
                 "editor", "Runtime scene package validation: " + result.status_label);
    }

    void draw_runtime_scene_package_validation_controls() {
        const auto model = make_runtime_scene_package_validation_execution_model();

        ImGui::Text("Status: %s", model.status_label.c_str());
        ImGui::SameLine();
        ImGui::Text("Package: %s",
                    model.request.package_index_path.empty() ? "-" : model.request.package_index_path.c_str());
        if (!model.can_execute) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Validate Runtime Scene Package")) {
            execute_runtime_scene_package_validation(model);
        }
        if (!model.can_execute) {
            ImGui::EndDisabled();
        }

        if (!model.blocked_by.empty()) {
            const auto blockers = join_display_values(model.blocked_by);
            ImGui::TextWrapped("Blocked: %s", blockers.c_str());
        }
        if (!model.diagnostics.empty()) {
            const auto diagnostics = join_display_values(model.diagnostics);
            ImGui::TextWrapped("Diagnostic: %s", diagnostics.c_str());
        }
        if (runtime_scene_package_validation_result_.has_value()) {
            const auto& result = *runtime_scene_package_validation_result_;
            ImGui::Text("Last: %s  records %llu  nodes %u  refs %zu", result.status_label.c_str(),
                        static_cast<unsigned long long>(result.model.package_record_count),
                        result.model.scene_node_count, result.model.reference_count);
            if (!result.validation.diagnostics.empty()) {
                const auto& first = result.validation.diagnostics.front();
                ImGui::TextWrapped("Validation: %s", first.message.c_str());
            }
        }
    }

    [[nodiscard]] mirakana::editor::EditorGameModuleDriverLoadDesc make_game_module_driver_load_desc() const {
        mirakana::editor::EditorGameModuleDriverLoadDesc desc;
        desc.id = "reviewed_driver";
        desc.label = "Reviewed Game Module Driver";
        desc.module_path = game_module_driver_path_.data();
        desc.factory_symbol = std::string(mirakana::editor::editor_game_module_driver_factory_symbol_v1);
        desc.play_session_active = play_session_.active();
        desc.driver_already_loaded = game_module_driver_ != nullptr;
        return desc;
    }

    [[nodiscard]] mirakana::editor::EditorGameModuleDriverLoadModel make_game_module_driver_load_model() const {
        return mirakana::editor::make_editor_game_module_driver_load_model(make_game_module_driver_load_desc());
    }

    [[nodiscard]] mirakana::editor::EditorGameModuleDriverReloadDesc make_game_module_driver_reload_desc() const {
        mirakana::editor::EditorGameModuleDriverReloadDesc desc;
        desc.id = "reviewed_driver";
        desc.label = "Reviewed Game Module Driver Reload";
        desc.module_path = game_module_driver_path_.data();
        desc.factory_symbol = std::string(mirakana::editor::editor_game_module_driver_factory_symbol_v1);
        desc.driver_loaded = game_module_driver_ != nullptr;
        desc.play_session_active = play_session_.active();
        return desc;
    }

    [[nodiscard]] mirakana::editor::EditorGameModuleDriverReloadModel make_game_module_driver_reload_model() const {
        return mirakana::editor::make_editor_game_module_driver_reload_model(make_game_module_driver_reload_desc());
    }

    [[nodiscard]] mirakana::editor::EditorGameModuleDriverUnloadDesc make_game_module_driver_unload_desc() const {
        mirakana::editor::EditorGameModuleDriverUnloadDesc desc;
        desc.id = "reviewed_driver";
        desc.label = "Reviewed Game Module Driver Unload";
        desc.driver_loaded = game_module_driver_ != nullptr;
        desc.play_session_active = play_session_.active();
        return desc;
    }

    [[nodiscard]] mirakana::editor::EditorGameModuleDriverUnloadModel make_game_module_driver_unload_model() const {
        return mirakana::editor::make_editor_game_module_driver_unload_model(make_game_module_driver_unload_desc());
    }

    void unload_game_module_driver() {
        if (play_session_.active()) {
            game_module_driver_status_ = "active";
            game_module_driver_diagnostic_ = "cannot unload a game module driver while Play-In-Editor is active";
            log_.log(mirakana::LogLevel::warn, "editor", game_module_driver_diagnostic_);
            return;
        }

        game_module_driver_.reset();
        game_module_driver_library_.reset();
        game_module_driver_status_ = "not_loaded";
        game_module_driver_diagnostic_ = "game module driver unloaded";
        log_.log(mirakana::LogLevel::info, "editor", game_module_driver_diagnostic_);
    }

    void load_game_module_driver_from_review(const mirakana::editor::EditorGameModuleDriverLoadModel& model,
                                             std::string success_status, std::string success_diagnostic) {
        game_module_driver_.reset();
        game_module_driver_library_.reset();

        auto load = mirakana::load_dynamic_library(std::filesystem::path{game_module_driver_path_.data()});
        if (load.status != mirakana::DynamicLibraryLoadStatus::loaded || !load.library.loaded()) {
            game_module_driver_status_ = "load_failed";
            game_module_driver_diagnostic_ =
                load.diagnostic.empty() ? "game module driver module did not load" : load.diagnostic;
            log_.log(mirakana::LogLevel::warn, "editor", game_module_driver_diagnostic_);
            return;
        }

        const auto symbol = mirakana::resolve_dynamic_library_symbol(load.library, model.factory_symbol);
        if (symbol.status != mirakana::DynamicLibrarySymbolStatus::resolved || symbol.address == nullptr) {
            game_module_driver_status_ = "symbol_failed";
            game_module_driver_diagnostic_ =
                symbol.diagnostic.empty() ? "game module driver factory symbol did not resolve" : symbol.diagnostic;
            log_.log(mirakana::LogLevel::warn, "editor", game_module_driver_diagnostic_);
            return;
        }

        const auto factory = reinterpret_cast<mirakana::editor::EditorGameModuleDriverFactoryFn>(symbol.address);
        auto created = mirakana::editor::make_editor_game_module_driver_from_symbol(factory);
        if (created.status != mirakana::editor::EditorGameModuleDriverStatus::ready || created.driver == nullptr) {
            game_module_driver_status_ = "create_failed";
            game_module_driver_diagnostic_ = !created.diagnostics.empty() ? join_display_values(created.diagnostics)
                                                                          : join_display_values(created.blocked_by);
            log_.log(mirakana::LogLevel::warn, "editor", game_module_driver_diagnostic_);
            return;
        }

        game_module_driver_ = std::move(created.driver);
        game_module_driver_library_.emplace(std::move(load.library));
        game_module_driver_status_ = std::move(success_status);
        game_module_driver_diagnostic_ = std::move(success_diagnostic);
        log_.log(mirakana::LogLevel::info, "editor", game_module_driver_diagnostic_);
    }

    void load_game_module_driver() {
        const auto model = make_game_module_driver_load_model();
        if (!model.can_load) {
            game_module_driver_status_ = model.status_label;
            game_module_driver_diagnostic_ = join_display_values(model.diagnostics);
            log_.log(mirakana::LogLevel::warn, "editor", game_module_driver_diagnostic_);
            return;
        }

        load_game_module_driver_from_review(model, "loaded", "game module driver loaded");
    }

    void reload_game_module_driver() {
        const auto reload_model = make_game_module_driver_reload_model();
        if (!reload_model.can_reload) {
            game_module_driver_status_ = reload_model.status_label;
            game_module_driver_diagnostic_ = join_display_values(reload_model.diagnostics);
            log_.log(mirakana::LogLevel::warn, "editor", game_module_driver_diagnostic_);
            return;
        }

        load_game_module_driver_from_review(make_game_module_driver_load_model(), "reloaded",
                                            "game module driver reloaded");
    }

    void draw_game_module_driver_controls() {
        const auto model = make_game_module_driver_load_model();
        const auto reload_model = make_game_module_driver_reload_model();
        const auto unload_model = make_game_module_driver_unload_model();
        const auto contract_model = mirakana::editor::make_editor_game_module_driver_contract_metadata_model();
        ImGui::SeparatorText("Game Module Driver");
        ImGui::InputText("Module Path", game_module_driver_path_.data(), game_module_driver_path_.size());
        ImGui::Text("Status: %s", game_module_driver_status_.c_str());
        ImGui::SameLine();
        ImGui::Text("Review: %s / Reload: %s / Unload: %s", model.status_label.c_str(),
                    reload_model.status_label.c_str(), unload_model.status_label.c_str());
        const auto session_snapshot = mirakana::editor::make_editor_game_module_driver_host_session_snapshot(
            play_session_.active(), game_module_driver_ != nullptr);
        ImGui::TextDisabled("Host session phase: %s", session_snapshot.phase_id.c_str());
        ImGui::TextWrapped("%s", session_snapshot.summary.c_str());
        ImGui::TextDisabled("DLL surface mutation barrier: %s",
                            session_snapshot.barrier_play_dll_surface_mutation_status.c_str());
        ImGui::TextDisabled("Active-session hot reload policy: %s",
                            session_snapshot.policy_active_session_hot_reload.c_str());
        ImGui::TextDisabled("Stopped-state reload scope: %s",
                            session_snapshot.policy_stopped_state_reload_scope.c_str());
        ImGui::TextDisabled("DLL mutation order guidance: %s",
                            session_snapshot.policy_dll_mutation_order_guidance.c_str());
        ImGui::TextWrapped("Contract: %s v%u", contract_model.abi_contract.c_str(), contract_model.abi_version);
        ImGui::TextWrapped("Factory: %s", contract_model.factory_symbol.c_str());
        ImGui::TextWrapped("Compatibility: same-engine-build only; stable third-party ABI and hot reload unsupported");
        const auto ctest_probe_evidence = mirakana::editor::make_editor_game_module_driver_ctest_probe_evidence_model();
        ImGui::Spacing();
        ImGui::TextDisabled("CTest dynamic-library probe evidence (reviewed labels; editor does not run CTest here)");
        ImGui::TextDisabled("Targets: MK_editor_game_module_driver_probe, MK_editor_game_module_driver_load_tests");
        ImGui::TextWrapped("CTest probe DLL CMake target: %s",
                           ctest_probe_evidence.probe_shared_library_target.c_str());
        ImGui::TextWrapped("CTest executable target: %s", ctest_probe_evidence.ctest_executable_target.c_str());
        ImGui::TextWrapped("Factory symbol: %s", ctest_probe_evidence.factory_symbol.c_str());
        ImGui::TextWrapped("%s", ctest_probe_evidence.host_scope_note.c_str());
        ImGui::TextWrapped("%s", ctest_probe_evidence.editor_boundary_note.c_str());

        const auto reload_transaction_recipe_evidence =
            mirakana::editor::make_editor_game_module_driver_reload_transaction_recipe_evidence_model();
        ImGui::Spacing();
        ImGui::TextDisabled(
            "Reload transaction validation recipe evidence (reviewed commands; editor does not run the recipe here)");
        // Retained agent needle (check-ai-integration):
        // ge.editor.editor_game_module_driver_reload_transaction_recipe_evidence.v1
        imgui_text_unformatted(
            mirakana::editor::editor_game_module_driver_reload_transaction_recipe_evidence_contract_v1());
        // Retained agent needle (check-ai-integration): dev-windows-editor-game-module-driver-load-tests (reviewed argv
        // only)
        ImGui::TextWrapped("Validation recipe: %s", reload_transaction_recipe_evidence.validation_recipe_id.c_str());
        ImGui::TextWrapped("Host gate acknowledgement: %s",
                           reload_transaction_recipe_evidence.host_gate_acknowledgement_id.c_str());
        ImGui::TextWrapped("Reviewed DryRun command: %s",
                           reload_transaction_recipe_evidence.reviewed_dry_run_command.c_str());
        ImGui::TextWrapped("Reviewed Execute command: %s",
                           reload_transaction_recipe_evidence.reviewed_execute_command.c_str());
        ImGui::TextWrapped("%s", reload_transaction_recipe_evidence.editor_boundary_note.c_str());

        const bool load_enabled = model.can_load;
        if (!load_enabled) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Load Game Module Driver")) {
            load_game_module_driver();
        }
        if (!load_enabled) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        if (!reload_model.can_reload) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Reload Game Module Driver")) {
            reload_game_module_driver();
        }
        if (!reload_model.can_reload) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        const bool unload_enabled = unload_model.can_unload;
        if (!unload_enabled) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Unload Game Module Driver")) {
            unload_game_module_driver();
        }
        if (!unload_enabled) {
            ImGui::EndDisabled();
        }

        if (!model.diagnostics.empty()) {
            const auto diagnostics = join_display_values(model.diagnostics);
            ImGui::TextWrapped("Diagnostic: %s", diagnostics.c_str());
        }
        if (!reload_model.diagnostics.empty()) {
            const auto diagnostics = join_display_values(reload_model.diagnostics);
            ImGui::TextWrapped("Reload Diagnostic: %s", diagnostics.c_str());
        }
        if (!model.unsupported_claims.empty()) {
            const auto unsupported = join_display_values(model.unsupported_claims);
            ImGui::TextWrapped("Unsupported: %s", unsupported.c_str());
        }
        if (!reload_model.unsupported_claims.empty()) {
            const auto unsupported = join_display_values(reload_model.unsupported_claims);
            ImGui::TextWrapped("Reload Unsupported: %s", unsupported.c_str());
        }
        if (!unload_model.diagnostics.empty()) {
            const auto diagnostics = join_display_values(unload_model.diagnostics);
            ImGui::TextWrapped("Unload Diagnostic: %s", diagnostics.c_str());
        }
        if (!game_module_driver_diagnostic_.empty()) {
            ImGui::TextWrapped("Last: %s", game_module_driver_diagnostic_.c_str());
        }
    }

    [[nodiscard]] bool ai_host_gate_acknowledged(std::string_view recipe_id) const {
        return std::ranges::find(ai_acknowledged_host_gate_recipe_ids_, recipe_id) !=
               ai_acknowledged_host_gate_recipe_ids_.end();
    }

    void set_ai_host_gate_acknowledged(std::string_view recipe_id, bool acknowledged) {
        const auto it = std::ranges::find(ai_acknowledged_host_gate_recipe_ids_, recipe_id);
        if (acknowledged) {
            if (it == ai_acknowledged_host_gate_recipe_ids_.end()) {
                ai_acknowledged_host_gate_recipe_ids_.push_back(std::string(recipe_id));
            }
        } else if (it != ai_acknowledged_host_gate_recipe_ids_.end()) {
            ai_acknowledged_host_gate_recipe_ids_.erase(it);
        }
    }

    [[nodiscard]] static std::string summarize_ai_process_text(std::string_view text) {
        std::string summary;
        summary.reserve(std::min<std::size_t>(text.size(), 512U));
        for (const auto character : text) {
            if (character == '\r' || character == '\n' || character == '\t') {
                if (!summary.empty() && summary.back() != ' ') {
                    summary.push_back(' ');
                }
            } else {
                summary.push_back(character);
            }
            if (summary.size() >= 512U) {
                summary += "...";
                break;
            }
        }
        return summary;
    }

    [[nodiscard]] mirakana::editor::EditorRuntimeHostPlaytestLaunchModel
    make_runtime_host_playtest_launch_model() const {
        mirakana::editor::EditorRuntimeHostPlaytestLaunchDesc desc;
        desc.id = "sample_desktop_runtime_game";
        desc.label = "Sample Desktop Runtime Game";
        desc.working_directory = std::filesystem::current_path().generic_string();
        desc.argv = {
            "out/install/desktop-runtime-release/bin/sample_desktop_runtime_game.exe",
            "--smoke",
            "--require-config",
            "runtime/sample_desktop_runtime_game.config",
            "--require-scene-package",
            "runtime/sample_desktop_runtime_game.geindex",
            "--require-d3d12-scene-shaders",
            "--video-driver",
            "windows",
            "--require-d3d12-renderer",
            "--require-scene-gpu-bindings",
        };
        desc.host_gates = {"d3d12-windows-primary"};
        desc.acknowledge_host_gates = runtime_host_playtest_host_gate_acknowledged_;
        if (desc.acknowledge_host_gates) {
            desc.acknowledged_host_gates = desc.host_gates;
        }
        return mirakana::editor::make_editor_runtime_host_playtest_launch_model(desc);
    }

    void draw_in_process_runtime_host_controls() {
        const auto model = make_in_process_runtime_host_model();
        ImGui::SeparatorText("In-Process Runtime Host");
        ImGui::Text("Status: %s", model.status_label.c_str());
        ImGui::SameLine();
        ImGui::Text("Driver: %s", model.linked_gameplay_driver_available ? "linked" : "missing");
        if (!model.can_begin) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Begin In-Process Runtime Host")) {
            log_.log(mirakana::LogLevel::warn, "editor",
                     "No linked in-process gameplay driver is installed in the stock editor shell");
        }
        if (!model.can_begin) {
            ImGui::EndDisabled();
        }
        if (!model.diagnostics.empty()) {
            const auto diagnostics = join_display_values(model.diagnostics);
            ImGui::TextWrapped("Diagnostic: %s", diagnostics.c_str());
        }
        if (!model.unsupported_claims.empty()) {
            const auto unsupported = join_display_values(model.unsupported_claims);
            ImGui::TextWrapped("Unsupported: %s", unsupported.c_str());
        }
    }

    void apply_runtime_host_playtest_result(const mirakana::editor::EditorRuntimeHostPlaytestLaunchModel& model,
                                            const mirakana::ProcessResult& result) {
        if (!result.launched) {
            runtime_host_playtest_status_ = "blocked";
        } else if (result.succeeded()) {
            runtime_host_playtest_status_ = "passed";
        } else {
            runtime_host_playtest_status_ = "failed";
        }
        runtime_host_playtest_exit_code_ = result.exit_code;
        runtime_host_playtest_summary_ =
            result.launched ? "runtime host playtest executed by MK_editor" : "runtime host playtest launch failed";
        runtime_host_playtest_stdout_ = summarize_ai_process_text(result.stdout_text);
        runtime_host_playtest_stderr_ = summarize_ai_process_text(result.stderr_text);
        if (!result.diagnostic.empty()) {
            runtime_host_playtest_stderr_ = summarize_ai_process_text(result.diagnostic);
        }
        log_.log(result.succeeded() ? mirakana::LogLevel::info : mirakana::LogLevel::warn, "editor",
                 model.label + ": " + runtime_host_playtest_status_);
    }

    void execute_runtime_host_playtest_launch(const mirakana::editor::EditorRuntimeHostPlaytestLaunchModel& model) {
        if (!model.can_execute) {
            runtime_host_playtest_status_ = model.status_label;
            runtime_host_playtest_summary_ = "runtime host playtest launch is not ready";
            runtime_host_playtest_stderr_ = join_display_values(model.diagnostics);
            log_.log(mirakana::LogLevel::warn, "editor", runtime_host_playtest_summary_);
            return;
        }

#if defined(_WIN32)
        try {
            mirakana::Win32ProcessRunner runner;
            const auto result = mirakana::run_process_command(runner, model.command);
            apply_runtime_host_playtest_result(model, result);
        } catch (const std::exception& error) {
            mirakana::ProcessResult result;
            result.diagnostic = error.what();
            apply_runtime_host_playtest_result(model, result);
        }
#else
        mirakana::ProcessResult result;
        result.diagnostic = "runtime host playtest launch is host-gated on this editor platform";
        apply_runtime_host_playtest_result(model, result);
#endif
    }

    void draw_runtime_host_playtest_launch_controls() {
        const auto model = make_runtime_host_playtest_launch_model();
        ImGui::SeparatorText("Runtime Host Playtest");
        ImGui::Text("Status: %s", model.status_label.c_str());
        ImGui::SameLine();
        ImGui::Text("Evidence: %s", runtime_host_playtest_status_.c_str());
        if (model.host_gate_acknowledgement_required) {
            bool acknowledged = runtime_host_playtest_host_gate_acknowledged_;
            if (ImGui::Checkbox("Acknowledge D3D12 host gate", &acknowledged)) {
                runtime_host_playtest_host_gate_acknowledged_ = acknowledged;
            }
        }
        ImGui::SameLine();
        if (!model.can_execute) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Execute Runtime Host")) {
            execute_runtime_host_playtest_launch(model);
        }
        if (!model.can_execute) {
            ImGui::EndDisabled();
        }
        const auto command = model.command.executable.empty() ? std::string{"-"} : model.command.executable;
        ImGui::TextWrapped("Command: %s", command.c_str());
        if (!model.diagnostics.empty()) {
            const auto diagnostics = join_display_values(model.diagnostics);
            ImGui::TextWrapped("Diagnostic: %s", diagnostics.c_str());
        }
        if (!runtime_host_playtest_summary_.empty()) {
            ImGui::TextWrapped("Last: %s", runtime_host_playtest_summary_.c_str());
        }
        if (runtime_host_playtest_exit_code_.has_value()) {
            ImGui::Text("Exit: %d", *runtime_host_playtest_exit_code_);
        }
        if (!runtime_host_playtest_stderr_.empty()) {
            ImGui::TextWrapped("stderr: %s", runtime_host_playtest_stderr_.c_str());
        }
    }

    void upsert_ai_playtest_evidence_row(mirakana::editor::EditorAiPlaytestValidationEvidenceRow row) {
        const auto it = std::ranges::find_if(
            ai_playtest_evidence_rows_, [&row](const auto& existing) { return existing.recipe_id == row.recipe_id; });
        if (it == ai_playtest_evidence_rows_.end()) {
            ai_playtest_evidence_rows_.push_back(std::move(row));
        } else {
            *it = std::move(row);
        }
    }

    void apply_ai_validation_process_result(const mirakana::editor::EditorAiReviewedValidationExecutionModel& plan,
                                            const mirakana::ProcessResult& result) {
        mirakana::editor::EditorAiPlaytestValidationEvidenceRow row;
        row.recipe_id = plan.recipe_id;
        if (!result.launched) {
            row.status = mirakana::editor::EditorAiPlaytestEvidenceStatus::blocked;
        } else if (result.succeeded()) {
            row.status = mirakana::editor::EditorAiPlaytestEvidenceStatus::passed;
        } else {
            row.status = mirakana::editor::EditorAiPlaytestEvidenceStatus::failed;
        }
        row.exit_code = result.exit_code;
        row.summary = result.launched ? "reviewed validation recipe executed by MK_editor"
                                      : "reviewed validation recipe launch failed";
        row.stdout_summary = summarize_ai_process_text(result.stdout_text);
        row.stderr_summary = summarize_ai_process_text(result.stderr_text);
        row.blocked_by =
            result.launched ? std::vector<std::string>{} : std::vector<std::string>{"process-launch-failed"};
        if (!result.diagnostic.empty()) {
            row.stderr_summary = summarize_ai_process_text(result.diagnostic);
        }
        row.externally_supplied = true;
        row.claims_editor_core_execution = false;
        upsert_ai_playtest_evidence_row(std::move(row));
    }

    void execute_ai_reviewed_validation_recipe(const mirakana::editor::EditorAiReviewedValidationExecutionModel& plan) {
        if (!plan.can_execute) {
            log_.log(mirakana::LogLevel::warn, "editor", "AI validation recipe execution is not ready");
            return;
        }

#if defined(_WIN32)
        try {
            mirakana::Win32ProcessRunner runner;
            const auto result = mirakana::run_process_command(runner, plan.command);
            apply_ai_validation_process_result(plan, result);
            log_.log(result.succeeded() ? mirakana::LogLevel::info : mirakana::LogLevel::warn, "editor",
                     result.succeeded() ? "AI validation recipe execution passed"
                                        : "AI validation recipe execution failed");
        } catch (const std::exception& error) {
            mirakana::ProcessResult result;
            result.diagnostic = error.what();
            apply_ai_validation_process_result(plan, result);
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
#else
        mirakana::ProcessResult result;
        result.diagnostic = "AI validation recipe execution is host-gated on this editor platform";
        apply_ai_validation_process_result(plan, result);
        log_.log(mirakana::LogLevel::warn, "editor", result.diagnostic);
#endif
    }

    void
    execute_ai_reviewed_validation_batch(const mirakana::editor::EditorAiReviewedValidationExecutionBatchModel& batch) {
        if (!batch.can_execute_any) {
            log_.log(mirakana::LogLevel::warn, "editor", "AI validation batch has no executable reviewed rows");
            return;
        }

        for (const auto plan_index : batch.executable_plan_indexes) {
            if (plan_index < batch.plans.size()) {
                execute_ai_reviewed_validation_recipe(batch.plans[plan_index]);
            }
        }
    }

    void draw_ai_reviewed_execution_controls(
        std::span<const mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow> rows) {
        if (rows.empty()) {
            ImGui::TextUnformatted("No reviewed execution rows");
            return;
        }

        const auto batch = make_ai_reviewed_execution_batch(rows);
        ImGui::Text("Batch: ready %zu  host-gated %zu  blocked %zu  executable %zu", batch.ready_count,
                    batch.host_gated_count, batch.blocked_count, batch.commands.size());
        if (!batch.diagnostics.empty()) {
            const auto diagnostics = join_display_values(batch.diagnostics);
            ImGui::TextWrapped("%s", diagnostics.c_str());
        }
        if (batch.can_execute_any) {
            if (ImGui::Button("Execute Ready")) {
                execute_ai_reviewed_validation_batch(batch);
            }
        } else {
            ImGui::TextUnformatted("No executable reviewed rows");
        }

        if (ImGui::BeginTable("AI Reviewed Validation Execution", 6,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Recipe");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Command");
            ImGui::TableSetupColumn("Diagnostic");
            ImGui::TableSetupColumn("Ack");
            ImGui::TableSetupColumn("Run");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                const auto plan = make_ai_reviewed_execution_plan(row);
                ImGui::PushID(row.recipe_id.c_str());
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(plan.recipe_id.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(plan.status_label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(plan.command.executable.empty() ? "-" : plan.command.executable.c_str());
                ImGui::TableSetColumnIndex(3);
                const auto diagnostics = join_display_values(plan.diagnostics);
                ImGui::TextUnformatted(diagnostics.c_str());
                ImGui::TableSetColumnIndex(4);
                if (plan.host_gate_acknowledgement_required) {
                    bool acknowledged = ai_host_gate_acknowledged(row.recipe_id);
                    if (ImGui::Checkbox("Host gate", &acknowledged)) {
                        set_ai_host_gate_acknowledged(row.recipe_id, acknowledged);
                    }
                } else {
                    ImGui::TextUnformatted("-");
                }
                ImGui::TableSetColumnIndex(5);
                if (plan.can_execute) {
                    if (ImGui::Button("Execute")) {
                        execute_ai_reviewed_validation_recipe(plan);
                    }
                } else {
                    ImGui::TextUnformatted("-");
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    void draw_ai_commands_panel() {
        const auto context = make_ai_command_panel_context();
        const auto& model = context.panel_model;
        const auto evidence_import_model = make_ai_evidence_import_model();

        ImGui::Begin("AI Commands");
        ImGui::Text("Status: %s", model.status_label.c_str());
        ImGui::Text("Stages: ready %zu  host-gated %zu  blocked %zu  external %zu", model.ready_stage_count,
                    model.host_gated_stage_count, model.blocked_stage_count, model.external_stage_count);
        if (model.external_action_required) {
            ImGui::TextWrapped("External operator validation or evidence collection is required.");
        }
        if (model.has_blocking_diagnostics) {
            ImGui::TextWrapped("Blocking diagnostics are present; the panel remains read-only.");
        }

        ImGui::SeparatorText("Workflow Stages");
        draw_ai_command_stage_table(model.stage_rows);
        ImGui::SeparatorText("Reviewed Commands");
        draw_ai_command_rows_table(model.command_rows);
        ImGui::SeparatorText("Runtime Scene Package Validation");
        draw_runtime_scene_package_validation_controls();
        ImGui::SeparatorText("Reviewed Execution");
        draw_ai_reviewed_execution_controls(context.operator_handoff.command_rows);
        ImGui::SeparatorText("Evidence Import");
        ImGui::InputTextMultiline("External Evidence Text", ai_evidence_import_text_.data(),
                                  ai_evidence_import_text_.size(), ImVec2{0.0F, 96.0F});
        ImGui::Text("Import: %s  Imported: %zu  Missing: %zu  Blocked: %zu", evidence_import_model.status_label.c_str(),
                    evidence_import_model.imported_count, evidence_import_model.missing_expected_count,
                    evidence_import_model.blocked_count);
        if (ImGui::Button("Import Evidence") && evidence_import_model.importable) {
            ai_playtest_evidence_rows_ = evidence_import_model.evidence_rows;
            log_.log(mirakana::LogLevel::info, "editor", "AI validation evidence imported into editor session state");
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Imported Evidence")) {
            ai_playtest_evidence_rows_.clear();
            log_.log(mirakana::LogLevel::info, "editor", "AI validation evidence cleared from editor session state");
        }
        draw_ai_evidence_import_review_table(evidence_import_model.review_rows);
        ImGui::SeparatorText("External Evidence");
        draw_ai_evidence_rows_table(model.evidence_rows);

        if (!model.unsupported_claims.empty()) {
            ImGui::SeparatorText("Unsupported Claims");
            for (const auto& claim : model.unsupported_claims) {
                ImGui::BulletText("%s", claim.c_str());
            }
        }
        if (!model.diagnostics.empty()) {
            ImGui::SeparatorText("Diagnostics");
            for (const auto& diagnostic : model.diagnostics) {
                ImGui::TextWrapped("%s", diagnostic.c_str());
            }
        }
        ImGui::End();
    }

    static void draw_input_rebinding_binding_rows_table(
        std::span<const mirakana::editor::EditorInputRebindingProfileBindingRow> rows) {
        if (rows.empty()) {
            ImGui::TextUnformatted("No runtime input action bindings");
            return;
        }

        if (ImGui::BeginTable("Input Rebinding Bindings", 6,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Kind");
            ImGui::TableSetupColumn("Context");
            ImGui::TableSetupColumn("Action");
            ImGui::TableSetupColumn("Current");
            ImGui::TableSetupColumn("Profile");
            ImGui::TableSetupColumn("Status");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.kind_label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.context.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.action.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.current_binding.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(row.profile_binding.c_str());
                ImGui::TableSetColumnIndex(5);
                ImGui::TextUnformatted(row.status_label.c_str());
            }
            ImGui::EndTable();
        }
    }

    static void draw_input_rebinding_review_rows_table(
        std::span<const mirakana::editor::EditorInputRebindingProfileReviewRow> rows) {
        if (rows.empty()) {
            ImGui::TextUnformatted("No review rows");
            return;
        }

        if (ImGui::BeginTable("Input Rebinding Review", 4,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Id");
            ImGui::TableSetupColumn("Surface");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Diagnostic");
            ImGui::TableHeadersRow();
            for (const auto& row : rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.id.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.surface.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(
                    std::string(mirakana::editor::editor_input_rebinding_profile_review_status_label(row.status))
                        .c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.diagnostic.c_str());
            }
            ImGui::EndTable();
        }
    }

    void arm_input_rebinding_capture(std::string context, std::string action) {
        input_rebinding_capture_target_ = InputRebindingCaptureTarget{.kind = InputRebindingCaptureKind::action,
                                                                      .context = std::move(context),
                                                                      .action = std::move(action),
                                                                      .armed_frame = editor_input_frame_};
        input_rebinding_capture_status_ = "Capture armed; press a key, pointer, or gamepad button";
        input_rebinding_last_capture_.reset();
        input_rebinding_last_axis_capture_.reset();
    }

    void arm_input_rebinding_axis_capture(std::string context, std::string action) {
        input_rebinding_capture_target_ = InputRebindingCaptureTarget{.kind = InputRebindingCaptureKind::axis,
                                                                      .context = std::move(context),
                                                                      .action = std::move(action),
                                                                      .armed_frame = editor_input_frame_};
        input_rebinding_capture_status_ =
            "Axis capture armed: move a gamepad stick/trigger past the deadzone, or hold two keyboard keys "
            "simultaneously for a key-pair axis";
        input_rebinding_last_capture_.reset();
        input_rebinding_last_axis_capture_.reset();
    }

    void update_input_rebinding_capture() {
        if (!input_rebinding_capture_target_.has_value()) {
            return;
        }
        if (input_rebinding_capture_target_->armed_frame == editor_input_frame_) {
            return;
        }

        switch (input_rebinding_capture_target_->kind) {
        case InputRebindingCaptureKind::action: {
            mirakana::editor::EditorInputRebindingCaptureDesc desc;
            desc.base_actions = input_rebinding_base_actions_;
            desc.profile = input_rebinding_profile_;
            desc.context = input_rebinding_capture_target_->context;
            desc.action = input_rebinding_capture_target_->action;
            desc.state = mirakana::runtime::RuntimeInputStateView{&editor_keyboard_input_, &editor_pointer_input_,
                                                                  &editor_gamepad_input_};

            auto capture = mirakana::editor::make_editor_input_rebinding_capture_action_model(desc);
            input_rebinding_capture_status_ =
                "Capture " + capture.status_label + " for " + capture.context + "/" + capture.action;
            if (capture.status == mirakana::editor::EditorInputRebindingCaptureStatus::captured) {
                input_rebinding_profile_ = capture.candidate_profile;
                input_rebinding_capture_status_ += " -> " + capture.trigger_label;
                input_rebinding_capture_target_.reset();
            } else if (capture.status == mirakana::editor::EditorInputRebindingCaptureStatus::blocked) {
                input_rebinding_capture_target_.reset();
            }
            input_rebinding_last_capture_ = std::move(capture);
            break;
        }
        case InputRebindingCaptureKind::axis: {
            mirakana::editor::EditorInputRebindingAxisCaptureDesc desc;
            desc.base_actions = input_rebinding_base_actions_;
            desc.profile = input_rebinding_profile_;
            desc.context = input_rebinding_capture_target_->context;
            desc.action = input_rebinding_capture_target_->action;
            desc.state = mirakana::runtime::RuntimeInputStateView{&editor_keyboard_input_, &editor_pointer_input_,
                                                                  &editor_gamepad_input_};

            auto capture = mirakana::editor::make_editor_input_rebinding_capture_axis_model(desc);
            input_rebinding_capture_status_ =
                "Axis capture " + capture.status_label + " for " + capture.context + "/" + capture.action;
            if (capture.status == mirakana::editor::EditorInputRebindingCaptureStatus::captured) {
                input_rebinding_profile_ = capture.candidate_profile;
                input_rebinding_capture_status_ += " -> " + capture.axis_source_label;
                input_rebinding_capture_target_.reset();
            } else if (capture.status == mirakana::editor::EditorInputRebindingCaptureStatus::blocked) {
                input_rebinding_capture_target_.reset();
            }
            input_rebinding_last_axis_capture_ = std::move(capture);
            break;
        }
        }
    }

    void draw_input_rebinding_capture_controls() {
        ImGui::TextWrapped("%s", input_rebinding_capture_status_.c_str());

        if (input_rebinding_capture_target_.has_value()) {
            const char* const kind_label =
                input_rebinding_capture_target_->kind == InputRebindingCaptureKind::axis ? "axis" : "action";
            ImGui::Text("Target (%s): %s/%s", kind_label, input_rebinding_capture_target_->context.c_str(),
                        input_rebinding_capture_target_->action.c_str());
            if (ImGui::Button("Cancel Capture")) {
                input_rebinding_capture_target_.reset();
                input_rebinding_capture_status_ = "Capture cancelled";
            }
        }

        ImGui::SeparatorText("Action Capture");
        if (ImGui::BeginTable("Input Rebinding Action Capture", 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Context");
            ImGui::TableSetupColumn("Action");
            ImGui::TableSetupColumn("Capture");
            ImGui::TableHeadersRow();
            for (const auto& binding : input_rebinding_base_actions_.bindings()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(binding.context.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(binding.action.c_str());
                ImGui::TableSetColumnIndex(2);
                const auto disabled = input_rebinding_capture_target_.has_value();
                ImGui::BeginDisabled(disabled);
                const auto button_id = "Capture##" + binding.context + "." + binding.action;
                if (ImGui::Button(button_id.c_str())) {
                    arm_input_rebinding_capture(binding.context, binding.action);
                }
                ImGui::EndDisabled();
            }
            ImGui::EndTable();
        }

        if (input_rebinding_last_capture_.has_value() && !input_rebinding_last_capture_->diagnostics.empty()) {
            ImGui::SeparatorText("Action Capture Diagnostics");
            for (const auto& diagnostic : input_rebinding_last_capture_->diagnostics) {
                ImGui::TextWrapped("%s", diagnostic.c_str());
            }
        }

        ImGui::SeparatorText("Axis Capture (gamepad / keyboard key pair)");

        if (ImGui::BeginTable("Input Rebinding Axis Capture", 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Context");
            ImGui::TableSetupColumn("Action");
            ImGui::TableSetupColumn("Capture");
            ImGui::TableHeadersRow();
            for (const auto& binding : input_rebinding_base_actions_.axis_bindings()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(binding.context.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(binding.action.c_str());
                ImGui::TableSetColumnIndex(2);
                const auto disabled = input_rebinding_capture_target_.has_value();
                ImGui::BeginDisabled(disabled);
                const auto button_id = "Axis Capture##axis." + binding.context + "." + binding.action;
                if (ImGui::Button(button_id.c_str())) {
                    arm_input_rebinding_axis_capture(binding.context, binding.action);
                }
                ImGui::EndDisabled();
            }
            ImGui::EndTable();
        }

        if (input_rebinding_last_axis_capture_.has_value() &&
            !input_rebinding_last_axis_capture_->diagnostics.empty()) {
            ImGui::SeparatorText("Axis Capture Diagnostics");
            for (const auto& diagnostic : input_rebinding_last_axis_capture_->diagnostics) {
                ImGui::TextWrapped("%s", diagnostic.c_str());
            }
        }
        ImGui::TextWrapped(
            "Hold two keys simultaneously to assign the lexicographically first pair (by Key enum order) to "
            "negative/positive axis sources. Input glyphs, UI focus consumption, and multiplayer device assignment "
            "remain separate reviewed workflows; use Profile file Save/Load for explicit `.inputrebinding` "
            "persistence.");
    }

    [[nodiscard]] std::string trimmed_input_rebinding_profile_store_path() const {
        const std::size_t len =
            strnlen(input_rebinding_profile_store_path_.data(), input_rebinding_profile_store_path_.size());
        std::string_view view(input_rebinding_profile_store_path_.data(), len);
        while (!view.empty() && std::isspace(static_cast<unsigned char>(view.front())) != 0) {
            view.remove_prefix(1);
        }
        while (!view.empty() && std::isspace(static_cast<unsigned char>(view.back())) != 0) {
            view.remove_suffix(1);
        }
        return std::string(view);
    }

    void save_input_rebinding_profile_to_project_store_file() {
        input_rebinding_persistence_diagnostics_.clear();
        const std::string path = trimmed_input_rebinding_profile_store_path();
        const auto result = mirakana::editor::save_editor_input_rebinding_profile_to_project_store(
            project_store_, path, input_rebinding_base_actions_, input_rebinding_profile_);
        if (result.succeeded) {
            input_rebinding_persistence_status_ = "Saved profile to " + path;
        } else {
            input_rebinding_persistence_status_ = "Save failed";
            input_rebinding_persistence_diagnostics_ = {result.diagnostic};
        }
    }

    void load_input_rebinding_profile_from_project_store_file() {
        input_rebinding_persistence_diagnostics_.clear();
        const std::string path = trimmed_input_rebinding_profile_store_path();
        auto result = mirakana::editor::load_editor_input_rebinding_profile_from_project_store(
            project_store_, path, input_rebinding_base_actions_);
        if (result.succeeded) {
            input_rebinding_profile_ = std::move(result.profile);
            input_rebinding_capture_target_.reset();
            input_rebinding_last_capture_.reset();
            input_rebinding_last_axis_capture_.reset();
            input_rebinding_capture_status_ = "No capture armed";
            input_rebinding_persistence_status_ = "Loaded profile from " + path;
        } else {
            input_rebinding_persistence_status_ = "Load failed";
            input_rebinding_persistence_diagnostics_ = {result.diagnostic};
        }
    }

    void draw_input_rebinding_panel() {
        update_input_rebinding_capture();
        const auto model = mirakana::editor::make_editor_input_rebinding_profile_panel_model(
            mirakana::editor::EditorInputRebindingProfileReviewDesc{input_rebinding_base_actions_,
                                                                    input_rebinding_profile_});

        ImGui::Begin("Input Rebinding");
        ImGui::Text("Status: %s", model.status_label.c_str());
        ImGui::Text("Profile: %s", model.profile_id.c_str());
        ImGui::Text("Bindings: actions %zu  axes %zu  Overrides: actions %zu  axes %zu", model.action_binding_count,
                    model.axis_binding_count, model.action_override_count, model.axis_override_count);
        if (model.has_blocking_diagnostics) {
            ImGui::TextWrapped("Blocking diagnostics are present; this panel remains read-only.");
        }
        if (model.has_conflicts) {
            ImGui::TextWrapped("Runtime input conflicts must be resolved before saving the profile.");
        }

        ImGui::SeparatorText("Profile file (GameEngine.RuntimeInputRebindingProfile.v1)");
        ImGui::InputText("Project-relative path", input_rebinding_profile_store_path_.data(),
                         input_rebinding_profile_store_path_.size());
        {
            const auto disabled = input_rebinding_capture_target_.has_value();
            if (disabled) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Save profile")) {
                save_input_rebinding_profile_to_project_store_file();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load profile")) {
                load_input_rebinding_profile_from_project_store_file();
            }
            if (disabled) {
                ImGui::EndDisabled();
            }
        }
        ImGui::TextWrapped("File op: %s", input_rebinding_persistence_status_.c_str());
        for (const auto& line : input_rebinding_persistence_diagnostics_) {
            ImGui::TextWrapped("%s", line.c_str());
        }

        ImGui::SeparatorText("Bindings");
        draw_input_rebinding_binding_rows_table(model.binding_rows);
        draw_input_rebinding_capture_controls();
        ImGui::SeparatorText("Review");
        draw_input_rebinding_review_rows_table(model.review_rows);

        if (!model.unsupported_claims.empty()) {
            ImGui::SeparatorText("Unsupported Claims");
            for (const auto& claim : model.unsupported_claims) {
                ImGui::BulletText("%s", claim.c_str());
            }
        }
        if (!model.diagnostics.empty()) {
            ImGui::SeparatorText("Diagnostics");
            for (const auto& diagnostic : model.diagnostics) {
                ImGui::TextWrapped("%s", diagnostic.c_str());
            }
        }
        ImGui::End();
    }

    void draw_profiler_panel() {
        const auto capture = profiler_recorder_.snapshot();
        const auto model = mirakana::editor::make_editor_profiler_panel_model(capture, make_profiler_status());

        ImGui::Begin("Profiler");
        if (model.capture_empty) {
            ImGui::TextUnformatted("No diagnostic samples recorded");
        }

        if (ImGui::BeginTable("Profiler Status", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : model.status_rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginTable("Profiler Summary", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : model.summary_rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginTable("Profiler Trace Export", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : model.trace_export.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }
        if (model.trace_export.can_export) {
            ImGui::InputText("Trace Path", profiler_trace_export_path_.data(), profiler_trace_export_path_.size());
            if (ImGui::Button("Copy Trace JSON")) {
                ImGui::SetClipboardText(model.trace_export.payload.c_str());
                profiler_trace_export_status_ =
                    "Trace JSON copied (" + std::to_string(model.trace_export.payload_bytes) + " bytes)";
            }
            ImGui::SameLine();
            if (ImGui::Button("Save Trace JSON")) {
                save_profiler_trace_json_to_path(capture, std::string_view{profiler_trace_export_path_.data()});
            }
            ImGui::SameLine();
            if (ImGui::Button("Browse Save Trace JSON")) {
                show_profiler_trace_save_dialog();
            }
        } else {
            for (const auto& diagnostic : model.trace_export.diagnostics) {
                ImGui::TextWrapped("%s", diagnostic.c_str());
            }
        }
        if (!profiler_trace_save_dialog_.rows.empty() &&
            ImGui::BeginTable("Profiler Trace Save Dialog", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : profiler_trace_save_dialog_.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }
        for (const auto& diagnostic : profiler_trace_save_dialog_.diagnostics) {
            ImGui::TextWrapped("%s", diagnostic.c_str());
        }
        if (!profiler_trace_export_status_.empty()) {
            ImGui::TextUnformatted(profiler_trace_export_status_.c_str());
        }

        ImGui::SeparatorText("Trace JSON Review");
        ImGui::InputText("Trace Import Path", profiler_trace_import_path_.data(), profiler_trace_import_path_.size());
        ImGui::SameLine();
        if (ImGui::Button("Import Trace JSON")) {
            import_profiler_trace_json_from_path(std::string_view{profiler_trace_import_path_.data()});
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse Trace JSON")) {
            show_profiler_trace_open_dialog();
        }
        if (!profiler_trace_open_dialog_.rows.empty() &&
            ImGui::BeginTable("Profiler Trace Open Dialog", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : profiler_trace_open_dialog_.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }
        for (const auto& diagnostic : profiler_trace_open_dialog_.diagnostics) {
            ImGui::TextWrapped("%s", diagnostic.c_str());
        }
        if (!profiler_trace_file_import_.rows.empty() &&
            ImGui::BeginTable("Profiler Trace File Import", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : profiler_trace_file_import_.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }
        for (const auto& diagnostic : profiler_trace_file_import_.diagnostics) {
            ImGui::TextWrapped("%s", diagnostic.c_str());
        }
        ImGui::InputTextMultiline("Trace JSON", profiler_trace_import_payload_.data(),
                                  profiler_trace_import_payload_.size(), ImVec2{0.0F, 96.0F});
        if (ImGui::Button("Review Trace JSON")) {
            profiler_trace_import_review_ =
                mirakana::editor::make_editor_profiler_trace_import_review_model(profiler_trace_import_payload_.data());
            if (profiler_trace_import_review_.valid) {
                profiler_trace_import_status_ = "Trace JSON review ready (" +
                                                std::to_string(profiler_trace_import_review_.payload_bytes) + " bytes)";
            } else if (!profiler_trace_import_review_.diagnostics.empty()) {
                profiler_trace_import_status_ = profiler_trace_import_review_.diagnostics.front();
            } else {
                profiler_trace_import_status_ = profiler_trace_import_review_.status_label;
            }
        }
        if (!profiler_trace_import_status_.empty()) {
            ImGui::TextUnformatted(profiler_trace_import_status_.c_str());
        }
        if (ImGui::BeginTable("Profiler Trace Import Review", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : profiler_trace_import_review_.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }
        for (const auto& diagnostic : profiler_trace_import_review_.diagnostics) {
            ImGui::TextWrapped("%s", diagnostic.c_str());
        }

        if (ImGui::BeginTable("Profiler Telemetry Handoff", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : model.telemetry.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }
        for (const auto& diagnostic : model.telemetry.diagnostics) {
            ImGui::TextWrapped("%s", diagnostic.c_str());
        }

        ImGui::TextUnformatted(model.trace_status.c_str());

        if (ImGui::BeginTable("Profiler Profiles", 4,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Frame");
            ImGui::TableSetupColumn("Depth");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Duration");
            ImGui::TableHeadersRow();
            for (const auto& row : model.profile_rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%llu", static_cast<unsigned long long>(row.frame_index));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", row.depth);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.name.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.duration.c_str());
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginTable("Profiler Counters", 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Frame");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : model.counter_rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%llu", static_cast<unsigned long long>(row.frame_index));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.name.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginTable("Profiler Events", 4,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Frame");
            ImGui::TableSetupColumn("Severity");
            ImGui::TableSetupColumn("Category");
            ImGui::TableSetupColumn("Message");
            ImGui::TableHeadersRow();
            for (const auto& row : model.event_rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%llu", static_cast<unsigned long long>(row.frame_index));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.severity.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.category.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.message.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }

    void draw_timeline_panel() {
        if (timeline_playing_) {
            timeline_playhead_seconds_ += ImGui::GetIO().DeltaTime;
            if (timeline_playhead_seconds_ > editor_timeline_.duration_seconds) {
                timeline_playhead_seconds_ = editor_timeline_.looping ? 0.0F : editor_timeline_.duration_seconds;
                timeline_playing_ = editor_timeline_.looping;
            }
        }

        auto model = mirakana::editor::make_editor_timeline_panel_model(editor_timeline_, timeline_playhead_seconds_,
                                                                        timeline_playing_);
        timeline_playhead_seconds_ = model.playhead_seconds;

        ImGui::Begin("Timeline");
        if (ImGui::Button("Play", ImVec2{64.0F, 0.0F})) {
            timeline_playing_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause", ImVec2{64.0F, 0.0F})) {
            timeline_playing_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Step", ImVec2{64.0F, 0.0F})) {
            timeline_playing_ = false;
            timeline_playhead_seconds_ = std::min(editor_timeline_.duration_seconds, timeline_playhead_seconds_ + 0.1F);
        }
        ImGui::SliderFloat("Playhead", &timeline_playhead_seconds_, 0.0F, editor_timeline_.duration_seconds, "%.3f s");
        ImGui::Text("Duration: %.3f s", model.duration_seconds);
        ImGui::Text("Mode: %s", model.looping ? "Looping" : "One Shot");

        if (ImGui::BeginTable("Timeline Events", 4,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Track");
            ImGui::TableSetupColumn("Time");
            ImGui::TableSetupColumn("Event");
            ImGui::TableSetupColumn("Payload");
            ImGui::TableHeadersRow();
            for (const auto& track : model.tracks) {
                for (const auto& event : track.events) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(track.label.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", event.time_seconds);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(event.name.c_str());
                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextUnformatted(event.payload.c_str());
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }

    void draw_project_settings_panel() {
        ImGui::Begin("Project Settings");
        ImGui::Text("Project: %s", project_.name.c_str());
        ImGui::Text("Root: %s", project_.root_path.c_str());
        ImGui::Text("Assets: %s", project_.asset_root.c_str());
        ImGui::Text("Source Registry: %s", project_.source_registry_path.c_str());
        ImGui::Text("Manifest: %s", project_.game_manifest_path.c_str());
        ImGui::Text("Startup Scene: %s", project_.startup_scene_path.c_str());
        ImGui::SeparatorText("Viewport Backend");
        static constexpr std::array<const char*, 5> backend_labels = {"Automatic", "Null", "Direct3D 12", "Vulkan",
                                                                      "Metal"};
        ImGui::Combo("Backend", &project_render_backend_index_, backend_labels.data(),
                     static_cast<int>(backend_labels.size()));
        sync_project_settings_draft_from_inputs();
        const std::string requested_backend_label{
            mirakana::editor::editor_render_backend_label(project_settings_draft_.render_backend())};
        ImGui::Text("Requested: %s", requested_backend_label.c_str());
        const std::string active_backend_label{
            mirakana::editor::editor_render_backend_label(viewport_.active_render_backend())};
        ImGui::Text("Active: %s", active_backend_label.c_str());
        const std::string render_backend_diag{viewport_.render_backend_diagnostic()};
        ImGui::TextWrapped("%s", render_backend_diag.c_str());
        const auto backend_descriptors =
            mirakana::editor::make_editor_render_backend_descriptors(editor_viewport_backend_availability());
        for (const auto& descriptor : backend_descriptors) {
            const std::string descriptor_label{descriptor.label};
            ImGui::Text("%s: %s", descriptor_label.c_str(), descriptor.available ? "available" : "unavailable");
            if (ImGui::IsItemHovered()) {
                const std::string descriptor_diagnostic{descriptor.diagnostic};
                ImGui::SetTooltip("%s", descriptor_diagnostic.c_str());
            }
        }
        ImGui::SeparatorText("Shader Tool");
        ImGui::InputText("Executable", project_shader_tool_executable_.data(), project_shader_tool_executable_.size());
        ImGui::InputText("Working Directory", project_shader_tool_working_directory_.data(),
                         project_shader_tool_working_directory_.size());
        ImGui::InputText("Artifact Root", project_shader_artifact_output_root_.data(),
                         project_shader_artifact_output_root_.size());
        ImGui::InputText("Cache Index", project_shader_cache_index_.data(), project_shader_cache_index_.size());
        if (ImGui::Button("Discover Shader Tools")) {
            initialize_shader_tool_discovery();
            log_.log(mirakana::LogLevel::info, "editor", "Shader tool discovery refreshed");
        }
        ImGui::Text("Discovered: %zu", shader_tool_discovery_.item_count());
        const auto& toolchain_readiness = shader_tool_discovery_.readiness();
        ImGui::Text("D3D12 tools: %s", toolchain_readiness.ready_for_d3d12_dxil() ? "ready" : "missing");
        ImGui::Text("Vulkan SPIR-V tools: %s", toolchain_readiness.ready_for_vulkan_spirv() ? "ready" : "missing");
        ImGui::Text("DXC SPIR-V CodeGen: %s", toolchain_readiness.dxc_spirv_codegen_available ? "ready" : "missing");
        ImGui::Text("Metal tools: %s", toolchain_readiness.ready_for_metal_library() ? "ready" : "missing");
        for (const auto& diagnostic : toolchain_readiness.diagnostics) {
            ImGui::TextWrapped("%s", diagnostic.c_str());
        }
        if (ImGui::BeginChild("Shader Tool Discovery", ImVec2{0.0F, 92.0F}, ImGuiChildFlags_Borders)) {
            for (const auto& tool : shader_tool_discovery_.items()) {
                const auto label = tool.label + "  " + tool.executable_path + "##" + tool.executable_path;
                if (ImGui::Selectable(label.c_str())) {
                    copy_to_buffer(project_shader_tool_executable_, tool.executable_path);
                    sync_project_settings_draft_from_inputs();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", tool.version.c_str());
                }
            }
        }
        ImGui::EndChild();
        ImGui::SeparatorText("Viewport Shader Artifacts");
        ImGui::Text("D3D12 readiness: %s", viewport_shader_artifacts_.ready_for_d3d12() ? "ready" : "not ready");
        ImGui::Text("Vulkan readiness: %s", viewport_shader_artifacts_.ready_for_vulkan() ? "ready" : "not ready");
        if (ImGui::BeginChild("Viewport Shader Artifacts", ImVec2{0.0F, 80.0F}, ImGuiChildFlags_Borders)) {
            for (const auto& artifact : viewport_shader_artifacts_.items()) {
                const std::string artifact_status{
                    mirakana::editor::viewport_shader_artifact_status_label(artifact.status)};
                ImGui::Text("%s  %zu bytes  %s", artifact_status.c_str(), artifact.byte_size,
                            artifact.output_path.c_str());
                if (!artifact.diagnostic.empty() && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", artifact.diagnostic.c_str());
                }
            }
        }
        ImGui::EndChild();
        ImGui::SeparatorText("Material Preview Shader Artifacts");
        ImGui::Text(
            "D3D12 factor: %s  textured: %s",
            material_preview_shader_artifacts_.ready_for_d3d12(material_preview_factor_shader_id()) ? "ready"
                                                                                                    : "not ready",
            material_preview_shader_artifacts_.ready_for_d3d12(material_preview_textured_shader_id()) ? "ready"
                                                                                                      : "not ready");
        ImGui::Text(
            "Vulkan factor: %s  textured: %s",
            material_preview_shader_artifacts_.ready_for_vulkan(material_preview_factor_shader_id()) ? "ready"
                                                                                                     : "not ready",
            material_preview_shader_artifacts_.ready_for_vulkan(material_preview_textured_shader_id()) ? "ready"
                                                                                                       : "not ready");
        if (ImGui::BeginChild("Material Preview Shader Artifacts", ImVec2{0.0F, 80.0F}, ImGuiChildFlags_Borders)) {
            for (const auto& artifact : material_preview_shader_artifacts_.items()) {
                const std::string artifact_status{
                    mirakana::editor::viewport_shader_artifact_status_label(artifact.status)};
                ImGui::Text("%s  %zu bytes  %s", artifact_status.c_str(), artifact.byte_size,
                            artifact.output_path.c_str());
                if (!artifact.diagnostic.empty() && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", artifact.diagnostic.c_str());
                }
            }
        }
        ImGui::EndChild();
        const auto settings_errors = project_settings_draft_.validation_errors();
        for (const auto& error : settings_errors) {
            ImGui::TextWrapped("%s: %s", error.field.c_str(), error.message.c_str());
        }
        if (ImGui::Button("Apply Settings")) {
            (void)apply_project_settings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Revert")) {
            reset_project_settings_inputs_from_project();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Project")) {
            if (apply_project_settings()) {
                save_project_bundle();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Open Project...")) {
            show_project_open_dialog();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Project As...")) {
            if (apply_project_settings()) {
                show_project_save_dialog();
            }
        }
        ImGui::SeparatorText("Storage");
        ImGui::Text("Project File: %s", project_paths_.project_path.c_str());
        ImGui::Text("Workspace File: %s", project_paths_.workspace_path.c_str());
        ImGui::Text("Scene File: %s", project_paths_.scene_path.c_str());
        ImGui::Text("Dirty: %s", dirty_state_.dirty() ? "yes" : "no");
        draw_project_file_dialog_table("Project Open Dialog", project_open_dialog_);
        draw_project_file_dialog_table("Project Save Dialog", project_save_dialog_);
        ImGui::End();
    }

    void draw_command_palette() {
        ImGui::SetNextWindowSize(ImVec2{520.0F, 420.0F}, ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Command Palette", &show_command_palette_)) {
            ImGui::End();
            return;
        }

        ImGui::InputText("Search", command_palette_query_.data(), command_palette_query_.size());
        ImGui::Separator();
        if (ImGui::BeginChild("Command Palette Results")) {
            const auto matches =
                mirakana::editor::query_command_palette(commands_, std::string_view{command_palette_query_.data()});
            for (const auto& entry : matches) {
                if (ImGui::Selectable(entry.label.c_str(), false, entry.enabled ? 0 : ImGuiSelectableFlags_Disabled)) {
                    if (entry.enabled) {
                        execute_command(entry.id);
                        show_command_palette_ = false;
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", entry.id.c_str());
                }
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }

    static void draw_scene_file_dialog_table(const char* table_id,
                                             const mirakana::editor::EditorSceneFileDialogModel& model) {
        if (!model.rows.empty() && ImGui::BeginTable(table_id, 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : model.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }
        for (const auto& diagnostic : model.diagnostics) {
            ImGui::TextWrapped("%s", diagnostic.c_str());
        }
    }

    static void draw_project_file_dialog_table(const char* table_id,
                                               const mirakana::editor::EditorProjectFileDialogModel& model) {
        if (!model.rows.empty() && ImGui::BeginTable(table_id, 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : model.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }
        for (const auto& diagnostic : model.diagnostics) {
            ImGui::TextWrapped("%s", diagnostic.c_str());
        }
    }

    static void
    draw_prefab_variant_file_dialog_table(const char* table_id,
                                          const mirakana::editor::EditorPrefabVariantFileDialogModel& model) {
        if (!model.rows.empty() && ImGui::BeginTable(table_id, 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (const auto& row : model.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.label.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.value.c_str());
            }
            ImGui::EndTable();
        }
        for (const auto& diagnostic : model.diagnostics) {
            ImGui::TextWrapped("%s", diagnostic.c_str());
        }
    }

    void draw_prefab_variant_panel() {
        ImGui::SeparatorText("Prefab Variant Authoring");
        ImGui::InputText("Variant Path", prefab_variant_path_.data(), prefab_variant_path_.size());
        ImGui::InputText("Variant Name", prefab_variant_name_.data(), prefab_variant_name_.size());
        if (ImGui::Button("New From Saved Prefab")) {
            create_prefab_variant_from_default_prefab();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Variant")) {
            load_prefab_variant_document();
        }
        ImGui::SameLine();
        const bool has_variant = prefab_variant_document_.has_value();
        const bool can_instantiate_variant = has_variant && prefab_variant_document_->model(assets_).valid();
        if (ImGui::Button("Save Variant") && has_variant) {
            save_prefab_variant_document();
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse Load Variant")) {
            show_prefab_variant_open_dialog();
        }
        ImGui::SameLine();
        if (!has_variant) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Browse Save Variant")) {
            show_prefab_variant_save_dialog();
        }
        if (!has_variant) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (ImGui::Button("Instantiate Composed")) {
            if (can_instantiate_variant) {
                instantiate_prefab_variant_composed_prefab();
            } else {
                log_.log(mirakana::LogLevel::warn, "editor", "Prefab variant instantiate requires a valid document");
            }
        }
        draw_prefab_variant_file_dialog_table("Prefab Variant Open Dialog", prefab_variant_open_dialog_);
        draw_prefab_variant_file_dialog_table("Prefab Variant Save Dialog", prefab_variant_save_dialog_);

        if (!has_variant) {
            ImGui::TextDisabled("Create or load a prefab variant to edit overrides.");
            return;
        }

        auto& document = *prefab_variant_document_;
        const auto model = document.model(assets_);
        const auto conflict_model = mirakana::editor::make_prefab_variant_conflict_review_model(document);
        ImGui::Text("Base: %s", document.base_prefab().name.c_str());
        ImGui::Text("Dirty: %s  Valid: %s  Overrides: %zu", model.dirty ? "yes" : "no", model.valid() ? "yes" : "no",
                    model.override_rows.size());
        ImGui::Text("Undo: %zu  Redo: %zu", prefab_variant_history_.undo_count(), prefab_variant_history_.redo_count());
        if (ImGui::Button("Undo Variant") && prefab_variant_history_.can_undo()) {
            (void)prefab_variant_history_.undo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Redo Variant") && prefab_variant_history_.can_redo()) {
            (void)prefab_variant_history_.redo();
        }

        if (!model.diagnostics.empty()) {
            ImGui::SeparatorText("Variant Diagnostics");
            if (ImGui::BeginChild("Prefab Variant Diagnostics", ImVec2{0.0F, 72.0F}, ImGuiChildFlags_Borders)) {
                for (const auto& diagnostic : model.diagnostics) {
                    const std::string override_kind_label{
                        mirakana::prefab_override_kind_label(diagnostic.override_kind)};
                    ImGui::TextWrapped("node #%u %s %s asset #%llu: %s", diagnostic.node_index,
                                       override_kind_label.c_str(), diagnostic.field.c_str(),
                                       static_cast<unsigned long long>(diagnostic.asset.value),
                                       diagnostic.diagnostic.c_str());
                }
            }
            ImGui::EndChild();
        }

        ImGui::SeparatorText("Conflict Review");
        ImGui::Text("Status: %s  Blocking: %zu  Warnings: %zu", conflict_model.status_label.c_str(),
                    conflict_model.blocking_count, conflict_model.warning_count);
        const auto& batch_resolution = conflict_model.batch_resolution;
        if (!batch_resolution.available) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Apply All Reviewed")) {
            execute_prefab_variant_authoring_action(
                mirakana::editor::make_prefab_variant_conflict_batch_resolution_action(document));
        }
        if (!batch_resolution.available) {
            ImGui::EndDisabled();
        }
        if (!batch_resolution.diagnostic.empty() && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", batch_resolution.diagnostic.c_str());
        }
        ImGui::SameLine();
        ImGui::Text("Reviewed resolutions: %zu (%zu blocking, %zu warnings)", batch_resolution.resolution_count,
                    batch_resolution.blocking_resolution_count, batch_resolution.warning_resolution_count);
        if (ImGui::BeginTable("Prefab Variant Conflict Rows", 8,
                              ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Node");
            ImGui::TableSetupColumn("Kind");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Conflict");
            ImGui::TableSetupColumn("Base");
            ImGui::TableSetupColumn("Override");
            ImGui::TableSetupColumn("Resolution");
            ImGui::TableSetupColumn("Diagnostic");
            ImGui::TableHeadersRow();
            for (const auto& row : conflict_model.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", row.node_index);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.override_kind_label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.status_label.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.conflict_label.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(row.base_value.c_str());
                ImGui::TableSetColumnIndex(5);
                ImGui::TextUnformatted(row.override_value.c_str());
                ImGui::TableSetColumnIndex(6);
                if (row.resolution_available) {
                    const auto button_id = std::string("Apply##") + row.resolution_id;
                    if (ImGui::Button(button_id.c_str())) {
                        execute_prefab_variant_authoring_action(
                            mirakana::editor::make_prefab_variant_conflict_resolution_action(document,
                                                                                             row.resolution_id));
                    }
                    if (!row.resolution_diagnostic.empty() && ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", row.resolution_diagnostic.c_str());
                    }
                } else {
                    ImGui::TextDisabled("-");
                }
                ImGui::TableSetColumnIndex(7);
                ImGui::TextUnformatted(row.diagnostic.c_str());
            }
            ImGui::EndTable();
        }
        if (!conflict_model.diagnostics.empty()) {
            ImGui::SeparatorText("Conflict Diagnostics");
            for (const auto& diagnostic : conflict_model.diagnostics) {
                ImGui::TextWrapped("%s", diagnostic.c_str());
            }
        }

        ImGui::SeparatorText("Override Rows");
        if (ImGui::BeginTable("Prefab Variant Overrides", 6, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Node");
            ImGui::TableSetupColumn("Base Name");
            ImGui::TableSetupColumn("Kind");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Components");
            ImGui::TableSetupColumn("Diagnostics");
            ImGui::TableHeadersRow();
            for (const auto& row : model.override_rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", row.node_index);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.node_name.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.kind_label.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(row.has_name ? "yes" : "no");
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("camera:%s light:%s mesh:%s sprite:%s", row.has_camera ? "yes" : "no",
                            row.has_light ? "yes" : "no", row.has_mesh_renderer ? "yes" : "no",
                            row.has_sprite_renderer ? "yes" : "no");
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%zu", row.diagnostic_count);
            }
            ImGui::EndTable();
        }

        ImGui::SeparatorText("Override Editor");
        ImGui::InputInt("Variant Node", &prefab_variant_node_index_);
        ImGui::InputText("Name Override", prefab_variant_name_override_.data(), prefab_variant_name_override_.size());
        if (ImGui::Button("Apply Name Override")) {
            execute_prefab_variant_authoring_action(mirakana::editor::make_prefab_variant_name_override_action(
                document, prefab_variant_node_index(), std::string(prefab_variant_name_override_.data())));
        }

        if (ImGui::DragFloat3("Variant Position", prefab_variant_position_.data(), 0.05F) ||
            ImGui::DragFloat3("Variant Rotation", prefab_variant_rotation_degrees_.data(), 0.5F) ||
            ImGui::DragFloat3("Variant Scale", prefab_variant_scale_.data(), 0.05F, 0.001F, 1000.0F)) {
            // Keep values in the edit buffers until the author applies the override.
        }
        if (ImGui::Button("Apply Transform Override")) {
            mirakana::Transform3D transform;
            transform.position =
                mirakana::Vec3{prefab_variant_position_[0], prefab_variant_position_[1], prefab_variant_position_[2]};
            transform.scale =
                mirakana::Vec3{prefab_variant_scale_[0], prefab_variant_scale_[1], prefab_variant_scale_[2]};
            transform.rotation_radians = mirakana::Vec3{prefab_variant_rotation_degrees_[0] * degrees_to_radians,
                                                        prefab_variant_rotation_degrees_[1] * degrees_to_radians,
                                                        prefab_variant_rotation_degrees_[2] * degrees_to_radians};
            execute_prefab_variant_authoring_action(mirakana::editor::make_prefab_variant_transform_override_action(
                document, prefab_variant_node_index(), transform));
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy Base Node Transform")) {
            copy_prefab_variant_base_node_to_buffers();
        }

        if (ImGui::Button("Override Components From Selection")) {
            apply_prefab_variant_selected_components_override();
        }
        ImGui::SameLine();
        if (ImGui::Button("Default Mesh Override")) {
            mirakana::SceneNodeComponents components;
            components.mesh_renderer = mirakana::MeshRendererComponent{
                mirakana::AssetId::from_name("editor.default.mesh"),
                mirakana::AssetId::from_name("editor.default.material"),
                true,
            };
            execute_prefab_variant_authoring_action(mirakana::editor::make_prefab_variant_component_override_action(
                document, prefab_variant_node_index(), components));
        }

        ImGui::SeparatorText("Composed Prefab Review");
        if (conflict_model.can_compose) {
            const auto composed = document.composed_prefab();
            ImGui::Text("Name: %s", composed.name.c_str());
            ImGui::Text("Nodes: %zu", composed.nodes.size());
        } else {
            ImGui::TextDisabled("Resolve blocking conflict rows before composing this variant.");
        }
    }

    void draw_project_wizard() {
        ImGui::SetNextWindowSize(ImVec2{560.0F, 440.0F}, ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("New Project", &show_project_wizard_)) {
            ImGui::End();
            return;
        }

        const std::string step_name{mirakana::editor::project_creation_step_name(project_wizard_.step())};
        ImGui::Text("Step: %s", step_name.c_str());
        ImGui::Separator();

        ImGui::InputText("Name", project_wizard_name_.data(), project_wizard_name_.size());
        ImGui::InputText("Root", project_wizard_root_.data(), project_wizard_root_.size());
        if (project_wizard_.step() != mirakana::editor::ProjectCreationStep::identity) {
            ImGui::InputText("Assets", project_wizard_assets_.data(), project_wizard_assets_.size());
            ImGui::InputText("Source Registry", project_wizard_source_registry_.data(),
                             project_wizard_source_registry_.size());
            ImGui::InputText("Manifest", project_wizard_manifest_.data(), project_wizard_manifest_.size());
            ImGui::InputText("Startup Scene", project_wizard_scene_.data(), project_wizard_scene_.size());
        }

        sync_project_wizard_from_inputs();
        const auto errors = project_wizard_.validation_errors();
        for (const auto& error : errors) {
            ImGui::TextWrapped("%s: %s", error.field.c_str(), error.message.c_str());
        }

        ImGui::Separator();
        if (ImGui::Button("Back")) {
            (void)project_wizard_.back();
        }
        ImGui::SameLine();
        if (project_wizard_.step() != mirakana::editor::ProjectCreationStep::review) {
            if (ImGui::Button("Next")) {
                (void)project_wizard_.advance();
            }
        } else if (ImGui::Button("Create")) {
            create_project_from_wizard();
        }

        ImGui::End();
    }

    void add_empty_node() {
        const auto name = std::string("Empty Node ") + std::to_string(active_scene().nodes().size() + 1U);
        (void)execute_scene_authoring_action(
            mirakana::editor::make_scene_authoring_create_node_action(scene_document_, name, selected_node()));
    }

    void delete_selected_node() {
        const auto node = selected_node();
        if (node == mirakana::null_scene_node) {
            log_.log(mirakana::LogLevel::warn, "editor", "Delete scene node requires a selection");
            return;
        }
        (void)execute_scene_authoring_action(
            mirakana::editor::make_scene_authoring_delete_node_action(scene_document_, node));
    }

    void duplicate_selected_node() {
        const auto node = selected_node();
        const auto* scene_node = active_scene().find_node(node);
        if (scene_node == nullptr) {
            log_.log(mirakana::LogLevel::warn, "editor", "Duplicate scene node requires a selection");
            return;
        }
        (void)execute_scene_authoring_action(mirakana::editor::make_scene_authoring_duplicate_subtree_action(
            scene_document_, node, scene_node->name + " Copy"));
    }

    void save_selected_prefab() {
        try {
            auto prefab = mirakana::editor::build_prefab_from_selected_node(scene_document_, "selected_prefab");
            if (!prefab.has_value()) {
                log_.log(mirakana::LogLevel::warn, "editor", "Save prefab requires a selected node");
                return;
            }
            mirakana::editor::save_prefab_authoring_document(project_store_, current_prefab_path(), *prefab);
            log_.log(mirakana::LogLevel::info, "editor", "Prefab saved");
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void instantiate_default_prefab() {
        try {
            auto prefab = mirakana::editor::load_prefab_authoring_document(project_store_, current_prefab_path());
            (void)execute_scene_authoring_action(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
                scene_document_, std::move(prefab), current_prefab_path()));
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    [[nodiscard]] mirakana::editor::ScenePrefabInstanceRefreshPolicy make_scene_prefab_refresh_policy() {
        mirakana::editor::ScenePrefabInstanceRefreshPolicy policy{};
        policy.keep_local_children = scene_prefab_refresh_keep_local_children_;
        policy.keep_stale_source_nodes_as_local = scene_prefab_refresh_keep_stale_source_nodes_;
        policy.keep_nested_prefab_instances = scene_prefab_refresh_keep_nested_prefab_instances_;
        policy.apply_reviewed_nested_prefab_propagation = scene_prefab_refresh_apply_nested_prefab_propagation_;
        policy.load_prefab_for_nested_propagation =
            [this](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            try {
                return mirakana::editor::load_prefab_authoring_document(project_store_, path);
            } catch (const std::exception&) {
                return std::nullopt;
            }
        };
        return policy;
    }

    void refresh_selected_prefab_instance() {
        const auto node = selected_node();
        const auto* scene_node = active_scene().find_node(node);
        if (scene_node == nullptr || !scene_node->prefab_source.has_value() ||
            !mirakana::is_valid_scene_prefab_source_link(*scene_node->prefab_source) ||
            scene_node->prefab_source->source_node_index != 1U || scene_node->prefab_source->prefab_path.empty()) {
            log_.log(mirakana::LogLevel::warn, "editor",
                     "Refresh prefab instance requires a selected linked prefab root");
            return;
        }

        // make_scene_prefab_instance_refresh_action remains the single-root undoable factory in editor core;
        // MK_editor uses plan_scene_prefab_instance_refresh_batch even for one target so apply shares one code path.

        try {
            const auto prefab_path = scene_node->prefab_source->prefab_path;
            auto prefab = mirakana::editor::load_prefab_authoring_document(project_store_, prefab_path);
            const auto policy = make_scene_prefab_refresh_policy();
            std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> targets;
            targets.push_back(
                mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput{node, std::move(prefab), prefab_path});
            auto batch_plan =
                mirakana::editor::plan_scene_prefab_instance_refresh_batch(scene_document_, std::move(targets), policy);
            const bool can_apply = batch_plan.can_apply;
            scene_prefab_refresh_review_ = ScenePrefabRefreshReviewState{policy, std::move(batch_plan)};
            if (!can_apply) {
                const auto& plan = scene_prefab_refresh_review_->batch_plan;
                const auto diagnostic = prefab_refresh_blocked_diagnostic(plan, "prefab refresh blocked");
                log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
                return;
            }
            log_.log(mirakana::LogLevel::info, "editor", "Prefab instance refresh reviewed");
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void refresh_batch_prefab_instances_review() {
        if (scene_prefab_batch_refresh_node_ids_.empty()) {
            log_.log(mirakana::LogLevel::warn, "editor",
                     "Batch prefab refresh requires at least one linked prefab root in the batch set");
            return;
        }
        std::vector<std::uint32_t> ordered_ids(scene_prefab_batch_refresh_node_ids_.begin(),
                                               scene_prefab_batch_refresh_node_ids_.end());
        std::sort(ordered_ids.begin(), ordered_ids.end());
        try {
            const auto policy = make_scene_prefab_refresh_policy();
            std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> targets;
            targets.reserve(ordered_ids.size());
            for (const auto id_value : ordered_ids) {
                const mirakana::SceneNodeId node{id_value};
                const auto* scene_node = active_scene().find_node(node);
                if (scene_node == nullptr || !scene_node->prefab_source.has_value() ||
                    !mirakana::is_valid_scene_prefab_source_link(*scene_node->prefab_source) ||
                    scene_node->prefab_source->source_node_index != 1U ||
                    scene_node->prefab_source->prefab_path.empty()) {
                    log_.log(mirakana::LogLevel::warn, "editor",
                             "Batch prefab refresh skipped an ineligible node id; clear batch selection");
                    return;
                }
                const auto prefab_path = scene_node->prefab_source->prefab_path;
                auto prefab = mirakana::editor::load_prefab_authoring_document(project_store_, prefab_path);
                targets.push_back(
                    mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput{node, std::move(prefab), prefab_path});
            }
            auto batch_plan =
                mirakana::editor::plan_scene_prefab_instance_refresh_batch(scene_document_, std::move(targets), policy);
            const bool can_apply = batch_plan.can_apply;
            scene_prefab_refresh_review_ = ScenePrefabRefreshReviewState{policy, std::move(batch_plan)};
            if (!can_apply) {
                const auto& plan = scene_prefab_refresh_review_->batch_plan;
                const auto diagnostic = prefab_refresh_blocked_diagnostic(plan, "prefab batch refresh blocked");
                log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
                return;
            }
            log_.log(mirakana::LogLevel::info, "editor", "Prefab instance batch refresh reviewed");
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void apply_reviewed_prefab_instance_refresh() {
        if (!scene_prefab_refresh_review_.has_value()) {
            log_.log(mirakana::LogLevel::warn, "editor", "Prefab refresh review is not available");
            return;
        }

        auto review = *scene_prefab_refresh_review_;
        auto targets_copy = review.batch_plan.ordered_targets;
        review.batch_plan = mirakana::editor::plan_scene_prefab_instance_refresh_batch(
            scene_document_, std::move(targets_copy), review.policy);
        scene_prefab_refresh_review_ = review;
        if (!scene_prefab_refresh_review_->batch_plan.can_apply) {
            const auto& plan = scene_prefab_refresh_review_->batch_plan;
            const auto diagnostic = prefab_refresh_blocked_diagnostic(plan, "prefab refresh blocked");
            log_.log(mirakana::LogLevel::warn, "editor", diagnostic);
            return;
        }

        auto apply_inputs = scene_prefab_refresh_review_->batch_plan.ordered_targets;
        const auto policy = scene_prefab_refresh_review_->policy;
        if (execute_scene_authoring_action(mirakana::editor::make_scene_prefab_instance_refresh_batch_action(
                scene_document_, std::move(apply_inputs), policy))) {
            log_.log(mirakana::LogLevel::info, "editor", "Prefab instance refresh applied");
        }
    }

    [[nodiscard]] std::uint32_t prefab_variant_node_index() const noexcept {
        return prefab_variant_node_index_ > 0 ? static_cast<std::uint32_t>(prefab_variant_node_index_) : 0U;
    }

    void create_prefab_variant_from_default_prefab() {
        try {
            auto base_prefab = mirakana::editor::load_prefab_authoring_document(project_store_, current_prefab_path());
            prefab_variant_document_ = mirakana::editor::PrefabVariantAuthoringDocument::from_base_prefab(
                std::move(base_prefab), std::string(prefab_variant_name_.data()),
                std::string(prefab_variant_path_.data()));
            prefab_variant_history_.clear();
            copy_prefab_variant_base_node_to_buffers();
            log_.log(mirakana::LogLevel::info, "editor", "Prefab variant created");
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void load_prefab_variant_document() {
        try {
            prefab_variant_document_ = mirakana::editor::load_prefab_variant_authoring_document(
                project_store_, std::string_view(prefab_variant_path_.data()));
            prefab_variant_history_.clear();
            sync_prefab_variant_buffers_from_document();
            log_.log(mirakana::LogLevel::info, "editor", "Prefab variant loaded");
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void save_prefab_variant_document() {
        if (!prefab_variant_document_.has_value()) {
            return;
        }

        try {
            mirakana::editor::save_prefab_variant_authoring_document(project_store_, prefab_variant_path_.data(),
                                                                     *prefab_variant_document_, assets_);
            log_.log(mirakana::LogLevel::info, "editor", "Prefab variant saved");
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void instantiate_prefab_variant_composed_prefab() {
        if (!prefab_variant_document_.has_value()) {
            return;
        }

        try {
            if (!prefab_variant_document_->model(assets_).valid()) {
                log_.log(mirakana::LogLevel::warn, "editor", "Prefab variant has unresolved diagnostics");
                return;
            }
            auto composed = prefab_variant_document_->composed_prefab();
            (void)execute_scene_authoring_action(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
                scene_document_, std::move(composed), std::string(prefab_variant_path_.data())));
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    bool execute_prefab_variant_authoring_action(mirakana::editor::UndoableAction action) {
        if (!prefab_variant_document_.has_value()) {
            return false;
        }
        if (!prefab_variant_history_.execute(std::move(action))) {
            log_.log(mirakana::LogLevel::warn, "editor", "Prefab variant action was rejected");
            return false;
        }
        return true;
    }

    void sync_prefab_variant_buffers_from_document() {
        if (!prefab_variant_document_.has_value()) {
            return;
        }
        copy_to_buffer(prefab_variant_path_, std::string(prefab_variant_document_->path()));
        copy_to_buffer(prefab_variant_name_, prefab_variant_document_->variant().name);
        copy_prefab_variant_base_node_to_buffers();
    }

    void reset_prefab_variant_document() {
        prefab_variant_history_.clear();
        prefab_variant_document_.reset();
        prefab_variant_open_dialog_id_.reset();
        prefab_variant_save_dialog_id_.reset();
        prefab_variant_open_dialog_ = mirakana::editor::EditorPrefabVariantFileDialogModel{};
        prefab_variant_save_dialog_ = mirakana::editor::EditorPrefabVariantFileDialogModel{
            .mode = mirakana::editor::EditorPrefabVariantFileDialogMode::save,
            .accepted = false,
            .status_label = {},
            .selected_path = {},
            .rows = {},
            .diagnostics = {},
        };
        copy_to_buffer(prefab_variant_path_, current_prefab_variant_path());
        copy_to_buffer(prefab_variant_name_, "selected_prefab_variant");
        prefab_variant_node_index_ = 1;
        copy_to_buffer(prefab_variant_name_override_, "VariantNode");
        prefab_variant_position_[0] = 0.0F;
        prefab_variant_position_[1] = 0.0F;
        prefab_variant_position_[2] = 0.0F;
        prefab_variant_rotation_degrees_[0] = 0.0F;
        prefab_variant_rotation_degrees_[1] = 0.0F;
        prefab_variant_rotation_degrees_[2] = 0.0F;
        prefab_variant_scale_[0] = 1.0F;
        prefab_variant_scale_[1] = 1.0F;
        prefab_variant_scale_[2] = 1.0F;
    }

    void reset_ai_evidence_import_state() {
        ai_evidence_import_text_.fill('\0');
        ai_playtest_evidence_rows_.clear();
        ai_acknowledged_host_gate_recipe_ids_.clear();
    }

    void reset_resource_capture_request_state() {
        resource_acknowledged_capture_request_ids_.clear();
        pix_host_helper_last_run_valid_ = false;
        pix_host_helper_last_run_succeeded_ = false;
        pix_host_helper_last_exit_code_.reset();
        pix_host_helper_last_session_dir_.clear();
        pix_host_helper_last_stdout_summary_.clear();
        pix_host_helper_last_stderr_summary_.clear();
        pix_host_helper_last_run_skip_launch_.reset();
    }

    void copy_prefab_variant_base_node_to_buffers() {
        if (!prefab_variant_document_.has_value()) {
            return;
        }

        const auto node_index = prefab_variant_node_index();
        const auto& nodes = prefab_variant_document_->base_prefab().nodes;
        if (node_index == 0 || node_index > nodes.size()) {
            return;
        }

        const auto& node = nodes[node_index - 1U];
        copy_to_buffer(prefab_variant_name_override_, node.name);
        prefab_variant_position_[0] = node.transform.position.x;
        prefab_variant_position_[1] = node.transform.position.y;
        prefab_variant_position_[2] = node.transform.position.z;
        prefab_variant_scale_[0] = node.transform.scale.x;
        prefab_variant_scale_[1] = node.transform.scale.y;
        prefab_variant_scale_[2] = node.transform.scale.z;
        prefab_variant_rotation_degrees_[0] = node.transform.rotation_radians.x * radians_to_degrees;
        prefab_variant_rotation_degrees_[1] = node.transform.rotation_radians.y * radians_to_degrees;
        prefab_variant_rotation_degrees_[2] = node.transform.rotation_radians.z * radians_to_degrees;
    }

    void apply_prefab_variant_selected_components_override() {
        if (!prefab_variant_document_.has_value()) {
            return;
        }

        auto draft = mirakana::editor::make_scene_node_component_draft(active_scene(), selected_node());
        if (!draft.has_value()) {
            log_.log(mirakana::LogLevel::warn, "editor",
                     "Prefab variant component override requires a selected scene node");
            return;
        }

        execute_prefab_variant_authoring_action(mirakana::editor::make_prefab_variant_component_override_action(
            *prefab_variant_document_, prefab_variant_node_index(), draft->components));
    }

    void open_project_wizard() {
        project_wizard_.reset();
        copy_to_buffer(project_wizard_name_, "Untitled Project");
        copy_to_buffer(project_wizard_root_, "games/untitled-project");
        copy_to_buffer(project_wizard_assets_, "assets");
        copy_to_buffer(project_wizard_source_registry_, "source/assets/package.geassets");
        copy_to_buffer(project_wizard_manifest_, "game.agent.json");
        copy_to_buffer(project_wizard_scene_, "scenes/start.scene");
        sync_project_wizard_from_inputs();
        show_project_wizard_ = true;
    }

    void sync_project_wizard_from_inputs() {
        project_wizard_.set_name(std::string(project_wizard_name_.data()));
        project_wizard_.set_root_path(std::string(project_wizard_root_.data()));
        project_wizard_.set_asset_root(std::string(project_wizard_assets_.data()));
        project_wizard_.set_source_registry_path(std::string(project_wizard_source_registry_.data()));
        project_wizard_.set_game_manifest_path(std::string(project_wizard_manifest_.data()));
        project_wizard_.set_startup_scene_path(std::string(project_wizard_scene_.data()));
    }

    void create_project_from_wizard() {
        try {
            project_ = project_wizard_.create_project_document();
            reset_project_settings_inputs_from_project();
            workspace_ = mirakana::editor::Workspace::create_default(
                mirakana::editor::ProjectInfo{project_.name, project_.root_path});
            project_paths_ = mirakana::editor::ProjectBundlePaths{
                join_project_path(project_.root_path, "GameEngine.geproject"),
                join_project_path(project_.root_path, "GameEngine.geworkspace"),
                join_project_path(project_.root_path, project_.startup_scene_path),
            };
            reset_prefab_variant_document();
            reset_ai_evidence_import_state();
            reset_resource_capture_request_state();
            replace_scene_document(make_default_scene_document(project_paths_.scene_path), true);
            initialize_asset_pipeline();
            refresh_content_browser_from_project_or_cooked_assets();
            initialize_shader_compile_state();
            configure_viewport_render_backend();
            if (viewport_.ready()) {
                recreate_viewport_render_resources(current_viewport_extent());
            }
            show_project_wizard_ = false;
            log_.log(mirakana::LogLevel::info, "editor", "Project created");
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    template <std::size_t Size> static void copy_to_buffer(std::array<char, Size>& buffer, const char* text) {
        std::snprintf(buffer.data(), buffer.size(), "%s", text);
    }

    template <std::size_t Size> static void copy_to_buffer(std::array<char, Size>& buffer, const std::string& text) {
        copy_to_buffer(buffer, text.c_str());
    }

    static std::string join_project_path(std::string_view root, std::string_view child) {
        if (root.empty() || root == ".") {
            return std::string(child);
        }
        return std::string(root) + "/" + std::string(child);
    }

    [[nodiscard]] std::string current_prefab_path() const {
        return join_project_path(project_.root_path, default_prefab_path);
    }

    [[nodiscard]] std::string current_prefab_variant_path() const {
        return join_project_path(project_.root_path, default_prefab_variant_path);
    }

    [[nodiscard]] std::string current_cooked_scene_path() const {
        return join_project_path(project_.root_path, default_cooked_scene_path);
    }

    [[nodiscard]] std::string current_package_index_path() const {
        return join_project_path(project_.root_path, default_package_index_path);
    }

    [[nodiscard]] const std::vector<std::string>& current_runtime_package_files() const noexcept {
        return manifest_runtime_package_files_;
    }

    static int project_render_backend_to_index(mirakana::editor::EditorRenderBackend backend) noexcept {
        switch (backend) {
        case mirakana::editor::EditorRenderBackend::automatic:
            return 0;
        case mirakana::editor::EditorRenderBackend::null:
            return 1;
        case mirakana::editor::EditorRenderBackend::d3d12:
            return 2;
        case mirakana::editor::EditorRenderBackend::vulkan:
            return 3;
        case mirakana::editor::EditorRenderBackend::metal:
            return 4;
        }
        return 0;
    }

    static mirakana::editor::EditorRenderBackend project_render_backend_from_index(int index) noexcept {
        switch (index) {
        case 1:
            return mirakana::editor::EditorRenderBackend::null;
        case 2:
            return mirakana::editor::EditorRenderBackend::d3d12;
        case 3:
            return mirakana::editor::EditorRenderBackend::vulkan;
        case 4:
            return mirakana::editor::EditorRenderBackend::metal;
        case 0:
        default:
            return mirakana::editor::EditorRenderBackend::automatic;
        }
    }

    void initialize_asset_pipeline() {
        const auto texture_id = mirakana::AssetId::from_name("editor.default.texture");
        const auto mesh_id = mirakana::AssetId::from_name("editor.default.mesh");
        const auto material_id = mirakana::AssetId::from_name("editor.default.material");
        const auto audio_id = mirakana::AssetId::from_name("editor.default.audio");

        mirakana::AssetImportMetadataRegistry imports;
        imports.add_texture(mirakana::TextureImportMetadata{
            texture_id,
            "source/builtin/default.texture",
            "builtin/default.texture",
            mirakana::TextureColorSpace::srgb,
            true,
            mirakana::TextureCompression::none,
        });
        imports.add_mesh(mirakana::MeshImportMetadata{
            mesh_id,
            "source/builtin/default.mesh",
            "builtin/default.mesh",
            1.0F,
            false,
            true,
        });
        imports.add_material(mirakana::MaterialImportMetadata{
            material_id,
            "source/builtin/default.material",
            "builtin/default.material",
            {texture_id},
        });
        imports.add_audio(mirakana::AudioImportMetadata{
            audio_id,
            "source/builtin/default.audio_source",
            "builtin/default.audio",
            false,
        });
        asset_import_plan_ = mirakana::build_asset_import_plan(imports);
        asset_pipeline_.set_import_plan(asset_import_plan_);
    }

    void refresh_content_browser_from_cooked_assets() {
        content_browser_.refresh_from(assets_);
        content_browser_source_registry_loaded_ = false;
    }

    [[nodiscard]] bool reload_project_source_registry_browser(bool update_import_plan) {
        auto result = mirakana::editor::refresh_content_browser_from_project_source_registry(project_store_, project_,
                                                                                             content_browser_);
        const bool loaded = result.loaded;
        if (loaded && update_import_plan) {
            asset_import_plan_ = result.import_plan;
            asset_pipeline_.set_import_plan(asset_import_plan_);
        }
        source_registry_browser_refresh_ = std::move(result);
        content_browser_source_registry_loaded_ = loaded;
        return loaded;
    }

    void refresh_content_browser_from_project_or_cooked_assets() {
        if (!reload_project_source_registry_browser(true)) {
            initialize_asset_pipeline();
            refresh_content_browser_from_cooked_assets();
        }
    }

    void refresh_content_browser_after_asset_registry_update() {
        if (content_browser_source_registry_loaded_) {
            if (!reload_project_source_registry_browser(false)) {
                initialize_asset_pipeline();
                refresh_content_browser_from_cooked_assets();
            }
            return;
        }
        refresh_content_browser_from_cooked_assets();
    }

    void initialize_shader_compile_state() {
        shader_compile_requests_ =
            mirakana::editor::make_viewport_shader_compile_requests(project_.shader_tool.artifact_output_root);
        material_preview_shader_compile_requests_ =
            mirakana::editor::make_material_preview_shader_compile_requests(project_.shader_tool.artifact_output_root);
        shader_compiles_.set_requests(all_shader_compile_requests());
        refresh_viewport_shader_artifacts();
        refresh_material_preview_shader_artifacts();
    }

    void initialize_shader_tool_discovery() {
        auto discovered = mirakana::discover_shader_tools(tool_filesystem_, mirakana::ShaderToolDiscoveryRequest{{
                                                                                "toolchains/dxc/bin",
                                                                                "toolchains/vulkan/bin",
                                                                                "toolchains/apple/bin",
                                                                                "external/dxc/bin",
                                                                                "external/vulkan/bin",
                                                                                "external/apple/bin",
                                                                            }});
        append_known_installed_shader_tools(discovered);
        shader_tool_discovery_.refresh_from(std::move(discovered));
    }

    void reset_project_settings_inputs_from_project() {
        copy_to_buffer(project_shader_tool_executable_, project_.shader_tool.executable);
        copy_to_buffer(project_shader_tool_working_directory_, project_.shader_tool.working_directory);
        copy_to_buffer(project_shader_artifact_output_root_, project_.shader_tool.artifact_output_root);
        copy_to_buffer(project_shader_cache_index_, project_.shader_tool.cache_index_path);
        project_render_backend_index_ = project_render_backend_to_index(project_.render_backend);
        project_settings_draft_ = mirakana::editor::ProjectSettingsDraft::from_project(project_);
    }

    void sync_project_settings_draft_from_inputs() {
        project_settings_draft_ = mirakana::editor::ProjectSettingsDraft::from_project(project_);
        project_settings_draft_.set_shader_tool_executable(std::string(project_shader_tool_executable_.data()));
        project_settings_draft_.set_shader_tool_working_directory(
            std::string(project_shader_tool_working_directory_.data()));
        project_settings_draft_.set_shader_artifact_output_root(
            std::string(project_shader_artifact_output_root_.data()));
        project_settings_draft_.set_shader_cache_index_path(std::string(project_shader_cache_index_.data()));
        project_settings_draft_.set_render_backend(project_render_backend_from_index(project_render_backend_index_));
    }

    [[nodiscard]] bool apply_project_settings() {
        try {
            sync_project_settings_draft_from_inputs();
            const auto updated_project = project_settings_draft_.apply();
            const bool shader_settings_changed =
                updated_project.shader_tool.executable != project_.shader_tool.executable ||
                updated_project.shader_tool.working_directory != project_.shader_tool.working_directory ||
                updated_project.shader_tool.artifact_output_root != project_.shader_tool.artifact_output_root ||
                updated_project.shader_tool.cache_index_path != project_.shader_tool.cache_index_path;
            const bool render_backend_changed = updated_project.render_backend != project_.render_backend;
            project_ = updated_project;
            if (shader_settings_changed) {
                initialize_shader_compile_state();
            }
            if (shader_settings_changed || render_backend_changed) {
                if (render_backend_changed) {
                    reset_resource_capture_request_state();
                }
                configure_viewport_render_backend();
                if (viewport_.ready()) {
                    recreate_viewport_render_resources(current_viewport_extent());
                }
            }
            if (shader_settings_changed || render_backend_changed) {
                dirty_state_.mark_dirty();
                log_.log(mirakana::LogLevel::info, "editor", "Project settings applied");
            } else {
                log_.log(mirakana::LogLevel::info, "editor", "Project settings unchanged");
            }
            return true;
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
            return false;
        }
    }

    [[nodiscard]] mirakana::Extent2D current_viewport_extent() const noexcept {
        const auto extent = viewport_.extent();
        return mirakana::Extent2D{extent.width, extent.height};
    }

    [[nodiscard]] std::optional<std::string> viewport_shader_output_path(mirakana::ShaderSourceStage stage) const {
        for (const auto& request : shader_compile_requests_) {
            if (request.source.stage == stage) {
                return request.output_path;
            }
        }
        return std::nullopt;
    }

    void refresh_viewport_shader_artifacts() {
        viewport_shader_artifacts_.refresh_from(tool_filesystem_, shader_compile_requests_);
    }

    void refresh_material_preview_shader_artifacts() {
        material_preview_shader_artifacts_.refresh_from(tool_filesystem_, material_preview_shader_compile_requests_);
    }

    [[nodiscard]] std::optional<std::string> read_viewport_shader_bytecode(mirakana::ShaderSourceStage stage,
                                                                           mirakana::ShaderCompileTarget target) const {
        const auto* item = viewport_shader_artifacts_.find(stage, target);
        if (item == nullptr || item->status != mirakana::editor::ViewportShaderArtifactStatus::ready) {
            return std::nullopt;
        }
        return tool_filesystem_.read_text(item->output_path);
    }

    [[nodiscard]] std::optional<std::string>
    read_material_preview_shader_bytecode(mirakana::AssetId shader, mirakana::ShaderSourceStage stage,
                                          mirakana::ShaderCompileTarget target) const {
        const auto* item = material_preview_shader_artifacts_.find(shader, stage, target);
        if (item == nullptr || item->status != mirakana::editor::ViewportShaderArtifactStatus::ready) {
            return std::nullopt;
        }
        return tool_filesystem_.read_text(item->output_path);
    }

    [[nodiscard]] std::vector<mirakana::ShaderCompileRequest> all_shader_compile_requests() const {
        auto requests = shader_compile_requests_;
        requests.insert(requests.end(), material_preview_shader_compile_requests_.begin(),
                        material_preview_shader_compile_requests_.end());
        return requests;
    }

    void reset_material_preview_gpu_cache() {
        material_preview_gpu_.reset();
    }

    void reset_viewport_render_resources() {
        reset_material_preview_gpu_cache();
        viewport_display_texture_.reset();
        viewport_surface_.reset();
        viewport_pipeline_ = mirakana::rhi::GraphicsPipelineHandle{};
        viewport_device_.reset();
    }

    void update_display_texture_from_surface(mirakana::RhiViewportSurface& surface,
                                             mirakana::editor::SdlViewportTexture& display_texture) {
        bool display_updated = false;
        bool attempted_native_display = false;
#if defined(MK_EDITOR_ENABLE_D3D12)
        if (viewport_device_ != nullptr &&
            viewport_.active_render_backend() == mirakana::editor::EditorRenderBackend::d3d12) {
            attempted_native_display = true;
            const auto display_frame = surface.prepare_display_frame();
            display_updated = display_texture.update_from_d3d12_shared_texture(*viewport_device_, display_frame);
        }
#endif
        if (!display_updated) {
            const auto frame = surface.readback_color_frame();
            (void)display_texture.update_from_frame(frame, attempted_native_display);
        }
    }

    [[nodiscard]] mirakana::editor::MaterialPreviewGpuRebuildDeps material_preview_gpu_rebuild_deps() {
        return {
            .device = viewport_device_.get(),
            .sdl_renderer = sdl_renderer_,
            .tool_filesystem = &tool_filesystem_,
            .assets = &assets_,
            .read_material_preview_shader_bytecode =
                [this](mirakana::AssetId shader, mirakana::ShaderSourceStage stage,
                       mirakana::ShaderCompileTarget target) {
                    return read_material_preview_shader_bytecode(shader, stage, target);
                },
        };
    }

    [[nodiscard]] mirakana::editor::MaterialPreviewGpuRenderDeps material_preview_gpu_render_deps() {
        return {
            .device = viewport_device_.get(),
            .sync_display_texture =
                [this](mirakana::RhiViewportSurface& surface, mirakana::editor::SdlViewportTexture& display_texture) {
                    update_display_texture_from_surface(surface, display_texture);
                },
        };
    }

    mirakana::editor::MaterialPreviewGpuCache& rebuild_material_preview_gpu_cache(mirakana::AssetId material) {
        return material_preview_gpu_.rebuild(material_preview_gpu_rebuild_deps(), material);
    }

    mirakana::editor::MaterialPreviewGpuCache& ensure_material_preview_gpu_cache(mirakana::AssetId material) {
        return material_preview_gpu_.ensure(material_preview_gpu_rebuild_deps(), material);
    }

    void render_material_preview_gpu_cache(mirakana::editor::MaterialPreviewGpuCache& cache) {
        cache.render(material_preview_gpu_render_deps());
    }

    [[nodiscard]] mirakana::editor::EditorRenderBackendAvailability editor_viewport_backend_availability() const {
        mirakana::editor::EditorRenderBackendAvailability availability{};
#if defined(MK_EDITOR_ENABLE_D3D12)
        if (viewport_shader_artifacts_.ready_for_d3d12() && mirakana::rhi::d3d12::compiled_with_windows_sdk()) {
            const auto probe = mirakana::rhi::d3d12::probe_runtime();
            availability.d3d12 = probe.hardware_device_supported || probe.warp_device_supported;
        }
#endif
#if defined(MK_EDITOR_ENABLE_VULKAN)
        const auto loader_probe = mirakana::rhi::vulkan::probe_runtime_loader();
        const bool loader_ready = loader_probe.runtime_loaded && loader_probe.get_instance_proc_addr_found;
        if (loader_ready && viewport_shader_artifacts_.ready_for_vulkan()) {
            availability.vulkan = true;
        }
#endif
        return availability;
    }

    void configure_viewport_render_backend() {
        const auto choice = mirakana::editor::choose_editor_render_backend(
            project_.render_backend, editor_viewport_backend_availability(),
            mirakana::editor::current_editor_render_backend_host());
        viewport_.set_render_backend_selection(choice);
    }

    [[nodiscard]] static std::unique_ptr<mirakana::rhi::IRhiDevice>
    create_viewport_rhi_device(mirakana::editor::EditorRenderBackend backend) {
#if defined(MK_EDITOR_ENABLE_D3D12)
        if (backend == mirakana::editor::EditorRenderBackend::d3d12) {
            return mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
                false,
                false,
            });
        }
#endif
#if defined(MK_EDITOR_ENABLE_VULKAN)
        if (backend == mirakana::editor::EditorRenderBackend::vulkan) {
            auto runtime_result =
                mirakana::rhi::vulkan::create_runtime_device({}, {}, {}, mirakana::rhi::SurfaceHandle{});
            if (!runtime_result.created) {
                return nullptr;
            }
            const auto mapping_plan = mirakana::rhi::vulkan::minimal_irhi_device_mapping_plan();
            if (!mapping_plan.supported) {
                return nullptr;
            }
            return mirakana::rhi::vulkan::create_rhi_device(std::move(runtime_result.device), mapping_plan);
        }
#endif
        (void)backend;
        return std::make_unique<mirakana::rhi::NullRhiDevice>();
    }

    void create_null_viewport_render_resources(mirakana::Extent2D extent) {
        auto device = std::make_unique<mirakana::rhi::NullRhiDevice>();
        auto surface = std::make_unique<mirakana::RhiViewportSurface>(mirakana::RhiViewportSurfaceDesc{
            device.get(),
            extent,
            mirakana::rhi::Format::rgba8_unorm,
            true,
            false,
        });
        const auto pipeline = create_viewport_graphics_pipeline(*device, surface->color_format());
        auto display_texture = std::make_unique<mirakana::editor::SdlViewportTexture>(sdl_renderer_, extent);

        reset_viewport_render_resources();
        viewport_device_ = std::move(device);
        viewport_surface_ = std::move(surface);
        viewport_pipeline_ = pipeline;
        viewport_display_texture_ = std::move(display_texture);
    }

    void recreate_viewport_render_resources(mirakana::Extent2D extent) {
        if (sdl_renderer_ == nullptr) {
            throw std::invalid_argument("editor viewport requires an SDL renderer");
        }

        const auto active_backend = viewport_.active_render_backend();
        try {
            auto device = create_viewport_rhi_device(active_backend);
            if (device == nullptr) {
                throw std::runtime_error("viewport RHI device creation failed");
            }

            std::optional<std::string> vertex_bytecode;
            std::optional<std::string> fragment_bytecode;
            if (active_backend == mirakana::editor::EditorRenderBackend::d3d12) {
                vertex_bytecode = read_viewport_shader_bytecode(mirakana::ShaderSourceStage::vertex,
                                                                mirakana::ShaderCompileTarget::d3d12_dxil);
                fragment_bytecode = read_viewport_shader_bytecode(mirakana::ShaderSourceStage::fragment,
                                                                  mirakana::ShaderCompileTarget::d3d12_dxil);
                if (!vertex_bytecode.has_value() || !fragment_bytecode.has_value()) {
                    throw std::runtime_error("D3D12 viewport shader bytecode is missing");
                }
            } else if (active_backend == mirakana::editor::EditorRenderBackend::vulkan) {
                vertex_bytecode = read_viewport_shader_bytecode(mirakana::ShaderSourceStage::vertex,
                                                                mirakana::ShaderCompileTarget::vulkan_spirv);
                fragment_bytecode = read_viewport_shader_bytecode(mirakana::ShaderSourceStage::fragment,
                                                                  mirakana::ShaderCompileTarget::vulkan_spirv);
                if (!vertex_bytecode.has_value() || !fragment_bytecode.has_value()) {
                    throw std::runtime_error("Vulkan viewport shader SPIR-V is missing");
                }
            }

            auto surface = std::make_unique<mirakana::RhiViewportSurface>(mirakana::RhiViewportSurfaceDesc{
                device.get(),
                extent,
                mirakana::rhi::Format::rgba8_unorm,
                true,
                active_backend == mirakana::editor::EditorRenderBackend::d3d12,
            });
            const auto pipeline = create_viewport_graphics_pipeline(
                *device, surface->color_format(), vertex_bytecode.has_value() ? &(*vertex_bytecode) : nullptr,
                fragment_bytecode.has_value() ? &(*fragment_bytecode) : nullptr);
            if (pipeline.value == 0) {
                throw std::runtime_error("viewport graphics pipeline creation failed");
            }
            auto display_texture = std::make_unique<mirakana::editor::SdlViewportTexture>(sdl_renderer_, extent);

            reset_viewport_render_resources();
            viewport_device_ = std::move(device);
            viewport_surface_ = std::move(surface);
            viewport_pipeline_ = pipeline;
            viewport_display_texture_ = std::move(display_texture);
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
            if (active_backend != mirakana::editor::EditorRenderBackend::null) {
                viewport_.set_render_backend_selection(mirakana::editor::EditorRenderBackendChoice{
                    project_.render_backend,
                    mirakana::editor::EditorRenderBackend::null,
                    false,
                    "Native viewport backend failed; using NullRhiDevice",
                });
            }
            create_null_viewport_render_resources(extent);
        }
    }

    void write_editor_asset_sources() {
        tool_filesystem_.write_text("source/builtin/default.texture",
                                    mirakana::serialize_texture_source_document(mirakana::TextureSourceDocument{
                                        .width = 64,
                                        .height = 64,
                                        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
                                        .bytes = {},
                                    }));
        tool_filesystem_.write_text("source/builtin/default.mesh",
                                    mirakana::serialize_mesh_source_document(mirakana::MeshSourceDocument{
                                        .vertex_count = 3,
                                        .index_count = 3,
                                        .has_normals = false,
                                        .has_uvs = false,
                                        .has_tangent_frame = false,
                                        .vertex_bytes = {},
                                        .index_bytes = {},
                                    }));
        tool_filesystem_.write_text("source/builtin/default.material",
                                    mirakana::serialize_material_definition(make_editor_default_material_definition()));
        tool_filesystem_.write_text("source/builtin/default.audio_source",
                                    mirakana::serialize_audio_source_document(mirakana::AudioSourceDocument{
                                        .sample_rate = 48000,
                                        .channel_count = 1,
                                        .frame_count = 4800,
                                        .sample_format = mirakana::AudioSourceSampleFormat::pcm16,
                                        .samples = {},
                                    }));
    }

    void execute_editor_asset_import() {
        try {
            if (asset_import_plan_.actions.empty()) {
                initialize_asset_pipeline();
            }
            write_editor_asset_sources();
            mirakana::ExternalAssetImportAdapters adapters;
            const auto result =
                mirakana::execute_asset_import_plan(tool_filesystem_, asset_import_plan_, adapters.options());
            asset_pipeline_.apply_import_execution_result(result);
            const auto added_assets = mirakana::editor::add_imported_asset_records(assets_, result);
            if (added_assets > 0) {
                refresh_content_browser_after_asset_registry_update();
            }
            reset_material_preview_gpu_cache();
            log_.log(result.succeeded() ? mirakana::LogLevel::info : mirakana::LogLevel::warn, "editor",
                     result.succeeded() ? "Asset import completed" : "Asset import failed");
        } catch (const std::exception& error) {
            std::vector<mirakana::editor::EditorAssetImportUpdate> updates;
            updates.reserve(asset_pipeline_.items().size());
            for (const auto& item : asset_pipeline_.items()) {
                updates.push_back(mirakana::editor::EditorAssetImportUpdate{item.asset, false, error.what()});
            }
            asset_pipeline_.apply_import_updates(updates);
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void execute_editor_asset_recook(std::vector<mirakana::AssetHotReloadRecookRequest> requests) {
        try {
            if (asset_import_plan_.actions.empty()) {
                initialize_asset_pipeline();
            }
            asset_pipeline_.apply_recook_requests(std::move(requests));
            write_editor_asset_sources();

            const auto& ready_requests = asset_pipeline_.recook_requests();
            const auto recook_plan = mirakana::build_asset_recook_plan(asset_import_plan_, ready_requests);
            mirakana::ExternalAssetImportAdapters adapters;
            const auto result = mirakana::execute_asset_import_plan(tool_filesystem_, recook_plan, adapters.options());
            asset_pipeline_.apply_import_execution_result(result);

            const auto added_assets = mirakana::editor::add_imported_asset_records(assets_, result);
            if (added_assets > 0) {
                refresh_content_browser_after_asset_registry_update();
            }
            reset_material_preview_gpu_cache();

            std::vector<mirakana::AssetHotReloadApplyResult> apply_results;
            apply_results.reserve(result.imported.size() + result.failures.size());
            for (const auto& imported : result.imported) {
                const auto* request = find_recook_request(ready_requests, imported.asset);
                if (request == nullptr) {
                    continue;
                }
                apply_results.push_back(mirakana::AssetHotReloadApplyResult{
                    mirakana::AssetHotReloadApplyResultKind::applied,
                    imported.asset,
                    imported.output_path,
                    request->current_revision,
                    request->current_revision,
                    {},
                });
            }
            for (const auto& failure : result.failures) {
                const auto* request = find_recook_request(ready_requests, failure.asset);
                if (request == nullptr) {
                    continue;
                }
                apply_results.push_back(mirakana::AssetHotReloadApplyResult{
                    mirakana::AssetHotReloadApplyResultKind::failed_rolled_back,
                    failure.asset,
                    failure.output_path,
                    request->current_revision,
                    request->previous_revision,
                    failure.diagnostic,
                });
            }
            asset_pipeline_.apply_hot_reload_results(std::move(apply_results));
            log_.log(result.succeeded() ? mirakana::LogLevel::info : mirakana::LogLevel::warn, "editor",
                     result.succeeded() ? "Asset recook completed" : "Asset recook failed");
        } catch (const std::exception& error) {
            asset_pipeline_.apply_hot_reload_results({
                mirakana::AssetHotReloadApplyResult{
                    mirakana::AssetHotReloadApplyResultKind::failed_rolled_back,
                    mirakana::AssetId{},
                    "asset recook",
                    0,
                    0,
                    error.what(),
                },
            });
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
        }
    }

    void complete_demo_shader_compile(bool cache_hit) {
        std::vector<mirakana::editor::EditorShaderCompileUpdate> updates;
        updates.reserve(shader_compiles_.items().size());
        for (const auto& item : shader_compiles_.items()) {
            updates.push_back(mirakana::editor::EditorShaderCompileUpdate{
                item.shader,
                item.output_path,
                cache_hit ? mirakana::editor::EditorShaderCompileStatus::cached
                          : mirakana::editor::EditorShaderCompileStatus::compiled,
                cache_hit ? "cache hit" : "compiled",
                cache_hit,
            });
        }
        shader_compiles_.apply_updates(updates);
        refresh_viewport_shader_artifacts();
        refresh_material_preview_shader_artifacts();
        log_.log(mirakana::LogLevel::info, "editor",
                 cache_hit ? "Shader compile cache hit" : "Shader compile completed");
    }

    void compile_editor_shaders() {
        if (shader_compile_requests_.empty() || material_preview_shader_compile_requests_.empty()) {
            initialize_shader_compile_state();
        }
        const auto requests = all_shader_compile_requests();
        for (const auto& request : requests) {
            tool_filesystem_.write_text(request.source.source_path, editor_shader_source_for_request(request));
        }

        std::vector<mirakana::editor::EditorShaderCompileExecution> executions;
        executions.reserve(requests.size());

#if defined(_WIN32)
        mirakana::Win32ProcessRunner process_runner;
        mirakana::ShaderToolProcessRunner shader_runner(process_runner, mirakana::ShaderToolExecutionPolicy{
                                                                            project_.shader_tool.artifact_output_root,
                                                                            project_.shader_tool.working_directory,
                                                                            project_.shader_tool.executable,
                                                                        });
        for (const auto& request : requests) {
            try {
                const auto result = mirakana::execute_shader_compile_action(
                    tool_filesystem_, shader_runner,
                    mirakana::ShaderCompileExecutionRequest{
                        request,
                        mirakana::ShaderToolDescriptor{mirakana::ShaderToolKind::dxc, project_.shader_tool.executable,
                                                       "project-settings"},
                        project_.shader_tool.cache_index_path,
                        true,
                        false,
                    });
                executions.push_back(mirakana::editor::EditorShaderCompileExecution{request, result});
            } catch (const std::exception& error) {
                auto artifact = mirakana::make_shader_compile_command(request).artifact;
                executions.push_back(mirakana::editor::EditorShaderCompileExecution{
                    request,
                    mirakana::ShaderCompileExecutionResult{
                        mirakana::ShaderToolRunResult{-1, error.what(), {}, {}, std::move(artifact)},
                        {},
                        mirakana::shader_artifact_provenance_path(
                            mirakana::make_shader_compile_command(request).artifact),
                        false,
                        false,
                    },
                });
            }
        }
#else
        for (const auto& request : requests) {
            auto artifact = mirakana::make_shader_compile_command(request).artifact;
            executions.push_back(mirakana::editor::EditorShaderCompileExecution{
                request,
                mirakana::ShaderCompileExecutionResult{
                    mirakana::ShaderToolRunResult{-1,
                                                  "shader compile process runner is not available on this platform",
                                                  {},
                                                  {},
                                                  std::move(artifact)},
                    {},
                    mirakana::shader_artifact_provenance_path(mirakana::make_shader_compile_command(request).artifact),
                    false,
                    false,
                },
            });
        }
#endif

        shader_compiles_.apply_execution_results(executions);
        refresh_viewport_shader_artifacts();
        refresh_material_preview_shader_artifacts();
        if (shader_compiles_.failed_count() == 0) {
            configure_viewport_render_backend();
            if (viewport_.ready()) {
                recreate_viewport_render_resources(current_viewport_extent());
            }
            log_.log(mirakana::LogLevel::info, "editor", "Shader compile action completed");
        } else {
            log_.log(mirakana::LogLevel::warn, "editor", "Shader compile action failed");
        }
    }

    void simulate_asset_hot_reload() {
        const auto texture_id = mirakana::AssetId::from_name("editor.default.texture");
        const auto previous_revision = asset_hot_reload_revision_;
        ++asset_hot_reload_revision_;
        const auto event = mirakana::AssetHotReloadEvent{
            mirakana::AssetHotReloadEventKind::modified,
            texture_id,
            "builtin/default.texture",
            previous_revision,
            asset_hot_reload_revision_,
            128,
            160,
        };
        const auto request = mirakana::AssetHotReloadRecookRequest{
            texture_id,
            texture_id,
            event.path,
            event.kind,
            mirakana::AssetHotReloadRecookReason::source_modified,
            event.previous_revision,
            event.current_revision,
            asset_hot_reload_revision_ + 2,
        };
        asset_pipeline_.apply_hot_reload_events({event});
        execute_editor_asset_recook({request});
        log_.log(mirakana::LogLevel::info, "editor", "Asset hot reload event received");
    }

    [[nodiscard]] bool save_current_scene_to_path(std::string_view path) {
        try {
            mirakana::editor::save_scene_authoring_document(project_store_, path, scene_document_);
            dirty_state_.mark_saved();
            log_.log(mirakana::LogLevel::info, "editor", "Scene saved");
            return true;
        } catch (const std::exception& error) {
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
            return false;
        }
    }

    void save_current_scene() {
        (void)save_current_scene_to_path(project_paths_.scene_path);
    }

    void apply_package_registration(const mirakana::editor::ScenePackageRegistrationApplyPlan& plan) {
        const auto result = mirakana::editor::apply_scene_package_registration_to_manifest(project_store_, plan);
        package_registration_apply_status_ = result.diagnostic;
        if (result.applied) {
            manifest_runtime_package_files_ = result.runtime_package_files;
            log_.log(mirakana::LogLevel::info, "editor", "Package registration applied");
        } else {
            log_.log(mirakana::LogLevel::warn, "editor", result.diagnostic);
        }
    }

    void replace_scene_document(mirakana::editor::SceneAuthoringDocument document, bool mark_dirty) {
        if (play_session_.active()) {
            (void)play_session_.stop();
            (void)viewport_.stop();
        }
        history_.clear();
        scene_document_ = std::move(document);
        sync_scene_rename_buffer();
        dirty_state_.reset_clean();
        if (mark_dirty) {
            dirty_state_.mark_dirty();
        }
    }

    [[nodiscard]] bool open_project_bundle_from_paths(const mirakana::editor::ProjectBundlePaths& paths,
                                                      std::string* diagnostic = nullptr) {
        try {
            const auto bundle = mirakana::editor::load_project_bundle(project_store_, paths);
            project_paths_ = paths;
            project_ = bundle.project;
            workspace_ = bundle.workspace;
            reset_prefab_variant_document();
            reset_ai_evidence_import_state();
            reset_resource_capture_request_state();
            reset_project_settings_inputs_from_project();
            auto document = mirakana::editor::load_scene_authoring_document(project_store_, project_paths_.scene_path);
            replace_scene_document(std::move(document), false);
            initialize_asset_pipeline();
            refresh_content_browser_from_project_or_cooked_assets();
            initialize_shader_compile_state();
            configure_viewport_render_backend();
            if (viewport_.ready()) {
                recreate_viewport_render_resources(current_viewport_extent());
            }
            log_.log(mirakana::LogLevel::info, "editor", "Project bundle loaded");
            return true;
        } catch (const std::exception& error) {
            if (diagnostic != nullptr) {
                *diagnostic = error.what();
            }
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
            return false;
        }
    }

    void open_project_bundle() {
        (void)open_project_bundle_from_paths(project_paths_);
    }

    [[nodiscard]] bool save_project_bundle_to_paths(const mirakana::editor::ProjectBundlePaths& paths,
                                                    std::string* diagnostic = nullptr) {
        try {
            mirakana::editor::save_project_bundle(project_store_, paths, project_, workspace_,
                                                  mirakana::serialize_scene(active_scene()));
            scene_document_.mark_saved();
            dirty_state_.mark_saved();
            log_.log(mirakana::LogLevel::info, "editor", "Project bundle saved");
            return true;
        } catch (const std::exception& error) {
            if (diagnostic != nullptr) {
                *diagnostic = error.what();
            }
            log_.log(mirakana::LogLevel::warn, "editor", error.what());
            return false;
        }
    }

    void save_project_bundle() {
        (void)save_project_bundle_to_paths(project_paths_);
    }

    void sync_scene_rename_buffer() {
        if (const auto* node = active_scene().find_node(selected_node())) {
            copy_to_buffer(scene_rename_buffer_, node->name);
        } else {
            scene_rename_buffer_[0] = '\0';
        }
    }

    mirakana::editor::ProjectDocument project_;
    mirakana::editor::ProjectSettingsDraft project_settings_draft_;
    mirakana::editor::Workspace workspace_;
    mirakana::editor::CommandRegistry commands_;
    mirakana::editor::FileTextStore project_store_{"."};
    mirakana::editor::ProjectBundlePaths project_paths_{
        "GameEngine.geproject",
        "GameEngine.geworkspace",
        "scenes/start.scene",
    };
    std::optional<mirakana::FileDialogId> project_open_dialog_id_;
    std::optional<mirakana::FileDialogId> project_save_dialog_id_;
    mirakana::editor::EditorProjectFileDialogModel project_open_dialog_{};
    mirakana::editor::EditorProjectFileDialogModel project_save_dialog_{
        .mode = mirakana::editor::EditorProjectFileDialogMode::save,
        .status_label = {},
        .selected_path = {},
        .accepted = false,
        .rows = {},
        .diagnostics = {},
    };
    std::vector<std::string> manifest_runtime_package_files_{std::string(default_package_index_path)};
    std::string package_registration_apply_status_;
    mirakana::editor::DocumentDirtyState dirty_state_;
    mirakana::WindowDesc window_desc_;
    mirakana::editor::SceneAuthoringDocument scene_document_;
    mirakana::editor::EditorPlaySession play_session_;
    std::array<char, 1024> game_module_driver_path_{};
    std::optional<mirakana::DynamicLibrary> game_module_driver_library_;
    std::unique_ptr<mirakana::editor::IEditorPlaySessionDriver> game_module_driver_;
    std::string game_module_driver_status_{"not_loaded"};
    std::string game_module_driver_diagnostic_{"No game module driver loaded"};
    mirakana::editor::UndoStack history_;
    std::array<char, 128> scene_rename_buffer_{};
    std::vector<mirakana::editor::SceneReparentParentOption> scene_reparent_parent_options_;
    int scene_reparent_combo_idx_{0};
    bool scene_prefab_refresh_keep_local_children_{false};
    bool scene_prefab_refresh_keep_stale_source_nodes_{false};
    bool scene_prefab_refresh_keep_nested_prefab_instances_{false};
    bool scene_prefab_refresh_apply_nested_prefab_propagation_{false};
    std::unordered_set<std::uint32_t> scene_prefab_batch_refresh_node_ids_;
    std::optional<ScenePrefabRefreshReviewState> scene_prefab_refresh_review_;
    std::optional<mirakana::FileDialogId> scene_open_dialog_id_;
    std::optional<mirakana::FileDialogId> scene_save_dialog_id_;
    mirakana::editor::EditorSceneFileDialogModel scene_open_dialog_{};
    mirakana::editor::EditorSceneFileDialogModel scene_save_dialog_{
        .mode = mirakana::editor::EditorSceneFileDialogMode::save,
        .accepted = false,
        .status_label = {},
        .selected_path = {},
        .rows = {},
        .diagnostics = {},
    };
    std::optional<mirakana::editor::PrefabVariantAuthoringDocument> prefab_variant_document_;
    mirakana::editor::UndoStack prefab_variant_history_;
    std::array<char, 256> prefab_variant_path_{copy_chars_to_fixed_array<256>("assets/prefabs/selected.prefabvariant")};
    std::array<char, 128> prefab_variant_name_{copy_chars_to_fixed_array<128>("selected_prefab_variant")};
    std::optional<mirakana::FileDialogId> prefab_variant_open_dialog_id_;
    std::optional<mirakana::FileDialogId> prefab_variant_save_dialog_id_;
    mirakana::editor::EditorPrefabVariantFileDialogModel prefab_variant_open_dialog_{};
    mirakana::editor::EditorPrefabVariantFileDialogModel prefab_variant_save_dialog_{
        .mode = mirakana::editor::EditorPrefabVariantFileDialogMode::save,
        .accepted = false,
        .status_label = {},
        .selected_path = {},
        .rows = {},
        .diagnostics = {},
    };
    int prefab_variant_node_index_{1};
    std::array<char, 128> prefab_variant_name_override_{copy_chars_to_fixed_array<128>("VariantNode")};
    std::array<float, 3> prefab_variant_position_{0.0F, 0.0F, 0.0F};
    std::array<float, 3> prefab_variant_rotation_degrees_{0.0F, 0.0F, 0.0F};
    std::array<float, 3> prefab_variant_scale_{1.0F, 1.0F, 1.0F};
    mirakana::AssetRegistry assets_;
    mirakana::editor::ContentBrowserState content_browser_;
    mirakana::editor::EditorSourceRegistryBrowserRefreshResult source_registry_browser_refresh_{};
    bool content_browser_source_registry_loaded_{false};
    mirakana::runtime::RuntimeInputActionMap input_rebinding_base_actions_;
    mirakana::runtime::RuntimeInputRebindingProfile input_rebinding_profile_;
    mirakana::VirtualInput editor_keyboard_input_;
    mirakana::VirtualPointerInput editor_pointer_input_;
    mirakana::VirtualGamepadInput editor_gamepad_input_;
    std::uint64_t editor_input_frame_{0};
    std::optional<InputRebindingCaptureTarget> input_rebinding_capture_target_;
    std::optional<mirakana::editor::EditorInputRebindingCaptureModel> input_rebinding_last_capture_;
    std::optional<mirakana::editor::EditorInputRebindingAxisCaptureModel> input_rebinding_last_axis_capture_;
    std::string input_rebinding_capture_status_{"No capture armed"};
    std::array<char, 512> input_rebinding_profile_store_path_{
        copy_chars_to_fixed_array<512>("settings/runtime_input_rebinding.inputrebinding")};
    std::string input_rebinding_persistence_status_{"No file operation performed yet"};
    std::vector<std::string> input_rebinding_persistence_diagnostics_;
    std::array<char, 4096> ai_evidence_import_text_{};
    std::vector<mirakana::editor::EditorAiPlaytestValidationEvidenceRow> ai_playtest_evidence_rows_;
    std::vector<std::string> ai_acknowledged_host_gate_recipe_ids_;
    std::optional<mirakana::editor::EditorRuntimeScenePackageValidationExecutionResult>
        runtime_scene_package_validation_result_;
    bool runtime_host_playtest_host_gate_acknowledged_{false};
    std::string runtime_host_playtest_status_{"not_run"};
    std::optional<int> runtime_host_playtest_exit_code_;
    std::string runtime_host_playtest_summary_{"No runtime host playtest executed"};
    std::string runtime_host_playtest_stdout_;
    std::string runtime_host_playtest_stderr_;
    std::vector<std::string> resource_acknowledged_capture_request_ids_;
    bool pix_host_helper_last_run_valid_{false};
    bool pix_host_helper_last_run_succeeded_{false};
    std::optional<int> pix_host_helper_last_exit_code_;
    std::string pix_host_helper_last_session_dir_;
    std::string pix_host_helper_last_stdout_summary_;
    std::string pix_host_helper_last_stderr_summary_;
    std::optional<bool> pix_host_helper_last_run_skip_launch_;
    mirakana::AssetImportPlan asset_import_plan_;
    mirakana::editor::AssetPipelineState asset_pipeline_;
    std::optional<mirakana::FileDialogId> asset_import_open_dialog_id_;
    mirakana::editor::EditorContentBrowserImportOpenDialogModel asset_import_open_dialog_{};
    std::string asset_import_open_dialog_status_;
    std::vector<std::string> asset_import_pending_project_source_paths_;
    std::vector<AssetImportExternalSourceCopySelection> asset_import_pending_external_copies_;
    mirakana::editor::EditorContentBrowserImportExternalSourceCopyModel asset_import_external_copy_{};
    mirakana::editor::ShaderCompileState shader_compiles_;
    mirakana::editor::ShaderToolDiscoveryState shader_tool_discovery_;
    mirakana::editor::ViewportShaderArtifactState viewport_shader_artifacts_;
    mirakana::editor::ViewportShaderArtifactState material_preview_shader_artifacts_;
    std::vector<mirakana::ShaderCompileRequest> shader_compile_requests_;
    std::vector<mirakana::ShaderCompileRequest> material_preview_shader_compile_requests_;
    mirakana::AnimationAuthoredTimelineDesc editor_timeline_{
        2.0F,
        true,
        {
            mirakana::AnimationTimelineEventTrackDesc{
                "gameplay",
                {
                    mirakana::AnimationTimelineEventDesc{0.25F, "spawn", "preview"},
                    mirakana::AnimationTimelineEventDesc{1.25F, "state", "resolve"},
                },
            },
            mirakana::AnimationTimelineEventTrackDesc{
                "audio",
                {
                    mirakana::AnimationTimelineEventDesc{0.5F, "play", "step"},
                    mirakana::AnimationTimelineEventDesc{1.5F, "play", "impact"},
                },
            },
        },
    };
    mirakana::RootedFileSystem tool_filesystem_{"."};
    mirakana::editor::ViewportState viewport_;
    SDL_Renderer* sdl_renderer_{nullptr};
    mirakana::IFileDialogService* file_dialogs_{nullptr};
    std::unique_ptr<mirakana::rhi::IRhiDevice> viewport_device_;
    mirakana::rhi::GraphicsPipelineHandle viewport_pipeline_;
    std::unique_ptr<mirakana::RhiViewportSurface> viewport_surface_;
    std::unique_ptr<mirakana::editor::SdlViewportTexture> viewport_display_texture_;
    mirakana::editor::MaterialPreviewGpuCache material_preview_gpu_;
    mirakana::SceneMaterialPalette viewport_materials_;
    mirakana::SceneRenderSubmitResult last_viewport_submit_;
    mirakana::RingBufferLogger log_{128};
    mirakana::DiagnosticsRecorder profiler_recorder_{1024};
    mirakana::SteadyProfileClock profiler_clock_;
    std::string profiler_trace_export_status_;
    std::array<char, 256> profiler_trace_export_path_{
        copy_chars_to_fixed_array<256>("diagnostics/editor-profiler-trace.json")};
    std::optional<mirakana::FileDialogId> profiler_trace_save_dialog_id_;
    mirakana::editor::EditorProfilerTraceSaveDialogModel profiler_trace_save_dialog_{};
    std::string profiler_trace_import_status_;
    std::array<char, 256> profiler_trace_import_path_{
        copy_chars_to_fixed_array<256>("diagnostics/editor-profiler-trace.json")};
    std::optional<mirakana::FileDialogId> profiler_trace_open_dialog_id_;
    mirakana::editor::EditorProfilerTraceOpenDialogModel profiler_trace_open_dialog_{};
    mirakana::editor::EditorProfilerTraceFileImportResult profiler_trace_file_import_{};
    std::array<char, 8192> profiler_trace_import_payload_{};
    mirakana::editor::EditorProfilerTraceImportReviewModel profiler_trace_import_review_ =
        mirakana::editor::make_editor_profiler_trace_import_review_model({});
    bool show_command_palette_{false};
    std::array<char, 128> command_palette_query_{};
    mirakana::editor::ProjectCreationWizard project_wizard_{mirakana::editor::ProjectCreationWizard::begin()};
    bool show_project_wizard_{false};
    std::array<char, 128> asset_filter_{};
    std::array<char, 512> gltf_mesh_inspect_path_{};
    bool gltf_mesh_inspect_follow_assets_selection_{false};
    mirakana::GltfMeshInspectReport gltf_mesh_inspect_report_{};
    std::string gltf_mesh_inspect_ui_snapshot_;
    int asset_kind_filter_index_{0};
    std::array<char, 128> project_wizard_name_{copy_chars_to_fixed_array<128>("Untitled Project")};
    std::array<char, 128> project_wizard_root_{copy_chars_to_fixed_array<128>("games/untitled-project")};
    std::array<char, 128> project_wizard_assets_{copy_chars_to_fixed_array<128>("assets")};
    std::array<char, 128> project_wizard_source_registry_{
        copy_chars_to_fixed_array<128>("source/assets/package.geassets")};
    std::array<char, 128> project_wizard_manifest_{copy_chars_to_fixed_array<128>("game.agent.json")};
    std::array<char, 128> project_wizard_scene_{copy_chars_to_fixed_array<128>("scenes/start.scene")};
    std::array<char, 256> project_shader_tool_executable_{copy_chars_to_fixed_array<256>("dxc")};
    std::array<char, 256> project_shader_tool_working_directory_{copy_chars_to_fixed_array<256>(".")};
    std::array<char, 256> project_shader_artifact_output_root_{copy_chars_to_fixed_array<256>("out/editor/shaders")};
    std::array<char, 256> project_shader_cache_index_{
        copy_chars_to_fixed_array<256>("out/editor/shaders/shader-cache.gecache")};
    int project_render_backend_index_{0};
    float timeline_playhead_seconds_{0.0F};
    bool timeline_playing_{false};
    std::uint64_t asset_hot_reload_revision_{1};
    std::uint64_t profiler_frame_index_{0};
};

} // namespace

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    try {
        mirakana::SdlRuntime sdl;
        mirakana::SdlWindow window(mirakana::WindowDesc{"GameEngine Editor", mirakana::WindowExtent{1280, 720}});

#if defined(MK_EDITOR_ENABLE_D3D12)
        (void)SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d12,direct3d11,opengl,software");
#endif

        SDL_Window* sdl_window = native_sdl_window(window);
        SDL_Renderer* renderer = SDL_CreateRenderer(sdl_window, nullptr);
        if (renderer == nullptr) {
            return 1;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();
        ImGui_ImplSDL3_InitForSDLRenderer(sdl_window, renderer);
        ImGui_ImplSDLRenderer3_Init(renderer);

        mirakana::SdlFileDialogService file_dialogs(&window);
        EditorState editor(renderer, &file_dialogs);

        while (window.is_open()) {
            editor.begin_input_frame();
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL3_ProcessEvent(&event);
                window.handle_event(
                    mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, window.extent()),
                    &editor.keyboard_input(), &editor.pointer_input(), &editor.gamepad_input());
            }

            ImGui_ImplSDLRenderer3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            editor.draw();

            ImGui::Render();
            SDL_SetRenderDrawColor(renderer, 15, 18, 24, 255);
            SDL_RenderClear(renderer);
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
            SDL_RenderPresent(renderer);
        }

        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyRenderer(renderer);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "editor fatal: %s\n", error.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "editor fatal: unknown exception\n");
        return 1;
    }
}
