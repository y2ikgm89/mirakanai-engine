// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/mavg_ray_tracing_consistency.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {
namespace {

struct RasterEntry {
    MavgRasterClusterPayloadRow row;
    bool duplicate{false};
};

struct RayTracingEntry {
    MavgRayTracingClusterPayloadRow row;
    bool duplicate{false};
};

[[nodiscard]] bool same_payload_key(AssetId graph_asset, std::uint32_t cluster_index, AssetId candidate_graph_asset,
                                    std::uint32_t candidate_cluster_index) noexcept {
    return graph_asset == candidate_graph_asset && cluster_index == candidate_cluster_index;
}

[[nodiscard]] const MavgClusterGraphCluster* find_cluster(const MavgClusterGraphDocument& graph,
                                                          std::uint32_t cluster_index) noexcept {
    for (const auto& cluster : graph.clusters) {
        if (cluster.cluster_index == cluster_index) {
            return &cluster;
        }
    }
    return nullptr;
}

[[nodiscard]] bool has_cluster(const MavgClusterGraphDocument& graph, AssetId graph_asset,
                               std::uint32_t cluster_index) {
    return graph.asset == graph_asset && find_cluster(graph, cluster_index) != nullptr;
}

void add_diagnostic(std::vector<MavgRayTracingConsistencyDiagnostic>& diagnostics,
                    MavgRayTracingConsistencyDiagnosticCode code, AssetId graph_asset, std::uint32_t cluster_index,
                    std::string_view field, std::string_view message) {
    diagnostics.push_back(MavgRayTracingConsistencyDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .cluster_index = cluster_index,
        .field = std::string{field},
        .message = std::string{message},
    });
}

[[nodiscard]] std::vector<RasterEntry>
collect_raster_entries(const MavgRayTracingConsistencyDesc& desc,
                       std::vector<MavgRayTracingConsistencyDiagnostic>& diagnostics) {
    std::vector<RasterEntry> entries;
    if (desc.cluster_graph == nullptr) {
        return entries;
    }

    for (const auto& row : desc.raster_payloads) {
        if (!has_cluster(*desc.cluster_graph, row.graph_asset, row.cluster_index)) {
            add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::unknown_cluster, row.graph_asset,
                           row.cluster_index, "raster_payloads",
                           "raster payload row references a cluster outside the selected MAVG cluster graph");
            continue;
        }

        auto duplicate = false;
        for (auto& entry : entries) {
            if (same_payload_key(entry.row.graph_asset, entry.row.cluster_index, row.graph_asset, row.cluster_index)) {
                entry.duplicate = true;
                duplicate = true;
            }
        }
        if (duplicate) {
            add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::duplicate_raster_payload_row,
                           row.graph_asset, row.cluster_index, "raster_payloads",
                           "duplicate raster payload row for the same graph cluster");
            continue;
        }
        entries.push_back(RasterEntry{.row = row});
    }

    return entries;
}

[[nodiscard]] std::vector<RayTracingEntry>
collect_ray_tracing_entries(const MavgRayTracingConsistencyDesc& desc,
                            std::vector<MavgRayTracingConsistencyDiagnostic>& diagnostics) {
    std::vector<RayTracingEntry> entries;
    if (desc.cluster_graph == nullptr) {
        return entries;
    }

    for (const auto& row : desc.ray_tracing_payloads) {
        if (!has_cluster(*desc.cluster_graph, row.graph_asset, row.cluster_index)) {
            add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::unknown_cluster, row.graph_asset,
                           row.cluster_index, "ray_tracing_payloads",
                           "ray tracing payload row references a cluster outside the selected MAVG cluster graph");
            continue;
        }

        auto duplicate = false;
        for (auto& entry : entries) {
            if (same_payload_key(entry.row.graph_asset, entry.row.cluster_index, row.graph_asset, row.cluster_index)) {
                entry.duplicate = true;
                duplicate = true;
            }
        }
        if (duplicate) {
            add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::duplicate_ray_tracing_payload_row,
                           row.graph_asset, row.cluster_index, "ray_tracing_payloads",
                           "duplicate ray tracing payload row for the same graph cluster");
            continue;
        }
        entries.push_back(RayTracingEntry{.row = row});
    }

    return entries;
}

