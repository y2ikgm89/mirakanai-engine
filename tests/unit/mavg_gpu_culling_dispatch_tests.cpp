// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_gpu_culling.hpp"
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"
#include "mirakana/rhi/d3d12/d3d12_mavg_gpu_culling_dispatch.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace {

[[nodiscard]] bool d3d12_warp_available() noexcept {
    const auto probe = mirakana::rhi::d3d12::probe_runtime();
    return probe.windows_sdk_available && probe.warp_device_supported;
}

[[nodiscard]] mirakana::MavgLodSelectionResult make_selection_result() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/gpu_culling_dispatch_graph");
    return mirakana::MavgLodSelectionResult{
        .selected_clusters =
            {
                mirakana::MavgLodSelectedCluster{
                    .graph_asset = graph_asset,
                    .cluster_index = 7,
                    .page_index = 2,
                    .lod_level = 1,
                    .material_partition = 3,
                    .first_index = 96,
                    .index_count = 144,
                    .vertex_base = 12,
                    .fallback_substitution = false,
                },
                mirakana::MavgLodSelectedCluster{
                    .graph_asset = graph_asset,
                    .cluster_index = 8,
                    .page_index = 3,
                    .lod_level = 1,
                    .material_partition = 3,
                    .first_index = 240,
                    .index_count = 96,
                    .vertex_base = 44,
                    .fallback_substitution = true,
                },
            },
    };
}

