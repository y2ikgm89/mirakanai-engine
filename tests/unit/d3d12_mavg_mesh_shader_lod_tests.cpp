// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_mesh_shader_lod.hpp"
#include "mirakana/rhi/d3d12/d3d12_mavg_mesh_shader_lod.hpp"

#include <span>
#include <vector>

namespace {

[[nodiscard]] bool d3d12_available() noexcept {
    const auto probe = mirakana::rhi::d3d12::probe_runtime();
    return probe.windows_sdk_available && (probe.adapter_count > 0U || probe.warp_device_supported);
}

[[nodiscard]] mirakana::MavgMeshShaderLodPlan make_plan() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/d3d12-mesh-shader-lod-graph");
    return mirakana::MavgMeshShaderLodPlan{
        .task_rows =
            {
                mirakana::MavgMeshShaderLodTaskRow{
                    .graph_asset = graph_asset,
                    .cluster_index = 1,
                    .page_index = 0,
                    .lod_level = 0,
                    .material_partition = 0,
                    .material_root = mirakana::AssetId::from_name("materials/d3d12-mesh-shader/root"),
                    .meshlet_index = 11,
                    .local_meshlet_index = 0,
                    .output_vertex_count = 3,
                    .output_primitive_count = 1,
                    .group_thread_count = 1,
                    .fallback_first_index = 0,
                    .fallback_index_count = 3,
                    .fallback_vertex_base = 0,
                },
            },
        .selected_cluster_count = 1,
        .meshlet_task_count = 1,
        .fallback_draw_count = 1,
        .uses_mesh_shader_bind_points = true,
        .uses_amplification_shader_bind_point = true,
        .requires_input_assembler = false,
        .requires_index_buffer = false,
        .fallback_to_conventional_indexed_draws = true,
    };
}

[[nodiscard]] std::vector<mirakana::rhi::d3d12::D3d12MavgMeshShaderLodTaskRow>
to_d3d12_rows(std::span<const mirakana::MavgMeshShaderLodTaskRow> rows) {
    std::vector<mirakana::rhi::d3d12::D3d12MavgMeshShaderLodTaskRow> d3d12_rows;
    d3d12_rows.reserve(rows.size());
    for (const auto& row : rows) {
        d3d12_rows.push_back(mirakana::rhi::d3d12::D3d12MavgMeshShaderLodTaskRow{
            .cluster_index = row.cluster_index,
            .meshlet_index = row.meshlet_index,
            .output_vertex_count = row.output_vertex_count,
            .output_primitive_count = row.output_primitive_count,
            .group_thread_count = row.group_thread_count,
            .fallback_index_count = row.fallback_index_count,
        });
    }
    return d3d12_rows;
}

} // namespace

MK_TEST("d3d12 mavg mesh shader lod capability probe records options7 support state") {
    if (!d3d12_available()) {
        return;
    }

    const auto capability =
        mirakana::rhi::d3d12::probe_d3d12_mavg_mesh_shader_lod_capability(mirakana::rhi::d3d12::DeviceBootstrapDesc{
            .prefer_warp = true,
            .enable_debug_layer = false,
        });

    MK_REQUIRE(capability.feature_query_executed);
    MK_REQUIRE(capability.mesh_shader_tier == 0U || capability.mesh_shader_tier >= 10U);
    if (!capability.mesh_shader_supported) {
        MK_REQUIRE(!capability.diagnostic_text.empty());
    }
    MK_REQUIRE(!capability.exposed_native_handles);
    MK_REQUIRE(!capability.claimed_nanite_equivalence);
    MK_REQUIRE(!capability.claimed_metal_readiness);
}

