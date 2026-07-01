// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/tools/cooked_animation_workflow_review.hpp"

#include <vector>

namespace {

using mirakana::CookedAnimationWorkflowDiagnosticCode;
using mirakana::CookedAnimationWorkflowEvidenceKind;
using mirakana::CookedAnimationWorkflowEvidenceRow;
using mirakana::CookedAnimationWorkflowReviewStatus;

[[nodiscard]] CookedAnimationWorkflowEvidenceRow make_base_row(CookedAnimationWorkflowEvidenceKind kind) {
    return CookedAnimationWorkflowEvidenceRow{
        .kind = kind,
        .row_id = "cooked_animation.workflow.row",
        .official_source_id = "mirakanai-cooked-animation-workflow",
        .reviewed = true,
    };
}

[[nodiscard]] CookedAnimationWorkflowEvidenceRow make_spec_row() {
    auto row = make_base_row(CookedAnimationWorkflowEvidenceKind::gltf_animation_spec);
    row.row_id = "cooked_animation.workflow.gltf_spec";
    row.official_source_id = "khronos-gltf-2.0-spec-animation";
    row.gltf_20_animation_spec_reviewed = true;
    row.gltf_translation_rotation_scale_paths_reviewed = true;
    row.gltf_sampler_interpolation_modes_reviewed = true;
    row.gltf_input_time_accessor_min_max_reviewed = true;
    row.gltf_quaternion_xyzw_rotation_reviewed = true;
    row.gltf_clamped_time_range_reviewed = true;
    row.gltf_duplicate_target_rejection_reviewed = true;
    row.gltf_morph_weights_separate_path_reviewed = true;
    return row;
}

[[nodiscard]] CookedAnimationWorkflowEvidenceRow make_import_row() {
    auto row = make_base_row(CookedAnimationWorkflowEvidenceKind::gltf_quaternion_trs_import);
    row.row_id = "cooked_animation.workflow.gltf_quaternion_trs_import";
    row.official_source_id = "mirakanai-gltf-node-animation-import";
    row.explicit_mesh_preset_required = true;
    row.coordinate_normalization_reviewed = true;
    row.unit_quaternion_validation_reviewed = true;
    row.unsupported_interpolation_fails_closed = true;
    row.no_runtime_gltf_parser_required = true;
    return row;
}

[[nodiscard]] CookedAnimationWorkflowEvidenceRow make_source_document_row() {
    auto row = make_base_row(CookedAnimationWorkflowEvidenceKind::first_party_source_document);
    row.row_id = "cooked_animation.workflow.source_document";
    row.official_source_id = "mirakanai-animation-quaternion-clip-source-v1";
    row.animation_quaternion_clip_source_v1_reviewed = true;
    row.source_document_validation_reviewed = true;
    row.track_target_joint_rows_reviewed = true;
    return row;
}

[[nodiscard]] CookedAnimationWorkflowEvidenceRow make_cooked_payload_row() {
    auto row = make_base_row(CookedAnimationWorkflowEvidenceKind::cooked_payload);
    row.row_id = "cooked_animation.workflow.cooked_payload";
    row.official_source_id = "mirakanai-cooked-animation-quaternion-clip-v1";
    row.cooked_animation_quaternion_clip_v1_reviewed = true;
    row.asset_kind_animation_quaternion_clip_reviewed = true;
    row.deterministic_f32_payload_reviewed = true;
    return row;
}

[[nodiscard]] CookedAnimationWorkflowEvidenceRow make_runtime_payload_row() {
    auto row = make_base_row(CookedAnimationWorkflowEvidenceKind::runtime_package_payload);
    row.row_id = "cooked_animation.workflow.runtime_payload";
    row.official_source_id = "mirakanai-runtime-animation-quaternion-clip-payload";
    row.runtime_payload_reader_reviewed = true;
    row.runtime_package_record_kind_reviewed = true;
    row.no_runtime_gltf_parser_required = true;
    return row;
}

[[nodiscard]] CookedAnimationWorkflowEvidenceRow make_package_smoke_row() {
    auto row = make_base_row(CookedAnimationWorkflowEvidenceKind::generated_package_smoke);
    row.row_id = "cooked_animation.workflow.generated_package_smoke";
    row.official_source_id = "mirakanai-generated-3d-quaternion-package-smoke";
    row.generated_desktop_3d_package_smoke_reviewed = true;
    row.pose_sampling_counters_reviewed = true;
    row.deterministic_replay_counter_reviewed = true;
    return row;
}

[[nodiscard]] CookedAnimationWorkflowEvidenceRow make_legal_row() {
    auto row = make_base_row(CookedAnimationWorkflowEvidenceKind::clean_room_legal_boundary);
    row.row_id = "cooked_animation.workflow.clean_room_legal_boundary";
    row.official_source_id = "mirakanai-clean-room-legal-boundary";
    row.source_provenance_reviewed = true;
    row.external_engine_non_claims_reviewed = true;
    row.no_copied_external_material_reviewed = true;
    row.legal_approval_non_claim_reviewed = true;
    return row;
}

[[nodiscard]] std::vector<CookedAnimationWorkflowEvidenceRow> make_ready_rows() {
    return {
        make_spec_row(),           make_import_row(),          make_source_document_row(),
        make_cooked_payload_row(), make_runtime_payload_row(), make_package_smoke_row(),
        make_legal_row(),
    };
}

[[nodiscard]] bool has_code(const mirakana::CookedAnimationWorkflowReviewResult& result,
                            CookedAnimationWorkflowDiagnosticCode code) noexcept {
    return mirakana::has_cooked_animation_workflow_diagnostic(result, code);
}

} // namespace

