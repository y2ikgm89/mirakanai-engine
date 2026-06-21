// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_mesh_shader_capability_gate.hpp"

#include <array>

namespace {

[[nodiscard]] mirakana::AssetId graph_asset() {
    return mirakana::AssetId::from_name("mavg/mesh-shader-capability-gate");
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityBackendRow make_d3d12_row() {
    return mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityBackendRow{
        .backend = mirakana::rhi::BackendKind::d3d12,
        .query_kind = mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityQueryKind::d3d12_options7_mesh_shader_tier,
        .feature_query_executed = true,
        .mesh_shader_supported = true,
        .task_or_amplification_shader_supported = true,
        .pipeline_statistics_supported = true,
        .max_mesh_workgroup_invocations = 128,
        .max_mesh_output_vertices = 256,
        .max_mesh_output_primitives = 128,
        .max_task_payload_bytes = 16U * 1024U,
        .max_shared_memory_bytes = 28U * 1024U,
    };
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityBackendRow make_vulkan_row() {
    return mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityBackendRow{
        .backend = mirakana::rhi::BackendKind::vulkan,
        .query_kind =
            mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityQueryKind::vulkan_ext_mesh_shader_features_properties,
        .feature_query_executed = true,
        .mesh_shader_supported = true,
        .task_or_amplification_shader_supported = true,
        .pipeline_statistics_supported = true,
        .max_mesh_workgroup_invocations = 128,
        .max_mesh_output_vertices = 256,
        .max_mesh_output_primitives = 128,
        .max_task_payload_bytes = 16U * 1024U,
        .max_shared_memory_bytes = 28U * 1024U,
    };
}

} // namespace

MK_TEST("runtime rhi mavg mesh shader capability gate accepts selected D3D12 and Vulkan query rows") {
    const std::array rows{make_d3d12_row(), make_vulkan_row()};

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_mesh_shader_capability_gate(
        mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityGateDesc{
            .graph_asset = graph_asset(),
            .backend_rows = rows,
            .require_d3d12_row = true,
            .require_vulkan_row = true,
            .compute_indirect_fallback_ready = true,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.capability_gate_ready);
    MK_REQUIRE(result.d3d12_capability_ready);
    MK_REQUIRE(result.vulkan_capability_ready);
    MK_REQUIRE(result.compute_indirect_fallback_ready);
    MK_REQUIRE(result.fallback_diagnostics_ready);
    MK_REQUIRE(result.fallback_to_conventional_indexed_draws);
    MK_REQUIRE(result.backend_row_count == 2U);
    MK_REQUIRE(result.ready_backend_count == 2U);
    MK_REQUIRE(result.feature_query_row_count == 2U);
    MK_REQUIRE(result.pipeline_statistics_row_count == 2U);
    MK_REQUIRE(!result.exposed_native_handles);
    MK_REQUIRE(!result.executed_mesh_shader);
    MK_REQUIRE(!result.claimed_backend_execution);
    MK_REQUIRE(!result.executed_backend);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_d3d12_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
    MK_REQUIRE(!result.claimed_metal_readiness);
    MK_REQUIRE(!result.claimed_nanite_equivalence);
    MK_REQUIRE(!result.claimed_broad_backend_readiness);
}

MK_TEST("runtime rhi mavg mesh shader capability gate requires compute indirect fallback") {
    const std::array rows{make_d3d12_row(), make_vulkan_row()};

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_mesh_shader_capability_gate(
        mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityGateDesc{
            .graph_asset = graph_asset(),
            .backend_rows = rows,
            .compute_indirect_fallback_ready = false,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.capability_gate_ready);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_mesh_shader_capability_gate_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_compute_indirect_fallback));
}

MK_TEST("runtime rhi mavg mesh shader capability gate rejects missing feature query and unsupported backend") {
    auto d3d12 = make_d3d12_row();
    d3d12.feature_query_executed = false;
    const auto metal = mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityBackendRow{
        .backend = mirakana::rhi::BackendKind::metal,
        .query_kind = mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityQueryKind::none,
        .feature_query_executed = false,
        .mesh_shader_supported = true,
        .task_or_amplification_shader_supported = true,
        .pipeline_statistics_supported = true,
        .max_mesh_workgroup_invocations = 128,
        .max_mesh_output_vertices = 256,
        .max_mesh_output_primitives = 128,
        .max_task_payload_bytes = 16U * 1024U,
        .max_shared_memory_bytes = 28U * 1024U,
    };
    const std::array rows{d3d12, metal};

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_mesh_shader_capability_gate(
        mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityGateDesc{
            .graph_asset = graph_asset(),
            .backend_rows = rows,
            .require_d3d12_row = true,
            .require_vulkan_row = false,
            .compute_indirect_fallback_ready = true,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_mesh_shader_capability_gate_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_feature_query));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_mesh_shader_capability_gate_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityDiagnosticCode::unsupported_backend));
}

MK_TEST("runtime rhi mavg mesh shader capability gate rejects execution and broad claims") {
    auto d3d12 = make_d3d12_row();
    d3d12.exposed_native_handles = true;
    d3d12.executed_mesh_shader = true;
    d3d12.claimed_backend_execution = true;
    d3d12.executed_backend = true;
    d3d12.claimed_nanite_equivalence = true;
    d3d12.claimed_broad_backend_readiness = true;
    auto vulkan = make_vulkan_row();
    vulkan.claimed_metal_readiness = true;
    const std::array rows{d3d12, vulkan};

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_mesh_shader_capability_gate(
        mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityGateDesc{
            .graph_asset = graph_asset(),
            .backend_rows = rows,
            .compute_indirect_fallback_ready = true,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.exposed_native_handles);
    MK_REQUIRE(result.executed_mesh_shader);
    MK_REQUIRE(result.claimed_backend_execution);
    MK_REQUIRE(result.executed_backend);
    MK_REQUIRE(result.claimed_metal_readiness);
    MK_REQUIRE(result.claimed_nanite_equivalence);
    MK_REQUIRE(result.claimed_broad_backend_readiness);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_mesh_shader_capability_gate_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityDiagnosticCode::native_handle_access));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_mesh_shader_capability_gate_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityDiagnosticCode::executed_mesh_shader));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_mesh_shader_capability_gate_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityDiagnosticCode::claimed_broad_backend_readiness));
}

MK_TEST("runtime rhi mavg mesh shader capability gate rejects invalid mesh output limits") {
    auto d3d12 = make_d3d12_row();
    d3d12.max_mesh_output_vertices = 1;
    auto vulkan = make_vulkan_row();
    vulkan.max_mesh_output_primitives = 0;
    const std::array rows{d3d12, vulkan};

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_mesh_shader_capability_gate(
        mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityGateDesc{
            .graph_asset = graph_asset(),
            .backend_rows = rows,
            .compute_indirect_fallback_ready = true,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_mesh_shader_capability_gate_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgMeshShaderCapabilityDiagnosticCode::invalid_mesh_output_limits));
}

int main() {
    return mirakana::test::run_all();
}
