// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/material.hpp"
#include "mirakana/renderer/rhi_frame_renderer.hpp"
#include "mirakana/renderer/rhi_postprocess_frame_renderer.hpp"
#include "mirakana/renderer/shadow_map.hpp"
#include "mirakana/rhi/metal/metal_backend.hpp"
#include "mirakana/rhi/vulkan/vulkan_backend.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

#if defined(_WIN32)
class HiddenVulkanTestWindow final {
  public:
    HiddenVulkanTestWindow() {
        instance_ = GetModuleHandleW(nullptr);

        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.lpfnWndProc = DefWindowProcW;
        window_class.hInstance = instance_;
        window_class.lpszClassName = class_name;
        registered_ = RegisterClassExW(&window_class) != 0;

        hwnd_ = CreateWindowExW(0, class_name, L"GameEngineVulkanTestWindow", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                CW_USEDEFAULT, 128, 128, nullptr, nullptr, instance_, nullptr);
    }

    HiddenVulkanTestWindow(const HiddenVulkanTestWindow&) = delete;
    HiddenVulkanTestWindow& operator=(const HiddenVulkanTestWindow&) = delete;

    ~HiddenVulkanTestWindow() {
        if (hwnd_ != nullptr) {
            DestroyWindow(hwnd_);
        }
        if (registered_) {
            UnregisterClassW(class_name, instance_);
        }
    }

    [[nodiscard]] bool valid() const noexcept {
        return hwnd_ != nullptr;
    }

    [[nodiscard]] HWND hwnd() const noexcept {
        return hwnd_;
    }

  private:
    static constexpr const wchar_t* class_name{L"GameEngineVulkanTestWindowClass"};
    HINSTANCE instance_{nullptr};
    HWND hwnd_{nullptr};
    bool registered_{false};
};
#endif

[[nodiscard]] bool has_non_zero_byte(const std::vector<std::byte>& bytes) noexcept {
    return std::ranges::any_of(bytes, [](const auto byte) { return std::to_integer<unsigned int>(byte) != 0U; });
}

[[nodiscard]] bool has_non_zero_byte(const std::vector<std::uint8_t>& bytes) noexcept {
    return std::ranges::any_of(bytes, [](const auto byte) { return byte != 0U; });
}

[[nodiscard]] mirakana::rhi::vulkan::VulkanRhiDeviceMappingPlan ready_vulkan_rhi_mapping_plan() {
    mirakana::rhi::vulkan::VulkanRhiDeviceMappingDesc desc;
    desc.command_pool_ready = true;
    desc.swapchain.supported = true;
    desc.dynamic_rendering.supported = true;
    desc.frame_synchronization.supported = true;
    desc.vertex_shader.valid = true;
    desc.fragment_shader.valid = true;
    desc.compute_shader.valid = true;
    desc.descriptor_binding_ready = true;
    desc.compute_dispatch_ready = true;
    desc.visible_clear_readback_ready = true;
    desc.visible_draw_readback_ready = true;
    desc.visible_texture_sampling_readback_ready = true;
    desc.visible_depth_readback_ready = true;
    return mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
}

struct SpirvArtifactFromEnvironment {
    bool configured{false};
    std::vector<std::uint32_t> words;
    std::string diagnostic;
};

[[nodiscard]] std::string environment_variable_value(const char* name) {
#if defined(_WIN32)
    char* value{nullptr};
    std::size_t value_size{0};
    if (_dupenv_s(&value, &value_size, name) != 0 || value == nullptr) {
        return {};
    }

    std::string result{value};
    std::free(value);
    return result;
#else
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
#endif
}

[[nodiscard]] SpirvArtifactFromEnvironment load_spirv_artifact_from_environment(const char* environment_variable) {
    const auto path = environment_variable_value(environment_variable);
    if (path.empty()) {
        return SpirvArtifactFromEnvironment{.configured = false, .words = {}, .diagnostic = "not configured"};
    }

    std::ifstream input{path, std::ios::binary | std::ios::ate};
    if (!input) {
        return SpirvArtifactFromEnvironment{
            .configured = true, .words = {}, .diagnostic = std::string{"unable to open "} + environment_variable};
    }

    const auto size = input.tellg();
    if (size <= 0 || size % static_cast<std::streamoff>(sizeof(std::uint32_t)) != 0) {
        return SpirvArtifactFromEnvironment{.configured = true,
                                            .words = {},
                                            .diagnostic =
                                                std::string{"invalid SPIR-V byte size in "} + environment_variable};
    }

    input.seekg(0, std::ios::beg);
    std::vector<std::uint32_t> words(
        static_cast<std::size_t>(size / static_cast<std::streamoff>(sizeof(std::uint32_t))));
    input.read(reinterpret_cast<char*>(words.data()), size);
    if (!input) {
        return SpirvArtifactFromEnvironment{
            .configured = true, .words = {}, .diagnostic = std::string{"failed to read "} + environment_variable};
    }

    return SpirvArtifactFromEnvironment{.configured = true, .words = std::move(words), .diagnostic = "loaded"};
}

void append_le_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
}

void append_le_f32(std::vector<std::uint8_t>& bytes, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    append_le_u32(bytes, bits);
}

[[nodiscard]] float read_le_f32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    std::uint32_t bits = 0;
    bits |= static_cast<std::uint32_t>(bytes.at(offset + 0));
    bits |= static_cast<std::uint32_t>(bytes.at(offset + 1)) << 8U;
    bits |= static_cast<std::uint32_t>(bytes.at(offset + 2)) << 16U;
    bits |= static_cast<std::uint32_t>(bytes.at(offset + 3)) << 24U;

    float value = 0.0F;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}

[[nodiscard]] bool nearly_equal(float left, float right, float tolerance = 0.0001F) noexcept {
    return std::abs(left - right) <= tolerance;
}

[[nodiscard]] std::string hex_encode(const std::vector<std::uint8_t>& bytes) {
    constexpr auto digits = std::string_view{"0123456789abcdef"};
    std::string encoded;
    encoded.reserve(bytes.size() * 2U);
    for (const auto byte : bytes) {
        encoded.push_back(digits[(byte >> 4U) & 0x0fU]);
        encoded.push_back(digits[byte & 0x0fU]);
    }
    return encoded;
}

[[nodiscard]] std::string vulkan_runtime_scene_texture_payload(mirakana::AssetId texture) {
    return "format=GameEngine.CookedTexture.v1\n"
           "asset.id=" +
           std::to_string(texture.value) +
           "\n"
           "asset.kind=texture\n"
           "texture.width=1\n"
           "texture.height=1\n"
           "texture.pixel_format=rgba8_unorm\n"
           "texture.source_bytes=4\n"
           "texture.data_hex=" +
           hex_encode(std::vector<std::uint8_t>{64, 200, 80, 255}) + "\n";
}

[[nodiscard]] std::string vulkan_runtime_scene_mesh_payload(mirakana::AssetId mesh) {
    std::vector<std::uint8_t> vertex_bytes;
    vertex_bytes.reserve(36);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, 0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, 0.75F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);

    std::vector<std::uint8_t> index_bytes;
    index_bytes.reserve(12);
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    return "format=GameEngine.CookedMesh.v2\n"
           "asset.id=" +
           std::to_string(mesh.value) +
           "\n"
           "asset.kind=mesh\n"
           "mesh.vertex_count=3\n"
           "mesh.index_count=3\n"
           "mesh.has_normals=false\n"
           "mesh.has_uvs=false\n"
           "mesh.has_tangent_frame=false\n"
           "mesh.vertex_data_hex=" +
           hex_encode(vertex_bytes) +
           "\n"
           "mesh.index_data_hex=" +
           hex_encode(index_bytes) + "\n";
}

[[nodiscard]] std::string vulkan_runtime_scene_material_payload(mirakana::AssetId material, mirakana::AssetId texture) {
    return mirakana::serialize_material_definition(mirakana::MaterialDefinition{
        .id = material,
        .name = "VulkanRuntimeSceneMaterial",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.5F, 0.5F, 1.0F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 1.0F,
            },
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture}},
        .double_sided = false,
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_vulkan_runtime_scene_package(mirakana::AssetId mesh, mirakana::AssetId material, mirakana::AssetId texture) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/vulkan_runtime_scene_base_color.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = vulkan_runtime_scene_texture_payload(texture),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/vulkan_runtime_scene_triangle.mesh",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = vulkan_runtime_scene_mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{3},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/vulkan_runtime_scene_material.material",
            .content_hash = 3,
            .source_revision = 1,
            .dependencies = {texture},
            .content = vulkan_runtime_scene_material_payload(material, texture),
        },
    });
}

[[nodiscard]] mirakana::Scene make_vulkan_runtime_scene(mirakana::AssetId mesh, mirakana::AssetId material) {
    mirakana::Scene scene("VulkanRuntimeScene");
    const auto node = scene.create_node("Triangle");
    mirakana::SceneNodeComponents components;
    components.mesh_renderer = mirakana::MeshRendererComponent{.mesh = mesh, .material = material, .visible = true};
    scene.set_components(node, components);
    return scene;
}

template <typename T>
inline constexpr bool has_legacy_descriptor_set_layout_count_v = requires { &T::descriptor_set_layout_count; };

} // namespace

MK_TEST("vulkan backend scaffold exposes api independent capability and probe contract") {
    const auto capabilities = mirakana::rhi::vulkan::capabilities();
    const auto plan = mirakana::rhi::vulkan::probe_plan();
    const auto missing_runtime = mirakana::rhi::vulkan::make_probe_result(
        mirakana::rhi::RhiHostPlatform::windows, mirakana::rhi::BackendProbeStatus::missing_runtime,
        "Vulkan loader unavailable");

    MK_REQUIRE(mirakana::rhi::vulkan::backend_kind() == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(mirakana::rhi::vulkan::backend_name() == "vulkan");
    MK_REQUIRE(capabilities.shader_format == mirakana::rhi::ShaderArtifactFormat::spirv);
    MK_REQUIRE(capabilities.explicit_queue_family_selection);
    MK_REQUIRE(mirakana::rhi::vulkan::supports_host(mirakana::rhi::RhiHostPlatform::windows));
    MK_REQUIRE(mirakana::rhi::vulkan::supports_host(mirakana::rhi::RhiHostPlatform::linux));
    MK_REQUIRE(mirakana::rhi::vulkan::supports_host(mirakana::rhi::RhiHostPlatform::android));
    MK_REQUIRE(!mirakana::rhi::vulkan::supports_host(mirakana::rhi::RhiHostPlatform::macos));
    MK_REQUIRE(plan.backend == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(plan.steps[2] == mirakana::rhi::BackendProbeStep::enumerate_physical_devices);
    MK_REQUIRE(plan.steps[3] == mirakana::rhi::BackendProbeStep::query_queue_families);
    MK_REQUIRE(missing_runtime.status == mirakana::rhi::BackendProbeStatus::missing_runtime);
    MK_REQUIRE(!missing_runtime.capabilities.native_device);
    MK_REQUIRE(missing_runtime.diagnostic == "Vulkan loader unavailable");
}

MK_TEST("vulkan backend loader probe classifies runtime and symbol availability") {
    const auto missing_runtime = mirakana::rhi::vulkan::make_loader_probe_result(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::RhiHostPlatform::windows}, false, false);
    const auto missing_symbol = mirakana::rhi::vulkan::make_loader_probe_result(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::RhiHostPlatform::windows}, true, false);
    const auto available = mirakana::rhi::vulkan::make_loader_probe_result(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::RhiHostPlatform::windows}, true, true);
    const auto unsupported = mirakana::rhi::vulkan::make_loader_probe_result(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::RhiHostPlatform::macos}, true, true);

    MK_REQUIRE(mirakana::rhi::vulkan::default_runtime_library_name(mirakana::rhi::RhiHostPlatform::windows) ==
               "vulkan-1.dll");
    MK_REQUIRE(mirakana::rhi::vulkan::default_runtime_library_name(mirakana::rhi::RhiHostPlatform::linux) ==
               "libvulkan.so.1");
    MK_REQUIRE(missing_runtime.probe.status == mirakana::rhi::BackendProbeStatus::missing_runtime);
    MK_REQUIRE(!missing_runtime.runtime_loaded);
    MK_REQUIRE(!missing_runtime.get_instance_proc_addr_found);
    MK_REQUIRE(missing_runtime.runtime_library == "vulkan-1.dll");
    MK_REQUIRE(missing_runtime.probe.diagnostic == "Vulkan runtime library could not be loaded");

    MK_REQUIRE(missing_symbol.probe.status == mirakana::rhi::BackendProbeStatus::missing_runtime);
    MK_REQUIRE(missing_symbol.runtime_loaded);
    MK_REQUIRE(!missing_symbol.get_instance_proc_addr_found);
    MK_REQUIRE(missing_symbol.probe.diagnostic == "Vulkan loader missing vkGetInstanceProcAddr");

    MK_REQUIRE(available.probe.status == mirakana::rhi::BackendProbeStatus::available);
    MK_REQUIRE(available.probe.capabilities.native_device);
    MK_REQUIRE(available.runtime_loaded);
    MK_REQUIRE(available.get_instance_proc_addr_found);

    MK_REQUIRE(unsupported.probe.status == mirakana::rhi::BackendProbeStatus::unsupported_host);
    MK_REQUIRE(!unsupported.probe.capabilities.native_device);
}

MK_TEST("vulkan backend loader probe can inspect the current host without exposing native handles") {
    const auto result = mirakana::rhi::vulkan::probe_runtime_loader();
    const auto host = mirakana::rhi::current_rhi_host_platform();

    MK_REQUIRE(result.probe.backend == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(result.probe.host == host);
    if (mirakana::rhi::vulkan::supports_host(host)) {
        MK_REQUIRE(!result.runtime_library.empty());
    } else {
        MK_REQUIRE(result.runtime_library.empty());
        MK_REQUIRE(result.probe.status == mirakana::rhi::BackendProbeStatus::unsupported_host);
    }
    MK_REQUIRE(!result.probe.diagnostic.empty());
}

MK_TEST("vulkan device selection prefers suitable discrete devices with graphics and present queues") {
    mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate integrated;
    integrated.name = "Integrated";
    integrated.type = mirakana::rhi::vulkan::VulkanPhysicalDeviceType::integrated_gpu;
    integrated.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    integrated.supports_swapchain_extension = true;
    integrated.supports_dynamic_rendering = true;
    integrated.supports_synchronization2 = true;
    integrated.queue_families.push_back(mirakana::rhi::vulkan::VulkanQueueFamilyCandidate{
        .index = 0,
        .queue_count = 1,
        .capabilities = mirakana::rhi::vulkan::VulkanQueueCapability::graphics |
                        mirakana::rhi::vulkan::VulkanQueueCapability::compute,
        .supports_present = true,
    });

    mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate discrete;
    discrete.name = "Discrete";
    discrete.type = mirakana::rhi::vulkan::VulkanPhysicalDeviceType::discrete_gpu;
    discrete.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    discrete.supports_swapchain_extension = true;
    discrete.supports_dynamic_rendering = true;
    discrete.supports_synchronization2 = true;
    discrete.queue_families.push_back(mirakana::rhi::vulkan::VulkanQueueFamilyCandidate{
        .index = 2,
        .queue_count = 1,
        .capabilities = mirakana::rhi::vulkan::VulkanQueueCapability::graphics |
                        mirakana::rhi::vulkan::VulkanQueueCapability::transfer,
        .supports_present = false,
    });
    discrete.queue_families.push_back(mirakana::rhi::vulkan::VulkanQueueFamilyCandidate{
        .index = 3,
        .queue_count = 1,
        .capabilities = mirakana::rhi::vulkan::VulkanQueueCapability::graphics |
                        mirakana::rhi::vulkan::VulkanQueueCapability::compute,
        .supports_present = true,
    });

    const auto selection = mirakana::rhi::vulkan::select_physical_device({integrated, discrete});

    MK_REQUIRE(selection.suitable);
    MK_REQUIRE(selection.device_index == 1);
    MK_REQUIRE(selection.graphics_queue_family == 3);
    MK_REQUIRE(selection.present_queue_family == 3);
    MK_REQUIRE(selection.score > 1000);
    MK_REQUIRE(selection.diagnostic == "selected Discrete");
}

MK_TEST("vulkan device selection rejects missing api extension features and present queue") {
    mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate old_api;
    old_api.name = "Old API";
    old_api.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 2);
    old_api.supports_swapchain_extension = true;
    old_api.supports_dynamic_rendering = true;
    old_api.supports_synchronization2 = true;
    old_api.queue_families.push_back(mirakana::rhi::vulkan::VulkanQueueFamilyCandidate{
        .index = 0,
        .queue_count = 1,
        .capabilities = mirakana::rhi::vulkan::VulkanQueueCapability::graphics,
        .supports_present = true,
    });

    mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate missing_swapchain = old_api;
    missing_swapchain.name = "Missing Swapchain";
    missing_swapchain.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    missing_swapchain.supports_swapchain_extension = false;

    mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate missing_present = old_api;
    missing_present.name = "Missing Present";
    missing_present.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    missing_present.queue_families[0].supports_present = false;

    mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate missing_synchronization2 = old_api;
    missing_synchronization2.name = "Missing Synchronization2";
    missing_synchronization2.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    missing_synchronization2.supports_synchronization2 = false;

    mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate missing_compute = old_api;
    missing_compute.name = "Missing Compute Queue";
    missing_compute.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    missing_compute.queue_families[0].capabilities = mirakana::rhi::vulkan::VulkanQueueCapability::graphics;

    const auto selection = mirakana::rhi::vulkan::select_physical_device(
        {old_api, missing_swapchain, missing_present, missing_synchronization2, missing_compute});

    MK_REQUIRE(!selection.suitable);
    MK_REQUIRE(selection.device_index == mirakana::rhi::vulkan::invalid_vulkan_device_index);
    MK_REQUIRE(selection.diagnostic == "no suitable Vulkan physical device");
}

MK_TEST("vulkan instance create plan enables required and available optional extensions") {
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc desc;
    desc.application_name = "GameEngineEditor";
    desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    desc.required_extensions = {"VK_KHR_surface", "VK_KHR_win32_surface"};
    desc.optional_extensions = {"VK_EXT_debug_utils", "VK_KHR_portability_enumeration"};
    desc.enable_validation = true;

    const auto plan = mirakana::rhi::vulkan::build_instance_create_plan(
        desc, {"VK_EXT_debug_utils", "VK_KHR_surface", "VK_KHR_win32_surface"});

    MK_REQUIRE(plan.supported);
    MK_REQUIRE(plan.api_version.major == 1);
    MK_REQUIRE(plan.api_version.minor == 3);
    MK_REQUIRE(plan.enabled_extensions.size() == 3);
    MK_REQUIRE(plan.enabled_extensions[0] == "VK_KHR_surface");
    MK_REQUIRE(plan.enabled_extensions[1] == "VK_KHR_win32_surface");
    MK_REQUIRE(plan.enabled_extensions[2] == "VK_EXT_debug_utils");
    MK_REQUIRE(plan.validation_enabled);
    MK_REQUIRE(plan.diagnostic == "Vulkan instance create plan ready");
}

MK_TEST("vulkan instance create plan rejects old api empty app names and missing required extensions") {
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc old_api;
    old_api.application_name = "GameEngineEditor";
    old_api.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 2);
    old_api.required_extensions = {"VK_KHR_surface"};

    const auto rejected_api = mirakana::rhi::vulkan::build_instance_create_plan(old_api, {"VK_KHR_surface"});

    MK_REQUIRE(!rejected_api.supported);
    MK_REQUIRE(rejected_api.diagnostic == "Vulkan 1.3 or newer is required");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc empty_name = old_api;
    empty_name.application_name.clear();
    empty_name.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const auto rejected_name = mirakana::rhi::vulkan::build_instance_create_plan(empty_name, {"VK_KHR_surface"});

    MK_REQUIRE(!rejected_name.supported);
    MK_REQUIRE(rejected_name.diagnostic == "Vulkan application name is required");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc missing_extension = empty_name;
    missing_extension.application_name = "GameEngineEditor";
    missing_extension.required_extensions = {"VK_KHR_surface", "VK_KHR_win32_surface"};

    const auto rejected_extension =
        mirakana::rhi::vulkan::build_instance_create_plan(missing_extension, {"VK_KHR_surface"});

    MK_REQUIRE(!rejected_extension.supported);
    MK_REQUIRE(rejected_extension.diagnostic == "missing required Vulkan instance extension: VK_KHR_win32_surface");
}

MK_TEST("vulkan logical device create plan deduplicates queue families and enables device extensions") {
    mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate device;
    device.name = "Discrete";
    device.type = mirakana::rhi::vulkan::VulkanPhysicalDeviceType::discrete_gpu;
    device.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    device.supports_swapchain_extension = true;
    device.supports_dynamic_rendering = true;
    device.supports_synchronization2 = true;
    device.queue_families.push_back(mirakana::rhi::vulkan::VulkanQueueFamilyCandidate{
        .index = 4,
        .queue_count = 1,
        .capabilities = mirakana::rhi::vulkan::VulkanQueueCapability::graphics |
                        mirakana::rhi::vulkan::VulkanQueueCapability::compute,
        .supports_present = true,
    });

    const auto selection = mirakana::rhi::vulkan::select_physical_device({device});
    mirakana::rhi::vulkan::VulkanLogicalDeviceCreateDesc desc;
    desc.optional_extensions = {"VK_EXT_memory_budget"};

    const auto plan = mirakana::rhi::vulkan::build_logical_device_create_plan(
        desc, device, selection, {"VK_KHR_swapchain", "VK_EXT_memory_budget"});

    MK_REQUIRE(plan.supported);
    MK_REQUIRE(plan.queue_families.size() == 1);
    MK_REQUIRE(plan.queue_families[0].queue_family == 4);
    MK_REQUIRE(plan.queue_families[0].queue_count == 1);
    MK_REQUIRE(plan.queue_families[0].priority == 1.0F);
    MK_REQUIRE(plan.enabled_extensions.size() == 2);
    MK_REQUIRE(plan.enabled_extensions[0] == "VK_KHR_swapchain");
    MK_REQUIRE(plan.enabled_extensions[1] == "VK_EXT_memory_budget");
    MK_REQUIRE(plan.dynamic_rendering_enabled);
    MK_REQUIRE(plan.synchronization2_enabled);
    MK_REQUIRE(plan.diagnostic == "Vulkan logical device create plan ready");
}

MK_TEST("vulkan logical device create plan preserves separate graphics and present queues") {
    mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate device;
    device.name = "Separate Queues";
    device.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    device.supports_swapchain_extension = true;
    device.supports_dynamic_rendering = true;
    device.supports_synchronization2 = true;
    device.queue_families.push_back(mirakana::rhi::vulkan::VulkanQueueFamilyCandidate{
        .index = 2,
        .queue_count = 1,
        .capabilities = mirakana::rhi::vulkan::VulkanQueueCapability::graphics |
                        mirakana::rhi::vulkan::VulkanQueueCapability::compute,
        .supports_present = false,
    });
    device.queue_families.push_back(mirakana::rhi::vulkan::VulkanQueueFamilyCandidate{
        .index = 3,
        .queue_count = 1,
        .capabilities = mirakana::rhi::vulkan::VulkanQueueCapability::transfer,
        .supports_present = true,
    });

    const auto selection = mirakana::rhi::vulkan::select_physical_device({device});
    const auto plan = mirakana::rhi::vulkan::build_logical_device_create_plan(
        mirakana::rhi::vulkan::VulkanLogicalDeviceCreateDesc{}, device, selection, {"VK_KHR_swapchain"});

    MK_REQUIRE(plan.supported);
    MK_REQUIRE(plan.queue_families.size() == 2);
    MK_REQUIRE(plan.queue_families[0].queue_family == 2);
    MK_REQUIRE(plan.queue_families[1].queue_family == 3);
}

MK_TEST("vulkan logical device create plan rejects unsuitable selections missing extensions and features") {
    mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate device;
    device.name = "Candidate";
    device.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    device.supports_swapchain_extension = true;
    device.supports_dynamic_rendering = true;
    device.supports_synchronization2 = true;
    device.queue_families.push_back(mirakana::rhi::vulkan::VulkanQueueFamilyCandidate{
        .index = 0,
        .queue_count = 1,
        .capabilities = mirakana::rhi::vulkan::VulkanQueueCapability::graphics |
                        mirakana::rhi::vulkan::VulkanQueueCapability::compute,
        .supports_present = true,
    });

    mirakana::rhi::vulkan::VulkanDeviceSelection unsuitable;
    const auto rejected_selection = mirakana::rhi::vulkan::build_logical_device_create_plan(
        mirakana::rhi::vulkan::VulkanLogicalDeviceCreateDesc{}, device, unsuitable, {"VK_KHR_swapchain"});

    MK_REQUIRE(!rejected_selection.supported);
    MK_REQUIRE(rejected_selection.diagnostic == "Vulkan device selection is not suitable");

    const auto selection = mirakana::rhi::vulkan::select_physical_device({device});
    const auto rejected_extension = mirakana::rhi::vulkan::build_logical_device_create_plan(
        mirakana::rhi::vulkan::VulkanLogicalDeviceCreateDesc{}, device, selection, {});

    MK_REQUIRE(!rejected_extension.supported);
    MK_REQUIRE(rejected_extension.diagnostic == "missing required Vulkan device extension: VK_KHR_swapchain");

    device.supports_dynamic_rendering = false;
    const auto rejected_feature = mirakana::rhi::vulkan::build_logical_device_create_plan(
        mirakana::rhi::vulkan::VulkanLogicalDeviceCreateDesc{}, device, selection, {"VK_KHR_swapchain"});

    MK_REQUIRE(!rejected_feature.supported);
    MK_REQUIRE(rejected_feature.diagnostic == "Vulkan dynamic rendering feature is required");

    device.supports_dynamic_rendering = true;
    device.supports_synchronization2 = false;
    const auto rejected_synchronization2 = mirakana::rhi::vulkan::build_logical_device_create_plan(
        mirakana::rhi::vulkan::VulkanLogicalDeviceCreateDesc{}, device, selection, {"VK_KHR_swapchain"});

    MK_REQUIRE(!rejected_synchronization2.supported);
    MK_REQUIRE(rejected_synchronization2.diagnostic == "Vulkan synchronization2 feature is required");
}

MK_TEST("vulkan command resolution plan accepts required loader global instance and device commands") {
    const auto requests = mirakana::rhi::vulkan::vulkan_backend_command_requests();
    std::vector<mirakana::rhi::vulkan::VulkanCommandAvailability> available;
    bool has_loader = false;
    bool has_create_instance = false;
    bool has_swapchain = false;
    bool has_dynamic_rendering = false;
    bool has_synchronization2 = false;
    bool has_compute_pipeline = false;
    bool has_compute_dispatch = false;

    for (const auto& request : requests) {
        available.push_back(mirakana::rhi::vulkan::VulkanCommandAvailability{
            .name = request.name,
            .scope = request.scope,
            .available = true,
        });
        has_loader = has_loader || (request.scope == mirakana::rhi::vulkan::VulkanCommandScope::loader &&
                                    request.name == "vkGetInstanceProcAddr");
        has_create_instance =
            has_create_instance ||
            (request.scope == mirakana::rhi::vulkan::VulkanCommandScope::global && request.name == "vkCreateInstance");
        has_swapchain = has_swapchain || (request.scope == mirakana::rhi::vulkan::VulkanCommandScope::device &&
                                          request.name == "vkCreateSwapchainKHR");
        has_dynamic_rendering =
            has_dynamic_rendering || (request.scope == mirakana::rhi::vulkan::VulkanCommandScope::device &&
                                      request.name == "vkCmdBeginRendering");
        has_synchronization2 =
            has_synchronization2 ||
            (request.scope == mirakana::rhi::vulkan::VulkanCommandScope::device &&
             request.name == "vkCmdPipelineBarrier2") ||
            (request.scope == mirakana::rhi::vulkan::VulkanCommandScope::device && request.name == "vkQueueSubmit2");
        has_compute_pipeline =
            has_compute_pipeline || (request.scope == mirakana::rhi::vulkan::VulkanCommandScope::device &&
                                     request.name == "vkCreateComputePipelines");
        has_compute_dispatch =
            has_compute_dispatch ||
            (request.scope == mirakana::rhi::vulkan::VulkanCommandScope::device && request.name == "vkCmdDispatch");
    }

    const auto plan = mirakana::rhi::vulkan::build_command_resolution_plan(requests, available);

    MK_REQUIRE(has_loader);
    MK_REQUIRE(has_create_instance);
    MK_REQUIRE(has_swapchain);
    MK_REQUIRE(has_dynamic_rendering);
    MK_REQUIRE(has_synchronization2);
    MK_REQUIRE(has_compute_pipeline);
    MK_REQUIRE(has_compute_dispatch);
    MK_REQUIRE(plan.supported);
    MK_REQUIRE(plan.resolutions.size() == requests.size());
    MK_REQUIRE(plan.missing_required_commands.empty());
    MK_REQUIRE(plan.diagnostic == "Vulkan command resolution plan ready");
}

