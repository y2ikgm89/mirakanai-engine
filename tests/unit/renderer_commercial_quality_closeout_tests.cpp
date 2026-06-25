// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/renderer_commercial_quality_closeout.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace {

using mirakana::RendererCommercialQualityCloseoutDiagnosticCode;
using mirakana::RendererCommercialQualityCloseoutStatus;
using mirakana::RendererCommercialQualityEvidenceKind;
using mirakana::RendererCommercialQualityEvidenceStatus;

[[nodiscard]] mirakana::BackendRendererParityPolicyPlan make_backend_parity_plan() {
    return mirakana::BackendRendererParityPolicyPlan{
        .status = mirakana::BackendRendererParityPolicyStatus::ready,
        .diagnostics = {},
        .required_backends =
            {
                mirakana::rhi::BackendKind::d3d12,
                mirakana::rhi::BackendKind::vulkan,
                mirakana::rhi::BackendKind::metal,
            },
        .required_features = {},
        .proofs = {},
        .row_count = 15U,
        .ready_row_count = 15U,
        .host_gated_row_count = 0U,
        .host_validated_backend_count = 3U,
        .replay_hash = 101U,
        .d3d12_parity_ready = true,
        .vulkan_parity_ready = true,
        .metal_parity_ready = true,
    };
}

[[nodiscard]] mirakana::RendererQualityMatrixPlan make_quality_matrix_plan() {
    return mirakana::RendererQualityMatrixPlan{
        .status = mirakana::RendererQualityMatrixStatus::ready,
        .diagnostics = {},
        .required_backends =
            {
                mirakana::rhi::BackendKind::d3d12,
                mirakana::rhi::BackendKind::vulkan,
                mirakana::rhi::BackendKind::metal,
            },
        .rows = {},
        .row_count = 21U,
        .ready_row_count = 21U,
        .host_gated_row_count = 0U,
        .dependency_gated_row_count = 0U,
        .unsupported_row_count = 0U,
        .host_validated_backend_count = 3U,
        .replay_hash = 202U,
        .d3d12_quality_matrix_ready = true,
        .vulkan_strict_quality_matrix_ready = true,
        .metal_quality_matrix_ready = true,
        .requires_metal_host_evidence = true,
        .has_metal_host_evidence = true,
        .selected_package_quality_evidence_ready = true,
        .general_renderer_quality_ready = true,
        .invoked_gpu_commands = false,
        .invoked_native_capture = false,
        .invoked_crash_upload = false,
    };
}

[[nodiscard]] mirakana::RendererProductionVfxProfilingPlan make_vfx_profiling_plan() {
    return mirakana::RendererProductionVfxProfilingPlan{
        .status = mirakana::RendererProductionVfxProfilingStatus::ready,
        .diagnostics = {},
        .required_backends =
            {
                mirakana::rhi::BackendKind::d3d12,
                mirakana::rhi::BackendKind::vulkan,
                mirakana::rhi::BackendKind::metal,
            },
        .feature_rows = {},
        .gpu_particle_budget_rows = {},
        .postprocess_rows = {},
        .backend_timing_rows = {},
        .backend_evidence_rows = {},
        .cpu_profile_rows = {},
        .package_counter_rows = {},
        .crash_telemetry_handoff_rows = {},
        .feature_row_count = 3U,
        .gpu_particle_budget_row_count = 3U,
        .postprocess_row_count = 3U,
        .backend_timing_row_count = 3U,
        .backend_evidence_row_count = 3U,
        .backend_evidence_ready_count = 3U,
        .backend_evidence_host_gated_count = 0U,
        .cpu_profile_row_count = 3U,
        .package_counter_row_count = 3U,
        .package_counter_ready_count = 3U,
        .package_counter_host_gated_count = 0U,
        .crash_telemetry_handoff_row_count = 3U,
        .host_validated_backend_count = 3U,
        .rejected_unsafe_row_count = 0U,
        .replay_hash = 303U,
        .d3d12_host_evidence_ready = true,
        .vulkan_strict_host_evidence_ready = true,
        .metal_host_evidence_ready = true,
        .requires_metal_host_evidence = true,
        .has_metal_host_evidence = true,
        .invoked_gpu_commands = false,
        .invoked_native_capture = false,
        .invoked_crash_upload = false,
    };
}