[[nodiscard]] const RasterEntry* find_raster_entry(const std::vector<RasterEntry>& entries, AssetId graph_asset,
                                                   std::uint32_t cluster_index) noexcept {
    for (const auto& entry : entries) {
        if (same_payload_key(entry.row.graph_asset, entry.row.cluster_index, graph_asset, cluster_index)) {
            return &entry;
        }
    }
    return nullptr;
}

[[nodiscard]] const RayTracingEntry* find_ray_tracing_entry(const std::vector<RayTracingEntry>& entries,
                                                            AssetId graph_asset, std::uint32_t cluster_index) noexcept {
    for (const auto& entry : entries) {
        if (same_payload_key(entry.row.graph_asset, entry.row.cluster_index, graph_asset, cluster_index)) {
            return &entry;
        }
    }
    return nullptr;
}

[[nodiscard]] bool supports_deformation_tier(MavgRayTracingPayloadPolicy policy,
                                             MavgRayTracingDeformationTier tier) noexcept {
    if (tier == MavgRayTracingDeformationTier::dynamic_displacement) {
        return false;
    }

    switch (policy) {
    case MavgRayTracingPayloadPolicy::static_blas_build:
        return tier == MavgRayTracingDeformationTier::static_cluster ||
               tier == MavgRayTracingDeformationTier::rigid_instance;
    case MavgRayTracingPayloadPolicy::deformable_blas_refit:
    case MavgRayTracingPayloadPolicy::deformable_blas_rebuild:
        return tier == MavgRayTracingDeformationTier::skinned_cluster ||
               tier == MavgRayTracingDeformationTier::morph_cluster;
    case MavgRayTracingPayloadPolicy::unsupported:
        return false;
    }
    return false;
}

[[nodiscard]] bool row_has_diagnostic(const std::vector<MavgRayTracingConsistencyDiagnostic>& diagnostics,
                                      AssetId graph_asset, std::uint32_t cluster_index) noexcept {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.graph_asset == graph_asset && diagnostic.cluster_index == cluster_index) {
            return true;
        }
    }
    return false;
}

void validate_payload_against_cluster(const MavgClusterGraphCluster& cluster, const MavgRasterClusterPayloadRow& raster,
                                      const MavgRayTracingClusterPayloadRow& ray_tracing,
                                      std::vector<MavgRayTracingConsistencyDiagnostic>& diagnostics) {
    if (cluster.material_partition != raster.material_partition ||
        cluster.material_partition != ray_tracing.material_partition) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::payload_material_mismatch,
                       ray_tracing.graph_asset, ray_tracing.cluster_index, "cluster.material_partition",
                       "payload rows must match the authoritative cluster material partition");
    }
    if (cluster.page_index != raster.page_index || cluster.page_index != ray_tracing.page_index) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::payload_page_mismatch,
                       ray_tracing.graph_asset, ray_tracing.cluster_index, "cluster.page_index",
                       "payload rows must match the authoritative cluster payload page");
    }
    if (cluster.first_index != raster.first_index || cluster.first_index != ray_tracing.first_index ||
        cluster.index_count != raster.index_count || cluster.index_count != ray_tracing.index_count ||
        cluster.vertex_base != raster.vertex_base || cluster.vertex_base != ray_tracing.vertex_base) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::payload_draw_range_mismatch,
                       ray_tracing.graph_asset, ray_tracing.cluster_index, "cluster.draw_range",
                       "payload rows must match the authoritative cluster draw range");
    }
    if (cluster.resident_fallback_cluster_index != raster.resident_fallback_cluster_index ||
        cluster.resident_fallback_cluster_index != ray_tracing.resident_fallback_cluster_index) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::fallback_mismatch, ray_tracing.graph_asset,
                       ray_tracing.cluster_index, "cluster.resident_fallback_cluster_index",
                       "payload rows must match the authoritative cluster resident fallback");
    }
}

