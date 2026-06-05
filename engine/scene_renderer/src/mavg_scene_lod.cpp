// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene_renderer/mavg_scene_lod.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mirakana {
namespace {

void reject(MavgSceneLodSubmitResult& result, std::string diagnostic) {
    result.rejected = true;
    result.diagnostics.push_back(std::move(diagnostic));
}

void warn(MavgSceneLodSubmitResult& result, std::string diagnostic) {
    result.diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] const MavgClusterGraphCluster* find_cluster(const MavgClusterGraphDocument& graph,
                                                          std::uint32_t cluster_index) noexcept {
    const auto iter = std::ranges::find_if(graph.clusters, [cluster_index](const MavgClusterGraphCluster& cluster) {
        return cluster.cluster_index == cluster_index;
    });
    return iter == graph.clusters.end() ? nullptr : &(*iter);
}

[[nodiscard]] const MavgSceneLodMaterialBinding*
find_material_binding(std::span<const MavgSceneLodMaterialBinding> bindings, AssetId material) noexcept {
    const auto iter = std::ranges::find_if(
        bindings, [material](const MavgSceneLodMaterialBinding& binding) { return binding.material == material; });
    return iter == bindings.end() ? nullptr : &(*iter);
}

[[nodiscard]] bool validate_inputs(const MavgLodSelectionResult& selection, const MavgClusterGraphDocument& graph,
                                   const MavgSceneLodSubmitDesc& desc, MavgSceneLodSubmitResult& result) {
    bool valid = true;
    const auto graph_validation = validate_mavg_cluster_graph(graph);
    if (!graph_validation.valid()) {
        reject(result, "invalid-graph: MAVG cluster graph failed validation");
        valid = false;
    }
    if (!selection.succeeded()) {
        reject(result, "invalid-selection: MAVG LOD selection has fatal diagnostics");
        valid = false;
    }
    if (desc.graph_asset.value == 0 || desc.graph_asset != graph.asset) {
        reject(result, "invalid-graph-asset: submit graph asset must match the MAVG cluster graph asset");
        valid = false;
    }
    if (desc.mesh_binding == nullptr) {
        reject(result, "missing-mesh-binding: MAVG scene LOD submission requires a mesh binding");
        valid = false;
    } else if (desc.mesh_binding->graph_asset != graph.asset) {
        reject(result, "invalid-mesh-binding: MAVG scene LOD mesh binding graph asset mismatch");
        valid = false;
    }
    return valid;
}

} // namespace

bool MavgSceneLodSubmitResult::succeeded() const noexcept {
    return !rejected;
}

MavgSceneLodSubmitResult plan_mavg_scene_lod_mesh_commands(const MavgLodSelectionResult& selection,
                                                           const MavgClusterGraphDocument& graph,
                                                           const MavgSceneLodSubmitDesc& desc) {
    MavgSceneLodSubmitResult result;
    if (!validate_inputs(selection, graph, desc, result)) {
        return result;
    }

    result.mesh_commands.reserve(selection.selected_clusters.size());
    for (const auto& selected : selection.selected_clusters) {
        if (selected.graph_asset != graph.asset) {
            reject(result, "invalid-selected-cluster: selected cluster graph asset mismatch");
            return result;
        }

        const auto* cluster = find_cluster(graph, selected.cluster_index);
        if (cluster == nullptr) {
            reject(result, "missing-selected-cluster: selected cluster is absent from the MAVG cluster graph");
            return result;
        }
        if (selected.material_partition >= graph.material_partitions.size() ||
            cluster->material_partition != selected.material_partition) {
            reject(result, "invalid-material-partition: selected cluster material partition is invalid");
            return result;
        }

        const auto material = graph.material_partitions[selected.material_partition].material;
        MeshCommand command{
            .transform = desc.transform,
            .color = desc.fallback_color,
            .mesh = graph.asset,
            .material = material,
            .world_from_node = desc.transform.matrix(),
            .mesh_binding = desc.mesh_binding->mesh_binding,
            .material_binding = {},
            .instance_count = 1,
            .indexed_range = MeshIndexedDrawRange{.enabled = true,
                                                  .first_index = cluster->first_index,
                                                  .index_count = cluster->index_count,
                                                  .vertex_base = cluster->vertex_base,
                                                  .first_instance = 0},
        };

        if (const auto* material_binding = find_material_binding(desc.material_bindings, material);
            material_binding != nullptr) {
            command.material_binding = material_binding->material_binding;
        } else {
            ++result.missing_material_binding_count;
            warn(result, "missing-material-binding: MAVG scene LOD cluster will use fallback color");
        }

        if (selected.fallback_substitution) {
            ++result.fallback_substitution_count;
            warn(result, "fallback-substitution: MAVG scene LOD selected a resident fallback cluster");
        }

        result.mesh_commands.push_back(command);
        ++result.submitted_cluster_count;
    }

    return result;
}

} // namespace mirakana