[[nodiscard]] mirakana::RendererCommercialQualityEvidenceRow make_ready_row(std::string row_id,
                                                                            RendererCommercialQualityEvidenceKind kind,
                                                                            mirakana::rhi::BackendKind backend,
                                                                            std::uint32_t source_index) {
    const auto recipe_id = kind == RendererCommercialQualityEvidenceKind::metal_memory_profiling
                               ? std::string{"renderer-metal-memory-profiling-host-evidence"}
                               : std::string{"renderer-commercial-quality-closeout"};
    return mirakana::RendererCommercialQualityEvidenceRow{
        .row_id = std::move(row_id),
        .kind = kind,
        .backend = backend,
        .status = RendererCommercialQualityEvidenceStatus::ready,
        .reviewed = true,
        .ready = true,
        .host_gated = false,
        .dependency_gated = false,
        .host_validation_recipe_id = recipe_id,
        .package_counter_id = "renderer_commercial_quality.row_ready",
        .request_native_handle_access = false,
        .request_cross_backend_inference = false,
        .request_external_engine_parity = false,
        .request_subjective_quality_claim = false,
        .request_crash_upload = false,
        .external_engine_code_used = false,
        .external_engine_sample_used = false,
        .external_engine_asset_used = false,
        .external_engine_trademark_used = false,
        .external_engine_compatibility_claim = false,
        .approved_external_material = false,
        .third_party_notices_complete = true,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::RendererCommercialQualityCloseoutDesc make_ready_desc() {
    return mirakana::RendererCommercialQualityCloseoutDesc{
        .backend_parity = make_backend_parity_plan(),
        .quality_matrix = make_quality_matrix_plan(),
        .vfx_profiling = make_vfx_profiling_plan(),
        .evidence_rows =
            {
                make_ready_row("metal.memory_profiling", RendererCommercialQualityEvidenceKind::metal_memory_profiling,
                               mirakana::rhi::BackendKind::metal, 1U),
                make_ready_row("package.visible_3d", RendererCommercialQualityEvidenceKind::visible_3d_package,
                               mirakana::rhi::BackendKind::null, 2U),
                make_ready_row("package.runtime_ui", RendererCommercialQualityEvidenceKind::runtime_ui_package,
                               mirakana::rhi::BackendKind::null, 3U),
                make_ready_row("package.environment", RendererCommercialQualityEvidenceKind::environment_package,
                               mirakana::rhi::BackendKind::null, 4U),
                make_ready_row("package.generated_game", RendererCommercialQualityEvidenceKind::generated_game_package,
                               mirakana::rhi::BackendKind::null, 5U),
                make_ready_row("claim_control.no_native_handles", RendererCommercialQualityEvidenceKind::claim_control,
                               mirakana::rhi::BackendKind::null, 6U),
                make_ready_row("clean_room.source_review", RendererCommercialQualityEvidenceKind::clean_room,
                               mirakana::rhi::BackendKind::null, 7U),
            },
        .row_budget = 32U,
        .seed = 404U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::RendererCommercialQualityCloseoutPlan& plan,
                                           RendererCommercialQualityCloseoutDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

void erase_kind(std::vector<mirakana::RendererCommercialQualityEvidenceRow>& rows,
                RendererCommercialQualityEvidenceKind kind) {
    std::erase_if(rows, [kind](const auto& row) { return row.kind == kind; });
}

} // namespace

MK_TEST("renderer commercial quality closeout is ready only when every selected evidence family is ready") {
    const auto plan = mirakana::plan_renderer_commercial_quality_closeout(make_ready_desc());

    MK_REQUIRE(plan.status == RendererCommercialQualityCloseoutStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.renderer_backend_parity_ready);
    MK_REQUIRE(plan.renderer_metal_broad_readiness);
    MK_REQUIRE(plan.renderer_broad_quality_ready);
    MK_REQUIRE(plan.renderer_commercial_readiness);
    MK_REQUIRE(plan.clean_room_source_review_ready);
    MK_REQUIRE(plan.third_party_notices_complete);
    MK_REQUIRE(plan.native_handle_access_count == 0U);
    MK_REQUIRE(plan.cross_backend_inference_count == 0U);
    MK_REQUIRE(plan.external_engine_parity_count == 0U);
    MK_REQUIRE(plan.ready_row_count == 7U);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("renderer commercial quality closeout keeps missing Metal backend parity host gated") {
    auto desc = make_ready_desc();
    desc.backend_parity.metal_parity_ready = false;

    const auto plan = mirakana::plan_renderer_commercial_quality_closeout(desc);

    MK_REQUIRE(plan.status == RendererCommercialQualityCloseoutStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(!plan.renderer_backend_parity_ready);
    MK_REQUIRE(!plan.renderer_metal_broad_readiness);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_backend_parity) == 1U);
}

MK_TEST("renderer commercial quality closeout keeps missing Metal quality matrix host gated") {
    auto desc = make_ready_desc();
    desc.quality_matrix.status = mirakana::RendererQualityMatrixStatus::host_evidence_required;
    desc.quality_matrix.metal_quality_matrix_ready = false;
    desc.quality_matrix.general_renderer_quality_ready = false;

    const auto plan = mirakana::plan_renderer_commercial_quality_closeout(desc);

    MK_REQUIRE(plan.status == RendererCommercialQualityCloseoutStatus::host_evidence_required);
    MK_REQUIRE(!plan.renderer_broad_quality_ready);
    MK_REQUIRE(!plan.renderer_metal_broad_readiness);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_quality_matrix) == 1U);
}

MK_TEST("renderer commercial quality closeout requires dedicated Metal memory profiling evidence") {
    auto desc = make_ready_desc();
    erase_kind(desc.evidence_rows, RendererCommercialQualityEvidenceKind::metal_memory_profiling);

    const auto plan = mirakana::plan_renderer_commercial_quality_closeout(desc);

    MK_REQUIRE(plan.status == RendererCommercialQualityCloseoutStatus::host_evidence_required);
    MK_REQUIRE(!plan.renderer_metal_broad_readiness);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(diagnostic_count(
                   plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_metal_memory_profiling_evidence) ==
               1U);
}

MK_TEST("renderer commercial quality closeout requires every selected package row") {
    auto desc = make_ready_desc();
    erase_kind(desc.evidence_rows, RendererCommercialQualityEvidenceKind::generated_game_package);

    const auto plan = mirakana::plan_renderer_commercial_quality_closeout(desc);

    MK_REQUIRE(plan.status == RendererCommercialQualityCloseoutStatus::host_evidence_required);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(!plan.generated_game_package_ready);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_required_package_row) ==
               1U);
}

