// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_mesh_shader_capability_gate.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgMeshShaderCapabilityGateResult& result,
                    RuntimeMavgMeshShaderCapabilityDiagnosticCode code, AssetId graph_asset, rhi::BackendKind backend,
                    std::string message) {
    result.diagnostics.push_back(RuntimeMavgMeshShaderCapabilityDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .backend = backend,
        .message = std::move(message),
    });
}

[[nodiscard]] RuntimeMavgMeshShaderCapabilityQueryKind required_query_kind(rhi::BackendKind backend) noexcept {
    switch (backend) {
    case rhi::BackendKind::d3d12:
        return RuntimeMavgMeshShaderCapabilityQueryKind::d3d12_options7_mesh_shader_tier;
    case rhi::BackendKind::vulkan:
        return RuntimeMavgMeshShaderCapabilityQueryKind::vulkan_ext_mesh_shader_features_properties;
    case rhi::BackendKind::null:
    case rhi::BackendKind::metal:
        return RuntimeMavgMeshShaderCapabilityQueryKind::none;
    }
    return RuntimeMavgMeshShaderCapabilityQueryKind::none;
}

[[nodiscard]] bool is_supported_mesh_shader_backend(rhi::BackendKind backend) noexcept {
    return backend == rhi::BackendKind::d3d12 || backend == rhi::BackendKind::vulkan;
}

[[nodiscard]] bool has_required_limits(const RuntimeMavgMeshShaderCapabilityBackendRow& row) noexcept {
    return row.max_mesh_workgroup_invocations > 0U && row.max_mesh_output_vertices > 0U &&
           row.max_mesh_output_primitives > 0U && row.max_task_payload_bytes > 0U && row.max_shared_memory_bytes > 0U;
}

[[nodiscard]] bool has_invalid_output_limits(const RuntimeMavgMeshShaderCapabilityBackendRow& row) noexcept {
    return row.max_mesh_output_vertices < 3U || row.max_mesh_output_primitives == 0U;
}

[[nodiscard]] bool backend_row_ready(const RuntimeMavgMeshShaderCapabilityBackendRow& row) noexcept {
    return is_supported_mesh_shader_backend(row.backend) && row.query_kind == required_query_kind(row.backend) &&
           row.feature_query_executed && row.mesh_shader_supported && row.task_or_amplification_shader_supported &&
           row.pipeline_statistics_supported && has_required_limits(row) && !has_invalid_output_limits(row) &&
           !row.exposed_native_handles && !row.executed_mesh_shader && !row.claimed_backend_execution &&
           !row.executed_backend && !row.claimed_metal_readiness && !row.claimed_nanite_equivalence &&
           !row.claimed_broad_backend_readiness;
}

void validate_backend_row(RuntimeMavgMeshShaderCapabilityGateResult& result,
                          const RuntimeMavgMeshShaderCapabilityGateDesc& desc,
                          const RuntimeMavgMeshShaderCapabilityBackendRow& row) {
    if (!is_supported_mesh_shader_backend(row.backend)) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::unsupported_backend, desc.graph_asset,
                       row.backend, "MAVG mesh shader capability gate currently accepts only D3D12 and Vulkan rows");
    }
    if (row.query_kind != required_query_kind(row.backend) || !row.feature_query_executed) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_feature_query, desc.graph_asset,
                       row.backend, "MAVG mesh shader capability row must use the backend's official feature query");
    }
    if (!row.mesh_shader_supported || !row.task_or_amplification_shader_supported) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::mesh_shader_unsupported, desc.graph_asset,
                       row.backend, "MAVG mesh shader capability row must prove mesh and task/amplification support");
    }
    if (!has_required_limits(row)) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_required_limits, desc.graph_asset,
                       row.backend, "MAVG mesh shader capability row must include non-zero output and memory limits");
    } else if (has_invalid_output_limits(row)) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::invalid_mesh_output_limits,
                       desc.graph_asset, row.backend,
                       "MAVG mesh shader capability row must expose at least one triangle worth of output capacity");
    }
    if (row.exposed_native_handles) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::native_handle_access, desc.graph_asset,
                       row.backend, "MAVG mesh shader capability gate must not expose native handles");
    }
    if (row.executed_mesh_shader) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::executed_mesh_shader, desc.graph_asset,
                       row.backend, "MAVG mesh shader capability gate must not execute mesh shaders");
    }
    if (row.claimed_backend_execution || row.executed_backend) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::claimed_backend_execution,
                       desc.graph_asset, row.backend,
                       "MAVG mesh shader capability gate must not claim backend execution readiness");
    }
    if (row.claimed_metal_readiness) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::claimed_metal_readiness, desc.graph_asset,
                       row.backend, "MAVG mesh shader capability gate must not claim Metal readiness");
    }
    if (row.claimed_nanite_equivalence) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::claimed_nanite_equivalence,
                       desc.graph_asset, row.backend,
                       "MAVG mesh shader capability gate must not claim Nanite equivalence or superiority");
    }
    if (row.claimed_broad_backend_readiness) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::claimed_broad_backend_readiness,
                       desc.graph_asset, row.backend,
                       "MAVG mesh shader capability gate must not claim broad MAVG backend readiness");
    }
}

} // namespace

