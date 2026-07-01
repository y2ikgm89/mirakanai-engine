// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_scene_rhi {

enum class MavgDeformationIntegrationKind : std::uint8_t {
    rigid_transform = 0,
    linear_blend_skinning,
    morph_target,
};

enum class MavgDeformationBackendKind : std::uint8_t {
    d3d12 = 0,
    vulkan,
    metal_apple_host,
};

enum class MavgDeformationIntegrationDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_graph,
    missing_cluster_bounds_row,
    duplicate_cluster_bounds_row,
    unknown_cluster,
    unstable_cluster_id,
    invalid_conservative_bounds,
    bounds_not_conservative,
    material_root_not_preserved,
    resident_page_invalid,
    fallback_draw_range_not_preserved,
    topology_changing_deformation,
    runtime_generated_triangle_topology,
    unbounded_vertex_displacement,
    excessive_joint_influence_count,
    invalid_deformation_kind,
    backend_execution_required,
    backend_execution_not_ready,
    native_handle_access,
    broad_deformation_readiness_not_promoted,
};

struct MavgDeformationClusterBoundsRow {
    std::uint32_t cluster_index{0};
    MavgDeformationIntegrationKind deformation_kind{MavgDeformationIntegrationKind::rigid_transform};
    MavgBounds3f base_bounds;
    MavgBounds3f conservative_deformed_bounds;
    std::uint32_t page_index{0};
    std::uint32_t material_partition{0};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
    std::uint32_t max_joint_influences{0};
    bool stable_cluster_id{false};
    bool resident_page_valid{false};
    bool material_root_preserved{false};
    bool fallback_draw_range_preserved{false};
    bool topology_changing_deformation{false};
    bool runtime_generated_triangle_topology{false};
    bool unbounded_vertex_displacement{false};
    bool touched_native_handles{false};
};

struct MavgDeformationBackendExecutionRow {
    MavgDeformationBackendKind backend{MavgDeformationBackendKind::d3d12};
    std::string_view row_id;
    bool reviewed{false};
    bool execution_evidence{false};
    bool ready{false};
    bool touched_native_handles{false};
};

struct MavgDeformationBackendExecutionEvidenceDesc {
    MavgDeformationBackendKind backend{MavgDeformationBackendKind::d3d12};
    std::string_view row_id;
    bool reviewed{false};
    bool upload_execution_ready{false};
    std::size_t compute_morph_skinned_mesh_bindings{0};
    std::size_t morph_mesh_uploads{0};
    std::uint64_t uploaded_morph_bytes{0};
    std::uint64_t uploaded_compute_morph_base_position_bytes{0};
    std::uint64_t compute_morph_output_position_bytes{0};
    std::size_t submitted_upload_fence_count{0};
    bool renderer_consumption_reviewed{false};
    bool apple_host_execution_evidence{false};
    bool touched_native_handles{false};
    bool request_broad_deformation_readiness{false};
};

struct MavgDeformationIntegrationDesc {
    const MavgClusterGraphDocument* graph{nullptr};
    std::span<const MavgDeformationClusterBoundsRow> cluster_bounds_rows;
    std::span<const MavgDeformationBackendExecutionRow> backend_execution_rows;
    bool require_backend_execution{true};
    bool request_broad_deformation_readiness{false};
};

struct MavgDeformationIntegratedCluster {
    std::uint32_t cluster_index{0};
    MavgDeformationIntegrationKind deformation_kind{MavgDeformationIntegrationKind::rigid_transform};
    MavgBounds3f conservative_bounds;
    std::uint32_t page_index{0};
    std::uint32_t material_partition{0};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
};

struct MavgDeformationIntegrationDiagnostic {
    MavgDeformationIntegrationDiagnosticCode code{MavgDeformationIntegrationDiagnosticCode::none};
    std::uint32_t cluster_index{0};
    std::string row_id;
    std::string message;
};

struct MavgDeformationBackendExecutionEvidenceResult {
    MavgDeformationBackendExecutionRow row;
    std::vector<MavgDeformationIntegrationDiagnostic> diagnostics;
    bool ready{false};
    bool native_handles_exposed{false};
    bool broad_deformation_readiness_ready{false};
};

struct MavgDeformationIntegrationResult {
    std::vector<MavgDeformationIntegratedCluster> integrated_clusters;
    std::vector<MavgDeformationIntegrationDiagnostic> diagnostics;
    std::size_t reviewed_cluster_count{0};
    std::size_t policy_ready_cluster_count{0};
    std::size_t backend_execution_ready_count{0};
    bool mavg_deformation_policy_ready{false};
    bool mavg_deformation_integration_ready{false};
    bool mavg_broad_deformation_readiness_ready{false};
    bool native_handles_exposed{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return mavg_deformation_policy_ready && diagnostics.empty();
    }
};

[[nodiscard]] MavgDeformationIntegrationResult
plan_mavg_deformation_integrated_clusters(const MavgDeformationIntegrationDesc& desc);

[[nodiscard]] MavgDeformationBackendExecutionEvidenceResult
evaluate_mavg_deformation_backend_execution_evidence(const MavgDeformationBackendExecutionEvidenceDesc& desc);

[[nodiscard]] bool has_mavg_deformation_integration_diagnostic(const MavgDeformationIntegrationResult& result,
                                                               MavgDeformationIntegrationDiagnosticCode code) noexcept;

[[nodiscard]] bool
has_mavg_deformation_integration_cluster_diagnostic(const MavgDeformationIntegrationResult& result,
                                                    std::uint32_t cluster_index,
                                                    MavgDeformationIntegrationDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_scene_rhi
