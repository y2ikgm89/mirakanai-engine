// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_gpu_culling.hpp"
#include "mirakana/rhi/indirect_draw.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/rhi/vulkan/vulkan_mavg_gpu_culling_dispatch.hpp"

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
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <span>
#include <string>
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

        hwnd_ =
            CreateWindowExW(0, class_name, L"GameEngineMavgVulkanIndirectConsumptionTestWindow", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, 128, 128, nullptr, nullptr, instance_, nullptr);
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
    static constexpr const wchar_t* class_name{L"GameEngineMavgVulkanIndirectConsumptionTestWindowClass"};
    HINSTANCE instance_{nullptr};
    HWND hwnd_{nullptr};
    bool registered_{false};
};
#endif

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
    return mirakana::rhi::vulkan::build_rhi_device_mapping_plan(desc);
}

[[nodiscard]] mirakana::MavgLodSelectionResult make_selection_result() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/vulkan_compute_generated_indirect_graph");
    return mirakana::MavgLodSelectionResult{
        .selected_clusters =
            {
                mirakana::MavgLodSelectedCluster{
                    .graph_asset = graph_asset,
                    .cluster_index = 1,
                    .page_index = 0,
                    .lod_level = 0,
                    .material_partition = 0,
                    .first_index = 0,
                    .index_count = 3,
                    .vertex_base = 0,
                    .fallback_substitution = false,
                },
                mirakana::MavgLodSelectedCluster{
                    .graph_asset = graph_asset,
                    .cluster_index = 2,
                    .page_index = 0,
                    .lod_level = 0,
                    .material_partition = 0,
                    .first_index = 0,
                    .index_count = 3,
                    .vertex_base = 0,
                    .fallback_substitution = false,
                },
            },
    };
}

[[nodiscard]] std::vector<mirakana::MavgGpuCullingClusterBoundsRow> make_bounds_rows(bool first_visible,
                                                                                     bool second_visible) {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/vulkan_compute_generated_indirect_graph");
    return {
        mirakana::MavgGpuCullingClusterBoundsRow{
            .graph_asset = graph_asset,
            .cluster_index = 1,
            .center = {.x = 0.0F, .y = 0.0F, .z = 1.0F},
            .radius = 1.0F,
            .visible = first_visible,
        },
        mirakana::MavgGpuCullingClusterBoundsRow{
            .graph_asset = graph_asset,
            .cluster_index = 2,
            .center = {.x = 0.0F, .y = 0.0F, .z = 1.0F},
            .radius = 1.0F,
            .visible = second_visible,
        },
    };
}

[[nodiscard]] std::vector<mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchClusterRow>
to_vulkan_cluster_rows(std::span<const mirakana::MavgGpuCullingDispatchClusterRow> rows) {
    std::vector<mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchClusterRow> vulkan_rows;
    vulkan_rows.reserve(rows.size());
    for (const auto& row : rows) {
        vulkan_rows.push_back(mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchClusterRow{
            .index_count_per_instance = row.index_count_per_instance,
            .instance_count = row.instance_count,
            .start_index_location = row.start_index_location,
            .base_vertex_location = row.base_vertex_location,
            .start_instance_location = row.start_instance_location,
            .visible = row.visible,
            .padding0 = row.padding0,
            .padding1 = row.padding1,
        });
    }
    return vulkan_rows;
}

[[nodiscard]] bool has_non_zero_byte(const std::vector<std::uint8_t>& bytes) noexcept {
    return std::ranges::any_of(bytes, [](const auto byte) { return byte != 0U; });
}