MK_TEST("vulkan device command requests gate synchronization2 commands on enabled features") {
    mirakana::rhi::vulkan::VulkanLogicalDeviceCreatePlan plan;
    plan.supported = true;
    plan.enabled_extensions = {"VK_KHR_swapchain"};
    plan.dynamic_rendering_enabled = true;
    plan.synchronization2_enabled = false;

    const auto without_synchronization2 = mirakana::rhi::vulkan::vulkan_device_command_requests(plan);
    bool has_pipeline_barrier2 = false;
    bool has_queue_submit2 = false;
    for (const auto& request : without_synchronization2) {
        has_pipeline_barrier2 = has_pipeline_barrier2 || request.name == "vkCmdPipelineBarrier2";
        has_queue_submit2 = has_queue_submit2 || request.name == "vkQueueSubmit2";
    }

    MK_REQUIRE(!has_pipeline_barrier2);
    MK_REQUIRE(!has_queue_submit2);

    plan.synchronization2_enabled = true;
    const auto with_synchronization2 = mirakana::rhi::vulkan::vulkan_device_command_requests(plan);
    for (const auto& request : with_synchronization2) {
        has_pipeline_barrier2 = has_pipeline_barrier2 || request.name == "vkCmdPipelineBarrier2";
        has_queue_submit2 = has_queue_submit2 || request.name == "vkQueueSubmit2";
    }

    MK_REQUIRE(has_pipeline_barrier2);
    MK_REQUIRE(has_queue_submit2);
}

MK_TEST("vulkan command resolution plan rejects missing required commands but tolerates missing optional commands") {
    std::vector<mirakana::rhi::vulkan::VulkanCommandRequest> requests;
    requests.push_back(mirakana::rhi::vulkan::VulkanCommandRequest{
        .name = "vkCreateInstance",
        .scope = mirakana::rhi::vulkan::VulkanCommandScope::global,
        .required = true,
    });
    requests.push_back(mirakana::rhi::vulkan::VulkanCommandRequest{
        .name = "vkCreateDebugUtilsMessengerEXT",
        .scope = mirakana::rhi::vulkan::VulkanCommandScope::instance,
        .required = false,
    });
    requests.push_back(mirakana::rhi::vulkan::VulkanCommandRequest{
        .name = "vkCreateDevice",
        .scope = mirakana::rhi::vulkan::VulkanCommandScope::instance,
        .required = true,
    });

    const auto plan = mirakana::rhi::vulkan::build_command_resolution_plan(
        requests, {
                      mirakana::rhi::vulkan::VulkanCommandAvailability{
                          .name = "vkCreateInstance",
                          .scope = mirakana::rhi::vulkan::VulkanCommandScope::global,
                          .available = true,
                      },
                      mirakana::rhi::vulkan::VulkanCommandAvailability{
                          .name = "vkCreateDebugUtilsMessengerEXT",
                          .scope = mirakana::rhi::vulkan::VulkanCommandScope::instance,
                          .available = false,
                      },
                  });

    MK_REQUIRE(!plan.supported);
    MK_REQUIRE(plan.resolutions.size() == 3);
    MK_REQUIRE(plan.resolutions[0].resolved);
    MK_REQUIRE(!plan.resolutions[1].resolved);
    MK_REQUIRE(!plan.resolutions[2].resolved);
    MK_REQUIRE(plan.missing_required_commands.size() == 1);
    MK_REQUIRE(plan.missing_required_commands[0].name == "vkCreateDevice");
    MK_REQUIRE(plan.missing_required_commands[0].scope == mirakana::rhi::vulkan::VulkanCommandScope::instance);
    MK_REQUIRE(plan.diagnostic == "missing required Vulkan command: vkCreateDevice");
}

