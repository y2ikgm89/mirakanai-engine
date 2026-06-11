// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_mesh_shader_policy.hpp"

#include <algorithm>
#include <span>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgMeshShaderOutputShape make_required_shape() noexcept {
    return mirakana::MavgMeshShaderOutputShape{
        .max_output_vertices = 128,
        .max_output_primitives = 128,
        .threadgroup_size = 64,
        .payload_bytes = 4096,
        .output_memory_bytes = 16384,
    };
}

[[nodiscard]] std::vector<mirakana::MavgMeshShaderBackendCapabilityRow> make_supported_capabilities() {
    return {
        mirakana::MavgMeshShaderBackendCapabilityRow{
            .backend = mirakana::MavgMeshShaderBackend::d3d12,
            .mesh_shader_supported = true,
            .task_shader_supported = true,
            .pipeline_statistics_supported = true,
            .max_output_vertices = 256,
            .max_output_primitives = 256,
            .max_threadgroup_size = 128,
            .max_payload_bytes = 16384,
            .max_output_memory_bytes = 32768,
            .max_payload_and_output_memory_bytes = 49152,
        },
        mirakana::MavgMeshShaderBackendCapabilityRow{
            .backend = mirakana::MavgMeshShaderBackend::vulkan,
            .mesh_shader_supported = true,
            .task_shader_supported = true,
            .pipeline_statistics_supported = true,
            .max_output_vertices = 256,
            .max_output_primitives = 256,
            .max_threadgroup_size = 128,
            .max_payload_bytes = 16384,
            .max_output_memory_bytes = 32768,
            .max_payload_and_output_memory_bytes = 49152,
        },
    };
}

[[nodiscard]] bool has_backend_plan(const mirakana::MavgMeshShaderCapabilityPlan& plan,
                                    mirakana::MavgMeshShaderBackend backend) {
    return std::ranges::any_of(plan.backend_rows, [backend](const mirakana::MavgMeshShaderBackendPlanRow& row) {
        return row.backend == backend;
    });
}

[[nodiscard]] bool has_diagnostic(const mirakana::MavgMeshShaderCapabilityPlan& plan,
                                  mirakana::MavgMeshShaderDiagnosticCode code) {
    return std::ranges::any_of(plan.diagnostics, [code](const mirakana::MavgMeshShaderDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

[[nodiscard]] const mirakana::MavgMeshShaderBackendPlanRow*
find_backend_plan(const mirakana::MavgMeshShaderCapabilityPlan& plan, mirakana::MavgMeshShaderBackend backend) {
    const auto found =
        std::ranges::find_if(plan.backend_rows, [backend](const mirakana::MavgMeshShaderBackendPlanRow& row) {
            return row.backend == backend;
        });
    return found == plan.backend_rows.end() ? nullptr : &*found;
}

} // namespace

MK_TEST("mavg mesh shader capability planner selects mesh shader path for reviewed d3d12 and vulkan support") {
    const auto capabilities = make_supported_capabilities();

    const auto plan = mirakana::plan_mavg_mesh_shader_capability(mirakana::MavgMeshShaderCapabilityDesc{
        .backend_capabilities = std::span<const mirakana::MavgMeshShaderBackendCapabilityRow>{capabilities},
        .required_shape = make_required_shape(),
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.path == mirakana::MavgMeshShaderPathKind::mesh_shader);
    MK_REQUIRE(plan.selected_mesh_shader_path);
    MK_REQUIRE(!plan.selected_compute_indirect_fallback);
    MK_REQUIRE(!plan.executed_mesh_shader);
    MK_REQUIRE(!plan.touched_native_handles);
    MK_REQUIRE(plan.reviewed_backend_count == 2U);
    MK_REQUIRE(plan.mesh_shader_ready_backend_count == 2U);
    MK_REQUIRE(plan.backend_rows.size() == 2U);
    MK_REQUIRE(has_backend_plan(plan, mirakana::MavgMeshShaderBackend::d3d12));
    MK_REQUIRE(has_backend_plan(plan, mirakana::MavgMeshShaderBackend::vulkan));
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("mavg mesh shader capability planner falls back when d3d12 support is missing") {
    auto capabilities = make_supported_capabilities();
    capabilities[0].mesh_shader_supported = false;

    const auto plan = mirakana::plan_mavg_mesh_shader_capability(mirakana::MavgMeshShaderCapabilityDesc{
        .backend_capabilities = std::span<const mirakana::MavgMeshShaderBackendCapabilityRow>{capabilities},
        .required_shape = make_required_shape(),
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.path == mirakana::MavgMeshShaderPathKind::compute_indirect_fallback);
    MK_REQUIRE(!plan.selected_mesh_shader_path);
    MK_REQUIRE(plan.selected_compute_indirect_fallback);
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::d3d12_mesh_shader_unsupported));
}

MK_TEST("mavg mesh shader capability planner falls back when vulkan extension support is missing") {
    auto capabilities = make_supported_capabilities();
    capabilities[1].mesh_shader_supported = false;

    const auto plan = mirakana::plan_mavg_mesh_shader_capability(mirakana::MavgMeshShaderCapabilityDesc{
        .backend_capabilities = std::span<const mirakana::MavgMeshShaderBackendCapabilityRow>{capabilities},
        .required_shape = make_required_shape(),
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.path == mirakana::MavgMeshShaderPathKind::compute_indirect_fallback);
    MK_REQUIRE(!plan.selected_mesh_shader_path);
    MK_REQUIRE(plan.selected_compute_indirect_fallback);
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::vulkan_mesh_shader_unsupported));
}

MK_TEST("mavg mesh shader capability planner falls back when output limits are insufficient") {
    auto capabilities = make_supported_capabilities();
    capabilities[0].max_output_vertices = 64;
    capabilities[0].max_output_primitives = 64;
    capabilities[0].max_threadgroup_size = 32;
    capabilities[0].max_payload_bytes = 1024;
    capabilities[0].max_output_memory_bytes = 8192;
    capabilities[0].max_payload_and_output_memory_bytes = 8192;

    const auto plan = mirakana::plan_mavg_mesh_shader_capability(mirakana::MavgMeshShaderCapabilityDesc{
        .backend_capabilities = std::span<const mirakana::MavgMeshShaderBackendCapabilityRow>{capabilities},
        .required_shape = make_required_shape(),
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.path == mirakana::MavgMeshShaderPathKind::compute_indirect_fallback);
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::output_vertex_limit_insufficient));
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::output_primitive_limit_insufficient));
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::threadgroup_limit_insufficient));
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::payload_limit_insufficient));
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::output_memory_limit_insufficient));
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::payload_output_memory_limit_insufficient));
}

