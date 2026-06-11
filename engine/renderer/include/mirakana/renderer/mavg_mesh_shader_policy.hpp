// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgMeshShaderBackend : std::uint8_t {
    d3d12,
    vulkan,
};

enum class MavgMeshShaderPathKind : std::uint8_t {
    compute_indirect_fallback,
    mesh_shader,
};

enum class MavgMeshShaderDiagnosticCode : std::uint8_t {
    invalid_required_shape,
    missing_backend_capability,
    duplicate_backend_capability,
    d3d12_mesh_shader_unsupported,
    vulkan_mesh_shader_unsupported,
    d3d12_pipeline_statistics_unavailable,
    output_vertex_limit_insufficient,
    output_primitive_limit_insufficient,
    threadgroup_limit_insufficient,
    payload_limit_insufficient,
    output_memory_limit_insufficient,
    payload_output_memory_limit_insufficient,
};

struct MavgMeshShaderBackendCapabilityRow {
    MavgMeshShaderBackend backend{MavgMeshShaderBackend::d3d12};
    bool mesh_shader_supported{false};
    bool task_shader_supported{false};
    bool pipeline_statistics_supported{false};
    std::uint32_t max_output_vertices{0};
    std::uint32_t max_output_primitives{0};
    std::uint32_t max_threadgroup_size{0};
    std::uint32_t max_payload_bytes{0};
    std::uint32_t max_output_memory_bytes{0};
    std::uint32_t max_payload_and_output_memory_bytes{0};
};

struct MavgMeshShaderOutputShape {
    std::uint32_t max_output_vertices{0};
    std::uint32_t max_output_primitives{0};
    std::uint32_t threadgroup_size{0};
    std::uint32_t payload_bytes{0};
    std::uint32_t output_memory_bytes{0};
};

struct MavgMeshShaderCapabilityDesc {
    std::span<const MavgMeshShaderBackendCapabilityRow> backend_capabilities;
    MavgMeshShaderOutputShape required_shape;
};

struct MavgMeshShaderBackendPlanRow {
    MavgMeshShaderBackend backend{MavgMeshShaderBackend::d3d12};
    bool reviewed{false};
    bool mesh_shader_supported{false};
    bool task_shader_supported{false};
    bool pipeline_statistics_supported{false};
    bool limits_satisfy_required_shape{false};
    std::uint32_t max_output_vertices{0};
    std::uint32_t max_output_primitives{0};
    std::uint32_t max_threadgroup_size{0};
    std::uint32_t max_payload_bytes{0};
    std::uint32_t max_output_memory_bytes{0};
    std::uint32_t max_payload_and_output_memory_bytes{0};
};

struct MavgMeshShaderDiagnostic {
    MavgMeshShaderDiagnosticCode code{MavgMeshShaderDiagnosticCode::invalid_required_shape};
    MavgMeshShaderBackend backend{MavgMeshShaderBackend::d3d12};
    bool blocking{true};
    std::string message;
};

struct MavgMeshShaderCapabilityPlan {
    MavgMeshShaderPathKind path{MavgMeshShaderPathKind::compute_indirect_fallback};
    std::vector<MavgMeshShaderBackendPlanRow> backend_rows;
    std::vector<MavgMeshShaderDiagnostic> diagnostics;
    std::uint32_t reviewed_backend_count{0};
    std::uint32_t mesh_shader_ready_backend_count{0};
    bool selected_mesh_shader_path{false};
    bool selected_compute_indirect_fallback{true};
    bool executed_mesh_shader{false};
    bool touched_native_handles{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MavgMeshShaderCapabilityPlan plan_mavg_mesh_shader_capability(const MavgMeshShaderCapabilityDesc& desc);

[[nodiscard]] bool has_mavg_mesh_shader_diagnostic(const MavgMeshShaderCapabilityPlan& plan,
                                                   MavgMeshShaderDiagnosticCode code) noexcept;

} // namespace mirakana
