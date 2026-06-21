// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_rhi {

enum class MavgAdvancedBackendEvidenceDiagnosticCode : std::uint8_t {
    none = 0,
    missing_source_gate_date,
    invalid_source_gate_date,
    stale_source_gate,
    missing_context7_source,
    missing_official_source,
    missing_official_source_doc_date,
    missing_task_row,
    unsupported_nanite_product_claim,
    current_active_plan_mutation_requested,
};

struct MavgAdvancedBackendEvidenceDiagnostic {
    MavgAdvancedBackendEvidenceDiagnosticCode code{MavgAdvancedBackendEvidenceDiagnosticCode::none};
    std::string row_id;
    std::string message;
};

struct MavgAdvancedBackendEvidenceDesc {
    std::string_view source_gate_date_yyyy_mm_dd;
    std::string_view source_gate_current_date_yyyy_mm_dd;
    std::uint32_t source_gate_max_age_days{14};

    bool context7_vulkan_docs_ready{false};
    bool context7_cmake_ready{false};
    bool context7_vcpkg_ready{false};
    bool official_d3d12_mesh_shader_ready{false};
    bool official_vulkan_mesh_shader_ready{false};
    bool official_apple_metal_ready{false};
    bool official_directstorage_ready{false};
    bool official_nanite_docs_ready{false};
    bool official_profiler_docs_ready{false};

    std::string_view official_d3d12_mesh_tier_doc_date;
    std::string_view official_vulkan_mesh_ext_doc_date;
    std::string_view official_metal_feature_table_date;
    std::string_view official_pix_doc_date;
    std::string_view official_nsight_doc_date;
    std::string_view official_rgp_doc_date;
    std::string_view official_intel_gpa_doc_date;

    bool mavg_mesh_shader_lod_d3d12_ready{false};
    bool mavg_mesh_shader_lod_vulkan_ready{false};
    bool mavg_metal_mesh_lod_ready{false};
    bool mavg_package_visible_backend_readiness_ready{false};
    bool mavg_autonomous_streaming_scheduler_ready{false};
    bool mavg_async_overlap_measured_performance_ready{false};
    bool mavg_deformation_integration_ready{false};
    bool mavg_ray_tracing_integration_ready{false};
    bool mavg_broad_cpu_gpu_memory_optimization_ready{false};
    bool mavg_nanite_comparison_report_ready{false};

    bool request_mavg_nanite_compatible{false};
    bool request_mavg_nanite_equivalent{false};
    bool request_mavg_nanite_superior{false};
    bool request_current_active_plan_mutation{false};
};

struct MavgAdvancedBackendEvidenceResult {
    std::vector<MavgAdvancedBackendEvidenceDiagnostic> diagnostics;
    std::size_t missing_source_gate_count{0};
    std::size_t missing_task_row_count{0};

    bool source_gate_ready{false};
    bool mavg_advanced_backend_evidence_ready{false};
    bool mavg_mesh_shader_lod_ready{false};
    bool mavg_mesh_shader_lod_d3d12_ready{false};
    bool mavg_mesh_shader_lod_vulkan_ready{false};
    bool mavg_metal_mesh_lod_ready{false};
    bool mavg_package_visible_backend_readiness_ready{false};
    bool mavg_autonomous_streaming_scheduler_ready{false};
    bool mavg_async_overlap_measured_performance_ready{false};
    bool mavg_deformation_integration_ready{false};
    bool mavg_ray_tracing_integration_ready{false};
    bool mavg_broad_cpu_gpu_memory_optimization_ready{false};
    bool mavg_nanite_comparison_report_ready{false};
    bool mavg_nanite_compatible{false};
    bool mavg_nanite_equivalent{false};
    bool mavg_nanite_superior{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && mavg_advanced_backend_evidence_ready;
    }
};

/// Evaluates the source freshness and task-row matrix for the MAVG advanced backend evidence closeout. This value-only
/// gate does not execute backends, expose native handles, infer Metal/D3D12/Vulkan parity, or promote Nanite
/// compatibility/equivalence/superiority claims.
[[nodiscard]] MavgAdvancedBackendEvidenceResult
evaluate_mavg_advanced_backend_evidence(const MavgAdvancedBackendEvidenceDesc& desc);

[[nodiscard]] bool
has_mavg_advanced_backend_evidence_diagnostic(const MavgAdvancedBackendEvidenceResult& result,
                                              MavgAdvancedBackendEvidenceDiagnosticCode code) noexcept;

[[nodiscard]] bool has_mavg_advanced_backend_evidence_row_diagnostic(const MavgAdvancedBackendEvidenceResult& result,
                                                                     std::string_view row_id) noexcept;

} // namespace mirakana::runtime_rhi