MK_TEST("vulkan runtime global command probe inspects current host without exposing native pointers") {
    const auto result = mirakana::rhi::vulkan::probe_runtime_global_commands();
    const auto host = mirakana::rhi::current_rhi_host_platform();

    MK_REQUIRE(result.loader.probe.backend == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(result.loader.probe.host == host);
    if (mirakana::rhi::vulkan::supports_host(host)) {
        MK_REQUIRE(!result.loader.runtime_library.empty());
    } else {
        MK_REQUIRE(result.loader.runtime_library.empty());
        MK_REQUIRE(result.loader.probe.status == mirakana::rhi::BackendProbeStatus::unsupported_host);
    }
    MK_REQUIRE(!result.command_plan.resolutions.empty());
    MK_REQUIRE(!result.command_plan.diagnostic.empty());
}

MK_TEST("vulkan api version encoding and decoding use Vulkan packed version layout") {
    const auto encoded = mirakana::rhi::vulkan::encode_vulkan_api_version(
        mirakana::rhi::vulkan::VulkanApiVersion{.major = 1, .minor = 4});
    const auto version = mirakana::rhi::vulkan::decode_vulkan_api_version(encoded | 275U);

    MK_REQUIRE(encoded == ((1U << 22U) | (4U << 12U)));
    MK_REQUIRE(version.major == 1);
    MK_REQUIRE(version.minor == 4);
}

MK_TEST("vulkan runtime instance capability probe inspects version and extensions without exposing native handles") {
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc desc;
    desc.application_name = "GameEngineBackendProbe";
    desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    desc.required_extensions = {};

    const auto result = mirakana::rhi::vulkan::probe_runtime_instance_capabilities({}, desc);

    MK_REQUIRE(result.global.loader.probe.backend == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(result.global.loader.probe.host == mirakana::rhi::current_rhi_host_platform());
    MK_REQUIRE(!result.global.command_plan.resolutions.empty());
    MK_REQUIRE(!result.instance_plan.diagnostic.empty());
    if (result.instance_plan.supported) {
        MK_REQUIRE(result.api_version.major >= 1);
    }
}

MK_TEST("vulkan runtime instance command probe creates and destroys a transient instance without exposing handles") {
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc desc;
    desc.application_name = "GameEngineInstanceCommandProbe";
    desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    desc.required_extensions = {};

    const auto result = mirakana::rhi::vulkan::probe_runtime_instance_commands({}, desc);

    MK_REQUIRE(result.capabilities.global.loader.probe.backend == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(result.capabilities.global.loader.probe.host == mirakana::rhi::current_rhi_host_platform());
    MK_REQUIRE(!result.diagnostic.empty());
    if (result.instance_created) {
        bool resolved_destroy_instance = false;
        for (const auto& resolution : result.command_plan.resolutions) {
            resolved_destroy_instance =
                resolved_destroy_instance ||
                (resolution.resolved &&
                 resolution.request.scope == mirakana::rhi::vulkan::VulkanCommandScope::instance &&
                 resolution.request.name == "vkDestroyInstance");
        }
        MK_REQUIRE(result.instance_destroyed);
        MK_REQUIRE(result.command_plan.supported);
        MK_REQUIRE(resolved_destroy_instance);
    }
}

MK_TEST("vulkan runtime instance owner is move only and destroys persistent instances without exposing handles") {
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeInstance>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeInstance>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeInstance>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeInstance>);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc desc;
    desc.application_name = "GameEnginePersistentInstanceOwner";
    desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    desc.required_extensions = {};

    auto result = mirakana::rhi::vulkan::create_runtime_instance({}, desc);

    MK_REQUIRE(result.probe.capabilities.global.loader.probe.backend == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(!result.diagnostic.empty());
    MK_REQUIRE(result.created == result.instance.owns_instance());
    if (result.created) {
        MK_REQUIRE(result.instance.command_plan().supported);
        MK_REQUIRE(!result.instance.destroyed());
        result.instance.reset();
        MK_REQUIRE(!result.instance.owns_instance());
        MK_REQUIRE(result.instance.destroyed());
    }
}

MK_TEST("vulkan runtime device owner is move only and resolves device commands without exposing handles") {
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeDevice>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeDevice>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeDevice>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeDevice>);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeDeviceOwner";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    mirakana::rhi::vulkan::VulkanLogicalDeviceCreateDesc device_desc;
    const auto expected_requests = mirakana::rhi::vulkan::vulkan_device_command_requests(
        mirakana::rhi::vulkan::VulkanLogicalDeviceCreatePlan{.supported = true,
                                                             .queue_families = {},
                                                             .enabled_extensions = {"VK_KHR_swapchain"},
                                                             .dynamic_rendering_enabled = true,
                                                             .synchronization2_enabled = true,
                                                             .diagnostic = {}});

    auto result = mirakana::rhi::vulkan::create_runtime_device({}, instance_desc, device_desc);

    MK_REQUIRE(!expected_requests.empty());
    MK_REQUIRE(result.selection_probe.snapshots.count_probe.instance.capabilities.global.loader.probe.backend ==
               mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(!result.diagnostic.empty());
    MK_REQUIRE(result.created == result.device.owns_device());
    if (result.created) {
        MK_REQUIRE(result.device.logical_device_plan().supported);
        MK_REQUIRE(result.device.command_plan().supported);
        MK_REQUIRE(result.device.has_graphics_queue());
        MK_REQUIRE(result.device.has_present_queue());
        MK_REQUIRE(result.device.command_plan().resolutions.size() == expected_requests.size());
        result.device.reset();
        MK_REQUIRE(!result.device.owns_device());
        MK_REQUIRE(result.device.destroyed());
    }
}

MK_TEST("vulkan runtime command pool owns a primary command buffer without exposing handles") {
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeCommandPool>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeCommandPool>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeCommandPool>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeCommandPool>);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeCommandPoolOwner";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    auto device_result = mirakana::rhi::vulkan::create_runtime_device({}, instance_desc, {});
    auto pool_result = mirakana::rhi::vulkan::create_runtime_command_pool(device_result.device, {});

    MK_REQUIRE(!pool_result.diagnostic.empty());
    MK_REQUIRE(pool_result.created == pool_result.pool.owns_pool());
    if (pool_result.created) {
        MK_REQUIRE(pool_result.pool.owns_primary_command_buffer());
        MK_REQUIRE(!pool_result.pool.recording());
        MK_REQUIRE(!pool_result.pool.ended());
        MK_REQUIRE(pool_result.pool.begin_primary_command_buffer());
        MK_REQUIRE(pool_result.pool.recording());
        MK_REQUIRE(pool_result.pool.end_primary_command_buffer());
        MK_REQUIRE(!pool_result.pool.recording());
        MK_REQUIRE(pool_result.pool.ended());
        pool_result.pool.reset();
        MK_REQUIRE(!pool_result.pool.owns_pool());
        MK_REQUIRE(pool_result.pool.destroyed());
    }
}

MK_TEST("vulkan runtime dynamic rendering draw recording gates objects without exposing handles") {
    mirakana::rhi::vulkan::VulkanDynamicRenderingPlan rendering_plan;
    rendering_plan.supported = true;
    rendering_plan.extent = mirakana::rhi::Extent2D{.width = 32, .height = 32};
    rendering_plan.color_attachment_count = 1;
    rendering_plan.color_formats = {mirakana::rhi::Format::bgra8_unorm};
    rendering_plan.begin_rendering_command_resolved = true;
    rendering_plan.end_rendering_command_resolved = true;

    mirakana::rhi::vulkan::VulkanRuntimeDynamicRenderingDrawDesc desc;
    desc.dynamic_rendering = rendering_plan;
    desc.image_index = 0;
    desc.vertex_count = 3;
    desc.clear_color = mirakana::rhi::ClearColorValue{.red = 0.15F, .green = 0.25F, .blue = 0.35F, .alpha = 1.0F};

    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    mirakana::rhi::vulkan::VulkanRuntimeCommandPool empty_pool;
    mirakana::rhi::vulkan::VulkanRuntimeSwapchain empty_swapchain;
    mirakana::rhi::vulkan::VulkanRuntimeGraphicsPipeline empty_pipeline;
    const auto missing_device = mirakana::rhi::vulkan::record_runtime_dynamic_rendering_draw(
        empty_device, empty_pool, empty_swapchain, empty_pipeline, desc);
    MK_REQUIRE(!missing_device.recorded);
    MK_REQUIRE(missing_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeDynamicRenderingRecording";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    auto device_result = mirakana::rhi::vulkan::create_runtime_device({}, instance_desc, {});
    if (device_result.created) {
        const auto missing_pool = mirakana::rhi::vulkan::record_runtime_dynamic_rendering_draw(
            device_result.device, empty_pool, empty_swapchain, empty_pipeline, desc);
        MK_REQUIRE(!missing_pool.recorded);
        MK_REQUIRE(missing_pool.diagnostic == "Vulkan runtime command pool is required");

        auto pool_result = mirakana::rhi::vulkan::create_runtime_command_pool(device_result.device, {});
        if (pool_result.created) {
            const auto not_recording = mirakana::rhi::vulkan::record_runtime_dynamic_rendering_draw(
                device_result.device, pool_result.pool, empty_swapchain, empty_pipeline, desc);
            MK_REQUIRE(!not_recording.recorded);
            MK_REQUIRE(not_recording.diagnostic == "Vulkan command buffer must be recording");
        } else {
            MK_REQUIRE(!pool_result.diagnostic.empty());
        }
    } else {
        MK_REQUIRE(!device_result.diagnostic.empty());
    }
}

MK_TEST("vulkan runtime texture dynamic rendering draw recording gates objects without exposing handles") {
    mirakana::rhi::vulkan::VulkanDynamicRenderingPlan rendering_plan;
    rendering_plan.supported = true;
    rendering_plan.extent = mirakana::rhi::Extent2D{.width = 32, .height = 32};
    rendering_plan.color_attachment_count = 1;
    rendering_plan.color_formats = {mirakana::rhi::Format::rgba8_unorm};
    rendering_plan.begin_rendering_command_resolved = true;
    rendering_plan.end_rendering_command_resolved = true;

    mirakana::rhi::vulkan::VulkanRuntimeTextureRenderingDrawDesc desc;
    desc.dynamic_rendering = rendering_plan;
    desc.vertex_count = 3;
    desc.clear_color = mirakana::rhi::ClearColorValue{.red = 0.15F, .green = 0.25F, .blue = 0.35F, .alpha = 1.0F};

    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    mirakana::rhi::vulkan::VulkanRuntimeCommandPool empty_pool;
    mirakana::rhi::vulkan::VulkanRuntimeTexture empty_texture;
    mirakana::rhi::vulkan::VulkanRuntimeGraphicsPipeline empty_pipeline;
    const auto missing_device = mirakana::rhi::vulkan::record_runtime_texture_rendering_draw(
        empty_device, empty_pool, empty_texture, empty_pipeline, desc);
    MK_REQUIRE(!missing_device.recorded);
    MK_REQUIRE(missing_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeTextureDynamicRenderingRecording";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    auto device_result = mirakana::rhi::vulkan::create_runtime_device({}, instance_desc, {});
    if (device_result.created) {
        const auto missing_pool = mirakana::rhi::vulkan::record_runtime_texture_rendering_draw(
            device_result.device, empty_pool, empty_texture, empty_pipeline, desc);
        MK_REQUIRE(!missing_pool.recorded);
        MK_REQUIRE(missing_pool.diagnostic == "Vulkan runtime command pool is required");

        auto pool_result = mirakana::rhi::vulkan::create_runtime_command_pool(device_result.device, {});
        if (pool_result.created) {
            const auto not_recording = mirakana::rhi::vulkan::record_runtime_texture_rendering_draw(
                device_result.device, pool_result.pool, empty_texture, empty_pipeline, desc);
            MK_REQUIRE(!not_recording.recorded);
            MK_REQUIRE(not_recording.diagnostic == "Vulkan command buffer must be recording");
        } else {
            MK_REQUIRE(!pool_result.diagnostic.empty());
        }
    } else {
        MK_REQUIRE(!device_result.diagnostic.empty());
    }
}

MK_TEST("vulkan runtime synchronization2 barrier and submit gate objects without exposing handles") {
    mirakana::rhi::vulkan::VulkanRuntimeSwapchainFrameBarrierDesc barrier_desc;
    barrier_desc.barrier = mirakana::rhi::vulkan::VulkanFrameSynchronizationBarrier{
        .before = mirakana::rhi::ResourceState::present,
        .after = mirakana::rhi::ResourceState::render_target,
        .src_stage = mirakana::rhi::vulkan::VulkanSynchronizationStage::none,
        .src_access = mirakana::rhi::vulkan::VulkanSynchronizationAccess::none,
        .dst_stage = mirakana::rhi::vulkan::VulkanSynchronizationStage::color_attachment_output,
        .dst_access = mirakana::rhi::vulkan::VulkanSynchronizationAccess::color_attachment_write,
    };
    barrier_desc.image_index = 0;

    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    mirakana::rhi::vulkan::VulkanRuntimeCommandPool empty_pool;
    mirakana::rhi::vulkan::VulkanRuntimeSwapchain empty_swapchain;
    const auto missing_device = mirakana::rhi::vulkan::record_runtime_swapchain_frame_barrier(
        empty_device, empty_pool, empty_swapchain, barrier_desc);
    MK_REQUIRE(!missing_device.recorded);
    MK_REQUIRE(missing_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanRuntimeFrameSync empty_sync;
    const auto submit_missing_device =
        mirakana::rhi::vulkan::submit_runtime_command_buffer(empty_device, empty_pool, empty_sync, {});
    MK_REQUIRE(!submit_missing_device.submitted);
    MK_REQUIRE(submit_missing_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeSynchronization2Recording";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    auto device_result = mirakana::rhi::vulkan::create_runtime_device({}, instance_desc, {});
    if (device_result.created) {
        const auto missing_pool = mirakana::rhi::vulkan::record_runtime_swapchain_frame_barrier(
            device_result.device, empty_pool, empty_swapchain, barrier_desc);
        MK_REQUIRE(!missing_pool.recorded);
        MK_REQUIRE(missing_pool.diagnostic == "Vulkan runtime command pool is required");

        const auto submit_missing_pool =
            mirakana::rhi::vulkan::submit_runtime_command_buffer(device_result.device, empty_pool, empty_sync, {});
        MK_REQUIRE(!submit_missing_pool.submitted);
        MK_REQUIRE(submit_missing_pool.diagnostic == "Vulkan runtime command pool is required");

        auto pool_result = mirakana::rhi::vulkan::create_runtime_command_pool(device_result.device, {});
        if (pool_result.created) {
            const auto not_recording = mirakana::rhi::vulkan::record_runtime_swapchain_frame_barrier(
                device_result.device, pool_result.pool, empty_swapchain, barrier_desc);
            MK_REQUIRE(!not_recording.recorded);
            MK_REQUIRE(not_recording.diagnostic == "Vulkan command buffer must be recording");

            const auto not_ended = mirakana::rhi::vulkan::submit_runtime_command_buffer(
                device_result.device, pool_result.pool, empty_sync, {});
            MK_REQUIRE(!not_ended.submitted);
            MK_REQUIRE(not_ended.diagnostic == "Vulkan command buffer must be ended before submit");
        } else {
            MK_REQUIRE(!pool_result.diagnostic.empty());
        }
    } else {
        MK_REQUIRE(!device_result.diagnostic.empty());
    }
}

MK_TEST("vulkan runtime readback buffer and swapchain copy gates objects without exposing handles") {
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeReadbackBuffer>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeReadbackBuffer>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeReadbackBuffer>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeReadbackBuffer>);

    mirakana::rhi::vulkan::VulkanRuntimeReadbackBufferDesc buffer_desc;
    buffer_desc.byte_size = 32U * 32U * 4U;

    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    const auto missing_device = mirakana::rhi::vulkan::create_runtime_readback_buffer(empty_device, buffer_desc);
    MK_REQUIRE(!missing_device.created);
    MK_REQUIRE(!missing_device.buffer.owns_buffer());
    MK_REQUIRE(!missing_device.buffer.owns_memory());
    MK_REQUIRE(missing_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanRuntimeCommandPool empty_pool;
    mirakana::rhi::vulkan::VulkanRuntimeSwapchain empty_swapchain;
    mirakana::rhi::vulkan::VulkanRuntimeReadbackBuffer empty_buffer;
    mirakana::rhi::vulkan::VulkanRuntimeSwapchainReadbackDesc readback_desc;
    readback_desc.image_index = 0;
    readback_desc.extent = mirakana::rhi::Extent2D{.width = 32, .height = 32};
    readback_desc.bytes_per_pixel = 4;

    const auto copy_missing_device = mirakana::rhi::vulkan::record_runtime_swapchain_image_readback(
        empty_device, empty_pool, empty_swapchain, empty_buffer, readback_desc);
    MK_REQUIRE(!copy_missing_device.recorded);
    MK_REQUIRE(copy_missing_device.diagnostic == "Vulkan runtime device is not available");

    const auto read_missing_device =
        mirakana::rhi::vulkan::read_runtime_readback_buffer(empty_device, empty_buffer, {});
    MK_REQUIRE(!read_missing_device.read);
    MK_REQUIRE(read_missing_device.bytes.empty());
    MK_REQUIRE(read_missing_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeReadback";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    auto device_result = mirakana::rhi::vulkan::create_runtime_device({}, instance_desc, {});
    if (device_result.created) {
        mirakana::rhi::vulkan::VulkanRuntimeReadbackBufferDesc zero_size_desc;
        const auto zero_size =
            mirakana::rhi::vulkan::create_runtime_readback_buffer(device_result.device, zero_size_desc);
        MK_REQUIRE(!zero_size.created);
        MK_REQUIRE(zero_size.diagnostic == "Vulkan readback buffer size is required");

        auto buffer_result = mirakana::rhi::vulkan::create_runtime_readback_buffer(device_result.device, buffer_desc);
        MK_REQUIRE(!buffer_result.diagnostic.empty());
        MK_REQUIRE(buffer_result.created == buffer_result.buffer.owns_buffer());
        if (buffer_result.created) {
            MK_REQUIRE(buffer_result.buffer.owns_memory());
            MK_REQUIRE(buffer_result.buffer.byte_size() == buffer_desc.byte_size);

            mirakana::rhi::vulkan::VulkanRuntimeReadbackBufferReadDesc read_desc;
            read_desc.byte_count = 16;
            const auto read_result = mirakana::rhi::vulkan::read_runtime_readback_buffer(
                device_result.device, buffer_result.buffer, read_desc);
            MK_REQUIRE(read_result.read);
            MK_REQUIRE(read_result.bytes.size() == read_desc.byte_count);
        }

        const auto missing_pool = mirakana::rhi::vulkan::record_runtime_swapchain_image_readback(
            device_result.device, empty_pool, empty_swapchain, empty_buffer, readback_desc);
        MK_REQUIRE(!missing_pool.recorded);
        MK_REQUIRE(missing_pool.diagnostic == "Vulkan runtime command pool is required");

        auto pool_result = mirakana::rhi::vulkan::create_runtime_command_pool(device_result.device, {});
        if (pool_result.created) {
            const auto not_recording = mirakana::rhi::vulkan::record_runtime_swapchain_image_readback(
                device_result.device, pool_result.pool, empty_swapchain, empty_buffer, readback_desc);
            MK_REQUIRE(!not_recording.recorded);
            MK_REQUIRE(not_recording.diagnostic == "Vulkan command buffer must be recording");

            if (buffer_result.created && pool_result.pool.begin_primary_command_buffer()) {
                const auto missing_swapchain = mirakana::rhi::vulkan::record_runtime_swapchain_image_readback(
                    device_result.device, pool_result.pool, empty_swapchain, buffer_result.buffer, readback_desc);
                MK_REQUIRE(!missing_swapchain.recorded);
                MK_REQUIRE(missing_swapchain.diagnostic == "Vulkan runtime swapchain is required");
            }
        } else {
            MK_REQUIRE(!pool_result.diagnostic.empty());
        }
    } else {
        MK_REQUIRE(!device_result.diagnostic.empty());
    }
}

MK_TEST("vulkan runtime dynamic rendering clear can feed swapchain readback when runtime is available") {
    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    mirakana::rhi::vulkan::VulkanRuntimeCommandPool empty_pool;
    mirakana::rhi::vulkan::VulkanRuntimeSwapchain empty_swapchain;
    mirakana::rhi::vulkan::VulkanRuntimeDynamicRenderingClearDesc clear_desc;

    const auto missing_device = mirakana::rhi::vulkan::record_runtime_dynamic_rendering_clear(
        empty_device, empty_pool, empty_swapchain, clear_desc);
    MK_REQUIRE(!missing_device.recorded);
    MK_REQUIRE(missing_device.diagnostic == "Vulkan runtime device is not available");

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeVisibleClear";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    mirakana::rhi::vulkan::VulkanSwapchainCreatePlan swapchain_plan;
    swapchain_plan.supported = true;
    swapchain_plan.extent = mirakana::rhi::Extent2D{.width = 64, .height = 64};
    swapchain_plan.format = mirakana::rhi::Format::bgra8_unorm;
    swapchain_plan.image_count = 2;
    swapchain_plan.image_view_count = 2;
    swapchain_plan.present_mode = mirakana::rhi::vulkan::VulkanPresentMode::fifo;
    swapchain_plan.acquire_before_render = true;
    swapchain_plan.diagnostic = "Vulkan swapchain create plan ready";

    auto swapchain_result = mirakana::rhi::vulkan::create_runtime_swapchain(
        device_result.device, {.surface = surface, .plan = swapchain_plan});
    if (!swapchain_result.created) {
        MK_REQUIRE(!swapchain_result.diagnostic.empty());
        return;
    }

    auto sync_result = mirakana::rhi::vulkan::create_runtime_frame_sync(device_result.device);
    auto pool_result = mirakana::rhi::vulkan::create_runtime_command_pool(device_result.device);
    auto readback_result = mirakana::rhi::vulkan::create_runtime_readback_buffer(
        device_result.device, mirakana::rhi::vulkan::VulkanRuntimeReadbackBufferDesc{64ULL * 64ULL * 4ULL});
    if (!sync_result.created || !pool_result.created || !readback_result.created) {
        MK_REQUIRE(!sync_result.diagnostic.empty() || !pool_result.diagnostic.empty() ||
                   !readback_result.diagnostic.empty());
        return;
    }

    auto acquire_result = mirakana::rhi::vulkan::acquire_next_runtime_swapchain_image(
        device_result.device, swapchain_result.swapchain, sync_result.sync);
    if (!acquire_result.acquired) {
        MK_REQUIRE(!acquire_result.diagnostic.empty());
        return;
    }

    const auto dynamic_plan = mirakana::rhi::vulkan::build_dynamic_rendering_plan(
        mirakana::rhi::vulkan::VulkanDynamicRenderingDesc{
            .extent = swapchain_plan.extent,
            .color_attachments = {mirakana::rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
                .format = swapchain_plan.format,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            }},
            .has_depth_attachment = false,
            .depth_format = mirakana::rhi::Format::unknown,
            .depth_load_action = mirakana::rhi::LoadAction::clear,
            .depth_store_action = mirakana::rhi::StoreAction::store,
        },
        device_result.device.command_plan());
    MK_REQUIRE(dynamic_plan.supported);

    mirakana::rhi::vulkan::VulkanFrameSynchronizationDesc sync_desc;
    sync_desc.readback_required = true;
    sync_desc.present_required = true;
    const auto sync_plan =
        mirakana::rhi::vulkan::build_frame_synchronization_plan(sync_desc, device_result.device.command_plan());
    MK_REQUIRE(sync_plan.supported);
    MK_REQUIRE(sync_plan.barriers.size() == 3);

    MK_REQUIRE(pool_result.pool.begin_primary_command_buffer());
    mirakana::rhi::vulkan::VulkanRuntimeSwapchainFrameBarrierDesc barrier_desc;
    barrier_desc.image_index = acquire_result.image_index;
    barrier_desc.barrier = sync_plan.barriers[0];
    const auto render_barrier = mirakana::rhi::vulkan::record_runtime_swapchain_frame_barrier(
        device_result.device, pool_result.pool, swapchain_result.swapchain, barrier_desc);
    MK_REQUIRE(render_barrier.recorded);

    clear_desc.dynamic_rendering = dynamic_plan;
    clear_desc.image_index = acquire_result.image_index;
    clear_desc.clear_color = mirakana::rhi::ClearColorValue{.red = 0.15F, .green = 0.55F, .blue = 0.95F, .alpha = 1.0F};
    const auto clear_result = mirakana::rhi::vulkan::record_runtime_dynamic_rendering_clear(
        device_result.device, pool_result.pool, swapchain_result.swapchain, clear_desc);
    MK_REQUIRE(clear_result.recorded);
    MK_REQUIRE(clear_result.began_rendering);
    MK_REQUIRE(clear_result.ended_rendering);

    barrier_desc.barrier = sync_plan.barriers[1];
    const auto readback_barrier = mirakana::rhi::vulkan::record_runtime_swapchain_frame_barrier(
        device_result.device, pool_result.pool, swapchain_result.swapchain, barrier_desc);
    MK_REQUIRE(readback_barrier.recorded);

    const auto copy_result = mirakana::rhi::vulkan::record_runtime_swapchain_image_readback(
        device_result.device, pool_result.pool, swapchain_result.swapchain, readback_result.buffer,
        mirakana::rhi::vulkan::VulkanRuntimeSwapchainReadbackDesc{
            .image_index = acquire_result.image_index, .extent = swapchain_plan.extent, .bytes_per_pixel = 4});
    MK_REQUIRE(copy_result.recorded);

    barrier_desc.barrier = sync_plan.barriers[2];
    const auto present_barrier = mirakana::rhi::vulkan::record_runtime_swapchain_frame_barrier(
        device_result.device, pool_result.pool, swapchain_result.swapchain, barrier_desc);
    MK_REQUIRE(present_barrier.recorded);

    MK_REQUIRE(pool_result.pool.end_primary_command_buffer());
    const auto submit_result = mirakana::rhi::vulkan::submit_runtime_command_buffer(
        device_result.device, pool_result.pool, sync_result.sync,
        mirakana::rhi::vulkan::VulkanRuntimeCommandBufferSubmitDesc{.wait_image_available_semaphore = true,
                                                                    .signal_render_finished_semaphore = true,
                                                                    .signal_in_flight_fence = false,
                                                                    .wait_for_graphics_queue_idle = true});
    MK_REQUIRE(submit_result.submitted);

    const auto read_result = mirakana::rhi::vulkan::read_runtime_readback_buffer(
        device_result.device, readback_result.buffer,
        mirakana::rhi::vulkan::VulkanRuntimeReadbackBufferReadDesc{.byte_offset = 0,
                                                                   .byte_count = copy_result.required_bytes});
    MK_REQUIRE(read_result.read);
    MK_REQUIRE(has_non_zero_byte(read_result.bytes));

    const auto present_result = mirakana::rhi::vulkan::present_runtime_swapchain_image(
        device_result.device, swapchain_result.swapchain, sync_result.sync,
        mirakana::rhi::vulkan::VulkanRuntimeSwapchainPresentDesc{.image_index = acquire_result.image_index,
                                                                 .wait_render_finished_semaphore = true});
    MK_REQUIRE(present_result.presented || present_result.resize_required);
#endif
}

MK_TEST("vulkan runtime descriptor set layout allocation and binding hide native handles") {
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeDescriptorSetLayout>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeDescriptorSetLayout>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeDescriptorSetLayout>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeDescriptorSetLayout>);
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeDescriptorSet>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeDescriptorSet>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeDescriptorSet>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeDescriptorSet>);

    mirakana::rhi::vulkan::VulkanRuntimeDescriptorSetLayoutDesc layout_desc;
    layout_desc.layout.bindings.push_back(mirakana::rhi::DescriptorBindingDesc{
        .binding = 0,
        .type = mirakana::rhi::DescriptorType::uniform_buffer,
        .count = 1,
        .stages = mirakana::rhi::ShaderStageVisibility::vertex,
    });

    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    const auto missing_device = mirakana::rhi::vulkan::create_runtime_descriptor_set_layout(empty_device, layout_desc);
    MK_REQUIRE(!missing_device.created);
    MK_REQUIRE(!missing_device.layout.owns_layout());
    MK_REQUIRE(missing_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanRuntimeCommandPool empty_pool;
    mirakana::rhi::vulkan::VulkanRuntimePipelineLayout empty_pipeline_layout;
    mirakana::rhi::vulkan::VulkanRuntimeDescriptorSet empty_set;
    const auto bind_missing_device = mirakana::rhi::vulkan::record_runtime_descriptor_set_binding(
        empty_device, empty_pool, empty_pipeline_layout, empty_set, {});
    MK_REQUIRE(!bind_missing_device.recorded);
    MK_REQUIRE(bind_missing_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeDescriptors";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    auto device_result = mirakana::rhi::vulkan::create_runtime_device({}, instance_desc, {});
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    mirakana::rhi::vulkan::VulkanRuntimeDescriptorSetLayoutDesc invalid_layout_desc;
    invalid_layout_desc.layout.bindings.push_back(mirakana::rhi::DescriptorBindingDesc{
        .binding = 0,
        .type = mirakana::rhi::DescriptorType::uniform_buffer,
        .count = 0,
        .stages = mirakana::rhi::ShaderStageVisibility::vertex,
    });
    const auto invalid_layout =
        mirakana::rhi::vulkan::create_runtime_descriptor_set_layout(device_result.device, invalid_layout_desc);
    MK_REQUIRE(!invalid_layout.created);
    MK_REQUIRE(invalid_layout.diagnostic == "Vulkan descriptor binding count is required");

    auto layout_result = mirakana::rhi::vulkan::create_runtime_descriptor_set_layout(device_result.device, layout_desc);
    MK_REQUIRE(!layout_result.diagnostic.empty());
    MK_REQUIRE(layout_result.created == layout_result.layout.owns_layout());
    if (!layout_result.created) {
        return;
    }
    MK_REQUIRE(layout_result.layout.binding_count() == 1);

    auto set_result =
        mirakana::rhi::vulkan::create_runtime_descriptor_set(device_result.device, layout_result.layout, {});
    MK_REQUIRE(!set_result.diagnostic.empty());
    MK_REQUIRE(set_result.created == set_result.set.owns_set());
    if (!set_result.created) {
        return;
    }
    MK_REQUIRE(set_result.set.owns_pool());

    mirakana::rhi::vulkan::VulkanRuntimePipelineLayoutDesc pipeline_layout_desc;
    pipeline_layout_desc.descriptor_set_layouts.push_back(&layout_result.layout);
    auto pipeline_layout_result =
        mirakana::rhi::vulkan::create_runtime_pipeline_layout(device_result.device, pipeline_layout_desc);
    MK_REQUIRE(!pipeline_layout_result.diagnostic.empty());
    if (!pipeline_layout_result.created) {
        return;
    }
    MK_REQUIRE(pipeline_layout_result.layout.descriptor_set_layout_count() == 1);

    const auto missing_pool = mirakana::rhi::vulkan::record_runtime_descriptor_set_binding(
        device_result.device, empty_pool, pipeline_layout_result.layout, set_result.set, {});
    MK_REQUIRE(!missing_pool.recorded);
    MK_REQUIRE(missing_pool.diagnostic == "Vulkan runtime command pool is required");

    auto pool_result = mirakana::rhi::vulkan::create_runtime_command_pool(device_result.device, {});
    if (pool_result.created) {
        const auto not_recording = mirakana::rhi::vulkan::record_runtime_descriptor_set_binding(
            device_result.device, pool_result.pool, pipeline_layout_result.layout, set_result.set, {});
        MK_REQUIRE(!not_recording.recorded);
        MK_REQUIRE(not_recording.diagnostic == "Vulkan command buffer must be recording");

        if (pool_result.pool.begin_primary_command_buffer()) {
            const auto bound = mirakana::rhi::vulkan::record_runtime_descriptor_set_binding(
                device_result.device, pool_result.pool, pipeline_layout_result.layout, set_result.set, {});
            MK_REQUIRE(bound.recorded);
            MK_REQUIRE(bound.diagnostic == "Vulkan descriptor set binding recorded");
        }
    } else {
        MK_REQUIRE(!pool_result.diagnostic.empty());
    }
}

MK_TEST("vulkan runtime shader module owner validates spirv and hides native handles") {
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeShaderModule>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeShaderModule>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeShaderModule>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeShaderModule>);

    constexpr std::array<std::uint32_t, 5> valid_spirv{
        0x07230203U, 0x00010000U, 0U, 1U, 0U,
    };

    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    const auto no_device = mirakana::rhi::vulkan::create_runtime_shader_module(
        empty_device, mirakana::rhi::vulkan::VulkanRuntimeShaderModuleDesc{
                          .stage = mirakana::rhi::ShaderStage::vertex,
                          .bytecode = valid_spirv.data(),
                          .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
                      });
    MK_REQUIRE(!no_device.created);
    MK_REQUIRE(!no_device.module.owns_module());
    MK_REQUIRE(no_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeShaderModuleOwner";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    auto device_result = mirakana::rhi::vulkan::create_runtime_device({}, instance_desc, {});
    const auto invalid_shader = mirakana::rhi::vulkan::create_runtime_shader_module(
        device_result.device, mirakana::rhi::vulkan::VulkanRuntimeShaderModuleDesc{
                                  .stage = mirakana::rhi::ShaderStage::fragment,
                                  .bytecode = nullptr,
                                  .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
                              });
    MK_REQUIRE(!invalid_shader.created);
    MK_REQUIRE(!invalid_shader.validation.valid);
    MK_REQUIRE(invalid_shader.diagnostic == "SPIR-V bytecode is required");

    auto module_result = mirakana::rhi::vulkan::create_runtime_shader_module(
        device_result.device, mirakana::rhi::vulkan::VulkanRuntimeShaderModuleDesc{
                                  .stage = mirakana::rhi::ShaderStage::vertex,
                                  .bytecode = valid_spirv.data(),
                                  .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
                              });

    MK_REQUIRE(!module_result.diagnostic.empty());
    MK_REQUIRE(module_result.created == module_result.module.owns_module());
    if (module_result.created) {
        MK_REQUIRE(module_result.validation.valid);
        MK_REQUIRE(module_result.module.stage() == mirakana::rhi::ShaderStage::vertex);
        MK_REQUIRE(module_result.module.bytecode_size() == valid_spirv.size() * sizeof(std::uint32_t));
        module_result.module.reset();
        MK_REQUIRE(!module_result.module.owns_module());
        MK_REQUIRE(module_result.module.destroyed());
    }
}

MK_TEST("vulkan runtime pipeline layout and graphics pipeline owners hide native handles") {
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimePipelineLayout>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimePipelineLayout>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimePipelineLayout>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimePipelineLayout>);
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeGraphicsPipeline>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeGraphicsPipeline>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeGraphicsPipeline>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeGraphicsPipeline>);
    static_assert(!has_legacy_descriptor_set_layout_count_v<mirakana::rhi::vulkan::VulkanRuntimePipelineLayoutDesc>);

    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    const auto empty_layout = mirakana::rhi::vulkan::create_runtime_pipeline_layout(
        empty_device, mirakana::rhi::vulkan::VulkanRuntimePipelineLayoutDesc{});
    MK_REQUIRE(!empty_layout.created);
    MK_REQUIRE(empty_layout.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeGraphicsPipelineOwner";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    auto device_result = mirakana::rhi::vulkan::create_runtime_device({}, instance_desc, {});
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    mirakana::rhi::vulkan::VulkanRuntimePipelineLayoutDesc invalid_layout_desc;
    invalid_layout_desc.descriptor_set_layouts.push_back(nullptr);
    auto invalid_layout =
        mirakana::rhi::vulkan::create_runtime_pipeline_layout(device_result.device, invalid_layout_desc);
    MK_REQUIRE(!invalid_layout.created);
    MK_REQUIRE(invalid_layout.diagnostic == "Vulkan runtime descriptor set layout is required");

    auto layout_result = mirakana::rhi::vulkan::create_runtime_pipeline_layout(
        device_result.device, mirakana::rhi::vulkan::VulkanRuntimePipelineLayoutDesc{});
    if (!layout_result.created) {
        MK_REQUIRE(!layout_result.diagnostic.empty());
        return;
    }
    MK_REQUIRE(layout_result.layout.owns_layout());
    MK_REQUIRE(layout_result.layout.descriptor_set_layout_count() == 0);
    MK_REQUIRE(layout_result.layout.push_constant_bytes() == 0);

    mirakana::rhi::vulkan::VulkanDynamicRenderingDesc rendering_desc;
    rendering_desc.extent = mirakana::rhi::Extent2D{.width = 16, .height = 16};
    rendering_desc.color_attachments.push_back(mirakana::rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
        .format = mirakana::rhi::Format::bgra8_unorm,
        .load_action = mirakana::rhi::LoadAction::clear,
        .store_action = mirakana::rhi::StoreAction::store,
    });
    const auto dynamic_rendering =
        mirakana::rhi::vulkan::build_dynamic_rendering_plan(rendering_desc, device_result.device.command_plan());

    mirakana::rhi::vulkan::VulkanRuntimeShaderModule empty_vertex;
    mirakana::rhi::vulkan::VulkanRuntimeShaderModule empty_fragment;
    auto missing_shader = mirakana::rhi::vulkan::create_runtime_graphics_pipeline(
        device_result.device, layout_result.layout, empty_vertex, empty_fragment,
        mirakana::rhi::vulkan::VulkanRuntimeGraphicsPipelineDesc{
            .dynamic_rendering = dynamic_rendering,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        });
    MK_REQUIRE(!missing_shader.created);
    MK_REQUIRE(missing_shader.diagnostic == "Vulkan vertex shader module is required");

    constexpr std::array<std::uint32_t, 5> valid_spirv{
        0x07230203U, 0x00010000U, 0U, 1U, 0U,
    };
    auto vertex = mirakana::rhi::vulkan::create_runtime_shader_module(
        device_result.device, mirakana::rhi::vulkan::VulkanRuntimeShaderModuleDesc{
                                  .stage = mirakana::rhi::ShaderStage::vertex,
                                  .bytecode = valid_spirv.data(),
                                  .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
                              });
    auto fragment = mirakana::rhi::vulkan::create_runtime_shader_module(
        device_result.device, mirakana::rhi::vulkan::VulkanRuntimeShaderModuleDesc{
                                  .stage = mirakana::rhi::ShaderStage::fragment,
                                  .bytecode = valid_spirv.data(),
                                  .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
                              });
    if (!vertex.created || !fragment.created || !dynamic_rendering.supported) {
        MK_REQUIRE(!vertex.diagnostic.empty());
        MK_REQUIRE(!fragment.diagnostic.empty());
        MK_REQUIRE(!dynamic_rendering.diagnostic.empty());
        return;
    }

    auto pipeline_result = mirakana::rhi::vulkan::create_runtime_graphics_pipeline(
        device_result.device, layout_result.layout, vertex.module, fragment.module,
        mirakana::rhi::vulkan::VulkanRuntimeGraphicsPipelineDesc{
            .dynamic_rendering = dynamic_rendering,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        });
    MK_REQUIRE(!pipeline_result.diagnostic.empty());
    MK_REQUIRE(pipeline_result.created == pipeline_result.pipeline.owns_pipeline());
    if (!pipeline_result.created) {
        return;
    }

    MK_REQUIRE(pipeline_result.pipeline.color_format() == mirakana::rhi::Format::bgra8_unorm);
    MK_REQUIRE(pipeline_result.pipeline.depth_format() == mirakana::rhi::Format::unknown);
    MK_REQUIRE(pipeline_result.pipeline.topology() == mirakana::rhi::PrimitiveTopology::triangle_list);

    pipeline_result.pipeline.reset();
    MK_REQUIRE(!pipeline_result.pipeline.owns_pipeline());
    MK_REQUIRE(pipeline_result.pipeline.destroyed());
}

MK_TEST("vulkan runtime swapchain owns surface images and image views without exposing handles") {
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeSwapchain>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeSwapchain>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeSwapchain>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeSwapchain>);

    mirakana::rhi::vulkan::VulkanSwapchainCreatePlan supported_plan;
    supported_plan.supported = true;
    supported_plan.extent = mirakana::rhi::Extent2D{.width = 64, .height = 64};
    supported_plan.format = mirakana::rhi::Format::bgra8_unorm;
    supported_plan.image_count = 2;
    supported_plan.image_view_count = 2;
    supported_plan.present_mode = mirakana::rhi::vulkan::VulkanPresentMode::fifo;
    supported_plan.acquire_before_render = true;
    supported_plan.diagnostic = "Vulkan swapchain create plan ready";

    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    const auto missing_surface =
        mirakana::rhi::vulkan::create_runtime_swapchain(empty_device, mirakana::rhi::vulkan::VulkanRuntimeSwapchainDesc{
                                                                          .surface = mirakana::rhi::SurfaceHandle{},
                                                                          .plan = supported_plan,
                                                                      });
    MK_REQUIRE(!missing_surface.created);
    MK_REQUIRE(missing_surface.diagnostic == "Vulkan runtime swapchain surface handle is required");

    const auto unsupported_plan = mirakana::rhi::vulkan::create_runtime_swapchain(
        empty_device, mirakana::rhi::vulkan::VulkanRuntimeSwapchainDesc{
                          .surface = mirakana::rhi::SurfaceHandle{1},
                          .plan = mirakana::rhi::vulkan::VulkanSwapchainCreatePlan{},
                      });
    MK_REQUIRE(!unsupported_plan.created);
    MK_REQUIRE(unsupported_plan.diagnostic == "Vulkan swapchain create plan is required");

    const auto no_device =
        mirakana::rhi::vulkan::create_runtime_swapchain(empty_device, mirakana::rhi::vulkan::VulkanRuntimeSwapchainDesc{
                                                                          .surface = mirakana::rhi::SurfaceHandle{1},
                                                                          .plan = supported_plan,
                                                                      });
    MK_REQUIRE(!no_device.created);
    MK_REQUIRE(no_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineRuntimeSwapchainOwner";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    auto device_result = mirakana::rhi::vulkan::create_runtime_device({}, instance_desc, {});
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    const auto unsupported_host = mirakana::rhi::vulkan::create_runtime_swapchain(
        device_result.device, mirakana::rhi::vulkan::VulkanRuntimeSwapchainDesc{
                                  .surface = mirakana::rhi::SurfaceHandle{1},
                                  .plan = supported_plan,
                              });
    MK_REQUIRE(!unsupported_host.created);
    MK_REQUIRE(!unsupported_host.diagnostic.empty());
    MK_REQUIRE(unsupported_host.created == unsupported_host.swapchain.owns_swapchain());
}

MK_TEST("vulkan runtime frame sync owns semaphores fence and gates swapchain acquire") {
    static_assert(!std::is_copy_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeFrameSync>);
    static_assert(!std::is_copy_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeFrameSync>);
    static_assert(std::is_move_constructible_v<mirakana::rhi::vulkan::VulkanRuntimeFrameSync>);
    static_assert(std::is_move_assignable_v<mirakana::rhi::vulkan::VulkanRuntimeFrameSync>);

    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    const auto missing_device = mirakana::rhi::vulkan::create_runtime_frame_sync(empty_device);
    MK_REQUIRE(!missing_device.created);
    MK_REQUIRE(missing_device.diagnostic == "Vulkan runtime device is not available");

    mirakana::rhi::vulkan::VulkanRuntimeFrameSync empty_sync;
    mirakana::rhi::vulkan::VulkanRuntimeSwapchain empty_swapchain;
    const auto missing_acquire_device =
        mirakana::rhi::vulkan::acquire_next_runtime_swapchain_image(empty_device, empty_swapchain, empty_sync);
    MK_REQUIRE(!missing_acquire_device.acquired);
    MK_REQUIRE(missing_acquire_device.diagnostic == "Vulkan runtime device is not available");

    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, {}, {},
        mirakana::rhi::SurfaceHandle{1});
    if (device_result.created) {
        auto sync_result = mirakana::rhi::vulkan::create_runtime_frame_sync(device_result.device);
        if (sync_result.created) {
            MK_REQUIRE(sync_result.sync.owns_image_available_semaphore());
            MK_REQUIRE(sync_result.sync.owns_render_finished_semaphore());
            MK_REQUIRE(sync_result.sync.owns_in_flight_fence());
            MK_REQUIRE(sync_result.diagnostic == "Vulkan runtime frame sync owner ready");

            const auto missing_swapchain = mirakana::rhi::vulkan::acquire_next_runtime_swapchain_image(
                device_result.device, empty_swapchain, sync_result.sync);
            MK_REQUIRE(!missing_swapchain.acquired);
            MK_REQUIRE(missing_swapchain.diagnostic == "Vulkan runtime swapchain is required");
        } else {
            MK_REQUIRE(!sync_result.diagnostic.empty());
        }
    }
}

MK_TEST("vulkan runtime queue present maps results without exposing native handles") {
    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;
    mirakana::rhi::vulkan::VulkanRuntimeFrameSync empty_sync;
    mirakana::rhi::vulkan::VulkanRuntimeSwapchain empty_swapchain;
    const auto missing_device =
        mirakana::rhi::vulkan::present_runtime_swapchain_image(empty_device, empty_swapchain, empty_sync);
    MK_REQUIRE(!missing_device.presented);
    MK_REQUIRE(missing_device.diagnostic == "Vulkan runtime device is not available");

    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, {}, {},
        mirakana::rhi::SurfaceHandle{1});
    if (device_result.created) {
        auto sync_result = mirakana::rhi::vulkan::create_runtime_frame_sync(device_result.device);
        if (sync_result.created) {
            const auto missing_swapchain = mirakana::rhi::vulkan::present_runtime_swapchain_image(
                device_result.device, empty_swapchain, sync_result.sync);
            MK_REQUIRE(!missing_swapchain.presented);
            MK_REQUIRE(missing_swapchain.diagnostic == "Vulkan runtime swapchain is required");
        } else {
            MK_REQUIRE(!sync_result.diagnostic.empty());
        }
    }
}

MK_TEST("vulkan runtime physical device count probe enumerates devices without exposing handles") {
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc desc;
    desc.application_name = "GameEnginePhysicalDeviceCountProbe";
    desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    desc.required_extensions = {};

    const auto result = mirakana::rhi::vulkan::probe_runtime_physical_device_count({}, desc);

    MK_REQUIRE(result.instance.capabilities.global.loader.probe.backend == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(result.instance.capabilities.global.loader.probe.host == mirakana::rhi::current_rhi_host_platform());
    MK_REQUIRE(!result.diagnostic.empty());
    if (result.enumerated) {
        MK_REQUIRE(result.instance.instance_created);
        MK_REQUIRE(result.instance.instance_destroyed);
    }
}

MK_TEST("vulkan runtime physical device snapshot probe maps queues and device extensions without exposing handles") {
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc desc;
    desc.application_name = "GameEnginePhysicalDeviceSnapshotProbe";
    desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    desc.required_extensions = {};

    const auto result = mirakana::rhi::vulkan::probe_runtime_physical_device_snapshots({}, desc);

    MK_REQUIRE(result.count_probe.instance.capabilities.global.loader.probe.backend ==
               mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(result.count_probe.instance.capabilities.global.loader.probe.host ==
               mirakana::rhi::current_rhi_host_platform());
    MK_REQUIRE(!result.diagnostic.empty());
    if (result.enumerated) {
        MK_REQUIRE(result.count_probe.enumerated);
        MK_REQUIRE(result.devices.size() == result.count_probe.physical_device_count);
        for (std::size_t device_index = 0; device_index < result.devices.size(); ++device_index) {
            const auto& device = result.devices[device_index];
            MK_REQUIRE(device.device_index == device_index);
            bool extension_list_contains_swapchain = false;
            for (const auto& extension : device.device_extensions) {
                extension_list_contains_swapchain =
                    extension_list_contains_swapchain || extension == "VK_KHR_swapchain";
            }
            MK_REQUIRE(device.supports_swapchain_extension == extension_list_contains_swapchain);
            mirakana::rhi::vulkan::VulkanPhysicalDeviceCandidate candidate;
            candidate.supports_swapchain_extension = device.supports_swapchain_extension;
            candidate.supports_dynamic_rendering = device.supports_dynamic_rendering;
            candidate.supports_synchronization2 = device.supports_synchronization2;
            MK_REQUIRE(candidate.supports_dynamic_rendering == device.supports_dynamic_rendering);
            MK_REQUIRE(candidate.supports_synchronization2 == device.supports_synchronization2);
            for (const auto& queue_family : device.queue_families) {
                MK_REQUIRE(queue_family.index != mirakana::rhi::vulkan::invalid_vulkan_queue_family);
                MK_REQUIRE(queue_family.queue_count > 0);
                MK_REQUIRE(!queue_family.supports_present);
            }
        }
    }
}

MK_TEST("vulkan runtime physical device snapshots expose properties for deterministic selection") {
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc desc;
    desc.application_name = "GameEnginePhysicalDevicePropertiesProbe";
    desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    desc.required_extensions = {};

    const auto result = mirakana::rhi::vulkan::probe_runtime_physical_device_snapshots({}, desc);

    MK_REQUIRE(!result.diagnostic.empty());
    if (result.enumerated) {
        for (const auto& device : result.devices) {
            MK_REQUIRE(!device.name.empty());
            MK_REQUIRE(device.api_version.major >= 1);

            const auto candidate = mirakana::rhi::vulkan::make_physical_device_candidate(device);

            MK_REQUIRE(candidate.name == device.name);
            MK_REQUIRE(candidate.type == device.type);
            MK_REQUIRE(candidate.api_version.major == device.api_version.major);
            MK_REQUIRE(candidate.api_version.minor == device.api_version.minor);
            MK_REQUIRE(candidate.supports_swapchain_extension == device.supports_swapchain_extension);
            MK_REQUIRE(candidate.supports_dynamic_rendering == device.supports_dynamic_rendering);
            MK_REQUIRE(candidate.supports_synchronization2 == device.supports_synchronization2);
            MK_REQUIRE(candidate.queue_families.size() == device.queue_families.size());
        }
    }
}

MK_TEST("vulkan runtime physical device selection probe projects snapshots into candidates") {
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc desc;
    desc.application_name = "GameEnginePhysicalDeviceSelectionProbe";
    desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    desc.required_extensions = {};

    const auto result = mirakana::rhi::vulkan::probe_runtime_physical_device_selection({}, desc);

    MK_REQUIRE(!result.diagnostic.empty());
    if (result.snapshots.enumerated) {
        MK_REQUIRE(result.candidates.size() == result.snapshots.devices.size());
        for (std::size_t index = 0; index < result.candidates.size(); ++index) {
            const auto& candidate = result.candidates[index];
            const auto& snapshot = result.snapshots.devices[index];
            MK_REQUIRE(candidate.name == snapshot.name);
            MK_REQUIRE(candidate.type == snapshot.type);
            MK_REQUIRE(candidate.api_version.major == snapshot.api_version.major);
            MK_REQUIRE(candidate.api_version.minor == snapshot.api_version.minor);
            MK_REQUIRE(candidate.supports_swapchain_extension == snapshot.supports_swapchain_extension);
            MK_REQUIRE(candidate.supports_dynamic_rendering == snapshot.supports_dynamic_rendering);
            MK_REQUIRE(candidate.supports_synchronization2 == snapshot.supports_synchronization2);
            MK_REQUIRE(candidate.queue_families.size() == snapshot.queue_families.size());
        }
        MK_REQUIRE(result.selected == result.selection.suitable);
        MK_REQUIRE(!result.selection.diagnostic.empty());
    }
}

MK_TEST("vulkan surface support probe plans platform extensions and rejects empty surfaces") {
    const auto windows_extensions =
        mirakana::rhi::vulkan::vulkan_surface_instance_extensions(mirakana::rhi::RhiHostPlatform::windows);
    const auto android_extensions =
        mirakana::rhi::vulkan::vulkan_surface_instance_extensions(mirakana::rhi::RhiHostPlatform::android);
    const auto macos_extensions =
        mirakana::rhi::vulkan::vulkan_surface_instance_extensions(mirakana::rhi::RhiHostPlatform::macos);

    MK_REQUIRE(windows_extensions.size() == 2);
    MK_REQUIRE(windows_extensions[0] == "VK_KHR_surface");
    MK_REQUIRE(windows_extensions[1] == "VK_KHR_win32_surface");
    MK_REQUIRE(android_extensions.size() == 2);
    MK_REQUIRE(android_extensions[0] == "VK_KHR_surface");
    MK_REQUIRE(android_extensions[1] == "VK_KHR_android_surface");
    MK_REQUIRE(macos_extensions.empty());

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc desc;
    desc.application_name = "GameEngineSurfaceSupportProbe";
    desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    desc.required_extensions = {};

    const auto result = mirakana::rhi::vulkan::probe_runtime_surface_support({}, desc, mirakana::rhi::SurfaceHandle{});

    MK_REQUIRE(!result.probed);
    MK_REQUIRE(!result.surface_created);
    MK_REQUIRE(!result.surface_destroyed);
    MK_REQUIRE(!result.diagnostic.empty());
}

MK_TEST("vulkan swapchain create plan selects extent format image count present mode and image views") {
    mirakana::rhi::vulkan::VulkanSwapchainSupport support;
    support.capabilities.min_image_count = 2;
    support.capabilities.max_image_count = 3;
    support.capabilities.min_image_extent = mirakana::rhi::Extent2D{.width = 640, .height = 360};
    support.capabilities.max_image_extent = mirakana::rhi::Extent2D{.width = 3840, .height = 2160};
    support.formats = {
        mirakana::rhi::vulkan::VulkanSurfaceFormatCandidate{.format = mirakana::rhi::Format::rgba8_unorm},
        mirakana::rhi::vulkan::VulkanSurfaceFormatCandidate{.format = mirakana::rhi::Format::bgra8_unorm},
    };
    support.present_modes = {
        mirakana::rhi::vulkan::VulkanPresentMode::fifo,
        mirakana::rhi::vulkan::VulkanPresentMode::mailbox,
        mirakana::rhi::vulkan::VulkanPresentMode::immediate,
    };

    mirakana::rhi::vulkan::VulkanSwapchainCreateDesc desc;
    desc.requested_extent = mirakana::rhi::Extent2D{.width = 4096, .height = 1440};
    desc.preferred_format = mirakana::rhi::Format::bgra8_unorm;
    desc.requested_image_count = 4;
    desc.vsync = false;

    const auto plan = mirakana::rhi::vulkan::build_swapchain_create_plan(desc, support);
    const auto no_resize = mirakana::rhi::vulkan::build_swapchain_resize_plan(plan, plan.extent);
    const auto resize =
        mirakana::rhi::vulkan::build_swapchain_resize_plan(plan, mirakana::rhi::Extent2D{.width = 1280, .height = 720});

    MK_REQUIRE(plan.supported);
    MK_REQUIRE(plan.extent.width == 3840);
    MK_REQUIRE(plan.extent.height == 1440);
    MK_REQUIRE(plan.format == mirakana::rhi::Format::bgra8_unorm);
    MK_REQUIRE(plan.image_count == 3);
    MK_REQUIRE(plan.image_view_count == 3);
    MK_REQUIRE(plan.present_mode == mirakana::rhi::vulkan::VulkanPresentMode::mailbox);
    MK_REQUIRE(plan.acquire_before_render);
    MK_REQUIRE(plan.diagnostic == "Vulkan swapchain create plan ready");

    MK_REQUIRE(!no_resize.resize_required);
    MK_REQUIRE(no_resize.extent.width == plan.extent.width);
    MK_REQUIRE(no_resize.extent.height == plan.extent.height);
    MK_REQUIRE(resize.resize_required);
    MK_REQUIRE(resize.extent.width == 1280);
    MK_REQUIRE(resize.extent.height == 720);
}

MK_TEST("vulkan swapchain create plan rejects missing surface formats present modes and extents") {
    mirakana::rhi::vulkan::VulkanSwapchainCreateDesc desc;
    desc.requested_extent = mirakana::rhi::Extent2D{.width = 1280, .height = 720};

    mirakana::rhi::vulkan::VulkanSwapchainSupport support;
    support.capabilities.min_image_count = 2;
    support.capabilities.max_image_count = 3;
    support.capabilities.min_image_extent = mirakana::rhi::Extent2D{.width = 1, .height = 1};
    support.capabilities.max_image_extent = mirakana::rhi::Extent2D{.width = 3840, .height = 2160};
    support.present_modes = {mirakana::rhi::vulkan::VulkanPresentMode::fifo};

    const auto missing_format = mirakana::rhi::vulkan::build_swapchain_create_plan(desc, support);
    MK_REQUIRE(!missing_format.supported);
    MK_REQUIRE(missing_format.diagnostic == "Vulkan surface exposes no color formats");

    support.formats = {
        mirakana::rhi::vulkan::VulkanSurfaceFormatCandidate{.format = mirakana::rhi::Format::rgba8_unorm}};
    support.present_modes.clear();
    const auto missing_present_mode = mirakana::rhi::vulkan::build_swapchain_create_plan(desc, support);
    MK_REQUIRE(!missing_present_mode.supported);
    MK_REQUIRE(missing_present_mode.diagnostic == "Vulkan surface exposes no present modes");

    support.present_modes = {mirakana::rhi::vulkan::VulkanPresentMode::fifo};
    desc.requested_extent = mirakana::rhi::Extent2D{};
    const auto missing_extent = mirakana::rhi::vulkan::build_swapchain_create_plan(desc, support);
    MK_REQUIRE(!missing_extent.supported);
    MK_REQUIRE(missing_extent.diagnostic == "Vulkan swapchain extent is required");
}

MK_TEST("vulkan spirv shader artifact validation rejects malformed bytecode before native shader modules") {
    constexpr std::array<std::uint32_t, 5> valid_spirv{
        0x07230203U, 0x00010000U, 0U, 1U, 0U,
    };
    const auto valid =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = valid_spirv.data(),
            .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
        });

    MK_REQUIRE(valid.valid);
    MK_REQUIRE(valid.word_count == valid_spirv.size());
    MK_REQUIRE(valid.diagnostic == "SPIR-V shader artifact ready");

    const auto missing =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = nullptr,
            .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(!missing.valid);
    MK_REQUIRE(missing.diagnostic == "SPIR-V bytecode is required");

    const auto unaligned =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = valid_spirv.data(),
            .bytecode_size = (valid_spirv.size() * sizeof(std::uint32_t)) - 1,
        });
    MK_REQUIRE(!unaligned.valid);
    MK_REQUIRE(unaligned.diagnostic == "SPIR-V bytecode size must be a multiple of 4 bytes");

    constexpr std::array<std::uint32_t, 5> invalid_magic{
        0x00000000U, 0x00010000U, 0U, 1U, 0U,
    };
    const auto wrong_magic =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::compute,
            .bytecode = invalid_magic.data(),
            .bytecode_size = invalid_magic.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(!wrong_magic.valid);
    MK_REQUIRE(wrong_magic.diagnostic == "SPIR-V magic word is invalid");
}

