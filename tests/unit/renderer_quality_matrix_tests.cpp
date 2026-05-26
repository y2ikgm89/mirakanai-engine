// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/renderer_quality_matrix.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::RendererQualityFeatureKind;
using mirakana::RendererQualityMatrixDiagnosticCode;
using mirakana::RendererQualityMatrixStatus;
using mirakana::RendererQualityProofKind;

constexpr RendererQualityFeatureKind kRequiredFeatures[] = {
    RendererQualityFeatureKind::materials,         RendererQualityFeatureKind::lighting_shadows,
    RendererQualityFeatureKind::postprocess,       RendererQualityFeatureKind::sprite_ui,
    RendererQualityFeatureKind::scene_scale,       RendererQualityFeatureKind::gpu_memory_residency,
    RendererQualityFeatureKind::profiling_capture,
};

[[nodiscard]] std::string feature_id(RendererQualityFeatureKind feature) {
    switch (feature) {
    case RendererQualityFeatureKind::materials:
        return "materials.lit";
    case RendererQualityFeatureKind::lighting_shadows:
        return "lighting.directional_shadow";
    case RendererQualityFeatureKind::postprocess:
        return "postprocess.depth_aware";
    case RendererQualityFeatureKind::sprite_ui:
        return "sprite_ui.atlas_overlay";
    case RendererQualityFeatureKind::scene_scale:
        return "scene_scale.visibility_budget";
    case RendererQualityFeatureKind::gpu_memory_residency:
        return "gpu_memory.residency_budget";
    case RendererQualityFeatureKind::profiling_capture:
        return "profiling.capture_handoff";
    }
    return "unknown";
}

