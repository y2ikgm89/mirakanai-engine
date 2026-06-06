// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_gpu_visibility_buffer_layout.hpp"

#include <algorithm>
#include <cstdint>

namespace {

[[nodiscard]] mirakana::MavgGpuVisibleClusterPacketPlan make_packet_plan() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/visibility-buffer-layout");
    return mirakana::MavgGpuVisibleClusterPacketPlan{
        .packets =
            {
                mirakana::MavgGpuVisibleClusterPacketRow{
                    .graph_asset = graph_asset,
                    .packet_index = 0,
                    .indirect_command_index = 3,
                    .cluster_index = 9,
                    .page_index = 4,
                    .local_cluster_index = 1,
                    .lod_level = 2,
                    .material_partition = 5,
                    .parent_cluster_index = 2,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 2,
                    .fallback_substitution = false,
                    .first_index = 96,
                    .index_count = 144,
                    .vertex_base = 12,
                    .triangle_count = 48,
                    .vertex_count = 72,
                    .payload_page_byte_offset = 256,
                    .payload_page_byte_size = 128,
                    .bounds_center = {.x = -1.0F, .y = 0.0F, .z = 4.0F},
                    .bounds_radius = 2.0F,
                    .meshlet_ready = true,
                    .index_count_per_instance = 144,
                    .instance_count = 1,
                    .start_index_location = 96,
                    .base_vertex_location = 12,
                    .start_instance_location = 0,
                },
                mirakana::MavgGpuVisibleClusterPacketRow{
                    .graph_asset = graph_asset,
                    .packet_index = 1,
                    .indirect_command_index = 4,
                    .cluster_index = 10,
                    .page_index = 5,
                    .local_cluster_index = 2,
                    .lod_level = 2,
                    .material_partition = 5,
                    .parent_cluster_index = 2,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 9,
                    .fallback_substitution = true,
                    .first_index = 240,
                    .index_count = 96,
                    .vertex_base = 44,
                    .triangle_count = 32,
                    .vertex_count = 48,
                    .payload_page_byte_offset = 384,
                    .payload_page_byte_size = 64,
                    .bounds_center = {.x = 2.0F, .y = 0.0F, .z = 4.0F},
                    .bounds_radius = 2.0F,
                    .meshlet_ready = false,
                    .index_count_per_instance = 96,
                    .instance_count = 1,
                    .start_index_location = 240,
                    .base_vertex_location = 44,
                    .start_instance_location = 0,
                },
            },
        .source_cluster_row_count = 2,
        .source_visible_command_count = 2,
        .packet_count = 2,
        .meshlet_ready_packet_count = 1,
        .fallback_packet_count = 1,
        .payload_byte_span_offset = 256,
        .payload_byte_span_size = 192,
    };
}

[[nodiscard]] bool has_diagnostic(const mirakana::MavgGpuVisibilityBufferLayoutPlan& plan,
                                  mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode code) {
    return std::ranges::any_of(plan.diagnostics,
                               [code](const mirakana::MavgGpuVisibilityBufferLayoutDiagnostic& diagnostic) {
                                   return diagnostic.code == code;
                               });
}

} // namespace