MK_TEST("vulkan dynamic rendering plan requires begin and end commands plus valid attachments") {
    const auto requests = mirakana::rhi::vulkan::vulkan_backend_command_requests();
    std::vector<mirakana::rhi::vulkan::VulkanCommandAvailability> available;
    available.reserve(requests.size());
    for (const auto& request : requests) {
        available.push_back(mirakana::rhi::vulkan::VulkanCommandAvailability{
            .name = request.name, .scope = request.scope, .available = true});
    }
    const auto commands = mirakana::rhi::vulkan::build_command_resolution_plan(requests, available);

    mirakana::rhi::vulkan::VulkanDynamicRenderingDesc desc;
    desc.extent = mirakana::rhi::Extent2D{.width = 1280, .height = 720};
    desc.color_attachments.push_back(mirakana::rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
        .format = mirakana::rhi::Format::bgra8_unorm,
        .load_action = mirakana::rhi::LoadAction::clear,
        .store_action = mirakana::rhi::StoreAction::store,
    });

    const auto plan = mirakana::rhi::vulkan::build_dynamic_rendering_plan(desc, commands);

    MK_REQUIRE(plan.supported);
    MK_REQUIRE(plan.extent.width == 1280);
    MK_REQUIRE(plan.extent.height == 720);
    MK_REQUIRE(plan.color_attachment_count == 1);
    MK_REQUIRE(plan.color_formats.size() == 1);
    MK_REQUIRE(plan.color_formats[0] == mirakana::rhi::Format::bgra8_unorm);
    MK_REQUIRE(!plan.depth_attachment_enabled);
    MK_REQUIRE(plan.begin_rendering_command_resolved);
    MK_REQUIRE(plan.end_rendering_command_resolved);
    MK_REQUIRE(plan.diagnostic == "Vulkan dynamic rendering plan ready");

    std::vector<mirakana::rhi::vulkan::VulkanCommandAvailability> missing_end_available;
    missing_end_available.reserve(requests.size());
    for (const auto& request : requests) {
        missing_end_available.push_back(mirakana::rhi::vulkan::VulkanCommandAvailability{
            .name = request.name,
            .scope = request.scope,
            .available = request.name != "vkCmdEndRendering",
        });
    }
    const auto missing_end_commands =
        mirakana::rhi::vulkan::build_command_resolution_plan(requests, missing_end_available);
    const auto missing_end = mirakana::rhi::vulkan::build_dynamic_rendering_plan(desc, missing_end_commands);

    MK_REQUIRE(!missing_end.supported);
    MK_REQUIRE(!missing_end.end_rendering_command_resolved);
    MK_REQUIRE(missing_end.diagnostic == "Vulkan dynamic rendering commands are unavailable");
}

MK_TEST("vulkan dynamic rendering plan rejects empty extent missing colors and unsupported formats") {
    const auto requests = mirakana::rhi::vulkan::vulkan_backend_command_requests();
    std::vector<mirakana::rhi::vulkan::VulkanCommandAvailability> available;
    available.reserve(requests.size());
    for (const auto& request : requests) {
        available.push_back(mirakana::rhi::vulkan::VulkanCommandAvailability{
            .name = request.name, .scope = request.scope, .available = true});
    }
    const auto commands = mirakana::rhi::vulkan::build_command_resolution_plan(requests, available);

    mirakana::rhi::vulkan::VulkanDynamicRenderingDesc desc;
    desc.extent = mirakana::rhi::Extent2D{};
    desc.color_attachments.push_back(mirakana::rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
        .format = mirakana::rhi::Format::bgra8_unorm,
        .load_action = mirakana::rhi::LoadAction::clear,
        .store_action = mirakana::rhi::StoreAction::store,
    });
    const auto empty_extent = mirakana::rhi::vulkan::build_dynamic_rendering_plan(desc, commands);
    MK_REQUIRE(!empty_extent.supported);
    MK_REQUIRE(empty_extent.diagnostic == "Vulkan dynamic rendering extent is required");

    desc.extent = mirakana::rhi::Extent2D{.width = 640, .height = 360};
    desc.color_attachments.clear();
    const auto missing_color = mirakana::rhi::vulkan::build_dynamic_rendering_plan(desc, commands);
    MK_REQUIRE(!missing_color.supported);
    MK_REQUIRE(missing_color.diagnostic == "Vulkan dynamic rendering requires at least one color attachment");

    desc.color_attachments.push_back(mirakana::rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
        .format = mirakana::rhi::Format::unknown,
        .load_action = mirakana::rhi::LoadAction::clear,
        .store_action = mirakana::rhi::StoreAction::store,
    });
    const auto unsupported_format = mirakana::rhi::vulkan::build_dynamic_rendering_plan(desc, commands);
    MK_REQUIRE(!unsupported_format.supported);
    MK_REQUIRE(unsupported_format.diagnostic == "Vulkan dynamic rendering color attachment format is unsupported");

    desc.color_attachments[0].format = mirakana::rhi::Format::rgba8_unorm;
    desc.has_depth_attachment = true;
    desc.depth_format = mirakana::rhi::Format::unknown;
    const auto unsupported_depth = mirakana::rhi::vulkan::build_dynamic_rendering_plan(desc, commands);
    MK_REQUIRE(!unsupported_depth.supported);
    MK_REQUIRE(unsupported_depth.diagnostic == "Vulkan dynamic rendering depth attachment format is unsupported");

    desc.depth_format = mirakana::rhi::Format::depth24_stencil8;
    const auto color_depth = mirakana::rhi::vulkan::build_dynamic_rendering_plan(desc, commands);
    MK_REQUIRE(color_depth.supported);
    MK_REQUIRE(color_depth.depth_attachment_enabled);
    MK_REQUIRE(color_depth.depth_format == mirakana::rhi::Format::depth24_stencil8);
}

MK_TEST("vulkan frame synchronization2 plan orders acquire render readback and present") {
    const auto requests = mirakana::rhi::vulkan::vulkan_backend_command_requests();
    std::vector<mirakana::rhi::vulkan::VulkanCommandAvailability> available;
    available.reserve(requests.size());
    for (const auto& request : requests) {
        available.push_back(mirakana::rhi::vulkan::VulkanCommandAvailability{
            .name = request.name, .scope = request.scope, .available = true});
    }
    const auto commands = mirakana::rhi::vulkan::build_command_resolution_plan(requests, available);

    mirakana::rhi::vulkan::VulkanFrameSynchronizationDesc desc;
    desc.readback_required = true;
    desc.present_required = true;

    const auto plan = mirakana::rhi::vulkan::build_frame_synchronization_plan(desc, commands);

    MK_REQUIRE(plan.supported);
    MK_REQUIRE(plan.pipeline_barrier2_command_resolved);
    MK_REQUIRE(plan.queue_submit2_command_resolved);
    MK_REQUIRE(plan.order.size() == 4);
    MK_REQUIRE(plan.order[0] == mirakana::rhi::vulkan::VulkanFrameSynchronizationStep::acquire);
    MK_REQUIRE(plan.order[1] == mirakana::rhi::vulkan::VulkanFrameSynchronizationStep::render);
    MK_REQUIRE(plan.order[2] == mirakana::rhi::vulkan::VulkanFrameSynchronizationStep::readback);
    MK_REQUIRE(plan.order[3] == mirakana::rhi::vulkan::VulkanFrameSynchronizationStep::present);
    MK_REQUIRE(plan.barriers.size() == 3);
    MK_REQUIRE(plan.barriers[0].before == mirakana::rhi::ResourceState::present);
    MK_REQUIRE(plan.barriers[0].after == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(plan.barriers[0].dst_stage ==
               mirakana::rhi::vulkan::VulkanSynchronizationStage::color_attachment_output);
    MK_REQUIRE(plan.barriers[0].dst_access ==
               mirakana::rhi::vulkan::VulkanSynchronizationAccess::color_attachment_write);
    MK_REQUIRE(plan.barriers[1].before == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(plan.barriers[1].after == mirakana::rhi::ResourceState::copy_source);
    MK_REQUIRE(plan.barriers[1].src_access ==
               mirakana::rhi::vulkan::VulkanSynchronizationAccess::color_attachment_write);
    MK_REQUIRE(plan.barriers[1].dst_access == mirakana::rhi::vulkan::VulkanSynchronizationAccess::transfer_read);
    MK_REQUIRE(plan.barriers[2].before == mirakana::rhi::ResourceState::copy_source);
    MK_REQUIRE(plan.barriers[2].after == mirakana::rhi::ResourceState::present);
    MK_REQUIRE(plan.diagnostic == "Vulkan frame synchronization2 plan ready");
}

MK_TEST("vulkan texture transition barrier maps shader read to shader access hazards") {
    const auto upload_to_sample = mirakana::rhi::vulkan::build_texture_transition_barrier(
        mirakana::rhi::ResourceState::copy_destination, mirakana::rhi::ResourceState::shader_read);

    MK_REQUIRE(upload_to_sample.supported);
    MK_REQUIRE(upload_to_sample.barrier.before == mirakana::rhi::ResourceState::copy_destination);
    MK_REQUIRE(upload_to_sample.barrier.after == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(upload_to_sample.barrier.src_stage == mirakana::rhi::vulkan::VulkanSynchronizationStage::transfer);
    MK_REQUIRE(upload_to_sample.barrier.src_access ==
               mirakana::rhi::vulkan::VulkanSynchronizationAccess::transfer_write);
    MK_REQUIRE(upload_to_sample.barrier.dst_stage == mirakana::rhi::vulkan::VulkanSynchronizationStage::shader);
    MK_REQUIRE(upload_to_sample.barrier.dst_access == mirakana::rhi::vulkan::VulkanSynchronizationAccess::shader_read);

    const auto sample_to_copy = mirakana::rhi::vulkan::build_texture_transition_barrier(
        mirakana::rhi::ResourceState::shader_read, mirakana::rhi::ResourceState::copy_source);

    MK_REQUIRE(sample_to_copy.supported);
    MK_REQUIRE(sample_to_copy.barrier.src_stage == mirakana::rhi::vulkan::VulkanSynchronizationStage::shader);
    MK_REQUIRE(sample_to_copy.barrier.src_access == mirakana::rhi::vulkan::VulkanSynchronizationAccess::shader_read);
    MK_REQUIRE(sample_to_copy.barrier.dst_stage == mirakana::rhi::vulkan::VulkanSynchronizationStage::transfer);
    MK_REQUIRE(sample_to_copy.barrier.dst_access == mirakana::rhi::vulkan::VulkanSynchronizationAccess::transfer_read);

    const auto depth_write = mirakana::rhi::vulkan::build_texture_transition_barrier(
        mirakana::rhi::ResourceState::undefined, mirakana::rhi::ResourceState::depth_write);

    MK_REQUIRE(depth_write.supported);
    MK_REQUIRE(depth_write.barrier.src_stage == mirakana::rhi::vulkan::VulkanSynchronizationStage::none);
    MK_REQUIRE(depth_write.barrier.src_access == mirakana::rhi::vulkan::VulkanSynchronizationAccess::none);
    MK_REQUIRE(depth_write.barrier.dst_stage == mirakana::rhi::vulkan::VulkanSynchronizationStage::depth_attachment);
    MK_REQUIRE(depth_write.barrier.dst_access ==
               mirakana::rhi::vulkan::VulkanSynchronizationAccess::depth_attachment_read_write);

    const auto depth_sample = mirakana::rhi::vulkan::build_texture_transition_barrier(
        mirakana::rhi::ResourceState::depth_write, mirakana::rhi::ResourceState::shader_read);

    MK_REQUIRE(depth_sample.supported);
    MK_REQUIRE(depth_sample.barrier.before == mirakana::rhi::ResourceState::depth_write);
    MK_REQUIRE(depth_sample.barrier.after == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(depth_sample.barrier.src_stage == mirakana::rhi::vulkan::VulkanSynchronizationStage::depth_attachment);
    MK_REQUIRE(depth_sample.barrier.src_access ==
               mirakana::rhi::vulkan::VulkanSynchronizationAccess::depth_attachment_read_write);
    MK_REQUIRE(depth_sample.barrier.dst_stage == mirakana::rhi::vulkan::VulkanSynchronizationStage::shader);
    MK_REQUIRE(depth_sample.barrier.dst_access == mirakana::rhi::vulkan::VulkanSynchronizationAccess::shader_read);
}

MK_TEST("vulkan frame synchronization2 plan rejects missing commands and omits readback when not requested") {
    const auto requests = mirakana::rhi::vulkan::vulkan_backend_command_requests();
    std::vector<mirakana::rhi::vulkan::VulkanCommandAvailability> available;
    available.reserve(requests.size());
    for (const auto& request : requests) {
        available.push_back(mirakana::rhi::vulkan::VulkanCommandAvailability{
            .name = request.name,
            .scope = request.scope,
            .available = request.name != "vkCmdPipelineBarrier2",
        });
    }
    const auto missing_commands = mirakana::rhi::vulkan::build_command_resolution_plan(requests, available);
    const auto rejected = mirakana::rhi::vulkan::build_frame_synchronization_plan(
        mirakana::rhi::vulkan::VulkanFrameSynchronizationDesc{}, missing_commands);

    MK_REQUIRE(!rejected.supported);
    MK_REQUIRE(!rejected.pipeline_barrier2_command_resolved);
    MK_REQUIRE(rejected.diagnostic == "Vulkan synchronization2 commands are unavailable");

    available.clear();
    for (const auto& request : requests) {
        available.push_back(mirakana::rhi::vulkan::VulkanCommandAvailability{
            .name = request.name, .scope = request.scope, .available = true});
    }
    const auto commands = mirakana::rhi::vulkan::build_command_resolution_plan(requests, available);
    mirakana::rhi::vulkan::VulkanFrameSynchronizationDesc no_readback;
    no_readback.present_required = true;

    const auto plan = mirakana::rhi::vulkan::build_frame_synchronization_plan(no_readback, commands);

    MK_REQUIRE(plan.supported);
    MK_REQUIRE(plan.order.size() == 3);
    MK_REQUIRE(plan.order[0] == mirakana::rhi::vulkan::VulkanFrameSynchronizationStep::acquire);
    MK_REQUIRE(plan.order[1] == mirakana::rhi::vulkan::VulkanFrameSynchronizationStep::render);
    MK_REQUIRE(plan.order[2] == mirakana::rhi::vulkan::VulkanFrameSynchronizationStep::present);
    MK_REQUIRE(plan.barriers.size() == 2);
    MK_REQUIRE(plan.barriers[1].before == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(plan.barriers[1].after == mirakana::rhi::ResourceState::present);
}

MK_TEST("vulkan rhi device mapping plan gates native backend promotion prerequisites") {
    const auto requests = mirakana::rhi::vulkan::vulkan_backend_command_requests();
    std::vector<mirakana::rhi::vulkan::VulkanCommandAvailability> available;
    available.reserve(requests.size());
    for (const auto& request : requests) {
        available.push_back(mirakana::rhi::vulkan::VulkanCommandAvailability{
            .name = request.name, .scope = request.scope, .available = true});
    }
    const auto commands = mirakana::rhi::vulkan::build_command_resolution_plan(requests, available);

    mirakana::rhi::vulkan::VulkanSwapchainSupport swapchain_support;
    swapchain_support.capabilities.min_image_count = 2;
    swapchain_support.capabilities.max_image_count = 3;
    swapchain_support.capabilities.min_image_extent = mirakana::rhi::Extent2D{.width = 1, .height = 1};
    swapchain_support.capabilities.max_image_extent = mirakana::rhi::Extent2D{.width = 1920, .height = 1080};
    swapchain_support.formats = {
        mirakana::rhi::vulkan::VulkanSurfaceFormatCandidate{.format = mirakana::rhi::Format::bgra8_unorm}};
    swapchain_support.present_modes = {mirakana::rhi::vulkan::VulkanPresentMode::fifo};
    mirakana::rhi::vulkan::VulkanSwapchainCreateDesc swapchain_desc;
    swapchain_desc.requested_extent = mirakana::rhi::Extent2D{.width = 1280, .height = 720};
    const auto swapchain_plan = mirakana::rhi::vulkan::build_swapchain_create_plan(swapchain_desc, swapchain_support);

    mirakana::rhi::vulkan::VulkanDynamicRenderingDesc rendering_desc;
    rendering_desc.extent = mirakana::rhi::Extent2D{.width = 1280, .height = 720};
    rendering_desc.color_attachments.push_back(mirakana::rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
        .format = mirakana::rhi::Format::bgra8_unorm,
        .load_action = mirakana::rhi::LoadAction::clear,
        .store_action = mirakana::rhi::StoreAction::store,
    });
    const auto rendering_plan = mirakana::rhi::vulkan::build_dynamic_rendering_plan(rendering_desc, commands);

    mirakana::rhi::vulkan::VulkanFrameSynchronizationDesc sync_desc;
    sync_desc.readback_required = true;
    const auto sync_plan = mirakana::rhi::vulkan::build_frame_synchronization_plan(sync_desc, commands);

    constexpr std::array<std::uint32_t, 5> spirv{
        0x07230203U, 0x00010000U, 0U, 1U, 0U,
    };
    const auto vertex_shader =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = spirv.data(),
            .bytecode_size = spirv.size() * sizeof(std::uint32_t),
        });
    const auto fragment_shader =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = spirv.data(),
            .bytecode_size = spirv.size() * sizeof(std::uint32_t),
        });
    const auto compute_shader =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::compute,
            .bytecode = spirv.data(),
            .bytecode_size = spirv.size() * sizeof(std::uint32_t),
        });

    mirakana::rhi::vulkan::VulkanRhiDeviceMappingDesc desc;
    desc.command_pool_ready = true;
    desc.swapchain = swapchain_plan;
    desc.dynamic_rendering = rendering_plan;
    desc.frame_synchronization = sync_plan;
    desc.vertex_shader = vertex_shader;
    desc.fragment_shader = fragment_shader;
    desc.compute_shader = compute_shader;
    desc.descriptor_binding_ready = true;
    desc.compute_dispatch_ready = true;
    desc.visible_clear_readback_ready = true;
    desc.visible_draw_readback_ready = true;
    desc.visible_texture_sampling_readback_ready = true;
    desc.visible_depth_readback_ready = true;

    const auto plan = mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);

    MK_REQUIRE(plan.supported);
    MK_REQUIRE(plan.resources_mapped);
    MK_REQUIRE(plan.swapchains_mapped);
    MK_REQUIRE(plan.render_passes_mapped);
    MK_REQUIRE(plan.pipelines_mapped);
    MK_REQUIRE(plan.command_lists_mapped);
    MK_REQUIRE(plan.fences_mapped);
    MK_REQUIRE(plan.readbacks_mapped);
    MK_REQUIRE(plan.descriptor_sets_mapped);
    MK_REQUIRE(plan.compute_dispatch_mapped);
    MK_REQUIRE(plan.visible_clear_readbacks_mapped);
    MK_REQUIRE(plan.visible_draw_readbacks_mapped);
    MK_REQUIRE(plan.visible_texture_sampling_readbacks_mapped);
    MK_REQUIRE(plan.visible_depth_readbacks_mapped);
    MK_REQUIRE(plan.diagnostic == "Vulkan IRhiDevice mapping plan ready");

    desc.command_pool_ready = false;
    const auto missing_pool = mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
    MK_REQUIRE(!missing_pool.supported);
    MK_REQUIRE(missing_pool.diagnostic == "Vulkan command pool is required before IRhiDevice mapping");

    desc.command_pool_ready = true;
    desc.vertex_shader = mirakana::rhi::vulkan::VulkanSpirvShaderArtifactValidation{};
    const auto missing_shader = mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
    MK_REQUIRE(!missing_shader.supported);
    MK_REQUIRE(missing_shader.diagnostic ==
               "Vulkan valid SPIR-V vertex and fragment shaders are required before IRhiDevice mapping");

    desc.vertex_shader = vertex_shader;
    desc.compute_shader = mirakana::rhi::vulkan::VulkanSpirvShaderArtifactValidation{};
    const auto missing_compute_shader = mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
    MK_REQUIRE(!missing_compute_shader.supported);
    MK_REQUIRE(missing_compute_shader.diagnostic ==
               "Vulkan valid SPIR-V compute shader is required before IRhiDevice mapping");

    desc.compute_shader = compute_shader;
    desc.descriptor_binding_ready = false;
    const auto missing_descriptors = mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
    MK_REQUIRE(!missing_descriptors.supported);
    MK_REQUIRE(missing_descriptors.diagnostic == "Vulkan descriptor binding is required before IRhiDevice mapping");

    desc.descriptor_binding_ready = true;
    desc.compute_dispatch_ready = false;
    const auto missing_compute_dispatch = mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
    MK_REQUIRE(!missing_compute_dispatch.supported);
    MK_REQUIRE(missing_compute_dispatch.diagnostic ==
               "Vulkan compute pipeline dispatch proof is required before IRhiDevice mapping");

    desc.compute_dispatch_ready = true;
    desc.visible_clear_readback_ready = false;
    const auto missing_visible_clear = mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
    MK_REQUIRE(!missing_visible_clear.supported);
    MK_REQUIRE(missing_visible_clear.diagnostic ==
               "Vulkan visible clear/readback proof is required before IRhiDevice mapping");

    desc.visible_clear_readback_ready = true;
    desc.visible_draw_readback_ready = false;
    const auto missing_visible_draw = mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
    MK_REQUIRE(!missing_visible_draw.supported);
    MK_REQUIRE(missing_visible_draw.diagnostic ==
               "Vulkan visible draw/readback proof is required before IRhiDevice mapping");

    desc.visible_draw_readback_ready = true;
    desc.visible_texture_sampling_readback_ready = false;
    const auto missing_visible_texture_sampling = mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
    MK_REQUIRE(!missing_visible_texture_sampling.supported);
    MK_REQUIRE(missing_visible_texture_sampling.diagnostic ==
               "Vulkan visible texture sampling/readback proof is required before IRhiDevice mapping");

    desc.visible_texture_sampling_readback_ready = true;
    desc.visible_depth_readback_ready = false;
    const auto missing_visible_depth = mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
    MK_REQUIRE(!missing_visible_depth.supported);
    MK_REQUIRE(missing_visible_depth.diagnostic ==
               "Vulkan visible depth/readback proof is required before IRhiDevice mapping");
}

MK_TEST("vulkan runtime buffer create plan maps first party usage and memory domains") {
    const auto device_local =
        mirakana::rhi::vulkan::build_runtime_buffer_create_plan(mirakana::rhi::vulkan::VulkanRuntimeBufferDesc{
            .buffer =
                mirakana::rhi::BufferDesc{
                    .size_bytes = 256,
                    .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::uniform |
                             mirakana::rhi::BufferUsage::copy_destination,
                },
            .memory_domain = mirakana::rhi::vulkan::VulkanBufferMemoryDomain::device_local,
        });

    MK_REQUIRE(device_local.supported);
    MK_REQUIRE(device_local.size_bytes == 256);
    MK_REQUIRE(device_local.usage.vertex);
    MK_REQUIRE(device_local.usage.uniform);
    MK_REQUIRE(device_local.usage.transfer_destination);
    MK_REQUIRE(!device_local.usage.transfer_source);
    MK_REQUIRE(device_local.memory.device_local);
    MK_REQUIRE(!device_local.memory.host_visible);
    MK_REQUIRE(!device_local.memory.host_coherent);
    MK_REQUIRE(device_local.diagnostic == "Vulkan runtime buffer create plan ready");

    const auto upload =
        mirakana::rhi::vulkan::build_runtime_buffer_create_plan(mirakana::rhi::vulkan::VulkanRuntimeBufferDesc{
            .buffer = mirakana::rhi::BufferDesc{.size_bytes = 128, .usage = mirakana::rhi::BufferUsage::copy_source},
            .memory_domain = mirakana::rhi::vulkan::VulkanBufferMemoryDomain::upload,
        });
    MK_REQUIRE(upload.supported);
    MK_REQUIRE(upload.usage.transfer_source);
    MK_REQUIRE(upload.memory.host_visible);
    MK_REQUIRE(upload.memory.host_coherent);
    MK_REQUIRE(!upload.memory.device_local);
}

