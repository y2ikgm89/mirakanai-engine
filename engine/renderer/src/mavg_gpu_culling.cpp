// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/mavg_gpu_culling.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace mirakana {
namespace {

constexpr std::uint32_t indexed_indirect_argument_u32_count = 5;
constexpr std::uint32_t indexed_indirect_record_stride_bytes = indexed_indirect_argument_u32_count * 4U;
constexpr std::uint32_t indirect_count_buffer_size_bytes = 4;
constexpr std::uint32_t indirect_buffer_offset_alignment_bytes = 4;

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void add_diagnostic(MavgGpuCullingIndirectPlan& plan, MavgGpuCullingDiagnosticCode code, AssetId graph_asset,
                    std::uint32_t cluster_index, std::string message) {
    plan.diagnostics.push_back(MavgGpuCullingDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .cluster_index = cluster_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool same_cluster(const MavgGpuCullingClusterBoundsRow& bounds,
                                const MavgLodSelectedCluster& selected) noexcept {
    return bounds.graph_asset.value == selected.graph_asset.value && bounds.cluster_index == selected.cluster_index;
}

[[nodiscard]] std::size_t matching_bounds_count(std::span<const MavgGpuCullingClusterBoundsRow> bounds_rows,
                                                const MavgLodSelectedCluster& selected) noexcept {
    std::size_t count = 0;
    for (const auto& bounds : bounds_rows) {
        if (same_cluster(bounds, selected)) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] const MavgGpuCullingClusterBoundsRow*
find_bounds(std::span<const MavgGpuCullingClusterBoundsRow> bounds_rows,
            const MavgLodSelectedCluster& selected) noexcept {
    for (const auto& bounds : bounds_rows) {
        if (same_cluster(bounds, selected)) {
            return &bounds;
        }
    }
    return nullptr;
}

[[nodiscard]] bool valid_bounds(const MavgGpuCullingClusterBoundsRow& bounds) noexcept {
    return finite_vec3(bounds.center) && std::isfinite(bounds.radius) && bounds.radius >= 0.0F;
}

[[nodiscard]] bool valid_draw_range(const MavgLodSelectedCluster& selected) noexcept {
    return selected.index_count > 0U &&
           selected.first_index <= (std::numeric_limits<std::uint32_t>::max() - selected.index_count);
}

void append_compute_sync_requirements(MavgGpuCullingIndirectPlan& plan) {
    plan.sync_requirements.push_back(MavgGpuCullingSyncRequirement{
        .api = MavgGpuCullingSyncApi::d3d12,
        .producer_stage = "compute_shader",
        .producer_access = "unordered_access_write",
        .consumer_stage = "execute_indirect",
        .consumer_access = "indirect_argument_read",
        .argument_buffer_required_state_or_usage = "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        .count_buffer_required_state_or_usage = "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT",
        .argument_buffer_offset_alignment_bytes = indirect_buffer_offset_alignment_bytes,
        .count_buffer_offset_alignment_bytes = indirect_buffer_offset_alignment_bytes,
    });
    plan.sync_requirements.push_back(MavgGpuCullingSyncRequirement{
        .api = MavgGpuCullingSyncApi::vulkan,
        .producer_stage = "VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT",
        .producer_access = "VK_ACCESS_2_SHADER_WRITE_BIT",
        .consumer_stage = "VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT",
        .consumer_access = "VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT",
        .argument_buffer_required_state_or_usage =
            "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT",
        .count_buffer_required_state_or_usage =
            "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT",
        .argument_buffer_offset_alignment_bytes = indirect_buffer_offset_alignment_bytes,
        .count_buffer_offset_alignment_bytes = indirect_buffer_offset_alignment_bytes,
    });
}

void fail_closed(MavgGpuCullingIndirectPlan& plan) {
    plan.commands.clear();
    plan.sync_requirements.clear();
    plan.argument_buffer_size_bytes = 0;
    plan.count_buffer_value = 0;
    plan.visible_cluster_count = 0;
    plan.culled_cluster_count = 0;
}

} // namespace

bool MavgGpuCullingIndirectPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

MavgGpuCullingIndirectPlan plan_mavg_gpu_culling_indirect_commands(const MavgGpuCullingIndirectDesc& desc) {
    MavgGpuCullingIndirectPlan plan;
    plan.command_layout = MavgGpuCullingIndirectCommandLayout{
        .record_stride_bytes = indexed_indirect_record_stride_bytes,
        .indexed_argument_u32_count = indexed_indirect_argument_u32_count,
        .argument_buffer_offset_alignment_bytes = indirect_buffer_offset_alignment_bytes,
        .count_buffer_offset_alignment_bytes = indirect_buffer_offset_alignment_bytes,
    };
    plan.count_buffer_size_bytes = indirect_count_buffer_size_bytes;

    if (desc.selection == nullptr) {
        add_diagnostic(plan, MavgGpuCullingDiagnosticCode::invalid_selection, AssetId{}, 0,
                       "MAVG GPU culling indirect planning requires a selection result");
        return plan;
    }
    plan.selected_cluster_count = static_cast<std::uint32_t>(desc.selection->selected_clusters.size());

    if (!desc.selection->succeeded()) {
        add_diagnostic(plan, MavgGpuCullingDiagnosticCode::invalid_selection, AssetId{}, 0,
                       "MAVG GPU culling indirect planning requires a successful selection result");
    }
    if (desc.selection->selected_clusters.empty()) {
        add_diagnostic(plan, MavgGpuCullingDiagnosticCode::no_selected_clusters, AssetId{}, 0,
                       "MAVG GPU culling indirect planning requires at least one selected cluster");
    }
    if (desc.instance_count == 0U) {
        add_diagnostic(plan, MavgGpuCullingDiagnosticCode::invalid_instance_count, AssetId{}, 0,
                       "MAVG GPU culling indirect commands require a non-zero instance_count");
    }
    if (desc.max_command_count == 0U) {
        add_diagnostic(plan, MavgGpuCullingDiagnosticCode::invalid_max_command_count, AssetId{}, 0,
                       "MAVG GPU culling indirect planning requires a non-zero max_command_count");
    }

    for (const auto& selected : desc.selection->selected_clusters) {
        const auto bounds_count = matching_bounds_count(desc.cluster_bounds, selected);
        if (desc.require_culling_bounds_for_selected_clusters && bounds_count == 0U) {
            add_diagnostic(plan, MavgGpuCullingDiagnosticCode::missing_culling_bounds, selected.graph_asset,
                           selected.cluster_index, "selected MAVG cluster is missing a culling bounds row");
            continue;
        }
        if (bounds_count > 1U) {
            add_diagnostic(plan, MavgGpuCullingDiagnosticCode::duplicate_culling_bounds, selected.graph_asset,
                           selected.cluster_index, "selected MAVG cluster has duplicate culling bounds rows");
            continue;
        }
        const auto* bounds = find_bounds(desc.cluster_bounds, selected);
        if (bounds != nullptr && !valid_bounds(*bounds)) {
            add_diagnostic(plan, MavgGpuCullingDiagnosticCode::invalid_culling_bounds, selected.graph_asset,
                           selected.cluster_index,
                           "selected MAVG cluster culling bounds must be finite and non-negative");
        }
        if (!valid_draw_range(selected)) {
            add_diagnostic(
                plan, MavgGpuCullingDiagnosticCode::invalid_cluster_draw_range, selected.graph_asset,
                selected.cluster_index,
                "selected MAVG cluster draw range must have a non-zero index_count and stay in uint32 bounds");
        }
    }

    if (!plan.diagnostics.empty()) {
        fail_closed(plan);
        return plan;
    }

    plan.prepared_gpu_culling = true;
    for (const auto& selected : desc.selection->selected_clusters) {
        const auto* bounds = find_bounds(desc.cluster_bounds, selected);
        if (bounds != nullptr && !bounds->visible) {
            ++plan.culled_cluster_count;
            continue;
        }

        ++plan.visible_cluster_count;
        plan.commands.push_back(MavgGpuCullingIndirectCommand{
            .index_count_per_instance = selected.index_count,
            .instance_count = desc.instance_count,
            .start_index_location = selected.first_index,
            .base_vertex_location = selected.vertex_base,
            .start_instance_location = desc.first_instance,
            .graph_asset = selected.graph_asset,
            .cluster_index = selected.cluster_index,
            .page_index = selected.page_index,
            .lod_level = selected.lod_level,
            .material_partition = selected.material_partition,
            .fallback_substitution = selected.fallback_substitution,
        });
    }

    if (plan.commands.size() > desc.max_command_count) {
        add_diagnostic(plan, MavgGpuCullingDiagnosticCode::max_command_count_exceeded, AssetId{},
                       static_cast<std::uint32_t>(plan.commands.size()),
                       "MAVG GPU culling indirect command count exceeds max_command_count");
        fail_closed(plan);
        return plan;
    }
    if (plan.commands.size() > (std::numeric_limits<std::uint64_t>::max() / indexed_indirect_record_stride_bytes)) {
        add_diagnostic(plan, MavgGpuCullingDiagnosticCode::command_buffer_size_overflow, AssetId{},
                       static_cast<std::uint32_t>(plan.commands.size()),
                       "MAVG GPU culling indirect command buffer size overflow");
        fail_closed(plan);
        return plan;
    }

    plan.count_buffer_value = static_cast<std::uint32_t>(plan.commands.size());
    plan.argument_buffer_size_bytes =
        static_cast<std::uint64_t>(plan.commands.size()) * indexed_indirect_record_stride_bytes;

    if (desc.producer == MavgGpuCullingProducer::compute_shader) {
        append_compute_sync_requirements(plan);
    }

    return plan;
}

bool has_mavg_gpu_culling_diagnostic(const MavgGpuCullingIndirectPlan& plan,
                                     MavgGpuCullingDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics,
                               [code](const MavgGpuCullingDiagnostic& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