MK_TEST("renderer commercial quality closeout rejects cross backend Metal memory proof transfer") {
    auto desc = make_ready_desc();
    for (auto& row : desc.evidence_rows) {
        if (row.kind == RendererCommercialQualityEvidenceKind::metal_memory_profiling) {
            row.backend = mirakana::rhi::BackendKind::d3d12;
            row.request_cross_backend_inference = true;
        }
    }

    const auto plan = mirakana::plan_renderer_commercial_quality_closeout(desc);

    MK_REQUIRE(plan.status == RendererCommercialQualityCloseoutStatus::invalid_request);
    MK_REQUIRE(!plan.renderer_metal_broad_readiness);
    MK_REQUIRE(plan.cross_backend_inference_count == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::cross_backend_inference) >= 1U);
}

MK_TEST("renderer commercial quality closeout rejects unsafe native capture and subjective claims") {
    auto desc = make_ready_desc();
    desc.evidence_rows[1].request_native_handle_access = true;
    desc.evidence_rows[2].request_crash_upload = true;
    desc.evidence_rows[3].request_subjective_quality_claim = true;
    desc.evidence_rows[4].host_validation_recipe_id = "unreviewed-renderer-recipe";

    const auto plan = mirakana::plan_renderer_commercial_quality_closeout(desc);

    MK_REQUIRE(plan.status == RendererCommercialQualityCloseoutStatus::invalid_request);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(plan.native_handle_access_count == 1U);
    MK_REQUIRE(
        diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::unsupported_native_handle_claim) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::unsupported_crash_upload) == 1U);
    MK_REQUIRE(diagnostic_count(
                   plan, RendererCommercialQualityCloseoutDiagnosticCode::unsupported_subjective_quality_claim) == 1U);
    MK_REQUIRE(diagnostic_count(
                   plan, RendererCommercialQualityCloseoutDiagnosticCode::unreviewed_host_validation_recipe) == 1U);
}

