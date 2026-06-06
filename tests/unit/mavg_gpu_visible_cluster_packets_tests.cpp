// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_gpu_visible_cluster_packets.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgGpuClusterFormatPlan make_cluster_format_plan() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/visible-cluster-packets");
    return mirakana::MavgGpuClusterFormatPlan{
        .rows =
            {
                mirakana::MavgGpuClusterFormatRow{
                    .graph_asset = graph_asset,
                    .cluster_index = 7,
                    .page_index = 2,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .material_partition = 3,
                    .parent_cluster_index = 1,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 1,
                    .first_index = 96,
                    .index_count = 144,
                    .vertex_base = 12,
                    .triangle_count = 48,
                    .vertex_count = 72,
                    .payload_page_byte_offset = 0,
                    .payload_page_byte_size = 128,
                    .bounds_center = {.x = -1.0F, .y = 0.0F, .z = 4.0F},
                    .bounds_radius = 2.0F,
                    .meshlet_ready = true,
                },
                mirakana::MavgGpuClusterFormatRow{
                    .graph_asset = graph_asset,
                    .cluster_index = 8,
                    .page_index = 3,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .material_partition = 3,
                    .parent_cluster_index = 1,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 1,
                    .first_index = 240,
                    .index_count = 96,
                    .vertex_base = 44,
                    .triangle_count = 32,
                    .vertex_count = 48,
                    .payload_page_byte_offset = 128,
                    .payload_page_byte_size = 64,
                    .bounds_center = {.x = 2.0F, .y = 0.0F, .z = 4.0F},
                    .bounds_radius = 2.0F,
                    .meshlet_ready = true,
                },
            },
        .cluster_row_count = 2,
        .meshlet_ready_cluster_count = 2,
        .vertex_stride_bytes = 32,
        .index_stride_bytes = 4,
        .index_format_uint32 = true,
    };
}

[[nodiscard]] mirakana::MavgGpuCullingIndirectPlan make_visible_culling_plan() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/visible-cluster-packets");
    return mirakana::MavgGpuCullingIndirectPlan{
        .commands =
            {
                mirakana::MavgGpuCullingIndirectCommand{
                    .index_count_per_instance = 96,
                    .instance_count = 2,
                    .start_index_location = 240,
                    .base_vertex_location = 44,
                    .start_instance_location = 5,
                    .graph_asset = graph_asset,
                    .cluster_index = 8,
                    .page_index = 3,
                    .lod_level = 1,
                    .material_partition = 3,
                    .fallback_substitution = true,
                },
                mirakana::MavgGpuCullingIndirectCommand{
                    .index_count_per_instance = 144,
                    .instance_count = 2,
                    .start_index_location = 96,
                    .base_vertex_location = 12,
                    .start_instance_location = 5,
                    .graph_asset = graph_asset,
                    .cluster_index = 7,
                    .page_index = 2,
                    .lod_level = 1,
                    .material_partition = 3,
                    .fallback_substitution = false,
                },
            },
        .argument_buffer_size_bytes = 40,
        .count_buffer_size_bytes = 4,
        .count_buffer_value = 2,
        .selected_cluster_count = 2,
        .visible_cluster_count = 2,
        .culled_cluster_count = 0,
        .prepared_gpu_culling = true,
    };
}

[[nodiscard]] bool has_diagnostic(const mirakana::MavgGpuVisibleClusterPacketPlan& plan,
                                  mirakana::MavgGpuVisibleClusterPacketDiagnosticCode code) {
    return std::ranges::any_of(
        plan.diagnostics,
        [code](const mirakana::MavgGpuVisibleClusterPacketDiagnostic& diagnostic) { return diagnostic.code == code; });
}

} // namespace

