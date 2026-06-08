// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_gpu_culling.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgLodSelectionResult make_selection_result() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/gpu_culling_graph");
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
    const auto graph_asset = mirakana::AssetId::from_name("mavg/gpu_culling_graph");
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

[[nodiscard]] bool has_diagnostic_code(const mirakana::MavgGpuCullingIndirectPlan& plan,
                                       mirakana::MavgGpuCullingDiagnosticCode code) {
    return std::ranges::any_of(plan.diagnostics, [code](const mirakana::MavgGpuCullingDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

[[nodiscard]] bool has_sync_api(const mirakana::MavgGpuCullingIndirectPlan& plan, mirakana::MavgGpuCullingSyncApi api) {
    return std::ranges::any_of(plan.sync_requirements,
                               [api](const mirakana::MavgGpuCullingSyncRequirement& row) { return row.api == api; });
}

} // namespace

MK_TEST("mavg gpu culling indirect planning emits packed indexed command rows") {
    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows();

    const auto plan = mirakana::plan_mavg_gpu_culling_indirect_commands(mirakana::MavgGpuCullingIndirectDesc{
        .selection = &selection,
        .cluster_bounds = std::span<const mirakana::MavgGpuCullingClusterBoundsRow>{bounds},
        .producer = mirakana::MavgGpuCullingProducer::cpu_reference,
        .max_command_count = 8,
        .instance_count = 2,
        .first_instance = 5,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.prepared_gpu_culling);
    MK_REQUIRE(!plan.executed_gpu_culling);
    MK_REQUIRE(!plan.executed_indirect_draw);
    MK_REQUIRE(!plan.executed_mesh_shader);
    MK_REQUIRE(!plan.touched_native_handles);
    MK_REQUIRE(plan.selected_cluster_count == 2U);
    MK_REQUIRE(plan.visible_cluster_count == 2U);
    MK_REQUIRE(plan.culled_cluster_count == 0U);
    MK_REQUIRE(plan.command_layout.record_stride_bytes == 20U);
    MK_REQUIRE(plan.command_layout.indexed_argument_u32_count == 5U);
    MK_REQUIRE(plan.argument_buffer_size_bytes == 40U);
    MK_REQUIRE(plan.count_buffer_size_bytes == 4U);
    MK_REQUIRE(plan.count_buffer_value == 2U);
    MK_REQUIRE(plan.commands.size() == 2U);
    MK_REQUIRE(plan.commands[0].index_count_per_instance == 144U);
    MK_REQUIRE(plan.commands[0].instance_count == 2U);
    MK_REQUIRE(plan.commands[0].start_index_location == 96U);
    MK_REQUIRE(plan.commands[0].base_vertex_location == 12);
    MK_REQUIRE(plan.commands[0].start_instance_location == 5U);
    MK_REQUIRE(plan.commands[0].cluster_index == 7U);
    MK_REQUIRE(!plan.commands[0].fallback_substitution);
    MK_REQUIRE(plan.commands[1].cluster_index == 8U);
    MK_REQUIRE(plan.commands[1].fallback_substitution);
}

MK_TEST("mavg gpu culling indirect planning filters invisible cluster bounds deterministically") {
    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows(false);

    const auto plan = mirakana::plan_mavg_gpu_culling_indirect_commands(mirakana::MavgGpuCullingIndirectDesc{
        .selection = &selection,
        .cluster_bounds = std::span<const mirakana::MavgGpuCullingClusterBoundsRow>{bounds},
        .producer = mirakana::MavgGpuCullingProducer::cpu_reference,
        .max_command_count = 8,
        .instance_count = 1,
        .first_instance = 0,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.selected_cluster_count == 2U);
    MK_REQUIRE(plan.visible_cluster_count == 1U);
    MK_REQUIRE(plan.culled_cluster_count == 1U);
    MK_REQUIRE(plan.commands.size() == 1U);
    MK_REQUIRE(plan.commands[0].cluster_index == 7U);
    MK_REQUIRE(plan.count_buffer_value == 1U);
}

MK_TEST("mavg gpu culling indirect planning fails closed on invalid culling bounds") {
    const auto selection = make_selection_result();
    auto bounds = make_bounds_rows();
    bounds[0].radius = -1.0F;

    const auto plan = mirakana::plan_mavg_gpu_culling_indirect_commands(mirakana::MavgGpuCullingIndirectDesc{
        .selection = &selection,
        .cluster_bounds = std::span<const mirakana::MavgGpuCullingClusterBoundsRow>{bounds},
        .producer = mirakana::MavgGpuCullingProducer::cpu_reference,
        .max_command_count = 8,
        .instance_count = 1,
        .first_instance = 0,
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.commands.empty());
    MK_REQUIRE(has_diagnostic_code(plan, mirakana::MavgGpuCullingDiagnosticCode::invalid_culling_bounds));
}

MK_TEST("mavg gpu culling indirect planning fails closed when command budget is exceeded") {
    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows();

    const auto plan = mirakana::plan_mavg_gpu_culling_indirect_commands(mirakana::MavgGpuCullingIndirectDesc{
        .selection = &selection,
        .cluster_bounds = std::span<const mirakana::MavgGpuCullingClusterBoundsRow>{bounds},
        .producer = mirakana::MavgGpuCullingProducer::cpu_reference,
        .max_command_count = 1,
        .instance_count = 1,
        .first_instance = 0,
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.commands.empty());
    MK_REQUIRE(has_diagnostic_code(plan, mirakana::MavgGpuCullingDiagnosticCode::max_command_count_exceeded));
}

MK_TEST("mavg gpu culling indirect planning records backend sync requirements for compute produced commands") {
    const auto selection = make_selection_result();
    const auto bounds = make_bounds_rows();

    const auto plan = mirakana::plan_mavg_gpu_culling_indirect_commands(mirakana::MavgGpuCullingIndirectDesc{
        .selection = &selection,
        .cluster_bounds = std::span<const mirakana::MavgGpuCullingClusterBoundsRow>{bounds},
        .producer = mirakana::MavgGpuCullingProducer::compute_shader,
        .max_command_count = 8,
        .instance_count = 1,
        .first_instance = 0,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(has_sync_api(plan, mirakana::MavgGpuCullingSyncApi::d3d12));
    MK_REQUIRE(has_sync_api(plan, mirakana::MavgGpuCullingSyncApi::vulkan));
    MK_REQUIRE(plan.sync_requirements.size() == 2U);
    MK_REQUIRE(plan.sync_requirements[0].argument_buffer_offset_alignment_bytes == 4U);
    MK_REQUIRE(plan.sync_requirements[0].count_buffer_offset_alignment_bytes == 4U);
}

int main() {
    return mirakana::test::run_all();
}