MK_TEST("cooked animation workflow review accepts selected quaternion clip evidence") {
    const auto rows = make_ready_rows();
    const auto result = mirakana::review_cooked_animation_asset_workflow({.rows = rows});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == CookedAnimationWorkflowReviewStatus::ready);
    MK_REQUIRE(result.total_rows == 7U);
    MK_REQUIRE(result.reviewed_rows == 7U);
    MK_REQUIRE(result.ready_rows == 7U);
    MK_REQUIRE(result.gltf_spec_rows == 1U);
    MK_REQUIRE(result.import_rows == 1U);
    MK_REQUIRE(result.source_document_rows == 1U);
    MK_REQUIRE(result.cooked_payload_rows == 1U);
    MK_REQUIRE(result.runtime_payload_rows == 1U);
    MK_REQUIRE(result.package_smoke_rows == 1U);
    MK_REQUIRE(result.legal_boundary_rows == 1U);
    MK_REQUIRE(result.cooked_animation_asset_workflow_review_ready);
    MK_REQUIRE(result.cooked_animation_quaternion_clip_workflow_ready);
    MK_REQUIRE(result.cooked_animation_gltf_animation_spec_reviewed);
    MK_REQUIRE(result.cooked_animation_gltf_quaternion_import_reviewed);
    MK_REQUIRE(result.cooked_animation_source_document_ready);
    MK_REQUIRE(result.cooked_animation_cooked_payload_ready);
    MK_REQUIRE(result.cooked_animation_runtime_payload_ready);
    MK_REQUIRE(result.cooked_animation_generated_package_smoke_ready);
    MK_REQUIRE(result.cooked_animation_clean_room_legal_boundary_ready);
    MK_REQUIRE(!result.cooked_animation_runtime_gltf_parsing_ready);
    MK_REQUIRE(!result.cooked_animation_animation_graph_ready);
    MK_REQUIRE(!result.cooked_animation_retargeting_ready);
    MK_REQUIRE(!result.cooked_animation_renderer_rhi_execution_ready);
    MK_REQUIRE(!result.cooked_animation_broad_skeletal_animation_ready);
    MK_REQUIRE(!result.cooked_animation_native_handles_exposed);
    MK_REQUIRE(!result.cooked_animation_external_engine_compatibility);
    MK_REQUIRE(!result.cooked_animation_legal_approval);
}

MK_TEST("cooked animation workflow review requires official gltf spec obligations") {
    auto rows = make_ready_rows();
    rows[0].gltf_sampler_interpolation_modes_reviewed = false;
    rows[0].gltf_input_time_accessor_min_max_reviewed = false;
    rows[0].gltf_quaternion_xyzw_rotation_reviewed = false;
    rows[0].gltf_duplicate_target_rejection_reviewed = false;

    const auto result = mirakana::review_cooked_animation_asset_workflow({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == CookedAnimationWorkflowReviewStatus::review_required);
    MK_REQUIRE(result.review_required_rows == 1U);
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_gltf_sampler_interpolation_review));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_gltf_time_accessor_review));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_gltf_quaternion_review));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_duplicate_target_review));
}

