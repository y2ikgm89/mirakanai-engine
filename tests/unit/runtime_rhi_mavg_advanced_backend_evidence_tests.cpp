// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_advanced_backend_evidence.hpp"

namespace {

[[nodiscard]] mirakana::runtime_rhi::MavgAdvancedBackendEvidenceDesc make_fresh_source_gate_desc() {
    return mirakana::runtime_rhi::MavgAdvancedBackendEvidenceDesc{
        .source_gate_date_yyyy_mm_dd = "2026-06-22",
        .source_gate_current_date_yyyy_mm_dd = "2026-06-22",
        .context7_vulkan_docs_ready = true,
        .context7_cmake_ready = true,
        .context7_vcpkg_ready = true,
        .official_d3d12_mesh_shader_ready = true,
        .official_vulkan_mesh_shader_ready = true,
        .official_apple_metal_ready = true,
        .official_directstorage_ready = true,
        .official_nanite_docs_ready = true,
        .official_profiler_docs_ready = true,
        .official_d3d12_mesh_tier_doc_date = "2026-06-22",
        .official_vulkan_mesh_ext_doc_date = "2026-06-22",
        .official_metal_feature_table_date = "2026-06-22",
        .official_pix_doc_date = "2026-06-22",
        .official_nsight_doc_date = "2026-06-22",
        .official_rgp_doc_date = "2026-06-22",
        .official_intel_gpa_doc_date = "2026-06-22",
    };
}

} // namespace

MK_TEST("runtime rhi mavg advanced backend evidence fails closed when matrix is empty") {
    const auto result = mirakana::runtime_rhi::evaluate_mavg_advanced_backend_evidence({});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.source_gate_ready);
    MK_REQUIRE(!result.mavg_advanced_backend_evidence_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_d3d12_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
    MK_REQUIRE(!result.mavg_metal_mesh_lod_ready);
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_autonomous_streaming_scheduler_ready);
    MK_REQUIRE(!result.mavg_async_overlap_measured_performance_ready);
    MK_REQUIRE(!result.mavg_deformation_integration_ready);
    MK_REQUIRE(!result.mavg_ray_tracing_integration_ready);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(!result.mavg_nanite_comparison_report_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
    MK_REQUIRE(result.missing_task_row_count == 10U);
    MK_REQUIRE(mirakana::runtime_rhi::has_mavg_advanced_backend_evidence_diagnostic(
        result, mirakana::runtime_rhi::MavgAdvancedBackendEvidenceDiagnosticCode::missing_source_gate_date));
    MK_REQUIRE(
        mirakana::runtime_rhi::has_mavg_advanced_backend_evidence_row_diagnostic(result, "mavg_mesh_shader_lod_ready"));
    MK_REQUIRE(mirakana::runtime_rhi::has_mavg_advanced_backend_evidence_row_diagnostic(
        result, "mavg_package_visible_backend_readiness_ready"));
}

MK_TEST("runtime rhi mavg advanced backend evidence accepts fresh sources but keeps missing tasks unclaimed") {
    const auto desc = make_fresh_source_gate_desc();
    const auto result = mirakana::runtime_rhi::evaluate_mavg_advanced_backend_evidence(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.source_gate_ready);
    MK_REQUIRE(!result.mavg_advanced_backend_evidence_ready);
    MK_REQUIRE(result.missing_source_gate_count == 0U);
    MK_REQUIRE(result.missing_task_row_count == 10U);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_ready);
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
}

MK_TEST("runtime rhi mavg advanced backend evidence rejects stale source gates") {
    auto desc = make_fresh_source_gate_desc();
    desc.source_gate_date_yyyy_mm_dd = "2026-06-01";

    const auto result = mirakana::runtime_rhi::evaluate_mavg_advanced_backend_evidence(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.source_gate_ready);
    MK_REQUIRE(mirakana::runtime_rhi::has_mavg_advanced_backend_evidence_diagnostic(
        result, mirakana::runtime_rhi::MavgAdvancedBackendEvidenceDiagnosticCode::stale_source_gate));
}