MK_TEST("vulkan runtime buffer owner rejects invalid descriptions before native commands") {
    mirakana::rhi::vulkan::VulkanRuntimeDevice device;

    const auto missing_size = mirakana::rhi::vulkan::create_runtime_buffer(
        device, mirakana::rhi::vulkan::VulkanRuntimeBufferDesc{
                    .buffer = mirakana::rhi::BufferDesc{.size_bytes = 0, .usage = mirakana::rhi::BufferUsage::vertex},
                    .memory_domain = mirakana::rhi::vulkan::VulkanBufferMemoryDomain::device_local,
                });
    MK_REQUIRE(!missing_size.created);
    MK_REQUIRE(missing_size.diagnostic == "Vulkan runtime buffer size is required");

    const auto missing_device = mirakana::rhi::vulkan::create_runtime_buffer(
        device, mirakana::rhi::vulkan::VulkanRuntimeBufferDesc{
                    .buffer = mirakana::rhi::BufferDesc{.size_bytes = 64,
                                                        .usage = mirakana::rhi::BufferUsage::copy_destination},
                    .memory_domain = mirakana::rhi::vulkan::VulkanBufferMemoryDomain::readback,
                });
    MK_REQUIRE(!missing_device.created);
    MK_REQUIRE(!missing_device.buffer.owns_buffer());
    MK_REQUIRE(!missing_device.buffer.owns_memory());
    MK_REQUIRE(missing_device.plan.supported);
    MK_REQUIRE(missing_device.diagnostic == "Vulkan runtime device is not available");
}

MK_TEST("vulkan runtime texture create plan maps first party usage into image ownership") {
    const auto color_target = mirakana::rhi::vulkan::build_runtime_texture_create_plan(
        mirakana::rhi::vulkan::VulkanRuntimeTextureDesc{mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 320, .height = 180, .depth = 1},
            .format = mirakana::rhi::Format::bgra8_unorm,
            .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource |
                     mirakana::rhi::TextureUsage::copy_source | mirakana::rhi::TextureUsage::copy_destination,
        }});

    MK_REQUIRE(color_target.supported);
    MK_REQUIRE(color_target.extent.width == 320);
    MK_REQUIRE(color_target.extent.height == 180);
    MK_REQUIRE(color_target.extent.depth == 1);
    MK_REQUIRE(color_target.format == mirakana::rhi::Format::bgra8_unorm);
    MK_REQUIRE(color_target.usage.color_attachment);
    MK_REQUIRE(color_target.usage.sampled);
    MK_REQUIRE(color_target.usage.transfer_source);
    MK_REQUIRE(color_target.usage.transfer_destination);
    MK_REQUIRE(!color_target.usage.storage);
    MK_REQUIRE(!color_target.usage.depth_stencil_attachment);
    MK_REQUIRE(color_target.memory.device_local);
    MK_REQUIRE(color_target.diagnostic == "Vulkan runtime texture create plan ready");

    const auto depth = mirakana::rhi::vulkan::build_runtime_texture_create_plan(
        mirakana::rhi::vulkan::VulkanRuntimeTextureDesc{mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = mirakana::rhi::Format::depth24_stencil8,
            .usage = mirakana::rhi::TextureUsage::depth_stencil,
        }});
    MK_REQUIRE(depth.supported);
    MK_REQUIRE(depth.usage.depth_stencil_attachment);
    MK_REQUIRE(!depth.usage.color_attachment);

    const auto depth_sampling = mirakana::rhi::vulkan::build_runtime_texture_create_plan(
        mirakana::rhi::vulkan::VulkanRuntimeTextureDesc{mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = mirakana::rhi::Format::depth24_stencil8,
            .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource,
        }});
    MK_REQUIRE(depth_sampling.supported);
    MK_REQUIRE(depth_sampling.usage.depth_stencil_attachment);
    MK_REQUIRE(depth_sampling.usage.sampled);
    MK_REQUIRE(!depth_sampling.usage.color_attachment);
    MK_REQUIRE(depth_sampling.diagnostic == "Vulkan runtime texture create plan ready");

    const auto depth_copy = mirakana::rhi::vulkan::build_runtime_texture_create_plan(
        mirakana::rhi::vulkan::VulkanRuntimeTextureDesc{mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = mirakana::rhi::Format::depth24_stencil8,
            .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::copy_source,
        }});
    MK_REQUIRE(!depth_copy.supported);
    MK_REQUIRE(depth_copy.diagnostic == "Vulkan runtime depth texture usage is not implemented beyond depth_stencil");

    const auto color_format_depth_usage = mirakana::rhi::vulkan::build_runtime_texture_create_plan(
        mirakana::rhi::vulkan::VulkanRuntimeTextureDesc{mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = mirakana::rhi::Format::rgba8_unorm,
            .usage = mirakana::rhi::TextureUsage::depth_stencil,
        }});
    MK_REQUIRE(!color_format_depth_usage.supported);
    MK_REQUIRE(color_format_depth_usage.diagnostic == "Vulkan runtime depth texture format is unsupported");
}

MK_TEST("vulkan runtime texture owner rejects invalid descriptions before native commands") {
    mirakana::rhi::vulkan::VulkanRuntimeDevice device;

    const auto missing_extent = mirakana::rhi::vulkan::create_runtime_texture(
        device, mirakana::rhi::vulkan::VulkanRuntimeTextureDesc{mirakana::rhi::TextureDesc{
                    .extent = mirakana::rhi::Extent3D{.width = 0, .height = 64, .depth = 1},
                    .format = mirakana::rhi::Format::rgba8_unorm,
                    .usage = mirakana::rhi::TextureUsage::shader_resource,
                }});
    MK_REQUIRE(!missing_extent.created);
    MK_REQUIRE(missing_extent.diagnostic == "Vulkan runtime texture extent is required");

    const auto present_texture = mirakana::rhi::vulkan::build_runtime_texture_create_plan(
        mirakana::rhi::vulkan::VulkanRuntimeTextureDesc{mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = mirakana::rhi::Format::bgra8_unorm,
            .usage = mirakana::rhi::TextureUsage::present,
        }});
    MK_REQUIRE(!present_texture.supported);
    MK_REQUIRE(present_texture.diagnostic == "Vulkan runtime texture present usage requires swapchain ownership");

    const auto missing_device = mirakana::rhi::vulkan::create_runtime_texture(
        device, mirakana::rhi::vulkan::VulkanRuntimeTextureDesc{mirakana::rhi::TextureDesc{
                    .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
                    .format = mirakana::rhi::Format::rgba8_unorm,
                    .usage = mirakana::rhi::TextureUsage::shader_resource,
                }});
    MK_REQUIRE(!missing_device.created);
    MK_REQUIRE(!missing_device.texture.owns_image());
    MK_REQUIRE(!missing_device.texture.owns_memory());
    MK_REQUIRE(missing_device.plan.supported);
    MK_REQUIRE(missing_device.diagnostic == "Vulkan runtime device is not available");
}

MK_TEST("vulkan rhi device factory rejects missing runtime device") {
    mirakana::rhi::vulkan::VulkanRuntimeDevice empty_device;

    auto rhi = mirakana::rhi::vulkan::create_rhi_device(std::move(empty_device), ready_vulkan_rhi_mapping_plan());

    MK_REQUIRE(rhi == nullptr);
}

MK_TEST("vulkan rhi device factory rejects incomplete supported mapping plans when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiIncompleteMappingPlan";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto incomplete_plan = ready_vulkan_rhi_mapping_plan();
    incomplete_plan.visible_depth_readbacks_mapped = false;
    MK_REQUIRE(incomplete_plan.supported);

    auto rhi = mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), incomplete_plan);

    MK_REQUIRE(rhi == nullptr);
#endif
}

MK_TEST("vulkan runtime texture barrier rejects states incompatible with texture usage and format") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRuntimeTextureBarrierCompatibility";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto pool_result = mirakana::rhi::vulkan::create_runtime_command_pool(device_result.device, {});
    MK_REQUIRE(pool_result.created);
    MK_REQUIRE(pool_result.pool.begin_primary_command_buffer());

    auto color_result = mirakana::rhi::vulkan::create_runtime_texture(
        device_result.device, mirakana::rhi::vulkan::VulkanRuntimeTextureDesc{mirakana::rhi::TextureDesc{
                                  .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
                                  .format = mirakana::rhi::Format::rgba8_unorm,
                                  .usage = mirakana::rhi::TextureUsage::render_target,
                              }});
    MK_REQUIRE(color_result.created);
    auto depth_result = mirakana::rhi::vulkan::create_runtime_texture(
        device_result.device, mirakana::rhi::vulkan::VulkanRuntimeTextureDesc{mirakana::rhi::TextureDesc{
                                  .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
                                  .format = mirakana::rhi::Format::depth24_stencil8,
                                  .usage = mirakana::rhi::TextureUsage::depth_stencil,
                              }});
    MK_REQUIRE(depth_result.created);

    const auto color_to_depth = mirakana::rhi::vulkan::record_runtime_texture_barrier(
        device_result.device, pool_result.pool, color_result.texture,
        mirakana::rhi::vulkan::VulkanRuntimeTextureBarrierDesc{
            .before = mirakana::rhi::ResourceState::undefined,
            .after = mirakana::rhi::ResourceState::depth_write,
        });
    const auto depth_to_color = mirakana::rhi::vulkan::record_runtime_texture_barrier(
        device_result.device, pool_result.pool, depth_result.texture,
        mirakana::rhi::vulkan::VulkanRuntimeTextureBarrierDesc{
            .before = mirakana::rhi::ResourceState::undefined,
            .after = mirakana::rhi::ResourceState::render_target,
        });

    MK_REQUIRE(!color_to_depth.recorded);
    MK_REQUIRE(color_to_depth.diagnostic ==
               "Vulkan texture barrier state is incompatible with texture usage or format");
    MK_REQUIRE(!depth_to_color.recorded);
    MK_REQUIRE(depth_to_color.diagnostic ==
               "Vulkan texture barrier state is incompatible with texture usage or format");
    MK_REQUIRE(pool_result.pool.end_primary_command_buffer());
#endif
}

MK_TEST("vulkan rhi device bridge creates backend neutral resources when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiResourceBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);
    MK_REQUIRE(rhi->backend_kind() == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(rhi->backend_name() == "vulkan");

    const auto upload = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 128, .usage = mirakana::rhi::BufferUsage::copy_source});
    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 128, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const auto target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource |
                 mirakana::rhi::TextureUsage::copy_source,
    });

    MK_REQUIRE(upload.value == 1);
    MK_REQUIRE(readback.value == 2);
    MK_REQUIRE(target.value == 1);
    MK_REQUIRE(rhi->stats().buffers_created == 2);
    MK_REQUIRE(rhi->stats().textures_created == 1);
#endif
}

MK_TEST("vulkan rhi device bridge reads backend neutral readback buffers when runtime is available") {
#if defined(_WIN32) || defined(__linux__)
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiReadBufferBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    mirakana::rhi::BufferHandle readback;
    try {
        readback = rhi->create_buffer(
            mirakana::rhi::BufferDesc{.size_bytes = 64, .usage = mirakana::rhi::BufferUsage::copy_destination});
    } catch (const std::invalid_argument& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi buffer") != std::string::npos);
        return;
    }

    const auto bytes = rhi->read_buffer(readback, 8, 16);
    MK_REQUIRE(bytes.size() == 16);
    MK_REQUIRE(rhi->stats().buffer_reads == 1);
    MK_REQUIRE(rhi->stats().bytes_read == 16);

    bool rejected_out_of_range = false;
    try {
        (void)rhi->read_buffer(readback, 56, 16);
    } catch (const std::invalid_argument&) {
        rejected_out_of_range = true;
    }
    MK_REQUIRE(rejected_out_of_range);
#endif
}

MK_TEST("vulkan rhi device bridge creates shader descriptor and pipeline handles when runtime is available") {
#if defined(_WIN32) || defined(__linux__)
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiPipelineBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {});
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    mirakana::rhi::DescriptorSetLayoutDesc descriptor_layout_desc;
    descriptor_layout_desc.bindings.push_back(mirakana::rhi::DescriptorBindingDesc{
        .binding = 0,
        .type = mirakana::rhi::DescriptorType::uniform_buffer,
        .count = 1,
        .stages = mirakana::rhi::ShaderStageVisibility::vertex,
    });

    mirakana::rhi::DescriptorSetLayoutHandle descriptor_layout;
    mirakana::rhi::DescriptorSetHandle descriptor_set;
    mirakana::rhi::PipelineLayoutHandle pipeline_layout;
    try {
        descriptor_layout = rhi->create_descriptor_set_layout(descriptor_layout_desc);
        descriptor_set = rhi->allocate_descriptor_set(descriptor_layout);
        pipeline_layout = rhi->create_pipeline_layout(
            mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {descriptor_layout}, .push_constant_bytes = 0});
    } catch (const std::invalid_argument& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi") != std::string::npos);
        return;
    }

    MK_REQUIRE(descriptor_layout.value == 1);
    MK_REQUIRE(descriptor_set.value == 1);
    MK_REQUIRE(pipeline_layout.value == 1);

    constexpr std::array<std::uint32_t, 5> valid_spirv{
        0x07230203U, 0x00010000U, 0U, 1U, 0U,
    };

    mirakana::rhi::ShaderHandle vertex_shader;
    mirakana::rhi::ShaderHandle fragment_shader;
    try {
        vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .entry_point = "main",
            .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
            .bytecode = valid_spirv.data(),
        });
        fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .entry_point = "main",
            .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
            .bytecode = valid_spirv.data(),
        });
    } catch (const std::invalid_argument& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi shader") != std::string::npos);
        return;
    }

    MK_REQUIRE(vertex_shader.value == 1);
    MK_REQUIRE(fragment_shader.value == 2);

    bool rejected_depth_state_without_format = false;
    try {
        mirakana::rhi::GraphicsPipelineDesc desc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        };
        desc.depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                                .depth_write_enabled = true,
                                                                .depth_compare = mirakana::rhi::CompareOp::less_equal};
        (void)rhi->create_graphics_pipeline(desc);
    } catch (const std::invalid_argument&) {
        rejected_depth_state_without_format = true;
    }

    bool rejected_public_depth_pipeline_with_unimplemented_diagnostic = false;
    try {
        mirakana::rhi::GraphicsPipelineDesc desc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .depth_format = mirakana::rhi::Format::depth24_stencil8,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        };
        desc.depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                                .depth_write_enabled = true,
                                                                .depth_compare = mirakana::rhi::CompareOp::less_equal};
        (void)rhi->create_graphics_pipeline(desc);
    } catch (const std::invalid_argument& error) {
        rejected_public_depth_pipeline_with_unimplemented_diagnostic =
            std::string{error.what()}.find("vulkan rhi graphics pipeline depth state is not implemented") !=
            std::string::npos;
    }

    mirakana::rhi::GraphicsPipelineHandle pipeline;
    try {
        pipeline = rhi->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        });
        MK_REQUIRE(pipeline.value == 1);
    } catch (const std::invalid_argument& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi graphics pipeline") != std::string::npos);
        return;
    }

    const auto mismatched_target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    bool rejected_incompatible_render_target = false;
    try {
        auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = mismatched_target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
        });
        commands->bind_graphics_pipeline(pipeline);
    } catch (const std::invalid_argument&) {
        rejected_incompatible_render_target = true;
    }

    MK_REQUIRE(rejected_depth_state_without_format);
    MK_REQUIRE(!rejected_public_depth_pipeline_with_unimplemented_diagnostic);
    MK_REQUIRE(rejected_incompatible_render_target);

    const auto stats = rhi->stats();
    MK_REQUIRE(stats.shader_modules_created == 2);
    MK_REQUIRE(stats.descriptor_set_layouts_created == 1);
    MK_REQUIRE(stats.descriptor_sets_allocated == 1);
    MK_REQUIRE(stats.pipeline_layouts_created == 1);
    MK_REQUIRE(stats.graphics_pipelines_created == 1);
    MK_REQUIRE(stats.graphics_pipelines_bound == 0);
#endif
}

MK_TEST("vulkan rhi device bridge creates compute pipeline and records dispatch when runtime is available") {
#if defined(_WIN32) || defined(__linux__)
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiComputePipelineBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);
    instance_desc.required_extensions = {};

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto storage = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 64,
        .usage = mirakana::rhi::BufferUsage::storage,
    });

    mirakana::rhi::DescriptorSetLayoutDesc descriptor_layout_desc;
    descriptor_layout_desc.bindings.push_back(mirakana::rhi::DescriptorBindingDesc{
        .binding = 0,
        .type = mirakana::rhi::DescriptorType::storage_buffer,
        .count = 1,
        .stages = mirakana::rhi::ShaderStageVisibility::compute,
    });

    const auto descriptor_layout = rhi->create_descriptor_set_layout(descriptor_layout_desc);
    const auto descriptor_set = rhi->allocate_descriptor_set(descriptor_layout);
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::storage_buffer,
                                                                storage)},
    });
    const auto pipeline_layout = rhi->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {descriptor_layout}, .push_constant_bytes = 0});

    constexpr std::array<std::uint32_t, 5> valid_spirv{
        0x07230203U, 0x00010000U, 0U, 1U, 0U,
    };
    const auto shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "main",
        .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
        .bytecode = valid_spirv.data(),
    });

    mirakana::rhi::ComputePipelineHandle pipeline;
    try {
        pipeline = rhi->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
            .layout = pipeline_layout,
            .compute_shader = shader,
        });
    } catch (const std::invalid_argument& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi compute pipeline") != std::string::npos);
        return;
    }

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::compute);
    commands->bind_compute_pipeline(pipeline);
    commands->bind_descriptor_set(pipeline_layout, 0, descriptor_set);
    commands->dispatch(2, 3, 1);
    commands->close();
    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto stats = rhi->stats();
    MK_REQUIRE(stats.compute_pipelines_created == 1);
    MK_REQUIRE(stats.compute_pipelines_bound == 1);
    MK_REQUIRE(stats.descriptor_sets_bound == 1);
    MK_REQUIRE(stats.compute_dispatches == 1);
    MK_REQUIRE(stats.compute_workgroups_x == 2);
    MK_REQUIRE(stats.compute_workgroups_y == 3);
    MK_REQUIRE(stats.compute_workgroups_z == 1);
#endif
}

MK_TEST("vulkan rhi device bridge proves compute dispatch readback with configured SPIR-V artifact") {
#if defined(_WIN32) || defined(__linux__)
    const auto compute_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_COMPUTE_SPV");
    if (!compute_artifact.configured) {
        return;
    }

    MK_REQUIRE(compute_artifact.diagnostic == "loaded");
    const auto compute_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::compute,
            .bytecode = compute_artifact.words.data(),
            .bytecode_size = compute_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(compute_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiConfiguredComputeDispatch";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto storage = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 16,
        .usage = mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_source,
    });
    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 16, .usage = mirakana::rhi::BufferUsage::copy_destination});

    mirakana::rhi::DescriptorSetLayoutDesc descriptor_layout_desc;
    descriptor_layout_desc.bindings.push_back(mirakana::rhi::DescriptorBindingDesc{
        .binding = 0,
        .type = mirakana::rhi::DescriptorType::storage_buffer,
        .count = 1,
        .stages = mirakana::rhi::ShaderStageVisibility::compute,
    });
    const auto descriptor_layout = rhi->create_descriptor_set_layout(descriptor_layout_desc);
    const auto descriptor_set = rhi->allocate_descriptor_set(descriptor_layout);
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::storage_buffer,
                                                                storage)},
    });
    const auto pipeline_layout = rhi->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {descriptor_layout}, .push_constant_bytes = 0});
    const auto shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "main",
        .bytecode_size = compute_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = compute_artifact.words.data(),
    });
    const auto pipeline = rhi->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = pipeline_layout,
        .compute_shader = shader,
    });

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::compute);
    commands->bind_compute_pipeline(pipeline);
    commands->bind_descriptor_set(pipeline_layout, 0, descriptor_set);
    commands->dispatch(1, 1, 1);
    commands->copy_buffer(
        storage, readback,
        mirakana::rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = 16});
    commands->close();
    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto bytes = rhi->read_buffer(readback, 0, 16);
    MK_REQUIRE(bytes.size() == 16);
    MK_REQUIRE(bytes[0] == 0x41U);
    MK_REQUIRE(bytes[4] == 0x42U);
    MK_REQUIRE(bytes[8] == 0x43U);
    MK_REQUIRE(bytes[12] == 0x44U);
    MK_REQUIRE(rhi->stats().compute_dispatches == 1);
    MK_REQUIRE(rhi->stats().buffer_copies == 1);
#endif
}

MK_TEST("vulkan rhi device bridge proves runtime compute morph position readback with configured SPIR-V artifact") {
#if defined(_WIN32) || defined(__linux__)
    const auto compute_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_COMPUTE_MORPH_SPV");
    if (!compute_artifact.configured) {
        return;
    }

    MK_REQUIRE(compute_artifact.diagnostic == "loaded");
    const auto compute_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::compute,
            .bytecode = compute_artifact.words.data(),
            .bytecode_size = compute_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(compute_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRuntimeComputeMorphPosition";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    append_le_f32(vertex_bytes, 1.0F);
    append_le_f32(vertex_bytes, 2.0F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -2.0F);
    append_le_f32(vertex_bytes, 4.0F);
    append_le_f32(vertex_bytes, 0.5F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -1.0F);
    append_le_f32(vertex_bytes, 2.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/vulkan_compute_morph_base_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{225},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*rhi, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target0;
    mirakana::MorphMeshCpuTargetSourceDocument target1;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_le_f32(target0.position_delta_bytes, 1.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 2.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
    }
    morph.targets.push_back(std::move(target0));
    morph.targets.push_back(std::move(target1));
    append_le_f32(morph.target_weight_bytes, 0.25F);
    append_le_f32(morph.target_weight_bytes, 0.5F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/vulkan_compute_morph_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{226},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*rhi, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());
    MK_REQUIRE(morph_upload.target_count == 2);

    const auto compute_binding =
        mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(*rhi, mesh_upload, morph_upload);
    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.output_position_bytes == 36);

    const auto pipeline_layout = rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "main",
        .bytecode_size = compute_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = compute_artifact.words.data(),
    });
    const auto pipeline = rhi->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = pipeline_layout,
        .compute_shader = shader,
    });
    const auto readback = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = compute_binding.output_position_bytes,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::compute);
    commands->bind_compute_pipeline(pipeline);
    commands->bind_descriptor_set(pipeline_layout, 0, compute_binding.descriptor_set);
    commands->dispatch(compute_binding.vertex_count, 1, 1);
    commands->copy_buffer(compute_binding.output_position_buffer, readback,
                          mirakana::rhi::BufferCopyRegion{.source_offset = 0,
                                                          .destination_offset = 0,
                                                          .size_bytes = compute_binding.output_position_bytes});
    commands->close();
    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto bytes = rhi->read_buffer(readback, 0, compute_binding.output_position_bytes);

    MK_REQUIRE(bytes.size() == 36);
    MK_REQUIRE(read_le_f32(bytes, 0) == 1.25F);
    MK_REQUIRE(read_le_f32(bytes, 4) == 3.0F);
    MK_REQUIRE(read_le_f32(bytes, 8) == 0.0F);
    MK_REQUIRE(read_le_f32(bytes, 12) == -1.75F);
    MK_REQUIRE(read_le_f32(bytes, 16) == 5.0F);
    MK_REQUIRE(read_le_f32(bytes, 20) == 0.5F);
    MK_REQUIRE(read_le_f32(bytes, 24) == 0.25F);
    MK_REQUIRE(read_le_f32(bytes, 28) == 0.0F);
    MK_REQUIRE(read_le_f32(bytes, 32) == 2.0F);
    MK_REQUIRE(rhi->stats().compute_pipelines_created == 1);
    MK_REQUIRE(rhi->stats().compute_dispatches == 1);
    MK_REQUIRE(rhi->stats().compute_workgroups_x == 3);
    MK_REQUIRE(rhi->stats().buffer_reads == 1);
    MK_REQUIRE(rhi->stats().buffer_copies >= 5);
#endif
}

MK_TEST(
    "vulkan rhi device bridge proves runtime compute morph normal tangent readback with configured SPIR-V artifact") {
#if defined(_WIN32) || defined(__linux__)
    const auto compute_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV");
    if (!compute_artifact.configured) {
        return;
    }

    MK_REQUIRE(compute_artifact.diagnostic == "loaded");

    const auto compute_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::compute,
            .bytecode = compute_artifact.words.data(),
            .bytecode_size = compute_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(compute_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRuntimeComputeMorphTangentFrame";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    auto append_lit_vertex = [&vertex_bytes](float x, float y, float z) {
        append_le_f32(vertex_bytes, x);
        append_le_f32(vertex_bytes, y);
        append_le_f32(vertex_bytes, z);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
        append_le_f32(vertex_bytes, 0.5F);
        append_le_f32(vertex_bytes, 0.5F);
        append_le_f32(vertex_bytes, 1.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
    };
    append_lit_vertex(-0.6F, 0.75F, 0.0F);
    append_lit_vertex(0.15F, -0.75F, 0.0F);
    append_lit_vertex(-1.35F, -0.75F, 0.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_mesh_tangent_space_vertex_stride_bytes);

    std::vector<std::uint8_t> bind_positions;
    append_le_f32(bind_positions, -0.6F);
    append_le_f32(bind_positions, 0.75F);
    append_le_f32(bind_positions, 0.0F);
    append_le_f32(bind_positions, 0.15F);
    append_le_f32(bind_positions, -0.75F);
    append_le_f32(bind_positions, 0.0F);
    append_le_f32(bind_positions, -1.35F);
    append_le_f32(bind_positions, -0.75F);
    append_le_f32(bind_positions, 0.0F);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/vulkan_compute_morph_tangent_frame_base_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{229},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = true,
        .has_tangent_frame = true,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*rhi, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = bind_positions;
    for (std::uint32_t vertex = 0; vertex < 3; ++vertex) {
        append_le_f32(morph.bind_normal_bytes, 0.0F);
        append_le_f32(morph.bind_normal_bytes, 0.0F);
        append_le_f32(morph.bind_normal_bytes, 1.0F);
        append_le_f32(morph.bind_tangent_bytes, 1.0F);
        append_le_f32(morph.bind_tangent_bytes, 0.0F);
        append_le_f32(morph.bind_tangent_bytes, 0.0F);
    }

    mirakana::MorphMeshCpuTargetSourceDocument target0;
    mirakana::MorphMeshCpuTargetSourceDocument target1;
    for (std::uint32_t vertex = 0; vertex < 3; ++vertex) {
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.normal_delta_bytes, 0.0F);
        append_le_f32(target0.normal_delta_bytes, 1.0F);
        append_le_f32(target0.normal_delta_bytes, -1.0F);
        append_le_f32(target0.tangent_delta_bytes, -1.0F);
        append_le_f32(target0.tangent_delta_bytes, 0.0F);
        append_le_f32(target0.tangent_delta_bytes, 1.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.normal_delta_bytes, 0.0F);
        append_le_f32(target1.normal_delta_bytes, 0.0F);
        append_le_f32(target1.normal_delta_bytes, 0.0F);
        append_le_f32(target1.tangent_delta_bytes, 0.0F);
        append_le_f32(target1.tangent_delta_bytes, 0.0F);
        append_le_f32(target1.tangent_delta_bytes, 0.0F);
    }
    morph.targets.push_back(std::move(target0));
    morph.targets.push_back(std::move(target1));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    append_le_f32(morph.target_weight_bytes, 0.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/vulkan_compute_morph_tangent_frame_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{230},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*rhi, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());
    MK_REQUIRE(morph_upload.uploaded_normal_delta_bytes == 72);
    MK_REQUIRE(morph_upload.uploaded_tangent_delta_bytes == 72);

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_normal_usage = mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_source;
    compute_options.output_tangent_usage =
        mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_source;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        *rhi, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.output_normal_bytes == 36);
    MK_REQUIRE(compute_binding.output_tangent_bytes == 36);
    MK_REQUIRE(compute_binding.output_normal_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_tangent_buffer.value != 0);

    const auto pipeline_layout = rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "main",
        .bytecode_size = compute_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = compute_artifact.words.data(),
    });
    const auto pipeline = rhi->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = pipeline_layout,
        .compute_shader = shader,
    });
    const auto normal_readback = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = compute_binding.output_normal_bytes,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto tangent_readback = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = compute_binding.output_tangent_bytes,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::compute);
    commands->bind_compute_pipeline(pipeline);
    commands->bind_descriptor_set(pipeline_layout, 0, compute_binding.descriptor_set);
    commands->dispatch(compute_binding.vertex_count, 1, 1);
    commands->copy_buffer(compute_binding.output_normal_buffer, normal_readback,
                          mirakana::rhi::BufferCopyRegion{.source_offset = 0,
                                                          .destination_offset = 0,
                                                          .size_bytes = compute_binding.output_normal_bytes});
    commands->copy_buffer(compute_binding.output_tangent_buffer, tangent_readback,
                          mirakana::rhi::BufferCopyRegion{.source_offset = 0,
                                                          .destination_offset = 0,
                                                          .size_bytes = compute_binding.output_tangent_bytes});
    commands->close();
    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto normal_bytes = rhi->read_buffer(normal_readback, 0, compute_binding.output_normal_bytes);
    const auto tangent_bytes = rhi->read_buffer(tangent_readback, 0, compute_binding.output_tangent_bytes);

    MK_REQUIRE(normal_bytes.size() == 36);
    MK_REQUIRE(tangent_bytes.size() == 36);
    for (std::uint32_t vertex = 0; vertex < 3; ++vertex) {
        const auto offset = static_cast<std::size_t>(vertex) * 12U;
        MK_REQUIRE(nearly_equal(read_le_f32(normal_bytes, offset + 0), 0.0F));
        MK_REQUIRE(nearly_equal(read_le_f32(normal_bytes, offset + 4), 1.0F));
        MK_REQUIRE(nearly_equal(read_le_f32(normal_bytes, offset + 8), 0.0F));
        MK_REQUIRE(nearly_equal(read_le_f32(tangent_bytes, offset + 0), 0.0F));
        MK_REQUIRE(nearly_equal(read_le_f32(tangent_bytes, offset + 4), 0.0F));
        MK_REQUIRE(nearly_equal(read_le_f32(tangent_bytes, offset + 8), 1.0F));
    }
    MK_REQUIRE(rhi->stats().compute_pipelines_created == 1);
    MK_REQUIRE(rhi->stats().compute_dispatches == 1);
    MK_REQUIRE(rhi->stats().compute_workgroups_x == 3);
    MK_REQUIRE(rhi->stats().buffer_reads == 2);
