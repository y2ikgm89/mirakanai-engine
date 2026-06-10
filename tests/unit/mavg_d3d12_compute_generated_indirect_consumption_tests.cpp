// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_gpu_culling.hpp"
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"
#include "mirakana/rhi/d3d12/d3d12_mavg_gpu_culling_dispatch.hpp"
#include "mirakana/rhi/indirect_draw.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace {

[[nodiscard]] bool d3d12_warp_available() noexcept {
    const auto probe = mirakana::rhi::d3d12::probe_runtime();
    return probe.windows_sdk_available && probe.warp_device_supported;
}

[[nodiscard]] mirakana::rhi::d3d12::DeviceBootstrapDesc d3d12_test_device_desc() noexcept {
    return mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = true,
        .enable_debug_layer = false,
    };
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_shader(const char* source, const char* entry_point,
                                                              const char* target) {
    Microsoft::WRL::ComPtr<ID3DBlob> bytecode;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry_point, target,
                                      D3DCOMPILE_ENABLE_STRICTNESS, 0, &bytecode, &errors);
    MK_REQUIRE(SUCCEEDED(result));
    return bytecode;
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_triangle_vertex_shader() {
    return compile_shader(
        "struct VsOut {"
        "  float4 position : SV_Position;"
        "  float4 color : COLOR0;"
        "};"
        "VsOut vs_main(uint vertex_id : SV_VertexID) {"
        "  float2 positions[3] = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };"
        "  float4 colors[3] = { float4(1.0, 0.0, 0.0, 1.0), float4(0.0, 1.0, 0.0, 1.0), float4(0.0, 0.0, 1.0, 1.0) };"
        "  VsOut output;"
        "  output.position = float4(positions[vertex_id], 0.0, 1.0);"
        "  output.color = colors[vertex_id];"
        "  return output;"
        "}",
        "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_solid_orange_pixel_shader() {
    return compile_shader("float4 ps_main(float4 position : SV_Position, float4 color : COLOR0) : SV_Target {"
                          "  return float4(1.0, 0.25, 0.0, 1.0);"
                          "}",
                          "ps_main", "ps_5_0");
}

[[nodiscard]] mirakana::MavgLodSelectionResult make_selection_result() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/compute_generated_indirect_graph");
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

[[nodiscard]] std::vector<mirakana::MavgGpuCullingClusterBoundsRow> make_bounds_rows(bool second_visible) {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/compute_generated_indirect_graph");
    return {
        mirakana::MavgGpuCullingClusterBoundsRow{
            .graph_asset = graph_asset,
            .cluster_index = 1,
            .center = {.x = 0.0F, .y = 0.0F, .z = 1.0F},
            .radius = 1.0F,
            .visible = true,
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

[[nodiscard]] std::vector<mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchClusterRow>
to_d3d12_cluster_rows(std::span<const mirakana::MavgGpuCullingDispatchClusterRow> rows) {
    std::vector<mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchClusterRow> d3d12_rows;
    d3d12_rows.reserve(rows.size());
    for (const auto& row : rows) {
        d3d12_rows.push_back(mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchClusterRow{
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
    return d3d12_rows;
}

[[nodiscard]] std::uint32_t center_pixel_offset() noexcept {
    return (32U * 256U) + (32U * 4U);
}

void render_compute_generated_indirect_triangle(mirakana::rhi::IRhiDevice& device,
                                                mirakana::rhi::BufferHandle argument_buffer,
                                                mirakana::rhi::BufferHandle count_buffer,
                                                std::uint32_t max_draw_count) {
    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_solid_orange_pixel_shader();

    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto vertices = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 96,
        .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_source,
    });
    const auto indices = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = sizeof(std::uint32_t) * 3U,
        .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_source,
    });

    const std::array<std::uint8_t, 96> vertex_bytes{};
    const std::array<std::uint8_t, 12> index_bytes{0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0};
    device.write_buffer(vertices, 0, vertex_bytes);
    device.write_buffer(indices, 0, index_bytes);

    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
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
    commands->bind_vertex_buffer(mirakana::rhi::VertexBufferBinding{.buffer = vertices, .offset = 0, .stride = 32});
    commands->bind_index_buffer(mirakana::rhi::IndexBufferBinding{
        .buffer = indices, .offset = 0, .format = mirakana::rhi::IndexFormat::uint32});
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

    const auto bytes = device.read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = center_pixel_offset();
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 250);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 60 && bytes.at(center_pixel + 1U) <= 68);
    MK_REQUIRE(bytes.at(center_pixel + 2U) <= 5);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
}

} // namespace