MK_TEST("mavg mesh shader capability planner records d3d12 pipeline statistics as nonblocking diagnostic") {
    auto capabilities = make_supported_capabilities();
    capabilities[0].pipeline_statistics_supported = false;

    const auto plan = mirakana::plan_mavg_mesh_shader_capability(mirakana::MavgMeshShaderCapabilityDesc{
        .backend_capabilities = std::span<const mirakana::MavgMeshShaderBackendCapabilityRow>{capabilities},
        .required_shape = make_required_shape(),
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.path == mirakana::MavgMeshShaderPathKind::mesh_shader);
    MK_REQUIRE(plan.selected_mesh_shader_path);
    MK_REQUIRE(!plan.selected_compute_indirect_fallback);
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::d3d12_pipeline_statistics_unavailable));
}

MK_TEST("mavg mesh shader capability planner keeps missing backend rows for deterministic fallback evidence") {
    const auto supported = make_supported_capabilities();
    const auto capabilities = std::vector<mirakana::MavgMeshShaderBackendCapabilityRow>{supported[1]};

    const auto plan = mirakana::plan_mavg_mesh_shader_capability(mirakana::MavgMeshShaderCapabilityDesc{
        .backend_capabilities = std::span<const mirakana::MavgMeshShaderBackendCapabilityRow>{capabilities},
        .required_shape = make_required_shape(),
    });

    const auto* d3d12_row = find_backend_plan(plan, mirakana::MavgMeshShaderBackend::d3d12);
    const auto* vulkan_row = find_backend_plan(plan, mirakana::MavgMeshShaderBackend::vulkan);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.path == mirakana::MavgMeshShaderPathKind::compute_indirect_fallback);
    MK_REQUIRE(plan.backend_rows.size() == 2U);
    MK_REQUIRE(d3d12_row != nullptr);
    MK_REQUIRE(!d3d12_row->reviewed);
    MK_REQUIRE(vulkan_row != nullptr);
    MK_REQUIRE(vulkan_row->reviewed);
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::missing_backend_capability));
}

MK_TEST("mavg mesh shader capability planner falls back when combined payload output memory is insufficient") {
    auto capabilities = make_supported_capabilities();
    capabilities[1].max_payload_and_output_memory_bytes = 8192;

    const auto plan = mirakana::plan_mavg_mesh_shader_capability(mirakana::MavgMeshShaderCapabilityDesc{
        .backend_capabilities = std::span<const mirakana::MavgMeshShaderBackendCapabilityRow>{capabilities},
        .required_shape = make_required_shape(),
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.path == mirakana::MavgMeshShaderPathKind::compute_indirect_fallback);
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::payload_output_memory_limit_insufficient));
}

MK_TEST("mavg mesh shader capability planner fails closed on invalid required output shape") {
    const auto capabilities = make_supported_capabilities();

    const auto plan = mirakana::plan_mavg_mesh_shader_capability(mirakana::MavgMeshShaderCapabilityDesc{
        .backend_capabilities = std::span<const mirakana::MavgMeshShaderBackendCapabilityRow>{capabilities},
        .required_shape = mirakana::MavgMeshShaderOutputShape{},
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.path == mirakana::MavgMeshShaderPathKind::compute_indirect_fallback);
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::invalid_required_shape));
}

MK_TEST("mavg mesh shader capability planner fails closed on duplicate backend capability rows") {
    auto capabilities = make_supported_capabilities();
    capabilities.push_back(capabilities[0]);

    const auto plan = mirakana::plan_mavg_mesh_shader_capability(mirakana::MavgMeshShaderCapabilityDesc{
        .backend_capabilities = std::span<const mirakana::MavgMeshShaderBackendCapabilityRow>{capabilities},
        .required_shape = make_required_shape(),
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.path == mirakana::MavgMeshShaderPathKind::compute_indirect_fallback);
    MK_REQUIRE(has_diagnostic(plan, mirakana::MavgMeshShaderDiagnosticCode::duplicate_backend_capability));
}

int main() {
    return mirakana::test::run_all();
}
