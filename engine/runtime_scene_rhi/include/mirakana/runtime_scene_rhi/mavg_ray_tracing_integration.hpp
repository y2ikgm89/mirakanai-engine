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

enum class MavgRayTracingGeometryPolicy : std::uint8_t {
    resident_cluster_blas = 0,
    fallback_mesh_blas,
};

enum class MavgRayTracingBackendKind : std::uint8_t {
    d3d12 = 0,
    vulkan,
    metal_apple_host,
};

enum class MavgRayTracingIndexFormat : std::uint8_t {
    uint16 = 0,
    uint32,
};

enum class MavgRayTracingMaterialAlphaPolicy : std::uint8_t {
    opaque = 0,
    alpha_tested,
    alpha_blended_rejected,
};

enum class MavgRayTracingIntegrationDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_graph,
    missing_blas_input_row,
    duplicate_blas_input_row,
    unknown_cluster,
    unstable_cluster_id,
    missing_replay_hash,
    invalid_geometry_byte_count,
    invalid_index_format,
    invalid_transform_row,
    invalid_material_alpha_policy,
    implicit_geometry_mode_switch,
    fallback_mode_mismatch,
    resident_page_invalid,
    backend_execution_required,
    backend_execution_not_ready,
    d3d12_dxr_feature_not_ready,
    d3d12_acceleration_structure_build_missing,
    vulkan_acceleration_structure_feature_not_ready,
    vulkan_acceleration_structure_build_missing,
    metal_readiness_not_promoted,
    native_handle_access,
    broad_ray_tracing_readiness_not_promoted,
};

struct MavgRayTracingBlasInputRow {
    std::uint32_t cluster_index{0};
    MavgRayTracingGeometryPolicy geometry_policy{MavgRayTracingGeometryPolicy::resident_cluster_blas};
    std::string_view replay_hash;
    std::uint64_t geometry_byte_count{0};
    MavgRayTracingIndexFormat index_format{MavgRayTracingIndexFormat::uint32};
    std::uint32_t page_index{0};
    std::uint32_t transform_row_id{0};
    MavgRayTracingMaterialAlphaPolicy material_alpha_policy{MavgRayTracingMaterialAlphaPolicy::opaque};
    bool stable_cluster_id{false};
    bool resident_page_valid{false};
    bool transform_row_valid{false};
    bool uses_resident_cluster_geometry{false};
    bool uses_fallback_mesh_geometry{false};
    bool fallback_mesh_matches_cluster{false};
    bool touched_native_handles{false};
};

struct MavgRayTracingBackendExecutionRow {
    MavgRayTracingBackendKind backend{MavgRayTracingBackendKind::d3d12};
    std::string_view row_id;
    bool reviewed{false};
    bool feature_query_ready{false};
    bool acceleration_structure_build_evidence{false};
    bool ready{false};
    bool touched_native_handles{false};
};

struct MavgRayTracingIntegrationDesc {
    const MavgClusterGraphDocument* graph{nullptr};
    std::span<const MavgRayTracingBlasInputRow> blas_input_rows;
    std::span<const MavgRayTracingBackendExecutionRow> backend_execution_rows;
    bool require_backend_execution{true};
    bool request_metal_readiness{false};
    bool request_broad_ray_tracing_readiness{false};
};

struct MavgRayTracingBlasInput {
    std::uint32_t cluster_index{0};
    MavgRayTracingGeometryPolicy geometry_policy{MavgRayTracingGeometryPolicy::resident_cluster_blas};
    std::string replay_hash;
    std::uint64_t geometry_byte_count{0};
    MavgRayTracingIndexFormat index_format{MavgRayTracingIndexFormat::uint32};
    std::uint32_t transform_row_id{0};
    MavgRayTracingMaterialAlphaPolicy material_alpha_policy{MavgRayTracingMaterialAlphaPolicy::opaque};
};

struct MavgRayTracingIntegrationDiagnostic {
    MavgRayTracingIntegrationDiagnosticCode code{MavgRayTracingIntegrationDiagnosticCode::none};
    std::uint32_t cluster_index{0};
    std::string row_id;
    std::string message;
};

struct MavgRayTracingIntegrationResult {
    std::vector<MavgRayTracingBlasInput> blas_inputs;
    std::vector<MavgRayTracingIntegrationDiagnostic> diagnostics;
    std::size_t reviewed_blas_input_count{0};
    std::size_t policy_ready_blas_input_count{0};
    std::size_t backend_execution_ready_count{0};
    bool mavg_ray_tracing_policy_ready{false};
    bool mavg_ray_tracing_integration_ready{false};
    bool mavg_metal_ray_tracing_ready{false};
    bool mavg_broad_ray_tracing_readiness_ready{false};
    bool native_handles_exposed{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return mavg_ray_tracing_policy_ready && diagnostics.empty();
    }
};

[[nodiscard]] MavgRayTracingIntegrationResult
plan_mavg_ray_tracing_blas_inputs(const MavgRayTracingIntegrationDesc& desc);

[[nodiscard]] bool has_mavg_ray_tracing_integration_diagnostic(const MavgRayTracingIntegrationResult& result,
                                                               MavgRayTracingIntegrationDiagnosticCode code) noexcept;

[[nodiscard]] bool
has_mavg_ray_tracing_integration_cluster_diagnostic(const MavgRayTracingIntegrationResult& result,
                                                    std::uint32_t cluster_index,
                                                    MavgRayTracingIntegrationDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_scene_rhi