MK_TEST("d3d12 mavg mesh shader lod execution fails closed before host work for invalid rows") {
    const auto result = mirakana::rhi::d3d12::execute_d3d12_mavg_mesh_shader_lod(
        mirakana::rhi::d3d12::D3d12MavgMeshShaderLodDispatchDesc{
            .task_rows = {},
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.diagnostic_count == 1U);
    MK_REQUIRE(result.failure_stage == 1U);
    MK_REQUIRE(!result.diagnostic_text.empty());
    MK_REQUIRE(!result.created_mesh_pipeline_state);
    MK_REQUIRE(!result.executed_mesh_shader);
    MK_REQUIRE(!result.used_input_layout);
    MK_REQUIRE(!result.used_index_buffer);
}

MK_TEST("d3d12 mavg mesh shader lod execution is host gated when tier support is unavailable") {
    if (!d3d12_available()) {
        return;
    }

    const auto plan = make_plan();
    const auto rows = to_d3d12_rows(plan.task_rows);
    const auto capability =
        mirakana::rhi::d3d12::probe_d3d12_mavg_mesh_shader_lod_capability(mirakana::rhi::d3d12::DeviceBootstrapDesc{
            .prefer_warp = true,
            .enable_debug_layer = false,
        });
    if (capability.mesh_shader_supported) {
        return;
    }

    const auto result = mirakana::rhi::d3d12::execute_d3d12_mavg_mesh_shader_lod(
        mirakana::rhi::d3d12::D3d12MavgMeshShaderLodDispatchDesc{
            .device =
                mirakana::rhi::d3d12::DeviceBootstrapDesc{
                    .prefer_warp = true,
                    .enable_debug_layer = false,
                },
            .task_rows = std::span<const mirakana::rhi::d3d12::D3d12MavgMeshShaderLodTaskRow>{rows},
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(result.host_gated);
    MK_REQUIRE(result.feature_query_executed);
    MK_REQUIRE(!result.created_mesh_pipeline_state);
    MK_REQUIRE(!result.executed_mesh_shader);
    MK_REQUIRE(!result.pipeline_statistics_available);
    MK_REQUIRE(result.pipeline_statistics_host_gated);
    MK_REQUIRE(!result.used_index_buffer);
    MK_REQUIRE(!result.used_input_layout);
    MK_REQUIRE(result.diagnostic_count > 0U);
}

MK_TEST("d3d12 mavg mesh shader lod execution renders readback only on supported hosts") {
    if (!d3d12_available()) {
        return;
    }

    const auto plan = make_plan();
    const auto rows = to_d3d12_rows(plan.task_rows);
    const auto result = mirakana::rhi::d3d12::execute_d3d12_mavg_mesh_shader_lod(
        mirakana::rhi::d3d12::D3d12MavgMeshShaderLodDispatchDesc{
            .device =
                mirakana::rhi::d3d12::DeviceBootstrapDesc{
                    .prefer_warp = false,
                    .enable_debug_layer = false,
                },
            .task_rows = std::span<const mirakana::rhi::d3d12::D3d12MavgMeshShaderLodTaskRow>{rows},
        });

    if (result.host_gated) {
        MK_REQUIRE(!result.succeeded);
        MK_REQUIRE(!result.mavg_mesh_shader_lod_d3d12_ready);
        MK_REQUIRE(!result.diagnostic_text.empty());
        return;
    }

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.created_mesh_pipeline_state);
    MK_REQUIRE(result.executed_mesh_shader);
    MK_REQUIRE(result.used_mesh_shader_bind_point);
    MK_REQUIRE(result.used_amplification_shader_bind_point);
    MK_REQUIRE(!result.used_index_buffer);
    MK_REQUIRE(!result.used_input_layout);
    MK_REQUIRE(result.dispatch_mesh_direct_calls == 1U);
    MK_REQUIRE(result.execute_indirect_mesh_dispatch_calls == 0U);
    MK_REQUIRE(result.readback_nonzero);
    MK_REQUIRE(result.readback_hash != 0U);
    MK_REQUIRE(result.mavg_mesh_shader_lod_d3d12_ready);
    MK_REQUIRE(result.amplification_shader_model == "as_6_5");
    MK_REQUIRE(result.mesh_shader_model == "ms_6_5");
    MK_REQUIRE(result.pixel_shader_model == "ps_6_0");
    MK_REQUIRE(result.diagnostic_text.empty());
    MK_REQUIRE(!result.pipeline_statistics_available);
    MK_REQUIRE(result.pipeline_statistics_host_gated);
    MK_REQUIRE(result.amplification_shader_invocations == 0U);
    MK_REQUIRE(result.mesh_shader_invocations == 0U);
    MK_REQUIRE(result.mesh_shader_primitives == 0U);
    MK_REQUIRE(!result.claimed_nanite_equivalence);
    MK_REQUIRE(!result.claimed_metal_readiness);
}

int main() {
    return mirakana::test::run_all();
}