void validate_payload_match(const MavgRasterClusterPayloadRow& raster,
                            const MavgRayTracingClusterPayloadRow& ray_tracing,
                            std::vector<MavgRayTracingConsistencyDiagnostic>& diagnostics) {
    if (raster.material_partition != ray_tracing.material_partition) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::payload_material_mismatch,
                       ray_tracing.graph_asset, ray_tracing.cluster_index, "material_partition",
                       "raster and ray tracing payload rows use different material partitions");
    }
    if (raster.page_index != ray_tracing.page_index) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::payload_page_mismatch,
                       ray_tracing.graph_asset, ray_tracing.cluster_index, "page_index",
                       "raster and ray tracing payload rows use different payload pages");
    }
    if (raster.first_index != ray_tracing.first_index || raster.index_count != ray_tracing.index_count ||
        raster.vertex_base != ray_tracing.vertex_base) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::payload_draw_range_mismatch,
                       ray_tracing.graph_asset, ray_tracing.cluster_index, "draw_range",
                       "raster and ray tracing payload rows use different draw ranges");
    }
    if (raster.resident_fallback_cluster_index != ray_tracing.resident_fallback_cluster_index) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::fallback_mismatch, ray_tracing.graph_asset,
                       ray_tracing.cluster_index, "resident_fallback_cluster_index",
                       "raster and ray tracing payload rows use different resident fallback clusters");
    }
}

void validate_ray_tracing_policy(const MavgRayTracingClusterPayloadRow& row,
                                 std::vector<MavgRayTracingConsistencyDiagnostic>& diagnostics) {
    if (row.payload_policy == MavgRayTracingPayloadPolicy::unsupported) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::unsupported_payload_policy,
                       row.graph_asset, row.cluster_index, "payload_policy",
                       "ray tracing payload row does not select a supported BLAS policy");
    }
    if (row.deformation_tier == MavgRayTracingDeformationTier::dynamic_displacement) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::unsupported_dynamic_displacement,
                       row.graph_asset, row.cluster_index, "deformation_tier",
                       "dynamic displacement clusters do not have RT refit/rebuild evidence in this slice");
    }
    if (!supports_deformation_tier(row.payload_policy, row.deformation_tier)) {
        add_diagnostic(diagnostics, MavgRayTracingConsistencyDiagnosticCode::unsupported_deformation_tier,
                       row.graph_asset, row.cluster_index, "deformation_tier",
                       "ray tracing payload policy is not compatible with the selected deformation tier");
    }
}

[[nodiscard]] MavgRayTracingConsistencyRow make_consistency_row(const MavgRasterClusterPayloadRow& raster,
                                                                const MavgRayTracingClusterPayloadRow& ray_tracing,
                                                                bool consistent) noexcept {
    return MavgRayTracingConsistencyRow{
        .graph_asset = ray_tracing.graph_asset,
        .cluster_index = ray_tracing.cluster_index,
        .payload_policy = ray_tracing.payload_policy,
        .deformation_tier = ray_tracing.deformation_tier,
        .raster_payload_page_index = raster.page_index,
        .ray_tracing_payload_page_index = ray_tracing.page_index,
        .resident_fallback_cluster_index = ray_tracing.resident_fallback_cluster_index,
        .consistent = consistent,
        .requires_backend_blas_build = ray_tracing.payload_policy == MavgRayTracingPayloadPolicy::static_blas_build,
        .requires_backend_blas_refit = ray_tracing.payload_policy == MavgRayTracingPayloadPolicy::deformable_blas_refit,
        .requires_backend_blas_rebuild =
            ray_tracing.payload_policy == MavgRayTracingPayloadPolicy::deformable_blas_rebuild,
    };
}