void render_compute_generated_indirect_triangle(mirakana::rhi::IRhiDevice& device,
                                                mirakana::rhi::BufferHandle argument_buffer,
                                                mirakana::rhi::BufferHandle count_buffer, std::uint32_t max_draw_count,
                                                std::span<const std::uint32_t> vertex_spirv,
                                                std::span<const std::uint32_t> fragment_spirv) {
    const auto upload = device.create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 256, .usage = mirakana::rhi::BufferUsage::copy_source});
    const auto vertices = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 64, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto indices = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 64, .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_destination});

    constexpr std::array<float, 9> vertex_data{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    constexpr std::array<std::uint16_t, 3> index_data{0, 1, 2};
    std::array<std::uint8_t, 256> upload_bytes{};
    const auto vertex_bytes = std::as_bytes(std::span{vertex_data});
    const auto index_bytes = std::as_bytes(std::span{index_data});
    std::memcpy(upload_bytes.data(), vertex_bytes.data(), vertex_bytes.size());
    std::memcpy(upload_bytes.data() + vertex_bytes.size(), index_bytes.data(), index_bytes.size());
    device.write_buffer(upload, 0, upload_bytes);

    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "main",
        .bytecode_size = vertex_spirv.size() * sizeof(std::uint32_t),
        .bytecode = vertex_spirv.data(),
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "main",
        .bytecode_size = fragment_spirv.size() * sizeof(std::uint32_t),
        .bytecode = fragment_spirv.data(),
    });
    const auto pipeline_layout = device.create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device.create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 4096, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 8,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->copy_buffer(upload, vertices,
                          mirakana::rhi::BufferCopyRegion{
                              .source_offset = 0, .destination_offset = 0, .size_bytes = sizeof(vertex_data)});
    commands->copy_buffer(upload, indices,
                          mirakana::rhi::BufferCopyRegion{.source_offset = sizeof(vertex_data),
                                                          .destination_offset = 0,
                                                          .size_bytes = sizeof(index_data)});
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
    commands->bind_vertex_buffer(
        mirakana::rhi::VertexBufferBinding{.buffer = vertices, .offset = 0, .stride = 12, .binding = 0});
    commands->bind_index_buffer(mirakana::rhi::IndexBufferBinding{
        .buffer = indices, .offset = 0, .format = mirakana::rhi::IndexFormat::uint16});
    commands->draw_indexed_indirect(mirakana::rhi::IndexedIndirectDrawDesc{
        .argument_buffer = argument_buffer,
        .argument_buffer_offset = 0,
        .command_stride_bytes = mirakana::rhi::indexed_indirect_draw_command_stride_bytes,
        .max_draw_count = max_draw_count,
        .count_buffer = count_buffer,
        .count_buffer_offset = 0,
    });
    commands->end_render_pass();
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(target, readback, footprint);
    commands->close();

    const auto fence = device.submit(*commands);
    device.wait(fence);

    const auto bytes = device.read_buffer(readback, 0, 8 * 8 * 4);
    MK_REQUIRE(has_non_zero_byte(bytes));
}

} // namespace

MK_TEST("mavg vulkan dispatch plus draw consumes compute generated indirect buffers") {
#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))
    const auto compute_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_GPU_CULLING_DISPATCH_SPV");
    const auto vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_VERTEX_SPV");
    const auto fragment_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_FRAGMENT_SPV");
    if (!compute_artifact.configured && !vertex_artifact.configured && !fragment_artifact.configured) {
        return;
    }
    MK_REQUIRE(compute_artifact.diagnostic == "loaded");
    MK_REQUIRE(vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(fragment_artifact.diagnostic == "loaded");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineMavgVulkanComputeGeneratedIndirectConsumption";
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

    auto device =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(device != nullptr);

    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows(true, true);
    const mirakana::MavgGpuCullingIndirectDesc culling_desc{
        .selection = &selection,
        .cluster_bounds = std::span<const mirakana::MavgGpuCullingClusterBoundsRow>{bounds},
        .producer = mirakana::MavgGpuCullingProducer::compute_shader,
        .max_command_count = 4,
        .instance_count = 1,
        .first_instance = 0,
    };
    const auto cpu_plan = mirakana::plan_mavg_gpu_culling_indirect_commands(culling_desc);
    const auto cluster_rows = mirakana::build_mavg_gpu_culling_dispatch_cluster_rows(culling_desc);
    const auto vulkan_rows = to_vulkan_cluster_rows(cluster_rows);
    const auto argument_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = static_cast<std::uint64_t>(culling_desc.max_command_count) *
                      static_cast<std::uint64_t>(cpu_plan.command_layout.record_stride_bytes),
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });
    const auto count_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::rhi::indexed_indirect_draw_count_buffer_size_bytes,
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });

    const auto dispatch = mirakana::rhi::vulkan::dispatch_mavg_gpu_culling_indirect(
        *device,
        mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchDesc{
            .compute_shader_spirv = std::span<const std::uint32_t>{compute_artifact.words},
            .cluster_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchClusterRow>{vulkan_rows},
            .max_command_count = culling_desc.max_command_count,
            .record_stride_bytes = cpu_plan.command_layout.record_stride_bytes,
            .external_argument_buffer = argument_buffer,
            .external_count_buffer = count_buffer,
            .leave_indirect_argument_state_for_consumption = true,
        });

    MK_REQUIRE(cpu_plan.succeeded());
    MK_REQUIRE(dispatch.succeeded);
    MK_REQUIRE(dispatch.executed_gpu_culling);
    MK_REQUIRE(dispatch.visible_cluster_count == 2U);
    render_compute_generated_indirect_triangle(*device, argument_buffer, count_buffer, culling_desc.max_command_count,
                                               std::span<const std::uint32_t>{vertex_artifact.words},
                                               std::span<const std::uint32_t>{fragment_artifact.words});
    MK_REQUIRE(device->stats().indexed_indirect_draw_calls >= 1U);
#endif
}