MK_TEST("mavg gpu visible cluster packets preserve visible command order and join cluster format rows") {
    const auto format = make_cluster_format_plan();
    const auto culling = make_visible_culling_plan();

    const auto plan = mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
        .cluster_format_plan = &format,
        .culling_plan = &culling,
        .max_packet_count = 8,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.source_cluster_row_count == 2U);
    MK_REQUIRE(plan.source_visible_command_count == 2U);
    MK_REQUIRE(plan.packet_count == 2U);
    MK_REQUIRE(plan.meshlet_ready_packet_count == 2U);
    MK_REQUIRE(plan.fallback_packet_count == 1U);
    MK_REQUIRE(plan.payload_byte_span_offset == 0U);
    MK_REQUIRE(plan.payload_byte_span_size == 192U);
    MK_REQUIRE(!plan.allocated_rhi_resources);
    MK_REQUIRE(!plan.submitted_gpu_work);
    MK_REQUIRE(!plan.executed_gpu_traversal);
    MK_REQUIRE(!plan.wrote_gpu_visibility_buffer);
    MK_REQUIRE(!plan.executed_mesh_shader);
    MK_REQUIRE(!plan.executed_indirect_draw);
    MK_REQUIRE(!plan.used_gpu_decompression);
    MK_REQUIRE(!plan.touched_native_handles);
    MK_REQUIRE(!plan.claimed_nanite_compatibility);
    MK_REQUIRE(plan.packets.size() == 2U);
    MK_REQUIRE(plan.packets[0].packet_index == 0U);
    MK_REQUIRE(plan.packets[0].indirect_command_index == 0U);
    MK_REQUIRE(plan.packets[0].cluster_index == 8U);
    MK_REQUIRE(plan.packets[0].page_index == 3U);
    MK_REQUIRE(plan.packets[0].lod_level == 1U);
    MK_REQUIRE(plan.packets[0].material_partition == 3U);
    MK_REQUIRE(plan.packets[0].parent_cluster_index == 1U);
    MK_REQUIRE(plan.packets[0].has_parent);
    MK_REQUIRE(plan.packets[0].resident_fallback_cluster_index == 1U);
    MK_REQUIRE(plan.packets[0].fallback_substitution);
    MK_REQUIRE(plan.packets[0].first_index == 240U);
    MK_REQUIRE(plan.packets[0].index_count == 96U);
    MK_REQUIRE(plan.packets[0].vertex_base == 44);
    MK_REQUIRE(plan.packets[0].triangle_count == 32U);
    MK_REQUIRE(plan.packets[0].vertex_count == 48U);
    MK_REQUIRE(plan.packets[0].payload_page_byte_offset == 128U);
    MK_REQUIRE(plan.packets[0].payload_page_byte_size == 64U);
    MK_REQUIRE(plan.packets[0].bounds_center.x == 2.0F);
    MK_REQUIRE(plan.packets[0].bounds_radius == 2.0F);
    MK_REQUIRE(plan.packets[0].meshlet_ready);
    MK_REQUIRE(plan.packets[0].index_count_per_instance == 96U);
    MK_REQUIRE(plan.packets[0].instance_count == 2U);
    MK_REQUIRE(plan.packets[0].start_index_location == 240U);
    MK_REQUIRE(plan.packets[0].base_vertex_location == 44);
    MK_REQUIRE(plan.packets[0].start_instance_location == 5U);
    MK_REQUIRE(plan.packets[1].cluster_index == 7U);
    MK_REQUIRE(!plan.packets[1].fallback_substitution);
}

MK_TEST("mavg gpu visible cluster packets fail closed on missing or invalid source plans") {
    const auto format = make_cluster_format_plan();
    const auto culling = make_visible_culling_plan();

    const auto missing_format =
        mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
            .cluster_format_plan = nullptr,
            .culling_plan = &culling,
            .max_packet_count = 8,
        });
    MK_REQUIRE(!missing_format.succeeded());
    MK_REQUIRE(missing_format.packets.empty());
    MK_REQUIRE(has_diagnostic(missing_format,
                              mirakana::MavgGpuVisibleClusterPacketDiagnosticCode::missing_cluster_format_plan));

    auto invalid_format = format;
    invalid_format.diagnostics.push_back(mirakana::MavgGpuClusterFormatDiagnostic{});
    const auto format_failure =
        mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
            .cluster_format_plan = &invalid_format,
            .culling_plan = &culling,
            .max_packet_count = 8,
        });
    MK_REQUIRE(!format_failure.succeeded());
    MK_REQUIRE(has_diagnostic(format_failure,
                              mirakana::MavgGpuVisibleClusterPacketDiagnosticCode::invalid_cluster_format_plan));

    const auto missing_culling =
        mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
            .cluster_format_plan = &format,
            .culling_plan = nullptr,
            .max_packet_count = 8,
        });
    MK_REQUIRE(!missing_culling.succeeded());
    MK_REQUIRE(
        has_diagnostic(missing_culling, mirakana::MavgGpuVisibleClusterPacketDiagnosticCode::missing_culling_plan));

    auto invalid_culling = culling;
    invalid_culling.diagnostics.push_back(mirakana::MavgGpuCullingDiagnostic{});
    const auto culling_failure =
        mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
            .cluster_format_plan = &format,
            .culling_plan = &invalid_culling,
            .max_packet_count = 8,
        });
    MK_REQUIRE(!culling_failure.succeeded());
    MK_REQUIRE(
        has_diagnostic(culling_failure, mirakana::MavgGpuVisibleClusterPacketDiagnosticCode::invalid_culling_plan));
}