[[nodiscard]] std::vector<mirakana::MavgGpuCullingClusterBoundsRow> make_bounds_rows(bool second_visible = true) {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/gpu_culling_dispatch_graph");
    return {
        mirakana::MavgGpuCullingClusterBoundsRow{
            .graph_asset = graph_asset,
            .cluster_index = 7,
            .center = {.x = -1.0F, .y = 0.0F, .z = 4.0F},
            .radius = 2.0F,
            .visible = true,
        },
        mirakana::MavgGpuCullingClusterBoundsRow{
            .graph_asset = graph_asset,
            .cluster_index = 8,
            .center = {.x = 2.0F, .y = 0.0F, .z = 4.0F},
            .radius = 2.0F,
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

[[nodiscard]] bool bytes_equal(std::span<const std::uint8_t> lhs, std::span<const std::uint8_t> rhs) noexcept {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

} // namespace

MK_TEST("mavg gpu culling dispatch skips when d3d12 warp is unavailable") {
    if (d3d12_warp_available()) {
        return;
    }

    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows();
    const auto cluster_rows =
        mirakana::build_mavg_gpu_culling_dispatch_cluster_rows(mirakana::MavgGpuCullingIndirectDesc{
            .selection = &selection,
            .cluster_bounds = std::span<const mirakana::MavgGpuCullingClusterBoundsRow>{bounds},
            .producer = mirakana::MavgGpuCullingProducer::compute_shader,
            .max_command_count = 8,
            .instance_count = 1,
            .first_instance = 0,
        });
    const auto d3d12_rows = to_d3d12_cluster_rows(cluster_rows);

    const auto dispatch =
        mirakana::rhi::d3d12::dispatch_mavg_gpu_culling_indirect(mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchDesc{
            .device =
                mirakana::rhi::d3d12::DeviceBootstrapDesc{
                    .prefer_warp = true,
                    .enable_debug_layer = false,
                },
            .cluster_rows = std::span<const mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchClusterRow>{d3d12_rows},
            .max_command_count = 8,
            .record_stride_bytes = 20,
        });

    MK_REQUIRE(!dispatch.succeeded);
    MK_REQUIRE(!dispatch.executed_gpu_culling);
}

MK_TEST("mavg gpu culling dispatch writes visible cluster indirect bytes on d3d12 warp") {
    if (!d3d12_warp_available()) {
        return;
    }

    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows(true);
    const mirakana::MavgGpuCullingIndirectDesc desc{
        .selection = &selection,
        .cluster_bounds = std::span<const mirakana::MavgGpuCullingClusterBoundsRow>{bounds},
        .producer = mirakana::MavgGpuCullingProducer::compute_shader,
        .max_command_count = 8,
        .instance_count = 2,
        .first_instance = 5,
    };
    const auto cpu_plan = mirakana::plan_mavg_gpu_culling_indirect_commands(desc);
    const auto cluster_rows = mirakana::build_mavg_gpu_culling_dispatch_cluster_rows(desc);
    const auto d3d12_rows = to_d3d12_cluster_rows(cluster_rows);
    const auto expected_argument_bytes = mirakana::encode_mavg_gpu_culling_indirect_argument_buffer_bytes(cpu_plan);
    const auto expected_count_bytes = mirakana::encode_mavg_gpu_culling_indirect_count_buffer_bytes(cpu_plan);

    const auto dispatch =
        mirakana::rhi::d3d12::dispatch_mavg_gpu_culling_indirect(mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchDesc{
            .device =
                mirakana::rhi::d3d12::DeviceBootstrapDesc{
                    .prefer_warp = true,
                    .enable_debug_layer = false,
                },
            .cluster_rows = std::span<const mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchClusterRow>{d3d12_rows},
            .max_command_count = desc.max_command_count,
            .record_stride_bytes = cpu_plan.command_layout.record_stride_bytes,
        });

    MK_REQUIRE(cpu_plan.succeeded());
    MK_REQUIRE(!cpu_plan.executed_gpu_culling);
    MK_REQUIRE(dispatch.succeeded);
    MK_REQUIRE(dispatch.executed_gpu_culling);
    MK_REQUIRE(dispatch.compute_dispatches == 1U);
    MK_REQUIRE(dispatch.resource_barriers_recorded > 0U);
    MK_REQUIRE(dispatch.visible_cluster_count == 2U);
    MK_REQUIRE(dispatch.culled_cluster_count == 0U);
    MK_REQUIRE(dispatch.count_buffer_value == cpu_plan.count_buffer_value);
    MK_REQUIRE(bytes_equal(dispatch.argument_readback_bytes, std::span<const std::uint8_t>{expected_argument_bytes}));
    MK_REQUIRE(bytes_equal(dispatch.count_readback_bytes, std::span<const std::uint8_t>{expected_count_bytes}));
}

MK_TEST("mavg gpu culling dispatch reduces count for culled clusters on d3d12 warp") {
    if (!d3d12_warp_available()) {
        return;
    }

    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows(false);
    const mirakana::MavgGpuCullingIndirectDesc desc{
        .selection = &selection,
        .cluster_bounds = std::span<const mirakana::MavgGpuCullingClusterBoundsRow>{bounds},
        .producer = mirakana::MavgGpuCullingProducer::compute_shader,
        .max_command_count = 8,
        .instance_count = 1,
        .first_instance = 0,
    };
    const auto cpu_plan = mirakana::plan_mavg_gpu_culling_indirect_commands(desc);
    const auto cluster_rows = mirakana::build_mavg_gpu_culling_dispatch_cluster_rows(desc);
    const auto d3d12_rows = to_d3d12_cluster_rows(cluster_rows);
    const auto expected_argument_bytes = mirakana::encode_mavg_gpu_culling_indirect_argument_buffer_bytes(cpu_plan);
    const auto expected_count_bytes = mirakana::encode_mavg_gpu_culling_indirect_count_buffer_bytes(cpu_plan);

    const auto dispatch =
        mirakana::rhi::d3d12::dispatch_mavg_gpu_culling_indirect(mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchDesc{
            .device =
                mirakana::rhi::d3d12::DeviceBootstrapDesc{
                    .prefer_warp = true,
                    .enable_debug_layer = false,
                },
            .cluster_rows = std::span<const mirakana::rhi::d3d12::D3d12MavgGpuCullingDispatchClusterRow>{d3d12_rows},
            .max_command_count = desc.max_command_count,
            .record_stride_bytes = cpu_plan.command_layout.record_stride_bytes,
        });

    MK_REQUIRE(cpu_plan.succeeded());
    MK_REQUIRE(cpu_plan.visible_cluster_count == 1U);
    MK_REQUIRE(cpu_plan.culled_cluster_count == 1U);
    MK_REQUIRE(cpu_plan.count_buffer_value == 1U);
    MK_REQUIRE(dispatch.succeeded);
    MK_REQUIRE(dispatch.executed_gpu_culling);
    MK_REQUIRE(dispatch.visible_cluster_count == 1U);
    MK_REQUIRE(dispatch.culled_cluster_count == 1U);
    MK_REQUIRE(dispatch.count_buffer_value == 1U);
    MK_REQUIRE(dispatch.count_buffer_value < 2U);
    MK_REQUIRE(bytes_equal(dispatch.argument_readback_bytes, std::span<const std::uint8_t>{expected_argument_bytes}));
    MK_REQUIRE(bytes_equal(dispatch.count_readback_bytes, std::span<const std::uint8_t>{expected_count_bytes}));
}

int main() {
    return mirakana::test::run_all();
}