MK_TEST("mavg gpu visibility buffer layout derives deterministic slot rows from packet rows") {
    const auto packet_plan = make_packet_plan();

    const auto plan = mirakana::plan_mavg_gpu_visibility_buffer_layout(mirakana::MavgGpuVisibilityBufferLayoutDesc{
        .packet_plan = &packet_plan,
        .max_slot_count = 8,
        .slot_record_stride_bytes = 64,
        .slot_record_alignment_bytes = 16,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.source_packet_count == 2U);
    MK_REQUIRE(plan.slot_count == 2U);
    MK_REQUIRE(plan.meshlet_ready_slot_count == 1U);
    MK_REQUIRE(plan.fallback_slot_count == 1U);
    MK_REQUIRE(plan.byte_range_count == 2U);
    MK_REQUIRE(plan.layout_row_count == 1U);
    MK_REQUIRE(plan.sync_intent_row_count == 1U);
    MK_REQUIRE(plan.slot_record_stride_bytes == 64U);
    MK_REQUIRE(plan.slot_record_alignment_bytes == 16U);
    MK_REQUIRE(plan.slot_buffer_size_bytes == 128U);
    MK_REQUIRE(plan.payload_byte_span_offset == 256U);
    MK_REQUIRE(plan.payload_byte_span_size == 192U);
    MK_REQUIRE(!plan.allocated_rhi_resources);
    MK_REQUIRE(!plan.wrote_gpu_visibility_buffer);
    MK_REQUIRE(!plan.submitted_gpu_work);
    MK_REQUIRE(!plan.executed_gpu_traversal);
    MK_REQUIRE(!plan.executed_mesh_shader);
    MK_REQUIRE(!plan.executed_indirect_draw);
    MK_REQUIRE(!plan.used_gpu_decompression);
    MK_REQUIRE(!plan.touched_native_handles);
    MK_REQUIRE(!plan.claimed_vulkan_parity);
    MK_REQUIRE(!plan.claimed_metal_parity);
    MK_REQUIRE(!plan.claimed_nanite_compatibility);
    MK_REQUIRE(plan.slots.size() == 2U);
    MK_REQUIRE(plan.byte_ranges.size() == 2U);
    MK_REQUIRE(plan.layout_rows.size() == 1U);
    MK_REQUIRE(plan.sync_intents.size() == 1U);
    MK_REQUIRE(plan.layout_rows[0].slot_record_stride_bytes == 64U);
    MK_REQUIRE(plan.layout_rows[0].slot_record_alignment_bytes == 16U);
    MK_REQUIRE(plan.layout_rows[0].cluster_key_field_offset_bytes == 8U);
    MK_REQUIRE(plan.sync_intents[0].producer == mirakana::MavgGpuVisibilityBufferSyncProducer::visibility_buffer_write);
    MK_REQUIRE(plan.sync_intents[0].consumer == mirakana::MavgGpuVisibilityBufferSyncConsumer::visibility_resolve_read);
    MK_REQUIRE(plan.sync_intents[0].backend_neutral);
    MK_REQUIRE(plan.sync_intents[0].requires_write_completion_before_read);
    MK_REQUIRE(plan.slots[0].slot_index == 0U);
    MK_REQUIRE(plan.slots[0].packet_index == 0U);
    MK_REQUIRE(plan.slots[0].indirect_command_index == 3U);
    MK_REQUIRE(plan.slots[0].cluster_index == 9U);
    MK_REQUIRE(plan.slots[0].page_index == 4U);
    MK_REQUIRE(plan.slots[0].lod_level == 2U);
    MK_REQUIRE(plan.slots[0].material_partition == 5U);
    MK_REQUIRE(!plan.slots[0].fallback_substitution);
    MK_REQUIRE(plan.slots[0].meshlet_ready);
    MK_REQUIRE(plan.slots[0].payload_page_byte_offset == 256U);
    MK_REQUIRE(plan.slots[0].payload_page_byte_size == 128U);
    MK_REQUIRE(plan.slots[0].slot_byte_offset == 0U);
    MK_REQUIRE(plan.slots[0].slot_byte_size == 64U);
    MK_REQUIRE(plan.slots[0].cluster_key != 0U);
    MK_REQUIRE(plan.byte_ranges[0].slot_index == 0U);
    MK_REQUIRE(plan.byte_ranges[0].byte_offset == 0U);
    MK_REQUIRE(plan.byte_ranges[0].byte_size == 64U);
    MK_REQUIRE(plan.slots[1].slot_index == 1U);
    MK_REQUIRE(plan.slots[1].packet_index == 1U);
    MK_REQUIRE(plan.slots[1].cluster_index == 10U);
    MK_REQUIRE(plan.slots[1].fallback_substitution);
    MK_REQUIRE(!plan.slots[1].meshlet_ready);
    MK_REQUIRE(plan.slots[1].slot_byte_offset == 64U);
    MK_REQUIRE(plan.slots[1].cluster_key != plan.slots[0].cluster_key);
}

MK_TEST("mavg gpu visibility buffer layout fails closed on missing invalid or inconsistent packet plans") {
    const auto missing = mirakana::plan_mavg_gpu_visibility_buffer_layout(
        mirakana::MavgGpuVisibilityBufferLayoutDesc{.packet_plan = nullptr});
    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.slots.empty());
    MK_REQUIRE(has_diagnostic(
        missing, mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode::missing_visible_cluster_packet_plan));

    auto invalid_packet_plan = make_packet_plan();
    invalid_packet_plan.diagnostics.push_back(mirakana::MavgGpuVisibleClusterPacketDiagnostic{});
    const auto invalid = mirakana::plan_mavg_gpu_visibility_buffer_layout(
        mirakana::MavgGpuVisibilityBufferLayoutDesc{.packet_plan = &invalid_packet_plan});
    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(has_diagnostic(
        invalid, mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode::invalid_visible_cluster_packet_plan));

    auto mismatched_count = make_packet_plan();
    mismatched_count.packet_count = 3;
    const auto count_failure = mirakana::plan_mavg_gpu_visibility_buffer_layout(
        mirakana::MavgGpuVisibilityBufferLayoutDesc{.packet_plan = &mismatched_count});
    MK_REQUIRE(!count_failure.succeeded());
    MK_REQUIRE(has_diagnostic(count_failure,
                              mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode::source_packet_count_mismatch));
}