#endif
}

MK_TEST("vulkan rhi frame renderer consumes runtime compute morph output positions when configured") {
#if defined(_WIN32) || defined(__linux__)
    const auto compute_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_COMPUTE_MORPH_SPV");
    const auto vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV");
    const auto fragment_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV");
    if (!compute_artifact.configured && !vertex_artifact.configured && !fragment_artifact.configured) {
        return;
    }

    MK_REQUIRE(compute_artifact.configured);
    MK_REQUIRE(vertex_artifact.configured);
    MK_REQUIRE(fragment_artifact.configured);
    MK_REQUIRE(compute_artifact.diagnostic == "loaded");
    MK_REQUIRE(vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(fragment_artifact.diagnostic == "loaded");

    const auto compute_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::compute,
            .bytecode = compute_artifact.words.data(),
            .bytecode_size = compute_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = vertex_artifact.words.data(),
            .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = fragment_artifact.words.data(),
            .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(compute_validation.valid);
    MK_REQUIRE(vertex_validation.valid);
    MK_REQUIRE(fragment_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRuntimeComputeMorphRenderer";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    append_le_f32(vertex_bytes, -0.6F);
    append_le_f32(vertex_bytes, 0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, 0.15F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -1.35F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/vulkan_compute_morph_renderer_base_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{227},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*rhi, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target0;
    mirakana::MorphMeshCpuTargetSourceDocument target1;
    for (std::uint32_t vertex = 0; vertex < 3; ++vertex) {
        append_le_f32(target0.position_delta_bytes, 0.6F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
    }
    morph.targets.push_back(std::move(target0));
    morph.targets.push_back(std::move(target1));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    append_le_f32(morph.target_weight_bytes, 0.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/vulkan_compute_morph_renderer_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{228},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*rhi, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        *rhi, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());

    const auto compute_layout = rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto compute_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "main",
        .bytecode_size = compute_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = compute_artifact.words.data(),
    });
    const auto compute_pipeline = rhi->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = compute_layout,
        .compute_shader = compute_shader,
    });

    auto compute_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->bind_compute_pipeline(compute_pipeline);
    compute_commands->bind_descriptor_set(compute_layout, 0, compute_binding.descriptor_set);
    compute_commands->dispatch(compute_binding.vertex_count, 1, 1);
    compute_commands->close();
    const auto compute_fence = rhi->submit(*compute_commands);
    rhi->wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence);

    const auto mesh_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding);
    MK_REQUIRE(mesh_binding.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(mesh_binding.index_buffer.value == mesh_upload.index_buffer.value);

    const auto target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto render_layout =
        rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = vertex_artifact.words.data(),
    });
    const auto fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = fragment_artifact.words.data(),
    });
    const auto vertex_layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(mesh_payload);
    MK_REQUIRE(vertex_layout.succeeded());
    const auto render_pipeline = rhi->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = render_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = vertex_layout.vertex_buffers,
        .vertex_attributes = vertex_layout.vertex_attributes,
    });

    auto prepare_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = rhi->submit(*prepare_commands);
    rhi->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = rhi.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = render_pipeline,
        .wait_for_completion = true,
    });
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = mesh_payload.asset,
        .material = mirakana::AssetId{},
        .world_from_node = mirakana::Mat4::identity(),
        .mesh_binding = mesh_binding,
    });
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };
    auto readback_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = rhi->submit(*readback_commands);
    rhi->wait(readback_fence);

    const auto bytes = rhi->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 50 && bytes.at(center_pixel + 0U) <= 54);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 151 && bytes.at(center_pixel + 1U) <= 155);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 228 && bytes.at(center_pixel + 2U) <= 232);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);

    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(rhi->stats().compute_dispatches == 1);
    MK_REQUIRE(rhi->stats().queue_waits == 1);
    MK_REQUIRE(rhi->stats().queue_wait_failures == 0);
    MK_REQUIRE(rhi->stats().vertex_buffer_bindings == 1);
    MK_REQUIRE(rhi->stats().index_buffer_bindings == 1);
    MK_REQUIRE(rhi->stats().indexed_draw_calls == 1);
    MK_REQUIRE(rhi->stats().texture_buffer_copies == 1);
    MK_REQUIRE(rhi->stats().buffer_reads == 1);
#endif
}

MK_TEST("vulkan rhi device bridge updates buffer descriptor sets when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiDescriptorUpdateBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto uniform =
        rhi->create_buffer(mirakana::rhi::BufferDesc{.size_bytes = 256, .usage = mirakana::rhi::BufferUsage::uniform});

    mirakana::rhi::DescriptorSetLayoutDesc layout_desc;
    layout_desc.bindings.push_back(mirakana::rhi::DescriptorBindingDesc{
        .binding = 0,
        .type = mirakana::rhi::DescriptorType::uniform_buffer,
        .count = 1,
        .stages = mirakana::rhi::ShaderStageVisibility::vertex,
    });
    const auto layout = rhi->create_descriptor_set_layout(layout_desc);
    const auto set = rhi->allocate_descriptor_set(layout);

    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                uniform)},
    });

    MK_REQUIRE(rhi->stats().descriptor_writes == 1);
#endif
}

MK_TEST("vulkan rhi device bridge updates sampled texture and sampler descriptor sets when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiTextureSamplerDescriptorUpdateBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto texture = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto sampled_depth = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto sampler = rhi->create_sampler(mirakana::rhi::SamplerDesc{
        .min_filter = mirakana::rhi::SamplerFilter::nearest,
        .mag_filter = mirakana::rhi::SamplerFilter::nearest,
        .address_u = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
    });

    mirakana::rhi::DescriptorSetLayoutDesc layout_desc;
    layout_desc.bindings.push_back(mirakana::rhi::DescriptorBindingDesc{
        .binding = 0,
        .type = mirakana::rhi::DescriptorType::sampled_texture,
        .count = 1,
        .stages = mirakana::rhi::ShaderStageVisibility::fragment,
    });
    layout_desc.bindings.push_back(mirakana::rhi::DescriptorBindingDesc{
        .binding = 1,
        .type = mirakana::rhi::DescriptorType::sampler,
        .count = 1,
        .stages = mirakana::rhi::ShaderStageVisibility::fragment,
    });
    const auto layout = rhi->create_descriptor_set_layout(layout_desc);
    const auto set = rhi->allocate_descriptor_set(layout);

    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 texture)},
    });
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 sampled_depth)},
    });
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = set,
        .binding = 1,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(sampler)},
    });

    MK_REQUIRE(rhi->stats().descriptor_writes == 3);
    MK_REQUIRE(rhi->stats().samplers_created == 1);
#endif
}

MK_TEST("vulkan rhi device bridge records standalone texture copy commands when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiStandaloneTextureCopyBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto upload = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 1024, .usage = mirakana::rhi::BufferUsage::copy_source});
    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 1024, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const auto texture = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination | mirakana::rhi::TextureUsage::copy_source,
    });
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 4,
        .buffer_image_height = 4,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
    };

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::copy_destination);
    commands->copy_buffer(
        upload, readback,
        mirakana::rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = 16});
    commands->copy_buffer_to_texture(upload, texture, footprint);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::copy_destination,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(texture, readback, footprint);
    commands->close();

    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto stats = rhi->stats();
    MK_REQUIRE(stats.resource_transitions == 2);
    MK_REQUIRE(stats.buffer_copies == 1);
    MK_REQUIRE(stats.buffer_texture_copies == 1);
    MK_REQUIRE(stats.texture_buffer_copies == 1);
    MK_REQUIRE(stats.command_lists_submitted == 1);
#endif
}

MK_TEST("vulkan rhi device bridge records texture aliasing barriers when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiTextureAliasingBarrierBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto first = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto second = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->texture_aliasing_barrier(first, second);
    commands->close();

    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto stats = rhi->stats();
    MK_REQUIRE(stats.texture_aliasing_barriers == 1);
    MK_REQUIRE(stats.resource_transitions == 0);
#endif
}

MK_TEST("vulkan rhi command list abandons texture transitions without committing state") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiAbandonedTextureState";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto texture = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination | mirakana::rhi::TextureUsage::copy_source,
    });

    {
        auto abandoned = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        abandoned->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                      mirakana::rhi::ResourceState::copy_destination);
        abandoned->transition_texture(texture, mirakana::rhi::ResourceState::copy_destination,
                                      mirakana::rhi::ResourceState::copy_source);
        abandoned->close();
    }

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::copy_destination);
    commands->close();

    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    MK_REQUIRE(rhi->stats().command_lists_submitted == 1);
#endif
}

MK_TEST("vulkan rhi command list rejects stale texture state submissions") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiStaleTextureSubmit";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto texture = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination,
    });

    auto first = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    first->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                              mirakana::rhi::ResourceState::copy_destination);
    first->close();

    auto stale = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    stale->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                              mirakana::rhi::ResourceState::copy_destination);
    stale->close();

    const auto fence = rhi->submit(*first);
    rhi->wait(fence);

    bool rejected_stale_submission = false;
    try {
        (void)rhi->submit(*stale);
    } catch (const std::logic_error&) {
        rejected_stale_submission = true;
    }

    MK_REQUIRE(rejected_stale_submission);
    MK_REQUIRE(rhi->stats().command_lists_submitted == 1);
#endif
}

MK_TEST("vulkan rhi command list rejects texture transitions incompatible with usage and format") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiTextureTransitionCompatibility";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto color_target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto depth = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto upload_only_texture = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination,
    });

    bool rejected_color_to_depth = false;
    try {
        auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->transition_texture(color_target, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::depth_write);
    } catch (const std::invalid_argument&) {
        rejected_color_to_depth = true;
    }

    bool rejected_depth_to_color = false;
    try {
        auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->transition_texture(depth, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::render_target);
    } catch (const std::invalid_argument&) {
        rejected_depth_to_color = true;
    }

    bool rejected_copy_without_usage = false;
    try {
        auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->transition_texture(color_target, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::render_target);
        commands->transition_texture(color_target, mirakana::rhi::ResourceState::render_target,
                                     mirakana::rhi::ResourceState::copy_source);
    } catch (const std::invalid_argument&) {
        rejected_copy_without_usage = true;
    }

    bool rejected_shader_read_without_usage = false;
    try {
        auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->transition_texture(upload_only_texture, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::copy_destination);
        commands->transition_texture(upload_only_texture, mirakana::rhi::ResourceState::copy_destination,
                                     mirakana::rhi::ResourceState::shader_read);
    } catch (const std::invalid_argument&) {
        rejected_shader_read_without_usage = true;
    }

    MK_REQUIRE(rejected_color_to_depth);
    MK_REQUIRE(rejected_depth_to_color);
    MK_REQUIRE(rejected_copy_without_usage);
    MK_REQUIRE(rejected_shader_read_without_usage);
    MK_REQUIRE(rhi->stats().command_lists_submitted == 0);
#endif
}

MK_TEST("vulkan rhi render pass rejects invalid depth attachments when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiInvalidDepthAttachments";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto color_as_depth = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto depth = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto small_depth = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 2, .height = 2, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });

    const auto color_attachment = mirakana::rhi::RenderPassColorAttachment{
        .texture = target,
        .load_action = mirakana::rhi::LoadAction::clear,
        .store_action = mirakana::rhi::StoreAction::store,
        .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
        .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
    };

    bool rejected_wrong_usage = false;
    try {
        auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::render_target);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color = color_attachment,
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = color_as_depth},
        });
    } catch (const std::invalid_argument&) {
        rejected_wrong_usage = true;
    }

    bool rejected_wrong_state = false;
    try {
        auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::render_target);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color = color_attachment,
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = depth},
        });
    } catch (const std::invalid_argument&) {
        rejected_wrong_state = true;
    }

    bool rejected_invalid_clear_depth = false;
    try {
        auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::render_target);
        commands->transition_texture(depth, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::depth_write);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color = color_attachment,
            .depth =
                mirakana::rhi::RenderPassDepthAttachment{
                    .texture = depth,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                    .clear_depth = mirakana::rhi::ClearDepthValue{1.5F},
                },
        });
    } catch (const std::invalid_argument&) {
        rejected_invalid_clear_depth = true;
    }

    bool rejected_extent_mismatch = false;
    try {
        auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::render_target);
        commands->transition_texture(small_depth, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::depth_write);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color = color_attachment,
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = small_depth},
        });
    } catch (const std::invalid_argument&) {
        rejected_extent_mismatch = true;
    }

    MK_REQUIRE(rejected_wrong_usage);
    MK_REQUIRE(rejected_wrong_state);
    MK_REQUIRE(rejected_invalid_clear_depth);
    MK_REQUIRE(rejected_extent_mismatch);
    MK_REQUIRE(rhi->stats().command_lists_submitted == 0);
#endif
}

MK_TEST("vulkan rhi device bridge clears standalone texture render targets when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiStandaloneTextureRenderBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto depth = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 1024, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 4,
        .buffer_image_height = 4,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
    };

    bool rejected_depth_attachment = false;
    try {
        auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = mirakana::rhi::TextureHandle{999}},
        });
    } catch (const std::invalid_argument&) {
        rejected_depth_attachment = true;
    }

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);
    commands->transition_texture(depth, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::depth_write);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color =
                    mirakana::rhi::ClearColorValue{.red = 0.25F, .green = 0.5F, .blue = 0.75F, .alpha = 1.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    commands->end_render_pass();
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(target, readback, footprint);
    commands->close();

    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto bytes = rhi->read_buffer(readback, 0, 64);
    const auto non_zero = std::ranges::any_of(bytes, [](std::uint8_t value) { return value != 0; });
    MK_REQUIRE(non_zero);
    MK_REQUIRE(rejected_depth_attachment);
    MK_REQUIRE(rhi->stats().render_passes_begun == 1);
    MK_REQUIRE(rhi->stats().texture_buffer_copies == 1);
#endif
}

MK_TEST("vulkan rhi device bridge draws standalone texture render targets when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiStandaloneTextureDrawBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    constexpr std::array<std::uint32_t, 5> valid_spirv{
        0x07230203U, 0x00010000U, 0U, 1U, 0U,
    };

    mirakana::rhi::GraphicsPipelineHandle pipeline;
    try {
        const auto vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .entry_point = "main",
            .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
            .bytecode = valid_spirv.data(),
        });
        const auto fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .entry_point = "main",
            .bytecode_size = valid_spirv.size() * sizeof(std::uint32_t),
            .bytecode = valid_spirv.data(),
        });
        const auto pipeline_layout = rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{});
        pipeline = rhi->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::rgba8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        });
    } catch (const std::invalid_argument& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi") != std::string::npos);
        return;
    }

    const auto target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 1024, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 4,
        .buffer_image_height = 4,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
    };

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.1F, .green = 0.2F, .blue = 0.3F, .alpha = 1.0F},
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(target, readback, footprint);
    commands->close();

    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto stats = rhi->stats();
    MK_REQUIRE(stats.render_passes_begun == 1);
    MK_REQUIRE(stats.graphics_pipelines_bound == 1);
    MK_REQUIRE(stats.draw_calls == 1);
    MK_REQUIRE(stats.vertices_submitted == 3);
    MK_REQUIRE(stats.texture_buffer_copies == 1);
#endif
}

MK_TEST("vulkan rhi device bridge proves visible draw readback with configured SPIR-V artifacts") {
#if defined(_WIN32) || defined(__linux__)
    const auto vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_VERTEX_SPV");
    const auto fragment_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_FRAGMENT_SPV");
    if (!vertex_artifact.configured && !fragment_artifact.configured) {
        return;
    }

    MK_REQUIRE(vertex_artifact.configured);
    MK_REQUIRE(fragment_artifact.configured);
    MK_REQUIRE(vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(fragment_artifact.diagnostic == "loaded");

    const auto vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = vertex_artifact.words.data(),
            .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = fragment_artifact.words.data(),
            .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(vertex_validation.valid);
    MK_REQUIRE(fragment_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiConfiguredVisibleDraw";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = vertex_artifact.words.data(),
    });
    const auto fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = fragment_artifact.words.data(),
    });
    const auto pipeline_layout = rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{});
    const auto pipeline = rhi->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    const auto target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 4096, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 8,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 0.0F},
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(target, readback, footprint);
    commands->close();

    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto bytes = rhi->read_buffer(readback, 0, 8 * 8 * 4);
    MK_REQUIRE(has_non_zero_byte(bytes));
    MK_REQUIRE(rhi->stats().draw_calls == 1);
    MK_REQUIRE(rhi->stats().texture_buffer_copies == 1);
#endif
}

MK_TEST("vulkan rhi device bridge proves depth ordered draw readback with configured SPIR-V artifacts") {
#if defined(_WIN32) || defined(__linux__)
    const auto vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_DEPTH_VERTEX_SPV");
    const auto fragment_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV");
    if (!vertex_artifact.configured && !fragment_artifact.configured) {
        return;
    }

    MK_REQUIRE(vertex_artifact.configured);
    MK_REQUIRE(fragment_artifact.configured);
    MK_REQUIRE(vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(fragment_artifact.diagnostic == "loaded");

    const auto vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = vertex_artifact.words.data(),
            .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = fragment_artifact.words.data(),
            .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(vertex_validation.valid);
    MK_REQUIRE(fragment_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiConfiguredDepthDraw";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto upload = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 512, .usage = mirakana::rhi::BufferUsage::copy_source});
    const auto vertices = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto indices = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 64, .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_destination});

    constexpr std::array<float, 42> vertex_data{
        -0.8F, -0.8F, 0.25F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F,  0.8F,  0.25F, 0.0F, 1.0F, 0.0F, 1.0F,
        0.8F,  -0.8F, 0.25F, 0.0F, 1.0F, 0.0F, 1.0F, -0.8F, -0.8F, 0.75F, 1.0F, 0.0F, 0.0F, 1.0F,
        0.0F,  0.8F,  0.75F, 1.0F, 0.0F, 0.0F, 1.0F, 0.8F,  -0.8F, 0.75F, 1.0F, 0.0F, 0.0F, 1.0F,
    };
    constexpr std::array<std::uint16_t, 6> index_data{0, 1, 2, 3, 4, 5};
    std::array<std::uint8_t, 512> upload_bytes{};
    const auto vertex_bytes = std::as_bytes(std::span{vertex_data});
    const auto index_bytes = std::as_bytes(std::span{index_data});
    auto upload_bytes_view = std::span{upload_bytes};
    std::memcpy(upload_bytes_view.data(), vertex_bytes.data(), vertex_bytes.size());
    std::memcpy(upload_bytes_view.subspan(vertex_bytes.size()).data(), index_bytes.data(), index_bytes.size());
    rhi->write_buffer(upload, 0, upload_bytes);

    const auto vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = vertex_artifact.words.data(),
    });
    const auto fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = fragment_artifact.words.data(),
    });
    const auto pipeline_layout = rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{});
    mirakana::rhi::GraphicsPipelineDesc pipeline_desc{
        .layout = pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    pipeline_desc.vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
        .binding = 0, .stride = 28, .input_rate = mirakana::rhi::VertexInputRate::vertex}};
    pipeline_desc.vertex_attributes = {
        mirakana::rhi::VertexAttributeDesc{.location = 0,
                                           .binding = 0,
                                           .offset = 0,
                                           .format = mirakana::rhi::VertexFormat::float32x3,
                                           .semantic = mirakana::rhi::VertexSemantic::position,
                                           .semantic_index = 0},
        mirakana::rhi::VertexAttributeDesc{.location = 1,
                                           .binding = 0,
                                           .offset = 12,
                                           .format = mirakana::rhi::VertexFormat::float32x4,
                                           .semantic = mirakana::rhi::VertexSemantic::color,
                                           .semantic_index = 0},
    };
    pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto pipeline = rhi->create_graphics_pipeline(pipeline_desc);
    auto color_only_pipeline_desc = pipeline_desc;
    color_only_pipeline_desc.depth_format = mirakana::rhi::Format::unknown;
    color_only_pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{};
    const auto color_only_pipeline = rhi->create_graphics_pipeline(color_only_pipeline_desc);

    const auto target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto depth = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 4096, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 8,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };

    {
        auto mismatch = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        mismatch->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::render_target);
        mismatch->transition_texture(depth, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::depth_write);
        mismatch->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                    .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                    .clear_color =
                        mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
                },
            .depth =
                mirakana::rhi::RenderPassDepthAttachment{
                    .texture = depth,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                    .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
                },
        });
        bool rejected_no_depth_pipeline_in_depth_pass = false;
        try {
            mismatch->bind_graphics_pipeline(color_only_pipeline);
        } catch (const std::invalid_argument&) {
            rejected_no_depth_pipeline_in_depth_pass = true;
        }
        mismatch->end_render_pass();
        mismatch->close();
        MK_REQUIRE(rejected_no_depth_pipeline_in_depth_pass);
    }

    {
        auto mismatch = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
        mismatch->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::render_target);
        mismatch->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                    .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                    .clear_color =
                        mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
                },
        });
        bool rejected_depth_pipeline_in_color_only_pass = false;
        try {
            mismatch->bind_graphics_pipeline(pipeline);
        } catch (const std::invalid_argument&) {
            rejected_depth_pipeline_in_color_only_pass = true;
        }
        mismatch->end_render_pass();
        mismatch->close();
        MK_REQUIRE(rejected_depth_pipeline_in_color_only_pass);
    }

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->copy_buffer(upload, vertices,
                          mirakana::rhi::BufferCopyRegion{
                              .source_offset = 0, .destination_offset = 0, .size_bytes = sizeof(vertex_data)});
    commands->copy_buffer(upload, indices,
                          mirakana::rhi::BufferCopyRegion{.source_offset = sizeof(vertex_data),
                                                          .destination_offset = 0,
                                                          .size_bytes = sizeof(index_data)});
    commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);
    commands->transition_texture(depth, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::depth_write);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 0.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->bind_vertex_buffer(
        mirakana::rhi::VertexBufferBinding{.buffer = vertices, .offset = 0, .stride = 28, .binding = 0});
    commands->bind_index_buffer(mirakana::rhi::IndexBufferBinding{
        .buffer = indices, .offset = 0, .format = mirakana::rhi::IndexFormat::uint16});
    commands->draw_indexed(6, 1);
    commands->end_render_pass();
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(target, readback, footprint);
    commands->close();

    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto bytes = rhi->read_buffer(readback, 0, 8 * 8 * 4);
    const auto center_pixel = (4U * 8U * 4U) + (4U * 4U);
    MK_REQUIRE(bytes.at(center_pixel + 0U) <= 80);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 120);
    MK_REQUIRE(bytes.at(center_pixel + 2U) <= 80);
    MK_REQUIRE(rhi->stats().draw_calls == 1);
    MK_REQUIRE(rhi->stats().indexed_draw_calls == 1);
    MK_REQUIRE(rhi->stats().texture_buffer_copies == 1);
#endif
}