MK_TEST("mavg vulkan dispatch plus draw respects culled cluster count") {
#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))
    const auto compute_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_GPU_CULLING_DISPATCH_SPV");
    const auto vertex_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_VERTEX_SPV");
    const auto fragment_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_FRAGMENT_SPV");
    if (!compute_artifact.configured && !vertex_artifact.configured && !fragment_artifact.configured) {
        return;
    }
    MK_REQUIRE(compute_artifact.diagnostic == "loaded");
    MK_REQUIRE(vertex_artifact.diagnostic == "loaded");
    MK_REQUIRE(fragment_artifact.diagnostic == "loaded");

    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineMavgVulkanComputeGeneratedIndirectConsumptionCulled";
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

    auto device =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    MK_REQUIRE(device != nullptr);

    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows(true, false);
    const mirakana::MavgGpuCullingIndirectDesc culling_desc{
        .selection = &selection,
        .cluster_bounds = std::span<const mirakana::MavgGpuCullingClusterBoundsRow>{bounds},
        .producer = mirakana::MavgGpuCullingProducer::compute_shader,
        .max_command_count = 4,
        .instance_count = 1,
        .first_instance = 0,
    };
    const auto cpu_plan = mirakana::plan_mavg_gpu_culling_indirect_commands(culling_desc);
    const auto cluster_rows = mirakana::build_mavg_gpu_culling_dispatch_cluster_rows(culling_desc);
    const auto vulkan_rows = to_vulkan_cluster_rows(cluster_rows);
    const auto argument_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = static_cast<std::uint64_t>(culling_desc.max_command_count) *
                      static_cast<std::uint64_t>(cpu_plan.command_layout.record_stride_bytes),
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });
    const auto count_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::rhi::indexed_indirect_draw_count_buffer_size_bytes,
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });

    const auto dispatch = mirakana::rhi::vulkan::dispatch_mavg_gpu_culling_indirect(
        *device,
        mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchDesc{
            .compute_shader_spirv = std::span<const std::uint32_t>{compute_artifact.words},
            .cluster_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchClusterRow>{vulkan_rows},
            .max_command_count = culling_desc.max_command_count,
            .record_stride_bytes = cpu_plan.command_layout.record_stride_bytes,
            .external_argument_buffer = argument_buffer,
            .external_count_buffer = count_buffer,
            .leave_indirect_argument_state_for_consumption = true,
        });

    MK_REQUIRE(cpu_plan.succeeded());
    MK_REQUIRE(dispatch.succeeded);
    MK_REQUIRE(dispatch.executed_gpu_culling);
    MK_REQUIRE(dispatch.visible_cluster_count == 1U);
    MK_REQUIRE(dispatch.count_buffer_value == 1U);
    render_compute_generated_indirect_triangle(*device, argument_buffer, count_buffer, culling_desc.max_command_count,
                                               std::span<const std::uint32_t>{vertex_artifact.words},
                                               std::span<const std::uint32_t>{fragment_artifact.words});
    MK_REQUIRE(device->stats().indexed_indirect_draw_calls >= 1U);
#endif
}

MK_TEST("mavg vulkan dispatch rejects non vulkan and non compute generated external buffers") {
    const std::array<mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchClusterRow, 1> rows{
        mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchClusterRow{
            .index_count_per_instance = 3,
            .instance_count = 1,
            .start_index_location = 0,
            .base_vertex_location = 0,
            .start_instance_location = 0,
            .visible = 1,
        },
    };

    mirakana::rhi::NullRhiDevice null_device;
    const auto null_argument = null_device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::rhi::indexed_indirect_draw_command_stride_bytes,
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });
    const auto null_count = null_device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::rhi::indexed_indirect_draw_count_buffer_size_bytes,
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });
    const auto wrong_device_result = mirakana::rhi::vulkan::dispatch_mavg_gpu_culling_indirect(
        null_device,
        mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchDesc{
            .cluster_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchClusterRow>{rows},
            .max_command_count = 1,
            .external_argument_buffer = null_argument,
            .external_count_buffer = null_count,
        });
    MK_REQUIRE(!wrong_device_result.succeeded);
    MK_REQUIRE(wrong_device_result.failure_stage == 21U);

#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))
    mirakana::rhi::vulkan::VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineMavgVulkanComputeGeneratedIndirectConsumptionInvalidUsage";
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

    auto device =
        mirakana::rhi::vulkan::create_rhi_device(std::move(device_result.device), ready_vulkan_rhi_mapping_plan());
    if (device == nullptr) {
        return;
    }

    const auto invalid_argument = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::rhi::indexed_indirect_draw_command_stride_bytes,
        .usage = mirakana::rhi::BufferUsage::indirect,
    });
    const auto valid_count = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::rhi::indexed_indirect_draw_count_buffer_size_bytes,
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });
    const auto invalid_usage_result = mirakana::rhi::vulkan::dispatch_mavg_gpu_culling_indirect(
        *device,
        mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchDesc{
            .cluster_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgGpuCullingDispatchClusterRow>{rows},
            .max_command_count = 1,
            .external_argument_buffer = invalid_argument,
            .external_count_buffer = valid_count,
        });
    MK_REQUIRE(!invalid_usage_result.succeeded);
    MK_REQUIRE(invalid_usage_result.failure_stage == 24U);
#endif
}

int main() {
    return mirakana::test::run_all();
}