[[nodiscard]] mirakana::RendererQualityMatrixRow
make_ready_row(RendererQualityFeatureKind feature, mirakana::rhi::BackendKind backend, std::uint32_t source_index) {
    const auto is_d3d12 = backend == mirakana::rhi::BackendKind::d3d12;
    const auto is_vulkan = backend == mirakana::rhi::BackendKind::vulkan;
    const auto is_metal = backend == mirakana::rhi::BackendKind::metal;
    return mirakana::RendererQualityMatrixRow{
        .feature_id = feature_id(feature),
        .feature = feature,
        .backend = backend,
        .proof = RendererQualityProofKind::selected_package,
        .reviewed = true,
        .backend_local_evidence = true,
        .d3d12_resource_state_barrier_evidence = is_d3d12,
        .d3d12_fence_evidence = is_d3d12,
        .vulkan_synchronization2_evidence = is_vulkan,
        .vulkan_layout_transition_evidence = is_vulkan,
        .vulkan_validation_layer_evidence = is_vulkan,
        .vulkan_spirv_validation_evidence = is_vulkan,
        .metal_resource_synchronization_evidence = is_metal,
        .metal_feature_set_evidence = is_metal,
        .shader_tool_validation_evidence = true,
        .package_counter_ids = {std::string{"renderer_quality_matrix."} + feature_id(feature)},
        .timing_budget_us = 1000U,
        .gpu_memory_evidence = true,
        .backend_parity_evidence = true,
        .host_validated = true,
        .host_gate_required = false,
        .request_native_handle_access = false,
        .request_capture_execution = false,
        .request_crash_upload_execution = false,
        .request_inferred_backend_parity = false,
        .request_subjective_visual_quality_claim = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::RendererQualityMatrixRow make_metal_host_gated_row(RendererQualityFeatureKind feature,
                                                                           std::uint32_t source_index) {
    return mirakana::RendererQualityMatrixRow{
        .feature_id = feature_id(feature),
        .feature = feature,
        .backend = mirakana::rhi::BackendKind::metal,
        .proof = RendererQualityProofKind::host_gate,
        .reviewed = true,
        .backend_local_evidence = true,
        .d3d12_resource_state_barrier_evidence = false,
        .d3d12_fence_evidence = false,
        .vulkan_synchronization2_evidence = false,
        .vulkan_layout_transition_evidence = false,
        .vulkan_validation_layer_evidence = false,
        .vulkan_spirv_validation_evidence = false,
        .metal_resource_synchronization_evidence = false,
        .metal_feature_set_evidence = false,
        .shader_tool_validation_evidence = false,
        .package_counter_ids = {},
        .timing_budget_us = 0U,
        .gpu_memory_evidence = false,
        .backend_parity_evidence = false,
        .host_validated = false,
        .host_gate_required = true,
        .request_native_handle_access = false,
        .request_capture_execution = false,
        .request_crash_upload_execution = false,
        .request_inferred_backend_parity = false,
        .request_subjective_visual_quality_claim = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::RendererQualityMatrixRequest make_request(bool include_metal_host_evidence) {
    std::vector<mirakana::RendererQualityMatrixRow> rows;
    rows.reserve(21U);
    std::uint32_t source_index{1U};
    for (const auto feature : kRequiredFeatures) {
        rows.push_back(make_ready_row(feature, mirakana::rhi::BackendKind::d3d12, source_index++));
        rows.push_back(make_ready_row(feature, mirakana::rhi::BackendKind::vulkan, source_index++));
        rows.push_back(include_metal_host_evidence
                           ? make_ready_row(feature, mirakana::rhi::BackendKind::metal, source_index++)
                           : make_metal_host_gated_row(feature, source_index++));
    }

    return mirakana::RendererQualityMatrixRequest{
        .required_backends =
            {
                mirakana::rhi::BackendKind::d3d12,
                mirakana::rhi::BackendKind::vulkan,
                mirakana::rhi::BackendKind::metal,
            },
        .rows = std::move(rows),
        .row_budget = 64U,
        .seed = 456U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::RendererQualityMatrixPlan& plan,
                                           RendererQualityMatrixDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("renderer quality matrix keeps general production claim host gated by Metal") {
    const auto plan = mirakana::plan_renderer_quality_matrix(make_request(false));

    MK_REQUIRE(plan.status == RendererQualityMatrixStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 21U);
    MK_REQUIRE(plan.ready_row_count == 14U);
    MK_REQUIRE(plan.host_gated_row_count == 7U);
    MK_REQUIRE(plan.host_validated_backend_count == 2U);
    MK_REQUIRE(plan.d3d12_quality_matrix_ready);
    MK_REQUIRE(plan.vulkan_strict_quality_matrix_ready);
    MK_REQUIRE(!plan.metal_quality_matrix_ready);
    MK_REQUIRE(plan.requires_metal_host_evidence);
    MK_REQUIRE(!plan.has_metal_host_evidence);
    MK_REQUIRE(plan.selected_package_quality_evidence_ready);
    MK_REQUIRE(!plan.general_renderer_quality_ready);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.invoked_gpu_commands);
    MK_REQUIRE(!plan.invoked_native_capture);
    MK_REQUIRE(!plan.invoked_crash_upload);
}

MK_TEST("renderer quality matrix is ready when every backend has local host evidence") {
    const auto plan = mirakana::plan_renderer_quality_matrix(make_request(true));

    MK_REQUIRE(plan.status == RendererQualityMatrixStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 21U);
    MK_REQUIRE(plan.ready_row_count == 21U);
    MK_REQUIRE(plan.host_gated_row_count == 0U);
    MK_REQUIRE(plan.host_validated_backend_count == 3U);
    MK_REQUIRE(plan.d3d12_quality_matrix_ready);
    MK_REQUIRE(plan.vulkan_strict_quality_matrix_ready);
    MK_REQUIRE(plan.metal_quality_matrix_ready);
    MK_REQUIRE(plan.requires_metal_host_evidence);
    MK_REQUIRE(plan.has_metal_host_evidence);
    MK_REQUIRE(plan.selected_package_quality_evidence_ready);
    MK_REQUIRE(plan.general_renderer_quality_ready);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("renderer quality matrix rejects missing D3D12 barrier and fence evidence") {
    auto request = make_request(true);
    request.rows[0].d3d12_resource_state_barrier_evidence = false;
    request.rows[0].d3d12_fence_evidence = false;

    const auto plan = mirakana::plan_renderer_quality_matrix(request);

    MK_REQUIRE(plan.status == RendererQualityMatrixStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_resource_synchronization_evidence) ==
               1U);
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("renderer quality matrix rejects missing strict Vulkan synchronization validation and SPIR-V evidence") {
    auto request = make_request(true);
    request.rows[1].vulkan_synchronization2_evidence = false;
    request.rows[1].vulkan_layout_transition_evidence = false;
    request.rows[1].vulkan_validation_layer_evidence = false;
    request.rows[1].vulkan_spirv_validation_evidence = false;
    request.rows[1].shader_tool_validation_evidence = false;

    const auto plan = mirakana::plan_renderer_quality_matrix(request);

    MK_REQUIRE(plan.status == RendererQualityMatrixStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_resource_synchronization_evidence) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_backend_validation_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_shader_tool_validation_evidence) ==
               1U);
    MK_REQUIRE(plan.ready_row_count == 0U);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("renderer quality matrix rejects unsafe side effects and subjective quality claims") {
    auto request = make_request(true);
    request.rows[0].backend_local_evidence = false;
    request.rows[0].package_counter_ids.clear();
    request.rows[0].timing_budget_us = 0U;
    request.rows[0].gpu_memory_evidence = false;
    request.rows[0].backend_parity_evidence = false;
    request.rows[0].request_native_handle_access = true;
    request.rows[0].request_capture_execution = true;
    request.rows[0].request_crash_upload_execution = true;
    request.rows[0].request_inferred_backend_parity = true;
    request.rows[0].request_subjective_visual_quality_claim = true;

    const auto plan = mirakana::plan_renderer_quality_matrix(request);

    MK_REQUIRE(plan.status == RendererQualityMatrixStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_backend_local_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_package_counter_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_timing_budget_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_gpu_memory_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_backend_parity_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::unsupported_native_handle_claim) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::unsupported_capture_execution) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::unsupported_crash_upload_execution) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::unsupported_inferred_backend_parity) == 1U);
    MK_REQUIRE(
        diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::unsupported_subjective_visual_quality_claim) == 1U);
    MK_REQUIRE(plan.invoked_gpu_commands == false);
    MK_REQUIRE(plan.invoked_native_capture == false);
    MK_REQUIRE(plan.invoked_crash_upload == false);
}

