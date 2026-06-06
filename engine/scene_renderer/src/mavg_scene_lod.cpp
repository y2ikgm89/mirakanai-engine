// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene_renderer/mavg_scene_lod.hpp"

#include <string>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(MavgSceneLodSubmitResult& result, MavgSceneLodDiagnosticCode code, std::uint32_t cluster_index,
                    AssetId asset, std::string message) {
    result.diagnostics.push_back(MavgSceneLodDiagnostic{
        .code = code,
        .cluster_index = cluster_index,
        .asset = asset,
        .message = std::move(message),
    });
}

void reject(MavgSceneLodSubmitResult& result, MavgSceneLodDiagnosticCode code, std::uint32_t cluster_index,
            AssetId asset, std::string message) {
    add_diagnostic(result, code, cluster_index, asset, std::move(message));
    result.rejected = true;
    result.mesh_commands.clear();
    result.submitted_cluster_count = 0;
    result.fallback_substitution_count = 0;
    result.missing_material_binding_count = 0;
}

[[nodiscard]] const MavgClusterGraphCluster* find_cluster_by_index(const MavgClusterGraphDocument& graph,
                                                                   std::uint32_t cluster_index) noexcept {
    for (const auto& cluster : graph.clusters) {
        if (cluster.cluster_index == cluster_index) {
            return &cluster;
        }
    }
    return nullptr;
}

[[nodiscard]] bool selected_cluster_matches_graph(const MavgLodSelectedCluster& selected,
                                                  const MavgClusterGraphCluster& cluster,
                                                  const MavgClusterGraphDocument& graph) noexcept {
    return selected.graph_asset == graph.asset && selected.page_index == cluster.page_index &&
           selected.lod_level == cluster.lod_level && selected.material_partition == cluster.material_partition &&
           selected.first_index == cluster.first_index && selected.index_count == cluster.index_count &&
           selected.vertex_base == cluster.vertex_base;
}

[[nodiscard]] bool material_partition_contains_cluster(const MavgClusterGraphMaterialPartition& partition,
                                                       std::uint32_t cluster_index) noexcept {
    return cluster_index >= partition.first_cluster &&
           cluster_index - partition.first_cluster < partition.cluster_count;
}

[[nodiscard]] MeshCommand make_command(const MavgLodSelectedCluster& selected,
                                       const MavgClusterGraphMaterialPartition& partition,
                                       const MeshGpuBinding& mesh_binding, MaterialGpuBinding material_binding,
                                       const MavgSceneLodSubmitDesc& desc) noexcept {
    return MeshCommand{
        .transform = desc.transform,
        .color = desc.fallback_color,
        .mesh = selected.graph_asset,
        .material = partition.material,
        .world_from_node = desc.transform.matrix(),
        .mesh_binding = mesh_binding,
        .material_binding = material_binding,
        .instance_count = desc.instance_count,
        .indexed_range =
            MeshIndexedDrawRange{
                .enabled = true,
                .first_index = selected.first_index,
                .index_count = selected.index_count,
                .vertex_base = selected.vertex_base,
                .first_instance = 0,
            },
    };
}

} // namespace

bool MavgSceneLodSubmitResult::succeeded() const noexcept {
    return !rejected;
}

MavgSceneLodSubmitResult plan_mavg_scene_lod_mesh_commands(const MavgLodSelectionResult& selection,
                                                           const MavgClusterGraphDocument& graph,
                                                           const MavgSceneLodSubmitDesc& desc) {
    MavgSceneLodSubmitResult result;

    const auto graph_validation = validate_mavg_cluster_graph(graph);
    if (!graph_validation.valid()) {
        reject(result, MavgSceneLodDiagnosticCode::invalid_graph, 0, graph.asset,
               graph_validation.diagnostics.empty() ? "invalid MAVG cluster graph"
                                                    : graph_validation.diagnostics.front().message);
        return result;
    }

    if (!selection.succeeded()) {
        reject(result, MavgSceneLodDiagnosticCode::invalid_selection, 0, graph.asset,
               "MAVG LOD selection has fatal diagnostics");
        return result;
    }

    if (desc.gpu_bindings == nullptr) {
        reject(result, MavgSceneLodDiagnosticCode::missing_mesh_binding, 0, graph.asset,
               "MAVG scene LOD submission requires GPU bindings");
        return result;
    }

    const auto* mesh_binding = desc.gpu_bindings->find_mesh(graph.asset);
    if (mesh_binding == nullptr) {
        reject(result, MavgSceneLodDiagnosticCode::missing_mesh_binding, 0, graph.asset,
               "MAVG scene LOD submission is missing a mesh GPU binding for the graph asset");
        return result;
    }

    result.mesh_commands.reserve(selection.selected_clusters.size());
    for (const auto& selected : selection.selected_clusters) {
        const auto* cluster = find_cluster_by_index(graph, selected.cluster_index);
        if (cluster == nullptr || !selected_cluster_matches_graph(selected, *cluster, graph)) {
            reject(result, MavgSceneLodDiagnosticCode::invalid_selected_cluster, selected.cluster_index,
                   selected.graph_asset, "MAVG selected cluster does not match the cluster graph");
            return result;
        }

        if (selected.material_partition >= graph.material_partitions.size()) {
            reject(result, MavgSceneLodDiagnosticCode::invalid_material_partition, selected.cluster_index, graph.asset,
                   "MAVG selected cluster references an unknown material partition");
            return result;
        }

        const auto& partition = graph.material_partitions[selected.material_partition];
        if (!material_partition_contains_cluster(partition, selected.cluster_index)) {
            reject(result, MavgSceneLodDiagnosticCode::invalid_material_partition, selected.cluster_index,
                   partition.material, "MAVG selected cluster is outside its material partition");
            return result;
        }

        MaterialGpuBinding material_binding;
        if (const auto* found = desc.gpu_bindings->find_material(partition.material); found != nullptr) {
            material_binding = *found;
        } else {
            ++result.missing_material_binding_count;
            add_diagnostic(result, MavgSceneLodDiagnosticCode::missing_material_binding, selected.cluster_index,
                           partition.material, "MAVG scene LOD submission is missing a material GPU binding");
        }

        if (selected.fallback_substitution) {
            ++result.fallback_substitution_count;
            add_diagnostic(result, MavgSceneLodDiagnosticCode::fallback_substitution, selected.cluster_index,
                           selected.graph_asset, "MAVG selected cluster used a resident fallback substitution");
        }

        result.mesh_commands.push_back(make_command(selected, partition, *mesh_binding, material_binding, desc));
        ++result.submitted_cluster_count;
    }

    return result;
}

} // namespace mirakana