MK_TEST("runtime rhi mavg advanced backend evidence rejects invalid calendar source gate dates") {
    auto desc = make_fresh_source_gate_desc();
    desc.source_gate_date_yyyy_mm_dd = "2026-02-31";

    const auto result = mirakana::runtime_rhi::evaluate_mavg_advanced_backend_evidence(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.source_gate_ready);
    MK_REQUIRE(mirakana::runtime_rhi::has_mavg_advanced_backend_evidence_diagnostic(
        result, mirakana::runtime_rhi::MavgAdvancedBackendEvidenceDiagnosticCode::invalid_source_gate_date));
}

MK_TEST("runtime rhi mavg advanced backend evidence rejects missing official source rows") {
    auto desc = make_fresh_source_gate_desc();
    desc.official_vulkan_mesh_shader_ready = false;

    const auto result = mirakana::runtime_rhi::evaluate_mavg_advanced_backend_evidence(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.source_gate_ready);
    MK_REQUIRE(result.missing_source_gate_count == 1U);
    MK_REQUIRE(mirakana::runtime_rhi::has_mavg_advanced_backend_evidence_diagnostic(
        result, mirakana::runtime_rhi::MavgAdvancedBackendEvidenceDiagnosticCode::missing_official_source));
    MK_REQUIRE(mirakana::runtime_rhi::has_mavg_advanced_backend_evidence_row_diagnostic(
        result, "official_vulkan_mesh_shader_ready"));
}

MK_TEST("runtime rhi mavg advanced backend evidence succeeds only when every required row is ready") {
    auto desc = make_fresh_source_gate_desc();
    desc.mavg_mesh_shader_lod_d3d12_ready = true;
    desc.mavg_mesh_shader_lod_vulkan_ready = true;
    desc.mavg_metal_mesh_lod_ready = true;
    desc.mavg_package_visible_backend_readiness_ready = true;
    desc.mavg_autonomous_streaming_scheduler_ready = true;
    desc.mavg_async_overlap_measured_performance_ready = true;
    desc.mavg_deformation_integration_ready = true;
    desc.mavg_ray_tracing_integration_ready = true;
    desc.mavg_broad_cpu_gpu_memory_optimization_ready = true;
    desc.mavg_nanite_comparison_report_ready = true;

    const auto result = mirakana::runtime_rhi::evaluate_mavg_advanced_backend_evidence(desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.source_gate_ready);
    MK_REQUIRE(result.mavg_advanced_backend_evidence_ready);
    MK_REQUIRE(result.mavg_mesh_shader_lod_ready);
    MK_REQUIRE(result.missing_source_gate_count == 0U);
    MK_REQUIRE(result.missing_task_row_count == 0U);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
}

MK_TEST("runtime rhi mavg advanced backend evidence blocks nanite compatibility equivalence and superiority claims") {
    auto desc = make_fresh_source_gate_desc();
    desc.mavg_nanite_comparison_report_ready = true;
    desc.request_mavg_nanite_compatible = true;
    desc.request_mavg_nanite_equivalent = true;
    desc.request_mavg_nanite_superior = true;

    const auto result = mirakana::runtime_rhi::evaluate_mavg_advanced_backend_evidence(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.source_gate_ready);
    MK_REQUIRE(result.mavg_nanite_comparison_report_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
    MK_REQUIRE(mirakana::runtime_rhi::has_mavg_advanced_backend_evidence_diagnostic(
        result, mirakana::runtime_rhi::MavgAdvancedBackendEvidenceDiagnosticCode::unsupported_nanite_product_claim));
}

MK_TEST("runtime rhi mavg advanced backend evidence rejects current active plan mutation requests") {
    auto desc = make_fresh_source_gate_desc();
    desc.request_current_active_plan_mutation = true;

    const auto result = mirakana::runtime_rhi::evaluate_mavg_advanced_backend_evidence(desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(mirakana::runtime_rhi::has_mavg_advanced_backend_evidence_diagnostic(
        result,
        mirakana::runtime_rhi::MavgAdvancedBackendEvidenceDiagnosticCode::current_active_plan_mutation_requested));
}

int main() {
    return mirakana::test::run_all();
}