MK_TEST("mavg gpu visible cluster packets fail closed on duplicate missing or stale format rows") {
    const auto culling = make_visible_culling_plan();

    auto duplicate_format = make_cluster_format_plan();
    duplicate_format.rows.push_back(duplicate_format.rows[0]);
    const auto duplicate_failure =
        mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
            .cluster_format_plan = &duplicate_format,
            .culling_plan = &culling,
            .max_packet_count = 8,
        });
    MK_REQUIRE(!duplicate_failure.succeeded());
    MK_REQUIRE(has_diagnostic(duplicate_failure,
                              mirakana::MavgGpuVisibleClusterPacketDiagnosticCode::duplicate_cluster_format_row));

    auto missing_format = make_cluster_format_plan();
    missing_format.rows.pop_back();
    const auto missing_failure =
        mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
            .cluster_format_plan = &missing_format,
            .culling_plan = &culling,
            .max_packet_count = 8,
        });
    MK_REQUIRE(!missing_failure.succeeded());
    MK_REQUIRE(has_diagnostic(missing_failure,
                              mirakana::MavgGpuVisibleClusterPacketDiagnosticCode::missing_cluster_format_row));

    auto stale_culling = culling;
    stale_culling.commands[0].start_index_location = 999;
    const auto stale_format = make_cluster_format_plan();
    const auto stale_failure =
        mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
            .cluster_format_plan = &stale_format,
            .culling_plan = &stale_culling,
            .max_packet_count = 8,
        });
    MK_REQUIRE(!stale_failure.succeeded());
    MK_REQUIRE(
        has_diagnostic(stale_failure, mirakana::MavgGpuVisibleClusterPacketDiagnosticCode::stale_visible_command));
}

MK_TEST("mavg gpu visible cluster packets enforce meshlet readiness and packet budgets") {
    auto format = make_cluster_format_plan();
    format.rows[1].meshlet_ready = false;
    const auto culling = make_visible_culling_plan();

    const auto meshlet_failure =
        mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
            .cluster_format_plan = &format,
            .culling_plan = &culling,
            .max_packet_count = 8,
            .require_meshlet_ready_packets = true,
        });
    MK_REQUIRE(!meshlet_failure.succeeded());
    MK_REQUIRE(has_diagnostic(meshlet_failure, mirakana::MavgGpuVisibleClusterPacketDiagnosticCode::meshlet_not_ready));

    const auto budget_failure =
        mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
            .cluster_format_plan = &format,
            .culling_plan = &culling,
            .max_packet_count = 1,
        });
    MK_REQUIRE(!budget_failure.succeeded());
    MK_REQUIRE(
        has_diagnostic(budget_failure, mirakana::MavgGpuVisibleClusterPacketDiagnosticCode::max_packet_count_exceeded));
}

MK_TEST("mavg gpu visible cluster packets allow zero visible commands and reject count mismatches") {
    const auto format = make_cluster_format_plan();

    auto zero_culling = make_visible_culling_plan();
    zero_culling.commands.clear();
    zero_culling.visible_cluster_count = 0;
    zero_culling.count_buffer_value = 0;
    zero_culling.argument_buffer_size_bytes = 0;
    zero_culling.culled_cluster_count = 2;
    const auto zero_plan = mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
        .cluster_format_plan = &format,
        .culling_plan = &zero_culling,
        .max_packet_count = 8,
    });
    MK_REQUIRE(zero_plan.succeeded());
    MK_REQUIRE(zero_plan.packets.empty());
    MK_REQUIRE(zero_plan.packet_count == 0U);
    MK_REQUIRE(zero_plan.source_visible_command_count == 0U);
    MK_REQUIRE(!zero_plan.wrote_gpu_visibility_buffer);

    auto mismatch_culling = make_visible_culling_plan();
    mismatch_culling.count_buffer_value = 1;
    const auto mismatch_failure =
        mirakana::plan_mavg_gpu_visible_cluster_packets(mirakana::MavgGpuVisibleClusterPacketDesc{
            .cluster_format_plan = &format,
            .culling_plan = &mismatch_culling,
            .max_packet_count = 8,
        });
    MK_REQUIRE(!mismatch_failure.succeeded());
    MK_REQUIRE(has_diagnostic(mismatch_failure,
                              mirakana::MavgGpuVisibleClusterPacketDiagnosticCode::visible_command_count_mismatch));
}

int main() {
    return mirakana::test::run_all();
}
