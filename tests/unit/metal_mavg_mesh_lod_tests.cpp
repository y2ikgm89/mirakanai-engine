// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/rhi/metal/metal_mavg_mesh_lod.hpp"

#include <string>

namespace {

using mirakana::rhi::RhiHostPlatform;
using mirakana::rhi::metal::MetalMavgMeshLodDiagnosticCode;
using mirakana::rhi::metal::MetalMavgMeshLodEvidenceStatus;

[[nodiscard]] mirakana::rhi::metal::MetalMavgMeshLodHostEvidenceDesc make_complete_desc() {
    return mirakana::rhi::metal::MetalMavgMeshLodHostEvidenceDesc{
        .host = RhiHostPlatform::macos,
        .feature_set_table_row_id = "Metal Feature Set Tables 2026-05-21 Apple GPU family mesh shaders",
        .xcode_version = "Xcode 17.x",
        .macos_version = "macOS 15.x",
        .apple_gpu_family = "Apple9",
        .full_xcode_selected = true,
        .metal_tool_ready = true,
        .metallib_tool_ready = true,
        .apple_gpu_family_supported = true,
        .mesh_shader_supported = true,
        .object_shader_supported = true,
        .selected_first_party_mavg_workload = true,
        .object_mesh_pipeline_created = true,
        .object_mesh_dispatch_executed = true,
        .package_visible_output_written = true,
        .readback_hash_valid = true,
    };
}

[[nodiscard]] bool has_code(const mirakana::rhi::metal::MetalMavgMeshLodHostEvidenceResult& result,
                            MetalMavgMeshLodDiagnosticCode code) noexcept {
    return mirakana::rhi::metal::has_mavg_metal_mesh_lod_diagnostic(result, code);
}

} // namespace

MK_TEST("metal mavg mesh lod host gate rejects non apple hosts") {
    auto desc = make_complete_desc();
    desc.host = RhiHostPlatform::windows;

    const auto result = mirakana::rhi::metal::evaluate_mavg_metal_mesh_lod_host_evidence(desc);

    MK_REQUIRE(result.status == MetalMavgMeshLodEvidenceStatus::host_evidence_required);
    MK_REQUIRE(result.mavg_metal_mesh_lod_host_gated);
    MK_REQUIRE(!result.mavg_metal_mesh_lod_ready);
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::unsupported_host));
}

MK_TEST("metal mavg mesh lod host gate requires apple feature table and toolchain evidence") {
    mirakana::rhi::metal::MetalMavgMeshLodHostEvidenceDesc desc{
        .host = RhiHostPlatform::macos,
        .xcode_version = "Xcode 17.x",
        .macos_version = "macOS 15.x",
    };

    const auto result = mirakana::rhi::metal::evaluate_mavg_metal_mesh_lod_host_evidence(desc);

    MK_REQUIRE(result.status == MetalMavgMeshLodEvidenceStatus::host_evidence_required);
    MK_REQUIRE(!result.mavg_metal_mesh_lod_ready);
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_full_xcode));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_metal_tool));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_metallib_tool));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_feature_set_table_row));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_apple_gpu_family));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_mesh_shader_support));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_object_shader_support));
}

MK_TEST("metal mavg mesh lod host gate requires object mesh execution proof") {
    auto desc = make_complete_desc();
    desc.object_mesh_pipeline_created = false;
    desc.object_mesh_dispatch_executed = false;
    desc.readback_hash_valid = false;
    desc.package_visible_output_written = false;

    const auto result = mirakana::rhi::metal::evaluate_mavg_metal_mesh_lod_host_evidence(desc);

    MK_REQUIRE(result.status == MetalMavgMeshLodEvidenceStatus::host_evidence_required);
    MK_REQUIRE(!result.mavg_metal_mesh_lod_ready);
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_object_mesh_pipeline));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_object_mesh_dispatch));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_readback_hash));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::missing_package_visible_output));
}

MK_TEST("metal mavg mesh lod host gate promotes only selected metal local readiness") {
    const auto result = mirakana::rhi::metal::evaluate_mavg_metal_mesh_lod_host_evidence(make_complete_desc());
    const auto line = mirakana::rhi::metal::metal_mavg_mesh_lod_host_evidence_status_line(result);

    MK_REQUIRE(result.status == MetalMavgMeshLodEvidenceStatus::ready);
    MK_REQUIRE(result.mavg_metal_mesh_lod_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_ready);
    MK_REQUIRE(!result.mavg_metal_ray_tracing_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
    MK_REQUIRE(!result.mavg_broad_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_broad_optimization_ready);
    MK_REQUIRE(line.find("mavg_metal_mesh_lod_ready=1") != std::string::npos);
    MK_REQUIRE(line.find("mavg_mesh_shader_lod_ready=0") != std::string::npos);
    MK_REQUIRE(line.find("mavg_metal_ray_tracing_ready=0") != std::string::npos);
}

MK_TEST("metal mavg mesh lod host gate blocks inference native handles nanite and broad claims") {
    auto desc = make_complete_desc();
    desc.simulator_only_evidence = true;
    desc.d3d12_or_vulkan_inference = true;
    desc.ray_tracing_conflated_with_mesh_pipeline = true;
    desc.native_handles_exposed = true;
    desc.request_nanite_claim = true;
    desc.request_broad_readiness = true;

    const auto result = mirakana::rhi::metal::evaluate_mavg_metal_mesh_lod_host_evidence(desc);

    MK_REQUIRE(result.status == MetalMavgMeshLodEvidenceStatus::blocked);
    MK_REQUIRE(!result.mavg_metal_mesh_lod_ready);
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::simulator_only_evidence));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::cross_backend_inference));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::ray_tracing_pipeline_conflation));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::native_handle_access));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::nanite_claim_not_allowed));
    MK_REQUIRE(has_code(result, MetalMavgMeshLodDiagnosticCode::broad_readiness_not_allowed));
}

int main() {
    return mirakana::test::run_all();
}