void sort_result(MavgRayTracingConsistencyResult& result) {
    std::ranges::sort(result.rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.graph_asset.value != rhs.graph_asset.value) {
            return lhs.graph_asset.value < rhs.graph_asset.value;
        }
        if (lhs.cluster_index != rhs.cluster_index) {
            return lhs.cluster_index < rhs.cluster_index;
        }
        return static_cast<std::uint8_t>(lhs.payload_policy) < static_cast<std::uint8_t>(rhs.payload_policy);
    });
    std::ranges::sort(result.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.graph_asset.value != rhs.graph_asset.value) {
            return lhs.graph_asset.value < rhs.graph_asset.value;
        }
        if (lhs.cluster_index != rhs.cluster_index) {
            return lhs.cluster_index < rhs.cluster_index;
        }
        return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
    });
}

} // namespace

MavgRayTracingConsistencyResult plan_mavg_ray_tracing_consistency(const MavgRayTracingConsistencyDesc& desc) {
    MavgRayTracingConsistencyResult result;
    if (desc.cluster_graph == nullptr) {
        add_diagnostic(result.diagnostics, MavgRayTracingConsistencyDiagnosticCode::missing_cluster_graph, AssetId{}, 0,
                       "cluster_graph", "MAVG ray tracing consistency planning requires a cluster graph");
        return result;
    }

    const auto graph_validation = validate_mavg_cluster_graph(*desc.cluster_graph);
    if (!graph_validation.valid()) {
        add_diagnostic(result.diagnostics, MavgRayTracingConsistencyDiagnosticCode::invalid_cluster_graph,
                       desc.cluster_graph->asset, 0, "cluster_graph",
                       "MAVG ray tracing consistency planning requires a valid cluster graph");
        return result;
    }

    const auto raster_entries = collect_raster_entries(desc, result.diagnostics);
    const auto ray_tracing_entries = collect_ray_tracing_entries(desc, result.diagnostics);

    for (const auto& cluster : desc.cluster_graph->clusters) {
        const auto* raster = find_raster_entry(raster_entries, desc.cluster_graph->asset, cluster.cluster_index);
        const auto* ray_tracing =
            find_ray_tracing_entry(ray_tracing_entries, desc.cluster_graph->asset, cluster.cluster_index);

        if (raster == nullptr) {
            add_diagnostic(result.diagnostics, MavgRayTracingConsistencyDiagnosticCode::missing_raster_payload,
                           desc.cluster_graph->asset, cluster.cluster_index, "raster_payloads",
                           "cluster graph row has no matching raster payload evidence");
        }
        if (ray_tracing == nullptr) {
            add_diagnostic(result.diagnostics, MavgRayTracingConsistencyDiagnosticCode::missing_ray_tracing_payload,
                           desc.cluster_graph->asset, cluster.cluster_index, "ray_tracing_payloads",
                           "cluster graph row has no matching ray tracing payload evidence");
        }
        if (raster == nullptr || ray_tracing == nullptr) {
            continue;
        }

        validate_payload_against_cluster(cluster, raster->row, ray_tracing->row, result.diagnostics);
        validate_payload_match(raster->row, ray_tracing->row, result.diagnostics);
        validate_ray_tracing_policy(ray_tracing->row, result.diagnostics);
        result.rows.push_back(make_consistency_row(
            raster->row, ray_tracing->row,
            !row_has_diagnostic(result.diagnostics, desc.cluster_graph->asset, cluster.cluster_index)));
    }

    sort_result(result);
    return result;
}

bool has_mavg_ray_tracing_consistency_diagnostic(const MavgRayTracingConsistencyResult& result,
                                                 MavgRayTracingConsistencyDiagnosticCode code) noexcept {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace mirakana
