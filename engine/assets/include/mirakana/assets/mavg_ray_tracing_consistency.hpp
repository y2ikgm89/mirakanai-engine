// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgRayTracingPayloadPolicy : std::uint8_t {
    unsupported,
    static_blas_build,
    deformable_blas_refit,
    deformable_blas_rebuild,
};

enum class MavgRayTracingDeformationTier : std::uint8_t {
    static_cluster,
    rigid_instance,
    skinned_cluster,
    morph_cluster,
    dynamic_displacement,
};

enum class MavgRayTracingConsistencyDiagnosticCode : std::uint8_t {
    missing_cluster_graph,
    invalid_cluster_graph,
    unknown_cluster,
    duplicate_raster_payload_row,
    duplicate_ray_tracing_payload_row,
    missing_raster_payload,
    missing_ray_tracing_payload,
    payload_material_mismatch,
    payload_page_mismatch,
    payload_draw_range_mismatch,
    fallback_mismatch,
    unsupported_payload_policy,
    unsupported_deformation_tier,
    unsupported_dynamic_displacement,
};

struct MavgRasterClusterPayloadRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::uint32_t material_partition{0};
    std::uint32_t page_index{0};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
    std::uint32_t resident_fallback_cluster_index{0};
};

struct MavgRayTracingClusterPayloadRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::uint32_t material_partition{0};
    std::uint32_t page_index{0};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
    std::uint32_t resident_fallback_cluster_index{0};
    MavgRayTracingPayloadPolicy payload_policy{MavgRayTracingPayloadPolicy::unsupported};
    MavgRayTracingDeformationTier deformation_tier{MavgRayTracingDeformationTier::static_cluster};
};

struct MavgRayTracingConsistencyDiagnostic {
    MavgRayTracingConsistencyDiagnosticCode code{MavgRayTracingConsistencyDiagnosticCode::missing_cluster_graph};
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::string field;
    std::string message;
};

struct MavgRayTracingConsistencyDesc {
    const MavgClusterGraphDocument* cluster_graph{nullptr};
    std::vector<MavgRasterClusterPayloadRow> raster_payloads;
    std::vector<MavgRayTracingClusterPayloadRow> ray_tracing_payloads;
};

struct MavgRayTracingConsistencyRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    MavgRayTracingPayloadPolicy payload_policy{MavgRayTracingPayloadPolicy::unsupported};
    MavgRayTracingDeformationTier deformation_tier{MavgRayTracingDeformationTier::static_cluster};
    std::uint32_t raster_payload_page_index{0};
    std::uint32_t ray_tracing_payload_page_index{0};
    std::uint32_t resident_fallback_cluster_index{0};
    bool consistent{false};
    bool requires_backend_blas_build{false};
    bool requires_backend_blas_refit{false};
    bool requires_backend_blas_rebuild{false};
};

struct MavgRayTracingConsistencyResult {
    std::vector<MavgRayTracingConsistencyRow> rows;
    std::vector<MavgRayTracingConsistencyDiagnostic> diagnostics;
    bool executed_ray_tracing{false};
    bool exposed_native_handles{false};

    [[nodiscard]] bool valid() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] MavgRayTracingConsistencyResult
plan_mavg_ray_tracing_consistency(const MavgRayTracingConsistencyDesc& desc);

[[nodiscard]] bool has_mavg_ray_tracing_consistency_diagnostic(const MavgRayTracingConsistencyResult& result,
                                                               MavgRayTracingConsistencyDiagnosticCode code) noexcept;

} // namespace mirakana