MK_TEST("vulkan rhi device bridge visibly samples depth textures after depth write when configured") {
#if defined(_WIN32) || defined(__linux__)
    const auto depth_vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_DEPTH_VERTEX_SPV");
    const auto depth_fragment_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV");
    const auto sample_vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_DEPTH_SAMPLE_VERTEX_SPV");
    const auto sample_fragment_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_DEPTH_SAMPLE_FRAGMENT_SPV");
    if (!depth_vertex_artifact.configured && !depth_fragment_artifact.configured &&
        !sample_vertex_artifact.configured && !sample_fragment_artifact.configured) {
        return;
    }

    MK_REQUIRE(depth_vertex_artifact.configured);
    MK_REQUIRE(depth_fragment_artifact.configured);
    MK_REQUIRE(sample_vertex_artifact.configured);
    MK_REQUIRE(sample_fragment_artifact.configured);
    MK_REQUIRE(depth_vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(depth_fragment_artifact.diagnostic == "loaded");
    MK_REQUIRE(sample_vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(sample_fragment_artifact.diagnostic == "loaded");

    const auto depth_vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = depth_vertex_artifact.words.data(),
            .bytecode_size = depth_vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto depth_fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = depth_fragment_artifact.words.data(),
            .bytecode_size = depth_fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto sample_vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = sample_vertex_artifact.words.data(),
            .bytecode_size = sample_vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto sample_fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = sample_fragment_artifact.words.data(),
            .bytecode_size = sample_fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(depth_vertex_validation.valid);
    MK_REQUIRE(depth_fragment_validation.valid);
    MK_REQUIRE(sample_vertex_validation.valid);
    MK_REQUIRE(sample_fragment_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiConfiguredDepthSampling";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto upload = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 512, .usage = mirakana::rhi::BufferUsage::copy_source});
    const auto vertices = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto indices = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 64, .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_destination});

    constexpr std::array<float, 42> vertex_data{
        -0.8F, -0.8F, 0.25F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F,  0.8F,  0.25F, 0.0F, 1.0F, 0.0F, 1.0F,
        0.8F,  -0.8F, 0.25F, 0.0F, 1.0F, 0.0F, 1.0F, -0.8F, -0.8F, 0.75F, 1.0F, 0.0F, 0.0F, 1.0F,
        0.0F,  0.8F,  0.75F, 1.0F, 0.0F, 0.0F, 1.0F, 0.8F,  -0.8F, 0.75F, 1.0F, 0.0F, 0.0F, 1.0F,
    };
    constexpr std::array<std::uint16_t, 6> index_data{0, 1, 2, 3, 4, 5};
    std::array<std::uint8_t, 512> upload_bytes{};
    const auto vertex_bytes = std::as_bytes(std::span{vertex_data});
    const auto index_bytes = std::as_bytes(std::span{index_data});
    auto upload_bytes_view = std::span{upload_bytes};
    std::memcpy(upload_bytes_view.data(), vertex_bytes.data(), vertex_bytes.size());
    std::memcpy(upload_bytes_view.subspan(vertex_bytes.size()).data(), index_bytes.data(), index_bytes.size());
    rhi->write_buffer(upload, 0, upload_bytes);

    const auto depth_vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = depth_vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = depth_vertex_artifact.words.data(),
    });
    const auto depth_fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = depth_fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = depth_fragment_artifact.words.data(),
    });
    const auto sample_vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = sample_vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = sample_vertex_artifact.words.data(),
    });
    const auto sample_fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = sample_fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = sample_fragment_artifact.words.data(),
    });

    const auto depth_pipeline_layout = rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{});
    mirakana::rhi::GraphicsPipelineDesc depth_pipeline_desc{
        .layout = depth_pipeline_layout,
        .vertex_shader = depth_vertex_shader,
        .fragment_shader = depth_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    depth_pipeline_desc.vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
        .binding = 0, .stride = 28, .input_rate = mirakana::rhi::VertexInputRate::vertex}};
    depth_pipeline_desc.vertex_attributes = {
        mirakana::rhi::VertexAttributeDesc{.location = 0,
                                           .binding = 0,
                                           .offset = 0,
                                           .format = mirakana::rhi::VertexFormat::float32x3,
                                           .semantic = mirakana::rhi::VertexSemantic::position,
                                           .semantic_index = 0},
        mirakana::rhi::VertexAttributeDesc{.location = 1,
                                           .binding = 0,
                                           .offset = 12,
                                           .format = mirakana::rhi::VertexFormat::float32x4,
                                           .semantic = mirakana::rhi::VertexSemantic::color,
                                           .semantic_index = 0},
    };
    depth_pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto depth_pipeline = rhi->create_graphics_pipeline(depth_pipeline_desc);

    const auto set_layout = rhi->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 1,
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto sample_pipeline_layout = rhi->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {set_layout}, .push_constant_bytes = 0});
    const auto sample_pipeline = rhi->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = sample_pipeline_layout,
        .vertex_shader = sample_vertex_shader,
        .fragment_shader = sample_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    const auto depth_color = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto sampled_depth = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto sample_target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto sampler = rhi->create_sampler(mirakana::rhi::SamplerDesc{
        .min_filter = mirakana::rhi::SamplerFilter::nearest,
        .mag_filter = mirakana::rhi::SamplerFilter::nearest,
        .address_u = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
    });
    const auto descriptor_set = rhi->allocate_descriptor_set(set_layout);
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 sampled_depth)},
    });
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 1,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(sampler)},
    });

    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 4096, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 8,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };

    auto depth_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    depth_commands->copy_buffer(upload, vertices,
                                mirakana::rhi::BufferCopyRegion{
                                    .source_offset = 0, .destination_offset = 0, .size_bytes = sizeof(vertex_data)});
    depth_commands->copy_buffer(upload, indices,
                                mirakana::rhi::BufferCopyRegion{.source_offset = sizeof(vertex_data),
                                                                .destination_offset = 0,
                                                                .size_bytes = sizeof(index_data)});
    depth_commands->transition_texture(depth_color, mirakana::rhi::ResourceState::undefined,
                                       mirakana::rhi::ResourceState::render_target);
    depth_commands->transition_texture(sampled_depth, mirakana::rhi::ResourceState::undefined,
                                       mirakana::rhi::ResourceState::depth_write);
    depth_commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = depth_color,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 0.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = sampled_depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    depth_commands->bind_graphics_pipeline(depth_pipeline);
    depth_commands->bind_vertex_buffer(
        mirakana::rhi::VertexBufferBinding{.buffer = vertices, .offset = 0, .stride = 28, .binding = 0});
    depth_commands->bind_index_buffer(mirakana::rhi::IndexBufferBinding{
        .buffer = indices, .offset = 0, .format = mirakana::rhi::IndexFormat::uint16});
    depth_commands->draw_indexed(6, 1);
    depth_commands->end_render_pass();
    depth_commands->transition_texture(sampled_depth, mirakana::rhi::ResourceState::depth_write,
                                       mirakana::rhi::ResourceState::shader_read);
    depth_commands->close();

    const auto depth_fence = rhi->submit(*depth_commands);
    rhi->wait(depth_fence);

    auto sample_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    sample_commands->transition_texture(sample_target, mirakana::rhi::ResourceState::undefined,
                                        mirakana::rhi::ResourceState::render_target);
    sample_commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = sample_target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 1.0F, .green = 0.0F, .blue = 1.0F, .alpha = 1.0F},
            },
    });
    sample_commands->bind_graphics_pipeline(sample_pipeline);
    sample_commands->bind_descriptor_set(sample_pipeline_layout, 0, descriptor_set);
    sample_commands->draw(3, 1);
    sample_commands->end_render_pass();
    sample_commands->transition_texture(sample_target, mirakana::rhi::ResourceState::render_target,
                                        mirakana::rhi::ResourceState::copy_source);
    sample_commands->copy_texture_to_buffer(sample_target, readback, footprint);
    sample_commands->close();

    const auto sample_fence = rhi->submit(*sample_commands);
    rhi->wait(sample_fence);

    const auto bytes = rhi->read_buffer(readback, 0, 8 * 8 * 4);
    const auto center_pixel = (4U * 8U * 4U) + (4U * 4U);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 55 && bytes.at(center_pixel + 0U) <= 75);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 55 && bytes.at(center_pixel + 1U) <= 75);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 55 && bytes.at(center_pixel + 2U) <= 75);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    MK_REQUIRE(rhi->stats().samplers_created == 1);
    MK_REQUIRE(rhi->stats().descriptor_writes == 2);
    MK_REQUIRE(rhi->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(rhi->stats().draw_calls == 2);
    MK_REQUIRE(rhi->stats().indexed_draw_calls == 1);
    MK_REQUIRE(rhi->stats().texture_buffer_copies == 1);
#endif
}

MK_TEST("vulkan rhi device bridge samples scene depth in a postprocess pass when configured") {
#if defined(_WIN32) || defined(__linux__)
    const auto depth_vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_DEPTH_VERTEX_SPV");
    const auto depth_fragment_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV");
    const auto postprocess_vertex_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_POSTPROCESS_DEPTH_VERTEX_SPV");
    const auto postprocess_fragment_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_POSTPROCESS_DEPTH_FRAGMENT_SPV");
    if (!depth_vertex_artifact.configured && !depth_fragment_artifact.configured &&
        !postprocess_vertex_artifact.configured && !postprocess_fragment_artifact.configured) {
        return;
    }

    MK_REQUIRE(depth_vertex_artifact.configured);
    MK_REQUIRE(depth_fragment_artifact.configured);
    MK_REQUIRE(postprocess_vertex_artifact.configured);
    MK_REQUIRE(postprocess_fragment_artifact.configured);
    MK_REQUIRE(depth_vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(depth_fragment_artifact.diagnostic == "loaded");
    MK_REQUIRE(postprocess_vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(postprocess_fragment_artifact.diagnostic == "loaded");

    const auto depth_vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = depth_vertex_artifact.words.data(),
            .bytecode_size = depth_vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto depth_fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = depth_fragment_artifact.words.data(),
            .bytecode_size = depth_fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto postprocess_vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = postprocess_vertex_artifact.words.data(),
            .bytecode_size = postprocess_vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto postprocess_fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = postprocess_fragment_artifact.words.data(),
            .bytecode_size = postprocess_fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(depth_vertex_validation.valid);
    MK_REQUIRE(depth_fragment_validation.valid);
    MK_REQUIRE(postprocess_vertex_validation.valid);
    MK_REQUIRE(postprocess_fragment_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiConfiguredPostprocessDepth";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto upload = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 512, .usage = mirakana::rhi::BufferUsage::copy_source});
    const auto vertices = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto indices = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 64, .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_destination});

    constexpr std::array<float, 42> vertex_data{
        -0.8F, -0.8F, 0.25F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F,  0.8F,  0.25F, 0.0F, 1.0F, 0.0F, 1.0F,
        0.8F,  -0.8F, 0.25F, 0.0F, 1.0F, 0.0F, 1.0F, -0.8F, -0.8F, 0.75F, 1.0F, 0.0F, 0.0F, 1.0F,
        0.0F,  0.8F,  0.75F, 1.0F, 0.0F, 0.0F, 1.0F, 0.8F,  -0.8F, 0.75F, 1.0F, 0.0F, 0.0F, 1.0F,
    };
    constexpr std::array<std::uint16_t, 6> index_data{0, 1, 2};
    std::array<std::uint8_t, 512> upload_bytes{};
    const auto vertex_bytes = std::as_bytes(std::span{vertex_data});
    const auto index_bytes = std::as_bytes(std::span{index_data});
    auto upload_bytes_view = std::span{upload_bytes};
    std::memcpy(upload_bytes_view.data(), vertex_bytes.data(), vertex_bytes.size());
    std::memcpy(upload_bytes_view.subspan(vertex_bytes.size()).data(), index_bytes.data(), index_bytes.size());
    rhi->write_buffer(upload, 0, upload_bytes);

    const auto scene_vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = depth_vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = depth_vertex_artifact.words.data(),
    });
    const auto scene_fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = depth_fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = depth_fragment_artifact.words.data(),
    });
    const auto postprocess_vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = postprocess_vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = postprocess_vertex_artifact.words.data(),
    });
    const auto postprocess_fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = postprocess_fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = postprocess_fragment_artifact.words.data(),
    });

    const auto scene_pipeline_layout = rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{});
    mirakana::rhi::GraphicsPipelineDesc scene_pipeline_desc{
        .layout = scene_pipeline_layout,
        .vertex_shader = scene_vertex_shader,
        .fragment_shader = scene_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    scene_pipeline_desc.vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
        .binding = 0, .stride = 28, .input_rate = mirakana::rhi::VertexInputRate::vertex}};
    scene_pipeline_desc.vertex_attributes = {
        mirakana::rhi::VertexAttributeDesc{.location = 0,
                                           .binding = 0,
                                           .offset = 0,
                                           .format = mirakana::rhi::VertexFormat::float32x3,
                                           .semantic = mirakana::rhi::VertexSemantic::position,
                                           .semantic_index = 0},
        mirakana::rhi::VertexAttributeDesc{.location = 1,
                                           .binding = 0,
                                           .offset = 12,
                                           .format = mirakana::rhi::VertexFormat::float32x4,
                                           .semantic = mirakana::rhi::VertexSemantic::color,
                                           .semantic_index = 0},
    };
    scene_pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto scene_pipeline = rhi->create_graphics_pipeline(scene_pipeline_desc);

    const auto postprocess_layout = rhi->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::postprocess_scene_color_texture_binding(),
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::postprocess_scene_color_sampler_binding(),
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::postprocess_scene_depth_texture_binding(),
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::postprocess_scene_depth_sampler_binding(),
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto postprocess_pipeline_layout = rhi->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {postprocess_layout}, .push_constant_bytes = 0});
    const auto postprocess_pipeline = rhi->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = postprocess_pipeline_layout,
        .vertex_shader = postprocess_vertex_shader,
        .fragment_shader = postprocess_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    const auto scene_color = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto scene_depth = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto postprocess_target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto sampler_desc = mirakana::rhi::SamplerDesc{
        .min_filter = mirakana::rhi::SamplerFilter::nearest,
        .mag_filter = mirakana::rhi::SamplerFilter::nearest,
        .address_u = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
    };
    const auto scene_color_sampler = rhi->create_sampler(sampler_desc);
    const auto scene_depth_sampler = rhi->create_sampler(sampler_desc);
    const auto postprocess_set = rhi->allocate_descriptor_set(postprocess_layout);
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = postprocess_set,
        .binding = mirakana::postprocess_scene_color_texture_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 scene_color)},
    });
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = postprocess_set,
        .binding = mirakana::postprocess_scene_color_sampler_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(scene_color_sampler)},
    });
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = postprocess_set,
        .binding = mirakana::postprocess_scene_depth_texture_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 scene_depth)},
    });
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = postprocess_set,
        .binding = mirakana::postprocess_scene_depth_sampler_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(scene_depth_sampler)},
    });

    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 4096, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 8,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->copy_buffer(upload, vertices,
                          mirakana::rhi::BufferCopyRegion{
                              .source_offset = 0, .destination_offset = 0, .size_bytes = sizeof(vertex_data)});
    commands->copy_buffer(upload, indices,
                          mirakana::rhi::BufferCopyRegion{.source_offset = sizeof(vertex_data),
                                                          .destination_offset = 0,
                                                          .size_bytes = sizeof(index_data)});
    commands->transition_texture(scene_color, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);
    commands->transition_texture(scene_depth, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::depth_write);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = scene_color,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color =
                    mirakana::rhi::ClearColorValue{.red = 0.125F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = scene_depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    commands->bind_graphics_pipeline(scene_pipeline);
    commands->bind_vertex_buffer(
        mirakana::rhi::VertexBufferBinding{.buffer = vertices, .offset = 0, .stride = 28, .binding = 0});
    commands->bind_index_buffer(mirakana::rhi::IndexBufferBinding{
        .buffer = indices, .offset = 0, .format = mirakana::rhi::IndexFormat::uint16});
    commands->draw_indexed(3, 1);
    commands->end_render_pass();
    commands->transition_texture(scene_color, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->transition_texture(scene_depth, mirakana::rhi::ResourceState::depth_write,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->transition_texture(postprocess_target, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = postprocess_target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
    });
    commands->bind_graphics_pipeline(postprocess_pipeline);
    commands->bind_descriptor_set(postprocess_pipeline_layout, 0, postprocess_set);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->transition_texture(scene_depth, mirakana::rhi::ResourceState::shader_read,
                                 mirakana::rhi::ResourceState::depth_write);
    commands->transition_texture(postprocess_target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(postprocess_target, readback, footprint);
    commands->close();

    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto bytes = rhi->read_buffer(readback, 0, 8 * 8 * 4);
    const auto center_pixel = (4U * 8U * 4U) + (4U * 4U);
    const auto corner_pixel = 0U;
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 55 && bytes.at(center_pixel + 0U) <= 75);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 248);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 180 && bytes.at(center_pixel + 2U) <= 205);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    MK_REQUIRE(bytes.at(corner_pixel + 0U) >= 248);
    MK_REQUIRE(bytes.at(corner_pixel + 1U) <= 8);
    MK_REQUIRE(bytes.at(corner_pixel + 2U) <= 8);
    MK_REQUIRE(bytes.at(corner_pixel + 3U) == 255);
    MK_REQUIRE(rhi->stats().samplers_created == 2);
    MK_REQUIRE(rhi->stats().descriptor_writes == 4);
    MK_REQUIRE(rhi->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(rhi->stats().draw_calls == 2);
    MK_REQUIRE(rhi->stats().indexed_draw_calls == 1);
    MK_REQUIRE(rhi->stats().texture_buffer_copies == 1);
#endif
}

MK_TEST("vulkan rhi device bridge darkens a directional shadow receiver when configured") {
#if defined(_WIN32) || defined(__linux__)
    const auto depth_vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_DEPTH_VERTEX_SPV");
    const auto depth_fragment_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV");
    const auto receiver_vertex_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV");
    const auto receiver_fragment_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV");
    if (!depth_vertex_artifact.configured && !depth_fragment_artifact.configured &&
        !receiver_vertex_artifact.configured && !receiver_fragment_artifact.configured) {
        return;
    }

    MK_REQUIRE(depth_vertex_artifact.configured);
    MK_REQUIRE(depth_fragment_artifact.configured);
    MK_REQUIRE(receiver_vertex_artifact.configured);
    MK_REQUIRE(receiver_fragment_artifact.configured);
    MK_REQUIRE(depth_vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(depth_fragment_artifact.diagnostic == "loaded");
    MK_REQUIRE(receiver_vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(receiver_fragment_artifact.diagnostic == "loaded");

    const auto depth_vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = depth_vertex_artifact.words.data(),
            .bytecode_size = depth_vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto depth_fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = depth_fragment_artifact.words.data(),
            .bytecode_size = depth_fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto receiver_vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = receiver_vertex_artifact.words.data(),
            .bytecode_size = receiver_vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto receiver_fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = receiver_fragment_artifact.words.data(),
            .bytecode_size = receiver_fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(depth_vertex_validation.valid);
    MK_REQUIRE(depth_fragment_validation.valid);
    MK_REQUIRE(receiver_vertex_validation.valid);
    MK_REQUIRE(receiver_fragment_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiConfiguredShadowReceiver";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto shadow_map_plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 8, .height = 8},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(shadow_map_plan.succeeded());
    mirakana::DirectionalShadowLightSpacePlan light_space_plan_b;
    light_space_plan_b.clip_from_world_cascades.push_back(mirakana::Mat4::identity());
    const auto receiver_plan = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &shadow_map_plan,
        .light_space = &light_space_plan_b,
        .depth_bias = 0.002F,
        .lit_intensity = 1.0F,
        .shadow_intensity = 0.3F,
    });
    MK_REQUIRE(receiver_plan.succeeded());

    std::array<std::uint8_t, mirakana::shadow_receiver_constants_byte_size()> receiver_cb_bytes{};
    mirakana::pack_shadow_receiver_constants(receiver_cb_bytes, light_space_plan_b,
                                             shadow_map_plan.directional_cascade_count, mirakana::Mat4::identity());

    const auto upload = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 512, .usage = mirakana::rhi::BufferUsage::copy_source});
    const auto vertices = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto indices = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 64, .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_destination});

    constexpr std::array<float, 42> vertex_data{
        -0.8F, -0.8F, 0.25F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F,  0.8F,  0.25F, 0.0F, 1.0F, 0.0F, 1.0F,
        0.8F,  -0.8F, 0.25F, 0.0F, 1.0F, 0.0F, 1.0F, -0.8F, -0.8F, 0.75F, 1.0F, 0.0F, 0.0F, 1.0F,
        0.0F,  0.8F,  0.75F, 1.0F, 0.0F, 0.0F, 1.0F, 0.8F,  -0.8F, 0.75F, 1.0F, 0.0F, 0.0F, 1.0F,
    };
    constexpr std::array<std::uint16_t, 6> index_data{0, 1, 2, 3, 4, 5};
    std::array<std::uint8_t, 512> upload_bytes{};
    const auto vertex_bytes = std::as_bytes(std::span{vertex_data});
    const auto index_bytes = std::as_bytes(std::span{index_data});
    auto upload_bytes_view = std::span{upload_bytes};
    std::memcpy(upload_bytes_view.data(), vertex_bytes.data(), vertex_bytes.size());
    std::memcpy(upload_bytes_view.subspan(vertex_bytes.size()).data(), index_bytes.data(), index_bytes.size());
    rhi->write_buffer(upload, 0, upload_bytes);

    const auto depth_vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = depth_vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = depth_vertex_artifact.words.data(),
    });
    const auto depth_fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = depth_fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = depth_fragment_artifact.words.data(),
    });
    const auto receiver_vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = receiver_vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = receiver_vertex_artifact.words.data(),
    });
    const auto receiver_fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = receiver_fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = receiver_fragment_artifact.words.data(),
    });

    const auto depth_pipeline_layout = rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{});
    mirakana::rhi::GraphicsPipelineDesc depth_pipeline_desc{
        .layout = depth_pipeline_layout,
        .vertex_shader = depth_vertex_shader,
        .fragment_shader = depth_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    depth_pipeline_desc.vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
        .binding = 0, .stride = 28, .input_rate = mirakana::rhi::VertexInputRate::vertex}};
    depth_pipeline_desc.vertex_attributes = {
        mirakana::rhi::VertexAttributeDesc{.location = 0,
                                           .binding = 0,
                                           .offset = 0,
                                           .format = mirakana::rhi::VertexFormat::float32x3,
                                           .semantic = mirakana::rhi::VertexSemantic::position,
                                           .semantic_index = 0},
        mirakana::rhi::VertexAttributeDesc{.location = 1,
                                           .binding = 0,
                                           .offset = 12,
                                           .format = mirakana::rhi::VertexFormat::float32x4,
                                           .semantic = mirakana::rhi::VertexSemantic::color,
                                           .semantic_index = 0},
    };
    depth_pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto depth_pipeline = rhi->create_graphics_pipeline(depth_pipeline_desc);

    const auto receiver_layout = rhi->create_descriptor_set_layout(receiver_plan.descriptor_set_layout);
    const auto receiver_pipeline_layout = rhi->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {receiver_layout}, .push_constant_bytes = 0});
    const auto receiver_pipeline = rhi->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = receiver_pipeline_layout,
        .vertex_shader = receiver_vertex_shader,
        .fragment_shader = receiver_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    const auto shadow_color = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto shadow_depth = rhi->create_texture(receiver_plan.depth_texture);
    const auto receiver_target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto sampler = rhi->create_sampler(receiver_plan.sampler);
    const auto receiver_constants_buffer = rhi->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::shadow_receiver_constants_byte_size(),
        .usage = mirakana::rhi::BufferUsage::uniform | mirakana::rhi::BufferUsage::copy_source,
    });
    rhi->write_buffer(receiver_constants_buffer, 0,
                      std::span<const std::uint8_t>(receiver_cb_bytes.data(), receiver_cb_bytes.size()));
    const auto receiver_set = rhi->allocate_descriptor_set(receiver_layout);
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = receiver_set,
        .binding = mirakana::shadow_receiver_depth_texture_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 shadow_depth)},
    });
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = receiver_set,
        .binding = mirakana::shadow_receiver_sampler_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(sampler)},
    });
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = receiver_set,
        .binding = mirakana::shadow_receiver_constants_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                receiver_constants_buffer)},
    });

    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 4096, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 8,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };

    auto depth_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    depth_commands->copy_buffer(upload, vertices,
                                mirakana::rhi::BufferCopyRegion{
                                    .source_offset = 0, .destination_offset = 0, .size_bytes = sizeof(vertex_data)});
    depth_commands->copy_buffer(upload, indices,
                                mirakana::rhi::BufferCopyRegion{.source_offset = sizeof(vertex_data),
                                                                .destination_offset = 0,
                                                                .size_bytes = sizeof(index_data)});
    depth_commands->transition_texture(shadow_color, mirakana::rhi::ResourceState::undefined,
                                       mirakana::rhi::ResourceState::render_target);
    depth_commands->transition_texture(shadow_depth, mirakana::rhi::ResourceState::undefined,
                                       mirakana::rhi::ResourceState::depth_write);
    depth_commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = shadow_color,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 0.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = shadow_depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    depth_commands->bind_graphics_pipeline(depth_pipeline);
    depth_commands->bind_vertex_buffer(
        mirakana::rhi::VertexBufferBinding{.buffer = vertices, .offset = 0, .stride = 28, .binding = 0});
    depth_commands->bind_index_buffer(mirakana::rhi::IndexBufferBinding{
        .buffer = indices, .offset = 0, .format = mirakana::rhi::IndexFormat::uint16});
    depth_commands->draw_indexed(6, 1);
    depth_commands->end_render_pass();
    depth_commands->transition_texture(shadow_depth, mirakana::rhi::ResourceState::depth_write,
                                       mirakana::rhi::ResourceState::shader_read);
    depth_commands->close();

    const auto depth_fence = rhi->submit(*depth_commands);
    rhi->wait(depth_fence);

    auto receiver_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    receiver_commands->transition_texture(receiver_target, mirakana::rhi::ResourceState::undefined,
                                          mirakana::rhi::ResourceState::render_target);
    receiver_commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = receiver_target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
    });
    receiver_commands->bind_graphics_pipeline(receiver_pipeline);
    receiver_commands->bind_descriptor_set(receiver_pipeline_layout, 0, receiver_set);
    receiver_commands->draw(3, 1);
    receiver_commands->end_render_pass();
    receiver_commands->transition_texture(receiver_target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    receiver_commands->copy_texture_to_buffer(receiver_target, readback, footprint);
    receiver_commands->close();

    const auto receiver_fence = rhi->submit(*receiver_commands);
    rhi->wait(receiver_fence);

    const auto bytes = rhi->read_buffer(readback, 0, 8 * 8 * 4);
    const auto center_pixel = (4U * 8U * 4U) + (4U * 4U);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 68 && bytes.at(center_pixel + 0U) <= 84);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 68 && bytes.at(center_pixel + 1U) <= 84);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 68 && bytes.at(center_pixel + 2U) <= 84);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    const auto lit_pixel = 0U;
    MK_REQUIRE(bytes.at(lit_pixel + 0U) >= 248);
    MK_REQUIRE(bytes.at(lit_pixel + 1U) >= 248);
    MK_REQUIRE(bytes.at(lit_pixel + 2U) >= 248);
    MK_REQUIRE(bytes.at(lit_pixel + 3U) == 255);
    MK_REQUIRE(rhi->stats().samplers_created == 1);
    MK_REQUIRE(rhi->stats().descriptor_writes == 3);
    MK_REQUIRE(rhi->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(rhi->stats().draw_calls == 2);
    MK_REQUIRE(rhi->stats().indexed_draw_calls == 1);
    MK_REQUIRE(rhi->stats().texture_buffer_copies == 1);
#endif
}

MK_TEST("vulkan rhi device bridge proves visible texture sampling with configured SPIR-V artifacts") {
#if defined(_WIN32) || defined(__linux__)
    const auto vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_TEXTURE_VERTEX_SPV");
    const auto fragment_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_TEXTURE_FRAGMENT_SPV");
    if (!vertex_artifact.configured && !fragment_artifact.configured) {
        return;
    }

    MK_REQUIRE(vertex_artifact.configured);
    MK_REQUIRE(fragment_artifact.configured);
    MK_REQUIRE(vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(fragment_artifact.diagnostic == "loaded");

    const auto vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = vertex_artifact.words.data(),
            .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = fragment_artifact.words.data(),
            .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(vertex_validation.valid);
    MK_REQUIRE(fragment_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiConfiguredVisibleTextureSampling";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto upload = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 256, .usage = mirakana::rhi::BufferUsage::copy_source});
    std::array<std::uint8_t, 256> upload_bytes{};
    upload_bytes[0] = 32;
    upload_bytes[1] = 180;
    upload_bytes[2] = 224;
    upload_bytes[3] = 255;
    rhi->write_buffer(upload, 0, upload_bytes);

    const auto sampled_texture = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto sampler = rhi->create_sampler(mirakana::rhi::SamplerDesc{
        .min_filter = mirakana::rhi::SamplerFilter::nearest,
        .mag_filter = mirakana::rhi::SamplerFilter::nearest,
        .address_u = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
    });
    const auto target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 4096, .usage = mirakana::rhi::BufferUsage::copy_destination});

    const auto set_layout = rhi->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 1,
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto descriptor_set = rhi->allocate_descriptor_set(set_layout);
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 sampled_texture)},
    });
    rhi->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 1,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(sampler)},
    });

    const auto vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = vertex_artifact.words.data(),
    });
    const auto fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = fragment_artifact.words.data(),
    });
    const auto pipeline_layout = rhi->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {set_layout}, .push_constant_bytes = 0});
    const auto pipeline = rhi->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    const mirakana::rhi::BufferTextureCopyRegion upload_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 1,
        .buffer_image_height = 1,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
    };
    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 8,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(sampled_texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::copy_destination);
    commands->copy_buffer_to_texture(upload, sampled_texture, upload_footprint);
    commands->transition_texture(sampled_texture, mirakana::rhi::ResourceState::copy_destination,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->bind_descriptor_set(pipeline_layout, 0, descriptor_set);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(target, readback, readback_footprint);
    commands->close();

    const auto fence = rhi->submit(*commands);
    rhi->wait(fence);

    const auto bytes = rhi->read_buffer(readback, 0, 8 * 8 * 4);
    const auto center_pixel = (4U * 8U * 4U) + (4U * 4U);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 30 && bytes.at(center_pixel + 0U) <= 34);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 178 && bytes.at(center_pixel + 1U) <= 182);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 222 && bytes.at(center_pixel + 2U) <= 226);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    MK_REQUIRE(rhi->stats().samplers_created == 1);
    MK_REQUIRE(rhi->stats().descriptor_writes == 2);
    MK_REQUIRE(rhi->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(rhi->stats().buffer_texture_copies == 1);
    MK_REQUIRE(rhi->stats().draw_calls == 1);
    MK_REQUIRE(rhi->stats().texture_buffer_copies == 1);
#endif
}

