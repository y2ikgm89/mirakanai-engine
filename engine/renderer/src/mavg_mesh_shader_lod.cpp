// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/mavg_mesh_shader_lod.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr std::uint32_t d3d12_mesh_shader_max_group_threads = 128U;

void add_diagnostic(MavgMeshShaderLodPlan& plan, MavgMeshShaderLodDiagnosticCode code, AssetId graph_asset,
                    std::uint32_t cluster_index, std::string message) {
    plan.diagnostics.push_back(MavgMeshShaderLodDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .cluster_index = cluster_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool same_asset(AssetId lhs, AssetId rhs) noexcept {
    return lhs.value == rhs.value;
}

[[nodiscard]] bool valid_draw_range(const MavgLodSelectedCluster& selected) noexcept {
    return selected.index_count > 0U &&
           selected.first_index <= (std::numeric_limits<std::uint32_t>::max() - selected.index_count);
}

[[nodiscard]] std::vector<const MavgMeshShaderLodMaterialRootRow*>
matching_material_roots(std::span<const MavgMeshShaderLodMaterialRootRow> material_roots,
                        const MavgLodSelectedCluster& selected) {
    std::vector<const MavgMeshShaderLodMaterialRootRow*> matches;
    for (const auto& row : material_roots) {
        if (same_asset(row.graph_asset, selected.graph_asset) &&
            row.material_partition == selected.material_partition) {
            matches.push_back(&row);
        }
    }
    return matches;
}

[[nodiscard]] const MavgMeshShaderLodMaterialRootRow* find_material_root(MavgMeshShaderLodPlan& plan,
                                                                         const MavgMeshShaderLodDesc& desc,
                                                                         const MavgLodSelectedCluster& selected) {
    const auto matches = matching_material_roots(desc.material_roots, selected);
    if (matches.empty()) {
        add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::missing_material_root, selected.graph_asset,
                       selected.cluster_index, "selected MAVG cluster is missing a material root row");
        return nullptr;
    }
    if (matches.size() > 1U) {
        const auto first_root = matches.front()->material_root;
        const bool mismatched = std::ranges::any_of(matches, [first_root](const auto* row) {
            return row != nullptr && !same_asset(row->material_root, first_root);
        });
        add_diagnostic(plan,
                       mismatched ? MavgMeshShaderLodDiagnosticCode::mismatched_material_roots
                                  : MavgMeshShaderLodDiagnosticCode::duplicate_material_root,
                       selected.graph_asset, selected.cluster_index,
                       mismatched ? "selected MAVG material partition has mismatched material roots"
                                  : "selected MAVG material partition has duplicate material root rows");
        return nullptr;
    }
    return matches.front();
}

[[nodiscard]] std::vector<const MavgMeshShaderLodMeshletRow*>
matching_meshlets(std::span<const MavgMeshShaderLodMeshletRow> meshlets, const MavgLodSelectedCluster& selected) {
    std::vector<const MavgMeshShaderLodMeshletRow*> matches;
    for (const auto& row : meshlets) {
        if (same_asset(row.graph_asset, selected.graph_asset) && row.cluster_index == selected.cluster_index) {
            matches.push_back(&row);
        }
    }
    std::ranges::sort(matches, [](const auto* lhs, const auto* rhs) {
        if (lhs->local_meshlet_index != rhs->local_meshlet_index) {
            return lhs->local_meshlet_index < rhs->local_meshlet_index;
        }
        return lhs->meshlet_index < rhs->meshlet_index;
    });
    return matches;
}

[[nodiscard]] bool valid_meshlet_group_size(const MavgMeshShaderLodMeshletRow& row,
                                            std::uint32_t expected_group_thread_count) noexcept {
    return row.group_thread_count > 0U && row.group_thread_count <= d3d12_mesh_shader_max_group_threads &&
           (expected_group_thread_count == 0U || row.group_thread_count == expected_group_thread_count);
}

[[nodiscard]] bool valid_meshlet_output(const MavgMeshShaderLodMeshletRow& row) noexcept {
    return row.output_vertex_count >= 3U && row.output_primitive_count > 0U;
}

[[nodiscard]] bool has_duplicate_meshlet_indices(const std::vector<const MavgMeshShaderLodMeshletRow*>& meshlets) {
    for (std::size_t index = 1; index < meshlets.size(); ++index) {
        if (meshlets[index - 1]->local_meshlet_index == meshlets[index]->local_meshlet_index ||
            meshlets[index - 1]->meshlet_index == meshlets[index]->meshlet_index) {
            return true;
        }
    }
    return false;
}

void fail_closed(MavgMeshShaderLodPlan& plan) {
    plan.task_rows.clear();
    plan.fallback_draws.clear();
    plan.meshlet_task_count = 0;
    plan.fallback_draw_count = 0;
    plan.uses_mesh_shader_bind_points = false;
    plan.uses_amplification_shader_bind_point = false;
    plan.fallback_to_conventional_indexed_draws = false;
}

} // namespace

bool MavgMeshShaderLodPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