MK_TEST("cooked animation workflow review requires exact evidence identities") {
    auto rows = make_ready_rows();
    rows[0].official_source_id = "mirakanai-cooked-animation-workflow";
    rows[1].row_id = "cooked_animation.workflow.placeholder";

    const auto result = mirakana::review_cooked_animation_asset_workflow({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == CookedAnimationWorkflowReviewStatus::review_required);
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::unexpected_official_source));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::invalid_row_id));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_gltf_spec_row));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_import_row));
}

MK_TEST("cooked animation workflow review rejects duplicate and blank evidence identities") {
    auto rows = make_ready_rows();
    rows.push_back(make_spec_row());
    rows[2].row_id = "   ";
    rows[3].official_source_id = "\t";

    const auto result = mirakana::review_cooked_animation_asset_workflow({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == CookedAnimationWorkflowReviewStatus::review_required);
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::duplicate_row_id));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_row_id));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_official_source));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_source_document_row));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_cooked_payload_row));
}

MK_TEST("cooked animation workflow review requires all selected workflow rows") {
    const std::vector<CookedAnimationWorkflowEvidenceRow> rows{make_spec_row(), make_import_row()};

    const auto result = mirakana::review_cooked_animation_asset_workflow({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == CookedAnimationWorkflowReviewStatus::review_required);
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_source_document_row));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_cooked_payload_row));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_runtime_payload_row));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_package_smoke_row));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_legal_boundary_row));
}

MK_TEST("cooked animation workflow review blocks unsupported broad and external claims") {
    auto rows = make_ready_rows();
    rows[1].claims_runtime_gltf_parsing_ready = true;
    rows[1].claims_animation_graph_ready = true;
    rows[1].claims_retargeting_ready = true;
    rows[1].claims_renderer_rhi_execution_ready = true;
    rows[1].claims_broad_skeletal_animation_ready = true;
    rows[1].native_handles_exposed = true;
    rows[1].claims_copied_external_engine_code_assets_or_shaders = true;
    rows[1].claims_unity_unreal_godot_compatibility = true;
    rows[1].claims_legal_approval = true;

    const auto result = mirakana::review_cooked_animation_asset_workflow({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == CookedAnimationWorkflowReviewStatus::blocked);
    MK_REQUIRE(result.blocked_rows == 1U);
    MK_REQUIRE(result.cooked_animation_native_handles_exposed);
    MK_REQUIRE(result.cooked_animation_legal_approval);
    MK_REQUIRE(!result.cooked_animation_runtime_gltf_parsing_ready);
    MK_REQUIRE(!result.cooked_animation_animation_graph_ready);
    MK_REQUIRE(!result.cooked_animation_retargeting_ready);
    MK_REQUIRE(!result.cooked_animation_renderer_rhi_execution_ready);
    MK_REQUIRE(!result.cooked_animation_broad_skeletal_animation_ready);
    MK_REQUIRE(!result.cooked_animation_external_engine_compatibility);
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::runtime_gltf_parsing_claim_not_allowed));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::animation_graph_claim_not_allowed));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::retargeting_claim_not_allowed));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::renderer_rhi_execution_claim_not_allowed));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::broad_skeletal_animation_claim_not_allowed));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::native_handle_exposure));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::copied_external_material_claim_not_allowed));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::external_engine_claim_not_allowed));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::legal_approval_claim_not_allowed));
}

MK_TEST("cooked animation workflow review requires clean room legal boundary evidence") {
    auto rows = make_ready_rows();
    rows[6].source_provenance_reviewed = false;
    rows[6].external_engine_non_claims_reviewed = false;
    rows[6].no_copied_external_material_reviewed = false;
    rows[6].legal_approval_non_claim_reviewed = false;

    const auto result = mirakana::review_cooked_animation_asset_workflow({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == CookedAnimationWorkflowReviewStatus::review_required);
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_source_provenance_review));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_external_engine_non_claim_review));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_no_copied_external_material_review));
    MK_REQUIRE(has_code(result, CookedAnimationWorkflowDiagnosticCode::missing_legal_approval_non_claim_review));
}

int main() {
    return mirakana::test::run_all();
}
