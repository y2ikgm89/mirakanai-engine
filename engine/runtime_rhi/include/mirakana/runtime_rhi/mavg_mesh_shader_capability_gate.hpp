// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgMeshShaderCapabilityQueryKind : std::uint8_t {
    none = 0,
    d3d12_options7_mesh_shader_tier,
    vulkan_ext_mesh_shader_features_properties,
};

enum class RuntimeMavgMeshShaderCapabilityDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_graph_asset,
    missing_backend_rows,
    duplicate_backend_row,
    unsupported_backend,
    missing_feature_query,
    mesh_shader_unsupported,
    missing_required_limits,
    invalid_mesh_output_limits,
    missing_compute_indirect_fallback,
    native_handle_access,
    executed_mesh_shader,
    claimed_backend_execution,
    claimed_metal_readiness,
    claimed_nanite_equivalence,
    claimed_broad_backend_readiness,
};

struct RuntimeMavgMeshShaderCapabilityDiagnostic {
    RuntimeMavgMeshShaderCapabilityDiagnosticCode code{RuntimeMavgMeshShaderCapabilityDiagnosticCode::none};
    AssetId graph_asset;
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::string message;
};

struct RuntimeMavgMeshShaderCapabilityBackendRow {
    rhi::BackendKind backend{rhi::BackendKind::null};
    RuntimeMavgMeshShaderCapabilityQueryKind query_kind{RuntimeMavgMeshShaderCapabilityQueryKind::none};
    bool feature_query_executed{false};
    bool mesh_shader_supported{false};
    bool task_or_amplification_shader_supported{false};
    bool pipeline_statistics_supported{false};
    std::uint32_t max_mesh_workgroup_invocations{0};
    std::uint32_t max_mesh_output_vertices{0};
    std::uint32_t max_mesh_output_primitives{0};
    std::uint32_t max_task_payload_bytes{0};
    std::uint32_t max_shared_memory_bytes{0};
    bool exposed_native_handles{false};
    bool executed_mesh_shader{false};
    bool claimed_backend_execution{false};
    bool executed_backend{false};
    bool claimed_metal_readiness{false};
    bool claimed_nanite_equivalence{false};
    bool claimed_broad_backend_readiness{false};
};

struct RuntimeMavgMeshShaderCapabilityGateDesc {
    AssetId graph_asset;
    std::span<const RuntimeMavgMeshShaderCapabilityBackendRow> backend_rows;
    bool require_d3d12_row{true};
    bool require_vulkan_row{true};
    bool compute_indirect_fallback_ready{false};
};

struct RuntimeMavgMeshShaderCapabilityGateResult {
    std::vector<RuntimeMavgMeshShaderCapabilityDiagnostic> diagnostics;
    std::size_t backend_row_count{0};
    std::size_t ready_backend_count{0};
    std::size_t feature_query_row_count{0};
    std::size_t pipeline_statistics_row_count{0};
    bool d3d12_capability_ready{false};
    bool vulkan_capability_ready{false};
    bool compute_indirect_fallback_ready{false};
    bool fallback_diagnostics_ready{false};
    bool fallback_to_conventional_indexed_draws{false};
    bool capability_gate_ready{false};
    bool mavg_mesh_shader_lod_ready{false};
    bool mavg_mesh_shader_lod_d3d12_ready{false};
    bool mavg_mesh_shader_lod_vulkan_ready{false};
    bool exposed_native_handles{false};
    bool executed_mesh_shader{false};
    bool claimed_backend_execution{false};
    bool executed_backend{false};
    bool claimed_metal_readiness{false};
    bool claimed_nanite_equivalence{false};
    bool claimed_broad_backend_readiness{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && capability_gate_ready;
    }
};

/// Evaluates caller-supplied backend mesh shader feature-query evidence rows before any MAVG mesh shader
/// execution path is allowed. Backend rows must represent official mesh/task shader feature and limit queries. The gate
/// keeps compute/indirect fallback mandatory and does not dispatch mesh shaders, expose native handles, or claim
/// backend execution readiness.
[[nodiscard]] RuntimeMavgMeshShaderCapabilityGateResult
evaluate_runtime_mavg_mesh_shader_capability_gate(const RuntimeMavgMeshShaderCapabilityGateDesc& desc);

[[nodiscard]] bool
has_runtime_mavg_mesh_shader_capability_gate_diagnostic(const RuntimeMavgMeshShaderCapabilityGateResult& result,
                                                        RuntimeMavgMeshShaderCapabilityDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