RuntimeMavgMeshShaderCapabilityGateResult
evaluate_runtime_mavg_mesh_shader_capability_gate(const RuntimeMavgMeshShaderCapabilityGateDesc& desc) {
    RuntimeMavgMeshShaderCapabilityGateResult result;
    result.backend_row_count = desc.backend_rows.size();
    result.compute_indirect_fallback_ready = desc.compute_indirect_fallback_ready;
    result.fallback_diagnostics_ready = desc.compute_indirect_fallback_ready;
    result.fallback_to_conventional_indexed_draws = desc.compute_indirect_fallback_ready;

    if (desc.graph_asset.value == 0U) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::invalid_graph_asset, desc.graph_asset,
                       rhi::BackendKind::null, "MAVG mesh shader capability gate graph asset id must be non-zero");
    }
    if (desc.backend_rows.empty()) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_backend_rows, desc.graph_asset,
                       rhi::BackendKind::null, "MAVG mesh shader capability gate requires backend evidence rows");
    }
    if (!desc.compute_indirect_fallback_ready) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_compute_indirect_fallback,
                       desc.graph_asset, rhi::BackendKind::null,
                       "MAVG mesh shader capability gate requires the compute/indirect fallback to remain ready");
    }

    bool seen_d3d12 = false;
    bool seen_vulkan = false;
    for (const auto& row : desc.backend_rows) {
        result.exposed_native_handles = result.exposed_native_handles || row.exposed_native_handles;
        result.executed_mesh_shader = result.executed_mesh_shader || row.executed_mesh_shader;
        result.claimed_backend_execution = result.claimed_backend_execution || row.claimed_backend_execution;
        result.executed_backend = result.executed_backend || row.executed_backend;
        result.claimed_metal_readiness = result.claimed_metal_readiness || row.claimed_metal_readiness;
        result.claimed_nanite_equivalence = result.claimed_nanite_equivalence || row.claimed_nanite_equivalence;
        result.claimed_broad_backend_readiness =
            result.claimed_broad_backend_readiness || row.claimed_broad_backend_readiness;
        if (row.feature_query_executed) {
            ++result.feature_query_row_count;
        }
        if (row.pipeline_statistics_supported) {
            ++result.pipeline_statistics_row_count;
        }

        if (row.backend == rhi::BackendKind::d3d12) {
            if (seen_d3d12) {
                add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::duplicate_backend_row,
                               desc.graph_asset, row.backend, "MAVG mesh shader capability gate accepts one D3D12 row");
            }
            seen_d3d12 = true;
        }
        if (row.backend == rhi::BackendKind::vulkan) {
            if (seen_vulkan) {
                add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::duplicate_backend_row,
                               desc.graph_asset, row.backend,
                               "MAVG mesh shader capability gate accepts one Vulkan row");
            }
            seen_vulkan = true;
        }

        validate_backend_row(result, desc, row);
        if (backend_row_ready(row)) {
            ++result.ready_backend_count;
            result.d3d12_capability_ready = result.d3d12_capability_ready || row.backend == rhi::BackendKind::d3d12;
            result.vulkan_capability_ready = result.vulkan_capability_ready || row.backend == rhi::BackendKind::vulkan;
        }
    }

    if (desc.require_d3d12_row && !seen_d3d12) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_backend_rows, desc.graph_asset,
                       rhi::BackendKind::d3d12, "MAVG mesh shader capability gate requires a D3D12 row");
    }
    if (desc.require_vulkan_row && !seen_vulkan) {
        add_diagnostic(result, RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_backend_rows, desc.graph_asset,
                       rhi::BackendKind::vulkan, "MAVG mesh shader capability gate requires a Vulkan row");
    }

    result.capability_gate_ready = result.diagnostics.empty() && desc.compute_indirect_fallback_ready &&
                                   (!desc.require_d3d12_row || result.d3d12_capability_ready) &&
                                   (!desc.require_vulkan_row || result.vulkan_capability_ready);
    return result;
}

bool has_runtime_mavg_mesh_shader_capability_gate_diagnostic(
    const RuntimeMavgMeshShaderCapabilityGateResult& result,
    RuntimeMavgMeshShaderCapabilityDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const RuntimeMavgMeshShaderCapabilityDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

} // namespace mirakana::runtime_rhi