MK_TEST("mavg gpu visibility buffer layout rejects duplicate or stale packet indexes") {
    auto duplicate = make_packet_plan();
    duplicate.packets[1].packet_index = 0;
    const auto duplicate_failure = mirakana::plan_mavg_gpu_visibility_buffer_layout(
        mirakana::MavgGpuVisibilityBufferLayoutDesc{.packet_plan = &duplicate});
    MK_REQUIRE(!duplicate_failure.succeeded());
    MK_REQUIRE(has_diagnostic(duplicate_failure,
                              mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode::duplicate_packet_index));

    auto stale = make_packet_plan();
    stale.packets[0].packet_index = 4;
    const auto stale_failure = mirakana::plan_mavg_gpu_visibility_buffer_layout(
        mirakana::MavgGpuVisibilityBufferLayoutDesc{.packet_plan = &stale});
    MK_REQUIRE(!stale_failure.succeeded());
    MK_REQUIRE(
        has_diagnostic(stale_failure, mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode::non_dense_packet_index));
}

MK_TEST("mavg gpu visibility buffer layout validates stride alignment meshlet and budget gates") {
    const auto packet_plan = make_packet_plan();

    const auto bad_stride =
        mirakana::plan_mavg_gpu_visibility_buffer_layout(mirakana::MavgGpuVisibilityBufferLayoutDesc{
            .packet_plan = &packet_plan,
            .slot_record_stride_bytes =
                mirakana::MavgGpuVisibilityBufferLayoutDesc::minimum_slot_record_stride_bytes - 1U,
        });
    MK_REQUIRE(!bad_stride.succeeded());
    MK_REQUIRE(has_diagnostic(bad_stride, mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode::invalid_slot_stride));

    const auto bad_alignment =
        mirakana::plan_mavg_gpu_visibility_buffer_layout(mirakana::MavgGpuVisibilityBufferLayoutDesc{
            .packet_plan = &packet_plan,
            .slot_record_alignment_bytes = 24,
        });
    MK_REQUIRE(!bad_alignment.succeeded());
    MK_REQUIRE(
        has_diagnostic(bad_alignment, mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode::invalid_slot_alignment));

    const auto meshlet_failure =
        mirakana::plan_mavg_gpu_visibility_buffer_layout(mirakana::MavgGpuVisibilityBufferLayoutDesc{
            .packet_plan = &packet_plan,
            .require_meshlet_ready_slots = true,
        });
    MK_REQUIRE(!meshlet_failure.succeeded());
    MK_REQUIRE(
        has_diagnostic(meshlet_failure, mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode::meshlet_not_ready));

    const auto budget_failure =
        mirakana::plan_mavg_gpu_visibility_buffer_layout(mirakana::MavgGpuVisibilityBufferLayoutDesc{
            .packet_plan = &packet_plan,
            .max_slot_count = 1,
        });
    MK_REQUIRE(!budget_failure.succeeded());
    MK_REQUIRE(
        has_diagnostic(budget_failure, mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode::max_slot_count_exceeded));
}

MK_TEST("mavg gpu visibility buffer layout allows zero packets") {
    auto packet_plan = make_packet_plan();
    packet_plan.packets.clear();
    packet_plan.packet_count = 0;
    packet_plan.meshlet_ready_packet_count = 0;
    packet_plan.fallback_packet_count = 0;
    packet_plan.payload_byte_span_offset = 0;
    packet_plan.payload_byte_span_size = 0;

    const auto plan = mirakana::plan_mavg_gpu_visibility_buffer_layout(
        mirakana::MavgGpuVisibilityBufferLayoutDesc{.packet_plan = &packet_plan});

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.slots.empty());
    MK_REQUIRE(plan.source_packet_count == 0U);
    MK_REQUIRE(plan.slot_count == 0U);
    MK_REQUIRE(plan.slot_buffer_size_bytes == 0U);
    MK_REQUIRE(!plan.wrote_gpu_visibility_buffer);
}

int main() {
    return mirakana::test::run_all();
}
