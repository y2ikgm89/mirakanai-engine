// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_scene_rhi/mavg_deformation_integration.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mirakana::runtime_scene_rhi {
namespace {

using Code = MavgDeformationIntegrationDiagnosticCode;
using Kind = MavgDeformationIntegrationKind;

void add_diagnostic(MavgDeformationIntegrationResult& result, Code code, std::uint32_t cluster_index,
                    std::string row_id, std::string message) {
    result.diagnostics.push_back(MavgDeformationIntegrationDiagnostic{
        .code = code,
        .cluster_index = cluster_index,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool valid_bounds(const MavgBounds3f& bounds) noexcept {
    return bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y && bounds.min.z <= bounds.max.z;
}

[[nodiscard]] bool contains_bounds(const MavgBounds3f& outer, const MavgBounds3f& inner) noexcept {
    return outer.min.x <= inner.min.x && outer.min.y <= inner.min.y && outer.min.z <= inner.min.z &&
           outer.max.x >= inner.max.x && outer.max.y >= inner.max.y && outer.max.z >= inner.max.z;
}

[[nodiscard]] bool same_bounds(const MavgBounds3f& left, const MavgBounds3f& right) noexcept {
    return left.min.x == right.min.x && left.min.y == right.min.y && left.min.z == right.min.z &&
           left.max.x == right.max.x && left.max.y == right.max.y && left.max.z == right.max.z;
}

[[nodiscard]] const MavgClusterGraphCluster* find_cluster(const MavgClusterGraphDocument& graph,
                                                          const std::uint32_t cluster_index) noexcept {
    const auto it = std::ranges::find_if(graph.clusters, [cluster_index](const MavgClusterGraphCluster& cluster) {
        return cluster.cluster_index == cluster_index;
    });
    return it == graph.clusters.end() ? nullptr : &*it;
}

[[nodiscard]] bool has_page(const MavgClusterGraphDocument& graph, const std::uint32_t page_index) noexcept {
    return std::ranges::any_of(
        graph.pages, [page_index](const MavgClusterGraphPage& page) { return page.page_index == page_index; });
}

[[nodiscard]] bool supported_kind(const Kind kind) noexcept {
    switch (kind) {
    case Kind::rigid_transform:
    case Kind::linear_blend_skinning:
    case Kind::morph_target:
        return true;
    }
    return false;
}

[[nodiscard]] std::string cluster_row_id(const std::uint32_t cluster_index) {
    return "mavg.deformation.cluster." + std::to_string(cluster_index);
}

void validate_cluster_row(MavgDeformationIntegrationResult& result, const MavgClusterGraphDocument& graph,
                          const MavgDeformationClusterBoundsRow& row) {
    const auto row_id = cluster_row_id(row.cluster_index);
    const auto* cluster = find_cluster(graph, row.cluster_index);
    if (cluster == nullptr) {
        add_diagnostic(result, Code::unknown_cluster, row.cluster_index, row_id,
                       "MAVG deformation row references an unknown cluster");
        return;
    }

    const auto diagnostics_before = result.diagnostics.size();
    if (!row.stable_cluster_id || cluster->cluster_index != row.cluster_index) {
        add_diagnostic(result, Code::unstable_cluster_id, row.cluster_index, row_id,
                       "MAVG deformation integration requires stable cluster ids");
    }
    if (!valid_bounds(row.conservative_deformed_bounds)) {
        add_diagnostic(result, Code::invalid_conservative_bounds, row.cluster_index, row_id,
                       "MAVG deformation conservative bounds must be valid");
    }
    if (!same_bounds(row.base_bounds, cluster->bounds) ||
        !contains_bounds(row.conservative_deformed_bounds, cluster->bounds)) {
        add_diagnostic(result, Code::bounds_not_conservative, row.cluster_index, row_id,
                       "MAVG deformation conservative bounds must contain the original cluster bounds");
    }
    if (row.material_partition != cluster->material_partition || !row.material_root_preserved) {
        add_diagnostic(result, Code::material_root_not_preserved, row.cluster_index, row_id,
                       "MAVG deformation integration must preserve cluster material partition roots");
    }
    if (row.page_index != cluster->page_index || !row.resident_page_valid || !has_page(graph, row.page_index)) {
        add_diagnostic(result, Code::resident_page_invalid, row.cluster_index, row_id,
                       "MAVG deformation integration requires valid resident page evidence");
    }
    if (row.first_index != cluster->first_index || row.index_count != cluster->index_count ||
        row.vertex_base != cluster->vertex_base || !row.fallback_draw_range_preserved) {
        add_diagnostic(result, Code::fallback_draw_range_not_preserved, row.cluster_index, row_id,
                       "MAVG deformation integration must preserve fallback draw ranges");
    }
    if (row.topology_changing_deformation) {
        add_diagnostic(result, Code::topology_changing_deformation, row.cluster_index, row_id,
                       "MAVG deformation integration rejects topology-changing deformation");
    }
    if (row.runtime_generated_triangle_topology) {
        add_diagnostic(result, Code::runtime_generated_triangle_topology, row.cluster_index, row_id,
                       "MAVG deformation integration rejects runtime-generated triangle topology");
    }
    if (row.unbounded_vertex_displacement) {
        add_diagnostic(result, Code::unbounded_vertex_displacement, row.cluster_index, row_id,
                       "MAVG deformation integration rejects unbounded vertex displacement");
    }
    if (!supported_kind(row.deformation_kind)) {
        add_diagnostic(result, Code::invalid_deformation_kind, row.cluster_index, row_id,
                       "MAVG deformation integration supports only selected deformation kinds");
    }
    if (row.deformation_kind == Kind::linear_blend_skinning &&
        (row.max_joint_influences == 0U || row.max_joint_influences > 4U)) {
        add_diagnostic(result, Code::excessive_joint_influence_count, row.cluster_index, row_id,
                       "MAVG linear blend skinning integration requires one to four joint influences");
    }
    if (row.deformation_kind != Kind::linear_blend_skinning && row.max_joint_influences != 0U) {
        add_diagnostic(result, Code::excessive_joint_influence_count, row.cluster_index, row_id,
                       "MAVG non-skinning deformation rows must not claim joint influences");
    }
    if (row.deformation_kind == Kind::morph_target && same_bounds(row.conservative_deformed_bounds, cluster->bounds)) {
        add_diagnostic(result, Code::bounds_not_conservative, row.cluster_index, row_id,
                       "MAVG morph target integration requires explicit conservative bounds expansion");
    }
    if (row.touched_native_handles) {
        result.native_handles_exposed = true;
        add_diagnostic(result, Code::native_handle_access, row.cluster_index, row_id,
                       "MAVG deformation integration must not expose native handles");
    }

    if (result.diagnostics.size() == diagnostics_before) {
        ++result.policy_ready_cluster_count;
        result.integrated_clusters.push_back(MavgDeformationIntegratedCluster{
            .cluster_index = row.cluster_index,
            .deformation_kind = row.deformation_kind,
            .conservative_bounds = row.conservative_deformed_bounds,
            .page_index = row.page_index,
            .material_partition = row.material_partition,
            .first_index = row.first_index,
            .index_count = row.index_count,
            .vertex_base = row.vertex_base,
        });
    }
}

} // namespace

MavgDeformationIntegrationResult plan_mavg_deformation_integrated_clusters(const MavgDeformationIntegrationDesc& desc) {
    MavgDeformationIntegrationResult result;
    if (desc.graph == nullptr) {
        add_diagnostic(result, Code::invalid_graph, 0, "mavg.deformation.graph",
                       "MAVG deformation integration requires a graph");
        return result;
    }

    const auto graph_validation = validate_mavg_cluster_graph(*desc.graph);
    if (!graph_validation.valid()) {
        add_diagnostic(result, Code::invalid_graph, 0, "mavg.deformation.graph",
                       "MAVG deformation integration requires a valid MAVG cluster graph");
        return result;
    }
    if (desc.cluster_bounds_rows.empty()) {
        add_diagnostic(result, Code::missing_cluster_bounds_row, 0, "mavg.deformation.cluster_bounds",
                       "MAVG deformation integration requires reviewed cluster bounds rows");
    }
    if (desc.request_broad_deformation_readiness) {
        add_diagnostic(result, Code::broad_deformation_readiness_not_promoted, 0, "mavg.deformation.broad",
                       "MAVG deformation integration does not promote broad deformation readiness");
    }

    std::vector<std::uint32_t> seen_clusters;
    seen_clusters.reserve(desc.cluster_bounds_rows.size());
    for (const auto& row : desc.cluster_bounds_rows) {
        ++result.reviewed_cluster_count;
        if (std::ranges::contains(seen_clusters, row.cluster_index)) {
            add_diagnostic(result, Code::duplicate_cluster_bounds_row, row.cluster_index,
                           cluster_row_id(row.cluster_index),
                           "MAVG deformation integration rejects duplicate cluster bounds rows");
            continue;
        }
        seen_clusters.push_back(row.cluster_index);
        validate_cluster_row(result, *desc.graph, row);
    }

    for (const auto& row : desc.backend_execution_rows) {
        const auto row_id = row.row_id.empty() ? "mavg.deformation.backend" : std::string(row.row_id);
        if (row.touched_native_handles) {
            result.native_handles_exposed = true;
            add_diagnostic(result, Code::native_handle_access, 0, row_id,
                           "MAVG deformation backend execution rows must keep native handles private");
        }
        if (!row.reviewed || !row.execution_evidence || !row.ready) {
            add_diagnostic(result, Code::backend_execution_not_ready, 0, row_id,
                           "MAVG deformation backend execution rows must be reviewed, executed, and ready");
        } else {
            ++result.backend_execution_ready_count;
        }
    }

    result.mavg_deformation_policy_ready = result.diagnostics.empty() &&
                                           result.policy_ready_cluster_count == result.reviewed_cluster_count &&
                                           result.reviewed_cluster_count > 0U;

    if (desc.require_backend_execution && result.backend_execution_ready_count == 0U) {
        add_diagnostic(result, Code::backend_execution_required, 0, "mavg.deformation.backend",
                       "MAVG deformation integration readiness requires selected backend execution evidence");
    }

    result.mavg_broad_deformation_readiness_ready = false;
    result.mavg_deformation_integration_ready =
        result.mavg_deformation_policy_ready && desc.require_backend_execution &&
        result.backend_execution_ready_count > 0U && result.diagnostics.empty() && !result.native_handles_exposed;
    return result;
}

bool has_mavg_deformation_integration_diagnostic(const MavgDeformationIntegrationResult& result,
                                                 const MavgDeformationIntegrationDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const MavgDeformationIntegrationDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

bool has_mavg_deformation_integration_cluster_diagnostic(const MavgDeformationIntegrationResult& result,
                                                         const std::uint32_t cluster_index,
                                                         const MavgDeformationIntegrationDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics,
                               [cluster_index, code](const MavgDeformationIntegrationDiagnostic& diagnostic) {
                                   return diagnostic.cluster_index == cluster_index && diagnostic.code == code;
                               });
}

} // namespace mirakana::runtime_scene_rhi
