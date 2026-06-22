// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/metal/metal_mavg_mesh_lod.hpp"

#include <string>
#include <utility>

namespace mirakana::rhi::metal {

std::string_view metal_mavg_mesh_lod_diagnostic_label(MetalMavgMeshLodDiagnosticCode code) noexcept {
    switch (code) {
    case MetalMavgMeshLodDiagnosticCode::none:
        return "none";
    case MetalMavgMeshLodDiagnosticCode::unsupported_host:
        return "unsupported_host";
    case MetalMavgMeshLodDiagnosticCode::missing_full_xcode:
        return "missing_full_xcode";
    case MetalMavgMeshLodDiagnosticCode::missing_metal_tool:
        return "missing_metal_tool";
    case MetalMavgMeshLodDiagnosticCode::missing_metallib_tool:
        return "missing_metallib_tool";
    case MetalMavgMeshLodDiagnosticCode::missing_feature_set_table_row:
        return "missing_feature_set_table_row";
    case MetalMavgMeshLodDiagnosticCode::missing_xcode_version:
        return "missing_xcode_version";
    case MetalMavgMeshLodDiagnosticCode::missing_macos_version:
        return "missing_macos_version";
    case MetalMavgMeshLodDiagnosticCode::missing_apple_gpu_family:
        return "missing_apple_gpu_family";
    case MetalMavgMeshLodDiagnosticCode::missing_mesh_shader_support:
        return "missing_mesh_shader_support";
    case MetalMavgMeshLodDiagnosticCode::missing_object_shader_support:
        return "missing_object_shader_support";
    case MetalMavgMeshLodDiagnosticCode::missing_first_party_mavg_workload:
        return "missing_first_party_mavg_workload";
    case MetalMavgMeshLodDiagnosticCode::missing_object_mesh_pipeline:
        return "missing_object_mesh_pipeline";
    case MetalMavgMeshLodDiagnosticCode::missing_object_mesh_dispatch:
        return "missing_object_mesh_dispatch";
    case MetalMavgMeshLodDiagnosticCode::missing_readback_hash:
        return "missing_readback_hash";
    case MetalMavgMeshLodDiagnosticCode::missing_package_visible_output:
        return "missing_package_visible_output";
    case MetalMavgMeshLodDiagnosticCode::simulator_only_evidence:
        return "simulator_only_evidence";
    case MetalMavgMeshLodDiagnosticCode::cross_backend_inference:
        return "cross_backend_inference";
    case MetalMavgMeshLodDiagnosticCode::ray_tracing_pipeline_conflation:
        return "ray_tracing_pipeline_conflation";
    case MetalMavgMeshLodDiagnosticCode::native_handle_access:
        return "native_handle_access";
    case MetalMavgMeshLodDiagnosticCode::nanite_claim_not_allowed:
        return "nanite_claim_not_allowed";
    case MetalMavgMeshLodDiagnosticCode::broad_readiness_not_allowed:
        return "broad_readiness_not_allowed";
    }

    return "unknown";
}

std::string_view metal_mavg_mesh_lod_status_label(MetalMavgMeshLodEvidenceStatus status) noexcept {
    switch (status) {
    case MetalMavgMeshLodEvidenceStatus::host_evidence_required:
        return "host_evidence_required";
    case MetalMavgMeshLodEvidenceStatus::blocked:
        return "blocked";
    case MetalMavgMeshLodEvidenceStatus::ready:
        return "ready";
    }

    return "unknown";
}

namespace {

void add_diagnostic(MetalMavgMeshLodHostEvidenceResult& result, MetalMavgMeshLodDiagnosticCode code,
                    std::string message) {
    result.diagnostics.push_back(MetalMavgMeshLodDiagnostic{.code = code, .message = std::move(message)});
}

[[nodiscard]] RhiHostPlatform resolve_host(RhiHostPlatform host) noexcept {
    return host == RhiHostPlatform::unknown ? current_rhi_host_platform() : host;
}

} // namespace

MetalMavgMeshLodHostEvidenceResult
evaluate_mavg_metal_mesh_lod_host_evidence(const MetalMavgMeshLodHostEvidenceDesc& desc) {
    MetalMavgMeshLodHostEvidenceResult result;
    const auto host = resolve_host(desc.host);
    bool host_evidence_required = false;
    bool blocked = false;

    auto require = [&](bool condition, MetalMavgMeshLodDiagnosticCode code, std::string message) {
        if (condition) {
            host_evidence_required = true;
            add_diagnostic(result, code, std::move(message));
        }
    };
    auto block = [&](bool condition, MetalMavgMeshLodDiagnosticCode code, std::string message) {
        if (condition) {
            blocked = true;
            add_diagnostic(result, code, std::move(message));
        }
    };

    require(host != RhiHostPlatform::macos, MetalMavgMeshLodDiagnosticCode::unsupported_host,
            "Metal MAVG mesh LOD evidence requires a macOS Apple host with Xcode Metal tooling");
    require(!desc.full_xcode_selected, MetalMavgMeshLodDiagnosticCode::missing_full_xcode,
            "Metal MAVG mesh LOD evidence requires full Xcode selection");
    require(!desc.metal_tool_ready, MetalMavgMeshLodDiagnosticCode::missing_metal_tool,
            "Metal MAVG mesh LOD evidence requires the xcrun metal tool");
    require(!desc.metallib_tool_ready, MetalMavgMeshLodDiagnosticCode::missing_metallib_tool,
            "Metal MAVG mesh LOD evidence requires the xcrun metallib tool");
    require(desc.feature_set_table_row_id.empty(), MetalMavgMeshLodDiagnosticCode::missing_feature_set_table_row,
            "Metal MAVG mesh LOD evidence requires an Apple Metal Feature Set Tables row id");
    require(desc.xcode_version.empty(), MetalMavgMeshLodDiagnosticCode::missing_xcode_version,
            "Metal MAVG mesh LOD evidence requires the Xcode version");
    require(desc.macos_version.empty(), MetalMavgMeshLodDiagnosticCode::missing_macos_version,
            "Metal MAVG mesh LOD evidence requires the macOS version");
    require(desc.apple_gpu_family.empty(), MetalMavgMeshLodDiagnosticCode::missing_apple_gpu_family,
            "Metal MAVG mesh LOD evidence requires the Apple GPU family");
    require(!desc.apple_gpu_family_supported, MetalMavgMeshLodDiagnosticCode::missing_apple_gpu_family,
            "Metal MAVG mesh LOD evidence requires a supported Apple GPU family row");
    require(!desc.mesh_shader_supported, MetalMavgMeshLodDiagnosticCode::missing_mesh_shader_support,
            "Metal MAVG mesh LOD evidence requires documented mesh shader support");
    require(!desc.object_shader_supported, MetalMavgMeshLodDiagnosticCode::missing_object_shader_support,
            "Metal MAVG mesh LOD evidence requires documented object shader support");
    require(!desc.selected_first_party_mavg_workload, MetalMavgMeshLodDiagnosticCode::missing_first_party_mavg_workload,
            "Metal MAVG mesh LOD evidence requires a selected first-party MAVG workload");
    require(!desc.object_mesh_pipeline_created, MetalMavgMeshLodDiagnosticCode::missing_object_mesh_pipeline,
            "Metal MAVG mesh LOD evidence requires a Metal object/mesh pipeline");
    require(!desc.object_mesh_dispatch_executed, MetalMavgMeshLodDiagnosticCode::missing_object_mesh_dispatch,
            "Metal MAVG mesh LOD evidence requires object/mesh shader dispatch execution");
    require(!desc.readback_hash_valid, MetalMavgMeshLodDiagnosticCode::missing_readback_hash,
            "Metal MAVG mesh LOD evidence requires deterministic readback hash proof");
    require(!desc.package_visible_output_written, MetalMavgMeshLodDiagnosticCode::missing_package_visible_output,
            "Metal MAVG mesh LOD evidence requires package-visible output counters");

    block(desc.simulator_only_evidence, MetalMavgMeshLodDiagnosticCode::simulator_only_evidence,
          "simulator-only Metal evidence cannot promote MAVG mesh LOD readiness");
    block(desc.d3d12_or_vulkan_inference, MetalMavgMeshLodDiagnosticCode::cross_backend_inference,
          "D3D12 or Vulkan evidence cannot promote Metal MAVG mesh LOD readiness");
    block(desc.ray_tracing_conflated_with_mesh_pipeline,
          MetalMavgMeshLodDiagnosticCode::ray_tracing_pipeline_conflation,
          "Metal ray tracing evidence must stay separate from Metal mesh LOD evidence");
    block(desc.native_handles_exposed, MetalMavgMeshLodDiagnosticCode::native_handle_access,
          "Metal MAVG mesh LOD evidence must not expose native handles");
    block(desc.request_nanite_claim, MetalMavgMeshLodDiagnosticCode::nanite_claim_not_allowed,
          "Metal MAVG mesh LOD evidence cannot claim Nanite compatibility, equivalence, or superiority");
    block(desc.request_broad_readiness, MetalMavgMeshLodDiagnosticCode::broad_readiness_not_allowed,
          "Metal MAVG mesh LOD evidence cannot promote broad MAVG backend readiness or broad optimization");

    if (blocked) {
        result.status = MetalMavgMeshLodEvidenceStatus::blocked;
        result.blocked_evidence_rows = 1U;
        result.host_gated_evidence_rows = host_evidence_required ? 1U : 0U;
        result.mavg_metal_mesh_lod_host_gated = host_evidence_required;
        return result;
    }

    if (host_evidence_required) {
        result.status = MetalMavgMeshLodEvidenceStatus::host_evidence_required;
        result.host_gated_evidence_rows = 1U;
        result.mavg_metal_mesh_lod_host_gated = true;
        return result;
    }

    result.status = MetalMavgMeshLodEvidenceStatus::ready;
    result.mavg_metal_mesh_lod_host_gated = false;
    result.mavg_metal_mesh_lod_ready = true;
    result.ready_evidence_rows = 1U;
    return result;
}

bool has_mavg_metal_mesh_lod_diagnostic(const MetalMavgMeshLodHostEvidenceResult& result,
                                        MetalMavgMeshLodDiagnosticCode code) noexcept {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

std::string metal_mavg_mesh_lod_host_evidence_status_line(const MetalMavgMeshLodHostEvidenceResult& result) {
    auto bit = [](bool value) { return value ? "1" : "0"; };

    std::string line;
    line += "mavg_metal_mesh_lod_status=";
    line += metal_mavg_mesh_lod_status_label(result.status);
    line += " mavg_metal_mesh_lod_host_gated=";
    line += bit(result.mavg_metal_mesh_lod_host_gated);
    line += " mavg_metal_mesh_lod_ready=";
    line += bit(result.mavg_metal_mesh_lod_ready);
    line += " mavg_mesh_shader_lod_ready=";
    line += bit(result.mavg_mesh_shader_lod_ready);
    line += " mavg_metal_ray_tracing_ready=";
    line += bit(result.mavg_metal_ray_tracing_ready);
    line += " mavg_nanite_compatible=";
    line += bit(result.mavg_nanite_compatible);
    line += " mavg_nanite_equivalent=";
    line += bit(result.mavg_nanite_equivalent);
    line += " mavg_nanite_superior=";
    line += bit(result.mavg_nanite_superior);
    line += " mavg_broad_backend_readiness_ready=";
    line += bit(result.mavg_broad_backend_readiness_ready);
    line += " mavg_broad_optimization_ready=";
    line += bit(result.mavg_broad_optimization_ready);
    line += " mavg_metal_mesh_lod_ready_rows=" + std::to_string(result.ready_evidence_rows);
    line += " mavg_metal_mesh_lod_host_gated_rows=" + std::to_string(result.host_gated_evidence_rows);
    line += " mavg_metal_mesh_lod_blocked_rows=" + std::to_string(result.blocked_evidence_rows);
    line += " mavg_metal_mesh_lod_diagnostics=" + std::to_string(result.diagnostics.size());
    return line;
}

} // namespace mirakana::rhi::metal