MK_TEST(
    "renderer commercial quality closeout rejects external engine code assets trademarks and compatibility claims") {
    auto desc = make_ready_desc();
    desc.evidence_rows[6].external_engine_code_used = true;
    desc.evidence_rows[6].external_engine_sample_used = true;
    desc.evidence_rows[6].external_engine_asset_used = true;
    desc.evidence_rows[6].external_engine_trademark_used = true;
    desc.evidence_rows[6].external_engine_compatibility_claim = true;

    const auto plan = mirakana::plan_renderer_commercial_quality_closeout(desc);

    MK_REQUIRE(plan.status == RendererCommercialQualityCloseoutStatus::invalid_request);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(plan.external_engine_code_used_count == 1U);
    MK_REQUIRE(plan.external_engine_sample_used_count == 1U);
    MK_REQUIRE(plan.external_engine_asset_used_count == 1U);
    MK_REQUIRE(plan.external_engine_trademark_used_count == 1U);
    MK_REQUIRE(plan.external_engine_compatibility_claim_count == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_code_used) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_sample_used) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_asset_used) ==
               1U);
    MK_REQUIRE(
        diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_trademark_used) == 1U);
    MK_REQUIRE(diagnostic_count(
                   plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_compatibility_claim) == 1U);
}

MK_TEST("renderer commercial quality closeout requires notices for approved external material") {
    auto desc = make_ready_desc();
    desc.evidence_rows[6].approved_external_material = true;
    desc.evidence_rows[6].third_party_notices_complete = false;

    const auto plan = mirakana::plan_renderer_commercial_quality_closeout(desc);

    MK_REQUIRE(plan.status == RendererCommercialQualityCloseoutStatus::invalid_request);
    MK_REQUIRE(!plan.third_party_notices_complete);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_third_party_notices) ==
               1U);
}

MK_TEST("renderer commercial quality closeout replay hash changes with accepted row details") {
    const auto first = mirakana::plan_renderer_commercial_quality_closeout(make_ready_desc());

    auto desc = make_ready_desc();
    desc.evidence_rows[1].package_counter_id = "renderer_commercial_quality.visible_3d_package_v2";
    const auto second = mirakana::plan_renderer_commercial_quality_closeout(desc);

    MK_REQUIRE(first.status == RendererCommercialQualityCloseoutStatus::ready);
    MK_REQUIRE(second.status == RendererCommercialQualityCloseoutStatus::ready);
    MK_REQUIRE(first.replay_hash != 0U);
    MK_REQUIRE(second.replay_hash != 0U);
    MK_REQUIRE(first.replay_hash != second.replay_hash);
}

int main() {
    return mirakana::test::run_all();
}