MK_TEST("renderer quality matrix rejects native handle tokens in ids and counters") {
    auto request = make_request(true);
    request.rows[0].feature_id = "ID3D12Resource";
    request.rows[0].package_counter_ids = {"renderer_quality_matrix.MTLBuffer"};

    const auto plan = mirakana::plan_renderer_quality_matrix(request);

    MK_REQUIRE(plan.status == RendererQualityMatrixStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::invalid_quality_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_package_counter_evidence) == 1U);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("renderer quality matrix rejects missing backend feature rows and duplicate backend rows") {
    auto request = make_request(true);
    request.rows.pop_back();
    request.rows.push_back(request.rows.front());
    request.rows.back().source_index = 99U;

    const auto plan = mirakana::plan_renderer_quality_matrix(request);

    MK_REQUIRE(plan.status == RendererQualityMatrixStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::missing_required_quality_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererQualityMatrixDiagnosticCode::duplicate_quality_row) == 1U);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("renderer quality matrix reports no rows without backend claims") {
    const auto plan = mirakana::plan_renderer_quality_matrix(mirakana::RendererQualityMatrixRequest{});

    MK_REQUIRE(plan.status == RendererQualityMatrixStatus::no_rows);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 0U);
    MK_REQUIRE(plan.replay_hash == 0U);
}

int main() {
    return mirakana::test::run_all();
}
