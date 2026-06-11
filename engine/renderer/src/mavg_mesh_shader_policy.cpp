// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/mavg_mesh_shader_policy.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <utility>

namespace mirakana {
namespace {

constexpr std::array required_backends{
    MavgMeshShaderBackend::d3d12,
    MavgMeshShaderBackend::vulkan,
};

void add_diagnostic(MavgMeshShaderCapabilityPlan& plan, MavgMeshShaderDiagnosticCode code,
                    MavgMeshShaderBackend backend, bool blocking, std::string message) {
    plan.diagnostics.push_back(MavgMeshShaderDiagnostic{
        .code = code,
        .backend = backend,
        .blocking = blocking,
        .message = std::move(message),
    });
}

[[nodiscard]] bool same_backend(const MavgMeshShaderBackendCapabilityRow& row, MavgMeshShaderBackend backend) noexcept {
    return row.backend == backend;
}

[[nodiscard]] std::size_t backend_count(std::span<const MavgMeshShaderBackendCapabilityRow> rows,
                                        MavgMeshShaderBackend backend) noexcept {
    return static_cast<std::size_t>(std::ranges::count_if(
        rows, [backend](const MavgMeshShaderBackendCapabilityRow& row) { return same_backend(row, backend); }));
}

[[nodiscard]] const MavgMeshShaderBackendCapabilityRow*
find_backend(std::span<const MavgMeshShaderBackendCapabilityRow> rows, MavgMeshShaderBackend backend) noexcept {
    for (const auto& row : rows) {
        if (same_backend(row, backend)) {
            return &row;
        }
    }
    return nullptr;
}

[[nodiscard]] bool valid_required_shape(MavgMeshShaderOutputShape shape) noexcept {
    return shape.max_output_vertices > 0U && shape.max_output_primitives > 0U && shape.threadgroup_size > 0U &&
           shape.output_memory_bytes > 0U;
}

[[nodiscard]] bool limits_satisfy_required_shape(const MavgMeshShaderBackendCapabilityRow& row,
                                                 MavgMeshShaderOutputShape shape) noexcept {
    return row.max_output_vertices >= shape.max_output_vertices &&
           row.max_output_primitives >= shape.max_output_primitives &&
           row.max_threadgroup_size >= shape.threadgroup_size && row.max_payload_bytes >= shape.payload_bytes &&
           row.max_output_memory_bytes >= shape.output_memory_bytes &&
           row.max_payload_and_output_memory_bytes >= (shape.payload_bytes + shape.output_memory_bytes);
}

void append_limit_diagnostics(MavgMeshShaderCapabilityPlan& plan, const MavgMeshShaderBackendCapabilityRow& row,
                              MavgMeshShaderOutputShape shape) {
    if (row.max_output_vertices < shape.max_output_vertices) {
        add_diagnostic(plan, MavgMeshShaderDiagnosticCode::output_vertex_limit_insufficient, row.backend, true,
                       "MAVG mesh shader required output vertex count exceeds backend limit");
    }
    if (row.max_output_primitives < shape.max_output_primitives) {
        add_diagnostic(plan, MavgMeshShaderDiagnosticCode::output_primitive_limit_insufficient, row.backend, true,
                       "MAVG mesh shader required output primitive count exceeds backend limit");
    }
    if (row.max_threadgroup_size < shape.threadgroup_size) {
        add_diagnostic(plan, MavgMeshShaderDiagnosticCode::threadgroup_limit_insufficient, row.backend, true,
                       "MAVG mesh shader required threadgroup size exceeds backend limit");
    }
    if (row.max_payload_bytes < shape.payload_bytes) {
        add_diagnostic(plan, MavgMeshShaderDiagnosticCode::payload_limit_insufficient, row.backend, true,
                       "MAVG mesh shader required payload bytes exceed backend limit");
    }
    if (row.max_output_memory_bytes < shape.output_memory_bytes) {
        add_diagnostic(plan, MavgMeshShaderDiagnosticCode::output_memory_limit_insufficient, row.backend, true,
                       "MAVG mesh shader required output memory bytes exceed backend limit");
    }
    if (row.max_payload_and_output_memory_bytes < (shape.payload_bytes + shape.output_memory_bytes)) {
        add_diagnostic(plan, MavgMeshShaderDiagnosticCode::payload_output_memory_limit_insufficient, row.backend, true,
                       "MAVG mesh shader required payload plus output memory bytes exceed backend combined limit");
    }
}

[[nodiscard]] MavgMeshShaderDiagnosticCode unsupported_code(MavgMeshShaderBackend backend) noexcept {
    return backend == MavgMeshShaderBackend::d3d12 ? MavgMeshShaderDiagnosticCode::d3d12_mesh_shader_unsupported
                                                   : MavgMeshShaderDiagnosticCode::vulkan_mesh_shader_unsupported;
}

[[nodiscard]] const char* unsupported_message(MavgMeshShaderBackend backend) noexcept {
    return backend == MavgMeshShaderBackend::d3d12
               ? "D3D12 MAVG mesh shader path requires reviewed mesh/amplification shader support"
               : "Vulkan MAVG mesh shader path requires reviewed VK_EXT_mesh_shader task/mesh support";
}

[[nodiscard]] bool has_blocking_diagnostic(const MavgMeshShaderCapabilityPlan& plan) noexcept {
    return std::ranges::any_of(plan.diagnostics,
                               [](const MavgMeshShaderDiagnostic& diagnostic) { return diagnostic.blocking; });
}

} // namespace

bool MavgMeshShaderCapabilityPlan::succeeded() const noexcept {
    return !has_blocking_diagnostic(*this);
}

MavgMeshShaderCapabilityPlan plan_mavg_mesh_shader_capability(const MavgMeshShaderCapabilityDesc& desc) {
    MavgMeshShaderCapabilityPlan plan;
    plan.reviewed_backend_count = static_cast<std::uint32_t>(desc.backend_capabilities.size());

    if (!valid_required_shape(desc.required_shape)) {
        add_diagnostic(plan, MavgMeshShaderDiagnosticCode::invalid_required_shape, MavgMeshShaderBackend::d3d12, true,
                       "MAVG mesh shader planning requires non-zero output vertices, primitives, threadgroup, and "
                       "output memory");
        return plan;
    }

    for (const auto backend : required_backends) {
        const auto count = backend_count(desc.backend_capabilities, backend);
        if (count == 0U) {
            plan.backend_rows.push_back(MavgMeshShaderBackendPlanRow{
                .backend = backend,
                .reviewed = false,
            });
            add_diagnostic(plan, MavgMeshShaderDiagnosticCode::missing_backend_capability, backend, true,
                           "MAVG mesh shader planning requires reviewed D3D12 and Vulkan capability rows");
            continue;
        }
        if (count > 1U) {
            plan.backend_rows.push_back(MavgMeshShaderBackendPlanRow{
                .backend = backend,
                .reviewed = false,
            });
            add_diagnostic(plan, MavgMeshShaderDiagnosticCode::duplicate_backend_capability, backend, true,
                           "MAVG mesh shader planning requires at most one capability row per backend");
            continue;
        }

        const auto* capability = find_backend(desc.backend_capabilities, backend);
        if (capability == nullptr) {
            continue;
        }

        const bool supported = capability->mesh_shader_supported && capability->task_shader_supported;
        const bool limits_ready = limits_satisfy_required_shape(*capability, desc.required_shape);
        plan.backend_rows.push_back(MavgMeshShaderBackendPlanRow{
            .backend = capability->backend,
            .reviewed = true,
            .mesh_shader_supported = capability->mesh_shader_supported,
            .task_shader_supported = capability->task_shader_supported,
            .pipeline_statistics_supported = capability->pipeline_statistics_supported,
            .limits_satisfy_required_shape = limits_ready,
            .max_output_vertices = capability->max_output_vertices,
            .max_output_primitives = capability->max_output_primitives,
            .max_threadgroup_size = capability->max_threadgroup_size,
            .max_payload_bytes = capability->max_payload_bytes,
            .max_output_memory_bytes = capability->max_output_memory_bytes,
            .max_payload_and_output_memory_bytes = capability->max_payload_and_output_memory_bytes,
        });

        if (!supported) {
            add_diagnostic(plan, unsupported_code(backend), backend, true, unsupported_message(backend));
        }
        if (backend == MavgMeshShaderBackend::d3d12 && !capability->pipeline_statistics_supported) {
            add_diagnostic(plan, MavgMeshShaderDiagnosticCode::d3d12_pipeline_statistics_unavailable, backend, false,
                           "D3D12 mesh shader pipeline statistics support is separately queryable and unavailable");
        }
        if (!limits_ready) {
            append_limit_diagnostics(plan, *capability, desc.required_shape);
        }

        if (supported && limits_ready) {
            ++plan.mesh_shader_ready_backend_count;
        }
    }

    if (plan.succeeded() && plan.mesh_shader_ready_backend_count == required_backends.size()) {
        plan.path = MavgMeshShaderPathKind::mesh_shader;
        plan.selected_mesh_shader_path = true;
        plan.selected_compute_indirect_fallback = false;
    }

    return plan;
}

bool has_mavg_mesh_shader_diagnostic(const MavgMeshShaderCapabilityPlan& plan,
                                     MavgMeshShaderDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics,
                               [code](const MavgMeshShaderDiagnostic& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