MavgMeshShaderLodPlan plan_mavg_mesh_shader_lod_tasks(const MavgMeshShaderLodDesc& desc) {
    MavgMeshShaderLodPlan plan;
    if (desc.selection == nullptr) {
        add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::invalid_selection, AssetId{}, 0,
                       "MAVG mesh shader LOD planning requires a selection result");
        return plan;
    }

    plan.selected_cluster_count = static_cast<std::uint32_t>(desc.selection->selected_clusters.size());
    if (!desc.selection->succeeded()) {
        add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::invalid_selection, AssetId{}, 0,
                       "MAVG mesh shader LOD planning requires a successful selection result");
    }
    if (desc.selection->selected_clusters.empty()) {
        add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::no_selected_clusters, AssetId{}, 0,
                       "MAVG mesh shader LOD planning requires at least one selected cluster");
    }
    if (desc.max_task_rows == 0U) {
        add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::max_task_rows_exceeded, AssetId{}, 0,
                       "MAVG mesh shader LOD planning requires a non-zero task row budget");
    }

    for (const auto& selected : desc.selection->selected_clusters) {
        if (!valid_draw_range(selected)) {
            add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::invalid_fallback_draw_range, selected.graph_asset,
                           selected.cluster_index,
                           "selected MAVG cluster fallback draw range must have a non-zero index_count");
        }

        const auto* material_root = find_material_root(plan, desc, selected);
        const auto meshlets = matching_meshlets(desc.meshlets, selected);
        if (meshlets.empty()) {
            add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::missing_meshlet_rows, selected.graph_asset,
                           selected.cluster_index, "selected MAVG cluster is missing meshlet task rows");
            continue;
        }
        if (has_duplicate_meshlet_indices(meshlets)) {
            add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::duplicate_meshlet_rows, selected.graph_asset,
                           selected.cluster_index, "selected MAVG cluster has duplicate meshlet task rows");
        }

        for (const auto* meshlet : meshlets) {
            if (meshlet == nullptr) {
                continue;
            }
            if (!valid_meshlet_group_size(*meshlet, desc.expected_group_thread_count)) {
                add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::invalid_meshlet_group_size, selected.graph_asset,
                               selected.cluster_index, "MAVG meshlet task row has an invalid mesh shader group size");
            }
            if (!valid_meshlet_output(*meshlet)) {
                add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::invalid_meshlet_output, selected.graph_asset,
                               selected.cluster_index,
                               "MAVG meshlet task row must emit at least one triangle worth of output");
            }
            if (material_root != nullptr) {
                plan.task_rows.push_back(MavgMeshShaderLodTaskRow{
                    .graph_asset = selected.graph_asset,
                    .cluster_index = selected.cluster_index,
                    .page_index = selected.page_index,
                    .lod_level = selected.lod_level,
                    .material_partition = selected.material_partition,
                    .material_root = material_root->material_root,
                    .meshlet_index = meshlet->meshlet_index,
                    .local_meshlet_index = meshlet->local_meshlet_index,
                    .output_vertex_count = meshlet->output_vertex_count,
                    .output_primitive_count = meshlet->output_primitive_count,
                    .group_thread_count = meshlet->group_thread_count,
                    .fallback_first_index = selected.first_index,
                    .fallback_index_count = selected.index_count,
                    .fallback_vertex_base = selected.vertex_base,
                    .fallback_substitution = selected.fallback_substitution,
                });
            }
        }

        plan.fallback_draws.push_back(MavgMeshShaderLodFallbackDrawRow{
            .graph_asset = selected.graph_asset,
            .cluster_index = selected.cluster_index,
            .page_index = selected.page_index,
            .lod_level = selected.lod_level,
            .material_partition = selected.material_partition,
            .first_index = selected.first_index,
            .index_count = selected.index_count,
            .vertex_base = selected.vertex_base,
            .fallback_substitution = selected.fallback_substitution,
            .uses_index_buffer = true,
        });
    }

    if (plan.task_rows.size() > desc.max_task_rows) {
        add_diagnostic(plan, MavgMeshShaderLodDiagnosticCode::max_task_rows_exceeded, AssetId{},
                       static_cast<std::uint32_t>(plan.task_rows.size()),
                       "MAVG mesh shader LOD task row count exceeds max_task_rows");
    }

    if (!plan.diagnostics.empty()) {
        fail_closed(plan);
        return plan;
    }

    std::ranges::sort(plan.task_rows, [](const MavgMeshShaderLodTaskRow& lhs, const MavgMeshShaderLodTaskRow& rhs) {
        if (lhs.material_partition != rhs.material_partition) {
            return lhs.material_partition < rhs.material_partition;
        }
        if (lhs.page_index != rhs.page_index) {
            return lhs.page_index < rhs.page_index;
        }
        if (lhs.cluster_index != rhs.cluster_index) {
            return lhs.cluster_index < rhs.cluster_index;
        }
        if (lhs.local_meshlet_index != rhs.local_meshlet_index) {
            return lhs.local_meshlet_index < rhs.local_meshlet_index;
        }
        return lhs.meshlet_index < rhs.meshlet_index;
    });

    plan.meshlet_task_count = static_cast<std::uint32_t>(plan.task_rows.size());
    plan.fallback_draw_count = static_cast<std::uint32_t>(plan.fallback_draws.size());
    plan.uses_mesh_shader_bind_points = true;
    plan.uses_amplification_shader_bind_point = true;
    plan.requires_input_assembler = false;
    plan.requires_index_buffer = false;
    plan.fallback_to_conventional_indexed_draws = !plan.fallback_draws.empty();
    return plan;
}

bool has_mavg_mesh_shader_lod_diagnostic(const MavgMeshShaderLodPlan& plan,
                                         MavgMeshShaderLodDiagnosticCode code) noexcept {
    return std::ranges::any_of(
        plan.diagnostics, [code](const MavgMeshShaderLodDiagnostic& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