MK_TEST("mavg d3d12 compute generated indirect consumption skips when warp is unavailable") {
    if (d3d12_warp_available()) {
        return;
    }
}

MK_TEST("mavg d3d12 dispatch plus draw renders visible geometry from compute generated indirect buffers") {
    if (!d3d12_warp_available()) {
        return;
    }

    auto device = mirakana::rhi::d3d12::create_rhi_device(d3d12_test_device_desc());
    MK_REQUIRE(device != nullptr);

    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows(true);
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
    const auto d3d12_rows = to_d3d12_cluster_rows(cluster_rows);

    const auto argument_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = static_cast<std::uint64_t>(culling_desc.max_command_count) *
                      static_cast<std::uint64_t>(cpu_plan.command_layout.record_stride_bytes),
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });
    const auto count_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::rhi::indexed_indirect_draw_count_buffer_size_bytes,
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });

    const auto dispatch = mirakana::rhi::d3d12::dispatch_mavg_gpu_culling_indirect(
        *device,
        mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchDesc{
            .cluster_rows = std::span<const mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchClusterRow>{d3d12_rows},
            .max_command_count = culling_desc.max_command_count,
            .record_stride_bytes = cpu_plan.command_layout.record_stride_bytes,
            .external_argument_buffer = mirakana::rhi::d3d12::native_buffer_resource(*device, argument_buffer),
            .external_count_buffer = mirakana::rhi::d3d12::native_buffer_resource(*device, count_buffer),
            .leave_indirect_argument_state_for_consumption = true,
        });

    MK_REQUIRE(cpu_plan.succeeded());
    MK_REQUIRE(dispatch.succeeded);
    MK_REQUIRE(dispatch.executed_gpu_culling);
    MK_REQUIRE(dispatch.visible_cluster_count == 2U);
    render_compute_generated_indirect_triangle(*device, argument_buffer, count_buffer, culling_desc.max_command_count);
    MK_REQUIRE(device->stats().indexed_indirect_draw_calls >= 1U);
}

MK_TEST("mavg d3d12 dispatch plus draw respects culled cluster count on compute generated indirect buffers") {
    if (!d3d12_warp_available()) {
        return;
    }

    auto device = mirakana::rhi::d3d12::create_rhi_device(d3d12_test_device_desc());
    MK_REQUIRE(device != nullptr);

    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows(false);
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
    const auto d3d12_rows = to_d3d12_cluster_rows(cluster_rows);

    const auto argument_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = static_cast<std::uint64_t>(culling_desc.max_command_count) *
                      static_cast<std::uint64_t>(cpu_plan.command_layout.record_stride_bytes),
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });
    const auto count_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::rhi::indexed_indirect_draw_count_buffer_size_bytes,
        .usage = mirakana::rhi::BufferUsage::indirect | mirakana::rhi::BufferUsage::storage,
    });

    const auto dispatch = mirakana::rhi::d3d12::dispatch_mavg_gpu_culling_indirect(
        *device,
        mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchDesc{
            .cluster_rows = std::span<const mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchClusterRow>{d3d12_rows},
            .max_command_count = culling_desc.max_command_count,
            .record_stride_bytes = cpu_plan.command_layout.record_stride_bytes,
            .external_argument_buffer = mirakana::rhi::d3d12::native_buffer_resource(*device, argument_buffer),
            .external_count_buffer = mirakana::rhi::d3d12::native_buffer_resource(*device, count_buffer),
            .leave_indirect_argument_state_for_consumption = true,
        });

    MK_REQUIRE(cpu_plan.succeeded());
    MK_REQUIRE(cpu_plan.count_buffer_value == 1U);
    MK_REQUIRE(cpu_plan.culled_cluster_count == 1U);
    MK_REQUIRE(dispatch.succeeded);
    MK_REQUIRE(dispatch.executed_gpu_culling);
    MK_REQUIRE(dispatch.visible_cluster_count == 1U);
    MK_REQUIRE(dispatch.culled_cluster_count == 1U);
    render_compute_generated_indirect_triangle(*device, argument_buffer, count_buffer, 1U);
}

int main() {
    return mirakana::test::run_all();
}
