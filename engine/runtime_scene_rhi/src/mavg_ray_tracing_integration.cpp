// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_scene_rhi/mavg_ray_tracing_integration.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mirakana::runtime_scene_rhi {
namespace {

using Backend = MavgRayTracingBackendKind;
using Code = MavgRayTracingIntegrationDiagnosticCode;
using GeometryPolicy = MavgRayTracingGeometryPolicy;
using IndexFormat = MavgRayTracingIndexFormat;
using MaterialAlphaPolicy = MavgRayTracingMaterialAlphaPolicy;

void add_diagnostic(MavgRayTracingIntegrationResult& result, Code code, std::uint32_t cluster_index, std::string row_id,
                    std::string message) {
    result.diagnostics.push_back(MavgRayTracingIntegrationDiagnostic{
        .code = code,
        .cluster_index = cluster_index,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
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

[[nodiscard]] bool supported_index_format(const IndexFormat format) noexcept {
    switch (format) {
    case IndexFormat::uint16:
    case IndexFormat::uint32:
        return true;
    }
    return false;
}

[[nodiscard]] bool supported_material_alpha_policy(const MaterialAlphaPolicy policy) noexcept {
    switch (policy) {
    case MaterialAlphaPolicy::opaque:
    case MaterialAlphaPolicy::alpha_tested:
        return true;
    case MaterialAlphaPolicy::alpha_blended_rejected:
        return false;
    }
    return false;
}

[[nodiscard]] std::string cluster_row_id(const std::uint32_t cluster_index) {
    return "mavg.ray_tracing.cluster." + std::to_string(cluster_index);
}

void validate_blas_input_row(MavgRayTracingIntegrationResult& result, const MavgClusterGraphDocument& graph,
                             const MavgRayTracingBlasInputRow& row) {
    const auto row_id = cluster_row_id(row.cluster_index);
    const auto* cluster = find_cluster(graph, row.cluster_index);
    if (cluster == nullptr) {
        add_diagnostic(result, Code::unknown_cluster, row.cluster_index, row_id,
                       "MAVG ray tracing BLAS row references an unknown cluster");
        return;
    }

    const auto diagnostics_before = result.diagnostics.size();
    if (!row.stable_cluster_id || cluster->cluster_index != row.cluster_index) {
        add_diagnostic(result, Code::unstable_cluster_id, row.cluster_index, row_id,
                       "MAVG ray tracing BLAS rows require stable cluster ids");
    }
    if (row.replay_hash.empty()) {
        add_diagnostic(result, Code::missing_replay_hash, row.cluster_index, row_id,
                       "MAVG ray tracing BLAS rows require a stable replay hash");
    }
    if (row.geometry_byte_count == 0U) {
        add_diagnostic(result, Code::invalid_geometry_byte_count, row.cluster_index, row_id,
                       "MAVG ray tracing BLAS rows require a positive geometry byte count");
    }
    if (!supported_index_format(row.index_format)) {
        add_diagnostic(result, Code::invalid_index_format, row.cluster_index, row_id,
                       "MAVG ray tracing BLAS rows support only uint16 or uint32 indices");
    }
    if (row.transform_row_id == 0U || !row.transform_row_valid) {
        add_diagnostic(result, Code::invalid_transform_row, row.cluster_index, row_id,
                       "MAVG ray tracing BLAS rows require a valid transform row");
    }
    if (!supported_material_alpha_policy(row.material_alpha_policy)) {
        add_diagnostic(result, Code::invalid_material_alpha_policy, row.cluster_index, row_id,
                       "MAVG ray tracing BLAS rows reject alpha-blended material policy");
    }
    if (row.uses_resident_cluster_geometry == row.uses_fallback_mesh_geometry) {
        add_diagnostic(result, Code::implicit_geometry_mode_switch, row.cluster_index, row_id,
                       "MAVG ray tracing BLAS rows require exactly one explicit geometry source");
    }
    if (row.geometry_policy == GeometryPolicy::resident_cluster_blas) {
        if (!row.uses_resident_cluster_geometry || row.uses_fallback_mesh_geometry) {
            add_diagnostic(result, Code::implicit_geometry_mode_switch, row.cluster_index, row_id,
                           "MAVG resident-cluster BLAS rows must not silently switch to fallback mesh geometry");
        }
        if (row.page_index != cluster->page_index || !row.resident_page_valid ||
            !has_page(graph, cluster->page_index)) {
            add_diagnostic(result, Code::resident_page_invalid, row.cluster_index, row_id,
                           "MAVG resident-cluster BLAS rows require valid resident page evidence");
        }
    } else if (row.geometry_policy == GeometryPolicy::fallback_mesh_blas) {
        if (!row.uses_fallback_mesh_geometry || row.uses_resident_cluster_geometry ||
            !row.fallback_mesh_matches_cluster) {
            add_diagnostic(result, Code::fallback_mode_mismatch, row.cluster_index, row_id,
                           "MAVG fallback BLAS rows require explicit fallback mesh match evidence");
        }
    } else {
        add_diagnostic(result, Code::implicit_geometry_mode_switch, row.cluster_index, row_id,
                       "MAVG ray tracing BLAS rows require an explicit geometry policy");
    }
    if (row.touched_native_handles) {
        result.native_handles_exposed = true;
        add_diagnostic(result, Code::native_handle_access, row.cluster_index, row_id,
                       "MAVG ray tracing integration must not expose native handles");
    }

    if (result.diagnostics.size() == diagnostics_before) {
        ++result.policy_ready_blas_input_count;
        result.blas_inputs.push_back(MavgRayTracingBlasInput{
            .cluster_index = row.cluster_index,
            .geometry_policy = row.geometry_policy,
            .replay_hash = std::string(row.replay_hash),
            .geometry_byte_count = row.geometry_byte_count,
            .index_format = row.index_format,
            .transform_row_id = row.transform_row_id,
            .material_alpha_policy = row.material_alpha_policy,
        });
    }
}

void validate_backend_row(MavgRayTracingIntegrationResult& result, const MavgRayTracingBackendExecutionRow& row) {
    const auto row_id = row.row_id.empty() ? "mavg.ray_tracing.backend" : std::string(row.row_id);
    if (row.touched_native_handles) {
        result.native_handles_exposed = true;
        add_diagnostic(result, Code::native_handle_access, 0, row_id,
                       "MAVG ray tracing backend execution rows must keep native handles private");
    }

    if (!row.reviewed || !row.ready) {
        add_diagnostic(result, Code::backend_execution_not_ready, 0, row_id,
                       "MAVG ray tracing backend execution rows must be reviewed and ready");
    }
    if (row.backend == Backend::d3d12) {
        if (!row.feature_query_ready) {
            add_diagnostic(result, Code::d3d12_dxr_feature_not_ready, 0, row_id,
                           "D3D12 MAVG ray tracing rows require a ready DXR feature query");
        }
        if (!row.acceleration_structure_build_evidence) {
            add_diagnostic(result, Code::d3d12_acceleration_structure_build_missing, 0, row_id,
                           "D3D12 MAVG ray tracing rows require BuildRaytracingAccelerationStructure evidence");
        }
    } else if (row.backend == Backend::vulkan) {
        if (!row.feature_query_ready) {
            add_diagnostic(result, Code::vulkan_acceleration_structure_feature_not_ready, 0, row_id,
                           "Vulkan MAVG ray tracing rows require VK_KHR_acceleration_structure feature evidence");
        }
        if (!row.acceleration_structure_build_evidence) {
            add_diagnostic(result, Code::vulkan_acceleration_structure_build_missing, 0, row_id,
                           "Vulkan MAVG ray tracing rows require acceleration-structure build evidence");
        }
    } else if (row.backend == Backend::metal_apple_host) {
        add_diagnostic(result, Code::metal_readiness_not_promoted, 0, row_id,
                       "Metal ray tracing readiness belongs to the Apple-host Metal task");
    }

    if (row.reviewed && row.ready && row.feature_query_ready && row.acceleration_structure_build_evidence &&
        row.backend != Backend::metal_apple_host && !row.touched_native_handles) {
        ++result.backend_execution_ready_count;
    }
}

} // namespace

MavgRayTracingIntegrationResult plan_mavg_ray_tracing_blas_inputs(const MavgRayTracingIntegrationDesc& desc) {
    MavgRayTracingIntegrationResult result;
    if (desc.graph == nullptr) {
        add_diagnostic(result, Code::invalid_graph, 0, "mavg.ray_tracing.graph",
                       "MAVG ray tracing integration requires a graph");
        return result;
    }

    const auto graph_validation = validate_mavg_cluster_graph(*desc.graph);
    if (!graph_validation.valid()) {
        add_diagnostic(result, Code::invalid_graph, 0, "mavg.ray_tracing.graph",
                       "MAVG ray tracing integration requires a valid MAVG cluster graph");
        return result;
    }
    if (desc.blas_input_rows.empty()) {
        add_diagnostic(result, Code::missing_blas_input_row, 0, "mavg.ray_tracing.blas_inputs",
                       "MAVG ray tracing integration requires reviewed BLAS input rows");
    }
    if (desc.request_metal_readiness) {
        add_diagnostic(result, Code::metal_readiness_not_promoted, 0, "mavg.ray_tracing.metal",
                       "MAVG ray tracing integration does not infer Metal readiness");
    }
    if (desc.request_broad_ray_tracing_readiness) {
        add_diagnostic(result, Code::broad_ray_tracing_readiness_not_promoted, 0, "mavg.ray_tracing.broad",
                       "MAVG ray tracing integration does not promote broad ray tracing readiness");
    }

    std::vector<std::uint32_t> seen_clusters;
    seen_clusters.reserve(desc.blas_input_rows.size());
    for (const auto& row : desc.blas_input_rows) {
        ++result.reviewed_blas_input_count;
        if (std::ranges::contains(seen_clusters, row.cluster_index)) {
            add_diagnostic(result, Code::duplicate_blas_input_row, row.cluster_index, cluster_row_id(row.cluster_index),
                           "MAVG ray tracing integration rejects duplicate BLAS input rows");
            continue;
        }
        seen_clusters.push_back(row.cluster_index);
        validate_blas_input_row(result, *desc.graph, row);
    }

    for (const auto& row : desc.backend_execution_rows) {
        validate_backend_row(result, row);
    }

    result.mavg_ray_tracing_policy_ready = result.diagnostics.empty() &&
                                           result.policy_ready_blas_input_count == result.reviewed_blas_input_count &&
                                           result.reviewed_blas_input_count > 0U;

    if (desc.require_backend_execution && result.backend_execution_ready_count == 0U) {
        add_diagnostic(result, Code::backend_execution_required, 0, "mavg.ray_tracing.backend",
                       "MAVG ray tracing readiness requires selected backend acceleration-structure build evidence");
    }

    result.mavg_metal_ray_tracing_ready = false;
    result.mavg_broad_ray_tracing_readiness_ready = false;
    result.mavg_ray_tracing_integration_ready =
        result.mavg_ray_tracing_policy_ready && desc.require_backend_execution &&
        result.backend_execution_ready_count > 0U && result.diagnostics.empty() && !result.native_handles_exposed;
    return result;
}

bool has_mavg_ray_tracing_integration_diagnostic(const MavgRayTracingIntegrationResult& result,
                                                 const MavgRayTracingIntegrationDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const MavgRayTracingIntegrationDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

bool has_mavg_ray_tracing_integration_cluster_diagnostic(const MavgRayTracingIntegrationResult& result,
                                                         const std::uint32_t cluster_index,
                                                         const MavgRayTracingIntegrationDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics,
                               [cluster_index, code](const MavgRayTracingIntegrationDiagnostic& diagnostic) {
                                   return diagnostic.cluster_index == cluster_index && diagnostic.code == code;
                               });
}

} // namespace mirakana::runtime_scene_rhi
