// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/backend_capabilities.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::rhi::metal {

enum class MetalMavgMeshLodDiagnosticCode : std::uint8_t {
    none,
    unsupported_host,
    missing_full_xcode,
    missing_metal_tool,
    missing_metallib_tool,
    missing_feature_set_table_row,
    missing_xcode_version,
    missing_macos_version,
    missing_apple_gpu_family,
    missing_mesh_shader_support,
    missing_object_shader_support,
    missing_first_party_mavg_workload,
    missing_object_mesh_pipeline,
    missing_object_mesh_dispatch,
    missing_readback_hash,
    missing_package_visible_output,
    simulator_only_evidence,
    cross_backend_inference,
    ray_tracing_pipeline_conflation,
    native_handle_access,
    nanite_claim_not_allowed,
    broad_readiness_not_allowed,
};

enum class MetalMavgMeshLodEvidenceStatus : std::uint8_t {
    host_evidence_required,
    blocked,
    ready,
};

struct MetalMavgMeshLodHostEvidenceDesc {
    RhiHostPlatform host{RhiHostPlatform::unknown};
    std::string_view feature_set_table_row_id;
    std::string_view xcode_version;
    std::string_view macos_version;
    std::string_view apple_gpu_family;
    bool full_xcode_selected{false};
    bool metal_tool_ready{false};
    bool metallib_tool_ready{false};
    bool apple_gpu_family_supported{false};
    bool mesh_shader_supported{false};
    bool object_shader_supported{false};
    bool selected_first_party_mavg_workload{false};
    bool object_mesh_pipeline_created{false};
    bool object_mesh_dispatch_executed{false};
    bool package_visible_output_written{false};
    bool readback_hash_valid{false};
    bool simulator_only_evidence{false};
    bool d3d12_or_vulkan_inference{false};
    bool ray_tracing_conflated_with_mesh_pipeline{false};
    bool native_handles_exposed{false};
    bool request_nanite_claim{false};
    bool request_broad_readiness{false};
};

struct MetalMavgMeshLodDiagnostic {
    MetalMavgMeshLodDiagnosticCode code{MetalMavgMeshLodDiagnosticCode::none};
    std::string message;
};

struct MetalMavgMeshLodHostEvidenceResult {
    MetalMavgMeshLodEvidenceStatus status{MetalMavgMeshLodEvidenceStatus::host_evidence_required};
    std::vector<MetalMavgMeshLodDiagnostic> diagnostics;
    bool mavg_metal_mesh_lod_host_gated{true};
    bool mavg_metal_mesh_lod_ready{false};
    bool mavg_mesh_shader_lod_ready{false};
    bool mavg_metal_ray_tracing_ready{false};
    bool mavg_nanite_compatible{false};
    bool mavg_nanite_equivalent{false};
    bool mavg_nanite_superior{false};
    bool mavg_broad_backend_readiness_ready{false};
    bool mavg_broad_optimization_ready{false};
    std::size_t ready_evidence_rows{0U};
    std::size_t host_gated_evidence_rows{0U};
    std::size_t blocked_evidence_rows{0U};
};

[[nodiscard]] std::string_view metal_mavg_mesh_lod_diagnostic_label(MetalMavgMeshLodDiagnosticCode code) noexcept;
[[nodiscard]] std::string_view metal_mavg_mesh_lod_status_label(MetalMavgMeshLodEvidenceStatus status) noexcept;
[[nodiscard]] MetalMavgMeshLodHostEvidenceResult
evaluate_mavg_metal_mesh_lod_host_evidence(const MetalMavgMeshLodHostEvidenceDesc& desc);
[[nodiscard]] bool has_mavg_metal_mesh_lod_diagnostic(const MetalMavgMeshLodHostEvidenceResult& result,
                                                      MetalMavgMeshLodDiagnosticCode code) noexcept;
[[nodiscard]] std::string
metal_mavg_mesh_lod_host_evidence_status_line(const MetalMavgMeshLodHostEvidenceResult& result);

} // namespace mirakana::rhi::metal