MK_TEST("vulkan rhi frame renderer visibly samples runtime material texture bindings when configured") {
#if defined(_WIN32) || defined(__linux__)
    const auto vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_RUNTIME_MATERIAL_VERTEX_SPV");
    const auto fragment_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_RUNTIME_MATERIAL_FRAGMENT_SPV");
    if (!vertex_artifact.configured && !fragment_artifact.configured) {
        return;
    }

    MK_REQUIRE(vertex_artifact.configured);
    MK_REQUIRE(fragment_artifact.configured);
    MK_REQUIRE(vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(fragment_artifact.diagnostic == "loaded");

    const auto vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = vertex_artifact.words.data(),
            .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = fragment_artifact.words.data(),
            .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(vertex_validation.valid);
    MK_REQUIRE(fragment_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRuntimeMaterialSampling";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto texture_asset = mirakana::AssetId::from_name("textures/vulkan_runtime_base_color");
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture_asset,
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .width = 1,
        .height = 1,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 4,
        .bytes = std::vector<std::uint8_t>{64, 200, 80, 255},
    };
    const auto texture_upload = mirakana::runtime_rhi::upload_runtime_texture(*rhi, payload);
    MK_REQUIRE(texture_upload.succeeded());
    MK_REQUIRE(texture_upload.copy_recorded);

    const mirakana::MaterialDefinition material{
        .id = mirakana::AssetId::from_name("materials/vulkan_runtime_base_color"),
        .name = "VulkanRuntimeBaseColor",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.5F, 0.5F, 1.0F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 1.0F,
            },
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_asset}},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);
    const auto material_binding = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
        *rhi, metadata, material.factors,
        {mirakana::runtime_rhi::RuntimeMaterialTextureResource{.slot = mirakana::MaterialTextureSlot::base_color,
                                                               .texture = texture_upload.texture,
                                                               .owner_device = texture_upload.owner_device}});
    MK_REQUIRE(material_binding.succeeded());

    const auto target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 4096, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const auto pipeline_layout = rhi->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = vertex_artifact.words.data(),
    });
    const auto fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = fragment_artifact.words.data(),
    });
    const auto pipeline = rhi->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    auto prepare_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = rhi->submit(*prepare_commands);
    rhi->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = rhi.get(),
        .extent = mirakana::Extent2D{.width = 8, .height = 8},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = mirakana::AssetId{},
        .material = material.id,
        .world_from_node = mirakana::Mat4::identity(),
        .mesh_binding = mirakana::MeshGpuBinding{},
        .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = pipeline_layout,
                                                         .descriptor_set = material_binding.descriptor_set,
                                                         .descriptor_set_index = 0,
                                                         .owner_device = rhi.get()},
    });
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 8,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };
    auto readback_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = rhi->submit(*readback_commands);
    rhi->wait(readback_fence);

    const auto bytes = rhi->read_buffer(readback, 0, 8 * 8 * 4);
    const auto center_pixel = (4U * 8U * 4U) + (4U * 4U);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 30 && bytes.at(center_pixel + 0U) <= 34);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 98 && bytes.at(center_pixel + 1U) <= 102);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 78 && bytes.at(center_pixel + 2U) <= 82);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(rhi->stats().samplers_created == 1);
    MK_REQUIRE(rhi->stats().descriptor_writes == 3);
    MK_REQUIRE(rhi->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(rhi->stats().buffer_texture_copies == 1);
    MK_REQUIRE(rhi->stats().texture_buffer_copies == 1);
    MK_REQUIRE(rhi->stats().draw_calls == 1);
#endif
}

MK_TEST("vulkan rhi frame renderer visibly draws cooked runtime scene gpu palette when configured") {
#if defined(_WIN32) || defined(__linux__)
    const auto vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_RUNTIME_SCENE_VERTEX_SPV");
    const auto fragment_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_RUNTIME_SCENE_FRAGMENT_SPV");
    if (!vertex_artifact.configured && !fragment_artifact.configured) {
        return;
    }

    MK_REQUIRE(vertex_artifact.configured);
    MK_REQUIRE(fragment_artifact.configured);
    MK_REQUIRE(vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(fragment_artifact.diagnostic == "loaded");

    const auto vertex_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .bytecode = vertex_artifact.words.data(),
            .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        });
    const auto fragment_validation =
        mirakana::rhi::vulkan::validate_spirv_shader_artifact(mirakana::rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = mirakana::rhi::ShaderStage::fragment,
            .bytecode = fragment_artifact.words.data(),
            .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        });
    MK_REQUIRE(vertex_validation.valid);
    MK_REQUIRE(fragment_validation.valid);

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRuntimeSceneGpuPalette";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }
    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
#else
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{mirakana::rhi::current_rhi_host_platform()}, instance_desc, {});
#endif
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    const auto mesh = mirakana::AssetId::from_name("meshes/vulkan_runtime_scene_triangle");
    const auto material = mirakana::AssetId::from_name("materials/vulkan_runtime_scene_material");
    const auto texture = mirakana::AssetId::from_name("textures/vulkan_runtime_scene_base_color");
    const auto package = make_vulkan_runtime_scene_package(mesh, material, texture);
    const auto scene = make_vulkan_runtime_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);

    const auto gpu_bindings =
        mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(*rhi, package, packet);

    MK_REQUIRE(gpu_bindings.succeeded());
    MK_REQUIRE(gpu_bindings.palette.mesh_count() == 1);
    MK_REQUIRE(gpu_bindings.palette.material_count() == 1);
    MK_REQUIRE(gpu_bindings.material_pipeline_layouts.size() == 1);

    const auto* mesh_binding = gpu_bindings.palette.find_mesh(mesh);
    const auto* material_binding = gpu_bindings.palette.find_material(material);
    MK_REQUIRE(mesh_binding != nullptr);
    MK_REQUIRE(material_binding != nullptr);
    MK_REQUIRE(mesh_binding->owner_device == rhi.get());
    MK_REQUIRE(material_binding->owner_device == rhi.get());
    MK_REQUIRE(material_binding->pipeline_layout.value == gpu_bindings.material_pipeline_layouts[0].value);

    const auto target = rhi->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = rhi->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 256 * 64, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const auto vertex_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = vertex_artifact.words.data(),
    });
    const auto fragment_shader = rhi->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = fragment_artifact.words.size() * sizeof(std::uint32_t),
        .bytecode = fragment_artifact.words.data(),
    });
    const auto pipeline = rhi->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = material_binding->pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0, .stride = mesh_binding->vertex_stride, .input_rate = mirakana::rhi::VertexInputRate::vertex}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
            .semantic_index = 0,
        }},
    });

    auto prepare_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = rhi->submit(*prepare_commands);
    rhi->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = rhi.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });
    renderer.begin_frame();
    const auto submitted = mirakana::submit_scene_render_packet(
        renderer, packet,
        mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                            mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                                        .material_palette = nullptr,
                                        .gpu_bindings = &gpu_bindings.palette});
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };
    auto readback_commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = rhi->submit(*readback_commands);
    rhi->wait(readback_fence);

    const auto bytes = rhi->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 30 && bytes.at(center_pixel + 0U) <= 34);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 98 && bytes.at(center_pixel + 1U) <= 102);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 78 && bytes.at(center_pixel + 2U) <= 82);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);

    MK_REQUIRE(submitted.meshes_submitted == 1);
    MK_REQUIRE(submitted.mesh_gpu_bindings_resolved == 1);
    MK_REQUIRE(submitted.material_gpu_bindings_resolved == 1);
    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(rhi->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(rhi->stats().indexed_draw_calls == 1);
    MK_REQUIRE(rhi->stats().indices_submitted == 3);
    MK_REQUIRE(rhi->stats().texture_buffer_copies == 1);
    MK_REQUIRE(rhi->stats().buffer_reads == 1);
#endif
}

MK_TEST("vulkan rhi device bridge owns swapchains and explicit frame release when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiSwapchainBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    mirakana::rhi::SwapchainHandle swapchain;
    try {
        swapchain = rhi->create_swapchain(mirakana::rhi::SwapchainDesc{
            .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
            .format = mirakana::rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = true,
            .surface = surface,
        });
    } catch (const std::invalid_argument& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi swapchain") != std::string::npos);
        return;
    }
    MK_REQUIRE(swapchain.value == 1);
    MK_REQUIRE(rhi->stats().swapchains_created == 1);

    mirakana::rhi::SwapchainFrameHandle frame;
    try {
        frame = rhi->acquire_swapchain_frame(swapchain);
    } catch (const std::logic_error& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi swapchain frame acquisition failed") !=
                   std::string::npos);
        return;
    }
    MK_REQUIRE(frame.value == 1);
    MK_REQUIRE(rhi->stats().swapchain_frames_acquired == 1);

    rhi->release_swapchain_frame(frame);
    MK_REQUIRE(rhi->stats().swapchain_frames_released == 1);

    try {
        rhi->resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 80, .height = 80});
    } catch (const std::invalid_argument& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi swapchain") != std::string::npos);
        return;
    }
    MK_REQUIRE(rhi->stats().swapchain_resizes == 1);
#endif
}

MK_TEST("vulkan rhi device bridge records swapchain clear present submit and wait when runtime is available") {
#if defined(_WIN32)
    HiddenVulkanTestWindow window;
    if (!window.valid()) {
        return;
    }

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanRhiCommandBridge";
    instance_desc.api_version = mirakana::rhi::vulkan::make_vulkan_api_version(1, 3);

    const mirakana::rhi::SurfaceHandle surface{reinterpret_cast<std::uintptr_t>(window.hwnd())};
    auto device_result = mirakana::rhi::vulkan::create_runtime_device(
        mirakana::rhi::vulkan::VulkanLoaderProbeDesc{.host = mirakana::rhi::current_rhi_host_platform()}, instance_desc,
        {}, surface);
    if (!device_result.created) {
        MK_REQUIRE(!device_result.diagnostic.empty());
        return;
    }

    auto rhi =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(rhi != nullptr);

    mirakana::rhi::SwapchainHandle swapchain;
    try {
        swapchain = rhi->create_swapchain(mirakana::rhi::SwapchainDesc{
            .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
            .format = mirakana::rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = true,
            .surface = surface,
        });
    } catch (const std::invalid_argument& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi swapchain") != std::string::npos);
        return;
    }

    mirakana::rhi::SwapchainFrameHandle frame;
    try {
        frame = rhi->acquire_swapchain_frame(swapchain);
    } catch (const std::logic_error& error) {
        MK_REQUIRE(std::string{error.what()}.find("vulkan rhi swapchain frame acquisition failed") !=
                   std::string::npos);
        return;
    }

    auto commands = rhi->begin_command_list(mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(commands != nullptr);
    MK_REQUIRE(commands->queue_kind() == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(!commands->closed());

    mirakana::rhi::RenderPassDesc pass;
    pass.color.swapchain_frame = frame;
    pass.color.load_action = mirakana::rhi::LoadAction::clear;
    pass.color.store_action = mirakana::rhi::StoreAction::store;
    pass.color.clear_color = mirakana::rhi::ClearColorValue{.red = 0.2F, .green = 0.45F, .blue = 0.85F, .alpha = 1.0F};
    commands->begin_render_pass(pass);
    commands->end_render_pass();
    commands->present(frame);
    commands->close();
    MK_REQUIRE(commands->closed());

    const auto fence = rhi->submit(*commands);
    MK_REQUIRE(fence.value == 1);
    rhi->wait(fence);

    const auto stats = rhi->stats();
    MK_REQUIRE(stats.command_lists_begun == 1);
    MK_REQUIRE(stats.command_lists_submitted == 1);
    MK_REQUIRE(stats.render_passes_begun == 1);
    MK_REQUIRE(stats.resource_transitions == 2);
    MK_REQUIRE(stats.present_calls == 1);
    MK_REQUIRE(stats.swapchain_frames_released == 1);
    MK_REQUIRE(stats.fences_signaled == 1);
    MK_REQUIRE(stats.fence_waits == 1);
    MK_REQUIRE(stats.last_submitted_fence_value == fence.value);
    MK_REQUIRE(stats.last_completed_fence_value == fence.value);
#endif
}

MK_TEST("metal shader library artifact validation requires non-empty metallib bytes") {
    const auto missing = mirakana::rhi::metal::validate_shader_library_artifact({});
    MK_REQUIRE(!missing.valid);
    MK_REQUIRE(missing.diagnostic == "Metal shader library bytecode is required");

    const std::array<std::uint8_t, 4> metallib_bytes{0x4d, 0x54, 0x4c, 0x42};
    const auto valid =
        mirakana::rhi::metal::validate_shader_library_artifact(mirakana::rhi::metal::MetalShaderLibraryArtifactDesc{
            .bytecode = metallib_bytes.data(),
            .bytecode_size = metallib_bytes.size(),
        });
    MK_REQUIRE(valid.valid);
    MK_REQUIRE(valid.bytecode_size == metallib_bytes.size());
    MK_REQUIRE(valid.diagnostic == "Metal shader library artifact ready");
}

MK_TEST("metal runtime readiness plan gates host device queue and shader library") {
    const std::array<std::uint8_t, 4> metallib_bytes{0x4d, 0x54, 0x4c, 0x42};
    const auto shader_library =
        mirakana::rhi::metal::validate_shader_library_artifact(mirakana::rhi::metal::MetalShaderLibraryArtifactDesc{
            .bytecode = metallib_bytes.data(),
            .bytecode_size = metallib_bytes.size(),
        });

    auto desc = mirakana::rhi::metal::MetalRuntimeReadinessDesc{
        .host = mirakana::rhi::RhiHostPlatform::macos,
        .runtime_available = true,
        .default_device_created = true,
        .command_queue_created = true,
        .shader_library = shader_library,
    };
    const auto ready = mirakana::rhi::metal::build_runtime_readiness_plan(desc);
    MK_REQUIRE(ready.probe.status == mirakana::rhi::BackendProbeStatus::available);
    MK_REQUIRE(ready.host_supported);
    MK_REQUIRE(ready.runtime_loaded);
    MK_REQUIRE(ready.default_device_ready);
    MK_REQUIRE(ready.command_queue_ready);
    MK_REQUIRE(ready.shader_library_ready);
    MK_REQUIRE(ready.diagnostic == "Metal runtime readiness plan ready");

    desc.host = mirakana::rhi::RhiHostPlatform::windows;
    const auto unsupported = mirakana::rhi::metal::build_runtime_readiness_plan(desc);
    MK_REQUIRE(unsupported.probe.status == mirakana::rhi::BackendProbeStatus::unsupported_host);
    MK_REQUIRE(unsupported.diagnostic == "Metal is only available on Apple platforms");

    desc.host = mirakana::rhi::RhiHostPlatform::ios;
    desc.runtime_available = false;
    const auto missing_runtime = mirakana::rhi::metal::build_runtime_readiness_plan(desc);
    MK_REQUIRE(missing_runtime.probe.status == mirakana::rhi::BackendProbeStatus::missing_runtime);
    MK_REQUIRE(missing_runtime.diagnostic == "Metal runtime is unavailable");

    desc.runtime_available = true;
    desc.default_device_created = false;
    const auto missing_device = mirakana::rhi::metal::build_runtime_readiness_plan(desc);
    MK_REQUIRE(missing_device.probe.status == mirakana::rhi::BackendProbeStatus::no_suitable_device);
    MK_REQUIRE(missing_device.diagnostic == "Metal default device is required");

    desc.default_device_created = true;
    desc.command_queue_created = false;
    const auto missing_queue = mirakana::rhi::metal::build_runtime_readiness_plan(desc);
    MK_REQUIRE(missing_queue.probe.status == mirakana::rhi::BackendProbeStatus::unavailable);
    MK_REQUIRE(missing_queue.diagnostic == "Metal command queue is required");

    desc.command_queue_created = true;
    desc.shader_library = {};
    const auto missing_library = mirakana::rhi::metal::build_runtime_readiness_plan(desc);
    MK_REQUIRE(missing_library.probe.status == mirakana::rhi::BackendProbeStatus::missing_shader_artifacts);
    MK_REQUIRE(missing_library.diagnostic == "Metal non-empty shader library artifact is required");
}

MK_TEST("metal texture synchronization plan maps rhi states to encoder work") {
    const auto readback_plan =
        mirakana::rhi::metal::build_texture_synchronization_plan(mirakana::rhi::metal::MetalTextureSynchronizationDesc{
            .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
            .before = mirakana::rhi::ResourceState::render_target,
            .after = mirakana::rhi::ResourceState::copy_source,
            .drawable = false,
        });
    MK_REQUIRE(readback_plan.supported);
    MK_REQUIRE(readback_plan.before_usage == mirakana::rhi::metal::MetalResourceUsage::render_target_write);
    MK_REQUIRE(readback_plan.after_usage == mirakana::rhi::metal::MetalResourceUsage::blit_read);
    MK_REQUIRE(readback_plan.requires_encoder_boundary);
    MK_REQUIRE(readback_plan.requires_blit_encoder);
    MK_REQUIRE(!readback_plan.requires_render_encoder);
    MK_REQUIRE(readback_plan.steps.size() == 2);
    MK_REQUIRE(readback_plan.steps[0] == mirakana::rhi::metal::MetalSynchronizationStep::end_render_encoder);
    MK_REQUIRE(readback_plan.steps[1] == mirakana::rhi::metal::MetalSynchronizationStep::begin_blit_encoder);

    const auto render_plan =
        mirakana::rhi::metal::build_texture_synchronization_plan(mirakana::rhi::metal::MetalTextureSynchronizationDesc{
            .usage = mirakana::rhi::TextureUsage::render_target,
            .before = mirakana::rhi::ResourceState::undefined,
            .after = mirakana::rhi::ResourceState::render_target,
            .drawable = false,
        });
    MK_REQUIRE(render_plan.supported);
    MK_REQUIRE(render_plan.before_usage == mirakana::rhi::metal::MetalResourceUsage::none);
    MK_REQUIRE(render_plan.after_usage == mirakana::rhi::metal::MetalResourceUsage::render_target_write);
    MK_REQUIRE(render_plan.requires_render_encoder);
    MK_REQUIRE(render_plan.steps.size() == 1);
    MK_REQUIRE(render_plan.steps[0] == mirakana::rhi::metal::MetalSynchronizationStep::begin_render_encoder);
}

MK_TEST("metal texture synchronization plan rejects usage and drawable mismatches") {
    const auto missing_copy_usage =
        mirakana::rhi::metal::build_texture_synchronization_plan(mirakana::rhi::metal::MetalTextureSynchronizationDesc{
            .usage = mirakana::rhi::TextureUsage::render_target,
            .before = mirakana::rhi::ResourceState::render_target,
            .after = mirakana::rhi::ResourceState::copy_source,
            .drawable = false,
        });
    MK_REQUIRE(!missing_copy_usage.supported);
    MK_REQUIRE(missing_copy_usage.diagnostic == "Metal texture usage does not support destination state");

    const auto missing_drawable =
        mirakana::rhi::metal::build_texture_synchronization_plan(mirakana::rhi::metal::MetalTextureSynchronizationDesc{
            .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::present,
            .before = mirakana::rhi::ResourceState::render_target,
            .after = mirakana::rhi::ResourceState::present,
            .drawable = false,
        });
    MK_REQUIRE(!missing_drawable.supported);
    MK_REQUIRE(missing_drawable.diagnostic == "Metal present state requires a drawable texture");

    const auto present_plan =
        mirakana::rhi::metal::build_texture_synchronization_plan(mirakana::rhi::metal::MetalTextureSynchronizationDesc{
            .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::present,
            .before = mirakana::rhi::ResourceState::render_target,
            .after = mirakana::rhi::ResourceState::present,
            .drawable = true,
        });
    MK_REQUIRE(present_plan.supported);
    MK_REQUIRE(present_plan.after_usage == mirakana::rhi::metal::MetalResourceUsage::drawable_present);
    MK_REQUIRE(present_plan.requires_encoder_boundary);
    MK_REQUIRE(present_plan.requires_drawable_present);
    MK_REQUIRE(present_plan.steps.size() == 2);
    MK_REQUIRE(present_plan.steps[0] == mirakana::rhi::metal::MetalSynchronizationStep::end_render_encoder);
    MK_REQUIRE(present_plan.steps[1] == mirakana::rhi::metal::MetalSynchronizationStep::present_drawable);
}

MK_TEST("metal platform availability diagnostics remain sdk independent") {
    const auto unsupported =
        mirakana::rhi::metal::diagnose_platform_availability(mirakana::rhi::RhiHostPlatform::windows, false);
    MK_REQUIRE(!unsupported.host_supported);
    MK_REQUIRE(!unsupported.can_compile_native_sources);
    MK_REQUIRE(!unsupported.runtime_probe_required);
    MK_REQUIRE(unsupported.diagnostic == "Metal native backend is unavailable on this host");

    const auto apple_without_objcxx =
        mirakana::rhi::metal::diagnose_platform_availability(mirakana::rhi::RhiHostPlatform::macos, false);
    MK_REQUIRE(apple_without_objcxx.host_supported);
    MK_REQUIRE(!apple_without_objcxx.can_compile_native_sources);
    MK_REQUIRE(apple_without_objcxx.runtime_probe_required);
    MK_REQUIRE(apple_without_objcxx.diagnostic == "Metal native backend requires Objective-C++ SDK linkage");

    const auto apple_ready =
        mirakana::rhi::metal::diagnose_platform_availability(mirakana::rhi::RhiHostPlatform::ios, true);
    MK_REQUIRE(apple_ready.host_supported);
    MK_REQUIRE(apple_ready.can_compile_native_sources);
    MK_REQUIRE(apple_ready.runtime_probe_required);
    MK_REQUIRE(apple_ready.diagnostic == "Metal native backend can compile and must probe runtime availability");
}

MK_TEST("metal native device queue and command buffer ownership stays host gated") {
    const auto unsupported = mirakana::rhi::metal::create_native_device_and_command_queue(
        mirakana::rhi::metal::MetalNativeDeviceQueueDesc{mirakana::rhi::RhiHostPlatform::windows});
    MK_REQUIRE(!unsupported.created);
    MK_REQUIRE(!unsupported.device.owns_device());
    MK_REQUIRE(!unsupported.device.owns_command_queue());
    MK_REQUIRE(unsupported.probe.status == mirakana::rhi::BackendProbeStatus::unsupported_host);
    MK_REQUIRE(unsupported.diagnostic == "Metal native device and command queue require an Apple host");

#if defined(__APPLE__)
    auto native = mirakana::rhi::metal::create_native_device_and_command_queue(
        mirakana::rhi::metal::MetalNativeDeviceQueueDesc{mirakana::rhi::current_rhi_host_platform()});
    if (!native.created) {
        MK_REQUIRE(!native.diagnostic.empty());
        return;
    }

    MK_REQUIRE(native.device.owns_device());
    MK_REQUIRE(native.device.owns_command_queue());

    auto command_buffer = mirakana::rhi::metal::create_native_command_buffer(native.device);
    MK_REQUIRE(command_buffer.created);
    MK_REQUIRE(command_buffer.command_buffer.owns_command_buffer());
#endif
}

MK_TEST("metal native texture render encoder and readback ownership stays host gated") {
    mirakana::rhi::metal::MetalRuntimeDevice missing_device;
    const auto missing_texture = mirakana::rhi::metal::create_native_texture_target(
        missing_device,
        mirakana::rhi::metal::MetalTextureTargetDesc{.extent = mirakana::rhi::Extent2D{.width = 4, .height = 4}});
    MK_REQUIRE(!missing_texture.created);
    MK_REQUIRE(missing_texture.diagnostic == "Metal device is required before creating a texture target");

    mirakana::rhi::metal::MetalRuntimeCommandBuffer missing_command_buffer;
    mirakana::rhi::metal::MetalRuntimeTexture missing_texture_target;
    const auto missing_encoder =
        mirakana::rhi::metal::create_native_render_encoder(missing_command_buffer, missing_texture_target);
    MK_REQUIRE(!missing_encoder.created);
    MK_REQUIRE(missing_encoder.diagnostic == "Metal command buffer is required before creating a render encoder");

    const auto missing_readback =
        mirakana::rhi::metal::read_native_texture_bytes(missing_device, missing_texture_target);
    MK_REQUIRE(!missing_readback.read);
    MK_REQUIRE(missing_readback.diagnostic == "Metal command queue is required before texture readback");

#if defined(__APPLE__)
    auto native = mirakana::rhi::metal::create_native_device_and_command_queue(
        mirakana::rhi::metal::MetalNativeDeviceQueueDesc{mirakana::rhi::current_rhi_host_platform()});
    if (!native.created) {
        MK_REQUIRE(!native.diagnostic.empty());
        return;
    }

    auto command_buffer = mirakana::rhi::metal::create_native_command_buffer(native.device);
    MK_REQUIRE(command_buffer.created);

    auto texture = mirakana::rhi::metal::create_native_texture_target(
        native.device, mirakana::rhi::metal::MetalTextureTargetDesc{
                           mirakana::rhi::Extent2D{4, 4},
                           mirakana::rhi::Format::rgba8_unorm,
                           mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
                           false,
                       });
    MK_REQUIRE(texture.created);
    MK_REQUIRE(texture.texture.owns_texture());

    auto encoder = mirakana::rhi::metal::create_native_render_encoder(
        command_buffer.command_buffer, texture.texture,
        mirakana::rhi::metal::MetalRenderEncoderDesc{mirakana::rhi::ClearColorValue{1.0F, 0.0F, 0.0F, 1.0F}, true});
    MK_REQUIRE(encoder.created);
    MK_REQUIRE(encoder.render_encoder.owns_render_encoder());
    encoder.render_encoder.end();
    MK_REQUIRE(encoder.render_encoder.ended());

    const auto submitted = mirakana::rhi::metal::commit_and_wait_native_command_buffer(command_buffer.command_buffer);
    MK_REQUIRE(submitted.submitted);
    MK_REQUIRE(submitted.completed);
    MK_REQUIRE(command_buffer.command_buffer.completed());

    const auto readback = mirakana::rhi::metal::read_native_texture_bytes(native.device, texture.texture);
    MK_REQUIRE(readback.read);
    MK_REQUIRE(readback.bytes.size() == 4 * 4 * 4);
    MK_REQUIRE(has_non_zero_byte(readback.bytes));
#endif
}

MK_TEST("metal native drawable render encoder and present ownership stays host gated") {
    mirakana::rhi::metal::MetalRuntimeDevice missing_device;
    const auto missing_drawable = mirakana::rhi::metal::acquire_native_drawable(
        missing_device, mirakana::rhi::metal::MetalDrawableAcquireDesc{
                            .surface = mirakana::rhi::SurfaceHandle{1},
                            .extent = mirakana::rhi::Extent2D{.width = 4, .height = 4},
                            .format = mirakana::rhi::Format::bgra8_unorm,
                            .framebuffer_only = true,
                        });
    MK_REQUIRE(!missing_drawable.acquired);
    MK_REQUIRE(missing_drawable.diagnostic == "Metal device is required before acquiring a drawable");

    mirakana::rhi::metal::MetalRuntimeCommandBuffer missing_command_buffer;
    mirakana::rhi::metal::MetalRuntimeDrawable missing_drawable_target;
    const auto missing_encoder =
        mirakana::rhi::metal::create_native_render_encoder(missing_command_buffer, missing_drawable_target);
    MK_REQUIRE(!missing_encoder.created);
    MK_REQUIRE(missing_encoder.diagnostic == "Metal command buffer is required before creating a render encoder");

    const auto missing_present =
        mirakana::rhi::metal::present_native_drawable(missing_command_buffer, missing_drawable_target);
    MK_REQUIRE(!missing_present.scheduled);
    MK_REQUIRE(missing_present.diagnostic == "Metal command buffer is required before presenting a drawable");

#if defined(__APPLE__)
    auto native = mirakana::rhi::metal::create_native_device_and_command_queue(
        mirakana::rhi::metal::MetalNativeDeviceQueueDesc{mirakana::rhi::current_rhi_host_platform()});
    if (!native.created) {
        MK_REQUIRE(!native.diagnostic.empty());
        return;
    }

    const auto missing_surface =
        mirakana::rhi::metal::acquire_native_drawable(native.device, mirakana::rhi::metal::MetalDrawableAcquireDesc{
                                                                         mirakana::rhi::SurfaceHandle{},
                                                                         mirakana::rhi::Extent2D{4, 4},
                                                                         mirakana::rhi::Format::bgra8_unorm,
                                                                         true,
                                                                     });
    MK_REQUIRE(!missing_surface.acquired);
    MK_REQUIRE(missing_surface.diagnostic == "Metal drawable surface handle is required");
#endif
}

MK_TEST("metal backend scaffold exposes api independent capability and probe contract") {
    const auto capabilities = mirakana::rhi::metal::capabilities();
    const auto plan = mirakana::rhi::metal::probe_plan();
    const auto missing_library = mirakana::rhi::metal::make_probe_result(
        mirakana::rhi::RhiHostPlatform::ios, mirakana::rhi::BackendProbeStatus::missing_shader_artifacts,
        "Metal library artifact unavailable");

    MK_REQUIRE(mirakana::rhi::metal::backend_kind() == mirakana::rhi::BackendKind::metal);
    MK_REQUIRE(mirakana::rhi::metal::backend_name() == "metal");
    MK_REQUIRE(capabilities.shader_format == mirakana::rhi::ShaderArtifactFormat::metallib);
    MK_REQUIRE(capabilities.surface_presentation);
    MK_REQUIRE(mirakana::rhi::metal::supports_host(mirakana::rhi::RhiHostPlatform::macos));
    MK_REQUIRE(mirakana::rhi::metal::supports_host(mirakana::rhi::RhiHostPlatform::ios));
    MK_REQUIRE(!mirakana::rhi::metal::supports_host(mirakana::rhi::RhiHostPlatform::windows));
    MK_REQUIRE(plan.backend == mirakana::rhi::BackendKind::metal);
    MK_REQUIRE(plan.steps[2] == mirakana::rhi::BackendProbeStep::create_default_device);
    MK_REQUIRE(plan.steps[3] == mirakana::rhi::BackendProbeStep::create_command_queue);
    MK_REQUIRE(missing_library.status == mirakana::rhi::BackendProbeStatus::missing_shader_artifacts);
    MK_REQUIRE(!missing_library.capabilities.native_device);
    MK_REQUIRE(missing_library.diagnostic == "Metal library artifact unavailable");
}

int main() {
    return mirakana::test::run_all();
}
